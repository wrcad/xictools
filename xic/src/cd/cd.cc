
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

#include "config.h"
#include "cd.h"
#include "cd_memmgr.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "cd_sdb.h"
#include "cd_netname.h"
#include "fio_gencif.h"
#include "miscutil/filestat.h"


//
// Miscellaneous CD definitions and functions.
//

// CIF output generator primitives class.
sCifGen Gen;

// Database resolution.
int CDphysResolution = 1000;
const int CDelecResolution = 1000;

// Database size limit.
const int CDinfinity = 1000000000;
const BBox CDinfiniteBB(-CDinfinity, -CDinfinity, CDinfinity, CDinfinity);
const BBox CDnullBB(CDinfinity, CDinfinity, -CDinfinity, -CDinfinity);

// Default label character size (microns).
double CDphysDefTextHeight = CD_DEF_TEXT_HEI;
double CDphysDefTextWidth = CD_TEXT_WID_PER_HEI * CD_DEF_TEXT_HEI;
const double CDelecDefTextHeight = CD_DEF_TEXT_HEI;
const double CDelecDefTextWidth = CD_TEXT_WID_PER_HEI * CD_DEF_TEXT_HEI;

// Used for timer interval test.
unsigned long CDcheckTime;


cCD *cCD::instancePtr = 0;

cCD::cCD()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cCD already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    cdCellNameTable = 0;
    cdPfxTable = 0;
    cdInstNameTable = 0;
    cdArchiveTable = 0;
    cdAttrDB = 0;
#ifdef CD_PRPTY_TAB
    cdPrptyTab = 0;
#endif
    cdAllocTab = 0;
    cdNameCache = 0;
    cdLabelCache = 0;

    cdCoincErrCnt = 0;
    cdReading = 0;
    cdDeferInst = 0;

    cdUseNameCache = false;
    cdUseLabelCache = false;
    cdIgnoreIntr = false;
    cdNotStrict = false;
    cdNoPolyCheck = false;
    cdNoElec = false;
    cdNoMergeObjects = false;
    cdNoMergePolys = false;
    cdSubcCatchar = DEF_SUBC_CATCHAR;
    cdSubcCatmode = SUBC_CATMODE_WR;
    cdDupCheckMode = DupWarn;
    cdOut32nodes = false;

    cTfmStack::TClearStorage();

    new cCDmmgr;    // Memory manager.
    new cCDcdb;     // Cell database.
    new cCDvdb;     // Variable database.
    new cCDldb;     // Layer database.
    new cCDchdDB;   // CHD database.
    new cCDcgdDB;   // CGD database.
    new cCDsdbDB;   // SDB database.
}


#ifdef CD_TEST_NULL
// Private static error exit.
//
void
cCD::on_null_ptr()
{
    fprintf(stderr, "Singleton class cCD used before instantiated.\n");
    exit(1);
}
#endif


// The next three functions implement a table of struct allocation
// counts, for allocation debugging.  To track a struct, put
// RegisterCreate in the constructor, RegesterDestroy in the
// destructor.  Call with a constant string giving the struct name.
//
void
cCD::RegisterCreate(const char *name)
{
    if (!cdAllocTab)
        cdAllocTab = new SymTab(false, false);
    SymTabEnt *h = SymTab::get_ent(cdAllocTab, name);
    if (!h)
        cdAllocTab->add(name, (void*)1L, false);
    else {
        intptr_t cnt = (intptr_t)h->stData;
        cnt++;
        h->stData = (void*)cnt;
    }
}


void
cCD::RegisterDestroy(const char *name)
{
    if (!cdAllocTab)
        return;
    SymTabEnt *h = SymTab::get_ent(cdAllocTab, name);
    if (!h)
        cdAllocTab->add(name, (void*)-1L, false);
    else {
        intptr_t cnt = (intptr_t)h->stData;
        cnt--;
        h->stData = (void*)cnt;
    }
}


// Print the nonzero allocation counts to the console.
//
void
cCD::CheckAlloc()
{
    if (!cdAllocTab)
        return;
    SymTabGen gen(cdAllocTab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        intptr_t cnt = (intptr_t)h->stData;
        if (cnt != 0)
            printf("%s %lld\n", h->stTag, (long long)cnt);
    }
}


// Destroy all internal state, and revert the layer and variable
// databases.  If clear_tech is true, revert to a completely clean
// state, otherwise revert to the state just after the technology file
// was read.
//
void
cCD::ClearAll(bool clear_tech)
{
    CDcdb()->destroyAllTables();
    CDchd()->chdClear();
    CDcgd()->cgdClear();
    ClearStringTables();
    CDsdb()->destroyDB(0);
    if (clear_tech)
        CDldb()->reset();
    else
        CDldb()->revertState();
    ClearAttrDB();
    ClearPrptyTab();
    CDmmgr()->collectTrash();
    cdNoElec = false;
    cdCoincErrCnt = 0;
    cTfmStack::TClearStorage();
}


// If the CHD and symbol table lists are empty, clear the central
// symbol name table.  If there are no symbol tables, clear the
// archive and terminal name tables.
//
void
cCD::ClearStringTables()
{
    if (CDcdb()->isEmpty()) {
        if (CDchd()->isEmpty()) {
            delete cdCellNameTable;
            cdCellNameTable = 0;
        }
        delete cdPfxTable;
        cdPfxTable = 0;
        delete cdInstNameTable;
        cdInstNameTable = 0;
        delete cdArchiveTable;
        cdArchiveTable = 0;
        CDnetex::name_tab_clear();
    }
}


void
cCD::Error(int id, const char *str)
{
    if (!str)
        str = "<unknown>";
    switch (id) {
    case CDfailed:
    case CDok:
        return;
    case CDbadLayer:
        Errs()->add_error("No or bad layer given in cell %s.", str);
        return;
    case CDbadBox:
        Errs()->add_error("Zero width box, not allowed in cell %s.", str);
        return;
    case CDbadPolygon:
        Errs()->add_error("Degenerate polygon, not allowed in cell %s.", str);
        return;
    case CDbadWire:
        Errs()->add_error("Bad width or length wire, not allowed in cell %s.",
            str);
        return;
    case CDbadLabel:
        Errs()->add_error("Bad label specification.");
        return;
    default:
        Errs()->add_error("Unknown Error.");
        return;
    }
}


// Debugging error message printing, this should probably disappear.
//
void
cCD::DbgError(const char *s1, const char *s2)
{
    fprintf(stderr, "internal error: %s passed to %s.\n", s1, s2);
}


//
//  Cell name processing for the rename function.
//

namespace {
    inline bool
    pref(const char *pre, const char *string)
    {
        const char *s = string;
        const char *p = pre;
        while (*p) {
            if (*p != *s)
                return (false);
            p++;
            s++;
        }
        return (true);
    }


    inline bool
    sufx(const char *suf, const char *string)
    {
        const char *s = string + strlen(string) - strlen(suf);
        if (s < string)
            return (false);
        const char *p = suf;
        while (*p) {
            if (*p != *s)
                return (false);
            p++;
            s++;
        }
        return (true);
    }


    // Return /str/sub/ if in that form, or str = 0, sub = "string" if not
    // a substitution.
    //
    void
    split(const char *string, char **str, char **sub)
    {
        if (!string) {
            *str = 0;
            *sub = 0;
        }
        else if (*string == '/') {
            char buf[256];
            strcpy(buf, string+1);
            char *t = buf;
            while (*t && *t != '/')
                t++;
            *t++ = 0;
            if (*buf)
                *str = lstring::copy(buf);
            else
                *str = 0;
            char *s = t;
            while (*t && *t != '/')
                t++;
            *t = 0;
            if (*s)
                *sub = lstring::copy(s);
            else
                *sub = 0;
        }
        else {
            *str = 0;
            *sub = lstring::copy(string);
        }
    }
}


// Static function.
// Return a new name substituting or concatenating pre, oldname, suf.
// The pre and suf can either be a simple string, or a substitution
// form /str/sub/.  The substitution form syntax is assumed to be
// correct, no checking is done here.
//
char *
cCD::AlterName(const char *oldname, const char *pre, const char *suf)
{
    if (!oldname)
        return (0);
    char buf[256];
    buf[0] = 0;
    char *str, *sub;
    split(pre, &str, &sub);
    if (str) {
        if (pref(str, oldname)) {
            char *s = buf;
            const char *t = oldname + strlen(str);
            if (sub) {
                char *p = sub;
                while (*p)
                    *s++ = *p++;
            }
            while (*t)
                *s++ = *t++;
            *s = 0;
        }
        else
            strcpy(buf, oldname);
    }
    else if (sub && !pref(sub, oldname)) {
        const char *p = sub;
        char *s = buf;
        while (*p)
            *s++ = *p++;
        p = oldname;
        while (*p)
            *s++ = *p++;
        *s = 0;
    }
    else
        strcpy(buf, oldname);
    delete [] str;
    delete [] sub;

    split(suf, &str, &sub);
    if (str) {
        if (sufx(str, buf)) {
            char *s = buf + strlen(buf) - strlen(str);
            if (sub) {
                char *p = sub;
                while (*p)
                    *s++ = *p++;
            }
            *s = 0;
        }
    }
    else if (sub && !sufx(sub, oldname)) {
        char *p = sub;
        char *s = buf + strlen(buf);
        while (*p)
            *s++ = *p++;
        *s = 0;
    }
    delete [] str;
    delete [] sub;

    if (!*buf)
        return (lstring::copy(oldname));
    return (lstring::copy(buf));
}

