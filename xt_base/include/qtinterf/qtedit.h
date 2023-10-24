
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

#ifndef QTEDIT_H
#define QTEDIT_H

#include "qtinterf.h"
#include <QVariant>
#include <QDialog>

class QLineEdit;
class QMenu;
class QMenuBar;
class QToolBar;
class QStatusBar;
class QTextEdit;

namespace qtinterf {
    class QTsearchDlg;
    class QTeditDlg;
}


class qtinterf::QTeditDlg : public QDialog, public GReditPopup,
    public QTbag
{
    Q_OBJECT

public:
    // internal widget state
    enum EventType { QUIT, SAVE, SAVEAS, SOURCE, LOAD, TEXTMOD };

    // widget configuration
    enum WidgetType { Editor, Browser, StringEditor, Mailer };

    QTeditDlg(QTbag*, QTeditDlg::WidgetType, const char*, bool,
        void*);
    ~QTeditDlg();

    // GRpopup overrides
    void set_visible(bool visib)
        {
            if (visib) {
                show();
                raise();
                activateWindow();
            }
            else
                hide();
        }
    void popdown();

    void set_caller(GRobject);
    WidgetType get_widget_type() { return (ed_widget_type); }
    const char *get_file() { return (ed_sourceFile); }
    void load_file(const char *f) { load_file_slot(f, 0); }

    void set_mailaddr(const char*);
    void set_mailsubj(const char*);

    QSize sizeHint() const
        {
            if (ed_widget_type == Mailer)
                return (QSize(400, 350));
            return (QSize(500, 300));
        }

    // This widget will be deleted when closed with the title bar "X"
    // button.  Qt::WA_DeleteOnClose does not work - our destructor is
    // not called.  The default behavior is to hide the widget instead
    // of deleting it, which would likely be a core leak here.
    void closeEvent(QCloseEvent*) { quit_slot(); }

private slots:
    void file_selection_slot(const char*, void*);
    void open_slot();
    void load_file_slot(const char*, void*);
    void load_slot();
    void read_file_slot(const char*, void*);
    void read_slot();
    void save_slot();
    void save_file_as_slot(const char*, void*);
    void save_as_slot();
    void print_slot();
    void send_slot();
    void quit_slot();
    void cut_slot();
    void copy_slot();
    void paste_slot();
    void search_slot();
    void source_slot();
    void attach_file_slot(const char*, void*);
    void attach_slot();
    void unattach_slot(QAction*);
    void font_slot();
    void help_slot();
    void text_changed_slot();
    void search_down_slot();
    void search_up_slot();
    void ignore_case_slot(bool);
    void font_changed_slot(int);

private:
    bool read_file(const char*, bool);
    bool write_file(const char*, int, int);
    void set_source(const char*);
    void set_sens(bool);

    WidgetType  ed_widget_type;
    EventType   ed_lastEvent;
    char        *ed_savedAs;
    char        *ed_sourceFile;
    char        *ed_dropFile;
    char        *ed_lastSearch;
    bool        ed_textChanged;
    bool        ed_ignCase;
    bool        ed_ignChange;
    QTsearchDlg *ed_searcher;

    QWidget     *ed_bar;
    QMenu       *ed_filemenu;
    QMenu       *ed_editmenu;
    QMenu       *ed_optmenu;
    QMenu       *ed_helpmenu;
    QLineEdit   *ed_to_entry;
    QLineEdit   *ed_subj_entry;
    QTextEdit   *ed_text_editor;
    QStatusBar  *ed_status_bar;

    QAction     *ed_Load;
    QAction     *ed_Read;
    QAction     *ed_Save;
    QAction     *ed_SaveAs;
    QAction     *ed_HelpMenu;
};

#endif

