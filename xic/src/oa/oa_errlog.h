
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
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

