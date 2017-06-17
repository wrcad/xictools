
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
 $Id: aspice.cc,v 2.97 2015/11/19 04:49:03 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

//
// Stuff for asynchronous spice runs.
//

#include "config.h"
#include "spglobal.h"
#include "frontend.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "inpline.h"
#include "outplot.h"
#include "outdata.h"
#include "services.h"
#include "pathlist.h"
#include "filestat.h"
#include "childproc.h"

#ifdef HAVE_SOCKET

#include <sys/types.h>
#ifdef WIN32
#include "msw.h"
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#ifdef HAVE_SIGNAL
#include <signal.h>
#endif


#ifdef WIN32
#define CLOSESOCKET(x) shutdown(x, SD_SEND), closesocket(x)
#else
#define CLOSESOCKET(x) close(x)
#endif

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

namespace { sJobc Jobc; }


// Start an asynchronous job.
//
void
CommandTab::com_aspice(wordlist *wl)
{
    const char *spicepath = 0;
    VTvalue vv;
    if (Sp.GetVar(kw_spicepath, VTYP_STRING, &vv))
        spicepath = vv.get_string();
    else {
        if (Global.ExecProg() && *Global.ExecProg())
            spicepath = Global.ExecProg();
        else {
            GRpkgIf()->ErrPrintf(ET_MSG,
                "No executable is available for the aspice command.\n");
            return;
        }
    }

    char *deck = 0;
    if (wl) {
        deck = wl->wl_word;
        wl = wl->wl_next;
    }
    char *output = 0;
    if (wl)
        output = wl->wl_word;
    Jobc.submit_local(spicepath, 0, deck, Sp.CurCircuit(), 0, output);
}


// Run a spice job remotely. See the description of the spice daemon for
// the protocol. This is 4.2 specific.
//
void
CommandTab::com_rspice(wordlist *wl)
{
    const char *umsg =
    "Usage: rspice  filename | [-h host][-p program][-f filename][analysis]\n";
    char rhost[64], program[128];
    *rhost = '\0';
    *program = '\0';
    char *filename = 0;
    if (wl && !wl->wl_next) {
        filename = wl->wl_word;
        wl = 0;
    }
    char *analysis = 0;
    while (wl) {
        if (*wl->wl_word == '-') {
            wordlist *next = wl->wl_next;
            if (!next) {
                GRpkgIf()->ErrPrintf(ET_ERROR, umsg);
                return;
            }
            switch (wl->wl_word[1]) {
            case 'f':
                filename = next->wl_word;
                wl = next->wl_next;
                break;
            case 'p':
                strcpy(program, next->wl_word);
                wl = next->wl_next;
                break;
            case 'h':
                strcpy(rhost, next->wl_word);
                wl = next->wl_next;
                break;
            default:
                GRpkgIf()->ErrPrintf(ET_ERROR, umsg);
                return;
            }
        }
        else {
            analysis = wl->flatten();
            break;
        }
    }

    if (!*rhost)
        strcpy(rhost, Jobc.gethost());
    if (!*program) {
        VTvalue vv;
        if (Sp.GetVar(kw_rprogram, VTYP_STRING, &vv))
            strcpy(program, vv.get_string());
        else {
            if (Sp.GetVar(kw_spicepath, VTYP_STRING, &vv))
                strcpy(program, vv.get_string());
            else {
                if (Global.ExecProg() && *Global.ExecProg())
                    strcpy(program, Global.ExecProg());
                else
                    strcpy(program, CP.Program());
            }
        }
    }
    Jobc.submit(rhost, program, analysis, filename, Sp.CurCircuit(), 0);
    delete [] analysis;
}


void
CommandTab::com_rhost(wordlist *wl)
{
    Jobc.rhost(wl);
}


void
CommandTab::com_jobs(wordlist*)
{
    Jobc.jobs();
}
// End of CommandTab functions.


// This gets called every once in a while, and checks to see if any
// jobs have finished. If they have it gets the data.  The problem is
// that wait(0) is probably more portable, but it can't tell
// whether the exit was normal or not.
//
void
IFsimulator::CheckAsyncJobs()
{
    Jobc.check_jobs();
}


void
sCHECKprms::registerJob()
{
    Jobc.register_job(this);
}


// End the analysis, free storage.
//
void
sCHECKprms::endJob()
{
    if (ch_op && ch_opname)
        TTY.printf_force(
            "%s analysis complete.  Data in file %s.\n", ch_monte ?
                "Monte Carlo" : "Operating range", ch_opname);

    if (ch_tmpout) {
        TTY.ioOverride(0, ch_op, 0);
        fclose(ch_tmpout);
        ch_tmpout = 0;
        unlink(ch_tmpoutname);
        delete [] ch_tmpoutname;
    }
    else {
        if (ch_op && ch_op != TTY.outfile())
            fclose(ch_op);
    }
    GP.MpDone(ch_graphid);

    if (ch_iterno > 0 && !ch_doall)
        set_rangevec();
    out_cir->set_check(0);
    OP.endPlot(out_rundesc, true);
}


// Process the output from an asynchronous run.  If there is nothing
// useful in the file, return true.
//
bool
sCHECKprms::processReturn(const char *fname)
{
    FILE *fp = fopen(fname, "r");
    if (!fp) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't open %s.\n", fname);
        return (true);
    }

    bool goodstuff = false;
    char string1[80], string2[80], string3[80];
    char buf[BSIZE_SP];
    int num1 = 2*ch_step1 + 1;
    while (fgets(buf, 80, fp)) {
        int pf, d1, d2;
        if (GP.MpParse(buf, &d1, &d2, string1, string2, string3)) {
            goodstuff = true;
            if (lstring::eq(string3, "PASS"))
                pf = 1;
            else
                pf = 0;
            bool mcrun = false;
            if (lstring::eq(string1, "run")) {
                sprintf(string2, "%d", 
                    (d2 + ch_step2)*(2*ch_step1 + 1) + d1 + ch_step1);
                mcrun = true;
            }
            if (GP.MpWhere(ch_graphid, d1, d2) && !ch_batchmode) {
                if (mcrun)
                    TTY.printf_force("%3d %3d %3s %3s\t\t%s\n", d1, d2,
                        string1, string2, string3);
                else
                    TTY.printf_force("%3d %3d %12s %12s\t\t%s\n", d1, d2,
                        string1, string2, string3);
            }
            GP.MpMark(ch_graphid, pf);
            if (ch_op) {
                if (mcrun)
                    fprintf(ch_op,
                        "[DATA] %3d %3d %3s %3s\t\t%s\n", d1, d2, string1,
                        string2, string3);
                else
                    fprintf(ch_op,
                        "[DATA] %3d %3d %12s %12s\t\t%s\n", d1, d2, string1,
                        string2, string3);
            }
            char *flag = ch_flags + (d2 + ch_step2)*num1 + d1;
            *flag = 1 + (1-pf);
        }
        else if (ch_op)
            fprintf(ch_op, "%s", buf);
    }
    fclose(fp);
    if (goodstuff)
        return (false);
    return (true);
}


// Compute the indices of the next trial.  Return true if no more
// trials to do.
//
int
sCHECKprms::nextTask(int *pi, int *pj)
{
    int i, j, last;
    char *rowflags;
    int num1 = 2*ch_step1 + 1;
    if (ch_doall) {
        for (j = -ch_step2; j <= ch_step2; j++) {
            rowflags = ch_flags + (j + ch_step2)*num1;
            for (i = -ch_step1; i <= ch_step1; i++, rowflags++) {
                if (*rowflags) continue;
                *rowflags = -1;
                *pi = i;
                *pj = j;
                return (false);
            }
        }
        return (true);
    }

    // Find the range along the rows
    if (ch_step1 > 0 || ch_step2 == 0) {
        for (j = -ch_step2; j <= ch_step2; j++) {
            rowflags = ch_flags + (j + ch_step2)*num1;
            for (last = i = -ch_step1; i <= ch_step1; i++, rowflags++) {
                if (*rowflags == 2) continue;
                if (*rowflags == 1) break;
                if (*rowflags == -1) break;
                *rowflags = -1;
                *pi = i;
                *pj = j;
                return (false);
            }
            last = i;

            rowflags = ch_flags + (j + ch_step2 + 1)*num1 - 1;
            for (i = ch_step1; i > last; i--, rowflags--) {
                if (*rowflags == 2) continue;
                if (*rowflags == 1) break;
                if (*rowflags == -1) break;
                *rowflags = -1;
                *pi = i;
                *pj = j;
                return (false);
            }
        }
    }

    // Now check the columns, fill in any missing points.
    //
    if (ch_step2 > 0) {
        for (i = -ch_step1; i <= ch_step1; i++) {
            rowflags = ch_flags + i + ch_step1;
            for (last = j = -ch_step2; j <= ch_step2;
                    j++, rowflags += num1) {
                if (*rowflags == 2) continue;
                if (*rowflags == 1) break;
                if (*rowflags == -1) break;
                *rowflags = -1;
                *pi = i;
                *pj = j;
                return (false);
            }
            last = j;

            rowflags = ch_flags + i + ch_step1 + 2*ch_step2*num1;
            for (j = ch_step2; j > last; j--, rowflags -= num1) {
                if (*rowflags == 2) continue;
                if (*rowflags == 1) break;
                if (*rowflags == -1) break;
                *rowflags = -1;
                *pi = i;
                *pj = j;
                return (false);
            }
        }
    }
    return (true);
}
// End of cJobs functions.


void
sJobc::rhost(wordlist *wl)
{
    if (!wl) {
        // print the hosts
        wordlist *w0 = 0, *wn = 0;
        for (rserv_t *s = jc_servers; s; s = s->next()) {
            if (!w0)
                w0 = wn = new wordlist(s->host(), 0);
            else {
                wn->wl_next = new wordlist(s->host(), wn);
                wn = wn->wl_next;
            }
        }
        if (w0) {
            w0->sort();
            TTY.wlprint(w0);
            w0->free();
            TTY.printf("\n");
        }
        else if (TTY.outfile() == stdout)
            TTY.printf("No servers given.\n");
        return;
    }
    bool adding = true;
    while (wl) {
        if (*wl->wl_word == '-') {
            if (wl->wl_word[1] == 'a')
                adding = true;
            else if (wl->wl_word[1] == 'd')
                adding = false;
            else
                GRpkgIf()->ErrPrintf(ET_WARN, "unknown option, ignored.\n");
            wl = wl->wl_next;
            continue;
        }
        if (adding) {
            rserv_t *s;
            for (s = jc_servers; s; s = s->next())
                if (lstring::eq(wl->wl_word, s->host()))
                    break;
            if (s) {
                // already there
                wl = wl->wl_next;
                continue;
            }
            if (!gethostbyname(wl->wl_word)) {
                GRpkgIf()->ErrPrintf(ET_WARN, "unknown host, ignored.\n");
                wl = wl->wl_next;
                continue;
            }
            s = new rserv_t(wl->wl_word, 0);
            s->set_next(jc_servers);
            jc_servers = s;
        }
        else {
            rserv_t *s;
            for (s = jc_servers; s; s = s->next())
                if (lstring::eq(wl->wl_word, s->host()))
                    break;
            if (s)
                s->set_clearing(true);
        }
        wl = wl->wl_next;
    }
}


void
sJobc::jobs()
{
    for (rserv_t *s = jc_servers; s; s = s->next()) {
        if (s->joblist())
            TTY.printf("%s:\n", s->host());
        for (proc_t *p = s->joblist(); p; p = p->next())
            TTY.printf("%d\t%.70s\n", p->pid(), p->name());
    }
}


void
sJobc::check_jobs()
{
    static bool here = false;   // Don't want to be re-entrant
    if (here)
        return;
    here = true;

    char buf[BSIZE_SP];
    bool dopr = false;
    while (jc_numchanged > 0 && jc_complete_list) {

#ifdef WIN32
        // find a completed job, and remove it from the list
        sdone_t *sp = 0, *sn;
        int pid = -1;
        for (sdone_t *sd = jc_complete_list; sd; sd = sn) {
            sn = sd->next;
            if (sd->status) {
                if (sp)
                    sp->next = sn;
                else
                    jc_complete_list = sn;
                pid = sd->pid;
                delete sd;
                break;
            }
            sp = sd;
        }
#else
        int pid = jc_complete_list->pid;
        sdone_t *jd = jc_complete_list;
        jc_complete_list = jc_complete_list->next;
        delete jd;
#endif

        proc_t *p =0, *lp = 0;
        rserv_t *s;
        for (s = jc_servers; s; s = s->next()) {
            lp = 0;
            for (p = s->joblist(); p; lp = p, p = p->next()) {
                if (p->pid() == pid)
                    break;
            }
            if (p)
                break;
        }
        if (p == 0) {
            GRpkgIf()->ErrPrintf(ET_INTERR,
                "CheckAsyncJobs: process %d not found.\n", pid);
            here = false;
            return;
        }
        jc_numchanged--;
        s->set_numjobs(s->numjobs() - 1);
        if (p == s->joblist())
            s->set_joblist(p->next());
        else
            lp->set_next(p->next());
        if (p->rjob()) {
            rjob_t *rj = p->rjob();
            if (rj->job()->processReturn(p->rawfile())) {
                // uh oh, bad
                rj->job()->set_nogo(true);
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "no data returned from run, aborting.\n");
            }
            unlink(p->rawfile());
            if (p->inpfile() && p->tempinp())
                unlink(p->inpfile());
            if (p->outfile())
                unlink(p->outfile());
            if (!rj->job()->nogo()) {
                if (!rj->job()->nextTask(&rj->i, &rj->j)) {
                    if (submit(s->host(), s->program(), 0, 0,
                            rj->job()->out_cir, rj)) {
                        int num1 = 2*rj->job()->step1() + 1;
                        rj->job()->set_pflag(
                            (rj->j + rj->job()->step2())*num1 + rj->i, 0);
                    }
                }
            }
        }
        else {
            TTY.printf("Job finished - %.60s\n", p->name());
            const char *stmp = p->rawfile();
            if (stmp) {
                Sp.LoadFile(&stmp, false);
                unlink(p->rawfile());
            }
            if (p->inpfile() && p->tempinp())
                unlink(p->inpfile());
            if (p->outfile()) {
                FILE *fp;
                if ((fp = fopen(p->outfile(), "r")) != 0) {
                    int cnt = 0;
                    while (fgets(buf, BSIZE_SP, fp)) {
                        cnt++;
                        // strip the copyright, etc
                        if (cnt <= 3)
                            continue;
                        // strip some other crap
                        if (lstring::eq(buf, "async\n"))
                            continue;
                        if (lstring::eq(buf, "@\n"))
                            continue;
                        TTY.send(buf);
                    }
                    fclose(fp);
                }
                else
                    GRpkgIf()->Perror(p->outfile());
                if (!p->saveout())
                    unlink(p->outfile());
            }
            TTY.send("-----\n");
            dopr = true;
        }
        // finished with this proc
        delete p;
    }
    // remove cleared servers from list
    rserv_t *slast = 0, *sn;
    for (rserv_t *s = jc_servers; s; s = sn) {
        sn = s->next();
        if (s->clearing() && !s->numjobs()) {
            if (slast)
                slast->set_next(sn);
            else
                jc_servers = sn;
            delete s;
            continue;
        }
        slast = s;
    }
    // see if any rjobs are done
    rjob_t *rl = 0, *rn;
    for (rjob_t *rj = jc_jobs; rj; rl = rj, rj = rn) {
        rn = rj->next();
        for (rserv_t *s = jc_servers; s; s = s->next()) {
            for (proc_t *p = s->joblist(); p; p = p->next()) {
                if (p->rjob() == rj)
                    goto skip;
            }
        }
        // done
        // unlink and free rjob struct
        if (!rl)
            jc_jobs = rn;
        else
            rl->set_next(rn);
        TTY.printf("Job finished - %.60s\n", rj->name());
        rj->job()->endJob();
        delete rj;
        rj = rl;
        dopr = true;
skip:
    ;
    }
    if (dopr)
        CP.Prompt();
    here = false;
}


void
sJobc::register_job(sCHECKprms *job)
{
    char buf[128];
    sprintf(buf, "%s %s", job->out_cir->name(),
        job->monte() ? "Monte Carlo" : "Operating Range");
    rjob_t *rj = new rjob_t(buf, job);
    rj->set_next(jc_jobs);
    jc_jobs = rj;

    // submit one task to each server
    int cnt = 0;
    for (rserv_t *s = jc_servers; s; s = s->next()) {
        if (s->clearing())
            continue;
        if (job->nextTask(&rj->i, &rj->j))
            // no more jobs
            return;
        if (submit(s->host(), s->program(), 0, 0, job->out_cir, rj)) {
            int num1 = 2*job->step1() + 1;
            job->set_pflag((rj->j + job->step2())*num1 + rj->i, 0);
            continue;
        }
        cnt++;
    }
    if (!cnt) {
        const char *program = 0;
        VTvalue vv;
        if (!Sp.GetVar(kw_rprogram, VTYP_STRING, &vv))
            program = vv.get_string();
        else {
            if (Sp.GetVar(kw_spicepath, VTYP_STRING, &vv))
                program = vv.get_string();
            else {
                if (Global.ExecProg() && *Global.ExecProg())
                    program = Global.ExecProg();
                else
                    program = CP.Program();
            }
        }
        if (job->nextTask(&rj->i, &rj->j))
            // no more jobs
            return;
        if (submit("local", program, 0, 0, job->out_cir, rj)) {
            int num1 = 2*job->step1() + 1;
            job->set_pflag((rj->j + job->step2())*num1 + rj->i, 0);
        }
    }
}



#ifdef WIN32

struct t_data
{
    t_data(char *o, int s, int p) { outfile = o; sfd = s; pid = p; }

    const char *outfile;
    int sfd;
    int pid;
};

#endif


namespace {
    // Return true if s points to a string of digits terminated by white   
    // space or null.
    //
    bool check_digits(const char *s)
    {
        if (!isdigit(*s))
            return (false);
        while (*++s) {
            if (isspace(*s))
                break;
            if (!isdigit(*s))
                return (false);
        }
        return (true);
    }
}


bool
sJobc::submit(const char *host, const char *program, const char *analysis,
    const char *filename, sFtCirc *cir, rjob_t *rj)
{
    if (!host || !*host)
        host = gethost();
    if (lstring::eq(host, "local"))
        return (submit_local(program, analysis, filename, cir, rj, 0));

    char localhost[64];
    if (gethostname(localhost, 64) > 0) {
        GRpkgIf()->Perror("gethostname");
        return (true);
    }

    char hostname[256];
    strcpy(hostname, host);
    // host can have :portnum appended, hostname does not

    unsigned short wrspiced_port = 0;
    // Look for :portnum in hostname, if found set wrspiced_port
    // and strip this out.
    char *col = strrchr(hostname, ':');
    if (col && check_digits(col+1)) {
        wrspiced_port = atoi(col+1);
        *col = 0;
    }
    if (wrspiced_port <= 0) {
        servent *sp = getservbyname(WRSPICE_SERVICE, "tcp");
        if (sp)
            wrspiced_port = ntohs(sp->s_port);
        else
            wrspiced_port = WRSPICE_PORT;
    }
    protoent *pp = getprotobyname("tcp");
    if (pp == 0) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "tcp: unknown protocol.\n");
        return (true);
    }
    hostent *hp = gethostbyname(hostname);
    if (hp == 0) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "unknown host %s.\n", hostname);
        return (true);
    }
    sockaddr_in server;
    memset(&server, 0, sizeof(struct sockaddr_in));
    memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family = hp->h_addrtype;
    server.sin_port = htons(wrspiced_port);

    // Create the socket
    int sfd = socket(AF_INET, SOCK_STREAM, pp->p_proto);
    if (sfd < 0) {
        GRpkgIf()->Perror("socket");
        return (true);
    }

    if (connect(sfd, (struct sockaddr *) &server, 
            sizeof (struct sockaddr)) < 0) {
        GRpkgIf()->Perror("connect");
        return (true);
    }

    // Now we are ready to do the stuff
    char buf[BSIZE_SP];
    char *user = pathlist::get_user_name(false);
    for (char *t = user; *t; t++) {
        // in Win32, this may have spaces
        if (isspace(*t))
            *t = '_';
    }
    if (*program)
        sprintf(buf, "%s %s %s", user, localhost, program);
    else
        sprintf(buf, "%s %s", user, localhost);
    delete [] user;
    send(sfd, buf, strlen(buf) + 1, 0);      // Get the trailing \0
    if (recv(sfd, buf, BSIZE_SP, 0) <= 0) {
        GRpkgIf()->ErrPrintf(ET_MSG, "Connection closed.\n");
        CLOSESOCKET(sfd);
        return (true);
    }

    if (lstring::eq(buf, "toomany")) {
        GRpkgIf()->ErrPrintf(ET_MSG,
            "\nJob refused by %s: load exceeded.\n"
            "Please use another host or try again later.\n", hostname);
        CLOSESOCKET(sfd);
        return (true);
    }
    else if (!lstring::eq(buf, "ok")) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "Job refused by %s: %s.\n", hostname,
            buf);
        CLOSESOCKET(sfd);
        return (true);
    }

    // Send the circuit over
    const char *name, *deck;
    if (filename) {
        deck = filename;
        FILE *inp;
        if (!(inp = Sp.PathOpen(deck, "r"))) {
            GRpkgIf()->Perror(deck);
            CLOSESOCKET(sfd);
            return (true);
        }
        sprintf(buf, "%s: ", hostname);
        char *t = buf + strlen(buf);
        if (!fgets(t, BSIZE_SP, inp)) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "bad deck %s.\n", deck);
            fclose(inp);
            CLOSESOCKET(sfd);
            return (true);
        }
        for ( ; *t && (*t != '\n'); t++) ;
        *t = '\0';
        name = lstring::copy(buf);
        rewind(inp);
        int i;
        while ((i = fread(buf, 1, BSIZE_SP, inp)) > 0)
            send(sfd, buf, i, 0);
        if (analysis) {
            sprintf(buf, "%s\n%s\n%s\n", CBLK_KW, analysis, ENDC_KW);
            send(sfd, buf, strlen(buf), 0);
        }
        send(sfd, "@\n", 3, 0);
    }
    else {
        if (!cir) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "no circuits loaded.\n");
            CLOSESOCKET(sfd);
            return (true);
        }
            
        // We have to make a FILE struct for the socket
        deck = cir->name();
        sprintf(buf, "%s: %s", hostname, cir->deck()->line());
        name = lstring::copy(buf);
        FILE *inp = fdopen(sfd, "w");
        if (rj) {
            fprintf(inp, "%s\n", cir->deck()->line());
            if (rj->job()->monte())
                fprintf(inp, "%s\n", MONTE_KW);
            else
                fprintf(inp, "%s\n", CHECK_KW);
            fprintf(inp, "%s\n", EBLK_KW);
            if (cir->execs()->name())
                CP.PrintBlock(cir->execs()->name(), inp);
            else if (cir->execs()->tree())
                CP.PrintControl(cir->execs()->tree(), inp);
            if (!rj->job()->monte()) {
                double v1 = rj->job()->val1() + rj->i*rj->job()->delta1();
                double v2 = rj->job()->val2() + rj->j*rj->job()->delta2();
                // "central" values
                fprintf(inp, "let checkVAL1 = %g\n", v1);
                fprintf(inp, "let checkVAL2 = %g\n", v2);
            }
            // So that i, j is printed in output
            fprintf(inp, "let checkSTP1 = %d\n", -rj->i);
            fprintf(inp, "let checkSTP2 = %d\n", -rj->j);
            fprintf(inp, "%s\n", ENDC_KW);
            fprintf(inp, "%s\n", CBLK_KW);
            if (cir->controls()->name())
                CP.PrintBlock(cir->controls()->name(), inp);
            else
                CP.PrintControl(cir->controls()->tree(), inp);
            fprintf(inp, "%s\n", ENDC_KW);
            Sp.Listing(inp, cir->origdeck()->next(), cir->options(),
                LS_DECK);
        }
        else {
            Sp.Listing(inp, cir->deck(), cir->options(), LS_DECK);
            if (analysis)
                fprintf(inp, "%s\n%s\n%s\n", CBLK_KW, analysis, ENDC_KW);
        }
        fputs("@\n", inp);
        fflush(inp);
    }

    // Now wait for things to come through
    TTY.init_more();
    TTY.printf("Starting remote spice run:\n%s\n", name);
    char *outfile = filestat::make_temp("rsp");

#ifdef WIN32

    bool noerr = true;
    while (noerr) {
        int cnt = 0;
        for (;;) {
            if (recv(sfd, buf + cnt, 1, 0) > 0)
                cnt++;
            else {
                buf[cnt] = 0;
                noerr = false;
                break;
            }
            if (buf[cnt-1] == '\n') {
                buf[cnt] = 0;
                break;
            }
        }
        if (*buf == '@')
            break;
        TTY.send(buf);
    }

    // Start a thread to monitor the process, and add an entry to the
    // job list.
    t_data *th = new t_data(outfile, sfd, 0);
    th->pid = _beginthread(th_hdlr, 0, th);
    add_done(th->pid, 0);
    int pid = th->pid;

#else

    FILE *serv = fdopen(sfd, "r");
    while (fgets(buf, BSIZE_SP, serv) != 0) {
        if (*buf == '@')
            break;
        TTY.send(buf);
    }

    int pid = fork();
    if (pid == 0) {
        FILE *out;
        if (!(out = fopen(outfile, "w"))) {
            GRpkgIf()->Perror(outfile);
            fclose(serv);
            _exit(EXIT_BAD);
        }
        int i;
        while ((i = fread(buf, 1, BSIZE_SP, serv)) != 0)
            fwrite(buf, 1, i, out);
        fclose(out);
        fclose(serv);
        _exit(0);
    }
    else
        fclose(serv);

#endif

    // Add this one to the job list
    proc_t *p = new proc_t(pid, name, outfile, deck, 0, rj);
    delete [] name;
    delete [] outfile;
    if (!filename)
        p->set_tempinp(true);
    rserv_t *s;
    for (s = jc_servers; s; s = s->next())
        if (lstring::eq(host, s->host()))
            break;
    if (s) {
        p->set_next(s->joblist());
        s->set_joblist(p);
    }
    else {
        s = new rserv_t(host, program);
        s->set_joblist(p);
        s->set_next(jc_servers);
        jc_servers = s;
    }
    s->set_numjobs(s->numjobs() + 1);
#ifndef WIN32
    Proc()->RegisterChildHandler(pid, sigchild, 0);
#endif
    return (false);
}


#ifdef WIN32

// Wait for the process to complete, and set the done flag

struct thdata
{
    thdata(HANDLE p, HANDLE i, HANDLE o, pid_t id)
        { process = p; infile = i; outfile = o; pid = id; }

    HANDLE process;
    HANDLE infile;
    HANDLE outfile;
    pid_t pid;
};

#endif


bool
sJobc::submit_local(const char *program, const char *analysis,
    const char *filename, sFtCirc *cir, rjob_t *rj, const char *outfile)
{
    bool saveout;
    if (outfile)
        saveout = true;
    else {
        outfile = filestat::make_temp("spout");
        saveout = false;
    }

    bool tempinp = false;
    if (!filename) {
        if (!cir) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "no circuits loaded.\n");
            return (true);
        }
            
        filename = filestat::make_temp("sptmp");
        FILE *inp;
        if ((inp = fopen(filename, "w")) == 0) {
            GRpkgIf()->Perror(filename);
            return (true);
        }
        if (rj) {
            fprintf(inp, "%s\n", cir->deck()->line());
            if (rj->job()->monte())
                fprintf(inp, "%s\n", MONTE_KW);
            else
                fprintf(inp, "%s\n", CHECK_KW);
            fprintf(inp, "%s\n", EBLK_KW);
            if (cir->execs()->name())
                CP.PrintBlock(cir->execs()->name(), inp);
            else if (cir->execs()->tree())
                CP.PrintControl(cir->execs()->tree(), inp);
            if (!rj->job()->monte()) {
                double v1 = rj->job()->val1() + rj->i*rj->job()->delta1();
                double v2 = rj->job()->val2() + rj->j*rj->job()->delta2();
                // "central" values
                fprintf(inp, "let checkVAL1 = %g\n", v1);
                fprintf(inp, "let checkVAL2 = %g\n", v2);
            }
            // So that i, j is printed in output
            fprintf(inp, "let checkSTP1 = %d\n", -rj->i);
            fprintf(inp, "let checkSTP2 = %d\n", -rj->j);
            fprintf(inp, "%s\n", ENDC_KW);
            fprintf(inp, "%s\n", CBLK_KW);
            if (cir->controls()->name())
                CP.PrintBlock(cir->controls()->name(), inp);
            else
                CP.PrintControl(cir->controls()->tree(), inp);
            fprintf(inp, "%s\n", ENDC_KW);
            Sp.Listing(inp, cir->origdeck()->next(), cir->options(),
                LS_DECK);
        }
        else {
            Sp.Listing(inp, cir->deck(), cir->options(), LS_DECK);
            if (analysis)
                fprintf(inp, "%s\n%s\n%s\n", CBLK_KW, analysis, ENDC_KW);
        }
        fflush(inp);
        fclose(inp);
        tempinp = true;
    }

    char buf[BSIZE_SP];
    if (!rj) {
        FILE *inp;
        if ((inp = fopen(filename, "r")) == 0) {
            GRpkgIf()->Perror(filename);
            return (true);
        }
        if (!fgets(buf, BSIZE_SP, inp)) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "bad deck %s.\n", filename);
            fclose(inp);
            return (true);
        }
        char *t;
        for (t = buf; *t && (*t != '\n'); t++) ;
        *t = '\0';
        TTY.printf("Starting spice run for:\n%s\n", buf);
        fclose(inp);
    }

    char *raw = filestat::make_temp("raw");
    // So there isn't a race condition
    fclose(fopen(raw, "w"));

#ifdef WIN32

    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = true;

    // Start a new Spice server process on the local machine to do the run
    HANDLE hin = CreateFile(filename, GENERIC_READ, 0, &sa,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    HANDLE hout = CreateFile(outfile, GENERIC_WRITE, FILE_SHARE_READ,
        &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    char cmdline[256];
    if (*program == '/' || *program == '\\') {
        char *wd = getcwd(0, 256);
        if (!wd)
            wd = lstring::copy("c:");
        wd[2] = 0;
        sprintf(cmdline, "%s%s -S -r %s", wd, program, raw);
        delete [] wd;
    }
    else
        sprintf(cmdline, "%s -S -r %s", program, raw);

    int pid;
    PROCESS_INFORMATION *info =
        msw::NewProcess(cmdline, 0, true, hin, hout, hout);
    if (!info) {
        CloseHandle(hin);
        CloseHandle(hout);
        pid = -1;
    }
    else {
        // success
        pid = info->dwProcessId;
        add_done(pid, 0);
        thdata *t = new thdata(info->hProcess, hin, hout, pid);
        delete info;
        _beginthread(th_local_hdlr, 0, t);
    }

#else

    int pid = fork();
    if (pid == 0) {
        if (!(freopen(filename, "r", stdin))) {
            perror(filename);
            _exit(EXIT_BAD);
        }
        if (!(freopen(outfile, "w", stdout))) {
            perror(outfile);
            _exit(EXIT_BAD);
        }
        dup2(fileno(stdout), fileno(stderr));

        execl(program, program, "-S", "-r", raw, (char*)0);

        // Screwed up
        perror(program);
        _exit(EXIT_BAD);
    }

#endif

    // Add this one to the job list
    proc_t *p = new proc_t(pid, buf, raw, filename, outfile, rj);
    delete [] raw;
    p->set_saveout(saveout);
    p->set_tempinp(tempinp);
    rserv_t *s;
    for (s = jc_servers; s; s = s->next())
        if (lstring::eq(s->host(), "local"))
            break;
    if (s) {
        p->set_next(s->joblist());
        s->set_joblist(p);
    }
    else {
        s = new rserv_t("local", program);
        s->set_joblist(p);
        s->set_next(jc_servers);
        jc_servers = s;
    }
    s->set_numjobs(s->numjobs() + 1);
#ifndef WIN32
    Proc()->RegisterChildHandler(pid, sigchild, 0);
#endif
    return (false);
}


// Return a host name with the fewest active submissions.
//
const char *
sJobc::gethost()
{
    int njobs = 1000;
    const char *host = 0;
    rserv_t *s;
    for (s = jc_servers; s; s = s->next()) {
        if (s->clearing())
            continue;
        if (s->numjobs() < njobs) {
            njobs = s->numjobs();
            host = s->host();
        }
    }
    if (!host) {
        VTvalue vv;
        if (Sp.GetVar(kw_rhost, VTYP_STRING, &vv)) {
            s = new rserv_t(vv.get_string(), 0);
            s->set_next(jc_servers);
            jc_servers = s;
            host = s->host();
        }
        else {
            if (Global.Host() && *Global.Host()) {
                s = new rserv_t(Global.Host(), 0);
                s->set_next(jc_servers);
                jc_servers = s;
                host = s->host();
            }
            else
                host = "local";
        }
    }
    return (host);
}


#ifdef WIN32

// Static function.
// This function executes in its own thread.  When complete, it
// modifies the job completion list and cleans up.  This handles
// the returns from remote jobs only.
//
void
sJobc::th_hdlr(void *arg)
{
    t_data *t = (t_data*)arg;
    char buf[BSIZE_SP];
    FILE *out;
    if (!(out = fopen(t->outfile, "w"))) {
        GRpkgIf()->Perror(t->outfile);
        Jobc.jc_numchanged++;
        for (sdone_t *sd = Jobc.jc_complete_list; sd; sd = sd->next) {
            if (sd->pid == t->pid) {
                sd->status = 2;
                break;
            }
        }
        delete t;
        if (Sp.GetFlag(FT_ASYNCDB))
            GRpkgIf()->ErrPrintf(ET_MSG, "%d jobs done now.\n",
                Jobc.jc_numchanged);
        if (CP.GetFlag(CP_CWAIT))
            Sp.CheckAsyncJobs();
        return;
    }
    int i;
    while ((i = recv(t->sfd, buf, BSIZE_SP, 0)) > 0)
        fwrite(buf, 1, i, out);
    fclose(out);
    Jobc.jc_numchanged++;
    for (sdone_t *sd = Jobc.jc_complete_list; sd; sd = sd->next) {
        if (sd->pid == t->pid) {
            sd->status = 1;
            break;
        }
    }
    delete t;
    if (Sp.GetFlag(FT_ASYNCDB))
        GRpkgIf()->ErrPrintf(ET_MSG, "%d jobs done now.\n",
            Jobc.jc_numchanged);
    if (CP.GetFlag(CP_CWAIT))
        Sp.CheckAsyncJobs();
}


// Static function.
void
sJobc::th_local_hdlr(void *arg)
{
    thdata *t = (thdata*)arg;
    WaitForSingleObject(t->process, INFINITE);
    Jobc.jc_numchanged++;
    CloseHandle(t->infile);
    CloseHandle(t->outfile);
    for (sdone_t *sd = Jobc.jc_complete_list; sd; sd = sd->next) {
        if (sd->pid == t->pid) {
            sd->status = 1;
            break;
        }
    }
    if (Sp.GetFlag(FT_ASYNCDB))
        GRpkgIf()->ErrPrintf(ET_MSG, "%d jobs done now.\n",
            Jobc.jc_numchanged);
    if (CP.GetFlag(CP_CWAIT))
        Sp.CheckAsyncJobs();
    delete t;
}

#else

// Static function.
void
sJobc::sigchild(int pid, int status, void*)
{
    if (WIFEXITED(status)) {
        Jobc.jc_numchanged++;
        Jobc.add_done(pid, WEXITSTATUS(status));
        if (Sp.GetFlag(FT_ASYNCDB)) {
            GRpkgIf()->ErrPrintf(ET_MSG,
                "process %d exited with status %d.\n",
                pid, WEXITSTATUS(status));
        }
        if (CP.GetFlag(CP_CWAIT))
            Sp.CheckAsyncJobs();
    }
    else if (WIFSIGNALED(status)) {
        Jobc.jc_numchanged++;
        Jobc.add_done(pid, -1);
        if (Sp.GetFlag(FT_ASYNCDB)) {
            GRpkgIf()->ErrPrintf(ET_MSG,
                "process %d terminated by signal %d.\n",
                pid, WIFSIGNALED(status));
        }
        if (CP.GetFlag(CP_CWAIT))
            Sp.CheckAsyncJobs();
    }
}

#endif


#else // HAVE_SOCKET

#define SUBMIT "/bin/csh /usr/bin/rspice"

namespace {
    const char *nyet = "Asynchronous spice %s are not available.\n";
}


void
CommandTab::com_aspice(wordlist*)
{
    GRpkgIf()->ErrPrintf(ET_MSG, nyet, "runs");
}


void
CommandTab::com_rspice(wordlist *wl)
{
    char *input;
    if (wl && !wl->wl_next)
        input = wl->wl_word;
    else {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "you must supply the input deck name.\n");
        return;
    }
    char *output = filestat::make_temp("out");
    char *raw = filestat::make_temp("raw");
    GCarray<char*> gc_output(output);
    GCarray<char*> gc_raw(raw);

    TTY.printf("Running job, please wait. ");
    TTY.flush();
    char buf[BSIZE_SP];
    sprintf(buf, "%s %s %s %s", SUBMIT, input, output, raw);
    if (CP.System(buf) != 0)
        return;

    TTY.printf("done.\n\n");

    FILE *fp;
    if (!(fp = fopen(output, "r"))) {
        GRpkgIf()->Perror(output);
        return;
    }
    while (fgets(buf, BSIZE_SP, fp))
        TTY.send(buf);
    fclose(fp);
    unlink(output);

    const char *stmp = raw;
    Sp.LoadFile(&stmp, false);
    unlink(raw);
}


void
CommandTab::com_jobs(wordlist*)
{
    GRpkgIf()->ErrPrintf(ET_MSG, nyet, "jobs");
}


void IFsimulator::CheckAsyncJobs()
{
}


void
sCHECKprms::registerJob()
{
    GRpkgIf()->ErrPrintf(ET_MSG, nyet, "jobs");
}

#endif // HAVE_SOCKET

