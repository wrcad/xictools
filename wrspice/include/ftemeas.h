
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: ftemeas.h,v 2.42 2015/07/29 04:50:21 stevew Exp $
 *========================================================================*/

#ifndef FTEMEAS_H
#define FTEMEAS_H

#include <string.h>


//
//  Structure for the measurement post-processor
//

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

struct sMeas
{
    sMeas(const char*, char**);
    ~sMeas();
    bool parse(const char*, char**);
    void reset(sPlot*);
    bool check(sFtCirc*);
    char *print();
    bool shouldstop() { return stop_flag; }
    void nostop() { stop_flag = false; }

    void free()
        {
            sMeas *m = this;
            while (m) {
                sMeas *mx = m;
                m = m->next;
                delete mx;
            }
        }

    sMeas *find(const char *res)
        {
            if (res) {
                for (sMeas *m = this; m; m = m->next) {
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
    sMeas *next;                // pointer to next measurement spec.

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

#endif // FTEMEAS_H

