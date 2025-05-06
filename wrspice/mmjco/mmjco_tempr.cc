//=================================================================
// Class to compute superconductor properties as a function of
// temperature.  Presently, this will compute the gap parameter.
//
// Stephen R. Whiteley, wrcad.com,  Synopsys, Inc.  7/7/2021
//=================================================================
// Released under Apache License, Version 2.0.
//   http://www.apache.org/licenses/LICENSE-2.0
//
// References:
// Tinkham, "Introduction to Superconductivity".
// https://physics.stackexchange.com/questions/54200/superconducting-gap-
//  temperature-dependence-how-to-calculate-this-integral
// https://en.wikipedia.org/wiki/Romberg%27s_method
// http://www.knowledgedoor.com/2/elements_handbook/debye_temperature.html

// Define to use GSL for integration, otherwise use the romberg method
// below.  The romberg function seems to do as well.
//
//#define USE_GSL

#include "mmjco_tempr.h"


#ifndef USE_GSL

//#define DEBUG
#ifdef DEBUG
namespace {
    void dump_row(size_t i, double *R)
    {
       printf("R[%2zu] = ", i);
       for (size_t j = 0; j <= i; ++j){
          printf("%f ", R[j]);
       }
       printf("\n");
    }
}
#endif

#define MAX_STEPS 20

// Romberg numerical integration method from Wikipedia.
//
// f            Function to integrate.
// ptr          Void pointer passed to f.
// a,b          Lower, upper limits.
// max_steps    Maximum iteration levels.
// acc          Desired accuracy.
//
double romberg(double (*f)(double, void*), void *ptr, double a, double b,
    size_t max_steps, double acc)
{
    if (max_steps > MAX_STEPS)
        max_steps = MAX_STEPS;
    double R1[MAX_STEPS], R2[MAX_STEPS]; // Buffers.
    double *Rp = &R1[0], *Rc = &R2[0];   // Rp is prev row, Rc is current row.
    double h = (b-a);                    // Step size.
    Rp[0] = (f(a, ptr) + f(b, ptr))*h*0.5; // First trapezoidal step.

#ifdef DEBUG
    dump_row(0, Rp);
#endif

    for (size_t i = 1; i < max_steps; ++i) {
        h /= 2.0;
        double c = 0.0;
        size_t ep = 1 << (i-1); // 2^(n-1)
        for (size_t j = 1; j <= ep; ++j) {
            c += f(a+(2*j-1)*h, ptr);
        }
        Rc[0] = h*c + 0.5*Rp[0]; // R(i,0)

        for (size_t j = 1; j <= i; ++j) {
            double n_k = pow(4, j);
            Rc[j] = (n_k*Rc[j-1] - Rp[j-1])/(n_k-1); // Compute R(i,j).
        }

#ifdef DEBUG
        // Dump ith row of R, R[i,i] is the best estimate so far
        dump_row(i, Rc);
#endif

        if (i > 1 && fabs(Rp[i-1]-Rc[i]) < acc) {
            return (Rc[i-1]);
        }

        // Swap Rn and Rc as we only need the last row.
        double *rt = Rp;
        Rp = Rc;
        Rc = rt;
    }
    return (Rp[max_steps-1]); // Return our best guess.
}

#endif  // notdef USE_GSL


namespace {
    double intfunc(double x, void *ptr)
    {
        mmjco_tempr::tprms *tp = (mmjco_tempr::tprms*)ptr;
        double a = sqrt(tp->del*tp->del + x*x);
        double b = 0.5/(BOLTZ*tp->T);
        return (tanh(b*a)/a);
    }

#ifdef USE_GSL
    double func(mmjco_tempr::tprms *tp, double dbe, 
            gsl_integration_workspace *ws)
    {
        gsl_function gsl_f;
        gsl_f.function = intfunc;
        gsl_f.params = tp;

        double intret;
        double abserr;
        int er = gsl_integration_qag(&gsl_f, 0.0, dbe, 0.0, 1e-7, 200,
            GSL_INTEG_GAUSS21, ws, &intret, &abserr);
        if (er) {
            printf("%s\n", gsl_strerror(er));
            return (0.0);
        }
        return (intret);
#else
    double func(mmjco_tempr::tprms *tp, double dbe)
    {
        return (romberg(intfunc, tp, 0.0, dbe, MAX_STEPS, 1e-6));
    }
#endif  // USE_GSL
}


double
mmjco_tempr::gap_parameter(double T)
{
    if (T >= (t_tc - 0.01))
        return (0);
    if (T < 0.01)
        T = 0.01;

    tprms tp;
    tp.T = T;
    double xdel = 1e-3;
    double dd = 1e-4;

    int ll = 0;
    for (;;) {
        tp.del = xdel * ECHG;

#ifdef USE_GSL
        double f = func(&tp, t_dbe, t_ws) - t_eta;
#else
        double f = func(&tp, t_dbe) - t_eta;
#endif
        if (fabs(f) < 1e-8*t_eta)
            break;

        if (f > 0.0) {
            if (ll == -1)
                dd /= 2.0;
            xdel += dd;
            ll = 1;
        }
        else {
            if (ll == 1)
                dd /= 2.0;
            xdel -= dd;
            ll = -1;
        }
    }
    return (tp.del/ECHG);
}


#ifdef STAND_ALONE

// Calculate gap parameter for Nb as a function of temperature.
// No args:  print Del over range T=[0,Tc].
// One arg:  print Del for the given T.
// Two args: print Del for range of two temperatures given.
//
int main(int argc, char *argv[])
{
    mmjco_tempr t;

    if (argc == 2) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "-?") || *argv[1]== 'h' ||
                *argv[1] == '?') {
            puts(
                "\nCalculate gap parameter for Nb as a function of "
                "temperature.\n"
                "No args:  print Del over range T=[0,Tc].\n"
                "One arg:  print Del for the given T.\n"
                "Two args: print Del for range of two temperatures given.\n");
            return (0);
        }
        double T = atof(argv[1]);
        if (T >= 0.0 && T <= TC_NB) {
            double del = t.gap_parameter(T);
            printf("T= %.4e Del= %.4e\n", T, del);
            return (0);
        }
    }
    else if (argc == 3) {
        double Tmin = atof(argv[1]);
        double Tmax = atof(argv[2]);
        if (Tmin >= 0.0 && Tmin <= TC_NB && Tmax >= 0.0 && Tmax <= TC_NB) {
            if (Tmin > Tmax) {
                double tmp = Tmin;
                Tmin = Tmax;
                Tmax = tmp;
            }
            for (double T = Tmin; T <= Tmax; T += 0.1) {
                double del = t.gap_parameter(T);
                printf("T= %.4e Del= %.4e\n", T, del);
            }
            return (0);
        }
    }
    else {
        for (double T = 0.0; T <= TC_NB; T += 0.1) {
            double del = t.gap_parameter(T);
#define CHECK_FIT_FUNC
#ifdef CHECK_FIT_FUNC
            double fit = 1.3994e-3*tanh(1.74*sqrt(TC_NB/(T+1e-3) - 1.0));
            double pcterr = 100*2*fabs(del - fit)/(del + fit);
            printf("T= %.4e Del= %.4e Fit= %.4e PctErr= %.4e\n", T, del, fit,
                pcterr);
#else
            printf("T= %.4e Del= %.4e\n", T, del);
#endif
        }
        return (0);
    }
    printf("Bad input: T must be >= 0 and <= %.2f\n", TC_NB);
    return (1);
}

#endif

