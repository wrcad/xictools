
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef SPGLOBAL_H
#define SPGLOBAL_H


struct updif_t;

struct sGlobal
{
    void initialize(const char*);

    const char *Product()               { return (g_product); }
        // program name

    const char *Version()               { return (g_version); }
        // version number

    const char *DevlibVersion()         { return (g_devlib_version); }
        // device library version

    const char *TagString()             { return (g_tag_string); }
        // CVS tag string

    const char *Notice()                { return (g_notice); }
        // text printed with version

    const char *BuildDate()             { return (g_build_date); }
        // date string of build

    const char *OSname()                { return (g_os_name); }
        // operating system name

    const char *Arch()                  { return (g_arch); }
        // target architecture name

    const char *DistSuffix()            { return (g_dist_suffix); }
        // distribution file suffix

    const char *GfxProgName()           { return (g_gfx_prog_name); }
        // graphical editor program name

    const char *FifoName()              { return (g_fifo_name); }
    void SetFifoName(char *n)           { g_fifo_name = n; }
        // name of source fifo


    const char *ToolsRoot()             { return (g_tools_root); }
        // XicTools container installation directory

    const char *AppsRoot()              { return (g_app_root); }
        // Installation subdirectory name

    const char *Prefix()                { return (g_prefix); }
        // installation location
        // environment: XT_PREFIX

    const char *ProgramRoot()           { return (g_program_root); }
        // Installation directory
        // environment: WRSPICE_HOME


    const char *StartupDir()            { return (g_startup_dir); }
        // system directory for startup files
        // environment: SPICE_LIB_DIR

    const char *InputPath()             { return (g_input_path); }
        // path to scripts, circuit files
        // environment: SPICE_INP_PATH

    const char *HelpPath()              { return (g_help_path); }
        // search path for help
        // environment: SPICE_HLP_PATH

    const char *DocsDir()               { return (g_docs_dir); }
        // path to release notes
        // environment: SPICE_DOCS_DIR


    const char *ExecDir()               { return (g_exec_dir); }
        // directory for executables
        // environment: SPICE_EXEC_DIR

    const char *ExecProg()              { return (g_exec_prog); }
        // full path to program (for apsice)
        // environment: SPICE_PATH

    const char *NewsFile()              { return (g_news_file); }
        // news file printed at startup
        // environment: SPICE_NEWS_FILE

    const char *Editor()                { return (g_editor); }
        // text editor name or path
        // environment: SPICE_EDITOR

    const char *BugAddr()               { return (g_bug_addr); }
        // address for "bug" command
        // environment: SPICE_BUGADDR

    const char *Host()                  { return (g_host); }
        // host for remote spice (rspice) runs
        // environment: SPICE_HOST

    const char *DaemonLog()             { return (g_daemon_log); }
        // path to daemon log file
        // environment: SPICE_DAEMONLOG


    char OptChar()                      { return (g_opt_char); }
        // command line option prefix
        // environment: SPICE_OPTCHAR

    bool AsciiOut()                     { return (g_ascii_out); }
        // set if ASCII rawfile output
        // environment: SPICE_ASCIIRAWFILE

    bool MemError()                     { return (g_mem_error); }
    void SetMemError(bool b)            { g_mem_error = b; }
        // set by application if memory error.

    static const updif_t *UpdateIf()    { return (g_upd_if); }

private:
    const char *g_product;
    const char *g_version;
    const char *g_devlib_version;
    const char *g_tag_string;
    const char *g_notice;
    const char *g_build_date;
    const char *g_os_name;
    const char *g_arch;
    const char *g_dist_suffix;
    const char *g_gfx_prog_name;
    const char *g_fifo_name;

    const char *g_tools_root;
    const char *g_app_root;
    const char *g_prefix;
    const char *g_program_root;

    const char *g_startup_dir;
    const char *g_input_path;
    const char *g_help_path;
    const char *g_docs_dir;

    const char *g_exec_dir;
    const char *g_exec_prog;
    const char *g_news_file;
    const char *g_editor;
    const char *g_bug_addr;
    const char *g_host;
    const char *g_daemon_log;

    char g_opt_char;
    bool g_ascii_out;
    bool g_mem_error;

    static const updif_t *g_upd_if;
};
extern sGlobal Global;

// Turn on security tests
#define SECURITY_TEST

// Mail address for security reports
#define MAIL_ADDR "stevew@srware.com"

#endif

