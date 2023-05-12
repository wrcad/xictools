
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

#include "main.h"
#include "cvrt.h"
#include "pcell.h"
#include "fio.h"
#include "cd_celldb.h"
#include "cd_propnum.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "oa_if.h"
#include "tech.h"
#include "promptline.h"
#include "errorlog.h"
#include "miscutil/timer.h"
#include "miscutil/filestat.h"


//-----------------------------------------------------------------------------
// FIO configuration

// This is a hack to print (in InfoMessage) the bytes read and written
// in separate columns during format conversions.  The saved strings are
// cleared before and after the i/o operation.

namespace {
    const char *write_msg;   // "Processed xxxx Mb"
    const char *read_msg;    // "Wrote xxxx Mb"
    bool two_col_progress_msgs;

    void
    setup_two_col_msgs(bool set)
    {
        delete [] write_msg;
        write_msg = 0;
        delete [] read_msg;
        read_msg = 0;
        two_col_progress_msgs = set;
    }

    void
    ifSetWorking(bool b)
    {
        DSPpkg::self()->SetWorking(b);
    }

    // Process an informational message.  These messages would generally
    // go to the screen.
    //

    // Common back-end, we want to pass separate headers to the
    // error logging functions.
    //
    void
    infoMsgBackend(int code, char *buf, const char *loghdr)
    {
        if (code == IFMSG_INFO)
            PL()->ShowPrompt(buf);
        else if (code == IFMSG_RD_PGRS) {
            if (two_col_progress_msgs) {
                delete [] read_msg;
                read_msg = lstring::copy(buf);
                if (write_msg)
                    sprintf(buf, "%-20s %s", read_msg, write_msg);
            }
            PL()->ShowPrompt(buf);
        }
        else if (code == IFMSG_WR_PGRS) {
            if (two_col_progress_msgs) {
                delete [] write_msg;
                write_msg = lstring::copy(buf);
                if (read_msg)
                    sprintf(buf, "%-20s %s", read_msg, write_msg);
            }
            PL()->ShowPrompt(buf);
        }
        else if (code == IFMSG_CNAME)
            PL()->ShowPrompt(buf);
        else if (code == IFMSG_POP_ERR)
            Log()->PopUpErr(buf);
        else if (code == IFMSG_POP_WARN)
            Log()->PopUpWarn(buf);
        else if (code == IFMSG_POP_INFO) {
            if (XM()->RunMode() == ModeNormal)
                DSPmainWbag(PopUpInfo(MODE_ON, buf))
            else
                fprintf(stderr, "%s\n", buf);
        }
        else if (code == IFMSG_LOG_ERR)
            Log()->ErrorLog(loghdr, buf);
        else if (code == IFMSG_LOG_WARN)
            Log()->WarningLog(loghdr, buf);
    }

    // Message handler for CD and the script parser/evaluator.
    //
    void
    ifInfoMessageCD(INFOmsgType code, const char *string, va_list args)
    {
        char buf[512];
        if (!string)
            string = "";
        vsnprintf(buf, 512, string, args);
        infoMsgBackend(code, buf, "main database and script processing");
    }

    // Message handler for FIO.
    //
    void
    ifInfoMessageFIO(INFOmsgType code, const char *string, va_list args)
    {
        char buf[512];
        if (!string)
            string = "";
        vsnprintf(buf, 512, string, args);
        infoMsgBackend(code, buf, "layout file i/o");
    }

    // Message handler for GEO.
    //
    void
    ifInfoMessageGEO(INFOmsgType code, const char *string, va_list args)
    {
        char buf[512];
        if (!string)
            string = "";
        vsnprintf(buf, 512, string, args);
        infoMsgBackend(code, buf, "geometry engine");
    }

    // Called before i/o operation starts, set up the log file and
    // perform other initialization.  Returns a file pointer to the
    // log file (named from fname).  If null is returned, no log will
    // be kept.
    //
    FILE *
    ifInitCvLog(const char *fname)
    {
        FILE *fp = 0;
        if (fname) {
            if (XM()->PanicFp())
                fprintf(XM()->PanicFp(),
                    "# Opening new log ------------------\n");
            if (XM()->RunMode() == ModeBatch) {
                if (filestat::create_bak(fname))
                    fp = fopen(fname, "w");
                else {
                    DSPpkg::self()->ErrPrintf(ET_ERROR, "%s",
                        filestat::error_msg());
                }
            }
            else
                fp = Log()->OpenLog(fname, "w");
            if (fp)
                fprintf(fp, "# %s\n", XM()->IdString());
            else
                PL()->ShowPrompt(
                    "Warning: Can't open log file.  Continuing anyway.");
        }
        DSPpkg::self()->SetWorking(true);
        Cvt()->SetLogFp(fp);
        Cvt()->SetShowLog(false);
        setup_two_col_msgs(true);
        return (fp);
    }

    // Process a message emitted during file i/o.  The message is
    // formatted and printed to the file pointer returned from InitCvLog.
    //
    void
    ifPrintCvLog(OUTmsgType msgtype, const char *fmt, va_list args)
    {
        char buf[BUFSIZ];
        *buf = 0;
        char *sprm = buf;
        if (msgtype == IFLOG_WARN) {
            strcpy(buf, "**  Warning: ");
            sprm = buf + 4;
            if (Cvt()->LogFp())
                Cvt()->SetShowLog(true);
        }
        else if (msgtype == IFLOG_FATAL) {
            strcpy(buf, "*** Error: ");
            sprm = buf + 4;
            if (Cvt()->LogFp())
                Cvt()->SetShowLog(true);
        }
        char *s = buf + strlen(buf);

        vsnprintf(s, BUFSIZ - (s - buf), fmt, args);

        if (Cvt()->LogFp())
            fprintf(Cvt()->LogFp(), "%s\n", buf);
        if (XM()->PanicFp())
            fprintf(XM()->PanicFp(), "%s\n", buf);
        if (msgtype == IFLOG_INFO_SHOW || msgtype == IFLOG_FATAL)
            PL()->ShowPrompt(sprm);
    }

    // Close the file pointer to the log file.  The first argument is the
    // return from InitCvLog, and if nonzero should be closed in this
    // function.  The second arg is the return from the i/o operation, and
    // the third argument is the file name passed to InitCvLog.  This is
    // called when the operation is complete.
    //
    void
    ifShowCvLog(FILE *fp, OItype oiret, const char *fname)
    {
        setup_two_col_msgs(false);
        Cvt()->SetLogFp(0);
        DSPpkg::self()->SetWorking(false);
        if (!fp) {
            Cvt()->SetShowLog(false);
            return;
        }
        fclose(fp);
        if (!Cvt()->ShowLog())
            return;
        Cvt()->SetShowLog(false);
        if (XM()->PanicFp())
            return;
        if (oiret != OIaborted && fname &&
                !CDvdb()->getVariable(VA_NoPopUpLog)) {
            char buf[256];
            if (Log()->LogDirectory() && *Log()->LogDirectory()) {
                sprintf(buf, "%s/%s", Log()->LogDirectory(), fname);
                fname = buf;
            }
            DSPmainWbag(PopUpFileBrowser(fname))
        }
    }

    // Handle database name collision.  This is intended to start a
    // process that will pop-up a dialog when a cell is being read
    // whose name is already in the database.  This is called for each
    // cell name clash.
    //
    void
    ifMergeControl(mitem_t *mi)
    {
        Cvt()->PopUpMergeControl(MODE_ON, mi);
    }

    // Terminate handling symbol name collisions.
    //
    void
    ifMergeControlClear()
    {
        Cvt()->PopUpMergeControl(MODE_OFF, 0);
    }

    // This initiates a pop-up which allows the user to click-select from
    // among the list of items.  It is used to select cells from a library
    // listing, and to select the top-level cell of interest when opening
    // an archive file with multiple top-level cells.
    // Arguments:
    //   sl         string list of entries
    //   msg        list title string
    //   fname      name of originating file
    //   callback   function called with entry and arg when user clicks
    //   arg        argument passed to callback
    //
    // Archive opening functions that require this callback will return an
    // "ambiguous" error.  The open is completed when callback is called.
    //
    void
    ifAmbiguityCallback(stringlist *sl, const char *msg,
        const char *fname, void (*callback)(const char*, void*), void *arg)
    {
        sLstr lstr;
        if (msg && *msg) {
            lstr.add(msg);
            lstr.add_c('\n');
        }
        lstr.add(fname);

        DSPmainWbag(PopUpMultiCol(sl, lstr.string(), callback, arg, 0))
    }

    bool
    ifOpenSubMaster(CDp *pd, CDs **psd)
    {
        if (psd)
            *psd = 0;
        CDp *psv = 0;
        CDp *pcn = 0;
        CDp *pcp = 0;
        for (CDp *p = pd; p; p = p->next_prp()) {
            if (p->value() == XICP_STDVIA) {
                psv = p;
                break;
            }
            if (p->value() == XICP_PC) {
                pcn = p;
                if (pcp)
                    break;
            }
            if (p->value() == XICP_PC_PARAMS) {
                pcp = p;
                if (pcn)
                    break;
            }
        }
        if (psv) {
            CDs *sd = Tech()->OpenViaSubMaster(psv->string());
            if (psd)
                *psd = sd;
            if (!sd) {
                // Ignore errors here, an unresolved via will be
                // reported later.
                // The caller doesn't use the message anyway!

                Errs()->get_error();
            }
        }
        else if (pcn && pcp) {
            char *lib = 0;
            const char *c = pcn->string();
            if (c && c[0] == '<' && c[1] != '[') {
                // Must be OA.
                const char *d = strchr(c, '>');
                if (!d)
                    return (true);
                c++;
                if (!FIO()->IsEvalOaPCells())
                    return (true);
                int len = d - c;
                lib = new char[len+1];
                strncpy(lib, c, len);
                lib[len] = 0;
                bool op = false;
                OAif()->is_lib_open(lib, &op);
                delete [] lib;
                if (!op)
                    return (true);
            }
            else {
                // Must be native.  If native, the string is a path to
                // the super master cell file.

                if (FIO()->IsNoEvalNativePCells())
                    return (true);
            }

            if (!PC()->openSubMaster(pcn->string(), pcp->string(), psd, 0)) {
                // If we fail to find the super-master, just continue.
                // The archive may contain the sub-masters.
                // The caller doesn't use the message anyway!
                //
                // This will spew error messages if OA access fails.

                Errs()->get_error();
            }
        }
        return (true);
    }

    // Handle pcell sub-master generation and checking.
    //
    bool
    ifReopenSubMaster(CDs *sd)
    {
        return (PC()->reopenSubMaster(sd));
    }

    // If cbin is not null, attempt to open a cell through the
    // OpenAccess libraries.  Return true and fill in cbin on success. 
    // Otherwise return true if the cell exists in OA.  The library
    // can be null, which indicates to search all *open* OA libraries.
    //
    bool
    ifOpenOA(const char *libname, const char *cellname, CDcbin *cbin)
    {
        if (cbin) {
            if (libname && *libname) {
                bool haslib;
                if (OAif()->is_library(libname, &haslib) && haslib) {
                    bool hascell;
                    if (OAif()->is_oa_cell(cellname, true, &hascell) &&
                            hascell) {
                        if (!OAif()->load_cell(libname, cellname, 0,
                                CDMAXCALLDEPTH, false))
                            return (false);
                        return (CDcdb()->findSymbol(cellname, cbin));
                    }
                }
                return (false);
            }
            return (OAif()->open_lib_cell(cellname, cbin) == OIok);
        }
        // Silence errors.
        if (libname && *libname) {
            bool haslib;
            if (!OAif()->is_library(libname, &haslib) || !haslib) {
                Errs()->get_error();
                return (false);
            }
            bool hascell;
            if (!OAif()->is_cell_in_lib(libname, cellname, &hascell)) {
                Errs()->get_error();
                return (false);
            }
            return (hascell);
        }
        bool hascell;
        if (!OAif()->is_oa_cell(cellname, true, &hascell)) {
            Errs()->get_error();
            return (false);
        }
        return (hascell);
    }

    // The internal list of open libraries changed.
    //
    void
    ifLibraryChange()
    {
        Cvt()->PopUpLibraries(0, MODE_UPD);
    }

    // The search path in PGetPath() has changed.  Have to set the
    // variable to update.
    //
    void
    ifPathChange()
    {
        CDvdb()->setVariable(VA_Path, FIO()->PGetPath());
    }
}


//-----------------------------------------------------------------------------
// CD and GEO configuration

// reuse ifPrintCvLog above

namespace {
    void 
    ifChdChange()
    {
        Cvt()->PopUpHierarchies(0, MODE_UPD);
    }

    void
    ifCgdChange()
    {
        Cvt()->PopUpGeometries(0, MODE_UPD);
    }
}


void
cConvert::setupInterface()
{
    CD()->RegisterIfInfoMessage(ifInfoMessageCD);
    CD()->RegisterIfPrintCvLog(ifPrintCvLog);
    CD()->RegisterIfChdDbChange(ifChdChange);
    CD()->RegisterIfCgdDbChange(ifCgdChange);

    GEO()->RegisterIfInfoMessage(ifInfoMessageGEO);

    FIO()->RegisterIfSetWorking(ifSetWorking);
    FIO()->RegisterIfInfoMessage(ifInfoMessageFIO);
    FIO()->RegisterIfInitCvLog(ifInitCvLog);
    FIO()->RegisterIfPrintCvLog(ifPrintCvLog);
    FIO()->RegisterIfShowCvLog(ifShowCvLog);
    FIO()->RegisterIfMergeControl(ifMergeControl);
    FIO()->RegisterIfMergeControlClear(ifMergeControlClear);
    FIO()->RegisterIfAmbiguityCallback(ifAmbiguityCallback);
    FIO()->RegisterIfOpenSubMaster(ifOpenSubMaster);
    FIO()->RegisterIfReopenSubMaster(ifReopenSubMaster);
    FIO()->RegisterIfOpenOA(ifOpenOA);
    FIO()->RegisterIfLibraryChange(ifLibraryChange);
    FIO()->RegisterIfPathChange(ifPathChange);
}

