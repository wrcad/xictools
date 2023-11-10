
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
#include "qtmain.h"
#include "qtinterf/qtfile.h"
#include "promptline.h"
#include "miscutil/pathlist.h"


// Save File Dialog - pop up a selectable directory tree, use the prompt
// line for text input.
//
class QTsaveFileDlg
{
public:
    QTsaveFileDlg()
    {
        sfd_fsel = 0;
        sfd_dir_only = false;
    }

    char *SaveFileDlg(const char*, const char*);
    char *OpenFileDlg(const char*, const char*);
    
private:
    static char *path_get();
    static void path_set(const char*);
    static void go_cb(const char*, void*);

    QTfileDlg *sfd_fsel;
    bool sfd_dir_only;
};

namespace { QTsaveFileDlg SFD; }


// Open a save-file selection window.
//
char *
cMain::SaveFileDlg(const char *prompt, const char *fnamein)
{
    if (!QTdev::exists())
        return (0);
    xm_htext_cnames_only = true;
    char *ret = SFD.SaveFileDlg(prompt, fnamein);
    xm_htext_cnames_only = false;
    return (ret);
}


// Open an open-file selection window.
//
char *
cMain::OpenFileDlg(const char *prompt, const char *fnamein)
{
    if (!QTdev::exists())
        return (0);
    xm_htext_cnames_only = true;
    char *ret = SFD.OpenFileDlg(prompt, fnamein);
    xm_htext_cnames_only = false;
    return (ret);
}


// Open a file selection window.
//
void
cMain::PopUpFileSel(const char *root, void(*cb)(const char*, void*), void *arg)
{
    if (!QTdev::exists())
        return;
    static int posn_cnt;

    QTfileDlg *fs = new QTfileDlg(QTmainwin::self(), fsSEL, arg, root);
    fs->set_transient_for(QTmainwin::self());
    fs->register_callback(cb);

    int cnt = posn_cnt % 4;
    int xcnt = (posn_cnt/4) % 4;
    posn_cnt++;

    QPoint pt = QTmainwin::self()->Viewport()->pos();
    pt = QTmainwin::self()->mapToGlobal(pt);
    pt.setX(pt.x() + cnt*200 + xcnt*50);
    pt.setY(pt.y() + cnt*100);
    fs->move(pt);

    fs->show();
}
// End od cMain functions.


// Obtain the file name to save a file under.  The return is the static
// string from the hypertext editor.  If fname is null, we are seeking a
// directory path
//
char *
QTsaveFileDlg::SaveFileDlg(const char *prompt, const char *fnamein)
{
    if (sfd_fsel)
        return (0);
    if (!QTmainwin::exists())
        return (0);
    sfd_dir_only = true;
    char *fname = pathlist::expand_path(fnamein, true, true);
    if (fname)
        sfd_dir_only = false;

    QTfileDlg *fs = new QTfileDlg(QTmainwin::self(), fsSAVE, 0, fname);
    fs->set_transient_for(QTmainwin::self());
    fs->register_callback(go_cb);
    fs->register_get_callback(path_get);
    fs->register_set_callback(path_set);
    sfd_fsel = fs;
    fs->register_usrptr((void**)&sfd_fsel);

    QTdev::self()->SetPopupLocation(GRloc(LW_LL), fs,
        QTmainwin::self()->Viewport());
    fs->show();

    char *in = PL()->EditPrompt(prompt, fname);
    pathlist::path_canon(in);
    if (sfd_fsel) {
        sfd_fsel->popdown();
        sfd_fsel = 0;
    }
    delete [] fname;
    return (in);
}


// Obtain the name of a file to open.  The return is the static
// string from the hypertext editor.
//
char *
QTsaveFileDlg::OpenFileDlg(const char *prompt, const char *fnamein)
{
    if (sfd_fsel)
        return (0);
    if (!QTmainwin::exists())
        return (0);
    char *fname = pathlist::expand_path(fnamein, true, true);

    QTfileDlg *fs = new QTfileDlg(QTmainwin::self(), fsOPEN, 0, fname);
    fs->set_transient_for(QTmainwin::self());
    fs->register_callback(go_cb);
    fs->register_set_callback(path_set);
    sfd_fsel = fs;
    fs->register_usrptr((void**)&sfd_fsel);

    QTdev::self()->SetPopupLocation(GRloc(LW_LL), fs,
        QTmainwin::self()->Viewport());
    fs->show();

    char *in = PL()->EditPrompt(prompt, fname);
    pathlist::path_canon(in);
    if (sfd_fsel) {
        sfd_fsel->popdown();
        sfd_fsel = 0;
    }
    delete [] fname;
    return (in);
}


// Static function.
char *
QTsaveFileDlg::path_get()
{
    if (SFD.sfd_dir_only)
        return (0);
    hyList *hp = PL()->List();
    if (!hp)
        return (0);
    char *s = hyList::string(hp, HYcvPlain, true);
    hyList::destroy(hp);
    // remove any quoting
    char *t = s;
    char *path = lstring::getqtok(&t);
    delete [] s;
    return (path);  // freed by widget
}


// Static function.
void
QTsaveFileDlg::path_set(const char *path)
{
    if (!path)
    {
        SFD.sfd_fsel = 0;
    }
    else {
        // quote if white space
        for (const char *s = path; *s; s++) {
            if (isspace(*s)) {
                char *t = new char[strlen(path) + 3];
                *t = '"';
                strcpy(t+1, path);
                strcat(t, "\"");
                delete [] path;
                path = t;
                break;
            }
        }
        PL()->EditPrompt(0, path, PLedUpdate);
        delete [] path;
    }
}


// Static function.
void
QTsaveFileDlg::go_cb(const char*, void*)
{
    // Simulate a Return press
    XM()->SendKeyEvent(0, Qt::Key_Return, 0, false);
    XM()->SendKeyEvent(0, Qt::Key_Return, 0, true);
}
// End of QTsaveFileDlg functions.

