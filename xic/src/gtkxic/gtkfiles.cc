
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
 $Id: gtkfiles.cc,v 5.71 2016/06/30 16:37:05 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "cvrt.h"
#include "editif.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_library.h"
#include "fio_alias.h"
#include "fio_chd.h"
#include "gtkmain.h"
#include "gtkmcol.h"
#include "gtkpfiles.h"
#include "gtkfont.h"
#include "gtkinlines.h"
#include "events.h"
#include "errorlog.h"


//----------------------------------------------------------------------
//  Files Popup
//
// Help system keywords used:
//  filespanel

// The buttons displayed for various modes:
// no editing:      view, content, help
// displaying chd:  content, help
// normal mode:     edit, master, content, help

#define FB_OPEN     "Open"
#define FB_PLACE    "Place"
#define FB_CONTENT  "Content"
#define FB_HELP     "Help"


namespace {
    namespace gtkfiles {
        inline struct sFL *FL();

        struct sFL : public files_bag
        {
            friend inline sFL *FL()
                { return (dynamic_cast<sFL*>(instance())); }

            sFL(GRobject);
            ~sFL();

            void update();
            char *get_selection();

        private:
            void action_hdlr(GtkWidget*);
            bool button_hdlr(GtkWidget*, GdkEvent*);
            bool show_content();
            void set_sensitive(const char*, bool);

            static sPathList *fl_listing(int);
            static char *fl_is_symfile(const char*);
            static void fl_action_proc(GtkWidget*, void*);
            static int fl_btn_proc(GtkWidget*, GdkEvent*, void*);
            static void fl_content_cb(const char*, void*);
            static void fl_down_cb(GtkWidget*, void*);
            static void fl_desel(void*);

            char *fl_selection;
            char *fl_contlib;
            GRmcolPopup *fl_content_pop;
            cCHD *fl_chd;
            GRobject fl_caller;
            int fl_noupdate;

            static const char *nofiles_msg;
            static const char *files_msg;
        };
    }
}

using namespace gtkfiles;

const char *sFL::nofiles_msg = "  no recognized files found\n";
const char *sFL::files_msg =
    "Files from search path.\nTypes: B CGX, C CIF, "
#ifdef HANDLE_SCED
    "G GDSII, L library, O OASIS, S JSPICE3 (sced), X Xic";
#else
    "G GDSII, L library, O OASIS, X Xic";
#endif


// Static function.
//
char *
main_bag::get_file_selection()
{
    if (FL())
        return (FL()->get_selection());
    return (0);
}


// Static function.
// Called on crash to prevent updates.
//
void
main_bag::files_panic()
{
    files_bag::panic();
}


// It can take a while to process the files, unfortunately the "busy"
// cursor seems to never appear with the standard logic.  In order to
// make the busy cursor appear, had to use a timeout as below.

namespace {
    int msw_timeout(void *caller)
    {
        new sFL(caller);

        gtk_window_set_transient_for(GTK_WINDOW(FL()->Shell()),
            GTK_WINDOW(mainBag()->Shell()));
        GRX->SetPopupLocation(GRloc(), FL()->Shell(), mainBag()->Viewport());
        gtk_widget_show(FL()->Shell());

        dspPkgIf()->SetWorking(false);
        return (0);
    }
}


void
cConvert::PopUpFiles(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete FL();
        return;
    }
    if (mode == MODE_UPD) {
        if (FL())
            FL()->update();
        return;
    }

    if (FL())
        return;

    // This is needed to reliable show the busy cursor.
    dspPkgIf()->SetWorking(true);
    gtk_timeout_add(500, msw_timeout, caller);

    /****
    new sFL(caller);

    gtk_window_set_transient_for(GTK_WINDOW(FL()->Shell()),
        GTK_WINDOW(mainBag()->Shell()));
    GRX->SetPopupLocation(GRloc(), FL()->Shell(), mainBag()->Viewport());
    gtk_widget_show(FL()->Shell());
    ****/
}


sFL::sFL(GRobject c) :
    files_bag(mainBag(), 0, 0, files_msg, fl_action_proc, fl_btn_proc,
        fl_listing, fl_down_cb, fl_desel)
{
    fl_selection = 0;
    fl_contlib = 0;
    fl_content_pop = 0;
    fl_chd = 0;
    fl_caller = c;
    fl_noupdate = 0;

    update();

    if (fl_caller)
        gtk_signal_connect_after(GTK_OBJECT(fl_caller), "toggled",
            GTK_SIGNAL_FUNC(fl_down_cb), this);
}


sFL::~sFL()
{
    if (fl_caller) {
        gtk_signal_disconnect_by_func(GTK_OBJECT(fl_caller),
            GTK_SIGNAL_FUNC(fl_down_cb), this);
        GRX->Deselect(fl_caller);
    }
    if (fl_content_pop)
        fl_content_pop->popdown();
    delete [] fl_selection;
    delete [] fl_contlib;
    delete fl_chd;
}


void
sFL::update()
{
    if (fl_noupdate) {
        fl_noupdate++;
        return;
    }
    if (fl_content_pop) {
        if (DSP()->MainWdesc()->DbType() == WDchd)
            fl_content_pop->set_button_sens(0);
        else
            fl_content_pop->set_button_sens(-1);
    }

    const char *btns[5];
    int nbtns;
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        btns[0] = FB_CONTENT;
        btns[1] = FB_HELP;
        nbtns = 2;
    }
    else if (!EditIf()->hasEdit()) {
        btns[0] = FB_OPEN;
        btns[1] = FB_CONTENT;
        btns[2] = FB_HELP;
        nbtns = 3;
    }
    else {
        btns[0] = FB_OPEN;
        btns[1] = FB_PLACE;
        btns[2] = FB_CONTENT;
        btns[3] = FB_HELP;
        nbtns = 4;
    }
    files_bag::update(FIO()->PGetPath(), btns, nbtns, fl_action_proc);
    fl_desel(0);
}


// This overrides files_bag::get_selection().
//
char *
sFL::get_selection()
{
    if (!(void*)this)
        return (0);
    if (fl_contlib && fl_content_pop) {
        char *sel = fl_content_pop->get_selection();
        if (sel) {
            int len = strlen(fl_contlib) + strlen(sel) + 2;
            char *tbuf = new char[len];
            char *t = lstring::stpcpy(tbuf, fl_contlib);
            *t++ = ' ';
            strcpy(t, sel);
            delete [] sel;
            return (tbuf);
        }
    }
    if (fl_selection && text_has_selection(wb_textarea))
        return (lstring::copy(fl_selection));
    return (0);
}


void
sFL::action_hdlr(GtkWidget *caller)
{
    if (!wb_textarea) {
        GRX->Deselect(caller);
        return;
    }
    const char *wname = gtk_widget_get_name(caller);
    if (!wname) {
        GRX->Deselect(caller);
        return;
    }

    // Note:  during open and place, callbacks to update are deferred. 
    // These are generated when the path changes when reading native
    // cells.
    //
    if (!strcmp(wname, FB_OPEN)) {
        GRX->Deselect(caller);
        if (fl_selection) {
            fl_noupdate = 1;
            EV()->InitCallback();
            XM()->EditCell(fl_selection, false, FIO()->DefReadPrms(), 0, 0);
            if (FL()) {
                if (fl_noupdate > 1) {
                    files_bag::update(FIO()->PGetPath(), 0, 0, 0);
                    fl_desel(0);
                }
                fl_noupdate = 0;
            }
        }
    }
    else if (!strcmp(wname, FB_PLACE)) {
        GRX->Deselect(caller);
        if (fl_selection) {
            fl_noupdate = 1;
            EV()->InitCallback();
            EditIf()->addMaster(fl_selection, 0, 0);
            if (FL()) {
                if (fl_noupdate > 1) {
                    files_bag::update(FIO()->PGetPath(), 0, 0, 0);
                    fl_desel(0);
                }
                fl_noupdate = 0;
            }
        }
    }
    else if (!strcmp(wname, FB_CONTENT)) {
        GRX->Deselect(caller);
        show_content();
    }
    else if (!strcmp(wname, FB_HELP)) {
        GRX->Deselect(caller);
        DSPmainWbag(PopUpHelp("filespanel"))
    }
}


bool
sFL::button_hdlr(GtkWidget *caller, GdkEvent *event)
{
    if (event->type != GDK_BUTTON_PRESS)
        return (true);

    set_sensitive(FB_OPEN, false);
    set_sensitive(FB_PLACE, false);
    set_sensitive(FB_CONTENT, false);

    char *string = text_get_chars(caller, 0, -1);
    if (!strcmp(string, nofiles_msg)) {
        delete [] string;
        return (true);
    }
    int x = (int)event->button.x;
    int y = (int)event->button.y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    x = gtk_text_iter_get_offset(&ihere) - gtk_text_iter_get_offset(&iline);
    char *line_start = string + gtk_text_iter_get_offset(&iline);

    const int *colwid = 0;
    for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next()) {
        if (dl->dataptr() && !strcmp(f_directory, dl->dirname())) {
            colwid = dl->col_width();
            break;
        }
    }
    if (!colwid) {
        delete [] string;
        return (true);
    }

    int cstart = 0;
    int cend = 0;
    for (int i = 0; colwid[i]; i++) {
        cstart = cend;
        cend += colwid[i];
        if (x >= cstart && x < cend)
            break;
    }
    if (cstart == cend || x >= cend) {
            delete [] string;
            return (0);
    }
    for (int st = 0 ; st <= x; st++) {
        if (line_start[st] == '\n' || line_start[st] == 0) {
            // pointing to right of line end
            delete [] string;
            return (0);
        }
    }

    // We know that the file name starts at cstart, find the actual
    // end.  Note that we deal with file names that contain spaces.
    for (int st = 0 ; st < cend; st++) {
        if (line_start[st] == '\n') {
            cend = st;
            break;
        }
    }

    // Omit trailing space.
    while (isspace(line_start[cend-1]))
        cend--;
    if (x >= cend) {
        // Clicked on trailing white space.
        delete [] string;
        return (0);
    }

    // Skip over the filetype indicator, which is a single character
    // and a space separator.
    char code = line_start[cstart];
    cstart += 2;

    cstart += (line_start - string);
    cend += (line_start - string);
    delete [] string;

    if (cstart == cend) {
        select_range(caller, 0, 0);
        return (true);
    }
    select_range(caller, cstart, cend);

    // The fl_selection has the full path.
    delete [] fl_selection;
    fl_selection = files_bag::get_selection();

    if (fl_selection) {
        set_sensitive(FB_OPEN, true);
        set_sensitive(FB_PLACE, true);
        if (strchr("GBOCL", code))
            set_sensitive(FB_CONTENT, true);

        f_drag_start = true;
        f_drag_btn = event->button.button;
        f_drag_x = (int)event->button.x;
        f_drag_y = (int)event->button.y;
    }
    return (true);
}


// Pop up the contents listing for archives.
//
bool
sFL::show_content()
{
    if (!fl_selection)
        return (false);

    FILE *fp = fopen(fl_selection, "rb");
    if (!fp)
        return (false);

    FileType filetype = Fnone;
    CFtype ct;
    bool issc = false;
    if (FIO()->IsGDSII(fp))  // must be first!
        filetype = Fgds;
    else if (FIO()->IsCGX(fp))
        filetype = Fcgx;
    else if (FIO()->IsOASIS(fp))
        filetype = Foas;
    else if (FIO()->IsLibrary(fp))
        filetype = Fnative;
    else if (FIO()->IsCIF(fp, &ct, &issc) && ct != CFnative && !issc)
        filetype = Fcif;
    fclose(fp);

    if (fl_chd && strcmp(fl_chd->filename(), fl_selection)) {
        delete fl_chd;
        fl_chd = 0;
    }

    if (filetype == Fnone) {
        stringlist *list = new stringlist(
            lstring::copy("Selected file is not an archive."), 0);
        sLstr lstr;
        lstr.add("Regular file\n");
        lstr.add(fl_selection);

        delete [] fl_contlib;
        fl_contlib = 0;

        if (fl_content_pop)
            fl_content_pop->update(list, lstr.string());
        else {
            const char *buttons[3];
            buttons[0] = FB_OPEN;
            buttons[1] = EditIf()->hasEdit() ? FB_PLACE : 0;
            buttons[2] = 0;

            int pagesz = 0;
            const char *s = CDvdb()->getVariable(VA_ListPageEntries);
            if (s) {
                pagesz = atoi(s);
                if (pagesz < 100 || pagesz > 50000)
                    pagesz = 0;
            }
            fl_content_pop = DSPmainWbagRet(PopUpMultiCol(list, lstr.string(),
                fl_content_cb, 0, buttons, pagesz));
            if (fl_content_pop) {
                fl_content_pop->register_usrptr((void**)&fl_content_pop);
                if (DSP()->MainWdesc()->DbType() == WDchd)
                    fl_content_pop->set_button_sens(0);
                else
                    fl_content_pop->set_button_sens(-1);
            }
        }
        list->free();
        return (true);
    }
    else if (filetype == Fnative) {
        // library
        if (FIO()->OpenLibrary(0, fl_selection)) {
            stringlist *list = FIO()->GetLibNamelist(fl_selection, LIBuser);
            if (list) {
                sLstr lstr;
                lstr.add("References found in library - click to select\n");
                lstr.add(fl_selection);

                delete [] fl_contlib;
                fl_contlib = lstring::copy(fl_selection);

                if (fl_content_pop)
                    fl_content_pop->update(list, lstr.string());
                else {
                    const char *buttons[3];
                    buttons[0] = FB_OPEN;
                    buttons[1] = EditIf()->hasEdit() ? FB_PLACE : 0;
                    buttons[2] = 0;

                    int pagesz = 0;
                    const char *s = CDvdb()->getVariable(VA_ListPageEntries);
                    if (s) {
                        pagesz = atoi(s);
                        if (pagesz < 100 || pagesz > 50000)
                            pagesz = 0;
                    }
                    fl_content_pop = DSPmainWbagRet(PopUpMultiCol(list,
                        lstr.string(), fl_content_cb, 0, buttons, pagesz));
                   if (fl_content_pop) {
                        fl_content_pop->register_usrptr(
                            (void**)&fl_content_pop);
                        if (DSP()->MainWdesc()->DbType() == WDchd)
                            fl_content_pop->set_button_sens(0);
                        else
                            fl_content_pop->set_button_sens(-1);
                   }
                }
                list->free();
                return (true);
            }
        }
    }
    else if (FIO()->IsSupportedArchiveFormat(filetype)) {
        if (!fl_chd) {
            unsigned int alias_mask = (CVAL_CASE | CVAL_PFSF | CVAL_FILE);
            FIOaliasTab *tab = FIO()->NewReadingAlias(alias_mask);
            if (tab)
                tab->read_alias(fl_selection);
            fl_chd = FIO()->NewCHD(fl_selection, filetype, Electrical, tab,
                cvINFOplpc);
            delete tab;
        }
        if (fl_chd) {
            stringlist *list = fl_chd->listCellnames(-1, false);
            sLstr lstr;
            lstr.add("Cells found in ");
            lstr.add(FIO()->TypeName(filetype));
            lstr.add(" file - click to select\n");
            lstr.add(fl_selection);

            delete [] fl_contlib;
            fl_contlib = lstring::copy(fl_selection);

            if (fl_content_pop)
                fl_content_pop->update(list, lstr.string());
            else {
                const char *buttons[3];
                buttons[0] = FB_OPEN;
                buttons[1] = EditIf()->hasEdit() ? FB_PLACE : 0;
                buttons[2] = 0;

                int pagesz = 0;
                const char *s = CDvdb()->getVariable(VA_ListPageEntries);
                if (s) {
                    pagesz = atoi(s);
                    if (pagesz < 100 || pagesz > 50000)
                        pagesz = 0;
                }
                fl_content_pop = DSPmainWbagRet(PopUpMultiCol(list,
                    lstr.string(), fl_content_cb, 0, buttons, pagesz));
                if (fl_content_pop) {
                    fl_content_pop->register_usrptr((void**)&fl_content_pop);
                    if (DSP()->MainWdesc()->DbType() == WDchd)
                        fl_content_pop->set_button_sens(0);
                    else
                        fl_content_pop->set_button_sens(-1);
                }
            }
            list->free();
            return (true);
        }
        else {
            Log()->ErrorLogV(mh::Processing,
                "Content scan failed: %s", Errs()->get_error());
        }

    }
    return (false);
}


void
sFL::set_sensitive(const char *bname, bool state)
{
    for (int i = 0; f_buttons[i]; i++) {
        const char *wname = gtk_widget_get_name(f_buttons[i]);
        if (!wname)
            continue;
        if (!strcmp(wname, bname)) {
            gtk_widget_set_sensitive(f_buttons[i], state);
            break;
        }
    }
}


// Static function.
// Create the listing struct.
//
sPathList *
sFL::fl_listing(int cols)
{
    dspPkgIf()->SetWorking(true);
    sPathList *l = new sPathList(FIO()->PGetPath(), fl_is_symfile, nofiles_msg,
        0, 0, cols, false);
    dspPkgIf()->SetWorking(false);
    return (l);
}


// Static function.
// Test condition for files to be listed.
// Return non-null if a known layout file.
//
char*
sFL::fl_is_symfile(const char *fname)
{
    FILE *fp = fopen(fname, "rb");
    if (!fp)
        return (0);
    fname = lstring::strip_path(fname);
    char buf[128];
    // *** The GDSII test MUST come first
    if (FIO()->IsGDSII(fp)) {
        // GDSII
        sprintf(buf, "G %s", fname);
        fclose(fp);
        return (lstring::copy(buf));
    }
    if (FIO()->IsCGX(fp)) {
        // CGX
        sprintf(buf, "B %s", fname);
        fclose(fp);
        return (lstring::copy(buf));
    }
    if (FIO()->IsOASIS(fp)) {
        // OASIS
        sprintf(buf, "O %s", fname);
        fclose(fp);
        return (lstring::copy(buf));
    }
    if (FIO()->IsLibrary(fp)) {
        // Library
        sprintf(buf, "L %s", fname);
        fclose(fp);
        return (lstring::copy(buf));
    }
    CFtype type;
    bool issced;
    if (FIO()->IsCIF(fp, &type, &issced)) {
        // CIF, XIC
        sprintf(buf, "  %s", fname);
        if (type == CFnative)
            buf[0] = 'X';
        else
            buf[0] = 'C';
        if (issced) {
#ifdef HANDLE_SCED
            buf[0] = 'S';
#else
            fclose(fp);
            return (0);
#endif
        }
        fclose(fp);
        return (lstring::copy(buf));
    }
    fclose(fp);
    return (0);
}


// Static function.
// If there is something selected, perform the action.
//
void
sFL::fl_action_proc(GtkWidget *caller, void*)
{
    if (FL())
        FL()->action_hdlr(caller);
}


// Static function.
int
sFL::fl_btn_proc(GtkWidget *caller, GdkEvent *event, void*)
{
    if (FL())
        return (FL()->button_hdlr(caller, event));
    return (true);
}


// Static function.
void
sFL::fl_content_cb(const char *cellname, void*)
{
    if (!FL())
        return;
    if (!FL()->fl_contlib || !FL()->fl_content_pop)
        return;
    if (!cellname)
        return;
    if (*cellname != '/')
        return;

    cellname++;
    if (!strcmp(cellname, FB_OPEN)) {
        char *sel = FL()->fl_content_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            XM()->EditCell(FL()->fl_contlib, false, FIO()->DefReadPrms(), sel,
                FL()->fl_chd);
            delete [] sel;
        }
    }
    else if (!strcmp(cellname, FB_PLACE)) {
        char *sel = FL()->fl_content_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            EditIf()->addMaster(FL()->fl_contlib, sel, FL()->fl_chd);
            delete [] sel;
        }
    }
}


// Static function.
void
sFL::fl_down_cb(GtkWidget*, void*)
{
    Cvt()->PopUpFiles(0, MODE_OFF);
}


// Static function.
void
sFL::fl_desel(void*)
{
    if (!FL())
        return;
    if (text_has_selection(FL()->wb_textarea)) {
        FL()->set_sensitive(FB_OPEN, true);
        FL()->set_sensitive(FB_PLACE, true);
        FL()->set_sensitive(FB_CONTENT, true);
    }
    else {
        delete [] FL()->fl_selection;
        FL()->fl_selection = 0;
        FL()->set_sensitive(FB_OPEN, false);
        FL()->set_sensitive(FB_PLACE, false);
        FL()->set_sensitive(FB_CONTENT, false);
    }
}

