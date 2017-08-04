
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

#ifndef GEO_IF_H
#define GEO_IF_H

#include "if_common.h"
#include <stdarg.h>


//
// Application interface for GEO, which the base class for cGEO.  The
// application should call the Register functions to register callbacks
// to participate in the interface.
//

struct CDs;
struct CDo;
struct Zoid;
struct Zlist;

// This is the bloat mode that uses DRC sizing.
#define DRC_BLOAT 3

// Return values, for interrupt sensitive functions.
// XIok *must* be 0 for setjmp/longjmp.
enum XIrt { XIok = 0, XIbad, XIintr };

// Callback prototypes.
typedef void(*GEOif_InfoMessage)(INFOmsgType, const char*, va_list);

typedef void(*GEOif_RecordObjectChange)(CDs*, CDo*, CDo*);

typedef void(*GEOif_SaveZlist)(const Zlist*, const char*);
typedef void(*GEOif_ShowAndPause)(const char*);
typedef void(*GEOif_ClearLayer)(const char*);

typedef void(*GEOif_DrawZoid)(const Zoid*);
typedef Zlist*(*GEOif_BloatList)(const Zlist*, int, bool, XIrt*);
typedef Zlist*(*GEOif_BloatObj)(const CDo*, int, XIrt*);


// Interface class for callback functions.  GEO sub-classes this interface
// to provide callbacks to the application.  The application can call the
// registration functions to implement the interface, overriding the
// default stubs.  The return from the registration functions is the
// previous registration, if any.
//
struct GEOif
{
    GEOif()
        {
            if_info_message = 0;

            if_record_object_change = 0;

            if_save_zlist = 0;
            if_show_and_pause = 0;
            if_clear_layer = 0;

            if_draw_zoid = 0;
            if_bloat_list = 0;
            if_bloat_obj = 0;
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

    GEOif_InfoMessage RegisterIfInfoMessage(GEOif_InfoMessage f)
        {
            GEOif_InfoMessage tmp = if_info_message;
            if_info_message = f;
            return (tmp);
        }


    // Database change recording.
    // These are called when something is created or modified.
    //---------------------------------------------------------

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

    GEOif_RecordObjectChange RegisterIfRecordObjectChange(
            GEOif_RecordObjectChange f)
        {
            GEOif_RecordObjectChange tmp = if_record_object_change;
            if_record_object_change = f;
            return (tmp);
        }


    // Visualization for debugging.
    // These are used to display trapezoid lists for debugging.
    //---------------------------------------------------------

    // Add the Zlist to the current display, using a layer named
    // in lname.
    //
    void ifSaveZlist(const Zlist *zl, const char *lname)
        {
            if (if_save_zlist)
                (*if_save_zlist)(zl, lname);
        }

    GEOif_SaveZlist RegisterIfSaveZlist(GEOif_SaveZlist f)
        {
            GEOif_SaveZlist tmp = if_save_zlist;
            if_save_zlist = f;
            return (tmp);
        }

    // Update the display, and prompt the user.  The user must respond
    // for the program to continue.
    //
    void ifShowAndPause(const char *prompt)
        {
            if (if_show_and_pause)
                (*if_show_and_pause)(prompt);
        }

    GEOif_ShowAndPause RegisterIfShowAndPause(GEOif_ShowAndPause f)
        {
            GEOif_ShowAndPause tmp = if_show_and_pause;
            if_show_and_pause = f;
            return (tmp);
        }

    // Clear all objects on the named layer from the display database.
    //
    void ifClearLayer(const char *lname)
        {
            if (if_clear_layer)
                (*if_clear_layer)(lname);
        }

    GEOif_ClearLayer RegisterIfClearLayer(GEOif_ClearLayer f)
        {
            GEOif_ClearLayer tmp = if_clear_layer;
            if_clear_layer = f;
            return (tmp);
        }


    // Misc.
    // These are miscellaneous callbacks and info functions.
    //---------------------------------------------------------

    // Render the trapezoid in a drawing window using highlighting.
    //
    void ifDrawZoid(const Zoid *z)
        {
            if (if_draw_zoid)
                (*if_draw_zoid)(z);
        }

    GEOif_DrawZoid RegisterIfDrawZoid(GEOif_DrawZoid f)
        {
            GEOif_DrawZoid tmp = if_draw_zoid;
            if_draw_zoid = f;
            return (tmp);
        }

    // For trapezoid list bloating:  Zlist::blost(delta, DRC_BLOAT)
    // This implements an external bloating algorithm.
    //
    Zlist *ifBloatList(const Zlist *zl, int delta, bool edgeonly, XIrt *retp)
        {
            if (if_bloat_list)
                return ((*if_bloat_list)(zl, delta, edgeonly, retp));
            return (0);
        }

    GEOif_BloatList RegisterIfBloatList(GEOif_BloatList f)
        {
            GEOif_BloatList tmp = if_bloat_list;
            if_bloat_list = f;
            return (tmp);
        }

    // As above, but bloat an object.
    //
    Zlist *ifBloatObj(const CDo *od, int delta, XIrt *retp)
        {
            if (if_bloat_obj)
                return ((*if_bloat_obj)(od, delta, retp));
            return (0);
        }

    GEOif_BloatObj RegisterIfBloatObj(GEOif_BloatObj f)
        {
            GEOif_BloatObj tmp = if_bloat_obj;
            if_bloat_obj = f;
            return (tmp);
        }

protected:
    GEOif_InfoMessage if_info_message;

    GEOif_RecordObjectChange if_record_object_change;

    GEOif_SaveZlist if_save_zlist;
    GEOif_ShowAndPause if_show_and_pause;
    GEOif_ClearLayer if_clear_layer;

    GEOif_DrawZoid if_draw_zoid;
    GEOif_BloatList if_bloat_list;
    GEOif_BloatObj if_bloat_obj;
};

#endif

