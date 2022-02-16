//==============================================
//---------- by Dmitry R. Gulevich -------------
//--------- drgulevich@corp.ifmo.ru ------------
//--- ITMO University, St Petersburg, Russia ---
//==============================================
// Translation to C++ and augmentation
// S. R. Whiteley, wrcad.com,  Synopsys, Inc
//==============================================
//
// License:  GNU General Public License Version 3, 29 June 2007.

#ifndef MMJCO_H
#define MMJCO_H

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <complex>
#include <cmath>
#include <gsl/gsl_math.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_integration.h>

// Uncomment this to use the cminpack library for the least-squares
// fitting, otherwise we will use the cmpfit library provided. 
//#define WITH_CMINPACK

#define MM_ECHG 1.602176634e-19     // Electron charge (SI units).
#define MM_BOLTZ 1.3806226e-23      // Boltzmann constant (SI units).

using namespace std;

class mmjco
{
public:
    // Constructor/destructor.
    mmjco(double, double, double, double);
    ~mmjco();

    // Functions return values for arbitrary x (values x<0 obtained by
    // symmetry), not smoothed.

    complex<double> Jpair(double x)
        {
            double absx = maximum(fabs(x), 1e-5);
            return (complex<double>(Repair(absx), sign(x)*Impair(absx)));
        }

    complex<double> Jqp(double x)
        {
            double absx = maximum(fabs(x), 1e-5);
            return (complex<double>(Reqp(absx), sign(x)*Imqp(absx)));
        }

    // As above, but smoothed.

    complex<double> Jpair_smooth(double x)
        {
            return (Jpair(x) + Jpair_correction(x));
        }

    complex<double> Jqp_smooth(double x)
        {
            return (Jqp(x) + Jqp_correction(x));
        }

    // Utilities.

    static double sign(double x)
        {
            if (x > 0.0)
                return (1.0);
            if (x == 0.0)
                return (0.0);
            return (-1.0);
        }

    static double minimum(double x, double y) { return (x < y ? x : y); }
    static double maximum(double x, double y) { return (x > y ? x : y); }

    static const char *version()    { return (mm_version); }

private:
    double Repair(double);
    double Impair(double);
    double Reqp(double);
    double Imqp(double);

    double Repair_integrand_part1(double y, double x)	
        {
            return (tanh(mm_b*fabs(y))/
                ( sqrt(mm_dd1-(y-x)*(y-x)) * sqrt(y*y-mm_dd2) ));
        }

    double Repair_integrand_part2(double y, double x)
        {
            return (tanh(mm_b*fabs(y))/
                ( sqrt(y*y-mm_dd1) * sqrt(mm_dd2-(y+x)*(y+x)) ));
        }

    double Impair_integrand(double y, double x)
        {
            return (( tanh(mm_b*(y+x)) - tanh(mm_b*y) )*sign(y)*sign(y+x)/(
                sqrt(y*y-mm_dd1) * sqrt((y+x)*(y+x)-mm_dd2) ));
        }

    double Reqp_integrand_part1(double y, double x)
        {
            return (fabs(y)*tanh(mm_b*y)*(y-x)/( sqrt(y*y-mm_dd1) *
                sqrt(mm_dd2-(y-x)*(y-x)) ));
        }

    double Reqp_integrand_part1b_d1_xd2(double y, double x)
        {
            return (fabs(y)*tanh(mm_b*y)*(y-x)/sqrt((y+mm_d1)*(mm_d2+y-x)));
        }

    double Reqp_integrand_part1b_xd2_xd2(double y, double x)
        {
            return (fabs(y)*tanh(mm_b*y)*(y-x)/sqrt(y*y-mm_dd1));
        }

    double Reqp_integrand_part2_xd1_d2(double y, double x)
        {
            return (fabs(y)*tanh(mm_b*y)*(y+x)/sqrt((mm_d1-y-x)*(mm_d2-y)));
        }

    double Reqp_integrand_part2_xd1_xd1(double y, double x)
        {
            return (fabs(y)*tanh(mm_b*y)*(y+x)/sqrt(y*y-mm_dd2));
        }

    double Imqp_integrand(double y, double x)
        {
            return (( tanh(mm_b*(y+x))-tanh(mm_b*y) )*fabs(y)*fabs(y+x)/
                ( sqrt((y+x)*(y+x)-mm_dd1) * sqrt(y*y-mm_dd2) ));
        }

    // Passed to integration functions.

    static double re_p_i1(double x, void *ptr)
        {
            mmjco *m = (mmjco*)ptr; 
            return (m->Repair_integrand_part1(x, m->mm_arg));
        }

    static double re_p_i2(double x, void *ptr)
        {
            mmjco *m = (mmjco*)ptr; 
            return (m->Repair_integrand_part2(x, m->mm_arg));
        }

    static double im_p_i(double x, void *ptr)
        {
            mmjco *m = (mmjco*)ptr; 
            return (m->Impair_integrand(x, m->mm_arg));
        }

    static double re_q_i1(double x, void *ptr)
        {
            mmjco *m = (mmjco*)ptr; 
            return (m->Reqp_integrand_part1(x, m->mm_arg));
        }

    static double re_q_i1b1x2(double x, void *ptr)
        {
            mmjco *m = (mmjco*)ptr; 
            return (m->Reqp_integrand_part1b_d1_xd2(x, m->mm_arg));
        }

    static double re_q_i1bx2x2(double x, void *ptr)
        {
            mmjco *m = (mmjco*)ptr; 
            return (m->Reqp_integrand_part1b_xd2_xd2(x, m->mm_arg));
        }

    static double re_q_i2x12(double x, void *ptr)
        {
            mmjco *m = (mmjco*)ptr; 
            return (m->Reqp_integrand_part2_xd1_d2(x, m->mm_arg));
        }

    static double re_q_i2x1x1(double x, void *ptr)
        {
            mmjco *m = (mmjco*)ptr; 
            return (m->Reqp_integrand_part2_xd1_xd1(x, m->mm_arg));
        }

    static double im_q_i(double x, void *ptr)
        {
            mmjco *m = (mmjco*)ptr; 
            return (m->Imqp_integrand(x, m->mm_arg));
        }

    // Smoothing.

    complex<double> Jpair_correction(double x)
        {
            if (mm_symj) {
                double absx = maximum(fabs(x),1.e-5);
                return (complex<double>(dRe(absx),
                    sign(x)*(dIm(absx) + dIm_at_0(absx))));
            }
            else {
                double absx = maximum(fabs(x),1.e-5);
                return (complex<double>(dRe(absx) + dRe_minus(absx),
                    sign(x)*(dIm(absx) + dIm_minus(absx))));
            }
        }

    complex<double> Jqp_correction(double x)
        {

            if (mm_symj) {
                double absx = maximum(fabs(x),1.e-5);
                return (complex<double>(dRe(absx),
                    sign(x)*(-dIm(absx) + dIm_at_0(absx))));
            }
            else {
                double absx = maximum(fabs(x),1.e-5);
                return (complex<double>(dRe(absx) - dRe_minus(absx),
                    sign(x)*(-dIm(absx) + dIm_minus(absx))));
            }
        }

    // Smoothing for Repair, Reqp.
    double dRe(double x)
        {
            double sqpos = (x-1.0)*(x-1.0);
            double sqneg = (x+1.0)*(x+1.0);
            return (-x*(1.0/M_PI)*mm_ip0*0.5*log(
                ((sqpos+mm_dsm*mm_dsm)/sqpos)*(sqneg/(sqneg+mm_dsm*mm_dsm)) ));
        }

    // Smoothing for Repair, -Reqp at x=mm_d2-mm_d1. 
    double dRe_minus(double x)
        {
            return (M_PI*x*sqrt(mm_d1*mm_d2)*
                (tanh(mm_b*mm_d2)-tanh(mm_b*mm_d1)) *
                0.5*( (2.0/M_PI)*atan((x-mm_d21)/mm_dsm) 
                - sign(x-mm_d21) + (2.0/M_PI)*atan((x+mm_d21)/mm_dsm) -
                sign(x+mm_d21) ) / (4.0*mm_d21));
        }

    // Smoothing for Impair, Imqp at x=mm_d2-mm_d1.
    double dIm_minus(double x)
        {
            double square1 = (x-mm_d21)*(x-mm_d21);
            double square2 = (x+mm_d21)*(x+mm_d21);
            return (-x*sqrt(mm_d1*mm_d2)*(tanh(mm_b*mm_d2)-tanh(mm_b*mm_d1))*
                0.5*log((square1+mm_dsm*mm_dsm)*(square2+mm_dsm*mm_dsm)/
                (square1*square2)) / (4.0*mm_d21));
        }

    // Smoothing for Impair, -Imqp.
    double dIm(double x)
        {
            return (x*0.5*mm_ip0*((2.0/M_PI)*atan((1.0-x)/mm_dsm) -
                sign(1.0-x) + (2.0/M_PI)*atan((1.0+x)/mm_dsm) - sign(1.0+x)));
        }

    // Smoothing for Impair, Imqp at mm_d2-mm_d1=0.
    double dIm_at_0(double x)
        {
            double x2=x*x;
            return (-mm_b*x*mm_expb*0.5*log(
                (x2+mm_dsm*mm_dsm)/x2)/((mm_expb+1.0)*(mm_expb+1.0)));
        }

    static const char *mm_version;

    double mm_d1, mm_d2;
    double mm_d21;
    double mm_dd1, mm_dd2;
    double mm_b;
    double mm_arg;
    int mm_limit;
    int mm_key;
    double mm_epsabs;
    double mm_epsrel;
    gsl_integration_workspace *mm_ws;
    gsl_integration_qaws_table *mm_tbl;

    // Smoothing.
    double mm_expb;
    double mm_ip0;
    double mm_dsm;
    bool mm_symj;;
};


//
// Class to compute optimized TCA fitting parameters.
//

// Largest acceptable fitting table.
#define MAX_NTERMS 20

class mmjco_fit
{
public:
    struct mm_adata
    {
        const double *x;
        double thr;
        const complex<double> *Jpair_data;
        const complex<double> *Jqp_data;
    };

    mmjco_fit()
        {
            mmf_pAB = 0;
            mmf_nterms = 0;
        }

    ~mmjco_fit()
        {
            delete [] mmf_pAB;
        }

    void new_fit_parameters(const double*, const complex<double>*,
        const complex<double>*, int, int, double);
    void save_fit_parameters(const char*, FILE* = 0);
    void load_fit_parameters(const char*, FILE*);
    void load_fit_parameters(const double*, int);
    void tca_fit(const double*, int, complex<double>**, complex<double>**);
    double residual(const complex<double>*, const complex<double>*,
        const complex<double>*, const complex<double>*, int, double);

    const complex<double> *params()     const { return (mmf_pAB); }
    int numterms()                      const { return (mmf_nterms); }

private:
    static complex<double> *modelJpair(const complex<double>*, int,
        const double*, int);
    static complex<double> *modelJqp(const complex<double>*, int,
        const double*, int);

    static double rep(double zeta)     { return (-fabs(zeta)); }
    static double invrep(double xi)    { return (-xi); }

    // Relative difference with threshold thr.
    static double Drel(double X, double Xref, double thr)
        {
            double axref = fabs(Xref);
            if (thr > axref)
                axref = thr;
            return (fabs(X-Xref)/axref);
        }

    // Flatten complex array.
    static double *realimag(complex<double> *carray, int sz)
        {
            double *param_list = new double[2*sz];
            for (int i = 0; i < sz; i++) {
                param_list[i*2] = carray[i].real();
                param_list[i*2+1] = carray[i].imag();
            }
            return (param_list);
        }

    // Form complex array.
    static complex<double> *ccombine(const double *param_list, int plsz)
        {
            complex<double> *cpars = new complex<double>[plsz/2];
            for (int i = 0; i < plsz; i++) {
                int n = i/2;
                if (i & 1)
                    cpars[n].imag(param_list[i]);
                else
                    cpars[n].real(param_list[i]);
            }
            return (cpars);
        }

#ifdef WITH_CMINPACK
    static int func(void*, int, int, const double*, double*, int);
#else
    static int func(int, int, double*, double*, double**, void*);
#endif

    complex<double> *mmf_pAB;
    int mmf_nterms;
};

#endif // MMJCO_H

