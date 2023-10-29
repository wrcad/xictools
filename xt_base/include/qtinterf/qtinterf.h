
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTINTERF_H
#define QTINTERF_H

#include "ginterf/graphics.h"
#include <string.h>
#include <QEventLoop>
#include "qtinterf/qtdraw.h"

//
//  Main header for the QT-5 library
//

class QCursor;
class QWidget;

#define NUMITEMS(x)  sizeof(x)/sizeof(x[0])

namespace qtinterf {
    class QTtimer;
    class QTidleproc;
    class QTfileDlg;
    class QTfontDlg;
    class QTlistDlg;
    class QTaffirmDlg;
    class QTnumDlg;
    class QTledDlg;
    class QTmsgDlg;
    class QTprintDlg;
    class QTtextDlg;
    class QTtextEdit;
    class QTbag;
    class QTdev;

    // An event loop that can be linked into a list.
    //
    class event_loop : public QEventLoop
    {
    public:
        event_loop(event_loop *nx) : QEventLoop(0)
        {
            next = nx;
        }
        event_loop *next;
    };
}

using namespace qtinterf;

class qtinterf::QTbag : virtual public GRwbag
{
public:
    QTbag(QWidget* = 0);
    virtual ~QTbag();

    // QT-SPECIFIC
    QWidget *ItemFromTicket(GRobject);

    // misc. access to protected members

    void MonitorAdd(GRpopup *w)     { wb_monitor.add(w); }
    void MonitorRemove(GRpopup *w)  { wb_monitor.remove(w); }
    bool MonitorActive(GRpopup *w)  { return (wb_monitor.is_active(w)); }

    QWidget *Shell()                { return (wb_shell); }
    QTprintDlg *HC()                { return (wb_hc); }
    void SetHC(QTprintDlg *p)       { wb_hc = p; }
    QTtextEdit *TextArea()          { return (wb_textarea); }

    // Pass a title for the window and icon.
    //
    void Title(const char*, const char*);

    // qtedit.cc
    GReditPopup *PopUpTextEditor(const char*,
        bool(*)(const char*, void*, XEtype), void*, bool);
    GReditPopup *PopUpFileBrowser(const char*);
    GReditPopup *PopUpStringEditor(const char*,
        bool(*)(const char*, void*, XEtype), void*);
    GReditPopup *PopUpMail(const char*, const char*,
        void(*)(GReditPopup*) = 0, GRloc = GRloc());

    // qtfile.cc
    GRfilePopup *PopUpFileSelector(FsMode, GRloc,
        void(*)(const char*, void*),
        void(*)(GRfilePopup*, void*), void*, const char*);

    // qtfont.cc
    void PopUpFontSel(GRobject, GRloc, ShowMode,
        void(*)(const char*, const char*, void*),
        void*, int, const char** = 0, const char* = 0);

    // qthcopy.cc
    void PopUpPrint(GRobject, HCcb*, HCmode, GRdraw* = 0);
    void HCupdate(HCcb*, GRobject);
    void HCsetFormat(int);
    void HcopyDisableMsgs();

    // Override these when printing support for a widget collection
    // is needed.
    virtual char *GetPostscriptText(int, const char*, const char*,
        bool, bool)
                                    { return (0); } // see htm/htm_text.cc
    virtual char *GetPlainText()    { return (0); }
    virtual char *GetHtmlText()     { return (0); }

    // qthelp.cc
    bool PopUpHelp(const char*);

    // qtlist.cc
    GRlistPopup *PopUpList(stringlist*, const char*, const char*,
        void(*)(const char*, void*), void*, bool, bool);

    // qtmcol.cc
    GRmcolPopup *PopUpMultiCol(stringlist*, const char*,
        void(*)(const char*, void*), void*, const char**,
        int = 0, bool = false);

    // utilities
    void ClearPopups();
    void ClearPopup(GRpopup*);

    GRaffirmPopup *PopUpAffirm(GRobject, GRloc, const char*,
        void(*)(bool, void*), void*);
    GRnumPopup *PopUpNumeric(GRobject, GRloc, const char*,
        double initd, double, double, double, int,
        void(*)(double, bool, void*), void*);
    GRledPopup *PopUpEditString(GRobject, GRloc, const char*,
        const char*, ESret(*)(const char*, void*), void*, int,
        void(*)(bool), bool = false, const char* = 0);
    void PopUpInput(const char*, const char*, const char*,
        void(*)(const char*, void*), void*, int=0);
    GRmsgPopup *PopUpMessage(const char*, bool, bool = false,
        bool = false, GRloc = GRloc());
    int PopUpWarn(ShowMode, const char*, STYtype = STY_NORM,
        GRloc = GRloc());
    int PopUpErr(ShowMode, const char*, STYtype = STY_NORM,
        GRloc = GRloc());
    GRtextPopup *PopUpErrText(const char*, STYtype = STY_NORM,
        GRloc = GRloc(LW_UL));
    int PopUpInfo(ShowMode, const char*, STYtype = STY_NORM,
        GRloc = GRloc(LW_LL));
    int PopUpInfo2(ShowMode, const char*, bool(*)(bool, void*),
        void*, STYtype = STY_NORM, GRloc = GRloc(LW_LL));
    int PopUpHTMLinfo(ShowMode, const char*, GRloc = GRloc(LW_LL));

    GRledPopup      *ActiveInput();
    GRmsgPopup      *ActiveMessage();
    GRtextPopup     *ActiveInfo();
    GRtextPopup     *ActiveInfo2();
    GRtextPopup     *ActiveHtinfo();
    GRtextPopup     *ActiveWarn();
    GRtextPopup     *ActiveError();
    GRfontPopup     *ActiveFontsel();

    void            SetErrorLogName(const char*);

    // QT-SPECIFIC
    static QColor   PopupColor(GRattrColor);

protected:
    QWidget         *wb_shell;      // top level widget
    QTtextEdit      *wb_textarea;   // text widget
    QTledDlg        *wb_input;      // dialog input popup
    QTmsgDlg        *wb_message;    // message popup
    QTtextDlg       *wb_info;       // info popup
    QTtextDlg       *wb_info2;      // two button info popup
    QTtextDlg       *wb_htinfo;     // html info popup
    QTtextDlg       *wb_warning;    // warning popup
    QTtextDlg       *wb_error;      // error popup
    QTfontDlg       *wb_fontsel;    // font select popup
    QTprintDlg      *wb_hc;         // hard copy parameters
    GRmonList       wb_monitor;     // certain popups use this
    void            *wb_call_data;  // internal data
    void            (*wb_sens_set)(QTbag*, bool, int);
                                    // sensitivity change callback
    int             wb_warn_cnt;    // counters for window id
    int             wb_err_cnt;
    int             wb_info_cnt;
    int             wb_info2_cnt;
    int             wb_htinfo_cnt;

    // These are used in a lot of places.
    static const char *wb_closed_folder_xpm[];
    static const char *wb_open_folder_xpm[];
};


// QT driver class
//
class qtinterf::QTdev : public GRscreenDev
{
public:
    QTdev();
    ~QTdev();

    // virtual overrides
    // qtinterf.cc
    bool        Init(int*, char**);
    bool        InitColormap(int, int, bool);
    void        RGBofPixel(int, int*, int*, int*);
    int         AllocateColor(int*, int, int, int);
    int         NameColor(const char*);
    bool        NameToRGB(const char*, int*);
    GRdraw      *NewDraw(int);
    GRwbag      *NewWbag(const char*, GRwbag*);
    int         AddTimer(int, int(*)(void*), void*);
    void        RemoveTimer(int);
    int         AddIdleProc(int(*)(void*), void*);
    void        RemoveIdleProc(int);
    GRsigintHdlr RegisterSigintHdlr(GRsigintHdlr);
    bool        CheckForEvents();
    int         Input(int, int, int*);
    void        MainLoop(bool=false);
    int         LoopLevel()             { return (dv_loop_level); }
    void        BreakLoop();
    // virtual override, qthcopy.cc
    void HCmessage(const char*);

    // Remaining functions are unique to class.

    // This can be set by the application to the main frame widget
    // bag, in which case certain pop-ups called from other pop-ups
    // will be rooted in the main window, rather than the pop-up's
    // window.
    //
    void RegisterMainFrame(QTbag *w)    { dv_main_bag = w; }
    QTbag *MainFrame()                  { return (dv_main_bag); }

#ifdef WIN32
    // Flag for DOS/UNIX line termination in output.
    bool GetCRLFtermination()           { return (dv_crlf_terminate); }
    void SetCRLFtermination(bool b)     { dv_crlf_terminate = b; }
#endif

    static QTdev *self()
    {
        if (!instancePtr)
            on_null_ptr();
        return (instancePtr);
    }

    static bool exists()
    {
        return (instancePtr != 0);
    }

    // qtinterf.cc
    void Location(GRobject, int*, int*);
    void SetPopupLocation(GRloc, QWidget*, QWidget*);
    void ComputePopupLocation(GRloc, QWidget*, QWidget*, int*, int*);

    static void Deselect(GRobject);
    static void Select(GRobject);
    static bool GetStatus(GRobject);
    static void SetStatus(GRobject, bool);
    static void CallCallback(GRobject);
    static const char *GetLabel(GRobject);
    static void SetLabel(GRobject, const char*);
    static void SetSensitive(GRobject, bool);
    static bool IsSensitive(GRobject);
    static void SetVisible(GRobject, bool);
    static bool IsVisible(GRobject);
    static void DestroyButton(GRobject);
    //static void SetFocus(GtkWidget*);
    static int  ConnectFd();
    static void PointerRootLoc(int*, int*);

private:
    static void on_null_ptr();

    event_loop      *dv_loop;       // event loop stack
    QTbag           *dv_main_bag;   // top level bag
    QTtimer         *dv_timers;     // list of timers
    QTidleproc      *dv_idle_ctrl;  // idle process control
    int             dv_minx;
    int             dv_miny;
    int             dv_loop_level;  // loop level
#ifdef WIN32
    bool            dv_crlf_terminate;
#endif

    static QTdev    *instancePtr;
};

#endif

