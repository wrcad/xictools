
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef RUNOP_H
#define RUNOP_H


//
// Definitions for the "runops" (analysis tracing, stop on condition,
// etc).
//

struct sRunDesc;

enum ROtype
{
    RO_NONE,
    RO_SAVE,                // Save a node.
    RO_TRACE,               // Print the value of a node every iteration.
    RO_IPLOT,               // Incrementally plot listed vectors.
    RO_IPLOTALL,            // Incrementally plot everything.
    RO_DEADIPLOT,           // Iplot is being destroyed.
    RO_STOPAFTER,           // Break after this many iterations.
    RO_STOPAT,              // Break at this many iterations.
    RO_STOPBEFORE,          // Break before this many iterations.
    RO_STOPWHEN,            // Break when a node reaches this value.
    RO_MEASURE              // Perform a measurement.
};

// Call function returns.
enum { CB_OK, CB_PAUSE, CB_ENDIT };

// Returns from runop test, should match call function returns;
enum ROret
{
    RO_OK = CB_OK,  // Proceed with run.
    RO_PAUSE,       // Pause run, can be resumed.
    RO_ENDIT        // Abort run, can not be resumed.
};

// Base class, not instantiated directy.  This contains common fields.
struct sRunop
{
    sRunop()
        {
            ro_next     = 0;
            ro_type     = RO_NONE;
            ro_number   = 0;
            ro_string   = 0;
            ro_active   = false;
            ro_bad      = false;
        }

    virtual ~sRunop()
        {
            delete [] ro_string;
        }

    static void destroy_list(sRunop *l)
        {
            while (l) {
                sRunop *x = l;
                l = l->ro_next;
                x->destroy();
            }
        }

    void set_next(sRunop *d)    { ro_next = d; }

    ROtype type()               { return (ro_type); }
    void set_type(ROtype t)     { ro_type = t; }

    int number()                { return (ro_number); }
    void set_number(int i)      { ro_number = i; }

    const char *string()        { return (ro_string); }
    void set_string(char *s)    { ro_string = s; }

    bool active()               { return (ro_active); }
    void set_active(bool b)     { ro_active = b; }

    bool bad()                  { return (ro_bad); }
    void set_bad(bool b)        { ro_bad = b; }

    virtual void print(char**) = 0;
    virtual void destroy() = 0;

protected:
    sRunop *ro_next;            // List of active runop commands.
    ROtype ro_type;             // One of the above.
    int ro_number;              // The number of this runop command.
    char *ro_string;            // Condition or node, text.
    bool ro_active;             // True if active.
    bool ro_bad;                // True if error.
};

// Save an expression when running.
struct sRunopSave : public sRunop
{
    sRunopSave()
        {
            ro_type = RO_SAVE;
        }

    sRunopSave *next()          { return ((sRunopSave*)ro_next); }

    void print(char**);         // Print, in string if given, the runop msg.
    void destroy();             // Destroy this runop.
};

// Trace an expression (print value of at each time point) when running.
struct sRunopTrace : public sRunop
{
    sRunopTrace()
        {
            ro_type = RO_TRACE;
        }

    sRunopTrace *next()         { return ((sRunopTrace*)ro_next); }

    void print(char**);         // Print, in string if given, the runop msg.
    void destroy();             // Destroy this runop.

    bool print_trace(sPlot*, bool*, int);  // Print trace output.
};

// Plot incrementally while running.
struct sRunopIplot : public sRunop
{
    sRunopIplot()
        {
            ro_type = RO_IPLOT;
        }

    sRunopIplot *next()         { return ((sRunopIplot*)ro_next); }

    int graphid()               { return (ro_graphid); }
    void set_graphid(int i)     { ro_graphid = i; }

    int reuseid()               { return (ro_reuseid); }
    void set_reuseid(int i)     { ro_reuseid = i; }

    void print(char**);         // Print, in string if given, the runop msg.
    void destroy();             // Destroy this runop.

private:
    int ro_graphid;             // If iplot, id of graph.
    int ro_reuseid;             // Iplot window to reuse.
};

// Check for breakout condition while running.
struct sRunopStop : public sRunop
{
    sRunopStop()
        {
            ro_type     = RO_STOPAFTER;

            ro_call_flag    = false;
            ro_ptmode       = false;
            ro_p.dpoint     = 0.0;
            ro_call         = 0;
            ro_also         = 0;
            ro_index        = 0;
            ro_numpts       = 0;
            ro_a.dpoints    = 0;
        }

    ~sRunopStop()
        {
            delete [] ro_call;
            if (ro_ptmode)
                delete [] ro_a.ipoints;
            else
                delete [] ro_a.dpoints;
        }

    sRunopStop *next()          { return ((sRunopStop*)ro_next); }

    void set_point(double d)
        {
            ro_p.dpoint = d;
            ro_ptmode = false;
        }

    void set_point(int i)
        {
            ro_p.ipoint = i;
            ro_ptmode = true;
        }

    bool call_flag()            { return (ro_call_flag); }
    const char *call_func()     { return (ro_call_flag ? ro_call : 0); }
    void set_call(bool b, const char *fn)
        {
            ro_call_flag = b;
            char *nm = lstring::copy(fn);
            delete [] ro_call;
            ro_call = nm;
        }

    sRunopStop *also()          { return (ro_also); }
    void set_also(sRunopStop *d){ ro_also = d; }

    void set_index(int n)       { ro_index = n; }
    void set_points(int sz, double *p)
        {
            ro_numpts = sz;
            ro_index = 0;
            if (ro_ptmode)
                delete [] ro_a.ipoints;
            else
                delete [] ro_a.dpoints;
            ro_a.dpoints = p;
            ro_ptmode = false;
        }

    void set_points(int sz, int *p)
        {
            ro_numpts = sz;
            ro_index = 0;
            if (ro_ptmode)
                delete [] ro_a.ipoints;
            else
                delete [] ro_a.dpoints;
            ro_a.ipoints = p;
            ro_ptmode = true;
        }

    void print(char**);         // Print, in string if given, the runop msg.
    void destroy();             // Destroy this runop and alsos.

    void parse(char*);              // Parse specification line.
    bool istrue();                  // Evaluate true if condition met.
    ROret should_stop(sRunDesc*);   // Return if stop condition met.
    ROret call(sRunDesc*);          // Call function wrapper.
    void printcond(char**, bool);   // Print the conditional expression.

private:
    bool ro_call_flag;          // Call script or bound codeblock on stop.
    bool ro_ptmode;             // Input in points, else absolute.
    union {                     // Output point for test:
        double dpoint;          //   Value.
        int ipoint;             //   Plot point index.
    } ro_p;
    sRunopStop *ro_also;        // Link for conjunctions.
    const char *ro_call;        // Name of script to call on stop.
    int ro_index;               // Index into the ro_points array.
    int ro_numpts;              // Size of the ro_points array.
    union {                     // Array of points to test for "stop at".
        double *dpoints;
        int *ipoints;
    } ro_a;
};


// The general form of the definition string is
//   [when/at] expr[val][=][expr] [td=delay] [cross=crosses] [rise=rises]
//     [fall=falls]
// The initial keyword (which may be missing if unambiguous) is one of
// "at" or "when".  These are equivalent.  One or two expressions follow,
// with optional '=' or 'val=' ahead of the second expression.  the second
// expression can be missing.
//
// MPexp2:  expr1 and expr2 are both given, then the point is when
//   expr==expr2, and the td,cross,rise,fall keywords apply.  The risis,
//   falls, crosses are integers.  The delay is a numeric value, or the
//   name of another measure.  The trigger is the matching
//   rise/fall/cross found after the delay.
//
// If expr2 is not given, then expr1 is one of:
//
// MPnum:  (numeric value) Gives the point directly, no other keywords
//   apply.
//
// MPmref:  (measure name) Point where given measure completes,
//   numeric td applies, triggers at the referenced measure time plus
//   delay.
//
// MPexpr1:  (expression) Point where expression is boolen true, td
//   applies, can be numeric or measure name, trigers when expr is true
//   after delay.
//
enum MPform
{
    MPunknown,      // unknown/unspecified
    MPnum,          // single numeric value given
    MPmref,         // measure reference name given
    MPexp1,         // single expression given
    MPexp2          // two expressions given
};

enum MPrange
{
    MPatwhen,       // applies at defined value
    MPbefore,       // applies before defined value
    MPafter         // appies after defined value
};

struct pnode;

struct sMpoint
{
    sMpoint()
        {
            t_conj          = 0;
            t_kw1           = 0;
            t_kw2           = 0;
            t_when_expr1    = 0;
            t_when_expr2    = 0;
            t_tree1         = 0;
            t_tree2         = 0;
            t_mname         = 0;
            t_found         = 0.0;
            t_delay_given   = 0.0;
            t_delay         = 0.0;
            t_indx          = 0;
            t_crosses       = 0;
            t_rises         = 0;
            t_falls         = 0;
            t_found_flag    = false;
            t_init          = false;
            t_active        = false;
            t_type          = MPunknown;
            t_range         = MPatwhen;
        }

    ~sMpoint();

    const char *when_expr1()    { return (t_when_expr1); }
    const char *when_expr2()    { return (t_when_expr2); }
    double found()              { return (t_found); }
    int indx()                  { return (t_indx); }
    bool found_flag()           { return (t_found_flag); }
    bool active()               { return (t_active); }

    void reset()
        {
            t_delay = 0.0;
            t_found = 0.0;
            t_indx = 0;
            t_found_flag = false;
            t_init = false;
        }

    int parse(const char**, char**, const char*);
    void print(sLstr&);
    bool setup_delay(sFtCirc*, bool*);
    bool check_found(sFtCirc*, bool*, bool);
    sDataVec *eval1();
    sDataVec *eval2();

private:
    sMpoint *t_conj;        // Conjunction list.
    const char *t_kw1;      // Optional first keyword saved.
    const char *t_kw2;      // Optional second keyword saved.
    char *t_when_expr1;     // First expression text.
    char *t_when_expr2;     // Second expression text.
    pnode *t_tree1;
    pnode *t_tree2;
    char *t_mname;          // Measure reference name.
    double t_found;         // The measure point, once found.
    double t_delay_given;   // The 'td' value.
    double t_delay;         // Actual measurement point.
    int t_indx;             // Index of trigger point.
    int t_crosses;          // The 'crosses' value.
    int t_rises;            // The 'rises' value.
    int t_falls;;           // The 'falls' value.
    bool t_found_flag;      // The measure point was found.
    bool t_init;            // This is initialized.
    bool t_active;          // This is active, if not skip it.
    unsigned char t_type;   // Syntax type, MPform.
    unsigned char t_range;  // Before/at/after, MPrange.
};

enum Mfunc { Mmin, Mmax, Mpp, Mavg, Mrms, Mpw, Mrft, Mfind };

// List element for measurement jobs
//
struct sMfunc
{
    sMfunc(Mfunc t, const char *e)
        {
            f_type  = t;
            f_error = false;
            f_next  = 0;
            f_expr  = e;
            f_val   = 0.0;
        }

    ~sMfunc()       { delete [] f_expr; }

    static void destroy_list(sMfunc *f)
        {
            while (f) {
                sMfunc *x = f;
                f = f->f_next;
                delete x;
            }
        }

    Mfunc type()                { return (f_type); }
    bool error()                { return (f_error); }
    void set_error(bool b)      { f_error = b; }
    sMfunc *next()              { return (f_next); }
    void set_next(sMfunc *x)    { f_next = x; }
    const char *expr()          { return (f_expr); }
    double val()                { return (f_val); }
    void set_val(double d)      { f_val = d; }

    void print(sLstr&);

    Mfunc f_type;       // type of job
    bool f_error;       // set if expr evaluation fails
    sMfunc *f_next;     // pointer to next job
    const char *f_expr; // expression to evaluate
    double f_val;       // result of measurement
};


struct sRunopMeas : public sRunop
{
    sRunopMeas(const char *str, char **errstr)
    {
        ro_type = RO_MEASURE;

        ro_result               = 0;
        ro_expr2                = 0;
        ro_call                 = 0;
        ro_cktptr               = 0;
        ro_funcs                = 0;
        ro_finds                = 0;
        ro_analysis             = 0;
        ro_found_rises          = 0;
        ro_found_falls          = 0;
        ro_found_crosses        = 0;
        ro_measure_done         = false;
        ro_measure_error        = false;
        ro_measure_skip         = false;
        ro_stop_flag            = false;
        ro_end_flag             = false;
        ro_call_flag            = false;
        ro_print_flag           = 0;

        parse(str, errstr);
    }

    ~sRunopMeas()
        {
            delete [] ro_result;
            delete [] ro_expr2;
            delete [] ro_call;

            sMfunc::destroy_list(ro_funcs);
            sMfunc::destroy_list(ro_finds);
        }

    sRunopMeas *next()          { return ((sRunopMeas*)ro_next); }

    sMpoint &start()            { return (ro_start); }
    sMpoint &end()              { return (ro_end); }

    const char *result()        { return (ro_result); }
    int analysis()              { return (ro_analysis); }
    const char *expr2()         { return (ro_expr2); }
    const char *call()          { return (ro_call); }
    const char *start_when_expr1()  { return (ro_start.when_expr1()); }
    const char *start_when_expr2()  { return (ro_start.when_expr2()); }
    const char *end_when_expr1()    { return (ro_end.when_expr1()); }
    const char *end_when_expr2()    { return (ro_end.when_expr2()); }
    sMfunc *funcs()             { return (ro_funcs); }
    bool measure_done()         { return (ro_measure_done); }
    bool stop_flag()            { return (ro_stop_flag); }
    bool end_flag()             { return (ro_end_flag); }
    void nostop()               { ro_stop_flag = false; ro_end_flag = false; }

    static sRunopMeas *find(sRunopMeas *thism, const char *res)
        {
            if (res) {
                for (sRunopMeas *m = thism; m; m = m->next()) {
                    if (m->ro_result && !strcmp(res, m->ro_result))
                        return (m);
                }
            }
            return (0);
        }

    void print(char**);         // Print, in string if given, the runop msg.
    void destroy();             // Destroy this runop.

    // measure.cc
    bool parse(const char*, char**);
    void reset(sPlot*);
    bool check(sRunDesc*);
    bool measure(sDataVec**, int*);
    bool update_plot(sDataVec*, int);
    ROret call(sRunDesc*);
    char *print();

private:
    void addMeas(Mfunc, const char*);
    double endval(sDataVec*, sDataVec*, bool);
    double findavg(sDataVec*, sDataVec*);
    double findrms(sDataVec*, sDataVec*);
    double findpw(sDataVec*, sDataVec*);
    double findrft(sDataVec*, sDataVec*);

    sMpoint ro_start;
    sMpoint ro_end;

    const char *ro_result;      // result name for measurement
    const char *ro_expr2;       // misc expression
    const char *ro_call;        // function to call when measure comp[lete
    sFtCirc *ro_cktptr;         // back pointer to circuit
    sMfunc *ro_funcs;           // list of measurements over interval
    sMfunc *ro_finds;           // list of measurements at point
    int ro_analysis;            // type index of analysis 
    int ro_found_rises;         // number of rising crossings
    int ro_found_falls;         // number of falling crossings
    int ro_found_crosses;       // number of crossings
    bool ro_measure_done;       // measurement done successfully
    bool ro_measure_error;      // measurement can't be done
    bool ro_measure_skip;       // parse error so skip
    bool ro_stop_flag;          // pause analysis when done
    bool ro_end_flag;           // terminate analysis when done
    bool ro_call_flag;          // call a function or bound codeblock
    char ro_print_flag;         // print result on screen, 1 terse  2 verbose
};

#endif // RUNOP_H

