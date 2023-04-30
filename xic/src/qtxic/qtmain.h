
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

//
// Main Xic application-specific header
//

#ifndef QTMAIN_H
#define QTMAIN_H

#include "main.h"
#include "qtinterf/qtinterf.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/draw_qt_w.h"
#include "dsp_tkif.h"

#include <QDialog>
#include <QFontMetrics>

class cCoord;
class cParam;
class cKeys;
class cExpand;
class idle_proc;
struct sKeyEvent;
class QTltab;

class QMenu;
class QMenuBar;

// Graphics contexgt classes for application windows.
enum XIC_WINDOW_CLASS
{
    XW_DEFAULT, // Misc.
    XW_TEXT,    // Prompt line and similar.
    XW_DRAWING, // Main drawing area and viewports.
    XW_LPAL,    // The layer palette.
    XW_LTAB     // The layer table.
};

// Length of keypress buffer
#define CBUFMAX 16

inline class QTpkg *qtPkgIf();

class QTpkg : public cGrPkg
{
public:
    QTpkg()
        {
            pkg_busy_popup      = 0;
            pkg_idle_control    = 0;
            pkg_in_main_loop    = false;
            pkg_not_mapped      = false;
        }

    friend inline QTpkg *qtPkgIf()
        { return (dynamic_cast<QTpkg*>(GRpkgIf())); }

    // cGrPkg virtual overrides
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

    bool IsDualPlane();
    bool IsTrueColor();
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

    void RegisterEventHandler(void(*)(QEvent*, void*), void*);
    bool NotMapped()                    { return (pkg_not_mapped); }
    GRpopup *BusyPopup()                { return (pkg_busy_popup); }
    void SetBusyPopup(GRpopup *w)       { pkg_busy_popup = w; }
    void *BusyPopupHome()               { return (&pkg_busy_popup); }

private:
    GRpopup     *pkg_busy_popup;        // busy indicator
    idle_proc   *pkg_idle_control;
    bool        pkg_in_main_loop;       // gtk_main called
    bool        pkg_not_mapped;         // true when iconic
};


// Length of keypress buffer
#define CBUFMAX 16

class cKeys : public draw_qt_w
{
public:
    cKeys(int, QWidget*);

    void show_keys();
    void set_keys(const char*);
    void bsp_keys();
    void check_exec(bool);

    int key_pos()       { return (keypos); }
    int key(int k)      { return (keys[k]); }
    char *key_buf()     { return (keys); }
    void append(char c) { keys[keypos++] = c; }

private:
    int keypos;
    char keys[CBUFMAX + 1];
    int win_number;
};

class QTsubwin : public QDialog, virtual public DSPwbag, public QTbag,
    public QTdraw
{
    Q_OBJECT

public:
    QTsubwin(int, QWidget*);
    ~QTsubwin();

    void subw_initialize(int);
    void pre_destroy(int);

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

    QMenuBar *MenuBar()             { return (sw_menubar); }

    cKeys *Keys()                   { return (sw_keys_pressed); }
    void clear_expand()             { sw_expand = 0; }

    static DrawType draw_type()     { return (sw_drawtype); }

    QSize sizeHint()                const { return (QSize(500, 400)); }
    QSize minimumSizeHint()         const { return (QSize(250, 200)); }

    bool keypress_handler(unsigned int, unsigned int, const char*, bool, bool);

signals:
    void update_coords(int, int);

protected slots:
    void resize_slot(QResizeEvent*);
    void new_painter_slot(QPainter*);
    void paint_slot(QPaintEvent*);
    void button_down_slot(QMouseEvent*);
    void button_up_slot(QMouseEvent*);
    void motion_slot(QMouseEvent*);
    void key_down_slot(QKeyEvent*);
    void key_up_slot(QKeyEvent*);
    void enter_slot(QEvent*);
    void leave_slot(QEvent*);
    void drag_enter_slot(QDragEnterEvent*);
    void drop_slot(QDropEvent*);

protected:
    QPixmap     *sw_pixmap;
    QMenuBar    *sw_menubar;
    cKeys       *sw_keys_pressed;
    cExpand     *sw_expand;
    WindowDesc  *sw_windesc;
    int         sw_win_number;

private:
    static DrawType sw_drawtype;
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

    // qtmain.cc
    QTmainwin();
    void initialize();
    void send_key_event(sKeyEvent*);

    draw_if *PromptLine()       { return (mw_promptline); }
    QTltab *LayerTable()        { return (mw_layertab); }
    
    /*
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
    */

    QSize sizeHint() const      { return (QSize(800, 650)); }
    QSize minimumSizeHint()     const { return (QSize(800, 650)); }

    QWidget *TopButtonBox()     { return (mw_top_button_box); }
    QWidget *PhysButtonBox()    { return (mw_phys_button_box); }
    QWidget *ElecButtonBox()    { return (mw_elec_button_box); }

signals:
    void side_button_press(MenuEnt*);

private slots:
    void wr_btn_slot();
    void update_coords_slot(int, int);

private:
    // QWidget virtual overrides
    void closeEvent(QCloseEvent*);

    QWidget     *mw_top_button_box;
    QWidget     *mw_phys_button_box;
    QWidget     *mw_elec_button_box;

    draw_if     *mw_promptline;
    cCoord      *mw_coords;
    QTltab      *mw_layertab;
    cParam      *mw_status;
};


//-----------------------------------------------------------------------------
// Utilities


inline unsigned int
mod_state(int qtstate)
{
    int state = 0;
    if (qtstate & Qt::ShiftModifier)
        state |= GR_SHIFT_MASK;
    if (qtstate & Qt::ControlModifier)
        state |= GR_CONTROL_MASK;
    if (qtstate & Qt::AltModifier)
        state |= GR_ALT_MASK;
    return (state);
}


inline bool
is_modifier_key(int key)
{
    return (
        key == Qt::Key_Shift    ||
        key == Qt::Key_Control  ||
        key == Qt::Key_Meta     ||
        key == Qt::Key_Alt      ||
        key == Qt::Key_CapsLock);
}

#endif

