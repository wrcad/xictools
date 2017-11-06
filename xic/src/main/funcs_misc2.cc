
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

#include "config.h"
#include "main.h"
#include "editif.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_sdb.h"
#include "fio.h"
#include "fio_oasis.h"
#include "fio_cgd.h"
#include "si_parsenode.h"
// regex.h must come before si_handle.h to enable regex handle.
#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include "regex/regex.h"
#endif
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_daemon.h"
#include "si_lspec.h"
#include "main_scriptif.h"
#include "events.h"
#include "ghost.h"
#include "select.h"
#include "menu.h"
#include "promptline.h"
#include "pcell.h"
#include "errorlog.h"
#include "spnumber/spnumber.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "miscutil/tvals.h"
#include "miscutil/miscutil.h"
#ifdef HAVE_LOCAL_ALLOCATOR
#include "malloc/local_malloc.h"
#else
#include "miscutil/coresize.h"
#endif

#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#else
#ifdef HAVE_FTIME
#include <sys/timeb.h>
#endif
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#ifndef direct
#define direct dirent
#endif
#else
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#endif

#ifdef WIN32
#include <winsock2.h>
#include "miscutil/msw.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif


// The WIN32 defines are for the MINGW version of gcc running on Windows.

#ifdef WIN32
#define CLOSESOCKET(x) (shutdown(x, SD_SEND), closesocket(x))
#else
#define CLOSESOCKET(x) ::close(x)
#endif

////////////////////////////////////////////////////////////////////////
//
// Script Functions:  Utility and I/O Functions
//
////////////////////////////////////////////////////////////////////////

//
// Utility Functions
//
namespace {
    char *prstring(Variable*, bool, bool);
    stringlist *prlist(Variable*, bool);

    namespace misc2_funcs {
        // Arrays
        bool IFarrayDims(Variable*, Variable*, void*);
        bool IFarrayDimension(Variable*, Variable*, void*);
        bool IFgetDims(Variable*, Variable*, void*);
        bool IFdupArray(Variable*, Variable*, void*);
        bool IFsortArray(Variable*, Variable*, void*);

        // Bitwise Logic
        bool IFshiftBits(Variable*, Variable*, void*);
        bool IFandBits(Variable*, Variable*, void*);
        bool IForBits(Variable*, Variable*, void*);
        bool IFxorBits(Variable*, Variable*, void*);
        bool IFnotBits(Variable*, Variable*, void*);

        // Error Reporting
        bool IFgetError(Variable*, Variable*, void*);
        bool IFaddError(Variable*, Variable*, void*); // global!
        bool IFgetLogNumber(Variable*, Variable*, void*);
        bool IFgetLogMessage(Variable*, Variable*, void*);
        bool IFaddLogMessage(Variable*, Variable*, void*);

        // Generic Handle Functions
        bool IFnumHandles(Variable*, Variable*, void*);
        bool IFhandleContent(Variable*, Variable*, void*);
        bool IFhandleTruncate(Variable*, Variable*, void*);
        bool IFhandleNext(Variable*, Variable*, void*);
        bool IFhandleDup(Variable*, Variable*, void*);
        bool IFhandleDupNitems(Variable*, Variable*, void*);
        bool IFh(Variable*, Variable*, void*);
        bool IFhandleArray(Variable*, Variable*, void*);
        bool IFhandleCat(Variable*, Variable*, void*);
        bool IFhandleReverse(Variable*, Variable*, void*);
        bool IFhandlePurgeList(Variable*, Variable*, void*);
        bool IFclose(Variable*, Variable*, void*);
        bool IFcloseArray(Variable*, Variable*, void*);

        // Memory Management
        bool IFfreeArray(Variable*, Variable*, void*);
        bool IFcoreSize(Variable*, Variable*, void*);

        // Miscellaneous
        bool IFdefined(Variable*, Variable*, void*);
        bool IFtypeOf(Variable*, Variable*, void*);

        // Path Manipulation and Query
        bool IFpathToEnd(Variable*, Variable*, void*);
        bool IFpathToFront(Variable*, Variable*, void*);
        bool IFinPath(Variable*, Variable*, void*);
        bool IFremovePath(Variable*, Variable*, void*);

        // Regular Expressions
        bool IFregCompile(Variable*, Variable*, void*);
        bool IFregCompare(Variable*, Variable*, void*);
        bool IFregError(Variable*, Variable*, void*);

        // String List Handles
        bool IFstringHandle(Variable*, Variable*, void*);
        bool IFlistHandle(Variable*, Variable*, void*);
        bool IFlistContent(Variable*, Variable*, void*);
        bool IFlistReverse(Variable*, Variable*, void*);
        bool IFlistNext(Variable*, Variable*, void*);
        bool IFlistAddFront(Variable*, Variable*, void*);
        bool IFlistAddBack(Variable*, Variable*, void*);
        bool IFlistAlphaSort(Variable*, Variable*, void*);
        bool IFlistUnique(Variable*, Variable*, void*);
        bool IFlistFormatCols(Variable*, Variable*, void*);
        bool IFlistConcat(Variable*, Variable*, void*);
        bool IFlistIncluded(Variable*, Variable*, void*);

        // String Manipulation and Conversion
        bool IFstrcat(Variable*, Variable*, void*);
        bool IFstrcmp(Variable*, Variable*, void*);
        bool IFstrncmp(Variable*, Variable*, void*);
        bool IFstrcasecmp(Variable*, Variable*, void*);
        bool IFstrncasecmp(Variable*, Variable*, void*);
        bool IFstrdup(Variable*, Variable*, void*);
        bool IFstrtok(Variable*, Variable*, void*);
        bool IFstrchr(Variable*, Variable*, void*);
        bool IFstrrchr(Variable*, Variable*, void*);
        bool IFstrstr(Variable*, Variable*, void*);
        bool IFstrpath(Variable*, Variable*, void*);
        bool IFstrlen(Variable*, Variable*, void*);
        bool IFsizeof(Variable*, Variable*, void*);
        bool IFtoReal(Variable*, Variable*, void*);
        bool IFtoString(Variable*, Variable*, void*);
        bool IFtoStringA(Variable*, Variable*, void*);
        bool IFtoFormat(Variable*, Variable*, void*);
        bool IFtoChar(Variable*, Variable*, void*);

        // Current Directory
        bool IFcwd(Variable*, Variable*, void*);
        bool IFpwd(Variable*, Variable*, void*);

        // Date and Time
        bool IFdateString(Variable*, Variable*, void*);
        bool IFtime(Variable*, Variable*, void*);
        bool IFmakeTime(Variable*, Variable*, void*);
        bool IFtimeToString(Variable*, Variable*, void*);
        bool IFtimeToVals(Variable*, Variable*, void*);
        bool IFmilliSec(Variable*, Variable*, void*);
        bool IFstartTiming(Variable*, Variable*, void*);
        bool IFstopTiming(Variable*, Variable*, void*);

        // File System Interface
        bool IFglob(Variable*, Variable*, void*);
        bool IFopen(Variable*, Variable*, void*);
        bool IFpopen(Variable*, Variable*, void*);
        bool IFsopen(Variable*, Variable*, void*);
        bool IFreadLine(Variable*, Variable*, void*);
        bool IFreadChar(Variable*, Variable*, void*);
        bool IFwriteLine(Variable*, Variable*, void*);
        bool IFwriteChar(Variable*, Variable*, void*);
        bool IFtempFile(Variable*, Variable*, void*);
        bool IFlistDirectory(Variable*, Variable*, void*);
        bool IFmakeDir(Variable*, Variable*, void*);
        bool IFfileStat(Variable*, Variable*, void*);
        bool IFdeleteFile(Variable*, Variable*, void*);
        bool IFmoveFile(Variable*, Variable*, void*);
        bool IFcopyFile(Variable*, Variable*, void*);
        bool IFcreateBak(Variable*, Variable*, void*);
        bool IFmd5Digest(Variable*, Variable*, void*);

        // Socket and Xic Client/Server Interface
        bool IFreadData(Variable*, Variable*, void*);
        bool IFreadReply(Variable*, Variable*, void*);
        bool IFconvertReply(Variable*, Variable*, void*);
        bool IFwriteMsg(Variable*, Variable*, void*);

        // System Command Interface
        bool IFshell(Variable*, Variable*, void*);
        bool IFgetPID(Variable*, Variable*, void*);

        // Menu Buttons
        bool IFsetButtonStatus(Variable*, Variable*, void*);
        bool IFgetButtonStatus(Variable*, Variable*, void*);
        bool IFpressButton(Variable*, Variable*, void*);
        bool IFbtnDown(Variable*, Variable*, void*);
        bool IFbtnUp(Variable*, Variable*, void*);
        bool IFkeyDown(Variable*, Variable*, void*);
        bool IFkeyUp(Variable*, Variable*, void*);

        // Mouse Input
        bool IFpoint(Variable*, Variable*, void*);
        bool IFselection(Variable*, Variable*, void*);

        // Graphical Input
        bool IFpopUpInput(Variable*, Variable*, void*);
        bool IFpopUpAffirm(Variable*, Variable*, void*);
        bool IFpopUpNumeric(Variable*, Variable*, void*);

        // Text Input
        bool IFaskReal(Variable*, Variable*, void*);
        bool IFaskString(Variable*, Variable*, void*);
        bool IFaskConsoleReal(Variable*, Variable*, void*);
        bool IFaskConsoleString(Variable*, Variable*, void*);
        bool IFgetKey(Variable*, Variable*, void*);

        // Text Output
        bool IFsepString(Variable*, Variable*, void*);
        bool IFshowPrompt(Variable*, Variable*, void*);
        bool IFsetIndent(Variable*, Variable*, void*);
        bool IFsetPrintLimits(Variable*, Variable*, void*);
        bool IFprint(Variable*, Variable*, void*);
        bool IFprintLog(Variable*, Variable*, void*);
        bool IFprintString(Variable*, Variable*, void*);
        bool IFprintStringEsc(Variable*, Variable*, void*);
        bool IFmessage(Variable*, Variable*, void*);
        bool IFerrorMsg(Variable*, Variable*, void*);
        bool IFtextWindow(Variable*, Variable*, void*);

        // Polymorphic Database Visualization Functions
        // Part of the Polymorphic Flat Database function group in
        // funcs_lexpr.cc.
        bool IFshowDb(Variable*, Variable*, void*);
    }
    using namespace misc2_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    // Arrays
    PY_FUNC(ArrayDims,              2,  IFarrayDims);
    PY_FUNC(ArrayDimension,         2,  IFarrayDimension);
    PY_FUNC(GetDims,                2,  IFgetDims);
    PY_FUNC(DupArray,               2,  IFdupArray);
    PY_FUNC(SortArray,              4,  IFsortArray);

    // Bitwise Logic
    PY_FUNC(ShiftBits,              2,  IFshiftBits);
    PY_FUNC(AndBits,                2,  IFandBits);
    PY_FUNC(OrBits,                 2,  IForBits);
    PY_FUNC(XorBits,                2,  IFxorBits);
    PY_FUNC(NotBits,                1,  IFnotBits);

    // Error Reporting
    PY_FUNC(GetError,               0,  IFgetError);
    PY_FUNC(AddError,               1,  IFaddError);
    PY_FUNC(GetLogNumber,           0,  IFgetLogNumber);
    PY_FUNC(GetLogMessage,          1,  IFgetLogMessage);
    PY_FUNC(AddLogMessage,          2,  IFaddLogMessage);

    // Generic Handle Functions
    PY_FUNC(NumHandles,             0,  IFnumHandles);
    PY_FUNC(HandleContent,          1,  IFhandleContent);
    PY_FUNC(HandleTruncate,         2,  IFhandleTruncate);
    PY_FUNC(HandleNext,             1,  IFhandleNext);
    PY_FUNC(HandleDup,              1,  IFhandleDup);
    PY_FUNC(HandleDupNitems,        2,  IFhandleDupNitems);
    PY_FUNC(H,                      1,  IFh);
    PY_FUNC(HandleArray,            2,  IFhandleArray);
    PY_FUNC(HandleCat,              2,  IFhandleCat);
    PY_FUNC(HandleReverse,          1,  IFhandleReverse);
    PY_FUNC(HandlePurgeList,        2,  IFhandlePurgeList);
    PY_FUNC(Close,                  1,  IFclose);
    PY_FUNC(CloseArray,             2,  IFcloseArray);

    // Memory Management
    PY_FUNC(FreeArray,              1,  IFfreeArray);
    PY_FUNC(CoreSize,               0,  IFcoreSize);

    // Miscellaneous
    PY_FUNC(Defined,                1,  IFdefined);
    PY_FUNC(TypeOf,                 1,  IFtypeOf);

    // Path Manipulation and Query
    PY_FUNC(PathToEnd,              2,  IFpathToEnd);
    PY_FUNC(PathToFront,            2,  IFpathToFront);
    PY_FUNC(InPath,                 2,  IFinPath);
    PY_FUNC(RemovePath,             2,  IFremovePath);

    // Regular Expressions
    PY_FUNC(RegCompile,             2,  IFregCompile);
    PY_FUNC(RegCompare,             3,  IFregCompare);
    PY_FUNC(RegError,               1,  IFregError);

    // String List Handles
    PY_FUNC(StringHandle,           2,  IFstringHandle);
    PY_FUNC(ListHandle,       VARARGS,  IFlistHandle);
    PY_FUNC(ListContent,            1,  IFlistContent);
    PY_FUNC(ListReverse,            1,  IFlistReverse);
    PY_FUNC(ListNext,               1,  IFlistNext);
    PY_FUNC(ListAddFront,           2,  IFlistAddFront);
    PY_FUNC(ListAddBack,            2,  IFlistAddBack);
    PY_FUNC(ListAlphaSort,          1,  IFlistAlphaSort);
    PY_FUNC(ListUnique,             1,  IFlistUnique);
    PY_FUNC(ListFormatCols,         2,  IFlistFormatCols);
    PY_FUNC(ListConcat,             2,  IFlistConcat);
    PY_FUNC(ListIncluded,           2,  IFlistIncluded);

    // String Manipulation and Conversion
    PY_FUNC(Strcat,                 2,  IFstrcat);
    PY_FUNC(Strcmp,                 2,  IFstrcmp);
    PY_FUNC(Strncmp,                3,  IFstrncmp);
    PY_FUNC(Strcasecmp,             2,  IFstrcasecmp);
    PY_FUNC(Strncasecmp,            3,  IFstrncasecmp);
    PY_FUNC(Strdup,                 1,  IFstrdup);
    PY_FUNC(Strtok,                 2,  IFstrtok);
    PY_FUNC(Strchr,                 2,  IFstrchr);
    PY_FUNC(Strrchr,                2,  IFstrrchr);
    PY_FUNC(Strstr,                 2,  IFstrstr);
    PY_FUNC(Strpath,                1,  IFstrpath);
    PY_FUNC(Strlen,                 1,  IFstrlen);
    PY_FUNC(Sizeof,                 1,  IFsizeof);
    PY_FUNC(ToReal,                 1,  IFtoReal);
    PY_FUNC(ToString,               1,  IFtoString);
    PY_FUNC(ToStringA,              2,  IFtoStringA);
    PY_FUNC(ToFormat,         VARARGS,  IFtoFormat);
    PY_FUNC(ToChar,                 1,  IFtoChar);

    // Current Directory
    PY_FUNC(Cwd,                    1,  IFcwd);
    PY_FUNC(Pwd,                    0,  IFpwd);

    // Date and Time
    PY_FUNC(DateString,             0,  IFdateString);
    PY_FUNC(Time,                   0,  IFtime);
    PY_FUNC(MakeTime,               2,  IFmakeTime);
    PY_FUNC(TimeToString,           2,  IFtimeToString);
    PY_FUNC(TimeToVals,             3,  IFtimeToVals);
    PY_FUNC(MilliSec,               0,  IFmilliSec);
    PY_FUNC(StartTiming,            1,  IFstartTiming);
    PY_FUNC(StopTiming,             1,  IFstopTiming);

    // File System Interface
    PY_FUNC(Glob,                   1,  IFglob);
    PY_FUNC(Open,                   2,  IFopen);
    PY_FUNC(Popen,                  2,  IFpopen);
    PY_FUNC(Sopen,                  2,  IFsopen);
    PY_FUNC(ReadLine,               2,  IFreadLine);
    PY_FUNC(ReadChar,               1,  IFreadChar);
    PY_FUNC(WriteLine,              2,  IFwriteLine);
    PY_FUNC(WriteChar,              2,  IFwriteChar);
    PY_FUNC(TempFile,               1,  IFtempFile);
    PY_FUNC(ListDirectory,          2,  IFlistDirectory);
    PY_FUNC(MakeDir,                1,  IFmakeDir);
    PY_FUNC(FileStat,               2,  IFfileStat);
    PY_FUNC(DeleteFile,             1,  IFdeleteFile);
    PY_FUNC(MoveFile,               2,  IFmoveFile);
    PY_FUNC(CopyFile,               2,  IFcopyFile);
    PY_FUNC(CreateBak,              1,  IFcreateBak);
    PY_FUNC(Md5Digest,              1,  IFmd5Digest);

    // Xic Client/Server Interface
    PY_FUNC(ReadData,               2,  IFreadData);
    PY_FUNC(ReadReply,              2,  IFreadReply);
    PY_FUNC(ConvertReply,           2,  IFconvertReply);
    PY_FUNC(WriteMsg,               2,  IFwriteMsg);

    // System Command Interface
    PY_FUNC(Shell,                  1,  IFshell);
    PY_FUNC(GetPID,                 1,  IFgetPID);

    // Menu Buttons
    PY_FUNC(SetButtonStatus,        3,  IFsetButtonStatus);
    PY_FUNC(GetButtonStatus,        2,  IFgetButtonStatus);
    PY_FUNC(PressButton,            2,  IFpressButton);
    PY_FUNC(BtnDown,                5,  IFbtnDown);
    PY_FUNC(BtnUp,                  5,  IFbtnUp);
    PY_FUNC(KeyDown,                3,  IFkeyDown);
    PY_FUNC(KeyUp,                  3,  IFkeyUp);

    // Mouse Input
    PY_FUNC(Point,                  1,  IFpoint);
    PY_FUNC(Selection,              0,  IFselection);

    // Graphical Input
    PY_FUNC(PopUpInput,             4,  IFpopUpInput);
    PY_FUNC(PopUpAffirm,            1,  IFpopUpAffirm);
    PY_FUNC(PopUpNumeric,           6,  IFpopUpNumeric);

    // Text Input
    PY_FUNC(AskReal,                2,  IFaskReal);
    PY_FUNC(AskString,              2,  IFaskString);
    PY_FUNC(AskConsoleReal,         2,  IFaskConsoleReal);
    PY_FUNC(AskConsoleString,       2,  IFaskConsoleString);
    PY_FUNC(GetKey,                 0,  IFgetKey);

    // Text Output
    PY_FUNC(SepString,              2,  IFsepString);
    PY_FUNC(ShowPrompt,       VARARGS,  IFshowPrompt);
    PY_FUNC(SetIndent,              1,  IFsetIndent);
    PY_FUNC(SetPrintLimits,         2,  IFsetPrintLimits);
    PY_FUNC(Print,            VARARGS,  IFprint);
    PY_FUNC(PrintLog,         VARARGS,  IFprintLog);
    PY_FUNC(PrintString,      VARARGS,  IFprintString);
    PY_FUNC(PrintStringEsc,   VARARGS,  IFprintStringEsc);
    PY_FUNC(Message,          VARARGS,  IFmessage);
    PY_FUNC(ErrorMsg,         VARARGS,  IFerrorMsg);
    PY_FUNC(TextWindow,             2,  IFtextWindow);

    // Database Visualization Functions (export)
    PY_FUNC(ShowDb,                 2,  IFshowDb);


    void py_register_misc2()
    {
      // Arrays
      cPyIf::register_func("ArrayDims",              pyArrayDims);
      cPyIf::register_func("ArrayDimension",         pyArrayDimension);
      cPyIf::register_func("GetDims",                pyGetDims);
      cPyIf::register_func("DupArray",               pyDupArray);
      cPyIf::register_func("SortArray",              pySortArray);

      // Bitwise Logic
      cPyIf::register_func("ShiftBits",              pyShiftBits);
      cPyIf::register_func("AndBits",                pyAndBits);
      cPyIf::register_func("OrBits",                 pyOrBits);
      cPyIf::register_func("XorBits",                pyXorBits);
      cPyIf::register_func("NotBits",                pyNotBits);

      // Error Reporting
      cPyIf::register_func("GetError",               pyGetError);
      cPyIf::register_func("AddError",               pyAddError);
      cPyIf::register_func("GetLogNumber",           pyGetLogNumber);
      cPyIf::register_func("GetLogMessage",          pyGetLogMessage);
      cPyIf::register_func("AddLogMessage",          pyAddLogMessage);

      // Generic Handle Functions
      cPyIf::register_func("NumHandles",             pyNumHandles);
      cPyIf::register_func("HandleContent",          pyHandleContent);
      cPyIf::register_func("HandleTruncate",         pyHandleTruncate);
      cPyIf::register_func("HandleNext",             pyHandleNext);
      cPyIf::register_func("HandleDup",              pyHandleDup);
      cPyIf::register_func("HandleDupNitems",        pyHandleDupNitems);
      cPyIf::register_func("H",                      pyH);
      cPyIf::register_func("HandleArray",            pyHandleArray);
      cPyIf::register_func("HandleCat",              pyHandleCat);
      cPyIf::register_func("HandleReverse",          pyHandleReverse);
      cPyIf::register_func("HandlePurgeList",        pyHandlePurgeList);
      cPyIf::register_func("Close",                  pyClose);
      cPyIf::register_func("CloseArray",             pyCloseArray);

      // Memory Management
      cPyIf::register_func("FreeArray",              pyFreeArray);
      cPyIf::register_func("CoreSize",               pyCoreSize);

      // Miscellaneous
      cPyIf::register_func("Defined",                pyDefined);
      cPyIf::register_func("TypeOf",                 pyTypeOf);

      // Path Manipulation and Query
      cPyIf::register_func("PathToEnd",              pyPathToEnd);
      cPyIf::register_func("PathToFront",            pyPathToFront);
      cPyIf::register_func("InPath",                 pyInPath);
      cPyIf::register_func("RemovePath",             pyRemovePath);

      // Regular Expressions
      cPyIf::register_func("RegCompile",             pyRegCompile);
      cPyIf::register_func("RegCompare",             pyRegCompare);
      cPyIf::register_func("RegError",               pyRegError);

      // String List Handles
      cPyIf::register_func("StringHandle",           pyStringHandle);
      cPyIf::register_func("ListHandle",             pyListHandle);
      cPyIf::register_func("ListContent",            pyListContent);
      cPyIf::register_func("ListReverse",            pyListReverse);
      cPyIf::register_func("ListNext",               pyListNext);
      cPyIf::register_func("ListAddFront",           pyListAddFront);
      cPyIf::register_func("ListAddBack",            pyListAddBack);
      cPyIf::register_func("ListAlphaSort",          pyListAlphaSort);
      cPyIf::register_func("ListUnique",             pyListUnique);
      cPyIf::register_func("ListFormatCols",         pyListFormatCols);
      cPyIf::register_func("ListConcat",             pyListConcat);
      cPyIf::register_func("ListIncluded",           pyListIncluded);

      // String Manipulation and Conversion
      cPyIf::register_func("Strcat",                 pyStrcat);
      cPyIf::register_func("Strcmp",                 pyStrcmp);
      cPyIf::register_func("Strncmp",                pyStrncmp);
      cPyIf::register_func("Strcasecmp",             pyStrcasecmp);
      cPyIf::register_func("Strncasecmp",            pyStrncasecmp);
      cPyIf::register_func("Strdup",                 pyStrdup);
      cPyIf::register_func("Strtok",                 pyStrtok);
      cPyIf::register_func("Strchr",                 pyStrchr);
      cPyIf::register_func("Strrchr",                pyStrrchr);
      cPyIf::register_func("Strstr",                 pyStrstr);
      cPyIf::register_func("Strpath",                pyStrpath);
      cPyIf::register_func("Strlen",                 pyStrlen);
      cPyIf::register_func("Sizeof",                 pySizeof);
      cPyIf::register_func("ToReal",                 pyToReal);
      cPyIf::register_func("ToString",               pyToString);
      cPyIf::register_func("ToStringA",              pyToStringA);
      cPyIf::register_func("ToFormat",               pyToFormat);
      cPyIf::register_func("ToChar",                 pyToChar);

      // Current Directory
      cPyIf::register_func("Cwd",                    pyCwd);
      cPyIf::register_func("Pwd",                    pyPwd);

      // Date and Time
      cPyIf::register_func("DateString",             pyDateString);
      cPyIf::register_func("Time",                   pyTime);
      cPyIf::register_func("MakeTime",               pyMakeTime);
      cPyIf::register_func("TimeToString",           pyTimeToString);
      cPyIf::register_func("TimeToVals",             pyTimeToVals);
      cPyIf::register_func("MilliSec",               pyMilliSec);
      cPyIf::register_func("StartTiming",            pyStartTiming);
      cPyIf::register_func("StopTiming",             pyStopTiming);

      // File System Interface
      cPyIf::register_func("Glob",                   pyGlob);
      cPyIf::register_func("Open",                   pyOpen);
      cPyIf::register_func("Popen",                  pyPopen);
      cPyIf::register_func("Sopen",                  pySopen);
      cPyIf::register_func("ReadLine",               pyReadLine);
      cPyIf::register_func("ReadChar",               pyReadChar);
      cPyIf::register_func("WriteLine",              pyWriteLine);
      cPyIf::register_func("WriteChar",              pyWriteChar);
      cPyIf::register_func("TempFile",               pyTempFile);
      cPyIf::register_func("ListDirectory",          pyListDirectory);
      cPyIf::register_func("MakeDir",                pyMakeDir);
      cPyIf::register_func("FileStat",               pyFileStat);
      cPyIf::register_func("DeleteFile",             pyDeleteFile);
      cPyIf::register_func("MoveFile",               pyMoveFile);
      cPyIf::register_func("CopyFile",               pyCopyFile);
      cPyIf::register_func("CreateBak",              pyCreateBak);
      cPyIf::register_func("Md5Digest",              pyMd5Digest);

      // Xic Client/Server Interface
      cPyIf::register_func("ReadData",               pyReadData);
      cPyIf::register_func("ReadReply",              pyReadReply);
      cPyIf::register_func("ConvertReply",           pyConvertReply);
      cPyIf::register_func("WriteMsg",               pyWriteMsg);

      // System Command Interface
      cPyIf::register_func("Shell",                  pyShell);
      cPyIf::register_func("System",                 pyShell);  // alias
      cPyIf::register_func("GetPID",                 pyGetPID);

      // Menu Buttons
      cPyIf::register_func("SetButtonStatus",        pySetButtonStatus);
      cPyIf::register_func("GetButtonStatus",        pyGetButtonStatus);
      cPyIf::register_func("PressButton",            pyPressButton);
      cPyIf::register_func("BtnDown",                pyBtnDown);
      cPyIf::register_func("BtnUp",                  pyBtnUp);
      cPyIf::register_func("KeyDown",                pyKeyDown);
      cPyIf::register_func("KeyUp",                  pyKeyUp);

      // Mouse Input
      cPyIf::register_func("Point",                  pyPoint);
      cPyIf::register_func("Selection",              pySelection);

      // Graphical Input
      cPyIf::register_func("PopUpInput",             pyPopUpInput);
      cPyIf::register_func("PopUpAffirm",            pyPopUpAffirm);
      cPyIf::register_func("PopUpNumeric",           pyPopUpNumeric);

      // Text Input
      cPyIf::register_func("AskReal",                pyAskReal);
      cPyIf::register_func("AskString",              pyAskString);
      cPyIf::register_func("AskConsoleReal",         pyAskConsoleReal);
      cPyIf::register_func("AskConsoleString",       pyAskConsoleString);
      cPyIf::register_func("GetKey",                 pyGetKey);

      // Text Output
      cPyIf::register_func("SepString",              pySepString);
      cPyIf::register_func("ShowPrompt",             pyShowPrompt);
      cPyIf::register_func("SetIndent",              pySetIndent);
      cPyIf::register_func("SetPrintLimits",         pySetPrintLimits);
      cPyIf::register_func("Print",                  pyPrint);
      cPyIf::register_func("PrintLog",               pyPrintLog);
      cPyIf::register_func("PrintString",            pyPrintString);
      cPyIf::register_func("PrintStringEsc",         pyPrintStringEsc);
      cPyIf::register_func("Message",                pyMessage);
      cPyIf::register_func("ErrorMsg",               pyErrorMsg);
      cPyIf::register_func("TextWindow",             pyTextWindow);

      // Database Visualization Functions (export)
      cPyIf::register_func("ShowDb",                 pyShowDb);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // Tcl/Tk wrappers.

    // Arrays
    TCL_FUNC(ArrayDims,              2,  IFarrayDims);
    TCL_FUNC(ArrayDimension,         2,  IFarrayDimension);
    TCL_FUNC(GetDims,                2,  IFgetDims);
    TCL_FUNC(DupArray,               2,  IFdupArray);
    TCL_FUNC(SortArray,              4,  IFsortArray);

    // Bitwise Logic
    TCL_FUNC(ShiftBits,              2,  IFshiftBits);
    TCL_FUNC(AndBits,                2,  IFandBits);
    TCL_FUNC(OrBits,                 2,  IForBits);
    TCL_FUNC(XorBits,                2,  IFxorBits);
    TCL_FUNC(NotBits,                1,  IFnotBits);

    // Error Reporting
    TCL_FUNC(GetError,               0,  IFgetError);
    TCL_FUNC(AddError,               1,  IFaddError);
    TCL_FUNC(GetLogNumber,           0,  IFgetLogNumber);
    TCL_FUNC(GetLogMessage,          1,  IFgetLogMessage);
    TCL_FUNC(AddLogMessage,          2,  IFaddLogMessage);

    // Generic Handle Functions
    TCL_FUNC(NumHandles,             0,  IFnumHandles);
    TCL_FUNC(HandleContent,          1,  IFhandleContent);
    TCL_FUNC(HandleTruncate,         2,  IFhandleTruncate);
    TCL_FUNC(HandleNext,             1,  IFhandleNext);
    TCL_FUNC(HandleDup,              1,  IFhandleDup);
    TCL_FUNC(HandleDupNitems,        2,  IFhandleDupNitems);
    TCL_FUNC(H,                      1,  IFh);
    TCL_FUNC(HandleArray,            2,  IFhandleArray);
    TCL_FUNC(HandleCat,              2,  IFhandleCat);
    TCL_FUNC(HandleReverse,          1,  IFhandleReverse);
    TCL_FUNC(HandlePurgeList,        2,  IFhandlePurgeList);
    TCL_FUNC(Close,                  1,  IFclose);
    TCL_FUNC(CloseArray,             2,  IFcloseArray);

    // Memory Management
    TCL_FUNC(FreeArray,              1,  IFfreeArray);
    TCL_FUNC(CoreSize,               0,  IFcoreSize);

    // Miscellaneous
    TCL_FUNC(Defined,                1,  IFdefined);
    TCL_FUNC(TypeOf,                 1,  IFtypeOf);

    // Path Manipulation and Query
    TCL_FUNC(PathToEnd,              2,  IFpathToEnd);
    TCL_FUNC(PathToFront,            2,  IFpathToFront);
    TCL_FUNC(InPath,                 2,  IFinPath);
    TCL_FUNC(RemovePath,             2,  IFremovePath);

    // Regular Expressions
    TCL_FUNC(RegCompile,             2,  IFregCompile);
    TCL_FUNC(RegCompare,             3,  IFregCompare);
    TCL_FUNC(RegError,               1,  IFregError);

    // String List Handles
    TCL_FUNC(StringHandle,           2,  IFstringHandle);
    TCL_FUNC(ListHandle,       VARARGS,  IFlistHandle);
    TCL_FUNC(ListContent,            1,  IFlistContent);
    TCL_FUNC(ListReverse,            1,  IFlistReverse);
    TCL_FUNC(ListNext,               1,  IFlistNext);
    TCL_FUNC(ListAddFront,           2,  IFlistAddFront);
    TCL_FUNC(ListAddBack,            2,  IFlistAddBack);
    TCL_FUNC(ListAlphaSort,          1,  IFlistAlphaSort);
    TCL_FUNC(ListUnique,             1,  IFlistUnique);
    TCL_FUNC(ListFormatCols,         2,  IFlistFormatCols);
    TCL_FUNC(ListConcat,             2,  IFlistConcat);
    TCL_FUNC(ListIncluded,           2,  IFlistIncluded);

    // String Manipulation and Conversion
    TCL_FUNC(Strcat,                 2,  IFstrcat);
    TCL_FUNC(Strcmp,                 2,  IFstrcmp);
    TCL_FUNC(Strncmp,                3,  IFstrncmp);
    TCL_FUNC(Strcasecmp,             2,  IFstrcasecmp);
    TCL_FUNC(Strncasecmp,            3,  IFstrncasecmp);
    TCL_FUNC(Strdup,                 1,  IFstrdup);
    TCL_FUNC(Strtok,                 2,  IFstrtok);
    TCL_FUNC(Strchr,                 2,  IFstrchr);
    TCL_FUNC(Strrchr,                2,  IFstrrchr);
    TCL_FUNC(Strstr,                 2,  IFstrstr);
    TCL_FUNC(Strpath,                1,  IFstrpath);
    TCL_FUNC(Strlen,                 1,  IFstrlen);
    TCL_FUNC(Sizeof,                 1,  IFsizeof);
    TCL_FUNC(ToReal,                 1,  IFtoReal);
    TCL_FUNC(ToString,               1,  IFtoString);
    TCL_FUNC(ToStringA,              2,  IFtoStringA);
    TCL_FUNC(ToFormat,         VARARGS,  IFtoFormat);
    TCL_FUNC(ToChar,                 1,  IFtoChar);

    // Current Directory
    TCL_FUNC(Cwd,                    1,  IFcwd);
    TCL_FUNC(Pwd,                    0,  IFpwd);

    // Date and Time
    TCL_FUNC(DateString,             0,  IFdateString);
    TCL_FUNC(Time,                   0,  IFtime);
    TCL_FUNC(MakeTime,               2,  IFmakeTime);
    TCL_FUNC(TimeToString,           2,  IFtimeToString);
    TCL_FUNC(TimeToVals,             3,  IFtimeToVals);
    TCL_FUNC(MilliSec,               0,  IFmilliSec);
    TCL_FUNC(StartTiming,            1,  IFstartTiming);
    TCL_FUNC(StopTiming,             1,  IFstopTiming);

    // File System Interface
    TCL_FUNC(Glob,                   1,  IFglob);
    TCL_FUNC(Open,                   2,  IFopen);
    TCL_FUNC(Popen,                  2,  IFpopen);
    TCL_FUNC(Sopen,                  2,  IFsopen);
    TCL_FUNC(ReadLine,               2,  IFreadLine);
    TCL_FUNC(ReadChar,               1,  IFreadChar);
    TCL_FUNC(WriteLine,              2,  IFwriteLine);
    TCL_FUNC(WriteChar,              2,  IFwriteChar);
    TCL_FUNC(TempFile,               1,  IFtempFile);
    TCL_FUNC(ListDirectory,          2,  IFlistDirectory);
    TCL_FUNC(MakeDir,                1,  IFmakeDir);
    TCL_FUNC(FileStat,               2,  IFfileStat);
    TCL_FUNC(DeleteFile,             1,  IFdeleteFile);
    TCL_FUNC(MoveFile,               2,  IFmoveFile);
    TCL_FUNC(CopyFile,               2,  IFcopyFile);
    TCL_FUNC(CreateBak,              1,  IFcreateBak);
    TCL_FUNC(Md5Digest,              1,  IFmd5Digest);

    // Xic Client/Server Interface
    TCL_FUNC(ReadData,               2,  IFreadData);
    TCL_FUNC(ReadReply,              2,  IFreadReply);
    TCL_FUNC(ConvertReply,           2,  IFconvertReply);
    TCL_FUNC(WriteMsg,               2,  IFwriteMsg);

    // System Command Interface
    TCL_FUNC(Shell,                  1,  IFshell);
    TCL_FUNC(GetPID,                 1,  IFgetPID);

    // Menu Buttons
    TCL_FUNC(SetButtonStatus,        3,  IFsetButtonStatus);
    TCL_FUNC(GetButtonStatus,        2,  IFgetButtonStatus);
    TCL_FUNC(PressButton,            2,  IFpressButton);
    TCL_FUNC(BtnDown,                5,  IFbtnDown);
    TCL_FUNC(BtnUp,                  5,  IFbtnUp);
    TCL_FUNC(KeyDown,                3,  IFkeyDown);
    TCL_FUNC(KeyUp,                  3,  IFkeyUp);

    // Mouse Input
    TCL_FUNC(Point,                  1,  IFpoint);
    TCL_FUNC(Selection,              0,  IFselection);

    // Graphical Input
    TCL_FUNC(PopUpInput,             4,  IFpopUpInput);
    TCL_FUNC(PopUpAffirm,            1,  IFpopUpAffirm);
    TCL_FUNC(PopUpNumeric,           6,  IFpopUpNumeric);

    // Text Input
    TCL_FUNC(AskReal,                2,  IFaskReal);
    TCL_FUNC(AskString,              2,  IFaskString);
    TCL_FUNC(AskConsoleReal,         2,  IFaskConsoleReal);
    TCL_FUNC(AskConsoleString,       2,  IFaskConsoleString);
    TCL_FUNC(GetKey,                 0,  IFgetKey);

    // Text Output
    TCL_FUNC(SepString,              2,  IFsepString);
    TCL_FUNC(ShowPrompt,       VARARGS,  IFshowPrompt);
    TCL_FUNC(SetIndent,              1,  IFsetIndent);
    TCL_FUNC(SetPrintLimits,         2,  IFsetPrintLimits);
    TCL_FUNC(Print,            VARARGS,  IFprint);
    TCL_FUNC(PrintLog,         VARARGS,  IFprintLog);
    TCL_FUNC(PrintString,      VARARGS,  IFprintString);
    TCL_FUNC(PrintStringEsc,   VARARGS,  IFprintStringEsc);
    TCL_FUNC(Message,          VARARGS,  IFmessage);
    TCL_FUNC(ErrorMsg,         VARARGS,  IFerrorMsg);
    TCL_FUNC(TextWindow,             2,  IFtextWindow);

    // Database Visualization Functions (export)
    TCL_FUNC(ShowDb,                 2,  IFshowDb);


    void tcl_register_misc2()
    {
      // Arrays
      cTclIf::register_func("ArrayDims",              tclArrayDims);
      cTclIf::register_func("ArrayDimension",         tclArrayDimension);
      cTclIf::register_func("GetDims",                tclGetDims);
      cTclIf::register_func("DupArray",               tclDupArray);
      cTclIf::register_func("SortArray",              tclSortArray);

      // Bitwise Logic
      cTclIf::register_func("ShiftBits",              tclShiftBits);
      cTclIf::register_func("AndBits",                tclAndBits);
      cTclIf::register_func("OrBits",                 tclOrBits);
      cTclIf::register_func("XorBits",                tclXorBits);
      cTclIf::register_func("NotBits",                tclNotBits);

      // Error Reporting
      cTclIf::register_func("GetError",               tclGetError);
      cTclIf::register_func("AddError",               tclAddError);
      cTclIf::register_func("GetLogNumber",           tclGetLogNumber);
      cTclIf::register_func("GetLogMessage",          tclGetLogMessage);
      cTclIf::register_func("AddLogMessage",          tclAddLogMessage);

      // Generic Handle Functions
      cTclIf::register_func("NumHandles",             tclNumHandles);
      cTclIf::register_func("HandleContent",          tclHandleContent);
      cTclIf::register_func("HandleTruncate",         tclHandleTruncate);
      cTclIf::register_func("HandleNext",             tclHandleNext);
      cTclIf::register_func("HandleDup",              tclHandleDup);
      cTclIf::register_func("HandleDupNitems",        tclHandleDupNitems);
      cTclIf::register_func("H",                      tclH);
      cTclIf::register_func("HandleArray",            tclHandleArray);
      cTclIf::register_func("HandleCat",              tclHandleCat);
      cTclIf::register_func("HandleReverse",          tclHandleReverse);
      cTclIf::register_func("HandlePurgeList",        tclHandlePurgeList);
      cTclIf::register_func("Close",                  tclClose);
      cTclIf::register_func("CloseArray",             tclCloseArray);

      // Memory Management
      cTclIf::register_func("FreeArray",              tclFreeArray);
      cTclIf::register_func("CoreSize",               tclCoreSize);

      // Miscellaneous
      cTclIf::register_func("Defined",                tclDefined);
      cTclIf::register_func("TypeOf",                 tclTypeOf);

      // Path Manipulation and Query
      cTclIf::register_func("PathToEnd",              tclPathToEnd);
      cTclIf::register_func("PathToFront",            tclPathToFront);
      cTclIf::register_func("InPath",                 tclInPath);
      cTclIf::register_func("RemovePath",             tclRemovePath);

      // Regular Expressions
      cTclIf::register_func("RegCompile",             tclRegCompile);
      cTclIf::register_func("RegCompare",             tclRegCompare);
      cTclIf::register_func("RegError",               tclRegError);

      // String List Handles
      cTclIf::register_func("StringHandle",           tclStringHandle);
      cTclIf::register_func("ListHandle",             tclListHandle);
      cTclIf::register_func("ListContent",            tclListContent);
      cTclIf::register_func("ListReverse",            tclListReverse);
      cTclIf::register_func("ListNext",               tclListNext);
      cTclIf::register_func("ListAddFront",           tclListAddFront);
      cTclIf::register_func("ListAddBack",            tclListAddBack);
      cTclIf::register_func("ListAlphaSort",          tclListAlphaSort);
      cTclIf::register_func("ListUnique",             tclListUnique);
      cTclIf::register_func("ListFormatCols",         tclListFormatCols);
      cTclIf::register_func("ListConcat",             tclListConcat);
      cTclIf::register_func("ListIncluded",           tclListIncluded);

      // String Manipulation and Conversion
      cTclIf::register_func("Strcat",                 tclStrcat);
      cTclIf::register_func("Strcmp",                 tclStrcmp);
      cTclIf::register_func("Strncmp",                tclStrncmp);
      cTclIf::register_func("Strcasecmp",             tclStrcasecmp);
      cTclIf::register_func("Strncasecmp",            tclStrncasecmp);
      cTclIf::register_func("Strdup",                 tclStrdup);
      cTclIf::register_func("Strtok",                 tclStrtok);
      cTclIf::register_func("Strchr",                 tclStrchr);
      cTclIf::register_func("Strrchr",                tclStrrchr);
      cTclIf::register_func("Strstr",                 tclStrstr);
      cTclIf::register_func("Strpath",                tclStrpath);
      cTclIf::register_func("Strlen",                 tclStrlen);
      cTclIf::register_func("Sizeof",                 tclSizeof);
      cTclIf::register_func("ToReal",                 tclToReal);
      cTclIf::register_func("ToString",               tclToString);
      cTclIf::register_func("ToStringA",              tclToStringA);
      cTclIf::register_func("ToFormat",               tclToFormat);
      cTclIf::register_func("ToChar",                 tclToChar);

      // Current Directory
      cTclIf::register_func("Cwd",                    tclCwd);
      cTclIf::register_func("Pwd",                    tclPwd);

      // Date and Time
      cTclIf::register_func("DateString",             tclDateString);
      cTclIf::register_func("Time",                   tclTime);
      cTclIf::register_func("MakeTime",               tclMakeTime);
      cTclIf::register_func("TimeToString",           tclTimeToString);
      cTclIf::register_func("TimeToVals",             tclTimeToVals);
      cTclIf::register_func("MilliSec",               tclMilliSec);
      cTclIf::register_func("StartTiming",            tclStartTiming);
      cTclIf::register_func("StopTiming",             tclStopTiming);

      // File System Interface
      cTclIf::register_func("Glob",                   tclGlob);
      cTclIf::register_func("Open",                   tclOpen);
      cTclIf::register_func("Popen",                  tclPopen);
      cTclIf::register_func("Sopen",                  tclSopen);
      cTclIf::register_func("ReadLine",               tclReadLine);
      cTclIf::register_func("ReadChar",               tclReadChar);
      cTclIf::register_func("WriteLine",              tclWriteLine);
      cTclIf::register_func("WriteChar",              tclWriteChar);
      cTclIf::register_func("TempFile",               tclTempFile);
      cTclIf::register_func("ListDirectory",          tclListDirectory);
      cTclIf::register_func("MakeDir",                tclMakeDir);
      cTclIf::register_func("FileStat",               tclFileStat);
      cTclIf::register_func("DeleteFile",             tclDeleteFile);
      cTclIf::register_func("MoveFile",               tclMoveFile);
      cTclIf::register_func("CopyFile",               tclCopyFile);
      cTclIf::register_func("CreateBak",              tclCreateBak);
      cTclIf::register_func("Md5Digest",              tclMd5Digest);

      // Xic Client/Server Interface
      cTclIf::register_func("ReadData",               tclReadData);
      cTclIf::register_func("ReadReply",              tclReadReply);
      cTclIf::register_func("ConvertReply",           tclConvertReply);
      cTclIf::register_func("WriteMsg",               tclWriteMsg);

      // System Command Interface
      cTclIf::register_func("Shell",                  tclShell);
      cTclIf::register_func("System",                 tclShell);  // alias
      cTclIf::register_func("GetPID",                 tclGetPID);

      // Menu Buttons
      cTclIf::register_func("SetButtonStatus",        tclSetButtonStatus);
      cTclIf::register_func("GetButtonStatus",        tclGetButtonStatus);
      cTclIf::register_func("PressButton",            tclPressButton);
      cTclIf::register_func("BtnDown",                tclBtnDown);
      cTclIf::register_func("BtnUp",                  tclBtnUp);
      cTclIf::register_func("KeyDown",                tclKeyDown);
      cTclIf::register_func("KeyUp",                  tclKeyUp);

      // Mouse Input
      cTclIf::register_func("Point",                  tclPoint);
      cTclIf::register_func("Selection",              tclSelection);

      // Graphical Input
      cTclIf::register_func("PopUpInput",             tclPopUpInput);
      cTclIf::register_func("PopUpAffirm",            tclPopUpAffirm);
      cTclIf::register_func("PopUpNumeric",           tclPopUpNumeric);

      // Text Input
      cTclIf::register_func("AskReal",                tclAskReal);
      cTclIf::register_func("AskString",              tclAskString);
      cTclIf::register_func("AskConsoleReal",         tclAskConsoleReal);
      cTclIf::register_func("AskConsoleString",       tclAskConsoleString);
      cTclIf::register_func("GetKey",                 tclGetKey);

      // Text Output
      cTclIf::register_func("SepString",              tclSepString);
      cTclIf::register_func("ShowPrompt",             tclShowPrompt);
      cTclIf::register_func("SetIndent",              tclSetIndent);
      cTclIf::register_func("SetPrintLimits",         tclSetPrintLimits);
      cTclIf::register_func("Print",                  tclPrint);
      cTclIf::register_func("PrintLog",               tclPrintLog);
      cTclIf::register_func("PrintString",            tclPrintString);
      cTclIf::register_func("PrintStringEsc",         tclPrintStringEsc);
      cTclIf::register_func("Message",                tclMessage);
      cTclIf::register_func("ErrorMsg",               tclErrorMsg);
      cTclIf::register_func("TextWindow",             tclTextWindow);

      // Database Visualization Functions (export)
      cTclIf::register_func("ShowDb",                 tclShowDb);
    }
#endif  // HAVE_PYTHON
}


// Export to load functions in this script library.
//
void
cMain::load_funcs_misc2()
{
  using namespace misc2_funcs;

  // Arrays
  SIparse()->registerFunc("ArrayDims",              2,  IFarrayDims);
  SIparse()->registerFunc("ArrayDimension",         2,  IFarrayDimension);
  SIparse()->registerFunc("GetDims",                2,  IFgetDims);
  SIparse()->registerFunc("DupArray",               2,  IFdupArray);
  SIparse()->registerFunc("SortArray",              4,  IFsortArray);

  // Bitwise Logic
  SIparse()->registerFunc("ShiftBits",              2,  IFshiftBits);
  SIparse()->registerFunc("AndBits",                2,  IFandBits);
  SIparse()->registerFunc("OrBits",                 2,  IForBits);
  SIparse()->registerFunc("XorBits",                2,  IFxorBits);
  SIparse()->registerFunc("NotBits",                1,  IFnotBits);

  // Error Reporting
  SIparse()->registerFunc("GetError",               0,  IFgetError);
  SIparse()->registerFunc("AddError",               1,  IFaddError);

  // IFaddError is the only script function that does not reinit
  // the error message queue when called.
  SIparse()->setSuppressErrInitFunc(IFaddError);

  SIparse()->registerFunc("GetLogNumber",           0,  IFgetLogNumber);
  SIparse()->registerFunc("GetLogMessage",          1,  IFgetLogMessage);
  SIparse()->registerFunc("AddLogMessage",          2,  IFaddLogMessage);

  // Generic Handle Functions
  SIparse()->registerFunc("NumHandles",             0,  IFnumHandles);
  SIparse()->registerFunc("HandleContent",          1,  IFhandleContent);
  SIparse()->registerFunc("HandleTruncate",         2,  IFhandleTruncate);
  SIparse()->registerFunc("HandleNext",             1,  IFhandleNext);
  SIparse()->registerFunc("HandleDup",              1,  IFhandleDup);
  SIparse()->registerFunc("HandleDupNitems",        2,  IFhandleDupNitems);
  SIparse()->registerFunc("H",                      1,  IFh);
  SIparse()->registerFunc("HandleArray",            2,  IFhandleArray);
  SIparse()->registerFunc("HandleCat",              2,  IFhandleCat);
  SIparse()->registerFunc("HandleReverse",          1,  IFhandleReverse);
  SIparse()->registerFunc("HandlePurgeList",        2,  IFhandlePurgeList);
  SIparse()->registerFunc("Close",                  1,  IFclose);
  SIparse()->registerFunc("CloseArray",             2,  IFcloseArray);

  // Memory Management
  SIparse()->registerFunc("FreeArray",              1,  IFfreeArray);
  SIparse()->registerFunc("CoreSize",               0,  IFcoreSize);

  // Miscellaneous
  SIparse()->registerFunc("Defined",                1,  IFdefined);
  SIparse()->registerFunc("TypeOf",                 1,  IFtypeOf);

  // Path Manipulation and Query
  SIparse()->registerFunc("PathToEnd",              2,  IFpathToEnd);
  SIparse()->registerFunc("PathToFront",            2,  IFpathToFront);
  SIparse()->registerFunc("InPath",                 2,  IFinPath);
  SIparse()->registerFunc("RemovePath",             2,  IFremovePath);

  // Regular Expressions
  SIparse()->registerFunc("RegCompile",             2,  IFregCompile);
  SIparse()->registerFunc("RegCompare",             3,  IFregCompare);
  SIparse()->registerFunc("RegError",               1,  IFregError);

  // String List Handles
  SIparse()->registerFunc("StringHandle",           2,  IFstringHandle);
  SIparse()->registerFunc("ListHandle",       VARARGS,  IFlistHandle);
  SIparse()->registerFunc("ListContent",            1,  IFlistContent);
  SIparse()->registerFunc("ListReverse",            1,  IFlistReverse);
  SIparse()->registerFunc("ListNext",               1,  IFlistNext);
  SIparse()->registerFunc("ListAddFront",           2,  IFlistAddFront);
  SIparse()->registerFunc("ListAddBack",            2,  IFlistAddBack);
  SIparse()->registerFunc("ListAlphaSort",          1,  IFlistAlphaSort);
  SIparse()->registerFunc("ListUnique",             1,  IFlistUnique);
  SIparse()->registerFunc("ListFormatCols",         2,  IFlistFormatCols);
  SIparse()->registerFunc("ListConcat",             2,  IFlistConcat);
  SIparse()->registerFunc("ListIncluded",           2,  IFlistIncluded);

  // String Manipulation and Conversion
  SIparse()->registerFunc("Strcat",                 2,  IFstrcat);
  SIparse()->registerFunc("Strcmp",                 2,  IFstrcmp);
  SIparse()->registerFunc("Strncmp",                3,  IFstrncmp);
  SIparse()->registerFunc("Strcasecmp",             2,  IFstrcasecmp);
  SIparse()->registerFunc("Strncasecmp",            3,  IFstrncasecmp);
  SIparse()->registerFunc("Strdup",                 1,  IFstrdup);
  SIparse()->registerFunc("Strtok",                 2,  IFstrtok);
  SIparse()->registerFunc("Strchr",                 2,  IFstrchr);
  SIparse()->registerFunc("Strrchr",                2,  IFstrrchr);
  SIparse()->registerFunc("Strstr",                 2,  IFstrstr);
  SIparse()->registerFunc("Strpath",                1,  IFstrpath);
  SIparse()->registerFunc("Strlen",                 1,  IFstrlen);
  SIparse()->registerFunc("Sizeof",                 1,  IFsizeof);
  SIparse()->registerFunc("ToReal",                 1,  IFtoReal);
  SIparse()->registerFunc("ToString",               1,  IFtoString);
  SIparse()->registerFunc("ToStringA",              2,  IFtoStringA);
  SIparse()->registerFunc("ToFormat",         VARARGS,  IFtoFormat);
  SIparse()->registerFunc("ToChar",                 1,  IFtoChar);

  // Current Directory
  SIparse()->registerFunc("Cwd",                    1,  IFcwd);
  SIparse()->registerFunc("Pwd",                    0,  IFpwd);

  // Date and Time
  SIparse()->registerFunc("DateString",             0,  IFdateString);
  SIparse()->registerFunc("Time",                   0,  IFtime);
  SIparse()->registerFunc("MakeTime",               2,  IFmakeTime);
  SIparse()->registerFunc("TimeToString",           2,  IFtimeToString);
  SIparse()->registerFunc("TimeToVals",             3,  IFtimeToVals);
  SIparse()->registerFunc("MilliSec",               0,  IFmilliSec);
  SIparse()->registerFunc("StartTiming",            1,  IFstartTiming);
  SIparse()->registerFunc("StopTiming",             1,  IFstopTiming);

  // File System Interface
  SIparse()->registerFunc("Glob",                   1,  IFglob);
  SIparse()->registerFunc("Open",                   2,  IFopen);
  SIparse()->registerFunc("Popen",                  2,  IFpopen);
  SIparse()->registerFunc("Sopen",                  2,  IFsopen);
  SIparse()->registerFunc("ReadLine",               2,  IFreadLine);
  SIparse()->registerFunc("ReadChar",               1,  IFreadChar);
  SIparse()->registerFunc("WriteLine",              2,  IFwriteLine);
  SIparse()->registerFunc("WriteChar",              2,  IFwriteChar);
  SIparse()->registerFunc("TempFile",               1,  IFtempFile);
  SIparse()->registerFunc("ListDirectory",          2,  IFlistDirectory);
  SIparse()->registerFunc("MakeDir",                1,  IFmakeDir);
  SIparse()->registerFunc("FileStat",               2,  IFfileStat);
  SIparse()->registerFunc("DeleteFile",             1,  IFdeleteFile);
  SIparse()->registerFunc("MoveFile",               2,  IFmoveFile);
  SIparse()->registerFunc("CopyFile",               2,  IFcopyFile);
  SIparse()->registerFunc("CreateBak",              1,  IFcreateBak);
  SIparse()->registerFunc("Md5Digest",              1,  IFmd5Digest);

  // Xic Client/Server Interface
  SIparse()->registerFunc("ReadData",               2,  IFreadData);
  SIparse()->registerFunc("ReadReply",              2,  IFreadReply);
  SIparse()->registerFunc("ConvertReply",           2,  IFconvertReply);
  SIparse()->registerFunc("WriteMsg",               2,  IFwriteMsg);

  // System Command Interface
  SIparse()->registerFunc("Shell",                  1,  IFshell);
  SIparse()->registerFunc("System",                 1,  IFshell);  // alias
  SIparse()->registerFunc("GetPID",                 1,  IFgetPID);

  // Menu Buttons
  SIparse()->registerFunc("SetButtonStatus",        3,  IFsetButtonStatus);
  SIparse()->registerFunc("GetButtonStatus",        2,  IFgetButtonStatus);
  SIparse()->registerFunc("PressButton",            2,  IFpressButton);
  SIparse()->registerFunc("BtnDown",                5,  IFbtnDown);
  SIparse()->registerFunc("BtnUp",                  5,  IFbtnUp);
  SIparse()->registerFunc("KeyDown",                3,  IFkeyDown);
  SIparse()->registerFunc("KeyUp",                  3,  IFkeyUp);

  // Mouse Input
  SIparse()->registerFunc("Point",                  1,  IFpoint);
  SIparse()->registerFunc("Selection",              0,  IFselection);

  // Graphical Input
  SIparse()->registerFunc("PopUpInput",             4,  IFpopUpInput);
  SIparse()->registerFunc("PopUpAffirm",            1,  IFpopUpAffirm);
  SIparse()->registerFunc("PopUpNumeric",           6,  IFpopUpNumeric);

  // Text Input
  SIparse()->registerFunc("AskReal",                2,  IFaskReal);
  SIparse()->registerFunc("AskString",              2,  IFaskString);
  SIparse()->registerFunc("AskConsoleReal",         2,  IFaskConsoleReal);
  SIparse()->registerFunc("AskConsoleString",       2,  IFaskConsoleString);
  SIparse()->registerFunc("GetKey",                 0,  IFgetKey);

  // Text Output
  SIparse()->registerFunc("SepString",              2,  IFsepString);
  SIparse()->registerFunc("ShowPrompt",       VARARGS,  IFshowPrompt);
  SIparse()->registerFunc("SetIndent",              1,  IFsetIndent);
  SIparse()->registerFunc("SetPrintLimits",         2,  IFsetPrintLimits);
  SIparse()->registerFunc("Print",            VARARGS,  IFprint);
  SIparse()->registerFunc("PrintLog",         VARARGS,  IFprintLog);
  SIparse()->registerFunc("PrintString",      VARARGS,  IFprintString);
  SIparse()->registerFunc("PrintStringEsc",   VARARGS,  IFprintStringEsc);
  SIparse()->registerFunc("Message",          VARARGS,  IFmessage);
  SIparse()->registerFunc("ErrorMsg",         VARARGS,  IFerrorMsg);
  SIparse()->registerFunc("TextWindow",             2,  IFtextWindow);

  // Database Visualization Functions (export)
  SIparse()->registerFunc("ShowDb",                 2,  IFshowDb);

#ifdef HAVE_PYTHON
  py_register_misc2();
#endif
#ifdef HAVE_TCL
  tcl_register_misc2();
#endif
}


/*========================================================================*
 * Utility Functions
 *========================================================================*/
//-------------------------------------------------------------------------
// Arrays
//-------------------------------------------------------------------------

// (int) ArrayDims(out_array, array)
//
// This function returns the size (number of storage locations) of an
// array, and possibly the size of each dimension.  Arrays can have
// from one to three dimensions.  If the first argument is an array
// with size three or larger, the size of each dimension of the array
// in the second argument is stored in the first three locations of
// the first argument array, with the 0'th index being the lowest
// order.  Unused dimensions are saved as 0.  If the first argument is
// an integer 0, no dimension size information is returned.  The size
// of the array (number of storage locations, which should equal the
// product of the nonzero dimensions) is returned by the function.
// This function fails if the second argument is not an array.
//
bool
misc2_funcs::IFarrayDims(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array_if(args, 0, &vals, MAXDIMS))

    if (args[1].type != TYP_ARRAY)
        return (BAD);
    for (int i = 0; i < MAXDIMS; i++)
        if (vals) {
            vals[i] = args[1].content.a->dims()[i];
    }
    res->type = TYP_SCALAR;
    res->content.value = args[1].content.a->length();
    return (OK);
}


// (int) ArrayDimension(out_array, array)
//
// This function is very similar to ArrayDims(), and the arguments
// have the same types and purpose as for that function.  The return
// value is the number of dimensions used (1-3) if the second argument
// is an array, 0 otherwise.  Unlike ArrayDims(), this function does
// not fail if the second argument is not an array.
//
bool
misc2_funcs::IFarrayDimension(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array_if(args, 0, &vals, MAXDIMS))

    int ndims = 0;
    if (args[1].type == TYP_ARRAY) {
        for (int i = 0; i < MAXDIMS; i++) {
            if (vals)
                vals[i] = args[1].content.a->dims()[i];
            if (args[1].content.a->dims()[i])
                ndims++;
        }
    }
    res->type = TYP_SCALAR;
    res->content.value = ndims;
    return (OK);
}


// (int) GetDims(array, array_out)
//
// This is for backward compatibility.  This function is eqivalent to
// ArrayDimension(), but the two arguments are in reverse order.
// This function may disappear - don't use.
//
bool
misc2_funcs::IFgetDims(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array_if(args, 1, &vals, MAXDIMS))

    int ndims = 0;
    if (args[0].type == TYP_ARRAY) {
        for (int i = 0; i < MAXDIMS; i++) {
            if (vals)
                vals[i] = args[0].content.a->dims()[i];
            if (args[0].content.a->dims()[i])
                ndims++;
        }
    }
    res->type = TYP_SCALAR;
    res->content.value = ndims;
    return (OK);
}


// (int) DupArray(dest_array, src_array)
//
// This function duplicates the src_array into the dest_array.  The
// dest_array argument must be an unreferenced array.  Upon successful
// return, the dest_array will be a copy of the src_array, and the
// return value is 1.  If the dest_array can not be resized due to its
// being referenced by a pointer, 0 is returned.  The function will
// fail if either argument is not an array.

bool
misc2_funcs::IFdupArray(Variable *res, Variable *args, void*)
{
    if (args[0].type != TYP_ARRAY || args[1].type != TYP_ARRAY)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (args[0].content.a->refptr() || args[0].content.a->refcnt())
        return (OK);
    int ldst = args[1].content.a->length();
    if (ldst != args[0].content.a->length()) {
        args[0].content.a->allocate(ldst);
        if (!args[0].content.a->values())
            return (BAD);
    }
    for (int i = 0; i < MAXDIMS; i++)
        args[0].content.a->dims()[i] = args[1].content.a->dims()[i];
    memcpy(args[0].content.a->values(), args[1].content.a->values(),
        ldst*sizeof(double));
    res->content.value = 1;
    return (OK);
}


namespace {
    struct ael_t
    {
        double value;
        int ix;
    };


    inline bool
    asccmp(const ael_t &e1, const ael_t &e2)
    {
        return (e1.value < e2.value);
    }


    inline bool
    dsccmp(const ael_t &e1, const ael_t &e2)
    {
        return (e1.value > e2.value);
    }
}


// (int) SortArray(array, size, descend, indices)
//
// This function will sort the elements of the array passed as the
// first argument.  The number of elements to sort is given in the
// second argument.  The function will fail if size is negative, or
// will return without action if size is 0.  The size is implicitly
// limited to the size of the array.  The sorted values will be
// ascending if the third argument is 0, descending otherwise.  The
// fourth argument, if nonzero, is an array which will be filled in
// with the index mapping applied to the array.  For example, if
// array[5] is moved to array[0] during the sort, the value of
// indices[0] will be 5.  This array will be resized if necessary, but
// the function will fail if resizing fails.
//
// If the array being sorted is multi-dimensional, the sorting will
// use the internal linear order.  The return value is the actual
// number of items sorted, which will be the value of size unless this
// was limited by the actual array size.
//
bool
misc2_funcs::IFsortArray(Variable *res, Variable *args, void*)
{
    double *arr;
    ARG_CHK(arg_array(args, 0, &arr, 1))
    int size;
    ARG_CHK(arg_int(args, 1, &size))
    bool dsc;
    ARG_CHK(arg_boolean(args, 2, &dsc))
    double *ixes;
    ARG_CHK(arg_array_if(args, 3, &ixes, 1))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (size < 0)
        return (BAD);
    if (size > args[0].content.a->length())
        size = args[0].content.a->length();
    if (size == 0)
        return (OK);

    if (size > args[3].content.a->length()) {
        if (ADATA(args[3].content.a)->resize(size) == BAD)
            return (BAD);
        ixes = args[3].content.a->values();
    }

    res->content.value = size;
    if (size == 1) {
        if (ixes)
            *ixes = 0;
        return (OK);
    }
    ael_t *vals = new ael_t[size];
    for (int i = 0; i < size; i++) {
        vals[i].value = arr[i];
        vals[i].ix = i;
    }
    if (dsc)
        std::sort(vals, vals + size, dsccmp);
    else
        std::sort(vals, vals + size, asccmp);
    for (int i = 0; i < size; i++) {
        arr[i] = vals[i].value;
        if (ixes)
            ixes[i] = vals[i].ix;
    }
    delete [] vals;
    return (OK);
}


//-------------------------------------------------------------------------
// Bitwise Logic
//-------------------------------------------------------------------------

// (unsigned int) ShiftBits(var, val)
//
// This function will shift the binary representation of the unsigned
// integer var by val.  If val is positive, the bits are shifted to
// the right, or if negative the bits are shifted to the left.
//
// All numerical data are stored internally in double-precision
// floating point representation.  This function merely converts the
// internal values to integer data, and applies the integer shift
// operators.
//
// The function returns the shifted value.
//
bool
misc2_funcs::IFshiftBits(Variable *res, Variable *args, void*)
{
    unsigned int var;
    ARG_CHK(arg_unsigned(args, 0, &var))
    int val;
    ARG_CHK(arg_int(args, 1, &val))

    int bw = 8*sizeof(int);
    if (val > bw)
        val = bw;
    else if (val < -bw)
        val = -bw;

    res->type = TYP_SCALAR;
    res->content.value = val > 0 ? var >> val : var << -val;
    return (OK);
}


// (unsigned int) AndBits(val1, val2)
//
// This runction returns the bitwise AND of the two arguments, which
// are taken as unsigned integers.
//
bool
misc2_funcs::IFandBits(Variable *res, Variable *args, void*)
{
    unsigned int val1;
    ARG_CHK(arg_unsigned(args, 0, &val1))
    unsigned int val2;
    ARG_CHK(arg_unsigned(args, 1, &val2))

    res->type = TYP_SCALAR;
    res->content.value = val1 & val2;
    return (OK);
}


// (unsigned int) OrBits(val1, val2)
//
// This runction returns the bitwise OR of the two arguments, which
// are taken as unsigned integers.
//
bool
misc2_funcs::IForBits(Variable *res, Variable *args, void*)
{
    unsigned int val1;
    ARG_CHK(arg_unsigned(args, 0, &val1))
    unsigned int val2;
    ARG_CHK(arg_unsigned(args, 1, &val2))

    res->type = TYP_SCALAR;
    res->content.value = val1 | val2;
    return (OK);
}


// (unsigned int) XorBits(val1, val2)
//
// This runction returns the bitwise exclusive-OR of the two
// arguments, which are taken as unsigned integers.
//
bool
misc2_funcs::IFxorBits(Variable *res, Variable *args, void*)
{
    unsigned int val1;
    ARG_CHK(arg_unsigned(args, 0, &val1))
    unsigned int val2;
    ARG_CHK(arg_unsigned(args, 1, &val2))

    res->type = TYP_SCALAR;
    res->content.value = val1 ^ val2;
    return (OK);
}


// (unsigned int) NotBits(val)
//
// This runction returns the bitwise NOT of the argument, which is
// taken as an unsigned integer.
//
bool
misc2_funcs::IFnotBits(Variable *res, Variable *args, void*)
{
    unsigned int val;
    ARG_CHK(arg_unsigned(args, 0, &val))

    res->type = TYP_SCALAR;
    res->content.value = ~val;
    return (OK);
}


//-------------------------------------------------------------------------
// Error Reporting
//-------------------------------------------------------------------------

// (string) GetError()
//
// This returns the current error text.  Error messages generated by
// an unsuccessful operation that opens, translates, or writes cells
// or manipulates the database, can be retrieved with this function
// for diagnostic purposes.  This function should be called
// immediately after an error return is detected, since subsequent
// operations may clear or change the error text.  If there are no
// recorded errors, a "no errors" string is returned.  This function
// never fails and always returns a message string.
//
bool
misc2_funcs::IFgetError(Variable *res, Variable*, void*)
{
    const char *str;
    if (Errs()->has_error())
        // never true
        str = Errs()->get_error();
    else
        str = Errs()->get_lasterr();
    if (!str || !*str)
        str = "No errors.";
    res->type = TYP_STRING;
    res->content.string = lstring::copy(str);
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// AddError(string)
//
// This function will add a string to the current error message, which
// can be retrieved with GetError().  This is useful for error
// reporting from user-defined functions.  Any number of calls can be
// made, with the retrieved text consisting of a concatenation of the
// strings, with line termination added if necessary, in reverse order
// of the AddError calls.  No other built-in function should be
// executed between calls to AddError, or between a call that
// generated and error and a call to AddError, as this will cause the
// second string to overwrite the first.
//
bool
misc2_funcs::IFaddError(Variable*, Variable *args, void*)
{
    // Errs()->init_error is called before executing the function body
    // of all script functions but this one (in parstree.cc), which  is
    // why this function is not static.
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))

    if (string)
        Errs()->add_error(string);
    return (OK);
}


// (int) GetLogNumber()
//
// Return the integer index of the most recent error message dumped to
// the errors log file.  The return value is 0 if there are no errors
// recorded in the file.
//
bool
misc2_funcs::IFgetLogNumber(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = Log()->GetMsgNum();
    return (OK);
}


// (string) GetLogMessage(message_num)
//
// Return the error message string corresponding to the integer
// argument, as was appended to the errors log file.  The 10 most
// recent error messages are available.  If the argument is out of
// range, a null string is returned.  The range is the current index
// to (not including) this index minus 10, or 0, whichever is larger.
//
bool
misc2_funcs::IFgetLogMessage(Variable *res, Variable *args, void*)
{
    int num;
    ARG_CHK(arg_int(args, 0, &num))

    res->type = TYP_STRING;
    res->content.string = Log()->GetMsg(num);
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) AddLogMessage(string, error)
//
// Apply a new message to the error/warning log file.  The second
// argument is a boolean which if nonzero will add the string as an
// error message, otherwise the message is added as a warning.  The
// return value is the index assigned to the new message, or 0 if the
// string is empty or null.
//
bool
misc2_funcs::IFaddLogMessage(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    bool err;
    ARG_CHK(arg_boolean(args, 1, &err))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (string && *string) {
        if (err)
            Log()->ErrorLog("AddLogMessage", string);
        else
            Log()->WarningLog("AddLogMessage", string);
        res->content.value = Log()->GetMsgNum();
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Generic Handle Functions
//-------------------------------------------------------------------------

// (int) NumHandles()
//
// This returns the number of handles of all types currently in the
// hash table.  It can be used as a check to make sure handles are
// being properly closed (and thus removed from the table) in the
// user's scripts.
//
bool
misc2_funcs::IFnumHandles(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value =  sHdl::HdlTable ? sHdl::HdlTable->allocated() : 0;
    return (OK);
}


// (int) HandleContent(handle)
//
// This function returns the number of objects currently referenced by
// the list-type handle passed as an argument.  The return value is 1
// for other types of handle.  The return value is 0 for an empty or
// closed handle.
//
bool
misc2_funcs::IFhandleContent(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLstring) {
            if (!hdl->data)
                hdl->close(id);
            else
                res->content.value =
                    stringlist::length((stringlist*)hdl->data);
        }
        else if (hdl->type == HDLobject) {
            if (!hdl->data)
                hdl->close(id);
            else {
                int cnt = 0;
                for (CDol *ol = (CDol*)hdl->data; ol; ol = ol->next, cnt++) ;
                res->content.value = cnt;
            }
        }
        else if (hdl->type == HDLprpty) {
            if (!hdl->data)
                hdl->close(id);
            else {
                int cnt = 0;
                for (CDpl *pl = (CDpl*)hdl->data; pl; pl = pl->next, cnt++) ;
                res->content.value = cnt;
            }
        }
        else if (
                hdl->type == HDLnode   || hdl->type == HDLterminal ||
                hdl->type == HDLdevice || hdl->type == HDLdcontact ||
                hdl->type == HDLsubckt || hdl->type == HDLscontact) {
            if (!hdl->data)
                hdl->close(id);
            else
                res->content.value =
                    tlist<void>::count((tlist<void>*)hdl->data);
        }
        else
            // HDLgeneric, HDLfd, HDLgen, HDLgraph, HDLregex, HDLchd
            res->content.value = 1;

    }
    return (OK);
}


// (int) HandleTruncate(handle, count)
//
// This function truncates the list referenced by the handle, leaving
// the current item plus at most count additional items.  If count is
// negative, it is taken as 0.  The function returns 1 on success, or
// 0 if the handle does not reference a list or is not found.
//
bool
misc2_funcs::IFhandleTruncate(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int items;
    ARG_CHK(arg_int(args, 1, &items))

    if (items < 0)
        items = 0;
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLstring) {
            stringlist *s0 = (stringlist*)hdl->data;
            for (stringlist *s = s0; s; s = s->next) {
                if (items == 0) {
                    stringlist::destroy(s->next);
                    s->next = 0;
                }
                items--;
            }
            res->content.value = 1;
        }
        else if (hdl->type == HDLobject) {
            CDol *o0 = (CDol*)hdl->data;
            for (CDol *o = o0; o; o = o->next) {
                if (items == 0) {
                    CDol::destroy(o->next);
                    o->next = 0;
                }
                items--;
            }
            res->content.value = 1;
        }
        else if (hdl->type == HDLprpty) {
            CDpl *p0 = (CDpl*)hdl->data;
            for (CDpl *p = p0; p; p = p->next) {
                if (items == 0) {
                    CDpl::destroy(p->next);
                    p->next = 0;
                }
                items--;
            }
            res->content.value = 1;
        }
        else if (
                hdl->type == HDLnode   || hdl->type == HDLterminal ||
                hdl->type == HDLdevice || hdl->type == HDLdcontact ||
                hdl->type == HDLsubckt || hdl->type == HDLscontact) {
            tlist<void> *t0 = (tlist<void>*)hdl->data;
            for (tlist<void> *t = t0; t; t = t->next) {
                if (items == 0) {
                    tlist<void>::destroy(t->next);
                    t->next = 0;
                }
                items--;
            }
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) HandleNext(handle)
//
// This function will advance the handle to reference the next element
// in its list, for handle types that reference a list.  It has no
// effect on other handles.  If there were no objects left in the
// list, or the handle was not found, 0 is returned, otherwise 1 is
// returned.
//
bool
misc2_funcs::IFhandleNext(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    if (id) {
        args[0].incr__handle(res);
        res->type = TYP_SCALAR;
        res->content.value = to_boolean(res->content.value);
        return (OK);
    }
    res->type = TYP_SCALAR;
    res->content.value = 0.0;
    return (OK);
}


// (handle) HandleDup(handle)
//
// This function will duplicate a handle and its underlying reference
// or list of references.  The new handle is not associated with the
// old, and should be iterated through or closed explicitly.  For file
// descriptors, the return value is a duplicate descriptor to the
// underlying file, with the same read/write mode and file position as
// the original handle.  If the function succeeds, a handle value is
// returned.  If the function fails, 0 is returned.
//
bool
misc2_funcs::IFhandleDup(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        sHdl *nhdl = 0;
        if (hdl->type == HDLgeneric)
            nhdl = new sHdl(hdl->data);
        else if (hdl->type == HDLstring) {
            stringlist *s0 = 0, *se = 0;
            for (stringlist *s = (stringlist*)hdl->data; s; s = s->next) {
                if (!s0)
                    s0 = se = new stringlist(lstring::copy(s->string), 0);
                else {
                    se->next = new stringlist(lstring::copy(s->string), 0);
                    se = se->next;
                }
            }
            nhdl = new sHdlString(s0);
        }
        else if (hdl->type == HDLobject) {
            bool copies = ((sHdlObject*)hdl)->copies;
            CDol *o0 = 0, *oe = 0;
            for (CDol *o = (CDol*)hdl->data; o; o = o->next) {
                if (!o0)
                    o0 = oe = new CDol(o->odesc, 0);
                else {
                    oe->next = new CDol(o->odesc, 0);
                    oe = oe->next;
                }
                if (copies)
                    oe->odesc = oe->odesc->copyObject();
            }
            nhdl = new sHdlObject(o0, ((sHdlObject*)hdl)->sdesc, copies);
        }
        else if (hdl->type == HDLfd) {
            int fd = dup(id);
            if (fd > 0)
                nhdl = new sHdlFd(fd, lstring::copy((char*)hdl->data));
        }
        else if (hdl->type == HDLprpty) {
            CDpl *p0 = 0, *pe = 0;
            for (CDpl *p = (CDpl*)hdl->data; p; p = p->next) {
                if (!p0)
                    p0 = pe = new CDpl(p->pdesc, 0);
                else {
                    pe->next = new CDpl(p->pdesc, 0);
                    pe = pe->next;
                }
            }
            nhdl = new sHdlPrpty(p0, ((sHdlPrpty*)hdl)->odesc,
                ((sHdlPrpty*)hdl)->sdesc);
        }
        else if (hdl->type == HDLnode) {
            tlist2<void> *t0 = 0, *te = 0;
            for (tlist2<void> *t = (tlist2<void>*)hdl->data; t; t = t->next) {
                if (!t0)
                    t0 = te = new tlist2<void>(t->elt, t->xtra, 0);
                else {
                    te->next = new tlist2<void>(t->elt, t->xtra, 0);
                    te = te->next;
                }
            }
            nhdl = new sHdlNode((tlist2<CDp_nodeEx>*)t0);
        }
        else if (hdl->type == HDLterminal ||
                hdl->type == HDLdevice || hdl->type == HDLdcontact ||
                hdl->type == HDLsubckt || hdl->type == HDLscontact) {
            tlist<void> *t0 = 0, *te = 0;
            for (tlist<void> *t = (tlist<void>*)hdl->data; t; t = t->next) {
                if (!t0)
                    t0 = te = new tlist<void>(t->elt, 0);
                else {
                    te->next = new tlist<void>(t->elt, 0);
                    te = te->next;
                }
            }
            if (hdl->type == HDLterminal)
                nhdl = new sHdlTerminal((tlist<CDterm>*)t0);
            else if (hdl->type == HDLdevice)
                nhdl = new sHdlDevice((tlist<sDevInst>*)t0,
                    ((sHdlDevice*)hdl)->gdesc);
            else if (hdl->type == HDLdcontact)
                nhdl = new sHdlDevContact((tlist<sDevContactInst>*)t0,
                    ((sHdlDevContact*)hdl)->gdesc);
            else if (hdl->type == HDLsubckt)
                nhdl = new sHdlSubckt((tlist<sSubcInst>*)t0,
                    ((sHdlSubckt*)hdl)->gdesc);
            else if (hdl->type == HDLscontact)
                nhdl = new sHdlSubcContact((tlist<sSubcContactInst>*)t0,
                    ((sHdlSubcContact*)hdl)->gdesc);
        }
        else if (hdl->type == HDLgen) {
            sPF *gen = (sPF*)hdl->data;
            nhdl = new sHdlGen(gen->dup(), ((sHdlGen*)hdl)->sdesc,
                gdrec::dup(((sHdlGen*)hdl)->rec));
        }
        if (nhdl) {
            res->type = TYP_HANDLE;
            res->content.value = nhdl->id;
        }
    }
    return (OK);
}


// (handle) HandleDupNitems(handle, count)
//
// This function acts similarly to HandleDup(), however for handles
// that are references to lists, the new handle will reference the
// current item plus at most count additional items.  For handles that
// are not references to lists, the count argument is ignored.  The
// new handle is returned on success, 0 is returned if there was an
// error.
//
bool
misc2_funcs::IFhandleDupNitems(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int items;
    ARG_CHK(arg_int(args, 1, &items))

    if (items < 0)
        items = 0;
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        sHdl *nhdl = 0;
        if (hdl->type == HDLgeneric)
            nhdl = new sHdl(hdl->data);
        else if (hdl->type == HDLstring) {
            stringlist *s0 = 0, *se = 0;
            for (stringlist *s = (stringlist*)hdl->data; s; s = s->next) {
                if (!s0)
                    s0 = se = new stringlist(lstring::copy(s->string), 0);
                else {
                    se->next = new stringlist(lstring::copy(s->string), 0);
                    se = se->next;
                }
                if (!items)
                    break;
                items--;
            }
            nhdl = new sHdlString(s0);
        }
        else if (hdl->type == HDLobject) {
            bool copies = ((sHdlObject*)hdl)->copies;
            CDol *o0 = 0, *oe = 0;
            for (CDol *o = (CDol*)hdl->data; o; o = o->next) {
                if (!o0)
                    o0 = oe = new CDol(o->odesc, 0);
                else {
                    oe->next = new CDol(o->odesc, 0);
                    oe = oe->next;
                }
                if (copies)
                    oe->odesc = oe->odesc->copyObject();
                if (!items)
                    break;
                items--;
            }
            nhdl = new sHdlObject(o0, ((sHdlObject*)hdl)->sdesc, copies);
        }
        else if (hdl->type == HDLfd) {
            int fd = dup(id);
            if (fd > 0)
                nhdl = new sHdlFd(fd, lstring::copy((char*)hdl->data));
        }
        else if (hdl->type == HDLprpty) {
            CDpl *p0 = 0, *pe = 0;
            for (CDpl *p = (CDpl*)hdl->data; p; p = p->next) {
                if (!p0)
                    p0 = pe = new CDpl(p->pdesc, 0);
                else {
                    pe->next = new CDpl(p->pdesc, 0);
                    pe = pe->next;
                }
                if (!items)
                    break;
                items--;
            }
            nhdl = new sHdlPrpty(p0, ((sHdlPrpty*)hdl)->odesc,
                ((sHdlPrpty*)hdl)->sdesc);
        }
        else if (hdl->type == HDLnode) {
            tlist2<void> *t0 = 0, *te = 0;
            for (tlist2<void> *t = (tlist2<void>*)hdl->data; t; t = t->next) {
                if (!t0)
                    t0 = te = new tlist2<void>(t->elt, t->xtra, 0);
                else {
                    te->next = new tlist2<void>(t->elt, t->xtra, 0);
                    te = te->next;
                }
                if (!items)
                    break;
                items--;
            }
            nhdl = new sHdlNode((tlist2<CDp_nodeEx>*)t0);
        }
        else if (hdl->type == HDLterminal ||
                hdl->type == HDLdevice || hdl->type == HDLdcontact ||
                hdl->type == HDLsubckt || hdl->type == HDLscontact) {
            tlist<void> *t0 = 0, *te = 0;
            for (tlist<void> *t = (tlist<void>*)hdl->data; t; t = t->next) {
                if (!t0)
                    t0 = te = new tlist<void>(t->elt, 0);
                else {
                    te->next = new tlist<void>(t->elt, 0);
                    te = te->next;
                }
                if (!items)
                    break;
                items--;
            }
            if (hdl->type == HDLterminal)
                nhdl = new sHdlTerminal((tlist<CDterm>*)t0);
            else if (hdl->type == HDLdevice)
                nhdl = new sHdlDevice((tlist<sDevInst>*)t0,
                    ((sHdlDevice*)hdl)->gdesc);
            else if (hdl->type == HDLdcontact)
                nhdl = new sHdlDevContact((tlist<sDevContactInst>*)t0,
                    ((sHdlDevContact*)hdl)->gdesc);
            else if (hdl->type == HDLsubckt)
                nhdl = new sHdlSubckt((tlist<sSubcInst>*)t0,
                    ((sHdlSubckt*)hdl)->gdesc);
            else if (hdl->type == HDLscontact)
                nhdl = new sHdlSubcContact((tlist<sSubcContactInst>*)t0,
                    ((sHdlSubcContact*)hdl)->gdesc);
        }
        else if (hdl->type == HDLgen) {
            sPF *gen = (sPF*)hdl->data;
            nhdl = new sHdlGen(gen->dup(), ((sHdlGen*)hdl)->sdesc,
                gdrec::dup(((sHdlGen*)hdl)->rec));
        }
        if (nhdl) {
            res->type = TYP_HANDLE;
            res->content.value = nhdl->id;
        }
    }
    return (OK);
}


// (handle) H(integer)
//
// This function creates a handle from an integer variable.  This is
// needed for using the handle values stored in the array created with
// the HandleArray() function, or otherwise.  Array elements are
// numeric variables, and can not be passed directly to functions
// expecting handles.  This function performs the necessary data
// conversion.  Example:  SomeFunction(H(handle_array[3]))
//
// Array elements are always numeric variables, though it is possible
// to assign a handle value to an array element.  In order to use as a
// handle an array element so defined, the H() function must be
// applied.  Since scalar variables become handles when assigned from
// a handle, the H() function should never be needed for scalar
// variables.
//
bool
misc2_funcs::IFh(Variable *res, Variable *args, void*)
{
    if (args[0].type == TYP_SCALAR || args[0].type == TYP_HANDLE) {
        res->type = TYP_HANDLE;
        res->content.value = (int)args[0].content.value;
        return (OK);
    }
    Errs()->add_error("bad argument 0.");
    return (BAD);
}


// (int) HandleArray(handle, array)
//
// This function will create a new handle for every object in the list
// referenced by the handle argument, and add that handle identifier
// to the array.  Each new handle references a single object.  The
// array argument is the name of a previously defined array variable.
// The array will be resized if necessary, if possible.  It is not
// possible to resize an array referenced through a pointer, or an
// array with pointer references.  The function returns 0 if the array
// cannot be resized and resizing is needed.  The number of new
// handles is returned, which will be 0 if the handle argument is
// empty or does not reference a list.  The handles in the array of
// handles identifiers can be closed conveniently with the
// CloseArray() function.  Since the array elements are numeric
// quantities and not handles, they can not be passed directly to
// functions expecting handles.  The H() function should be used to
// create a temporary handle variable from the array elements when a
// handle is needed:  for example, HandleNext(H(array[2])).
//
bool
misc2_funcs::IFhandleArray(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    if (args[1].type != TYP_ARRAY)
        return (BAD);

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLstring) {
            stringlist *s0 = (stringlist*)hdl->data;
            int size = stringlist::length(s0);
            if (ADATA(args[1].content.a)->resize(size) == BAD)
                return (OK);
            int cnt = 0;
            for (stringlist *s = s0; s; s = s->next) {
                stringlist *snew =
                    new stringlist(lstring::copy(s->string), 0);
                sHdl *h =  new sHdlString(snew);
                args[1].content.a->values()[cnt] = h->id;
                cnt++;
            }
            res->content.value = cnt;
        }
        else if (hdl->type == HDLobject) {
            int size = 0;
            for (CDol *ol = (CDol*)hdl->data; ol; ol = ol->next, size++) ;
            if (ADATA(args[1].content.a)->resize(size) == BAD)
                return (OK);
            bool copies = ((sHdlObject*)hdl)->copies;
            int cnt = 0;
            for (CDol *ol = (CDol*)hdl->data; ol; ol = ol->next) {
                CDol *onew = new CDol(ol->odesc, 0);
                if (copies)
                    onew->odesc = onew->odesc->copyObject();
                sHdl *h = new sHdlObject(onew, ((sHdlObject*)hdl)->sdesc,
                    copies);
                args[1].content.a->values()[cnt] = h->id;
                cnt++;
            }
            res->content.value = cnt;
        }
        else if (hdl->type == HDLprpty) {
            int size = 0;
            for (CDpl *pl = (CDpl*)hdl->data; pl; pl = pl->next, size++) ;
            if (ADATA(args[1].content.a)->resize(size) == BAD)
                return (OK);
            int cnt = 0;
            for (CDpl *pl = (CDpl*)hdl->data; pl; pl = pl->next) {
                CDpl *pnew = new CDpl(pl->pdesc, 0);
                sHdl *h = new sHdlPrpty(pnew, ((sHdlPrpty*)hdl)->odesc,
                    ((sHdlPrpty*)hdl)->sdesc);
                args[1].content.a->values()[cnt] = h->id;
                cnt++;
            }
            res->content.value = cnt;
        }
        else if (hdl->type == HDLnode) {
            int size = tlist2<void>::count((tlist2<void>*)hdl->data);
            if (ADATA(args[1].content.a)->resize(size) == BAD)
                return (OK);
            int cnt = 0;
            for (tlist2<void> *t = (tlist2<void>*)hdl->data; t; t = t->next) {
                tlist2<void> *tnew = new tlist2<void>(t->elt, t->xtra, 0);
                sHdl *h = new sHdlNode((tlist2<CDp_nodeEx>*)tnew);
                args[1].content.a->values()[cnt] = h->id;
                cnt++;
            }
        }
        else if (hdl->type == HDLterminal ||
                hdl->type == HDLdevice || hdl->type == HDLdcontact ||
                hdl->type == HDLsubckt || hdl->type == HDLscontact) {
            int size = tlist<void>::count((tlist<void>*)hdl->data);
            if (ADATA(args[1].content.a)->resize(size) == BAD)
                return (OK);
            int cnt = 0;
            for (tlist<void> *t = (tlist<void>*)hdl->data; t; t = t->next) {
                tlist<void> *tnew = new tlist<void>(t->elt, 0);
                sHdl *h = 0;
                if (hdl->type == HDLterminal)
                    h = new sHdlTerminal((tlist<CDterm>*)tnew);
                else if (hdl->type == HDLdevice)
                    h = new sHdlDevice((tlist<sDevInst>*)tnew,
                        ((sHdlDevice*)hdl)->gdesc);
                else if (hdl->type == HDLdcontact)
                    h = new sHdlDevContact((tlist<sDevContactInst>*)tnew,
                        ((sHdlDevContact*)hdl)->gdesc);
                else if (hdl->type == HDLsubckt)
                    h = new sHdlSubckt((tlist<sSubcInst>*)tnew,
                        ((sHdlSubckt*)hdl)->gdesc);
                else if (hdl->type == HDLscontact)
                    h = new sHdlSubcContact((tlist<sSubcContactInst>*)tnew,
                        ((sHdlSubcContact*)hdl)->gdesc);

                args[1].content.a->values()[cnt] = h->id;
                cnt++;
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


// (int) HandleCat(handle1, handle2)
//
// This function will add a copy of the list referenced by the second
// handle to the end of the list referenced by the first handle.  Both
// handles must point to handles referencing lists of the same kind.
// The return value is nonzero for success, 0 otherwise.
//
bool
misc2_funcs::IFhandleCat(Variable *res, Variable *args, void*)
{
    int id1;
    ARG_CHK(arg_handle(args, 0, &id1))
    int id2;
    ARG_CHK(arg_handle(args, 1, &id2))

    if (id1 && id2)
        args[0].cat__handle(&args[1], res);
    else
        res->content.value = 0.0;
    res->type = TYP_SCALAR;
    return (OK);

}


// (int) HandleReverse(handle)
//
// This function will reverse the order of the list referenced by the
// handle.  Calling this function on other types of handles does
// nothing.  The function returns 1 if the action was successful, 0
// otherwise.
//
bool
misc2_funcs::IFhandleReverse(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLstring)
            stringlist::reverse((stringlist*)hdl->data);
        else if (hdl->type == HDLobject) {
            CDol *ol = (CDol*)hdl->data;
            CDol *o0 = 0;
            while (ol) {
                CDol *on = ol->next;
                ol->next = o0;
                o0 = ol;
                ol = on;
            }
            hdl->data = o0;
        }
        else if (hdl->type == HDLprpty) {
            CDpl *pl = (CDpl*)hdl->data;
            CDpl *p0 = 0;
            while (pl) {
                CDpl *pn = pl->next;
                pl->next = p0;
                p0 = pl;
                pl = pn;
            }
            hdl->data = p0;
        }
        else if (
                hdl->type == HDLnode   || hdl->type == HDLterminal ||
                hdl->type == HDLdevice || hdl->type == HDLdcontact ||
                hdl->type == HDLsubckt || hdl->type == HDLscontact) {
            tlist<void> *t = (tlist<void>*)hdl->data;
            tlist<void> *t0 = 0;
            while (t) {
                tlist<void> *tn = t->next;
                t->next = t0;
                t0 = t;
                t = tn;
            }
            hdl->data = t0;
        }
        res->content.value = 1;
    }
    return (OK);
}


// (int) HandlePurgeList(handle1, handle2)
//
// This function removes from the list referenced by the second handle
// any items that are also found in the list referenced by the first
// handle.  Both handles must reference lists of the same type.  The
// return value is 1 on success, 0 otherwise.
//
bool
misc2_funcs::IFhandlePurgeList(Variable *res, Variable *args, void*)
{
    int id1;
    ARG_CHK(arg_handle(args, 0, &id1))
    int id2;
    ARG_CHK(arg_handle(args, 1, &id2))

    sHdl *hdl1 = sHdl::get(id1);
    sHdl *hdl2 = sHdl::get(id2);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl1 && hdl2 && hdl1->type == hdl2->type) {

        sHdlUniq *hu = sHdl::new_uniq(hdl1);
        if (!hu) {
            res->content.value = 1;
            return (OK);
        }

        if (hdl1->type == HDLstring) {
            stringlist *s0 = (stringlist*)hdl2->data;
            stringlist *sp = 0, *sn;
            for (stringlist *s2 = s0; s2; s2 = sn) {
                sn = s2->next;
                hdl2->data = s2;
                if (hu->test(hdl2)) {
                    if (!sp)
                        s0 = sn;
                    else
                        sp->next = sn;
                    delete [] s2->string;
                    delete s2;
                    continue;
                }
                sp = s2;
            }
            hdl2->data = s0;
            res->content.value = 1;
        }
        else if (hdl1->type == HDLobject) {
            if (hu->copies() != ((sHdlObject*)hdl2)->copies) {
                delete hu;
                res->content.value = 1;
                return (OK);
            }
            CDol *o0 = (CDol*)hdl2->data;
            CDol *op = 0, *on;
            for (CDol *o2 = o0; o2; o2 = on) {
                on = o2->next;
                hdl2->data = o2;
                if (hu->test(hdl2)) {
                    if (!op)
                        o0 = on;
                    else
                        op->next = on;
                    if (hu->copies())
                        delete o2->odesc;
                    delete o2;
                    continue;
                }
                op = o2;
            }
            hdl2->data = o0;
            res->content.value = 1;
        }
        else if (hdl1->type == HDLprpty) {
            CDpl *p0 = (CDpl*)hdl2->data;
            CDpl *pp = 0, *pn;
            for (CDpl *p2 = p0; p2; p2 = pn) {
                pn = p2->next;
                hdl2->data = p2;
                if (hu->test(hdl2)) {
                    if (!pp)
                        p0 = pn;
                    else
                        pp->next = pn;
                    delete p2;
                    continue;
                }
                pp = p2;
            }
            hdl2->data = p0;
            res->content.value = 1;
        }
        else if (
                hdl1->type == HDLnode   || hdl1->type == HDLterminal ||
                hdl1->type == HDLdevice || hdl1->type == HDLdcontact ||
                hdl1->type == HDLsubckt || hdl1->type == HDLscontact) {
            tlist<void> *t0 = (tlist<void>*)hdl2->data;
            tlist<void> *tp = 0, *tn;
            for (tlist<void> *t2 = t0; t2; t2 = tn) {
                tn = t2->next;
                hdl2->data = t2;
                if (hu->test(hdl2)) {
                    if (!tp)
                        t0 = tn;
                    else
                        tp->next = tn;
                    delete t2;
                    continue;
                }
                tp = t2;
            }
            hdl2->data = t0;
            res->content.value = 1;
        }
        delete hu;
    }
    return (OK);
}


// (int) Close(handle)
//
// This function deletes and frees the handle.  It can be used to free
// up resources when a handle is no longer in use.  In particular, for
// file handles, the underlying file descriptor is closed by calling
// this function.  The return value is 1 if the handle is closed
// successfully, 0 if the handle is not found in the internal hash
// table or some other error occurrs.
//
bool
misc2_funcs::IFclose(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (!hdl->close(id))
            res->content.value = 1;
    }
    return (OK);
}


// (int) CloseArray(array, size)
//
// This function will call Close() on the first size elements of the
// array.  The array is assumed to be an array of handles as returned
// from HandleArray().  The function will fail if the array is not an
// array variable.  The return value is always 1.
//
bool
misc2_funcs::IFcloseArray(Variable *res, Variable *args, void*)
{
    if (args[0].type != TYP_ARRAY)
        return (BAD);
    int size;
    ARG_CHK(arg_int(args, 1, &size))

    if (size > args[0].content.a->length())
        size = args[0].content.a->length();
    for (int i = 0; i < size; i++) {
        int id = (int)args[0].content.a->values()[i];
        sHdl *hdl = sHdl::get(id);
        if (hdl)
            hdl->close(id);
    }
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


//-------------------------------------------------------------------------
// Memory Management
//-------------------------------------------------------------------------

// (int) FreeArray(array)
//
// This function will delete the memory used in the array, and
// reallocate the size to 1.  This function may be useful when memory
// is tight.  It is not possible to free an array it there are
// variables that point to it.  This function returns 1 on success, 0
// otherwise.
//
bool
misc2_funcs::IFfreeArray(Variable *res, Variable *args, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (args[0].type == TYP_ARRAY) {
        if (ADATA(args[0].content.a)->reset() == OK)
            res->content.value = 1;
    }
    return (OK);
}


// (real) CoreSize()
//
// This returns the total size of dynamically allocated memory used by
// Xic, in kilobytes.
//
bool
misc2_funcs::IFcoreSize(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
#ifdef HAVE_LOCAL_ALLOCATOR
    res->content.value = Memory()->coresize();
#else
    res->content.value = coresize();
#endif
    return (OK);
}


//-------------------------------------------------------------------------
// Miscellaneous
//-------------------------------------------------------------------------


// (int) Defined(variable)
//
// If a variable is referenced before it is assigned to, the variable
// has no type, but behaves in all ways as a string set to the
// variable's name.  This function returns 1 if the argument has a
// type assigned, or 0 if it has no type.
//
bool
misc2_funcs::IFdefined(Variable *res, Variable *args, void*)
{
    res->type = TYP_SCALAR;
    if (args[0].type == TYP_NOTYPE)
        res->content.value = 0.0;
    else
        res->content.value = 1.0;
    return (OK);
}


// (string) TypeOf(variable)
//
// This function returns a string which indicates the type of variable
// passed as an argument.  The possible returns are
//  "none"       variable has no type
//  "scalar"     variable is a scalar number
//  "complex"    variable is a complex number
//  "string"     variable is a string
//  "array"      variable is an array
//  "zoidlist"   variable is a zoidlist
//  "lexper"     variable is a lexper
//  "handle"     variable is a handle to something
//
bool
misc2_funcs::IFtypeOf(Variable *res, Variable *args, void*)
{
    res->type = TYP_STRING;
    switch (args[0].type) {
    case TYP_NOTYPE:
        res->content.string = lstring::copy("none");
        break;
    case TYP_SCALAR:
        res->content.string = lstring::copy("scalar");
        break;
    case TYP_STRING:
        res->content.string = lstring::copy("string");
        break;
    case TYP_ARRAY:
        res->content.string = lstring::copy("array");
        break;
    case TYP_CMPLX:
        res->content.string = lstring::copy("complex");
        break;
    case TYP_ZLIST:
        res->content.string = lstring::copy("zoidlist");
        break;
    case TYP_LEXPR:
        res->content.string = lstring::copy("lexper");
        break;
    case TYP_HANDLE:
        res->content.string = lstring::copy("handle");
        break;
    default:
        res->content.string = lstring::copy("unknown type");
        break;
    }
    res->flags |= VF_ORIGINAL;
    return (OK);
}


//-------------------------------------------------------------------------
// Path Manipulation and Query
//-------------------------------------------------------------------------

// (int) PathToEnd(path_name, dir)
//
// This function manipulates path strings.  Path_name can be anything,
// but it is usually one of Path, LibPath, HlpPath, or ScrPath.  Dir
// will be appended to the path if it does not exist in the path, or
// moved to the end if it does.  If the path_name is not a recognized
// path keyword, a variable of that name will be created to hold the
// path, which will be created if empty.  This can be used to store
// alternate paths.
//
bool
misc2_funcs::IFpathToEnd(Variable *res, Variable *args, void*)
{
    const char *pname;
    ARG_CHK(arg_string(args, 0, &pname))
    const char *dir;
    ARG_CHK(arg_string(args, 1, &dir))

    if (!pname)
        return (BAD);
    while (isspace(*pname))
        pname++;
    if (!*pname)
        return (BAD);
    if (!dir || !*dir)
        return (BAD);
    char *str = lstring::copy(CDvdb()->getVariable(pname));
    char *pth = FIO()->PAppendPath(str, dir, true);
    CDvdb()->setVariable(pname, pth);
    delete [] pth;
    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    return (OK);
}


// (int) PathToFront(path_name, dir)
//
// As above, but the dir will be added or moved to the front.
//
bool
misc2_funcs::IFpathToFront(Variable *res, Variable *args, void*)
{
    const char *pname;
    ARG_CHK(arg_string(args, 0, &pname))
    const char *dir;
    ARG_CHK(arg_string(args, 1, &dir))

    if (!pname)
        return (BAD);
    while (isspace(*pname))
        pname++;
    if (!*pname)
        return (BAD);
    if (!dir || !*dir)
        return (BAD);
    char* str = lstring::copy(CDvdb()->getVariable(pname));
    char* pth = FIO()->PPrependPath(str, dir, true);
    CDvdb()->setVariable(pname, pth);
    delete [] pth;
    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    return (OK);
}


// (int) InPath(path_name, dir)
//
// Returns 1 if directory dir is in the path, 0 otherwise.  See above
// for an explanation of the first argument.
//
bool
misc2_funcs::IFinPath(Variable *res, Variable *args, void*)
{
    const char *pname;
    ARG_CHK(arg_string(args, 0, &pname))
    const char *dir;
    ARG_CHK(arg_string(args, 1, &dir))

    if (!pname)
        return (BAD);
    while (isspace(*pname))
        pname++;
    if (!*pname)
        return (BAD);
    if (!dir || !*dir)
        return (BAD);
    const char* str = CDvdb()->getVariable(pname);
    res->type = TYP_SCALAR;
    res->content.value = FIO()->PInPath(str, dir);
    return (OK);
}


// (int) RemovePath(path_name, dir)
//
// Remove dir from the path named in the first argument.  See above
// for an explanation of the first argument.  The function returns 1
// if the path is changed, 0 otherwise.
//
bool
misc2_funcs::IFremovePath(Variable *res, Variable *args, void*)
{
    const char *pname;
    ARG_CHK(arg_string(args, 0, &pname))
    const char *dir;
    ARG_CHK(arg_string(args, 1, &dir))

    if (!pname)
        return (BAD);
    while (isspace(*pname))
        pname++;
    if (!*pname)
        return (BAD);
    if (!dir || !*dir)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    char* str = lstring::copy(CDvdb()->getVariable(pname));
    if (FIO()->PRemovePath(str, dir)) {
        res->content.value = 1.0;
        CDvdb()->setVariable(pname, str);
    }
    delete [] str;
    return (OK);
}


//-------------------------------------------------------------------------
// Regular Expressions
//-------------------------------------------------------------------------

int reg_error;

// (regex_handle) RegCompile(regex, case_insens)
//
// This function returns a handle to a compiled regular expression, as
// given in the first (string) argument.  The handle can be used for
// string comparison in RegCompare(), and should be closed when no
// longer needed.  The second argument is a flag; if nonzero the
// regular expression is compiled such that comparisons will be
// case-insensitive.  If zero, the test will be case-sensitive.  If
// the compilation fails, this function returns 0, and an error
// message can be obtained from RegError().
//
bool
misc2_funcs::IFregCompile(Variable *res, Variable *args, void*)
{
    const char *regx;
    ARG_CHK(arg_string(args, 0, &regx))
    bool ci;
    ARG_CHK(arg_boolean(args, 1, &ci))

    if (!regx)
        return (BAD);
    reg_error = 0;
    regex_t *preg = new regex_t;
    int flags = REG_EXTENDED | REG_NEWLINE;
    if (ci)
        flags |= REG_ICASE;
    res->type = TYP_SCALAR;
    res->content.value = 0;
    int ret = regcomp(preg, regx, flags);
    if (!ret) {
        sHdl *hdl = new sHdlRegex(preg);
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        reg_error = ret;
        delete preg;
    }
    return (OK);
}


// (int) RegCompare(regex_handle, string, array)
//
// This function compares the regular expression represented by the
// handle to the string given in the second argument.  If a match is
// found, the function returns 1, and the match location is set in the
// array argument, unless 0 is passed for this argument.  If an array
// is passed, it must have size 2 or larger.  The 0'th array element
// is set to the character index in the string where the match starts,
// and the next array location is set to the character index of the
// first character following the match.  This function returns 0 if
// there is no match, and -1 if an error occurs.  If -1 is returned,
// an error message can be obtained from RegError().
//
bool
misc2_funcs::IFregCompare(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))
    double *d;
    ARG_CHK(arg_array_if(args, 2, &d, 2))

    if (!string)
        return (BAD);
    reg_error = 0;
    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLregex)
            return (BAD);
        regmatch_t pmatch[1];
        int ret = regexec((regex_t*)hdl->data, string, 1, pmatch, 0);
        if (ret == 0) {
            if (d) {
                d[0] = pmatch[0].rm_so;
                d[1] = pmatch[0].rm_eo;
            }
            res->content.value = 1;
        }
        else if (ret == REG_NOMATCH)
            res->content.value = 0;
        else {
            reg_error = ret;
            res->content.value = -1;
        }
    }
    return (OK);
}


// (string) RegError(regex_handle)
//
// This function returns an error message string generated by the
// failure of RegCompile() or RegCompare().  It can be called after
// one of these functions returns an error value.  The argument is the
// handle value returned from RegCompile(), which will be 0 if
// RegCompile() fails.  A null string is returned if the handle is
// bogus.
//
bool
misc2_funcs::IFregError(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    char buf[256];
    res->type = TYP_STRING;
    res->content.string = 0;
    if (id) {
        sHdl *hdl = sHdl::get(id);
        if (hdl) {
            if (hdl->type != HDLregex)
                return (BAD);
            regerror(reg_error, (regex_t*)hdl->data, buf, 256);
            res->content.string = lstring::copy(buf);
            res->flags |= VF_ORIGINAL;
        }
    }
    else {
        regerror(reg_error, 0, buf, 256);
        res->content.string = lstring::copy(buf);
        res->flags |= VF_ORIGINAL;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// String List Handles
//-------------------------------------------------------------------------

// (stringlist_handle) StringHandle(string, sepchars)
//
// This function returns a handle to a list of strings which are
// derived by splitting the string argument at characters found in the
// sepchars string.  If sepchars is empty or null, the strings will be
// separated by white space, so each string in the handle list will be
// a word from the argument string.
//
bool
misc2_funcs::IFstringHandle(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    const char *sepc;
    ARG_CHK(arg_string(args, 1, &sepc))

    if (!string)
        return (BAD);
    stringlist *s0 = 0, *se = 0;
    char buf[512];
    const char *s = string;
    while (*s) {
        int i = 0;
        if (sepc) {
            while (*s && strchr(sepc, *s))
                s++;
        }
        else {
            while (isspace(*s))
                s++;
        }
        if (!*s)
            break;
        if (sepc) {
            while (*s && !strchr(sepc, *s))
                buf[i++] = *s++;
        }
        else {
            while (*s && !isspace(*s))
                buf[i++] = *s++;
        }
        buf[i] = '\0';
        if (!s0)
            s0 = se = new stringlist(lstring::copy(buf), 0);
        else {
            se->next = new stringlist(lstring::copy(buf), 0);
            se = se->next;
        }
    }
    sHdl *hdl = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (stringlist_handle) ListHandle(arglist)
//
// This function creates a list of strings corresponding to the
// variable number of arguments, and returns a handle to the list.
// The arguments are converted to strings in the manner of the Print()
// function, however each argument corresponds to a unique string in
// the list.  The strings are accessed in (left to right) order of the
// arguments.
//
// If no argument is given, a handle to a null string list is created.
// Calls to ListAddFront and/or ListAddBack can be used to add strings
// subsequently.
//
bool
misc2_funcs::IFlistHandle(Variable *res, Variable *args, void*)
{
    stringlist *s0 = prlist(args, false);
    sHdl *hdl = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (string) ListContent(stringlist_handle)
//
// This function returns the string currently referenced by the
// handle, and does *not* increment the handle to the next string in the
// list.  If the handle is not found or contains no further list
// elements, a null string is returned.  The function will fail if the
// handle is not a reference to a list of strings.
//
bool
misc2_funcs::IFlistContent(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        stringlist *s = (stringlist*)hdl->data;
        if (s)
            res->content.string = lstring::copy(s->string);
        if (res->content.string)
            res->flags |= VF_ORIGINAL;
    }
    return (OK);
}


// (int) ListReverse(stringlist_handle)
//
// This function reverses the order of strings in the stringlist
// handle passed.  If the operation succeeds the return value is 1, or
// if the list is empty or an error occurs the value is 0.
//
bool
misc2_funcs::IFlistReverse(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        stringlist *s = (stringlist*)hdl->data;
        if (s) {
            stringlist *s0 = 0;
            while (s) {
                stringlist *sn = s->next;
                s->next = s0;
                s0 = s;
                s = sn;
            }
            hdl->data = s0;
            res->content.value = 0;
        }
    }
    return (OK);
}


// (string) ListNext(stringlist_handle)
//
// This function will return the string at the front of the list
// referenced by the handle, and set the handle to reference the next
// string in the list.  The function will fail if the handle is not a
// reference to a list of strings.  A null string is returned if the
// handle is not found, or after all strings in the list have been
// returned.
//
bool
misc2_funcs::IFlistNext(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        res->content.string = (char*)hdl->iterator();
        if (res->content.string)
            res->flags |= VF_ORIGINAL;
    }
    return (OK);
}


// (int) ListAddFront(stringlist_handle, string)
//
// This function adds string to the front of the list of strings
// referenced by the handle, so that the handle immediately references
// the new string.  The function will fail if the handle is not a
// reference to a string list, or the given string is null.  The
// return value is 1 unless the handle is not found, in which case 0
// is returned.
//
bool
misc2_funcs::IFlistAddFront(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    if (!string)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        hdl->data = new stringlist(lstring::copy(string),
            (stringlist*)hdl->data);
        res->content.value = 1;
    }
    return (OK);
}


// (int) ListAddBack(stringlist_handle, string)
//
// This function adds string to the back of the list of strings
// referenced by the handle, so that the handle references the new
// string after all existing strings have been cycled.  The function
// will fail if the handle is not a reference to a string list, or the
// given string is null.  The return value is 1 unless the handle is
// not found, in which case 0 is returned.
//
bool
misc2_funcs::IFlistAddBack(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    if (!string)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        if (!hdl->data)
            hdl->data = new stringlist(lstring::copy(string), 0);
        else {
            stringlist *s = (stringlist*)hdl->data;
            while (s->next)
                s = s->next;
            s->next = new stringlist(lstring::copy(string), 0);
        }
        res->content.value = 1;
    }
    return (OK);
}


// (int) ListAlphaSort(stringlist_handle)
//
// This function will alphabetically sort the list of strings
// referenced by the handle.  The function will fail if the handle is
// not a reference to a list of strings.  The return value is 1 unless
// the handle is not found, in which case 0 is returned.
//
bool
misc2_funcs::IFlistAlphaSort(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        stringlist *s = (stringlist*)hdl->data;
        if (s && s->next)
            stringlist::sort(s);
        res->content.value = 1;
    }
    return (OK);
}


// (int) ListUnique(stringlist_handle)
//
// This function deletes duplicate strings from the string list
// referenced by the handle, so that strings remaining in the list are
// unique.  The function will fail if the handle is not a reference to
// a list of strings.  The return value is 1 unless the handle is not
// found, in which case 0 is returned.
//
bool
misc2_funcs::IFlistUnique(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        stringlist *s = (stringlist*)hdl->data;
        if (s && s->next) {
            SymTab *st = new SymTab(false, false);
            for ( ; s; s = s->next)
                st->add(s->string, 0, true);
            s = SymTab::names(st);
            delete st;
            stringlist::destroy((stringlist*)hdl->data);
            hdl->data = s;
        }
        res->content.value = 1;
    }
    return (OK);
}


// (string) ListFormatCols(stringlist_handle, columns)
//
// This function returns a string which contains the column formatted
// list of strings referenced by the handle.  The columns argument
// sets the page width in character columns.  This function is useful
// for formatting lists of cell names, for example.  The return is a
// null string if the handle is not found.  The function fails if the
// handle does not reference a list of strings.
//
bool
misc2_funcs::IFlistFormatCols(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int cols;
    ARG_CHK(arg_int(args, 1, &cols))

    if (cols < 10)
        cols = 10;
    else if (cols > 160)
        cols = 160;
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        res->content.string = stringlist::col_format((stringlist*)hdl->data,
            cols);
        if (res->content.string)
            res->flags |= VF_ORIGINAL;
    }
    return (OK);
}


// (string) ListConcat(stringlist_handle, sepchars)
//
// This function returns a string consisting of each string in the
// list referenced by the handle separated by the sepchars string.  If
// the sepchars string is empty or null, there is no separation
// between the strings.  The function will fail if the handle does not
// reference a list of strings.  A null string is returned if the
// handle is not found.
//
bool
misc2_funcs::IFlistConcat(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *sepc;
    ARG_CHK(arg_string(args, 1, &sepc))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        sLstr lstr;
        stringlist *s = (stringlist*)hdl->data;
        while (s) {
            lstr.add(s->string);
            if (sepc && s->next)
                lstr.add(sepc);
            s = s->next;
        }
        res->content.string = lstr.string_trim();
        if (res->content.string)
            res->flags |= VF_ORIGINAL;
    }
    return (OK);
}


// (int) ListIncluded(stringlist_handle, string)
//
// This function compares string to each string in the list referenced by
// the handle and returns 1 if a match is found (case sensitive).  If no
// match, or the handle is not found, 0 is returned.  The function will
// fail if the handle is not a reference to a list of strings.
//
bool
misc2_funcs::IFlistIncluded(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    if (!string)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        stringlist *s = (stringlist*)hdl->data;
        for ( ; s; s = s->next) {
            if (s->string && !strcmp(s->string, string)) {
                res->content.value = 1;
                break;
            }
        }
    }
    return (OK);
}


//-------------------------------------------------------------------------
// String Manipulation and Conversion
//-------------------------------------------------------------------------

// (string) Strcat(string1, string2)
//
// Appends string2 on the end of string1 and returns the new string.
// The `+' operator has been overloaded to also perform this function
// on string operands.
//
bool
misc2_funcs::IFstrcat(Variable *res, Variable *args, void*)
{
    const char *s1;
    ARG_CHK(arg_string(args, 0, &s1))
    const char *s2;
    ARG_CHK(arg_string(args, 1, &s2))

    int l1 = s1 ? strlen(s1) : 0;
    int l2 = s2 ? strlen(s2) : 0;
    char *string = new char[l1 + l2 + 1];
    string[0] = 0;
    if (l1)
        strcpy(string, s1);
    if (l2)
        strcpy(string + l1, s2);
    res->type = TYP_STRING;
    res->content.string = string;
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) Strcmp(string1, string2)
//
// Returns an integer representing the lexical difference between
// string1 and string2.  This is the same as the "strcmp" C library
// function.  The comparison operators have been overloaded to also
// perform this function on string operands.
//
bool
misc2_funcs::IFstrcmp(Variable *res, Variable *args, void*)
{
    const char *s1;
    ARG_CHK(arg_string(args, 0, &s1))
    const char *s2;
    ARG_CHK(arg_string(args, 1, &s2))

    res->type = TYP_SCALAR;
    if (!s1 && !s2)
        res->content.value = 0.0;
    else if (!s1 || !s2)
        res->content.value = 1.0;
    else
        res->content.value = strcmp(s1, s2);
    return (OK);
}


// (int) Strncmp(string1, string2, n)
//
// This compares at most n characters in strings 1 and 2 and returns
// the lexicographic comparison:  1 if string1 is greater than
// string2, 0 if equal, and -1 if string1 is less than string2.  This
// is equivalent to the C library "strncmp" function.
//
bool
misc2_funcs::IFstrncmp(Variable *res, Variable *args, void*)
{
    const char *s1;
    ARG_CHK(arg_string(args, 0, &s1))
    const char *s2;
    ARG_CHK(arg_string(args, 1, &s2))
    int n;
    ARG_CHK(arg_int(args, 2, &n))

    res->type = TYP_SCALAR;
    if (!s1 && !s2)
        res->content.value = 0.0;
    else if (!s1 || !s2)
        res->content.value = 1.0;
    else
        res->content.value = strncmp(s1, s2, n);
    return (OK);
}


// (int) Strcasecmp(string1, string2)
//
// This internally converts strings 1 and 2 to lower case, and returns
// the lexicographic comparison:  1 if string1 is greater than
// string2, 0 if equal, and -1 if string1 is less than string2.  This
// is equivalent to the C library "strcasecmp" function.
//
bool
misc2_funcs::IFstrcasecmp(Variable *res, Variable *args, void*)
{
    const char *s1;
    ARG_CHK(arg_string(args, 0, &s1))
    const char *s2;
    ARG_CHK(arg_string(args, 1, &s2))

    res->type = TYP_SCALAR;
    if (!s1 && !s2)
        res->content.value = 0.0;
    else if (!s1 || !s2)
        res->content.value = 1.0;
    else
        res->content.value = strcasecmp(s1, s2);
    return (OK);
}


// (int) Strncasecmp(string1, string2, n)
//
// This internally converts strings 1 and 2 to lower case, and
// compares at most n characters.  It returns the lexicographic
// comparison:  1 if string1 is greater than string2, 0 if equal, and
// -1 if string1 is less than string2.  This is equivalent to the C
// library "strncasecmp" function.
//
bool
misc2_funcs::IFstrncasecmp(Variable *res, Variable *args, void*)
{
    const char *s1;
    ARG_CHK(arg_string(args, 0, &s1))
    const char *s2;
    ARG_CHK(arg_string(args, 1, &s2))
    int n;
    ARG_CHK(arg_int(args, 2, &n))

    res->type = TYP_SCALAR;
    if (!s1 && !s2)
        res->content.value = 0.0;
    else if (!s1 || !s2)
        res->content.value = 1.0;
    else
        res->content.value = strncasecmp(s1, s2, n);
    return (OK);
}


// (string) Strdup(string)
//
// This returns a new string variable containing a copy of the
// argument's string.  An error occurs if the argument is not
// string-type.  Note that this differs from assignment, which
// propagates a pointer to the string data rather than copying.
//
bool
misc2_funcs::IFstrdup(Variable *res, Variable *args, void*)
{
    const char *s1;
    ARG_CHK(arg_string(args, 0, &s1))

    res->type = TYP_STRING;
    res->content.string = lstring::copy(s1);
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (string) Strtok(str, sep)
//
// The Strtok() function is used to isolate sequential tokens in a
// string, str.  These tokens are separated in the string by at least
// one of the characters in the string sep.  The first time that
// Strtok() is called, str should be specified; subsequent calls,
// wishing to obtain further tokens from the same string, should pass
// 0 instead.  The separator string, sep, must be supplied each time,
// and may change between calls.
//
bool
misc2_funcs::IFstrtok(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    const char *sep;
    ARG_CHK(arg_string(args, 1, &sep))

    if (!sep)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = lstring::copy(strtok(args[0].content.string, sep));
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (string) Strchr(string, char)
//
// The second argument is an integer representing a character.  The
// return value is a pointer into string offset to point to the first
// instance of the character.  If the character is not in the string,
// a null pointer is returned.  This is basically the same as the C
// strchr function.
//
bool
misc2_funcs::IFstrchr(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    int chr;
    ARG_CHK(arg_int(args, 1, &chr))

    res->type = TYP_STRING;
    res->content.string = (char*)strchr(string, chr);
    return (OK);
}


// (string) Strrchr(string, char)
//
// The second argument is an integer representing a character.  The
// return value is a pointer into string offset to point to the last
// instance of the character.  If the character is not in the string,
// a null pointer is returned.  This is basically the same as the C
// strrchr function.
//
bool
misc2_funcs::IFstrrchr(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    int chr;
    ARG_CHK(arg_int(args, 1, &chr))

    res->type = TYP_STRING;
    res->content.string = (char*)strrchr(string, chr);
    return (OK);
}


// (string) Strstr(string, substring)
//
// The second argument is a string which is expected to be a substring
// of the string.  The return value is a pointer into string to the
// start of the first ovvurrence of the substring.  If there are no
// occurrences, a null pointer is returned.  This is equivalent to the
// C strstr function.
//
bool
misc2_funcs::IFstrstr(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    const char *subs;
    ARG_CHK(arg_string(args, 1, &subs))

    res->type = TYP_STRING;
    res->content.string = (char*)strstr(string, subs);
    return (OK);
}


// (string) Strpath(string)
//
// This returns a copy of the file name part of a full path given
// in string.
//
bool
misc2_funcs::IFstrpath(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))

    res->type = TYP_STRING;
    res->content.string = lstring::copy(lstring::strip_path(string));
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) Strlen(string)
//
// Returns the number of characters in the string.
//
bool
misc2_funcs::IFstrlen(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))

    res->type = TYP_SCALAR;
    res->content.value = string ? strlen(string) : 0;
    return (OK);
}


// (int) Sizeof(arg)
//
// This function returns the allocated size of the argument, which is
// mostly useful for determining the size of an array.  The return
// value is
//
//   string length          arg is a string
//   allocated array size   arg is an array
//   number of trapezoids   arg is a zoidlist
//   1                      arg is none of above
//
bool
misc2_funcs::IFsizeof(Variable *res, Variable *args, void*)
{
    int sz;
    if (args[0].type == TYP_SCALAR)
        sz = 1;
    else if (args[0].type == TYP_STRING || args[0].type == TYP_NOTYPE) {
        if (args[0].content.string)
            sz = strlen(args[0].content.string);
        else
            sz = 0;
    }
    else if (args[0].type == TYP_ARRAY)
        sz = args[0].content.a->length();
    else if (args[0].type == TYP_ZLIST) {
        sz = 0;
        Zlist *zl = args[0].content.zlist;
        for ( ; zl; zl = zl->next)
            sz++;
    }
    else if (args[0].type == TYP_LEXPR)
        sz = 1;
    else if (args[0].type == TYP_HANDLE)
        sz = 1;
    else
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = sz;
    return (OK);
}


// (real) ToReal(string)
//
// The returned value is a variable of type "real" containing the
// numeric value from the passed argument, which is a string.  The
// text of the string should be interpretable as a numeric constant.
// If the argument is real, the value is simply copied.
//
bool
misc2_funcs::IFtoReal(Variable *res, Variable *args, void*)
{
    res->type = TYP_SCALAR;
    if (args[0].type == TYP_STRING || args[0].type == TYP_NOTYPE) {
        if (!args[0].content.string)
            res->content.value = 0.0;
        else {
            const char *t = args[0].content.string;
            double *d = SPnum.parse(&t, false);
            if (d)
                res->content.value = *d;
            else
                res->content.value = 0.0;
        }
    }
    else if (args[0].type == TYP_SCALAR)
        res->content.value = args[0].content.value;
    else
        return (BAD);
    return (OK);
}


// (string) ToString(real)
//
// The returned value is a variable of type string containing a text
// representation of the passed variable, which is expected to be of
// type real.  The format is the same as the C printf function with
// "%g" as a format specifier.  If the argument is a string, the
// returned value points to that string.
//
bool
misc2_funcs::IFtoString(Variable *res, Variable *args, void*)
{
    if (args[0].type == TYP_SCALAR) {
        res->type = TYP_STRING;
        char buf[64];
        sprintf(buf, "%g", args[0].content.value);
        res->content.string = lstring::copy(buf);
        res->flags |= VF_ORIGINAL;
    }
    else if ((args[0].type == TYP_STRING || args[0].type == TYP_NOTYPE)) {
        res->type = TYP_STRING;
        res->content.string = args[0].content.string;
    }
    else
        return (BAD);
    return (OK);
}


// (string) ToStringA(real, digits)
//
// This will return a string containing the real number argument in
// "SPICE" format, which is a form consisting of a fixed point number
// followed by alpha character which designates a scale factor.  These
// are the same scale factors as used in the number parser.  though
// "mils" is not used.  The second argument is an integer giving the
// number of digits to print (in the range 2-15).  If out of this
// range, a default of 6 is used.
//
// If the first argument is a string, the string contents will be
// parsed as a number, and the result output as described above.  If
// the parse fails, the number is silently taken as zero.
//
bool
misc2_funcs::IFtoStringA(Variable *res, Variable *args, void*)
{
    int ndgt;
    ARG_CHK(arg_int(args, 1, &ndgt))

    if (args[0].type == TYP_SCALAR) {
        const char *nstr =
            SPnum.printnum(args[0].content.value, "", false, ndgt);
        res->type = TYP_STRING;
        res->content.string = lstring::copy(nstr);
        res->flags |= VF_ORIGINAL;
    }
    else if ((args[0].type == TYP_STRING || args[0].type == TYP_NOTYPE)) {
        const char *t = args[0].content.string;
        double *d = SPnum.parse(&t, false);
        res->type = TYP_STRING;
        if (d) {
            const char *nstr = SPnum.printnum(*d, "", false, ndgt);
            res->content.string = lstring::copy(nstr);
        }
        else {
            if (ndgt < 2)
                ndgt = 6;
            else if (ndgt > 15)
                ndgt = 15;
            char buf[32];
            sprintf(buf, "%.*f", ndgt-1, 0.0);
            res->content.string = lstring::copy(buf);
        }
        res->flags |= VF_ORIGINAL;
    }
    else
        return (BAD);
    return (OK);
}


namespace {
    // Return a list of the substitutions from "printf" fmt.  Each token
    // contains at most one substitution, including the '%' and the
    // substitution character (which ends the token).  Return 0 if error.
    //
    stringlist *
    tokenize_fmt(const char *fmt)
    {
        stringlist *s0 = 0, *se = 0;
        const char *skip = "-.0123456789#+ $*";
        const char *sints = "cdi";
        const char *uints = "ouxX";
        const char *dbls = "eEfgG";

        const char *start = fmt;
        for (const char *s = fmt; *s; s++) {
            if (*s != '%')
                continue;
            s++;
            if (!*s) {
                // error, stray % at end
                stringlist::destroy(s0);
                return (0);
            }
            if (*s == '%')
                continue;
            // start of substitution, skip to type
            while (*s && strchr(skip, *s))
                s++;
            if (!*s) {
                // premature end
                stringlist::destroy(s0);
                return (0);
            }
            if (*s == 's' || strchr(sints, *s) || strchr(uints, *s)
                    || strchr(dbls, *s) || *s == 'p') {

                // found good substitution
                char *tt = new char[s - start + 2];
                strncpy(tt, start, s+1 - start);
                tt[s+1 - start] = 0;
                if (!s0)
                    s0 = se = new stringlist(tt, 0);
                else {
                    se->next = new stringlist(tt, 0);
                    se = se->next;
                }
                start = s+1;
            }
            else {
                // found something unexpected
                stringlist::destroy(s0);
                return (0);
            }
        }
        const char *s = fmt + strlen(fmt);
        if (start < s) {
            char *tt = new char[s - start + 1];
            strncpy(tt, start, s - start);
            tt[s - start] = 0;
            if (!s0)
                s0 = se = new stringlist(tt, 0);
            else {
                se->next = new stringlist(tt, 0);
                se = se->next;
            }
        }
        return (s0);
    }
}


// (string) ToFormat(fmtstring, arg_list)
//
// This function returns a string, formatted in the manner of the C
// printf function.  The first argument is a format string, as would
// be given to printf.  Additional arguments (there can be zero or
// more) are the variables that correspond to the format
// specification.  The type and position of the arguments must match
// the format specification, which means that the variables passed
// must resolve to strings or to numeric scalars.  All of the
// formatting options described in the Unix manual page for printf are
// available, with the following exceptions:
//
//    1) No random argument access.
//    2) At most one '*' per substitution.
//    3) "%p" will always print zero.
//    4) "%n" is not supported.
//
// The function fails if the first argument is not a string, is null,
// or there is a syntax error or unsupported construct, or there is a
// type or number mismatch between specification and arguments.
//
bool
misc2_funcs::IFtoFormat(Variable *res, Variable *args, void*)
{
    const char *fmt;
    ARG_CHK(arg_string(args, 0, &fmt))
    if (!fmt)
        return (BAD);

    const char *sints = "cdi";
    const char *uints = "ouxX";
    const char *dbls = "eEfgG";
    const char *skip = "-.0123456789#+ ";

    int ac = 1;
    sLstr lstr;
    char buf[1024];
    stringlist *s0 = tokenize_fmt(fmt);
    if (!s0)
        return (BAD);
    for (stringlist *sl = s0; sl; sl = sl->next) {
        int ac0 = ac;
        for (char *s = sl->string; *s; s++) {
            if (*s != '%')
                continue;
            s++;
            if (!*s) {
                stringlist::destroy(s0);
                return (BAD);
            }
            if (*s == '%')
                continue;
            while (*s && strchr(skip, *s))
                s++;
            if (!*s) {
                stringlist::destroy(s0);
                return (BAD);
            }

            int fw = -1;
            if (*s == '*') {
                // Can't handle argument number
                if (isdigit(*(s-1)))
                    return (BAD);
                if (isdigit(*(s+1))) {
                    char *t = s+2;
                    while (isdigit(*t))
                        t++;
                    if (*t == '$')
                        return (BAD);
                }
                if (args[ac].type != TYP_SCALAR)
                    return (BAD);
                fw = (int)args[ac].content.value;
                ac++;
                s++;
                while (*s && strchr(skip, *s))
                    s++;
                if (!*s) {
                    stringlist::destroy(s0);
                    return (BAD);
                }
            }

            if (strchr(sints, *s)) {
                if (args[ac].type != TYP_SCALAR) {
                    stringlist::destroy(s0);
                    return (BAD);
                }
                if (fw > 0)
                    sprintf(buf, sl->string, fw, (int)args[ac].content.value);
                else
                    sprintf(buf, sl->string, (int)args[ac].content.value);
                ac++;
                break;
            }
            if (strchr(uints, *s)) {
                if (args[ac].type != TYP_SCALAR) {
                    stringlist::destroy(s0);
                    return (BAD);
                }
                if (fw > 0)
                    sprintf(buf, sl->string, fw,
                        (unsigned int)args[ac].content.value);
                else
                    sprintf(buf, sl->string,
                        (unsigned int)args[ac].content.value);
                ac++;
                break;
            }
            if (strchr(dbls, *s)) {
                if (args[ac].type != TYP_SCALAR) {
                    stringlist::destroy(s0);
                    return (BAD);
                }
                if (fw > 0)
                    sprintf(buf, sl->string, fw, args[ac].content.value);
                else
                    sprintf(buf, sl->string, args[ac].content.value);
                ac++;
                break;
            }
            if (*s == 's') {
                if (args[ac].type != TYP_STRING &&
                        args[ac].type != TYP_NOTYPE) {
                    stringlist::destroy(s0);
                    return (BAD);
                }
                if (fw > 0)
                    sprintf(buf, sl->string, fw, args[ac].content.string);
                else
                    sprintf(buf, sl->string, args[ac].content.string);
                ac++;
                break;
            }
            if (*s == 'p') {
                if (fw > 0)
                    sprintf(buf, sl->string, fw, (char*)0);
                else
                    sprintf(buf, sl->string, (char*)0);
                ac++;
                break;
            }
            stringlist::destroy(s0);
            return (BAD);
        }
        if (ac == ac0)
            strcpy(buf, sl->string);
        lstr.add(buf);
    }
    stringlist::destroy(s0);
    res->type = TYP_STRING;
    res->content.string = lstr.string_trim();
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (string) ToChar(num)
//
// This function takes as its input an integer value for a character,
// and returns a string containing a printable representation of the
// character.  A null string is returned if the input is not a valid
// character index.  This function can be used to preformat character
// data for printing with the other various functions.
//
bool
misc2_funcs::IFtoChar(Variable *res, Variable *args, void*)
{
    int num;
    ARG_CHK(arg_int(args, 0, &num))

    res->type = TYP_STRING;
    res->content.string = 0;
    if (num >= 0 && num <= 255) {
        char s[2];
        s[0] = num;
        s[1] = 0;
        res->content.string = SIparse()->toPrintable(s);
        if (res->content.string)
            res->flags |= VF_ORIGINAL;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Current Directory
//-------------------------------------------------------------------------

// Cwd(path)
//
// This function changes the current working directory to that given
// by the argument.  If path is null or empty, the change will be to
// the user's home directory.  A tilde character ('~') appearing in
// path is expanded to the user's home directory as in a UNIX shell.
// The return is 1 if the change succeeds, 0 otherwise.
//
bool
misc2_funcs::IFcwd(Variable *res, Variable *args, void*)
{
    static char defpath[] = "~";
    const char *path;
    ARG_CHK(arg_string(args, 0, &path))

    if (!path || !*path)
        path = defpath;
    path = pathlist::expand_path(path, false, true);
    res->type = TYP_SCALAR;
    res->content.value = chdir(path) ? 0 : 1;
    delete [] path;
    return (OK);
}


// (string) Pwd()
//
// This function returns a string containing the absolute path to the
// current directory.
//
bool
misc2_funcs::IFpwd(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = FIO()->PGetCWD(0, 256);
    if (!res->content.string)
        return (BAD);
    res->flags |= VF_ORIGINAL;
    return (OK);
}


//-------------------------------------------------------------------------
// Date and Time
//-------------------------------------------------------------------------

// (string) DateString()
//
// This function returns a string containing the date and time in
// the format
//
//  Tue Jun 12 23:42:38 PDT 2001
//
bool
misc2_funcs::IFdateString(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = lstring::copy(miscutil::dateString());
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) Time()
//
// This returns a system time value, which can be converted to more
// useful output by TimeToString or TimeToVals.  Actually, the
// returned value is the number of seconds since the start of the
// year 1970.
//
bool
misc2_funcs::IFtime(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = time(0);
    return (OK);
}


// (int) MakeTime(array, gmt)
//
// This function takes the time fields specified in the array and
// returns a time value is if returned from Time.  If the boolean
// argument gmt is nonzero, the interpretation is GMT, otherwise local
// time.  The array must be size 9 or larger, with the values set as
// when returned by the TimeToVals function (below).
//
// Under Windows, the gmt argument is ignored and local time is used.
//
bool
misc2_funcs::IFmakeTime(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array(args, 0, &vals, 9))
    bool gmt;
    ARG_CHK(arg_boolean(args, 1, &gmt))

    struct tm tm;
    tm.tm_sec   = (int)vals[0];
    tm.tm_min   = (int)vals[1];
    tm.tm_hour  = (int)vals[2];
    tm.tm_mday  = (int)vals[3];
    tm.tm_mon   = (int)vals[4];
    tm.tm_year  = (int)vals[5];
    tm.tm_wday  = (int)vals[6];
    tm.tm_yday  = (int)vals[7];
    tm.tm_isdst = (int)vals[8];

    res->type = TYP_SCALAR;
#ifndef WIN32
    if (gmt)
        res->content.value = timegm(&tm);
    else
#endif
        res->content.value = mktime(&tm);
    return (OK);
}


// (string) TimeToString(time, gmt)
//
// Given a time value as returned from Time, this returns a string
// in the form
//
//  Tue Jun 12 23:42:38 PDT 2001
//
// If the boolean argument gmt is nonzero, GMT will be used,
// otherwise the local time is used.
//
bool
misc2_funcs::IFtimeToString(Variable *res, Variable *args, void*)
{
    int tv;
    ARG_CHK(arg_int(args, 0, &tv))
    bool gmt;
    ARG_CHK(arg_boolean(args, 1, &gmt))

    time_t tt = tv;
    struct tm *tm = gmt ? gmtime(&tt) : localtime(&tt);

    res->type = TYP_STRING;
    res->content.string = lstring::copy(asctime(tm));
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (string) TimeToVals(time, gmt, array)
//
// Given a time value as returned from Time, this breaks out the
// time/date into the array.  The array must have size 9 or larger. 
// If the boolean argument gmt is nonzero, GMT is used, otherwise
// local time is used.
//
// The array values are set as follows.
//
// array[0]     seconds (0 - 59).
// array[1]     minutes (0 - 59).
// array[2]     hours (0 - 23).
// array[3]     day of month (1 - 31).
// array[4]     month of year (0 - 11).
// array[5]     year - 1900.
// array[6]     day of week (Sunday = 0).
// array[7]     day of year (0 - 365).
// array[8]     1 if summer time is in effect, or 0.
//
// The return value is a string containing an abbreviation of the
// local timezone name, except under Windows where the return is
// an empty string.
//
bool
misc2_funcs::IFtimeToVals(Variable *res, Variable *args, void*)
{
    int tv;
    ARG_CHK(arg_int(args, 0, &tv))
    bool gmt;
    ARG_CHK(arg_boolean(args, 1, &gmt))
    double *vals;
    ARG_CHK(arg_array(args, 2, &vals, 9))

    time_t tt = tv;
    struct tm *tm = gmt ? gmtime(&tt) : localtime(&tt);
    vals[0] = tm->tm_sec;
    vals[1] = tm->tm_min;
    vals[2] = tm->tm_hour;
    vals[3] = tm->tm_mday;
    vals[4] = tm->tm_mon;
    vals[5] = tm->tm_year;
    vals[6] = tm->tm_wday;
    vals[7] = tm->tm_yday;
    vals[8] = tm->tm_isdst;

    res->type = TYP_STRING;
#ifdef WIN32
    res->content.string = lstring::copy("");
#else
    res->content.string = lstring::copy(tm->tm_zone);
#endif
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (real) MilliSec()
//
// Return the elapsed time in milliseconds since midnight January 1,
// 1970 GMT.
//
bool
misc2_funcs::IFmilliSec(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
#ifdef HAVE_GETTIMEOFDAY
    struct timezone tz;
    struct timeval tv;
    gettimeofday(&tv, &tz);
    res->content.value = tv.tv_sec*1000.0 + tv.tv_usec/1000.0;
#else
#ifdef HAVE_FTIME
    struct timeb tb;
    ftime(&tb);
    res->content.value = 1000.0*tb.time + tb.millitm;
#endif
#endif
    return (OK);
}


// (int) StartTiming(array)
//
// This will initialize the values in the array, which must have size 3 or
// larger, for later use by the StopTiming function.  The return value is
// always 1.
//
bool
misc2_funcs::IFstartTiming(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array(args, 0, &vals, 3))

    Tvals tv;
    tv.start();
    vals[0] = tv.realTime();
    vals[1] = tv.userTime();
    vals[2] = tv.systemTime();
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) StopTiming(array)
//
// This will place time differences (in seconds) into the array, since
// the last call to StartTiming (with the same argument).  The array
// must have size 3 or larger.  the components are:
//  0    Elapsed wall-clock time
//  1    Elapsed user time
//  2    Elapsed system time
// The user time is the time the cpu spent executing in user mode.
// The system time is the time spent in the system executing on behalf
// of the process.  This uses the UNIX getrusage or times system
// calls, which may not be available on all systems.  If support is
// not available, e.g., in Windows, the user and system entries will
// be zero, but the wall-clock time is valid.  This function always
// returns 1.
//
bool
misc2_funcs::IFstopTiming(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array(args, 0, &vals, 3))

    Tvals tv;
    tv.start();
    vals[0] = tv.realTime() - vals[0];
    vals[1] = tv.userTime() - vals[1];
    vals[2] = tv.systemTime() - vals[2];
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}

//-------------------------------------------------------------------------
// File System Interface
//-------------------------------------------------------------------------

// (string) Glob(pattern)
//
// This function returns a string which is a filename expansion of the
// pattern string, in the manner of the C-shell.  The pattern can
// contain the usual substitution characters *, ?, [], {}.
//
bool
misc2_funcs::IFglob(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))

    if (!string || !*string)
        return (BAD);
    char *s = lstring::build_str(0, "csh -c \"echo ");
    s = lstring::build_str(s, string);
    s = lstring::build_str(s, "\"");
    FILE *pp = popen(s, "r");
    delete [] s;
    res->type = TYP_STRING;
    if (pp) {
        sLstr lstr;
        int c;
        while ((c = getc(pp)) != EOF)
            lstr.add_c(c);
        pclose(pp);
        res->content.string = lstr.string_trim();
        if (res->content.string)
            res->flags |= VF_ORIGINAL;
    }
    else
        return (BAD);
    return (OK);
}


// (file_handle) Open(file, mode)
//
// This command opens the file given as a string argument according to
// the string mode, and returns a file descriptor.  The mode string
// should consist of a single character:  'r' for reading, 'w' to
// write, or 'a' to append.  If the returned value is negative, an
// error occurred.
//
bool
misc2_funcs::IFopen(Variable *res, Variable *args, void*)
{
    const char *file;
    ARG_CHK(arg_string(args, 0, &file))
    const char *mode;
    ARG_CHK(arg_string(args, 1, &mode))

    if (!file || !*file)
        return (BAD);
    if (!mode)
        return (BAD);
    res->type = TYP_SCALAR;
    int flags;
    if (*mode == 'r')
        flags = O_RDONLY;
    else if (*mode == 'w')
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    else if (*mode == 'a')
        flags = O_WRONLY | O_CREAT | O_APPEND;
    else
        return (BAD);

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    int fd = open(file, flags, 0644);
    if (fd > 0) {
        new sHdlFd(fd, mode);
        res->type = TYP_HANDLE;
    }
    res->content.value = fd;
    return (OK);
}


// (file_handle) Popen(comand, mode)
//
// This command opens a pipe to the shell command given as the first
// argument, and returns a file handle that can be used to read and/or
// write to the process.  The handle should be closed with the Close()
// function.  This is a wrapper around the C library popen() command
// so has the same limitations as the local version of that command.
// In particular, on some systems the mode may be reading or writing,
// but not both.  The function will fail if either argument is null
// or if the popen() call fails.
//
bool
misc2_funcs::IFpopen(Variable *res, Variable *args, void*)
{
    const char *cmd;
    ARG_CHK(arg_string(args, 0, &cmd))
    const char *mode;
    ARG_CHK(arg_string(args, 1, &mode))

    if (!cmd || !*cmd)
        return (BAD);
    if (!mode)
        return (BAD);

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    FILE *fp = popen(cmd, mode);
    if (!fp)
        return (BAD);
    new sHdlFd(fp, mode);
    res->type = TYP_HANDLE;
    res->content.value = fileno(fp);
    return (OK);
}


// (skt_handle) Sopen(host, port)
//
// This function opens a "socket" which is a communications channel to
// the given host and port.  If the host string is null or empty, the
// local host is assumed.  The port number must be provided, there is
// no default.  If the open is successful, the return value is an
// integer larger than zero and is a handle that can be used in any of
// the read/write functions that accept a file handle.  The Close()
// function should be called on the handle when the interaction is
// complete.  If the connection fails, a negative number is returned.
// The function fails if there is a major error, such as no BSD
// sockets support.
//
bool
misc2_funcs::IFsopen(Variable *res, Variable *args, void*)
{
    static char defhost[] = "localhost";
    const char *hostname;
    ARG_CHK(arg_string(args, 0, &hostname))
    int port;
    ARG_CHK(arg_int(args, 1, &port))

    if (!hostname || !*hostname)
        hostname = defhost;
#ifdef WIN32
    // initialize winsock
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        Errs()->add_error(
            "Windows Socket Architecture initialization failed.");
        return (BAD);
    }
#endif

    hostent *hent = gethostbyname(hostname);
    if (!hent) {
        Errs()->sys_herror("gethostbyname");
        return (BAD);
    }

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        Errs()->sys_error("socket");
        return (BAD);
    }
    sockaddr_in skt;
    memset(&skt, 0, sizeof(sockaddr_in));
    memcpy(&skt.sin_addr, hent->h_addr, hent->h_length);
    skt.sin_family = AF_INET;
    skt.sin_port = htons(port);
    if (connect(sd, (sockaddr*)&skt, sizeof(skt)) < 0) {
        Errs()->sys_error("connect");
        CLOSESOCKET(sd);
        res->type = TYP_SCALAR;
        res->content.value = -1;
        return (OK);
    }
    new sHdlFd(sd, 0);
    res->type = TYP_HANDLE;
    res->content.value = sd;
    return (OK);
}


// (string) ReadLine(maxlen, file_handle)
//
// The ReadLine() function returns a string with length up to maxlen
// filled with characters read from file_handle.  The file_handle must
// have been successfully opened for reading by a call to Open(), The
// read is terminated by end of file, a return character, or a NULL
// byte.  The terminating character is not included in the string.  A
// null string is returned when the end of file is reached, or if the
// handle is not found.  The function will fail if the handle is not a
// file handle, or maxlen is less than 1.
//
bool
misc2_funcs::IFreadLine(Variable *res, Variable *args, void*)
{
    int len;
    ARG_CHK(arg_int(args, 0, &len))
    int id;
    ARG_CHK(arg_handle(args, 1, &id))

    if (len < 1)
        return (BAD);
    if (id < 0)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLfd)
            return (BAD);

        char *t = new char[len+1];
        char *s = t;
        for (;;) {
            char c;
            int i = (hdl->data ? read(id, &c, 1) : recv(id, &c, 1, 0));
            if (i == 0) {
                if (s == t) {
                    delete [] t;
                    t = 0;
                }
                break;
            }
            if (i == -1) {
                if (errno == EINTR)
                    continue;
                delete [] t;
                return (OK);
            }
            if (c == '\n' || !c)
                break;
            *s++ = c;
            if (s - t == len)
                break;
        }
        if (t) {
            *s = '\0';
            res->content.string = lstring::copy(t);
            delete [] t;
            if (res->content.string)
                res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) ReadChar(file_handle)
//
// The ReadChar() function returns a single character read from
// file_handle, which must have been successfully opened for reading
// with an Open() call.  The function returns EOF (-1) when the end of
// file is reached, or the handle is not found.  The function will
// fail if the handle is not a file handle,
//
bool
misc2_funcs::IFreadChar(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    if (id < 0)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = EOF;
    if (hdl) {
        if (hdl->type != HDLfd)
            return (BAD);
        char c = 0;
        for (;;) {
            int i = (hdl->data ? read(id, &c, 1) : recv(id, &c, 1, 0));
            if (i == 1)
                res->content.value = c;
            else if (i == -1 && errno == EINTR)
                continue;
            break;
        }
    }
    return (OK);
}


// (int) WriteLine(string, file_handle)
//
// The WriteLine() function writes the content of string to
// file_handle, which must have been successfully opened for writing
// or appending with an Open() call.  The number of characters written
// is returned.  The function will fail if the handle is not a file
// handle, or the string is null.
//
// This function has the unusual property that it will accept the
// arguments in reverse order.
//
bool
misc2_funcs::IFwriteLine(Variable *res, Variable *args, void*)
{
    const char *buf;
    int id;
    if (args[0].type == TYP_HANDLE) {
        ARG_CHK(arg_handle(args, 0, &id))
        ARG_CHK(arg_string(args, 1, &buf))
    }
    else {
        ARG_CHK(arg_string(args, 0, &buf))
        ARG_CHK(arg_handle(args, 1, &id))
    }

    if (!buf)
        return (BAD);
    if (id < 0)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLfd)
            return (BAD);
        int len = strlen(buf);
        int slen = len;
        if (len) {
            for (;;) {
                int i = (hdl->data ?
                    write(id, buf, len) : send(id, buf, len, 0));
                if (i == -1) {
                    if (errno == EINTR)
                        continue;
                    return (OK);
                }
                if (i == len)
                    break;
                len -= i;
                buf += i;
            }
        }
        res->type = TYP_SCALAR;
        res->content.value = slen;
    }
    return (OK);
}


// (int) WriteChar(c, file_handle)
//
// This function writes a single character c to file_handle, which
// must have been successfully opened for writing or appending with a
// call to Open().  The function returns 1 on success.  The function
// will fail if the handle is not a file handle, or the integer value
// of c is not in the range 0-255.
//
// This function has the unusual property that it will accept the
// arguments in reverse order.
//
bool
misc2_funcs::IFwriteChar(Variable *res, Variable *args, void*)
{
    int c;
    int id;
    if (args[0].type == TYP_HANDLE) {
        ARG_CHK(arg_handle(args, 0, &id))
        ARG_CHK(arg_int(args, 1, &c))
    }
    else {
        ARG_CHK(arg_int(args, 0, &c))
        ARG_CHK(arg_handle(args, 1, &id))
    }

    if (c < 0 || c > 255)
        return (BAD);
    if (id < 0)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLfd)
            return (BAD);
        char b = c;
        for (;;) {
            int i = (hdl->data ? write(id, &b, 1) : send(id, &b, 1, 0));
            if (i == 1)
                res->content.value = 1;
            else if (i == -1 && errno == EINTR)
                continue;
            break;
        }
    }
    return (OK);
}


// (string) TempFile(prefix)
//
// This function creates a unique temporary file name using the prefix
// string given, and arranges for the file of that name to be deleted
// when the program terminates.  The file is not actually created.
// The return from this command is passed to the Open() command to
// actually open the file for writing.
//
bool
misc2_funcs::IFtempFile(Variable *res, Variable *args, void*)
{
    static char defpref[] = "xic";
    const char *pref;
    ARG_CHK(arg_string(args, 0, &pref))

    if (!pref)
        pref = defpref;
    res->type = TYP_STRING;
    res->content.string = filestat::make_temp(pref);
    filestat::queue_deletion(res->content.string);
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (stringlist_handle) ListDirectory(directory, filter)
//
// This function returns a handle to a list of names of files and/or
// directories in the given directory.  If the directory argument is
// null or empty, the current directory is understood.  If the filter
// string is null or empty, all files and subdirectories will be
// listed.  Otherwise the filter string can be "f" in which case only
// regular files will be listed, or "d" in which case only directories
// will be listed.  If the directory does not exist or can't be read,
// 0 is returned, otherwise the return value is a handle to a list of
// strings.
bool
misc2_funcs::IFlistDirectory(Variable *res, Variable *args, void*)
{
    static char defdir[] = ".";
    const char *dir;
    ARG_CHK(arg_string(args, 0, &dir))
    const char *mode;
    ARG_CHK(arg_string(args, 1, &mode))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (!dir || !*dir)
        dir = defdir;
    char buf[256];
    sprintf(buf, "%s/", dir);
    char *s = buf + strlen(buf);
    DIR *wdir = opendir(dir);
    if (dir) {
        stringlist *s0 = 0;
        struct direct *de;
        while ((de = readdir(wdir)) != 0) {
            if (mode && *mode) {
                strcpy(s, de->d_name);
                GFTtype rt = filestat::get_file_type(buf);
                if (rt == GFT_FILE) {
                    if (!strchr(mode, 'f'))
                        continue;
                }
                else if (rt == GFT_DIR) {
                    if (!strchr(mode, 'd'))
                        continue;
                }
                else
                    continue;
            }
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;
            s0 = new stringlist(lstring::copy(de->d_name), s0);
        }
        closedir(wdir);
        stringlist::sort(s0);
        sHdl *hdl = new sHdlString(s0);
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    return (OK);
}


// (int) MakeDir(path)
//
// This function will create a directory, if it doesn't already exist. 
// If the path specifies a multi-component path, all parent
// directories needed will be created.  The function will fail if a
// null or empty path is passed, otherwise the return value is 1 if no
// errors, 0 otherwise with a message available from GetError. 
// Passing the name of an existing directory is not an error.
//
bool
misc2_funcs::IFmakeDir(Variable *res, Variable *args, void*)
{
    const char *path;
    ARG_CHK(arg_string(args, 0, &path))

    if (!path || !*path) {
        Errs()->add_error("MakeDir: null or empty path.");
        return (BAD);
    }

    res->type = TYP_SCALAR;
    res->content.value = 0;
    char *pbuf = pathlist::expand_path(path, true, true);
    const char *s = pbuf;
    if (lstring::is_rooted(s))
        s = lstring::strdirsep(s) + 1;

    for (;;) {
        const char *t = lstring::strdirsep(s);
        if (t) {
            int n = t - pbuf;
            char c = pbuf[n];
            pbuf[n] = 0;
            struct stat st;
            if (stat(pbuf, &st) < 0) {
#ifdef WIN32
                if (mkdir(pbuf) < 0) {
#else
                if (mkdir(pbuf, 0755) < 0) {
#endif
                    Errs()->sys_error("MakeDir");
                    break;
                }
            }
            else if (!S_ISDIR(st.st_mode)) {
                Errs()->add_error(
                    "MakeDir: path component not a directory.");
                break;
            }
            pbuf[n] = c;
            s = t+1;
        }
        else {
            struct stat st;
            if (stat(pbuf, &st) < 0) {
#ifdef WIN32
                if (mkdir(pbuf) < 0) {
#else
                if (mkdir(pbuf, 0755) < 0) {
#endif
                    Errs()->sys_error("MakeDir");
                    break;
                }
            }
            else if (!S_ISDIR(st.st_mode)) {
                Errs()->add_error(
                    "MakeDir: path component not a directory.");
                break;
            }
            res->content.value = 1.0;
            break;
        }
    }
    delete [] pbuf;
    return (OK);
}


// (int) FileStat(path, array)
//
// This function returns 1 if the file in path exists, and fills in
// some data about the file (or directory).  If the file does not
// exist, 0 is returned, and the array is untouched.
//
// The array must have size 7 or larger or a value 0 can be passed for
// this argument.  In this case, no statistics are returned, but the
// function return still indicates file existence.
//
// If an array is passed and the path points to an existing file or
// directory, the array is filled in as follows:
//
// array[0]
//    Set to 0 if path is a regular file.
//    Set to 1 if path is a directory.
//    Set to 2 if path is some other type of object.
//
// array[1]
//    The size of the regular file in bytes, undefined if not a regular
//    file.
//
// array[2]
//    Set to 1 if the present process has read access to the file, 0
//    otherwise.
//
// array[3]
//    Set to 1 if the present process has write access to the file, 0
//    otherwise.
//
// array[4]
//    Set to 1 if the present process has execute permission to the file,
//    0 otherwise.
//
// array[5]
//    Set to the user id of the file owner.
//
// array[6]
//    Set to the last modification time.  This is in a system-encoded
//    form, use TimeToString or TimeToVals to convert.
//
bool
misc2_funcs::IFfileStat(Variable *res, Variable *args, void*)
{
    const char *path;
    ARG_CHK(arg_string(args, 0, &path))
    double *vals;
    ARG_CHK(arg_array_if(args, 1, &vals, 7))

    if (!path || !*path) {
        Errs()->add_error("FileStat: null or empty path.");
        return (BAD);
    }

    res->type = TYP_SCALAR;
    res->content.value = 0;
    char *pbuf = pathlist::expand_path(path, true, true);
    struct stat st;
    if (stat(pbuf, &st) == 0) {
        res->content.value = 1.0;
        if (vals) {
            if (S_ISREG(st.st_mode))
                vals[0] = 0.0;
            else if (S_ISDIR(st.st_mode))
                vals[0] = 1.0;
            else
                vals[0] = 2.0;
            vals[1] = st.st_size;
            vals[2] = (access(pbuf, R_OK) == 0 ? 1.0 : 0.0);
            vals[3] = (access(pbuf, W_OK) == 0 ? 1.0 : 0.0);
            vals[4] = (access(pbuf, X_OK) == 0 ? 1.0 : 0.0);
            vals[5] = st.st_uid;
            vals[6] = st.st_mtime;
        }
    }

    delete [] pbuf;
    return (OK);
}


// (int) DeleteFile(path)
//
// Delete the file or directory given in path.  If a directory, it
// must be empty.  If the file or directory does not exist or was
// successfully deleted, 1 is returned, otherwise 0 is returned with
// an error message available from GetError.
//
bool
misc2_funcs::IFdeleteFile(Variable *res, Variable *args, void*)
{
    const char *path;
    ARG_CHK(arg_string(args, 0, &path))

    if (!path || !*path) {
        Errs()->add_error("DeleteFile: null or empty path.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = 0;
    char *pbuf = pathlist::expand_path(path, true, true);
    struct stat st;
    if (stat(pbuf, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            if (rmdir(pbuf) < 0)
                Errs()->sys_error("DeleteFile");
            else
                res->content.value = 1.0;
        }
        else {
            if (unlink(pbuf) < 0)
                Errs()->sys_error("DeleteFile");
            else
                res->content.value = 1.0;
        }
    }
    else
        res->content.value = 1.0;
    delete [] pbuf;
    return (OK);
}


// (int) MoveFile(from_path, to_path)
//
// Move (rename) the file from_path to a new file to_path.  On
// success, 1 is returned, otherwise 0 is returned with an error
// message available from GetError.
//
// Except under Windows, directories can be moved as well, but only
// within the same file system.
//
bool
misc2_funcs::IFmoveFile(Variable *res, Variable *args, void*)
{
    const char *fpath;
    ARG_CHK(arg_string(args, 0, &fpath))
    const char *tpath;
    ARG_CHK(arg_string(args, 1, &tpath))

    if (!fpath || !*fpath || !tpath || !*tpath) {
        Errs()->add_error("MoveFile: null or empty path.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = 0;
    char *fbuf = pathlist::expand_path(fpath, true, true);
    char *tbuf = pathlist::expand_path(tpath, true, true);
    if (!fbuf || !tbuf) {
        delete [] fbuf;
        delete [] tbuf;
        return (BAD);
    }
    if (!strcmp(fbuf, tbuf)) {
        delete [] fbuf;
        delete [] tbuf;
        return (OK);
    }
    struct stat fst, tst;
    if (stat(fbuf, &fst) == 0 && stat(tbuf, &tst) == 0) {
        if (fst.st_dev == tst.st_dev && fst.st_ino == tst.st_ino) {
            // Source and destination are the same.
            delete [] fbuf;
            delete [] tbuf;
            return (OK);
        }
    }

    if (access(fbuf, W_OK) == 0) {
        if (unlink(tbuf) < 0 && errno != ENOENT)
            Errs()->sys_error("MoveFile");
        else {
            FILE *tfp = large_fopen(tbuf, "wb");
            if (!tfp)
                Errs()->sys_error("MoveFile");
            else {
                fclose(tfp);
                if (stat(tbuf, &tst) < 0)
                    Errs()->sys_error("MoveFile");
                else {
#ifdef WIN32
// Windows doesn't have link.
                    if (0) {
#else
                    if (fst.st_dev == tst.st_dev) {
                        // Same device, can use link/unlink.  Works for
                        // directories.
                        if (unlink(tbuf) < 0)
                            Errs()->sys_error("MoveFile");
                        else {
                            if (link(fbuf, tbuf) < 0)
                                Errs()->sys_error("MoveFile");
                            else {
                                if (unlink(fbuf) < 0)
                                    Errs()->sys_error("MoveFile");
                                else
                                    res->content.value = 1.0;
                            }
                        }
#endif
                    }
                    else if (S_ISREG(fst.st_mode)) {
                        // Different device, copy bytes.
                        FILE *ffp = large_fopen(fbuf, "rb");
                        if (!ffp)
                            Errs()->sys_error("MoveFile");
                        else {
                            tfp = large_fopen(tbuf, "wb");
                            if (!tfp)
                                Errs()->sys_error("MoveFile");
                            else {
                                int c;
                                bool failed = false;
                                while ((c = getc(ffp)) != EOF) {
                                    if (putc(c, tfp) == EOF) {
                                        Errs()->sys_error("MoveFile");
                                        failed = true;
                                        break;
                                    }
                                }
                                fclose(tfp);
                                if (failed)
                                    unlink(tbuf);
                                else if (unlink(fbuf) == 0)
                                    res->content.value = 1.0;
                            }
                            fclose(ffp);
                        }
                    }
                    else
                        Errs()->add_error("MoveFile: not a regular file.");
                }
            }
        }
    }
    else
        Errs()->add_error("MoveFile: file not found or permission denied.");

    delete [] fbuf;
    delete [] tbuf;
    return (OK);
}


// (int) CopyFile(from_path, to_path)
//
// Copy the file from_path to a new file to_path.  On success, 1 is
// returned, otherwise 0 is returned with an error message available
// from GetError.
//
bool
misc2_funcs::IFcopyFile(Variable *res, Variable *args, void*)
{
    const char *fpath;
    ARG_CHK(arg_string(args, 0, &fpath))
    const char *tpath;
    ARG_CHK(arg_string(args, 1, &tpath))

    if (!fpath || !*fpath || !tpath || !*tpath) {
        Errs()->add_error("CopyFile: null or empty path.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = 0;
    char *fbuf = pathlist::expand_path(fpath, true, true);
    char *tbuf = pathlist::expand_path(tpath, true, true);
    if (!fbuf || !tbuf) {
        delete [] fbuf;
        delete [] tbuf;
        return (BAD);
    }
    if (!strcmp(fbuf, tbuf)) {
        delete [] fbuf;
        delete [] tbuf;
        return (OK);
    }
    struct stat fst, tst;
    if (stat(fbuf, &fst) == 0) {
        if (!S_ISREG(fst.st_mode)) {
            // Not a regular file.
            Errs()->add_error("CopyFile: not a regular file.");
            delete [] fbuf;
            delete [] tbuf;
            return (OK);
        }
        if (stat(tbuf, &tst) == 0) {
            if (fst.st_dev == tst.st_dev && fst.st_ino == tst.st_ino) {
                // Source and destination are the same.
                delete [] fbuf;
                delete [] tbuf;
                res->content.value = 1.0;
                return (OK);
            }
        }
    }

    if (access(fbuf, R_OK) == 0) {
        FILE *ffp = large_fopen(fbuf, "rb");
        if (!ffp)
            Errs()->sys_error("CopyFile");
        else {
            FILE *tfp = large_fopen(tbuf, "wb");
            if (!tfp)
                Errs()->sys_error("CopyFile");
            else {
                int c;
                bool failed = false;
                while ((c = getc(ffp)) != EOF) {
                    if (putc(c, tfp) == EOF) {
                        Errs()->sys_error("CopyFile");
                        failed = true;
                        break;
                    }
                }
                fclose(tfp);
                if (failed)
                    unlink(tbuf);
                else
                    res->content.value = 1.0;
            }
        }
    }
    else
        Errs()->add_error("CopyFile: file not found or permission denied.");

    delete [] fbuf;
    delete [] tbuf;
    return (OK);
}


// (int) CreateBak(path)
//
// If the path file exists, rename it, suffixing the name with a
// ".bak" extension.  If a file with this name already exists, it will
// be overwritten.  The function returns 1 if the file was moved or
// doesn't exist, 0 otherwise, with an error message available from
// GetError.
//
bool
misc2_funcs::IFcreateBak(Variable *res, Variable *args, void*)
{
    const char *path;
    ARG_CHK(arg_string(args, 0, &path))

    if (!path || !*path) {
        Errs()->add_error("CreateBak: null or empty path.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    char *pbuf = pathlist::expand_path(path, true, true);
    if (!pbuf)
        return (BAD);
    res->content.value = 0;
    if (!filestat::create_bak(path))
        Errs()->add_error(filestat::error_msg());
    else
        res->content.value = 1.0;
    delete [] path;
    return (OK);
}


// (string) Md5Digest(filepath)
//
// Return a string containing an MD5 digest for the file whose path is
// passed as the argument.  This is the same digest as returned from
// the !md5 command, and from "openssl dgst -md5 filepath" on many
// Linux-like systems.
//
// If the file can not be opened, an empty string is returned, and an
// error message is available from GetError.
//
bool
misc2_funcs::IFmd5Digest(Variable *res, Variable *args, void*)
{
    const char *path;
    ARG_CHK(arg_string(args, 0, &path))

    if (!path || !*path) {
        Errs()->add_error("Md5Digest: null or empty path.");
        return (BAD);
    }
    char *digest = PC()->md5Digest(path);
    res->type = TYP_STRING;
    res->content.string = digest ? digest : lstring::copy("");
    res->flags |= VF_ORIGINAL;
    return (OK);
}


//-------------------------------------------------------------------------
// Xic Client/Server Interface
//-------------------------------------------------------------------------

// (string) ReadData(size, skt_handle)
//
// This function will read exactly size bytes from a socket, and
// return string-type data containing the bytes read.  The skt_handle
// must be a socket handle returned from Sopen.  The function will
// fail (halt the script) only if the size argument is not an integer.
// On error, a null string is returned, and a message is available
// from GetError.
//
// Note that the string can contain binary data, and if reading an
// ascii string be sure to include the null termination byte.  With
// binary data, the standard string manipulations may not work, and in
// fact can easily cause a program crash.
//
bool
misc2_funcs::IFreadData(Variable *res, Variable *args, void*)
{
    int size;
    ARG_CHK(arg_int(args, 0, &size))

    res->type = TYP_STRING;
    res->content.string = 0;

    int id = 0;
    if (!arg_handle(args, 1, &id))
        return (OK);
    if (id < 0) {
        Errs()->add_error("bad handle argument");
        return (OK);
    }

    sHdl *hdl = sHdl::get(id);
    if (!hdl) {
        Errs()->add_error("bad handle");
        return (OK);
    }
    if (hdl->type != HDLfd) {
        Errs()->add_error("bad handle type");
        return (OK);
    }
    if (hdl->data) {
        Errs()->add_error("handle not a socket");
        return (OK);
    }

    if (size <= 0) {
        Errs()->add_error("zero or negative size argument");
        return (OK);
    }

    char *buf = new char[size+1];
    if (daemon_client::read_n_bytes(id, (unsigned char*)buf, size)) {
        res->content.string = buf;
        res->flags |= VF_ORIGINAL;
    }
    else
        delete [] buf;
    return (OK);
}


// (string) ReadReply(retcode, skt_handle)
//
// This function will read a response message from the Xic server.  It
// expects the Xic server protocol and can not be used for other
// purposes.
//
// The first argument is an array of size 3 or larger.  Upon return,
// retcode[0] will contain the server return code, which is an integer
// 0-9, or possibly -1 on error.  The value in retcode[1] will be the
// size of the message returned, which will be 0 or larger.  The value
// in retcode[2] will be 0 on success, 1 on error.  If an error
// occurred, an error message is available from GetError.
//
// The return code in retcode[0] can have the following response types:
// -1       error in function
//  0       ok
//  1       in block, waiting for "end"
//  2       error from server
//  3       scalar data
//  4       string data
//  5       array data
//  6       zlist data
//  7       lexpr data
//  8       handle data
//  9       geometry data
//
// The return value is of string-type, and may be null or binary.
// With binary data, the standard string manipulations may not work,
// and in fact can easily cause a program crash.  It is not likely
// that the return will have any use other than as an argument to
// ConvertReply.
//
// This function will fail (halt the script) only if the retcode
// argument is bad.
//
bool
misc2_funcs::IFreadReply(Variable *res, Variable *args, void*)
{
    double *retcode = 0;
    ARG_CHK(arg_array(args, 0, &retcode, 3))
    if (!retcode) {
        Errs()->add_error("bad array argument");
        return (BAD);
    }
    retcode[0] = -1;  // return code
    retcode[1] = 0;  // data size
    retcode[2] = 0;  // error

    res->type = TYP_STRING;
    res->content.string = 0;

    int id = 0;
    if (!arg_handle(args, 1, &id))
        return (OK);
    if (id < 0) {
        Errs()->add_error("bad handle argument");
        retcode[2] = 1;
        return (OK);
    }

    sHdl *hdl = sHdl::get(id);
    if (!hdl) {
        Errs()->add_error("bad handle");
        retcode[2] = 1;
        return (OK);
    }
    if (hdl->type != HDLfd) {
        Errs()->add_error("bad handle type");
        retcode[2] = 1;
        return (OK);
    }
    if (hdl->data) {
        Errs()->add_error("handle not a socket");
        retcode[2] = 1;
        return (OK);
    }

    int code;
    int size;
    unsigned char *msg;
    if (daemon_client::read_msg(id, &code, &size, &msg)) {
        res->content.string = (char*)msg;
        if (res->content.string)
            res->flags |= VF_ORIGINAL;
        retcode[0] = code;
        retcode[1] = size;
    }
    else
        retcode[2] = 1;
    return (OK);
}


// (variable) ConvertReply(message, retcode)
//
// This function will parse and analyze a return message from the Xic
// server, which has been received with ReadReply.  The first argument
// is the message returned from ReadReply.  The second argument is an
// array of size 3 or larger, and can be the same array passed to
// ReadReply.  The retcode[0] entry must be set to the message return
// code, and retcode[1] must be set to the size of the returned
// buffer.  These are the same values as set in ReadReply.
//
// Upon return, retcode[2] will contain a "data_ok" flag, which will
// be nonzero if the message contained data and the data were read
// properly.  The function will fail (by halting the script) if the
// retcode argument is bad, i.e., not an array of size 3 or larger, or
// the message argument is not string-type.
//
// The response codes 0-2 contain no data and are status responses
// from the server.  The data responses will set the type and data of
// the function return, if successful.  The retcode[2] value will be
// nonzero on success in these cases, and will always be false if
// "longmode" is not enabled.
//
// Note that the type returned can be anything, and if assigned to a
// variable that already has a different type, an error will occur.
// The delete operator can be applied to the assigned-to variable to
// clear its state, before the function call.
//
// The response type 9 is returned from the geom server function.
// This function will return a handle to a geometry stream, which can
// be passed to GsReadObject.
//
bool
misc2_funcs::IFconvertReply(Variable *res, Variable *args, void*)
{
    const char *buf = 0;
    ARG_CHK(arg_string(args, 0, &buf))

    double *retcode = 0;
    ARG_CHK(arg_array(args, 1, &retcode, 3))
    // [0]: return code (input)
    // [1]: size (input)
    // [2]: data_ok flag (output)

    if (!retcode) {
        Errs()->add_error("null return code argument");
        return (BAD);
    }

    res->type = TYP_NOTYPE;
    res->content.string = 0;
    int code = (int)retcode[0];
    int size = (int)retcode[1];

    bool data_ok;
    daemon_client::convert_reply(code, (unsigned char*)buf, size, res,
        &data_ok);
    retcode[2] = data_ok;
    return (OK);
}


// (int) WriteMsg(string, skt_handle)
//
// This function will write a message to a socket, adding the proper
// network line termination.  The first argument is a string
// containing the characters to write.  The second argument is a
// socket handle obtianed from Sopen.  Any trailing line termination
// will be stripped from the string, and the network termination
// "\r\n" will be added.
//
// This function never fails (halts the script).  The return value is
// the number of bytes written, or 0 on error.  On error, a message is
// available from GetError.
//
bool
misc2_funcs::IFwriteMsg(Variable *res, Variable *args, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;

    const char *buf = 0;
    if (!arg_string(args, 0, &buf))
        return (OK);
    if (!buf) {
        Errs()->add_error("null string argument");
        return (OK);
    }

    int id = 0;
    if (!arg_handle(args, 1, &id))
        return (OK);
    if (id < 0) {
        Errs()->add_error("bad handle argument");
        return (OK);
    }

    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLfd) {
            Errs()->add_error("incorrect handle type");
            return (OK);
        }
        if (hdl->data) {
            Errs()->add_error("handle not a socket");
            return (OK);
        }
        int len = strlen(buf);
        char *tbf = new char[len+3];
        strcpy(tbf, buf);
        char *t = tbf + len -1;
        while (t >= tbf && (*t == '\n' || *t == '\r'))
            *t-- = 0;
        *++t = '\r';
        *++t = '\n';
        *++t = 0;
        len = strlen(tbf);
        GCarray<char*> gc_tbf(tbf);

        int slen = len;
        for (;;) {
            int i = send(id, tbf, len, 0);
            if (i == -1) {
                if (errno == EINTR)
                    continue;
                Errs()->add_error("socket write error");
                return (OK);
            }
            if (i == len)
                break;
            len -= i;
            tbf += i;
        }
        res->type = TYP_SCALAR;
        res->content.value = slen;
    }
    else
        Errs()->add_error("handle not found");
    return (OK);
}


//-------------------------------------------------------------------------
// System Command Interface
//-------------------------------------------------------------------------

// (int) Shell(command)
//
// The Shell command will execute command under an operating system
// shell.  The command string consists of an executable name plus
// arguments, which should be meaningful to the operating system.
//
bool
misc2_funcs::IFshell(Variable *res, Variable *args, void*)
{
    const char *command;
    ARG_CHK(arg_string(args, 0, &command))

    if (!command)
        return (BAD);

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;
    res->content.value = cMain::System(command);
    return (OK);
}


// (int) GetPID(parent)
//
// If the boolean argument is zero, this function returns the process
// ID of the currently running Xic process.  If the argument is
// nonzero, the function returns the process ID of the parent process
// (typically a shell).  The process ID is a unique integer assigned
// by the operating system.
//
bool
misc2_funcs::IFgetPID(Variable *res, Variable *args, void*)
{
    bool ppid;
    ARG_CHK(arg_boolean(args, 0, &ppid))

    res->type = TYP_SCALAR;
#ifdef WIN32
    res->content.value = getpid();
#else
    res->content.value = ppid ? getppid() : getpid();
#endif
    return (OK);
}


//-------------------------------------------------------------------------
// Menu Buttons
//-------------------------------------------------------------------------

// (int) SetButtonStatus(menu, button, set)
//
// This command sets the state of the specified button in the given
// menu or button array, which must be a toggle button.  The button
// will be "pressed" if necessary to match the given state.
//
// The first argument is a string giving the internal name of a menu. 
// If the given name is null, empty, or "main", all of the menus in
// the main window will be searched.  The internal menu names are as
// follows:

//
//    main        Main window menus
//    side        Side buttons
//    top         Top buttons
//    sub1        Viewport 1 menus
//    sub2        Viewport 2 menus
//    sub3        Viewport 3 menus
//    sub4        Viewport 4 menus
//
//    file        File Menu
//    cell        Cell Menu
//    edit        Edit Menu
//    mod         Modify Menu
//    view        View Menu
//    attr        Attributes Menu
//    conv        Convert Menu
//    drc         DRC Menu
//    ext         Extract Menu
//    user        User menu
//    help        Help Menu
//
// The second argument is the button name, which is the code name
// given in the tooltip window which pops up when the mouse pointer
// rests over the button.  In the case of User Menu command buttons,
// the name is the text which appears on the button.  Only buttons
// and menus visible in the current mode (electrical or physical)
// can be accessed.
//
// It should be stressed that the string arguments refer to internal
// names, and not (in general) the label printed on the button.  For a
// button, this is the five character or fewer name that is shown in
// the tooltip that pops up when the pointer is over the button.  The
// same applies to the menu argument, however these names are not
// available from running Xic.  The internal menu names are provided
// in the table above.

// The identification of the menu is case insensitive.  In the lower
// group of entries, only the first one or two characters have to
// match.  Thus "Convert", "c", and "crazy" would all select the
// Convert menu, for example.  One character is sufficient, except for
// `e' (Extract and Edit).  So, the menu argument can be the menu
// label, or the internal name, or some simplification at the user's
// discretion.  For the upper group, the entire menu name must be
// given.
//
// If the third argument is nonzero, the button will be pressed if it
// is not already engaged.  If the third argument is zero, the button
// will be depressed if it is not already depressed.  The return value
// is 1 if the button state changed, 0 if the button state did not
// change, or -1 if the button was not found.
//
bool
misc2_funcs::IFsetButtonStatus(Variable *res, Variable *args, void*)
{
    const char *menuname;
    ARG_CHK(arg_string(args, 0, &menuname))
    const char *button;
    ARG_CHK(arg_string(args, 1, &button))
    bool set;
    ARG_CHK(arg_boolean(args, 2, &set))

    if (!button || !*button)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    int state = Menu()->MenuButtonStatus(menuname, button);
    if (state == 0) {
        if (set)
            res->content.value = Menu()->MenuButtonPress(menuname, button);
    }
    else if (state == 1) {
        if (!set)
            res->content.value = Menu()->MenuButtonPress(menuname, button);
    }
    else
        res->content.value = -1;
    return (OK);
}


// (int) GetButtonStatus(menu, button)
//
// This command returns the status of the indicated menu button, which
// should be a toggle button.  The two arguments are as described for
// SetButtonStatus().  The return value is 1 if the button is engaged,
// 0 if the button is not engaged, or -1 if the button is not found.
//
bool
misc2_funcs::IFgetButtonStatus(Variable *res, Variable *args, void*)
{
    const char *menuname;
    ARG_CHK(arg_string(args, 0, &menuname))
    const char *button;
    ARG_CHK(arg_string(args, 1, &button))

    if (!button || !*button)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = Menu()->MenuButtonStatus(menuname, button);
    return (OK);
}


// (int) PressButton(menu, button)
//
// This command "presses" the indicated button.  This works with all
// buttons, toggle or otherwise, and is equivalent to clicking on the
// button with the mouse.  The two arguments, which identify the menu
// and button, are described under SetButtonStatus().  The return
// value is 1 if the button was pressed, 0 if the button was not
// found.
//
bool
misc2_funcs::IFpressButton(Variable *res, Variable *args, void*)
{
    const char *menuname;
    ARG_CHK(arg_string(args, 0, &menuname))
    const char *button;
    ARG_CHK(arg_string(args, 1, &button))

    if (!button || !*button)
        return (BAD);

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;
    res->content.value = Menu()->MenuButtonPress(menuname, button);
    return (OK);
}


//
// The following four functions send raw events to the window system.
// They can be used to interpret lines from the xic_run.log file, but
// are not likely to be of much use to most Xic users.
//

// BtnDown(num, state, x, y, widget)
//
// This command generates a button press event dispatched to the
// widget specified by the last argument.  The num is the button
// number; 1 for left, 2 for middle, 3 for right.  The state is the
// "modifier" key state at the time of the event, and is the OR of 1
// if Shift pressed, 4 if Control pressed, 8 if Alt pressed, as in X
// windows.  Other flags may be given as per that spec, but are not
// used by Xic.  The coordinates are relative to the window of the
// target.  The widget argument is a string containing a resource path
// for the widget relative to the application, the syntax of which is
// dependent on the specific user interface.  A call to BtnDown should
// be followed by a call to BtnUp on the same widget.  There is no
// return value.
//
bool
misc2_funcs::IFbtnDown(Variable*, Variable *args, void*)
{
    int num;
    ARG_CHK(arg_int(args, 0, &num))
    int state;
    ARG_CHK(arg_int(args, 1, &state))
    int x;
    ARG_CHK(arg_int(args, 2, &x))
    int y;
    ARG_CHK(arg_int(args, 3, &y))
    const char *widget;
    ARG_CHK(arg_string(args, 4, &widget))

    if (!widget || !*widget)
        return (BAD);

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    if (!XM()->SendButtonEvent(widget, state, num, x, y, false)) {
        Log()->ErrorLog("BtnDown", "Can't resolve widget string.\n");
        return (BAD);
    }
    return (OK);
}


// BtnUp(num, state, x, y, widget)
//
// This command generates a button release event dispatched to the
// widget specified by the last argument.  The num is the button
// number; 1 for left, 2 for middle, 3 for right.  The state is the
// "modifier" key state at the time of the event, and is the OR of 1
// if Shift pressed, 4 if Control pressed, 8 if Alt pressed, as in X
// windows.  Other flags may be given as per that spec, but are not
// used by Xic.  The coordinates are relative to the window of the
// target.  The widget argument is a string containing a resource path
// for the widget relative to the application, the syntax of which is
// dependent on the specific user interface.  There is no return
// value.
//
bool
misc2_funcs::IFbtnUp(Variable*, Variable *args, void*)
{
    int num;
    ARG_CHK(arg_int(args, 0, &num))
    int state;
    ARG_CHK(arg_int(args, 1, &state))
    int x;
    ARG_CHK(arg_int(args, 2, &x))
    int y;
    ARG_CHK(arg_int(args, 3, &y))
    const char *widget;
    ARG_CHK(arg_string(args, 4, &widget))

    if (!widget || !*widget)
        return (BAD);

    if (!XM()->SendButtonEvent(widget, state, num, x, y, true)) {
        Log()->ErrorLog("BtnUp", "Can't resolve widget string.\n");
        return (BAD);
    }
    return (OK);
}


// KeyDown(keysym, state, widget)
//
// This command generates a key press event dispatched to the widget
// specified in the third argument.  The keysym is a user-interface
// dependent key code corresponding to the keypress to send, and the
// state is the modifier state.  These correspond to the X window
// system definitions under Unix/Linux.  The widget argument is a
// string containing a resource path for the widget relative to the
// application, the syntax of which is dependent on the specific user
// interface.  There is no return value.
//
bool
misc2_funcs::IFkeyDown(Variable*, Variable *args, void*)
{
    int keysym;
    ARG_CHK(arg_int(args, 0, &keysym))
    int state;
    ARG_CHK(arg_int(args, 1, &state))
    const char *widget;
    ARG_CHK(arg_string(args, 2, &widget))

    if (!widget || !*widget)
        return (BAD);

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    if (!XM()->SendKeyEvent(widget, keysym, state, false)) {
        Log()->ErrorLog("KeyDown", "Can't resolve widget string.\n");
        return (BAD);
    }
    return (OK);
}


// KeyUp(keysym, state, widget)
//
// This command generates a key release event dispatched to the widget
// specified in the third argument.  The arguments are as described for
// DeyDown().  There is no return value.
//
bool
misc2_funcs::IFkeyUp(Variable*, Variable *args, void*)
{
    int keysym;
    ARG_CHK(arg_int(args, 0, &keysym))
    int state;
    ARG_CHK(arg_int(args, 1, &state))
    const char *widget;
    ARG_CHK(arg_string(args, 2, &widget))

    if (!widget || !*widget)
        return (BAD);
    if (!XM()->SendKeyEvent(widget, keysym, state, true)) {
        Log()->ErrorLog("KeyUp", "Can't resolve widget string.\n");
        return (BAD);
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Mouse Input
//-------------------------------------------------------------------------

namespace {
    enum PTretType { PTok, PTesc, PTabort };
    struct PointState *PointCmd;

    struct PointState : public CmdState
    {
        PointState(const char *nm, const char *hk) : CmdState(nm, hk)
            {
                got_btn1 = false;
            }
        ~PointState()   { PointCmd = 0; }

        void b1down()   { got_btn1 = true; }
        void b1up()     { if (got_btn1) esc(); }

        void esc()
            {
                got_abort = Abort;
                if (GRpkgIf()->LoopLevel() > 1)
                    GRpkgIf()->BreakLoop();
                EV()->PopCallback(this);
                delete this;
            }

        static bool got_btn1;
        static bool got_abort;
    };


    // This function pushes into a new call state and waits for a
    // button 1 press (returns true) or an escape event (returns
    // false).  This is used in scripts as a wait loop for placing
    // objects
    //
    PTretType pointTo()
    {
        if (!GRpkgIf()->MainDev() || GRpkgIf()->MainDev()->ident != _devGTK_)
             return (PTesc);
        PointCmd = new PointState("POINT", "Point");
        PL()->SavePrompt();
        Gst()->SaveGhost();
        if (!EV()->PushCallback(PointCmd)) {
            delete PointCmd;
            PointCmd = 0;
            return (PTesc);
        }
        PL()->RestorePrompt();
        Gst()->RestoreGhost();
        GRpkgIf()->MainLoop();

        if (PointState::got_abort) {
            PointState::got_abort = false;
            return (PTabort);
        }
        if (PointState::got_btn1) {
            PointState::got_btn1 = false;
            return (PTok);
        }
        return (PTesc);
    }
}

bool PointState::got_btn1;
bool PointState::got_abort;


// (int) Point(array)
//
// The argument is the name of a two component (or larger) real array. 
// This function blocks until mouse button 1 (left button) is pressed, or the Esc
// key is pressed, while the pointer is in a drawing window.  The
// coordinates of the pointer at the time of the press are returned in
// the array.  The return value is 0 if Esc was pressed, or 1 for a
// button 1 press.  Buttons 2 and 3 have their normal effects while
// this function is active, i.e., they are not handled in this
// function.
//
// Example:
//     a[2]
//     ShowPrompt("Click in a drawing window")
//     Point(a)
//     ShowPrompt("x=", a[0], "y=", a[1])
//      
// When a ghost image is displayed with the ShowGhost function, the
// coordinates returned are either snapped to the grid or not,
// depending on the mode number passed to ShowGhost.  If no ghost
// image is displayed, the nearest grid point is returned.
//    
// If the UseTransform function has been called to enable use of the
// current transform, the current transform will be applied to the
// displayed objects when using mode 8.  The translation supplied to
// UseTransform is ignored (the translation tracks the mouse pointer).
//
bool
misc2_funcs::IFpoint(Variable *res, Variable *args, void*)
{
    double *vals;
    if (!arg_array(args, 0, &vals, 2))
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = 0;
    vals[0] = 0.0;
    vals[1] = 0.0;

    PTretType ret = pointTo();
    if (ret == PTabort)
        SI()->SetInterrupt();
    else if (ret == PTok) {
        int x, y;
        switch (SIlcx()->curGhost()) {
        default:
            EV()->Cursor().get_xy(&x, &y);
            vals[0] = MICRONS(x);
            vals[1] = MICRONS(y);
            break;
        case 3:
        case 4:
        case 7:
        case 11:
            EV()->Cursor().get_raw(&x, &y);
            vals[0] = MICRONS(x);
            vals[1] = MICRONS(y);
            break;
        case 5:
            EV()->Cursor().get_xy(&x, &y);
            XM()->To45snap(&x, &y, SIlcx()->lastX(), SIlcx()->lastY());
            vals[0] = MICRONS(x);
            vals[1] = MICRONS(y);
            break;
        case 9:
            EV()->Cursor().get_raw(&x, &y);
            XM()->To45snap(&x, &y, SIlcx()->lastX(), SIlcx()->lastY());
            vals[0] = MICRONS(x);
            vals[1] = MICRONS(y);
            break;
        }
        res->content.value = 1;
    }
    return (OK);
}


namespace {
    struct SelectState : public CmdState
    {
        SelectState(const char *nm, const char *hk) : CmdState(nm, hk) { }

        void b1down()   { cEventHdlr::sel_b1down(); }
        void b1up()     { cEventHdlr::sel_b1up(&AOI, 0, 0); }

        void esc()
            {
                got_abort = Abort;
                if (GRpkgIf()->LoopLevel() > 1)
                    GRpkgIf()->BreakLoop();
                EV()->PopCallback(this);
                delete this;
            }

        bool key(int, const char*, int)
            {
                esc();
                return (true);
            }

        static bool got_abort;

    private:
        BBox AOI;
    };

    SelectState *SelectCmd;
}

bool SelectState::got_abort;


// (int) Selection()
//
// Block, but allow selections in drawing windows.  Return on any
// keypress, or escape event.  Return the number of selected objects
// in the selection list.
//
bool
misc2_funcs::IFselection(Variable *res, Variable*, void*)
{
    SelectCmd = new SelectState("SELEC", "Selec");
    PL()->SavePrompt();
    Gst()->SaveGhost();
    if (!EV()->PushCallback(SelectCmd)) {
        delete SelectCmd;
        SelectCmd = 0;
        return (BAD);
    }
    PL()->RestorePrompt();
    Gst()->RestoreGhost();
    GRpkgIf()->MainLoop();

    if (SelectState::got_abort) {
        SelectState::got_abort = false;
        SI()->SetInterrupt();
    }

    res->type = TYP_SCALAR;
    res->content.value = Selections.queueLength(CurCell());
    return (OK);
}


//-------------------------------------------------------------------------
// Graphical Input
//-------------------------------------------------------------------------

namespace {
    struct InpState *InpCmd;

    struct InpState : public CmdState
    {
        InpState(const char *nm, const char *hk) : CmdState(nm, hk)
            {
                widget = 0;
                ret_string = 0;
                InpCmd = this;
            }
        virtual ~InpState() { InpCmd = 0; }

        void esc();

        static ESret inp_cb(const char*, void*);
        static void inp_dn(bool);

        static char *ret_string;
        GRledPopup *widget;
    };
    char *InpState::ret_string;

    void
    InpState::esc()
    {
        GRledPopup *p = widget;
        if (GRpkgIf()->LoopLevel() > 1)
            GRpkgIf()->BreakLoop();
        EV()->PopCallback(this);
        delete this;
        if (p)
            p->popdown();
    }

    ESret
    InpState::inp_cb(const char *str, void*)
    {
        delete [] ret_string;
        ret_string = lstring::copy(str);
        return (ESTR_DN);
    }

    void
    InpState::inp_dn(bool)
    {
        if (InpCmd) {
            InpCmd->widget = 0;
            InpCmd->esc();
        }
    }
}


// (string) PopUpInput(message, default, buttontext, multiline)
//
// This function will pop up a text-input widget, into which the user
// can enter text.  The function blocks until the user presses the
// affirmation button, at which time the text is returned, and the
// pop-up disappears.  If the user instead presses the Dismiss button
// or otherwise destroys the pop-up, the script will halt.
//
// The first argument is an explanatory string which is printed on
// the pop-up.  If this argument is null or empty, a default message
// is used.  Recall that passing 0 is equivalent to passing a null
// string.
//
// The second argument is a string providing default text which
// appears in the entry area when the pop-up appears.  If this
// argument is null or empty there will be no default text.
//
// The third argument is a string giving text that will appear on the
// affirmation button.  If null or empty, the button will show a
// default label.
//
// The fourth argument is a boolean that when nonzero, a multi-line
// text input widget will be used.  Otherwise, a single-line input
// widget will be used.
//
bool
misc2_funcs::IFpopUpInput(Variable *res, Variable *args, void*)
{
    const char *msg;
    ARG_CHK(arg_string(args, 0, &msg))
    const char *def;
    ARG_CHK(arg_string(args, 1, &def))
    const char *btn;
    ARG_CHK(arg_string(args, 2, &btn))
    bool multiline;
    ARG_CHK(arg_boolean(args, 3, &multiline))

    InpCmd = new InpState("INPUT", "PopUpInput");
    if (!EV()->PushCallback(InpCmd)) {
        delete InpCmd;
        return (BAD);
    }
    InpCmd->widget = 
    DSPmainWbagRet(PopUpEditString(0, GRloc(), msg, def, InpState::inp_cb, 0,
        0, InpState::inp_dn, multiline, btn));
    GRpkgIf()->MainLoop();

    res->type = TYP_STRING;
    res->content.string = InpState::ret_string;
    InpState::ret_string = 0;
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    else
        SI()->Halt();
    return (OK);
}


namespace {
    struct AfState *AfCmd;

    struct AfState : public CmdState
    {
        AfState(const char *nm, const char *hk) : CmdState(nm, hk)
            {
                widget = 0;
                AfCmd = this;
                ret_value = 0;
            }
        virtual ~AfState() { AfCmd = 0; }

        void esc();

        static void af_cb(bool, void*);

        static int ret_value;
        GRaffirmPopup *widget;
    };
    int AfState::ret_value;

    void
    AfState::esc()
    {
        GRaffirmPopup *p = widget;
        if (GRpkgIf()->LoopLevel() > 1)
            GRpkgIf()->BreakLoop();
        EV()->PopCallback(this);
        delete this;
        if (p)
            p->popdown();
    }

    void
    AfState::af_cb(bool yn, void*)
    {
        if (AfCmd) {
            ret_value = 1 + yn;
            AfCmd->widget = 0;
            AfCmd->esc();
        }
    }
}


// (int) PopUpAffirm(message)
//
// This button pops up a small window which allows the user to answer
// yes or no to a question.  Deleting the window is equivalent to
// answering no.  The argument is a string which should contain the
// text to which the user responds.  When the user responds, the
// pop-up disappears, and the return value is 1 if the user answered
// "yes", 0 otherwise.
//
bool
misc2_funcs::IFpopUpAffirm(Variable *res, Variable *args, void*)
{
    const char *msg;
    ARG_CHK(arg_string(args, 0, &msg))

    AfCmd = new AfState("YESNO", "PopUpAffirm");
    if (!EV()->PushCallback(AfCmd)) {
        delete AfCmd;
        return (BAD);
    }
    AfCmd->widget = 
    DSPmainWbagRet(PopUpAffirm(0, GRloc(), msg, AfState::af_cb, 0));
    GRpkgIf()->MainLoop();

    res->type = TYP_SCALAR;
    if (AfState::ret_value) {
        res->content.value =  AfState::ret_value - 1;
        AfState::ret_value = 0;
    }
    else
        SI()->Halt();
    return (OK);
}


namespace {
    struct NuState *NuCmd;

    struct NuState : public CmdState
    {
        NuState(const char *nm, const char *hk) : CmdState(nm, hk)
            {
                widget = 0;
                NuCmd = this;
                ret_value = 0;
            }
        virtual ~NuState() { NuCmd = 0; }

        void esc();

        static void nu_cb(double, bool, void*);

        static double ret_value;
        static bool ret_ok;
        GRnumPopup *widget;
    };
    double NuState::ret_value;
    bool NuState::ret_ok;

    void
    NuState::esc()
    {
        GRnumPopup *p = widget;
        if (GRpkgIf()->LoopLevel() > 1)
            GRpkgIf()->BreakLoop();
        EV()->PopCallback(this);
        delete this;
        if (p)
            p->popdown();
    }

    void
    NuState::nu_cb(double d, bool affirmed, void*)
    {
        if (NuCmd) {
            ret_value = d;
            ret_ok = affirmed;
            NuCmd->widget = 0;
            NuCmd->esc();
        }
    }
}


// (string) PopUpNumeric(message, initval, minval, maxval, delta, numdgt))
//
// This function pops up a small window which contains a "spin button"
// for numerical entry.  The user is able to enter a number directly,
// or by clicking on the increment/decrement buttons.
//
// The first argument is a string providing explanatory text.  The
// second argument provides the initial numeric value.  The minval and
// maxval arguments are the minimum and maximum allowed values.  The
// delta argument is the delta to increment or decrement when the user
// presses the up/down buttons.  These parameters are all real values. 
// The numdgt is an integer value which sets how many places to the
// right of a decimal point are shown.
//
// If the user presses Apply, the pop-up disappears, and this function
// returns the current value.  If the user presses the Dismiss button
// or otherwise destroys the widget, the script will halt. 
//
bool
misc2_funcs::IFpopUpNumeric(Variable *res, Variable *args, void*)
{
    const char *msg;
    ARG_CHK(arg_string(args, 0, &msg))
    double initd;
    ARG_CHK(arg_real(args, 1, &initd))
    double mind;
    ARG_CHK(arg_real(args, 2, &mind))
    double maxd;
    ARG_CHK(arg_real(args, 3, &maxd))
    double del;
    ARG_CHK(arg_real(args, 4, &del))
    int numd;
    ARG_CHK(arg_int(args, 5, &numd))

    NuCmd = new NuState("NUMERIC", "PopUpNumeric");
    if (!EV()->PushCallback(NuCmd)) {
        delete NuCmd;
        return (BAD);
    }
    NuCmd->widget = 
    DSPmainWbagRet(PopUpNumeric(0, GRloc(), msg, initd, mind, maxd, del,
        numd, NuState::nu_cb, 0));
    GRpkgIf()->MainLoop();

    res->type = TYP_SCALAR;
    res->content.value =  NuState::ret_value;
    NuState::ret_value = 0;
    if (!NuState::ret_ok)
        SI()->Halt();
    return (OK);
}


//-------------------------------------------------------------------------
// Text Input
//-------------------------------------------------------------------------

// (real) AskReal(prompt, default)
//
// The two arguments are both strings, or the predefined constant
// NULL.  The function will print the strings on the prompt line, and
// the user will type a response.  The response is converted to a real
// number which is returned by the function.  If either argument is
// NULL, that part of the message is not printed.  The prompt is
// immutable, but the default can be edited by the user.
//
bool
misc2_funcs::IFaskReal(Variable *res, Variable *args, void*)
{
    const char *prompt;
    ARG_CHK(arg_string(args, 0, &prompt))
    const char *deflt;
    ARG_CHK(arg_string(args, 1, &deflt))

    const char *in = PL()->EditPrompt(prompt, deflt);
    if (!in) {
        SI()->Halt();
        return (OK);
    }
    res->type = TYP_SCALAR;
    bool err;
    res->content.value = SIparse()->numberParse(&in, &err);
    if (err)
        res->content.value = 0.0;
    return (OK);
}


// (string) AskString(prompt, default)
//
// The two arguments and the return value are strings.  Similar to the
// AskReal function, however a string is returned.
//
bool
misc2_funcs::IFaskString(Variable *res, Variable *args, void*)
{
    const char *prompt;
    ARG_CHK(arg_string(args, 0, &prompt))
    const char *deflt;
    ARG_CHK(arg_string(args, 1, &deflt))

    char *in = PL()->EditPrompt(prompt, deflt);
    if (!in) {
        SI()->Halt();
        return (OK);
    }
    res->type = TYP_STRING;
    res->content.string = lstring::copy(in);
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (real) AskConsoleReal(prompt, default)
//
// This function prompts the user for a number, in the console window.
// It is otherwise similar to the AskReal() function.
//
bool
misc2_funcs::IFaskConsoleReal(Variable *res, Variable *args, void*)
{
    const char *prompt;
    ARG_CHK(arg_string(args, 0, &prompt))
    const char *deflt;
    ARG_CHK(arg_string(args, 1, &deflt))

    char buf[256];
    buf[0] = 0;
    if (prompt)
        sprintf(buf, "%s ", prompt);
    if (deflt)
        sprintf(buf + strlen(buf), "[%s] ", deflt);
    fputs(buf, stdout);
    fflush(stdout);
    if (!fgets(buf, 256, stdin)) {
        SI()->SetInterrupt();
        return (OK);
    }
    char *s = buf;
    while (isspace(*s))
        s++;
    if (!*s && deflt)
        strcpy(s, deflt);
    res->type = TYP_SCALAR;
    bool err;
    const char *t = s;
    res->content.value = SIparse()->numberParse(&t, &err);
    if (err)
        res->content.value = 0.0;
    return (OK);
}


// (string) AskConsoleString(prompt, default)
//
// This function prompts the user for a string, in the console window.
// It is otherwise similar to the AskString() function.
//
bool
misc2_funcs::IFaskConsoleString(Variable *res, Variable *args, void*)
{
    const char *prompt;
    ARG_CHK(arg_string(args, 0, &prompt))
    const char *deflt;
    ARG_CHK(arg_string(args, 1, &deflt))

    char buf[256];
    buf[0] = 0;
    if (prompt)
        sprintf(buf, "%s ", prompt);
    if (deflt)
        sprintf(buf + strlen(buf), "[%s] ", deflt);
    fputs(buf, stdout);
    fflush(stdout);
    if (!fgets(buf, 256, stdin)) {
        SI()->SetInterrupt();
        return (OK);
    }
    char *s = buf;
    while (isspace(*s))
        s++;
    if (!*s && deflt)
        strcpy(buf, deflt);
    for (s = buf; *s; s++) {
        if (*s == '\n' || *s == '\r')
            *s = 0;
    }
    res->type = TYP_STRING;
    res->content.string = lstring::copy(buf);
    res->flags |= VF_ORIGINAL;
    return (OK);
}


namespace {
    struct KState *KCmd;

    struct KState : public CmdState
    {
        KState(const char *nm, const char *hk) : CmdState(nm, hk) { }
        virtual ~KState() { KCmd = 0; }

        void esc();
        bool key(int, const char*, int);

        static int keyret;
    };
    int KState::keyret;


    void
    KState::esc()
    {
        if (Abort)
            keyret = ESCAPE_KEY;
        if (GRpkgIf()->LoopLevel() > 1)
            GRpkgIf()->BreakLoop();
        EV()->PopCallback(this);
        delete this;
    }


    bool
    KState::key(int code, const char*, int)
    {
        keyret = code;
        esc();
        return (true);
    }
}


// (int) GetKey()
//
// This function blocks until any key is pressed.  The return value is
// a key code, which is system dependent, but is generally the "keysym"
// of the key pressed.  If the value is less than 20, the value is an
// internal code.
//
bool
misc2_funcs::IFgetKey(Variable *res, Variable*, void*)
{
    KCmd = new KState("GETKEY", "GetKey");
    if (!EV()->PushCallback(KCmd)) {
        delete KCmd;
        return (BAD);
    }
    GRpkgIf()->MainLoop();
    res->type = TYP_SCALAR;
    res->content.value = KState::keyret;
    return (OK);
}


//-------------------------------------------------------------------------
// Text Output
//-------------------------------------------------------------------------

namespace {
    // Print an argument list to a string, which is returned.
    //
    char *
    prstring(Variable *args, bool show_escs, bool use_indent)
    {
        char buf[128];
        sLstr lstr;
        int indent_lev = use_indent ? SIlcx()->indentLevel() : 0;
        for (int i = 0; i < indent_lev; i++)
            lstr.add_c(' ');
        bool need_sp = false;
        for (int i = 0; i < MAXARGC; i++) {
            if (args[i].type == TYP_ENDARG)
                break;
            else if (args[i].type == TYP_NOTYPE || args[i].type == TYP_STRING) {
                if (need_sp)
                    lstr.add_c(' ');
                if (args[i].content.string) {
                    char *s;
                    if (show_escs)
                        s = SIparse()->toPrintable(args[i].content.string);
                    else
                        s = args[i].content.string;
                    if (s) {
                        for (char *t = s; *t; t++) {
                            lstr.add_c(*t);
                            need_sp = true;
                            if (*t == '\n') {
                                for (int j = 0; j < indent_lev; j++)
                                    lstr.add_c(' ');
                                need_sp = false;
                            }
                        }
                        if (show_escs)
                            delete [] s;
                    }
                }
                else {
                    lstr.add("(null)");
                    need_sp = true;
                }
            }
            else if (args[i].type == TYP_SCALAR) {
                if (need_sp)
                    lstr.add_c(' ');
                sprintf(buf, "%g", args[i].content.value);
                lstr.add(buf);
                need_sp = true;
            }
            else if (args[i].type == TYP_ARRAY) {
                int len = args[i].content.a->length();
                for (int j = 0; j < len; j++) {
                    if (j == SIlcx()->maxArrayPts()) {
                        lstr.add(" ...");
                        break;
                    }
                    sprintf(buf, "%g", args[i].content.a->values()[j]);
                    if (need_sp)
                        lstr.add_c(' ');
                    lstr.add(buf);
                    need_sp = true;
                }
            }
            else if (args[i].type == TYP_CMPLX) {
                if (need_sp)
                    lstr.add_c(' ');
                sprintf(buf, "(%g,%g)", args[i].content.cx.real,
                    args[i].content.cx.imag);
                lstr.add(buf);
                need_sp = true;
            }
            else if (args[i].type == TYP_ZLIST) {
                if (need_sp)
                    lstr.add_c(' ');
                int cnt = 0;
                for (Zlist *z = args[i].content.zlist; z; z = z->next) {
                    if (cnt == SIlcx()->maxZoids())
                        strcpy(buf, "list contains more zoids...");
                    else
                        sprintf(buf, "%g %g %g %g %g %g",
                        MICRONS(z->Z.xll), MICRONS(z->Z.xlr), MICRONS(z->Z.yl),
                        MICRONS(z->Z.xul), MICRONS(z->Z.xur), MICRONS(z->Z.yu));
                    lstr.add(buf);
                    if (z->next) {
                        lstr.add_c('\n');
                        for (int k = 0; k < indent_lev; k++)
                            lstr.add_c(' ');
                        need_sp = false;
                    }
                    else
                        need_sp = true;
                    if (cnt == SIlcx()->maxZoids())
                        break;
                    cnt++;
                }
            }
            else if (args[i].type == TYP_LEXPR) {
                if (need_sp)
                    lstr.add_c(' ');
                char *s = args[i].content.lspec->string();
                lstr.add(s);
                delete [] s;
                need_sp = true;
            }
            else if (args[i].type == TYP_HANDLE) {
                if (need_sp)
                    lstr.add_c(' ');
                int id = (int)args[i].content.value;
                sHdl *hdl = sHdl::get(id);
                if (hdl)
                    sprintf(buf, "(%d): handle to %s", id, hdl->string());
                else
                    sprintf(buf, "(%d): defunct handle", id);
                lstr.add(buf);
                need_sp = true;
            }
        }
        char *s = lstr.string_trim();
        if (!s)
            s = lstring::copy("");
        return (s);
    }


    // Print an argument list to a stringlist, which is returned.
    //
    stringlist *
    prlist(Variable *args, bool show_escs)
    {
        char buf[128];
        stringlist *s0 = 0;
        for (int i = 0; i < MAXARGC; i++) {
            if (args[i].type == TYP_ENDARG)
                break;
            else if (args[i].type == TYP_NOTYPE || args[i].type == TYP_STRING) {
                if (args[i].content.string) {
                    char *s;
                    if (show_escs)
                        s = SIparse()->toPrintable(args[i].content.string);
                    else
                        s = args[i].content.string;
                    s0 = new stringlist(lstring::copy(s), s0);
                    if (show_escs)
                        delete [] s;
                }
                else
                    s0 = new stringlist(lstring::copy("(null)"), s0);
            }
            else if (args[i].type == TYP_SCALAR) {
                sprintf(buf, "%g", args[i].content.value);
                s0 = new stringlist(lstring::copy(buf), s0);
            }
            else if (args[i].type == TYP_ARRAY) {
                int len = args[i].content.a->length();
                for (int j = 0; j < len; j++) {
                    if (j == SIlcx()->maxArrayPts()) {
                        s0 = new stringlist(lstring::copy(" ..."), s0);
                        break;
                    }
                    sprintf(buf, "%g", args[i].content.a->values()[j]);
                    s0 = new stringlist(lstring::copy(buf), s0);
                }
            }
            else if (args[i].type == TYP_CMPLX) {
                sprintf(buf, "(%g,%g)", args[i].content.cx.real,
                    args[i].content.cx.imag);
                s0 = new stringlist(lstring::copy(buf), s0);
            }
            else if (args[i].type == TYP_ZLIST) {
                int cnt = 0;
                for (Zlist *z = args[i].content.zlist; z; z = z->next) {
                    sprintf(buf, "%g %g %g %g %g %g",
                        MICRONS(z->Z.xll), MICRONS(z->Z.xlr), MICRONS(z->Z.yl),
                        MICRONS(z->Z.xul), MICRONS(z->Z.xur), MICRONS(z->Z.yu));
                    if (cnt == SIlcx()->maxZoids())
                        strcpy(buf, " ...");
                    s0 = new stringlist(lstring::copy(buf), s0);
                    if (cnt == SIlcx()->maxZoids())
                        break;
                    cnt++;
                }
            }
            else if (args[i].type == TYP_LEXPR) {
                char *s = args[i].content.lspec->string();
                s0 = new stringlist(s, s0);
            }
            else if (args[i].type == TYP_HANDLE) {
                int id = (int)args[i].content.value;
                sHdl *hdl = sHdl::get(id);
                if (hdl)
                    sprintf(buf, "(%d): handle to %s", id, hdl->string());
                else
                    sprintf(buf, "(%d): defunct handle", id);
                s0 = new stringlist(lstring::copy(buf), s0);
            }
        }
        stringlist::reverse(s0);
        return(s0);
    }
}


// (string) SepString(string, repeat)
//
// This function returns a string that is created by repeating the
// string argument repeat times.  The repeat value is an integer in
// the range 1-132.  The function will fail if string is null.
//
bool
misc2_funcs::IFsepString(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    int repeat;
    ARG_CHK(arg_int(args, 1, &repeat))

    if (!string)
        return (BAD);
    if (repeat < 1)
        repeat = 1;
    if (repeat > 132)
        repeat = 132;
    int len = strlen(string);
    char *s = new char[repeat*len + 1];
    char *t = s;
    for (int i = 0; i < repeat; i++) {
        strcpy(t, string);
        t += len;
    }
    res->type = TYP_STRING;
    res->content.string = s;
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) ShowPrompt(arg_list)
//
// Print the values of the arguments on the prompt line.  The number
// of arguments is variable.
// Example:
//    a = 2.5
//    b = "The value of a is "
//    ShowPrompt(b, a)
// will print "The value of a is 2.5" on the prompt line.
//
// If given without arguments, the prompt line will be erased, but
// without disturbing the current message as returned with
// GetLastPrompt().  The function returns 1 if something is printed
// (message updated), 0 otherwise.
//
bool
misc2_funcs::IFshowPrompt(Variable *res, Variable *args, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (args[0].type == TYP_ENDARG)
        PL()->ErasePrompt();
    else {
        char *s = prstring(args, false, false);
        if (s) {
            PL()->ShowPrompt(s);
            delete [] s;
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) SetIndent(level)
//
// This function is used to set the indentation level used for
// printing with the Print() and PrintLog() functions.  The argument
// is an integer which specifies the column where printed output will
// start.  The argument can also be a string in one fof the following
// formats:
//   "+N"   N is an optional integer (default 1), increases indentation
//          by N columns.
//   "-N"   N is an optional integer (default 1), decreases indentation
//          by N columns.
//   ""     Empty string, don't change indentation.
//
// The function returns the previous indentation level.
//
bool
misc2_funcs::IFsetIndent(Variable *res, Variable *args, void*)
{
    int level;
    if (args[0].type == TYP_SCALAR)
        level = (int)args[0].content.value;
    else if (args[0].type == TYP_STRING || args[0].type == TYP_NOTYPE) {
        char *s = args[0].content.string;
        if (!s)
            return (BAD);
        if (*s == '+') {
            s++;
            if (isdigit(*s))
                level = SIlcx()->indentLevel() + atoi(s);
            else
                level = SIlcx()->indentLevel() + 1;
        }
        else if (*s == '-') {
            s++;
            if (isdigit(*s))
                level = SIlcx()->indentLevel() - atoi(s);
            else
                level = SIlcx()->indentLevel() - 1;
        }
        else if (!*s)
            level = SIlcx()->indentLevel();
        else
            return (BAD);
    }
    else
        return (BAD);
    if (level < 0)
        level = 0;
    else if (level > 80)
        level = 80;
    res->type = TYP_SCALAR;
    res->content.value = SIlcx()->indentLevel();
    SIlcx()->setIndentLevel(level);
    return (OK);
}


// (int) SetPrintLimits(num_array_elts, num_zoids)
//
// While printing with the Print() family of functions, or when using
// ListHandle(), the number of array points and trapezoids actually
// printed is limited.  The default limits are 100 array points and 20
// trapezoids.  This function allows these limits to be changed.  A
// value for either argument of -1 will remove any limit, non-negative
// values will set the limit, and negative values of -2 or less will
// revert to the default values.  This function always returns 1 and
// never fails.
//
bool
misc2_funcs::IFsetPrintLimits(Variable *res, Variable *args, void*)
{
    int numelts;
    ARG_CHK(arg_int(args, 0, &numelts))
    int numzoids;
    ARG_CHK(arg_int(args, 1, &numzoids))

    if (numelts < -1)
        SIlcx()->setMaxArrayPts(100);
    else
        SIlcx()->setMaxArrayPts(numelts);
    if (numzoids < -1)
        SIlcx()->setMaxZoids(20);
    else
        SIlcx()->setMaxZoids(numzoids);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) Print(arg_list)
//
// Print the arguments on the terminal screen.  This is the window
// from which Xic was launched.  The number of arguments is variable.
// The printing is indented according to the level set with the
// SetIndent() function.
//
bool
misc2_funcs::IFprint(Variable *res, Variable *args, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
    char *s = prstring(args, false, true);
    if (s) {
        printf("%s\n", s);
        delete [] s;
        res->content.value = 1;
    }
    return (OK);
}


// (int) PrintLog(file_handle, arg_list)
//
// This works like the Print() function, however output goes to a file
// previously opened for writing with the Open() function.  The first
// argument is the file handle returned from Open().  Following
// arguments are printed to the file in order.  The printing is
// indented according to the level set with the SetIndent() function.
// The function returns the number of characters written.  The
// function will fail if the handle is not a file handle.
//
bool
misc2_funcs::IFprintLog(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    if (id < 0)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0.0;
    if (hdl) {
        if (hdl->type != HDLfd)
            return (BAD);
        char *s = prstring(args+1, false, true);
        if (s) {
            int n = strlen(s);
            if (hdl->data) {
                if (write(id, s, n) != n)
                    return (BAD);
                write(id, "\n", 1);
            }
            else {
                if (send(id, s, n, 0) != n)
                    return (BAD);
                send(id, "\n", 1, 0);
            }
            res->content.value = n+1;
        }
    }
    return (OK);
}


// (string) PrintString(arg_list)
//
// This works like the Print(), etc.  functions, however it returns a
// string containing the text.
//
bool
misc2_funcs::IFprintString(Variable *res, Variable *args, void*)
{
    char *s = prstring(args, false, false);
    res->type = TYP_STRING;
    res->content.string = s;
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (string) PrintStringEsc(arg_list)
//
// This works exactly like PrintString(), however, special characters
// in any string supplied as an argument are shown in their '\' escape
// form.
//
bool
misc2_funcs::IFprintStringEsc(Variable *res, Variable *args, void*)
{
    char *s = prstring(args, true, false);
    res->type = TYP_STRING;
    res->content.string = s;
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) Message(arg_list)
//
// Print the arguments in a pop-up message window.
//
bool
misc2_funcs::IFmessage(Variable *res, Variable *args, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
    char *s = prstring(args, false, false);
    if (s) {
        DSPmainWbag(PopUpInfo(MODE_ON, s))
        delete [] s;
        res->content.value = 1;
    }
    return (OK);
}


// (int) ErrorMsg(arg_list)
//
// Print the arguments in a pop-up error window.
//
bool
misc2_funcs::IFerrorMsg(Variable *res, Variable *args, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
    char *s = prstring(args, false, false);
    if (s) {
        DSPmainWbag(PopUpErr(MODE_ON, s))
        delete [] s;
        res->content.value = 1;
    }
    return (OK);
}


// (int) TextWindow(fname, readonly)
//
// This function brings up a text editor window loaded with the file
// whose path is given in the fname string.  If the integer readonly
// is 0, editing of the file is enabled, otherwise editing is
// prevented.
//
bool
misc2_funcs::IFtextWindow(Variable *res, Variable *args, void*)
{
    const char *path;
    ARG_CHK(arg_string(args, 0, &path))
    bool readonly;
    ARG_CHK(arg_boolean(args, 1, &readonly))

    if (!path || !*path)
        return (BAD);
    if (readonly)
        DSPmainWbag(PopUpFileBrowser(path))
    else
        DSPmainWbag(PopUpTextEditor(path, 0, 0, false))
    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    return (OK);
}


//-------------------------------------------------------------------------
// Database Visualization Functions (exports)
//-------------------------------------------------------------------------

// ShowDb(dbname, array)
//
// This function will pop up a window displaying the area given in
// the array of the special database named in dbname.  The array
// argument is in the same format as passed to ChdOpenDb or equivalent.
// If passed 0, the bounding box containing all objects in the
// database is understood.  The return value is the window number of
// the new window (1-4) or -1 if an error occurred.
//
bool
misc2_funcs::IFshowDb(Variable *res, Variable *args, void*)
{
    const char *dbname;
    ARG_CHK(arg_string(args, 0, &dbname))
    double *array;
    ARG_CHK(arg_array_if(args, 1, &array, 4))

    if (!dbname || !*dbname)
        return (BAD);
    BBox BB;
    if (array) {
        BB.left = INTERNAL_UNITS(array[0]);
        BB.bottom = INTERNAL_UNITS(array[1]);
        BB.right = INTERNAL_UNITS(array[2]);
        BB.top = INTERNAL_UNITS(array[3]);
        BB.fix();
    }
    else {
        cSDB *sdb = CDsdb()->findDB(dbname);
        if (sdb)
            BB = *sdb->BB();
        else
            return (BAD);
    }

    res->type = TYP_SCALAR;
    res->content.value = DSP()->OpenSubwin(&BB, WDsdb, dbname, false);
    return (OK);
}

