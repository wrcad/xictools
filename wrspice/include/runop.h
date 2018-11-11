
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

            ro_call     = false;
            ro_ptmode   = false;
            ro_p.dpoint = 0.0;
            ro_callfn   = 0;
            ro_also     = 0;
            ro_index    = 0;
            ro_numpts   = 0;
            ro_a.dpoints= 0;
        }

    ~sRunopStop()
        {
            delete [] ro_callfn;
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

    bool call()                 { return (ro_call); }
    const char *call_func()     { return (ro_call ? ro_callfn : 0); }
    void set_call(bool b, const char *fn)
        {
            ro_call = b;
            char *nm = lstring::copy(fn);
            delete [] ro_callfn;
            ro_callfn = nm;
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

    bool istrue();                  // Evaluate true if condition met.
    ROret should_stop(sRunDesc*);   // True if stop condition met.
    void printcond(char**, bool);   // Print the conditional expression.

private:
    bool ro_call;               // Call script or bound codeblock on stop.
    bool ro_ptmode;             // Input in points, else absolute.
    union {                     // Output point for test:
        double dpoint;          //   Value.
        int ipoint;             //   Plot point index.
    } ro_p;
    sRunopStop *ro_also;        // Link for conjunctions.
    char *ro_callfn;            // Name of script to call on stop.
    int ro_index;               // Index into the ro_points array.
    int ro_numpts;              // Size of the ro_points array.
    union {                     // Array of points to test for "stop at".
        double *dpoints;
        int *ipoints;
    } ro_a;
};

enum Mfunc { Mmin, Mmax, Mpp, Mavg, Mrms, Mpw, Mrft, Mfind };

// List element for measurement jobs
//
struct sMfunc
{
    sMfunc(Mfunc t, const char *e)
        {
            type    = t;
            expr    = e;
            val     = 0.0;
            next    = 0;
            error   = false;
        }

    ~sMfunc()       { delete [] expr; }

    Mfunc type;         // type of job
    const char *expr;   // expression to evaluate
    double val;         // result of measurement
    sMfunc *next;       // pointer to next job
    bool error;         // set if expr evaluation fails
};

// Argument to sTpoint::parse.
enum TPform { TPunknown, TPat, TPwhen, TPvar };

struct sTpoint
{
    sTpoint()
        {
            t_name          = 0;
            t_when_expr1    = 0;
            t_when_expr2    = 0;
            t_meas          = 0;
            t_at            = 0.0;
            t_val           = 0.0;
            t_delay         = 0.0;
            t_found         = 0.0;
            t_dv            = 0;
            t_indx          = 0;
            t_crosses       = 0;
            t_rises         = 0;
            t_falls         = 0;
            t_at_given      = false;
            t_when_given    = false;
            t_found_flag    = false;
        }

    ~sTpoint()
        {
            delete [] t_when_expr1;
            delete [] t_when_expr2;
            delete [] t_name;
            delete [] t_meas;
        }

    const char *name()          { return (t_name); }
    void set_name(char *s)      { t_name = s; }
    const char *when_expr1()    { return (t_when_expr1); }
    void set_when_expr1(char *s){ t_when_expr1 = s; }
    const char *when_expr2()    { return (t_when_expr2); }
    void set_when_expr2(char *s){ t_when_expr2 = s; }
    const char *meas()          { return (t_meas); }
    void set_meas(char *s)      { t_meas = s; }
    double at()                 { return (t_at); }
    void set_at(double d)       { t_at = d; }
    double val()                { return (t_val); }
    void set_val(double d)      { t_val = d; }
    double delay()              { return (t_delay); }
    void set_delay(double d)    { t_delay = d; }
    double found()              { return (t_found); }
    void set_found(double d)    { t_found = d; }
    sDataVec *dv()              { return (t_dv); }
    void set_dv(sDataVec *d)    { t_dv = d; }
    int indx()                  { return (t_indx); }
    void set_indx(int i)        { t_indx = i; }
    int crosses()               { return (t_crosses); }
    void set_crosses(int i)     { t_crosses = i; }
    int rises()                 { return (t_rises); }
    void set_rises(int i)       { t_rises = i; }
    int falls()                 { return (t_falls); }
    void set_falls(int i)       { t_falls = i; }
    bool at_given()             { return (t_at_given); }
    void set_at_given(bool b)   { t_at_given = b; }
    bool when_given()           { return (t_when_given); }
    void set_when_given(bool b) { t_when_given = b; }
    bool found_flag()           { return (t_found_flag); }
    void set_found_flag(bool b) { t_found_flag = b; }

    void reset()
        {
            t_delay = 0.0;
            t_found = 0.0;
            t_dv = 0;
            t_indx = 0;
            t_found_flag = false;
        }

    int parse(TPform, const char**, char**);
    bool setup_dv(sFtCirc*, bool*);
    bool check_found(sFtCirc*, bool*, bool);

private:
    char *t_name;           // Vector name.
    char *t_when_expr1;     // LHS expression for 'when lhs = rhs'.
    char *t_when_expr2;     // RHS expression for 'when lhs = rhs'.
    char *t_meas;           // Chained measure name.
    double t_at;            // The 'at' value.
    double t_val;           // The 'val' value.
    double t_delay;         // The 'td' value.
    double t_found;         // The measure point, once found.
    sDataVec *t_dv;         // Cached datavec for vector name.
    int t_indx;             // Index of trigger point.
    int t_crosses;          // The 'crosses' value.
    int t_rises;            // The 'rises' value.
    int t_falls;;           // The 'falls' value.
    bool t_at_given;        // The 'at' clause was given.
    bool t_when_given;      // The 'when' clause was given.
    bool t_found_flag;      // The measure point was found.
};


struct sRunopMeas : public sRunop
{
    sRunopMeas(const char *str, char **errstr)
    {
        ro_type = RO_MEASURE;

        ro_analysis             = 0;
        ro_result               = 0;
        ro_expr2                = 0;
        ro_cktptr               = 0;
        ro_funcs                = 0;
        ro_finds                = 0;
        ro_found_rises          = 0;
        ro_found_falls          = 0;
        ro_found_crosses        = 0;
        ro_measure_done         = false;
        ro_measure_error        = false;
        ro_measure_skip         = false;
        ro_stop_flag            = false;
        ro_print_flag           = 0;

        parse(str, errstr);
    }

    ~sRunopMeas()
        {
            delete [] ro_result;
            delete [] ro_expr2;

            while (ro_funcs) {
                sMfunc *f = ro_funcs->next;
                delete ro_funcs;
                ro_funcs = f;
            }
            while (ro_finds) {
                sMfunc *f = ro_finds->next;
                delete ro_finds;
                ro_finds = f;
            }
        }

    sRunopMeas *next()          { return ((sRunopMeas*)ro_next); }

    double found_start()        { return (ro_start.found()); }
    bool found_start_flag()     { return (ro_start.found_flag()); }
    double found_end()          { return (ro_end.found()); }
    bool found_end_flag()       { return (ro_end.found_flag()); }

    const char *result()        { return (ro_result); }
    int analysis()              { return (ro_analysis); }
    const char *start_name()    { return (ro_start.name()); }
    const char *end_name()      { return (ro_end.name()); }
    const char *expr2()         { return (ro_expr2); }
    const char *start_when_expr1()  { return (ro_start.when_expr1()); }
    const char *start_when_expr2()  { return (ro_start.when_expr2()); }
    const char *end_when_expr1()    { return (ro_end.when_expr1()); }
    const char *end_when_expr2()    { return (ro_end.when_expr2()); }
    sMfunc *funcs()             { return (ro_funcs); }
    bool measure_done()         { return (ro_measure_done); }
    bool shouldstop()           { return (ro_stop_flag); }
    void nostop()               { ro_stop_flag = false; }

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
    bool check(sFtCirc*);
    char *print();

private:
    void addMeas(Mfunc, const char*);
    double endval(sDataVec*, sDataVec*, bool);
    double findavg(sDataVec*, sDataVec*);
    double findrms(sDataVec*, sDataVec*);
    double findpw(sDataVec*, sDataVec*);
    double findrft(sDataVec*, sDataVec*);

    sTpoint ro_start;
    sTpoint ro_end;

    int ro_analysis;            // type index of analysis 
    const char *ro_result;      // result name for measurement
    const char *ro_expr2;       // misc expression
    sFtCirc *ro_cktptr;         // back pointer to circuit
    sMfunc *ro_funcs;           // list of measurements over interval
    sMfunc *ro_finds;           // list of measurements at point
    sDataVec *ro_end_dv;        // cached datavec for targ name
    int ro_end_indx;            // index of target point
    int ro_found_rises;         // number of rising crossings
    int ro_found_falls;         // number of falling crossings
    int ro_found_crosses;       // number of crossings
    bool ro_when_given;         // true if 'when' given without 'trig', 'targ'
    bool ro_measure_done;       // measurement done successfully
    bool ro_measure_error;      // measurement can't be done
    bool ro_measure_skip;       // parse error so skip
    bool ro_stop_flag;          // stop analysis when done
    char ro_print_flag;         // print result on screen, 1 terse  2 verbose
};

#endif // RUNOP_H

