
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef CIRCUIT_H
#define CIRCUIT_H


//
// Circuit and task control structures and related definitions.
//

#define WITH_THREADS
#define WITH_ATOMIC
#define THREAD_SAFE_EVAL

#include <math.h>
#include "ifdata.h"
#ifdef WITH_THREADS
#include <pthread.h>
#ifdef __APPLE__
#include <libkern/OSAtomic.h>
#endif
#endif

// Enable new code to support a true DCOP with Josephson junctions.
// See description in devlib/jj/jjload.cc.
#define NEWJJDC

// references
class spMatrixFrame;
class cThreadPool;
struct sSymTab;
struct sOUTdata;
struct sCKTtable;
struct sRunDesc;
struct variable;
struct VerilogBlock;
struct sJOB;
struct sTASK;
struct sCKT;
struct sFtCirc;
struct sHtab;
struct IFmacro;
struct sDCTprms;


//
// Some useful constants.
//

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif
#ifndef M_E
#define M_E             2.7182818284590452354
#endif
#ifndef M_LOG10E
#define M_LOG10E        0.43429448190325182765
#endif
#ifndef M_SQRT2
#define	M_SQRT2         1.41421356237309504880
#endif

// Release 3.2.15: The reference temperature is changed to 25C to match
// Hspice.
// #define wrsREFTEMP          300.15  // 27 degrees C
#define wrsREFTEMP          298.15  // 25 degrees C
#define wrsCHARGE           1.60217646e-19

#define wrsCONSTCtoK        273.15
#define wrsCONSTboltz       1.3806226e-23
#define wrsCONSTvt0         (wrsCONSTboltz*wrsREFTEMP/wrsCHARGE)
#define wrsCONSTKoverQ      (wrsCONSTboltz/wrsCHARGE)
#define wrsCONSTroot2       M_SQRT2
#define wrsCONSTe           M_E
#define wrsCONSTc           2.997925e8
#define wrsCONSTphi0        2.067833667e-15
#define wrsCONSTphi0_2pi    (wrsCONSTphi0/(2*M_PI))
#define wrsCONSTplanck      6.62606896e-34


//
// General macros.
//

#ifndef SPMIN
#define SPMIN(a,b) ((a) < (b) ? (a) : (b))
#define SPMAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define NUMELEMS(ARRAY) (sizeof(ARRAY)/sizeof(*ARRAY))

//
// Base classes for generic device and model instances.
//
// Neither of these have constructors or destructors, or virtual
// functions.  Both structs are completely zeroed on allocation.  We
// assume that the device model code, originally written in C for use
// with calloc'ed structs, will take care of everything.
//

// The base class for device instances.  The device instances struct
// MUST have the node integers at the top of the data area, in order. 
// These are accessed via nodeptr().  Note that alignment is correct
// for the usual 32 and 64-bit configurations.
//
struct sGENmodel;
//
struct sGENinstance
{
    // Hash table hooks.
    unsigned long tab_key()             { return ((unsigned long)GENname); }
    sGENinstance *tab_next()            { return (GENnextTabInstance); }
    void set_tab_next(sGENinstance *n)  { GENnextTabInstance = n; }

    int numnodes()                      { return (GENnumNodes); }
    int *nodeptr(int n)                 { return (&GENnumNodes + n ); }
    // index is 1-based.

    // cktparam.cc
    int setParam(int, IFdata*);
    int setParam(const char*, IFdata*);
    int askParam(const sCKT*, int, IFdata*) const;
    int askParam(const sCKT*, const char*, IFdata*) const;

    sGENmodel *GENmodPtr;               // backpointer to model
    sGENinstance *GENnextInstance;      // pointer to next instance of 
    sGENinstance *GENnextTabInstance;   // hash table link
                                        //  current model
    IFuid GENname;      // pointer to character string naming this instance
    int GENstate;       // state index number
    int GENnumNodes;    // max node count
};

struct sGENinstTable;

// The device model base class.
//
struct sGENmodel
{
    ~sGENmodel();

    // cktparam.cc
    int setParam(int, IFdata*);
    int setParam(const char*, IFdata*);
    int askParam(int, IFdata*) const;
    int askParam(const char*, IFdata*) const;

    int GENmodType;             // type index of this device type
    sGENmodel *GENnextModel;    // pointer to next possible model in 
                                //  linked list
    sGENinstance *GENinstances; // pointer to list of instances that have this
                                //  model
    sGENinstTable *GENinstTab;  // hash tab for fast uid->sGENinstance access
    IFuid GENmodName;           // pointer to IFuid string naming this model
};


// Structures used for keeping track of tables input by the user
// with .table lines.
//
struct sCKTtable;

// For struct sCKTentry e_type.
enum ENTtype
{
    ENT_UNDEF,
    ENT_REAL,
    ENT_CPLX,
    ENT_TABLE,
    ENT_OMITTED
};

struct sCKTentry
{
    sCKTentry()
        {
            ent_next = 0;
            ent_type = ENT_UNDEF;
            ent_val = 0;
            ent_data.real = 0.0;
            ent_data.imag = 0.0;
            ent_table = 0;
            ent_table_name = 0;
        }

    ~sCKTentry()
        {
            delete [] ent_table_name;
        }

    const char *table_name()    { return (ent_table_name); }
    sCKTtable *table()          { return (ent_table); }

    void set_table_name(const char*);
    void set_table(sCKTtable*);

    sCKTentry *ent_next;        // pointer to next entry
    ENTtype ent_type;           // entry type (ENT_???)
    double ent_val;             // table entry ordinate
    cIFcomplex ent_data;        // table value

private:
    sCKTtable *ent_table;       // referennced table
    char *ent_table_name;       // name of table, temporary
};

// For struct sCKTtable tab_type.
enum CTABtype
{
    CTAB_TRAN,
    CTAB_DC,
    CTAB_AC
};

struct sCKTtable
{
    sCKTtable(const char*);
    ~sCKTtable();

    IFcomplex tablEval(double x);    // evaluate the table at x
    double tablEvalDeriv(double x);  // compute derivative at x
    void tablfix();

    const char *name()              { return (tab_name); }

    CTABtype type()                 { return (tab_type); }
    void set_type(CTABtype t)       { tab_type = t; }

    sCKTentry *entries()            { return (tab_entry); }
    void set_entries(sCKTentry *e)  { tab_entry = e; }

    sCKTtable *next()               { return (tab_next); }
    void set_next(sCKTtable *n)     { tab_next = n; }

private:
    char *tab_name;             // name of table
    CTABtype tab_type;          // CTAB_...
    sCKTentry *tab_entry;       // pointer to entry list head
    sCKTtable *tab_next;        // pointer to next table, if any
};


//
//  Analysis classes and definitions
//

// General DC analysis defines.

#define DCTNESTLEVEL 2 // depth of nesting of curves - 2 for spice2

#define DC_NAME1  101
#define DC_START1 102
#define DC_STOP1  103
#define DC_STEP1  104
#define DC_NAME2  105
#define DC_START2 106
#define DC_STOP2  107
#define DC_STEP2  108

struct sGENSRCinstance;

typedef int(*LoopWorkFunc)(sCKT*, int);

#define ALLPRMS
// This define allows arbitrary device parameters in the form
// devicename,paramname to be passed in the position of the source
// names in DCT.

// General AC analysis defines.

// Available step types:
//
enum AC_STEPTYPE { DCSTEP, DECADE, OCTAVE, LINEAR };

#define AC_DEC   201
#define AC_OCT   202
#define AC_LIN   203
#define AC_START 204
#define AC_STOP  205
#define AC_STEPS 206

struct sACprms
{
    sACprms()
        {
            ac_fstart = 0.0;
            ac_fstop = 0.0;
            ac_fsave = 0.0;
            ac_numSteps = 0;
            ac_stepType = DCSTEP;
        }

    int query(int, IFdata*) const;
    int setp(int, IFdata*);
    int points(const sCKT*);
    int loop(LoopWorkFunc, sCKT*, int);

    double fstart()         { return (ac_fstart); }
    double fstop()          { return (ac_fstop); }
    AC_STEPTYPE stepType()  { return (ac_stepType); }

private:
    double ac_fstart;
    double ac_fstop;
    double ac_fsave;
    int ac_numSteps;
    AC_STEPTYPE ac_stepType;
};


// Used as a base class below.
//
class cBase
{
public:
    cBase()
        {
            b_name = 0;
            b_type = 0;
        }

    IFuid b_name;
    int b_type;
};


// Data structure passed to output functions.  Each thread has its
// own sOUTdata.
//
struct sOUTdata
{
    sOUTdata()
        {
            circuitPtr = 0;
            analysisPtr = 0;
            analName = 0;
            refName = 0;
            refType = 0;
            numNames = 0;
            dataNames = 0;
            dataType = 0;
            numPts = 0;
            count = 0;
            cycle = 0;
            initValue = 0.0;
            finalValue = 0.0;
            step = 0.0;
        }

    sCKT *circuitPtr;       // associated circuit (thread copy)
    sJOB *analysisPtr;      // associated task (thread copy)
    IFuid analName;         // analysis name
    IFuid refName;          // reference vector name
    int refType;            // referenct data type (IFvalue)
    int numNames;           // number of data names
    IFuid *dataNames;       // array of data names
    int dataType;           // data type (IFvalue)
    int numPts;             // number of output points
    int count;              // running output count
    int cycle;              // 1+sequence when multi-thread, 0 otherwise
    double initValue;       // analysis start
    double finalValue;      // analysis end
    double step;            // analysis delta
};


// Base class for an analysis instance
//
struct sJOB : public cBase
{
    sJOB()
        {
            JOBnextJob = 0;
            JOBoutdata = 0;
            JOBrun = 0;
        }

    virtual ~sJOB();

    virtual sJOB *dup()
        {
            sJOB *j = new sJOB;
            j->b_name = b_name;
            j->b_type = b_type;
            j->JOBoutdata = new sOUTdata(*JOBoutdata);
            j->JOBrun = JOBrun;
            return (j);
        }

    virtual bool threadable()       { return (false); }
    virtual int init(sCKT*)         { return (OK); }
    virtual int points(const sCKT*) { return (1); }

    // task.cc
    int setParam(int, IFdata*);
    int setParam(const char*, IFdata*);

    sJOB *JOBnextJob;           // next job in list
    sOUTdata *JOBoutdata;       // output data struct pointer
    sRunDesc *JOBrun;           // run descriptor
#define JOBname b_name
#define JOBtype b_type
};

// Options/Task defaults.

// Known integration methods for TSKintegrateMethod.
#define TRAPEZOIDAL 1
#define GEAR        2

// Values for options merging.
enum OMRG_TYPE { OMRG_GLOBAL, OMRG_LOCAL, OMRG_NOSHELL };

//
// Real-valued parameters.
//

#define DEF_abstol              1e-12
#define DEF_abstol_MIN          1e-15
#define DEF_abstol_MAX          1e-9

#define DEF_chgtol              1e-14
#define DEF_chgtol_MIN          1e-16
#define DEF_chgtol_MAX          1e-12

#define DEF_dcMu                0.5
#define DEF_dcMu_MIN            0.0
#define DEF_dcMu_MAX            0.5

#define DEF_defaultMosAD        0.0
#define DEF_defaultMosAD_MIN    0.0
#define DEF_defaultMosAD_MAX    1e-3

#define DEF_defaultMosAS        0.0
#define DEF_defaultMosAS_MIN    0.0
#define DEF_defaultMosAS_MAX    1e-3

#define DEF_defaultMosL         0.0
#define DEF_defaultMosL_MIN     0.0
#define DEF_defaultMosL_MAX     1e4

#define DEF_defaultMosW         0.0
#define DEF_defaultMosW_MIN     0.0
#define DEF_defaultMosW_MAX     1e4
// L/W defaults now set in model code.

#define DEF_delMin              0.0
#define DEF_delMin_MIN          0.0
#define DEF_delMin_MAX          1.0

#define DEF_dphiMax             .2*M_PI
#define DEF_dphiMax_MIN         M_PI/1000
#define DEF_dphiMax_MAX         M_PI

#define DEF_gmax                1e6
#define DEF_gmax_MIN            1e-2
#define DEF_gmax_MAX            1e12

#define DEF_gmin                1e-12
#define DEF_gmin_MIN            1e-15
#define DEF_gmin_MAX            1e-6

#define DEF_maxData             256000
#define DEF_maxData_MIN         1e3
#define DEF_maxData_MAX         2e9

#define DEF_minBreak            0.0
#define DEF_minBreak_MIN        0.0
#define DEF_minBreak_MAX        1.0

#define DEF_pivotRelTol         1e-3
#define DEF_pivotRelTol_MIN     1e-5
#define DEF_pivotRelTol_MAX     1

#define DEF_pivotAbsTol         1e-13
#define DEF_pivotAbsTol_MIN     1e-18
#define DEF_pivotAbsTol_MAX     1e-9

#define DEF_rampup              0.0
#define DEF_rampup_MIN          0.0
#define DEF_rampup_MAX          1.0

#define DEF_reltol              1e-3
#define DEF_reltol_MIN          1e-8
#define DEF_reltol_MAX          1e-2

#define DEF_temp                wrsREFTEMP
#define DEF_temp_MIN            0.0
#define DEF_temp_MAX            1000.0 + wrsCONSTCtoK

#define DEF_nomTemp             wrsREFTEMP
#define DEF_nomTemp_MIN         0.0
#define DEF_nomTemp_MAX         1000.0 + wrsCONSTCtoK

#define DEF_trapRatio           10.0
#define DEF_trapRatio_MIN       2.0
#define DEF_trapRatio_MAX       100.0

#define DEF_trtol               7.0
#define DEF_trtol_MIN           1.0
#define DEF_trtol_MAX           100.0

#define DEF_voltTol             1e-6
#define DEF_voltTol_MIN         1e-9
#define DEF_voltTol_MAX         1e-3

#define DEF_xmu                 0.5
#define DEF_xmu_MIN             0.0
#define DEF_xmu_MAX             0.5

//
// Integer Parameters
//

#define DEF_bypass              1
#define DEF_bypass_MIN          0
#define DEF_bypass_MAX          1

#define DEF_FPEmode             0
#define DEF_FPEmode_MIN         0
#define DEF_FPEmode_MAX         2

#define DEF_numGminSteps        0
#define DEF_numGminSteps_MIN    -1
#define DEF_numGminSteps_MAX    20

#define DEF_interplev           1
#define DEF_interplev_MIN       1
#define DEF_interplev_MAX       3

#define DEF_dcMaxIter           400
#define DEF_dcMaxIter_MIN       10
#define DEF_dcMaxIter_MAX       1000

#define DEF_dcTrcvMaxIter       100
#define DEF_dcTrcvMaxIter_MIN   4
#define DEF_dcTrcvMaxIter_MAX   500

#define DEF_dcOpGminMaxIter     20
#define DEF_dcOpGminMaxIter_MIN 4
#define DEF_dcOpGminMaxIter_MAX 500

#define DEF_dcOpSrcMaxIter      20
#define DEF_dcOpSrcMaxIter_MIN  4
#define DEF_dcOpSrcMaxIter_MAX  500

#define DEF_tranMaxIter         20
#define DEF_tranMaxIter_MIN     4
#define DEF_tranMaxIter_MAX     100

#define DEF_maxOrder            2
#define DEF_maxOrder_MIN        1
#define DEF_maxOrder_MAX        6

#define DEF_numSrcSteps         0
#define DEF_numSrcSteps_MIN     -1
#define DEF_numSrcSteps_MAX     20

#ifdef WITH_THREADS
#define DEF_loadThreads         0
#define DEF_loadThreads_MIN     0
#define DEF_loadThreads_MAX     31

#define DEF_loopThreads         0
#define DEF_loopThreads_MIN     0
#define DEF_loopThreads_MAX     31
#endif

//
// Booleans
//
#define DEF_dcOddStep           false
#define DEF_extPrec             false
#define DEF_forceGmin           false
#define DEF_gminFirst           false
#define DEF_hspice              false
#define DEF_jjaccel             false
#define DEF_noiter              false
#define DEF_nojjtp              false
#define DEF_noKLU               false
#define DEF_noMatrixSort        false
#define DEF_noOpIter            false
#define DEF_nopmdc              false
#define DEF_noShellOpts         false
#define DEF_fixLimit            false
#define DEF_oldStepLim          false
#define DEF_renumber            false
#define DEF_saveCurrent         false
#define DEF_spice3              false
#define DEF_trapCheck           false
#define DEF_tryToCompact        false
#define DEF_useAdjoint          false
#define DEF_translate           false

//
// Input as strings.
//
#define DEF_integrateMethod     TRAPEZOIDAL
#define DEF_optMerge            OMRG_GLOBAL
#define DEF_parHier             0
#define DEF_tranStepType        0


// Structure to hold given options.
//
struct sOPTIONS : public sJOB
{
    sOPTIONS()
        {
            OPTabstol       = DEF_abstol;
            OPTchgtol       = DEF_chgtol;
            OPTdcmu         = DEF_dcMu;
            OPTdefad        = DEF_defaultMosAD;
            OPTdefas        = DEF_defaultMosAS;
            OPTdefl         = DEF_defaultMosL;
            OPTdefw         = DEF_defaultMosW;
            OPTdelmin       = DEF_delMin;
            OPTdphimax      = DEF_dphiMax;
            OPTgmax         = DEF_gmax;
            OPTgmin         = DEF_gmin;
            OPTmaxdata      = DEF_maxData;
            OPTminbreak     = DEF_minBreak;
            OPTpivrel       = DEF_pivotRelTol;
            OPTpivtol       = DEF_pivotAbsTol;
            OPTrampup       = DEF_rampup;
            OPTreltol       = DEF_reltol;
            OPTtemp         = DEF_temp;
            OPTtnom         = DEF_nomTemp;
            OPTtrapratio    = DEF_trapRatio;
            OPTtrtol        = DEF_trtol;
            OPTvntol        = DEF_voltTol;
            OPTxmu          = DEF_xmu;

            OPTbypass       = DEF_bypass;
            OPTfpemode      = DEF_FPEmode;
            OPTgminsteps    = DEF_numGminSteps;
            OPTinterplev    = DEF_interplev;
            OPTitl1         = DEF_dcMaxIter;
            OPTitl2         = DEF_dcTrcvMaxIter;
            OPTitl2gmin     = DEF_dcOpGminMaxIter;
            OPTitl2src      = DEF_dcOpSrcMaxIter;
            OPTitl4         = DEF_tranMaxIter;
#ifdef WITH_THREADS
            OPTloadthrds    = DEF_loadThreads;
            OPTloopthrds    = DEF_loopThreads;
#endif
            OPTmaxord       = DEF_maxOrder;
            OPTsrcsteps     = DEF_numSrcSteps;

            OPTdcoddstep    = DEF_dcOddStep;
            OPTextprec      = DEF_extPrec;
            OPTforcegmin    = DEF_forceGmin;
            OPTgminfirst    = DEF_gminFirst;
            OPThspice       = DEF_hspice;
            OPTjjaccel      = DEF_jjaccel;
            OPTnoiter       = DEF_noiter;
            OPTnojjtp       = DEF_nojjtp;
            OPTnoklu        = DEF_noKLU;
            OPTnomatsort    = DEF_noMatrixSort;
            OPTnoopiter     = DEF_noOpIter;
            OPTnopmdc       = DEF_nopmdc;
            OPTnoshellopts  = DEF_noShellOpts;
            OPToldlimit     = DEF_fixLimit;
            OPToldsteplim   = DEF_oldStepLim;
            OPTrenumber     = DEF_renumber;
            OPTsavecurrent  = DEF_saveCurrent;
            OPTspice3       = DEF_spice3;
            OPTtrapcheck    = DEF_trapCheck;
            OPTtrytocompact = DEF_tryToCompact;
            OPTuseadjoint   = DEF_useAdjoint;
            OPTtranslate    = DEF_translate;

            OPTmethod       = DEF_integrateMethod;
            OPToptmerge     = DEF_optMerge;
            OPTparhier      = DEF_parHier;
            OPTsteptype     = DEF_tranStepType;

            OPTabstol_given         = 0;
            OPTchgtol_given         = 0;
            OPTdcmu_given           = 0;
            OPTdefad_given          = 0;
            OPTdefas_given          = 0;
            OPTdefl_given           = 0;
            OPTdefw_given           = 0;
            OPTdelmin_given         = 0;
            OPTdphimax_given        = 0;
            OPTgmax_given           = 0;
            OPTgmin_given           = 0;
            OPTmaxdata_given        = 0;
            OPTminbreak_given       = 0;
            OPTpivrel_given         = 0;
            OPTpivtol_given         = 0;
            OPTrampup_given         = 0;
            OPTreltol_given         = 0;
            OPTtemp_given           = 0;
            OPTtnom_given           = 0;
            OPTtrapratio_given      = 0;
            OPTtrtol_given          = 0;
            OPTvntol_given          = 0;
            OPTxmu_given            = 0;

            OPTbypass_given         = 0;
            OPTfpemode_given        = 0;
            OPTgminsteps_given      = 0;
            OPTinterplev_given      = 0;
            OPTitl1_given           = 0;
            OPTitl2_given           = 0;
            OPTitl2gmin_given       = 0;
            OPTitl2src_given        = 0;
            OPTitl4_given           = 0;
#ifdef WITH_THREADS
            OPTloadthrds_given      = 0;
            OPTloopthrds_given      = 0;
#endif
            OPTmaxord_given         = 0;
            OPTsrcsteps_given       = 0;

            OPTdcoddstep_given      = 0;
            OPTextprec_given        = 0;
            OPTforcegmin_given      = 0;
            OPTgminfirst_given      = 0;
            OPThspice_given         = 0;
            OPTjjaccel_given        = 0;
            OPTnoiter_given         = 0;
            OPTnojjtp_given         = 0;
            OPTnoklu_given          = 0;
            OPTnomatsort_given      = 0;
            OPTnoopiter_given       = 0;
            OPTnopmdc_given         = 0;
            OPTnoshellopts_given    = 0;
            OPToldlimit_given       = 0;
            OPToldsteplim_given     = 0;
            OPTrenumber_given       = 0;
            OPTsavecurrent_given    = 0;
            OPTspice3_given         = 0;
            OPTtrapcheck_given      = 0;
            OPTtrytocompact_given   = 0;
            OPTuseadjoint_given     = 0;
            OPTtranslate_given      = 0;

            OPTmethod_given         = 0;
            OPToptmerge_given       = 0;
            OPTparhier_given        = 0;
            OPTsteptype_given       = 0;
        }

    sJOB *dup()     { return (copy()); }

    sOPTIONS *copy();                   // copy a sOPTIONS struct
    void setup(const sOPTIONS*, OMRG_TYPE);  // Set/merge options.
    void dump();                        // print list of set options
    int askOpt(int, IFvalue*, int*);    // obtain the value of an option
    variable *tovar();                  // obtain a 'variable' list

    static sOPTIONS *shellOpts()        { return (&OPTsh_opts); }

    double OPTabstol;
    double OPTchgtol;
    double OPTdcmu;
    double OPTdefad;
    double OPTdefas;
    double OPTdefl;
    double OPTdefw;
    double OPTdelmin;
    double OPTdphimax;
    double OPTgmax;
    double OPTgmin;
    double OPTmaxdata;
    double OPTminbreak;
    double OPTpivrel;
    double OPTpivtol;
    double OPTrampup;
    double OPTreltol;
    double OPTtemp;
    double OPTtnom;
    double OPTtrapratio;
    double OPTtrtol;
    double OPTvntol;
    double OPTxmu;

    int OPTbypass;
    int OPTfpemode;
    int OPTgminsteps;
    int OPTinterplev;
    int OPTitl1;
    int OPTitl2;
    int OPTitl2gmin;
    int OPTitl2src;
    int OPTitl4;
#ifdef WITH_THREADS
    int OPTloadthrds;
    int OPTloopthrds;
#endif
    int OPTmaxord;
    int OPTsrcsteps;

    bool OPTdcoddstep;
    bool OPTextprec;
    bool OPTforcegmin;
    bool OPTgminfirst;
    bool OPThspice;
    bool OPTjjaccel;
    bool OPTnoiter;
    bool OPTnojjtp;
    bool OPTnoklu;
    bool OPTnomatsort;
    bool OPTnoopiter;
    bool OPTnopmdc;
    bool OPTnoshellopts;
    bool OPToldlimit;
    bool OPToldsteplim;
    bool OPTrenumber;
    bool OPTsavecurrent;
    bool OPTspice3;
    bool OPTtrapcheck;
    bool OPTtrytocompact;
    bool OPTuseadjoint;
    bool OPTtranslate;

    int OPTmethod;
    int OPToptmerge;
    int OPTparhier;
    int OPTsteptype;

    unsigned int OPTabstol_given:1;
    unsigned int OPTchgtol_given:1;
    unsigned int OPTdcmu_given:1;
    unsigned int OPTdefad_given:1;
    unsigned int OPTdefas_given:1;
    unsigned int OPTdefl_given:1;
    unsigned int OPTdefw_given:1;
    unsigned int OPTdelmin_given:1;
    unsigned int OPTdphimax_given:1;
    unsigned int OPTgmax_given:1;
    unsigned int OPTgmin_given:1;
    unsigned int OPTmaxdata_given:1;
    unsigned int OPTminbreak_given:1;
    unsigned int OPTpivrel_given:1;
    unsigned int OPTpivtol_given:1;
    unsigned int OPTrampup_given:1;
    unsigned int OPTreltol_given:1;
    unsigned int OPTtemp_given:1;
    unsigned int OPTtnom_given:1;
    unsigned int OPTtrapratio_given:1;
    unsigned int OPTtrtol_given:1;
    unsigned int OPTvntol_given:1;
    unsigned int OPTxmu_given:1;

    unsigned int OPTbypass_given:1;
    unsigned int OPTfpemode_given:1;
    unsigned int OPTgminsteps_given:1;
    unsigned int OPTinterplev_given:1;
    unsigned int OPTitl1_given:1;
    unsigned int OPTitl2_given:1;
    unsigned int OPTitl2gmin_given:1;
    unsigned int OPTitl2src_given:1;
    unsigned int OPTitl4_given:1;
#ifdef WITH_THREADS
    unsigned int OPTloadthrds_given:1;
    unsigned int OPTloopthrds_given:1;
#endif
    unsigned int OPTmaxord_given:1;
    unsigned int OPTsrcsteps_given:1;

    unsigned int OPTdcoddstep_given:1;
    unsigned int OPTextprec_given:1;
    unsigned int OPTforcegmin_given:1;
    unsigned int OPTgminfirst_given:1;
    unsigned int OPThspice_given:1;
    unsigned int OPTjjaccel_given:1;
    unsigned int OPTnoiter_given:1;
    unsigned int OPTnojjtp_given:1;
    unsigned int OPTnoklu_given:1;
    unsigned int OPTnomatsort_given:1;
    unsigned int OPTnoopiter_given:1;
    unsigned int OPTnopmdc_given:1;
    unsigned int OPTnoshellopts_given:1;
    unsigned int OPToldlimit_given:1;
    unsigned int OPToldsteplim_given:1;
    unsigned int OPTrenumber_given:1;
    unsigned int OPTsavecurrent_given:1;
    unsigned int OPTspice3_given:1;
    unsigned int OPTtrapcheck_given:1;
    unsigned int OPTtrytocompact_given:1;
    unsigned int OPTuseadjoint_given:1;
    unsigned int OPTtranslate_given:1;

    unsigned int OPTmethod_given:1;
    unsigned int OPToptmerge_given:1;
    unsigned int OPTparhier_given:1;
    unsigned int OPTsteptype_given:1;

protected:
    static sOPTIONS OPTsh_opts;
};


// The task control struct.
//
struct sTASK : public cBase
{
    sTASK(IFuid taskName)
        {
            b_name = taskName;
            TSKjobs = 0;
            TSKshellOpts = 0;
        }

    ~sTASK();

    // Copy the values, not the jobs list or shellOpts.
    sTASK *dup()
        {
            sTASK *t = new sTASK(b_name);
            t->TSKopts = TSKopts;
            return (t);
        }

    // task.cc
    static int newAnal(sTASK*, int, sJOB** = 0);
    int findAnal(int*, sJOB**, IFuid);
    void setOptions(sOPTIONS*, bool);

    sJOB *TSKjobs;          // list of analyses to perform
    sOPTIONS *TSKshellOpts; // copy of shell options, when merged
    sOPTIONS TSKopts;
#define TSKname b_name
#define TSKtype b_type

#define TSKabstol           TSKopts.OPTabstol
#define TSKchgtol           TSKopts.OPTchgtol
#define TSKdcMu             TSKopts.OPTdcmu
#define TSKdefaultMosAD     TSKopts.OPTdefad
#define TSKdefaultMosAS     TSKopts.OPTdefas
#define TSKdefaultMosL      TSKopts.OPTdefl
#define TSKdefaultMosW      TSKopts.OPTdefw
#define TSKdelMin           TSKopts.OPTdelmin
#define TSKdphiMax          TSKopts.OPTdphimax
#define TSKgmax             TSKopts.OPTgmax
#define TSKgmin             TSKopts.OPTgmin
#define TSKmaxData          TSKopts.OPTmaxdata
#define TSKminBreak         TSKopts.OPTminbreak
#define TSKpivotRelTol      TSKopts.OPTpivrel
#define TSKpivotAbsTol      TSKopts.OPTpivtol
#define TSKrampUpTime       TSKopts.OPTrampup
#define TSKreltol           TSKopts.OPTreltol
#define TSKtemp             TSKopts.OPTtemp
#define TSKnomTemp          TSKopts.OPTtnom
#define TSKtrapRatio        TSKopts.OPTtrapratio
#define TSKtrtol            TSKopts.OPTtrtol
#define TSKvoltTol          TSKopts.OPTvntol
#define TSKxmu              TSKopts.OPTxmu

#define TSKbypass           TSKopts.OPTbypass
#define TSKfpeMode          TSKopts.OPTfpemode
#define TSKnumGminSteps     TSKopts.OPTgminsteps
#define TSKinterpLev        TSKopts.OPTinterplev
#define TSKdcMaxIter        TSKopts.OPTitl1
#define TSKdcTrcvMaxIter    TSKopts.OPTitl2
#define TSKdcOpGminMaxIter  TSKopts.OPTitl2gmin
#define TSKdcOpSrcMaxIter   TSKopts.OPTitl2src
#define TSKtranMaxIter      TSKopts.OPTitl4
#ifdef WITH_THREADS
#define TSKloadThreads      TSKopts.OPTloadthrds
#define TSKloopThreads      TSKopts.OPTloopthrds
#endif
#define TSKmaxOrder         TSKopts.OPTmaxord
#define TSKnumSrcSteps      TSKopts.OPTsrcsteps

#define TSKdcOddStep        TSKopts.OPTdcoddstep
#define TSKextPrec          TSKopts.OPTextprec
#define TSKforceGmin        TSKopts.OPTforcegmin
#define TSKgminFirst        TSKopts.OPTgminfirst
#define TSKhspice           TSKopts.OPThspice
#define TSKjjaccel          TSKopts.OPTjjaccel
#define TSKnoiter           TSKopts.OPTnoiter
#define TSKnojjtp           TSKopts.OPTnojjtp
#define TSKnoKLU            TSKopts.OPTnoklu
#define TSKnoMatrixSort     TSKopts.OPTnomatsort
#define TSKnoOpIter         TSKopts.OPTnoopiter
#define TSKnoPhaseModeDC    TSKopts.OPTnopmdc
#define TSKnoShellOpts      TSKopts.OPTnoshellopts
#define TSKfixLimit         TSKopts.OPToldlimit
#define TSKoldStepLim       TSKopts.OPToldsteplim
#define TSKrenumber         TSKopts.OPTrenumber
#define TSKsaveCurrent      TSKopts.OPTsavecurrent
#define TSKspice3           TSKopts.OPTspice3
#define TSKtrapCheck        TSKopts.OPTtrapcheck
#define TSKtryToCompact     TSKopts.OPTtrytocompact
#define TSKuseAdjoint       TSKopts.OPTuseadjoint
#define TSKtranslate        TSKopts.OPTtranslate

#define TSKintegrateMethod  TSKopts.OPTmethod
#define TSKoptMerge         TSKopts.OPToptmerge
#define TSKparHier          TSKopts.OPTparhier
#define TSKtranStepType     TSKopts.OPTsteptype
};


// Defines for the value of CKTcurrentAnalysis.
#define DOING_DCOP  0x1
#define DOING_TRCV  0x2
#define DOING_AC    0x4
#define DOING_TRAN  0x8
#define DOING_NOISE 0x10
#define DOING_SENS  0x20
#define DOING_DISTO 0x40
#define DOING_PZ    0x80
#define DOING_TF    0x100

// Transient analysis step types for TSKtranStepType.
#define STEP_NORMAL    0  // interpolated to user time steps
#define STEP_HITUSERTP 1  // coerce analysis to fall on user time steps
#define STEP_NOUSERTP  2  // output raw internal timestep data
#define STEP_FIXEDSTEP 3  // enforce fixed internal time step (=tran step)


// Structure used to hold the collected statistics.
//
struct sSTATS
{
    sSTATS()
        {
            STATtotAnalTime = 0.0;
            STATtranTime = 0.0;
            STATloadTime = 0.0;
            STATdecompTime = 0.0;
            STATsolveTime = 0.0;
            STATreorderTime = 0.0;
            STATcvChkTime = 0.0;
            STATtranDecompTime = 0.0;
            STATtranSolveTime = 0.0;
            STATtranOutTime = 0.0;
            STATtranTsTime = 0.0;
            STATtranPctDone = 0.0;

            STATnumIter = 0;
            STATtranIter = 0;
            STATtranLastIter = 0;
            STATtranIterCut = 0;
            STATtranTrapCut = 0;

            STATtimePts = 0;
            STATaccepted = 0;
            STATrejected = 0;
            STATmatSize = 0;
            STATnonZero = 0;
            STATfillIn = 0;
#ifdef WITH_THREADS
            STATloadThreads = 0;
            STATloopThreads = 0;
#endif
            STATpageFaults = 0;
            STATvolCxSwitch = 0;
            STATinvolCxSwitch = 0;
        }

    double STATtotAnalTime; // total time for analysis
    double STATtranTime;    // transient analysis time
    double STATloadTime;    // time spent in device loading
    double STATdecompTime;  // total time spent in LU decomposition
    double STATsolveTime;   // total time spent in F-B subst
    double STATreorderTime; // total time spent reordering
    double STATcvChkTime;   // time spen convergence checking
    double STATtranDecompTime;  // time spent in transient LU decomposition
    double STATtranSolveTime;   // time spent in transient F-B Subst
    double STATtranOutTime; // time spent writing output
    double STATtranTsTime;  // time spent estimating timestep
    double STATtranPctDone; // percentage of transient analysis complete

    int STATnumIter;        // number of total iterations performed
    int STATtranIter;       // number of iterations for transient analysis
    int STATtranLastIter;   // number of iterations at last tran timepoint
    int STATtranIterCut;    // number of tran timepoints where iteration failed
    int STATtranTrapCut;    // number of tran timepoints where trapcheck failed

    int STATtimePts;        // total number of timepoints
    int STATaccepted;       // number of timepoints accepted
    int STATrejected;       // number of timepoints rejected
    int STATmatSize;        // matrix size
    int STATnonZero;        // number of nonzero entries
    int STATfillIn;         // number of fill-in terms added in reorder
#ifdef WITH_THREADS
    int STATloadThreads;    // number of loading helper threads
    int STATloopThreads;    // number of looping helper threads
#endif
    int STATpageFaults;     // page faults during analysis
    int STATvolCxSwitch;    // voluntary context switches during analysis
    int STATinvolCxSwitch;  // involuntary context switches during analysis
};


// sCKTmodHead saves a list of list heads of device models used in the
// circuit.  It also provides a place to save a pointer to the default
// model.

struct sCKTmodItem
{
    sGENmodel *head;
    sGENmodel *default_model;
};

struct sCKTmodHead
{
    sCKTmodHead()
        {
            mh_mods = 0;
            mh_numMods = 0;
            mh_size = 0;
        }

    ~sCKTmodHead()
        {
            delete [] mh_mods;
        }

    sCKTmodItem *find(int type) const
        {
            for (unsigned int i = 0; i < mh_numMods; i++) {
                if (mh_mods[i].head->GENmodType == type)
                    return (mh_mods + i);
            }
            return (0);
        }

    sCKTmodItem *insert(sGENmodel *mod)
        {
            if (!mod)
                return (0);
            for (unsigned int i = 0; i < mh_numMods; i++) {
                if (mh_mods[i].head->GENmodType == mod->GENmodType) {
                    mod->GENnextModel = mh_mods[i].head;
                    mh_mods[i].head = mod;
                    return (mh_mods + i);
                }
            }
            if (mh_numMods == mh_size) {
                sCKTmodItem *tmp = new sCKTmodItem[mh_size + 20];
                if (mh_numMods)
                    memcpy(tmp, mh_mods, mh_numMods*sizeof(sCKTmodItem));
                delete [] mh_mods;
                mh_mods = tmp;
                mh_size += 20;
            }
            mh_mods[mh_numMods].head = mod;
            mod->GENnextModel = 0;
            mh_mods[mh_numMods].default_model = 0;
            mh_numMods++;
            return (mh_mods + mh_numMods-1);
        }

    unsigned int numMods() const    { return (mh_numMods); }

    sGENmodel *model(unsigned int ix) const
        {
            return (ix < mh_numMods ? mh_mods[ix].head : 0);
        }

private:
    sCKTmodItem     *mh_mods;
    unsigned int    mh_numMods;
    unsigned int    mh_size;
};

// An iterator for models.
struct sCKTmodGen
{
    sCKTmodGen(const sCKTmodHead &mh)
        {
            head = &mh;
            ix = 0;
        }

    sGENmodel *next()
        {
            if (!head)
                return (0);
            if (ix >= head->numMods())
                return (0);
            sGENmodel *m = head->model(ix);
            ix++;
            return (m);
        }

private:
    const sCKTmodHead   *head;
    unsigned int        ix;
};

// Breakpoint control.
//
struct sCKTlattice
{
    struct lattice
    {
        double offs;
        double per;
    };

    sCKTlattice()
        {
            lattices = 0;
            breaks = 0;
            numLattices = 0;
            numBreaks = 0;
        }

    ~sCKTlattice()
        {
            delete [] lattices;
            delete [] breaks;
        }

    void init()
        {
            delete [] lattices;
            delete [] breaks;
            lattices = 0;
            breaks = 0;
            numLattices = 0;
            numBreaks = 0;
        }

    int numbreaks() { return (numBreaks); }

    void clear_break(double, double, double*);
    bool set_break(double, double, double, double*);
    void set_lattice(double, double);
    double nextbreak(double, double);

private:
    lattice *lattices;
    double *breaks;
    int numLattices;
    int numBreaks;
};


#define STAB_START_MASK 31

// Structures used to maintain a symbol table for nodes and names.
//
template <class T>
struct sTab
{
    sTab(bool ci)
        {
            t_allocated = 0;
            t_hashmask = STAB_START_MASK;
            t_tab = new T*[t_hashmask + 1];
            memset(t_tab, 0, (t_hashmask + 1)*sizeof(T*));
            t_ci = ci;
        }

    ~sTab()
        {
            if (t_tab) {
                for (unsigned int i = 0; i <= t_hashmask; i++) {
                    T *lt;
                    for (T *t = t_tab[i]; t; t = lt) {
                        lt = t->t_next;
                        delete t;
                    }
                }
                delete [] t_tab;
            }
        }

    unsigned int hash(const char*) const;
    int comp(const char*, const char*) const;
    T *find(const char*) const;
    void add(T*);
    void remove(const char*);
    void check_rehash();

    void set(sTab<T> &tab)
        {
            delete [] t_tab;
            t_tab = tab.t_tab;
            tab.t_tab = 0;
            t_hashmask = tab.t_hashmask;
            t_allocated = tab.t_allocated;
            tab.t_allocated = 0;
        }

private:
    T **t_tab;
    unsigned int t_allocated;
    unsigned int t_hashmask;
    bool t_ci;  // case-insensitive flag
};


// Defines for node parameters.
#define PARM_NS 1         // 'nodeset' value
#define PARM_IC 2         // initial condition
#define PARM_NODETYPE 3   // type of node

// Defines for sCKTnode::nd_type.
enum { SP_VOLTAGE, SP_CURRENT };

// Structure to hold node parameters.  This is allocated by the
// sCKTnodeTab factory, and should never be explicitly freed.
//
struct sCKTnode
{
    friend struct sCKTnodeTab;

    // Bulk allocated, no constructor or destructor.

    // symtab.cc
    int ask(int, IFvalue*) const;
    int set(int, IFdata*);
    int bind(sGENinstance*, int);

    IFuid name()            const { return (nd_name); }
    int number()            const { return (nd_number); }
    int type()              const { return (nd_type); }
#ifdef NEWJJDC
    bool phase()            const { return (nd_phase); }
    void set_phase(bool b)        { nd_phase = b; }
#endif
    bool icGiven()          const { return (nd_icGiven); }
    bool nsGiven()          const { return (nd_nsGiven); }
    double ic()             const { return (nd_ic); }
    double nodeset()        const { return (nd_nodeset); }

    void set_name(IFuid n)        { nd_name = n; }

    static IFparm *nodeParam(int i)
        {
            if (i >= 0 && i < num_params)
                return (params + i);
            return 0;
        }

private:
    IFuid nd_name;          // Name (UID) of node.
    int nd_number;          // Internal allocation number.
#ifdef NEWJJDC
    unsigned char nd_type;  // SP_VOLTAGE or SP_CURRENT.
    bool nd_phase;          // Connected to inductor or Josephson junction.
#else
    unsigned short nd_type; // SP_VOLTAGE or SP_CURRENT.
#endif
    bool nd_icGiven;        // Ic field is valid.
    bool nd_nsGiven;        // Nodeset field is valid.
    double nd_ic;           // Initial condition.
    double nd_nodeset;      // Nodeset value.

    static IFparm params[];
    static int num_params;
};

#define CKT_NT_MASK     0xff
#define CKT_NT_SHIFT    8
#define CKT_NT_INITSZ   4

// The node struct factory.  This is instantiated as part of sCKT, whose
// constructor clears everything with memset, so the constructor below
// is redundant.  It also contains a name table for terminal names.
//
struct sCKTnodeTab
{
    sCKTnodeTab();
    ~sCKTnodeTab();

    // Terminal (UID) table element.
    struct sNEnt
    {
        sNEnt()
            {
                t_ent = 0;
                t_next = 0;
                t_node = 0;
            }

        ~sNEnt()
            {
                delete [] t_ent;
            }

        char *t_ent;
        sNEnt *t_next;
        sCKTnode* t_node;
    };

    int numNodes()              const { return (nt_count); }
    void deallocateLast()             { if (nt_count) nt_count--; }

    sCKTnode *nextNode(const sCKTnode *n) const
        {
            unsigned int i = n->number() + 1;
            if (i >= nt_count)
                return (0);
            return (nt_ary[i >> CKT_NT_SHIFT] + (i & CKT_NT_MASK));
        }

    void reset();
    sCKTnode *newNode(unsigned int);
    sCKTnode *find(unsigned int) const;

    // Terminal-related functions.
    int term_insert(char**, sCKTnode**);
    int mk_term(char**, sCKTnode**);
    int find_term(char**, sCKTnode**);
    int gnd_insert(char**, sCKTnode**);

    // Nodes and termnals.
    void dealloc(unsigned int);

private:
    sCKTnode **nt_ary;          // Array of blocks of nodes.
    unsigned int nt_size;       // Size of node bank array.
    unsigned int nt_count;      // Total number of nodes allocated.
    sTab<sNEnt> *nt_term_tab;   // The terminal name table.
};

// Created and returned by sCKT::nodeVals.
//
struct
sCKTnodeVal
{
    const char *name;   // name of node
    double value;       // last computed value
};


// Symbol table for names other than terminal/node names.
//
struct sSymTab
{
    struct sEnt
    {
        sEnt()
            {
                t_ent = 0;
                t_next = 0;
            };

        ~sEnt()
            {
                delete [] t_ent;
            }

        char *t_ent;
        sEnt *t_next;
    };
    sSymTab(bool ci) : sym_tab(ci) { }

    int insert(char**);

    void set(sSymTab *t) { sym_tab.set(t->sym_tab); }

private:
    sTab<sEnt> sym_tab;
};

// Types for sCKT::newUid.
//
enum UID_TYPE {
    UID_ANALYSIS = 0x1,
    UID_TASK     = 0x2,
    UID_INSTANCE = 0x4,
    UID_MODEL    = 0x8,
    UID_OTHER    = 0x10
};

// The circuit control struct
//
struct sCKT
{
    // ckt.cc
    sCKT();
    ~sCKT();
    void acDump(double, sRunDesc*);
    int acLoad();
    int accept();
    double *alloc(int, int);
    int breakClr();
    int breakInit();
    int breakSet(double);
    int breakSetLattice(double, double);
    void clrTable();
    int convTest();
    int delInst(sGENmodel*, IFuid, sGENinstance*);
    int delModl(sGENmodel**, IFuid, sGENmodel*);
    int doTask(bool);
    int doTaskSetup();
    void dump(double, sRunDesc*);
    int names(int*, IFuid**);
    int nodeVals(sCKTnodeVal**) const;
    int findBranch(IFuid);
    int findInst(int*, sGENinstance**, IFuid, sGENmodel*, IFuid) const;
    int findModl(int*, sGENmodel**, IFuid) const;
    int ic();
    int inst2Node(sGENinstance*, int, sCKTnode**, IFuid*) const;
    int load(bool = false);
    int loadGmin();
    double computeMinDelta();
    int backup(DEV_BKMODE);
    int newInst(sGENmodel*, sGENinstance**, IFuid);
    int newModl(int, sGENmodel**, IFuid);
    int newTask(const char*, const char*, sTASK**);
private:
    int dynamic_gmin(int, int, int, bool = false);
    int spice3_gmin(int, int, int);
    int dynamic_src(int, int, int);
    int spice3_src(int, int, int);
public:
    int op(int, int, int);
    double *predict();
    int setic();
    int setup();
    int unsetup();
    int resetup();
    int initTranFuncs(double, double);
    int temp();
    void terr(int, double*);
    char *trouble(const char*);
    int trunc(double*);
    IFmacro *find_macro(const char*, int);
    void save_macro(IFmacro*);
    void warnSingular();
    int checkLVloops();
    int checkCIcutSets();
    int evalTranFunc(double**, const char*, double*, int);

    static int checkFPE(bool);
    static int disableFPE();
    static void enableFPE(int);

    // cktparam.cc
    int typelook(const char*, sGENmodel** = 0) const;
    int typelook(int, sGENmodel** = 0) const;
    int doask(IFdata*, const sGENinstance*, const sGENmodel*,
        const IFparm*) const;
    int finddev(IFuid, sGENinstance**, sGENmodel**) const;
    variable *getParam(const char*, const char*, IFspecial* = 0) const;
    int getParam(const char*, const char*, IFdata*, IFspecial* = 0) const;
    int setParam(const char*, const char*, IFdata*);
    variable *getAnalParam(const char*, const char*, IFspecial* = 0) const;
    int getAnalParam(const char*, const char*, IFdata*, IFspecial* = 0) const;

    // niaciter.cc
    int NIacIter();

    // nicomcof.cc
    int NIcomCof(); 

    // niconv.cc
    int NIconvTest();

    // niditer.cc
    int NIdIter();

    // niinit.cc
    int NIinit();
    void NIprint(bool, bool, bool);
    void NIdbgPrint(bool, bool, bool, const char*);
    int NIreinit();
    void NIdestroy();

    // niinteg.cc
    double NIceq(int);
    bool NItrapCheck(int);

    // niiter.cc
    int NIiter(int);

    // niniter.cc
    void NInzIter(int, int);

    // symtab.cc
    int insert(char**) const;
    int newUid(IFuid*, IFuid, const char*, UID_TYPE);
    int newTerm(IFuid*, IFuid, const char*, sCKTnode**);
    int mkVolt(sCKTnode**, IFuid, const char*);
    int mkCur(sCKTnode**, IFuid, const char*);

    // veriloga.cc
    double va_ddt(int, double, double = 0.0);
    double va_idt(int, double, double = 0.0, bool = false, double = 0.0);
    void va_boundStep(double);
    bool va_analysis(const char*);
    double va_simparam(const char*, double, bool, sGENinstance*);
#ifdef NEWJJDC
    void va_set_phase_node(int);
#endif
    bool va_initial_step();
    bool va_final_step();
    double va_absdelay(double, double);
    int va_random(int = 0);
    double va_rdist(int, double, double, double, int,
        double(*)(double, double));
    double va_rdist_uniform(int, double, double, double, int);
    double va_rdist_normal(int, double, double, double, int);
    double va_rdist_exponential(int, double, double, int);
    double va_rdist_poisson(int, double, double, int);
    double va_rdist_chi_square(int, double, double, int);
    double va_rdist_t(int, double, double, int);
    double va_rdist_erlang(int, double, double, double, int);

#ifdef WITH_THREADS
    void incNoncon()
        {
            if (!CKTloadThreads)
                CKTnoncon++;
            else {
#ifdef WITH_ATOMIC
                __sync_fetch_and_add(&CKTnoncon, 1);
#else
                pthread_spin_lock(&CKTloadLock1);
                CKTnoncon++;
                pthread_spin_unlock(&CKTloadLock1);
#endif
            }
        }
#else
    void incNoncon()
        {
            CKTnoncon++;
        }
#endif

    // Functions for matrix loading, real matrix only.  We need to
    // support long double loading.  A call to this replaces the
    // pointer access in device models.
    //
#ifdef WITH_THREADS
    void ldadd(double *ptr, double val)
        {
            if (!CKTloadThreads) {
                if (CKTextPrec)
                    *(long double*)ptr += val;
                else
                    *ptr += val;
            }
            else if (CKTextPrec) {
                // Without a 16-byte compare_and_swap, I don't know
                // how to do this atomically.

#ifdef __APPLE__
                OSSpinLockLock(&CKTloadLock2);
                *(long double*)ptr += val;
                OSSpinLockUnlock(&CKTloadLock2);
#else
                pthread_spin_lock(&CKTloadLock2);
                *(long double*)ptr += val;
                pthread_spin_unlock(&CKTloadLock2);
#endif
            }
            else {
#ifdef WITH_ATOMIC
                // long long is 8 bytes on both 32/64-bit x86 Linux
                // systems.
                volatile union foo { double d; unsigned long long i; } f, g;

                do {
                    f.d = *ptr;
                    g.d = f.d + val;
                } while (! __sync_bool_compare_and_swap(
                    (unsigned long long*)ptr, f.i, g.i));
#else
                pthread_spin_lock(&CKTloadLock2);
                *ptr += val;
                pthread_spin_unlock(&CKTloadLock2);
#endif
            }
        }
#else
    void ldadd(double *ptr, double val)
        {
            if (CKTextPrec)
                *(long double*)ptr += val;
            else
                *ptr += val;
        }
#endif

    void ldset(double *ptr, double val)
        {
            if (CKTcurTask->TSKextPrec)
                *(long double*)ptr = val;
            else
                *ptr = val;
        }

#ifdef WITH_THREADS
    void rhsadd(int o, double val)
        {
            if (!CKTloadThreads)
                *(CKTrhs + o) += val;
            else {
#ifdef WITH_ATOMIC
                volatile union foo { double d; unsigned long long i; } f, g;

                double *ptr = CKTrhs + o;
                do {
                    f.d = *ptr;
                    g.d = f.d + val;
                } while (! __sync_bool_compare_and_swap(
                    (unsigned long long*)ptr, f.i, g.i));
#else
                pthread_spin_lock(&CKTloadLock3);
                *(CKTrhs + o) += val;
                pthread_spin_unlock(&CKTloadLock3);
#endif
            }
        }
#else
    void rhsadd(int o, double val)
        {
            *(CKTrhs + o) += val;
        }
#endif

// The expression evaluation is now thread-safe.
#if defined(WITH_THREADS) && !defined(THREAD_SAFE_EVAL)
#define BEGIN_EVAL  pthread_mutex_lock(&sCKT::CKTloadLock4);
#define END_EVAL  pthread_mutex_unlock(&sCKT::CKTloadLock4);
#else
#define BEGIN_EVAL
#define END_EVAL
#endif

    // If the computation will use long doubles, the preload must use long
    // doubles as well.

    void preldadd(double *ptr, double val)
        {
            if (CKTcurTask->TSKextPrec)
                *(long double*)ptr += val;
            else
                *ptr += val;
        }

    void preldset(double *ptr, double val)
        {
            if (CKTcurTask->TSKextPrec)
                *(long double*)ptr = val;
            else
                *ptr = val;
        }

    // Functions used im MOS device models to set default device size. 
    // The same defaults apply to all MOS devices.

#define CKT_DEF_MOS_L   1e-6
#define CKT_DEF_MOS_W   1e-6
#define CKT_DEF_MOS_AS  0.0
#define CKT_DEF_MOS_AD  0.0

    double mos_default_l()
        {
            double d = CKTcurTask ? CKTcurTask->TSKdefaultMosL : 0.0;
            if (d <= 0.0)
                d = CKT_DEF_MOS_L;
            return (d);
        }

    double mos_default_w()
        {
            double d = CKTcurTask ? CKTcurTask->TSKdefaultMosW : 0.0;
            if (d <= 0.0)
                d = CKT_DEF_MOS_W;
            return (d);
        }

    double mos_default_as()
        {
            double d = CKTcurTask ? CKTcurTask->TSKdefaultMosAS : 0.0;
            if (d <= 0.0)
                d = CKT_DEF_MOS_AS;
            return (d);
        }

    double mos_default_ad()
        {
            double d = CKTcurTask ? CKTcurTask->TSKdefaultMosAD : 0.0;
            if (d <= 0.0)
                d = CKT_DEF_MOS_AD;
            return (d);
        }

    // Integration

    double find_ceq(int q)
        {
            if (CKTcurTask->TSKintegrateMethod == TRAPEZOIDAL)
                return (CKTorder <= 1 ? *(CKTstates[1] + q)*CKTag[1] :
                    - *(CKTstates[1] + q+1)*CKTag[1] -
                    *(CKTstates[1] + q)*CKTag[0]);
            else
                return (NIceq(q));
        }

    void integrate(double *geq, double *ceq, double cap, int q)
        {
            double cq = find_ceq(q);
            *(CKTstates[0] + q+1) = CKTag[0]* *(CKTstates[0] + q) + cq;
            *ceq = cq;
            *geq = CKTag[0]*cap;
            if (CKTtrapCheck && !CKTtrapBad)
                CKTtrapBad = NItrapCheck(q);
        }

    void integrate(int q)
        {
            double cq = find_ceq(q);
            *(CKTstates[0] + q+1) = CKTag[0]* *(CKTstates[0] + q) + cq;
            if (CKTtrapCheck && !CKTtrapBad)
                CKTtrapBad = NItrapCheck(q);
        }

    void integrate(int q, double cq)
        {
            *(CKTstates[0] + q+1) = CKTag[0]* *(CKTstates[0] + q) + cq;
            if (CKTtrapCheck && !CKTtrapBad)
                CKTtrapBad = NItrapCheck(q);
        }


    // Interpolation utility for ask-instance device model functions,
    // to provide values interpolated to the user timepoint in
    // transient analysis.  If not doing transient analysis, return
    // the current value.  If no state vector, return the (optional
    // arg) deflt.
    //
    double interp(int which, double deflt = 0.0) const
        {
            if (!CKTstates[0])
                return (deflt);
            if (CKTcurrentAnalysis & DOING_TRAN) {
                double val = CKTtranDiffs[0]* *(CKTstates[0] + which);
                for (int i = 1; i <= CKTtranDegree; i++)
                    val += CKTtranDiffs[i]* *(CKTstates[i] + which);
                return (val);
            }
            return (*(CKTstates[0] + which));
        }

    // Return the value from the CKTrhsOld vector, after checking
    // pointer.
    //
    double rhsOld(int which) const
        {
            if (!CKTrhsOld)
                return (0.0);
            return (*(CKTrhsOld + which));
        }

    double irhsOld(int which) const
        {
            if (!CKTirhsOld)
                return (0.0);
            return (*(CKTirhsOld + which));
        }

    // Terminal/node table functions.

    // Insert 'token' into the terminal symbol table, create a new
    // node and return a pointer to it in *node.
    //
    int termInsert(char **token, sCKTnode **node)
        {
            return (CKTnodeTab.term_insert(token, node));
        }

    // Insert 'token' into the terminal symbol table, use node as the
    // node pointer.
    //
    int mkTerm(char **token, sCKTnode **node)
        {
            return (CKTnodeTab.mk_term(token, node));
        }

    int findTerm(char **token, sCKTnode **node)
        {
            return (CKTnodeTab.find_term(token, node));
        }

    // Insert 'token' into the terminal symbol table as a name for
    // ground.
    //
    int gndInsert(char **token, sCKTnode **node)
        {
            return (CKTnodeTab.gnd_insert(token, node));
        }

    IFuid nodeName(int nodenum) const
        {
            sCKTnode *n = CKTnodeTab.find(nodenum);
            if (n)
                return (n->name());
            // doesn't exist - do something
            return ((void*)"UNKNOWN NODE");
        }

    sCKTnode *num2node(int nodenum) const
        {
            return (CKTnodeTab.find(nodenum));
        }

    void setTask(sTASK *tsk)
        {
            delete CKTcurTask;
            CKTcurTask = tsk;
        }

    bool jjaccel()      { return (CKTjjPresent && CKTcurTask->TSKjjaccel); }

    static int CKTstepDebug;          // enable timepoint debugging

    double CKTtime;         // time variable for transient
    double CKTdelta;        // time delta for transient
    double CKTdeltaOld[7];  // previous time deltas
    double CKTsaveDelta;    // save old delta at breakpoint
    double CKTdevMaxDelta;  // max delta, optionally set by device in accept
    double CKTvt;           // thermal voltage cache
    double CKTag[7];        // the integration variable coefficient matrix
    double CKTpred[4];      // factors used to predict new values
    double CKTtranDiffs[4]; // factors used to interpolate tran data
    double CKTstep;         // user's time increment
    double CKTmaxStep;      // user's max internal increment
    double CKTfinalTime;    // final time for transient analysis
    double CKTinitTime;     // begin print time fro transient analysis
    double CKTfinalFreq;    // final freq for ac analysis
    double CKTinitFreq;     // initial freq for ac analysis
    double CKTfinalV1;      // final V1 for dct analysis, and chained
    double CKTinitV1;       // initial V1 for dct analysis, and chained
    double CKTfinalV2;      // final V2 for dct analysis, and chained
    double CKTinitV2;       // initial V2 for dct analysis, and chained
    double CKTinitDelta;    // initial timestep
    double CKTomega;        // current frequency for ac
    double CKTsrcFact;      // source stepping factor
    double CKTdiagGmin;     // gmin stepping

    double *CKTrhs;         // current rhs value - being loaded
    double *CKTrhsOld;      // previous rhs value for convergence testing
    double *CKTrhsSpare;    // spare rhs value for reordering
    double *CKTirhs;        // current rhs value - being loaded (imag)
    double *CKTirhsOld;     // previous rhs value (imaginary)
    double *CKTirhsSpare;   // spare rhs value (imaginary)
    double *CKTsols[8];     // previous 8 rhs solutions
    double *CKToldSol;      // known-good solution backup
    double *CKToldState0;   // known-good state0 backup

    sCKTmodHead CKTmodels;  // list of device models
    sFtCirc *CKTbackPtr;    // backpointer to container
    sCKTtable *CKTtableHead; // head of table list
    sSTATS *CKTstat;        // STATistics
    double *(CKTstates[8]); // state vectors

    double *CKTtemps;       // list of temperatures from .TEMP

    cThreadPool *CKTloadPool; // multi-thread load pool
    sTASK *CKTcurTask;      // pointer to current task
    sJOB *CKTcurJob;        // pointer to current job
    spMatrixFrame *CKTmatrix; // pointer to sparse matrix
    sHtab *CKTmacroTab;     // hash table for macros

    int CKTtranDegree;      // degree of interpolation for tran
    int CKTcurrentAnalysis; // the analysis in progress (if any)
    int CKTniState;         // internal state
    int CKTnumTemps;        // length of CKTtemps
    int CKTmaxUserNodenum;  // top node number before setup()
    int CKTnumStates;       // in-use length of state vector
    int CKTstateSize;       // actual length of state vector
    int CKTmode;            // current analysis mode
    int CKTorder;           // integration order
    int CKTnoncon;          // nonconvergence count
    int CKTpreload;         // preload constants in matrix during setup
    int CKTbreak;           // stop at breakpoint
    int CKTtranTrace;       // debugging mode
    int CKTchargeCompNeeded;// flag passed to device load function
    int CKTextPrec;
#ifdef WITH_THREADS
    int CKTloadThreads;     // number of loading threads in use
    int CKTthreadId;        // thread index, 0 is main thread
#endif
    int CKTnumDC;           // number of DC calls to analysis (chained DC)
    int CKTcntDC;           // analysis calls thus far

    // flags
    bool CKThadNodeset;     // nodeset was used
    bool CKTkeepOpInfo;     // flag for small signal analyses
    bool CKTisSetup;        // CKTsetup done
    bool CKTjjPresent;      // Josephson junctions are in circuit
#ifdef NEWJJDC
    double CKTjjDCphase;    // Compute phase in DC analysis.
#endif
    bool CKTtrapCheck;      // check for non-convergence in TRAP
    bool CKTtrapBad;        // check found non-convergence
    bool CKTneedsRevertResetup;  // need to call resetup after dev restore
    bool CKTnogo;           // error found, circuit bad

    double CKTbreaks[2];          // breakpoint table
    sCKTlattice CKTlattice;       // breakpoint control
    sCKTnodeTab CKTnodeTab;       // sCKTnode factory

    // Verilog interface
    VerilogBlock *CKTvblk;        // Verilog stuff from circuit

    sGENmodel *CKTmutModels;      // for MUTs, that must be loaded before
                                  // inductors

    // for TRA history
    double *CKTtimePoints;        // list of accepted timepoints
    double *CKTdeltaList;         // list of timesteps
    int CKTtimeListSize;          // size of above lists
    int CKTtimeIndex;             // current position in above lists
    int CKTsizeIncr;              // amount to increment size of above arrays
                                  //  when out of space
    // debugging
    int CKTtroubleNode;           // Non-convergent node number
    sGENinstance *CKTtroubleElt;  // Non-convergent device instance

#ifdef WITH_THREADS
#ifdef __APPLE__
    static OSSpinLock CKTloadLock2;
#else
    static pthread_spinlock_t CKTloadLock2;
#endif
#ifndef WITH_ATOMIC
    static pthread_spinlock_t CKTloadLock1;
    static pthread_spinlock_t CKTloadLock3;
#endif
#ifndef THREAD_SAFE_EVAL
    static pthread_mutex_t CKTloadLock4;
#endif
#endif
};

#define CKTstate0 CKTstates[0]
#define CKTstate1 CKTstates[1]
#define CKTstate2 CKTstates[2]
#define CKTstate3 CKTstates[3]
#define CKTstate4 CKTstates[4]
#define CKTstate5 CKTstates[5]
#define CKTstate6 CKTstates[6]
#define CKTstate7 CKTstates[7]

//  Symbolic constants for CKTniState.
//
#define NISHOULDREORDER   0x1
#define NIREORDERED       0x2
#define NIUNINITIALIZED   0x4
#define NIACSHOULDREORDER 0x10
#define NIACREORDERED     0x20
#define NIACUNINITIALIZED 0x40
#define NIDIDPREORDER     0x100
#define NIPZSHOULDREORDER 0x200

// Defines for CKTmode.
//
// Old 'mode' parameters.
#define MODE            0x3
#define MODETRAN        0x1
#define MODEAC          0x2
//
// Old 'modedc' parameters.
#define MODEDC          0x70
#define MODEDCOP        0x10
#define MODETRANOP      0x20
#define MODEDCTRANCURVE 0x40
//
// Old 'initf' parameters.
#define INITF           0x7f00
#define MODEINITFLOAT   0x100
#define MODEINITJCT     0x200
#define MODEINITFIX     0x400
#define MODEINITSMSIG   0x800
#define MODEINITTRAN    0x1000
#define MODEINITPRED    0x2000
#define MODETRANUPD     0x4000
//
// Old 'nosolv' parameter.
#define MODEUIC         0x10000
#define MODESCROLL      0x20000

#endif // CIRCUIT_H

