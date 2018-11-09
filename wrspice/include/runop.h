
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

        start_at                = 0.0;
        end_at                  = 0.0;
        start_val               = 0.0;
        end_val                 = 0.0;
        start_delay             = 0.0;
        end_delay               = 0.0;
        start_crosses           = 0;
        start_rises             = 0;
        start_falls             = 0;
        end_crosses             = 0;
        end_rises               = 0;
        end_falls               = 0;
        start_at_given          = 0;
        start_when_given        = 0;
        end_at_given            = 0;
        end_when_given          = 0;
        when_given              = 0;
        analysis                = 0;
        result                  = 0;
        start_name              = 0;
        end_name                = 0;
        expr2                   = 0;
        start_when_expr1        = 0;
        start_when_expr2        = 0;
        end_when_expr1          = 0;
        end_when_expr2          = 0;
        start_meas              = 0;
        end_meas                = 0;
        cktptr                  = 0;
        funcs                   = 0;
        finds                   = 0;

        start_dv                = 0;
        end_dv                  = 0;
        start_indx              = 0;
        end_indx                = 0;
        found_rises             = 0;
        found_falls             = 0;
        found_crosses           = 0;
        found_start             = 0.0;
        found_end               = 0.0;
        found_start_flag        = false;
        found_end_flag          = false;
        measure_done            = false;
        measure_error           = false;
        measure_skip            = false;
        stop_flag               = false;
        print_flag              = 0;

        parse(str, errstr);
    }

    ~sRunopMeas()
        {
            delete [] result;
            delete [] start_name;
            delete [] end_name;
            delete [] expr2;
            delete [] start_when_expr1;
            delete [] start_when_expr2;
            delete [] end_when_expr1;
            delete [] end_when_expr2;

            while (funcs) {
                sMfunc *f = funcs->next;
                delete funcs;
                funcs = f;
            }
            while (finds) {
                sMfunc *f = finds->next;
                delete finds;
                finds = f;
            }
        }

    sRunopMeas *next()          { return ((sRunopMeas*)ro_next); }

    void print(char**);         // Print, in string if given, the runop msg.
    void destroy();             // Destroy this runop.

    bool parse(const char*, char**);
    void reset(sPlot*);
    bool check(sFtCirc*);
    char *print();
    bool shouldstop() { return stop_flag; }
    void nostop() { stop_flag = false; }

    static sRunopMeas *find(sRunopMeas *thism, const char *res)
        {
            if (res) {
                for (sRunopMeas *m = thism; m; m = m->next()) {
                    if (m->result && !strcmp(res, m->result))
                        return (m);
                }
            }
            return (0);
        }

    double start_at;            // 'trig at =' value
    double end_at;              // 'targ at =' value
    double start_val;           // 'trig ... val =' value
    double end_val;             // 'targ ... val =' value
    double start_delay;         // 'trig ... td =' value
    double end_delay;           // 'targ ... td =' value
    int start_crosses;          // 'trig ... cross =' vaule
    int start_rises;            // 'trig ... rise =' value
    int start_falls;            // 'trig ... fall =' value
    int end_crosses;            // 'targ ... cross =' value
    int end_rises;              // 'targ ... rise =' value
    int end_falls;              // 'targ ... fall =' value
    int start_at_given :1;      // true if 'trig at' given
    int start_when_given :1;    // true if 'trig when' given
    int end_at_given :1;        // true if 'targ at' given
    int end_when_given :1;      // true if 'targ when' given
    int when_given :1;          // true if 'when' given without 'trig', 'targ'
    int analysis;               // type index of analysis 
    const char *result;         // result name for measurement
    const char *start_name;     // 'trig' vector name
    const char *end_name;       // 'targ' vector name
    const char *expr2;          // misc expression
    const char *start_when_expr1; // lhs expression for 'trig when lhs = rhs'
    const char *start_when_expr2; // rhs expression for 'trig when lhs = rhs'
    const char *end_when_expr1; // lhs expression for 'targ when lhs = rhs'
    const char *end_when_expr2; // rhs expression for 'targ when lhs = rhs'
    const char *start_meas;     // chained measure, start
    const char *end_meas;       // chained measure, end
    sFtCirc *cktptr;            // back pointer to circuit
    sMfunc *funcs;              // list of measurements over interval
    sMfunc *finds;              // list of measurements at point

private:
    void addMeas(Mfunc, const char*);
    double endval(sDataVec*, sDataVec*, bool);
    double findavg(sDataVec*, sDataVec*);
    double findrms(sDataVec*, sDataVec*);
    double findpw(sDataVec*, sDataVec*);
    double findrft(sDataVec*, sDataVec*);

    struct sDataVec *start_dv;  // cached datavec for trig name
    struct sDataVec *end_dv;    // cached datavec for targ name
    int start_indx;             // index of trigger point
    int end_indx;               // index of target point
    int found_rises;            // number of rising crossings
    int found_falls;            // number of falling crossings
    int found_crosses;          // number of crossings
    double found_start;         // trigger point
    double found_end;           // target point
    bool found_start_flag;      // trigger point identified
    bool found_end_flag;        // target point identified
    bool measure_done;          // measurement done successfully
    bool measure_error;         // measurement can't be done
    bool measure_skip;          // parse error so skip
    bool stop_flag;             // stop analysis when done
    char print_flag;            // print result on screen
                                //  1 terse  2 verbose
};

#endif // RUNOP_H

