
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Help System Files                                                      *
 *                                                                        *
 *========================================================================*
 $Id: help_topic.cc,v 1.4 2015/06/11 05:54:04 stevew Exp $
 *========================================================================*/

#include "help_defs.h"
#include "help_context.h"
#include "help_cache.h"
#include "help_topic.h"
#include "htm/htm_widget.h"
#include "lstring.h"
#include "pathlist.h"
#include "filestat.h"

#include <algorithm>


// Static window counter for positioning.
int HLPtopic::tp_wincount = 0;

// Destroy this topic and its descendents.
//
void
HLPtopic::free()
{
    {
        HLPtopic *ht = this;
        if (!ht)
            return;
    }
    HLPtopic *tn;
    for (HLPtopic *t = tp_lastborn; t; t = tn) {
        tn = t->tp_sibling;
        t->free();
    }
    for (HLPtopic *t = tp_next; t; t = tn) {
        tn = t->tp_sibling;
        t->free();
    }
    delete this;
}


// Display the text for the topic, popping up a new window if
// necessary.  Returns true if ok, false on error.
//
bool
HLPtopic::show_in_window()
{
    if (HLP()->context()->isCurrent(tp_keyword)) {
        // Topic is already on display, delete this and return.
        delete this;
        return (true);
    }
    link_new_and_show(HLP()->hlp_multiwin, 0);
    return (true);
}


// If spawn is true, add this to the TopList and pop up a new window,
// otherwise add this to the parent's lastborn list and reuse the
// parent's window.
//
void
HLPtopic::link_new_and_show(bool spawn, HLPtopic *oldtop)
{
    if (!oldtop)
        oldtop = HLP()->context()->topList();

    if (!spawn && oldtop) {
        // Reuse present window.
        HLPtopic *last = oldtop->get_last();
        if (!strcmp(last->tp_keyword, tp_keyword)) {
            last->set_words(get_words());
            clear_words();
            HLPtopic *top = oldtop->get_parent();
            if (top != last)
                top->reuse(last, false);
            else
                top->reuse(0, false);
            delete this;
        }
        else
            oldtop->get_parent()->reuse(this, true);
        return;
    }

    if (tp_context) {
        // There is a window, but it is not displaying anything yet.
        HelpWidget *w = HelpWidget::get_widget(this);
        if (w) {
            w->link_new(this);
            return;
        }
        tp_context = 0;
    }

    // Have to pop up a new window.
    set_position(HLP()->get_init_x(), HLP()->get_init_y());
    HelpWidget *w =
        HelpWidget::new_widget(&tp_context, tp_xposition, tp_yposition);
    w->link_new(this);
}


// Replace the text in the window with that from newtop.  If newlink
// is true, link newtop as the current topic.
//
void
HLPtopic::reuse(HLPtopic *newtop, bool newlink)
{
    HelpWidget *w = HelpWidget::get_widget(this);
    if (w)
        w->reuse(newtop, newlink);
}


// Redisplay the text currently sourced into the viewer.
//
void
HLPtopic::redisplay()
{
    HelpWidget *w = HelpWidget::get_widget(this);
    if (w)
        w->redisplay();
}


// Set or update the position hint for the graphical window.
//
void
HLPtopic::set_position(int init_x, int init_y)
{
    if (tp_wincount > 5)
        tp_wincount = 0;
    if (!tp_parent) {
        tp_xposition = init_x;
        tp_yposition = init_y + tp_wincount *2*Y_INCR;
        tp_wincount++;
    }
    else {
        if (tp_parent->tp_lastborn && tp_parent->tp_lastborn->tp_sibling) {
            tp_xposition = tp_parent->tp_lastborn->tp_sibling->tp_xposition +
                X_INCR;
            tp_yposition = tp_parent->tp_lastborn->tp_sibling->tp_yposition +
                Y_INCR;
        }
        else {
            tp_xposition = tp_parent->tp_xposition + X_INCR;
            tp_yposition = tp_parent->tp_yposition + Y_INCR;
        }
    }
}


// Unlink this from its parent.  The parent pointer is not zeroed in
// case we want to relink.
//
void
HLPtopic::unlink()
{
    // unlink top from the tree
    if (tp_parent) {
        HLPtopic *prev = 0;
        for (HLPtopic *t = tp_parent->tp_lastborn; t; t = t->tp_sibling) {
            if (t == this) {
                if (!prev)
                    tp_parent->tp_lastborn = t->tp_sibling;
                else
                    prev->tp_sibling = t->tp_sibling;
                t->tp_sibling = 0;
                return;
            }
            prev = t;
        }
        prev = 0;
        for (HLPtopic *t = tp_parent->tp_next; t; t = t->tp_sibling) {
            if (t == this) {
                if (!prev)
                    tp_parent->tp_next = t->tp_sibling;
                else
                    prev->tp_sibling = t->tp_sibling;
                t->tp_sibling = 0;
                return;
            }
            prev = t;
        }
    }
}


namespace {
    // Return true if an HTML tag is found in str.
    //
    bool check_html(const char *str)
    {
        if (!str)
            return (false);
        while (isspace(*str))
            str++;
        // This is a bit of a hack, but assume lines that begin with
        // '#' characters are comments.  This prevents the .mozyrc
        // file from being tagged as HTML.

        if (!*str || *str == '#')
            return (false);
        str = strchr(str, '<');
        while (str) {
            str++;
            if (is_html_head(str) || is_html_body(str))
                return (true);
            str = strchr(str, '<');
        }
        return (false);
    }
}


// Grab all the text from fp into the HLPwords list.  This will create
// a directory listing for directories.
//
void
HLPtopic::get_file(FILE *fp, const char *fname)
{
    if (filestat::is_directory(fname)) {
        // We've got a directory, construct a file list.
        int *colwid;
        char *s = pathlist::dir_listing(fname, 0, 72, true, &colwid);
        if (!s)
            return;
        char *t = strchr(s, '\n');
        if (!t)
            return;
        *t++ = 0;
        HLPwords *hw = new HLPwords(s, 0);
        tp_text = hw;
        hw->next = new HLPwords("<hr width=100%><pre>", hw);
        hw = hw->next;
        bool trailslash = false;
        if (lstring::is_dirsep(fname[strlen(fname) - 1]))
            trailslash = true;

        // We need to consider file names that contain spaces, as if
        // the names were quoted.
        sLstr lstr;
        char buf[256];
        int colno = 0;
        while (*t) {
            char *e = t;
            int nc = colwid[colno++];
            if (!nc) {
                colno = 0;
                nc = colwid[colno++];
            }

            bool nl = false;
            int i = 0;
            for ( ; i < nc; i++) {
                buf[i] = *e++;
                if (!buf[i] || buf[i] == '\n') {
                    nl = true;
                    break;
                }
            }
            for ( ; i < nc; i++)
                buf[i] = ' ';
            buf[nc] = 0;
            t = e;
            while (isspace(*t))
                t++;
            e = buf + (nc - 1);
            while (e >= buf && isspace(*e))
                *e-- = 0;
            // The buf now contains the file name including interior
            // spaces, trailing space has been removed.
            lstr.add("<a href=\"");
            lstr.add(fname);
            if (!trailslash)
                lstr.add_c('/');
            lstr.add(buf);
            lstr.add("\">");
            lstr.add(buf);
            lstr.add("</a>");
            if (nl) {
                lstr.add_c('\n');
                hw->next = new HLPwords(lstr.string(), hw);
                hw = hw->next;
                lstr.clear();
                colno = 0;
            }
            else {
                int len = strlen(buf);
                while (len++ < nc)
                    lstr.add_c(' ');
            }
        }
        delete [] colwid;
        delete [] s;
        tp_is_html = true;
        tp_need_body = true;
        hw->next = new HLPwords("</pre>", hw);
        hw = hw->next;
        return;
    }

    int size = 256;
    char *line = new char [size];
    int lcnt = 0;
    HLPwords *hw = 0;
    for (;;) {
        int c, cnt = 0;
        while ((c = getc(fp)) != EOF) {
            if (cnt == size) {
                char *tmp = new char [2*size];
                memcpy(tmp, line, size);
                size *= 2;
                delete [] line;
                line = tmp;
            }
            if (c == '\n') {
                line[cnt] = 0;
                if (cnt > 0 && line[cnt-1] == '\r')
                    line[cnt-1] = 0;
                break;
            }
            line[cnt] = c;
            cnt++;
        }
        if (c == EOF) {
            if (cnt == size) {
                size++;
                char *tmp = new char [size];
                memcpy(tmp, line, size-1);
                delete [] line;
                line = tmp;
            }
            line[cnt] = 0;
        }
        if (hw) {
            hw->next = new HLPwords(line, hw);
            hw = hw->next;
        }
        else
            tp_text = hw = new HLPwords(line, 0);
        register_word(line);
        lcnt++;
        if (lcnt < 20 && !tp_is_html)
            tp_is_html = check_html(line);
        if (c == EOF)
            break;
    }
    delete [] line;
}


// Grab all the text from string into the HLPwords list.
//
void
HLPtopic::get_string(const char *string)
{
    HLPwords *hw = 0;
    int lcnt = 0;
    while (*string) {
        const char *s = string;
        while (*string && *string != '\n')
            string++;
        int len = string - s;
        char *t = new char [len + 1];
        memcpy(t, s, len);
        t[len] = 0;
        if (t[len-1] == '\r')
            t[len-1] = 0;
        if (*string == '\n')
            string++;

        if (hw) {
            hw->next = new HLPwords(0, hw);
            hw = hw->next;
        }
        else
            tp_text = hw = new HLPwords(0, 0);
        hw->word = t;
        register_word(t);
        lcnt++;
        if (lcnt < 20 && !tp_is_html)
            tp_is_html = check_html(t);
    }
}


// Create and fill a text buffer containing the text (chartext field).
//
void
HLPtopic::load_text()
{
    delete [] tp_chartext;
    sLstr lstr;
    char tbuf[256];
    bool body_added = false;

    if (!tp_show_plain && !tp_is_html && !tp_from_db) {
        if (tp_subtopics || tp_seealso)
            tp_is_html = true;
        else {
            int lcnt = 0;
            for (HLPwords *hw = tp_text; hw; hw = hw->next) {
                lcnt++;
                if (lcnt < 20 && !tp_is_html)
                    tp_is_html = check_html(hw->word);
                if (tp_is_html)
                    break;
            }
        }
        if (!tp_is_html)
            tp_show_plain = true;
    }
    if (!tp_show_plain && (!tp_is_html || tp_need_body)) {
        body_added = true;
        char *s = HLP()->context()->getBodyTag();
        lstr.add(s);
        delete [] s;
    }
    if (!tp_show_plain && tp_from_db) {
        bool did_title = false;
        const char *hdr = HLP()->get_header();
        if (hdr) {
            const char *ts = strstr(hdr, "%TITLE%");
            if (ts) {
                const char *t = hdr;
                while (t < ts)
                    lstr.add_c(*t++);
                lstr.add(tp_title);
                t += 7;
                while (*t)
                    lstr.add_c(*t++);
                did_title = true;
            }
            else
                lstr.add(hdr);
        }
        if (tp_title && *tp_title && !did_title) {
            sprintf(tbuf, "<H1>%s</H1>\n", tp_title);
            lstr.add(tbuf);
        }
        if (tp_tag && HLP()->get_main_tag() &&
                strcmp(tp_tag, HLP()->get_main_tag())) {
            const char *ttext = HLP()->get_tag_text();
            if (ttext) {
                const char *ts = strstr(ttext, "%TAG%");
                if (ts) {
                    const char *t = ttext;
                    while (t < ts)
                        lstr.add_c(*t++);
                    lstr.add(tp_tag);
                    t += 5;
                    while (*t)
                        lstr.add_c(*t++);
                }
                else
                    lstr.add(ttext);
            }
        }
    }
    for (HLPwords *hw = tp_text; hw; hw = hw->next) {
        char *s = hw->word;
        if (!tp_show_plain && tp_from_db) {
            while (isspace(*s))
                s++;
        }
        lstr.add(s);
        if (!tp_show_plain && !tp_is_html) {
            if (*s)
                lstr.add("<BR>");
            else
                lstr.add("<P>");
        }
        lstr.add_c('\n');
    }
    if (!tp_show_plain) {
        if (tp_subtopics) {
            lstr.add("<H3>Subtopics</H3>\n");
            for (HLPtopList *tl = tp_subtopics; tl; tl = tl->next) {
                tl->tl_buttontext = tl->tl_description;
                if (!tl->tl_buttontext)
                    tl->tl_buttontext = "<unknown>";
                sprintf(tbuf, "<A HREF=\"%s\">%s</A><BR>\n",
                    tl->tl_keyword, tl->tl_buttontext);
                lstr.add(tbuf);
            }
        }
        if (tp_seealso) {
            lstr.add("<H3>References</H3>\n");
            for (HLPtopList *tl = tp_seealso; tl; tl = tl->next) {
                tl->tl_buttontext = tl->tl_description;
                if (!tl->tl_buttontext)
                    tl->tl_buttontext = "<unknown>";
                sprintf(tbuf, "<A HREF=\"%s\">%s</A><BR>\n",
                    tl->tl_keyword, tl->tl_buttontext);
                lstr.add(tbuf);
            }
        }
        if (tp_from_db)
            lstr.add(HLP()->get_footer());
        if (body_added)
            lstr.add("</body>\n");
    }

    tp_chartext = lstr.string_trim();
}


// Return text with html tags stripped out.  If htext is 0, return the
// TEXT section as plain text.
//
char *
HLPtopic::strip_html(int lw, const char *htext)
{
    if (htext) {
        // this is used to strip html from titles
        char *str = htmPlainTextFromHTML(htext, lw, 0);
        if (str) {
            char *e = str + strlen(str) - 1;
            while (e >= str && isspace(*e))
                *e-- = 0;
        }
        return (str);
    }

    sLstr lstr;
    for (HLPwords *hw = tp_text; hw; hw = hw->next) {
        char *s = hw->word;
        while (isspace(*s))
            s++;
        lstr.add(s);
        if (!tp_is_html)
            lstr.add("<BR>");
        lstr.add("\n");
    }
    return (htmPlainTextFromHTML(lstr.string(), lw, 4));
}
// End of HLPtopic functions.


namespace {
    // Strip HTML tags from the beginning of the string, return the first
    // text.
    //
    inline char *
    striptag(char *s)
    {
        while (*s == '<') {
            char *t = s + 1;
            while (*t && *t != '>')
                t++;
            if (!*t)
                break;
            s = t+1;
        }
        return (s);
    }

    inline bool
    sortcmp(const HLPtopList *tlp1, const HLPtopList *tlp2)
    {
        char *s1 = tlp1->tl_description;
        char *s2 = tlp2->tl_description;
        s1 = striptag(s1);
        s2 = striptag(s2);
        while (*s1 && *s2) {
            if (isdigit(*s1) && isdigit(*s2)) {
                int d = atoi(s1) - atoi(s2);
                if (d != 0)
                    return (d < 0);
                // same number, but may be leading 0's
                while (isdigit(*s1)) {
                    d = *s1 - *s2;
                    if (d)
                        return (d < 0);
                    s1++;
                    s2++;
                }
                continue;
            }
            int d = *s1 - *s2;
            if (d)
                return (d < 0);
            s1++;
            s2++;
        }
        return (*s1 < *s2);
    }
}


HLPtopList *
HLPtopList::sort()
{
    int num;
    HLPtopList *tl;
    for (num = 0, tl = this; tl; num++, tl = tl->next) ;
    if (num < 2)
        return (this);

    HLPtopList **vec = new HLPtopList*[num];
    int i;
    for (tl = this, i = 0; tl; tl = tl->next, i++)
        vec[i] = tl;
    std::sort(vec, vec + num, sortcmp);
    tl = vec[0];
    for (i = 0; i < num - 1; i++)
        vec[i]->next = vec[i + 1];
    vec[i]->next = 0;
    delete [] vec;
    return (tl);
}

