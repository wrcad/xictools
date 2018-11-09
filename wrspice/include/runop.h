
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
    void printcond(char**);         // Print the conditional expression.

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
            type = t;
            expr = e;
            val = 0.0;
            next = 0;
            error = false;
        }

    ~sMfunc() { delete [] expr; }

    Mfunc type;         // type of job
    const char *expr;   // expression to evaluate
    double val;         // result of measurement
    sMfunc *next;       // pointer to next job
    bool error;         // set if expr evaluation fails
};

struct sRunopMeas : public sRunop
{
    sRunopMeas(const char *str, char **errstr)
    {
        ro_type = RO_MEASURE;

        ro_start_at             = 0.0;
        ro_end_at               = 0.0;
        ro_start_val            = 0.0;
        ro_end_val              = 0.0;
        ro_start_delay          = 0.0;
        ro_end_delay            = 0.0;
        ro_start_crosses        = 0;
        ro_start_rises          = 0;
        ro_start_falls          = 0;
        ro_end_crosses          = 0;
        ro_end_rises            = 0;
        ro_end_falls            = 0;
        ro_start_at_given       = 0;
        ro_start_when_given     = 0;
        ro_end_at_given         = 0;
        ro_end_when_given       = 0;
        ro_when_given           = 0;
        ro_analysis             = 0;
        ro_result               = 0;
        ro_start_name           = 0;
        ro_end_name             = 0;
        ro_expr2                = 0;
        ro_start_when_expr1     = 0;
        ro_start_when_expr2     = 0;
        ro_end_when_expr1       = 0;
        ro_end_when_expr2       = 0;
        ro_start_meas           = 0;
        ro_end_meas             = 0;
        ro_cktptr               = 0;
        ro_funcs                = 0;
        ro_finds                = 0;
        ro_start_dv             = 0;
        ro_end_dv               = 0;
        ro_start_indx           = 0;
        ro_end_indx             = 0;
        ro_found_rises          = 0;
        ro_found_falls          = 0;
        ro_found_crosses        = 0;
        ro_found_start          = 0.0;
        ro_found_end            = 0.0;
        ro_found_start_flag     = false;
        ro_found_end_flag       = false;
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
            delete [] ro_start_name;
            delete [] ro_end_name;
            delete [] ro_expr2;
            delete [] ro_start_when_expr1;
            delete [] ro_start_when_expr2;
            delete [] ro_end_when_expr1;
            delete [] ro_end_when_expr2;

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

    bool shouldstop()           { return (ro_stop_flag); }
    void nostop()               { ro_stop_flag = false; }

    const char *result()        { return (ro_result); }
    int analysis()              { return (ro_analysis); }
    const char *start_name()    { return (ro_start_name); }
    const char *end_name()      { return (ro_end_name); }
    const char *expr2()         { return (ro_expr2); }
    const char *start_when_expr1()  { return (ro_start_when_expr1); }
    const char *start_when_expr2()  { return (ro_start_when_expr2); }
    const char *end_when_expr1()    { return (ro_end_when_expr1); }
    const char *end_when_expr2()    { return (ro_end_when_expr2); }
    sMfunc *funcs()             { return (ro_funcs); }

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

    double ro_start_at;         // 'trig at =' value
    double ro_end_at;           // 'targ at =' value
    double ro_start_val;        // 'trig ... val =' value
    double ro_end_val;          // 'targ ... val =' value
    double ro_start_delay;      // 'trig ... td =' value
    double ro_end_delay;        // 'targ ... td =' value
    int ro_start_crosses;       // 'trig ... cross =' vaule
    int ro_start_rises;         // 'trig ... rise =' value
    int ro_start_falls;         // 'trig ... fall =' value
    int ro_end_crosses;         // 'targ ... cross =' value
    int ro_end_rises;           // 'targ ... rise =' value
    int ro_end_falls;           // 'targ ... fall =' value
    int ro_start_at_given :1;   // true if 'trig at' given
    int ro_start_when_given :1; // true if 'trig when' given
    int ro_end_at_given :1;     // true if 'targ at' given
    int ro_end_when_given :1;   // true if 'targ when' given
    int ro_when_given :1;       // true if 'when' given without 'trig', 'targ'
    int ro_analysis;            // type index of analysis 
    const char *ro_result;      // result name for measurement
    const char *ro_start_name;  // 'trig' vector name
    const char *ro_end_name;    // 'targ' vector name
    const char *ro_expr2;       // misc expression
    const char *ro_start_when_expr1; // lhs expression for 'trig when lhs = rhs'
    const char *ro_start_when_expr2; // rhs expression for 'trig when lhs = rhs'
    const char *ro_end_when_expr1; // lhs expression for 'targ when lhs = rhs'
    const char *ro_end_when_expr2; // rhs expression for 'targ when lhs = rhs'
    const char *ro_start_meas;  // chained measure, start
    const char *ro_end_meas;    // chained measure, end
    sFtCirc *ro_cktptr;         // back pointer to circuit
    sMfunc *ro_funcs;           // list of measurements over interval
    sMfunc *ro_finds;           // list of measurements at point
    sDataVec *ro_start_dv;      // cached datavec for trig name
    sDataVec *ro_end_dv;        // cached datavec for targ name
    int ro_start_indx;          // index of trigger point
    int ro_end_indx;            // index of target point
    int ro_found_rises;         // number of rising crossings
    int ro_found_falls;         // number of falling crossings
    int ro_found_crosses;       // number of crossings
    double ro_found_start;      // trigger point
    double ro_found_end;        // target point
    bool ro_found_start_flag;   // trigger point identified
    bool ro_found_end_flag;     // target point identified
    bool ro_measure_done;       // measurement done successfully
    bool ro_measure_error;      // measurement can't be done
    bool ro_measure_skip;       // parse error so skip
    bool ro_stop_flag;          // stop analysis when done
    char ro_print_flag;         // print result on screen, 1 terse  2 verbose
};

#endif // RUNOP_H

