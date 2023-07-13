
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

#ifndef QTDEVEDIT_H
#define QTDEVEDIT_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// The Device Parameters pop-up.  This panel allows addition of
// properties to library devices, and initiates saving the devices in
// the device library file or a cell file.
//

class QLineEdit;
class QPushButton;
class QComboBox;
class QCheckBox;

class QTdeviceDlg : public QDialog
{
    Q_OBJECT

public:
    // Container for text entries.
    //
    struct entries_t
    {
        entries_t()
        {
            cname = 0;
            prefix = 0;
            model = 0;
            value = 0;
            param = 0;
            branch = 0;
        }

        ~entries_t()
        {
            delete [] cname;
            delete [] prefix;
            delete [] model;
            delete [] value;
            delete [] param;
            delete [] branch;
        }

        char *cname;
        char *prefix;
        char *model;
        char *value;
        char *param;
        char *branch;
    };

    QTdeviceDlg(GRobject);
    ~QTdeviceDlg();

    void set_ref(int, int);

    static QTdeviceDlg *self()          { return (instPtr); }

private slots:
    void help_btn_slot();
    void hotspot_btn_slot(bool);
    void menu_changed_slot(int);
    void savlib_btn_slot();
    void savfile_btn_slot();
    void dismiss_btn_slot();

private:
    void load(entries_t*);
    void do_save(bool);

    GRobject    de_caller;
    QLineEdit   *de_cname;
    QLineEdit   *de_prefix;
    QLineEdit   *de_model;
    QLineEdit   *de_value;
    QLineEdit   *de_param;
    QPushButton *de_toggle;
    QLineEdit   *de_branch;
    QCheckBox   *de_nophys;
    int         de_menustate;
    int         de_xref, de_yref;

    static const char *orient_labels[];
    static QTdeviceDlg *instPtr;
};

#endif

