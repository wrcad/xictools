
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
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
 $Id: pcell.h,v 5.17 2015/03/16 04:26:30 stevew Exp $
 *========================================================================*/

#ifndef PCELL_H
#define PCELL_H


//
// Parameterized cell (PCell) support.
//

struct SIfile;
struct PCellParam;
struct PCellDesc;

// Tokens that may be found in head of the XIC_SCRIPT property text.
// The XIC_PC_SCRIPT property text is in the form
//
//    [@LANG langtok] [@READ path] [@MD5 digest] [script text]
//
// where langtok may be one of (case insensitive)
//   n[ative] (native sript, the default)
//   p[ython] (python script)
//   t[cl]    (tcl script)
//
// The path if given is to the script text, otherwise the script
// text follows.  The path should be quoted if it contains white
// space.
//
#define PCELL_READ_TOK      "@READ"
#define PCELL_LANG_TOK      "@LANG"
#define PCELL_MD5_TOK       "@MD5"

// Script languages recognized in PCells.
enum PClangType { PCnative, PCpython, PCtcl };


inline class cPCellDb *PC();

class cPCellDb
{
    static cPCellDb *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cPCellDb *PC() { return (ptr()); }

    cPCellDb();

    // pcell.cc
    bool setPCinstParams(const char*, const PCellParam*, bool = false);
    void revertPCinstParams();
    const PCellParam *curPCinstParams(const char*);

    char *addSubMaster(const char*, const char*, const char*,
        const PCellParam*);
    char *addSuperMaster(const char*, const char*, const char*,
        const PCellParam*);
    PCellDesc *findSuperMaster(const char*);

    bool getParams(const char*, const char*, PCellParam**);
    bool getDefaultParams(const char*, PCellParam**);
    bool parseParams(const char*, PCellParam**, const PCellParam*);
    bool formatParams(const char*, char**, const char*);

    OItype openMaster(const char*, DisplayMode, CDs**);
    OItype openSubMaster(const CDs*, const char*, CDs**);
    bool openSubMaster(const char*, const char*, CDs**, CDs*);
    bool reopenSubMaster(CDs*);
    bool isSuperMaster(const CDs*);

    bool evalScript(CDs*, const char*);
    bool openScript(const CDs*, SIfile**, char**, PClangType*);
    char *md5Digest(const char*);
    void dump(FILE*);

private:
    // The hash tables hash the lib/cell/view names as a token formatted
    // as a "dbname" with PCellDesc::mk_dbname.  For Xic native pcells,
    // the libname is XIC_NATIVE_LIBNAME, the view is "layout" or
    // "schematic".

    // Store last parameter set for each PCell encountered, but revert
    // after script execution.  Data item is a PCellParam list.
    SymTab *pc_param_tab;
    SymTab *pc_script_param_tab;

    // Store sub-master names created for each super-master.  Data
    // item is a PCellDesc.
    SymTab *pc_master_tab;

    static cPCellDb *instancePtr;
};

#endif

