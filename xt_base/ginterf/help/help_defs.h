
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
 $Id: help_defs.h,v 1.20 2015/04/25 01:13:31 stevew Exp $
 *========================================================================*/

#ifndef HLPDEFS_H
#define HLPDEFS_H

#include "graphics.h"
#include "lstring.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

// keywords used in the database file
#define HLP_COMMENT   "!! "
#define HLP_HEADER    "!!HEADER"
#define HLP_FOOTER    "!!FOOTER"
#define HLP_KEYWORD   "!!KEYWORD"
#define HLP_TITLE     "!!TITLE"
#define HLP_TEXT      "!!TEXT"
#define HLP_HTML      "!!HTML"
#define HLP_SEEALSO   "!!SEEALSO"
#define HLP_SUBTOPICS "!!SUBTOPICS"
#define HLP_INCLUDE   "!!INCLUDE"
#define HLP_IFDEF     "!!IFDEF"
#define HLP_IFNDEF    "!!IFNDEF"
#define HLP_ELSE      "!!ELSE"
#define HLP_ENDIF     "!!ENDIF"
#define HLP_REDIR     "!!REDIRECT"
#define HLP_MAINTAG   "!!MAINTAG"
#define HLP_TAGTEXT   "!!TAGTEXT"
#define HLP_TAG       "!!TAG"

// positioning
#define X_INCR      50
#define Y_INCR      50
#define START_XPOS  100
#define START_YPOS  100

// look ahead
class HLPcontext;
struct HLPwords;
struct HLPtopic;
struct HLPdirList;
struct HLPent;

// This is a base interface for "text mode" help.  If not using the
// graphics, the display method is called to display the topic, if a
// TextHelp subclass has been registered with cHelp::hlp_register_text_hlp.
// The implementation is up to the user.
//
struct TextHelp
{
    virtual ~TextHelp() { }

    virtual bool display(HLPtopic*) = 0;
};


inline class cHelp *HLP();

// The top-level interface to help system.
//
class cHelp
{
    static cHelp *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cHelp *HLP() { return (cHelp::ptr()); }

    friend struct HLPtopic;
    friend struct HLPdirList;

    // readhelp.cc
    cHelp();
    void set_path(const char*, bool);
    bool get_path(char**);
    void rehash();
    void list(HLPwords*);
    void word(const char*);
    HLPtopic *read(const char*);
    bool is_keyword(const char*);
    const char *alias(const char*);
    HLPtopic *search(const char*);
    void define(const char*);
    void undef(const char*);
    bool isdef(const char*);

    FILE *open(const char*, HLPent**);
    HLPent *get_entry(const char*);

    // in viewer code
    void set_font_family(const char*);
    void set_fixed_family(const char*);
    const char *get_font_family();
    const char *get_fixed_family();

    HLPcontext *context()       { return (hlp_context); }

    const char *get_header()    { return (hlp_header); }
    void set_header(char *h)    { delete [] hlp_header; hlp_header = h; }
    const char *get_footer()    { return (hlp_footer); }
    void set_footer(char *f)    { delete [] hlp_footer; hlp_footer = f; }
    const char *get_main_tag()  { return (hlp_main_tag); }
    void set_main_tag(char *t)  { delete [] hlp_main_tag; hlp_main_tag = t; }
    const char *get_tag_text()  { return (hlp_tag_text); }
    void set_tag_text(char *t)  { delete [] hlp_tag_text; hlp_tag_text = t; }

    const char *error_msg()     { return (hlp_errmsg); }
    HLPdirList *dir_list()      { return (hlp_dirlist); }
    void set_dir_list(HLPdirList *d) { hlp_dirlist = d; }
    void register_callback(bool(*cb)(char**, HLPtopic**))
                                { hlp_callback = cb; }
    void set_name(const char *name) { hlp_title = name; }
    const char *get_name()      { return (hlp_title ? hlp_title : "mozy"); }
    void set_init_x(int x)      { hlp_initxpos = x; }
    void set_init_y(int y)      { hlp_initypos = y; }
    int get_init_x()            { return (hlp_initxpos); }
    int get_init_y()            { return (hlp_initypos); }
    void register_text_help(TextHelp *t) { hlp_text_hdlr = t; }

    bool using_graphics()       { return (hlp_usex); }
    void set_using_graphics(bool b) { hlp_usex = b; }
    bool multi_win()            { return (hlp_multiwin); }
    void set_multi_win(bool b)  { hlp_multiwin = b; }
    bool no_file_fonts()        { return (hlp_no_file_fonts); }
    void set_no_file_fonts(bool b)  { hlp_no_file_fonts = b; }
    bool fifo_start()           { return (hlp_fifo_start); }
    void set_fifo_start(bool b) { hlp_fifo_start = b; }
    void set_debug(bool b)      { hlp_debug = b; }

private:
    HLPwords *scan(const char*);
    HLPwords *scan_file(FILE*, const char*);
    HLPdirList *recycle_dir(const char*);
    bool lookup_file(const char*);
    HLPwords *get_include(const char*);

    HLPcontext *hlp_context;        // associated display context manager
    HLPdirList *hlp_dirlist;        // list of search path directories
    const char *hlp_errmsg;         // error message return
    int hlp_initxpos;               // initial window location
    int hlp_initypos;
    const char *hlp_title;          // used in window title bar
    HLPwords *hlp_defines;          // list of words for !!IFDEF
    char *hlp_header;               // header text
    char *hlp_footer;               // footer text
    char *hlp_main_tag;             // main tag definition
    char *hlp_tag_text;             // main tag tex block
    TextHelp *hlp_text_hdlr;        // registered text-mode handler

    bool hlp_usex;              // true => use graphics
    bool hlp_multiwin;          // true => new window for each new entry
    bool hlp_no_file_fonts;     // true => don't use fonts in mozyrc file
    bool hlp_fifo_start;        // true => create fifo on startup
    bool hlp_debug;             // turn on debug mode

    // This intercepts and resolves words ahead of the database,
    // for user application.  Use register_callback to set this up.
    // This can modify the word, or supply a topic, or abort if true
    // is returned
    bool (*hlp_callback)(char**, HLPtopic**);

    static cHelp *instancePtr;
};


// Used to free various lists.
//
template <class T> void
hlp__free(T *x)
{
    while (x) {
        T *xn = x->next;
        delete x;
        x = xn;
    }
}


// A private 'wordlist'
//
struct HLPwords
{
    HLPwords(const char *w, HLPwords *p)
        {
            word = lstring::copy(w);
            next = 0;
            prev = p;
        }

    ~HLPwords() { delete [] word; }
    void free() { hlp__free(this); }

    char *word;
    HLPwords *next;
    HLPwords *prev;
};

// Struct used to list a database entry.
//
struct HLPent
{
    HLPent(char *kw, char *t, const char *fn, const char *tg, long o,
        HLPent *n)
        {
            next = n;
            offset = o;
            keyword = kw;
            title = t;
            filename = fn;
            tag = tg;
            is_html = false;
        }

    ~HLPent()
        {
            delete [] keyword;
            delete [] title;
        }

    void free() { hlp__free(this); }

    HLPent *next;
    long offset;                // Offset of topic text in file.
    char *keyword;              // Keyword of topic.
    char *title;                // Title of topic.
    const char *filename;       // Source file path.
    const char *tag;            // Tag for topic.
    bool is_html;               // True if text is HTML formatted.
};

// Element for the redirection table.
//
struct HLPrdir
{
    HLPrdir(char *k, char *a, HLPrdir *n)
        {
            key = k;
            alias = a;
            next = n;
        }

    ~HLPrdir()
        {
            delete [] key;
            delete [] alias;
        }

    void free() { hlp__free(this); }

    char *key;
    char *alias;
    HLPrdir *next;
};

// internal database
//
struct HLPdirList
{
    HLPdirList(const char *d)
        {
            next = 0;
            hd_dir = lstring::copy(d);
            hd_base = 0;
            hd_rdbase = 0;
            hd_files = 0;
            hd_tags = 0;
            hd_inactive = false;
            hd_empty = false;
        }

    ~HLPdirList();
    void free() { hlp__free(this); }
    void init();
    void reset();
    void read_file(const char*);

    HLPdirList *next;
    char *hd_dir;              // directory path of this subdirectory
    HLPent **hd_base;          // keyword hash table
    HLPrdir **hd_rdbase;       // redirection hash table
    stringlist *hd_files;      // list of .hlp files in directory (full path)
    stringlist *hd_tags;       // list of tags found in .hlp files
    bool hd_inactive;          // ignore this directory if set
    bool hd_empty;             // this directory contains no .hlp files if set
};

#endif

