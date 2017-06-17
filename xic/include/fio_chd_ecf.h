
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: fio_chd_ecf.h,v 5.3 2010/11/22 06:31:55 stevew Exp $
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

