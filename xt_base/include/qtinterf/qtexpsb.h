
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTEXPSB_H
#define QTEXPSB_H

#include <QDoubleSpinBox>


//----------------------------------------------------------------------------
// A Double Spin Box that uses exponential notation.

namespace qtinterf {
    class QTexpDoubleSpinBox;
}

class qtinterf::QTexpDoubleSpinBox : public QDoubleSpinBox
{
public:
    explicit QTexpDoubleSpinBox(QWidget *prnt = nullptr) :
        QDoubleSpinBox(prnt)
        {
            QDoubleSpinBox::setDecimals(400);
            d_decimals = 5;
        }
    ~QTexpDoubleSpinBox() { }

    // This is subtle:  there is a hard-coded round function in
    // QDoubleSpinBox that is used in setValue that will round the
    // values to zero if the negative exponent is too large.  A way
    // around this is to set the decimals value to something large
    // (see source code).  We implement our own decimals for
    // significant figs display.

    int decimals() const        const { return (d_decimals); }
    void setDecimals(int d)     { d_decimals = d; }

    // Virtual overrides.
    void stepBy(int);
    double valueFromText(const QString & text) const;
    QString textFromValue(double value) const;
    QValidator::State validate(QString &text, int&) const;

private:
    int d_decimals;
};

#endif

