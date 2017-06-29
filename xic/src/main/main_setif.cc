
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
 $Id: main_setif.cc,v 5.55 2016/03/02 00:39:46 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "fio.h"
#include "editif.h"
#include "scedif.h"
#include "drcif.h"
#include "extif.h"
#include "pcell.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "select.h"
#include "promptline.h"
#include "events.h"
#include "tech_layer.h"
#include "menu.h"
#include "ghost.h"
#include "timer.h"
#include "view_menu.h"
#include "filestat.h"


//-----------------------------------------------------------------------------
// CD configuration

namespace {
    // The object odesc is being added to sdesc.
    //
    void
    ifObjectInserted(CDs *sdesc, CDo *odesc)
    {
        if (!sdesc || !odesc)
            return;

        // reset extraction flags
        if (sdesc->isElectrical()) {
            sdesc->unsetConnected();
            CDcbin cbin(sdesc);
            if (cbin.phys())
                // The terminals are assigned during extraction in parent,
                // have to extract again since terminals may have changed.
                cbin.phys()->reflectBadExtract();
        }
        else {
            sdesc->reflectBadExtract();
            sdesc->unsetConnected();
            if (odesc->type() == CDINSTANCE ||
                    !(*sdesc->BB() > odesc->oBB()))
                sdesc->reflectBadGroundPlane();
            else {
                CDl *ld = odesc->ldesc();
                if (ld->isGroundPlane() && ld->isDarkField())
                    sdesc->reflectBadGroundPlane();
            }
        }
    }

    void
    ifNewLayer(CDl *ld)
    {
        ld->setDspData(new DspLayerParams(ld));
        ld->setAppData(new TechLayerParams(ld));
    }

    // The cell is being destroyed.
    //
    void
    ifInvalidateCell(CDs *sdesc)
    {
        // If this is the current or top cell, clear references.
        if (sdesc->cellname() == DSP()->TopCellName())
            XM()->ClearReferences(true, true);
        else if (sdesc->cellname() == DSP()->CurCellName())
            XM()->ClearReferences(false);

        // Purge any associated rulers.
        XM()->EraseRulers(sdesc, 0, 0);

        // Destroy connectivity data
        if (sdesc->isElectrical())
            ScedIf()->destroyNodes(sdesc);
        else
            ExtIf()->destroyGroups(sdesc);

        // Clear user marks
        DSP()->ClearUserMarks(sdesc);
    }

    // The object odesc is being removed.  If save is true, the object is
    // being unlinked but not destroyed.
    //
    void
    ifInvalidateObject(CDs *sdesc, CDo *odesc, bool save)
    {
        if (!sdesc || !odesc)
            return;
        Selections.removeObject(sdesc, odesc);
        EditIf()->invalidateObject(sdesc, odesc, save);

        // reset extraction flags
        if (sdesc->isElectrical()) {
            sdesc->unsetConnected();
            CDcbin cbin(sdesc);
            if (cbin.phys())
                // The terminals are assigned during extraction in
                // parent, have to extract again since terminals may
                // have changed.
                cbin.phys()->reflectBadExtract();
        }
        else {
            ExtIf()->clearGroups(sdesc);
            sdesc->reflectBadExtract();
            sdesc->unsetConnected();
            if (odesc->type() == CDINSTANCE ||
                    !(*sdesc->BB() > odesc->oBB()))
                sdesc->reflectBadGroundPlane();
            else {
                CDl *ld = odesc->ldesc();
                if (ld->isGroundPlane() && ld->isDarkField())
                    sdesc->reflectBadGroundPlane();
            }
        }
    }

    // Everything on layer ld is being deleted, within sdesc if sdesc is
    // not null, or globally if sdesc is null.
    //
    void
    ifInvalidateLayerContent(CDs *sd, CDl *ld)
    {
        if (sd)
            Selections.removeLayer(sd, ld);
        else
            Selections.removeAllLayer(ld);
        EditIf()->invalidateLayer(sd, ld);
    }

    // Return a static string which will be printed near the top of
    // certain created files.  Can contain the application version, date,
    // etc.
    //
    const char *
    ifIdString()
    {
        return (XM()->IdString());
    }

    // Called when a variable has been set, cleared, or modified.
    //
    void
    ifVariableChange()
    {
        XM()->PopUpVariables(false);
    }

    // Check for interrupt, return true if the current operation should be
    // aborted.  The argument is a message string for the confirmation
    // pop-up.
    //
    bool
    ifCheckInterrupt(const char *msg)
    {
        if (DSP()->MainWdesc() && DSP()->MainWdesc()->Wdraw())
            dspPkgIf()->CheckForInterrupt();
        return (XM()->ConfirmAbort(msg));
    }

    // On interrupt, by default the user is prompted whether to
    // continue or abort the operation.  If this flag is set, the
    // prompt is skipped, and the operation is aborted.
    //
    void
    ifNoConfirmAbort(bool b)
    {
        XM()->SetNoConfirmAbort(b);
    }
}


//-----------------------------------------------------------------------------
// GEO configuration

namespace {
    void ifSaveZlist(const Zlist *zl, const char *lname)
    {
        CDl *ld = CDldb()->newLayer(lname, Physical);
        if (ld)
            Zlist::add(zl, CurCell(), ld, false, false);
    }

    void ifShowAndPause(const char *prompt)
    {
        DSP()->RedisplayAll();
        PL()->EditPrompt(prompt, "");
    }

    void ifClearLayer(const char *lname)
    {
        CDl *ld = CDldb()->findLayer(lname, Physical);
        if (ld) {
            CurCell()->db_clear_layer(ld);
            CurCell()->setBBvalid(false);
            CurCell()->computeBB();
        }
    }
}


//-----------------------------------------------------------------------------
// FIO configuration

namespace {
    // Return the name used for the device library file, which
    // defaults to "device.lib" if this function returns null.
    //
    const char *
    ifDeviceLibName()
    {
        return (XM()->DeviceLibName());
    }
}


//-----------------------------------------------------------------------------
// DSP configuration

namespace {
    // Show the application-specific highlighting, called after redisplay.
    //
    void
    window_show_highlighting(WindowDesc *wd)
    {
        if (wd->DbType() == WDcddb) {

            // show DRC errors
            DrcIf()->showCurError(wd, DISPLAY);

            // show extraction highlighting
            ExtIf()->windowShowHighlighting(wd);

            // highlight selected objects
            Selections.show(wd);
        }

        // hard copy frame (main win only)
        if (DSP()->DoingHcopy() && wd == DSP()->MainWdesc())
            XM()->HCdrawFrame(DISPLAY);
    }

    // Show the application-specific blinking highlighting.
    //
    void
    window_show_blinking(WindowDesc *wd)
    {
        ExtIf()->windowShowBlinking(wd);
    }

    // The display area in the window has changed.
    //
    void
    window_view_change(WindowDesc *wd)
    {
        if (wd == DSP()->MainWdesc())
            XM()->ShowParameters();
        wd->Wbag()->PopUpZoom(0, MODE_UPD);
    }

    // A view is being saved.  The indx is the value returned from
    // wStack::add_view().
    //
    void
    window_view_saved(WindowDesc *wd, int indx)
    {
        if (indx >= 0) {
            MenuEnt *ent = Menu()->FindEntOfWin(wd, MenuVIEW);
            if (ent) {
                char buf[8];
                sprintf(buf, "%c    ", 'A'+indx);
                Menu()->NewDDentry(ent->cmd.caller, buf);
            }
            PL()->ShowPromptV("Current view assigned to: %s",
                wd->Views()->view()->name);
        }
        else
            PL()->ShowPromptV("Current view replacing: %s",
                wd->Views()->view()->name);
    }

    // The view stack is being cleared.
    //
    void
    window_clear_views(WindowDesc *wd)
    {
        MenuEnt *ent = Menu()->FindEntOfWin(wd, MenuVIEW);
        if (ent)
            Menu()->NewDDmenu(ent->cmd.caller, XM()->ViewList());
    }

    // Update the menu for the subwindow according to the mode.
    //
    void
    window_mode_change(WindowDesc *wd)
    {
        for (int i = 1; i < DSP_NUMWINS; i++) {
            if (wd == DSP()->Window(i)) {
                Menu()->SwitchSubwMenu(i, wd->Mode());
                Menu()->InitAfterModeSwitch(i);
                return;
            }
        }
        if (wd->Mode() == Electrical && wd->DbType() == WDcddb) {
            CDs *cursde = CDcdb()->findCell(wd->CurCellName(), Electrical);
            if (cursde)
                ScedIf()->connectAll(false, cursde);
        }
    }

    // Get long and short strings for window titles.
    //
    void
    window_get_title_strings(const char **strlong, const char **strshort)
    {
        static char *slng, *ssht;
        if (!slng) {
            sLstr lstr;
            lstr.add("Whiteley Research Inc.  ");
            lstr.add(XM()->Product());
            lstr.add_c('-');
            lstr.add(XM()->VersionString());
            lstr.add(" ic ");
            lstr.add(XM()->Description());
            if (!EditIf()->hasEdit())
                lstr.add(" (featureset: viewer)");
            else if (!ExtIf()->hasExtract())
                lstr.add(" (featureset: editor)");
            slng = lstr.string_trim();
            ssht = lstring::copy(XM()->Product());
            if (!ssht)
                ssht = lstring::copy("");
        }
        if (strlong)
            *strlong = slng;
        if (strshort)
            *strshort = ssht;
    }

    // The window is being destroyed.
    //
    void
    window_destroy(WindowDesc *wd)
    {
        if (EV()->CurrentWin() == wd)
            EV()->SetCurrentWin(0);
        if (EV()->KeypressWin() == wd)
            EV()->SetKeypressWin(0);
        DrcIf()->windowDestroyed(wd->WinNumber());
    }

    // Let the current command specify if two windows are "similar". 
    // Returning true indicates similarity, returning false follows
    // the default determination.
    //
    bool
    check_similar(const WindowDesc *wd1, const WindowDesc *wd2)
    {
        CmdState *cmd = EV()->CurCmd();
        if (cmd)
            return (cmd->check_similar(wd1, wd2));
        return (false);
    }

    // This is called when the display idle proc concludes, and the
    // first such notification starts the ghosting.  If the default
    // snap-point ghosting starts too early, residuals are left,
    // possibly caused by switching the cursor to/from the busy cursor
    // on top of the ghost.  This bit of hackery seems to avoid the
    // residuals problem.
    //
    void
    display_done()
    {
        Gst()->StartGhost();
    }
}


void
cMain::setupInterface()
{
    CD()->RegisterIfObjectInserted(ifObjectInserted);
    CD()->RegisterIfNewLayer(ifNewLayer);
    CD()->RegisterIfInvalidateCell(ifInvalidateCell);
    CD()->RegisterIfInvalidateObject(ifInvalidateObject);
    CD()->RegisterIfInvalidateLayerContent(ifInvalidateLayerContent);
    CD()->RegisterIfCheckInterrupt(ifCheckInterrupt);
    CD()->RegisterIfNoConfirmAbort(ifNoConfirmAbort);
    CD()->RegisterIfIdString(ifIdString);
    CD()->RegisterIfVariableChange(ifVariableChange);

    GEO()->RegisterIfSaveZlist(ifSaveZlist);
    GEO()->RegisterIfShowAndPause(ifShowAndPause);
    GEO()->RegisterIfClearLayer(ifClearLayer);

    FIO()->RegisterIfDeviceLibName(ifDeviceLibName);

    DSP()->Register_window_show_highlighting(window_show_highlighting);
    DSP()->Register_window_show_blinking(window_show_blinking);
    DSP()->Register_window_view_change(window_view_change);
    DSP()->Register_window_view_saved(window_view_saved);
    DSP()->Register_window_clear_views(window_clear_views);
    DSP()->Register_window_mode_change(window_mode_change);
    DSP()->Register_window_get_title_strings(window_get_title_strings);
    DSP()->Register_window_destroy(window_destroy);
    DSP()->Register_check_similar(check_similar);
    DSP()->Register_notify_display_done(display_done);
}

