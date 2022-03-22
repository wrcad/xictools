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

#include "mmjco.h"
#ifdef WITH_CMINPACK
#include "cminpack.h"
#else
#include "mpfit.h"
#endif


const char *mmjco::mm_version = "0.9";

//#define DEBUG

namespace {
    bool erhdlr(int er, const char *text)
    {
        if (er) {
            fprintf(stderr, "%s %s\n", gsl_strerror(er), text);
            return (true);
        }
        return (false);
    }
}


//-----------------------------------------------------------------------------
// Function description:
//     Calculation of tunnel current amplitudes 
//     from BCS expressions of Larkin and Ovchinnikov [1]
//     as given by formulas (A5)-(A8) in Appendix of Ref.[2].
//     Tunnel current amplitudes are normalized to Vg/R.
// Input:
//     T -- temperature in K
//     Delta1 -- superconducting gap in meV
//     Delta2 -- superconducting gap in meV
// References:
//     [1] A. I. Larkin and Yu. N. Ovchinnikov, Sov. Phys. JETP 24,
//         1035 (1967).
//     [2] D. R. Gulevich, V. P. Koshelets, and F. V. Kusmartsev,
//         Phys. Rev. B 96, 024515 (2017).
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
// Function description: 
//     Smoothing Riedel peaks of bare BCS tunnel current amplitudes
//     (TCAs).  We apply the smoothing procedure as described in Ref. 
//     [1-4].  Smoothing parameter 'dsm' used here is equal to the
//     parameter $\delta$ in [1-4].  Note, that the gaps mm_d1 and mm_d2
//     are interchanged as compared to Ref.[1]:  in our calculations we
//     use 0<mm_d1<mm_d2 as in Ref.[2-4].

// Output:
//     Jpair_smooth(x)
//     Jqp_smooth(x)
// References:
//    [1] A. B. Zorin, I. O. Kulik, K. K. Likharev, and J. R. Schrieffer, 
//        Sov. J. Low Temp. Phys. 5, 537 (1979).
//    [2] D. R. Gulevich, V. P. Koshelets, and F. V. Kusmartsev, Phys.
//        Rev. B 96, 024515 (2017).
//    [3] D. R. Gulevich, V. P. Koshelets, F. V. Kusmartsev,
//        arXiv:1709.04052 (2017).
//    [4] D. R. Gulevich, L. V. Filippenko, V. P. Koshelets,
//        arXiv:1809.01642 (2018).
//-----------------------------------------------------------------------------
//
mmjco::mmjco(double T, double Delta1, double Delta2, double sm)
{
    // Define normalized gaps mm_d1 and mm_d2 (0 < mm_d1 <= mm_d2).
    mm_d1 = minimum(Delta1, Delta2)/(Delta1+Delta2);
    mm_d2 = 1.0 - mm_d1;
    double Vg = 1e-3*(Delta1+Delta2);  // Gap voltage in volts.
    mm_b  = MM_ECHG*Vg/(2*MM_BOLTZ*T); // Parameter alpha in Ref.
                                       // PRB 96, 024515 (2017)

    mm_dd1 = mm_d1*mm_d1;
    mm_dd2 = mm_d2*mm_d2;

    mm_limit = 200;
    mm_key = 2;
    mm_ws = gsl_integration_workspace_alloc(mm_limit);
    mm_tbl = gsl_integration_qaws_table_alloc(-0.5, -0.5, 0, 0);

    // from Python (scipy)
    mm_epsabs = 1e-6;
    mm_epsrel = 1e-4;

    // Smoothing.
    mm_expb = exp(mm_b);
    mm_dsm = sm;
    mm_ip0 = Jpair(0.0).real();
    mm_d21=mm_d2-mm_d1;

    // The smoothing procedure is different for symmetric junction
    // (mm_d1=mm_d2) due to the difference mm_d2-mm_d1 appearing
    // in the denominator:  the limit mm_d2-mm_d1->0 is taken
    // analytically to avoid numerical errors. 

    if (mm_d21 < 0.001) {
        mm_symj = true;
        printf("# Symmetric junction assumed (Delta1=Delta2)\n");
    }
    else {
        mm_symj = false;
        printf("# Asymmetric junction (Delta1!=Delta2)\n");
    }
}


mmjco::~mmjco()
{
    gsl_integration_workspace_free(mm_ws);
    gsl_integration_qaws_table_free(mm_tbl);
}
    

//---------------------------------------------------
//          Define tunnel current amplitudes
//---------------------------------------------------
// In the calculations we assume x>0 without losing 
// the generality (x<0 are obtained by symmetry).

//------- Re pair current -------
//
double
mmjco::Repair(double x)
{
    gsl_function gsl_f;
    gsl_f.params = this;
    double abserr;
    mm_arg = x;

    int er;
    double part1 = 0.0;
    if (x+mm_d1 > mm_d2) {
        gsl_f.function = &mmjco::re_p_i1;
        er = gsl_integration_qag(&gsl_f, maximum(x-mm_d1,mm_d2), x+mm_d1,
            mm_epsabs, mm_epsrel, mm_limit, mm_key, mm_ws, &part1, &abserr);
        if (erhdlr(er, "Repair 1"))
            return (0.0);
    }

    double part2 = 0.0;
    if (-x-mm_d2 < -mm_d1) {
        gsl_f.function = &mmjco::re_p_i2;
        er = gsl_integration_qag(&gsl_f, -x-mm_d2, minimum(-x+mm_d2,-mm_d1),
            mm_epsabs, mm_epsrel, mm_limit, mm_key, mm_ws, &part2, &abserr);
        if (erhdlr(er, "Repair 2"))
            return (0.0);
    }

    double part3 = 0.0;
    if (-x+mm_d2 > mm_d1) {
        gsl_f.function = &mmjco::re_p_i2;
        er = gsl_integration_qag(&gsl_f, mm_d1, -x+mm_d2, mm_epsabs, mm_epsrel,
            mm_limit, mm_key, mm_ws, &part3, &abserr);
        if (erhdlr(er, "Repair 3"))
            return (0.0);
    }

    return (0.5*mm_d1*mm_d2*(part1 + part2 + part3));
}


//------- Im pair current -------
//
double
mmjco::Impair(double x)
{
    gsl_function gsl_f;
    gsl_f.function = &mmjco::im_p_i;
    gsl_f.params = this;
    double abserr;
    mm_arg = x;

    int er;
    double part1 = 0.0;
    er = gsl_integration_qagil(&gsl_f, minimum(-mm_d1,-x-mm_d2),
        mm_epsabs, mm_epsrel, mm_limit, mm_ws, &part1, &abserr);
    if (erhdlr(er, "Impair 1"))
        return (0.0);
    double part2 = 0.0;
    er = gsl_integration_qagiu(&gsl_f, maximum(mm_d1,-x+mm_d2),
        mm_epsabs, mm_epsrel, mm_limit, mm_ws, &part2, &abserr);
    if (erhdlr(er, "Impair 2"))
        return (0.0);

    double part3 = 0.0;
    if (-x+mm_d2 < -mm_d1) {
        er = gsl_integration_qag(&gsl_f, -x+mm_d2, -mm_d1,
            mm_epsabs, mm_epsrel, mm_limit, mm_key, mm_ws, &part3, &abserr);
        if (erhdlr(er, "Impair 3"))
            return (0.0);
    }

    return (0.5*mm_d1*mm_d2*(part1 + part2 + part3));
}
    

//------- Re qp current -------
//
double
mmjco::Reqp(double x)
{
    gsl_function gsl_f;
    gsl_f.params = this;
    double abserr;
    mm_arg = x;

    int er;
    double part1a = 0.0;
    if (x-mm_d2 < -mm_d1) {
        gsl_f.function = &mmjco::re_q_i1;
        er = gsl_integration_qag(&gsl_f, x-mm_d2, -mm_d1, mm_epsabs, mm_epsrel,
            mm_limit, mm_key, mm_ws, &part1a, &abserr);
        if (erhdlr(er, "Reqp 1"))
            return (0.0);
    }

    double part1b = 0.0;
    if (x-mm_d2 < mm_d1) {
        gsl_f.function = &mmjco::re_q_i1b1x2;
        er = gsl_integration_qaws(&gsl_f, mm_d1, x+mm_d2, mm_tbl,
            mm_epsabs, mm_epsrel, mm_limit, mm_ws, &part1b, &abserr);
        if (erhdlr(er, "Reqp 2"))
            return (0.0);
    }
    else {
        gsl_f.function = &mmjco::re_q_i1bx2x2;
        er = gsl_integration_qaws(&gsl_f, x-mm_d2, x+mm_d2, mm_tbl,
            mm_epsabs, mm_epsrel, mm_limit, mm_ws, &part1b, &abserr);
        if (erhdlr(er, "Reqp 3"))
            return (0.0);
    }

    double part2 = 0.0;
    if (-x-mm_d1 < -mm_d2) {
        if (-mm_d2 < -x+mm_d1) {
            gsl_f.function = &mmjco::re_q_i2x12;
            er = gsl_integration_qaws(&gsl_f, -x-mm_d1, -mm_d2, mm_tbl,
                mm_epsabs, mm_epsrel, mm_limit, mm_ws, &part2, &abserr);
            if (erhdlr(er, "Reqp 4"))
                return (0.0);
        }
        else {
            gsl_f.function = &mmjco::re_q_i2x1x1;
            er = gsl_integration_qaws(&gsl_f, -x-mm_d1, -x+mm_d1, mm_tbl,
                mm_epsabs, mm_epsrel, mm_limit, mm_ws, &part2, &abserr);
            if (erhdlr(er, "Reqp 5"))
                return (0.0);
        }
    }

    return (-0.5*(part1a + part1b + part2));
}


//------- Im qp current -------
//
double
mmjco::Imqp(double x)
{
    gsl_function gsl_f;
    gsl_f.function = &mmjco::im_q_i;
    gsl_f.params = this;
    double abserr;
    mm_arg = x;

    double part1 = 0.0;
    int er = gsl_integration_qagil(&gsl_f, minimum(-mm_d2,-x-mm_d1),
        mm_epsabs, mm_epsrel, mm_limit, mm_ws, &part1, &abserr);
    if (erhdlr(er, "Imqp 1"))
        return (0.0);
    double part2 = 0.0;
    if (-x+mm_d1 < -mm_d2) {
        er = gsl_integration_qag(&gsl_f, -x+mm_d1, -mm_d2, mm_epsabs, mm_epsrel,
            mm_limit, mm_key, mm_ws, &part2, &abserr);
        if (erhdlr(er, "Imqp 2"))
            return (0.0);
    }
    double part3 = 0.0;
    er = gsl_integration_qagiu(&gsl_f, mm_d2, mm_epsabs, mm_epsrel,
        mm_limit, mm_ws, &part3, &abserr);
    if (erhdlr(er, "Imqp 3"))
        return (0);

    return (0.5*(part1 + part2 + part3));
}
// End of mmjco functions.


//
// Fitting starts here ---------
//


//-------------------------------------------------------------------------
// Function description: 
//     Fitting by sum of complex exponentials, see Ref. [1,2].
// Input:
//     x -- array of frequency points prepared for fitting
//     Jpair_data -- complex array prepared for fitting of pair TCAs
//     Jqp_data -- complex array prepared for fitting of quasiparticle TCAs
//     use_nterms --- number of complex exponentials
//     thr --- ratio of absolute and relative tolerances (equal to
//             tau_a/tau_r in Ref.[2])
// References:
//    [1] A. A. Odintsov, V. K. Semenov and A. B. Zorin, IEEE Trans. Magn. 23,
//      763 (1987).
//    [2] D. R. Gulevich, V. P. Koshelets, and F. V. Kusmartsev, Phys. Rev. B
//      96, 024515 (2017).
//-------------------------------------------------------------------------
//
void
mmjco_fit::new_fit_parameters(const double *x,
    const complex<double> *Jpair_data, const complex<double> *Jqp_data,
    int lenx, int use_nterms, double thr)
{
    if (use_nterms > MAX_NTERMS)
        use_nterms = MAX_NTERMS;

    complex<double> p[use_nterms];
    complex<double> A[use_nterms];
    complex<double> B[use_nterms];
    p[0] = complex<double>(-1.0, 0.0);
    A[0] = complex<double>(0.0, 0.0);
    B[0] = complex<double>(0.0, 0.0);
    double xnew = 1.0;

    double *xd = new double[4*lenx];
    double *px = new double[6*use_nterms];

#ifndef WITH_CMINPACK
    mp_config_struct mpcfg;
    mpcfg.ftol = 1e-4;
    mpcfg.xtol = 0.0;
    mpcfg.gtol = 0.0;
    mpcfg.epsfcn = 0.0;
    mpcfg.stepfactor = 0.0;
    mpcfg.covtol = 0.0;
    mpcfg.maxiter = 1000;
    mpcfg.maxfev = 0;
    mpcfg.nprint = 0;
    mpcfg.douserscale = 0;
    mpcfg.nofinitecheck = 0;
#endif

    int nterms = 1;
    while (nterms < use_nterms) {

        p[nterms] = complex<double>(-1.0, 1.0*xnew);
        A[nterms] = complex<double>(0.0, 0.0);
        B[nterms] = complex<double>(0.0, 0.0);
        nterms++;

        printf("# nterms = %d with new term at frequency %f. Calculating...\n",
            nterms, xnew);

        for (int i = 0; i < nterms; i++) {
            px[2*i] = p[i].real();
            px[2*i+1] = p[i].imag();
            px[2*(i+nterms)] = A[i].real();
            px[2*(i+nterms)+1] = A[i].imag();
            px[2*(i+2*nterms)] = B[i].real();
            px[2*(i+2*nterms)+1] = B[i].imag();
        }

        mm_adata ad;
        ad.x = x;
        ad.thr = thr;
        ad.Jpair_data = Jpair_data;
        ad.Jqp_data = Jqp_data;

#ifdef WITH_CMINPACK
        double tol = 1e-4;
        int M = 4*lenx;
        int N = 6*nterms;
        int iwa[N];
        int lwa = M*N + 5*N + M;
        double wa[lwa];
        int info = lmdif1(func, &ad, 4*lenx, 6*nterms, px, xd, tol,
            iwa, wa, lwa);
        if (info == 5) {
            // Ditn't converge, keep on chooglin'
            info = lmdif1(func, &ad, 4*lenx, 6*nterms, px, xd, tol,
                iwa, wa, lwa);
        }
#else
        mp_result result;
        memset(&result, 0, sizeof(result));

        result.resid = xd;  // Need to copy this back.
        int info = mpfit(func, 4*lenx, 6*nterms, px, 0, &mpcfg, &ad, &result);
#endif

        for (int i = 0; i < nterms; i++) {
            p[i].real(rep(px[2*i]));
            p[i].imag(px[2*i+1]);
            A[i].real(px[2*(nterms+i)]);
            A[i].imag(px[2*(nterms+i)+1]);
            B[i].real(px[2*(2*nterms+i)]);
            B[i].imag(px[2*(2*nterms+i)+1]);
        }

        int ix = -1;
        double ax = 0.0;
        double tot = 0.0;
        for (int i = 0; i < lenx; i++) {
            for (int j = 0; j < 4; j++) {
                double a = xd[i+j*lenx];
                tot += a;
                if (a > ax) {
                    ax = a;
                    ix = i;
                }
            }
        }
        xnew = x[ix];
        tot /= lenx;
#ifdef WITH_CMINPACK
        printf("# info %d, residual %g\n", info, tot);
#else
        printf("# info %d, residual %g, norm %g, iters %d\n", info, tot,
            result.bestnorm, result.niter);
#endif

#ifdef DEBUG
        const char *fmt = " %10.6f, %10.6f";
        printf("p=");
        for (int i= 0; i < nterms; i++)
            printf(fmt, p[i].real(), p[i].imag());
        printf("\n");
        printf("A=");
        for (int i= 0; i < nterms; i++)
            printf(fmt, A[i].real(), A[i].imag());
        printf("\n");
        printf("B=");
        for (int i= 0; i < nterms; i++)
            printf(fmt, B[i].real(), B[i].imag());
        printf("\n");
#endif
    }
    delete [] xd;
    delete [] px;

    mmf_nterms = use_nterms;
    delete [] mmf_pAB;
    mmf_pAB = new complex<double>[3*use_nterms];
    for (int i = 0; i < use_nterms; i++) {
        mmf_pAB[i] = p[i];
        mmf_pAB[nterms+i] = A[i];
        mmf_pAB[2*nterms+i] = B[i];
    }
}


// Save TCA fit parameters p,A,B to file.  The destination can be set
// by passing fp, in which case the data string or file name will be
// added as a leading text string.
//
void
mmjco_fit::save_fit_parameters(const char *filename, FILE *fp, const char *data)
{
    if (!mmf_pAB || !mmf_nterms) {
        fprintf(stderr, "Error: no saved parameter set.\n");
        return;
    }
    complex<double> *A = mmf_pAB + mmf_nterms;
    complex<double> *B = A + mmf_nterms;
    bool closefp = false;
    if (!fp) {
        fp = fopen(filename, "w");
        if (!fp) {
            fprintf(stderr, "Error: unable to open file %s.\n", filename);
            return;
        }
        closefp = true;
    }
    else if (data) {
        fprintf(fp, "%s\n", data);
    }
    else {
        const char *f = strrchr(filename, '/');
        if (f)
            f++;
        else
            f = filename;
        fprintf(fp, "%s\n", f);
    }
    for (int i = 0; i < mmf_nterms; i++) {
        fprintf(fp, "%12.5e,%12.5e,%12.5e,%12.5e,%12.5e,%12.5e\n",
            mmf_pAB[i].real(), mmf_pAB[i].imag(), A[i].real(), A[i].imag(),
            B[i].real(), B[i].imag());
    }
    if (closefp) {
        fclose(fp);
        printf("Parameters saved to file %s.\n", filename);
    }
}


// Load TCA fit parameters from file.
//
void
mmjco_fit::load_fit_parameters(const char *filename, FILE *fp)
{
    complex<double> p[MAX_NTERMS];
    complex<double> A[MAX_NTERMS];
    complex<double> B[MAX_NTERMS];
    int ix = 0;
    char buf[256];
    while (fgets(buf, 255, fp) != 0) {
        double pr, pi, Ar, Ai, Br, Bi;
        int ns = sscanf(buf, "%lf %lf %lf %lf %lf %lf", &pr, &pi, &Ar, &Ai,
            &Br, &Bi);
        if (ns == 6) {
            p[ix].real(pr);
            p[ix].imag(pi);
            A[ix].real(Ar);
            A[ix].imag(Ai);
            B[ix].real(Br);
            B[ix].imag(Bi);
            ix++;
        }
    }
    fclose(fp);
    if (ix > 1) {
        mmf_nterms = ix;
        complex<double> *pAB = new complex<double>[3*ix];
        for (int i = 0; i < ix; i++) {
            pAB[i] = p[i];
            pAB[ix+i] = A[i];
            pAB[2*ix+i] = B[i];
        }
        delete [] mmf_pAB;
        mmf_pAB = pAB;
        printf("Loaded TCA parameters from file %s.\n", filename);
    }
}


// Load TCA fit parameters from array.
//
void
mmjco_fit::load_fit_parameters(const double *data, int nterms)
{
    complex<double> p[MAX_NTERMS];
    complex<double> A[MAX_NTERMS];
    complex<double> B[MAX_NTERMS];
    const double *dp = data;
    for (int i = 0; i < nterms; i++) {
        p[i].real(dp[0]);
        p[i].imag(dp[1]);
        A[i].real(dp[2]);
        A[i].imag(dp[3]);
        B[i].real(dp[4]);
        B[i].imag(dp[5]);
        dp += 6;
    }
    mmf_nterms = nterms;
    complex<double> *pAB = new complex<double>[3*nterms];
    for (int i = 0; i < nterms; i++) {
        pAB[i] = p[i];
        pAB[nterms+i] = A[i];
        pAB[2*nterms+i] = B[i];
    }
    delete [] mmf_pAB;
    mmf_pAB = pAB;
    printf("Loaded TCA parameters from data.\n");
}


// Return fitted TCAs from saved parameters for values in x.
//
void
mmjco_fit::tca_fit(const double *x, int xsz,
    complex<double> **Jpair_modelptr, complex<double> **Jqp_modelptr)
{
    if (!mmf_pAB || !mmf_nterms) {
        fprintf(stderr, "Error: no saved fit parameters.\n");
        *Jpair_modelptr = 0;
        *Jqp_modelptr = 0;
        return;
    }
    *Jpair_modelptr = modelJpair(mmf_pAB, mmf_nterms, x, xsz);
    *Jqp_modelptr = modelJqp(mmf_pAB, mmf_nterms, x, xsz);
    printf("TCA model values computed.\n");
}


// Compute a cost function for the model relative to data, using thr.
//  sum pair/qp/real/imag  [abs(model[i]-data[]) / max(thr, data[i])]
//
// Lower values imply better fit.
//
double
mmjco_fit::residual(const complex<double> *Jpair_model,
    const complex<double> *Jqp_model, const complex<double> *Jpair_data,
    const complex<double> *Jqp_data, int xsz, double thr)
{
    if (!Jpair_model || !Jqp_model) {
        fprintf(stderr, "Warning: no model TCA data, skipping residual.\n");
        return (0.0);
    }
    if (!Jpair_data || !Jqp_data) {
        fprintf(stderr, "Warning: no raw TCA data, skipping residual.\n");
        return (0.0);
    }
    double tot = 0.0;
    for (int i = 0; i < xsz; i++) {
        tot += Drel(Jpair_model[i].real(), Jpair_data[i].real(), thr);
        tot += Drel(Jpair_model[i].imag(), Jpair_data[i].imag(), thr);
        tot += Drel(Jqp_model[i].real(), Jqp_data[i].real(), thr);
        tot += Drel(Jqp_model[i].imag(), Jqp_data[i].imag(), thr);
    }
    return (tot/xsz);
}


// Jpair model.
//
complex<double> *
mmjco_fit::modelJpair(const complex<double> *cpars, int nterms,
    const double *w, int lenw)
{
#define zeta(n) cpars[n].real()
#define eta(n)  cpars[n].imag()
#define rea(n)  cpars[nterms+n].real()
#define ima(n)  cpars[nterms+n].imag()
    complex<double> *sum = new complex<double>[lenw];
    for (int k = 0; k < lenw; k++) {
        sum[k].real(0.0);
        sum[k].imag(0.0);
        for (int n = 0; n < nterms; n++) {
            double numr = rea(n)*rep(zeta(n)) + ima(n)*eta(n);
            double numi = -w[k]*rea(n);
            double denr = rep(zeta(n))*rep(zeta(n)) + eta(n)*eta(n) - w[k]*w[k];
            double deni = -2.0*w[k]*rep(zeta(n));
            double d = denr*denr + deni*deni;
            sum[k].real(sum[k].real() - (numr*denr + numi*deni)/d);
            sum[k].imag(sum[k].imag() - (numi*denr - numr*deni)/d);
        }
    }
    return (sum);
#undef zeta
#undef eta
#undef rea
#undef ima
}


// Jqp model.
//
complex<double> *
mmjco_fit::modelJqp(const complex<double> *cpars, int nterms,
    const double *w, int lenw)
{
#define zeta(n) cpars[n].real()
#define eta(n)  cpars[n].imag()
#define reb(n)  cpars[2*nterms+n].real()
#define imb(n)  cpars[2*nterms+n].imag()
    complex<double> *sum = new complex<double>[lenw];
    for (int k = 0; k < lenw; k++) {
        sum[k].real(0.0);
        sum[k].imag(0.0);
        for (int n = 0; n < nterms; n++) {
            double numr = reb(n)*rep(zeta(n)) + imb(n)*eta(n);
            double numi = w[k]*reb(n);
            double denr = rep(zeta(n))*rep(zeta(n)) + eta(n)*eta(n) - w[k]*w[k];
            double deni = 2.0*w[k]*rep(zeta(n));
            double d = denr*denr + deni*deni;
            sum[k].real(sum[k].real() - (numr*denr + numi*deni)/d);
            sum[k].imag(sum[k].imag() - (numi*denr - numr*deni)/d);
        }
        sum[k].imag(sum[k].imag() + w[k]);
    }
    return (sum);
#undef zeta
#undef eta
#undef reb
#undef imb
}


// Residual for least square optimization.
//
int
#ifdef WITH_CMINPACK
mmjco_fit::func(void *adata, int xsz, int nprms, const double *p, double *hx,
    int iflag)
#else
mmjco_fit::func(int xsz, int nprms, double *p, double *hx, double**,
    void *adata)
#endif
{
    xsz /= 4;
    mm_adata *a = (mm_adata*)adata;
    complex<double> *cpars = ccombine(p, nprms);
    int nterms = nprms/6;

    complex<double> *Jpair_model = modelJpair(cpars, nterms, a->x, xsz);
    complex<double> *Jqp_model = modelJqp(cpars, nterms, a->x, xsz);

#ifdef DEBUG
    for (int i = 0; i < nterms*3; i++)
        printf("cpars %.6e %.6e\n", cpars[i].real(), cpars[i].imag());
    for (int i = 0; i < 5; i++)
        printf("m %.6e %.6e %.6e %.6e\n", Jpair_model[i].real(),
        Jpair_model[i].imag(), Jqp_model[i].real(), Jqp_model[i].imag());
#endif
    delete [] cpars;

    for (int i = 0; i < xsz; i++) {
        hx[i] = Drel(Jpair_model[i].real(), a->Jpair_data[i].real(), a->thr);
        hx[xsz+i] = Drel(Jpair_model[i].imag(), a->Jpair_data[i].imag(),a->thr);
        hx[2*xsz+i] = Drel(Jqp_model[i].real(), a->Jqp_data[i].real(), a->thr);
        hx[3*xsz+i] = Drel(Jqp_model[i].imag(), a->Jqp_data[i].imag(), a->thr);
    }
    delete [] Jpair_model;
    delete [] Jqp_model;
    return (0);
}
// End of mmjco_fit functons;

