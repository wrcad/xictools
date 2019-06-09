
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

#ifndef PROMPTLINE_H
#define PROMPTLINE_H

#include "cd_hypertext.h"

// Defines for the Hypertext Editor

// Passed to the text editing functions in cPromptLine and
// cPromptEdit, determines response to button presses in drawing
// window while editing:
//   PLedEndBtn:    Button press acts like <Return>.
//   PLedIgnoreBtn: No "hypertext", press ignored.
//
enum PLedMode { PLedNormal, PLedEndBtn, PLedIgnoreBtn };

// Passed to the text editing functions in cPromptLine, determines how
// the passed text string is handled.
//   PLedStart:     Start of editing, initialize to string.
//   PLedInsert:    String is inserted at cursor, other arguments are
//                  ignored.
//   PLedUpdate:    Prompt and string are updated.
//
enum PLedType { PLedStart, PLedInsert, PLedUpdate };

enum HYstate {hyOFF, hyACTIVE, hyESCAPE};

// cursor height
#define CURHT 1
#define TOEND -1

#define UNDRAW false
#define DRAW true

// Text elements.
//
struct sHtxt
{
    sHtxt()
        {
            h_c[0] = 0;
            h_c[1] = 0;
            h_type = HLrefEnd;
            h_ent = 0;
            h_str = 0;
        }

    void set_ent(char *str, hyEnt *ent)
        {
            h_c[0] = 0;
            h_c[1] = 0;
            h_type = HLrefEnd;
            h_ent = 0;
            h_str = 0;
            if (ent && str) {
                h_ent = ent;
                h_str = str;
                if (ent->ref_type() == HYrefNode)
                    h_type = HLrefNode;
                else if (ent->ref_type() == HYrefBranch)
                    h_type = HLrefBranch;
                else if (ent->ref_type() == HYrefDevice)
                    h_type = HLrefDevice;
            }
        }

    void set_char(char ch)
        {
            h_c[0] = ch;
            h_c[1] = 0;
            h_type = HLrefText;
            h_ent = 0;
            h_str = 0;
        }

    void set_unichar(const char *cp)
        {
            char *c = h_c;
            while ((*c++ = *cp++) != 0) ;
            h_type = HLrefText;
            h_ent = 0;
            h_str = 0;
        }

    void set_lt(char *str)
        {
            h_c[0] = 0;
            h_c[1] = 0;
            h_type = HLrefLongText;
            h_ent = 0;
            h_str = str;
        }

    void upd_type(HLrefType t)
        {
            h_type = t;
        }

    void clear_free()
        {
            h_c[0] = 0;
            h_c[1] = 0;
            h_type = HLrefEnd;
            delete h_ent;
            h_ent = 0;
            delete [] h_str;
            h_str = 0;
        }

    const char *chr()       const { return (h_c); }
    HLrefType type()        const { return (h_type); }
    hyEnt *hyent()          const { return (h_ent); }
    const char *string()    const { return (h_str); }

private:
    char h_c[8];        // Unicode character, if type == HLrefText
    HLrefType h_type;   // reference type
    hyEnt *h_ent;       // point struct for HLrefNode, HLrefBranch, HLrefDevice
    char *h_str;        // string for HLrefNode, HLrefBranch, HLrefDevice
};


// Prompt line buffer, allowing arbitrary line length.
//
struct sPromptBuffer
{
    sPromptBuffer()
        {
            pb_size = 256;
            pb_hbuf = new sHtxt[pb_size];
            pb_tsize = pb_size;
            pb_tbuf = new char[pb_tsize];
            *pb_tbuf = 0;
        }

    // No destructor, allocated once in singleton.

    void set_ent(char *str, hyEnt *ent, int col)
        {
            if (check_size(col))
                pb_hbuf[col].set_ent(str, ent);
        }

    void set_char(char ch, int col)
        {
            if (check_size(col))
                pb_hbuf[col].set_char(ch);
        }

    void set_lt(char *str, int col)
        {
            if (check_size(col))
                pb_hbuf[col].set_lt(str);
        }

    void upd_type(HLrefType t, int col)
        {
            if (check_size(col))
                pb_hbuf[col].upd_type(t);
        }

    void clear_free(int col)
        {
            if (check_size(col))
                pb_hbuf[col].clear_free();
        }

    sHtxt *element(int col)
        {
            if (check_size(col))
                return (pb_hbuf + col);
            return (0);
        }

    int size()
        {
            return (pb_size);
        }

    void insert(sHtxt*, int);
    void replace(sHtxt*, int);
    void swap(int, int);
    void remove(int);
    int endcol();
    void clear_to_end(int);

    char *set_plain_text(const char*);
    char *get_plain_text(int);
    hyList *get_hyList(int, bool);

private:
    bool check_size(int col)
        {
            if (col < 0)
                return (false);
            if (col >= pb_size) {
                sHtxt *tmp = new sHtxt[2*pb_size];
                memcpy(tmp, pb_hbuf, pb_size*sizeof(sHtxt));
                // Zero pointers for delete.
                memset((void*)pb_hbuf, 0, pb_size*sizeof(sHtxt));
                delete [] pb_hbuf;
                pb_hbuf = tmp;
                pb_size += pb_size;
            }
            return (true);
        }

    sHtxt   *pb_hbuf;   // Hypertext main buffer.
    char    *pb_tbuf;   // Plain text, for editor return.
    int     pb_size;    // Size of main hypertext buffer.
    int     pb_tsize;   // Length of pb_tbuf.
};

struct sUni;

// Callback prototype for long text pop-up.
typedef void(*LongTextCallback)(hyList*, void*);

// Prompt-line text editor.
//
class cPromptEdit : virtual public GRdraw
{
public:
    cPromptEdit();
    virtual ~cPromptEdit() { }

    void set_no_graphics() { pe_disabled = true; }

    void init();
    void set_prompt(char*);
    char *get_prompt();
    hyList *get_hyList(bool = false);
    char *edit_plain_text(const char*, const char*, const char*, PLedType,
        PLedMode);
    hyList *edit_hypertext(const char*, hyList*, const char*, bool, PLedType,
        PLedMode, LongTextCallback, void*);
    void init_edit(PLedMode = PLedNormal);
    bool editor();
    void abort_long_text();
    void abort();
    void finish(bool);
    void insert(sHtxt*);
    void insert(const char*);
    void insert(hyList*);
    void replace(int, sHtxt*);
    void rotate_plotpts(int, int);
    void del_col(int, int);
    void show_del(int);
    void clear_cols_to_end(int);
    void set_col(int, bool=false);
    void set_offset(int);
    void text(const char*, int);
    void draw_text(bool, int, bool);
    void draw_cursor(bool);
    void draw_marks(bool);
    void redraw();
    int bg_pixel();
    void indicate(bool);

    bool key_handler(int, const char*, int);
    void button1_handler(bool);
    int find_ent(hyEnt*);
    void process_b1_text(char*, hyEnt*);
    void button_press_handler(int, int, int);
    void button_release_handler(int, int, int);
    void pointer_motion_handler(int, int);
    void lt_btn_press_handler();

    void select(int, int);
    void deselect(bool = false);
    char *get_sel();

    // Toolkit-specific functions.

    virtual void flash_msg(const char*, ...) = 0;
        // Pop up a message just above the prompt line for a couple
        // of seconds.

    virtual void flash_msg_here(int, int, const char*, ...) = 0;
        // As above, but let user pass position.

    virtual void save_line() = 0;
        // Save text in register 0.

    virtual int win_width(bool = false) = 0;
        // Return pixel width of rendering area, or width in chars
        // if arg is true.

    virtual void set_focus() = 0;
        // Set keyboard focus to this.

    virtual void set_indicate() = 0;
        // Turn on/off "editing" indicator.

    virtual void show_lt_button(bool) = 0;
        // Pop up/down the "L" button.

    virtual void get_selection(bool) = 0;
        // Push current selection into editor.

    virtual void *setup_backing(bool) = 0;
        // Initialize for drawing.

    virtual void restore_backing(void*) = 0;
        // Clean up after drawing.

    virtual void init_window() = 0;
        // Window initialization.

    virtual bool check_pixmap() = 0;
        // Reinitialize pixmap.

    virtual void init_selection(bool) = 0;
        // Text is selected or deselected.

    virtual void warp_pointer() = 0;
        // Move mouse pointer into editor.

    bool is_active()                { return (pe_active == hyACTIVE); }
    bool is_off()                   { return (pe_active == hyOFF); }

    bool is_using_popup()           { return (pe_using_popup); }
    void set_using_popup(bool b)    { pe_using_popup = b; }

    bool is_long_text_mode()        { return (pe_long_text_mode); }
    void set_long_text_mode(bool b) { pe_long_text_mode = b; }

    void set_col_min(int c)         { pe_colmin = c; }

    bool exec_down_callback()
        {
            if (pe_down_callback) {
                (*pe_down_callback)();
                return (true);
            }
            return (false);
        }

    bool exec_up_callback()
        {
            if (pe_up_callback) {
                (*pe_up_callback)();
                return (true);
            }
            return (false);
        }

    bool exec_ctrl_d_callback()
        {
            if (pe_ctrl_d_callback) {
                (*pe_ctrl_d_callback)();
                return (true);
            }
            return (false);
        }

    void set_down_callback(void(*c)())      { pe_down_callback = c; }
    void set_up_callback(void(*c)())        { pe_up_callback = c; }
    void set_ctrl_d_callback(void(*c)())    { pe_ctrl_d_callback = c; }

    bool is_obscure_mode()          { return (pe_obscure_mode); }
    void set_obscure_mode(bool b)   { pe_obscure_mode = b; }

protected:
    GReditPopup *pe_lt_popup;       // long text string editor

    void (*pe_down_callback)();     // misc. configurable callbacks
    void (*pe_up_callback)();
    void (*pe_ctrl_d_callback)();

    CDs *pe_pxdesc;             // hooks for the button1_handler
    hyEnt *pe_pxent;

    HYstate pe_active;          // true when editing
    int pe_colmin;              // minimum cursor location

    int pe_cwid;                // current cursor width in cols
    int pe_xpos, pe_ypos;       // lower left coords of string
    int pe_offset;              // drawing offset
    int pe_fntwid;              // font size
    int pe_column;              // current cursor column
    bool pe_firstinsert;        // true before first insertion or cursor mvmt
    bool pe_indicating;         // true when editing
    bool pe_disabled;           // suppress actions
    bool pe_obscure_mode;       // obscure text, for password entry
    bool pe_using_popup;        // ShowPrompt() using pop-up
    bool pe_long_text_mode;     // long text directive
    bool pe_in_select;          // selection in progress
    bool pe_entered;            // mouse pointer is in prompt area

    // For plot mark drag.
    bool pe_down;
    int pe_press_x;
    int pe_press_y;
    char *pe_last_string;
    hyEnt *pe_last_ent;

    // For text selection in non-editing mode.
    bool pe_has_drag;
    bool pe_dragged;
    int pe_drag_x;
    int pe_drag_y;
    int pe_sel_start;
    int pe_sel_end;

    // Transient unicode to utf8 encoder.
    sUni *pe_unichars;

    // Text buffer.
    sPromptBuffer pe_buf;
};


//  Struct to hold misc. context related to prompt line.
//
struct sPromptContext
{
    struct stringlist_list
    {
        stringlist_list(stringlist *sl, stringlist_list *n)
            { list = sl; next = n; }

        stringlist *list;
        stringlist_list *next;
    };

    sPromptContext()
        {
            pc_stuff_buf = 0;
            pc_prompt_stack = 0;
            pc_prompt_bak = 0;
            pc_saved_prompt = 0;
            pc_last_prompt = 0;
            pc_last_prompt_len = 0;
            pc_tee_fp1 = 0;
            pc_tee_fp2 = 0;
        }

    void stuff_string(const char*);
    char *pop_stuff();
    void redirect(const char*);
    void open_fp1(const char*);
    void set_fp2(FILE*);
    char *cat_prompts(char*);
    void save_prompt();
    void restore_prompt();
    void push_prompt(const char*);
    void pop_prompt();
    void save_last(const char*);
    const char *get_last();

private:
    stringlist *pc_stuff_buf;       // List of pending editor input
    stringlist *pc_prompt_stack;    // List of prefix strings for display
    stringlist_list *pc_prompt_bak; // Prompt stack storage
    stringlist *pc_saved_prompt;    // Saved prompt strings
    char *pc_last_prompt;           // The most recent prompt message
    int pc_last_prompt_len;         // Allocated length of LastPrompt
    FILE *pc_tee_fp1;               // User output redirection
    FILE *pc_tee_fp2;               // Internal output redirection
};

inline class cPromptLine *PL();

// Main class for prompt-line control
//
class cPromptLine
{
    static cPromptLine *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cPromptLine *PL() { return (cPromptLine::ptr()); }

    cPromptLine();

    // Initialization
    void SetNoGraphics();
    void Init();

    // Prompt line text display.
    void ShowPrompt(const char*);
    void ShowPromptV(const char*, ...);
    void ShowPromptNoTee(const char*);
    void ShowPromptNoTeeV(const char*, ...);
    char *GetPrompt();
    hyList *List(bool = false);
    const char *GetLastPrompt();
    void ErasePrompt();
    void SavePrompt();
    void RestorePrompt();
    void PushPrompt(const char*);
    void PushPromptV(const char*, ...);
    void PopPrompt();
    void TeePromptUser(const char*);
    void TeePrompt(FILE*);
    void FlashMessage(const char*);
    void FlashMessageV(const char*, ...);
    void FlashMessageHere(int, int, const char*);
    void FlashMessageHereV(int, int, const char*, ...);

    // Prompt line editing.
    bool IsEditing();
    char *EditPrompt(const char*, const char*, PLedType = PLedStart,
        PLedMode = PLedNormal, bool = false);
    hyList *EditHypertextPrompt(const char*, hyList*, bool,
        PLedType = PLedStart, PLedMode = PLedNormal,
        void(*)(hyList*, void*) = 0, void* = 0);
    void RegisterArrowKeyCallbacks(void(*)(), void(*)());
    void RegisterCtrlDCallback(void(*)());
    void StuffEditBuf(const char*);
    void AbortLongText();
    void AbortEdit();

    // Functions for keypress buffer.  Yes, this is handled here, too.
    void GetTextBuf(WindowDesc*, char*);
    void SetTextBuf(WindowDesc*, const char*);
    void ShowKeys(WindowDesc*);
    void SetKeys(WindowDesc*, const char*);
    void BspKeys(WindowDesc*);
    void CheckExec(WindowDesc*, bool);
    char *KeyBuf(WindowDesc*);
    int KeyPos(WindowDesc*);

    void SetEdit(cPromptEdit *e)    { pl_edit = e; }

private:
    void setupInterface();

    sPromptContext pl_cx;
    cPromptEdit *pl_edit;

    static cPromptLine *instancePtr;
};

// Unicode to UTF8 translation.
struct sUni
{
    sUni()
        {
            u_nchars = 0;
        }

    bool addc(int c)
        {
            // Up to 8 chars.
            if (u_nchars < 8 && isxdigit(c)) {
                u_buf[u_nchars++] = c;
                return (true);
            }
            return (false);
        }

    const char *utf8_encode();

private:
    int u_nchars;
    char u_buf[12];
};

#endif

