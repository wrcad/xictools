
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

#ifndef IFDATA_H
#define IFDATA_H

#include <string.h>
#include <ctype.h>
#include "errors.h"


//
// This file defines some basic data types used throughout the
// application.
//

// A function for memory reallocation of various objects, used in the
// device library and elsewhere.  allocated uninitialized memory is
// zeroed.
//
template<class T> inline void Realloc(T** oldp, unsigned int newsize,
    unsigned int oldsize)
{
    if (!oldp)
        return;
    if (newsize == 0) {
        delete [] *oldp;
        *oldp = 0;
        return;
    }
    if (newsize == oldsize)
        return;
    T *t = new T[newsize];
    if (*oldp && oldsize > 0) {
        if (newsize < oldsize)
            memcpy(t, *oldp, newsize*sizeof(T));
        else {
            memcpy(t, *oldp, oldsize*sizeof(T));
            memset(t + oldsize, 0, (newsize - oldsize)*sizeof(T));
        }

        // Zero out any pointers that might be freed in the destructor,
        // that we just copied.
        memset(*oldp, 0, oldsize*sizeof(T));

        delete [] *oldp;
    }
    else
        memset(t, 0, newsize*sizeof(T));
    *oldp = t;
}


// references
class Parser;
union  IFvalue;
struct variable;
struct IFdata;
struct IFdevice;
struct IFanalysis;
struct sGENmodel;
struct sGENinstance;
struct sCKT;
struct sCKTnode;
struct sJOB;
struct sTASK;
struct sLine;
struct sLstr;


// datatype: IFuid
//
// This object is a character pointer which points to an instance of
// a string kept in the symbol table.  All IFuid's that have the same
// text are therefor equal, so string comparison is avoided.
//
typedef const void *IFuid;


// datatype:   IFnode
//
typedef sCKTnode *IFnode;


// datatype:   IFcomplex
//
struct IFcomplex
{
    // No constructor, used in union.

    void set(double r, double i)
        {
            real = r;
            imag = i;
        }

    double real;
    double imag;
};


// Since IFcomplex is used in a union, it can't have a constructor.
//
struct cIFcomplex : public IFcomplex
{
    cIFcomplex(double r = 0.0, double i = 0.0) { real = r; imag = i; }

    cIFcomplex(const IFcomplex &c) { real = c.real; imag = c.imag; }

    cIFcomplex &operator=(const IFcomplex &c)
        {
            real = c.real;
            imag = c.imag;
            return (*this);
        }
};


struct IFparseNode;
struct sArgMap;

// Structure:   IFparseTree
//
// This structure is returned by the parser for a IF_PARSETREE valued
// parameter.
//
struct IFparseTree
{
    // Allocation block of IFparseNode structs.
    struct pn_block
    {
        pn_block(int, pn_block*);
        ~pn_block();

        pn_block *next;
        IFparseNode *block;
        int size;
    };

    // inpptree.cc
    IFparseTree(sCKT*, const char*);
    ~IFparseTree();

    static IFparseTree *getTree(const char**, sCKT*, const char* = 0,
        bool = false);

    IFparseNode *treeParse(const char**);
    bool differentiate();
    bool isConst();
    int eval(double*, double*, double*, int* = 0);
    void print(const char*);
    void varName(int, sLstr&);
    int initTranFuncs(double, double);
    double timeLim(double);
    IFparseNode *differentiate(IFparseNode*, int);
    IFparseNode *newNode();
    IFparseNode *newNnode(double);
    IFparseNode *newUnode(int, IFparseNode*);
    IFparseNode *newBnode(int, IFparseNode*, IFparseNode*);
    IFparseNode *newSnode(const char*);
    IFparseNode *newFnode(const char*, IFparseNode*);

    sCKT *ckt()                 { return (pt_ckt); }
    const char *line()          { return (pt_line); }
    IFparseNode *tree()         { return (pt_tree); }
    IFparseNode **derivs()      { return (pt_derivs); }
    IFdata *vars()              { return (pt_vars); }
    const char *xalias()        { return (pt_xalias); }
    IFparseNode *xtree()        { return (pt_xtree); }
    int num_vars()              { return (pt_num_vars); }
    int num_xvars()             { return (pt_num_xvars); }
    bool inMacro()              { return (pt_macro); }

    void set_vars(IFdata *v)    { pt_vars = v; }
    void set_num_vars(int n)    { pt_num_vars = n; }

    void set_derivs(int n, IFparseNode **d)
        {
            pt_num_vars = n;
            pt_derivs = d;
        }

private:
    void p_push_args(IFparseNode*, const char*);
    void p_pop_args();

    IFparseNode *p_mkcon(double);
    IFparseNode *p_mkb(int, IFparseNode*, IFparseNode*);
    IFparseNode *p_mkf(int, IFparseNode*);
    IFparseNode *p_mktrand(IFparseNode*, int);

    sCKT *pt_ckt;               // Circuit for tree, if any.
    char *pt_line;              // The parsed function string.
    IFparseNode *pt_tree;       // The real stuff.
    IFparseNode **pt_derivs;    // The derivative parse trees.
    IFdata *pt_vars;            // Array of structures describing values.
    char *pt_xalias;            // Token to replace "x".
    IFparseNode *pt_xtree;      // Parse tree for "x".
    int pt_num_vars;            // Number of variables used.
    int pt_num_xvars;           // Number of variables referenced for "x".
    sArgMap *pt_argmap;         // Temporary argument mapping for macros,
                                // used only when the macro contains a
                                // tran function so must be linked into
                                // the calling tree.

    // Memory management for IFparseNode.
    pn_block *pt_pn_blocks;     // Current allocation block.
    int pt_pn_used;             // Nodes allocated from current block.
    int pt_pn_size;             // Size of current block;

    bool pt_error;              // A parse or setup error occurred.
    bool pt_macro;
    // Set when parsing a macro.  In this case we skip the test for
    // placeholder nodes (which become macro arguments).
};


// Structure:   IFvalue
//
// Union used to pass the values corresponding to the dataType of an
// IFparm.
//
union IFvalue
{
    IFvalue() { memset(this, 0, sizeof(IFvalue)); }

    int iValue;             // integer or flag valued data
    double rValue;          // real valued data
    IFcomplex cValue;       // complex valued data
    const char *sValue;     // string valued data
    IFuid uValue;           // UID valued data
    IFnode nValue;          // node valued data
    IFparseTree *tValue;    // parse tree
    struct {
        int numValue;       // length of vector
        bool freeVec;       // receiver should free array
        union {
            const int *iVec;        // pointer to integer vector
            const double *rVec;     // pointer to real vector
            const IFcomplex *cVec;  // pointer to complex vector
            const char **sVec;      // pointer to string vector
            const IFuid *uVec;      // pointer to UID vector
            const IFnode *nVec;     // pointer to node vector
        } vec;
    } v;
};

// dataType values:
//
// Note:  The value is put together by ORing the appropriate bits from
// the fields below.
//
enum
{
    IF_NOTYPE       = 0x0,
    IF_FLAG         = 0x1,
    IF_INTEGER      = 0x2,
    IF_REAL         = 0x3,
    IF_COMPLEX      = 0x4,
    IF_NODE         = 0x5,
    IF_STRING       = 0x6,
    IF_INSTANCE     = 0x7,
    IF_PARSETREE    = 0x8,
    IF_TABLEVEC     = 0x9,

    IF_FLAGVEC      = 0x11,
    IF_INTVEC       = 0x12,
    IF_REALVEC      = 0x13,
    IF_CPLXVEC      = 0x14,
    IF_NODEVEC      = 0x15,
    IF_STRINGVEC    = 0x16,
    IF_INSTVEC      = 0x17
};

// Indicate a vector of the specified type
//
#define IF_VECTOR       0x10
#define IF_VARTYPES     0x1f

// Structure:   IFdata
//
// A data item, used to return parameter data.
//
struct IFdata
{
    IFdata() { type = IF_NOTYPE; }

    int toUU();         // Datatype translation.
    void cleanup();     // Garbage collect allocated arrays.

    int type;
    IFvalue v;
};


// structure:   IFparm
//
// This structure is used to keep a parameter description.  Parameters
// are passed to devices and models with an IFdata, which is the
// IFvalue union plus the type.  The type is needed in some cases
// where there is type redundancy, such as the case when a value can
// be a constant or a parse tree.
//
// IF_REQUIRED indicates that the parameter must be specified.
// IF_SET indicates that the specified item is an input parameter.
// IF_ASK indicates that the specified item is something the simulator
//    can provide information about.
// IF_SET and IF_ASK are NOT mutually exclusive.
// if IF_SET and IF_ASK are both zero, it indicates a parameter that
//    the simulator recoginizes as being a reasonable paremeter, but
//    which this simulator does not implement.
//
#define IF_ASK          0x200
#define IF_SET          0x400
#define IF_IO           0x600

#define IF_REQUIRED     0x800

// Codes for the units of the quantity returned from an ask
//
#define IF_VOLT         0x1000
#define IF_AMP          0x2000
#define IF_CHARGE       0x3000
#define IF_FLUX         0x4000
#define IF_CAP          0x5000
#define IF_IND          0x6000
#define IF_RES          0x7000
#define IF_COND         0x8000
#define IF_LEN          0x9000
#define IF_AREA         0xa000
#define IF_TEMP         0xb000
#define IF_FREQ         0xc000
#define IF_TIME         0xd000
#define IF_POWR         0xe000

#define IF_UNITS        0xf000

struct IFparm
{
    IFparm(const char *k, int i, int t, const char *d)
        {
            keyword = k;
            id = i;
            dataType = t;
            description = d;
        }

    variable *tovar(const IFvalue *pv)   // create a 'variable'
        { return (p_to_v(pv, dataType & IF_VARTYPES)); }
    variable *tovar(const IFdata *dv)    // create a 'variable'
        { return (p_to_v(&dv->v, dv->type & IF_VARTYPES)); }

    const char *keyword;        // parameter label
    int id;                     // unique identifier
    int dataType;               // a datatype identifier (see above)
    const char *description;    // text description of parameter

private:
    variable *p_to_v(const IFvalue*, int);
};


// Structure:   IFspecial
//
// Cache parameters for rapid access to internal device/model data.
//
struct IFspecial
{
    IFspecial()
        {
            reset();
        }

    void reset()
        {
            sp_inst = 0;
            sp_mod = 0;
            sp_job = 0;
            sp_an = 0;
            sp_parm = 0;
            sp_isset = false;
            sp_error = false;
        }

    // This function evaluates the special parameter, and fills this
    // struct so to avoid the search steps in the next evaluation.
    // If an error occurs, the error flag is set, and further evaluation
    // is skipped.
    //
    int evaluate(const char*, sCKT*, IFdata*, int);

    const sGENinstance *sp_inst;// device instance for parameter, if any
    const sGENmodel *sp_mod;    // model instance for parameter, if for model
    const sJOB *sp_job;         // job, if analysis or option parameter
    const IFanalysis *sp_an;    // analysis, if option, resource, or analysis
    IFparm *sp_parm;            // the parm descriptor (required)
    bool sp_isset;              // true if data are valid
    bool sp_error;              // set during evaluation if problems
};


// Structure: IFanalysis
//
// This structure serves as the base class for all analysis types.
//
struct IFanalysis
{
    IFanalysis()
        {
            name = 0;
            description = 0;

            numParms = 0;
            analysisParms = 0;
            domain = 0;
        }

    virtual ~IFanalysis() { }
    virtual sJOB *newAnal() = 0;
    virtual int parse(sLine*, sCKT*, int, const char**, sTASK*) = 0;
    virtual int setParm(sJOB*, int, IFdata*) = 0;
    virtual int askQuest(const sCKT*, const sJOB*, int, IFdata*) const = 0;
    virtual int anFunc(sCKT*, int) = 0;

    // These parse basic specifications, which may appear in string
    // describing more comples analysis.
    int parseDC(sLine*, sCKT*, const char**, sJOB*, int);
    int parseAC(sLine*, const char**, sJOB*);
    int parseTRAN(sLine*, const char**, sJOB*);

    // Data retrieval.
    variable *getOpt(const sCKT*, const sJOB*, const char*,
        IFspecial* = 0) const;
    int getOpt(const sCKT*, const sJOB*, const char*, IFdata*,
        IFspecial* = 0) const;

    const char *name;           // name of this analysis type
    const char *description;    // description of this type of analysis

    int numParms;               // number of analysis parameter descriptors
    IFparm *analysisParms;      // array  of analysis parameter descriptors
    int domain;

#define NODOMAIN        0
#define TIMEDOMAIN      1
#define FREQUENCYDOMAIN 2
#define SWEEPDOMAIN     3

    static IFanalysis *analysis(int i)
        {
            if (i >= 0 && i < num_analyses)
                return (analyses[i]);
            return (0);
        }

    static int numAnalyses()    { return (num_analyses); }

private:
    static IFanalysis *analyses[];
    static int num_analyses;
};


// sructure:  IFkeys
//
// This structure is used in the IFdevice structure below.  Each
// device can have multiple implementations, listed in the IFkeys
// structure.  Each implementation is keyed by the 'key' character in
// the input line.  minTerms is the minimum number of nodes in the
// implementation, maxTerms is the maximum number of nodes, which is
// equal to minTerms if the number of nodes is fixed.  termNames is a
// list of terminal names, at least maxTerms long.  numDevs is the
// number of controlling devices (as for a current controlled source).
//
struct IFkeys
{
    IFkeys(int k, const char **tn, int mn, int mx, int nd)
        {
            key = k;
            termNames = tn;
            minTerms = mn;
            maxTerms = mx;
            numDevs = nd;
        }

    int key;            // keying character in input
    const char **termNames; // pointer to array of pointers to names
    int minTerms;       // minimum number of terminals in this type
    int maxTerms;       // maximum number of terminals in this type
    int numDevs;        // number of controlling devices
};


// references
struct sNdata;
struct sLine;
struct sHtab;

// How many separate levels are supported by device.
#define NUM_DEV_LEVELS 8

// Passed of device backup function.
enum DEV_BKMODE { DEV_CLEAR, DEV_SAVE, DEV_RESTORE };

// Flags for IDdevice::dv_flags
//
#define DV_TRUNC    0x1     // device uses numerical integration
#define DV_NOAC     0x2     // no AC analysis with this device
#define DV_NODCO    0x4     // no DCO analysis with this device
#define DV_NODCT    0x8     // no DCT analysis with this device
#define DV_NOTRAN   0x10    // no transient analysis with this device
#define DV_NOLEVCHG 0x20    // device levels can't be changed
#define DV_VLAMS    0x40    // device compiled from Verilog-A/AMS source
#define DV_JJSTEP   0x80    // device limits time step via Josephson eqn.
#define DV_JJPMDC   0x100   // device uses scaled phase in DC analysis
#define DV_NONOIS   0x200   // no NOISE analysis with this device
#define DV_NOPZ     0x400   // no PZ analysis with this device
#define DV_NODIST   0x800   // no DISTO analysis with this device

// structure:  IFdevice
//
// This structure serves as the base class for all devices.
//
struct IFdevice
{
    IFdevice()
        {
            dv_name = 0;
            dv_description = 0;
            dv_module = 0;

            dv_numKeys = 0;
            dv_keys = 0;

            memset(dv_levels, 0, NUM_DEV_LEVELS);
            dv_modelKeys = 0;

            dv_numInstanceParms = 0;
            dv_instanceParms = 0;

            dv_numModelParms = 0;
            dv_modelParms = 0;

            dv_flags = 0;

            dv_instTab = 0;
            dv_modlTab = 0;
        }

    virtual ~IFdevice() { }

    // Return the IFkey* if c is a device key, case insensitive.
    //
    IFkeys *keyMatch(int c)
        {
            if (isupper(c))
                c = tolower(c);
            for (int i = 0; i < dv_numKeys; i++) {
                int d = dv_keys[i].key;
                if (c == (isupper(d) ? tolower(d) : d))
                    return (&dv_keys[i]);
            }
            return (0);
        }

    // Return true if nm matches a model name key, case insensitive.
    //
    bool modkeyMatch(const char *nm) const
        {
            if (nm && dv_modelKeys) {
                for (const char **s = dv_modelKeys; *s; s++) {
                    if (!strcasecmp(nm, *s))
                        return (true);
                }
            }
            return (false);
        }

    // Return true if a there is a match to the module name.
    //
    bool moduleNameMatch(const char *nm) const
        {
            if (!dv_module || !nm)
                return (false);
            return (!strcasecmp(dv_module, nm));
        }

    // These represent the traditional SPICE (Spice3) device key
    // assumptions.  Additionally:
    //   n    unused, but silently ignored as a "null device"
    //   p    unused
    //   x    subcircuit call
    //   y    unused

    bool isASRC() const
        { return (dv_keys && (dv_keys->key == 'a' || dv_keys->key == 'A')); }
    bool isJJ() const
        { return (dv_keys && (dv_keys->key == 'b' || dv_keys->key == 'B')); }
    bool isCAP() const
        { return (dv_keys && (dv_keys->key == 'c' || dv_keys->key == 'C')); }
    bool isDIO() const
        { return (dv_keys && (dv_keys->key == 'd' || dv_keys->key == 'D')); }
    bool isVCVS() const
        { return (dv_keys && (dv_keys->key == 'e' || dv_keys->key == 'E')); }
    bool isCCCS() const
        { return (dv_keys && (dv_keys->key == 'f' || dv_keys->key == 'F')); }
    bool isVCCS() const
        { return (dv_keys && (dv_keys->key == 'g' || dv_keys->key == 'G')); }
    bool isCCVS() const
        { return (dv_keys && (dv_keys->key == 'h' || dv_keys->key == 'H')); }
    bool isISRC() const
        { return (dv_keys && (dv_keys->key == 'i' || dv_keys->key == 'I')); }
    bool isJFET() const
        { return (dv_keys && (dv_keys->key == 'j' || dv_keys->key == 'J')); }
    bool isMUT() const
        { return (dv_keys && (dv_keys->key == 'k' || dv_keys->key == 'K')); }
    bool isIND() const
        { return (dv_keys && (dv_keys->key == 'l' || dv_keys->key == 'L')); }
    bool isMOS() const
        { return (dv_keys && (dv_keys->key == 'm' || dv_keys->key == 'M')); }
    bool isLTRA() const
        { return (dv_keys && (dv_keys->key == 'o' || dv_keys->key == 'O')); }
    bool isBJT() const
        { return (dv_keys && (dv_keys->key == 'q' || dv_keys->key == 'Q')); }
    bool isRES() const
        { return (dv_keys && (dv_keys->key == 'r' || dv_keys->key == 'R')); }
    bool isSW() const
        { return (dv_keys && (dv_keys->key == 's' || dv_keys->key == 'S')); }
    bool isTRA() const
        { return (dv_keys && (dv_keys->key == 't' || dv_keys->key == 'T')); }
    bool isURC() const
        { return (dv_keys && (dv_keys->key == 'u' || dv_keys->key == 'U')); }
    bool isVSRC() const
        { return (dv_keys && (dv_keys->key == 'v' || dv_keys->key == 'V')); }
    bool isCSW() const
        { return (dv_keys && (dv_keys->key == 'w' || dv_keys->key == 'W')); }
    bool isMES() const
        { return (dv_keys && (dv_keys->key == 'z' || dv_keys->key == 'Z')); }

    // Return true if the lev matches one of the supported levels. 
    // Note that levels 0 and 1 are always equivalent.  Passing lev =
    // 0 means that the "level" parameter was not given.  The level 1
    // device is therefor the default.
    //
    bool levelMatch(int lev) const
        {

            if (dv_levels[0] <= 1 && lev <= 1)
                return (true);
            for (int k = 0; k < NUM_DEV_LEVELS && dv_levels[k]; k++) {
                if (dv_levels[k] == (unsigned)lev)
                    return (true);
            }
            return (false);
        }

    IFparm *findInstanceParm(const char*, int);
    IFparm *findModelParm(const char*, int);

    virtual sGENmodel *newModl()                    { return (0); };
    virtual sGENinstance *newInst()                 { return (0); };

    // The base classes for models and instances, sGENmodel and
    // sGENinstance, do *NOT* provide virtual destructors or virtual
    // functions at all.  In the original C code, these structs were
    // allocated with calloc.  In the present device library, we use
    // operator new, and memset the entire object to 0.  This would
    // zero any virtual function thunks.
    //
    // The deallocation functions here should be used to destroy these
    // device objects.  Operater delete can be used only on the
    // derived object classes.
    //
    // Every library device should instantiate these three functions:

    // destroy();      destroy all models and instances
    virtual int destroy(sGENmodel**)                        { return (OK); }

    // delInst();      delete an instance
    virtual int delInst(sGENmodel*, IFuid, sGENinstance*)   { return (OK); }

    // delModl();      delete a model and all instances
    virtual int delModl(sGENmodel**, IFuid, sGENmodel*)     { return (OK); }

    // These are easily implemented with the following templates. 
    // These are instantiated in the device library to implement the
    // destroy functions.

    // This function deletes all typed models and instances from the
    // circuit and frees the storage they were using.
    //
    template <class T, class U> int
    destroy(sGENmodel **model)
    {
        T *nextmod;
        for (T *mod = (T*)*model; mod; mod = nextmod) {
            nextmod = (T*)mod->GENnextModel;
            U *inst, *next;
            for (inst = (U*)mod->GENinstances; inst; inst = next) {
                next = (U*)inst->GENnextInstance;
                delete inst;
            }
            delete mod;
        }
        *model = 0;
        return (OK);
    }

    // This function deletes an instance from the circuit and frees
    // the storage it was using.
    //
    template <class T, class U> int
    delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
    {
        for (T *mod = (T*)model; mod; mod = (T*)mod->GENnextModel) {
            sGENinstance **prev = &(mod->GENinstances);
            for (U *inst = (U*)*prev; inst; inst = (U*)*prev) {
                if (inst->GENname == dname || (fast && inst == (U*)fast)) {
                    *prev = inst->GENnextInstance;
                    delete inst;
                    return (OK);
                }
                prev = &inst->GENnextInstance;
            }
        }
        return (E_NODEV);
    }

    // This function deletes a typed model and its instances from the
    // circuit and frees the storage used.
    //
    template <class T, class U> int
    delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
    {
        T *mprv = 0, *mnxt;
        for (T *mod = (T*)*model; mod; mod = mnxt) {
            mnxt = (T*)mod->GENnextModel;
            if (mod->GENmodName == modname || 
                    (modfast && mod == (T*)modfast)) {
                if (mprv)
                    mprv->GENnextModel = mnxt;
                else
                    *model = (sGENmodel*)mnxt;

                U *inst, *next;
                for (inst = (U*)mod->GENinstances; inst; inst = next) {
                    next = (U*)inst->GENnextInstance;
                    delete inst;
                }
                delete mod;
                return (OK);
            }
            mprv = mod;
        }
        return (E_NOMOD);
    }


    // parse();        Parse the text containing instance spec.
    virtual void parse(int, sCKT*, sLine*)              { };

#define LOAD_SKIP_FLAG 0x100
    // loadTest();     Test if instance requires loading,
    // or special treatment.
    // Returns LOAD_SKIP_FLAG if this instance does not require a
    // load() call.  This is true when preloading is enabled for
    // simple linear resistors and sources.
    virtual int loadTest(sGENinstance*, sCKT*)          { return (0); };

    // load();         Load the device into the matrix.
    // Unlike other functions, this is called per-instance, as we may
    // want to parcel out the loading to different threads.  If
    // LOAD_SKIP_FLAG is returned, we can skip loading the remaining
    // devices in the model.
    virtual int load(sGENinstance*, sCKT*)              { return (OK); };

    // setup();        Initialize devices before soloution begins.
    virtual int setup(sGENmodel*, sCKT*, int*)          { return (OK); };

    // unsetup();      Undo the setup, call before recalling setup.
    virtual int unsetup(sGENmodel*, sCKT*)              { return (OK); }

    // resetup();      Reset all pointers to matrix elements.
    // All devices must implement this.
    virtual int resetup(sGENmodel*, sCKT*) = 0;

    // temperature();  Do temperature dependent setup processing.
    virtual int temperature(sGENmodel*, sCKT*)          { return (OK); };

    // getic();        Pick up device init conds from rhs.
    virtual int getic(sGENmodel*, sCKT*)                { return (OK); };

    // accept();       Call on acceptance of a timepoint.
    virtual int accept(sCKT*, sGENmodel*)               { return (OK); };

    // trunc();        Perform truncation error calc.
    virtual int trunc(sGENmodel*, sCKT*, double*)       { return (OK); };

    // convTest();     Convergence test function.
    virtual int convTest(sGENmodel*, sCKT*)             { return (OK); };

    // backup();       Save/restore device state.
    virtual void backup(sGENmodel*, DEV_BKMODE)         { }

    // setInst();      Input a parameter to a device instance.
    virtual int setInst(int, IFdata*, sGENinstance*)    { return (OK); };

    // setModl();      Input a paramater to a model.
    virtual int setModl(int, IFdata*, sGENmodel*)       { return (OK); };

    // askInst();      Ask about device details.
    virtual int askInst(const sCKT*, const sGENinstance*,
        int, IFdata*)                                   { return (OK); };

    // askModl();      Ask about model details.
    virtual int askModl(const sGENmodel*, int,
        IFdata*)                                        { return (OK); };

    // findBranch();   Search for device branch eqns.
    virtual int findBranch(sCKT*, sGENmodel*, IFuid)    { return (OK); };

    // acLoad();       AC analysis loading function.
    virtual int acLoad(sGENmodel*, sCKT*)               { return (OK); };

    // pzLoad();       Load for pole-zero analysis.
    virtual int pzLoad(sGENmodel*, sCKT*,
        IFcomplex*)                                     { return (OK); };

    // disto();        Do distortion operations.
    virtual int disto(int, sGENmodel*, sCKT*)           { return (OK); };

    // noise();        Noise function.
    virtual int noise(int, int, sGENmodel*, sCKT*,
        sNdata*, double*)                               { return (OK); };

    // initTranFuncs() Initialize "tran" funcs for analysis.
    virtual void initTranFuncs(sGENmodel*, double, double)   { }

    const char *name()          const { return (dv_name); }
    const char *description()   const { return (dv_description); }
    int numKeys()               const { return (dv_numKeys); }
    int flags()                 const { return (dv_flags); }

    const IFkeys *key(int i) const
        {
            if (i >= 0 && i < dv_numKeys)
                return (dv_keys + i);
            return (0);
        }

    int level(int i) const
        {
            if (i >= 0 && i < NUM_DEV_LEVELS)
                return (dv_levels[i]);
            return (0);
        }

    int setLevel(int i, int lev)
        {
            if (i < 0 || i >= NUM_DEV_LEVELS)
                return (-1);
            if (dv_flags & DV_NOLEVCHG)
                return (-1);
            int tlv = dv_levels[i];
            dv_levels[i] = lev;
            return (tlv);
        }

    IFparm *instanceParm(int i) const
        {
            if (i >= 0 && i < dv_numInstanceParms)
                return (dv_instanceParms + i);
            return (0);
        }

    IFparm *modelParm(int i) const
        {
            if (i >= 0 && i < dv_numModelParms)
                return (dv_modelParms + i);
            return (0);
        }

     const char *modelKey(int i) const
        {
            if (dv_modelKeys) {
                for (int j = 0; ; j++) {
                    if (!dv_modelKeys[j])
                        break;
                    if (j == i)
                        return (dv_modelKeys[j]);
                }
            }
            return (0);
        }

    // for sensgen.cc
    void getPtable(int *np, IFparm **p, bool mod)
        {
            if (mod) {
                *np = dv_numModelParms;
                *p = dv_modelParms;
            }
            else {
                *np = dv_numInstanceParms;
                *p = dv_instanceParms;
            }
        }

protected:
    const char *dv_name;        // Name of this type of device.
    const char *dv_description; // Description of this type of device.
    const char *dv_module;      // Module name for loadable module.

    int dv_numKeys;             // Number of implementations.
    IFkeys *dv_keys;            // List of device implementations.

    unsigned char dv_levels[NUM_DEV_LEVELS]; // List of model levels supported.
    const char **dv_modelKeys;  // List of model names, zero terminated.

    int dv_numInstanceParms;    // Number of instance parameter descriptors.
    IFparm *dv_instanceParms;   // Array  of instance parameter descriptors.

    int dv_numModelParms;       // Number of model parameter descriptors.
    IFparm *dv_modelParms;      // Array  of model parameter descriptors.

    unsigned dv_flags;          // Some info about the device.

    sHtab *dv_instTab;          // Hash table for instance keywords.
    sHtab *dv_modlTab;          // Hash table for model keywords.
};

#endif // IFDATA_H

