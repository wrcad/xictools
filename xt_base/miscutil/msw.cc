
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

// Needed for KEY_WOW64_64KEY
#define WINVER 0x502

#include "msw.h"
#include "lstring.h"
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#ifdef WIN32
#include <mapi.h>


// This function returns false for Win-95/98/ME, true for NT/2000/XP and
// anything that comes later.
//
bool
msw::IsWinNT()
{
    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osv);
    if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT)
        return (true);
    if (osv.dwMajorVersion >= 5)
        return (true);
    return (false);
}


namespace {
    // The inno installer places an item in the registry which gives the
    // location of the uninstall directory.  Return the full path to the
    // directory containing this directory.  The returned path uses '/' as
    // the separator character.
    //
    // This assumes admin install only.
    //
    // Locations:
    // Xic, XicII, WRspice:
    //  .../xictools/program/uninstall/uninst000.exe
    // XicTools accessories:
    //  .../xictools/accs-uninstall/uninst000.exe
    //
    static char *
    get_inno_uninst(const char *program)
    {
        unsigned int key_read = KEY_READ;

        // Note that "program" is the name used by the installer.
        char buf[1024];
        sprintf(buf,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s_is1",
            program);

        HKEY key;
        long ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, key_read, &key);
        if (ret != ERROR_SUCCESS) {
            // Try again with WOW64.  This seemed to be required in
            // early versions of Win-7_64 (could be wrong about this),
            // but presently this flag causes failure.
            //
            OSVERSIONINFO osv;
            osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx(&osv);
            if (osv.dwMajorVersion > 5 ||
                    (osv.dwMajorVersion == 5 && osv.dwMinorVersion >= 1)) {
                // XP or later
                key_read |= KEY_WOW64_64KEY;
                ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, key_read, &key);
            }
            if (ret != ERROR_SUCCESS)
                return (0);
        }
        DWORD len = 1024;
        DWORD type;
        ret = RegQueryValueEx(key, "UninstallString", 0, &type, (BYTE*)buf,
            &len);
        RegCloseKey(key);

        // The string should contain the full path to the uninstall program,
        //
        if (ret != ERROR_SUCCESS || type != REG_SZ || len < 20)
            return (0);

        char *s = buf;
        char *p = lstring::getqtok(&s);
        s = strrchr(p, '\\');
        if (!s) {
            delete [] p;
            return (0);
        }
        *s = 0;
        s = strrchr(p, '\\');
        if (!s) {
            delete [] p;
            return (0);
        }
        *s = 0;

        lstring::unix_path(p);
        return (p);
    }


    // The Ghost Installer places an item in the registry which gives the
    // location of the uninstall.log file.  Return the full path to the
    // directory containing this file.  The returned path uses '/' as the
    // separator character
    //
    // Locations:
    // Xic, XicII, WRspice:
    //  .../xictools/program/setup/install.log
    // XicTools accessories:
    //  .../xictools/accs-install.log
    //
    static char *
    get_gins_uninst(const char *program)
    {
        unsigned int key_read = KEY_READ;

        // Note that "program" is the name used by the installer.
        char buf[1024];
        sprintf(buf,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s",
            program);

        HKEY key;
        long ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, key_read, &key);
        if (ret != ERROR_SUCCESS) {
            // Try again with WOW64.  This seemed to be required in
            // early versions of Win-7_64 (could be wrong about this),
            // but presently this flag flag causes failure.
            //
            OSVERSIONINFO osv;
            osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx(&osv);
            if (osv.dwMajorVersion > 5 ||
                    (osv.dwMajorVersion == 5 && osv.dwMinorVersion >= 1)) {
                // XP or later
                key_read |= KEY_WOW64_64KEY;
                ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, key_read, &key);
            }
            if (ret != ERROR_SUCCESS)
                return (0);
        }
        DWORD len = 1024;
        DWORD type;
        ret = RegQueryValueEx(key, "UninstallString", 0, &type, (BYTE*)buf,
            &len);
        RegCloseKey(key);

        // The string should contain the full path to the uninstall program,
        // followed by the path to the uninstall log file, possibly quoted.
        // If the string length is too short to make sense, abort
        //
        if (ret != ERROR_SUCCESS || type != REG_SZ || len < 20)
            return (0);

        char *s = lstring::copy(buf);
        // Remove any quotes, and peel off the argument of the uninstall
        // command, which is the full path to install.log
        //
        char *t = s + strlen(s) - 1;
        while (t >= s && *t == '"')
            *t-- = '\0';
        while (t > s && *t != '"' && (!isalpha(*t) || *(t+1) != ':' ||
                (*(t+2) != '/' && *(t+2) != '\\')))
            t--;
        char *dir = 0;
        bool ok = false;
        if (t > s && *t != '"') {
            // found the start of the path
            dir = lstring::copy(t);
            t = lstring::strrdirsep(dir);
            if (t) {
                *t = 0;  // stripped "/uninstall.log";
                ok = true;
            }
        }
        delete [] s;
        if (!ok) {
            delete [] dir;
            dir = 0;
        }
        lstring::unix_path(dir);
        return (dir);
    }
}


// Return the path to the uninstall data.
//
char *
msw::GetInstallDir(const char *program)
{
    // Use the application name used in the setup file.  This
    // differentiates from previous releases.
    //
    char buf[256];
    sprintf(buf, "%s%s", program, MSWpkgSuffix);

    char *dir = get_inno_uninst(buf);
    if (!dir)
        dir = get_gins_uninst(buf);
    return (dir);
}


// Return the equivalent of "/usr/local/xictools/xic" rooted in the
// actual installation location.  This function is exported.
//
char *
msw::GetProgramRoot(const char *program)
{
    char *s = msw::GetInstallDir(program);
    if (!s)
        return (0);
    // This is the path the the install log file or directory, which may
    // be in the startup directory or its parent.
    char *t = lstring::strrdirsep(s);
    if (t && lstring::cieq(t+1, "startup")) {
        *t = 0;
        t = lstring::strrdirsep(s);
    }
    if (t && lstring::cieq(t+1, program))
        return (s);
    delete [] s;
    return (0);
}


// Get the product ID from the registry into buf.  This should be
// unique to the Windows distribution CD.  If fp, print a log entry to
// fp.
//
// In 64-bit Windows, we have to look in the 64-bit registry, by setting
// the KEY_WOW64_64KEY flag.
// See http://msdn.microsoft.com/en-us/library/aa384129(VS.85).aspx
//
// This flag should be ignored in 32-bit Windows.  It will fail in
// 64-bit Windows-2000.
//
bool
msw::GetProductID(char *buf, FILE *fp)
{
    unsigned int key_read = KEY_READ;
    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osv);
    if (osv.dwMajorVersion > 5 ||
            (osv.dwMajorVersion == 5 && osv.dwMinorVersion >= 1))
        // XP or later
        key_read |= KEY_WOW64_64KEY;

    HKEY key;
    const char *sk = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    long ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sk, 0, key_read, &key);
    if (ret != ERROR_SUCCESS) {
        sk = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sk, 0, key_read, &key);
    }
    if (ret != ERROR_SUCCESS && (key_read && KEY_WOW64_64KEY)) {
        // Try again without WOW64, probably won't work but try anyway.
        key_read &= ~KEY_WOW64_64KEY;
        sk = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sk, 0, key_read, &key);
        if (ret != ERROR_SUCCESS) {
            sk = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
            ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sk, 0, key_read, &key);
        }
    }
    if (ret != ERROR_SUCCESS)
        return (false);

    DWORD len = 256, type;
    ret = RegQueryValueEx(key, "ProductId", 0, &type, (BYTE*)buf, &len);
    RegCloseKey(key);
    if (ret == ERROR_SUCCESS && type == REG_SZ)
        return (true);

    char kn[256];
    char tbuf[256];
    sk = "SOFTWARE\\Microsoft";
    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sk, 0, key_read, &key);
    if (ret != ERROR_SUCCESS) {
        sprintf(buf, "RegOpenKeyEx failed 1, ret= %ld", ret);
        return (false);
    }
    for (int i = 0; ; i++) {
        DWORD ksz = 256;
        ret = RegEnumKeyEx(key, i, tbuf, &ksz, 0, 0, 0, 0);
        if (ret != ERROR_SUCCESS)
            break;
        if (strncasecmp(tbuf, "windows", 7))
            continue;

        char *t = tbuf + 7;
        if (fp)
            fprintf(fp, "%s\n", tbuf);
        while (isspace(*t))
            t++;
        if (!*t || isdigit(*t) || !strcasecmp(t, "nt")) {

            sprintf(kn, "SOFTWARE\\Microsoft\\%s\\CurrentVersion", tbuf);
            HKEY subkey;
            ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, kn, 0, key_read, &subkey);
            if (ret != ERROR_SUCCESS) {
                if (fp)
                    fprintf(fp, "RegOpenKeyEx failed 2, ret= %ld\n", ret);
                continue;
            }
            char *idname = 0;
            for (int j = 0; ; j++) {
                DWORD bsz = 256;
                ret = RegEnumValue(subkey, j, tbuf, &bsz, 0, &type, 0, 0);
                if (ret != ERROR_SUCCESS)
                    break;
                else {
                    if (!strncasecmp(tbuf, "product", 7)) {
                        t = tbuf + 7;
                        while (isspace(*t))
                            t++;
                        if (!strncasecmp(t, "id", 2)) {
                            idname = strdup(tbuf);
                            if (fp)
                                fprintf(fp, "  (id name:  %s)\n", idname);
                        }
                    }
                }
            }
            if (idname) {
                len = 256;
                BYTE *data = (BYTE*)buf;
                ret = RegQueryValueEx(subkey, idname, 0, &type, data, &len);
                if (ret == ERROR_SUCCESS && type == REG_SZ) {
                    RegCloseKey(subkey);
                    RegCloseKey(key);
                    return (true);
                }
                else {
                    if (fp)
                        fprintf(fp,
                            "RegQueryValueEx failed, ret = %ld, type = %ld",
                            ret, type);
                }
            }
            RegCloseKey(subkey);
        }
    }
    RegCloseKey(key);
    sprintf(buf, "not found");
    return (false);
}


// Make sure that each rooted directory has a drive prefix, which is
// obtained from the registry.
//
char *
msw::AddPathDrives(const char *path, const char *progname)
{
    static char *install_drv;
    if (!path)
        return (0);
    sLstr lstr;
    const char *s = path;
    while (isspace(*s) || *s == '(') {
        lstr.add_c(*s);
        s++;
    }
    const char *quot = 0;
    while (*s) {
        if (*s == '"' || *s =='\'') {
            quot = s++;
            lstr.add_c(*quot);
        }
        if (lstring::is_dirsep(*s) && !lstring::is_dirsep(*(s+1))) {
            // If two dirseps, it is a share, which is ok as-is.
            if (!install_drv) {
                char *t = GetInstallDir(progname);
                if (!t)
                    return (lstring::copy(path));
                if (t[1] != ':') {
                    delete [] t;
                    return (lstring::copy(path));
                }
                t[2] = 0;
                install_drv = lstring::copy(t);
                delete [] t;
            }
            lstr.add(install_drv);
        }
        if (quot) {
            while (*s && *s != *quot)
                lstr.add_c(*s++);
            lstr.add_c(*quot);
            quot = 0;
        }
        while (*s && !isspace(*s) && *s != ';') {
            lstr.add_c(*s);
            if (*s == '"' || *s =='\'') {
                quot = s++;
                while (*s && *s != *quot)
                    lstr.add_c(*s++);
                lstr.add_c(*quot);
                quot = 0;
                continue;
            }
            s++;
        }
        while (isspace(*s) || *s == ';' || *s == ')') {
            lstr.add_c(*s);
            s++;
        }
    }
    return (lstr.string_trim());
}


// Exec cmdline as a new sub-process.
//
PROCESS_INFORMATION *
msw::NewProcess(const char *cmdline, unsigned int flags, bool inherit_hdls,
    void *hin, void *hout, void *herr)
{
    if (!cmdline)
        return (0);
    // The path has to be in DOS format, and have a volume specifier.
    char *cmd = new char[strlen(cmdline) + 4];
    if (!isalpha(cmdline[0]) || cmdline[1] != ':') {
        char *cwd = getcwd(0, 0);
        if (!cwd) {
            delete [] cmd;
            return (0);
        }
        if (cwd[1] != ':') {
            // CWD not local.
            free(cwd);
            delete [] cmd;
            return (0);
        }
        cmd[0] = cwd[0];
        cmd[1] = cwd[1];
        free(cwd);
        if (lstring::is_dirsep(*cmdline))
            strcpy(cmd+2, cmdline);
        else {
            cmd[2] = '/';
            strcpy(cmd+3, cmdline);
        }
    }
    else
        strcpy(cmd, cmdline);
    for (char *s = cmd; *s; s++) {
        if (*s == '/')
            *s = '\\';
    }
    STARTUPINFO startup;
    memset(&startup, 0, sizeof(STARTUPINFO));
    startup.cb = sizeof(STARTUPINFO);
    if (hin || hout || herr) {
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = hin ? hin : GetStdHandle(STD_INPUT_HANDLE);
        startup.hStdOutput = hout ? hout : GetStdHandle(STD_OUTPUT_HANDLE);
        startup.hStdError = herr ? herr : GetStdHandle(STD_ERROR_HANDLE);
    }
    PROCESS_INFORMATION *info = new PROCESS_INFORMATION;
    if (!CreateProcess(0, cmd, 0, 0, inherit_hdls, flags, 0, 0, &startup,
            info)) {
        delete info;
        delete [] cmd;
        return (0);
    }
    delete [] cmd;
    CloseHandle(info->hThread);
    return (info);
}


// Status of MAPI in Windows 8.1.
//
// It doesn't exist.  It will be installed with certain applications,
// including Microsoft Office (Exchange), Thunderbird, Windows Live
// Mail.  The latter is part of a free download from Microsoft called
// Windows Essentials.  The program must be configured to send
// outgoing email to a server.
//
// Microsoft recommends using the "W" version of MAPISendMail in
// Windows 8.1, and in fact the original version is deprecated. 
// However, it still seems to work the same as the "W" version at
// present, so it will be used here for compatibility.
//
// Naturally, with Microsoft, there is always a problem.  Unless the
// Windows Live interface is brought up by giving MAPI_DIALOG, the
// Date header is missing, causing any self-respecting email receiver
// to reject the message.  This is a bug that Microsoft has known
// about for years but never bothered to fix.

//#define WIN8
#ifdef WIN8
    // These aren't in the MinGW headers yet.

typedef struct {
    ULONG       ulReserved;
    ULONG       ulRecipClass;
    PWSTR       lpszName;
    PWSTR       lpszAddress;
    ULONG       ulEIDSize;
    PVOID       lpEntryID;
} MapiRecipDescW, *lpMapiRecipDescW;

typedef struct {
    ULONG       ulReserved;
    ULONG       flFlags;
    ULONG       nPosition;
    PWSTR       lpszPathName;
    PWSTR       lpszFileName;
    PVOID       lpFileType;
} MapiFileDescW, *lpMapiFileDescW;

typedef struct {
    ULONG       ulReserved;
    PWSTR       lpszSubject;
    PWSTR       lpszNoteText;
    PWSTR       lpszMessageType;
    PWSTR       lpszDateReceived;
    PWSTR       lpszConversationID;
    FLAGS       flFlags;
    lpMapiRecipDescW lpOriginator;
    ULONG       nRecipCount;
    lpMapiRecipDescW lpRecips;
    ULONG       nFileCount;
    lpMapiFileDescW lpFiles;
} MapiMessageW, *lpMapiMessageW;

ULONG WINAPI MAPISendMailW(LHANDLE, ULONG_PTR, lpMapiMessageW, FLAGS, ULONG);
typedef ULONG WINAPI (*LPMAPISENDMAILW)(LHANDLE, ULONG_PTR, lpMapiMessageW, FLAGS, ULONG);

#endif


// Send email using the Simple MAPI interface.  The function returns an
// error message if things don't go well, 0 otherwise.  The attach array
// contains full path names, including drive letter, to attachment files.
//
const char *
msw::MapiSend(const char *toaddr, const char *subject, const char *body,
    int anum, const char **attach)
{
    static HMODULE hInstMail;

    if (!hInstMail)
        hInstMail = LoadLibrary("MAPI32.DLL");
    if (!hInstMail)
        return ("can't load MAPI32.DLL");

#ifdef WIN8
    LPMAPISENDMAILW lpfnSendMail =
        (LPMAPISENDMAILW)GetProcAddress(hInstMail, "MAPISendMailW");
#else
    LPMAPISENDMAIL lpfnSendMail =
        (LPMAPISENDMAIL)GetProcAddress(hInstMail, "MAPISendMail");
#endif
    if (!lpfnSendMail)
        return ("can't find entry point in MAPI32.DLL");

    int nrecip = 0;
    const char *s = toaddr;
    while (lstring::advtok(&s) != false)
        nrecip++;
    if (!nrecip)
        return ("no recipients");

#ifdef WIN8
    MapiRecipDescW *recip = new MapiRecipDescW[nrecip];
    memset(recip, 0, nrecip*sizeof(MapiRecipDescW));
    s = toaddr;
    for (int i = 0; i < nrecip; i++) {
        char *t = lstring::gettok(&s);
        char *ad = new char[strlen(t) + 6];
        strcpy(ad, "SMTP:");
        strcat(ad, t);
        delete [] t;
        int n = strlen(ad);
        char *p = new char[4*n];
        MultiByteToWideChar(CP_ACP, 0, ad, -1, (PWSTR)p, 2*n);
        recip[i].lpszAddress = (PWSTR)p;
        delete [] ad;
        recip[i].ulRecipClass = MAPI_TO;
    }
#else
    MapiRecipDesc *recip = new MapiRecipDesc[nrecip];
    memset(recip, 0, nrecip*sizeof(MapiRecipDesc));
    s = toaddr;
    for (int i = 0; i < nrecip; i++) {
        char *t = lstring::gettok(&s);
        char *ad = new char[strlen(t) + 6];
        strcpy(ad, "SMTP:");
        strcat(ad, t);
        delete [] t;
        recip[i].lpszAddress = ad;
        recip[i].ulRecipClass = MAPI_TO;
    }
#endif

#ifdef WIN8
    MapiFileDescW *adesc = 0;
    if (anum > 0) {
        adesc = new MapiFileDescW[anum];
        memset(adesc, 0, anum*sizeof(MapiFileDescW));
        for (int i = 0; i < anum; i++) {
            adesc[i].nPosition = (ULONG)-1;
            int n = strlen(attach[i]);
            char *p = new char[4*n];
            MultiByteToWideChar(CP_ACP, 0, attach[i], -1, (PWSTR)p, 2*n);
            adesc[i].lpszPathName = (PWSTR)p;
        }
    }
#else
    MapiFileDesc *adesc = 0;
    if (anum > 0) {
        adesc = new MapiFileDesc[anum];
        memset(adesc, 0, anum*sizeof(MapiFileDesc));
        for (int i = 0; i < anum; i++) {
            adesc[i].nPosition = (ULONG)-1;
            adesc[i].lpszPathName = lstring::copy(attach[i]);
        }
    }
#endif

    // prepare the message
#ifdef WIN8
    MapiMessageW message;
    memset(&message, 0, sizeof(message));
    int n = strlen(subject);
    char *p1 = new char[4*n];
    MultiByteToWideChar(CP_ACP, 0, subject, -1, (PWSTR)p1, 2*n);
    message.lpszSubject = (PWSTR)p1;
    n = strlen(body);
    char *p2 = new char[4*n];
    MultiByteToWideChar(CP_ACP, 0, body, -1, (PWSTR)p2, 2*n);
    message.lpszNoteText = (PWSTR)p2;
#else
    MapiMessage message;
    memset(&message, 0, sizeof(message));
    message.lpszSubject = lstring::copy(subject);
    message.lpszNoteText = lstring::copy(body);
#endif

    message.nRecipCount = nrecip;
    message.lpRecips = recip;
    message.nFileCount = anum;
    message.lpFiles = adesc;

#ifdef WIN8
//    int nError = lpfnSendMail(0, 0, &message, MAPI_DIALOG, 0);
    int nError = lpfnSendMail(0, 0, &message, 0, 0);
#else
//    int nError = lpfnSendMail(0, 0, &message, MAPI_DIALOG, 0);
    int nError = lpfnSendMail(0, 0, &message, 0, 0);
#endif

    delete [] message.lpszSubject;
    delete [] message.lpszNoteText;
    for (int i = 0; i < anum; i++)
        delete [] adesc[i].lpszPathName;
    delete [] adesc;
    for (int i = 0; i < nrecip; i++)
        delete [] recip[i].lpszAddress;
    delete [] recip;

    switch(nError) {
    case SUCCESS_SUCCESS:
        break;
    case MAPI_E_USER_ABORT:
        return ("user aborted");
    case MAPI_E_LOGON_FAILURE:
        return ("provider login error");
    case MAPI_E_INSUFFICIENT_MEMORY:
        return ("insufficient memory");
    case MAPI_E_TOO_MANY_FILES:
        return ("too many files");
    case MAPI_E_TOO_MANY_RECIPIENTS:
        return ("too many recipients");
    case MAPI_E_ATTACHMENT_NOT_FOUND:
        return ("attachment not found");
    case MAPI_E_ATTACHMENT_OPEN_FAILURE:
        return ("can't open attachment");
    case MAPI_E_UNKNOWN_RECIPIENT:
        return ("unknown recipient");
    case MAPI_E_TEXT_TOO_LARGE:
        return ("text too long");
    case MAPI_E_AMBIGUOUS_RECIPIENT:
        return ("ambiguous recipient");
    case MAPI_E_INVALID_RECIPS:
        return ("invalid recipient(s)");
    default:
    case MAPI_E_FAILURE:
        return ("unknown error");
    }
    return (0);
}


// Since Bill's multiline edit controls need \r\n line termination, here
// is a function that takes a perfectly good text string and adds the
// appropriate garbage.
//
char *
msw::Billize(const char *in)
{
    int tcnt = 0;
    int ncnt = 0;
    for (const char *s = in; *s; s++) {
        tcnt++;
        if (*s == '\n' && (s == in || *(s-1) != '\r'))
            ncnt++;
    }
    if (ncnt) {
        tcnt += ncnt;
        char *s0 = new char[tcnt+1];
        char *t = s0;
        for (const char *s = in; *s; s++) {
            if (*s == '\n' && (s == in || *(s-1) != '\r')) {
                *t++ = '\r';
                *t++ = '\n';
            }
            else
                *t++ = *s;
        }
        *t = 0;
        return (s0);
    }
    return (lstring::copy(in));
}


// Get rid of the '\r' characters (back to UNIX format), in place.
//
void 
msw::UnBillize(char *in)
{
    char *s = in;
    char *t = in;
    while (*s) {
        if (*s == '\r' && *(s+1) == '\n') {
            *t++ = '\n';
            s++;
            s++;
            continue;
        }
        *t++ = *s++;
    }
    *t = 0;
}


int
msw::ListPrinters(int *curprinter, char ***printers)
{
    int numprinters = 0;
    *printers = 0;

    // list available printers
    if (msw::IsWinNT()) {
        DWORD sz, num;
        EnumPrinters(PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL,
            0, 4, 0, 0, &sz, &num);
        BYTE *bf = new BYTE[sz];
        if (EnumPrinters(PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL,
                0, 4, bf, sz, &sz, &num) && num) {
            PRINTER_INFO_4 *pi = (PRINTER_INFO_4*)bf;
            numprinters = num;
            *printers = new char*[num];
            for (unsigned i = 0; i < num; i++)
                (*printers)[i] = lstring::copy(pi[i].pPrinterName);
        }
        delete [] bf;
        char buf[256];
        if (GetProfileString("windows", "device", ",,,", buf, 256) > 0) {
            strtok(buf, ",");
            for (unsigned i = 0; i < num; i++) {
                if (!strcmp((*printers)[i], buf)) {
                    *curprinter = i;
                    break;
                }
            }
        }
    }
    else {
        DWORD sz, num;
        EnumPrinters(PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL,
            0, 5, 0, 0, &sz, &num);
        BYTE *bf = new BYTE[sz];
        if (EnumPrinters(PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL,
                0, 5, bf, sz, &sz, &num) && num) {
            PRINTER_INFO_5 *pi = (PRINTER_INFO_5*)bf;
            numprinters = num;
            *printers = new char*[num];
            for (unsigned i = 0; i < num; i++) {
                (*printers)[i] = lstring::copy(pi[i].pPrinterName);
                if (pi[i].Attributes & PRINTER_ATTRIBUTE_DEFAULT)
                    *curprinter = i;
            }
        }
        delete [] bf;
    }
    return (numprinters);
}


// Send a binary file to the named printer through the spooler
//
const char *
msw::RawFileToPrinter(const char *szPrinterName, const char *szFileName)
{
    // Need a handle to the printer.
    HANDLE hPrinter;
    if (!OpenPrinter((char*)szPrinterName, &hPrinter, 0))
        return ("can't open printer");

    // Fill in the structure with info about this "document."
    DOC_INFO_1 DocInfo;
    DocInfo.pDocName = (char*)"XicTools Document";
    DocInfo.pOutputFile = 0;
    DocInfo.pDatatype = (char*)"RAW";
    // Inform the spooler the document is beginning.
    DWORD dwJob;
    if ((dwJob = StartDocPrinter(hPrinter, 1, (BYTE*)&DocInfo)) == 0) {
        ClosePrinter(hPrinter);
        return ("start document call failed");
    }
    // Start a page.
    if (!StartPagePrinter(hPrinter)) {
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
        return ("start page call failed");
    }
    // Send the data to the printer.
    FILE *fp = fopen(szFileName, "rb");
    if (!fp) {
        EndPagePrinter(hPrinter);
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
        return ("can't open file");
    }
    BYTE buf[8192];
    for (;;) {
        DWORD n = fread(buf, 1, 8192, fp);
        if (n == 0)
            break;
        DWORD dwBytesWritten;
        if (!WritePrinter(hPrinter, buf, n, &dwBytesWritten) ||
                dwBytesWritten != n) {
            EndPagePrinter(hPrinter);
            EndDocPrinter(hPrinter);
            ClosePrinter(hPrinter);
            return ("data transfer error");
        }
    }
    fclose(fp);

    // End the page.
    if (!EndPagePrinter(hPrinter)) {
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
        return ("page end call failed");
    }
    // Inform the spooler that the document is ending.
    if (!EndDocPrinter(hPrinter)) {
        ClosePrinter(hPrinter);
        return ("document end call failed");
    }
    // Tidy up the printer handle.
    ClosePrinter(hPrinter);
    return (0);
}

#endif

