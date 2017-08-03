
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

