
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
 $Id: si_if.h,v 5.11 2009/11/18 03:54:18 stevew Exp $
 *========================================================================*/

#ifndef SI_IF_H
#define SI_IF_H


// This can be used as a base class of a container for
// application-specific variables, used to support the application's
// function set.  The clear method is called from
// SIinterp::siCleanup().  The struct should be registered with
// SIinterp::siRegisterLocalContext() (can do this in the
// constructor).
//
struct SIlocal_context
{
    virtual ~SIlocal_context() { }

    virtual void clear() = 0;
};


class cGroupDesc;
struct stringlist;
struct SIfile;
struct Zlist;

// Callback prototypes.
typedef DisplayMode (*SIif_GetCurMode)();
typedef CDl*(*SIif_GetCurLayer)();
typedef CDs*(*SIif_GetCurPhysCell)();
typedef const char*(*SIif_GetCurCellName)();
typedef CDs*(*SIif_GetCellFromGroupDesc)(cGroupDesc*);
typedef void (*SIif_SendMessage)(const char*, va_list);
typedef bool (*SIif_CheckInterrupt)(const char*);
typedef void (*SIif_ShowFunctionList)(stringlist*);
typedef void (*SIif_MacroParse)(SIfile*, stringlist**, const char**, int*);
typedef Zlist*(*SIif_GetSqZlist)(const CDl*, const Zlist*);


// This defines a set callbacks and registration functions, providing
// an interface to the application.  This will be a parent to SIinterp.
//
struct SIif
{
    SIif()
        {
            if_get_cur_mode = 0;
            if_get_cur_layer = 0;
            if_get_cur_phys_cell = 0;
            if_get_cur_cell_name = 0;
            if_get_cell_from_group_desc = 0;
            if_send_message = 0;
            if_check_interrupt = 0;
            if_show_function_list = 0;
            if_macro_parse = 0;
            if_get_sq_zlist = 0;
        }

    // Return the current display mode.
    //
    DisplayMode ifGetCurMode()
        {
            if (if_get_cur_mode)
                return ((*if_get_cur_mode)());
            return (Physical);
        }

    SIif_GetCurMode RegisterIfGetCurMode(SIif_GetCurMode f)
        {
            SIif_GetCurMode tmp = if_get_cur_mode;
            if_get_cur_mode = f;
            return (tmp);
        }

    // Return the current layer.
    //
    CDl *ifGetCurLayer()
        {
            if (if_get_cur_layer)
                return ((*if_get_cur_layer)());
            return (0);
        }

    SIif_GetCurLayer RegisterIfGetCurLayer(SIif_GetCurLayer f)
        {
            SIif_GetCurLayer tmp = if_get_cur_layer;
            if_get_cur_layer = f;
            return (tmp);
        }

    // Return the physical part ot the current cell, used to
    // determine the effective field size for geometric operations if
    // not otherwise known.
    //
    CDs *ifGetCurPhysCell()
        {
            if (if_get_cur_phys_cell)
                return ((*if_get_cur_phys_cell)());
            return (0);
        }

    SIif_GetCurPhysCell RegisterIfGetCurPhysCell(SIif_GetCurPhysCell f)
        {
            SIif_GetCurPhysCell tmp = if_get_cur_phys_cell;
            if_get_cur_phys_cell = f;
            return (tmp);
        }

    // Return the name of the current cell.
    //
    const char *ifGetCurCellName()
        {
            if (if_get_cur_cell_name)
                return ((*if_get_cur_cell_name)());
            return (0);
        }

    SIif_GetCurCellName RegisterIfGetCurCellName(SIif_GetCurCellName f)
        {
            SIif_GetCurCellName tmp = if_get_cur_cell_name;
            if_get_cur_cell_name = f;
            return (tmp);
        }

    // Resolve the cell desc from the passed cGroupDesc, which is
    // an opaque type within the interpreter core.  If extraction
    // is not used, this can return null (it won't be called
    // anyway).
    //
    CDs *ifGetCellFromGroupDesc(cGroupDesc *gd)
        {
            if (if_get_cell_from_group_desc)
                return ((*if_get_cell_from_group_desc)(gd));
            return (0);
        }

    SIif_GetCellFromGroupDesc RegisterIfGetCellFromGroupDesc(
            SIif_GetCellFromGroupDesc f)
        {
            SIif_GetCellFromGroupDesc tmp = if_get_cell_from_group_desc;
            if_get_cell_from_group_desc = f;
            return (tmp);
        }

    // Print a progress message emitted from the evaluator.
    //
    void ifSendMessage(const char *fmt, ...)
        {
            va_list ap;
            va_start(ap, fmt);
            (*if_send_message)(fmt, ap);
            va_end(ap);
        }

    SIif_SendMessage RegisterIfSendMessage(SIif_SendMessage f)
        {
            SIif_SendMessage tmp = if_send_message;
            if_send_message = f;
            return (tmp);
        }

    // Return true if the current operation should be aborted,
    // called periodically.
    //
    bool ifCheckInterrupt(const char *msg = 0)
        {
            if (if_check_interrupt)
                return ((*if_check_interrupt)(msg));
            return (false);
        }

    SIif_CheckInterrupt RegisterIfCheckInterrupt(SIif_CheckInterrupt f)
        {
            SIif_CheckInterrupt tmp = if_check_interrupt;
            if_check_interrupt = f;
            return (tmp);
        }

    // Display a listing of the functions in memory.
    //
    void ifShowFunctionList(stringlist *sl)
        {
            if (if_show_function_list)
                (*if_show_function_list)(sl);
        }

    SIif_ShowFunctionList RegisterIfShowFunctionList(SIif_ShowFunctionList f)
        {
            SIif_ShowFunctionList tmp = if_show_function_list;
            if_show_function_list = f;
            return (tmp);
        }

    // Parse the "#macro" direective.
    //
    void ifMacroParse(SIfile *sfp, stringlist **wl, const char **line,
        int *lcnt)
        {
            if (if_macro_parse)
                (*if_macro_parse)(sfp, wl, line, lcnt);
        }

    SIif_MacroParse RegisterIfMacroParse(SIif_MacroParse f)
        {
            SIif_MacroParse tmp = if_macro_parse;
            if_macro_parse = f;
            return (tmp);
        }

    // Obtain a list of trapezoids from the selection queue.
    //
    Zlist *ifGetSqZlist(const CDl *ld, const Zlist *zref)
        {
            if (if_get_sq_zlist)
                return ((*if_get_sq_zlist)(ld, zref));
            return (0);
        }

    SIif_GetSqZlist RegisterIfGetSqZlist(SIif_GetSqZlist f)
        {
            SIif_GetSqZlist tmp = if_get_sq_zlist;
            if_get_sq_zlist = f;
            return (tmp);
        }

private:
    SIif_GetCurMode if_get_cur_mode;
    SIif_GetCurLayer if_get_cur_layer;
    SIif_GetCurPhysCell if_get_cur_phys_cell;
    SIif_GetCurCellName if_get_cur_cell_name;
    SIif_GetCellFromGroupDesc if_get_cell_from_group_desc;
    SIif_SendMessage if_send_message;
    SIif_CheckInterrupt if_check_interrupt;
    SIif_ShowFunctionList if_show_function_list;
    SIif_MacroParse if_macro_parse;
    SIif_GetSqZlist if_get_sq_zlist;
};

#endif

