
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
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
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: toolbar.h,v 2.58 2016/02/09 18:58:48 stevew Exp $
 *========================================================================*/

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include "wlist.h"
#include "graphics.h"


// arg to UpdateMain();
enum ResUpdType { RES_BEGIN, RES_UPD, RES_UPD_TIME };

// arg to PopUpTBhelp
enum TBH_type { TBH_CM, TBH_DB, TBH_PD, TBH_SH, TBH_SD, TBH_end };


// Interface for the "toolbar" which is a small control panel on-screen
// if a graphics window driver is available.
//
struct sToolbar
{
    virtual ~sToolbar() { }

    // timer/idle utilities
    virtual int RegisterIdleProc(int(*)(void*), void*) = 0;
    virtual bool RemoveIdleProc(int) = 0;
    virtual int RegisterTimeoutProc(int, int(*)(void*), void*) = 0;
    virtual bool RemoveTimeoutProc(int) = 0;
    virtual void RegisterBigForeignWindow(unsigned int) = 0;

    // command defaults popup
    virtual void PopUpCmdConfig(int, int) = 0;
    virtual void PopDownCmdConfig() = 0;

    // color control popup
    virtual void PopUpColors(int, int) = 0;
    virtual void PopDownColors() = 0;
    virtual void UpdateColors(const char*) = 0;
    virtual void LoadResourceColors() = 0;

    // debugging popup
    virtual void PopUpDebugDefs(int, int) = 0;
    virtual void PopDownDebugDefs() = 0;

    // plot defaults popup
    virtual void PopUpPlotDefs(int, int) = 0;
    virtual void PopDownPlotDefs() = 0;

    // shell defaults popup
    virtual void PopUpShellDefs(int, int) = 0;
    virtual void PopDownShellDefs() = 0;

    // simulation defaults popup
    virtual void PopUpSimDefs(int, int) = 0;
    virtual void PopDownSimDefs() = 0;

    // misc listing popups
    virtual void SuppressUpdate(bool) = 0;
    virtual void PopUpPlots(int, int) = 0;
    virtual void PopDownPlots() = 0;
    virtual void UpdatePlots(int) = 0;
    virtual void PopUpVectors(int, int) = 0;
    virtual void PopDownVectors() = 0;
    virtual void UpdateVectors(int) = 0;
    virtual void PopUpCircuits(int, int) = 0;
    virtual void PopDownCircuits() = 0;
    virtual void UpdateCircuits() = 0;
    virtual void PopUpFiles(int, int) = 0;
    virtual void PopDownFiles() = 0;
    virtual void UpdateFiles() = 0;
    virtual void PopUpTrace(int, int) = 0;
    virtual void PopDownTrace() = 0;
    virtual void UpdateTrace() = 0;
    virtual void PopUpVariables(int, int) = 0;
    virtual void PopDownVariables() = 0;
    virtual void UpdateVariables() = 0;

    // main popup and a few more
    virtual void Toolbar() = 0;
    virtual void PopUpBugRpt(int, int) = 0;
    virtual void PopDownBugRpt() = 0;
    virtual void PopUpFont(int, int) = 0;
    virtual void PopDownFont() = 0;
    virtual void PopUpTBhelp(GRobject, GRobject, TBH_type) = 0;
    virtual void PopDownTBhelp(TBH_type) = 0;
    virtual void PopUpSpiceErr(bool, const char*) = 0;
    virtual void PopUpSpiceMessage(const char*, int, int) = 0;
    virtual void UpdateMain(ResUpdType) = 0;
    virtual void CloseGraphicsConnection() = 0;

    virtual void PopUpInfo(const char*) = 0;
    virtual void PopUpNotes() = 0;
};
extern sToolbar *ToolBar();

//
// Error message handling/display base class.
//

// Name of default errors file.
#define ERR_FILENAME "wrspice.errors"

// Nominal number of displayed error message lines.
#define MAX_ERR_LINES 200

// Functions are defined in error.cc.

// This should be used to derive the error display window.
//
class cMsgHdlr
{
public:
    cMsgHdlr()
        {
            mh_list = 0;
            mh_list_end = 0;
            mh_proc_id = 0;
            mh_count = 0;
            mh_file_created = false;
        }

    virtual ~cMsgHdlr()
        {
            wordlist::destroy(mh_list);
        }

    void first_message(const char*);
    void append_message(const char*);

private:
    // This is provided by the display code.  It adds the text to the
    // end of the display, truncating lines at the beginning to keep
    // the displayed history length reasonable.
    //
    virtual void stuff_msg(const char*) = 0;

    static int mh_idle(void*);

    wordlist *mh_list;
    wordlist *mh_list_end;
    int mh_proc_id;
    int mh_count;
    bool mh_file_created;
};

#endif

