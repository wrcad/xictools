
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

#ifndef FIO_GDS_LAYER_H
#define FIO_GDS_LAYER_H

#include "cd_strmdata.h"


// Hash table element for layer names.
struct tmp_odata : public strm_odata
{
    tmp_odata()                     { td_name = 0; }
    ~tmp_odata()                    { delete [] td_name; }

    const char *tab_name()          { return (td_name); }
    tmp_odata *tab_next()           { return ((tmp_odata*)next()); }
    void set_tab_next(tmp_odata *n) { set_next(n); }

    void set_name(const char *nm)
        {
            char *s = lstring::copy(nm);
            delete [] td_name;
            td_name = s;
        }

    void set_layer(unsigned int l)  { so_layer = l; }
    void set_dtype(unsigned int d)  { so_dtype = d; }

private:
    const char *td_name;
};

// Cache object for GDSII layer/datatype mapping.
//
struct gds_lspec
{
    gds_lspec(int l, int d, char *hn, CDll *s)
        {
            layer = l;
            dtype = d;
            strncpy(hexname, hn ? hn : "", 12);
            list = s;
        }

    ~gds_lspec()
        {
            CDll::destroy(list);
        }

    int layer;          // Layer number.
    int dtype;          // Datatype number.
    char hexname[12];   // If list is null, the hex name code.
    CDll *list;         // List of layers, hexname is "" when set,
                        // if null and hexname is "", skip.
};

#endif

