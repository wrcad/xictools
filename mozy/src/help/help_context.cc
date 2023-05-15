
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
 * Help System Files                                                      *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "help_defs.h"
#include "help_context.h"
#include "help_startup.h"
#include "help_cache.h"
#include "help_topic.h"
#include "help_pkgs.h"
#include "httpget/transact.h"
#include "miscutil/pathlist.h"
#include "miscutil/lstring.h"
#include "miscutil/filestat.h"
#include "miscutil/symtab.h"
#include "miscutil/proxy.h"
#include "ginterf/graphics.h"
#include "htm/htm_widget.h"
#include "htm/htm_form.h"

#include <sys/stat.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <unistd.h>


// Default download file name
//
#define DEF_DL_FILE "http_return"

// Size of buffer to hold url string.
#define URL_BUFSIZE 4096

namespace {
    // File extensions recognized as ascii text files.
    //
    const char *text_exts[] = {
        "txt", "log", "scr", "sh", "csh", "c", "cc", "cpp",
        "h", "py", "tcl", "tk", 0 };

    // File extensions recognized as binary files.
    //
    const char *bin_exts[] = {
        "zip", "rpm", "gz", "Z", "bz", "bz2", "tar", "exe",
        "dll", "o", "a", "gds", "oas", "cgx", 0 };

    // Return true if suf matches a known binary file extension.
    //
    bool
    dl_suffix(const char *suf)
    {
        for (const char **s = bin_exts; *s; s++) {
            if (lstring::cieq(suf, *s))
                return (true);
        }
        return (false);
    }
}


//-----------------------------------------------------------------------------
// cHelp functions

void
cHelp::set_font_family(const char *fontname)
{
    if (fontname) {
        if (xfd_t::is_xfd(fontname)) {
            // X font for gtk-1
            xfd_t f(fontname);
            if (!f.get_family())
                return;
            if (!f.get_pixsize() || !isdigit(*f.get_pixsize()))
                f.set_pixsize(14);
            fontname = f.font_xfd();
            FC.setName(fontname, FNT_MOZY);
            delete [] fontname;
        }
        else
            FC.setName(fontname, FNT_MOZY);
        hlp_context->setFontSet(true);
    }
}


void
cHelp::set_fixed_family(const char *fontname)
{
    if (fontname) {
        if (xfd_t::is_xfd(fontname)) {
            // X font for gtk-1
            xfd_t f(fontname);
            if (!f.get_family())
                return;
            if (!f.get_pixsize() || !isdigit(*f.get_pixsize()))
                f.set_pixsize(14);
            fontname = f.font_xfd();
            FC.setName(fontname, FNT_MOZY_FIXED);
            delete [] fontname;
        }
        else
            FC.setName(fontname, FNT_MOZY_FIXED);
        hlp_context->setFixedFontSet(true);
    }
}


const char *
cHelp::get_font_family()
{
    const char *fontname = FC.getName(FNT_MOZY);
    if (xfd_t::is_xfd(fontname)) {
        // X font for gtk-1
        xfd_t f(fontname);
        if (!f.get_family())
            return (0);
        return (f.family_xfd(0, true));
    }
    return (fontname);
}


const char *
cHelp::get_fixed_family()
{
    const char *fontname =  FC.getName(FNT_MOZY_FIXED);
    if (xfd_t::is_xfd(fontname)) {
        // X font for gtk-1
        xfd_t f(fontname);
        if (!f.get_family())
            return (0);
        return (f.family_xfd(0, true));
    }
    return (fontname);
}


//-----------------------------------------------------------------------------
// Constructors

HLPimageList::HLPimageList(ViewerWidget *w, const char *f, const char *u,
    htmImageInfo *info, IMstatus p, HLPimageList *n)
{
    widget = w;
    next = n;
    filename = lstring::copy(f);
    url = lstring::copy(u);
    orig_url = lstring::copy(u);
    tmp_image = info;
    status = p;
    load_prog = false;
    start_prog = false;
    local_image = false;
    inactive = false;
}


namespace {
    // This intercepts "special" keywords in hlp_word.
    //
    bool
    special_callback(char **word, HLPtopic **ptop)
    {
        return (HLP()->context()->resolveKeyword(*word, ptop, 0, 0, 0,
            false, true));
    }
}


HLPcontext::HLPcontext()
{
    HLP()->register_callback(special_callback);

    hcxCache            = 0;
    hcxBookmarks        = 0;
    hcxAuthList         = 0;
    hcxVisitedTab       = 0;
    hcxTopList          = 0;
    hcxImageList        = 0;
    hcxImageFreeList    = 0;
    hcxLocalTest        = 0;

    // These are set in the <body> tag added to HTML documents without
    // one.

    hcxBGimage          = 0;
    hcxBGcolor          = lstring::copy(HLP_DEF_BG_COLOR);
    hcxFGcolor          = lstring::copy(HLP_DEF_FG_COLOR);
    hcxLNcolor          = lstring::copy(HLP_DEF_LN_COLOR);
    hcxVLcolor          = lstring::copy(HLP_DEF_VL_COLOR);
    hcxALcolor          = lstring::copy(HLP_DEF_AL_COLOR);

    // We use htm to store the default color names, these will be used
    // when rendering, unless overridden.
    //
    // The background is a special case, the value below will b e used
    // for plain text.

    htmDefColors &defc      = htmColorManager::cm_defcolors;
    defc.DefFgLink          = lstring::copy(HLP_DEF_LN_COLOR);
    defc.DefFgVisitedLink   = lstring::copy(HLP_DEF_VL_COLOR);
    defc.DefFgTargetLink    = lstring::copy(HLP_DEF_LN_COLOR);
    defc.DefFgActiveLink    = lstring::copy(HLP_DEF_AL_COLOR);
    defc.DefBg              = lstring::copy("white");  // plain text bg
    defc.DefFgText          = lstring::copy(HLP_DEF_FG_COLOR);
    defc.DefBgSelect        = lstring::copy(HLP_DEF_SL_COLOR);
    defc.DefFgImagemap      = lstring::copy(HLP_DEF_IM_COLOR);

    hcxRootAlii         = 0;

    hcxQuitProc         = 0;
    hcxSubmitProc       = 0;

    hcxWindowCnt        = 0;
    hcxFontSet          = false;
    hcxFixedFontSet     = false;

    hcxCache            = new HLPcache;
}


void
HLPcontext::quitHelp()
{
    if (!hcxTopList) {
        HLPtopic::set_win_count(0);
        if (hcxQuitProc)
            (*hcxQuitProc)(0);
    }
}

//-----------------------------------------------------------------------------
// URL Cache Control

void
HLPcontext::clearCache()
{
    if (hcxCache)
        hcxCache->clear();
}


void
HLPcontext::reloadCache()
{
    if (!hcxCache)
        hcxCache = new HLPcache;
    hcxCache->load();
}


stringlist *
HLPcontext::listCache()
{
    return (hcxCache ? hcxCache->list_entries() : 0);
}


//-----------------------------------------------------------------------------
// Visited URL Table

void
HLPcontext::addVisited(const char *href)
{
    if (!href || !*href)
        return;
    if (!hcxVisitedTab)
        hcxVisitedTab = new strtab_t;
    hcxVisitedTab->add(href);
}


void
HLPcontext::clearVisited()
{
    if (hcxVisitedTab)
        hcxVisitedTab->clear();
}


bool
HLPcontext::isVisited(const char *href)
{
    if (href && *href && hcxVisitedTab)
        return (hcxVisitedTab->find(href));
    return (false);
}


//-----------------------------------------------------------------------------
// Font Initialization

// Initialize the proportional font family.
//
void
HLPcontext::setFont(const char *fontname)
{
    if (!hcxFontSet && fontname) {
        if (xfd_t::is_xfd(fontname)) {
            // X font for gtk-1
            xfd_t f(fontname);
            if (!f.get_family())
                return;
            if (!f.get_pixsize() || !isdigit(*f.get_pixsize()))
                f.set_pixsize(14);
            fontname = f.font_xfd();
            FC.setName(fontname, FNT_MOZY);
            delete [] fontname;
        }
        else
            FC.setName(fontname, FNT_MOZY);
        hcxFontSet = true;
    }
}


// Initialize the fixed font family.
//
void
HLPcontext::setFixedFont(const char *fontname)
{
    if (!hcxFixedFontSet && fontname) {
        if (xfd_t::is_xfd(fontname)) {
            // X font for gtk-1
            xfd_t f(fontname);
            if (!f.get_family())
                return;
            if (!f.get_pixsize() || !isdigit(*f.get_pixsize()))
                f.set_pixsize(14);
            fontname = f.font_xfd();
            FC.setName(fontname, FNT_MOZY_FIXED);
            delete [] fontname;
        }
        else
            FC.setName(fontname, FNT_MOZY_FIXED);
        hcxFixedFontSet = true;
    }
}


//-----------------------------------------------------------------------------
// Color Initialization

// Initialize the default colors, by mozyrc keyword name.
//
void
HLPcontext::setDefaultColor(const char *name, const char *value)
{
    // These colors will be applied through a <body> tag to HTML text
    // that has no body tag, including help text.

    if (lstring::cieq(name, HLP_DefaultBgImage)) {
        delete [] hcxBGimage;
        hcxBGimage = lstring::copy(value);
        return;
    }
    if (lstring::cieq(name, HLP_DefaultBgColor)) {
        delete [] hcxBGcolor;
        hcxBGcolor = lstring::copy(value);
        return;
    }
    else if (lstring::cieq(name, HLP_DefaultFgText)) {
        delete [] hcxFGcolor;
        hcxFGcolor = lstring::copy(value);
    }
    if (lstring::cieq(name, HLP_DefaultFgLink)) {
        delete [] hcxLNcolor;;
        hcxLNcolor = lstring::copy(value);
    }
    else if (lstring::cieq(name, HLP_DefaultFgVisitedLink)) {
        delete [] hcxVLcolor;
        hcxVLcolor = lstring::copy(value);
    }
    else if (lstring::cieq(name, HLP_DefaultFgActiveLink)) {
        delete [] hcxALcolor;
        hcxALcolor = lstring::copy(value);
    }

    // These colors are saved as widget defaults.

    htmDefColors &defc = htmColorManager::cm_defcolors;

    if (lstring::cieq(name, HLP_DefaultBgSelect)) {
        delete [] defc.DefBgSelect;
        defc.DefBgSelect = lstring::copy(value);
    }
    else if (lstring::cieq(name, HLP_DefaultFgImagemap)) {
        delete [] defc.DefFgImagemap;
        defc.DefFgImagemap = lstring::copy(value);
    }
}


// Return the color values.
//
const char *
HLPcontext::getDefaultColor(const char *name)
{
    if (lstring::cieq(name, HLP_DefaultBgImage))
        return (hcxBGimage);
    if (lstring::cieq(name, HLP_DefaultBgColor))
        return (hcxBGcolor);
    if (lstring::cieq(name, HLP_DefaultFgText))
        return (hcxFGcolor);
    if (lstring::cieq(name, HLP_DefaultFgLink))
        return (hcxLNcolor);
    if (lstring::cieq(name, HLP_DefaultFgVisitedLink))
        return (hcxVLcolor);
    if (lstring::cieq(name, HLP_DefaultFgActiveLink))
        return (hcxALcolor);

    htmDefColors &defc = htmColorManager::cm_defcolors;

    if (lstring::cieq(name, HLP_DefaultBgSelect))
        return (defc.DefBgSelect);
    if (lstring::cieq(name, HLP_DefaultFgImagemap))
        return (defc.DefFgImagemap);
    return (0);
}


// Return a string containg a <body ...> tag using the default colors.
//
char *
HLPcontext::getBodyTag()
{
    char tbuf[64];
    sLstr lstr;
    lstr.add("<body");
    if (hcxBGimage && *hcxBGimage) {
        snprintf(tbuf, sizeof(tbuf), " background=\"%s\" ", hcxBGimage);
        lstr.add(tbuf);
    }
    else if (hcxBGcolor && *hcxBGcolor) {
        snprintf(tbuf, sizeof(tbuf), " bgcolor=\"%s\" ", hcxBGcolor);
        lstr.add(tbuf);
    }
    if (hcxFGcolor && *hcxFGcolor) {
        snprintf(tbuf, sizeof(tbuf), " text=\"%s\" ", hcxFGcolor);
        lstr.add(tbuf);
    }
    if (hcxLNcolor && *hcxLNcolor) {
        snprintf(tbuf, sizeof(tbuf), " link=\"%s\" ", hcxLNcolor);
        lstr.add(tbuf);
    }
    if (hcxVLcolor && *hcxVLcolor) {
        snprintf(tbuf, sizeof(tbuf), " vlink=\"%s\" ", hcxVLcolor);
        lstr.add(tbuf);
    }
    if (hcxALcolor && *hcxALcolor) {
        snprintf(tbuf, sizeof(tbuf), " alink=\"%s\" ", hcxALcolor);
        lstr.add(tbuf);
    }
    lstr.add(">\n");
    return (lstr.string_trim());
}


//-----------------------------------------------------------------------------
// Topic List Maintenance

// Link a new top-level topic into the list.
//
void
HLPcontext::linkNewTopic(HLPtopic *newtop)
{
    if (!hcxTopList)
        hcxTopList = newtop;
    else {
        HLPtopic *t = hcxTopList;
        while (t->sibling())
            t = t->sibling();
        t->set_sibling(newtop);
    }
}


// If top is found in the list, remove it, returning true.  The topic
// is owned by the display widget so dont't destroy it here.
//
bool
HLPcontext::removeTopic(HLPtopic *top)
{
    HLPtopic *tp = 0, *tn;
    for (HLPtopic *t = hcxTopList; t; t = tn) {
        tn = t->sibling();
        if (t == top) {
            if (tp)
                tp->set_sibling(tn);
            else
                hcxTopList = tn;
            t->set_sibling(0);
            return (true);
        }
        tp = t;
    }
    return (false);
}


HLPtopic *
HLPcontext::findUrlTopic(const char *url)
{
    for (HLPtopic *t = hcxTopList; t; t = t->sibling()) {
        if (t->target() && !strcmp(t->target(), url))
            return (t);
    }
    return (0);
}


HLPtopic *
HLPcontext::findKeywordTopic(const char *kw)
{
    for (HLPtopic *t = hcxTopList; t; t = t->sibling()) {
        if (!strcmp(t->keyword(), kw))
            return (t);
    }
    return (0);
}


// Return true if word matches a keyword of a displayed topic in the
// top list.
//
bool
HLPcontext::isCurrent(const char *word)
{
    for (HLPtopic *t = hcxTopList; t; t = t->sibling()) {
        HLPtopic *last = HLPtopic::get_last(t);
        if (!strcmp(last->keyword(), word))
            return (true);
    }
    return (false);
}


//-----------------------------------------------------------------------------
// Resolve Keyword/URL

// Create a topic from the given keyword, in hrefin.  The keyword
// might be a help database keyword or normal url.  The topic is
// returned in ptop.  If hanchor points to a buffer, any '#' extension
// will be copied there.  If true is returned, the caller should
// terminate the operation, as handling is complete, as for popping up
// a mail client.  If force_download is set, the file will be
// downloaded, not displayed.  If nonrelative is set, words that start
// with '/' are assumed to be a full path on the present machine and
// not relative to a previous url.
//
// If widg is non-null, it will be used for pop-ups, otherwise the
// viewer widget will be obtained from the existing topic in parent
// (but this argument can be null).  The parent can be the root topic
// or the previous topic.
//
bool
HLPcontext::resolveKeyword(const char *hrefin, HLPtopic **ptop, char *hanchor,
    HelpWidget *widg, HLPtopic *parent, bool force_download, bool nonrelative)
{
    if (hanchor)
        *hanchor = 0;
    *ptop = 0;
    if (!HLP()->using_graphics())
        return (false);
    if (!hrefin)
        return (true);

    HelpWidget *w = widg;
    if (!w)
        w = HelpWidget::get_widget(parent);
    if (w)
        w->unset_halt_flag();

    while (isspace(*hrefin))
        hrefin++;
    if (*hrefin == ':') {
        // Internal special addresses.

        const char *in = hrefin+1;
        const char *t = lstring::gettok(&in, "?");
        if (!strcmp(t, "xt_pkgs")) {
            // Show a page listing installed and available XicTools
            // packages, with option for downloading packages.

            char *html = pkgs::pkgs_page();
            HLPtopic *top = new HLPtopic(lstring::copy(hrefin), "");
            top->get_string(html);
            delete [] html;
            delete [] t;
            *ptop = top;
            return (false);
        }
        else if (!strcmp(t, "xt_download")) {
            // Action procedure for the form above, takes care of
            // downloading.

            const char *qin = strchr(hrefin, '?');
            if (!qin)
                return (true);
            qin++;
            stringlist *dpkg = 0;
            char *tok;

            // If the "Download and Install" submit button was pressed,
            // prepend wr_install, and install after downloading.

            unsigned int install = 0;
            while ((tok = lstring::gettok(&qin, "&=")) != 0) {
                if (lstring::prefix("d_", tok))
                    dpkg = new stringlist(lstring::copy(tok+2), dpkg);
                else if (!strcmp(tok, "Download+and+Install"))
                    install = 1;
                else if (!strcmp(tok, "Download+and+Test+Install"))
                    install = 2;
                delete [] tok;
            }
            if (dpkg) {
                if (install)
                    dpkg = new stringlist(lstring::copy("wr_install"), dpkg);
                for (stringlist *sl = dpkg; sl; sl = sl->next) {
                    char *url = pkgs::get_download_url(sl->string);
                    delete [] sl->string;
                    sl->string = url;
                }
                download(w, dpkg, install);
            }
            return (true);
        }
        else if (!strcmp(t, "xt_avail")) {
            // Show a page listing the available packages.

            char *html = pkgs::list_avail_pkgs();
            HLPtopic *top = new HLPtopic(lstring::copy(hrefin), "");
            top->get_string(html);
            delete [] html;
            delete [] t;
            *ptop = top;
            return (false);
        }
        else if (!strcmp(t, "xt_local")) {
            // Show a page listing the locally installed packages.

            char *html = pkgs::list_cur_pkgs();
            HLPtopic *top = new HLPtopic(lstring::copy(hrefin), "");
            top->get_string(html);
            delete [] html;
            delete [] t;
            *ptop = top;
            return (false);
        }
        delete [] t;
    }
    if (*hrefin == '+' && nonrelative) {
        // magic character '+' at front of keyword will cause relative
        // interpretation
        nonrelative = false;
        hrefin++;
    }

    const char *addr;
    char *protocol = getProtocol(hrefin, &addr);
    if (protocol) {
        if (lstring::cieq(protocol, "mailto")) {
            if (*addr) {
                if (w && w->get_widget_bag())
                    w->get_widget_bag()->PopUpMail("", addr, 0,
                        GRloc(LW_XYR, 50, 50));
                else if (GRpkg::self()->MainWbag())
                    GRpkg::self()->MainWbag()->PopUpMail("", addr);
                else
                    GRpkg::self()->ErrPrintf(ET_WARN,
                        "can't do mailto, no graphics.");
            }
            delete [] protocol;
            return (true);
        }
        if (lstring::cieq(protocol, "file")) {
            while (*addr == '/' && *(addr+1) == '/')
                addr++;
            hrefin = addr;
        }
        else if (!lstring::cieq(protocol, "http") &&
                !lstring::cieq(protocol, "ftp") && *addr == '/') {
            char buf[128];
            snprintf(buf, sizeof(buf), "Can't handle %s protocol.", protocol);
            if (w && w->get_widget_bag())
                w->get_widget_bag()->PopUpErr(MODE_ON, buf);
            else if (GRpkg::self()->MainWbag())
                GRpkg::self()->MainWbag()->PopUpErr(MODE_ON, buf);
            else
                GRpkg::self()->ErrPrintf(ET_ERROR, buf);
            delete [] protocol;
            return (true);
        }
        delete [] protocol;
    }

    char *href = lstring::copy(hrefin);
    href = GRappIf()->ExpandHelpInput(href);
    href = hrefExpand(href);
    {
        const char *a = HLP()->alias(href);
        if (a) {
            delete [] href;
            href = lstring::copy(a);
        }
    }

    // strip any named anchor reference
    char *anchor = 0;
    {
        char *t = findAnchorRef(href);
        if (t) {
            anchor = lstring::copy(t);
            *t = 0;
        }
    }

    HLPtopic *last = HLPtopic::get_last(parent);

    if (!force_download) {
        char *t = strrchr(href, '.');
        if (t && !strchr(t, '/') && dl_suffix(t+1))
            force_download = true;
    }

    char *url = urlcat((!nonrelative && last) ? last->keyword() : 0, href);
    if (force_download && isProtocol(url)) {
        download(w, url);
        delete [] href;
        delete [] url;
        delete [] anchor;
        return (true);
    }

    if (GRappIf()->ApplyHelpInput(href)) {
        // This callback allows the application to do something
        // special with this file.  If true is returned, the
        // application did its thing and we're done
        delete [] href;
        delete [] url;
        delete [] anchor;
        return (true);
    }

    if (href && *href)
        *ptop = HLP()->read(href);
    if (!*ptop)
        *ptop = findFile(url, parent, nonrelative);
    if (!*ptop && anchor && !*href)
        *ptop = HLP()->read(url);
    delete [] href;

    // if there was an anchor, add it back to the topic url
    if (*ptop && anchor) {
        char *tmp = new char [strlen((*ptop)->keyword()) + strlen(anchor) + 1];
        strcpy(tmp, (*ptop)->keyword());
        strcat(tmp, anchor);
        (*ptop)->set_keyword(tmp);
    }
    if (hanchor && anchor)
        strcpy(hanchor, anchor);
    delete [] anchor;
    delete [] url;
    return (false);
}


//-----------------------------------------------------------------------------
// Network Access

namespace {
    // This is the "tputs" callback from the Transaction package, which gets
    // called periodically with updates during the transfer.  This updates
    // the label with the transfer info, services events, and checks if
    // the transfer should be aborted.  If aborting (stop button pressed),
    // return true to accomplish the abort.
    //
    bool
    http_puts(void *data, const char *s)
    {
        Transaction *t = (Transaction*)data;
        if (!t)
            return (false);
        ViewerWidget *w = (ViewerWidget*)t->user_data1();

        if (s) {
            while (isspace(*s))
                s++;
        }
        if (s && *s) {

            // strip trailing white space (newline)
            char *ss = lstring::copy(s);
            char *e = ss + strlen(ss) - 1;
            while (isspace(*e))
                *e-- = 0;

            const char *f = strrchr(t->url(), '/');
            if (!f)
                f = t->url();
            else
                f++;

            char buf[256];
            strncpy(buf, f, 22);
            if (strlen(f) > 22)
                strcpy(buf + 22, "...");
            else
                buf[22] = 0;
            e = buf + strlen(buf);
            *e++ = ':';
            *e++ = ' ';
            strncpy(e, ss, 60);
            if (strlen(ss) > 60)
                 strcpy(e + 60, "...");
            else
                e[60] = 0;
            if (w)
                w->set_status_line(buf);
            delete [] ss;

            // call progressive image loader
            if (w && w->image_load_mode() == HLPparams::LoadProgressive) {
                HLPimageList *im = (HLPimageList*)t->user_data2();
                if (im && HLP()->context()->imageList()) {
                    w->call_plc(im->orig_url);
                    if (t->response()->read_complete && im->start_prog)
                        while (w->call_plc(im->orig_url)) ;
                }
            }
        }

        if (w && w->check_halt_processing(true))
            return (true);

        return (false);
    }


    // Error reporting from the transaction package.
    //
    void
    http_err_cb(Transaction *t, const char *msg)
    {
        ViewerWidget *w = (ViewerWidget*)t->user_data1();
        if (w && w->get_widget_bag())
            w->get_widget_bag()->PopUpErr(MODE_ON, msg);
        else if (GRpkg::self()->MainWbag())
            GRpkg::self()->MainWbag()->PopUpErr(MODE_ON, msg);
        else
            GRpkg::self()->ErrPrintf(ET_ERROR, msg);
    }


    // Return true of the url has a user/password.
    //
    bool has_pw(const char *url)
    {
        if (lstring::ciprefix("http:", url) ||
                lstring::ciprefix("ftp:", url)) {
            const char *t = url;
            while (*t != ':')
                t++;
            while (*t == ':' || *t == '/')
                t++;
            const char *a = strchr(t, '@');
            const char *c = strchr(t, ':');
            if (a && c && c < a)
                return (true);
        }
        return (false);
    }
}


// This function uses the Transaction package to download the file.  A file
// pointer to the temporary file containing the data is returned, as is
// its name in the first argument, if one was created.  This might be
// partial data if the user pressed the stop button.
//
// The url arg must point to free store, and the existing string will be
// deleted if the return value changes.  The fname arg, if given, must
// also point to free store, and any existing name will be deleted when
// the filename is returned.  Both returned values are copies.
//
FILE *
HLPcontext::httpRetrieve(char **fname, char **url, char *args,
    ViewerWidget *w, bool force_dl, HLPimageList *imlist)
{
    time_t mtime = 0;
    HLPcacheEnt *cent = 0;
    // Don't cache password-protected pages.  The would put the password
    // in the cache as plain text.
    bool nc = no_cache(w) || has_pw(*url);
    for (;;) {
        if (hcxCache) {
            if (!force_dl) {
                cent = hcxCache->get(*url, nc);
                if (!cent) {
                    // hack - try it w/wo trailing '/'
                    char *t = *url + strlen(*url) - 1;
                    char *str;
                    if (*t != '/') {
                        str = new char [strlen(*url) + 2];
                        strcpy(str, *url);
                        strcat(str, "/");
                    }
                    else {
                        str = lstring::copy(*url);
                        str[strlen(str) - 1] = 0;
                    }
                    cent = hcxCache->get(str, nc);
                    if (cent) {
                        delete [] *url;
                        *url = str;
                    }
                    else
                        delete [] str;
                }
                if (cent) {
                    if (cent->get_status() == DLnogo)
                        return (0);
                    else {
                        if (fname) {
                            delete [] *fname;
                            *fname = lstring::copy(cent->filename);
                        }
                    }
                    if (cent->get_status() == DLok) {
                        FILE *fp = fopen(cent->filename, "r");
                        if (fp) {
                            if (checkLocation(url, fp, w)) {
                                fclose(fp);
                                continue;
                            }
                            fclose(fp);
                            struct stat st;
                            if (!stat(cent->filename, &st))
                                mtime = st.st_mtime;
                        }
                        else
                            hcxCache->set_complete(cent, DLincomplete);
                    }
                }
            }
        }
        else
            hcxCache = new HLPcache;
        break;
    }

    if (w) {
        if (w->check_halt_processing(false))
            return (0);
        w->set_halt_proc_sens(true);
    }

    int ret;
    do {
        if (!cent) {
            cent = hcxCache->add(*url, nc);
            if (fname) {
                delete [] *fname;
                *fname = lstring::copy(cent->filename);
            }
        }
        sLstr cmd;
        cmd.add("httpget -o ");
        cmd.add(cent->filename);
        cmd.add(" -e ");
        if (args) {
            cmd.add(args);
            cmd.add_c(' ');
        }

        // Add a proxy if set,  The update system provides this.
        char *pxy = proxy::get_proxy();
        if (pxy) {
            cmd.add("-p ");
            cmd.add(pxy);
            cmd.add_c(' ');
            delete [] pxy;
        }
        cmd.add(*url);

        ret = 0;
        {
            Transaction t;
            t.set_err_func(http_err_cb);
            t.set_user_data1(w);
            t.set_user_data2(imlist);
            t.parse_cmd(cmd.string(), 0);
            t.set_puts(http_puts);
            if (imlist)
                t.set_return_mode(HTTP_FileAndBuf);
            if (w) {
                hcxImageQueueLoop.suspend();
                w->set_transaction(&t, hcxCache->dir_name());
                if (mtime > 0)
                    t.set_cachetime(mtime);
            }
            ret = t.transact();
            if (w) {
                w->set_transaction(0, 0);
                hcxImageQueueLoop.resume();
            }
        }

        if (ret == 0) {
            // transact successful, but can be relocation
            hcxCache->set_complete(cent, DLok);
            int rcnt = 0;
            for (;;) {
                FILE *fp = fopen(cent->filename, "r");
                if (!fp)
                    hcxCache->set_complete(cent,
                        rcnt ? DLincomplete : DLnogo);
                else {
                    bool reloc = checkLocation(url, fp, w);
                    fclose(fp);
                    if (reloc) {
                        cent = hcxCache->get(*url, nc);
                        if (cent) {
                            if (fname) {
                                delete [] *fname;
                                *fname = lstring::copy(cent->filename);
                            }
                            if (cent->get_status() == DLok) {
                                if (rcnt++ < 20)
                                    continue;
                            }
                        }
                    }
                }
                break;
            }
        }
    } while (!ret && (!cent || cent->get_status() == DLincomplete));

    FILE *fp = 0;
    if (ret == 0 || ret >= 100) {
        // ok, got something back
        if (cent->get_status() != DLnogo) {
            fp = fopen(cent->filename, "r");
            hcxCache->set_complete(cent, fp ? DLok : DLnogo);
        }
    }
    else if (ret == HTTPBadHost) {
        // maybe not on network, use local file if available
        if (mtime > 0) {
            fp = fopen(cent->filename, "r");
            hcxCache->set_complete(cent, fp ? DLok : DLnogo);
        }
    }
    else if (ret == HTTPBadProtocol || ret == HTTPBadHost ||
            ret == HTTPBadURL) {
        // bad url, not good for anything
        hcxCache->set_complete(cent, DLnogo);
    }
    // Anything else, keep the cache status DLincomplete so we can try
    // again.

    if (w)
        w->set_halt_proc_sens(false);
    return (fp);
}


// Look in fp for the location change info.  If there, change the url and
// return true.
//
bool
HLPcontext::checkLocation(char **url, FILE *fp, ViewerWidget *w)
{
    char buf[URL_BUFSIZE];
    if (fgets(buf, URL_BUFSIZE, fp) != 0) {
        if (lstring::prefix("301 Location", buf) ||
                lstring::prefix("302 Location", buf)) {

            if (lstring::prefix("302", buf)) {
                // temporary relocation, if the url is in cache, invalidate
                if (hcxCache) {
                    HLPcacheEnt *cent = hcxCache->get(*url, no_cache(w));
                    if (cent && cent->get_status() == DLok)
                        hcxCache->set_complete(cent, DLincomplete);
                }
            }

            char *s = buf + 13;
            while (isspace(*s))
                s++;
            delete [] *url;
            *url = lstring::copy(s);
            s = *url + strlen(s) - 1;
            while (s >= *url && isspace(*s))
                *s-- = 0;
            return (true);
        }
    }
    rewind(fp);
    return (false);
}


namespace {
    struct dl_arg
    {
        dl_arg(ViewerWidget *w, const char *u)
            {
                viewer = w;
                url = lstring::copy(u);
            }

        ~dl_arg()
            {
                delete [] url;
            }

        ViewerWidget *viewer;
        char *url;
    };


    // Do the download.
    //
    void dl_cb(const char *filename, void *arg)
    {
        dl_arg *da = (dl_arg*)arg;

        sLstr lstr;
        lstr.add("httpget");  // dummy first arg is ignored
        lstr.add(" -o ");
        lstr.add(filename);
        lstr.add_c(' ');
        // Add a proxy if set,  The update system provides this.
        char *pxy = proxy::get_proxy();
        if (pxy) {
            lstr.add("-p ");
            lstr.add(pxy);
            lstr.add_c(' ');
            delete [] pxy;
        }
        lstr.add(da->url);

        Transaction trn;
        trn.set_err_func(http_err_cb);
        trn.set_user_data1(da->viewer);
        trn.set_puts(http_puts);

        // parse command line
        if (trn.parse_cmd(lstr.string(), 0)) {
            GRpkg::self()->ErrPrintf(ET_WARN, "internal error, parse failed.");
            return;
        }
        trn.transact();
    }


    void dl_down(GRfilePopup*, void *arg)
    {
        dl_arg *da = (dl_arg*)arg;
        delete da;
    }
}


// Pop up the file browser to set the download target, with the
// "ok" button set to initiate the download
//
void
HLPcontext::download(ViewerWidget *w, char *url)
{
    sURL uinfo;
    cComm::http_parse_url(url, 0, &uinfo);
    if (!uinfo.filename)
        uinfo.filename = lstring::copy(DEF_DL_FILE);
    else if (!*uinfo.filename) {
        delete [] uinfo.filename;
        uinfo.filename = lstring::copy(DEF_DL_FILE);
    }
    else {
        char *t = strrchr(uinfo.filename, '/');
        if (t && *(t+1)) {
            t++;
            t = lstring::copy(t);
            delete [] uinfo.filename;
            uinfo.filename = t;
        }
        else {
            delete [] uinfo.filename;
            uinfo.filename = lstring::copy(DEF_DL_FILE);
        }
    }
    GRloc loc(LW_CENTER);
    if (w && w->get_widget_bag()) {

        dl_arg *arg = new dl_arg(w, url);
        w->get_widget_bag()->PopUpFileSelector(fsDOWNLOAD, loc,
            dl_cb, dl_down, arg, uinfo.filename);
    }
    else if (GRpkg::self()->MainWbag()) {

        dl_arg *arg = new dl_arg(0, url);
        GRpkg::self()->MainWbag()->PopUpFileSelector(fsDOWNLOAD, loc,
            dl_cb, dl_down, arg, uinfo.filename);
    }
    else
        GRpkg::self()->ErrPrintf(ET_WARN, "download aborted, no graphics.");
}


namespace {
    // Do the download.
    //
    void dl_list_cb(const char *filename, void *arg)
    {
        dl_elt_t *da0 = (dl_elt_t*)arg;

        char buf[256];
        strcpy(buf, filename);
        char *f = lstring::strrdirsep(buf);
        if (f)
            f++;
        else
            f = buf;
        for (dl_elt_t *da = da0; da; da = da->next) {
            sLstr lstr;
            lstr.add("httpget");  // dummy first arg is ignored
            lstr.add(" -o ");
            int len = strlen(f);
            snprintf(f, sizeof(buf) - len, "%s",
                da->filename ? da->filename : filename);
            lstr.add(buf);
            lstr.add_c(' ');
            // Add a proxy if set,  The update system provides this.
            char *pxy = proxy::get_proxy();
            if (pxy) {
                lstr.add("-p ");
                lstr.add(pxy);
                lstr.add_c(' ');
                delete [] pxy;
            }
            lstr.add(da->url);

            Transaction trn;
            trn.set_err_func(http_err_cb);
            trn.set_user_data1(da->viewer);
            trn.set_puts(http_puts);

            // parse command line
            if (trn.parse_cmd(lstr.string(), 0)) {
                GRpkg::self()->ErrPrintf(ET_WARN,
                    "internal error, parse failed.");
                return;
            }
            trn.transact();

            if (da->filename) {
                // Update the filename in the list to full path, in case
                // any post-download processing is done on the files.

                delete [] da->filename;
                da->filename = lstring::copy(buf);
            }
        }

        // If this is set, pop down when finished downloading.
        if (da0->fsel)
            da0->fsel->popdown();
    }


    void dl_list_down(GRfilePopup*, void *arg)
    {
        dl_elt_t *da = (dl_elt_t*)arg;
        while (da) {
            dl_elt_t *dx = da;
            da = da->next;
            delete dx;
        }
    }


    void dl_list_down_install(GRfilePopup*, void *arg)
    {
        dl_elt_t *da = (dl_elt_t*)arg;

        // If the first list element is the installer script, assume
        // we are installing (for xictools updater support).

        if (da && da->next) {
            const char *t = lstring::strrdirsep(da->filename);
            if (t)
                t++;
            else
                t = da->filename;
            if (t && !strcmp(t, "wr_install"))
                pkgs::xt_install(da, false);
        }

        while (da) {
            dl_elt_t *dx = da;
            da = da->next;
            delete dx;
        }
    }


    void dl_list_down_test_install(GRfilePopup*, void *arg)
    {
        dl_elt_t *da = (dl_elt_t*)arg;

        // If the first list element is the installer script, assume
        // we are installing (for xictools updater support).

        if (da && da->next) {
            const char *t = lstring::strrdirsep(da->filename);
            if (t)
                t++;
            else
                t = da->filename;
            if (t && !strcmp(t, "wr_install"))
                pkgs::xt_install(da, true);
        }

        while (da) {
            dl_elt_t *dx = da;
            da = da->next;
            delete dx;
        }
    }
}


// Pop up the file browser to set the download target directory, with
// the "ok" button set to initiate the download.  This will download a
// list of files to the chosen directory (default is CWD).  This is
// used by the XicTools package updater.
//
//  install:  0 don't install, 1 do install, else do dry run install.
//
void
HLPcontext::download(ViewerWidget *w, stringlist *list, unsigned int install)
{
    if (!list)
        return;
    GRloc loc(LW_CENTER);
    if (w && !w->get_widget_bag())
        w = 0;
    if (!w && !GRpkg::self()->MainWbag()) {
        GRpkg::self()->ErrPrintf(ET_WARN, "download aborted, no graphics.");
        return;
    }

    dl_elt_t *a0 = 0, *an = 0;
    for (stringlist *sl = list; sl; sl = sl->next) {
        if (!sl->string)
            continue;

        sURL uinfo;
        cComm::http_parse_url(sl->string, 0, &uinfo);
        if (!uinfo.filename)
            uinfo.filename = lstring::copy(DEF_DL_FILE);
        else if (!*uinfo.filename) {
            delete [] uinfo.filename;
            uinfo.filename = lstring::copy(DEF_DL_FILE);
        }
        else {
            char *t = strrchr(uinfo.filename, '/');
            if (t && *(t+1)) {
                t++;
                t = lstring::copy(t);
                delete [] uinfo.filename;
                uinfo.filename = t;
            }
            else {
                delete [] uinfo.filename;
                uinfo.filename = lstring::copy(DEF_DL_FILE);
            }
        }

        dl_elt_t *arg = new dl_elt_t(w, sl->string, uinfo.filename);
        if (!a0)
            a0 = an = arg;
        else {
            an->next = arg;
            an = arg;
        }
    }
    if (a0) {
        // Set fsel to pop down when downloads complete.
        const char *msg = "Press GO to download packages.";
        typedef void(*cbfunc)(GRfilePopup*, void*);
        cbfunc cb;
        if (install == 0)
            cb = &dl_list_down;
        else if (install == 1)
            cb = &dl_list_down_install;
        else
            cb = &dl_list_down_test_install;
        if (w) {
            a0->fsel = w->get_widget_bag()->PopUpFileSelector(fsDOWNLOAD,
                loc, dl_list_cb, cb, a0, msg);
        }
        else {
            a0->fsel = GRpkg::self()->MainWbag()->PopUpFileSelector(fsDOWNLOAD,
                loc, dl_list_cb, cb, a0, msg);
        }
    }
}


//-----------------------------------------------------------------------------
// URL Utilities

// Return true is the file extension indicates a plain-text file.
//
bool
HLPcontext::isPlain(const char *name)
{
    if (!name || !*name)
        return (false);
    bool nonlocal = isProtocol(name);
    if (!nonlocal) {
        struct stat st;
        if (!stat(name, &st) && S_ISDIR(st.st_mode))
            return (false);
    }
    const char *n = lstring::strrdirsep(name);
    const char *ext = strrchr(n ? n : name, '.');
    if (ext) {
        ext++;
        for (const char **s = text_exts; *s; s++)
            if (lstring::cieq(*s, ext))
                return (true);
    }
    if (n) {
        n++;
        if (!ext && !nonlocal)
            return (true);
        if (lstring::prefix("README", n))
            return (true);
    }
    return (false);
}


// Return true if s begins with "xxx://".
//
bool
HLPcontext::isProtocol(const char *s)
{
    if (!s)
        return (false);
    while (isspace(*s))
        s++;
    if (!isalpha(*s))
        return (false);
    for (s++; *s; s++) {
        if (isalpha(*s))
            continue;
        if (*s++ != ':')
            return (false);
        if (*s++ != '/')
            return (false);
        if (*s != '/')
            return (false);
        return (true);
    }
    return (false);
}


// If s begins with xxx:  return xxx (all alpha), and with ret
// pointing to the first char following ':'.
//
char *
HLPcontext::getProtocol(const char *s, const char **ret)
{
    if (!s)
        return (0);
    while (isspace(*s))
        s++;
    if (!isalpha(*s))
        return (0);
#ifdef WIN32
    // This can be a path with a drive specifier!
    if (*(s+1) == ':')
        return (0);
#endif
    const char *s0 = s;
    for (s++; *s; s++) {
        if (isalpha(*s))
            continue;
        if (*s != ':')
            return (0);
        char *p = new char[s-s0 + 1];
        strncpy(p, s0, s-s0);
        p[s-s0] = 0;
        if (ret)
            *ret = s+1;
        return (p);
    }
    return (0);
}


// Do tilde expansion here.  The argument is freed if changed.
//
char *
HLPcontext::hrefExpand(char *href)
{
#ifdef HAVE_PWD_H
    sLstr lstr;
    if (*href == '~') {
        char *t = href + 1;
        while (*t && *t != '/')
            t++;
        if (t - href > 128)
            // too strange
            return (href);
        char *tbf = 0;
        if (t - href > 1) {
            tbf = new char[t - href];
            strncpy(tbf, href+1, t-href-1);
            tbf[t-href-1] = 0;
        }
        passwd *pw = 0;
        if (!tbf)
            pw = getpwuid(getuid());
        else {
            pw = getpwnam(tbf);
            delete [] tbf;
        }
        if (pw) {
            char *hr = new char[strlen(pw->pw_dir) + strlen(t) + 1];
            strcpy(hr, pw->pw_dir);
            strcat(hr, t);
            delete [] href;
            href = hr;
        }
    }
#endif
    return (href);
}


// Return the start of the anchor reference, if any.
//
const char *
HLPcontext::findAnchorRef(const char *href)
{
    if (href) {
        const char *t = href + strlen(href) - 1;
        while (t >= href && !lstring::is_dirsep(*t))
            t--;
        if (t < href)
            t = href;
        const char *a = strrchr(t, '#');
        if (a)
            return (a);
    }
    return (0);
}


// Return the start of the anchor reference, if any.
//
char *
HLPcontext::findAnchorRef(char *href)
{
    char *t = href + strlen(href) - 1;
    while (t >= href && !lstring::is_dirsep(*t))
        t--;
    if (t < href)
        t = href;
    char *a = strrchr(t, '#');
    if (a)
        return (a);
    return (0);
}


// Return the appropriate full url or path, given the current page and
// reference.  The named anchor reference has been stripped, thus ref
// can be empty in this case.
//
char *
HLPcontext::urlcat(const char *pagein, const char *ref)
{
    char *page = 0;
    if (pagein) {
        page = lstring::copy(pagein);
        char *t = findAnchorRef(page);
        if (t)
            *t = 0;
    }
    if (!ref || !*ref)
        return (page);

    const char *refaddr;
    char *refproto = getProtocol(ref, &refaddr);
    if (refproto) {
        if (lstring::cieq(refproto, "file")) {
            while (*refaddr == '/' && *(refaddr+1) == '/')
                refaddr++;
            delete [] page;
            delete [] refproto;
            return (lstring::copy(refaddr));
        }
        if (*refaddr == '/' && *(refaddr+1) == '/') {
            delete [] page;
            delete [] refproto;
            return (lstring::copy(ref));
        }
        // relative url
        if (*refaddr == '/')
            refaddr++;
        ref = refaddr;
        delete [] refproto;
    }
    if (isProtocol(page)) {
        char *url = new char[strlen(page) + strlen(ref) + 2];
        strcpy(url, page);

        char *b = strchr(url + 8, '/');
        if (!b) {
            b = url + strlen(url);
            *b = '/';
            *(b+1) = 0;
        }
        else {
            while (*(b-1) == '/')
                b--;
        }
        if (*ref == '/') {
            while (*(ref+1) == '/')
                ref++;
            strcpy(b+1, ref+1);
            delete [] page;
            return (url);
        }

        char *t = strrchr(b, '/');
        while (*(t-1) == '/')
            t--;
        *(t+1) = 0;
        while (*ref == '.' &&
                (*(ref+1) == '/' || (*(ref+1) == '.' && *(ref+2) == '/'))) {
            if (*(ref+1) == '/')
                ref += 2;
            else {
                if (t != b) {
                    *t = 0;
                    t = strrchr(b, '/');
                    while (*(t-1) == '/')
                        t--;
                    *(t+1) = 0;
                    ref += 3;
                }
                else
                    break;
            }
            while (*ref == '/')
                ref++;
        }
        strcat(url, ref);
        delete [] page;
        return (url);
    }
    if (*ref == '/') {
        while (*(ref+1) == '/')
            ref++;
        delete [] page;
        return (lstring::copy(ref));
    }
    if (page && *page == '/') {
        while (*(page+1) == '/')
            page++;
        char *url = new char [strlen(page) + strlen(ref) + 2];
        strcpy(url, page);
        char *t = strrchr(url, '/');
        while (t > url && *(t-1) == '/')
            t--;
        *(t+1) = 0;
        while (*ref == '.' &&
                (*(ref+1) == '/' || (*(ref+1) == '.' && *(ref+2) == '/'))) {
            if (*(ref+1) == '/')
                ref += 2;
            else {
                if (t != url) {
                    *t = 0;
                    t = strrchr(url, '/');
                    while (t > url && *(t-1) == '/')
                        t--;
                    *(t+1) = 0;
                }
                ref += 3;
            }
            while (*ref == '/')
                ref++;
        }
        strcat(url, ref);
        delete [] page;
        return (url);
    }
    delete [] page;
    return (lstring::copy(ref));
}


struct ali_t
{
    ali_t(char *na, char *al, ali_t *nx)
        {
            name = na;
            alias = al;
            next = nx;
        }

    ~ali_t()
        {
            delete [] name;
            delete [] alias;
        }

    char *name;
    char *alias;
    ali_t *next;
};


// Add a "root alias", these provide local file redirection.
// Suppose html text being displayed contains
//  <img src=/images/pict.jpg ....
// and the startup file contains
//  Alias /image /real/path/to/images
// Then the file sought will be
//  /real/path/to/images/pict.jpg
//
void
HLPcontext::setRootAlias(const char *str)
{
    const char *sepchars = " \t=,";
    char *name = lstring::getqtok(&str, sepchars);
    char *alias = lstring::getqtok(&str, sepchars);
    if (name && alias) {
        for (ali_t *al = hcxRootAlii; al; al = al->next) {
            if (!strcmp(al->name, name)) {
                delete [] name;
                delete [] al->alias;
                al->alias = alias;
                return;
            }
        }
        hcxRootAlii = new ali_t(name, alias, hcxRootAlii);
    }
    else
        delete [] name;
}


int
HLPcontext::dumpRootAliases(FILE *fp)
{
    int cnt = 0;
    for (ali_t *al = hcxRootAlii; al; al = al->next) {
        fprintf(fp, "Alias %s %s\n", al->name, al->alias);
        cnt++;
    }
    return (cnt);
}


// If a prefix of the url is in the alias list, return the substitution
// url.
//
char *
HLPcontext::rootAlias(char *url)
{
    for (ali_t *al = hcxRootAlii; al; al = al->next) {
        if (lstring::prefix(al->name, url) &&
                lstring::is_dirsep(url[strlen(al->name)])) {
            char *s = new char[strlen(url) + strlen(al->alias) -
                strlen(al->name) + 1];
            char *t = lstring::stpcpy(s, al->alias);
            strcpy(t, url + strlen(al->name));
            return (s);
        }
    }
    return (url);
}


// Return a topic pointer to the given file, or perform an http
// transfer and return a topic pointer to a temporary file.  If
// nonrelative is set, assume fname is not relative to prior url.
//
HLPtopic *
HLPcontext::findFile(const char *fname, HLPtopic *parent, bool nonrelative)
{
    if (!fname)
        return (0);

    HLPtopic *last = HLPtopic::get_last(parent);
    char *url = urlcat((!nonrelative && last) ? last->keyword() : 0, fname);
    HelpWidget *w = HelpWidget::get_widget(parent);

    if (isProtocol(url)) {
        HLPtopic *top = checkImage(url, w);
        if (top) {
            delete [] url;
            return (top);
        }
        FILE *fp = httpRetrieve(0, &url, 0, w, false);
        if (fp) {
            top = checkAuth(fp, url);
            if (!top) {
                top = new HLPtopic(url, "");
                top->get_file(fp, 0);
            }
            fclose(fp);
            delete [] url;
            return (top);
        }
        delete [] url;
        return (0);
    }
    if (lstring::is_rooted(url)) {
        url = rootAlias(url);
        FILE *fp = fopen(url, "rb");
        if (fp) {
            HLPtopic *top = checkImage(url, w);
            if (top) {
                fclose(fp);
                delete [] url;
                return (top);
            }
            top = new HLPtopic(url, "");
            top->get_file(fp, url);
            fclose(fp);
            delete [] url;
            return (top);
        }
#ifdef WIN32
        // fopen returns 0 for directories, thanks Bill
        struct stat st;
        if (!stat(url, &st) && S_ISDIR(st.st_mode)) {
            HLPtopic *top = new HLPtopic(url, "");
            top->get_file(fp, url);
            delete [] url;
            return (top);
        }
#endif
        delete [] url;
        return (0);
    }

    char *s = findFileInPath(url);
    if (s) {
        FILE *fp = fopen(s, "rb");
        if (fp) {
            // don't include path
            HLPtopic *top = checkImage(url, w);
            if (top) {
                fclose(fp);
                delete [] url;
                delete [] s;
                return (top);
            }
            top = new HLPtopic(url, "");
            top->get_file(fp, s);
            fclose(fp);
            delete [] url;
            delete [] s;
            return (top);
        }
        delete [] s;
    }

    // still not found, take a last look in the CWD
    FILE *fp = fopen(url, "rb");
    if (fp) {
        char *cwd = getcwd(0, 0);
        char *ptmp = pathlist::mk_path(cwd, url);
        free(cwd);
        delete [] url;
        url = ptmp;
        HLPtopic *top = checkImage(url, w);
        if (top) {
            fclose(fp);
            delete [] url;
            return (top);
        }
        top = new HLPtopic(url, "");
        top->get_file(fp, url);
        fclose(fp);
        delete [] url;
        return (top);
    }
#ifdef WIN32
    // fopen returns 0 for directories, thanks Bill
    struct stat st;
    if (!stat(url, &st) && S_ISDIR(st.st_mode)) {
        HLPtopic *top = new HLPtopic(url, "");
        top->get_file(fp, url);
        delete [] url;
        return (top);
    }
#endif
    delete [] url;
    return (0);
}


// Return the full path for the file, found along the help search
// path, or null if not found.
//
char *
HLPcontext::findFileInPath(const char *name)
{
    char *path;
    if (HLP()->get_path(&path) && path) {
        pathgen pg(path);
        delete [] path;
        char *p;
        while ((p = pg.nextpath(false)) != 0) {
            char *ptmp = pathlist::mk_path(p, name);
            delete [] p;
            p = ptmp;

            // ignore directories here
            struct stat st;
            if (stat(p, &st) == 0 && (S_ISREG(st.st_mode)
#ifdef S_IFLNK
                    || S_ISLNK(st.st_mode)
#endif
                    ))
                return (p);
            delete [] p;
        }
    }
    return (0);
}


// The local password solicitation form for basic authenication
//
namespace { const char *pw_form = "\
<HTML>\n\
<BODY TEXT=\"%s\" BGCOLOR=\"%s\">\n\
<FORM method=\"post\" action=\"action_local_passwd\">\n\
<H2>Authorization Required</H2>\n\
Please enter user name and password:\n\
<p>\n\
<table border=0 cellspacing=3>\n\
<tr><td>Username:</td><td><input type=\"text\" name=\"username\"></td></tr>\n\
<tr><td>Password:</td><td><input type=\"password\" name=\"password\"></td></tr>\n\
</table>\n\
<HR>\n\
<INPUT TYPE=\"submit\" VALUE=\"send\"><INPUT TYPE=\"reset\">\n\
</FORM>\n\
</BODY>\n\
</HTML>"; }


// Look for the unauthorized signature in the returned file, and return
// a topic containing the authorization form if found.
//
HLPtopic *
HLPcontext::checkAuth(FILE *fp, char *url)
{
    char buf[256];
    if (fgets(buf, 256, fp)) {
        if (lstring::prefix("401 Unauthorized", buf)) {
            // current page is unauthorized
            HLPtopic *top = new HLPtopic(url, "");
            int len = strlen(pw_form) + 30;
            char *s = new char[len];
            snprintf(s, len, pw_form, htmColorManager::cm_defcolors.DefFgText,
                hcxBGcolor);
            top->get_string(s);
            delete [] s;
            return (top);
        }
    }
    rewind(fp);
    return (0);
}

//-----------------------------------------------------------------------------
// Bookmark Maintenance

// The following functions maintain the Bookmarks menu and file.  The
// "Add" entry in the menu will add the current url/keyword to the end
// of the list.  Bookmarks are saved in the "bookmarks" file in the
// cache directory, format is url title\n for each entry.  The menu
// shows the first 32 chars of the title.

// Read the bookmarks file and create the in-memory bookmarks list.
//
void
HLPcontext::readBookmarks()
{
    if (hcxCache) {
        HLPbookMark::destroy(hcxBookmarks);
        hcxBookmarks = 0;
        char buf[URL_BUFSIZE];
        snprintf(buf, sizeof(buf), "%s/bookmarks", hcxCache->dir_name());
        FILE *fp = fopen(buf, "r");
        if (!fp)
            return;
        while (fgets(buf, URL_BUFSIZE, fp) != 0) {
            char *s = buf;
            char *url = lstring::gettok(&s);
            char *title = lstring::copy(s);
            s = strchr(title, '\n');
            if (s)
                *s = 0;

            if (*title && url) {
                if (!hcxBookmarks)
                    hcxBookmarks = new HLPbookMark(url, title);
                else {
                    HLPbookMark *b = hcxBookmarks;
                    while (b->next)
                        b = b->next;
                    b->next = new HLPbookMark(url, title);
                }
            }
            else {
                delete [] title;
                delete [] url;
            }
        }
        fclose(fp);
    }
}


// Add the url/title to the bookmarks file and list, if title and url
// are both non-nil, and return the new bookmark item.  If title is
// nil, remove the line for url from the file, and unlink and return
// the corresponding item.
//
HLPbookMark *
HLPcontext::bookmarkUpdate(const char *title, const char *url)
{
    if (hcxCache) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s/bookmarks", hcxCache->dir_name());

        if (title) {
            FILE *fp = fopen(buf, "a");
            if (!fp)
                return (0);
            fputs(url, fp);
            putc(' ', fp);
            fputs(title, fp);
            putc('\n', fp);
            fclose(fp);
            HLPbookMark *b =
                new HLPbookMark(lstring::copy(url), lstring::copy(title));
            if (!hcxBookmarks)
                hcxBookmarks = b;
            else {
                HLPbookMark *bx = hcxBookmarks;
                while (bx->next)
                    bx = bx->next;
                bx->next = b;
            }
            return (b);
        }
        else {
            FILE *fp = fopen(buf, "r");
            if (!fp)
                return (0);
            fseek(fp, 0, SEEK_END);
            int len = ftell(fp);
            char *s = new char[len+1];
            rewind(fp);
            fread(s, 1, len, fp);
            fclose(fp);
            s[len] = 0;
            fp = fopen(buf, "w");
            if (!fp) {
                delete [] s;
                return (0);
            }
            char *t = s;
            while (*t) {
                char *e = strchr(t, '\n');
                if (!e)
                    e = t + strlen(t);
                if (!(lstring::prefix(url, t) && isspace(t[strlen(url)]))) {
                    fwrite(t, 1, e - t, fp);
                    putc('\n', fp);
                }
                if (!e)
                    break;
                t = e+1;
            }
            delete [] s;
            fclose(fp);

            HLPbookMark *bp = 0, *bn;
            for (HLPbookMark *b = hcxBookmarks; b; b = bn) {
                bn = b->next;
                if (!strcmp(b->url, url)) {
                    if (bp)
                        bp->next = bn;
                    else
                        hcxBookmarks = bn;
                    return (b);
                }
                bp = b;
            }
        }
    }
    return (0);
}


//-----------------------------------------------------------------------------
// Forms Processing

// Handle the "submit" request for an html form.  The form return is
// always downloaded and never taken from the cache, since this
// prevents multiple submissions of the same form.
//
void
HLPcontext::formProcess(htmFormCallbackStruct *cbs, HelpWidget *w)
{
    HLPtopic *newtop = 0;
    if (!strcmp(cbs->action, "action_local_passwd")) {
        // response from our password form
        char *username = 0, *password = 0;
        for (int i = 0; i < cbs->ncomponents; i++) {
            if (!cbs->components[i].name)
                break;
            if (!strcmp(cbs->components[i].name, "username"))
                username = lstring::copy(cbs->components[i].value);
            else if (!strcmp(cbs->components[i].name, "password"))
                password = lstring::copy(cbs->components[i].value);
            if (username && password) {
                char *url = lstring::copy(w->get_url());
                if (lstring::ciprefix("http:", url) ||
                        lstring::ciprefix("ftp:", url)) {
                    sLstr lstr;
                    const char *u = url;
                    while (*u != ':') {
                        lstr.add_c(*u);
                        u++;
                    }
                    while (*u == ':' || *u == '/') {
                        lstr.add_c(*u);
                        u++;
                    }
                    lstr.add(username);
                    lstr.add_c(':');
                    lstr.add(password);
                    lstr.add_c('@');
                    lstr.add(u);
                    delete [] url;
                    url = lstr.string_trim();

                    FILE *fp = httpRetrieve(0, &url, 0, w, true);
                    if (fp) {
                        newtop = checkAuth(fp, url);
                        if (!newtop) {
                            newtop = new HLPtopic(url, "");
                            newtop->get_file(fp, 0);
                            hcxAuthList = new HLPauthList(url,
                                username, password, hcxAuthList);
                        }
                        fclose(fp);
                    }
                }
                delete [] url;
                break;
            }
        }
        delete [] username;
        delete [] password;
    }
    else if (!strncmp(cbs->action, "action_local_", strlen("action_local_"))) {
        submit(cbs);
        return;
    }
    else {
        if (cbs->method == HTM_FORM_POST) {
            char *formfname = filestat::make_temp("ftmp");
            char buf[128];
            snprintf(buf, sizeof(buf), "-q %s", formfname);
            char *args = buf;
            FILE *fp = fopen(formfname, "w");
            if (!fp) {
                snprintf(buf, sizeof(buf), "Can't create temp file %s.",
                    formfname);
                if (w->get_widget_bag())
                    w->get_widget_bag()->PopUpErr(MODE_ON, buf);
                else if (GRpkg::self()->MainWbag())
                    GRpkg::self()->MainWbag()->PopUpErr(MODE_ON, buf);
                else
                    GRpkg::self()->ErrPrintf(ET_ERROR, buf);
                delete [] formfname;
                return;
            }
            for (int i = 0; i < cbs->ncomponents; i++) {
                if (cbs->components[i].type == FORM_SUBMIT)
                    continue;
                fprintf(fp, "%s=", cbs->components[i].name);
                const char *value = cbs->components[i].value;
                if (value) {
                    int len = strlen(value);
                    int cnt = 0;
                    const char *t = strchr(value, '"');
                    while (t) {
                        cnt++;
                        t++;
                        t = strchr(t, '"');
                    }
                    char *tbf = new char [len + cnt + 4];
                    char *s = tbf;
                    t = value;
                    *s++ = '"';
                    while (*t) {
                        if (*t == '"')
                            *s++ = '\\';
                        *s++ = *t++;
                    }
                    *s++ = '"';
                    *s = 0;
                    fprintf(fp, "%s\n", tbf);
                    delete [] tbf;
                }
                else
                    fprintf(fp, "\"\"\n");
            }
            fclose(fp);

            char *url = urlcat(w->get_url(), cbs->action);
            if (isProtocol(url)) {
                fp = httpRetrieve(0, &url, args, w, true);
                if (fp) {
                    newtop = new HLPtopic(url, "");
                    newtop->get_file(fp, 0);
                    fclose(fp);
                }
            }
            else {
                snprintf(buf, sizeof(buf), "%s < %s", url, formfname);
                fp = popen(buf, "r");
                if (fp) {
                    newtop = new HLPtopic(url, "");
                    newtop->get_file(fp, 0);
                    pclose(fp);
                }
            }
            unlink(formfname);
            delete formfname;
            delete [] url;
        }
        else if (cbs->method == HTM_FORM_GET) {

            sLstr lstr;
            char *url = urlcat(w->get_url(), cbs->action);
            lstr.add(url);
            bool nonlocal = isProtocol(url);
            delete [] url;

            HTTPNamedValues *nv = new HTTPNamedValues[cbs->ncomponents + 1];
            for (int i = 0; i < cbs->ncomponents; i++) {
                nv[i].name = lstring::copy(cbs->components[i].name);
                const char *v = cbs->components[i].value;
                char *tv = 0;
                if (v) {
                    if (*v == '"') {
                        v++;
                        tv = lstring::copy(v);
                        char *t = tv + strlen(tv) - 1;
                        if (*t == '"')
                            *t = 0;
                    }
                    else
                        tv = lstring::copy(v);
                }
                nv[i].value = tv;
            }
            char *qstr = cComm::http_query_string(nv);
            delete [] nv;

            if (nonlocal) {
                lstr.add_c('?');
                lstr.add(qstr);
                delete [] qstr;
                url = lstr.string_trim();

                FILE *fp = httpRetrieve(0, &url, 0, w, true);
                if (fp) {
                    newtop = new HLPtopic(url, "");
                    newtop->get_file(fp, 0);
                    fclose(fp);
                }
            }
            else if (*lstr.string() == ':') {
                // Internal, handle in resolveKeyword.
                lstr.add_c('?');
                lstr.add(qstr);
                delete [] qstr;
                url = lstr.string_trim();

                resolveKeyword(url, &newtop, 0, w, 0, false, true);
            }
            else {
                // Assume local script, pass args normally.
                for (char *s = qstr; *s; s++) {
                    if (*s == '&')
                        *s = ' ';
                }
                lstr.add_c(' ');
                lstr.add(qstr);
                delete [] qstr;
                url = lstr.string_trim();

                FILE *fp = popen(url, "r");
                if (fp) {
                    newtop = new HLPtopic(url, "");
                    newtop->get_file(fp, 0);
                    pclose(fp);
                }
            }
            delete [] url;
        }
        else {
            if (w->get_widget_bag())
                w->get_widget_bag()->PopUpErr(MODE_ON,
                    "Unsupported method in form action.");
            else if (GRpkg::self()->MainWbag())
                GRpkg::self()->MainWbag()->PopUpErr(MODE_ON,
                    "Unsupported method in form action.");
            else
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "Unsupported method in form action.");
        }
    }
    w->reuse(newtop, true);
}

