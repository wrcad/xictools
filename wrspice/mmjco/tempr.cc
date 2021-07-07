//=================================================================
// Class to compute superconductor properties as a function of
// temperature.  Presently, this will compute the order parameter.
//
// Stephen R. Whiteley, wrcad.com,  Synopsys, Inc.  7/7/2021
//=================================================================
// Released under Apache License, Version 2.0.
//   http://www.apache.org/licenses/LICENSE-2.0
//
// References:
// Tinkham, "Introduction to Superconductiovity".
// https://physics.stackexchange.com/questions/54200/superconducting-gap-
//  temperature-dependence-how-to-calculate-this-integral
// http://www.knowledgedoor.com/2/elements_handbook/debye_temperature.html

#include "tempr.h"


namespace {
    double intfunc(double x, void *ptr)
    {
        tempr::tprms *tp = (tempr::tprms*)ptr;
        double a = sqrt(tp->del*tp->del + x*x);
        double b = 0.5/(BOLTZ*tp->T);
        return (tanh(b*a)/a);
    }

    double func(tempr::tprms *tp, double dbe, gsl_integration_workspace *ws)
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
    }
}


double
tempr::order_parameter(double T)
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

        double f = func(&tp, t_dbe, t_ws) - t_eta;
        if (fabs(f) < 1e-4)
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

// Calculate order parameter for Nb as a function of temperature.
// No args:  print Del over range T=[0,Tc].
// One arg:  print Del for the given T.
// Two args: print Del for range of two temperatures given.
//
int main(int argc, char *argv[])
{
    tempr t;

    if (argc == 2) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "-?") || *argv[1]== 'h' ||
                *argv[1] == '?') {
            puts(
                "\nCalculate order parameter for Nb as a function of "
                "temperature.\n"
                "No args:  print Del over range T=[0,Tc].\n"
                "One arg:  print Del for the given T.\n"
                "Two args: print Del for range of two temperatures given.\n");
            return (0);
        }
        double T = atof(argv[1]);
        if (T >= 0.0 && T <= TC_NB) {
            double del = t.order_parameter(T);
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
                double del = t.order_parameter(T);
                printf("T= %.4e Del= %.4e\n", T, del);
            }
            return (0);
        }
    }
    else {
        for (double T = 0.0; T <= TC_NB; T += 0.1) {
            double del = t.order_parameter(T);
            printf("T= %.4e Del= %.4e\n", T, del);
        }
        return (0);
    }
    printf("Bad input: T must be >= 0 and <= %.2f\n", TC_NB);
    return (1);
}

#endif

