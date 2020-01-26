
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2018 Whiteley Research Inc., all rights reserved.       *
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

#ifndef INPTRAN_H
#define INPTRAN_H


// These are the tran functions that we support.
//
enum PTftType
{
    PTF_tNIL,
    PTF_tPULSE,
    PTF_tGPULSE,
    PTF_tPWL,
    PTF_tSIN,
    PTF_tSPULSE,
    PTF_tEXP,
    PTF_tSFFM,
    PTF_tAM,
    PTF_tGAUSS,
    PTF_tINTERP,
    PTF_tTABLE
};

// Cache parameters for tran function evaluation.
//
struct IFtranData
{
    IFtranData(PTftType tp = PTF_tNIL)
        {
            td_coeffs = 0;
            td_parms = 0;
            td_cache = 0;
            td_numcoeffs = 0;
            td_type = tp;
            td_enable_tran = false;
        }

    virtual ~IFtranData();

    virtual void setup(sCKT*, double, double, bool);
    virtual double eval_func(double);
    virtual double eval_deriv(double);
    virtual IFtranData *dup() const;
    virtual void time_limit(const sCKT*, double*);
    virtual void print(const char*, sLstr&);
    virtual void set_param(double, int);
    virtual double *get_param(int);

protected:
    double *td_coeffs;      // tran function parameters (as input)
    double *td_parms;       // tran function run time parameters
    double *td_cache;       // cached stuff for tran evaluation
    int td_numcoeffs;       // number of tran parameters input
    short int td_type;      // function type (PTftType)
    bool td_enable_tran;    // true if using tran functions
};

// Pulse pattern list element, consisting of a "bstring" and r/rb
// values, as for the HSPICE pattern source.
//
struct pbitList
{
    pbitList(char *bs)
        {
            pl_next = 0;
            pl_bstring = bs;    // 'b' followed by 0 and 1 pattern
            pl_rb = 1;          // repeat from first bit
            pl_r = 0;           // repeat this many times, -1 is forever
        }

    ~pbitList()
        {
            delete [] pl_bstring;
        }

    static void destroy(pbitList *l)
        {
            while (l) {
                pbitList *x = l;
                l = l->pl_next;
                delete x;
            }
        }

    pbitList *next()        { return (pl_next); }
    const char *bstring()   { return (pl_bstring); }
    int rb()                { return (pl_rb); }
    int r()                 { return (pl_r); }

    int length();
    static pbitList *parse(const char**, char**);
    static pbitList *dup(pbitList*);

private:
    pbitList *pl_next;
    char *pl_bstring;
    int pl_rb;
    int pl_r;
};

// Manage a bit field, used for pattern in pulse sources.
//
struct pbitAry
{
    pbitAry()
        {
            ba_ary = new unsigned long[1];
            ba_ary[0] = 0;
            ba_size = 1;
            ba_cnt = 0;
            ba_rst = 0;
        }

    ~pbitAry()
        {
            delete [] ba_ary;
        }

    void add(pbitList*);

    unsigned long *final() const
        {
            unsigned long *tmp = new unsigned long[ba_size+1];
            memcpy(tmp, ba_ary, ba_size*sizeof(unsigned long));
            tmp[ba_size] = 0;
            return (tmp);
        }

    int count()     const { return (ba_cnt); }
    int rep_start() const { return (ba_rst); }

private:
    void add(const char *s);
    void addPRBS(int, int);

    void addbit(bool b)
        {
            if (ba_cnt >= ba_size*sizeof(unsigned long)) {
                unsigned long *tmp = new unsigned long[ba_size+1];
                memcpy(tmp, ba_ary, ba_size*sizeof(unsigned long));
                tmp[ba_size] = 0;
                delete [] ba_ary;
                ba_ary = tmp;
                ba_size++;
            }
            if (b) {
                int c = ba_cnt - (ba_size - 1)*sizeof(unsigned long);
                ba_ary[ba_size-1] |= (1 << c);
            }
            ba_cnt++;
        }

    unsigned long *ba_ary;
    unsigned int ba_size;
    unsigned int ba_cnt;
    unsigned int ba_rst;
};


// PULSE
//
struct IFpulseData : public IFtranData
{
    IFpulseData(double*, int, pbitList*);
    ~IFpulseData()
        {
            pbitList::destroy(td_pblist);
            delete [] td_parray;
        }

    static void parse(const char*, IFparseNode*, int*);

    double V1()      const { return (td_parms[0]); }
    double V2()      const { return (td_parms[1]); }
    double TD()      const { return (td_parms[2]); }
    double TR()      const { return (td_parms[3]); }
    double TF()      const { return (td_parms[4]); }
    double PW()      const { return (td_parms[5]); }
    double PER()     const { return (td_parms[6]); }
    void set_V1(double d)      { td_parms[0] = d; }
    void set_V2(double d)      { td_parms[1] = d; }
    void set_TD(double d)      { td_parms[2] = d; }
    void set_TR(double d)      { td_parms[3] = d; }
    void set_TF(double d)      { td_parms[4] = d; }
    void set_PW(double d)      { td_parms[5] = d; }
    void set_PER(double d)     { td_parms[6] = d; }

    void setup(sCKT*, double, double, bool);
    double eval_func(double);
    double eval_deriv(double);
    IFtranData *dup() const;
    void set_param(double, int);
    double *get_param(int);

private:
    pbitList *td_pblist;        // Pattern description.
    unsigned long *td_parray;   // Pattern bit field.
    int td_plen;                // Length of pattern.
    int td_prst;                // Reset point if repeating.
};

// GPULSE
//
struct IFgpulseData : public IFtranData
{
    IFgpulseData(double*, int, pbitList*);
    ~IFgpulseData()
        {
            pbitList::destroy(td_pblist);
            delete [] td_parray;
        }

    static void parse(const char*, IFparseNode*, int*);

    double V1()      const { return (td_parms[0]); }
    double V2()      const { return (td_parms[1]); }
    double TD()      const { return (td_parms[2]); }
    double GPW()     const { return (td_parms[3]); }
    double RPT()     const { return (td_parms[4]); }
    void set_V1(double d)      { td_parms[0] = d; }
    void set_V2(double d)      { td_parms[1] = d; }
    void set_TD(double d)      { td_parms[2] = d; }
    void set_GPW(double d)     { td_parms[3] = d; }
    void set_RPT(double d)     { td_parms[4] = d; }

    void setup(sCKT*, double, double, bool);
    double eval_func(double);
    double eval_deriv(double);
    IFtranData *dup() const;
    void set_param(double, int);
    double *get_param(int);

private:
    pbitList *td_pblist;        // Pattern description.
    unsigned long *td_parray;   // Pattern bit field.
    int td_plen;                // Length of pattern.
    int td_prst;                // Reset point if repeating.
};

struct IFpwlData : public IFtranData
{
    IFpwlData(double*, int, bool, int, double);

    static void parse(const char*, IFparseNode*, int*);

    void setup(sCKT*, double, double, bool);
    double eval_func(double);
    double eval_deriv(double);
    IFtranData *dup() const;
    void print(const char*, sLstr&);

private:
    bool td_pwlRgiven;      // true if pwl r=repeat given
    int td_pwlRstart;       // repeat start index, negative for time=0
    int td_pwlindex;        // index into pwl tran func array
    double td_pwldelay;     // pwl td value
};

// SIN
//
struct IFsinData : public IFtranData
{
    IFsinData(double*, int);

    static void parse(const char*, IFparseNode*, int*);

    double VO()      const { return (td_parms[0]); }
    double VA()      const { return (td_parms[1]); }
    double FREQ()    const { return (td_parms[2]); }
    double TDL()     const { return (td_parms[3]); }
    double THETA()   const { return (td_parms[4]); }
    double PHI()     const { return (td_parms[5]); }
    void set_VO(double d)      { td_parms[0] = d; }
    void set_VA(double d)      { td_parms[1] = d; }
    void set_FREQ(double d)    { td_parms[2] = d; }
    void set_TDL(double d)     { td_parms[3] = d; }
    void set_THETA(double d)   { td_parms[4] = d; }
    void set_PHI(double d)     { td_parms[5] = d; }

    void setup(sCKT*, double, double, bool);
    double eval_func(double);
    double eval_deriv(double);
    IFtranData *dup() const;
    void time_limit(const sCKT*, double*);
    void set_param(double, int);
    double *get_param(int);
};

// SPULSE
//
struct IFspulseData : public IFtranData
{
    IFspulseData(double*, int);

    static void parse(const char*, IFparseNode*, int*);

    double V1()      const { return (td_parms[0]); }
    double V2()      const { return (td_parms[1]); }
    double SPER()    const { return (td_parms[2]); }
    double SDEL()    const { return (td_parms[3]); }
    double THETA()   const { return (td_parms[4]); }
    void set_V1(double d)      { td_parms[0] = d; }
    void set_V2(double d)      { td_parms[1] = d; }
    void set_SPER(double d)    { td_parms[2] = d; }
    void set_SDEL(double d)    { td_parms[3] = d; }
    void set_THETA(double d)   { td_parms[4] = d; }

    void setup(sCKT*, double, double, bool);
    double eval_func(double);
    double eval_deriv(double);
    IFtranData *dup() const;
    void time_limit(const sCKT*, double*);
    void set_param(double, int);
    double *get_param(int);
};

// EXP
//
struct IFexpData : public IFtranData
{
    IFexpData(double*, int);

    static void parse(const char*, IFparseNode*, int*);

    double V1()      const { return (td_parms[0]); }
    double V2()      const { return (td_parms[1]); }
    double TD1()     const { return (td_parms[2]); }
    double TAU1()    const { return (td_parms[3]); }
    double TD2()     const { return (td_parms[4]); }
    double TAU2()    const { return (td_parms[5]); }
    void set_V1(double d)      { td_parms[0] = d; }
    void set_V2(double d)      { td_parms[1] = d; }
    void set_TD1(double d)     { td_parms[2] = d; }
    void set_TAU1(double d)    { td_parms[3] = d; }
    void set_TD2(double d)     { td_parms[4] = d; }
    void set_TAU2(double d)    { td_parms[5] = d; }

    void setup(sCKT*, double, double, bool);
    double eval_func(double);
    double eval_deriv(double);
    IFtranData *dup() const;
    void time_limit(const sCKT*, double*);
    void set_param(double, int);
    double *get_param(int);
};

struct IFsffmData : public IFtranData
{
    IFsffmData(double*, int);

    static void parse(const char*, IFparseNode*, int*);

    // SFFM
    double VO()      const { return (td_parms[0]); }
    double VA()      const { return (td_parms[1]); }
    double FC()      const { return (td_parms[2]); }
    double MDI()     const { return (td_parms[3]); }
    double FS()      const { return (td_parms[4]); }
    void set_VO(double d)      { td_parms[0] = d; }
    void set_VA(double d)      { td_parms[1] = d; }
    void set_FC(double d)      { td_parms[2] = d; }
    void set_MDI(double d)     { td_parms[3] = d; }
    void set_FS(double d)      { td_parms[4] = d; }

    void setup(sCKT*, double, double, bool);
    double eval_func(double);
    double eval_deriv(double);
    IFtranData *dup() const;
    void time_limit(const sCKT*, double*);
    void set_param(double, int);
    double *get_param(int);
};

// AM (following HSPICE)
//
struct IFamData : public IFtranData
{
    IFamData(double*, int);

    static void parse(const char*, IFparseNode*, int*);

    double SA()      const { return (td_parms[0]); }
    double OC()      const { return (td_parms[1]); }
    double MF()      const { return (td_parms[2]); }
    double CF()      const { return (td_parms[3]); }
    double DL()      const { return (td_parms[4]); }
    void set_SA(double d)      { td_parms[0] = d; }
    void set_OC(double d)      { td_parms[1] = d; }
    void set_MF(double d)      { td_parms[2] = d; }
    void set_CF(double d)      { td_parms[3] = d; }
    void set_DL(double d)      { td_parms[4] = d; }

    void setup(sCKT*, double, double, bool);
    double eval_func(double);
    double eval_deriv(double);
    IFtranData *dup() const;
    void time_limit(const sCKT*, double*);
    void set_param(double, int);
    double *get_param(int);
};

// GAUSS
//
struct IFgaussData : public IFtranData
{
    IFgaussData(double*, int);

    static void parse(const char*, IFparseNode*, int*);

    double SD()      const { return (td_parms[0]); }
    double MEAN()    const { return (td_parms[1]); }
    double LATTICE() const { return (td_parms[2]); }
    double ILEVEL()  const { return (td_parms[3]); }
    double LVAL()    const { return (td_parms[4]); }
    double VAL()     const { return (td_parms[5]); }
    double NVAL()    const { return (td_parms[6]); }
    double TIME()    const { return (td_parms[7]); }
    void set_SD(double d)      { td_parms[0] = d; }
    void set_MEAN(double d)    { td_parms[1] = d; }
    void set_LATTICE(double d) { td_parms[2] = d; }
    void set_ILEVEL(double d)  { td_parms[3] = d; }
    void set_LVAL(double d)    { td_parms[4] = d; }
    void set_VAL(double d)     { td_parms[5] = d; }
    void set_NVAL(double d)    { td_parms[6] = d; }
    void set_TIME(double d)    { td_parms[7] = d; }

    void setup(sCKT*, double, double, bool);
    double eval_func(double);
    double eval_deriv(double);
    IFtranData *dup() const;
    void set_param(double, int);
    double *get_param(int);
};

// INTERP
//
struct IFinterpData : public IFtranData
{
    IFinterpData(double*, int, double*);

    static void parse(const char*, IFparseNode*, int*);

    void setup(sCKT*, double, double, bool);
    double eval_func(double);
    double eval_deriv(double);
    IFtranData *dup() const;

private:
    int td_index;
};

#endif

