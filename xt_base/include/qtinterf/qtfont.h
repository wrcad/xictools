
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

#ifndef QTFONT_H
#define QTFONT_H

#include "ginterf/graphics.h"
#include "ginterf/fontutil.h"

#include <QVariant>
#include <QDialog>

//
// Font handling
//

class QFont;
class QListWidget;
class QListWidgetItem;
class QTextEdit;
class QFontDatabase;
class QPushButton;
class QComboBox;

namespace qtinterf
{
    struct QTfont : public GRfont
    {
        void initFonts();
        GRfontType getType();
        void setName(const char*, int);
        const char *getName(int);
        char *getFamilyName(int);
        bool getFont(void*, int);
        void registerCallback(void*, int);
        void unregisterCallback(void*, int);

    private:
        QFont *new_font(const char*, bool);
        void refresh(int);

        struct FcbRec
        {
            FcbRec(QWidget *w, FcbRec *n) { widget = w; next = n; }

            QWidget *widget;
            FcbRec *next;
        };

        struct sFrec
        {
            sFrec() { name = 0; font = 0; cbs = 0; }

            const char *name;
            QFont *font;
            FcbRec *cbs;
        } fonts[MAX_NUM_APP_FONTS];
    };

    class qt_bag;

    class QTfontPopup : public QDialog, public GRfontPopup
    {
        Q_OBJECT

    public:
        QTfontPopup(qt_bag*, int, void*);
        ~QTfontPopup();

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

        // GRfontPopup overrides
        void set_font_name(const char*);
        void update_label(const char*);

        void select_font(const QFont*);
        QFont *current_selection();
        char *current_face();
        char *current_style();
        int current_size();
        void add_choice(const QFont*, const char*);

        // This widget will be deleted when closed with the title bar "X"
        // button.  Qt::WA_DeleteOnClose does not work - our destructor is
        // not called.  The default behavior is to hide the widget instead
        // of deleting it, which would likely be a core leak here.
        void closeEvent(QCloseEvent*) { quit_slot(); }

    signals:
        void select_action(int, const char*, void*);
        void dismiss();

    private slots:
        void action_slot();
        void quit_slot();
        void face_changed_slot(QListWidgetItem*, QListWidgetItem*);
        void style_changed_slot(QListWidgetItem*, QListWidgetItem*);
        void size_changed_slot(QListWidgetItem*, QListWidgetItem*);
        void menu_choice_slot(int);

    private:
        QListWidget *face_list;
        QListWidget *style_list;
        QListWidget *size_list;
        QTextEdit *preview;
        QPushButton *apply;
        QPushButton *quit;
        QComboBox *menu;
        QFontDatabase *fdb;
    };
}

#endif

