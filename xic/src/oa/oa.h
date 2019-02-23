
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

#ifndef OA_H
#define OA_H

#include "oa_base.h"
#include "oaDesignDB.h"
#include "oaDesignInterfaces.h"


using namespace oa;
using namespace std;

// Name for device library.
#define XIC_DEVICES "xic_devices"

// A fudge factor for text rendering size, to approximately match
// Virtuoso.  The Virtuoso text height is multiplied by this factor to
// yield the Xic text height.
#define CDS_TEXT_SCALE 1.5

// Default management system.
// Looks like oaDMFileSys is needed for Virtuoso 6.1.4 compatability.
//#define OA_DEF_DMSYS "oaDMTurbo"
#define OA_DEF_DMSYS "oaDMFileSys"

class cOAtechObserver;
class cOAlibObserver;
class cOApcellObserver;
struct PCellParam;

// Xic uses the cOA class to access OpenAccess functionality.  A
// pointer to an instance of cOA is provided by oa.so.  The cOA_base
// defines the interface.

class cOA : public cOA_base
{
public:

    // oa.cc
    cOA();
    const char *id_string();
    bool initialize();
    const char *version();
    static void handleFBCError(oaCompatibilityError&);

    // oa_errlog.cc
    const char *set_debug_flags(const char*, const char*);

    // oa_library.cc
    bool is_library(const char*, bool*);
    bool list_libraries(stringlist**);
    bool list_lib_cells(const char*, stringlist**);
    bool list_cell_views(const char*, const char*, stringlist**);
    bool set_lib_open(const char*, bool);
    bool is_lib_open(const char*, bool*);
    bool is_oa_cell(const char*, bool, bool*);
    bool is_cell_in_lib(const char*, const char*, bool*);
    bool is_oa_cellview(const char*, const char*, bool, bool*);
    bool is_cellview_in_lib(const char*, const char*, const char*, bool*);
    bool create_lib(const char*, const char*);
    bool brand_lib(const char*, bool);
    bool is_lib_branded(const char*, bool*);
    bool destroy(const char*, const char*, const char*);

    // oa_load.cc
    bool load_library(const char*);
    bool load_cell(const char*, const char*, const char*, int, bool,
        const char** = 0, PCellParam** = 0);
    OItype open_lib_cell(const char*, CDcbin*);
    void clear_name_table();

    // oa_save.cc
    bool save(const CDcbin*, const char*, bool = false, const char* = 0);

    // oa_tech.cc
    bool attach_tech(const char*, const char*);
    bool destroy_tech(const char*, bool);
    bool has_attached_tech(const char*, char**);
    bool has_local_tech(const char*, bool*);
    bool create_local_tech(const char*);
    bool save_tech();
    bool print_tech(FILE*, const char*, const char*, const char*);

private:
    bool save_cbin_hier(const CDcbin*, const char*);
    bool save_cell_hier(CDs*, const char*);
    bool save_cbin(const CDcbin*, const char*, const char*);
    bool save_cell(CDs*, const char*, const char*);

    cOAtechObserver *oa_tech_observer;
    cOAlibObserver *oa_lib_observer;
    cOApcellObserver *oa_pcell_observer;
    SymTab *oa_open_lib_tab;
    const char *oa_idstr;
    int oa_api_major;
    bool oa_initialized;
};

#endif

