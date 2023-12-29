
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

#ifndef QTPCPRMS_H
#define QTPCPRMS_H

#include "main.h"
#include "edit.h"
#include "qtmain.h"

#include <QDialog>
#include <QHash>


//-----------------------------------------------------------------------------
// QTpcellParamsDlg:  Dialog to edit a perhaps long list of parameters
// for a PCell.

class QLabel;
class QScrollArea;

class QTpcellParamsDlg : public QDialog
{
    Q_OBJECT

public:
    QTpcellParamsDlg(GRobject, PCellParam*, const char*, pcpMode);
    ~QTpcellParamsDlg();

    void update(const char*, PCellParam*);

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    static QTpcellParamsDlg *self()         { return (instPtr); }

private slots:
    void help_btn_slot();
    void open_btn_slot();
    void apply_btn_slot();
    void reset_btn_slot();
    void dismiss_btn_slot();

    void bool_type_slot(int);
    void choice_type_slot(const QString&);
    void num_type_slot(double);
    void ncint_type_slot(int);
    void nctime_type_slot(int);
    void ncfd_type_slot(double);
    void string_type_slot(const QString&);

private:
    QWidget *setup_entry(PCellParam*, sLstr&, char**);

    GRobject    pcp_caller;
    QLabel      *pcp_label;
    QScrollArea *pcp_swin;

    PCellParam  *pcp_params;
    PCellParam  *pcp_params_bak;
    char        *pcp_dbname;
    pcpMode     pcp_mode;
    QHash<QString, PCellParam*> pcp_hash;

    static QTpcellParamsDlg *instPtr;
};

#endif

