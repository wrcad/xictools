
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
#include "qtmain.h"
#include "qtmenu.h"
#include "qtcoord.h"
#include "qtparam.h"
#include "qtexpand.h"
#include "qtltab.h"
#include "qthtext.h"
#include "qtzoom.h"
#include "qtmenucfg.h"
#include "qtinterf/qtfile.h"
#include "qtinterf/qttext.h"
#include "qtinterf/qttextw.h"
#include "qtinterf/qtmsg.h"
#include "extif.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "ginterf/nulldev.h"
#include "promptline.h"
#include "fio.h"
#include "sced.h"
#include "tech.h"
#include "events.h"
#include "select.h"
#include "keymap.h"
#include "keymacro.h"
#include "errorlog.h"
#include "ghost.h"
#include "cd.h"
#include "cd_celldb.h"
#include "miscutil/pathlist.h"
#include "miscutil/tvals.h"
#include "help/help_context.h"
#include "qtinterf/qtidleproc.h"
#include "bitmaps/wr.xpm"
#ifdef HAVE_MOZY
#include "editif.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_macro.h"
#include "help/help_defs.h"
#include "help/help_context.h"
#include "htm/htm_widget.h"
#include "htm/htm_form.h"
#endif

#include <QApplication>
#include <QAction>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QSplitter>
#include <QLineEdit>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QToolButton>
#include <QScreen>
#include <QWheelEvent>


#include "file_menu.h"
#include "view_menu.h"
#include "attr_menu.h"

#include "../../icons/xic_16x16.xpm"
#include "../../icons/xic_32x32.xpm"
#include "../../icons/xic_48x48.xpm"


//-----------------------------------------------------------------------------
// Main Window and top-level functions.
// This file has definitions for:
// NULLwinApp/NULLwin
// QTpkg
// cMain
// QTeventMonitor
// cKeys
// QTsubwin
// QTmainwin
//
// Help system keywords used:
//  button4
//  button2
//  button3
//  keyspresd
//  xic:wrbtn

// Create and export the graphics package.
namespace { QTpkg _qt_; }

class NULLwinApp : virtual public cAppWinFuncs
{
public:
    // pixmap manipulations
    void SwitchToPixmap()                                           { }
    void SwitchFromPixmap(const BBox*)                              { }
    GRobject DrawableReset()                                { return (0); }
    void CopyPixmap(const BBox*)                                    { }
    void DestroyPixmap()                                            { }
    bool DumpWindow(const char*, const BBox*)               { return (false); }
    bool PixmapOk()                                         { return (false); }

    // key handling/display interface
    void GetTextBuf(char*)                                          { }
    void SetTextBuf(const char*)                                    { }
    void ShowKeys()                                                 { }
    void SetKeys(const char*)                                       { }
    void BspKeys()                                                  { }
    void CheckExec(bool)                                            { }
    char *KeyBuf()                                          { return (0); }
    int KeyPos()                                            { return (0); }

    // label
    void SetLabelText(const char*)                                  { }

    // misc. pop-ups
    void PopUpGrid(GRobject, ShowMode)                              { }
    void PopUpExpand(GRobject, ShowMode,
        bool(*)(const char*, void*), void*, const char*, bool)      { }
    void PopUpZoom(GRobject, ShowMode)                              { }
};

// Main and subwindow class for null graphics.
//
class NULLwin : public DSPwbag, public NULLwbag,  NULLwinApp,
    public NULLdraw
{
};


//-----------------------------------------------------------------------------
// QTpkg functions

namespace {
    // Callback for general purpose timer.  Set *set true when time limit
    // reached.
    //
    int timer_cb(void *client_data)
    {
        if (client_data)
            *((bool*)client_data) = true;
        return (false);
    }


#ifdef HAVE_MOZY

    void quit_help(void*)
    {
        XM()->QuitHelp();
    }


    //
    // This is the HTML form submission handler.  It sets the variables
    // from the form, and calls a script for processing.
    //

    // The form of the action string is the following token followed by the
    // name of the script (can be a path)
    //
#define ACTION_TOKEN "action_local_xic"

    void form_submit_hdlr(void *data)
    {
        static bool lock;  // the script interpreter isn't reentrant
        htmFormCallbackStruct *cbs = (htmFormCallbackStruct*)data;

        const char *t = cbs->action;
        char *tok = lstring::getqtok(&t);
        if (strcmp(tok, ACTION_TOKEN)) {
            QTpkg::self()->ErrPrintf(ET_ERROR,
                "unknown action_local submission.\n");
            delete [] tok;
            return;
        }
        delete [] tok;
        if (lock)
            return;
        lock = true;
        char *script = lstring::getqtok(&t);

        SIfile *sfp;
        stringlist *wl;
        XM()->OpenScript(script, &sfp, &wl);
        if (!sfp && !wl) {
            QTpkg::self()->ErrPrintf(ET_ERROR,
                "can't find action_local script.\n");
            delete [] script;
            lock = false;
            return;
        }

        siVariable *variables = 0;
        int scnt = 0;
        for (int i = 0; i < cbs->ncomponents; i++) {
            siVariable *v = new siVariable;
            v->name = lstring::copy(cbs->components[i].name);
            switch (cbs->components[i].type) {
            case FORM_NONE:
                delete v;
                continue;
            case FORM_SELECT:
                // deal with multiple selections
                if (i && cbs->components[i-1].type == FORM_SELECT &&
                        !strcmp(cbs->components[i-1].name,
                        cbs->components[i].name)) {
                    char buf[64];
                    snprintf(buf, 64, "%s_extra%d", cbs->components[i].name,
                        scnt);
                    delete [] v->name;
                    v->name = lstring::copy(buf);
                    scnt++;
                }
                else
                    scnt = 0;
                // fall through
            case FORM_OPTION:
            case FORM_PASSWD:
            case FORM_TEXT:
            case FORM_FILE:
            case FORM_TEXTAREA:
            case FORM_CHECK:
            case FORM_RADIO:
            case FORM_IMAGE:
            case FORM_HIDDEN:
            case FORM_RESET:
            case FORM_SUBMIT:
                v->type = TYP_STRING;
                v->content.string = lstring::copy(cbs->components[i].value);
                v->flags |= VF_ORIGINAL;
                break;
            }
            v->next = variables;
            variables = v;
        }

        EV()->InitCallback();
        CDs *cursd = CurCell();
        if (cursd) {

            // Preset "#define SUBMIT" when calling script.
            SImacroHandler *mh = new SImacroHandler;
            mh->parse_macro("SUBMIT", true);

            EditIf()->ulListCheck(script, cursd, false);
            SIparse()->presetVariables(variables);
            SI()->PresetMacros(mh);
            SI()->Interpret(sfp, wl, 0, 0);
            if (sfp)
                delete sfp;
            EditIf()->ulCommitChanges(true);
        }
        lock = false;
    }

#endif // HAVE_MOZY
}


// Create a new main_bag.  This will be passed to QTdev::New() for
// registration, then on to Initialize().
//
GRwbag *
QTpkg::NewGX()
{
    if (!MainDev())
        return (0);
    if (MainDev()->ident == _devNULL_)
        return (new NULLwin);
    if (MainDev()->ident == _devQT_)
        return (new QTmainwin);
    return (0);
}


namespace {
    // All QT text comes through here.
    // This function proto is a QtMessageHandler.
    void messageOutput(QtMsgType type, const QMessageLogContext&,
        const QString &smsg)
    {
        switch (type) {
        case QtDebugMsg:
            fprintf(stderr, "Debug: %s\n", (const char*)smsg.toLatin1());
            break;
        case QtInfoMsg:
            fprintf(stderr, "Info: %s\n", (const char*)smsg.toLatin1());
            break;
        case QtWarningMsg:
            fprintf(stderr, "Warning: %s\n", (const char*)smsg.toLatin1());
            break;
        case QtCriticalMsg:
            fprintf(stderr, "Critical: %s\n", (const char*)smsg.toLatin1());
            break;
        case QtFatalMsg:
            fprintf(stderr, "Fatal: %s\n", (const char*)smsg.toLatin1());
            break;
        }
    }
}


// Initialization function for the main window.  The argument *must* be
// a main_bag.
//
int
QTpkg::Initialize(GRwbag *wcp)
{
    if (!MainDev())
        return (true);
    qInstallMessageHandler(&messageOutput);
    if (MainDev()->ident == _devNULL_) {
        static QTltab qt_lt(true);
        static QTedit qt_hy(true);
        LT()->SetLtab(&qt_lt);
        PL()->SetEdit(&qt_hy);

        NULLwin *w = dynamic_cast<NULLwin*>(wcp);
        DSP()->SetWindow(0, new WindowDesc);
        EV()->SetCurrentWin(DSP()->MainWdesc());
        DSP()->MainWdesc()->SetWbag(w);
        DSP()->MainWdesc()->SetWdraw(w);

        DSP()->SetCurMode(XM()->InitialMode());

        DSP()->ColorTab()->init();
        DSP()->Initialize(600, 400, 0, true);
        LT()->InitLayerTable();

        return (false);
    }
    if (MainDev()->ident != _devQT_)
        return (true);

    QTmainwin *w = dynamic_cast<QTmainwin*>(wcp);
    if (!w)
        return (true);

    QTdev::self()->RegisterMainFrame(w);
    QTpkg::self()->RegisterMainWbag(w);
    w->initialize();
    w->show();

    QSize qs = w->Viewport()->size();
    DSP()->Initialize(qs.width(), qs.height(), 0,
        (XM()->RunMode() != ModeNormal));
    LT()->InitLayerTable();

#ifdef HAVE_MOZY
    // callback to tell application when quitting help
    HLP()->context()->registerQuitHelpProc(quit_help);
    HLP()->context()->registerFormSubmitProc(form_submit_hdlr);
#endif

    PL()->Init();
    QTedit::self()->init();
//    QTdev::self()->RegisterBigWindow(w->Shell());

    Gst()->SetGhost(GFnone);
    if (!QTmainwin::self())
        // Halt called
        return (true);
    return (false);
}


// Reinitialize the application to shut down all graphics.
//
void
QTpkg::ReinitNoGraphics()
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;
    if (!QTmainwin::self())
        return;
    DSPmainDraw(Halt());
    DSP()->MainWdesc()->SetWbag(0);
    DSP()->MainWdesc()->SetWdraw(0);
    CloseGraphicsConnection();

    PL()->SetNoGraphics();
    LT()->SetNoGraphics();

    QTpkg::self()->SetNullGraphics();
    EV()->SetCurrentWin(DSP()->MainWdesc());
    NULLwin *w = new NULLwin;
    DSP()->MainWdesc()->SetWbag(w);
    DSP()->MainWdesc()->SetWdraw(w);
}


// Halt application cleanup function.
//
void
QTpkg::Halt()
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;
//    QTdev::self()->RegisterBigWindow(0);
    EV()->InitCallback();
    if (DSP()->MainWdesc()) {
        DSP()->MainWdesc()->SetWbag(0);
        DSP()->MainWdesc()->SetWdraw(0);
    }
    XM()->SetAppReady(false);
}


// The main loop, does not return, but might be reentered with longjmp.
//
void
QTpkg::AppLoop()
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;

    QApplication::instance()->installEventFilter(&pkg_event_monitor);
    RegisterEventHandler(0, 0);
    pkg_in_main_loop = true;
    QTdev::self()->MainLoop(true);
}


// Set the interrupt flag and return true if a ^C event is found.
//
bool
QTpkg::CheckForInterrupt()
{
    static unsigned long lasttime;
    if (!pkg_in_main_loop)
        return (false);
    if (DSP()->Interrupt())
        return (true);

    // Cut expense if called too frequently.
    if (Tvals::millisec() == lasttime)
        return (false);
    lasttime = Tvals::millisec();

    QApplication::sendPostedEvents();
    QApplication::processEvents();
    if (DSP()->Interrupt())
        return (true);
    return (false);
}


// If state is true, iconify the application.  Return false if the
// application has not been realized.  Note that the map_hdlr()
// function takes care of the popups.
//
int
QTpkg::Iconify(int)
{
    if (!QTmainwin::self())
        return (false);
    QTmainwin::self()->showMinimized();
    return (true);
}


// Initialize popup subwindow number wnum (1 - DSP_NUMWINS-1).
//
bool
QTpkg::SubwinInit(int wnum)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return (false);
    if (!QTmainwin::self())
        return (false);
    if (wnum < 1 || wnum >= DSP_NUMWINS)
        return (false);
    new QTsubwin(wnum, QTmainwin::self());
    return (true);
}


void
QTpkg::SubwinDestroy(int wnum)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;
    WindowDesc *wdesc = DSP()->Window(wnum);
    if (!wdesc)
        return;
    delete dynamic_cast<QTsubwin*>(wdesc->Wbag());
}


// Increment/decrement the busy flag and switch to watch cursor.  If
// already busy when passing true, return false.
//
bool
QTpkg::SetWorking(bool busy)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return (false);

    // Have to be careful with reentrancy, specifically app_busy must
    // have its final value before calling the functions (which can
    // call back!).

    bool ready = true;
    if (busy) {
        if (!app_busy) {
            DSP()->SetInterrupt(DSPinterNone);
            if (!app_override_busy) {
                app_busy++;
                SetWaitCursor(true);
                return (ready);
            }
        }
        else
            ready = false;
        app_busy++;
    }
    else {
        if (app_busy == 1) {
            app_busy--;
            SetWaitCursor(false);
            if (pkg_event_monitor.has_saved_events()) {
                // dispatch saved events
                RegisterIdleProc(QTeventMonitor::saved_events_idle,
                    &pkg_event_monitor);
            }
            return (ready);
        }
        if (app_busy)
            app_busy--;
    }
    return (ready);
}


void
QTpkg::SetOverrideBusy(bool ovr)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;
    if (ovr) {
        if (app_busy)
            SetWaitCursor(false);
        app_override_busy = true;
    }
    else {
        if (app_busy)
            SetWaitCursor(true);
        app_override_busy = false;
    }
}


// Print into buf an identifier string for the main window, the
// X window number in this case.
//
bool
QTpkg::GetMainWinIdentifier(char *buf)
{
    *buf = 0;
//    if (!MainDev() || MainDev()->ident != _devQT_)
//        return (false);
    return (false);
}


bool
QTpkg::UsingX11()
{
#ifdef Q_OS_X11
    return (true);
#else
    return (false);
#endif
}


void
QTpkg::CloseGraphicsConnection()
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;
    if (QTdev::ConnectFd() > 0) {
//        gtk_main_quit();
        close(QTdev::ConnectFd());
    }
}


// Write in buf the display string, making sure that it contains the
// host name.
//
const char*
QTpkg::GetDisplayString()
{
    return (0);
}


// Check if host which has data in hent has screen access to the local
// display in display_string.  Simply print a warning if access fails.
//
bool
QTpkg::CheckScreenAccess(hostent*, const char*, const char*)
{
    return (true);
}


int
QTpkg::RegisterIdleProc(int(*cb)(void*), void *arg)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return (0);
    return (QTdev::self()->AddIdleProc(cb, arg));
}


bool
QTpkg::RemoveIdleProc(int id)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return (false);
    QTdev::self()->RemoveIdleProc(id);
    return (true);
}


int
QTpkg::RegisterTimeoutProc(int ms, int(*proc)(void*), void *arg)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return (0);
    return (QTdev::self()->AddTimer(ms, proc, arg));
}


bool
QTpkg::RemoveTimeoutProc(int id)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return (false);
    QTdev::self()->RemoveTimer(id);
    return (true);
}


// int time;  milliseconds
//
int
QTpkg::StartTimer(int time, bool *set)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return (0);
    int id = 0;
    if (set)
        *set = false;
    if (time > 0)
        id = QTdev::self()->AddTimer(time, timer_cb, set);
    return (id);
}


// Set the application fonts.  This should work for either Pango
// or XFD font names.
//
void
QTpkg::SetFont(const char *fname, int fnum, FNT_FMT fmt)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return;

    // Ignore XFDs, but anything else should be ok.
    if (fmt == FNT_FMT_X)
        return;

    if (!xfd_t::is_xfd(fname))
        Fnt()->setName(fname, fnum);
}


// Return the application font names.
//
const char *
QTpkg::GetFont(int fnum)
{
    // Valid for NULL graphics also.
    return (Fnt()->getName(fnum));
}


// Return the format code for the description string returned by GetFont.
//
FNT_FMT
QTpkg::GetFontFmt()
{
    return (FNT_FMT_Q);
}


void
QTpkg::PopUpBusy()
{
    const char *busy_msg =
        "Working...\nPress Control-C in main window to abort.";

    if (!pkg_busy_popup && QTmainwin::exists()) {
        pkg_busy_popup =
            QTmainwin::self()->PopUpErrText(busy_msg, STY_NORM);
        if (pkg_busy_popup)
            pkg_busy_popup->register_usrptr((void**)&pkg_busy_popup);
        RegisterTimeoutProc(3000, busy_msg_timeout, 0);
    }
}


void
QTpkg::SetWaitCursor(bool waiting)
{
    if (waiting)
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    else
        QApplication::restoreOverrideCursor();
}


// Static function
// Timeout procedure for message pop-up.
//
int
QTpkg::busy_msg_timeout(void*)
{
    if (QTpkg::self()->pkg_busy_popup)
        QTpkg::self()->pkg_busy_popup->popdown();
    return (false);
}
// End of QTpkg functions


//-----------------------------------------------------------------------------
// cMain functions

// Export the file/cell selected in the Files Selection, Cells Listing,
// Files Listing, Libraries Listing, or Tree pop-ups.
//
char *
cMain::GetCurFileSelection()
{
    if (!QTdev::exists())
        return (0);
    static char *tbuf;
    delete [] tbuf;
    tbuf = QTfileDlg::any_selection();
    if (tbuf)
        return (tbuf);

    tbuf = QTmainwin::get_cell_selection();
    if (tbuf)
        return (tbuf);

    tbuf = QTmainwin::get_file_selection();
    if (tbuf)
        return (tbuf);

    tbuf = QTmainwin::get_lib_selection();
    if (tbuf)
        return (tbuf);

    tbuf = QTmainwin::get_tree_selection();
    if (tbuf)
        return (tbuf);

    // Look for selected text in the Info window.
    QTtextDlg *tx =  dynamic_cast<QTtextDlg*>(
        QTmainwin::self()->ActiveInfo2());
    if (tx) {
        tbuf = tx->editor()->get_selection();
        if (tbuf && CDcdb()->findSymbol(tbuf))
            return (tbuf);
        delete [] tbuf;
    }
    return (0);
}


// Called when crashing, disable any updates
//
void
cMain::DisableDialogs()
{
    QTmainwin::cells_panic();
    QTmainwin::files_panic();
    QTmainwin::libs_panic();
    QTmainwin::tree_panic();
}


void
cMain::SetNoToTop(bool)
{
}


void
cMain::SetLowerWinOffset(int)
{
}
// End if cMain functions.


//-----------------------------------------------------------------------------
// QTeventMonitor functions

namespace {
    bool handle_event(const QObject *obj)
    {
        if (!obj)
            return (false);
        const QObject *w = QTmenu::self()->GetModal();
        if (!w)
            return (false);
        // Here, obj can be two things.
        // 1.  An object with no parent that has a name derived from
        // the source top-level widget name, either
        // "QTmainwinClassWindow" or similar for the modal window". 
        // Accept the event if it is not from the main window.
        // 2.  An object whose parent is a pointer to the modal
        // widget.  Accept these.

        if (!obj->parent()) {
            QString str = obj->objectName();
            if (str.isNull() || str == "QTmainwinClassWindow")
                return (false);
            return (true);
        }
        return (obj->parent() == w);
    }
}


bool
QTeventMonitor::eventFilter(QObject *obj, QEvent *ev)
{
    // Handle events here, return true to indicate handled.
//XXX
    if (ev->type() == QEvent::MouseMove) { 
//    printf("%p %x\n", qApp->widgetAt(QCursor::pos()), ev->type());
    }

    if (em_event_handler)
        return (em_event_handler(obj, ev, 0));

    // When the application is busy, all button presses and all key
    // presses except for Ctrl-C are locked out, and upon receipt a
    // pop-up appears telling the user to use Ctrl-C to abort.  The
    // pop-up disappears in a few seconds.
    //
    // All other events are dispatched normally when busy.
    //
    if (ev->type() == QEvent::KeyPress) {
        log_event(obj, ev);
        QKeyEvent *kev = static_cast<QKeyEvent*>(ev);
        if (kev->key() == Qt::Key_C
                && (kev->modifiers() & Qt::ControlModifier)) {
            cMain::InterruptHandler();
            return (true);
        }
        if (QTpkg::self()->IsBusy()) {
            if (!is_modifier_key(kev->key()))
                QTpkg::self()->PopUpBusy();
            return (true);
        }
        if (QTmenu::self()->IsGlobalInsensitive()) {
            if (handle_event(obj))
                return (QObject::eventFilter(obj, ev));
            return (true);
        }
    }
    else if (ev->type() == QEvent::KeyRelease) {
        log_event(obj, ev);
        QKeyEvent *kev = static_cast<QKeyEvent*>(ev);
        if (kev->key() == Qt::Key_C 
                && (kev->modifiers() & Qt::ControlModifier)) {
            return (true);
        }
        // Always handle key-up events, otherwise if busy modifier key
        // state may not be correct after operation.
    }
    else if (ev->type() == QEvent::MouseButtonPress) {
        log_event(obj, ev);
        if (QTpkg::self()->IsBusy()) {
            if (is_busy_allow(obj)) {
                // if (g_object_get_data(obj, "abort"))
                return (QObject::eventFilter(obj, ev));
            }
            QMouseEvent *mev = static_cast<QMouseEvent*>(ev);
            if (mev->button() < 4)
                // Ignore mouse wheel events.
                QTpkg::self()->PopUpBusy();
            return (true);
        }
        if (QTmenu::self()->IsGlobalInsensitive()) {
            if (handle_event(obj))
                return (QObject::eventFilter(obj, ev));
        }
    }
    else if (ev->type() == QEvent::MouseButtonRelease) {
        log_event(obj, ev);
        QMouseEvent *mev = static_cast<QMouseEvent*>(ev);
        if (QTpkg::self()->IsBusy()) {
            if (em_button_state[mev->button()]) {
                // If the button state is active, the mouse button must
                // have been down when busy mode was entered.  Save the
                // event for dispatch when leaving busy mode.
                save_event(obj, ev);
                em_button_state[mev->button()] = false;
                return (true);
            }
            if (is_busy_allow(obj)) {
                // Allow these.
                // if (g_object_get_data(G_OBJECT(evw), "abort"))
                return (QObject::eventFilter(obj, ev));
            }
            return (true);
        }
        if (QTmenu::self()->IsGlobalInsensitive()) {
            if (handle_event(obj))
                return (QObject::eventFilter(obj, ev));
        }
    }
    else if (ev->type() == QEvent::MouseButtonDblClick) {
        if (QTpkg::self()->IsBusy())
            return (true);
        if (QTmenu::self()->IsGlobalInsensitive()) {
            if (handle_event(obj))
                return (QObject::eventFilter(obj, ev));
        }
    }
    return (QObject::eventFilter(obj, ev));
}


//  Event history logging

namespace {
    // Print the first few lines of the run log file.
    //
    void
    log_init(FILE *fp)
    {
        fprintf(fp, "# %s\n", XM()->IdString());

        char buf[256];
        if (FIO()->PGetCWD(buf, 256))
            fprintf(fp, "Cwd(\"%s\")\n", buf);

        fprintf(fp, "Techfile(\"%s\")\n",
            Tech()->TechFilename() ? Tech()->TechFilename() : "<none>");
        fprintf(fp, "Edit(\"%s\", 0)\n",
            DSP()->CurCellName() ?
                Tstring(DSP()->CurCellName()) : "<none>");
        fprintf(fp, "Mode(%d)\n", DSP()->CurMode());
        if (DSP()->CurMode() == Physical) {
            fprintf(fp, "Window(%g, %g, %g, 0)\n",
                (double)(DSP()->MainWdesc()->Window()->left +
                    DSP()->MainWdesc()->Window()->right)/(2*CDphysResolution),
                (double)(DSP()->MainWdesc()->Window()->bottom +
                    DSP()->MainWdesc()->Window()->top)/(2*CDphysResolution),
                (double)DSP()->MainWdesc()->Window()->width()/
                    CDphysResolution);
        }
        else {
            fprintf(fp, "Window(%g, %g, %g, 0)\n",
                (double)(DSP()->MainWdesc()->Window()->left +
                    DSP()->MainWdesc()->Window()->right)/(2*CDelecResolution),
                (double)(DSP()->MainWdesc()->Window()->bottom +
                    DSP()->MainWdesc()->Window()->top)/(2*CDelecResolution),
                (double)DSP()->MainWdesc()->Window()->width()/
                    CDelecResolution);
        }
    }
}


// Static function.
// Print a line in the run log file describing the event.  The print format
// is readable by the function parser.
//
void
QTeventMonitor::log_event(const QObject *obj, const QEvent *ev)
{
    static int ccnt;
    const char *msg = "Can't open %s file, history won't be logged.";
    if (ccnt < 0)
        return;

    if (ev->type() == QEvent::KeyPress || ev->type() == QEvent::KeyRelease) {
        FILE *fp = Log()->OpenLog(XM()->LogName(), ccnt ? "a" : "w", true);
        if (fp) {
            if (!ccnt)
                log_init(fp);
            const QKeyEvent *kev = static_cast<const QKeyEvent*>(ev);
            QByteArray ba = kev->text().toLatin1();
            QByteArray ba2 = QKeySequence(kev->key()).toString().toLatin1();
            if (ba.constData() && *ba.constData())
                fprintf(fp, "# %s\n", ba.constData());
            else if (ba2.constData() && *ba2.constData())
                fprintf(fp, "# %s\n", ba2.constData());
            else
                fprintf(fp, "# %s\n", "");

            char *wname = qt_keyb::object_path(obj);
            if (!wname)
                wname = lstring::copy("unknown");
            fprintf(fp, "%s(%d, %x, \"%s\")\n",
                ev->type() == QEvent::KeyPress ? "KeyDown" : "KeyUp",
                kev->key(), (unsigned int)kev->modifiers(), wname);
            delete [] wname;
            fclose(fp);
        }
        else if (!ccnt) {
            Log()->WarningLogV(mh::Initialization, msg, XM()->LogName());
            ccnt = -1;
            return;
        }
        ccnt++;
    }
    else if (ev->type() == QEvent::MouseButtonPress ||
            ev->type() == QEvent::MouseButtonRelease) {
        FILE *fp = Log()->OpenLog(XM()->LogName(), ccnt ? "a" : "w", true);
        if (fp) {
            if (!ccnt)
                log_init(fp);
            const QMouseEvent *mev = static_cast<const QMouseEvent*>(ev);
            char *wname = qt_keyb::object_path(obj);
            if (!wname)
                wname = lstring::copy("unknown");
            fprintf(fp, "%s(%d, %d, %d, %d, \"%s\")\n",
                ev->type() == QEvent::MouseButtonPress ? "BtnDown" : "BtnUp",
                mev->button(), (unsigned int)mev->modifiers(),
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
                (int)mev->position().x(), (int)mev->position().y(), wname);
#else
                (int)mev->x(), (int)mev->y(), wname);
#endif
            delete [] wname;
            fclose(fp);
        }
        else if (!ccnt) {
            Log()->WarningLogV(mh::Initialization, msg, XM()->LogName());
            ccnt = -1;
            return;
        }
        ccnt++;
    }
}
// End of QTeventMonitor functions.


//-----------------------------------------------------------------------------
// cKeys functions

cKeys::cKeys(int wnum, QWidget *prnt) : QTcanvas(prnt)
{
    k_keypos = 0;
    k_win_number = wnum;
    memset(k_keys, 0, CBUFMAX+1);
    k_cmd = 0;

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_SCREEN))
        set_font(fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed(int)), Qt::QueuedConnection);
}


QSize
cKeys::sizeHint() const
{
    int h = QTfont::lineHeight(FNT_SCREEN) + 4;
    int w = QTfont::stringWidth(0, FNT_SCREEN);
    int n = k_cmd ? strlen(k_cmd) : strlen(k_keys);
    if (n > 5)
        n = 5;
    return (QSize(n*w + 4, h));
}


void
cKeys::show_keys()
{
    if (k_cmd) {
        delete [] k_cmd;
        k_cmd = 0;
    }
    bool rsz = (size() != sizeHint());
    if (rsz)
        resize(sizeHint().width(), sizeHint().height());
    const char *s = k_cmd;
    if (!s)
        s = k_keys + (k_keypos > 5 ? k_keypos - 5 : 0);
    clear();
    int yy = QTfont::lineHeight(FNT_SCREEN);
    draw_text(2, yy, s, -1);
    if (rsz)
        updateGeometry();
}


void
cKeys::set_keys(const char *text)
{
    while (k_keypos)
        k_keys[--k_keypos] = '\0';
    if (text) {
        const char *t = text;
        char *s = k_keys;
        bool ended = false;
        for (int i = 0; i < CBUFMAX; i++) {
            if (!ended && (!*t || i == CBUFMAX-1))
                ended = true;
            *s++ = ended ? 0 : *t++;
        }
        k_keypos = strlen(k_keys);
    }
}


void
cKeys::bsp_keys()
{
    if (k_keypos)
        k_keys[--k_keypos] = '\0';
}


namespace {
    // Idle proc to exec when command prefix is entered.
    //
    int check_exec_idle(void *arg)
    {
        MenuEnt *ent = (MenuEnt*)arg;
        if (ent->is_menu()) {
            // Opening a menu is pointless, but intercept these commands
            // and perhaps do something sensible.

            if (!strcmp(ent->entry, MenuOPEN)) {
                // Open a new file/cell, as for the "new" submenu button.
                XM()->HandleOpenCellMenu("new", false);
            }
            else if (!strcmp(ent->entry, MenuVIEW)) {
                // Centered full window display.
                if (ent->cmd.wdesc)
                    ent->cmd.wdesc->SetView("full");
                else
                    DSP()->MainWdesc()->SetView("full");
            }
            else if (!strcmp(ent->entry, MenuMAINW)) {
                // What to do here?  Ignore for now.
            }
            return (0);
        }

        // Do the command.
        MainMenu()->CallCallback(ent->cmd.caller);
        return (0);
    }
}


void
cKeys::check_exec(bool exact)
{
    if (!k_keypos)
        return;

    MenuEnt *ent =
        MainMenu()->MatchEntry(k_keys, k_keypos, k_win_number, exact);
    if (!ent || !ent->cmd.caller)
        return;

    if (ent->is_dynamic() && ent->is_menu())
        // Ignore the submenu buttons in the User menu
        return;

    char buf[CBUFMAX + 1];
    strncpy(buf, ent->entry, CBUFMAX);
    buf[CBUFMAX] = 0;
    int n = strlen(buf) - 5;
    if (n < 0)
        n = 0;
    k_cmd = lstring::copy(buf + n);

    resize(sizeHint().width(), sizeHint().height());
    clear();
    int yy = QTfont::lineHeight(FNT_SCREEN);
    draw_text(2, yy, k_cmd, -1);
    set_keys(0);

    // Put the execution in an idle proc.  This avoids an artifact in
    // prompt text from newly-generated text coming before the geometry
    // update is finished.
    QTpkg::self()->RegisterIdleProc(check_exec_idle, ent);
}


void
cKeys::font_changed(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_SCREEN))
            set_font(fnt);
        show_keys();
    }
}


//-----------------------------------------------------------------------------
// QTsubwin functions

namespace {
    int btnmap(int btn)
    {
        int button = 0;
        if (btn == Qt::LeftButton)
            button = 1;
        else if (btn == Qt::MiddleButton)
            button = 2;
        else if (btn == Qt::RightButton)
            button = 3;
        return (button);
    }

    // Share common ghost drawing parameters between similar drawing
    // windows.
    cGhostDrawCommon GhostDrawCommon;

    // Keep track of previous locations.
    QRect LastPos[DSP_NUMWINS];

    // Struct to hold state for button press grab action.
    //
    struct grabstate_t
    {
        grabstate_t()
        {
            for (int i = 0; i < GS_NBTNS; i++)
                gs_caller[i] = 0;
            gs_noopbtn = false;
        }

        void set(QWidget *w, QMouseEvent *ev)
        {
            int b = btnmap(ev->button());
            if (b)
                gs_caller[b] = w;
        }

        bool is_armed(QMouseEvent *ev)
        {
            int b = btnmap(ev->button());
            return (gs_caller[b] != 0);
        }

        void clear(QMouseEvent *ev)
        {
            int b = btnmap(ev->button());
            gs_caller[b] = 0;
        }

        void clear(unsigned int btn)
        {
            int b = btnmap(btn);
            gs_caller[b] = 0;
        }

        bool check_noop(bool n)
        {
            bool x = gs_noopbtn;
            gs_noopbtn = n;
            return (x);
        }

        QWidget *widget(unsigned int btn)
        {
            int b = btnmap(btn);
            return (gs_caller[b]);
        }

    private:
        QWidget *gs_caller[GS_NBTNS];     // target windows
        bool gs_noopbtn;                  // true when simulating no-op button
    };

    grabstate_t grabstate;
}


QTsubwin::QTsubwin(int wnum, QWidget *prnt) : QDialog(prnt), QTbag(this),
    QTdraw(XW_DRAWING)
{
    sw_toolbar = 0;
    sw_pixmap = 0;
    sw_keys_pressed = 0;
    sw_expand = 0;
    sw_zoom = 0;
    sw_gridpop = 0;
    sw_windesc = 0;
    sw_win_number = wnum < 0 ? -1 : wnum;
    sw_cursor_type = 0;

    setAttribute(Qt::WA_DeleteOnClose);

    QIcon icon;
    icon.addPixmap(QPixmap(xic_16x16_xpm));
    icon.addPixmap(QPixmap(xic_32x32_xpm));
    icon.addPixmap(QPixmap(xic_48x48_xpm));
    setWindowIcon(icon);

    gd_viewport = new QTcanvas();
    gd_viewport->set_ghost_common(&GhostDrawCommon);
    gd_viewport->setFocusPolicy(Qt::StrongFocus);
    gd_viewport->setAcceptDrops(true);
    if (sw_win_number >= 0)
        GhostDrawCommon.gd_windows[sw_win_number] = gd_viewport;

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_SCREEN))
        gd_viewport->set_font(fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed(int)), Qt::QueuedConnection);

    connect(gd_viewport, SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(gd_viewport, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(button_down_slot(QMouseEvent*)));
    connect(gd_viewport, SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(button_up_slot(QMouseEvent*)));
    connect(gd_viewport, SIGNAL(double_click_event(QMouseEvent*)),
        this, SLOT(double_click_slot(QMouseEvent*)));
    connect(gd_viewport, SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(motion_slot(QMouseEvent*)));
    connect(gd_viewport, SIGNAL(key_press_event(QKeyEvent*)),
        this, SLOT(key_down_slot(QKeyEvent*)));
    connect(gd_viewport, SIGNAL(key_release_event(QKeyEvent*)),
        this, SLOT(key_up_slot(QKeyEvent*)));
    connect(gd_viewport, SIGNAL(enter_event(QEnterEvent*)),
        this, SLOT(enter_slot(QEnterEvent*)));
    connect(gd_viewport, SIGNAL(leave_event(QEvent*)),
        this, SLOT(leave_slot(QEvent*)));
    connect(gd_viewport, SIGNAL(focus_in_event(QFocusEvent*)),
        this, SLOT(focus_in_slot(QFocusEvent*)));
    connect(gd_viewport, SIGNAL(focus_out_event(QFocusEvent*)),
        this, SLOT(focus_out_slot(QFocusEvent*)));
    connect(gd_viewport, SIGNAL(mouse_wheel_event(QWheelEvent*)),
        this, SLOT(mouse_wheel_slot(QWheelEvent*)));
    connect(gd_viewport, SIGNAL(drag_enter_event(QDragEnterEvent*)),
        this, SLOT(drag_enter_slot(QDragEnterEvent*)));
    connect(gd_viewport, SIGNAL(drop_event(QDropEvent*)),
        this, SLOT(drop_slot(QDropEvent*)));

    if (sw_win_number == 0)
        // being subclassed for main window
        return;

    sw_keys_pressed = new cKeys(sw_win_number, this);
    sw_keys_pressed->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sw_keys_pressed->show_keys();

    QMargins qmtop(2, 2, 2, 2);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    sw_toolbar = new QToolBar();

    QHBoxLayout *hbox = new QHBoxLayout(0);
    vbox->addLayout(hbox);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(2);

    hbox->addWidget(sw_toolbar);
    hbox->addWidget(sw_keys_pressed);

    vbox->addWidget(gd_viewport);
    set_transient_for(QTmainwin::self());

// Start of old init function.
    char buf[32];
    snprintf(buf, sizeof(buf), "%s %d", XM()->Product(), wnum);
    setWindowTitle(buf);

    sw_windesc = DSP()->Window(wnum);
    DSP()->Window(wnum)->SetWbag(this);
    DSP()->Window(wnum)->SetWdraw(this);

    // Create new menu, just copy template.
    MainMenu()->CreateSubwinMenu(wnum);

    connect(this, SIGNAL(update_coords(int, int)),
        QTmainwin::self(), SLOT(update_coords_slot(int, int)));

    QPoint mposn = QTmainwin::self()->pos();
    if (LastPos[wnum].width()) {
        move(LastPos[wnum].x() + mposn.x(), LastPos[wnum].y() + mposn.y());
    }
    else {
        QSize msz = QTmainwin::self()->size();
        move(mposn.x() + msz.width() - sizeHint().width(), mposn.y() + wnum*40 + 60);
    }

    SetWindowBackground(0);
    Clear();

    // Application initialization callback.
    //
    // The viewport size is wrong until show is called, however calling
    // show generates a premature resize event that causes trouble. 
    // Below is a hack to get around this by estimating the size from
    // the window size, which is correct here.

    QSize qs = sizeHint();
    DSP()->Initialize(qs.width()-4, qs.height()-30, wnum);
    show();
    raise();
    activateWindow();
    XM()->UpdateCursor(sw_windesc, (CursorType)cursor_type(), true);
}


QTsubwin::~QTsubwin()
{
    PopUpExpand(0, MODE_OFF, 0, 0, 0, false);
    PopUpGrid(0, MODE_OFF);
    PopUpZoom(0, MODE_OFF);
    if (sw_win_number >= 0)
        GhostDrawCommon.gd_windows[sw_win_number] = 0;
    if (sw_win_number > 0) {
        WindowDesc *wdesc = DSP()->Window(sw_win_number);
        if (!wdesc)
            return;
        wdesc->SetWbag(0);
        wdesc->SetWdraw(0);
        delete wdesc;

        MainMenu()->DestroySubwinMenu(sw_win_number);

        QSize sz = size();
        QPoint posn = pos();
        QPoint mposn = QTmainwin::self()->pos();

        // Save relative to corner of main window.  Absolute coords can
        // change if the main window is moved to a different monitor in
        // multi-monitor setups.

        LastPos[sw_win_number].setX(posn.x() - mposn.x());
        LastPos[sw_win_number].setY(posn.y() - mposn.y());
        LastPos[sw_win_number].setWidth(sz.width());
        LastPos[sw_win_number].setHeight(sz.height());
    }
}


// cAppWinFuncs interface

// The following two functions implement pixmap buffering for screen
// redisplays.  This often leads to faster redraws.  This function
// will create a pixmap and make it the current context drawable.
//
// In QT, we always draw to a pixmap, there is no asynchronous direct
// drawing option.
//
void
QTsubwin::SwitchToPixmap()
{
    // Switch to second pixmap.  Using a second pixmap gets around
    // rendering problems with the cell bounding box indicator.
    gd_viewport->switch_to_pixmap2();
}


// Copy out the BB area of the pixmap to the screen, and switch the
// context drawable back to the screen.
//
void
QTsubwin::SwitchFromPixmap(const BBox *BB)
{
    // Copy the area in a paint event, but the pixmap is retained.
    gd_viewport->switch_from_pixmap2(BB->left, BB->top, BB->left, BB->top,
        BB->right - BB->left + 1, BB->bottom - BB->top + 1);
}


// Return the pixmap, set by a previous call to SwitchToPixmap(), and
// reset the context to normal.
//
GRobject
QTsubwin::DrawableReset()
{
    // Nothing to do here.
    return (0);
}


// Copy out the BB area of the pixmap to the screen.
//
void
QTsubwin::CopyPixmap(const BBox *BB)
{
    gd_viewport->refresh(BB->left, BB->top, BB->right - BB->left + 1,
        BB->bottom - BB->top + 1);
}


// Destroy the pixmap (switching out first if necessary).
//
void
QTsubwin::DestroyPixmap()
{
    // Nothing to do here.
}


// Return true if the pixmap is alive and well.
//
bool
QTsubwin::PixmapOk()
{
    // Assume that our pixmap, managed by the drawing area widget is ok.
    return (true);
}


// Function to dump a window to a file, not part of the hardcopy system.
//
bool
QTsubwin::DumpWindow(const char *filename, const BBox *AOI = 0)
{
    // Note that the bounding values are included in the display.
    if (!sw_windesc)
        return (false);
    BBox BB = AOI ? *AOI : sw_windesc->Viewport();
    ViewportClip(BB, sw_windesc->Viewport());
    if (BB.right < BB.left || BB.bottom < BB.top)
        return (false);
    QRect rc(BB.left, BB.top, BB.right, BB.bottom);
    return (gd_viewport->pixmap()->copy(rc).save(filename));
}


// Get the "keyspressed" in buf
//
void
QTsubwin::GetTextBuf(char *buf)
{
    for (int i = 0; i < sw_keys_pressed->key_pos(); i++)
        buf[i] = sw_keys_pressed->key(i);
    buf[sw_keys_pressed->key_pos()] = 0;
}


// Set the "keyspressed".
//
void
QTsubwin::SetTextBuf(const char *buf)
{
    sw_keys_pressed->set_keys(buf);
    sw_keys_pressed->show_keys();
}


void
QTsubwin::ShowKeys()
{
    sw_keys_pressed->show_keys();
}


void
QTsubwin::SetKeys(const char *text)
{
    sw_keys_pressed->set_keys(text);
}


void
QTsubwin::BspKeys()
{
    sw_keys_pressed->bsp_keys();
}


bool
QTsubwin::AddKey(int c)
{
    int keypos = sw_keys_pressed->key_pos();
    if (c > ' ' && c <= '~' && keypos < CBUFMAX) {
        sw_keys_pressed->append(c);
        return (true);
    }
    if (c == ' ' && keypos > 0 && keypos < CBUFMAX) {
        sw_keys_pressed->append(c);
        return (true);
    }
    return (false);
}


bool
QTsubwin::CheckBsp()
{
    int keypos = sw_keys_pressed->key_pos();
    if (keypos > 0 &&
            sw_keys_pressed->key(keypos - 1) == Kmap()->SuppressChar()) {
        BspKeys();
        return (true);
    }
    return (false);
}


void
QTsubwin::CheckExec(bool exact)
{
    sw_keys_pressed->check_exec(exact);
}


char *
QTsubwin::KeyBuf()
{
    return (sw_keys_pressed->key_buf());
}


int
QTsubwin::KeyPos()
{
    return (sw_keys_pressed->key_pos());
}


void
QTsubwin::SetLabelText(const char*)
{
}


void
QTsubwin::PopUpExpand(GRobject caller, ShowMode mode,
    bool (*callback)(const char*, void*),
    void *arg, const char *string, bool nopeek)
{
    if (mode == MODE_OFF) {
        if (sw_expand)
            sw_expand->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (sw_expand)
            sw_expand->update(string);
        return;
    }
    if (sw_expand)
        return;

    sw_expand = new QTexpandDlg(this, string, nopeek, arg);
    sw_expand->register_caller(caller);
    sw_expand->register_callback(callback);
    sw_expand->set_visible(true);
}


void
QTsubwin::PopUpZoom(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (sw_zoom)
            sw_zoom->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (sw_zoom)
            sw_zoom->update();
        return;
    }
    if (sw_zoom)
        return;

    sw_zoom = new QTzoomDlg(this, sw_windesc);
    sw_zoom->register_usrptr((void**)&sw_zoom);
    sw_zoom->register_caller(caller);
    sw_zoom->initialize();
    sw_zoom->set_visible(true);
}

// End of cAppWinFuncs interface


// Prevent focus change with Tab/Shift-Tab, instead send the event on
// to the drawing window for Undo/Redo.
//
bool
QTsubwin::focusNextPrevChild(bool)
{
    return (false);
}


#define MODMASK (GR_SHIFT_MASK | GR_CONTROL_MASK | GR_ALT_MASK)


namespace {
    // Match the "code" below case-insensitively.
    //
    bool codes_match(int code1, int code2)
    {
        return ((islower(code1) ? code1 : tolower(code1)) ==
            (islower(code2) ? code2 : tolower(code2)));
    }
}


// Handle keypresses in the main and subwindows.  If certain modes
// are set, call the application.  Otherwise, save the keypress in
// a buffer, which is displayed in the keypress readout area.  If
// this text uniquely matches a command, send a button press to
// the appropriate command button, and clear the buffer.
//
bool
QTsubwin::keypress_handler(unsigned int keyval, unsigned int state,
    const char *keystring, bool from_prline, bool up)
{
    if (!sw_windesc)
        return (false);

    // QT returns a Backtab code for Shift-Tab, map that back to Tab
    // here so that Shift-Tab will perform the Redo operation.
    if (keyval == Qt::Key_Backtab && (state & GR_SHIFT_MASK))
        keyval = Qt::Key_Tab;

    // The code: 0x00 - 0x16 are the KEYcode enum values.
    //           0x17 - 0x1f unused
    //           0x20 - 0xffff X keysym
    // The X keysyms 0x20 - 0x7e match the ascii character.
    int code = 0;
    if (up) {
        for (keymap *k = Kmap()->KeymapUp(); k->keyval; k++) {
            if (k->keyval == keyval) {
                code = k->code;
                break;
            }
        }
        if (code == SHIFTUP_KEY || code == CTRLUP_KEY)
            EV()->KeypressCallback(sw_windesc, code, (char*)"", state);
        return (true);
    }

    // Tell the application if the mouse is currently over the prompt
    // area.  If so, we may want to handle some events differently.
    EV()->SetFromPromptLine(from_prline);

    // QT doesn't have separate keycodes for the numeric keypad keys,
    // instead there is a modifier.  Skip the numeric +/- recognition
    // unless the modifier is set.

    bool skipit = ((keyval == Qt::Key_Minus || keyval == Qt::Key_Plus) &&
        !(state & GR_KEYPAD_MASK));

    int subcode = 0;
    if (!skipit) {
        for (keymap *k = Kmap()->KeymapDown(); k->keyval; k++) {
            if (k->keyval == keyval) {
                code = k->code;
                subcode = k->subcode;
                break;
            }
        }
    }

    if (code == FUNC_KEY) {
        char tbuf[CBUFMAX + 1];
        memset(tbuf, 0, sizeof(tbuf));

        char *fstr = Tech()->GetFkey(subcode, 0);
        if (fstr) {
            if (*fstr == '!') {
                XM()->TextCmd(fstr + 1, false);
                delete [] fstr;
                return (true);
            }
            strncpy(tbuf, fstr, CBUFMAX);
            delete [] fstr;
        }

        if (EV()->KeypressCallback(sw_windesc, code, tbuf, state))
            return (true);
        SetKeys(tbuf);
        ShowKeys();
        CheckExec(true);
        return (true);
    }
    if (code == 0 && keyval >= 0x20)
        code = keyval;

    // In Apple, Command maps to Ctrl, Option to Alt.
    for (keyaction *k = Kmap()->ActionMapPre(); k->code; k++) {
        if (codes_match(k->code, code) &&
                (!k->state || k->state == (state & MODMASK))) {
            if (EV()->KeyActions(sw_windesc, k->action, &code))
                return (true);
            break;
        }
    }

    char tbuf[CBUFMAX + 1];
    memset(tbuf, 0, sizeof(tbuf));
    if (keystring)
        strncpy(tbuf, keystring, CBUFMAX);
    if (EV()->KeypressCallback(sw_windesc, code, tbuf, state))
        return (true);

    if (state & GR_ALT_MASK)
        return (false);

    for (keyaction *k = Kmap()->ActionMapPost(); k->code; k++) {
        if (codes_match(k->code, code) &&
                (!k->state || k->state == (state & MODMASK))) {
            if (EV()->KeyActions(sw_windesc, k->action, &code))
                return (true);
            break;
        }
    }
    if (code == RETURN_KEY) {
        if (sw_keys_pressed->key_pos() > 0) {
            CheckExec(true);
            return (true);
        }
        return (false);
    }
    if (!AddKey(*tbuf))
        return (false);
    ShowKeys();
    CheckExec(false);
    return (true);
}


// QTsubwin slots.

// Resize after the main or subwindow size change.  Call the application
// with the new size.
//
void
QTsubwin::resize_slot(QResizeEvent *ev)
{
    if (!sw_windesc)
        return;
    EV()->ResizeCallback(sw_windesc, ev->size().width(), ev->size().height());
    sw_windesc->Redisplay(0);
    ev->setAccepted(true);

    if (DSP()->NoPixmapStore() &&
            (!DSP()->CurCellName() || !XM()->IsAppReady())) {
        *sw_windesc->ClipRect() = sw_windesc->Viewport();
        sw_windesc->ShowGrid();
    }
    XM()->SetAppReady(true);
}


// Dispatch button press events to the application callbacks.
//
void
QTsubwin::button_down_slot(QMouseEvent *ev)
{
    if (!sw_windesc) {
        ev->ignore();
        return;
    }
    if (ev->type() != QEvent::MouseButtonPress &&
            ev->type() != QEvent::MouseButtonDblClick) {
        ev->ignore();
        return;
    }
    ev->accept();

    int button = btnmap(ev->button());
    button = Kmap()->ButtonMap(button);

    if (wb_message)
        wb_message->popdown();

    if (QTmenuConfig::self()->menu_disabled() && button == 1)
        // menu is insensitive, so ignore press
        return;

    bool showing_ghost = ShowingGhost();
    if (showing_ghost)
        ShowGhost(ERASE);

    if (!XM()->IsDoingHelp() || EV()->IsCallback())
        grabstate.set(gd_viewport, ev);

    int state = ev->modifiers();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif

    switch (button) {
    case 1:
        // basic point operation
        if ((state & Qt::ShiftModifier) &&
                (state & Qt::ControlModifier) &&
                (state & Qt::AltModifier)) {
            // Control-Shift-Alt-Button1 simulates a "no-op" button press.
            grabstate.check_noop(true);
            if (XM()->IsDoingHelp())
                PopUpHelp("noopbutton");
            else if (sw_windesc)
                EV()->ButtonNopCallback(sw_windesc, xx, yy, mod_state(state));
        }
        else
            EV()->Button1Callback(sw_windesc, xx, yy, mod_state(state));
        break;
    case 2:
        // Pan operation
        if (XM()->IsDoingHelp() && !(state & Qt::ShiftModifier))
            PopUpHelp("button2");
        else
            EV()->Button2Callback(sw_windesc, xx, yy, mod_state(state));
        break;
    case 3:
        // Zoom opertion
        if (XM()->IsDoingHelp() && !(state & Qt::ShiftModifier))
            PopUpHelp("button3");
        else
            EV()->Button3Callback(sw_windesc, xx, yy, mod_state(state));
        break;
    default:
        // No-op, update coordinates
        if (XM()->IsDoingHelp() && !(state & Qt::ShiftModifier))
            PopUpHelp("button4");
        else
            EV()->ButtonNopCallback(sw_windesc, xx, yy, mod_state(state));
        break;
    }
    if (showing_ghost)
        ShowGhost(DISPLAY);
}


// Dispatch button release events to the application callbacks.
//
void
QTsubwin::button_up_slot(QMouseEvent *ev)
{
    if (!sw_windesc || ev->type() != QEvent::MouseButtonRelease) {
        ev->ignore();
        return;
    }
    ev->accept();

    int button = btnmap(ev->button());
    button = Kmap()->ButtonMap(button);

    if (QTmenuConfig::self()->menu_disabled() && button == 1)
        // menu is insensitive, so ignore release
        return;

    if (!grabstate.is_armed(ev)) {
        grabstate.check_noop(false);
        return;
    }
    grabstate.clear(ev);

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif

    bool showing_ghost = ShowingGhost();
    if (showing_ghost)
        ShowGhost(ERASE);

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QPoint gpos = ev->globalPosition().toPoint();
#else
    QPoint gpos = ev->globalPos();
#endif

    // Get the QTcanvas (drawing window) if underneath the pointer.
    QTcanvas *cvs = dynamic_cast<QTcanvas*>(qApp->widgetAt(gpos));

    int state = ev->modifiers();
    WindowDesc *wdesc = sw_windesc;
    if (cvs) {
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        WindowDesc *wd;
        while ((wd = wgen.next()) != 0) {
            if (wd == sw_windesc)
                continue;
            if (!wd->IsSimilar(sw_windesc))
                continue;
            QTsubwin *w = dynamic_cast<QTsubwin*>(wd->Wbag());
            if (w->Viewport() == cvs) {
                wdesc = wd;
                QPoint gg = w->Viewport()->mapFromGlobal(gpos);
                xx = gg.x();
                yy = gg.y();
                break;
            }
        }
    }
    else
        wdesc = 0;

    switch (button) {
    case 1:
        if (grabstate.check_noop(false))
            EV()->ButtonNopReleaseCallback(wdesc, xx, yy, mod_state(state));
        else
            EV()->Button1ReleaseCallback(wdesc, xx, yy, mod_state(state));
        break;
    case 2:
        EV()->Button2ReleaseCallback(wdesc, xx, yy, mod_state(state));
        break;
    case 3:
        EV()->Button3ReleaseCallback(wdesc, xx, yy, mod_state(state));
        break;
    default:
        EV()->ButtonNopReleaseCallback(wdesc, xx, yy, mod_state(state));
        break;
    }
    if (showing_ghost)
        ShowGhost(DISPLAY);
}


void
QTsubwin::double_click_slot(QMouseEvent *ev)
{
    // Treat this as just another press, used to terminate poly/wire
    // vertex input.
    button_down_slot(ev);
}


// Handle pointer motion in the main and subwindows.  In certain
// commands, "ghost" drawing is utilized.
//
void
QTsubwin::motion_slot(QMouseEvent *ev)
{
    if (!sw_windesc || ev->type() != QEvent::MouseMove) {
        ev->ignore();
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    bool bdown = ev->buttons() &
        (Qt::LeftButton|Qt::MiddleButton|Qt::RightButton);
    if (bdown) {
        // If a button is pressed, move events are grabbed by the
        // "down" window, so we have to do our own dispatching to
        // get ghost visibility between drawing windows.

        // Look through other drawing windows, if the mouse pointer
        // is over any of the same type, send the event to the window.
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        QPoint gpos = ev->globalPosition().toPoint();
#else
        QPoint gpos = ev->globalPos();
#endif

        // Get the QTcanvas (drawing window) if underneath the pointer.
        // If there isn't one, we're done.
        // This takes care of Z ordering, the return is on top.
        QTcanvas *cvs = dynamic_cast<QTcanvas*>(qApp->widgetAt(gpos));
        if (!cvs) {
            UndrawGhost();
            ev->ignore();
            return;
        }

        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        WindowDesc *wd;
        while ((wd = wgen.next()) != 0) {
            if (wd == sw_windesc)
                continue;
            if (!wd->IsSimilar(sw_windesc))
                continue;
            QTsubwin *w = dynamic_cast<QTsubwin*>(wd->Wbag());
            if (w->Viewport() == cvs) {
                // Here's the target widget, dispatch to it.
                UndrawGhost(true);
                QPoint gg = w->Viewport()->mapFromGlobal(gpos);
                EV()->MotionCallback(wd, mod_state(ev->modifiers()));
                if (Gst()->ShowingGhostInWindow(wd)) {
                    w->UndrawGhost();
                    w->DrawGhost(gg.x(), gg.y());
                }
                wd->PToL(gg.x(), gg.y(), xx, yy);
                emit update_coords(xx, yy);
                ev->accept();
                return;
            }
        }
    }

    // Haven't changed windows.
    QRect r(QPoint(0, 0), gd_viewport->size());
    if (!r.contains(xx, yy)) {
        UndrawGhost(true);
        ev->ignore();
        return;
    }

    EV()->MotionCallback(sw_windesc, mod_state(ev->modifiers()));
    if (Gst()->ShowingGhostInWindow(sw_windesc)) {
        UndrawGhost();
        DrawGhost(xx, yy);
    }
    sw_windesc->PToL(xx, yy, xx, yy);
    emit update_coords(xx, yy);
}


// Key press processing for the drawing windows
//
void
QTsubwin::key_down_slot(QKeyEvent *ev)
{
    if (!QTmainwin::self() || QTpkg::self()->NotMapped()) {
        ev->ignore();
        return;
    }
    if (ev->type() != QEvent::KeyPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    if (wb_message)
        wb_message->popdown();

//printf("%x %x %x %x %x\n", ev->key(), ev->nativeScanCode(),
//ev->nativeVirtualKey(), ev->modifiers(), mod_state(ev->modifiers()));

    QString qs = ev->text().toLatin1();
    const char *string = (const char*)qs.constData();

    if (!is_modifier_key(ev->key()) && CheckBsp())
        ;
    else if (KbMac()->MacroExpand(ev->key(), mod_state(ev->modifiers()),
            false)) {
        return;
    }

    keypress_handler(ev->key(), mod_state(ev->modifiers()), string,
        gd_viewport->hasFocus() &&
        QTmainwin::self()->PromptLine()->underMouse(), false);
}


// Key release processing for the drawing windows.
//
void
QTsubwin::key_up_slot(QKeyEvent *ev)
{
    if (!QTmainwin::self() || QTpkg::self()->NotMapped()) {
        ev->ignore();
        return;
    }
    if (ev->type() != QEvent::KeyRelease) {
        ev->ignore();
        return;
    }
    ev->accept();

    if (!QTmainwin::self() || QTpkg::self()->NotMapped())
        return;
    if (KbMac()->MacroExpand(ev->key(), mod_state(ev->modifiers()), true))
        return;

    const char *string = ev->text().toLatin1().constData();
    keypress_handler(ev->key(), mod_state(ev->modifiers()), string,
        false, true);
}


// Reinitialize "ghost" drawing when the pointer enters a window.
// If the pointer enters a window with an active grab, check if
// a button release has occurred.  If so, and the condition has
// not already been cleared, call the appropriate callback to
// clean up.  This is for button up events that get 'lost'.
//
void
QTsubwin::enter_slot(QEnterEvent *ev)
{
    if (!sw_windesc) {
        ev->ignore();
        return;
    }
    if (ev->type() != QEvent::Enter) {
        ev->ignore();
        return;
    }
    ev->accept();

    gd_viewport->setFocus();

    if (grabstate.widget(Qt::LeftButton) == gd_viewport) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        if (!(ev->buttons() & Qt::LeftButton)) {
#else
        if (!(QApplication::mouseButtons() & Qt::LeftButton)) {
#endif
            grabstate.clear(Qt::LeftButton);
            EV()->Button1ReleaseCallback(0, 0, 0, 0);
        }
    }
    if (grabstate.widget(Qt::MiddleButton) == gd_viewport) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        if (!(ev->buttons() & Qt::MiddleButton)) {
#else
        if (!(QApplication::mouseButtons() & Qt::MiddleButton)) {
#endif
            grabstate.clear(Qt::MiddleButton);
            EV()->Button2ReleaseCallback(0, 0, 0, 0);
        }
    }
    if (grabstate.widget(Qt::RightButton) == gd_viewport) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        if (!(ev->buttons() & Qt::RightButton)) {
#else
        if (!(QApplication::mouseButtons() & Qt::RightButton)) {
#endif
            grabstate.clear(Qt::RightButton);
            EV()->Button3ReleaseCallback(0, 0, 0, 0);
        }
    }

    EV()->MotionCallback(sw_windesc,
        mod_state(QGuiApplication::keyboardModifiers()));
}


// Gracefully terminate ghost drawing when the pointer leaves a
// window.
//
void
QTsubwin::leave_slot(QEvent *ev)
{
    if (!sw_windesc) {
        ev->ignore();
        return;
    }
    if (ev->type() != QEvent::Leave) {
        ev->ignore();
        return;
    }
    ev->accept();

    EV()->MotionCallback(sw_windesc,
        mod_state(QGuiApplication::keyboardModifiers()));
    UndrawGhost(true);
}


void
QTsubwin::focus_in_slot(QFocusEvent *ev)
{
    ev->accept();
    if (QTedit::self() && QTedit::self()->is_active())
        QTedit::self()->draw_cursor(DRAW);
}


void
QTsubwin::focus_out_slot(QFocusEvent *ev)
{
    ev->accept();
    if (QTedit::self() && QTedit::self()->is_active())
        QTedit::self()->draw_cursor(UNDRAW);
}


void
QTsubwin::mouse_wheel_slot(QWheelEvent *ev)
{
    QPoint numDegrees = ev->angleDelta()/8;
    if (numDegrees.isNull() || numDegrees.y() == 0) {
        ev->ignore();
        return;
    }
    bool scroll_up = (numDegrees.y() > 0);
    ev->accept();
    if (scroll_up) {

        if (ev->modifiers() & Qt::ControlModifier) {
            if (DSP()->MouseWheelZoomFactor() > 0.0)
                sw_windesc->Zoom(1.0 - DSP()->MouseWheelZoomFactor());
        }
        else if (ev->modifiers() & Qt::ShiftModifier) {
            if (DSP()->MouseWheelPanFactor() > 0.0)
                sw_windesc->Pan(DirEast, DSP()->MouseWheelPanFactor());
        }
        else {
            if (DSP()->MouseWheelPanFactor() > 0.0)
                sw_windesc->Pan(DirNorth, DSP()->MouseWheelPanFactor());
        }
    }
    else {
        if (ev->modifiers() & Qt::ControlModifier) {
            if (DSP()->MouseWheelZoomFactor() > 0.0)
                sw_windesc->Zoom(1.0 + DSP()->MouseWheelZoomFactor());
        }
        else if (ev->modifiers() & Qt::ShiftModifier) {
            if (DSP()->MouseWheelPanFactor() > 0.0)
                sw_windesc->Pan(DirWest, DSP()->MouseWheelPanFactor());
        }
        else {
            if (DSP()->MouseWheelPanFactor() > 0.0)
                sw_windesc->Pan(DirSouth, DSP()->MouseWheelPanFactor());
        }
    }
}


namespace {
    struct load_file_data
    {
        load_file_data(const char *f, const char *c)
            {
                filename = lstring::copy(f);
                cellname = lstring::copy(c);
            }

        ~load_file_data()
            {
                delete [] filename;
                delete [] cellname;
            }

        char *filename;
        char *cellname;
    };

    // Idle procedure to load a file, called from the drop handler.
    //
    int load_file_idle(void *arg)
    {
        load_file_data *data = (load_file_data*)arg;
        XM()->Load(EV()->CurrentWin(), data->filename, 0, data->cellname);
        delete data;
        return (0);
    }

    void load_file_proc(const char *fmt, const char *s)
    {
        char *src = lstring::copy(s);
        char *t = 0;
        if (!strcmp(fmt, "text/twostring")) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".
            t = strchr(src, '\n');
            if (t)
                *t++ = 0;
        }
        load_file_data *lfd = new load_file_data(src, t);
        delete [] src;

        bool didit = false;
        if (QTedit::self() && QTedit::self()->is_active()) {
            if (ScedIf()->doingPlot()) {
                // Plot/Iplot edit line is active, break out.
                QTedit::self()->abort();
            }
            else {
                // If editing, push into prompt line.
                // Keep the cellname.
                if (lfd->cellname)
                    QTedit::self()->insert(lfd->cellname);
                else
                    QTedit::self()->insert(lfd->filename);
                delete lfd;
                didit = true;
            }
        }
        if (!didit)
            QTpkg::self()->RegisterIdleProc(load_file_idle, lfd);
    }
}


void
QTsubwin::drag_enter_slot(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasFormat(QTltab::mime_type())) {
        // Layer also has text/plain, ignore it if not editing.
        if (!QTedit::self() || !QTedit::self()->is_active()) {
            ev->ignore();
            return;
        }
    }
    else if (ev->mimeData()->hasFormat("text/property")) {
        // Ignore if not editong.
        if (QTedit::self() && QTedit::self()->is_active())
            ev->accept();
        else
            ev->ignore();
        return;
    }
    // Accept urls or plain text (should be file or cell names)
    // anytime.  If not editing they will load a cell to edit
    // in the main window.
    if (ev->mimeData()->hasUrls() ||
            ev->mimeData()->hasFormat("text/twostring") ||
            ev->mimeData()->hasFormat("text/cellname") ||
            ev->mimeData()->hasFormat("text/string") ||
            ev->mimeData()->hasFormat("text/plain")) {
        ev->accept();
        return;
    }
    ev->ignore();
}


void
QTsubwin::drop_slot(QDropEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        QByteArray ba = ev->mimeData()->data("text/plain");
        const char *str = ba.constData() + strlen("File://");
        load_file_proc("", str);
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat(QTltab::mime_type())) {
        // Layer also has text/plain, ignore it if not editing.
        if (!QTedit::self() || !QTedit::self()->is_active()) {
            ev->ignore();
            return;
        }
    }
    else if (ev->mimeData()->hasFormat("text/property")) {
        if (QTedit::self() && QTedit::self()->is_active()) {
            QByteArray bary = ev->mimeData()->data("text/property");
            const char *val = (const char*)bary.data() + sizeof(int);
            CDs *cursd = CurCell(true);
            hyList *hp = new hyList(cursd, val, HYcvAscii);
            QTedit::self()->insert(hp);
            hyList::destroy(hp);
            QTedit::self()->set_focus();
            ev->accept();
        }
        else
            ev->ignore();
        return;
    }
    const char *fmt = 0;
    if (ev->mimeData()->hasFormat("text/twostring"))
        fmt = "text/twostring";
    else if (ev->mimeData()->hasFormat("text/cellname"))
        fmt = "text/cellname";
    else if (ev->mimeData()->hasFormat("text/string"))
        fmt = "text/string";
    else if (ev->mimeData()->hasFormat("text/plain"))
        fmt = "text/plain";
    if (fmt) {
        QByteArray bary = ev->mimeData()->data(fmt);
        load_file_proc(fmt, bary.constData());
        ev->accept();
        return;
    }
    ev->ignore();
}


void
QTsubwin::font_changed(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_SCREEN))
            gd_viewport->set_font(fnt);
    }
}


void
QTsubwin::help_slot()
{
    PopUpHelp("xic:vport");
}
// End of QRsubwin slots.
 

//-----------------------------------------------------------------------------
// QTmainwin functions

namespace {
    bool is_shift_down()
    {
        return (QApplication::keyboardModifiers() & Qt::ShiftModifier);
    }
}


QTmainwin::QTmainwin(QWidget *prnt) : QTsubwin(0, prnt)
{
    mw_menubar = new QMenuBar();
    mw_top_button_box = 0;
    mw_phys_button_box = 0;
    mw_elec_button_box = 0;
    mw_splitter = 0;

    mw_promptline = 0;
    mw_coords = 0;
    mw_layertab = 0;
    mw_status = 0;

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);
#ifndef __APPLE__
    QAction *a = mw_menubar->addAction(tr("wr"));
    a->setIcon(QIcon(QPixmap(wr_xpm)));
    connect(a, SIGNAL(triggered()), this, SLOT(wr_btn_slot()));
#endif

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
#ifdef __APPLE__
    if (getenv("XIC_NO_MAC_MENU"))
        vbox->setMenuBar(mw_menubar);
#else
    vbox->setMenuBar(mw_menubar);
#endif

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // Search button and entry, used with layer table.
    QToolButton *ltab_sbtn = new QToolButton();
    ltab_sbtn->setMaximumWidth(20);
    hbox->addWidget(ltab_sbtn);
    hbox->setStretch(0, 0);

    QLineEdit *ltab_entry = new QLineEdit();
    // Important:  Don't let this eat the Tab, otherwise can't Undo!
    ltab_entry->setFocusPolicy(Qt::ClickFocus);
    ltab_entry->setMinimumWidth(60);
    ltab_entry->setMaximumWidth(120);
    hbox->addWidget(ltab_entry);
    hbox->setStretch(1, 0);

    mw_top_button_box = new QWidget();
    hbox->addWidget(mw_top_button_box);
    hbox->setStretch(2, 0);

    mw_coords = new QTcoord();
    hbox->addWidget(mw_coords);
    hbox->setStretch(3, 1);

    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    mw_phys_button_box = new QWidget(this);
    hbox->addWidget(mw_phys_button_box);
    mw_elec_button_box = new QWidget(this);
    hbox->addWidget(mw_elec_button_box);

    mw_layertab = new QTltab(false);
    LT()->SetLtab(mw_layertab);

    mw_layertab->set_search_widgets(ltab_sbtn, ltab_entry);

    mw_splitter = new QSplitter();
    hbox->addWidget(mw_splitter);
    mw_splitter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    mw_splitter->addWidget(mw_layertab);
    mw_splitter->addWidget(gd_viewport);

    // This sets the handle in a sensible place, not easy!
    int wd = sizeHint().width();
    int w = QTfont::stringWidth(0, FNT_SCREEN);
    mw_splitter->setSizes(QList<int>() << 50 + 10*w << wd - 200);

    vbox->addLayout(hbox);

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    PL()->SetEdit(new QTedit(false));
    mw_promptline = QTedit::self()->Viewport();
    sw_keys_pressed = QTedit::self()->keys();
    hbox->addWidget(QTedit::self());

    QFrame *f = new QFrame();
    vbox->addWidget(f);
    f->setFrameShape(QFrame::HLine);

    mw_status = new QTparam(this);
    vbox->addWidget(mw_status);

    connect(this, SIGNAL(update_coords(int, int)),
        this, SLOT(update_coords_slot(int, int)));
}


QSize
QTmainwin::sizeHint() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QSize sz = screen()->availableSize();
#else
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
    QScreen *scr = QGuiApplication::screenAt(mapToGlobal(
        QPoint(width()/2, height()/2)));
    QSize sz = scr ? scr->availableSize() : QSize(1024, 768);;
#else
    QSize sz(1024, 768);
    QList<QScreen*> qsl = QGuiApplication::screens();
    for (int i = 0; i < qsl.size(); i++) {
        QScreen *sc = qsl.at(i);
        QRect r = sc->availableGeometry();
        if (r.contains(QPoint(width()/2, height()/2))) {
            sz = QSize(r.width(), r.height());
            break;
        }
    }
#endif
#endif
    // Max honored size is 2/3 the screen width and height.
    return (QSize((sz.width()*2)/3, (sz.height()*2)/3));
}


QSize
QTmainwin::minimumSizeHint() const
{
    return (QSize(600, 600));
}


//#ifdef QT_OS_X11
#if 0
namespace {
    // Load colors using the X resource mechanism.
    //
    void xrm_load_colors()
    {
        // load string resource color names
        static bool doneit;
        if (!doneit) {
            // obtain the database
            doneit = true;
            passwd *pw = getpwuid(getuid());
            if (pw) {
                char buf[512];
                snprintf(buf, 512, "%s/%s", pw->pw_dir, XM()->Product());
                if (access(buf, R_OK))
                    return;
                XrmDatabase rdb = XrmGetFileDatabase(buf);
                XrmSetDatabase(gr_x_display(), rdb);
            }
        }
        char name[64], clss[64];
        snprintf(name, 64, "%s.", XM()->Product());
        lstring::strtolower(name);
        char *tn = name + strlen(name);
        snprintf(clss, 64, "%s.", XM()->Product());
        char *tc = clss + strlen(clss);
        XrmDatabase db = XrmGetDatabase(gr_x_display());
        for (int i = 0; i < GRattrColorEnd; i++) {
            const char *keyword = DSP()->ColorTab()->gui_color_name(i);
            if (!keyword)
                break;
            strcpy(tn, keyword);
            strcpy(tc, keyword);
            if (isupper(tn[0]))
                tn[0] = tolower(tn[0]);
            if (islower(tc[0]))
                tc[0] = toupper(tc[0]);
            char *ss;
            XrmValue v;
            if (XrmGetResource(db, name, clss, &ss, &v)) {
                if (v.addr && *v.addr)
                    QTpkg::self()->SetAttrColor((GRattrColor)i, v.addr);
            }
        }
    }
}
#endif


void
QTmainwin::initialize()
{
    // Deferred initialization, move this to constructor?
    // Called in QTpkg::Initialize.

    DSP()->SetWindow(0, new WindowDesc);
    EV()->SetCurrentWin(DSP()->MainWdesc());
    sw_windesc = DSP()->MainWdesc();

    // We use w_draw for rendering, as this pointer may point to a
    // hard-copy context.  Rendering through w_wbag will always go
    // to the screen.
    //
    DSP()->MainWdesc()->SetWbag(this);
    DSP()->MainWdesc()->SetWdraw(this);

    // This is done again in AppInit(), but it is needed for the menus.
    DSP()->SetCurMode(XM()->InitialMode());

#ifdef QT_OS_X11
    xrm_load_colors();
#endif
    DSP()->ColorTab()->init();

    QTmenu::self()->InitMainMenu();
    QTmenu::self()->InitTopButtonMenu();
    QTmenu::self()->InitSideButtonMenus();

    gd_viewport->setFocus();
}


void
QTmainwin::send_key_event(sKeyEvent *kev)
{
    Qt::KeyboardModifiers mod = Qt::NoModifier;
    if (kev->state & GR_SHIFT_MASK)
        mod |= Qt::ShiftModifier;
    if (kev->state & GR_CONTROL_MASK)
        mod |= Qt::ControlModifier;
    if (kev->state & GR_ALT_MASK)
        mod |= Qt::AltModifier;

    activateWindow();
    if (kev->type == KEY_PRESS) {
        QKeyEvent ev(QEvent::KeyPress, kev->key, mod, QString(kev->text));
        QCoreApplication::sendEvent(gd_viewport, &ev);
    }
    if (kev->type == KEY_RELEASE) {
        QKeyEvent ev(QEvent::KeyRelease, kev->key, mod, QString(kev->text));
        QCoreApplication::sendEvent(gd_viewport, &ev);
    }
}


namespace {
    int close_idle(void*)
    {
        XM()->Exit(ExitCheckMod);
        return (0);
    }
}


void
QTmainwin::closeEvent(QCloseEvent *ev)
{
    // Actual program exit is done from Exit() in the idle proc.  The idle
    // proc allows this handler to return immediately.  If a second event
    // occurs before the first returns, QT exits the program, which is not
    // what we want.

    ev->ignore();
    if (QTpkg::self()->IsBusy()) {
        QTpkg::self()->PopUpBusy();
        return;
    }
    if (!QTmenu::self()->IsGlobalInsensitive())
        QTpkg::self()->RegisterIdleProc(close_idle, 0);
}


// Handler for the "WR" button.
//
void
QTmainwin::wr_btn_slot()
{
    if (XM()->IsDoingHelp() && !is_shift_down()) {
        PopUpHelp("xic:wrbtn");
        return;
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "%s-%s bug", XM()->Product(),
        XM()->VersionString());
    PopUpMail(buf, Log()->MailAddress());
}


void
QTmainwin::update_coords_slot(int xx, int yy)
{
    mw_coords->print(xx, yy, QTcoord::COOR_MOTION);
}
// End of QTmainwin functions.

