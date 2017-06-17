
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
 $Id: sced_errlog.h,v 5.6 2013/09/10 06:53:04 stevew Exp $
 *========================================================================*/

#ifndef SCED_ERRLOG_H
#define SCED_ERRLOG_H


//
// An error and message logger for use during connection.
//

#define SC_LOGFILE "connect.log"
#define SC_ERRFILE "connect.errs"

class cScedErrLog
{
public:
    cScedErrLog();
    ~cScedErrLog();

    void start_logging(CDcellName);
    void end_logging();

    void add_log(const char*, ...);
    void add_err(const char*, ...);

    // Logging of the connection establishment system.
    bool log_connect()      const { return (el_log_connect); }
    void set_log_connect(bool b)  { el_log_connect = b; }

private:
    void open_files();
    void close_files();

    CDcellName el_cellname;
    FILE *el_logfp;
    FILE *el_errfp;
    char *el_logfile;
    char *el_errfile;

    int el_errcnt;
    int el_level;
    bool el_warned_log;
    bool el_warned_err;
    bool el_log_connect;
};

extern cScedErrLog ScedErrLog;

#endif

