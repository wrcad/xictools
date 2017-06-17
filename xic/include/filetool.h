
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 * FILETOOL Integrated Circuit Layout Manipulation Tool                   *
 *                                                                        *
 *========================================================================*
 $Id: filetool.h,v 5.6 2015/10/11 19:35:46 stevew Exp $
 *========================================================================*/

#ifndef FILETOOL_H
#define FILETOOL_H


class cFileTool;

// Job description struct - handles the file conversion commands.
//
struct jobdesc
{
    jobdesc()
        {
            script_in = 0;
            script_out = 0;
            cmdstr = 0;
        }

    ~jobdesc()
        {
            reset();
        }

    void reset()
        {
            delete [] script_in;
            script_in = 0;
            delete [] script_out;
            script_out = 0;
            delete [] cmdstr;
            cmdstr = 0;
        }

    bool run(const char*, cFileTool*);

    char *script_in;        // existing script file to run
    char *script_out;       // name of script file to generate
    char *cmdstr;           // command directives.
};

struct sVbak;

class cFileTool
{
    friend bool jobdesc::run(const char*, cFileTool*);

public:
    cFileTool(const char*);
    ~cFileTool();

    bool run(int, char**);

private:
    jobdesc *process_args(int, char**);
    bool process_set(const char*);
    bool print_info(const char*, int);
    bool print_text(const char*, const char*, const char*);
    void printline(bool, const char*, ...);
    bool do_split(const char*);
    bool do_chd_file(const char*);
    bool do_compare(const char*);
    void init_signals();

    // -- Modules Interface
    void setupInterface();
    void revertInterface();

    static FILE *ifInitCvLog(const char*);
    static void ifPrintCvLog(OUTmsgType, const char*, va_list);
    static void ifShowCvLog(FILE*, OItype, const char*);
    static void ifInfoMessage(INFOmsgType, const char*, va_list);
    static const char *ifIdString();
    static void ifAmbiguityCallback(stringlist*, const char*, const char*,
        void (*)(const char*, void*), void*);
    static bool ifCheckInterrupt(const char*);

    // -- Variables Interface
    void setupVariables();

    char *ft_version;       // version number to print

    // -- Modules Interface
    CDif_InfoMessage cdif_info_message;
    CDif_IdString cdif_id_string;
    CDif_CheckInterrupt cdif_check_interrupt;
    FIOif_InitCvLog fioif_init_cv_log;
    FIOif_PrintCvLog fioif_print_cv_log;
    FIOif_ShowCvLog fioif_show_cv_log;
    FIOif_InfoMessage fioif_info_message;
    FIOif_AmbiguityCallback fioif_ambiguity_callback;

    FILE *ft_cvt_log_fp;    // log file pointer

    // -- Variables Interface
    sVbak *ft_vars;         // prior variable state

    int ft_verbose;         // message verbosity
    bool ft_holding;        // printline control flag
    bool ft_error;          // set if error
    bool ft_cvt_show_log;   // show log flag (Modules Interface)


    static const char *ft_usage;
    static cFileTool *ft_instance;
};

#endif

