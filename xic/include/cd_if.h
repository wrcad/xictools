
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

#ifndef CD_IF_H
#define CD_IF_H

#include "if_common.h"
#include <stdarg.h>


//
// Application interface for CD.  The application should call the
// Register functions to register callbacks to participate in the
// interface.
//

// This is the bloat mode that uses DRC sizing.
#define DRC_BLOAT 3

struct CDcellNameStr;
typedef CDcellNameStr* CDcellName;
struct CDs;
struct CDc;
#define NEWNMP
#ifdef NEWNMP
struct CDp_cname;
#else
struct CDp_name;
#endif
struct CDp_nmut;
struct CDo;
struct CDp;
struct CDl;
struct CDll;
struct CDterm;
struct CallDesc;
struct Label;
struct hyEnt;


// The display mode
enum DisplayMode { Physical=0, Electrical };
inline const char *DisplayModeName(DisplayMode m)
    { return (m == Physical ? "Physical" : "Electrical"); }
inline const char *DisplayModeNameLC(DisplayMode m)
    { return (m == Physical ? "physical" : "electrical"); }

// Return from sCD::Open and others
//
enum OItype
{
    OIaborted = -1,
    OIerror = 0,
    OIok,
    OIold,
    OInew,
    OIambiguous
};

inline bool
OIfailed(OItype t) { return ((int)t <= 0); }

// Callback prototypes.
typedef const char *(*CDif_NodeName)(const CDs*, int, bool*);
typedef void(*CDif_UpdateNodes)(const CDs*);
#ifdef NEWNMP
typedef void(*CDif_UpdateNameLabel)(CDc*, CDp_cname*);
#else
typedef void(*CDif_UpdateNameLabel)(CDc*, CDp_name*);
#endif
typedef bool(*CDif_UpdateMutLabel)(CDs*, CDp_nmut*);
typedef void(*CDif_LabelInstance)(CDs*, CDc*);
typedef bool(*CDif_UpdatePrptyLabel)(int, CDo*, Label*);
typedef int(*CDif_DefaultLabelSize)(const char*, DisplayMode, double*,
    double*);

typedef void(*CDif_InfoMessage)(INFOmsgType, const char*, va_list);
typedef void(*CDif_PrintCvLog)(OUTmsgType, const char*, va_list);

typedef void(*CDif_RecordHYent)(hyEnt*, bool);
typedef void(*CDif_RecordObjectChange)(CDs*, CDo*, CDo*);
typedef void(*CDif_RecordPrptyChange)(CDs*, CDo*, CDp*, CDp*);
typedef void(*CDif_CellModified)(const CDs*);
typedef void(*CDif_ObjectInserted)(CDs*, CDo*);

typedef void(*CDif_NewLayer)(CDl*);
typedef void(*CDif_LayerTableChange)(DisplayMode);
typedef void(*CDif_UnusedLayerListChange)(CDll*, DisplayMode);

typedef void(*CDif_InvalidateSymbol)(CDcellName);
typedef void(*CDif_InvalidateCell)(CDs*);
typedef void(*CDif_InvalidateObject)(CDs*, CDo*, bool);
typedef void(*CDif_InvalidateLayerContent)(CDs*, CDl*);
typedef void(*CDif_InvalidateLayer)(CDl*);
typedef void(*CDif_InvalidateTerminal)(CDterm*);

typedef bool(*CDif_CheckInterrupt)(const char*);
typedef void(*CDif_NoConfirmAbort)(bool);
typedef const char*(*CDif_IdString)();
typedef void(*CDif_VariableChange)();
typedef void(*CDif_ChdDbChange)();
typedef void(*CDif_CgdDbChange)();


// Interface class for callback functions.  CD sub-classes this interface
// to provide callbacks to the application.  The application can call the
// registration functions to implement the interface, overriding the
// default stubs.  The return from the registration functions is the
// previous registration, if any.
//
struct CDif
{
    CDif()
        {
            if_node_name = 0;
            if_update_nodes = 0;
            if_update_name_label = 0;
            if_update_mut_label = 0;
            if_label_instance = 0;
            if_update_prpty_label = 0;
            if_default_label_size = 0;

            if_info_message = 0;
            if_print_cv_log = 0;

            if_record_hyent = 0;
            if_record_object_change = 0;
            if_record_prpty_change = 0;
            if_cell_modified = 0;
            if_object_inserted = 0;

            if_new_layer = 0;
            if_layer_table_change = 0;
            if_unused_layer_list_change = 0;

            if_invalidate_symbol = 0;
            if_invalidate_cell = 0;
            if_invalidate_terminal = 0;
            if_invalidate_object = 0;
            if_invalidate_layer_content = 0;
            if_invalidate_layer = 0;

            if_check_interrupt = 0;
            if_no_confirm_abort = 0;
            if_id_string = 0;
            if_variable_change = 0;
            if_chd_db_change = 0;
            if_cgd_db_change = 0;
        }

    // Labels, etc., mostly for electrical mode.
    // Labels are shown near device symbols in electrical mode,
    // showing property values.
    //---------------------------------------------------------

    // The text name for the node is returned.  Applies to electrical
    // mode.
    //
    const char *ifNodeName(const CDs *sdesc, int node, bool *glob)
        {
            if (if_node_name) {
                return ((*if_node_name)(sdesc, node, glob));
            }
            return (0);
        }

    CDif_NodeName RegisterIfNodeName(CDif_NodeName f)
        {
            CDif_NodeName tmp = if_node_name;
            if_node_name = f;
            return (tmp);
        }

    // Update the P_NODMAP property which contains a mapping between
    // internal node numbers and assigned node names.  Applies to
    // electrical mode.
    //
    void ifUpdateNodes(const CDs *sdesc)
        {
            if (if_update_nodes)
                (*if_update_nodes)(sdesc);
        }

    CDif_UpdateNodes RegisterIfUpdateNodes(CDif_UpdateNodes f)
        {
            CDif_UpdateNodes tmp = if_update_nodes;
            if_update_nodes = f;
            return (tmp);
        }

    // Update the name label for cdesc from name property pna.
    // Applies to electrical mode.
    //
#ifdef NEWNMP
    void ifUpdateNameLabel(CDc *cdesc, CDp_cname *pna)
#else
    void ifUpdateNameLabel(CDc *cdesc, CDp_name *pna)
#endif
        {
            if (if_update_name_label)
                (*if_update_name_label)(cdesc, pna);
        }

    CDif_UpdateNameLabel RegisterIfUpdateNameLabel(CDif_UpdateNameLabel f)
        {
            CDif_UpdateNameLabel tmp = if_update_name_label;
            if_update_name_label = f;
            return (tmp);
        }

    // Create or update a mutual inductor label from mutual property
    // pm.  Return true on success.  Applies to electrical mode.
    //
    bool ifUpdateMutLabel(CDs *sdesc, CDp_nmut *pm)
        {
            if (if_update_mut_label)
                return ((*if_update_mut_label)(sdesc, pm));
            return (true);
        }

    CDif_UpdateMutLabel RegisterIfUpdateMutLabel(CDif_UpdateMutLabel f)
        {
            CDif_UpdateMutLabel tmp = if_update_mut_label;
            if_update_mut_label = f;
            return (tmp);
        }

    // Create an instance label for cdesc.  Applies to electrical
    // mode.
    //
    void ifLabelInstance(CDs *sdesc, CDc *cdesc)
        {
            if (if_label_instance)
                (*if_label_instance)(sdesc, cdesc);
        }

    CDif_LabelInstance RegisterIfLabelInstance(CDif_LabelInstance f)
        {
            CDif_LabelInstance tmp = if_label_instance;
            if_label_instance = f;
            return (tmp);
        }

    // Update the property label, applies to electrical mode.  Return
    // true if the updating was done.
    //
    bool ifUpdatePrptyLabel(int prpnum, CDo *odesc, Label *label)
        {
            if (if_update_prpty_label)
                return ((*if_update_prpty_label)(prpnum, odesc, label));
            return (false);
        }

    CDif_UpdatePrptyLabel RegisterIfUpdatePrptyLabel(CDif_UpdatePrptyLabel f)
        {
            CDif_UpdatePrptyLabel tmp = if_update_prpty_label;
            if_update_prpty_label = f;
            return (tmp);
        }

    // Compute the label object database size and return the number of
    // lines in the string (always 1 or larger), or return 0 to
    // indicate use of default sizing.  Applies to all modes.
    //
    int ifDefaultLabelSize(const char *text, DisplayMode mode, double *width,
        double *height)
        {
            if (if_default_label_size)
                return ((*if_default_label_size)(text, mode, width, height));
            return (0);
        }

    CDif_DefaultLabelSize RegisterIfDefaultLabelSize(CDif_DefaultLabelSize f)
        {
            CDif_DefaultLabelSize tmp = if_default_label_size;
            if_default_label_size = f;
            return (tmp);
        }


    // Conversions and logging.
    // These process messages and events generated during archive
    // file reading/writing and translation.
    //---------------------------------------------------------

    // Process an informational message.  These messages would
    // generally go to the screen.
    //
    void ifInfoMessage(INFOmsgType code, const char *string, ...)
        {
            if (if_info_message) {
                va_list ap;
                va_start(ap, string);
                (*if_info_message)(code, string, ap);
                va_end(ap);
            }
        }

    CDif_InfoMessage RegisterIfInfoMessage(CDif_InfoMessage f)
        {
            CDif_InfoMessage tmp = if_info_message;
            if_info_message = f;
            return (tmp);
        }

    // Process a message emitted during file i/o.  The message is
    // formatted and printed to the file pointer returned from
    // InitCvLog.
    //
    void ifPrintCvLog(OUTmsgType msgtype, const char *fmt, ...)
        {
            if (if_print_cv_log) {
                va_list ap;
                va_start(ap, fmt);
                (*if_print_cv_log)(msgtype, fmt, ap);
                va_end(ap);
            }
        }

    CDif_PrintCvLog RegisterIfPrintCvLog(CDif_PrintCvLog f)
        {
            CDif_PrintCvLog tmp = if_print_cv_log;
            if_print_cv_log = f;
            return (tmp);
        }


    // Database change recording.
    // These are called when something is created or modified.
    //---------------------------------------------------------

    // If create is true, record the hypertext entry in the history
    // list.  If create is false, Remove all references to ent in the
    // history list.  Unlike objects and properties, hypertext
    // entries are owned by CD and should not be freed by the
    // application.
    //
    void ifRecordHYent(hyEnt *ent, bool create)
        {
            if (if_record_hyent)
                (*if_record_hyent)(ent, create);
        }

    CDif_RecordHYent RegisterIfRecordHYent(CDif_RecordHYent f)
        {
            CDif_RecordHYent tmp = if_record_hyent;
            if_record_hyent = f;
            return (tmp);
        }

    // An object (oldesc) is being deleted, and being replaced by
    // newdesc.  Either olddesc or newdesc can be null.
    //
    // This is called when the changes are to be recorded in a
    // history (undo) list.  It is up to the application to actually
    // delete old objects.
    //
    void ifRecordObjectChange(CDs *sdesc, CDo *olddesc, CDo *newdesc)
        {
            if (if_record_object_change)
                (*if_record_object_change)(sdesc, olddesc, newdesc);
        }

    CDif_RecordObjectChange RegisterIfRecordObjectChange(
            CDif_RecordObjectChange f)
        {
            CDif_RecordObjectChange tmp = if_record_object_change;
            if_record_object_change = f;
            return (tmp);
        }

    // A property (oldp) is being deleted, and being replaced by
    // newp.  Either oldp or newp can be null.  If odesc is not null,
    // the properties apply to that object, otherwise they apply to
    // sdesc.
    //
    // This is called when the changes are to be recorded in a
    // history (undo) list.  It is up to the application to actually
    // delete old properties.
    //
    void ifRecordPrptyChange(CDs *sdesc, CDo *odesc, CDp *oldp, CDp *newp)
        {
            if (if_record_prpty_change)
                (*if_record_prpty_change)(sdesc, odesc, oldp, newp);
        }

    CDif_RecordPrptyChange RegisterIfRecordPrptyChange(CDif_RecordPrptyChange f)
        {
            CDif_RecordPrptyChange tmp = if_record_prpty_change;
            if_record_prpty_change = f;
            return (tmp);
        }

    // The cell was just modified.
    //
    void ifCellModified(const CDs *sdesc)
        {
            if (if_cell_modified)
                (*if_cell_modified)(sdesc);
        }

    CDif_CellModified RegisterIfCellModified(CDif_CellModified f)
        {
            CDif_CellModified tmp = if_cell_modified;
            if_cell_modified = f;
            return (tmp);
        }

    // The object odesc is being added to sdesc.
    //
    void ifObjectInserted(CDs *sdesc, CDo *odesc)
        {
            if (if_object_inserted)
                (*if_object_inserted)(sdesc, odesc);
        }

    CDif_ObjectInserted RegisterIfObjectInserted(CDif_ObjectInserted f)
        {
            CDif_ObjectInserted tmp = if_object_inserted;
            if_object_inserted = f;
            return (tmp);
        }

    // Layer creation and modification.
    // Layer descriptors are managed by CD.  The following are called
    // when changes are made.
    //---------------------------------------------------------

    // A new layer was just created.  The application can set the
    // ld->user_data field to app-specific data for this layer.
    //
    void ifNewLayer(CDl *ld)
        {
            if (if_new_layer)
                (*if_new_layer)(ld);
        }

    CDif_NewLayer RegisterIfNewLayer(CDif_NewLayer f)
        {
            CDif_NewLayer tmp = if_new_layer;
            if_new_layer = f;
            return (tmp);
        }

    // The layer sequence for the passed mode was just changed.
    //
    void ifLayerTableChange(DisplayMode mode)
        {
            if (if_layer_table_change)
                (*if_layer_table_change)(mode);
        }

    CDif_LayerTableChange RegisterIfLayerTableChange(CDif_LayerTableChange f)
        {
            CDif_LayerTableChange tmp = if_layer_table_change;
            if_layer_table_change = f;
            return (tmp);
        }

    // The list of unused layers, i.e., those that have been removed
    // from the layer table, has changed.  The new list for the
    // display mode is passed.
    //
    void ifUnusedLayerListChange(CDll *list, DisplayMode mode)
        {
            if (if_unused_layer_list_change)
                (*if_unused_layer_list_change)(list, mode);
        }

    CDif_UnusedLayerListChange RegisterIfUnusedLayerListChange(
            CDif_UnusedLayerListChange f)
        {
            CDif_UnusedLayerListChange tmp = if_unused_layer_list_change;
            if_unused_layer_list_change = f;
            return (tmp);
        }


    // Destroy callbacks.
    // These allow the application to purge any references to an
    // object being destroyed.
    //---------------------------------------------------------

    // The symbol is being destroyed.
    //
    void ifInvalidateSymbol(CDcellNameStr *name)
        {
            if (if_invalidate_symbol)
                (*if_invalidate_symbol)(name);
        }

    CDif_InvalidateSymbol RegisterIfInvalidateSymbol(CDif_InvalidateSymbol f)
        {
            CDif_InvalidateSymbol tmp = if_invalidate_symbol;
            if_invalidate_symbol = f;
            return (tmp);
        }

    // The cell is being destroyed.
    //
    void ifInvalidateCell(CDs *sdesc)
        {
            if (if_invalidate_cell)
                (*if_invalidate_cell)(sdesc);
        }

    CDif_InvalidateCell RegisterIfInvalidateCell(CDif_InvalidateCell f)
        {
            CDif_InvalidateCell tmp = if_invalidate_cell;
            if_invalidate_cell = f;
            return (tmp);
        }

    // The object odesc is being removed.  If save is true, the
    // object is being unlinked but not destroyed.
    //
    void ifInvalidateObject(CDs *sdesc, CDo *odesc, bool save)
        {
            if (if_invalidate_object)
                (*if_invalidate_object)(sdesc, odesc, save);
        }

    CDif_InvalidateObject RegisterIfInvalidateObject(CDif_InvalidateObject f)
        {
            CDif_InvalidateObject tmp = if_invalidate_object;
            if_invalidate_object = f;
            return (tmp);
        }

    // Everything on layer ld is being deleted, within sdesc if sdesc
    // is not null, or globally if sdesc is null.
    //
    void ifInvalidateLayerContent(CDs *sdesc, CDl *ld)
        {
            if (if_invalidate_layer_content)
                (*if_invalidate_layer_content)(sdesc, ld);
        }

    CDif_InvalidateLayerContent RegisterIfInvalidateLayerContent(
            CDif_InvalidateLayerContent f)
        {
            CDif_InvalidateLayerContent tmp = if_invalidate_layer_content;
            if_invalidate_layer_content = f;
            return (tmp);
        }

    // The layer descriptor ld is being destroyed.  This should
    // destroy the user information (if any) in ld->user_data.
    //
    void ifInvalidateLayer(CDl *ld)
        {
            if (if_invalidate_layer)
                (*if_invalidate_layer)(ld);
        }

    CDif_InvalidateLayer RegisterIfInvalidateLayer(CDif_InvalidateLayer f)
        {
            CDif_InvalidateLayer tmp = if_invalidate_layer;
            if_invalidate_layer= f;
            return (tmp);
        }

    // The terminal is being destroyed.
    //
    void ifInvalidateTerminal(CDterm *t)
        {
            if (if_invalidate_terminal)
                (*if_invalidate_terminal)(t);
        }

    CDif_InvalidateTerminal RegisterIfInvalidateTerminal(
            CDif_InvalidateTerminal f)
        {
            CDif_InvalidateTerminal tmp = if_invalidate_terminal;
            if_invalidate_terminal = f;
            return (tmp);
        }


    // Misc.
    // These are miscellaneous callbacks and info functions.
    //---------------------------------------------------------

    // Check for interrupt, return true if the current operation
    // should be aborted.  The argument is a message string for the
    // confirmation pop-up.
    //
    bool ifCheckInterrupt(const char *msg = 0)
        {
            if (if_check_interrupt)
                return ((*if_check_interrupt)(msg));
            return (false);
        }

    CDif_CheckInterrupt RegisterIfCheckInterrupt(CDif_CheckInterrupt f)
        {
            CDif_CheckInterrupt tmp = if_check_interrupt;
            if_check_interrupt= f;
            return (tmp);
        }

    // Set a flag in the application to bypass the abort confirmation
    // prompt on interrupt.  Interrupted operations will always be
    // aborted.
    //
    void ifNoConfirmAbort(bool b)
        {
            if (if_no_confirm_abort)
                (*if_no_confirm_abort)(b);
        }

    CDif_NoConfirmAbort RegisterIfNoConfirmAbort(CDif_NoConfirmAbort f)
        {
            CDif_NoConfirmAbort tmp = if_no_confirm_abort;
            if_no_confirm_abort = f;
            return (tmp);
        }

    // Return a static string which will be printed near the top of
    // certain created files.  Can contain the application version,
    // date, etc.
    //
    const char *ifIdString()
        {
            if (if_id_string)
                return ((*if_id_string)());
            return ("");
        }

    CDif_IdString RegisterIfIdString(CDif_IdString f)
        {
            CDif_IdString tmp = if_id_string;
            if_id_string= f;
            return (tmp);
        }

    // Called when a variable has been set, cleared, or modified.
    //
    void ifVariableChange()
        {
            if (if_variable_change)
                (*if_variable_change)();
        }

    CDif_VariableChange RegisterIfVariableChange(CDif_VariableChange f)
        {
            CDif_VariableChange tmp = if_variable_change;
            if_variable_change= f;
            return (tmp);
        }

    // Called when the CHD database has changed, i.e., when a CHD is
    // added or removed.
    //
    void ifChdDbChange()
        {
            if (if_chd_db_change)
                (*if_chd_db_change)();
        }

    CDif_ChdDbChange RegisterIfChdDbChange(CDif_ChdDbChange f)
        {
            CDif_ChdDbChange tmp = if_chd_db_change;
            if_chd_db_change= f;
            return (tmp);
        }

    // Called when the CGD database has changed, i.e., when a CGD is
    // added or removed.
    //
    void ifCgdDbChange()
        {
            if (if_cgd_db_change)
                (*if_cgd_db_change)();
        }

    CDif_CgdDbChange RegisterIfCgdDbChange(CDif_CgdDbChange f)
        {
            CDif_CgdDbChange tmp = if_cgd_db_change;
            if_cgd_db_change= f;
            return (tmp);
        }

protected:
    CDif_NodeName if_node_name;
    CDif_UpdateNodes if_update_nodes;
    CDif_UpdateNameLabel if_update_name_label;
    CDif_UpdateMutLabel if_update_mut_label;
    CDif_LabelInstance if_label_instance;
    CDif_UpdatePrptyLabel if_update_prpty_label;
    CDif_DefaultLabelSize if_default_label_size;

    CDif_InfoMessage if_info_message;
    CDif_PrintCvLog if_print_cv_log;

    CDif_RecordHYent if_record_hyent;
    CDif_RecordObjectChange if_record_object_change;
    CDif_RecordPrptyChange if_record_prpty_change;
    CDif_CellModified if_cell_modified;
    CDif_ObjectInserted if_object_inserted;

    CDif_NewLayer if_new_layer;
    CDif_LayerTableChange if_layer_table_change;
    CDif_UnusedLayerListChange if_unused_layer_list_change;

    CDif_InvalidateSymbol if_invalidate_symbol;
    CDif_InvalidateCell if_invalidate_cell;
    CDif_InvalidateObject if_invalidate_object;
    CDif_InvalidateLayerContent if_invalidate_layer_content;
    CDif_InvalidateLayer if_invalidate_layer;
    CDif_InvalidateTerminal if_invalidate_terminal;

    CDif_CheckInterrupt if_check_interrupt;
    CDif_NoConfirmAbort if_no_confirm_abort;
    CDif_IdString if_id_string;
    CDif_VariableChange if_variable_change;
    CDif_ChdDbChange if_chd_db_change;
    CDif_CgdDbChange if_cgd_db_change;
};

#endif

