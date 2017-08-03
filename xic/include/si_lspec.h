
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

#ifndef SI_LSPEC_H
#define SI_LSPEC_H


enum CovType { CovNone, CovPartial, CovFull };

// Specification for a layer expression.  This can be a parse tree, or
// an individual layer, which is held separately to avoid overhead of
// a parse tree.  The layer can be specified as a string name, or as a
// layer descriptor.  The string is used to hold a name parsed from
// the tech file possibly before the layer is defined.  The layer desc
// will be filled in later.
//
struct sLspec
{
    sLspec()
        {
            clear();
        }

    ~sLspec();

    void clear()
        {
            ls_tree = 0;
            ls_ldesc = 0;
            ls_lname = 0;
        }

    ParseNode *tree()       const { return (ls_tree); }
    CDl *ldesc()            const { return (ls_ldesc); }
    const char *lname()     const { return (ls_lname); }

    void set_tree(ParseNode *p)     { ls_tree = p; }
    void set_ldesc(CDl *ld)         { ls_ldesc = ld; }

    void set_lname(const char *n)
        {
            char *nn = lstring::copy(n);
            delete [] ls_lname;
            ls_lname = nn;
        }

    void set_lname_pointer(char *n) { ls_lname = n; }

    // si_lexpr.cc
    void reset();
    bool parseExpr(const char**, bool = false);
    bool setup();
    char *string(bool = false) const;
    void print(FILE*);
    CDll *findLayers();

    XIrt testZlistCovFull(SIlexprCx*, CovType*, int);
    XIrt testZlistCovPartial(SIlexprCx*, CovType*, int);
    XIrt testZlistCovNone(SIlexprCx*, CovType*, int);
    XIrt getZlist(SIlexprCx*, Zlist**, bool = false);

    XIrt testContact(const CDs*, int, const CDo*, bool*);
    XIrt testContact(const CDs*, int, const Zlist*, bool*);

protected:
    ParseNode   *ls_tree;       // parse tree if expression
    CDl         *ls_ldesc;      // layer desc if layer
    char        *ls_lname;      // layer name if layer
};

#endif

