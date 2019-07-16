
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

#ifndef SCED_MODLIB_H
#define SCED_MODLIB_H

// The "model" library is used to cache archival device models and
// subcircuits.

// Entry to save in symbol table.
//
struct libent_t
{
    libent_t(char *d, char *f, char *t, long o)
        {
            dir = d;
            fname = f;
            token = lstring::copy(t);
            if (token)
                lstring::strtolower(token);
            offset = o;
        }
    ~libent_t() { delete [] token; }

    char *dir;     // directory containing file
    char *fname;   // file name
    char *token;   // name of model or subckt
    long offset;   // offset into file
};

// List of spice models, used in list generation.
//
struct mlib_t
{
    mlib_t(const char *n, int l, mlib_t *nx)
        {
            ml_name = lstring::copy(n);
            ml_level = l;
            ml_next = nx;
        }
    ~mlib_t() { delete [] ml_name; }

    char *ml_name;
    mlib_t *ml_next;
    int ml_level;
};

class cModLib
{
public:
    cModLib()
        {
            WordTab = 0;
            ModSymTab = 0;
            SubcSymTab = 0;
            Models = 0;
            Subckts = 0;
            ContextLevel = 0;
        }

    // Note: the destructor does *not* clear the contents.  One
    // should call Close before deleting.
    ~cModLib() { }

    void PushContext() { ContextLevel++; }
        // Push to the next context frame.

    void PopContext() { if (ContextLevel) ContextLevel--; }
        // Pop to the previous context frame.

    void Open(const char*);
    void QueueModel(const char*, const char*, const char*);
    void QueueSubckt(const char*);
    SpiceLine *PrintModels();
    SpiceLine *PrintSubckts();
    bool IsModel(const char*);
    SpiceLine *ModelText(const char*);
    SpiceLine *SubcktText(const char*);
    void Close();

private:
    char *word_tab(const char*);
    bool scan_file(const char*, const char*);
    SpiceLine *read_entry(libent_t*);
    libent_t *mosFind(const char*, const char*);

    SymTab *WordTab;    // Table for path strings that are used frequently,
                        // to avoid saving multiple copies.
    SymTab *ModSymTab;  // Symbol table for models.
    SymTab *SubcSymTab; // Symbol table for subcircuits.
    mlib_t *Models;     // Referenced model list head.
    mlib_t *Subckts;    // Referenced subcircuit list head.
    int ContextLevel;   // Keep track of subcircuit depth from application.
};

#endif

