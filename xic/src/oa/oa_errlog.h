
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2013 Whiteley Research Inc, all rights reserved.        *
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
 $Id: oa_errlog.h,v 1.7 2015/03/26 04:32:16 stevew Exp $
 *========================================================================*/

#ifndef OA_ERRLOG_H
#define OA_ERRLOG_H


//
// An error and message logger for use during OA operations.
//

#define OA_ERRLOG   "system"
#define OA_READLOG  READ_OA_FN
#define OA_DBGLOG   "oa_debug.log"

// Log message categories.
enum OAlogType { OAlogLoad, OAlogNet, OAlogPCell };

class cOAerrLog
{
public:
    cOAerrLog();
    ~cOAerrLog();

    void start_logging(const char*, const char* = 0, const char* = 0);
    void end_logging();

    void add_log(OAlogType, const char*, ...);
    void add_err(int, const char*, ...);
    void set_show_log(bool);

    void set_return(int r)  { el_return = r; }

    bool debug_load()       const { return (el_dbg_load); }
    void set_debug_load(bool b)   { el_dbg_load = b; }
    bool debug_net()        const { return (el_dbg_net); }
    void set_debug_net(bool b)    { el_dbg_net = b; }
    bool debug_pcell()      const { return (el_dbg_pcell); }
    void set_debug_pcell(bool b)  { el_dbg_pcell = b; }

    FILE *err_file()        const { return (el_errfp); }

private:
    void open_files(const char*, const char*);
    void close_files();

    char *el_name;
    FILE *el_logfp;
    FILE *el_errfp;
    char *el_logfile;
    char *el_errfile;
    stringlist *el_names;

    int el_errcnt;
    int el_level;
    bool el_warned_log;
    bool el_warned_err;
    bool el_syserr;
    int el_return;
    bool el_dbg_load;
    bool el_dbg_net;
    bool el_dbg_pcell;
};

extern cOAerrLog OAerrLog;

struct cOAerrLogWrap
{
    cOAerrLogWrap(const char *name, const char *err, const char *log)
    {
        OAerrLog.start_logging(name, err, log);
    }

    ~cOAerrLogWrap()
    {
        OAerrLog.end_logging();
    }
};

#endif

