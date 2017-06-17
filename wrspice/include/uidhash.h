
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: uidhash.h,v 2.1 2011/05/16 01:01:03 stevew Exp $
 *========================================================================*/

#ifndef UIDHASH_H
#define UIDHASH_H

#include "circuit.h"
#include "symtab.h"


// A uid hash table for mapping IFuid names to device instance
// structs, for sGENmodel.  Hashing speeds up construction for large
// circuits.

struct sGENinstTable
{
    sGENinstTable()
        {
            inst_tab = 0;
        }

    ~sGENinstTable()
        {
            delete inst_tab;
        }

    sGENinstance *find(IFuid n)
        {
            if (!inst_tab || !n)
                return (0);
            return (inst_tab->find((unsigned long)n));
        }

    sGENinstance *remove(IFuid n)
        {
            if (!inst_tab || !n)
                return (0);
            return (inst_tab->remove((unsigned long)n));
        }

    void link(sGENinstance *inst)
        {
            if (!inst_tab)
                inst_tab = new itable_t<sGENinstance>;
            inst_tab->link(inst, true);
            inst_tab = inst_tab->check_rehash();
        }

private:
    itable_t<sGENinstance> *inst_tab;
};

#endif

