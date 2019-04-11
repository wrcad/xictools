#include "device.h"
#include "ifdata.h"


namespace {
}

#ifdef notdef

class TJMModel : public ElementModel
{
    TJMModel(int i, int j, int eindex)
        {
            tjm_crit_current    = 1.0;
            tjm_beta            = 1.0;
            tjm_wvg             = 2.6/0.63;
            tjm_wvrat           = 0.6;
            tjm_wrrat           = 0.1;
            tjm_narray          = 6;
            tjm_i               = i;
            tjm_j               = j;
            tjm_eindex          = eindex;
        }


    void InitModel(int ind_ii, int ind_ij, int ind_ji, int ind_jj)
    void SetParameters(double *params):
    void Admitance0(double *matrix):
    void Admitance1(double *matrix, ModelContext *ctx);
    void Admitance2(double *matrix, ModelContext *ctx);
    void RHSCurrent(double *vec, ModelContext *ctx);
    void Values(ModelContext *ctx);

private:
    double          *tjm_node_phases;
    double          *tjm_node_voltages;
    double          *tjm_node_dvoltages;
    double          *tjm_elem_currents;
    double          *tjm_elem_phases;
    double          *tjm_elem_voltages;
    int             *tjm_elem_n;
    unsigned long   *tjm_elem_inc;
    unsigned long   *tjm_elem_dec;

    double          tjm_crit_current;

// model
    double          tjm_beta;
    double          tjm_wvg;
    double          tjm_wvrat;
    double          tjm_wrrat;
    double          tjm_sgw;
    IFcomplex       tjm_A[MaxTJMCoeffArraySize];
    IFcomplex       tjm_B[MaxTJMCoeffArraySize];
    IFcomplex       tjm_P[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fc[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fs[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fcdt[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fsdt[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fcprev[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fsprev[MaxTJMCoeffArraySize];
    IFcomplex       tjm_alpha0[MaxTJMCoeffArraySize];
    IFcomplex       tjm_beta0[MaxTJMCoeffArraySize];
    IFcomplex       tjm_alpha1[MaxTJMCoeffArraySize];
    double          tjm_sinphi2;
    double          tjm_cosphi2;
    int             tjm_narray;

// instance
    int             tjm_i;
    int             tjm_j;
    int             tjm_eindex;
    int             tjm_ind_ii;
    int             tjm_ind_ij;
    int             tjm_ind_ji;
    int             tjm_ind_jj;
};
    
void
TJModel::InitModel(ind_ii, ind_ij, ind_ji, ind_jj)
{
    tjm_ind_ii = ind_ii
    tjm_ind_ij = ind_ij
    tjm_ind_ji = ind_ji
    tjm_ind_jj = ind_jj

    cnp.ndarray[FLOAT_t, ndim=1, mode="c"] view_phase = psglobals.NodePhase
    cnp.ndarray[FLOAT_t, ndim=1, mode="c"] view_voltage = psglobals.NodeVoltage
    cnp.ndarray[FLOAT_t, ndim=1, mode="c"] view_dvoltage = psglobals.NodeDVoltage
    cnp.ndarray[FLOAT_t, ndim=1, mode="c"] view_ephase = psglobals.ElementPhase
    cnp.ndarray[FLOAT_t, ndim=1, mode="c"] view_evoltage = psglobals.ElementVoltage
    cnp.ndarray[FLOAT_t, ndim=1, mode="c"] view_ecurrent = psglobals.ElementCurrent
    cnp.ndarray[INT_t, ndim=1, mode="c"] view_en = psglobals.ElementN
    cnp.ndarray[UINT8_t, ndim=1, mode="c"] view_einc = psglobals.ElementInc
    cnp.ndarray[UINT8_t, ndim=1, mode="c"] view_edec = psglobals.ElementDec

    tjm_node_phases     = <double*>view_phase.data
    tjm_node_voltages   = <double*>view_voltage.data
    tjm_node_dvoltages  = <double*>view_dvoltage.data
    tjm_elem_phases     = <double*>view_ephase.data
    tjm_elem_voltages   = <double*>view_evoltage.data
    tjm_elem_currents   = <double*>view_ecurrent.data
    tjm_elem_n          = <int*>view_en.data
    tjm_elem_inc        = <UINT8_t*>view_einc.data
    tjm_elem_dec        = <UINT8_t*>view_edec.data
}
    
void
TJModel::SetParameters(double *params)
{
    double x = 0.0

    #cdef char* model;
    #mname = params[0].encode('UTF-8');
    #model = mname;
    #printf("TJM model %s\n", model);

    tjm_crit_current    = params[1];
    tjm_beta            = params[2];
    tjm_wvg             = params[3];
    tjm_wvrat           = params[4];
    tjm_wrrat           = params[5];

    TJMcoeffSet *cs = TJMcoeffSet::getTJMcoeffSet(mname);
    if (!cs) {
        // ERROR
    }
    tjm_narray = cs->cfs_size;
    tjm_A = new complex[tjm_narry];
    tjm_B = new complex[tjm_narry];
    tjm_P = new complex[tjm_narry];
    for (int i = 0; i < tjm_narry; i++) {
        tjm_A[i] = cs->cfs_A[i];
        tjm_B[i] = cs->cfs_B[i];
        tjm_P[i] = cs->cfs_P[i]*tjm_wvg*0.5;
        x = x - (tjm_A[i] / tjm_P[i]).real;
    }
/*
    if params[0] in psconfig.tjm_models:
        [A,B,P] = psconfig.tjm_models[params[0]]
        self.narray = len(A)
        for i in range(0, self.narray):
            self.A[i] = COMPLEX_t(A[i].real, A[i].imag)
            self.B[i] = COMPLEX_t(B[i].real, B[i].imag)
            self.P[i] = COMPLEX_t(P[i].real, P[i].imag)*self.wvg*0.5
            x = x - (self.A[i] / self.P[i]).real()

    else:
        raise Exception("Unknown TJM model: ".encode('UTF-8') + params[0].encode('UTF-8'))
*/

    tjm_sgw             = 1.0 / (tjm_wvg * tjm_wvrat);

//printf("X=%lf SGW=%lf\n", x, tjm_sgw);

    for (int i = 0; i < tjm_narray; i++) {
        tjm_A[i] = tjm_A[i] / (-1.0 * x);
        tjm_B[i] = tjm_B[i] * tjm_wvg * (tjm_wrrat - 1.0) / (2.0 * tjm_wvrat);
    }
}

            
void
TJModel::Admitance0(double *matrix)
{
    // This method called during initialization of simulation.
    cIFcomplex neg_one(-1.0, 0.0);

    // Fc(0) and Fs(0)
    for (int  = 0; i < tjm_narray; i++) {
        tjm_Fcprev[i] = neg_one/tjm_P[i];
        tjm_Fsprev[i] = cIFcomplex(0.0, 0.0);
    }
}
    
void
TJModel::Admitance1(double *matrix, ModelContext* ctx)
{
    // This method is called on the first Newton iteration of the time step.
    double adm = tjm_crit_current*(tjm_sgw *
        (ctx.coeff_dt + tjm_sgw * tjm_beta * ctx.coeff_ddt));

    double dphase_prev;
    double dvoltage_prev;
    if (tjm_i == 0) {
        dphase_prev = -ctx.phases_prev[tjm_j-1];
        dvoltage_prev = -ctx.voltages_prev[tjm_j-1];
        matrix[tjm_ind_jj] += adm;
    }
    else if (tjm_j == 0) {
        dphase_prev = ctx.phases_prev[tjm_i-1];
        dvoltage_prev = ctx.voltages_prev[tjm_i-1];
        matrix[tjm_ind_ii] += adm;
    }
    else {
        dphase_prev = ctx.phases_prev[tjm_i-1] - ctx.phases_prev[tjm_j-1];
        dvoltage_prev = ctx.voltages_prev[tjm_i-1] - ctx.voltages_prev[tjm_j-1];
        matrix[tjm_ind_ii] += adm;
        matrix[tjm_ind_ij] -= adm;
        matrix[tjm_ind_ji] -= adm;
        matrix[tjm_ind_jj] += adm;
    }
    
    double sinphi2 = sin(dphase_prev/2.0);
    double cosphi2 = cos(dphase_prev/2.0);

    for (int i = 0; i < tjm_narray; i++) {
        double pq = tjm_P[i]*ctx.time_step;
        double epq = exp(pq);
        tjm_alpha0[i] = (epq*(pq*pq - 2.0) + pq*2.0 + 2.0)/(pq*pq*tjm_P[i]);
        tjm_beta0[i] = (epq*pq - epq*2.0 + pq + 2.0)/(pq*tjm_P[i]*tjm_P[i]);
        tjm_alpha1[i] = (epq*2.0 - 2.0 - pq*2.0 - pq*pq)/(pq*pq*tjm_P[i]);
        //tjm_alpha0[i] = (epq * (pq - 1.0) + 1.0)/(pq * tjm_P[i]);
        //tjm_alpha1[i] = (epq - pq - 1.0)/(pq * tjm_P[i]);

        // Part of Fc(t+dt) and Fs(t+dt) which do not depends on phase(t+dt)
        tjm_Fcdt[i] = epq*tjm_Fcprev[i] + tjm_alpha0[i]*cosphi2 -
            tjm_beta0[i]*sinphi2*dvoltage_prev/2.0;
        tjm_Fsdt[i] = epq*tjm_Fsprev[i] + tjm_alpha0[i]*sinphi2 +
            tjm_beta0[i]*cosphi2*dvoltage_prev/2.0;
        //tjm_Fcdt[i] = epq*tjm_Fcprev[i] + tjm_alpha0[i]*cosphi2;
        //tjm_Fsdt[i] = epq*tjm_Fsprev[i] + tjm_alpha0[i]*sinphi2;
    }
}
        
void
TJModel::Admitance2(double *matrix, ModelContext* ctx)
{
    // This method called on each Newton step.
//    cIFcomplex Sum1(0.0, 0.0);
//    cIFcomplex Sum2(0.0, 0.0);

    double dphase;
    if (tjm_i == 0)
        dphase = -ctx.phases[tjm_j-1];
    elif (tjm_j == 0)
        dphase = ctx.phases[tjm_i-1];
    else
        dphase = ctx.phases[tjm_i-1] - ctx.phases[tjm_j-1];
    
    tjm_sinphi2 = sin(dphase/2.0);
    tjm_cosphi2 = cos(dphase/2.0);
    double cosphi = cos(dphase);

    cIFcomplex FcAnSum(0.0, 0.0);
    cIFcomplex FsAnSum(0.0, 0.0);
    for (int i = 0; i < tjm_narray; i++) {
        tjm_Fc[i] = tjm_Fcdt[i] + tjm_alpha1[i]*tjm_cosphi2;
        tjm_Fs[i] = tjm_Fsdt[i] + tjm_alpha1[i]*tjm_sinphi2;
        FcAnSum = FcAnSum + tjm_A[i]*tjm_Fc[i];
        FsAnSum = FsAnSum + tjm_A[i]*tjm_Fs[i];
    }

    double dcurr = -tjm_crit_current*
        (FcAnSum.real*tjm_cosphi2 - FsAnSum.real*tjm_sinphi2);

    //printf("TJM dcurr=%lf\n", dcurr);

    if (tjm_i == 0)
        matrix[tjm_ind_jj] += dcurr;
    elif (tjm_j == 0)
        matrix[tjm_ind_ii] += dcurr;
    else {
        matrix[tjm_ind_ii] += dcurr;
        matrix[tjm_ind_ij] -= dcurr;
        matrix[tjm_ind_ji] -= dcurr;
        matrix[tjm_ind_jj] += dcurr;
    }
}
        
void
TJModel::RHSCurrent(double *vec, ModelContext* ctx)
{
    double dphase;
    double dvoltage;
    double ddvoltage;
    if (tjm_i == 0) {
        dphase = -ctx.phases[tjm_j-1];
        dvoltage = -ctx.voltages[tjm_j-1];
        ddvoltage = -ctx.voltages_deriv[tjm_j-1];
    }
    elif (tjm_j == 0) {
        dphase = ctx.phases[tjm_i-1];
        dvoltage = ctx.voltages[tjm_i-1];
        ddvoltage = ctx.voltages_deriv[tjm_i-1];
    }
    else {
        dphase = ctx.phases[tjm_i-1] - ctx.phases[tjm_j-1];
        dvoltage = ctx.voltages[tjm_i-1] - ctx.voltages[tjm_j-1];
        ddvoltage = ctx.voltages_deriv[tjm_i-1] - ctx.voltages_deriv[tjm_j-1];
    }

    tjm_sinphi2 = sin(dphase/2.0);
    tjm_cosphi2 = cos(dphase/2.0);   

    cIFcomplex FcSum(0.0, 0.0);
    cIFcomplex FsSum(0.0, 0.0);
    for (int i = 0; i < tjm_narray; i++) {
        FcSum = FcSum + (tjm_A[i] + tjm_B[i])*tjm_Fc[i];
        FsSum = FsSum + (tjm_A[i] - tjm_B[i])*tjm_Fs[i];
    }

    double curr = FcSum.real*tjm_sinphi2 + FsSum.real*tjm_cosphi2;

    if (tjm_i == 0) {
        vec[tjm_j-1] += tjm_crit_current*(-curr +
            tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
    }
    elif (tjm_j == 0) {
        vec[tjm_i-1] -= tjm_crit_current*(-curr +
            tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
    }
    else {
        double full_current = tjm_crit_current*(-curr +
            tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
        vec[tjm_i-1] -= full_current;
        vec[tjm_j-1] += full_current;
    }

    //printf("TJM curr=%lf\n", tjm_jcurr)
}


// This method called when time step is accepted.
void
TJModel::Values(ModelContext* ctx)
{
    double dphase;
    double dvoltage;
    double ddvoltage;
    if (tjm_i == 0) {
        dphase = -tjm_node_phases[tjm_j-1];
        dvoltage = -tjm_node_voltages[tjm_j-1];
        ddvoltage = -tjm_node_dvoltages[tjm_j-1];
    }
    elif (tjm_j == 0) {
        dphase = tjm_node_phases[tjm_i-1];
        dvoltage = tjm_node_voltages[tjm_i-1];
        ddvoltage = tjm_node_dvoltages[tjm_i-1];
    }
    else {
        dphase = tjm_node_phases[tjm_i-1] - tjm_node_phases[tjm_j-1];
        dvoltage = tjm_node_voltages[tjm_i-1] - tjm_node_voltages[tjm_j-1];
        ddvoltage = tjm_node_dvoltages[tjm_i-1] - tjm_node_dvoltages[tjm_j-1];
    }

    if (tjm_i == 0)
        dphase = -tjm_node_phases[tjm_j-1];
    else if (tjm_j == 0)
        dphase = tjm_node_phases[tjm_i-1];
    else
        dphase = tjm_node_phases[tjm_i-1] - tjm_node_phases[tjm_j-1];

    double sinphi2 = sin(dphase/2.0);
    double cosphi2 = cos(dphase/2.0);

    cIFcomplex FcSum(0.0, 0.0);
    cIFcomplex FsSum(0.0, 0.0);
    for (int i = 0; i < tjm_narray; i++) {
        tjm_Fc[i] = tjm_Fcdt[i] + tjm_alpha1[i]*tjm_cosphi2;
        tjm_Fs[i] = tjm_Fsdt[i] + tjm_alpha1[i]*tjm_sinphi2;
        tjm_Fcprev[i] = tjm_Fc[i];
        tjm_Fsprev[i] = tjm_Fs[i];
        FcSum = FcSum + (tjm_A[i] + tjm_B[i])*tjm_Fc[i];
        FsSum = FsSum + (tjm_A[i] - tjm_B[i])*tjm_Fs[i];
    }

    double curr = FcSum.real*sinphi2 + FsSum.real*cosphi2;

    if (tjm_i == 0) {
        tjm_elem_currents[tjm_eindex] = -tjm_crit_current*
            (-curr + tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
        tjm_elem_phases[tjm_eindex] = -tjm_node_phases[tjm_j-1];
        tjm_elem_voltages[tjm_eindex] = tjm_.node_voltages[tjm_j-1];
    }
    else if (tjm_j == 0) {
        tjm_elem_currents[tjm_eindex] = tjm_crit_current*
            (-curr + tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
        tjm_elem_phases[tjm_eindex] = tjm_node_phases[tjm_i-1];
        tjm_elem_voltages[tjm_eindex] = tjm_node_voltages[tjm_i-1];
    }
    else {
        tjm_elem_currents[tjm_eindex] = tjm_crit_current*
            (-curr + tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
        tjm_elem_phases[tjm_eindex] =
            (tjm_node_phases[tjm_i-1] - tjm_node_phases[tjm_j-1]);
        tjm_elem_voltages[tjm_eindex] =
            (tjm_node_voltages[tjm_i-1] - tjm_node_voltages[tjm_j-1]);
    }

    NQuanta(tjm_elem_phases[tjm_eindex], 
        &tjm_elem_n[tjm_eindex], &tjm_elem_inc[tjm_eindex], 
        &tjm_elem_dec[tjm_eindex], ctx.histeresis);
}

#endif

