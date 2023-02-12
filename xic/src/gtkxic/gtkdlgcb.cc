
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
#include "cd_celldb.h"
#include "gtkmain.h"
#include "gtkinterf/gtkfile.h"


// Export the file/cell selected in the Files Selection, Cells Listing,
// Files Listing, Libraries Listing, or Tree pop-ups.
//
char *
cMain::GetCurFileSelection()
{
    if (!GRX)
        return (0);
    static char *tbuf;
    delete [] tbuf;
    tbuf = GTKfilePopup::any_selection();
    if (tbuf)
        return (tbuf);

    tbuf = main_bag::get_cell_selection();
    if (tbuf)
        return (tbuf);

    tbuf = main_bag::get_file_selection();
    if (tbuf)
        return (tbuf);

    tbuf = main_bag::get_lib_selection();
    if (tbuf)
        return (tbuf);

    tbuf = main_bag::get_tree_selection();
    if (tbuf)
        return (tbuf);

    // look for selected text in an Info window
    GdkWindow *window = gdk_selection_owner_get(GDK_SELECTION_PRIMARY);
    if (window) {
        GtkWidget *widget;
        gdk_window_get_user_data(window, (void**)&widget);
        if (widget &&
                g_object_get_data(G_OBJECT(widget), "export")) {
            tbuf = text_get_selection(widget);
            if (tbuf && CDcdb()->findSymbol(tbuf))
                return (tbuf);
            delete [] tbuf;
        }
    }
    return (0);
}


// Called when crashing, disable any updates
//
void
cMain::DisableDialogs()
{
    main_bag::cells_panic();
    main_bag::files_panic();
    main_bag::libs_panic();
    main_bag::tree_panic();
}

