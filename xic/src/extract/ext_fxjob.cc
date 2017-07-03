
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2014 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: ext_fxjob.cc,v 5.16 2017/03/22 23:55:34 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext_fxjob.h"
#include "dsp.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "errorlog.h"
#include "pathlist.h"
#include "filestat.h"
#include "childproc.h"

#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef WIN32
#include "msw.h"
#include <windows.h>
#else
#include <sys/wait.h>
#endif


//------------------------------------------------------------------------
// Job Control and Post-Processing for Fast[er]Cap/FastHenry interface
//------------------------------------------------------------------------

fxJob *fxJob::j_jobs = 0;


fxJob::fxJob(const char *ds, fxJobMode m, fxGif *gif, fxJob *n)
{
    static int job_count = 1;
    j_mode = m;
    j_infiles = 0;
    j_outfile = 0;
    j_resfile = 0;
    j_command = 0;
    if (ds && *ds)
        j_dataset = lstring::copy(ds);
    else
        j_dataset = lstring::copy(DEF_DATASET);
    j_gif = gif;
    j_pid = 0;
    j_num = job_count++;
    j_flags = 0;
    j_iftype = fxJobWR;
    j_start_time = 0;
    next = n;
}


fxJob::~fxJob()
{
    fxJob *jp = 0;
    for (fxJob *j = jobs(); j; j = j->next) {
        if (j == this) {
            if (!jp)
                set_jobs(j->next);
            else
                jp->next = j->next;
            break;
        }
        jp = j;
    }
    if (j_flags & FX_UNLINK_IN) {
        for (stringlist *sl = j_infiles; sl; sl = sl->next)
            unlink(sl->string);
    }
    stringlist::destroy(j_infiles);
    if (j_flags & FX_UNLINK_OUT)
        unlink(j_outfile);
    delete [] j_outfile;
    if (j_flags & FX_UNLINK_RES)
        unlink(j_resfile);
    delete [] j_resfile;
    delete [] j_command;
    delete [] j_dataset;
    if (j_gif) {
        j_gif->updateString();
        j_gif->PopUpExtIf(0, MODE_UPD);
    }
}


namespace{
    // Return true if the program in path is a Whiteley Research
    // version.
    //
    bool is_wr(const char *path)
    {
        sLstr lstr;
        lstr.add(path);
#ifdef WIN32
        lstr.add(" < NUL 2> NUL");
#else
        lstr.add(" < /dev/null 2> /dev/null");
#endif

        FILE *fp = popen(lstr.string(), "r");
        if (!fp)
            return (false);
        char buf[256];
        fgets(buf, 256, fp);
        pclose(fp);
        const char *s = buf;
        lstring::advtok(&s);   // "Running"
        lstring::advtok(&s);   // "Fast..."
        while (isspace(*s) || isdigit(*s) || *s == '.')
            s++;
        return (s[0] == 'w' && s[1] == 'r');
    }
}


// Set up the fastcap command line.
//
bool
fxJob::setup_fc_run(bool run_foreg, bool monitor)
{
    if (j_mode != fxCapMode)
        return (false);

    const char *path = CDvdb()->getVariable(VA_FcPath);
    if (!path || !*path)
        path = FC_DEFAULT_PATH;
#ifndef WIN32
    // Access is too restrictive for Windows (e.g., .exe missing
    // should not be an error), let CreateProcess handle reporting
    // of a bad path.
    if (access(path, X_OK)) {
        Log()->ErrorLogV(mh::Initialization, "Cannot access: %s.\n", path);
        return (false);
    }
#endif
    const char *prog = lstring::strrdirsep(path);
    if (prog)
        prog++;
    else
        prog = path;
    if (lstring::ciprefix("fastercap", prog))
        j_iftype = fxJobNEW;
    else if (!is_wr(path))
        j_iftype = fxJobMIT;
    sLstr lstr;
    lstr.add(path);

    const char *args = CDvdb()->getVariable(VA_FcArgs);
    if (args && *args) {
        if (j_iftype == fxJobNEW) {
            // Make sure that "-b" is present.  This puts
            // FasterCap into non-gui mode.  If not found, add it.

            const char *t = args;
            bool has_a = false;
            bool has_b = false;
            char *tok;
            double d;
            while ((tok = lstring::gettok(&t)) != 0) {
                if (!strcmp(tok, "-b"))
                    has_b = true;
                else if (tok[0] == '-' && tok[1] == 'a' &&
                        sscanf(tok+2, "%lf", &d) == 1)
                    has_a = true;
                delete [] tok;
                if (has_b && has_a)
                    break;
            }

            // -b (non-gui "batch" mode) is required.
            if (!has_b) {
                sLstr tlstr;
                tlstr.add("-b ");
                tlstr.add(args);
                CDvdb()->setVariable(VA_FcArgs, tlstr.string());
                args = CDvdb()->getVariable(VA_FcArgs);
            }

            // -ax.x sets the refinement tolerance.  If not given,
            // there will be no auto-refinement and the results are
            // likely horribly wrong.  Warn the user but run it
            // anyway, perhaps the input is pre-refined.
            if (!has_a) {
                Log()->WarningLog(mh::Initialization, 
            "The argument list does not provide the -a refinement option.\n"
            "Unless the data set is pre-refined, results will be horribly\n"
            "wrong.  Running anyway.");
            }
        }
        lstr.add_c(' ');
        lstr.add(args);
    }
    else if (j_iftype == fxJobNEW) {
        // Provide arguments for FasterCap.  The "-b" is
        // mandatory, -a is almost always used but is not
        // required.  If the user provides an argument list we
        // assume that they will add -a if needed, otherwise we
        // add it here.

        CDvdb()->setVariable(VA_FcArgs, "-b -a0.01");
        args = CDvdb()->getVariable(VA_FcArgs);
        lstr.add_c(' ');
        lstr.add(args);
    }

    lstr.add_c(' ');
    if (j_iftype != fxJobNEW) {
        // FasterCap does not use -l, and in fact -l causes an
        // error.

        lstr.add("-l");
    }
    lstr.add(j_infiles->string);
    if (run_foreg) {
        if (monitor) {
            // This assumes a tee function is available, not true
            // in Windows without Cygwin.

            lstr.add(" | tee ");
            lstr.add(j_outfile);
        }
        else {
            lstr.add(" > ");
            lstr.add(j_outfile);
        }
    }
    j_command = lstr.string_trim();
    return (true);
}


// Set up the fasthenry command line.
//
bool
fxJob::setup_fh_run(bool run_foreg, bool monitor)
{
    if (j_mode != fxIndMode)
        return (false);

    const char *path = CDvdb()->getVariable(VA_FhPath);
    if (!path || !*path)
        path = FH_DEFAULT_PATH;
    if (access(path, X_OK)) {
        Log()->ErrorLogV(mh::Initialization, "Cannot access: %s.\n", path);
        return (false);
    }
    if (!is_wr(path))
        j_iftype = fxJobMIT;
    sLstr lstr;
    lstr.add(path);
    lstr.add(" -S .");
    lstr.add(j_dataset);
    lstr.add_c('-');
    lstr.add_i(j_num);

    const char *args = CDvdb()->getVariable(VA_FhArgs);
    if (args && *args) {
        lstr.add_c(' ');
        lstr.add(args);
    }

    lstr.add_c(' ');
    lstr.add(j_infiles->string);
    if (run_foreg) {
        if (monitor) {
            // This assumes a tee function is available, not true
            // in Windows without Cygwin.

            lstr.add(" | tee ");
            lstr.add(j_outfile);
        }
        else {
            lstr.add(" > ");
            lstr.add(j_outfile);
        }
    }
    j_command = lstr.string_trim();
    return (true);
}


namespace {
    // Error returns from FasterCap 5.0.2.
    //
    const char *faster_cap_error(int err)
    {
        switch (err) {
        case 0:  return ("normal exit");
        case 64: return ("argument syntax error");
        case 66: return ("can't open input file");
        case 71: return ("out of memory");
        case 74: return ("file error");
        case 77: return ("license error");
        case 97: return ("cannot go out of core");
        case 98: return ("unknown exception");
        case 125: return ("user break");
        default:
        case 1:
            break;
        }
        return ("unknown error");
    }
}


// Signal/thread handler - wait for child process to finish
//

namespace {
    // Idle proc to process the output from an asynchronous fastcap
    // or fasthenry run.
    //
    int
    exec_idle(void *arg)
    {
        int pid = (long)arg;
        fxJob *j = fxJob::find(pid);
        if (j) {
            j->post_proc();
            delete j;
        }
        return (false);
    }


    // Pop up a failure message, don't want to do this from the handler.
    //
    int
    badexit_idle(void *arg)
    {
        char *s = (char*)arg;
        DSPmainWbag(PopUpMessage(s, false))
#ifndef WIN32
        delete [] s;
#endif
        return (0);
    }


#ifdef WIN32
    struct pipe_desc
    {
        pipe_desc(HANDLE hr, HANDLE hf, PROCESS_INFORMATION *pi)
            {
                read_desc = hr;
                file_desc = hf;
                info = pi;
            }

        HANDLE read_desc;
        HANDLE file_desc;
        PROCESS_INFORMATION *info;
    };


    void thread_cb(void *arg)
    {
        pipe_desc *pd = (pipe_desc*)arg;
        if (pd->read_desc) {
            HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
            char bf[4];
            DWORD nr;
            for (;;) {
                if (!ReadFile(pd->read_desc, bf, 1, &nr, 0))
                    break;
                if (pd->file_desc) {
                    if (!WriteFile(pd->file_desc, bf, 1, &nr, 0))
                        break;
                }
                if (!WriteFile(hout, bf, 1, &nr, 0))
                    break;
            }
            CloseHandle(pd->read_desc);
            CloseHandle(pd->file_desc);
        }
        PROCESS_INFORMATION *info = pd->info;
        delete pd;

        HANDLE h = info->hProcess;
        UINT pid = info->dwProcessId;
        delete info;
        if (WaitForSingleObject(h, INFINITE) == WAIT_OBJECT_0) {
            DWORD status;
            GetExitCodeProcess(h, &status);
            if (!status) {
                dspPkgIf()->RegisterIdleProc(exec_idle, (void*)pid);
                CloseHandle(h);
                return;
            }
            fxJob *j = fxJob::find(pid);
            if (!j) {
                CloseHandle(h);
                return;
            }
            if (j->if_type() == fxJobMIT) {
                // MIT FastCap (at least) does not return anything
                // useful.
                dspPkgIf()->RegisterIdleProc(exec_idle, (void*)(long)pid);
                CloseHandle(h);
                return;
            }

            // For whatever reason, an idle proc doesn't work here,
            // when user aborts the GTK faults and the program exits. 
            // We also can't malloc the string, causes a fault when
            // freed.  Use a timeout instead, which seems to be ok.

            static char buf[128];
            if (j->if_type() == fxJobWR) {
                snprintf(buf, 128,
                    "Child process %d exited with error status %ld.",
                    pid, status);
                dspPkgIf()->RegisterTimeoutProc(100, badexit_idle, buf);
            }
            else if (j->if_type() == fxJobNEW) {
                // FasterCap 5.0.2 and later only.
                snprintf(buf, 128,
                "FasterCap process %d exited with error status %ld\n(%s).",
                    pid, status, faster_cap_error(status));
                dspPkgIf()->RegisterTimeoutProc(100, badexit_idle, buf);
            }
            delete j;
        }
        CloseHandle(h);
    }

#else

    void child_hdlr(int pid, int status, void*)
    {
        char buf[128];
        if (WIFEXITED(status)) {
            if (!WEXITSTATUS(status)) {
                // Successful return.
                dspPkgIf()->RegisterIdleProc(exec_idle, (void*)(long)pid);
                return;
            }
            fxJob *j = fxJob::find(pid);
            if (!j)
                return;
            if (j->if_type() == fxJobMIT) {
                // MIT FastCap (at least) does not return anything
                // useful.
                dspPkgIf()->RegisterIdleProc(exec_idle, (void*)(long)pid);
                return;
            }
            if (j->if_type() == fxJobWR) {
                sprintf(buf,
                    "Child process %d exited with error status %d.",
                    pid, WEXITSTATUS(status));
                dspPkgIf()->RegisterIdleProc(badexit_idle, lstring::copy(buf));
            }
            else if (j->if_type() == fxJobNEW) {
                // FasterCap 5.0.2 and later only.
                sprintf(buf,
                "FasterCap process %d exited with error status %d\n(%s).",
                    pid, WEXITSTATUS(status),
                    faster_cap_error(WEXITSTATUS(status)));
                dspPkgIf()->RegisterIdleProc(badexit_idle, lstring::copy(buf));
            }
            delete j;
            return;
        }
        else if (WIFSIGNALED(status)) {
            sprintf(buf, "Child process %d exited on signal %d.", pid,
                WIFSIGNALED(status));
            dspPkgIf()->RegisterIdleProc(badexit_idle, lstring::copy(buf));
        }
        fxJob *j = fxJob::find(pid);
        delete j;
    }


    // This will pipe stdout to a read loop, which is forked.  The
    // read loop will dispatch unbuffered output to the file, and to
    // the original stdout.
    //
    // Unfortunately, the general concept doesn't work.  The stdio
    // from the forked program is set to block buffering, probably
    // during the execve, so that the read loop will read the block
    // buffered output.  There seems to be no general way around this.
    //
    // Here, we use pseudo-terminals, so that the output is line
    // buffered rather than block buffered.  This is an improvement,
    // but unbuffered would be better, so the FasterCap iteration
    // counts would resolve individually.
    //
    bool tee(const char *fname)
    {
#ifdef HAVE_POSIX_OPENPT
        int ptm = posix_openpt(O_RDWR | O_NOCTTY);
        if (ptm < 0)
            return (false);
        if (grantpt(ptm) < 0)
            return (false);
        if (unlockpt(ptm) < 0)
            return (false);
        char *ptsn = ptsname(ptm);
        int pts = open(ptsn, O_CREAT | O_WRONLY | O_TRUNC);
        if (pts < 0) {
            close(ptm);
            return (false);
        }

        int fd0 = dup(1);
        dup2(pts, 1);

        int pid = fork();
        if (pid == 0) {
            int fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC);
            for (;;) {
                char bf[4];
                int rv = read(ptm, bf, 1);
                if (rv == 0)
                    continue;
                if (rv < 0)
                    break;
                int wv = write(fd, bf, 1);
                if (wv < 0)
                    break;
                wv = write(fd0, bf, 1);
            }
            exit(0);
        }
        close(fd0);
        close(pts);
        close(ptm);

#else
        int pfds[2];
        if (pipe(pfds) < 0)
            return (false);
        int fd0 = dup(1);
        dup2(pfds[1], 1);

        int pid = fork();
        if (pid == 0) {
            int fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC);
            for (;;) {
                char bf[4];
                int rv = read(pfds[0], bf, 1);
                if (rv == 0)
                    continue;
                if (rv < 0)
                    break;
                int wv = write(fd, bf, 1);
                if (wv < 0)
                    break;
                wv = write(fd0, bf, 1);
            }
            exit(0);
        }
        close(pfds[0]);
        close(pfds[1]);
        close(fd0);
#endif
        return (pid != -1);
    }
#endif
}


// Run job.
//
bool
fxJob::run(bool run_foreg, bool monitor)
{
    time(&j_start_time);
    if (run_foreg) {
        int ret = cMain::System(j_command);
        if (ret != 0) {
            if (j_iftype == fxJobWR) {
                const char *s = j_command;
                char *tok = lstring::gettok(&s);
                Log()->ErrorLogV(mh::JobControl,
                    "The %s child process returned with error %d.\n",
                    tok ? lstring::strip_path(tok) : "", ret);
                delete [] tok;
                return (false);
            }
            if (j_iftype == fxJobNEW) {
                // FasterCap 5.0.2 and later only.
                Log()->ErrorLogV(mh::JobControl,
                    "FasterCap exited with error status %d\n(%s).",
                    ret, faster_cap_error(ret));
                return (false);
            }
        }
        return (true);
    }

#ifdef WIN32
    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = true;
    HANDLE ofile = CreateFile(j_outfile, GENERIC_WRITE, 0, &sa,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (!ofile || ofile == INVALID_HANDLE_VALUE) {
        Log()->ErrorLogV(mh::Initialization,
            "Error: file creation failed.  File:\n%s\n", j_outfile);
        return (false);
    }
    PROCESS_INFORMATION *info = 0;
    if (monitor) {
        HANDLE pr, pw;
        if (!CreatePipe(&pr, &pw, &sa, 0)) {
            Log()->ErrorLog(mh::Initialization, "Pipe creation failed.\n");
            CloseHandle(ofile);
            return (false);
        }
        if (!SetHandleInformation(pr, HANDLE_FLAG_INHERIT, 0)) {
            Log()->ErrorLog(mh::Internal,
                "Pipe inheritance setting failed.\n");
            CloseHandle(ofile);
            return (false);
        }
        info = msw::NewProcess(j_command, 0, true, 0, pw);
        CloseHandle(pw);
        if (!info) {
            CloseHandle(pr);
            CloseHandle(ofile);
            Log()->ErrorLogV(mh::JobControl,
                "New process creation failed.  Command:\n%s\n", j_command);
            return (false);
        }
        j_pid = info->dwProcessId;
        _beginthread(thread_cb, 0, new pipe_desc(pr, ofile, info));

    }
    else {
        info = msw::NewProcess(j_command, 0, true, 0, ofile);
        CloseHandle(ofile);
        if (!info) {
            Log()->ErrorLogV(mh::JobControl,
                "New process creation failed.  Command:\n%s\n", j_command);
            return (false);
        }
        j_pid = info->dwProcessId;
        _beginthread(thread_cb, 0, new pipe_desc(0, 0, info));
    }
#else

    int pid = fork();
    if (pid == -1) {
        Log()->ErrorLog(mh::Initialization, "Can't fork, system call failed.");
        return (false);
    }
    if (!pid) {
        dspPkgIf()->CloseGraphicsConnection();
        if (!monitor || !tee(j_outfile)) {
            int fd = open(j_outfile, O_CREAT | O_WRONLY | O_TRUNC);
            dup2(fd, 1);
        }

        const char *a = j_command;
        int ac = 0;
        while (lstring::advqtok(&a))
            ac++;
        ac++;
        char **av = new char*[ac];
        int cnt = 0;
        a = j_command;
        while ((av[cnt] = lstring::getqtok(&a)) != 0)
            cnt++;

        char *path = av[0];
        av[0] = lstring::strip_path(path);
        execv(path, av);
        _exit(1);
    }
    j_pid = pid;
    Proc()->RegisterChildHandler(pid, child_hdlr, 0);
#endif
    if (j_gif)
        j_gif->PopUpExtIf(0, MODE_UPD);
    return (true);
}


namespace {
    void copyin(FILE *fp, char *fname)
    {
        FILE *ip = fopen(fname, "r");
        if (!ip) {
            fprintf(fp, "** File not found.\n");
            return;
        }
        int c;
        while ((c = getc(ip)) != EOF)
            putc(c, fp);
        fclose(ip);
    }


    // Return the self-capacitance of t in mat.  This is the diagonal
    // term from the fastcap matrix, minus the mutual capacitance to
    // each other element.
    //
    double selfcap(int t, float **mat, int size)
    {
        // The matrix should be symmetric, however, for
        // FasterCap-5.0.6 this is often not true.  We'll deal with
        // this by averaging.

        double cap = 0.0;
        for (int i = 0; i < size; i++)
            cap += 0.5*(mat[t][i] + mat[i][t]);
        return (cap);
    }


    // Return the mutual capacatance for Cpq (p != q) from mat.  This
    // is simply the negative of the matrix element.
    //
    double mutcap(int p, int q, float **mat)
    {
        // The matrix should be symmetric, however, for
        // FasterCap-5.0.6 this is often not true.  We'll deal with
        // this by averaging.

        return (-0.5*(mat[p][q] + mat[q][p]));
    }


    // Return the appropriate scale factor prefix given the scale factor.
    //
    const char *scale_str(double sc)
    {
        if (sc == 1e-18)
            return ("atto");
        if (sc == 1e-15)
            return ("femto");
        if (sc == 1e-12)
            return ("pico");
        if (sc == 1e-9)
            return ("nano");
        if (sc == 1e-6)
            return ("micro");
        if (sc == 1e-3)
            return ("milli");
        if (sc == 1e+3)
            return ("kilo");
        if (sc == 1e+6)
            return ("mega");
        if (sc == 1e+9)
            return ("giga");
        if (sc == 1e+12)
            return ("tera");
        if (sc == 1e+15)
            return ("peta");
        if (sc == 1e+18)
            return ("exa");
        return ("");
    }
}


// Fastcap post-processor, find the capacitance matrix and pop up the
// result.
//
void
fxJob::fc_post_process()
{
    char buf[256];
    float **mat;
    int size;
    double units;
    char **names;
    char *err = fc_get_matrix(j_outfile, &size, &mat, &units, &names);
    if (err) {
        Log()->ErrorLog(mh::Processing, err);
        delete [] err;
        return;
    }
    if (!j_resfile) {
        sprintf(buf, "%s-%d.fc_log", j_dataset, j_pid);
        j_resfile = lstring::copy(buf);
    }
    if (!filestat::create_bak(j_resfile)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        return;
    }
    FILE *fp = filestat::open_file(j_resfile, "w");
    if (!fp) {
        Log()->ErrorLog(mh::Initialization, filestat::error_msg());
        return;
    }
    fprintf(fp, "** %s: Output from FastCap interface\n", j_resfile);
    fprintf(fp, "** Generated by %s\n", XM()->IdString());
    fprintf(fp, "DataSet: %s\n", j_dataset);
    fprintf(fp, "Input file: %s\n", j_infiles->string);
    fprintf(fp, "Output file: %s\n", j_outfile);
    fprintf(fp, "Command: %s\n", j_command);

    fprintf(fp, "\nSelf Capacitance (%sfarads):\n", scale_str(units));
    for (int i = 0; i < size; i++) {
        sprintf(buf, "C.%s", names[i]);
        fprintf(fp, " %-12s%.3g\n", buf, selfcap(i, mat, size));
    }
    if (size > 1) {
        fprintf(fp,
            "\nMutual Capacitance (%sfarads):\n", scale_str(units));
        for (int i = 0; i < size; i++) {
            for (int j = i+1; j < size; j++) {
                sprintf(buf, "C.%s.%s", names[i], names[j]);
                fprintf(fp, " %-12s%.3g\n", buf, mutcap(i, j, mat));
            }
        }
    }

    fprintf(fp,
        "\n-------------------------------------------------------\n");
    fprintf(fp, "** Input file listing: %s\n\n", j_infiles->string);
    copyin(fp, j_infiles->string);
    fprintf(fp,
        "\n-------------------------------------------------------\n");
    fprintf(fp, "** Output file listing: %s\n\n", j_outfile);
    copyin(fp, j_outfile);
    fclose(fp);
    DSPmainWbag(PopUpFileBrowser(j_resfile))
    for (int i = 0; i < size; i++) {
        delete [] mat[i];
        delete [] names[i];
    }
    delete [] mat;
    delete [] names;
}


inline char *rowtok(char *rstr)
{
    lstring::advtok(&rstr);
    return (rstr);
}


namespace {
    struct cplx_t
    {
        cplx_t() { re = 0.0; im = 0.0; }

        double re;
        double im;
    };

    // Impedance matrix, FastHenry return.
    //
    struct zmat_t
    {
        zmat_t(int, double);
        ~zmat_t();

        double freq;
        int size;
        cplx_t **matrix;
        char **row_strings;
    };


    zmat_t::zmat_t(int sz, double f)
    {
        size = sz;
        freq = f;
        matrix = new cplx_t*[size];
        for (int i = 0; i < size; i++)
            matrix[i] = new cplx_t[size];
        row_strings = new char*[size];
        for (int i = 0; i < size; i++)
            row_strings[i] = 0;
    }


    zmat_t::~zmat_t()
    {
        for (int i = 0; i < size; i++)
            delete [] matrix[i];
        delete [] matrix;
        for (int i = 0; i < size; i++)
            delete [] row_strings[i];
        delete [] row_strings;
    }
}


// Fasthenry post-processor, compute the inductance and resistance from
// the impedance matrix and pop up the results.
//
void
fxJob::fh_post_process()
{
    char buf[256];
    if (!j_resfile) {
        sprintf(buf, "%s-%d.fh_log", j_dataset, j_pid);
        j_resfile = lstring::copy(buf);
    }
    if (!filestat::create_bak(j_resfile)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        return;
    }
    FILE *fp = filestat::open_file(j_resfile, "w");
    if (!fp) {
        Log()->ErrorLog(mh::Initialization, filestat::error_msg());
        return;
    }
    fprintf(fp, "** %s: Output from FastHenry interface\n", j_resfile);
    fprintf(fp, "** Generated by %s\n", XM()->IdString());
    fprintf(fp, "DataSet: %s\n", j_dataset);
    fprintf(fp, "Input file: %s\n", j_infiles->string);
    fprintf(fp, "Output file: %s\n", j_outfile);
    fprintf(fp, "Command: %s\n\n", j_command);

    char zcmat[64];
    sprintf(zcmat, "Zc.%s-%d.mat", j_dataset, j_num);
    FILE *zp = fopen(zcmat, "r");
    if (!zp) {
        // FastHenry lower-cases the suffix!?
        char *old = lstring::copy(zcmat);
        for (char *t = zcmat + 3; *t; t++) {
            if (isupper(*t))
                *t = tolower(*t);
        }
        if (strcmp(old, zcmat)) {
            zp = fopen(zcmat, "r");
            if (!zp)
                fprintf(fp, "Error: can't open file %s or %s.", old, zcmat);
        }
        else {
            if (!zp)
                fprintf(fp, "Error: can't open file %s.", zcmat);
        }
        delete [] old;
    }
    if (zp) {
        zmat_t *zm = 0;
        for (int fcnt = 0; ; fcnt++) {
            const char *err = fh_get_matrix(zp, &zm);
            if (err) {
                if (lstring::prefix("no imped", err) && fcnt)
                    break;
                fprintf(fp, "Error reading %s: %s\n", zcmat, err);
                break;
            }
            if (fcnt == 0) {
                // Print the row mapping, and zero out the part of the
                // string following the row name token.
                for (int i = 0; i < zm->size; i++) {
                    fprintf(fp, "%s\n", zm->row_strings[i]);
                    char *t = strchr(zm->row_strings[i], ':');
                    if (t)
                        *t = 0;
                }
                fprintf(fp, "\n");
            }

            fprintf(fp, "Frequency: %.3e\n", zm->freq);
            double freq = 2*M_PI*zm->freq;
            if (freq == 0) {
                fprintf(fp, " Resistance (ohms):\n");
                for (int i = 0; i < zm->size; i++) {
                    sprintf(buf, "R.%s", rowtok(zm->row_strings[i]));
                    fprintf(fp,
                        " %-12s%12.3f\n", buf, zm->matrix[i][i].re);
                }
            }
            else {
                fprintf(fp,
                    " Self Resistance/Inductance (ohms/henries):\n");
                for (int i = 0; i < zm->size; i++) {
                    sprintf(buf, "RL.%s", rowtok(zm->row_strings[i]));
                    fprintf(fp, " %-12s%12.3e %12.3e\n", buf,
                        zm->matrix[i][i].re, zm->matrix[i][i].im/freq);
                }
                if (zm->size > 1) {
                    fprintf(fp, " Mutual Inductance (henries):\n");
                    for (int i = 0; i < zm->size; i++) {
                        for (int j = i+1; j < zm->size; j++) {
                            sprintf(buf, "L.%s.%s",
                                rowtok(zm->row_strings[i]),
                                rowtok(zm->row_strings[j]));
                            fprintf(fp, " %-12s%12.3e\n", buf,
                                zm->matrix[i][j].im/freq);
                        }
                    }
                }
                if (zm->size == 2) {
                    fprintf(fp,
                        " Transmission Impedance (ohms/henries):\n");
                    fprintf(fp, " %-12s%12.3e %12.3e\n", "Z",
                        zm->matrix[0][0].re + zm->matrix[1][1].re,
                        (zm->matrix[0][0].im + zm->matrix[1][1].im -
                            2.0*fabs(zm->matrix[0][1].im))/freq);
                }
            }
            fprintf(fp, "\n");
        }
        delete zm;
        fclose(zp);
    }

    fprintf(fp,
        "\n-------------------------------------------------------\n");
    fprintf(fp, "** %s file listing\n\n", zcmat);
    copyin(fp, zcmat);
    fprintf(fp,
        "\n-------------------------------------------------------\n");
    fprintf(fp, "** Input file listing: %s\n\n", j_infiles->string);
    copyin(fp, j_infiles->string);
    fprintf(fp,
        "\n-------------------------------------------------------\n");
    fprintf(fp, "** Output file listing: %s\n\n", j_outfile);
    copyin(fp, j_outfile);
    fclose(fp);
    DSPmainWbag(PopUpFileBrowser(j_resfile))
}


void
fxJob::pid_string(sLstr &lstr)
{
    char buf[256];
    const struct tm *t = localtime(&j_start_time);
    const char *prg = "unknown";
    switch (j_iftype) {
    case fxJobWR:
        prg = j_mode == fxIndMode ? "FastHenryWR" : "FastCapWR";
        break;
    case fxJobMIT:
        prg = j_mode == fxIndMode ? "FastHenry" : "FastCap";
        break;
    case fxJobNEW:
        prg = j_mode == fxIndMode ? "FastHenry" : "FasterCap";
        break;
    }
    sprintf(buf, "%-6d %-12s %02d/%02d/%4d %02d:%02d:%02d\n", j_pid,
        prg, t->tm_mon+1, t->tm_mday, t->tm_year + 1900, t->tm_hour,
        t->tm_min, t->tm_sec);
    lstr.add(buf);
}


void
fxJob::kill_process()
{
#ifdef WIN32
    if (j_pid > 0) {
        HANDLE h = OpenProcess(PROCESS_TERMINATE, 0, j_pid);
        // The 125 is FasterCap's "user break".
        if (h)
            TerminateProcess(h, 125);
    }
#else
    if (j_pid > 0)
        kill(j_pid, SIGTERM);
#endif
}


namespace {
    // The matrix lines in the fastcap output (at least) can be
    // arbitrarily long.  This function reads and returns an
    // arbitrarily long line.
    //
    char *
    getline(FILE *fp)
    {
        sLstr lstr;
        int c;
        while ((c = getc(fp)) != EOF) {
            lstr.add_c(c);
            if (c == '\n')
                break;
        }
        return (lstr.string_trim());
    }


    // Return the scale factor given a word with a standard prefix.
    //
    double
    get_scale(char *str, double *aux_scale)
    {
        *aux_scale = 1.0;
        if (lstring::ciprefix("atto", str))
            return (1e-18);
        if (lstring::ciprefix("femto", str))
            return (1e-15);
        if (lstring::ciprefix("pico", str))
            return (1e-12);
        if (lstring::ciprefix("nano", str))
            return (1e-9);
        if (lstring::ciprefix("micro", str))
            return (1e-6);
        if (lstring::ciprefix("milli", str))
            return (1e-3);
        if (lstring::ciprefix("kilo", str))
            return (1e+3);
        if (lstring::ciprefix("mega", str))
            return (1e+6);
        if (lstring::ciprefix("giga", str))
            return (1e+9);
        if (lstring::ciprefix("tera", str))
            return (1e+12);
        if (lstring::ciprefix("peta", str))
            return (1e+15);
        if (lstring::ciprefix("exa", str))
            return (1e+18);
        if (lstring::ciprefix("every", str)) {
            // FastCap can produce strange "out of range" units.
            lstring::advtok(&str);  // "every"
            lstring::advtok(&str);  // "unit"
            lstring::advtok(&str);  // "is"
            double d = atof(str);
            *aux_scale = d/1e-18;
            return (1e-18);
        }
        return (1.0);
    }
}


// Static function.
// Parse the fastcap output file (fname) and return the
// capacitance matrix and size.  On error, an error string
// (malloc'ed) is returned instead.
//
char *
fxJob::fc_get_matrix(const char *fname, int *size, float ***mret, double *units,
    char ***nameret)
{
    *size = 0;
    *mret = 0;
    *units = 0.0;
    *nameret = 0;
    char buf[256];
    FILE *fp = fopen(fname, "r");
    if (!fp) {
        sprintf(buf, "can't open file %s.", fname);
        return (lstring::copy(buf));
    }

    // We need to parse both forms.
    //
    // FastCap format:
    // CAPACITANCE MATRIX, femtofarads
    //                           1          2
    // 0%default_group 1      2.553     -2.393
    // 1%default_group 2     -2.393      2.551

    // FasterCap format:
    // Capacitance matrix is:
    // Demo evaluation version
    // Dimension 2 x 2
    // g1_0  3.76182e-15 -3.66421e-15
    // g2_1  -3.58443e-15 3.77836e-15
    //
    // This is repeated for each iteration, we need to grab the
    // last instance in the file.

    off_t posn = 0;
    char *str = 0;
    for (;;) {
        char *tstr;
        while ((tstr = getline(fp)) != 0) {
            if (lstring::ciprefix("CAPACITANCE MATRIX", tstr))
                break;
            delete [] tstr;
        }
        if (tstr) {
            delete [] str;
            str = tstr;
            posn = ftell(fp);
            continue;
        }
        break;
    }
    if (!str) {
        fclose(fp);
        sprintf(buf, "can't find matrix data in file %s.", fname);
        return (lstring::copy(buf));
    }
    fseek(fp, posn, SEEK_SET);

    char *s = str;
    lstring::advtok(&s);
    lstring::advtok(&s);
    double aux_scale;
    *units = get_scale(s, &aux_scale);
    delete [] str;

    int matsz = 0;
    for (;;) {
        if ((str = getline(fp)) == 0) {
            fclose(fp);
            sprintf(buf, "read error or premature EOF, file %s.", fname);
            return (lstring::copy(buf));
        }
        if (lstring::ciprefix("demo", str)) {
            // "Demo evaluation version"
            delete [] str;
            continue;
        }
        if (lstring::ciprefix("dimen", str)) {
            // "Dimension N x N"
            s = str;
            lstring::advtok(&s);
            matsz = atoi(s);
            delete [] str;
            break;
        }
        int cnt = 0;
        s = str;
        for (;;) {
            while (isspace(*s))
                s++;
            if (isdigit(*s)) {
                cnt++;
                while (isdigit(*s))
                    s++;
                continue;
            }
            break;
        }
        matsz = cnt;
        delete [] str;
        break;
    }

    if (!matsz) {
        fclose(fp);
        sprintf(buf, "matrix data has no size, file %s.", fname);
        return (lstring::copy(buf));
    }
    float **matrix = new float*[matsz];
    char **names = new char*[matsz];

    for (int i = 0; i < matsz; i++) {
        if ((str = getline(fp)) == 0) {
            fclose(fp);
            for (int k = 0; k < i; i++)
                delete [] matrix[k];
            delete [] matrix;
            sprintf(buf, "read error or premature EOF, file %s.", fname);
            return (lstring::copy(buf));
        }
        matrix[i] = new float[matsz];

        s = str;
        char *tok = lstring::gettok(&s);
        names[i] = tok;
        char *t = strchr(tok, '%');
        if (t) {
            // FastCap
            *t = 0;
            lstring::advtok(&s);
        }
        else {
            t = strchr(tok, '_');
            if (t) {
                // FasterCap
                names[i] = lstring::copy(t+1);
                delete [] tok;
            }
        }
        for (int j = 0; j < matsz; j++) {
            tok = lstring::gettok(&s);
            if (!tok) {
                fclose(fp);
                for (int k = 0; k <= i; i++)
                    delete [] matrix[k];
                delete [] matrix;
                sprintf(buf,
                    "read error or premature EOF, file %s.", fname);
                return (lstring::copy(buf));
            }
            matrix[i][j] = aux_scale*atof(tok);
            delete [] tok;
        }
        delete [] str;
    }
    fclose(fp);
    *size = matsz;
    *mret = matrix;
    *nameret = names;
    return (0);
}


// Static function.
// Parse the fasthenry Zc.mat file (fp) and return the impedance
// matrix and size.  This can be called repeatedly if there are
// multiple frequencies.  A message is returned on error.  If mret
// points to null, a new matrix will be created, otherwise the
// existing matrix will be reused if it is the same size.
//
const char *
fxJob::fh_get_matrix(FILE *fp, zmat_t **mret)
{
    stringlist *rstrs = 0;
    int size = 0;
    double freq = 0.0;
    {
        char *str;
        while ((str = getline(fp)) != 0) {
            if (lstring::prefix("Row ", str)) {
                char *t = str + strlen(str) - 1;
                while (isspace(*t) && t >= str)
                    *t-- = 0;
                rstrs = new stringlist(str, rstrs);
                continue;
            }
            if (lstring::prefix("Impedance matrix", str))
                break;
            delete [] str;
        }
        if (!str || !lstring::prefix("Impedance matrix", str)) {
            delete [] str;
            return ("no impedance matrix found.");
        }
        const char *s = str;
        lstring::advtok(&s); // Impedance
        lstring::advtok(&s); // matrix
        lstring::advtok(&s); // for
        lstring::advtok(&s); // frequency
        lstring::advtok(&s); // =
        freq = atof(s);
        lstring::advtok(&s); // <freq>
        size = atoi(s);
        delete [] str;
    }
    if (size <= 0)
        return ("matrix data has incorrect size.");

    zmat_t *m;
    if (*mret && (*mret)->size == size) {
        m = *mret;
        stringlist::destroy(rstrs);
        rstrs = 0;
        m->freq = freq;
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                m->matrix[i][j].re = 0.0;
                m->matrix[i][j].im = 0.0;
            }
        }
    }
    else
        m = new zmat_t(size, freq);
    if (rstrs) {
        for (int i = 0; i < size; i++) {
            for (stringlist *sl = rstrs; sl; sl = sl->next) {
                const char *s = sl->string;
                if (!s)
                    continue;
                lstring::advtok(&s);
                int r = atoi(s);
                if (r == i+1) {
                    m->row_strings[i] = sl->string;
                    sl->string = 0;
                    break;
                }
            }
            if (!m->row_strings[i])
                m->row_strings[i] = lstring::copy("unknown");
        }
        stringlist::destroy(rstrs);
    }
    for (int i = 0; i < size; i++) {
        char *str;
        if ((str = getline(fp)) == 0) {
            delete m;
            return ("read error or premature EOF.");
        }

        const char *msg = "incorrect element count in row.";
        const char *s = str;
        for (int j = 0; j < size; j++) {
            char *tok = lstring::gettok(&s);
            if (!tok) {
                delete m;
                return (msg);
            }
            m->matrix[i][j].re = atof(tok);
            delete [] tok;
            tok = lstring::gettok(&s);
            if (!tok) {
                delete m;
                return (msg);
            }
            m->matrix[i][j].im = atof(tok);
            delete [] tok;
        }
        delete [] str;
    }

    *mret = m;
    return (0);
}

