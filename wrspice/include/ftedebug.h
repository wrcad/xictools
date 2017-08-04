
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

enum DBtype
{
    DB_NONE,
    DB_SAVE,                // Save a node.
    DB_TRACE,               // Print the value of a node every iteration.
    DB_STOPAFTER,           // Break after this many iterations.
    DB_STOPAT,              // Break at this many iterations.
    DB_STOPBEFORE,          // Break before this many iterations.
    DB_STOPWHEN,            // Break when a node reaches this value.
    DB_IPLOT,               // Incrementally plot listed vectors.
    DB_IPLOTALL,            // Incrementally plot everything.
    DB_DEADIPLOT            // Iplot is being destroyed.
};

// Structure to save a debug context as a list element.
//
struct sDbComm
{
    sDbComm()
        {
            db_next = 0;
            db_also = 0;
            db_string = 0;
            db_number = 0;
            db_type = DB_NONE;
            db_point = 0;
            db_graphid = 0;
            db_reuseid = 0;
            db_active = false;
        }

    ~sDbComm()                  { delete [] db_string; }

    static void destroy(sDbComm*); // destroy this debug and descendents
    bool istrue();              // evaluate true if condition met
    bool should_stop(int);      // true if stop condition met
    void print(char**);         // print, in string if given, the debug msg
    bool print_trace(sPlot*, bool*, int);  // print trace output
    void printcond(char**);     // print the conditional expression

    sDbComm *next()             { return (db_next); }
    void set_next(sDbComm *d)   { db_next = d; }

    sDbComm *also()             { return (db_also); }
    void set_also(sDbComm *d)   { db_also = d; }

    const char *string()        { return (db_string); }
    void set_string(char *s)    { db_string = s; }

    int number()                { return (db_number); }
    void set_number(int i)      { db_number = i; }

    DBtype type()               { return (db_type); }
    void set_type(DBtype t)     { db_type = t; }

    int point()                 { return (db_point); }
    void set_point(int i)       { db_point = i; }

    int graphid()               { return (db_graphid); }
    void set_graphid(int i)     { db_graphid = i; }

    int reuseid()               { return (db_reuseid); }
    void set_reuseid(int i)     { db_reuseid = i; }

    bool active()               { return (db_active); }
    void set_active(bool b)     { db_active = b; }

private:
    sDbComm *db_next;           // List of active debugging commands.
    sDbComm *db_also;           // Link for conjunctions.
    char *db_string;            // Condition or node, text.
    int db_number;              // The number of this debugging command.
    DBtype db_type;             // One of the above.
    int db_point;               // Output point for test.
    int db_graphid;             // If iplot, id of graph.
    int db_reuseid;             // Iplot window to reuse.
    bool db_active;             // True if active.
};


// Container for debug list bases, etc.
//
struct sDebug
{
    sDebug()
        {
            sd_iplot = 0;
            sd_trace = 0;
            sd_save = 0;
            sd_stop = 0;
            sd_debugcnt = 1;
            sd_stepcnt = 0;
            sd_steps = 0;
        };

    bool isset() { return (sd_iplot || sd_trace || sd_save || sd_stop); }

    sDbComm *iplots()           { return (sd_iplot); }
    void set_iplots(sDbComm *d) { sd_iplot = d; }

    sDbComm *traces()           { return (sd_trace); }
    void set_traces(sDbComm *d) { sd_trace = d; }

    sDbComm *saves()            { return (sd_save); }
    void set_saves(sDbComm *d)  { sd_save = d; }

    sDbComm *stops()            { return (sd_stop); }
    void set_stops(sDbComm *d)  { sd_stop = d; }

    int new_count()             { return (sd_debugcnt++); }
    int decrement_count()       { return (sd_debugcnt--); }

    int step_count()            { return (sd_stepcnt); }
    void set_step_count(int i)  { sd_stepcnt = i; }
    int dec_step_count()        { return (--sd_stepcnt); }

    int num_steps()             { return (sd_steps); }
    void set_num_steps(int i)   { sd_steps = i; }

private:
    sDbComm *sd_iplot;          // iplot list head
    sDbComm *sd_trace;          // trace list head
    sDbComm *sd_save;           // save list head
    sDbComm *sd_stop;           // stop after/when list head

    int sd_debugcnt;       // running sum of debugs created, provides id
    int sd_stepcnt;        // number of steps done
    int sd_steps;          // number of steps to do
};

extern sDebug DB;

#endif // FTEDEBUG_H

