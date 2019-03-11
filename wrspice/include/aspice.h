
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
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef ASPICE_H
#define ASPICE_H

#include "miscutil/lstring.h"


//
// Stuff for asynchronous spice runs.
//

struct sCHECKprms;

struct sJobc
{
    struct rjob_t;

    // Struct to hold info on remote processes.
    struct proc_t
    {
        proc_t(int p, const char *n, const char *r, const char *i,
            const char *o, rjob_t *j)
            {
                pr_name = lstring::copy(n);
                pr_rawfile = lstring::copy(r);
                pr_inpfile = lstring::copy(i);
                pr_outfile = lstring::copy(o);
                pr_rjob = j;
                pr_next = 0;
                pr_pid = p;
                pr_saveout = false;
                pr_tempinp = false;
            }

        ~proc_t()
            {
                delete [] pr_name;
                delete [] pr_rawfile;
                delete [] pr_inpfile;
                delete [] pr_outfile;
            }

        rjob_t *rjob()                  { return (pr_rjob); }
        const char *name()              { return (pr_name); }
        const char *rawfile()           { return (pr_rawfile); }
        const char *inpfile()           { return (pr_inpfile); }
        const char *outfile()           { return (pr_outfile); }

        bool tempinp()                  { return (pr_tempinp); }
        void set_tempinp(bool b)        { pr_tempinp = b; }
        bool saveout()                  { return (pr_saveout); }
        void set_saveout(bool b)        { pr_saveout = b; }
        proc_t *next()                  { return (pr_next); }
        void set_next(proc_t *p)        { pr_next = p; }
        int pid()                       { return (pr_pid); }

    private:
        char *pr_name;          // The name of the spice run.
        char *pr_rawfile;       // The temporary raw file.
        char *pr_inpfile;       // The name of the input file.
        char *pr_outfile;       // The name of the (tmp) output file.
        rjob_t *pr_rjob;        // Set when job is an operating range point.
        proc_t *pr_next;        // Link.
        int pr_pid;             // The pid of the spice job.
        bool pr_saveout;        // Don't unlink the output file.
        bool pr_tempinp;        // Unlink input file.
    };

    // Struct to hold info on remote servers.
    struct rserv_t
    {
        rserv_t(const char *h, const char *p)
            {
                rs_host = lstring::copy(h);
                rs_program = lstring::copy(p);
                rs_joblist = 0;
                rs_next = 0;
                rs_numjobs = 0;
                rs_clearing = false;
            }

        ~rserv_t()
            {
                delete [] rs_host;
                delete [] rs_program;
            }

        char *host()                    { return (rs_host); }
        char *program()                 { return (rs_program); }
        proc_t *joblist()               { return (rs_joblist); }
        void set_joblist(proc_t *j)     { rs_joblist = j; }

        rserv_t *next()                 { return (rs_next); }
        void set_next(rserv_t *n)       { rs_next = n; }

        int numjobs()                   { return (rs_numjobs); }
        void set_numjobs(int n)         { rs_numjobs = n; }

        bool clearing()                 { return (rs_clearing); }
        void set_clearing(bool b)       { rs_clearing = b; }

    private:
        char *rs_host;
        char *rs_program;
        proc_t *rs_joblist;
        rserv_t *rs_next;
        int rs_numjobs;
        bool rs_clearing;
    };

    // Struct to hold values for looping jobs such as operating range
    // analysis.
    //
    struct rjob_t
    {
        rjob_t(const char *n, sCHECKprms *cj)
            {
                i = 0;
                j = 0;
                rj_name = lstring::copy(n);
                rj_job = cj;
                rj_next = 0;
            }

        ~rjob_t()
            {
                delete [] rj_name;
                delete rj_job;
            }

        const char *name()              { return (rj_name); }
        sCHECKprms *job()               { return (rj_job); }

        rjob_t *next()                  { return (rj_next); }
        void set_next(rjob_t *n)        { rj_next = n; }

        int i, j;

    private:
        char *rj_name;
        sCHECKprms *rj_job;
        rjob_t *rj_next;
    };

    // Keep a list of jobs started and their completion status.  For
    // Win32, status is 0 inprogress, 1 done OK, 2 done error.  For
    // UNIX, the status is the precess exit status.
    //
    struct sdone_t
    {
        sdone_t(int p, int s)
            {
                pid = p;
                status = s;
                next = 0;
            }

        int pid;
        int status;
        sdone_t *next;
    };

    sJobc()
        {
            jc_jobs = 0;
            jc_servers = 0;
            jc_complete_list = 0;
            jc_numchanged = 0;
        }

    void rhost(wordlist*);
    void jobs();
    void check_jobs();
    void register_job(sCHECKprms*);

    bool submit(const char*, const char*, const char*, const char*,
        sFtCirc*, rjob_t*);
    bool submit_local(const char*, const char*, const char*, sFtCirc*,
        rjob_t*, const char*);
    const char *gethost();

private:
    void add_done(int cur_pid, int cur_status)
        {
            sdone_t *s = new sdone_t(cur_pid, cur_status);
            if (!jc_complete_list)
                jc_complete_list = s;
            else {
                sdone_t *j = jc_complete_list;
                while (j->next)
                    j = j->next;
                j->next = s;
            }
        }

#ifdef WIN32
    static void th_hdlr(void *arg);
    static void th_local_hdlr(void*);
#else
    static void sigchild(int, int, void*);
#endif

    rjob_t *jc_jobs;
    rserv_t *jc_servers;
    sdone_t *jc_complete_list;
    int jc_numchanged;  // How many children have changed in state.
};


#endif

