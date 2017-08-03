
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

#ifndef FIO_CHD_ECF_H
#define FIO_CHD_ECF_H

#include "symtab.h"


//
// CVecFilt:  class to mark cells that are empty due to layer
// filtering.
//

struct info_lnames_t;

struct CVecFilt
{
    CVecFilt()
        {
            ec_table = 0;
            ec_symrefs = 0;
            ec_ifln = 0;
            ec_num_empty = 0;
            ec_skip_layers = false;
        }

    ~CVecFilt()
        {
            unsetup();
        }

    bool setup(cCHD*, symref_t*);
    void unsetup();

    unsigned int num_empties()      const { return (ec_num_empty); }
    symref_t *symref(unsigned int i) const
        {
            if (i < ec_num_empty && ec_symrefs)
                return (ec_symrefs[i]);
            return (0);
        }

private:
    bool load_table_rc(cCHD*, symref_t*);

    ptrtab_t            *ec_table;
    symref_t            **ec_symrefs;
    info_lnames_t       *ec_ifln;
    unsigned int        ec_num_empty;
    bool                ec_skip_layers;

    static bool         ec_lock;
};

#endif

