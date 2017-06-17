
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
 $Id: cd_digest.h,v 5.3 2013/11/10 06:13:13 stevew Exp $
 *========================================================================*/

#ifndef CD_DIGEST_H
#define CD_DIGEST_H


class cCHD;
class cCGD;
struct sChdPrp;


inline class cCDchdDB *CDchd();

class cCDchdDB
{
    static cCDchdDB *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cCDchdDB *CDchd() { return (cCDchdDB::ptr()); }

    cCDchdDB();

    bool chdStore(const char*, cCHD*);
    cCHD *chdRecall(const char*, bool);
    const char *chdFind(const cCHD*);
    const char *chdFind(const sChdPrp*);
    const char *chdFind(const char*, cv_alias_info*);
    stringlist *chdList();
    void chdClear();
    char *newChdName();

    bool isEmpty()      { return (!chd_table || !chd_table->allocated()); }

private:
    SymTab *chd_table;             // Table of saved cCHD structs.

    static cCDchdDB *instancePtr;
};


inline class cCDcgdDB *CDcgd();

class cCDcgdDB
{
    static cCDcgdDB *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cCDcgdDB *CDcgd() { return (cCDcgdDB::ptr()); }

    cCDcgdDB();

    bool cgdStore(const char*, cCGD*);
    cCGD *cgdRecall(const char*, bool);
    stringlist *cgdList();
    void cgdClear();
    char *newCgdName();

    bool isEmpty()      { return (!cgd_table || !cgd_table->allocated()); }

private:
    SymTab *cgd_table;             // Table of saved cCGD structs.

    static cCDcgdDB *instancePtr;
};

#endif

