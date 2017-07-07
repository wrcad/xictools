
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2013 Whiteley Research Inc, all rights reserved.        *
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
 $Id: help_topic.h,v 1.2 2015/06/11 05:54:04 stevew Exp $
 *========================================================================*/

#ifndef HELP_TOPIC_H
#define HELP_TOPIC_H

#include "lstring.h"

struct HLPtopic;

// Struct used to list a reference
//
struct HLPtopList
{
    HLPtopList(const char *d, const char *kw, HLPtopic *t, HLPtopList *n)
        {
            next = n;
            tl_keyword = lstring::copy(kw);
            tl_description = lstring::copy(d);
            tl_buttontext = 0;
            tl_button = 0;
            tl_top = t;
            tl_newtop = 0;
        }

    ~HLPtopList()
        {
            delete [] tl_keyword;
            delete [] tl_description;
        }

    static void destroy(HLPtopList *tl)
        {
            while (tl) {
                HLPtopList *tx = tl;
                tl = tl->next;
                delete tx;
            }
        }

    HLPtopList *sort();

    HLPtopList *next;
    char *tl_keyword;
    char *tl_description;
    const char *tl_buttontext;
    void *tl_button;
    HLPtopic *tl_top;
    HLPtopic *tl_newtop;
};

// Main struct for a database entry.  This supports both text-mode and
// graphical presentations
//
struct HLPtopic
{
    HLPtopic(const char *kw, const char *ti, const char *tg = 0, bool fdb = false)
        {
            tp_keyword = lstring::copy(kw);
            tp_title = lstring::copy(ti);
            tp_tag = tg;

            tp_parent = 0;
            tp_lastborn = 0;
            tp_next = 0;
            tp_sibling = 0;

            tp_subtopics = 0;
            tp_seealso = 0;

            tp_context = 0;
            tp_chartext = 0;
            tp_target = 0;
            tp_text = 0;

            tp_xposition = 0;
            tp_yposition = 0;
            tp_numlines = 0;
            tp_maxcols = 0;
            tp_curtopline = 0;

            tp_from_db = fdb;
            tp_is_html = false;
            tp_need_body = false;
            tp_show_plain = false;
        }

    ~HLPtopic()
        {
            delete [] tp_keyword;
            delete [] tp_title;

            HLPtopList::destroy(tp_subtopics);
            HLPtopList::destroy(tp_seealso);

            delete [] tp_chartext;
            delete [] tp_target;
            HLPwords::destroy(tp_text);
        }

    // Destroy this topic and its descendents.
    //
    static void destroy(HLPtopic *ht)
        {
            if (ht) {
                HLPtopic *tn;
                for (HLPtopic *t = ht->tp_lastborn; t; t = tn) {
                    tn = t->tp_sibling;
                    destroy(t);
                }
                for (HLPtopic *t = ht->tp_next; t; t = tn) {
                    tn = t->tp_sibling;
                    destroy(t);
                }
                delete ht;
            }
        }

    // topic.cc
    bool show_in_window();
    void link_new_and_show(bool, HLPtopic*);
    void reuse(HLPtopic*, bool);
    void redisplay();
    void set_position(int, int);
    void unlink();
    void get_file(FILE*, const char*);
    void get_string(const char*);
    void load_text();
    char *strip_html(int, const char*);

    const char *keyword()       { return (tp_keyword); }
    void set_keyword(char *k)   { delete [] tp_keyword; tp_keyword = k; }
    const char *title()         { return (tp_title); }
    const char *tag()           { return (tp_tag); }

    static HLPtopic *get_parent(HLPtopic *ht)
        {
            if (ht && ht->tp_parent)
                return (ht->tp_parent);
            return (ht);
        }

    static HLPtopic *get_last(HLPtopic *ht)
        {
            if (ht && ht->tp_lastborn)
                return (ht->tp_lastborn);
            return (ht);
        }

    HLPtopic *parent()              { return (tp_parent); }
    void set_parent(HLPtopic *t)    { tp_parent = t; }
    HLPtopic *lastborn()            { return (tp_lastborn); }
    void set_lastborn(HLPtopic *t)  { tp_lastborn = t; }
    HLPtopic *next()                { return (tp_next); }
    void set_next(HLPtopic *t)      { tp_next = t; }
    HLPtopic *sibling()             { return (tp_sibling); }
    void set_sibling(HLPtopic *t)   { tp_sibling = t; }

    HLPtopList *subtopics()     { return (tp_subtopics); }
    void add_subtopic(const char *t, const char *w)
        { tp_subtopics = new HLPtopList(t, w, this, tp_subtopics); }
    void sort_subtopics()       { tp_subtopics = tp_subtopics->sort(); }

    HLPtopList *seealso()       { return (tp_seealso); }
    void add_seealso(const char *t, const char *w)
        { tp_seealso = new HLPtopList(t, w, this, tp_seealso); }
    void sort_seealso()         { tp_seealso = tp_seealso->sort(); }

    GRwbag *context()           { return (tp_context); }
    void set_context(GRwbag *c) { tp_context = c; }

    char *get_text()            { load_text(); return (tp_chartext); }
    const char *get_cur_text()  { return (tp_chartext); }
    void set_text(char *s)      { delete [] tp_chartext; tp_chartext = s; }
    void clear_text()           { tp_chartext = 0; }

    char *target()              { return (tp_target); }
    void set_target(const char *t)
        { delete [] tp_target; tp_target = lstring::copy(t); }

    HLPwords *get_words()       { return (tp_text); }
    void set_words(HLPwords *h) { HLPwords::destroy(tp_text); tp_text = h; }
    void clear_words()          { tp_text = 0; }
    void register_word(const char *w)
        {
            tp_numlines++;
            int i = strlen(w);
            if (i > tp_maxcols)
                tp_maxcols = i;
        }

    int get_topline()           { return (tp_curtopline); }
    void set_topline(int t)     { tp_curtopline = t; }

    bool from_db()              { return (tp_from_db); }
    bool is_html()              { return (tp_is_html); }
    void set_is_html(bool b)    { tp_is_html = b; }
    bool need_body()            { return (tp_need_body); }
    void set_need_body(bool b)  { tp_need_body = b; }
    bool show_plain()           { return (tp_show_plain); }
    void set_show_plain(bool b) { tp_show_plain = b; }

    static void set_win_count(int c) { tp_wincount = c; }

private:
    char *tp_keyword;           // the keyword
    char *tp_title;             // title for topic
    const char *tp_tag;         // tag for topic

    HLPtopic *tp_parent;        // parent topic for current
    HLPtopic *tp_lastborn;      // last viewer topic
    HLPtopic *tp_next;          // list of popped topics from "back"
    HLPtopic *tp_sibling;       // list of previoulsy visited topics

    HLPtopList *tp_subtopics;   // list of selectable topics
    HLPtopList *tp_seealso;     // list of selectable topics

    GRwbag *tp_context;         // widget bag of display
    char *tp_chartext;          // formatted string
    char *tp_target;            // target name for window
    HLPwords *tp_text;          // raw text lines

    int tp_xposition;           // initial X position
    int tp_yposition;           // initial Y position
    int tp_numlines;            // number of raw input lines
    int tp_maxcols;             // number of raw input char cols
    int tp_curtopline;          // current visible top line

    bool tp_from_db;            // set if topic from database
    bool tp_is_html;            // set if HTML input
    bool tp_need_body;          // set if we need to add a body tag
    bool tp_show_plain;         // set if displaying as plain text

    static int tp_wincount;     // for positioning
};

// HTML check function for header tags.
//
inline bool
is_html_head(const char *s)
{
    if (lstring::ciprefix("!doctype ", s) || lstring::ciprefix("html>", s) ||
            lstring::ciprefix("head>", s) || lstring::ciprefix("title>", s) ||
            lstring::ciprefix("meta ", s) || lstring::ciprefix("link ", s) ||
            lstring::ciprefix("frameset ", s) ||
            lstring::ciprefix("frame ", s) ||
            lstring::ciprefix("body ", s))
        return (true);
    return (false);
}


// HTML check function for frames.
//
inline bool
is_html_frame(const char *s)
{
    if (lstring::ciprefix("frameset ", s) || lstring::ciprefix("frame ", s) ||
            lstring::ciprefix("noframes", s))
        return (true);
    return (false);
}


// HTML check function for body tags.
//
inline bool
is_html_body(const char *s)
{
    if (lstring::ciprefix("p>", s) || lstring::ciprefix("pre>", s) ||
            lstring::ciprefix("a ", s) || lstring::ciprefix("br", s) ||
            lstring::ciprefix("b>", s) || lstring::ciprefix("i>", s) ||
            lstring::ciprefix("center>", s) || lstring::ciprefix("font ", s) ||
            lstring::ciprefix("h1>", s) || lstring::ciprefix("h2>", s) ||
            lstring::ciprefix("h3>", s) || lstring::ciprefix("dl>", s) ||
            lstring::ciprefix("ol>", s) || lstring::ciprefix("ul>", s) ||
            lstring::ciprefix("img ", s) || lstring::ciprefix("hrule", s) ||
            lstring::ciprefix("form", s) || lstring::ciprefix("table", s))
        return (true);
    return (false);
}

#endif

