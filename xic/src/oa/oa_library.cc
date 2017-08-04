
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

#include "main.h"
#include "promptline.h"
#include "oa.h"
#include "oa_prop.h"


// Return true if libname is the name of a library in the OA database.
//
bool
cOA::is_library(const char *libname, bool *retval)
{
    if (retval)
        *retval = false;
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    try {
        oaScalarName libName(oaNativeNS(), libname);
        if (retval)
            *retval = (oaLib::find(libName) != 0);
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


namespace {
    // Recursive function to list libraries found in the lib.defs
    // hierarchy.
    //
    stringlist *list_libraries_rc(oaLibDefList *list)
    {
        if (!list)
            return (0);
        stringlist *s0 = 0;
        try {
            oaIter<oaLibDefListMem> listIter(list->getMembers());
            oaLibDefListMem *listMem;
            while ((listMem = listIter.getNext()) != 0) {
                switch (listMem->getType()) {
                case oacLibDefType:
                    {
                        oaScalarName libName;
                        ((oaLibDef*)listMem)->getLibName(libName);
                        oaString tempString;
                        libName.get(tempString);
                        s0 = new stringlist(
                            lstring::copy(tempString), s0);
                    }
                    break;

                case oacLibDefListRefType:
                    {
                        oaString tempString;
                        ((oaLibDefListRef*)listMem)->getRefListPath(
                            tempString);
                        oaLibDefList  *nextList =
                            oaLibDefList::get(tempString, 'r');
                        stringlist *sl = list_libraries_rc(nextList);
                        if (sl) {
                            stringlist *st = sl;
                            while (sl->next)
                                sl = sl->next;
                            sl->next = s0;
                            s0 = st;
                        }
                    }
                    break;

                default:
                    break;
                }
            }
            return (s0);
        }
        catch (oaCompatibilityError &ex) {
            stringlist::destroy(s0);
            throw;
        }
        catch (oaException &excp) {
            stringlist::destroy(s0);
            throw;
        }
    }
}


// Return a list of library names from the OA database.
//
bool
cOA::list_libraries(stringlist **retval)
{
    if (retval)
        *retval = 0;
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    try {
        oaLibDefList *list = oaLibDefList::getTopList();
        stringlist *sl = list_libraries_rc(list);
        if (sl) {
            // The list is in reverse search order.
            stringlist *s0 = 0;
            while (sl) {
                stringlist *sx = sl;
                sl = sl->next;
                sx->next = s0;
                s0 = sx;
            }
            sl = s0;
        }
        if (retval)
            *retval = sl;
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


// Return a list of cells in the given library.
//
bool
cOA::list_lib_cells(const char *libname, stringlist **retval)
{
    if (retval)
        *retval = 0;
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }

    oaLib *lib = 0;
    try {
        oaScalarName libName(oaNativeNS(), libname);
        lib = oaLib::find(libName);
        if (!lib) {
            Errs()->add_error("Library %s not found.", libname);
            return (false);
        }
        if (!lib->getAccess(oacReadLibAccess)) {
            Errs()->add_error("Can't obtain read access to library %s.",
                libname);
            return (false);
        }

        stringlist *s0 = 0;
        oaIter<oaCell> iter(lib->getCells());
        oaCell *cell;
        while ((cell = iter.getNext()) != 0) {
            oaScalarName cellName;
            cell->getName(cellName);
            oaString tempString;
            cellName.get(tempString);
            s0 = new stringlist(lstring::copy(tempString), s0);
        }
        lib->releaseAccess();
        if (retval)
            *retval = s0;
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        if (lib)
            lib->releaseAccess();
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        if (lib)
            lib->releaseAccess();
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


// Return a list of the view names from the given cell in the given
// library.
//
bool
cOA::list_cell_views(const char *libname, const char *cellname,
    stringlist **retval)
{
    if (retval)
        *retval = 0;
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (0);
    }
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }
    if (!cellname || !*cellname) {
        Errs()->add_error("Null or empty cell name encountered.");
        return (false);
    }

    oaLib *lib = 0;
    try {
        oaScalarName libName(oaNativeNS(), libname);
        lib = oaLib::find(libName);
        if (!lib) {
            Errs()->add_error("Library %s not found.", libname);
            return (false);
        }
        if (!lib->getAccess(oacReadLibAccess)) {
            Errs()->add_error("Can't obtain read access to library %s.",
                libname);
            return (false);
        }

        oaScalarName cellName(oaNativeNS(), cellname);
        oaCell *cell = oaCell::find(lib, cellName);
        if (!cell) {
            lib->releaseAccess();
            Errs()->add_error("Can't can't find cell %s in library %s.",
                cellname, libname);
            return (false);
        }

        stringlist *s0 = 0;
        oaIter<oaCellView> iter(cell->getCellViews());
        oaCellView *cellview;
        while ((cellview = iter.getNext()) != 0) {
            oaView *view = cellview->getView();
            oaScalarName viewName;
            view->getName(viewName);
            oaString tempString;
            viewName.get(tempString);
            s0 = new stringlist(lstring::copy(tempString), s0);
        }
        lib->releaseAccess();
        if (retval)
            *retval = s0;
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        if (lib)
            lib->releaseAccess();
        cOA::handleFBCError(ex);
        return (0);
    }
    catch (oaException &excp) {
        if (lib)
            lib->releaseAccess();
        Errs()->add_error((const char*)excp.getMsg());
        return (0);
    }
}


// Add or remove libname from the table of "open" libraries.
//
bool
cOA::set_lib_open(const char *libname, bool open)
{
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }

    if (open) {
        if (!oa_open_lib_tab)
            oa_open_lib_tab = new SymTab(true, false);
        if (SymTab::get(oa_open_lib_tab, libname) == ST_NIL)
            oa_open_lib_tab->add(lstring::copy(libname), 0, false);
    }
    else {
        if (oa_open_lib_tab)
            oa_open_lib_tab->remove(libname);
    }
    return (true);
}


// Return true if libname is in the open libraries table.
//
bool
cOA::is_lib_open(const char *libname, bool *retval)
{
    if (retval)
        *retval = false;
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }
    if (!oa_open_lib_tab)
        return (true);
    if (retval)
        *retval = (SymTab::get(oa_open_lib_tab, libname) != ST_NIL);
    return (true);
}


// Return true in retval if cellname can be resolved in the OA
// database.
//
bool
cOA::is_oa_cell(const char *cellname, bool open_libs_only, bool *retval)
{
    if (retval)
        *retval = false;
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    if (!cellname || !*cellname) {
        Errs()->add_error("Null or empty cell name encountered.");
        return (false);
    }
    stringlist *liblist;
    if (!list_libraries(&liblist))
        return (false);
    for (stringlist *s = liblist; s; s = s->next) {
        if (open_libs_only) {
            bool isopen;
            if (!is_lib_open(s->string, &isopen)) {
                stringlist::destroy(liblist);
                return (false);
            }
            if (!isopen)
                continue;
        }
        bool found;
        if (!is_cell_in_lib(s->string, cellname, &found)) {
            stringlist::destroy(liblist);
            return (false);
        }
        if (found) {
            if (retval)
                *retval = true;
            break;
        }
    }
    stringlist::destroy(liblist);
    return (true);
}


// Return true if cellname exists in the OA library libname.
//
bool
cOA::is_cell_in_lib(const char *libname, const char *cellname, bool *retval)
{
    if (retval)
        *retval = false;
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }
    if (!cellname || !*cellname) {
        Errs()->add_error("Null or empty cell name encountered.");
        return (false);
    }

    oaLib *lib = 0;
    try {
        oaScalarName libName(oaNativeNS(), libname);
        lib = oaLib::find(libName);
        if (!lib) {
            Errs()->add_error("Library %s not found.", libname);
            return (false);
        }
        if (!lib->getAccess(oacReadLibAccess)) {
            Errs()->add_error("Can't obtain read access to library %s.",
                libname);
            return (false);
        }
        oaScalarName cellName(oaNativeNS(), cellname);
        oaCell *cell = oaCell::find(lib, cellName);
        lib->releaseAccess();
        if (retval)
            *retval = (cell != 0);
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        if (lib)
            lib->releaseAccess();
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        if (lib)
            lib->releaseAccess();
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


// Return true in retval if cellname/viewname can be resolved in the
// OA database.
//
bool
cOA::is_oa_cellview(const char *cellname, const char *viewname,
    bool open_libs_only, bool *retval)
{
    if (retval)
        *retval = false;
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    if (!cellname || !*cellname) {
        Errs()->add_error("Null or empty cell name encountered.");
        return (false);
    }
    if (!viewname || !*viewname) {
        Errs()->add_error("Null or empty view name encountered.");
        return (false);
    }
    stringlist *liblist;
    if (!list_libraries(&liblist))
        return (false);
    for (stringlist *s = liblist; s; s = s->next) {
        if (open_libs_only) {
            bool isopen;
            if (!is_lib_open(s->string, &isopen)) {
                stringlist::destroy(liblist);
                return (false);
            }
            if (!isopen)
                continue;
        }
        bool found;
        if (!is_cellview_in_lib(s->string, cellname, viewname, &found)) {
            stringlist::destroy(liblist);
            return (false);
        }
        if (found) {
            if (retval)
                *retval = true;
            break;
        }
    }
    stringlist::destroy(liblist);
    return (true);
}


// Return true if cellname/viewname exists in the OA library libname.
//
bool
cOA::is_cellview_in_lib(const char *libname, const char *cellname,
    const char *viewname, bool *retval)
{
    if (retval)
        *retval = false;
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (0);
    }
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }
    if (!cellname || !*cellname) {
        Errs()->add_error("Null or empty cell name encountered.");
        return (false);
    }
    if (!viewname || !*viewname) {
        Errs()->add_error("Null or empty view name encountered.");
        return (false);
    }

    oaLib *lib = 0;
    try {
        oaScalarName libName(oaNativeNS(), libname);
        lib = oaLib::find(libName);
        if (!lib) {
            Errs()->add_error("Library %s not found.", libname);
            return (false);
        }
        if (!lib->getAccess(oacReadLibAccess)) {
            Errs()->add_error("Can't obtain read access to library %s.",
                libname);
            return (false);
        }
        oaScalarName cellName(oaNativeNS(), cellname);
        oaScalarName viewName(oaNativeNS(), viewname);

        oaCellView *cv = oaCellView::find(lib, cellName, viewName);
        lib->releaseAccess();
        if (retval)
            *retval = (cv != 0);
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        if (lib)
            lib->releaseAccess();
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        if (lib)
            lib->releaseAccess();
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


// This will create the library if libname currently does not exist. 
// This will also set up the oaTech for the new library if oldlibname
// is given.  The new library will attach to the same library as
// oldlibname, or will attach to oldlibname.  If oldlibname is given
// (not null) then it must exist.
//
bool
cOA::create_lib(const char *libname, const char *oldlibname)
{
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (0);
    }
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }
    if (oldlibname && !strcmp(oldlibname, libname))
        oldlibname = 0;
    try {
        if (oldlibname) {
            oaScalarName libName(oaNativeNS(), oldlibname);
            oaLib *lib = oaLib::find(libName);
            if (!lib) {
                Errs()->add_error("Library %s was not found.", oldlibname);
                return (false);
            }
        }

        oaScalarName libName(oaNativeNS(), libname);
        oaLib *lib = oaLib::find(libName);
        if (lib)
            return (true);

        // Library does not currently exist in lib.defs.

        // Check our local path.
        sLstr lstr;
        const char *libpath = CDvdb()->getVariable(VA_OaLibraryPath);
        if (libpath && *libpath)
             lstr.add(libpath);
        else
             lstr.add(".");
        lstr.add_c('/');
        lstr.add(libname);
        oaString libPath(lstr.string());

        if (oaLib::exists(libPath)) {
            // Library does exist at this path.
            lib = oaLib::open(libName, libPath);
        }
        else {
            // Library does not exist on disk, so create it.  Obtain the
            // DM system from the variable or from "DMSystem" in the
            // environment.
            //
            const char *DMSystem = CDvdb()->getVariable(VA_OaDmSystem);
            if (!DMSystem || !*DMSystem)
                DMSystem = getenv("DMSystem");
            if (DMSystem) {
                if (*DMSystem == 't' || *DMSystem == 'T' ||
                        lstring::ciprefix("oaDMt", DMSystem))
                    DMSystem = "oaDMTurbo";
                else
                    DMSystem = "oaDMFileSys";
            }
            else
                DMSystem = OA_DEF_DMSYS;
            
            lib = oaLib::create(libName, libPath, oacSharedLibMode, DMSystem);
            if (oldlibname) {
                // If the originating library had a tech attachment,
                // create the same attachment in the new library.
                // Otherwise, attach the old library.

                oaScalarName oldLibraryName(oaNativeNS(), oldlibname);
                oaLib *oldlib = oaLib::find(oldLibraryName);
                if (oldlib) {
                    if (oaTech::hasAttachment(oldlib)) {
                        oaScalarName techLibName;
                        oaTech::getAttachment(oldlib, techLibName);
                        oaTech::attach(lib, techLibName);
                    }
                    else {
                        oaTech::attach(lib, oldLibraryName);
                    }
                }
            }
            // If we have not attached a tech, use the default
            // attachment if given.
            const char *attach_lib_name =
                CDvdb()->getVariable(VA_OaDefTechLibrary);
            if (attach_lib_name) {
                if (!oaTech::hasAttachment(lib)) {
                    oaScalarName techLibName(oaNativeNS(), attach_lib_name);
                    oaTech::attach(lib, techLibName);
                }
            }
            else if (!oaTech::hasAttachment(lib))
                oaTech::create(lib);

            // Create a property in the new library to "brand" the
            // library for Xic.  Only libraries with this property can
            // be written to.
            //
            oaLibDMData *dmd = oaLibDMData::open(libName, 'a');
            oaStringProp::create(dmd, OA_XICP_PFX"version",
                XM()->VersionString());
            dmd->save();
            dmd->close();
        }
        if (lib) {
            // Save the new library definition in the session top library
            // definition file.  Since oaLibdefList::openLibs() opened in
            // 'r'ead-only mode, first call get() on top list to change
            // its mode to 'a'ppend.
            oaLibDefList *list = oaLibDefList::getTopList();

            // If no lib.defs file exists, create a new one.
            if (!list) {
                list = oaLibDefList::get("lib.defs", 'w');
                list->save();
                oaLibDefList::openLibs();
            }

            // Now add the library definition to the file.  If no file
            // exists at this point, then none could be found and created
            // on disk.
            if (list) {
                oaString topListPath;
                list->getPath(topListPath);
                list->get(topListPath, 'a');
                oaLibDef::create(list, libName, libPath);
                list->save();
            }
        }
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


// Set or remove the Xic "brand" of the library.  Xic can only write
// to a branded library.
//
bool
cOA::brand_lib(const char *libname, bool brand)
{
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (0);
    }
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }

    try {
        oaScalarName libName(oaNativeNS(), libname);
        if (!oaLib::find(libName)) {
            Errs()->add_error("Library %s not found.", libname);
            return (false);
        }

        oaLibDMData *dmd = oaLibDMData::open(libName, 'a');
        oaProp *p = oaProp::find(dmd, OA_XICP_PFX"version");
        bool need_save = false;
        if (p) {
            need_save = true;
            p->destroy();
        }
        if (brand) {
            need_save = true;
            oaStringProp::create(dmd, OA_XICP_PFX"version",
                XM()->VersionString());
        }
        if (need_save)
            dmd->save();
        dmd->close();
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


// Return true in retval it the library is branded.
//
bool
cOA::is_lib_branded(const char *libname, bool *retval)
{
    if (retval)
        *retval = false;
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }

    try {
        oaScalarName libName(oaNativeNS(), libname);
        if (!oaLib::find(libName)) {
            Errs()->add_error("Library %s not found.", libname);
            return (false);
        }

        if (oaLibDMData::exists(libName)) {
            oaLibDMData *dmd = oaLibDMData::open(libName, 'r');
            if (retval)
                *retval = (oaProp::find(dmd, OA_XICP_PFX"version") != 0);
            dmd->close();
        }
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


namespace {
    // There is probably an easier way.  Hunt through the lib.def
    // files recursively until libName is found.  Destroy the entry
    // and save the update, which removes libName from the lib.def
    // file.
    //
    bool removeLib(oaLibDefList *list, const oaScalarName &libName)
    {
        oaLibDef *xx = oaLibDef::find(list, libName);
        if (xx) {
            oaString xxpath;
            xx->getLibPath(xxpath);
            xx->destroy();
            oaString path;
            list->getPath(path);
            list = oaLibDefList::get(path, 'a');
            list->save();

            // Rename, giving a ".defunct" suffix, if this does not exist.
            // Otherwise, leave it as-is.
            sLstr lstr;
            lstr.add(xxpath);
            lstr.add(".defunct");
            if (access(lstr.string(), F_OK) < 0)
                rename((const char*)xxpath, lstr.string());
            return (true);
        }
        oaLibDefListMem  *listMem;
        oaIter<oaLibDefListMem>  listIter(list-> getMembers ());
        while ((listMem = listIter. getNext()) != 0) {
            oaType memType = listMem->getType();
            if (memType == oacLibDefListRefType) {
                oaString  tempString;
                ((oaLibDefListRef*)listMem)->getRefListPath(tempString);
                oaLibDefList *nextList = oaLibDefList::get(tempString, 'r');
                if (removeLib(nextList, libName))
                    return (true);
            }
        }
        return (false);
    }
}


bool
cOA::destroy(const char *libname, const char *cellname, const char *viewname)
{
    bool branded;
    if (!is_lib_branded(libname, &branded))
        return (false);
    if (!branded) {
        Errs()->add_error("Library not writable from Xic.");
        return (false);
    }

    oaScalarName libName(oaNativeNS(), libname);
    oaLib *lib = oaLib::find(libName);
    if (!lib) {
        Errs()->add_error("Library %s not found.", libname);
        return (false);
    }

    try {
        if (!cellname || !*cellname) {
            // Remove the library from the lib.defs file, and change
            // the directory name giving it a ".defunct" suffix.  We
            // don't blow away the directory, so user can revert by
            // hand if necessary.

            oaLibDefList *list = oaLibDefList::getTopList();
            if (!list) {
                Errs()->add_error("No lib.defs file found.");
                return (false);
            }
            bool ret = removeLib(list, libName);
            if (!ret)
                Errs()->add_error("Removal failed.");
            return (ret);
        }
        if (!lib->getAccess(oacWriteLibAccess)) {
            Errs()->add_error("Can't obtain read access to library %s.",
                libname);
            return (false);
        }
        oaScalarName cellName(oaNativeNS(), cellname);
        if (viewname && *viewname) {
            // delete lib/cell/view
            oaScalarName viewName(oaNativeNS(), viewname);
            oaCellView *cv = oaCellView::find(lib, cellName, viewName);
            if (!cv) {
                Errs()->add_error(
                "Can't can't find cell/view %s/%s in library %s.",
                    cellname, viewname, libname);
                lib->releaseAccess();
                return (false);
            }
            oaDesign *design = oaDesign::find(libName, cellName, viewName);
            if (design)
                design->purge();
            if (oaDesign::exists(libName, cellName, viewName))
                oaDesign::destroy(libName, cellName, viewName);
            cv->destroy();
        }
        else {
            // delete lib/cell/*
            oaCell *cell = oaCell::find(lib, cellName);
            if (!cell) {
                Errs()->add_error("Can't can't find cell %s in library %s.",
                    cellname, libname);
                lib->releaseAccess();
                return (false);
            }
            cell->destroy();
        }
        lib->releaseAccess();
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        if (lib)
            lib->releaseAccess();
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        if (lib)
            lib->releaseAccess();
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}

