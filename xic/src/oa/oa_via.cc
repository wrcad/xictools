
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

#include "main.h"
#include "promptline.h"
#include "oa_if.h"
#include "pcell_params.h"
#include "oa.h"
#include "oa_via.h"
#include "oa_tech_observer.h"
#include "oa_lib_observer.h"


namespace {
    void add_c(sLstr &lstr, int c)
    {
        if (lstr.string())
            lstr.add_c(' ');
        lstr.add_c(c);
    }
}


// Static function.
// Generate a string describing the non-default parameters.  The
// format is as a set of space-separated tokens, each token contains a
// keyword followed by a colon, followed by an integer, or
// comma-separated pair of integers.
// E.g., "keyword:d keyword:d,d ..."
//
char *
cOAvia::getStdViaString(const oaStdViaHeader *stdViaHeader)
{
    oaStdViaDef *def = (oaStdViaDef*)stdViaHeader->getViaDef();
    oaViaParam defparam;
    def->getParams(defparam);
    oaViaParam vparam;
    stdViaHeader->getParams(vparam);

    sLstr lstr;
    if (vparam.getCutWidth() != defparam.getCutWidth()) {
        add_c(lstr, 'a');
        lstr.add_g(MICRONS(vparam.getCutWidth())/1000);
    }
    if (vparam.getCutHeight() != defparam.getCutHeight()) {
        add_c(lstr, 'b');
        lstr.add_g(MICRONS(vparam.getCutHeight())/1000);
    }
    if (vparam.getCutRows() != defparam.getCutRows()) {
        add_c(lstr, 'c');
        lstr.add_g(vparam.getCutRows());
    }
    if (vparam.getCutColumns() != defparam.getCutColumns()) {
        add_c(lstr, 'd');
        lstr.add_g(vparam.getCutColumns());
    }
    if (vparam.getCutSpacing().x() != defparam.getCutSpacing().x()) {
        add_c(lstr, 'e');
        lstr.add_g(MICRONS(vparam.getCutSpacing().x())/1000);
    }
    if (vparam.getCutSpacing().y() != defparam.getCutSpacing().y()) {
        add_c(lstr, 'f');
        lstr.add_g(MICRONS(vparam.getCutSpacing().y())/1000);
    }
    if (vparam.getLayer1Enc().x() != defparam.getLayer1Enc().x()) {
        add_c(lstr, 'g');
        lstr.add_g(MICRONS(vparam.getLayer1Enc().x())/1000);
    }
    if (vparam.getLayer1Enc().y() != defparam.getLayer1Enc().y()) {
        add_c(lstr, 'h');
        lstr.add_g(MICRONS(vparam.getLayer1Enc().y())/1000);
    }
    if (vparam.getLayer1Offset().x() != defparam.getLayer1Offset().x()) {
        add_c(lstr, 'i');
        lstr.add_g(MICRONS(vparam.getLayer1Offset().x())/1000);
    }
    if (vparam.getLayer1Offset().y() != defparam.getLayer1Offset().y()) {
        add_c(lstr, 'j');
        lstr.add_g(MICRONS(vparam.getLayer1Offset().y())/1000);
    }
    if (vparam.getLayer2Enc().x() != defparam.getLayer2Enc().x()) {
        add_c(lstr, 'k');
        lstr.add_g(MICRONS(vparam.getLayer2Enc().x())/1000);
    }
    if (vparam.getLayer2Enc().y() != defparam.getLayer2Enc().y()) {
        add_c(lstr, 'l');
        lstr.add_g(MICRONS(vparam.getLayer2Enc().y())/1000);
    }
    if (vparam.getLayer2Offset().x() != defparam.getLayer2Offset().x()) {
        add_c(lstr, 'm');
        lstr.add_g(MICRONS(vparam.getLayer2Offset().x())/1000);
    }
    if (vparam.getLayer2Offset().y() != defparam.getLayer2Offset().y()) {
        add_c(lstr, 'n');
        lstr.add_g(MICRONS(vparam.getLayer2Offset().y())/1000);
    }
    if (vparam.getOriginOffset().x() != defparam.getOriginOffset().x()) {
        add_c(lstr, 'o');
        lstr.add_g(MICRONS(vparam.getOriginOffset().x())/1000);
    }
    if (vparam.getOriginOffset().y() != defparam.getOriginOffset().y()) {
        add_c(lstr, 'p');
        lstr.add_g(MICRONS(vparam.getOriginOffset().y())/1000);
    }

    if (def->hasImplant1()) {
        if (vparam.getImplant1Enc().x() != defparam.getImplant1Enc().x()) {
            add_c(lstr, 'q');
            lstr.add_g(MICRONS(vparam.getImplant1Enc().x())/1000);
        }
        if (vparam.getImplant1Enc().y() != defparam.getImplant1Enc().y()) {
            add_c(lstr, 'r');
            lstr.add_g(MICRONS(vparam.getImplant1Enc().y())/1000);
        }
        if (def->hasImplant2()) {
            if (vparam.getImplant2Enc().x() != defparam.getImplant2Enc().x()) {
                add_c(lstr, 's');
                lstr.add_g(MICRONS(vparam.getImplant2Enc().x())/1000);
            }
            if (vparam.getImplant2Enc().y() != defparam.getImplant2Enc().y()) {
                add_c(lstr, 't');
                lstr.add_g(MICRONS(vparam.getImplant2Enc().y())/1000);
            }
        }
    }

    /******* Old format, unused
    sLstr lstr;
    if (!vparam.hasDefault(oacLayer1EncViaParamType)) {
        oaVector v = vparam.getLayer1Enc();
        lstr.add(oaViaParamType(oacLayer1EncViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(v.x());
        lstr.add_c(',');
        lstr.add_i(v.y());
    }
    if (!vparam.hasDefault(oacLayer2EncViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        oaVector v = vparam.getLayer2Enc();
        lstr.add(oaViaParamType(oacLayer2EncViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(v.x());
        lstr.add_c(',');
        lstr.add_i(v.y());
    }
    if (!vparam.hasDefault(oacImplant1EncViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        oaVector v = vparam.getImplant1Enc();
        lstr.add(oaViaParamType(oacImplant1EncViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(v.x());
        lstr.add_c(',');
        lstr.add_i(v.y());
    }
    if (!vparam.hasDefault(oacImplant2EncViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        oaVector v = vparam.getImplant2Enc();
        lstr.add(oaViaParamType(oacImplant2EncViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(v.x());
        lstr.add_c(',');
        lstr.add_i(v.y());
    }
    if (!vparam.hasDefault(oacLayer1OffsetViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        oaVector v = vparam.getLayer1Offset();
        lstr.add(oaViaParamType(oacLayer1OffsetViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(v.x());
        lstr.add_c(',');
        lstr.add_i(v.y());
    }
    if (!vparam.hasDefault(oacLayer2OffsetViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        oaVector v = vparam.getLayer2Offset();
        lstr.add(oaViaParamType(oacLayer2OffsetViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(v.x());
        lstr.add_c(',');
        lstr.add_i(v.y());
    }
    if (!vparam.hasDefault(oacCutSpacingViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        oaVector v = vparam.getCutSpacing();
        lstr.add(oaViaParamType(oacCutSpacingViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(v.x());
        lstr.add_c(',');
        lstr.add_i(v.y());
    }
    if (!vparam.hasDefault(oacOriginOffsetViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        oaVector v = vparam.getOriginOffset();
        lstr.add(oaViaParamType(oacOriginOffsetViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(v.x());
        lstr.add_c(',');
        lstr.add_i(v.y());
    }
    if (!vparam.hasDefault(oacCutLayerViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        int d = vparam.getCutLayer();
        lstr.add(oaViaParamType(oacCutLayerViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(d);
    }
    if (!vparam.hasDefault(oacCutColumnsViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        int d = vparam.getCutColumns();
        lstr.add(oaViaParamType(oacCutColumnsViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(d);
    }
    if (!vparam.hasDefault(oacCutRowsViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        int d = vparam.getCutRows();
        lstr.add(oaViaParamType(oacCutRowsViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(d);
    }
    if (!vparam.hasDefault(oacCutWidthViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        int d = vparam.getCutWidth();
        lstr.add(oaViaParamType(oacCutWidthViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(d);
    }
    if (!vparam.hasDefault(oacCutHeightViaParamType)) {
        if (lstr.string())
            lstr.add_c(' ');
        int d = vparam.getCutHeight();
        lstr.add(oaViaParamType(oacCutHeightViaParamType).getName());
        lstr.add_c(':');
        lstr.add_i(d);
    }
    *******/

    return (lstr.string_trim());
}


// Static function.
// Parse the string, which is in the format described for
// getStdViaString.  Set the oaViaParam values accordingly.  Return
// false on a syntax error.
//
bool
cOAvia::parseStdViaString(const char *str, oaViaParam &vparam)
{
    if (!str)
        return (true);
    const char *strbk = str;
    char *tok = lstring::gettok(&str);
    if (!tok)
        return (true);
    str = strbk;
    bool old_style = strchr(tok, ':');
    delete [] tok;

    if (old_style) {
        // Old format: keyword:x[,y] ...

        const char *s = str;
        char tbuf[32];
        const int maxlen=20;
        while (*s) {
            while (isspace(*s))
                s++;
            if (!*s)
                break;
            char *t = tbuf;
            int i = 0;
            while (i < maxlen) {
                if (*s == ':')
                    break;
                if (!isalnum(*s))
                    return (false);
                *t++ = *s++;
                i++;
            }
            if (i == maxlen)
                return (false);
            if (*s != ':')
                return (false);
            s++;
            tbuf[i] = 0;
            if (lstring::cieq(tbuf,
                    oaViaParamType(oacLayer1EncViaParamType).getName())) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                vparam.setLayer1Enc(oaVector(x, y));
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacLayer2EncViaParamType).getName())) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                vparam.setLayer2Enc(oaVector(x, y));
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacImplant1EncViaParamType).getName())) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                vparam.setImplant1Enc(oaVector(x, y));
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacImplant2EncViaParamType).getName())) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                vparam.setImplant2Enc(oaVector(x, y));
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacLayer1OffsetViaParamType).getName())) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                vparam.setLayer1Offset(oaVector(x, y));
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacLayer2OffsetViaParamType).getName())) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                vparam.setLayer2Offset(oaVector(x, y));
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacCutSpacingViaParamType).getName())) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                vparam.setCutSpacing(oaVector(x, y));
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacOriginOffsetViaParamType).getName())) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                vparam.setOriginOffset(oaVector(x, y));
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacCutLayerViaParamType).getName())) {
                int x;
                if (sscanf(s, "%d", &x) != 1)
                    return (false);
                vparam.setCutLayer(x);
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacCutColumnsViaParamType).getName())) {
                int x;
                if (sscanf(s, "%d", &x) != 1)
                    return (false);
                vparam.setCutColumns(x);
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacCutRowsViaParamType).getName())) {
                int x;
                if (sscanf(s, "%d", &x) != 1)
                    return (false);
                vparam.setCutRows(x);
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacCutWidthViaParamType).getName())) {
                int x;
                if (sscanf(s, "%d", &x) != 1)
                    return (false);
                vparam.setCutWidth(x);
            }
            else if (lstring::cieq(tbuf,
                    oaViaParamType(oacCutHeightViaParamType).getName())) {
                int x;
                if (sscanf(s, "%d", &x) != 1)
                    return (false);
                vparam.setCutHeight(x);
            }
            else
                return (false);
            while (*s && !isspace(*s))
                s++;
        }
    }
    else {
        // New style.

        const char *s = str;
        while (*s) {
            while (isspace(*s))
                s++;
            if (!*s)
                break;
            int c = *s++;
            if (c < 'a' || c > 't')
                return (false);
            c -= 'a';
            const char *t = s;
            while (*s && !isspace(*s))
                s++;

            if (c == 0) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                vparam.setCutWidth(INTERNAL_UNITS(d/1000));
                continue;
            }
            if (c == 1) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                vparam.setCutHeight(INTERNAL_UNITS(d/1000));
                continue;
            }
            if (c == 2) {
                int i;
                if (sscanf(t, "%d", &i) != 1)
                    return (false);
                vparam.setCutRows(i);
                continue;
            }
            if (c == 3) {
                int i;
                if (sscanf(t, "%d", &i) != 1)
                    return (false);
                vparam.setCutColumns(i);
                continue;
            }
            if (c == 4) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getCutSpacing());
                v.x() = INTERNAL_UNITS(d/1000);
                vparam.setCutSpacing(v);
                continue;
            }
            if (c == 5) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getCutSpacing());
                v.y() = INTERNAL_UNITS(d/1000);
                vparam.setCutSpacing(v);
                continue;
            }
            if (c == 6) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getLayer1Enc());
                v.x() = INTERNAL_UNITS(d/1000);
                vparam.setLayer1Enc(v);
                continue;
            }
            if (c == 7) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getLayer1Enc());
                v.y() = INTERNAL_UNITS(d/1000);
                vparam.setLayer1Enc(v);
                continue;
            }
            if (c == 8) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getLayer1Offset());
                v.x() = INTERNAL_UNITS(d/1000);
                vparam.setLayer1Offset(v);
                continue;
            }
            if (c == 9) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getLayer1Offset());
                v.y() = INTERNAL_UNITS(d/1000);
                vparam.setLayer1Offset(v);
                continue;
            }
            if (c == 10) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getLayer2Enc());
                v.x() = INTERNAL_UNITS(d/1000);
                vparam.setLayer2Enc(v);
                continue;
            }
            if (c == 11) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getLayer2Enc());
                v.y() = INTERNAL_UNITS(d/1000);
                vparam.setLayer2Enc(v);
                continue;
            }
            if (c == 12) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getLayer2Offset());
                v.x() = INTERNAL_UNITS(d/1000);
                vparam.setLayer2Offset(v);
                continue;
            }
            if (c == 13) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getLayer2Offset());
                v.y() = INTERNAL_UNITS(d/1000);
                vparam.setLayer2Offset(v);
                continue;
            }
            if (c == 14) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getOriginOffset());
                v.x() = INTERNAL_UNITS(d/1000);
                vparam.setOriginOffset(v);
                continue;
            }
            if (c == 15) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getOriginOffset());
                v.y() = INTERNAL_UNITS(d/1000);
                vparam.setOriginOffset(v);
                continue;
            }
            if (c == 16) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getImplant1Enc());
                v.x() = INTERNAL_UNITS(d/1000);
                vparam.setImplant1Enc(v);
                continue;
            }
            if (c == 17) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getImplant1Enc());
                v.y() = INTERNAL_UNITS(d/1000);
                vparam.setImplant1Enc(v);
                continue;
            }
            if (c == 18) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getImplant2Enc());
                v.x() = INTERNAL_UNITS(d/1000);
                vparam.setImplant2Enc(v);
                continue;
            }
            if (c == 19) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                oaVector v(vparam.getImplant2Enc());
                v.y() = INTERNAL_UNITS(d/1000);
                vparam.setImplant2Enc(v);
                continue;
            }
            return (false);
        }
    }
    return (true);
}

