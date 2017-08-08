
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

#include "config.h"
#include "main.h"
#include "cd_celldb.h"
#include "dsp_color.h"
#include "menu.h"
#include "view_menu.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkfont.h"
#include "miscutil/coresize.h"

#include <sys/types.h>
#include <sys/time.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#else
#ifdef WIN32
#include <windows.h>
#endif
#endif


//-----------------------------------------------------------------------------
// Memory Monitor pop-up
//

namespace {
    namespace gtkmem {
        struct sMem : public gtk_bag, public gtk_draw
        {
            sMem();
            ~sMem();

            void update();

        private:
            static void mem_popdown(GtkWidget*, void*);
            static int mem_proc(void*);
            static void mem_redraw(GtkWidget*, GdkEvent*, void*);
            static void mem_font_change(GtkWidget*, void*, void*);
        };

        sMem *Mem;
    }


    double chk_val(double val, char *m)
    {
        *m = 'K';
        if (val >= 1e9) {
            val *= 1e-9;
            *m = 'T';
        }
        else if (val >= 1e6) {
            val *= 1e-6;
            *m = 'G';
        }
        else if (val >= 1e3) {
            val *= 1e-3;
            *m = 'M';
        }
        return (val);
    }
}

using namespace gtkmem;


void
cMain::PopUpMemory(ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Mem;
        return;
    }
    if (Mem) {
        Mem->update();
        return;
    }
    if (mode == MODE_UPD)
        return;

    new sMem;
    if (!Mem->Shell()) {
        delete Mem;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Mem->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), Mem->Shell(), mainBag()->Viewport());
    gtk_widget_show(Mem->Shell());
    Mem->SetWindow(Mem->Viewport()->window);
    Mem->SetWindowBackground(GRX->NameColor("white"));
}
// End of cMain functions.


// Minimum widget width so that title text isn't truncated.
#define MEM_MINWIDTH 240

sMem::sMem()
{
    Mem = this;
    wb_shell = gtk_NewPopup(mainBag(), "Memory Monitor", mem_popdown, 0);
    if (!wb_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    //
    // the drawing area, in a frame
    //
    gd_viewport = gtk_drawing_area_new();
    gtk_widget_set_name(gd_viewport, "Viewport");
    gtk_widget_show(gd_viewport);

    GTKfont::setupFont(gd_viewport, FNT_FIXED, true);

    int fw, fh;
    TextExtent(0, &fw, &fh);
    int ww = 34*fw + 4;
    if (ww < MEM_MINWIDTH)
        ww = MEM_MINWIDTH;
    gtk_drawing_area_size(GTK_DRAWING_AREA(gd_viewport), ww, 5*fh + 4);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), gd_viewport);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 4, 2);

    gtk_widget_add_events(gd_viewport, GDK_EXPOSURE_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "expose-event",
        GTK_SIGNAL_FUNC(mem_redraw), 0);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "style-set",
        GTK_SIGNAL_FUNC(mem_font_change), 0);

    //
    // the dismiss button
    //
    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(mem_popdown), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    gtkPkgIf()->RegisterTimeoutProc(5000, mem_proc, 0);
}


sMem::~sMem()
{
    Mem = 0;
    Menu()->MenuButtonSet(0, MenuALLOC, false);
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(mem_popdown), wb_shell);
}


void
sMem::update()
{
    unsigned long c1 = DSP()->Color(PromptTextColor);
    unsigned long c2 = DSP()->Color(PromptEditTextColor);
    int fwid, fhei;
    SetWindowBackground(GRX->NameColor("white"));
    TextExtent(0, &fwid, &fhei);
    SetFillpattern(0);
    SetLinestyle(0);
    Clear();
    char buf[128];

    int x = 2;
    int y = fhei;
    int spw = fwid;

    SetColor(c1);
    const char *str = "Cells:";
    Text(str, x, y, 0);
    x += 7 * spw;
    sprintf(buf, "phys=%d elec=%d", CDcdb()->cellCount(Physical),
        CDcdb()->cellCount(Electrical));
    str = buf;
    SetColor(c2);
    Text(str, x, y, 0);

    x = 2;
    y += fhei;
    SetColor(c1);
    char b;
    double v = chk_val(coresize(), &b);
    sprintf(buf, "Current Datasize (%cB):", b);
    Text(buf, x, y, 0);
    x += 24 * spw;
    sprintf(buf, "%.3f", v);
    SetColor(c2);
    Text(buf, x, y, 0);

#ifdef HAVE_SYS_RESOURCE_H
    rlimit rl;
    if (getrlimit(RLIMIT_DATA, &rl))    // data segment limit
        return;

    rlimit rl2;
    if (getrlimit(RLIMIT_AS, &rl2))     // mmap limit
    return;

    x = 2;
    y += fhei;
    str = "System Datasize Limits";
    SetColor(c1);
    Text(str, x, y, 0);
    x = 2;
    y += fhei;
    if (rl.rlim_cur != RLIM_INFINITY && rl2.rlim_cur != RLIM_INFINITY &&
            (v = chk_val(0.001 * (rl.rlim_cur + rl2.rlim_cur), &b)) > 0) {
        sprintf(buf, "Soft (%cB):", b);
        Text(buf, x, y, 0);
        x += 24*spw;
        sprintf(buf, "%.1f", v);
        SetColor(c2);
        Text(buf, x, y, 0);
    }
    else {
        str = "Soft:";
        Text(str, x, y, 0);
        x += 24*spw;
        str = "none set";
        SetColor(c2);
        Text(str, x, y, 0);
    }

    SetColor(c1);
    x = 2;
    y += fhei;

    if (rl.rlim_max != RLIM_INFINITY && rl2.rlim_max != RLIM_INFINITY &&
            (v = chk_val(0.001 * (rl.rlim_max + rl2.rlim_max), &b)) > 0) {
        sprintf(buf, "Hard (%cB):", b);
        Text(buf, x, y, 0);
        x += 24*spw;
        sprintf(buf, "%.1f", v);
        SetColor(c2);
        Text(buf, x, y, 0);
    }
    else {
        str = "Hard:";
        Text(str, x, y, 0);
        x += 24*spw;
        str = "none set";
        SetColor(c2);
        Text(str, x, y, 0);
    }
#else
#ifdef WIN32
    MEMORYSTATUS mem;
    memset(&mem, 0, sizeof(MEMORYSTATUS));
    mem.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&mem);
    unsigned avail = mem.dwAvailPhys + mem.dwAvailPageFile;
    unsigned total = mem.dwTotalPhys + mem.dwTotalPageFile;

    x = 2;
    y += fhei;
    v = chk_val(0.001 * avail, &b);
    sprintf(buf, "Available Memory (%cB):", b);
    SetColor(c1);
    Text(buf, x, y, 0);
    x += 24 * spw;
    sprintf(buf, "%.1f", v);
    SetColor(c2);
    Text(buf, x, y, 0);

    x = 2;
    y += fhei;
    v = chk_val(0.001 * total, &b);
    sprintf(buf, "Total Memory (%cB):", b);
    SetColor(c1);
    Text(buf, x, y, 0);
    x += 24 * spw;
    sprintf(buf, "%.1f", v);
    SetColor(c2);
    Text(buf, x, y, 0);
#endif
#endif
}


// Static function.
void
sMem::mem_redraw(GtkWidget*, GdkEvent*, void*)
{
    XM()->PopUpMemory(MODE_UPD);
}


// Static function.
void
sMem::mem_font_change(GtkWidget*, void*, void*)
{
    if (Mem) {
        int fw, fh;
        Mem->TextExtent(0, &fw, &fh);
        int ww = 34*fw + 4;
        if (ww < MEM_MINWIDTH)
            ww = MEM_MINWIDTH;
        gtk_drawing_area_size(GTK_DRAWING_AREA(Mem->gd_viewport), ww, 5*fh + 4);
    }
}


// Static function.
int
sMem::mem_proc(void*)
{
    XM()->PopUpMemory(MODE_UPD);
    return (Mem != 0);
}


// Static function.
void
sMem::mem_popdown(GtkWidget*, void*)
{
    XM()->PopUpMemory(MODE_OFF);
}

