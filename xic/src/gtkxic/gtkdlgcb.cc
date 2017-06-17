
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
 $Id: gtkdlgcb.cc,v 5.4 2012/06/07 01:56:50 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "gtkmain.h"
#include "gtkfile.h"
#include "cd_celldb.h"


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
                gtk_object_get_data(GTK_OBJECT(widget), "export")) {
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

