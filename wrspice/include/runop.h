
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
struct pnode;

enum ROtype
{
    RO_NONE,
    RO_SAVE,                // Save a node.
    RO_TRACE,               // Print the value of a node every iteration.
    RO_IPLOT,               // Incrementally plot listed vectors.
    RO_IPLOTALL,            // Incrementally plot everything.
    RO_DEADIPLOT,           // Iplot is being destroyed.
    RO_MEASURE,             // Perform a measurement.
    RO_STOP                 // Break on condition.
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

    virtual void print(sLstr*) = 0;
    virtual void destroy() = 0;

protected:
    sRunop *ro_next;            // List of active runop commands.
    ROtype ro_type;             // One of the above.
    int ro_number;              // The number of this runop command.
    char *ro_string;            // Condition or node, text.
    bool ro_active;             // True if active.
    bool ro_bad;                // True if error.
};


template <class T>
struct ROgen
{
    ROgen(T *list1, T *list2)
        {
            head1 = list1;
            head2 = list2;
        }

    T *next()
        {
            if (head1) {
                T *n = head1;
                head1 = head1->next();
                return (n);
            }
            if (head2) {
                T *n = head2;
                head2 = head2->next();
                return (n);
            }
            return (0);
        }
private:
    T *head1;
    T *head2;
};


// Save an expression when running.
struct sRunopSave : public sRunop
{
    sRunopSave()
        {
            ro_type = RO_SAVE;
        }

    sRunopSave *next()          { return ((sRunopSave*)ro_next); }

    void print(sLstr*);         // Print, in string if given, the runop msg.
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

    void print(sLstr*);         // Print, in string if given, the runop msg.
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

    void print(sLstr*);         // Print, in string if given, the runop msg.
    void destroy();             // Destroy this runop.

private:
    int ro_graphid;             // If iplot, id of graph.
    int ro_reuseid;             // Iplot window to reuse.
};


// Time point for measure/stop.
//
// The general form of the definition string is
//   [when/at] expr[val][=][expr] [td=offset] [cross=crosses] [rise=rises]
//     [fall=falls] [minx=val]
// The initial keyword (which may be missing if unambiguous) is one of
// "at" or "when".  These are equivalent.  One or two expressions follow,
// with optional '=' or 'val=' ahead of the second expression.  the second
// expression can be missing.
//
// MPexp2:  expr1 and expr2 are both given, then the point is when
//   expr==expr2, and the td,cross,rise,fall keywords apply.  The risis,
//   falls, crosses are integers.  The offset is a numeric value, or the
//   name of another measure.  The trigger is the matching
//   rise/fall/cross found after the offset.  If minx is given, crossing
//   events are only recognized if greater than this distance from the
//   previous one.
//
// If expr2 is not given, then expr1 is one of:
//
// MPnum:  (numeric value) Gives the point directly, no other keywords
//   apply.
//
// MPmref:  (measure name) Point where given measure completes,
//   numeric td applies, triggers at the referenced measure time plus
//   offset.
//
// MPexpr1:  (expression) Point where expression is boolen true, td
//   applies, can be numeric or measure name, trigers when expr is true
//   after offset.
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
    MPafter,        // appies at or after defined value
    MPat,           // applies at defined value only (strobing)
    MPbefore,       // applies before defined value
    MPwhen          // appies at or after defined value (same as after)
};

struct sMpoint
{
    sMpoint()
        {
            t_conj          = 0;
            t_kw1           = 0;
            t_kw2           = 0;
            t_expr1         = 0;
            t_expr2         = 0;
            t_tree1         = 0;
            t_tree2         = 0;
            t_mname         = 0;
            t_td            = 0.0;
            t_offset        = 0.0;
            t_found         = 0.0;
            t_v1            = 0.0;
            t_v2            = 0.0;
            t_px            = 0.0;
            t_minx          = 0.0;
            t_lastx         = 0.0;
            t_indx          = 0;
            t_crosses       = 0;
            t_rises         = 0;
            t_falls         = 0;
            t_cross_cnt     = 0;
            t_rise_cnt      = 0;
            t_fall_cnt      = 0;
            t_active        = false;
            t_ready         = false;
            t_found_local   = false;
            t_found_state   = false;
            t_offset_set    = false;
            t_last_saved    = false;
            t_px_saved      = false;
            t_td_given      = false;
            t_ptmode        = false;
            t_strobe        = false;
            t_dstrobe       = false;
            t_type          = MPunknown;
            t_range         = MPwhen;
        }

    ~sMpoint();

    sMpoint *conj()             { return (t_conj); }
    const char *expr1()         { return (t_expr1); }
    const char *expr2()         { return (t_expr2); }
    double found()              { return (t_found); }
    int indx()                  { return (t_indx); }
    bool active()               { return (t_active); }
    bool ready()                { return (t_ready); }
    bool strobe()               { return (t_strobe); }
    bool dstrobe()              { return (t_dstrobe); }

    void reset()
        {
            t_offset = 0.0;
            t_found = 0.0;
            t_v1 = 0.0;
            t_v2 = 0.0;
            t_px = 0.0;
            t_lastx = 0.0;
            t_indx = 0;
            t_cross_cnt = 0;
            t_rise_cnt = 0;
            t_fall_cnt = 0;
            t_ready = false;
            t_found_local = false;
            t_found_state = false;
            t_offset_set = false;
            t_last_saved = false;
            t_px_saved = false;
            if (t_conj)
                t_conj->reset();
        }

    int parse(const char**, char**, const char*);
    void print(sLstr*);
    bool check_found(sFtCirc*, bool*, bool, sMpoint* = 0);
    int check_trig(sDataVec*, double);

private:
    sDataVec *eval1();
    sDataVec *eval2();

    sMpoint *t_conj;        // Conjunction list.
    const char *t_kw1;      // Optional first keyword saved.
    const char *t_kw2;      // Optional second keyword saved.
    char *t_expr1;          // First expression text.
    char *t_expr2;          // Second expression text.
    pnode *t_tree1;         // Expression 1 parse tree.
    pnode *t_tree2;         // Expression 2 parse tree.
    char *t_mname;          // Measure reference name.
    double t_td;            // The 'td' value given.
    double t_offset;        // Internal offset value.
    double t_found;         // The measure point, once found.
    double t_v1;            // Previous expr1 value,
    double t_v2;            // Previous expr2 value,
    double t_px;            // Previous time value.
    double t_minx;          // Minimum time from last cross event.
    double t_lastx;         // Time of last cross event.
    int t_indx;             // Index of trigger point.
    int t_crosses;          // The 'crosses' value.
    int t_rises;            // The 'rises' value.
    int t_falls;;           // The 'falls' value.
    int t_cross_cnt;        // Running croses count.
    int t_rise_cnt;         // Running rises count.
    int t_fall_cnt;         // Running falls count.
    bool t_active;          // This is active, if not skip it.
    bool t_ready;           // The measure point was found for all conj.
    bool t_found_local;     // The measure point was found for this.
    bool t_found_state;     // The state to return when found.
    bool t_offset_set;      // This is initialized.
    bool t_last_saved;      // Last values of expressions saved.
    bool t_px_saved;        // Last time value saved.
    bool t_ptmode;          // Input in points, else absolute.
    bool t_td_given;        // Offset was given.
    bool t_strobe;          // Strobe mode set.
    bool t_dstrobe;         // Auto strobe at delay mode set.
    unsigned char t_type;   // Syntax type, MPform.
    unsigned char t_range;  // Before/at/after, MPrange.
};

struct sXpt
{
    double val;
    double scval;
};

enum Mfunc { Mmin, Mmax, Mpp, Mavg, Mrms, Mpw, Mrft, Mfind };

// List element for measurement jobs
//
struct sMfunc
{
    sMfunc(Mfunc t, const char *e, double v1 = 0.0, double v2 = 0.0)
        {
            f_type  = t;
            f_error = false;
            f_next  = 0;
            f_expr  = e;
            f_val   = 0.0;
            f_v1    = v1;
            f_v2    = v2;
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

    void print(sLstr*);

    bool mmin(sDataVec*, int, int, sXpt*, sXpt*);
    bool mmax(sDataVec*, int, int, sXpt*, sXpt*);
    bool mpp(sDataVec*,  int, int, sXpt*, sXpt*);
    bool mavg(sDataVec*, int, int, sXpt*, sXpt*);
    bool mrms(sDataVec*, int, int, sXpt*, sXpt*);
    bool mpw(sDataVec*,  int, int, sXpt*, sXpt*);
    bool mrft(sDataVec*, int, int, sXpt*, sXpt*, double =0.1, double =0.9);

private:
    Mfunc f_type;       // type of job
    bool f_error;       // set if expr evaluation fails
    sMfunc *f_next;     // pointer to next job
    const char *f_expr; // expression to evaluate
    double f_val;       // result of measurement
    double f_v1;        // aux. input
    double f_v2;        // aux. input
};


struct sRunopMeas : public sRunop
{
    sRunopMeas(const char *str, char **errstr)
    {
        ro_type = RO_MEASURE;

        ro_result               = 0;
        ro_prmexpr              = 0;
        ro_exec                 = 0;
        ro_call                 = 0;
        ro_cktptr               = 0;
        ro_funcs                = 0;
        ro_finds                = 0;
        ro_analysis             = 0;
        ro_found_rises          = 0;
        ro_found_falls          = 0;
        ro_found_crosses        = 0;
        ro_queue_measure        = false;
        ro_measure_done         = false;
        ro_measure_error        = false;
        ro_measure_skip         = false;
        ro_stop_flag            = false;
        ro_end_flag             = false;
        ro_print_flag           = 0;

        parse(str, errstr);
    }

    ~sRunopMeas()
        {
            delete [] ro_result;
            delete [] ro_prmexpr;
            delete [] ro_exec;
            delete [] ro_call;

            sMfunc::destroy_list(ro_funcs);
            sMfunc::destroy_list(ro_finds);
        }

    // Reset the measurement.
    //
    void reset()
        {
            ro_start.reset();
            ro_end.reset();

            ro_found_rises      = 0;
            ro_found_falls      = 0;
            ro_found_crosses    = 0;
            ro_measure_done     = false;
            ro_measure_error    = false;
            ro_measure_skip     = false;
            ro_stop_flag        = false;
            ro_end_flag         = false;
            ro_queue_measure    = false;
        }

    sRunopMeas *next()          { return ((sRunopMeas*)ro_next); }

    sMpoint &start()            { return (ro_start); }
    sMpoint &end()              { return (ro_end); }

    const char *result()        { return (ro_result); }
    int analysis()              { return (ro_analysis); }
    const char *prmexpr()       { return (ro_prmexpr); }
    const char *call()          { return (ro_call); }
    const char *start_expr1()   { return (ro_start.expr1()); }
    const char *start_expr2()   { return (ro_start.expr2()); }
    const char *end_expr1()     { return (ro_end.expr1()); }
    const char *end_expr2()     { return (ro_end.expr2()); }
    sMfunc *funcs()             { return (ro_funcs); }
    sMfunc *finds()             { return (ro_finds); }
    bool measure_queued()       { return (ro_queue_measure); }
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

    void print(sLstr*);         // Print, in string if given, the runop msg.
    void destroy();             // Destroy this runop.

    // measure.cc
    bool parse(const char*, char**);
    bool check_measure(sRunDesc*);
    bool do_measure();
    bool measure(sDataVec**, int*);
    bool update_plot(sDataVec*, int);
    char *print_meas();

private:
    void addMeas(Mfunc, const char*, double = 0.0, double = 0.0);
    sDataVec *evaluate(const char*);
    double startval(sDataVec*);
    double endval(sDataVec*);
    sXpt startpoint(sDataVec*);
    sXpt endpoint(sDataVec*);

    sMpoint ro_start;
    sMpoint ro_end;

    const char *ro_result;      // result name for measurement
    const char *ro_prmexpr;     // holds expression from param=expression
    const char *ro_exec;        // command to execute when measure complete
    const char *ro_call;        // function to call when measure complete
    sFtCirc *ro_cktptr;         // back pointer to circuit
    sMfunc *ro_funcs;           // list of measurements over interval
    sMfunc *ro_finds;           // list of measurements at point
    int ro_analysis;            // type index of analysis 
    int ro_found_rises;         // number of rising crossings
    int ro_found_falls;         // number of falling crossings
    int ro_found_crosses;       // number of crossings
    bool ro_queue_measure;      // ready to measure
    bool ro_measure_done;       // measurement done successfully
    bool ro_measure_error;      // measurement can't be done
    bool ro_measure_skip;       // parse error so skip
    bool ro_stop_flag;          // pause analysis when done
    bool ro_end_flag;           // terminate analysis when done
    char ro_print_flag;         // print result on screen, 1 terse  2 verbose
};

struct sRunopStop : public sRunop
{
    sRunopStop(const char *str, char **errstr)
    {
        ro_type = RO_STOP;

        ro_exec                 = 0;
        ro_call                 = 0;
        ro_offs                 = 0.0;
        ro_per                  = 0.0;
        ro_analysis             = 0;
        ro_found_rises          = 0;
        ro_found_falls          = 0;
        ro_found_crosses        = 0;
        ro_stop_done            = false;
        ro_stop_error           = false;
        ro_stop_skip            = false;
        ro_stop_flag            = false;
        ro_end_flag             = false;
        ro_silent               = false;
        ro_repeating            = false;

        parse(str, errstr);
    }

    ~sRunopStop()
        {
            delete [] ro_exec;
            delete [] ro_call;
        }

    // Reset the measurement.
    //
    void reset()
        {
            ro_start.reset();

            ro_found_rises      = 0;
            ro_found_falls      = 0;
            ro_found_crosses    = 0;
            ro_offs             = 0.0;
            ro_stop_done        = false;
            ro_stop_error       = false;
            ro_stop_flag        = false;
            ro_end_flag         = false;
            ro_repeating        = false;
        }

    sRunopStop *next()          { return ((sRunopStop*)ro_next); }

    sMpoint &start()            { return (ro_start); }

    int analysis()              { return (ro_analysis); }
    const char *call()          { return (ro_call); }
    const char *start_expr1()   { return (ro_start.expr1()); }
    const char *start_expr2()   { return (ro_start.expr2()); }
    bool stop_done()            { return (ro_stop_done); }
    bool stop_flag()            { return (ro_stop_flag); }
    bool end_flag()             { return (ro_end_flag); }
    bool silent()               { return (ro_silent); }
    void nostop()               { ro_stop_flag = false; ro_end_flag = false; }

    void print(sLstr*);         // Print, in string if given, the runop msg.
    void destroy();             // Destroy this runop.

    // measure.cc
    bool parse(const char*, char**);
    ROret check_stop(sRunDesc*);
    void print_cond(sLstr*, bool);

private:
    double endval(sDataVec*, sDataVec*, bool);

    sMpoint ro_start;

    const char *ro_exec;        // command to execute when measure complete
    const char *ro_call;        // function to call when measure comp[lete
    double ro_offs;             // offset for repeat
    double ro_per;              // period for repeat
    int ro_analysis;            // type index of analysis 
    int ro_found_rises;         // number of rising crossings
    int ro_found_falls;         // number of falling crossings
    int ro_found_crosses;       // number of crossings
    bool ro_stop_done;          // measurement done successfully
    bool ro_stop_error;         // measurement can't be done
    bool ro_stop_skip;          // parse error so skip
    bool ro_stop_flag;          // pause analysis when done
    bool ro_end_flag;           // terminate analysis when done
    bool ro_silent;             // don't print stop message
    bool ro_repeating;          // repeating exec/call periodically
};

#endif // RUNOP_H

