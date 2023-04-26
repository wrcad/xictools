
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

#ifndef FILE_D_H
#define FILE_D_H

#include "qtinterf/qtinterf.h"

#include <QVariant>
#include <QDialog>
#include <QIcon>

class QComboBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QMenu;
class QMenuBar;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;

namespace qtinterf
{
    class file_tree_widget;
    class file_list_widget;

    class QTfilePopup : public QDialog, public GRfilePopup, public QTbag
    {
        Q_OBJECT

    public:
        QTfilePopup(QTbag*, FsMode, void*, const char*);
        ~QTfilePopup();

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

        // GRfilePopup override
        char *get_selection();

        char *get_dir(QTreeWidgetItem *node = 0)
            { return (get_path(node ? node : curnode, false)); }
        void set_label();
        void flash(QTreeWidgetItem*);

        QSize sizeHint() const { return (QSize(500, 250)); }
        QSize minimumSizeHint() const { return (QSize(250, 125)); }

        // This widget will be deleted when closed with the title bar "X"
        // button.  Qt::WA_DeleteOnClose does not work - our destructor is
        // not called.  The default behavior is to hide the widget instead
        // of deleting it, which would likely be a core leak here.
        void closeEvent(QCloseEvent*) { quit_slot(); }

    signals:
        void file_selected(const char*, void*);
        void dismiss();

    private slots:
        void up_slot();
        void open_slot();
        void new_folder_slot();
        void new_folder_cb_slot(const char*, void*);
        void delete_slot();
        void delete_cb_slot(bool, void*);
        void rename_slot();
        void rename_cb_slot(const char*, void*);
        void new_root_slot();
        void root_cb_slot(const char*, void*);
        void new_cwd_slot();
        void new_cwd_cb_slot(const char*, void*);
        void show_filter_slot(bool);
        void filter_choice_slot(int);
        void filter_change_slot(const QString&);
        void quit_slot();
        void help_slot();
        void up_menu_slot(QAction*);
        void menu_update_slot();
        void tree_select_slot(QTreeWidgetItem*, QTreeWidgetItem*);
        void tree_collapse_slot(QTreeWidgetItem*);
        void tree_expand_slot(QTreeWidgetItem*);
        void list_files_slot();
        void list_select_slot(QListWidgetItem*, QListWidgetItem*);
        void list_double_clicked_slot(QListWidgetItem*);
        void flash_slot();
        void check_slot();

    private:
        void init();
        void select_file(const char*);
        void select_dir(QTreeWidgetItem*);
        char *get_path(QTreeWidgetItem*, bool);
        QTreeWidgetItem *insert_node(char*, QTreeWidgetItem*);
        void add_dir(QTreeWidgetItem*, char*);
        stringlist *tokenize_filter();
        char *get_newdir(const char*);

        QMenuBar *menubar;
        file_tree_widget *tree;
        file_list_widget *list;
        QLabel *label;
        QComboBox *filter;
        QAction *a_Up;
        QAction *a_Go;
        QAction *a_UpMenu;
        QAction *a_Open;
        QAction *a_New;
        QAction *a_Delete;
        QAction *a_Rename;
        QLineEdit *entry;
        QMenu *filemenu;
        QMenu *upmenu;
        QMenu *listmenu;
        QMenu *helpmenu;
        QTimer *timer;
        QTimer *flasher;
        int flasher_cnt;
        QTreeWidgetItem *flasher_item;

        FsMode config;
        QTreeWidgetItem *curnode;
        char *curfile;
        char *rootdir;
        char *cwd_bak;
        char *temp_string;
        int filter_index;
        QIcon closed_folder_icon;
        QIcon open_folder_icon;
        bool no_disable_go;
    };
}

#endif

