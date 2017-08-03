
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

#ifndef FIO_IF_H
#define FIO_IF_H

#include "if_common.h"
#include <stdarg.h>


//
// Application interface for FIO, which the base class for sFIO.  The
// application should call the Register functions to register callbacks
// to participate in the interface.
//

struct mitem_t;

// Callback prototypes.
typedef void(*FIOif_UpdateNodes)(const CDs*);

typedef void(*FIOif_SetWorking)(bool);
typedef void(*FIOif_InfoMessage)(INFOmsgType, const char*, va_list);
typedef FILE*(*FIOif_InitCvLog)(const char*);
typedef void(*FIOif_PrintCvLog)(OUTmsgType, const char*, va_list);
typedef void(*FIOif_ShowCvLog)(FILE*, OItype, const char*);
typedef void(*FIOif_MergeControl)(mitem_t*);
typedef void(*FIOif_MergeControlClear)();
typedef void(*FIOif_AmbiguityCallback)(stringlist*, const char*, const char*,
    void(*)(const char*, void*), void*);
typedef bool(*FIOif_OpenSubMaster)(CDp*, CDs**);
typedef bool(*FIOif_ReopenSubMaster)(CDs*);
typedef bool(*FIOif_OpenOA)(const char*, const char*, CDcbin*);

typedef DisplayMode(*FIOif_CurrentDisplayMode)();
typedef const char*(*FIOif_DeviceLibName)();
typedef void(*FIOif_LibraryChange)();
typedef void(*FIOif_PathChange)();


struct FIOif
{
    FIOif()
        {
            if_update_nodes = 0;

            if_set_working = 0;
            if_info_message = 0;
            if_init_cv_log = 0;
            if_print_cv_log = 0;
            if_show_cv_log = 0;
            if_merge_control = 0;
            if_merge_control_clear = 0;
            if_ambiguity_callback = 0;
            if_open_submaster = 0;
            if_reopen_submaster = 0;
            if_open_oa = 0;

            if_current_display_mode = 0;
            if_device_lib_name = 0;
            if_library_change = 0;
            if_path_change = 0;
        }

    // Labels, etc., mostly for electrical mode.
    // Labels are shown near device symbols in electrical mode,
    // showing property values.
    //---------------------------------------------------------

    // Update the P_NODMAP property which contains a mapping between
    // internal node numbers and assigned node names.  Applies to
    // electrical mode.
    //
    void ifUpdateNodes(const CDs *sdesc)
        {
            if (if_update_nodes)
                (*if_update_nodes)(sdesc);
        }

    FIOif_UpdateNodes RegisterIfUpdateNodes(FIOif_UpdateNodes f)
        {
            FIOif_UpdateNodes tmp = if_update_nodes;
            if_update_nodes = f;
            return (tmp);
        }


    // Conversions and logging.
    // These process messages and events generated during archive
    // file reading/writing and translation.
    //---------------------------------------------------------

    // Signal the application that a long operation is beginning or ending.
    // The application can indicated this by, e.g., changing the cursor.
    //
    void ifSetWorking(bool b)
        {
            if (if_set_working)
                (*if_set_working)(b);
        }

    FIOif_SetWorking RegisterIfSetWorking(FIOif_SetWorking f)
        {
            FIOif_SetWorking tmp = if_set_working;
            if_set_working = f;
            return (tmp);
        }

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

    FIOif_InfoMessage RegisterIfInfoMessage(FIOif_InfoMessage f)
        {
            FIOif_InfoMessage tmp = if_info_message;
            if_info_message = f;
            return (tmp);
        }

    // Called before i/o operation starts, set up the log file and
    // perform other initialization.  Returns a file pointer to the
    // log file (named from fname).  If null is returned, no log will
    // be kept.
    //
    FILE *ifInitCvLog(const char *fname)
        {
            if (if_init_cv_log)
                return ((*if_init_cv_log)(fname));
            return (0);
        }

    FIOif_InitCvLog RegisterIfInitCvLog(FIOif_InitCvLog f)
        {
            FIOif_InitCvLog tmp = if_init_cv_log;
            if_init_cv_log = f;
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

    FIOif_PrintCvLog RegisterIfPrintCvLog(FIOif_PrintCvLog f)
        {
            FIOif_PrintCvLog tmp = if_print_cv_log;
            if_print_cv_log = f;
            return (tmp);
        }

    // Close the file pointer to the log file.  The first argument is
    // the return from InitCvLog, and if nonzero should be closed in
    // this function.  The second arg is the return from the i/o
    // operation, and the third argument is the file name passed to
    // InitCvLog.  This is called when the operation is complete.
    //
    void ifShowCvLog(FILE *fp, OItype oiret, const char *fname)
        {
            if (if_show_cv_log)
                (*if_show_cv_log)(fp, oiret, fname);
        }

    FIOif_ShowCvLog RegisterIfShowCvLog(FIOif_ShowCvLog f)
        {
            FIOif_ShowCvLog tmp = if_show_cv_log;
            if_show_cv_log = f;
            return (tmp);
        }

    // Handle database name collision.  This is intended to start a
    // process that will pop-up a dialog when a cell is being read
    // whose name is already in the database.  This is called for each
    // cell name clash.
    //
    void ifMergeControl(mitem_t *mi)
        {
            if (if_merge_control)
                (*if_merge_control)(mi);
        }

    FIOif_MergeControl RegisterIfMergeControl(FIOif_MergeControl f)
        {
            FIOif_MergeControl tmp = if_merge_control;
            if_merge_control = f;
            return (tmp);
        }

    // Terminate handling symbol name collisions.
    //
    void ifMergeControlClear()
        {
            if (if_merge_control_clear)
                (*if_merge_control_clear)();
        }

    FIOif_MergeControlClear RegisterIfMergeControlClear(
            FIOif_MergeControlClear f)
        {
            FIOif_MergeControlClear tmp = if_merge_control_clear;
            if_merge_control_clear = f;
            return (tmp);
        }

    // This initiates a pop-up which allows the user to click-select
    // from among the list of items.  It is used to select cells from
    // a library listing, and to select the top-level cell of
    // interest when opening an archive file with multiple top-level
    // cells.
    //
    // Arguments:
    //   sl         string list of entries.
    //   msg        list title string.
    //   fname      name of originating file.
    //   callback   function called with entry and arg when user
    //              clicks.
    //   arg        argument passed to callback.
    //
    // Archive opening functions that require this callback will
    // return an "ambiguous" error.  The open is completed when
    // callback is called.
    //
    void ifAmbiguityCallback(stringlist *sl, const char *msg,
            const char *fname, void (*callback)(const char*, void*),
            void *arg)
        {
            if (if_ambiguity_callback)
                (*if_ambiguity_callback)(sl, msg, fname, callback, arg);
        }

    FIOif_AmbiguityCallback RegisterIfAmbiguityCallback(
            FIOif_AmbiguityCallback f)
        {
            FIOif_AmbiguityCallback tmp = if_ambiguity_callback;
            if_ambiguity_callback = f;
            return (tmp);
        }

    // This generates PCell sub-masters and does some related
    // checking.
    //
    bool ifOpenSubMaster(CDp *prp, CDs **psd)
        {
            if (if_open_submaster)
                return (*if_open_submaster)(prp, psd);
            return (true);
        }

    FIOif_OpenSubMaster RegisterIfOpenSubMaster(FIOif_OpenSubMaster f)
        {
            FIOif_OpenSubMaster tmp = if_open_submaster;
            if_open_submaster = f;
            return (tmp);
        }

    // This generates PCell sub-masters and does some related
    // checking.
    //
    bool ifReopenSubMaster(CDs *sd)
        {
            if (if_reopen_submaster)
                return (*if_reopen_submaster)(sd);
            return (true);
        }

    FIOif_ReopenSubMaster RegisterIfReopenSubMaster(FIOif_ReopenSubMaster f)
        {
            FIOif_ReopenSubMaster tmp = if_reopen_submaster;
            if_reopen_submaster = f;
            return (tmp);
        }

    // Attempt to open cname through OpenAccess.  If cbin is null,
    // return true if the libname/cellname exists.  Otherwise, if
    // found, open the cell and return pointers in cbin.  If
    // successful return true.  The libname can be null, signifying a
    // wildcard.
    //
    bool ifOpenOA(const char *lname, const char *cname, CDcbin *cbin)
        {
            if (if_open_oa)
                return (*if_open_oa)(lname, cname, cbin);
            return (false);
        }

    FIOif_OpenOA RegisterIfOpenOA(FIOif_OpenOA f)
        {
            FIOif_OpenOA tmp = if_open_oa;
            if_open_oa = f;
            return (tmp);
        }


    // Misc.
    // These are miscellaneous callbacks and info functions.
    //---------------------------------------------------------

    // Return the current display mode of the main window,
    // Physical or Electrical.
    //
    DisplayMode ifCurrentDisplayMode()
        {
            if (if_current_display_mode)
                return ((*if_current_display_mode)());
            return (Physical);
        }

    FIOif_CurrentDisplayMode RegisterIfCurrentDisplayMode(
            FIOif_CurrentDisplayMode f)
        {
            FIOif_CurrentDisplayMode tmp = if_current_display_mode;
            if_current_display_mode= f;
            return (tmp);
        }

    // Return the name used for the device library file, which
    // defaults to "device.lib" if this function returns null.
    //
    const char *ifDeviceLibName()
        {
            if (if_device_lib_name)
                return ((*if_device_lib_name)());
            return (0);
        }

    FIOif_DeviceLibName RegisterIfDeviceLibName(FIOif_DeviceLibName f)
        {
            FIOif_DeviceLibName tmp = if_device_lib_name;
            if_device_lib_name= f;
            return (tmp);
        }

    // The internal list of open libraries changed.
    //
    void ifLibraryChange()
        {
            if (if_library_change)
                (*if_library_change)();
        }

    FIOif_LibraryChange RegisterIfLibraryChange(FIOif_LibraryChange f)
        {
            FIOif_LibraryChange tmp = if_library_change;
            if_library_change= f;
            return (tmp);
        }

    // The search path has changed.
    void ifPathChange()
        {
            if (if_path_change)
                (*if_path_change)();
        }

    FIOif_PathChange RegisterIfPathChange(FIOif_PathChange f)
        {
            FIOif_PathChange tmp = if_path_change;
            if_path_change= f;
            return (tmp);
        }

protected:
    FIOif_UpdateNodes if_update_nodes;

    FIOif_SetWorking if_set_working;
    FIOif_InfoMessage if_info_message;
    FIOif_InitCvLog if_init_cv_log;
    FIOif_PrintCvLog if_print_cv_log;
    FIOif_ShowCvLog if_show_cv_log;
    FIOif_MergeControl if_merge_control;
    FIOif_MergeControlClear if_merge_control_clear;
    FIOif_AmbiguityCallback if_ambiguity_callback;
    FIOif_OpenSubMaster if_open_submaster;
    FIOif_ReopenSubMaster if_reopen_submaster;
    FIOif_OpenOA if_open_oa;

    FIOif_CurrentDisplayMode if_current_display_mode;
    FIOif_DeviceLibName if_device_lib_name;
    FIOif_LibraryChange if_library_change;
    FIOif_PathChange if_path_change;
};

#endif

