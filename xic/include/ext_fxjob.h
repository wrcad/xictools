
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef EXT_FXJOB_H
#define EXT_FXJOB_H

#include "miscutil/lstring.h"
#include <sys/types.h>

//------------------------------------------------------------------------
// Job Control for Fast[er]Cap/FastHenry interface
//------------------------------------------------------------------------

// Default dataset name
#define DEF_DATASET "unnamed"

// Base class for graphical interface panel.
//
struct fxGif
{
    virtual void PopUpExtIf(GRobject, ShowMode) = 0;
    virtual void updateString() = 0;
};

// fxJob flags: unlink temp file when done?
#define FX_UNLINK_IN  0x1
#define FX_UNLINK_OUT 0x2
#define FX_UNLINK_RES 0x4

// Basic interface mode.
enum fxJobMode { fxCapMode, fxIndMode };

// The types of programs supported:
// - The Whiteley Research program (fastcap/fasthenry).
// - The original MIT version or equivalent.
// - A successor program (used for FasterCap 5.0.2 and later only).
//
enum fxJobType { fxJobWR, fxJobMIT, fxJobNEW };

namespace { struct zmat_t; }

// Each asynchronous job is assigned an fxJob struct.
//
struct fxJob
{
    fxJob(const char*, fxJobMode, fxGif*, fxJob*);
    ~fxJob();

    static const char *fc_default_path();
    static const char *fh_default_path();

    bool setup_fc_run(bool, bool);
    bool setup_fh_run(bool, bool);
    bool run(bool, bool);
    void fc_post_process();
    void fh_post_process();
    void pid_string(sLstr&);
    void kill_process();

    void post_proc()
        {
            if (j_mode == fxCapMode)
                fc_post_process();
            else if (j_mode == fxIndMode)
                fh_post_process();
        }

    const char *infile()            { return (j_infiles->string); }
    const char *outfile()           { return (j_outfile); }
    const char *command()           { return (j_command); }
    fxJobType if_type()             { return (j_iftype); }

    stringlist **file_list_ptr()
        {
            return (j_infiles ? &j_infiles->next : 0);
        }

    void set_flag(unsigned int f)   { j_flags |= f; }
    void set_infiles(stringlist *s) { j_infiles = s; }
    void set_outfile(char *fn)      { j_outfile = fn; }
    void set_resfile(char *fn)      { j_resfile = fn; }
    fxJob *next_job()               { return (next); }

    static fxJob *jobs()            { return (j_jobs); }
    static void set_jobs(fxJob *j)  { j_jobs = j; }

    static int num_jobs()
        {
            int cnt = 0;
            for (fxJob *j = j_jobs; j; j = j->next)
                cnt++;
            return (cnt);
        }

    static fxJob *find(int pid)
        {
            for (fxJob *j = j_jobs; j; j = j->next) {
                if (j->j_pid == pid)
                    return (j);
            }
            return (0);
        }

private:
    static char *fc_get_matrix(const char*, int*, float***, double*, char***);
    static const char *fh_get_matrix(FILE*, zmat_t**);

    fxJobMode j_mode;           // program to run

    stringlist *j_infiles;      // source files list
    // The main list file must be first for FastCap old format. 
    // In other cases, there is only one file.

    char *j_outfile;            // output file path
    char *j_resfile;            // log file path
    char *j_command;            // command string
    char *j_dataset;            // data set name
    fxGif *j_gif;               // graphical interface
    int j_pid;                  // process id
    int j_num;                  // job number;
    int j_flags;                // UNLINK_XX flags
    fxJobType j_iftype;         // program interface
    time_t j_start_time;        // time run started
    fxJob *next;

    static fxJob *j_jobs;       // list of running jobs
    static char *j_fc_def_path; // fastcap default path
    static char *j_fh_def_path; // fasthenry default path
};

#endif

