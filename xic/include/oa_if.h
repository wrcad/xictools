
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

#ifndef OA_IF_H
#define OA_IF_H

#include "oa_base.h"

// Exported reading log file name.
#define READ_OA_FN  "read_oa.log"

inline class cOAif *OAif();

// Main class to interface Xic to OpenAccess.
//
class cOAif
{
    static cOAif *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cOAif *OAif() { return (cOAif::ptr()); }

    cOAif();
    virtual ~cOAif() { }

    // capability flag
    bool hasOA() { return (has_oa); }

    bool initialize()
        { return (oaPtr->initialize()); }
    const char *version()
        { return (oaPtr->version()); } 
    const char *set_debug_flags(const char *on, const char *off)
        { return (oaPtr->set_debug_flags(on, off)); }

    bool is_library(const char *libname, bool *retval)
        { return (oaPtr->is_library(libname, retval)); }
    bool list_libraries(stringlist **retval)
        { return (oaPtr->list_libraries(retval)); }
    bool list_lib_cells(const char *libname, stringlist **retval)
        { return (oaPtr->list_lib_cells(libname, retval)); }
    bool list_cell_views(const char *libname, const char *cellname,
            stringlist **retval)
        { return (oaPtr->list_cell_views(libname, cellname, retval)); }
    bool set_lib_open(const char *libname, bool open)
        { return (oaPtr->set_lib_open(libname, open)); }
    bool is_lib_open(const char *libname, bool *retval)
        { return (oaPtr->is_lib_open(libname, retval)); }
    bool is_oa_cell(const char *libname, bool open_libs_only, bool *retval)
        { return (oaPtr->is_oa_cell(libname, open_libs_only, retval)); }
    bool is_cell_in_lib(const char *libname, const char *cellname,
            bool *retval)
        { return (oaPtr->is_cell_in_lib(libname, cellname, retval)); }
    bool is_oa_cellview(const char *libname, const char *cellname,
            bool open_libs_only, bool *retval)
        { return (oaPtr->is_oa_cellview(libname, cellname, open_libs_only,
            retval)); }
    bool is_cellview_in_lib(const char *libname, const char *cellname,
            const char *viewname, bool *retval)
        { return (oaPtr->is_cellview_in_lib(libname, cellname, viewname,
            retval)); }
    bool create_lib(const char *libname, const char *oldlibname)
        { return (oaPtr->create_lib(libname, oldlibname)); }
    bool brand_lib(const char *libname, bool brand)
        { return (oaPtr->brand_lib(libname, brand)); }
    bool is_lib_branded(const char *libname, bool *retval)
        { return (oaPtr->is_lib_branded(libname, retval)); }
    bool destroy(const char *libname, const char *cellname,
            const char*viewname)
        { return (oaPtr->destroy(libname, cellname, viewname)); }

    bool load_library(const char *libname)
        { return (oaPtr->load_library(libname)); }
    bool load_cell(const char *libname, const char *cellname,
        const char* viewname, int depth, bool setcur,
        const char **new_cell_name = 0, PCellParam **pm = 0)
        { return (oaPtr->load_cell(libname, cellname, viewname, depth, setcur,
          new_cell_name, pm)); }
    OItype open_lib_cell(const char *cellname, CDcbin *cbin)
        { return (oaPtr->open_lib_cell(cellname, cbin)); }
    void clear_name_table()
        { oaPtr->clear_name_table(); }

    bool save(const CDcbin *cbin, const char *libname, bool allhier = false,
            const char *altname = 0)
        { return (oaPtr->save(cbin, libname, allhier, altname)); }

    // tech
    bool attach_tech(const char *libname, const char *fromlib)
        { return (oaPtr->attach_tech(libname, fromlib)); }
    bool destroy_tech(const char *libname, bool unattach_only)
        { return (oaPtr->destroy_tech(libname, unattach_only)); }
    bool has_attached_tech(const char *libname, char **attached_lib)
        { return (oaPtr->has_attached_tech(libname, attached_lib)); }
    bool has_local_tech(const char *libname, bool *hastech)
        { return (oaPtr->has_local_tech(libname, hastech)); }
    bool create_local_tech(const char *libname)
        { return (oaPtr->create_local_tech(libname)); }
    bool save_tech()
        { return (oaPtr->save_tech()); }
    bool print_tech(FILE *fp, const char *libname, const char *which,
            const char *prname)
        { return (oaPtr->print_tech(fp, libname, which, prname)); }

    // graphics
    void PopUpOAlibraries(GRobject, ShowMode);
    void GetSelection(const char**, const char**);
    void PopUpOAtech(GRobject, ShowMode, int, int);
    void PopUpOAdefs(GRobject, ShowMode, int, int);

private:
    cOA_base *oaPtr;
    bool has_oa;

    static cOAif *instancePtr;
};

#endif

