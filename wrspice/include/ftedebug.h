
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

#ifndef FTEDEBUG_H
#define FTEDEBUG_H


//
// Definitions for the "debugs" (analysis tracing, stop on condition,
// etc).
//

struct sRunDesc;
struct sMeas;

enum DBtype
{
    DB_NONE,
    DB_SAVE,                // Save a node.
    DB_TRACE,               // Print the value of a node every iteration.
    DB_IPLOT,               // Incrementally plot listed vectors.
    DB_IPLOTALL,            // Incrementally plot everything.
    DB_DEADIPLOT,           // Iplot is being destroyed.
    DB_STOPAFTER,           // Break after this many iterations.
    DB_STOPAT,              // Break at this many iterations.
    DB_STOPBEFORE,          // Break before this many iterations.
    DB_STOPWHEN,            // Break when a node reaches this value.
    DB_MEASURE              // Perform a measurement.
};

// Call function returns.
enum { CB_OK, CB_PAUSE, CB_ENDIT };

// Structure to save a debug context as a list element.
//
struct sDbComm
{
    sDbComm()
        {
            db_next     = 0;
            db_also     = 0;
            db_string   = 0;
            db_callfn   = 0;
            db_p.dpoint = 0.0;
            db_number   = 0;
            db_type     = DB_NONE;
            db_graphid  = 0;
            db_reuseid  = 0;
            db_active   = false;
            db_bad      = false;
            db_ptmode   = false;
            db_call     = false;
            db_index    = 0;
            db_numpts   = 0;
            db_a.dpoints= 0;
        }

    ~sDbComm()
        {
            delete [] db_string;
            delete [] db_callfn;
            if (db_ptmode)
                delete [] db_a.ipoints;
            else
                delete [] db_a.dpoints;
        }

    static void destroy(sDbComm*);  // destroy this debug and descendents
    bool istrue();                  // evaluate true if condition met
    bool should_stop(sRunDesc*);    // true if stop condition met
    bool run_call(sRunDesc*);       // call the callfunc is given
    void print(char**);             // print, in string if given, the debug msg
    bool print_trace(sPlot*, bool*, int);  // print trace output
    void printcond(char**);         // print the conditional expression

    static void destroy_list(sDbComm *l)
        {
            while (l) {
                sDbComm *x = l;
                l = l->db_next;
                destroy(x);
            }
        }

    sDbComm *next()             { return (db_next); }
    void set_next(sDbComm *d)   { db_next = d; }

    sDbComm *also()             { return (db_also); }
    void set_also(sDbComm *d)   { db_also = d; }

    const char *string()        { return (db_string); }
    void set_string(char *s)    { db_string = s; }

    const char *call_func()     { return (db_call ? db_callfn : 0); }
    void set_call(bool b, const char *fn)
        {
            db_call = b;
            char *nm = lstring::copy(fn);
            delete [] db_callfn;
            db_callfn = nm;
        }

    void set_point(double d)
        {
            db_p.dpoint = d;
            db_ptmode = false;
        }

    void set_point(int i)
        {
            db_p.ipoint = i;
            db_ptmode = true;
        }

    int number()                { return (db_number); }
    void set_number(int i)      { db_number = i; }

    DBtype type()               { return (db_type); }
    void set_type(DBtype t)     { db_type = t; }

    int graphid()               { return (db_graphid); }
    void set_graphid(int i)     { db_graphid = i; }

    int reuseid()               { return (db_reuseid); }
    void set_reuseid(int i)     { db_reuseid = i; }

    bool active()               { return (db_active); }
    void set_active(bool b)     { db_active = b; }

    bool bad()                  { return (db_bad); }
    void set_bad(bool b)        { db_bad = b; }

    bool call()                 { return (db_call); }

    void set_points(int sz, double *p)
        {
            db_numpts = sz;
            db_index = 0;
            if (db_ptmode)
                delete [] db_a.ipoints;
            else
                delete [] db_a.dpoints;
            db_a.dpoints = p;
            db_ptmode = false;
        }

    void set_points(int sz, int *p)
        {
            db_numpts = sz;
            db_index = 0;
            if (db_ptmode)
                delete [] db_a.ipoints;
            else
                delete [] db_a.dpoints;
            db_a.ipoints = p;
            db_ptmode = true;
        }

private:
    sDbComm *db_next;           // List of active debugging commands.
    sDbComm *db_also;           // Link for conjunctions.
    char *db_string;            // Condition or node, text.
    char *db_callfn;            // Name of script to call on stop.
    union {                     // Output point for test:
        double dpoint;          //   Value.
        int ipoint;             //   Plot point index.
    } db_p;
    int db_number;              // The number of this debugging command.
    DBtype db_type;             // One of the above.
    int db_graphid;             // If iplot, id of graph.
    int db_reuseid;             // Iplot window to reuse.
    bool db_active;             // True if active.
    bool db_bad;                // True if error.
    bool db_ptmode;             // Input to before/after/at in points.
    bool db_call;               // Call script or bound codeblock on stop.
    int db_index;               // Index into the db_points array.
    int db_numpts;              // Size of the db_points array.
    union {                     // Array of points to test for "stop at".
        double *dpoints;
        int *ipoints;
    } db_a;
};

#endif // FTEDEBUG_H

