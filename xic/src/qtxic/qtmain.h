
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

#ifndef QTMAIN_H
#define QTMAIN_H

#include "main.h"
#include "qtinterf/qtinterf.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtcanvas.h"
#include "dsp_tkif.h"

#include <QDialog>
#include <QEvent>
#include <QKeyEvent>
#include <QFontMetrics>


//-----------------------------------------------------------------------------
// Main Window and top-level functions.

struct sKeyEvent;
class QTcoord;
class QTparam;
class cKeys;
class QTexpandDlg;
class QTzoomDlg;
class QTgridDlg;
class QTidleproc;
class QTltab;

class QMenu;
class QMenuBar;
class QToolBar;
class QLineEdit;
class QPushButton;
class QEnterEvent;
class QFocusEvent;
class QSplitter;
class QWheelEvent;

// Graphics contexgt classes for application windows.
enum XIC_WINDOW_CLASS
{
    XW_DEFAULT, // Misc.
    XW_TEXT,    // Prompt line and similar.
    XW_DRAWING, // Main drawing area and viewports.
    XW_LPAL,    // The layer palette.
    XW_LTAB     // The layer table.
};

#define GS_NBTNS 8

typedef bool(*EventHandlerFunc)(QObject*, QEvent*, void*);

class QTeventMonitor : public QObject
{
    Q_OBJECT

public:
    // Event list element.
    //
    struct evl_t
    {
        evl_t(QObject *o, QEvent *e) : obj(o), ev(e), next(0) { }
        ~evl_t()    { delete ev; }

        QObject     *obj;
        QEvent      *ev;
        evl_t       *next;
    };

    // Object list element.
    //
    struct ol_t
    {
        ol_t(const QObject *o, ol_t *n) : obj(o), next(n) { }

        const QObject *obj;
        ol_t        *next;
    };

    QTeventMonitor()
    {
        em_event_list = 0;
        em_busy_allow_list = 0;
        em_event_handler = 0;
        em_event_handler_arg = 0;
        for (int i = 0; i < GS_NBTNS; i++)
            em_button_state[i] = false;
    }

    bool has_saved_events() const   { return (em_event_list != 0); }

    // Process and clear the event list.
    //
    void do_saved_events()
        {
            while (em_event_list) {
                evl_t *evtmp = em_event_list;
                em_event_list = evtmp->next;
                QObject::eventFilter(evtmp->obj, evtmp->ev);
                delete evtmp;
            }
        }

    // Append an event to the list.
    //
    void save_event(QObject *obj, const QEvent *evp)
        {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
            QEvent *ev = evp->clone();
#else
            //XXX no clone function in QT5.
            (void)evp;
            QEvent *ev = 0;
#endif
            if (!em_event_list)
                em_event_list = new evl_t(obj, ev);
            else {
                evl_t *evl = em_event_list;
                while (evl->next)
                    evl = evl->next;
                evl->next = new evl_t(obj, ev);
            }
        }

    // Return true if obj is in the busy_allow list.
    //
    bool is_busy_allow(const QObject *obj)
        {
            for (ol_t *e = em_busy_allow_list; e; e = e->next) {
                if (e->obj == obj)
                    return (true);
            }
            return (false);
        }

    // Add an object whose events will be processed when the Busy flag
    // is on.  These would have an "abort" data item in the GTK version.
    //
    void add_busy_allow(const QObject *obj)
        {
            if (!is_busy_allow(obj)) {
                em_busy_allow_list = new ol_t(obj, em_busy_allow_list);
            }
        }

    // Remove obj from the busy_allow list if present.
    //
    void remove_busy_allow(const QObject *obj)
        {
            ol_t *ep = 0;
            for (ol_t *e = em_busy_allow_list; e; e = e->next) {
                if (e->obj == obj) {
                    if (ep)
                        ep->next = e->next;
                    else
                        em_busy_allow_list = e->next;
                    delete e;
                    return;
                }
                ep = e;
            }
        }

    EventHandlerFunc set_event_handler(EventHandlerFunc func, void *arg)
        {
            EventHandlerFunc f = em_event_handler;
            em_event_handler = func;
            em_event_handler_arg = arg;
            return (f);
        }

    static int saved_events_idle(void *arg)
        {
            QTeventMonitor *evmon = static_cast<QTeventMonitor*>(arg);
            if (evmon)
                evmon->do_saved_events();
            return (0);
        }

    static void  log_event(const QObject*, const QEvent*);

private:
    evl_t       *em_event_list;
    ol_t        *em_busy_allow_list;
    EventHandlerFunc em_event_handler;
    void        *em_event_handler_arg;
    bool        em_button_state[GS_NBTNS];

protected:
    bool eventFilter(QObject*, QEvent*) override;
};


class QTpkg : public DSPpkg
{
public:
    QTpkg()
        {
            pkg_busy_popup      = 0;
            pkg_in_main_loop    = false;
            pkg_not_mapped      = false;
        }

    static QTpkg *self() { return (dynamic_cast<QTpkg*>(DSPpkg::self())); }

    // DSPpkg virtual overrides
    GRwbag *NewGX();
    int Initialize(GRwbag*);
    void ReinitNoGraphics();
    void Halt();
    void AppLoop();
    bool CheckForInterrupt();
    int Iconify(int);
    bool SubwinInit(int);
    void SubwinDestroy(int);

    bool SetWorking(bool);
    void SetOverrideBusy(bool);
    bool GetMainWinIdentifier(char*);

    bool UsingX11();
    void CloseGraphicsConnection();
    const char *GetDisplayString();
    bool CheckScreenAccess(hostent*, const char*, const char*);
    int RegisterIdleProc(int(*)(void*), void*);
    bool RemoveIdleProc(int);
    int RegisterTimeoutProc(int, int(*)(void*), void*);
    bool RemoveTimeoutProc(int);
    int StartTimer(int, bool*);
    void SetFont(const char*, int, FNT_FMT = FNT_FMT_ANY);
    const char *GetFont(int);
    FNT_FMT GetFontFmt();
    // end of overrides

    void PopUpBusy();
    void SetWaitCursor(bool);

    QTeventMonitor *EventMonitor()      { return (&pkg_event_monitor); }
    void RegisterEventHandler(EventHandlerFunc f, void *a)
                                { pkg_event_monitor.set_event_handler(f, a); }
    bool NotMapped()                    { return (pkg_not_mapped); }

private:
    static int busy_msg_timeout(void*);

    GRpopup     *pkg_busy_popup;        // busy indicator
    bool        pkg_in_main_loop;       // gtk_main called
    bool        pkg_not_mapped;         // true when iconic
    QTeventMonitor pkg_event_monitor;   // event dispatch control
};

// Length of keypress buffer.
#define CBUFMAX 15

class cKeys : public QTcanvas
{
    Q_OBJECT

public:
    cKeys(int, QWidget*);

    QSize sizeHint() const;
    void show_keys();
    void set_keys(const char*);
    void bsp_keys();
    void check_exec(bool);

    int key_pos()       { return (k_keypos); }
    int key(int k)      { return (k_keys[k]); }
    char *key_buf()     { return (k_keys); }
    void append(char c) { k_keys[k_keypos++] = c; }

private slots:
    void font_changed(int);

private:
    int k_keypos;
    int k_win_number;
    char k_keys[CBUFMAX + 1];
    const char *k_cmd;
};

class QTsubwin : public QDialog, virtual public DSPwbag, public QTbag,
    public QTdraw
{
    Q_OBJECT

public:
    QTsubwin(int, QWidget* = 0);
    ~QTsubwin();

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    // cAppWinFuncs interface
    //
    // pixmap manipulations
    void SwitchToPixmap();
    void SwitchFromPixmap(const BBox*);
    GRobject DrawableReset();
    void CopyPixmap(const BBox*);
    void DestroyPixmap();
    bool DumpWindow(const char*, const BBox*);
    bool PixmapOk();

    // key handling/display interface
    void GetTextBuf(char*);
    void SetTextBuf(const char*);
    void ShowKeys();
    void SetKeys(const char*);
    void BspKeys();
    bool AddKey(int);
    bool CheckBsp();
    void CheckExec(bool);
    char *KeyBuf();
    int KeyPos();

    // label
    void SetLabelText(const char*);

    // misc pop-ups
    void PopUpGrid(GRobject, ShowMode);
    void PopUpExpand(GRobject, ShowMode,
        bool(*)(const char*, void*), void*, const char*, bool);
    void PopUpZoom(GRobject, ShowMode);
    //
    // End of cAppWinFuncs interface

    QToolBar *ToolBar()             const { return (sw_toolbar); }
    cKeys *Keys()                   const { return (sw_keys_pressed); }
    void clear_expand()             { sw_expand = 0; }
    int win_number()                const { return (sw_win_number); }
    int cursor_type()               const { return (sw_cursor_type); }
    void set_cursor_type(int c)     { sw_cursor_type = c; }

    QSize sizeHint()                const { return (QSize(500, 400)); }
    QSize minimumSizeHint()         const { return (QSize(250, 200)); }

    // Override Tab/Shift-Tab focus change, these are used for Unde/Redo
    // in Xic.
    bool focusNextPrevChild(bool);

    bool keypress_handler(unsigned int, unsigned int, const char*, bool, bool);

signals:
    void update_coords(int, int);

protected slots:
    void resize_slot(QResizeEvent*);
    void button_down_slot(QMouseEvent*);
    void button_up_slot(QMouseEvent*);
    void double_click_slot(QMouseEvent*);
    void motion_slot(QMouseEvent*);
    void key_down_slot(QKeyEvent*);
    void key_up_slot(QKeyEvent*);
    void enter_slot(QEnterEvent*);
    void leave_slot(QEvent*);
    void focus_in_slot(QFocusEvent*);
    void focus_out_slot(QFocusEvent*);
    void mouse_wheel_slot(QWheelEvent*);
    void drag_enter_slot(QDragEnterEvent*);
    void drop_slot(QDropEvent*);
    void font_changed(int);
    void help_slot();

protected:
    QToolBar    *sw_toolbar;
    QPixmap     *sw_pixmap;
    cKeys       *sw_keys_pressed;
    QTexpandDlg *sw_expand;
    QTzoomDlg   *sw_zoom;
    QTgridDlg   *sw_gridpop;
    WindowDesc  *sw_windesc;
    int         sw_win_number;
    int         sw_cursor_type;
};

class QTmainwin : public QTsubwin
{
    Q_OBJECT

public:
    static QTmainwin *self()
        {
            if (DSP()->MainWdesc())
                return (dynamic_cast<QTmainwin*>(DSP()->MainWdesc()->Wbag()));
            return (0);
        }

    static bool exists()
        {
            if (DSP()->MainWdesc()) {
                return (dynamic_cast<QTmainwin*>(
                    DSP()->MainWdesc()->Wbag()) != 0);
            }
            return (false);
        }

    QMenuBar *MenuBar()         { return (mw_menubar); }
    QWidget *PromptLine()       { return (mw_promptline); }
    QTltab *LayerTable()        { return (mw_layertab); }
    QSplitter *splitter()       { return (mw_splitter); }

    QWidget *TopButtonBox()     { return (mw_top_button_box); }
    QWidget *PhysButtonBox()    { return (mw_phys_button_box); }
    QWidget *ElecButtonBox()    { return (mw_elec_button_box); }

    // qtmain.cc
    QTmainwin(QWidget* = 0);
#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif
    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    void initialize();
    void send_key_event(sKeyEvent*);

    // qtcells.cc
    static char *get_cell_selection();
    static void cells_panic();

    // qtfiles.cc
    static char *get_file_selection();
    static void files_panic();

    // qtlibs.cc
    static char *get_lib_selection();
    static void libs_panic();

    // qttree.cc
    static char *get_tree_selection();
    static void tree_panic();

signals:
    void side_button_press(MenuEnt*);
    void run_queued(void*, void*);

public slots:
    void update_coords_slot(int, int);

private slots:
    void run_queued_slot(void*, void*);

private:
    // QWidget virtual overrides
    void closeEvent(QCloseEvent*);

    QMenuBar    *mw_menubar;
    QWidget     *mw_top_button_box;
    QWidget     *mw_phys_button_box;
    QWidget     *mw_elec_button_box;
    QSplitter   *mw_splitter;

    QWidget     *mw_promptline;
    QTcoord     *mw_coords;
    QTltab      *mw_layertab;
    QTparam     *mw_status;
};


//-----------------------------------------------------------------------------
// Utilities


inline unsigned int mod_state(int qtstate)
{
    int state = 0;
    if (qtstate & Qt::ShiftModifier)
        state |= GR_SHIFT_MASK;
    if (qtstate & Qt::ControlModifier)
        state |= GR_CONTROL_MASK;
    if (qtstate & Qt::AltModifier)
        state |= GR_ALT_MASK;
    if (qtstate & Qt::KeypadModifier)
        state |= GR_KEYPAD_MASK;
    return (state);
}


inline bool is_modifier_key(int key)
{
    return (
        key == Qt::Key_Shift    ||
        key == Qt::Key_Control  ||
        key == Qt::Key_Meta     ||
        key == Qt::Key_Alt      ||
        key == Qt::Key_CapsLock);
}

#endif

