
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

#include "spnumber/spnumber.h"
#include "qtexpsb.h"


//----------------------------------------------------------------------------
// A Double Spin Box that uses exponential notation.

using namespace qtinterf;

void
QTexpDoubleSpinBox::stepBy(int n)
{
    double d = value();
    bool neg = false;
    if (d < 0) {
        neg = true;
        d = -d;
    }
    double logd = log10(d);
    int ex = (int)floor(logd);
    logd -= ex;
    double mant = pow(10.0, logd);

    double del = 1.0;
    if ((n > 0 && !neg) || (n < 0 && neg))
        mant += del;
    else {
        if (mant - del < 1.0)
            mant = 1.0 - (1.0 - mant + del)/10;
        else
            mant -= del;
    }
    d = mant * pow(10.0, ex);
    if (neg)
        d = -d;
    if (d > maximum())
        d = maximum();
    else if (d < minimum())
        d = minimum();
    setValue(d);
}


double
QTexpDoubleSpinBox::valueFromText(const QString & text) const
{
    QByteArray text_ba = text.toLatin1();
    const char *str = text_ba.constData();
    double *d = SPnum.parse(&str, true);
    if (d)
        return (*d);
    return (0.0/0.0);  // NaN, "can't happen"
}


QString
QTexpDoubleSpinBox::textFromValue(double value) const
{
    const char *str = SPnum.printnum(value, (const char*)0, true,
        decimals());
    while (isspace(*str))
        str++;
    return (QString(str));
}


// Change the way we validate user input (if validate => valueFromText)
QValidator::State
QTexpDoubleSpinBox::validate(QString &text, int&) const
{
    QByteArray text_ba = text.toLatin1();
    const char *str = text_ba.constData();
    double *d = SPnum.parse(&str, true);
    return (d ? QValidator::Acceptable : QValidator::Invalid);
}

