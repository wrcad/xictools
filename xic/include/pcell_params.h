
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2011 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: pcell_params.h,v 5.5 2016/03/10 20:32:56 stevew Exp $
 *========================================================================*/

#ifndef PCELL_PARAMS_H
#define PCELL_PARAMS_H

//
// Elements for a PCell database.
//


// Phony OA library name for Xic native PCells.  It is bad news if
// a library of this name exists in the OA system.
//
#define XIC_NATIVE_LIBNAME "[xic]"

// Syntax elements of the XICP_PC_PARAMS property string.
// [typechar:]name [=] value[:constraint] [,]
#define PCP_TYPE_SEP ':'
#define PCP_CONS_SEP ':'
#define PCP_WORD_SEP ','

// PCell parameter types.
//
enum PCPtype
{
    PCPbool,
    PCPint,
    PCPtime,
    PCPfloat,
    PCPdouble,
    PCPstring,
    PCPappType
};

// Constraint types.
enum PCtype { PCchoice, PCrange, PCstep, PCnumStep, PCunknown };

// Actions.
enum PCAtype { PCAreject, PCAaccept, PCAdefault };

struct PCellParam;


// A PCell constraint evaluation class.  This supports Ciranova
// Python constraint strings.
//
struct PConstraint
{
    PConstraint();

    ~PConstraint()
        {
            stringlist::destroy(pc_choice_list);
            delete [] pc_scale;
        }
        
    bool checkConstraint(const PCellParam*) const;
    bool checkConstraint(double) const;
    bool checkConstraint(const char*) const;
    bool parseConstraint(const char**);

    static double factor(const char*);

    PCtype type()           const { return (pc_type); }
    PCAtype action()        const { return (pc_action); }
    stringlist *choices()   const { return (pc_choice_list); }
    const char *scale()     const { return (pc_scale); }
    double low()            const { return (pc_low); }
    double high()           const { return (pc_high); }
    double resol()          const { return (pc_resol); }
    double step()           const { return (pc_step); }
    double start()          const { return (pc_start); }
    double limit()          const { return (pc_limit); }
    double scale_factor()   const { return (pc_sfact); }
    bool low_none()         const { return (pc_low_none); }
    bool high_none()        const { return (pc_high_none); }
    bool resol_none()       const { return (pc_resol_none); }
    bool step_none()        const { return (pc_step_none); }
    bool start_none()       const { return (pc_start_none); }
    bool limit_none()       const { return (pc_limit_none); }

private:
    bool parse_argument(const char**, const char*);
    bool parse_list(const char**);
    bool parse_action(const char**);
    char *parse_quoted(const char**);
    bool parse_number(const char**, double*, bool*);

    PCtype pc_type;
    PCAtype pc_action;
    stringlist *pc_choice_list;
    char *pc_scale;

    double pc_low;
    double pc_high;
    double pc_resol;
    double pc_step;
    double pc_start;
    double pc_limit;
    double pc_sfact;

    bool pc_low_none;
    bool pc_high_none;
    bool pc_resol_none;
    bool pc_step_none;
    bool pc_start_none;
    bool pc_limit_none;
};


// List element for a PCell parameter.
//
struct PCellParam
{
    // PCPbool
    PCellParam(PCPtype t, const char *n, const char *c, bool b)
        {
            init(t, n, c);
            u.ival = b;
        }

    // PCPint, PCPtime
    PCellParam(PCPtype t, const char *n, const char *c, long i)
        {
            init(t, n, c);
            u.ival = i;
        }

    // PCPfloat
    PCellParam(PCPtype t, const char *n, const char *c, float f)
        {
            init(t, n, c);
            u.fval = f;
        }

    // PCPdouble
    PCellParam(PCPtype t, const char *n, const char *c, double d)
        {
            init(t, n, c);
            u.dval = d;
        }

    // PCPstring
    PCellParam(PCPtype t, const char *n, const char *c, const char *s)
        {
            init(t, n, c);
            u.sval = s ? lstring::copy(s) : 0;
        }

    // PCPappType
    PCellParam(PCPtype t, const char *n, const char *c, const char *tn,
        const unsigned char *a, unsigned int sz)
        {
            init(t, n, c);
            p_appsize = sz;
            p_appname = lstring::copy(tn);
            if (sz) {
                u.aval = new unsigned char[sz+1];
                memcpy(u.aval, a, sz);
                u.aval[sz] = 0;
            }
            else
                u.aval = 0;
        }

    ~PCellParam()
        {
            delete [] p_name;
            delete [] p_appname;
            delete [] p_constr_str;
            delete p_constr;
            if (p_type == PCPstring)
                delete [] u.sval;
            if (p_type == PCPappType)
                delete [] u.aval;
        }

    void free()
        {
            PCellParam *p = this;
            while (p) {
                PCellParam *px = p;
                p = p->p_next;
                delete px;
            }
        }

    PCellParam(char, const char*, const char*, const char*);

    const char *typestr();
    bool set(const PCellParam*);
    PCellParam *dup() const;
    void setup(const PCellParam*);
    void reset(const PCellParam*);
    bool setValue(const char*, double, const char* = 0);
    bool setValue(const char*, const char*, const char* = 0);
    char *getValue() const;
    char *getValueByName(const char*) const;
    char *getAssignment() const;
    char *getTCLassignment() const;
    bool update(const char*);
    char *this_string(bool) const;
    char *string(bool) const;
    char *digest() const;
    void print() const;

    static bool getPair(const char**, char*, char**, char**, char**);
    static bool parseParams(const char*, PCellParam**);

    PCellParam *find(const char *nm)
        {
            if (!nm)
                return (0);
            PCellParam *p = this;
            for ( ; p; p = p->next()) {
                if (!strcmp(p->name(), nm))
                    return (p);
            }
            return (0);
        }

    const PCellParam *find_c(const char *nm) const
        {
            if (!nm)
                return (0);
            const PCellParam *p = this;
            for ( ; p; p = p->next()) {
                if (!strcmp(p->name(), nm))
                    return (p);
            }
            return (0);
        }

    PCellParam *next()              const { return (p_next); }
    void setNext(PCellParam *n)     { p_next = n; }
    const char *name()              const { return (p_name); }
    const char *constraint_string() const { return (p_constr_str); }
    const PConstraint *constraint() const { return (p_constr); }
    PCPtype type()                  const { return (p_type); }
    bool changed()                  const { return (p_changed); }

    bool boolVal()                  const { return (u.bval); }
    void setBoolVal(bool b)         { u.bval = b; p_changed = true; }
    long intVal()                   const { return (u.ival); }
    void setIntVal(long i)          { u.ival = i; p_changed = true; }
    time_t timeVal()                const { return (u.ival); }
    void setTimeVal(time_t t)       { u.ival = t; p_changed = true; }
    float floatVal()                const { return (u.fval); }
    void setFloatVal(float f)       { u.fval = f; p_changed = true; }
    double doubleVal()              const { return (u.dval); }
    void setDoubleVal(double d)     { u.dval = d; p_changed = true; }

    const char *stringVal()         const { return (u.sval); }
    void setStringVal(const char *s)
        {
            delete [] u.sval;
            u.sval = s ? lstring::copy(s) : 0;
            p_changed = true;
        }

    unsigned int appValSize()       const { return (p_appsize); }
    const unsigned char *appVal()   const { return (u.aval); }
    void setAppVal(const unsigned char *a, unsigned int sz)
        {
            delete [] u.aval;
            if  (sz) {
                u.aval = new unsigned char[sz+1];
                memcpy(u.aval, a, sz);
                u.aval[sz] = 0;
            }
            else
                u.aval = 0;
            p_changed = true;
        }
    const char *appName()           const { return (p_appname); }
    void setAppName(const char *n)
        {
            char *s = lstring::copy(n);
            delete [] p_appname;
            p_appname = s;
        }

private:
    void init(PCPtype t, const char *n, const char *c)
        {
            p_next = 0;
            p_name = lstring::copy(n);
            p_appname = 0;
            if (c) {
                while (isspace(*c))
                    c++;
                if (!*c)
                    c = 0;
            }
            p_constr_str = lstring::copy(c);
            init_constraint();
            p_type = t;
            p_appsize = 0;
            p_changed = false;
        }

    void init_constraint();

    PCellParam *p_next;
    char *p_name;           // Parameter name.
    char *p_appname;        // Name for app data.
    char *p_constr_str;     // Ciranova constraint spec.
    PConstraint *p_constr;  // Constraint reperesentation.
    PCPtype p_type;         // Parameter type.
    unsigned int p_appsize; // Size of app data.
    bool p_changed;         // Set true when new value given.
    union {
        double dval;
        char *sval;
        unsigned char *aval;
        long ival;
        float fval;
        bool bval;
    } u;
};

bool operator== (const PCellParam&, const PCellParam&);



// Instantiation descriptor.  We keep a list of these, one for each
// unique set of instantiation parameters, for a given super-master.
//
struct PCellItem
{
    PCellItem(const PCellParam *pm)
        {
            pi_next = 0;
            pi_params = pm->dup();
            pi_cellname = 0;
            pi_index = 0;
        }

    ~PCellItem()
        {
            pi_params->free();
            delete [] pi_cellname;
        }

    void free()
        {
            PCellItem *p0 = this;
            while (p0) {
                PCellItem *px = p0;
                p0 = p0->pi_next;
                delete px;
            }
        }

    PCellItem *next()               const { return (pi_next); }
    void setNext(PCellItem *n)      { pi_next = n; }

    const PCellParam *params()      const { return (pi_params); }
    void setParams(const PCellParam *p)
        {
            pi_params->free();
            pi_params = p->dup();
        }

    unsigned int index()            const { return (pi_index); }
    void setIndex(unsigned int i)   { pi_index = i; }

    const char *cellname()          const { return (pi_cellname); }
    void setCellname(const char *n)
        {
            char *nn = lstring::copy(n);
            delete [] pi_cellname;
            pi_cellname = nn;
        }

private:
    PCellItem *pi_next;
    PCellParam *pi_params;
    char *pi_cellname;
    unsigned int pi_index;
};


// Descriptor for a unique super-master, saved as data in a hash
// table.
//
struct PCellDesc
{
    // A convenience thing to manage lib/cell/view name deletion.
    struct LCVcleanup
    {
        LCVcleanup(char *libname, char *cellname, char *viewname)
            {
                lcv_libname = libname;
                lcv_cellname = cellname;
                lcv_viewname = viewname;
            }

        ~LCVcleanup()
            {
                delete [] lcv_libname;
                delete [] lcv_cellname;
                delete [] lcv_viewname;
            }
    private:
        char *lcv_libname;
        char *lcv_cellname;
        char *lcv_viewname;
    };

    PCellDesc(const char *nm, PCellParam *dp)
        {
            pd_dbname = lstring::copy(nm);
            pd_instances = 0;
            pd_defprms = dp;
        }

    ~PCellDesc()
        {
            delete [] pd_dbname;
            pd_instances->free();
            pd_defprms->free();
        }

    PCellItem *findItem(const PCellParam *prms) const
        {
            for (PCellItem *pi = pd_instances; pi; pi = pi->next()) {
                if (*pi->params() == *prms)
                    return (pi);
            }
            return (0);
        }

    void addItem(PCellItem *pi)
        {
            if (!pd_instances) {
                pd_instances = pi;
                pi->setIndex(1);
            }
            else {
                pi->setIndex(pd_instances->index() + 1);
                pi->setNext(pd_instances);
                pd_instances = pi;
            }
        }

    bool removeItem(const PCellItem *pi)
        {
            PCellItem *pp = 0;
            for (PCellItem *p = pd_instances; p; p = p->next()) {
                if (p == pi) {
                    if (pp)
                        pp->setNext(p->next());
                    else
                        pd_instances = p->next();
                    delete p;
                    return (true);
                }
                pp = p;
            }
            return (false);
        }

    const char *dbname()                const { return (pd_dbname); }
    const PCellParam *defaultParams()   const { return (pd_defprms); }

    char *cellname(const char*, const PCellItem*) const;

    static char *mk_dbname(const char*, const char*, const char*);
    static char *mk_native_dbname(const char*);
    static bool split_dbname(const char*, char**, char**, char**);

    // Return true if the string is a "dbname", which here means that
    // either the string is null, or it begins with '<'.
    //
    static bool is_dbname(const char *n) { return (!n || (*n == '<')); }

    // Return the canonical database name, assuming that strings that
    // dont start with '<' are plain cell names of native
    // super-masters.  this is useful for processing the XICP_PC
    // property string.
    //
    static char *canon(const char *dbname)
        {
            if (is_dbname(dbname))
                return (lstring::copy(dbname));
            return (mk_native_dbname(dbname));
        }

private:
    char *pd_dbname;
    PCellItem *pd_instances;
    PCellParam *pd_defprms;
};


#endif

