
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef TECH_SPACETAB_H
#define TECH_SPACETAB_H


// These are arrayed to provide a spacing table.  The entries field
// of the first struct contains the table size (including the first
// struct), and the dimen contains the default dimension.  The width
// and length take values explained velow.  The subsequent elements
// provide the actual table values.  Entries are sorted by ascending
// order in width and then length.  This implements size-dependent
// spacing rules.
//
struct sTspaceTable
{
    void set(int e, int w, int l, int d)
        {
            entries = e;
            width = w;
            length = l;
            dimen = d;
        }

    unsigned int evaluate(unsigned int v1, unsigned int v2) const
        {
            const sTspaceTable *t = this + entries - 1;
            while (t != this) {
                if (v1 >= t->width && v2 >= t->length)
                    return (t->dimen);
                t--;
            }
            return (dimen);
        }

    static bool check_sort(sTspaceTable*);
    static char *to_lisp_string(const sTspaceTable*, const char*,
        const char*, const char*);
    static void tech_print(const sTspaceTable*, FILE*, sLstr*);

    static sTspaceTable *tech_parse(const char**, const char**);

    // In the first record, the width will be 1 or 2 for the
    // dimensionality.  The length will be set to flags from the
    // defines below.  We record all of the Cadence keywords except
    // for `inLayerDir.
    //
    // ( minSpacing tx_layer
    //     ( ( "width" nil nil [ "width" | "length" nil nil ] ) 
    //         ['inLayerDir tx_layer2][ 'horizontal | 'vertical | 'any ]
    //         [ 'sameNet [ 'PGNet ] | 'sameMetal ]
    //         [ g_defaultValue ] 
    // )

#define STF_IGNORE  0x1         // table will not be used
#define STF_WIDWID  0x2         // table is "width" "width"
#define STF_TWOWID  0x4         // table is "twoWidth" "length"
#define STF_HORIZ   0x9
#define STF_VERT    0x10
#define STF_SAMENET 0x20
#define STF_PGNET   0x40
#define STF_SAMEMTL 0x80

    unsigned int entries;
    unsigned int width;
    unsigned int length;
    int dimen;
};

#endif

