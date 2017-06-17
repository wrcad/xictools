
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2011 Whiteley Research Inc, all rights reserved.        *
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
 $Id: oa_base.h,v 5.21 2016/03/04 23:48:05 stevew Exp $
 *========================================================================*/

#ifndef OA_BASE_H
#define OA_BASE_H

// Xic does not link the OpenAccess shared libraries directly, instead
// Xic dynamically loads oa.so, which in turn is linked to the
// libraries.  Thus, Xic will have OpenAccess if the libraries are
// found on the system, but will not require them.
//
// This is the base class for the interface, used in both Xic and
// oa.so.  If OpenAccess is not available, it is constructed
// explicitly and provides method stubs in Xic.  Otherwise, the cOA
// derived class, constructed in oa.so, is passed to Xic for use.


// Open Access Variables

// Path to library (directory), used if a library being accessed isn't
// in the library definitions file.
//
#define VA_OaLibraryPath        "OaLibraryPath"

// A default library, used when a library was not specified in some
// commands.
//
#define VA_OaDefLibrary         "OaDefLibrary"

// When a new library is created, and the first cell saved does not
// indicate a technology library (i.e., not from OA), attach the tech
// library named by this variable.
//
#define VA_OaDefTechLibrary     "OaDefTechLibrary"

// These set the default names of the oacMaskLayout, oacSchematic, and
// oacSchematicSymbol views to open in OpenAccess.  The named views
// must bo of the correct type.  If not set, the names are effectively
// "layout", "schematic", and "symbol".
//
#define VA_OaDefLayoutView      "OaDefLayoutView"
#define VA_OaDefSchematicView   "OaDefSchematicView"
#define VA_OaDefSymbolView      "OaDefSymbolView"

// For devices, if we find this view, it will supply CDF info instead
// of the symbol view.  This will be a simulator-specific view.
//
#define VA_OaDefDevPropView     "OaDefDevPropView"

// If value starts with t or T, use oaDMturbo, otherwise use
// oaDMfileSys.
//
#define VA_OaDmSystem           "OaDmSystem"

// If this variable is set when a PCell is opened, the CDF data for
// the cell will be dumped to a file in the current directory.  The
// file name is the cell name with a ".cdf" extension.  This is for
// development/debugging.
//
#define VA_OaDumpCdfFiles       "OaDumpCdfFiles"

// If set to "1" or a word starting with "p" or "P", read only
// physical (maskLayout) views into Xic.  If set to "2" or a word
// starting with "e" or "E", read only electrical data (schematic and
// schematicSymbol views).  If set to anything else of not set, read
// both.
//
#define VA_OaUseOnly            "OaUseOnly"

// Default view names.
#define OA_DEF_LAYOUT       "layout"
#define OA_DEF_SCHEMATIC    "schematic"
#define OA_DEF_SYMBOL       "symbol"
#define OA_DEF_DEV_PROP     "HspiceD"

struct CDs;
struct CDcbin;
struct PCellParam;

class cOA_base
{
public:
    virtual ~cOA_base() { }

    virtual const char *id_string() = 0;
    virtual bool initialize() = 0;
    virtual const char *version() = 0;
    virtual const char *set_debug_flags(const char*, const char*) = 0;

    virtual bool is_library(const char*, bool*) = 0;
    virtual bool list_libraries(stringlist**) = 0;
    virtual bool list_lib_cells(const char*, stringlist**) = 0;
    virtual bool list_cell_views(const char*, const char*, stringlist**) = 0;
    virtual bool set_lib_open(const char*, bool) = 0;
    virtual bool is_lib_open(const char*, bool*) = 0;
    virtual bool is_oa_cell(const char*, bool, bool*) = 0;
    virtual bool is_cell_in_lib(const char*, const char*, bool*) = 0;
    virtual bool is_oa_cellview(const char*, const char*, bool, bool*) = 0;
    virtual bool is_cellview_in_lib(const char*, const char*, const char*,
        bool*) = 0;
    virtual bool create_lib(const char*, const char*) = 0;
    virtual bool brand_lib(const char*, bool) = 0;
    virtual bool is_lib_branded(const char*, bool*) = 0;
    virtual bool destroy(const char*, const char*, const char*) = 0;

    virtual bool load_library(const char*) = 0;
    virtual bool load_cell(const char*, const char*, const char*, int, bool,
            PCellParam** = 0, char** = 0) = 0;
    virtual OItype open_lib_cell(const char*, CDcbin*) = 0;
    virtual void clear_name_table() = 0;

    virtual bool save(const CDcbin*, const char*, bool = false,
        const char* = 0) = 0;

    virtual bool attach_tech(const char*, const char*) = 0;
    virtual bool destroy_tech(const char*, bool) = 0;
    virtual bool has_attached_tech(const char*, char**) = 0;
    virtual bool has_local_tech(const char*, bool*) = 0;
    virtual bool create_local_tech(const char*) = 0;
    virtual bool save_tech() = 0;
    virtual bool print_tech(FILE*, const char*, const char*, const char*) = 0;
};

#endif

