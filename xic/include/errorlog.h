
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
 $Id: errorlog.h,v 5.17 2016/05/25 04:49:01 stevew Exp $
 *========================================================================*/

#ifndef ERRORLOG_H
#define ERRORLOG_H

struct stringlist;

// Some message header strings for global use.
namespace mh {
    extern const char *DRC;
    extern const char *DRCViolation;
    extern const char *ObjectCreation;
    extern const char *EditOperation; 
    extern const char *Flatten;
    extern const char *CellPlacement;
    extern const char *Techfile;
    extern const char *Initialization;
    extern const char *Internal;
    extern const char *NetlistCreation;
    extern const char *Variables;
    extern const char *PCells;
    extern const char *Properties;
    extern const char *JobControl;
    extern const char *Processing;
    extern const char *OpenAccess;
    extern const char *InputOutput;
}

// Struct to maintain a file and list of error or warning messages.
struct sMsgList
{
    sMsgList()
        {
            ml_msg_list = 0;
            ml_log_filename = 0;
            ml_msg_count = 0;
        }

    int get_count()             { return (ml_msg_count); }
    const char *log_filename()  { return (ml_log_filename); }

    void set_filename(const char*);
    char *get_msg(int);
    void add_msg(bool, const char*, const char*, int);

private:
    stringlist *ml_msg_list;    // List of most recent messages.
    char *ml_log_filename;      // Name of log file.
    int ml_msg_count;           // Count of messages.
};


inline class cErrLog *Log();

class cErrLog
{
    static cErrLog *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cErrLog *Log() { return (cErrLog::ptr()); }

    cErrLog();

    void ErrorLog(const char*, const char*);
    void ErrorLogV(const char*, const char*, ...);
    void WarningLog(const char*, const char*);
    void WarningLogV(const char*, const char*, ...);

    void PopUpErr(const char*);
    void PopUpErrEx(const char*, bool = false, int = -1);
    void PopUpErrV(const char*, ...);
    void PopUpWarn(const char*);
    void PopUpWarnEx(const char*, bool = false, int = -1);
    void PopUpWarnV(const char*, ...);

    bool OpenLogDir(const char*);
    stringlist *ListLogDir();
    void CloseLogDir();
    FILE *OpenLog(const char*, const char*, bool = false);

    void SetMsgLogName(const char *name)
        {
            el_list.set_filename(name);
        }

    void SetMemErrLogName(const char *name)
        {
            delete [] el_mem_err_log_name;
            el_mem_err_log_name = lstring::copy(name);
        }

    void SetMailAddress(const char *name)
        {
            delete [] el_mail_address;
            el_mail_address = lstring::copy(name);
        }

    // Return the current error record number.  This corresponds to
    // the most recent message recorded.
    //
    int GetMsgNum()             { return (el_list.get_count()); }

    // Return a *copy* of the error message corresponding to ernum, or
    // 0 if out of range.  The range is the current error number and
    // the positive KEEPMSGS below that.
    //
    char *GetMsg(int num)       { return (el_list.get_msg(num)); }

    const char *MemErrLogName() { return (el_mem_err_log_name); }
    const char *MailAddress()   { return (el_mail_address); }
    const char *LogDirectory()  { return (el_log_directory); }

private:
    // errorlog_setif.cc
    void setupInterface();

    char *el_log_directory;     // Temp directory for log files
    char *el_mem_err_log_name;  // Memory error log file ("xic_mem_errors.log")
    char *el_mail_address;      // Email address for return

    sMsgList el_list;           // Errors and warnings list.

    static cErrLog *instancePtr;
};

#endif

