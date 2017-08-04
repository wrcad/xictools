
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

#ifndef FIO_CHD_DIFF_H
#define FIO_CHD_DIFF_H

#include "cd_compare.h"


// State container for cell difference operations.
//
struct CHDdiff
{
    CHDdiff(cCHD*, cCHD*);
    ~CHDdiff();

    void set_layers(const char *ll, bool sl)
        {
            char *s = lstring::copy(ll);
            delete [] df_layer_list;
            df_layer_list = s;
            df_skip_layers = sl;
        }

    void set_types(const char *t)
        {
            char *s = lstring::copy(t);
            delete [] df_obj_types;
            df_obj_types = s;
        }

    void set_max_diffs(unsigned int m)
        {
            df_max_diffs = m;
        }

    void set_geometric(bool g)
        {
            df_geometric = g;
        }

    void set_exp_arrays(bool e)
        {
            df_exp_arrays = e;
        }

    void set_sloppy_boxes(bool b)
        {
            df_sloppy_boxes = b;
        }

    void set_ignore_dups(bool b)
        {
            df_ignore_dups = b;
        }

    void set_properties(unsigned int e)
        {
            df_properties = e;
        }

    unsigned int diff_count()
        {
            return (df_diff_count);
        }

    DFtype diff(const char*, const char*, DisplayMode, Sdiff**);

private:
    char *df_st1;
    char *df_st2;
    cCHD *df_chd1;
    cCHD *df_chd2;
    const char *df_layer_list;
    const char *df_obj_types;
    unsigned int df_max_diffs;
    unsigned int df_diff_count;
    unsigned int df_properties;
    bool df_skip_layers;
    bool df_geometric;
    bool df_exp_arrays;
    bool df_sloppy_boxes;
    bool df_ignore_dups;
};

#endif

