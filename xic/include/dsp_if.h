
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

#ifndef DSP_IF_H
#define DSP_IF_H

//
// Interface class for the display module.  This isolates application-
// specific calls.
//

struct WindowDesc;
struct CDc;
struct CDp;
struct stringlist;

// Callback prototypes.
typedef void (*DSPif_show_message)(const char*, bool);
typedef void (*DSPif_notify_interrupted)();
typedef void (*DSPif_notify_display_done)();
typedef void (*DSPif_notify_stack_overflow)();
typedef void (*DSPif_window_show_highlighting)(WindowDesc*);
typedef void (*DSPif_window_show_blinking)(WindowDesc*);
typedef void (*DSPif_window_view_change)(WindowDesc*);
typedef void (*DSPif_window_view_saved)(WindowDesc*, int);
typedef void (*DSPif_window_clear_views)(WindowDesc*);
typedef void (*DSPif_window_mode_change)(WindowDesc*);
typedef void (*DSPif_window_get_title_strings)(const char**, const char**);
typedef void (*DSPif_window_destroy)(WindowDesc*);
typedef const CDc *(*DSPif_context_cell)();
typedef bool (*DSPif_is_subct_cmd_active)();
typedef void (*DSPif_comment_dump)(FILE*, const char*, stringlist*);
typedef bool (*DSPif_check_ext_display)(WindowDesc*);
typedef bool (*DSPif_ext_display_cell)(WindowDesc*, const CDs*, const CDl*,
    int*);
typedef bool (*DSPif_check_display)(WindowDesc*, const CDs*, const CDc*);
typedef bool (*DSPif_check_similar)(const WindowDesc*, const WindowDesc*);


struct cDisplayIf
{
    cDisplayIf()
        {
            if_show_message = 0;
            if_notify_interrupted = 0;
            if_notify_display_done = 0;
            if_notify_stack_overflow = 0;
            if_window_show_highlighting = 0;
            if_window_show_blinking = 0;
            if_window_view_change = 0;
            if_window_view_saved = 0;
            if_window_clear_views = 0;
            if_window_mode_change = 0;
            if_window_get_title_strings = 0;
            if_window_destroy = 0;
            if_context_cell = 0;
            if_is_subct_cmd_active = 0;
            if_comment_dump = 0;
            if_check_ext_display = 0;
            if_ext_display_cell = 0;
            if_check_display = 0;
            if_check_similar = 0;
        }

    // Display a text message in the application.  If the second
    // argument is false, print in a status line, otherwise print in
    // a pop-up error window.
    //
    void show_message(const char *msg, bool popup = false)
        {
            if (if_show_message)
                (*if_show_message)(msg, popup);
        }

    DSPif_show_message Register_show_message(DSPif_show_message f)
        {
            DSPif_show_message tmp = if_show_message;
            if_show_message = f;
            return (tmp);
        }

    // Acknowledge an interrupt, such as by printing an "interrupted"
    // message.
    //
    void notify_interrupted()
        {
            if (if_notify_interrupted)
                (*if_notify_interrupted)();
        }

    DSPif_notify_interrupted Register_notify_interrupted(
            DSPif_notify_interrupted f)
        {
            DSPif_notify_interrupted tmp = if_notify_interrupted;
            if_notify_interrupted = f;
            return (tmp);
        }

    // Called after a redisplay idle completes.
    //
    void notify_display_done()
        {
            if (if_notify_display_done)
                (*if_notify_display_done)();
        }

    DSPif_notify_display_done Register_notify_display_done(
            DSPif_notify_display_done f)
        {
            DSPif_notify_display_done tmp = if_notify_display_done;
            if_notify_display_done = f;
            return (tmp);
        }

    // Notify of a transform stack overflow.  This is not a fatal
    // error, but may indicate a recursive hierarchy.
    //
    void notify_stack_overflow()
        {
            if (if_notify_stack_overflow)
                (*if_notify_stack_overflow)();
        }

    DSPif_notify_stack_overflow Register_notify_stack_overflow(
            DSPif_notify_stack_overflow f)
        {
            DSPif_notify_stack_overflow tmp = if_notify_stack_overflow;
            if_notify_stack_overflow = f;
            return (tmp);
        }

    // Show application-specific highlighting, called after a display
    // update.
    //
    void window_show_highlighting(WindowDesc *wdesc)
        {
            if (if_window_show_highlighting)
                (*if_window_show_highlighting)(wdesc);
        }

    DSPif_window_show_highlighting Register_window_show_highlighting(
            DSPif_window_show_highlighting f)
        {
            DSPif_window_show_highlighting tmp = if_window_show_highlighting;
            if_window_show_highlighting = f;
            return (tmp);
        }

    // Show blinking highlighting.  This is called by a timer.  The
    // function should reset appropriate pixel colors and redraw
    // blinking features.
    //
    void window_show_blinking(WindowDesc *wdesc)
        {
            if (if_window_show_blinking)
                (*if_window_show_blinking)(wdesc);
        }

    DSPif_window_show_blinking Register_window_show_blinking(
            DSPif_window_show_blinking f)
        {
            DSPif_window_show_blinking tmp = if_window_show_blinking;
            if_window_show_blinking = f;
            return (tmp);
        }

    // Called when the display area changes.
    //
    void window_view_change(WindowDesc *wdesc)
        {
            if (if_window_view_change)
                (*if_window_view_change)(wdesc);
        }

    DSPif_window_view_change Register_window_view_change(
            DSPif_window_view_change f)
        {
            DSPif_window_view_change tmp = if_window_view_change;
            if_window_view_change = f;
            return (tmp);
        }

    // Called when a view is being saved.  The indx is the value
    // returned from wStack::add_view().
    //
    void window_view_saved(WindowDesc *wdesc, int indx)
        {
            if (if_window_view_saved)
                (*if_window_view_saved)(wdesc, indx);
        }

    DSPif_window_view_saved Register_window_view_saved(
            DSPif_window_view_saved f)
        {
            DSPif_window_view_saved tmp = if_window_view_saved;
            if_window_view_saved = f;
            return (tmp);
        }

    // Called when the list of named views is cleared.
    //
    void window_clear_views(WindowDesc *wdesc)
        {
            if (if_window_clear_views)
                (*if_window_clear_views)(wdesc);
        }

    DSPif_window_clear_views Register_window_clear_views(
            DSPif_window_clear_views f)
        {
            DSPif_window_clear_views tmp = if_window_clear_views;
            if_window_clear_views = f;
            return (tmp);
        }

    // Called when the window display mode changes, new mode has been
    // set.
    //
    void window_mode_change(WindowDesc *wdesc)
        {
            if (if_window_mode_change)
                (*if_window_mode_change)(wdesc);
        }

    DSPif_window_mode_change Register_window_mode_change(
            DSPif_window_mode_change f)
        {
            DSPif_window_mode_change tmp = if_window_mode_change;
            if_window_mode_change = f;
            return (tmp);
        }

    // The application can set the title of a newly-created window.
    //
    void window_get_title_strings(const char **p1, const char **p2)
        {
            if (if_window_get_title_strings)
                (*if_window_get_title_strings)(p1, p2);
        }

    DSPif_window_get_title_strings Register_window_get_title_strings(
            DSPif_window_get_title_strings f)
        {
            DSPif_window_get_title_strings tmp = if_window_get_title_strings;
            if_window_get_title_strings = f;
            return (tmp);
        }

    // Called when the window is being destroyed.
    //
    void window_destroy(WindowDesc *wdesc)
        {
            if (if_window_destroy)
                (*if_window_destroy)(wdesc);
        }

    DSPif_window_destroy Register_window_destroy(DSPif_window_destroy f)
        {
            DSPif_window_destroy tmp = if_window_destroy;
            if_window_destroy = f;
            return (tmp);
        }

    // If the current cell is not the same as the top cell, the
    // current cell is expected to be in the hierarchy of the top
    // cell.  This function should return the instance of the current
    // cell puthed into.  This prevents drawing the current cell when
    // the context is being displayed.
    //
    const CDc *context_cell()
        {
            if (if_context_cell)
                return ((*if_context_cell)());
            return (0);
        }

    DSPif_context_cell Register_context_cell(DSPif_context_cell f)
        {
            DSPif_context_cell tmp = if_context_cell;
            if_context_cell = f;
            return (tmp);
        }

    // Hack for Xic, return true is the subct command mode is active.
    //
    bool is_subct_cmd_active()
        {
            if (if_is_subct_cmd_active)
                return ((*if_is_subct_cmd_active)());
            return (false);
        }

    DSPif_is_subct_cmd_active Register_is_subct_cmd_active(
            DSPif_is_subct_cmd_active f)
        {
            DSPif_is_subct_cmd_active tmp = if_is_subct_cmd_active;
            if_is_subct_cmd_active = f;
            return (tmp);
        }

    // Hack for Xic, add saved comment lines when generating output for
    // a tech file.
    //
    void comment_dump(FILE *techfp, const char *name, stringlist *alii)
        {
            if (if_comment_dump)
                (*if_comment_dump)(techfp, name, alii);
        }

    DSPif_comment_dump Register_comment_dump(DSPif_comment_dump f)
        {
            DSPif_comment_dump tmp = if_comment_dump;
            if_comment_dump = f;
            return (tmp);
        }

    bool check_ext_display(WindowDesc *wdesc)
        {
            if (if_check_ext_display)
                return ((*if_check_ext_display)(wdesc));
            return (false);
        }

    DSPif_check_ext_display Register_check_ext_display(
            DSPif_check_ext_display f)
        {
            DSPif_check_ext_display tmp = if_check_ext_display;
            if_check_ext_display = f;
            return (tmp);
        }

    // Register an alternate rendering engine for cells.  This will be
    // called for each layer as the hierarchy is recursively traversed
    // during display composition.
    //
    bool ext_display_cell(WindowDesc *wdesc, const CDs *sdesc,
            const CDl *ldesc, int *ng)
        {
            if (if_ext_display_cell)
                return ((*if_ext_display_cell)(wdesc, sdesc, ldesc, ng));
            return (false);
        }

    DSPif_ext_display_cell Register_ext_display_cell(DSPif_ext_display_cell f)
        {
            DSPif_ext_display_cell tmp = if_ext_display_cell;
            if_ext_display_cell = f;
            return (tmp);
        }

    // Return true is sdesc should be shown when composing the
    // display.  This provides external control over which cells are
    // displayed.
    //
    bool check_display(WindowDesc *wdesc, const CDs *sdesc, const CDc *cdesc)
        {
            if (if_check_display)
                return ((*if_check_display)(wdesc, sdesc, cdesc));
            return (true);
        }

    DSPif_check_display Register_check_display(DSPif_check_display f)
        {
            DSPif_check_display tmp = if_check_display;
            if_check_display = f;
            return (tmp);
        }

    // Return true if the two windows should be treated as similar, in
    // terms of event reporting (in WindowDesc::IsSimilar).  This
    // allows application modes and commands to modify event reporting
    // behavior.
    //
    bool check_similar(const WindowDesc *wd1, const WindowDesc *wd2)
        {
            if (if_check_similar)
                return ((*if_check_similar)(wd1, wd2));
            return (false);
        }

    DSPif_check_similar Register_check_similar(DSPif_check_similar f)
        {
            DSPif_check_similar tmp = if_check_similar;
            if_check_similar = f;
            return (tmp);
        }
private:
    DSPif_show_message if_show_message;
    DSPif_notify_interrupted if_notify_interrupted;
    DSPif_notify_display_done if_notify_display_done;
    DSPif_notify_stack_overflow if_notify_stack_overflow;
    DSPif_window_show_highlighting if_window_show_highlighting;
    DSPif_window_show_blinking if_window_show_blinking;
    DSPif_window_view_change if_window_view_change;
    DSPif_window_view_saved if_window_view_saved;
    DSPif_window_clear_views if_window_clear_views;
    DSPif_window_mode_change if_window_mode_change;
    DSPif_window_get_title_strings if_window_get_title_strings;
    DSPif_window_destroy if_window_destroy;
    DSPif_context_cell if_context_cell;
    DSPif_is_subct_cmd_active if_is_subct_cmd_active;
    DSPif_comment_dump if_comment_dump;
    DSPif_check_ext_display if_check_ext_display;
    DSPif_ext_display_cell if_ext_display_cell;
    DSPif_check_display if_check_display;
    DSPif_check_similar if_check_similar;
};

#endif

