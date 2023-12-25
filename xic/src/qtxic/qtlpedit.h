
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

#ifndef QTLPEDIT_H
#define QTLPEDIT_H

#include "main.h"
#include "qtmain.h"
#include "kwstr_lyr.h"
#include "kwstr_ext.h"
#include "kwstr_phy.h"
#include "kwstr_cvt.h"

#include <QDialog>


//--------------------------------------------------------------------
// Tech parameter editor dialog.
//

class QLabel;
class QMenu;
class QAction;
class QMouseEvent;

class QTlayerParamDlg : public QDialog
{
    Q_OBJECT

public:
    enum { LayerPage, ExtractPage, PhysicalPage, ConvertPage };
    enum { edEdit, edDelete, edUndo, edQuit };

    QTlayerParamDlg(GRobject, const char*, const char*);
    ~QTlayerParamDlg();

    QSize sizeHint() const;
    void update(const char*, const char*);

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    static QTlayerParamDlg *self()          { return (instPtr); }

private slots:
    void edit_menu_slot(QAction*);
    void layer_menu_slot(QAction*);
    void extract_menu_slot(QAction*);
    void physical_menu_slot(QAction*);
    void convert_menu_slot(QAction*);
    void global_menu_slot(QAction*);
    void help_slot();
    void page_changed_slot(int);
    void mouse_press_slot(QMouseEvent*);
    void font_changed_slot(int);

private:
    void insert(const char*, const char*, const char*);
    void undoop();
    void load(const char*);
    void clear_undo();
    void update();
    void call_callback(const char*);
    void select_range(int, int);
    void check_sens();

    GRobject    lp_caller;              // initiating button
    QTtextEdit  *lp_text;               // text window

    QMenu       *lp_lyr_menu;           // sub-menu
    QMenu       *lp_ext_menu;           // sub-menu
    QMenu       *lp_phy_menu;           // sub-menu
    QMenu       *lp_cvt_menu;           // sub-menu

    QLabel      *lp_label;              // info label
    QAction     *lp_cancel;             // quit menu entry
    QAction     *lp_edit;               // edit menu entry
    QAction     *lp_del;                // delete menu entry
    QAction     *lp_undo;               // undo menu entry
    QAction     *lp_ivis;               // Invisible menu entry
    QAction     *lp_nmrg;               // NoMerge menu entry
    QAction     *lp_xthk;               // CrossThick menu entry
    QAction     *lp_wira;               // WireActive menu entry
    QAction     *lp_noiv;               // NoInstView menu entry
    QAction     *lp_darkfield;          // DarkField menu entry
    QAction     *lp_rho;                // Rho menu entry
    QAction     *lp_sigma;              // Sigma menu entry
    QAction     *lp_rsh;                // Rsh menu entry
    QAction     *lp_tau;                // Tau menu entry
    QAction     *lp_epsrel;             // EpsRel menu entry
    QAction     *lp_cap;                // Capacitance menu entry
    QAction     *lp_lambda;             // Lambda menu entry
    QAction     *lp_tline;              // Tline menu entry
    QAction     *lp_antenna;            // Antenna menu entry
    QAction     *lp_nddt;               // NoDrcDataType menu entry

    CDl         *lp_ldesc;              // current layer
    int         lp_line_selected;       // number of line selected or -1
    bool        lp_in_callback;         // true when callback called
    int         lp_start;
    int         lp_end;
    int         lp_page;
    lyrKWstruct lp_lyr_kw;
    extKWstruct lp_ext_kw;
    phyKWstruct lp_phy_kw;
    cvtKWstruct lp_cvt_kw;

    static QTlayerParamDlg *instPtr;
};

#endif

