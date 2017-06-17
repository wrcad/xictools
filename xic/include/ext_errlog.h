
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
 $Id: ext_errlog.h,v 5.7 2013/11/24 02:34:12 stevew Exp $
 *========================================================================*/

#ifndef EXT_ERRLOG_H
#define EXT_ERRLOG_H


//
// Logging and error recording for use during extraction.
//

#define EL_ERRFILE          "extraction.errs"
#define EL_GROUP_LOG        "group.log"
#define EL_EXTRACT_LOG      "extract.log"
#define EL_ASSOCIATE_LOG    "associate.log"
#define EL_RLSOLVERLOG      "rlsolver.log"

struct CDs;
struct sDevInst;

// The classes of log messages.
enum ExtLogType {
    ExtLogGrp,      // log grouping
    ExtLogGrpV,     // log grouping, verbose
    ExtLogExt,      // log extracting
    ExtLogExtV,     // log extracting, verbose
    ExtLogAssoc,    // log associating
    ExtLogAssocV    // log associating, verbose
};

class cExtErrLog
{
public:
    enum ELstate { ELidle, ELgrouping, ELextracting, ELassociating };

    cExtErrLog();
    ~cExtErrLog();

    void start_logging(ExtLogType, CDcellName);
    void end_logging();

    void add_log(ExtLogType, const char*, ...);
    void add_err(const char*, ...);
    void add_dev_err(const CDs*, const sDevInst*, const char*, ...);

    bool log_grouping()         const { return (el_log_grouping); }
    void set_log_grouping(bool b)     { el_log_grouping = b; }
    bool log_extracting()       const { return (el_log_extracting); }
    void set_log_extracting(bool b)   { el_log_extracting = b; }
    bool log_associating()      const { return (el_log_associating); }
    void set_log_associating(bool b)  { el_log_associating = b; }
    bool verbose()              const { return (el_verbose); }
    void set_verbose(bool b)          { el_verbose = b; }

    FILE *log_fp()              const { return (el_logfp); }

    bool log_rlsolver()         const { return (el_rlex_log); }
    void set_log_rlsolver(bool b)
        {
            if (b) {
                if (!el_rlex_fp)
                    el_rlex_fp = open_rlsolver_log();
                el_rlex_log = (el_rlex_fp != 0);
            }
            else {
                if (el_rlex_fp) {
                    fclose(el_rlex_fp);
                    el_rlex_fp = 0;
                }
                el_rlex_log = false;
            }
        }

    bool rlsolver_msgs()        const { return (el_rlex_msg); }
    void set_rlsolver_msgs(bool b)    { el_rlex_msg = b; }
    FILE *rlsolver_log_fp()     const { return (el_rlex_fp); }

private:
    FILE *open_rlsolver_log();
    void open_files();
    void close_files();

    CDcellName el_cellname;
    FILE *el_logfp;
    FILE *el_errfp;
    char *el_logfile;
    char *el_errfile;

    int el_group_errcnt;
    int el_extract_errcnt;
    int el_associate_errcnt;
    ELstate el_state;
    bool el_warned_g_log;
    bool el_warned_e_log;
    bool el_warned_a_log;
    bool el_warned_err;

    bool el_log_grouping;
    bool el_log_extracting;
    bool el_log_associating;
    bool el_verbose;

    FILE *el_rlex_fp;
    bool el_rlex_log;
    bool el_rlex_msg;
};

extern cExtErrLog ExtErrLog;

#endif

