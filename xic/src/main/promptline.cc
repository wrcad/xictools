
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
 $Id: promptline.cc,v 5.116 2017/04/16 20:28:13 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "editif.h"
#include "scedif.h"
#include "dsp_tkif.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "keymacro.h"
#include "events.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "ghost.h"
#include "filestat.h"


cPromptLine *cPromptLine::instancePtr = 0;

cPromptLine::cPromptLine()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cPromptLine already instantiated.\n");
        exit(1);
    }
    instancePtr = this;
    pl_edit = 0;

    setupInterface();
}


// Private static error exit.
//
void
cPromptLine::on_null_ptr()
{
    fprintf(stderr, "Singleton class cPromptLine used before instantiated.\n");
    exit(1);
}


//------------------------------------
// Initialization

// Turn off graphics.
//
void
cPromptLine::SetNoGraphics()
{
    if (pl_edit)
        pl_edit->set_no_graphics();
}


// Initialize for a given font and window size.
//
void
cPromptLine::Init()
{
    if (pl_edit)
        pl_edit->init();
}


//------------------------------------
// Prompt Line Text Display

#define PROMPT_BUFSIZE 1024

// Show the string in the prompt area.  This text is not editable, and no
// cursor is drawn.  If the editor is in use, use a popup instead.
//
void
cPromptLine::ShowPrompt(const char *string)
{
    char buf[PROMPT_BUFSIZE];

    if (!string)
        string = "";
    char *s = pl_cx.cat_prompts(buf);
    int len = strlen(string);
    if (len < PROMPT_BUFSIZE - (s-buf))
        strcpy(s, string);
    else {
        strncpy(s, string, PROMPT_BUFSIZE - (s-buf));
        buf[PROMPT_BUFSIZE-1] = 0;
    }

    // Redirect a copy of the prompt messages.
    pl_cx.redirect(buf);

    // Save current message.
    pl_cx.save_last(buf);

    if (pl_edit)
        pl_edit->set_prompt(buf);
}


// Variadic version of above.  This is a separate function, since if a
// single argument is passed to this function that happens to contain
// '%' characters, bad things may happen.
//
void
cPromptLine::ShowPromptV(const char *string, ...)
{
    va_list args;
    char buf[PROMPT_BUFSIZE];

    if (!string)
        string = "";
    va_start(args, string);
    char *s = pl_cx.cat_prompts(buf);
    vsnprintf(s, PROMPT_BUFSIZE - (s-buf), string, args);
    va_end(args);

    // Redirect a copy of the prompt messages.
    pl_cx.redirect(buf);

    // Save current message.
    pl_cx.save_last(buf);

    if (pl_edit)
        pl_edit->set_prompt(buf);
}


// As for ShowPrompt, but never redirect, and don't save the message.
//
void
cPromptLine::ShowPromptNoTee(const char *string)
{
    char buf[PROMPT_BUFSIZE];

    if (!string)
        string = "";
    char *s = pl_cx.cat_prompts(buf);
    int len = strlen(string);
    if (len < PROMPT_BUFSIZE - (s-buf))
        strcpy(s, string);
    else {
        strncpy(s, string, PROMPT_BUFSIZE - (s-buf));
        buf[PROMPT_BUFSIZE-1] = 0;
    }

    if (pl_edit)
        pl_edit->set_prompt(buf);
}


// Variadic version.
//
void
cPromptLine::ShowPromptNoTeeV(const char *string, ...)
{
    va_list args;
    char buf[PROMPT_BUFSIZE];

    if (!string)
        string = "";
    va_start(args, string);
    char *s = pl_cx.cat_prompts(buf);
    vsnprintf(s, PROMPT_BUFSIZE - (s-buf), string, args);
    va_end(args);

    if (pl_edit)
        pl_edit->set_prompt(buf);
}


// Return the prompt-line text as a string.
//
char *
cPromptLine::GetPrompt()
{
    if (pl_edit)
        return (pl_edit->get_prompt());
    return (0);
}


// Return the prompt-line test as hypertext.
//
hyList *
cPromptLine::List(bool pre)
{
    if (pl_edit)
        return (pl_edit->get_hyList(pre));
    return (0);
}


// Return the last prompt message.
//
const char *
cPromptLine::GetLastPrompt()
{
    return (pl_cx.get_last());
}


// Erase the text, or pop down the popup.
//
void
cPromptLine::ErasePrompt()
{
    if (!pl_edit)
        return;
    if (pl_edit->is_using_popup()) {
        pl_edit->set_using_popup(false);
        DSPmainWbag(PopUpInfo(MODE_OFF, 0))
        return;
    }
    if (pl_edit->is_off())
        pl_edit->clear_cols_to_end(0);
}


// Save the current prompt line in a buffer.  Saves text only, not
// references.
//
void
cPromptLine::SavePrompt()
{
    pl_cx.save_prompt();
}


// Restore the saved prompt line, freeing the buffer.
//
void
cPromptLine::RestorePrompt()
{
    pl_cx.restore_prompt();
}


// Push a prefix string.  Each prefix string is printed ahead of the
// text passed to ShowPrompt().
//
void
cPromptLine::PushPrompt(const char *string)
{
    char buf[512];
    if (!string)
        string = "";
    int len = strlen(string);
    if (len < 512)
        strcpy(buf, string);
    else {
        strncpy(buf, string, 512);
        buf[512-1] = 0;
    }

    pl_cx.push_prompt(buf);
    ShowPrompt("");
}


// Variadic version.
//
void
cPromptLine::PushPromptV(const char *string, ...)
{
    va_list args;
    char buf[512];
    if (!string)
        string = "";
    va_start(args, string);
    vsnprintf(buf, 512, string, args);
    va_end(args);

    pl_cx.push_prompt(buf);
    ShowPrompt("");
}


// Pop the rightmost prefix string.
//
void
cPromptLine::PopPrompt()
{
    pl_cx.pop_prompt();
    ErasePrompt();
}


// Send a copy of the prompt messages to the file named.  This is part
// of the user interface.  If passed 0 or an empty string, stop
// redirection.
//
void
cPromptLine::TeePromptUser(const char *name)
{
    pl_cx.open_fp1(name);
}


// Send a copy of the prompt messages to fp.  This is used internally.
//
void
cPromptLine::TeePrompt(FILE *fp)
{
    pl_cx.set_fp2(fp);
}


// Pop up a message just above the prompt line for a couple of
// seconds.
//
void
cPromptLine::FlashMessage(const char *msg)
{
    if (!pl_edit)
        return;
    if (!msg)
        return;
    char buf[256];
    strncpy(buf, msg, 256);
    buf[256-1] = 0;

    pl_edit->flash_msg(buf);
}


// Variadic version.
//
void
cPromptLine::FlashMessageV(const char *msg, ...)
{
    if (!pl_edit)
        return;
    va_list args;
    char buf[256];
    if (!msg)
        return;
    va_start(args, msg);
    vsnprintf(buf, 256, msg, args);
    va_end(args);
    pl_edit->flash_msg(buf);
}


// Pop up a message anywhere for a couple of seconds.
//
void
cPromptLine::FlashMessageHere(int x, int y, const char *msg)
{
    if (!pl_edit)
        return;
    if (!msg)
        return;
    char buf[256];
    strncpy(buf, msg, 256);
    buf[256-1] = 0;

    pl_edit->flash_msg_here(x, y, buf);
}


// Variadic version.
//
void
cPromptLine::FlashMessageHereV(int x, int y, const char *msg, ...)
{
    if (!pl_edit)
        return;
    va_list args;
    char buf[256];
    if (!msg)
        return;
    va_start(args, msg);
    vsnprintf(buf, 256, msg, args);
    va_end(args);
    pl_edit->flash_msg_here(x, y, buf);
}


//------------------------------------
// Prompt Line Editing

// Return true if editing.
//
bool
cPromptLine::IsEditing()
{
    return (pl_edit && pl_edit->is_active());
}


// Text edit facility.  If prompt is not 0, this text will be shown
// ahead of the editable text, in a different color.  The prompt text
// is not editable, and will not be included in the edited text string
// returned.  If string is not 0, it is read into the editable
// area.  If update is true, overwrite the prompt and editable text
// currently in the editor and return.  0 is returned if Esc is
// entered.
//
char *
cPromptLine::EditPrompt(const char *prompt, const char *string,
    PLedType update, PLedMode emode, bool obscure)
{
    if (!pl_edit)
        return (0);
    char *stuff = pl_cx.pop_stuff();
    pl_edit->set_obscure_mode(obscure);
    char *str = pl_edit->edit_plain_text(prompt, string, stuff, update, emode);
    pl_edit->set_obscure_mode(false);
    delete [] stuff;
    return (str);
}


// Hypertext list editing facility.  Behavior is identical to EditPrompt(),
// except that the editable text is supplied and returned as a hypertext
// list.
// prompt         Uneditable prompt
// h              Initial hypertext list to edit
// update         Update the text string being edited
// use_long_text  If the first entry is type HLrefLongText, use a multi-line
//                text editor.  This is a hack to allow long text
//                properties, e.g., PWL statements for Spice.
//
hyList *
cPromptLine::EditHypertextPrompt(const char *prompt, hyList *h,
    bool use_long_text, PLedType update, PLedMode emode,
    void(*ltcb)(hyList*, void*), void *ltarg)
{
    if (!pl_edit)
        return (0);
    char *stuff = pl_cx.pop_stuff();
    hyList *hstr = pl_edit->edit_hypertext(prompt, h, stuff,
        use_long_text, update, emode, ltcb, ltarg);
    delete [] stuff;
    return (hstr);
}


// Register something for the up and down arrow keys.
//
void
cPromptLine::RegisterArrowKeyCallbacks(void (*down_call)(), void (*up_call)())
{
    if (pl_edit) {
        pl_edit->set_down_callback(down_call);
        pl_edit->set_up_callback(up_call);
    }
}


// Register something for the Ctrl-D keys.
//
void
cPromptLine::RegisterCtrlDCallback(void (*call)())
{
    if (pl_edit)
        pl_edit->set_ctrl_d_callback(call);
}


// Add a string to the "StuffBuf".  The next call to an edit function
// will return the string.
//
void
cPromptLine::StuffEditBuf(const char *string)
{
    pl_cx.stuff_string(string);
}


// Pop down the long-text editor.
//
void
cPromptLine::AbortLongText()
{
    if (pl_edit)
        pl_edit->abort_long_text();
}


// This causes the editor, if active, to return 0 as if Esc was pressed.
// This is used as a clean-up when the editor is active.
//
void
cPromptLine::AbortEdit()
{
    if (pl_edit)
        pl_edit->abort();
}


//
// Functions for keypress buffer.
//

// Copy the keypress buffer into buf (at most 16 chars).
//
void
cPromptLine::GetTextBuf(WindowDesc *wd, char *buf)
{
    if (!wd)
        wd = DSP()->MainWdesc();
    if (wd && wd->Wbag())
        wd->Wbag()->GetTextBuf(buf);
}


// Copy str into the keypress buffer.
//
void
cPromptLine::SetTextBuf(WindowDesc *wd, const char *str)
{
    if (!wd)
        wd = DSP()->MainWdesc();
    if (wd && wd->Wbag())
        wd->Wbag()->SetTextBuf(str);
}


void
cPromptLine::ShowKeys(WindowDesc *wd)
{
    if (!wd)
        wd = DSP()->MainWdesc();
    if (wd && wd->Wbag())
        wd->Wbag()->ShowKeys();
}


void
cPromptLine::SetKeys(WindowDesc *wd, const char *str)
{
    if (!wd)
        wd = DSP()->MainWdesc();
    if (wd && wd->Wbag())
        wd->Wbag()->SetKeys(str);
}


void
cPromptLine::BspKeys(WindowDesc *wd)
{
    if (!wd)
        wd = DSP()->MainWdesc();
    if (wd && wd->Wbag())
        wd->Wbag()->BspKeys();
}


void
cPromptLine::CheckExec(WindowDesc *wd, bool exact)
{
    if (!wd)
        wd = DSP()->MainWdesc();
    if (wd && wd->Wbag())
        wd->Wbag()->CheckExec(exact);
}


char *
cPromptLine::KeyBuf(WindowDesc *wd)
{
    if (!wd)
        wd = DSP()->MainWdesc();
    if (wd && wd->Wbag())
        return (wd->Wbag()->KeyBuf());
    return (0);
}


int
cPromptLine::KeyPos(WindowDesc *wd)
{
    if (!wd)
        wd = DSP()->MainWdesc();
    if (wd && wd->Wbag())
        return (wd->Wbag()->KeyPos());
    return (0);
}
// End of cPromptLine functions.


//-----------------------------------------------------------------------------
// Prompt Line Hypertext Editor


// Command state controller for editor
//
namespace {
    namespace main_promptline {
        struct EditState : public CmdState
        {
            EditState(const char*, const char*);
            virtual ~EditState();

            void setup(PLedMode m, cPromptEdit *e)
                {
                    edit = e;
                    edit_mode = m;
                }

            void Terminate()                { esc(); }
            void SetEditMode(PLedMode m)    { edit_mode = m; }

            void b1down();
            void b1up();
            void b1down_altw();
            void b1up_altw();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();

        private:
            cPromptEdit *edit;
            PLedMode edit_mode;
        };

        EditState *EditCmd;
    }
}

using namespace main_promptline;


EditState::EditState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    edit = 0;
    edit_mode = PLedNormal;
}


EditState::~EditState()
{
    EditCmd = 0;
}


// Handle button presses in the viewports.  If the user clicks on
// something with a hypertext property, we have to link this property
// into the hypertext list being edited.
//
void
EditState::b1down()
{
    if (edit_mode == PLedIgnoreBtn)
        return;
    if (edit) {
        if (edit_mode == PLedEndBtn) {
            edit->finish(false);
            return;
        }
        edit->button1_handler(false);
    }
}


void
EditState::b1up()
{
    if (edit_mode == PLedIgnoreBtn)
        return;
    if (edit) {
        if (edit_mode == PLedEndBtn)
            return;
        edit->button1_handler(true);
    }
}


void
EditState::b1down_altw()
{
    if (edit_mode == PLedIgnoreBtn)
        return;
    if (edit) {
        if (edit_mode == PLedEndBtn) {
            edit->finish(false);
            return;
        }
        edit->button1_handler(false);
    }
}


void
EditState::b1up_altw()
{
    if (edit_mode == PLedIgnoreBtn)
        return;
    if (edit) {
        if (edit_mode == PLedEndBtn)
            return;
        edit->button1_handler(true);
    }
}


// Esc was entered.  Clean up and quit.
//
void
EditState::esc()
{
    if (edit)
        edit->finish(true);
    delete this;
}


// Key processing.
//
bool
EditState::key(int code, const char *text, int mstate)
{
    if (edit)
        return (edit->key_handler(code, text, mstate));
    return (false);
}


void
EditState::undo()
{
}


void
EditState::redo()
{
}
// End of EditState functions


//-----------------------------------------------------------------------------
// cPromptEdit methods

cPromptEdit::cPromptEdit()
{
    pe_lt_popup = 0;

    pe_down_callback = 0;
    pe_up_callback = 0;
    pe_ctrl_d_callback = 0;

    pe_pxdesc = 0;
    pe_pxent = 0;

    pe_active = hyOFF;
    pe_colmin = 0;

    pe_cwid = 0;
    pe_xpos = pe_ypos = 0;
    pe_offset = 0;
    pe_fntwid = 0;
    pe_column = 0;
    pe_firstinsert = false;
    pe_indicating = false;
    pe_disabled = false;
    pe_obscure_mode = false;
    pe_using_popup = false;
    pe_long_text_mode = false;
    pe_in_select = false;
    pe_entered = false;

    pe_down = false;
    pe_press_x = 0;
    pe_press_y = 0;
    pe_last_string = 0;
    pe_last_ent = 0;

    pe_has_drag = false;
    pe_dragged = false;
    pe_drag_x = 0;
    pe_drag_y = 0;
    pe_sel_start = 0;
    pe_sel_end = 0;
    pe_unichars = 0;
}


void
cPromptEdit::init()
{
    init_window();
    int fw, fh;
    TextExtent(0, &fw, &fh);
    pe_xpos = 2;
    pe_ypos = fh + 2;
    pe_fntwid = fw;
    Update();
}


namespace {
    int prompt_idle_id;

    int prompt_idle(void *arg)
    {
        char *s = (char*)arg;
        PL()->ShowPrompt(s);
        delete [] s;
        prompt_idle_id = 0;
        return (0);
    }
}


// Set the prompt-line text (non-editing mode).
//
void
cPromptEdit::set_prompt(char *buf)
{
    if (pe_active == hyACTIVE) {
        // The prompt line editor is active, use a pop-up window for
        // the message.
        DSPmainWbag(PopUpInfo(MODE_ON, buf))
        set_using_popup(true);
        DSPmainDraw(Update())
        return;
    }
    if (pe_active == hyESCAPE) {
        // In the process of exiting from the editor.  Defer the
        // display in the prompt line until the editor has had a
        // chance to finalize.  The editor has its own event loop,
        // which will probably break before the idle proc is called. 
        // Try to avoid an infinite loop here by not setting up an
        // idle proc is one is already pending.

        if (!prompt_idle_id)
            prompt_idle_id = dspPkgIf()->RegisterIdleProc(prompt_idle,
                lstring::copy(buf));
        return;
    }

    deselect();

    // Limit the string to the display width, will show rightmost part
    // of string.
    int wd = win_width(true);  // prompt line visible char width
    int len = strlen(buf);
    int ofs = 0;
    if (wd && len > wd)
        ofs = len - wd;

    int i;
    char *t;
    for (i = 0, t = buf + ofs; *t; i++, t++) {
        if (*t < ' ')
            *t = ' ';
        pe_buf.set_char(*t, i);
    }
    pe_buf.set_ent(0, 0, i);

    set_col_min(-1); // Force text in prompt edit color.
    draw_text(DRAW, TOEND, true);
    set_col_min(0);
    DSPmainDraw(Update())
}


// Return the prompt-line plain text, if not in editing mode.
//
char *
cPromptEdit::get_prompt()
{
    if (is_off())
        return (pe_buf.get_plain_text(0));
    return (0);
}


// Return an hyList from the prompt_line text, editing or not.
//
hyList *
cPromptEdit::get_hyList(bool pre)
{
    if (pe_disabled)
        return (0);
    return (pe_buf.get_hyList(pe_colmin, pre));
}


// Edit plain text (no hypertext).  If stuff-text is non-0, return
// immediately with this text if update is Start.  If prompt is non-0,
// this text will be shown ahead of the editable text, in a different
// color.  The prompt text is not editable, and will not be included
// in the edited text string returned.  If string is not 0, it is read
// into the editable area.  If update is Update, overwrite the prompt
// and editable text currently in the editor and return.  In update is
// Insert, the string is inserted at the current insertion point, and
// the function will return.  Otherwise, this will block until the
// user presses Enter of Esc, or the editor is otherwise terminated. 
// On Esc or non-normal termination, 0 is returned.
//
char *
cPromptEdit::edit_plain_text(const char *prompt, const char *string,
    const char *stuff_text, PLedType update, PLedMode emode)
{
    // Can't edit when busy, hangs program!
    if (dspPkgIf()->IsBusy())
        return (0);

    if (update == PLedStart) {
        if (is_active())
            return (0);
        // If there is something in the stuff_text, return it.
        if (stuff_text) {
            PL()->ShowPromptV("%s%s", prompt ? prompt : "", stuff_text);
            return (pe_buf.set_plain_text(stuff_text));
        }
        if (dspPkgIf()->IsBusy()) {
            // If the Busy flag is set, then entering the editor will
            // hang the program.  Run the event queue and if still set
            // bail out.

            dspPkgIf()->CheckForInterrupt();
            if (dspPkgIf()->IsBusy()) {
                Log()->ErrorLog(mh::Internal,
                    "Busy flag set, edit aborted.");
                return (0);
            }
        }
        init_edit(emode);
    }
    else if (update == PLedInsert) {
        if (is_active())
            insert(string);
        return (0);
    }

    // Updating with prompt == 0 reuses old prompt.
    bool free_prompt = false;
    if (update == PLedUpdate && !prompt) {
        hyList *hp = get_hyList(true);
        prompt = hyList::string(hp, HYcvPlain, true);
        hyList::destroy(hp);
        free_prompt = true;
    }

    clear_cols_to_end(0);
    int i = 0;
    const char *t;
    if (prompt) {
        for (t = prompt; *t; i++, t++)
            pe_buf.set_char(*t, i);
        if (free_prompt)
            delete [] prompt;
    }
    set_col_min(i);
    if (string) {
        for (t = string; *t; i++, t++)
            pe_buf.set_char(*t, i);
    }
    pe_buf.set_ent(0, 0, i);
    if (update == PLedUpdate) {
        set_col(0);
        draw_text(DRAW, TOEND, false);
        set_col(pe_colmin);
        draw_cursor(DRAW);
        return (0);
    }

    if (editor())
        return (0);

    return (pe_buf.get_plain_text(pe_colmin));
}


namespace {
    // Callback used in the edit_hypertext method.
    //
    bool
    s_cb(const char *string, void *arg, XEtype)
    {
        if (!arg)
            HYlt::lt_clear();
        else {
#ifdef WIN32
            if (string) {
                // Strip out '\r' chars, these can cause trouble.
                char *text = new char[strlen(string) + 1];
                const char *s = string;
                char *t = text;
                while (*s) {
                    if (*s != '\r')
                        *t++ = *s;
                    s++;
                }
                *t = 0;
                HYlt::lt_update(arg, text);
                delete [] text;
                return (true);
            }
#endif
            HYlt::lt_update(arg, string);
        }
        return (true);
    }
}


// Editor for hypertext, similar to the plain-text version.  If the
// long-text feature is enabled, this may return immediately after
// launching a string editor pop-up.  The callback is called when
// editing from the pop-up is complete.
//
hyList *
cPromptEdit::edit_hypertext(const char *prompt, hyList *h,
    const char *stuff_text, bool use_long_text, PLedType update,
    PLedMode emode, LongTextCallback ltcb, void *ltarg)
{
    if (pe_lt_popup && use_long_text)
        pe_lt_popup->popdown();

    // Can't edit when busy, hangs program!
    if (dspPkgIf()->IsBusy())
        return (0);

    set_long_text_mode(use_long_text);
    if (update == PLedStart) {
        if (is_active())
            return (0);
        // If there is something in the stuff_text, return it
        if (stuff_text) {
            PL()->ShowPromptV("%s%s", prompt ? prompt : "", stuff_text);
            int i;
            const char *s;
            for (i = 0, s = stuff_text; *s; i++, s++)
                pe_buf.set_char(*s, i);
            pe_buf.set_ent(0, 0, i);
            return (get_hyList());
        }
        if (dspPkgIf()->IsBusy()) {
            // If the Busy flag is set, then entering the editor will
            // hang the program.  Run the event queue and if still set
            // bail out.

            dspPkgIf()->CheckForInterrupt();
            if (dspPkgIf()->IsBusy()) {
                Log()->ErrorLog(mh::Initialization,
                    "Busy flag set, edit aborted.");
                return (0);
            }
        }
        init_edit(emode);
    }
    else if (update == PLedInsert) {
        if (is_active())
            insert(h);
        return (0);
    }

    // Updating with prompt == 0 reuses old prompt.
    bool free_prompt = false;
    if (update == PLedUpdate && !prompt) {
        hyList *hp = get_hyList(true);
        prompt = hyList::string(hp, HYcvPlain, true);
        hyList::destroy(hp);
        free_prompt = true;
    }

    clear_cols_to_end(0);
    int i = 0;
    const char *s;
    if (prompt) {
        for (s = prompt; *s; i++, s++)
            pe_buf.set_char(*s, i);
        if (free_prompt)
            delete [] prompt;
    }
    set_col_min(i);

    bool force_lt = false;
    if (h && h->ref_type() == HLrefLongText) {
        pe_buf.element(i)->set_lt(lstring::copy(h->text()));
        i++;
    }
    else {
        for (hyList *hh = h; hh; hh = hh->next()) {
            if (hh->ref_type() == HLrefText) {
                int k = strlen(hh->text());
                for (int j = 0; j < k; j++, i++)
                    pe_buf.set_char(hh->text()[j], i);
            }
            else {
                if (hh->hent() && hh->hent()->ref_type() == HYrefBogus)
                    continue;
                char *str = hyList::get_entry_string(hh);
                if (hh->ref_type() == HLrefLongText)
                    pe_buf.set_lt(str, i);
                else if (hh->hent()) {
                    if (!str)
                        str = lstring::copy("???");
                    pe_buf.set_ent(str, hh->hent()->dup(), i);
                    pe_buf.upd_type(hh->ref_type(), i);
                }
                i++;
            }
        }
    }
    pe_buf.set_ent(0, 0, i);
    if (update == PLedUpdate) {
        set_col(0);
        draw_text(DRAW, TOEND, false);
        if (ScedIf()->doingPlot()) {
            draw_marks(DRAW);
            ScedIf()->setPlotMarkColors();
        }
        set_col(pe_colmin);
        draw_cursor(DRAW);
        return (0);
    }
    if (!force_lt) {
        i = editor();
        if (i)
            return (0);
    }
    hyList *hh = get_hyList();
    if (hh && hh->ref_type() == HLrefLongText) {
        pe_lt_popup = DSPmainWbagRet(PopUpStringEditor(hh->text(), s_cb,
            HYlt::lt_new(hh, ltcb, ltarg)));
        if (pe_lt_popup)
            // Pointer is cleared when pop-up is dismissed.
            pe_lt_popup->register_usrptr((void**)&pe_lt_popup);
            
    }
    if (force_lt)
        finish(false);
    return (hh);
}


void
cPromptEdit::init_edit(PLedMode mode)
{
    if (pe_disabled)
        return;
    // This is a little funny - EditCmd is not deleted except when an
    // Esc is caught, or an error occurs.
    if (!EditCmd) {
        EditCmd = new EditState("TEXTINPUT", 0);
        EditCmd->setup(mode, this);
    }
    else
        EditCmd->SetEditMode(mode);
    if (!EV()->PushCallback(EditCmd)) {
        delete EditCmd;
        EditCmd = 0;
    }
}


// Core editor function.
//
bool
cPromptEdit::editor()
{
    if (pe_disabled)
        return (false);
    // The exit_hack ensures that if call depth is > 1, all but the first
    // returns true.
    static bool exit_hack;
    exit_hack = true;
    pe_active = hyACTIVE;
    pe_offset = 0;
    set_focus();
    indicate(true);
    Menu()->SetUndoSens(false);
    pe_column = 0;
    set_col(0);
    draw_text(DRAW, TOEND, true);

    if (ScedIf()->doingPlot()) {
        // if in the plot command, set the cursor to the end
        draw_marks(DRAW);
        ScedIf()->setPlotMarkColors();
        set_col(TOEND);
        draw_cursor(DRAW);
        pe_firstinsert = false;
    }
    else {
        set_col(pe_colmin);
        draw_cursor(DRAW);
        pe_firstinsert = true;
    }
    warp_pointer();

    if (!KbMac()->MacroPush(0))
        GRpkgIf()->MainLoop();

    Menu()->SetUndoSens(true);
    indicate(false);
    if (pe_active == hyESCAPE) {
        pe_active = hyOFF;
        exit_hack = false;
        return (true);
    }
    pe_active = hyOFF;
    if (!exit_hack)
        return (true);
    exit_hack = false;
    return (false);
}


// Pop down the long-text editor window.
//
void
cPromptEdit::abort_long_text()
{
    if (pe_lt_popup)
        pe_lt_popup->popdown();
}


void
cPromptEdit::abort()
{
    if (EditCmd && pe_active == hyACTIVE)
        EditCmd->Terminate();
}


// Function called on termination with Enter or Esc.
//
void
cPromptEdit::finish(bool esc_entered)
{
    if (pe_disabled)
        return;
    if (!esc_entered)
        save_line();
    pe_long_text_mode = false;
    draw_cursor(UNDRAW);
    set_col(0);
    if (ScedIf()->doingPlot())
        draw_marks(ERASE);
    if (esc_entered) {
        pe_cwid = 1;
        draw_text(UNDRAW, TOEND, false);
        del_col(0, pe_buf.size());
        pe_buf.set_ent(0, 0, 0);
        pe_active = hyESCAPE;
    }
    else
        pe_active = hyOFF;
    EV()->PopCallback(EditCmd);
    show_lt_button(false);
    if (!KbMac()->MacroPop()) {
        if (GRpkgIf()->LoopLevel() > 1)
            GRpkgIf()->BreakLoop();
    }
}


namespace {
    // Return true if the type corresponds to a plot mark.
    //
    inline bool is_plref(HLrefType r)
    {
        return (r & (HLrefNode | HLrefBranch));
    }
}


// Insert an item into the hypertext list, and advance the cursor.
//
void
cPromptEdit::insert(sHtxt *hret)
{
    if (pe_disabled)
        return;
    if (pe_buf.element(pe_colmin)->type() == HLrefLongText)
        // lock out if long text block
        return;
    // If the user inserts something at colmin before any cursor motion
    // or deletion, assume a typeover is desired.
    if (pe_firstinsert) {
        pe_firstinsert = false;
        if (pe_column == pe_colmin) {
            // may not be colmin in paste
            clear_cols_to_end(pe_column);
        }
    }
    draw_cursor(UNDRAW);
    draw_text(UNDRAW, TOEND, false);
    if (ScedIf()->doingPlot() && is_plref(hret->type()))
        draw_marks(ERASE);
    pe_buf.insert(hret, pe_column);
    draw_text(DRAW, TOEND, false);
    if (ScedIf()->doingPlot()) {
        if (is_plref(hret->type()))
            draw_marks(DRAW);
        ScedIf()->setPlotMarkColors();
    }
    set_col(pe_column+1);
    draw_cursor(DRAW);
}


// Insert a string into the editor at the current position, cursor is
// advanced to end of insertion.
//
void
cPromptEdit::insert(const char *string)
{
    if (!string)
        return;
    sHtxt hret;
    for ( ; *string; string++) {
        if (*string & 0x80) {
            char tbf[8];
            int i = 0;
            for ( ; i < 6; i++) {
                if (!(string[i] & 0x80))
                    break;
                if (i && (string[i] & 0x40))
                    break;
                tbf[i] = string[i];
            }
            tbf[i] = 0;
            hret.set_unichar(tbf);
            insert(&hret);
            string += i-1;
        }
        else {
            hret.set_char(*string);
            insert(&hret);
        }
    }
}


// Insert an hyList into the editor at the current position, cursor
// is advanced to end of insertion.
//
void
cPromptEdit::insert(hyList *hpl)
{
    if (!hpl)
        return;
    sHtxt hret;
    for (hyList *h = hpl; h; h = h->next()) {
        if (h->ref_type() == HLrefText)
            insert(h->text());
        else if (h->ref_type() == HLrefLongText) {
            if (pe_column == pe_colmin) {
                hret.set_lt(lstring::copy(h->text()));
                insert(&hret);
            }
            else
                insert(h->text());
        }
        else if (h->hent()) {
            char *s = hyList::get_entry_string(h);
            if (!s)
                s = lstring::copy("<unknown>");
            hret.set_ent(s, h->hent()->dup());
            hret.upd_type(h->ref_type());  // Is this ever needed?
            insert(&hret);
        }
    }
}


// Update the entry at col with hret, redraw.
//
void
cPromptEdit::replace(int col, sHtxt *hret)
{
    if (pe_disabled)
        return;
    if (col < 0 || col >= pe_buf.size())
        return;

    draw_cursor(UNDRAW);
    set_col(col);
    draw_text(UNDRAW, TOEND, false);
    if (ScedIf()->doingPlot() && is_plref(hret->type()))
        draw_marks(ERASE);

    pe_buf.replace(hret, col);

    draw_text(DRAW, TOEND, false);
    if (ScedIf()->doingPlot()) {
        if (is_plref(hret->type()))
            draw_marks(DRAW);
        ScedIf()->setPlotMarkColors();
    }
    draw_cursor(DRAW);
}


// This just swaps the two references.
//
void
cPromptEdit::rotate_plotpts(int from, int to)
{
    if (!ScedIf()->doingPlot())
        return;
    draw_cursor(UNDRAW);
    set_col(pe_colmin);
    draw_text(UNDRAW, TOEND, false);
    draw_marks(ERASE);

    pe_buf.swap(from, to);

    draw_text(DRAW, TOEND, false);
    draw_marks(DRAW);
    ScedIf()->setPlotMarkColors();
    draw_cursor(DRAW);
}


// Primitive to delete num columns starting at col from the hypertext
// list.
//
void
cPromptEdit::del_col(int col, int num)
{
    if (pe_disabled)
        return;
    if (col < 0 || col >= pe_buf.size())
        return;
    if (num < 0)
        return;
    num += col;
    if (num > pe_buf.size())
        num = pe_buf.size();

    // If we delete a long text entry, stuff the text into the prompt
    // line.
    if (col == pe_colmin && num == pe_colmin+1 &&
            pe_buf.element(pe_colmin)->type() == HLrefLongText) {
        const char *s = pe_buf.element(pe_colmin)->string();
        if (!s)
            s = "";
        for (int i = pe_colmin; ; i++) {
            if (!*s) {
                pe_buf.set_ent(0, 0, i);
                break;
            }
            pe_buf.set_char(*s, i);
            s++;
        }
        return;
    }
    for ( ; col < num; col++)
        pe_buf.clear_free(col);
}


// Delete column col and redisplay the text, deleting any markers
// associated with the deleted object reference.
//
void
cPromptEdit::show_del(int col)
{
    if (pe_disabled)
        return;
    draw_cursor(UNDRAW);
    set_col(col);
    draw_text(UNDRAW, TOEND, false);
    bool was_plref = is_plref(pe_buf.element(col)->type());
    if (ScedIf()->doingPlot() && was_plref)
        draw_marks(ERASE);
    pe_buf.remove(col);
    draw_text(DRAW, TOEND, false);
    set_col(col);
    draw_cursor(DRAW);
    if (ScedIf()->doingPlot()) {
        if (was_plref)
            draw_marks(DRAW);
        ScedIf()->setPlotMarkColors();
    }
}


// Clear the hypertext list being edited from col to the end.
//
void
cPromptEdit::clear_cols_to_end(int col)
{
    if (pe_disabled)
        return;
    draw_cursor(UNDRAW);
    set_col(col);
    if (ScedIf()->doingPlot())
        draw_marks(ERASE);
    draw_text(UNDRAW, TOEND, false);
    del_col(col, pe_buf.size());
}


// Primitive to set the current column.
//
void
cPromptEdit::set_col(int col, bool nosetlt)
{
    if (pe_disabled)
        return;
    if (col == TOEND)
        col = pe_buf.endcol();
    switch (pe_buf.element(col)->type()) {
    case HLrefText:
    case HLrefEnd:
        pe_cwid = 1;
        break;
    case HLrefNode:
    case HLrefBranch:
    case HLrefDevice:
        if (pe_buf.element(col)->string())
            pe_cwid = strlen(pe_buf.element(col)->string());
        else
            pe_cwid = 1;
        break;
    case HLrefLongText:
        pe_cwid = strlen(HY_LT_MSG);
        break;
    default:
        // Points at garbage, make it an end record.
        pe_buf.set_ent(0, 0, col);
        pe_cwid = 1;
    }
    pe_column = col;

    if (!nosetlt) {
        if (pe_column == pe_colmin && pe_long_text_mode == true)
            show_lt_button(true);
        else
            show_lt_button(false);
    }
}


// This is to offset the text within the drawing window to make it visible.
//
void
cPromptEdit::set_offset(int org)
{
    int fw;
    TextExtent(0, &fw, 0);
    fw *= 8;
    if (org > 0) {
        if (pe_offset == 0)
            return;
        // move text right
        pe_offset += fw;
        if (pe_offset > 0)
            pe_offset = 0;
    }
    else
        // move text left
        pe_offset -= fw;

    int tmp = pe_column;
    set_col(0, true);
    draw_text(DRAW, TOEND, true);
    set_col(tmp, true);
}


namespace {
    // Return the number of characters, assuming Unicode.  The Unicode
    // sequences count as a single character.
    //
    int numchars(const char *s)
    {
        int k = 0;
        if (s) {
            while (*s) {
                if (*s & 0x80) {
                    k++;
                    s++;
                    while ((*s & 0x80) && !(*s & 0x40))
                        s++;
                }
                else {
                    k++;
                    s++;
                }
            }
        }
        return (k);
    }


    // Put the nth character into cret and return true if possible,
    // return false otherwise.
    //
    bool nthchar(const char *s, char *cret, int n)
    {
        int k = 0;
        if (s) {
            while (*s) {
                if (*s & 0x80) {
                    if (k == n) {
                        int i = 0;
                        cret[i++] = *s++;
                        while ((*s & 0x80) && !(*s & 0x40)) {
                            if (i == 7)
                                return (false);
                            cret[i++] = *s++;
                        }
                        cret[i] = 0;
                        return (true);
                    }
                    k++;
                    s++;
                    while ((*s & 0x80) && !(*s & 0x40))
                        s++;
                }
                else {
                    if (k == n) {
                        cret[0] = *s;
                        cret[1] = 0;
                        return (true);
                    }
                    k++;
                    s++;
                }
            }
        }
        return (false);
    }
}


// Low-level text rendering.  This draws one character at a time,
// which is needed for some fonts that may be "monospace" but
// apparently use kerning, since the string rendering length is
// different from the character-counting width.
//
void
cPromptEdit::text(const char *str, int xpos)
{
    char tb[8];
    int end = numchars(str);
    for (int j = 0; j < end; j++) {
        if (nthchar(str, tb, j))
            Text(tb, xpos + j*pe_fntwid, pe_ypos, 0);
    }
}


// Display ncols of hypertext string, starting at the current
// logical column, if ncols is TOEND, show to end.
//
void
cPromptEdit::draw_text(bool draw, int ncols, bool clear)
{
    if (pe_disabled)
        return;
    int realcol = 0, i;
    for (i = 0; i < pe_column; i++) {
        if (pe_buf.element(i)->type() == HLrefEnd)
            break;
        if (pe_buf.element(i)->type() == HLrefText)
            realcol++;
        else if (pe_buf.element(i)->type() == HLrefLongText)
            realcol += strlen(HY_LT_MSG);
        else
            realcol += numchars(pe_buf.element(i)->string());
    }
    if (i < pe_column)
        return;

    int tendcol;
    if (ncols == TOEND)
        tendcol = pe_buf.size()-1;
    else {
        tendcol = i + ncols;
        if (tendcol >= pe_buf.size())
            tendcol = pe_buf.size()-1;
    }

    void *tmp_window = setup_backing(clear);

    int endcol;
    bool pass2;
    unsigned long fgpix;
    if (i < pe_colmin) {
        endcol = pe_colmin;
        pass2 = true;
        fgpix = DSP()->Color(PromptTextColor);
    }
    else {
        endcol = tendcol;
        pass2 = false;
        fgpix = DSP()->Color(PromptEditTextColor);
    }
    SetLinestyle(0);
    SetFillpattern(0);
    if (clear) {
        SetColor(bg_pixel());
        Clear();
    }

    sLstr lstr;

again:
    for (;;) {
        if (pe_buf.element(i)->type() == HLrefEnd || i == endcol) {
            if (lstr.string()) {
                int xpos = pe_xpos + realcol*pe_fntwid + pe_offset;
                int xw = numchars(lstr.string());
                if (draw) {
                    if (pe_active == hyOFF && pe_sel_end > pe_sel_start) {
                        // Display selected text as inverse color.
                        char tb[8];
                        for (int j = 0; j < xw; j++) {
                            if (nthchar(lstr.string(), tb, j)) {
                                if (j + realcol >= pe_sel_start &&
                                        j + realcol < pe_sel_end) {
                                    SetColor(bg_pixel() ^ -1);
                                    Box(xpos, pe_ypos, xpos + pe_fntwid, 2);
                                    SetColor(bg_pixel());
                                    Text(tb, xpos, pe_ypos, 0);
                                }
                                else {
                                    SetColor(fgpix);
                                    Text(tb, xpos, pe_ypos, 0);
                                }
                            }
                            xpos += pe_fntwid;
                        }
                    }
                    else {
                        SetColor(fgpix);
                        text(lstr.string(), pe_xpos + realcol*pe_fntwid +
                            pe_offset);
                    }
                }
                else {
                    SetColor(bg_pixel());
                    Box(xpos, pe_ypos, xpos + xw*pe_fntwid, 0);
                }
                realcol += xw;
                lstr.free();
            }
            break;
        }
        if (pe_buf.element(i)->type() == HLrefText) {
            if (!pass2 && pe_active == hyACTIVE && pe_obscure_mode)
                // Obscure entered text, for password entry.
                lstr.add_c('*');
            else
                lstr.add(pe_buf.element(i)->chr());
        }
        else {
            if (lstr.string()) {
                int xpos = pe_xpos + realcol*pe_fntwid + pe_offset;
                int xw = numchars(lstr.string());
                if (draw) {
                    SetColor(fgpix);
                    text(lstr.string(), xpos);
                }
                else {
                    SetColor(bg_pixel());
                    Box(xpos, pe_ypos, xpos + xw*pe_fntwid, 0);
                }
                realcol += xw;
                lstr.free();
            }
            const char *s = pe_buf.element(i)->type() == HLrefLongText ?
                HY_LT_MSG : pe_buf.element(i)->string();
            if (s && *s) {
                int xpos = pe_xpos + realcol*pe_fntwid + pe_offset;
                int xw = numchars(s);
                if (draw) {
                    SetColor(DSP()->Color(PromptHighlightColor));
                    text(s, xpos);
                }
                else {
                    SetColor(bg_pixel());
                    Box(xpos, pe_ypos, xpos + xw*pe_fntwid, 0);
                }
                realcol += xw;
            }
        }
        i++;
    }
    if (pass2) {
        fgpix = DSP()->Color(PromptEditTextColor);
        pass2 = false;
        endcol = tendcol;
        goto again;
    }
    restore_backing(tmp_window);
    if (!tmp_window)
        Update();
}


// Erase or draw the cursor.
//
void
cPromptEdit::draw_cursor(bool draw)
{
    if (pe_disabled)
        return;
    int realcol = 0;
    bool at_end = false;
    for (int i = 0; i < pe_column; i++) {
        if (pe_buf.element(i)->type() == HLrefEnd) {
            realcol++;
            at_end = true;
            break;
        }
        if (pe_buf.element(i)->type() == HLrefText)
            realcol++;
        else if (pe_buf.element(i)->type() == HLrefLongText)
            realcol += strlen(HY_LT_MSG);
        else
            realcol += numchars(pe_buf.element(i)->string());
    }
    if (!at_end) {
        int i = pe_column;
        if (pe_buf.element(i)->type() == HLrefEnd) {
            at_end = true;
            pe_cwid = 1;
        }
        else if (pe_buf.element(i)->type() == HLrefText)
            pe_cwid = 1;
        else if (pe_buf.element(i)->type() == HLrefLongText)
            pe_cwid = strlen(HY_LT_MSG);
        else
            pe_cwid = numchars(pe_buf.element(i)->string());
    }
    int x = pe_xpos + realcol * pe_fntwid;
    int y = pe_ypos + 1;
    int w = pe_cwid * pe_fntwid;

    if (x + pe_offset < 2) {
        while (x + pe_offset < 2)
            set_offset(1);
    }
    else {
        while (x + pe_offset + w > win_width() - 2)
            set_offset(-1);
    }

    // Erase character and cursor.  Text rendering with pixel aliasing
    // looks bad when chars are overwritten.
    //
    SetColor(bg_pixel());
    Box(x + pe_offset, 0, x + pe_offset + w - 1, y + CURHT);

    if (draw) {
        SetColor(DSP()->Color(PromptCursorColor));
        Box(x + pe_offset, y, x + pe_offset + w - 1, y + CURHT);
    }

    if (at_end)
        return;

    int i = pe_column;
    int xpos = pe_xpos + realcol*pe_fntwid + pe_offset;
    if (pe_buf.element(i)->type() == HLrefText) {
        SetColor(DSP()->Color(PromptEditTextColor));
        if (pe_obscure_mode)
            text("*", xpos);
        else
            text(pe_buf.element(i)->chr(), xpos);
    }
    else {
        const char *s = pe_buf.element(i)->type() == HLrefLongText ?
            HY_LT_MSG : pe_buf.element(i)->string();
        if (s && *s) {
            SetColor(DSP()->Color(PromptHighlightColor));
            text(s, xpos);
        }
    }
    Update();
}


// Draw or Erase viewport markers associated with hypertext objects
// currently included in the editing list.
//
void
cPromptEdit::draw_marks(bool draw)
{
    if (pe_disabled)
        return;
    int i, indx = 0;
    for (i = pe_colmin; i < pe_column; i++) {
        if (is_plref(pe_buf.element(i)->type()))
            indx++;
    }
    for ( ; i < pe_buf.size(); i++) {
        if (pe_buf.element(i)->type() == HLrefEnd)
            break;
        if (is_plref(pe_buf.element(i)->type())) {
            DSP()->ShowPlotMark(draw, pe_buf.element(i)->hyent(), indx, false);
            indx++;
        }
    }
}


void
cPromptEdit::redraw()
{
    bool use_pm = check_pixmap();

    SetWindowBackground(bg_pixel());
    int tmp = pe_column;
    set_col(0, true);
    if (pe_active == hyOFF)
        pe_colmin = -1; // force text in prompt color
    draw_text(DRAW, TOEND, use_pm);
    set_col(tmp, true);
    if (pe_active == hyACTIVE)
        draw_cursor(DRAW);
    if (pe_active == hyOFF)
        pe_colmin = 0;
}


int
cPromptEdit::bg_pixel()
{
    if (!pe_indicating)
        return (DSP()->Color(PromptBackgroundColor));
    if (!pe_entered)
        return (DSP()->Color(PromptEditBackgColor));
    return (DSP()->Color(PromptEditFocusBackgColor));
}


// Switch the editor to indicate editing mode.
//
void
cPromptEdit::indicate(bool active)
{
    pe_indicating = active;
    set_indicate();
}


#define CTRL_A 1
#define CTRL_D 4
#define CTRL_E 5
#define CTRL_K 11
#define CTRL_P 16
#define CTRL_T 20
#define CTRL_U 21
#define CTRL_V 22


// Key press handler.
//
bool
cPromptEdit::key_handler(int code, const char *txt, int mstate)
{
    int ch = txt ? *txt : 0;

    if (pe_unichars) {
        if (ch && pe_unichars->addc(ch))
            return (true);
        if (code == SHIFTUP_KEY || code == CTRLUP_KEY)
            return (true);
        const char *uc = pe_unichars->utf8_encode();
        if (uc) {
            sHtxt hret;
            hret.set_unichar(uc);
            insert(&hret);
        }
        delete pe_unichars;
        pe_unichars = 0;
        return (true);
    }

    switch (code) {
    case BSP_KEY:
        ch = '\b';
        break;

    // The arrow keys will have the normal pan punction unless the
    // mouse pointer is over the prompt line.

    case DOWN_KEY:
        if (!(mstate & GR_SHIFT_MASK) && !(mstate & GR_CONTROL_MASK)) {
            if (EV()->IsFromPromptLine()) {
                if (exec_down_callback())
                    return (true);
            }
        }
        return (false);
    case RIGHT_KEY:
        if (!(mstate & GR_SHIFT_MASK) && !(mstate & GR_CONTROL_MASK)) {
            if (EV()->IsFromPromptLine()) {
                if (pe_buf.element(pe_column)->type() == HLrefEnd)
                    return (true);
                pe_firstinsert = false;
                draw_cursor(UNDRAW);
                set_col(pe_column+1);
                draw_cursor(DRAW);
                return (true);
            }
        }
        return (false);
    case UP_KEY:
        if (!(mstate & GR_SHIFT_MASK) && !(mstate & GR_CONTROL_MASK)) {
            if (EV()->IsFromPromptLine()) {
                if (exec_up_callback())
                    return (true);
            }
        }
        return (false);
    case LEFT_KEY:
        if (!(mstate & GR_SHIFT_MASK) && !(mstate & GR_CONTROL_MASK)) {
            if (EV()->IsFromPromptLine()) {
                if (pe_column <= pe_colmin) {
                    if (pe_offset != 0) {
                        set_offset(1);
                        draw_cursor(DRAW);
                    }
                    return (true);
                }
                draw_cursor(UNDRAW);
                set_col(pe_column-1);
                draw_cursor(DRAW);
                return (true);
            }
        }
        return (false);
    case DELETE_KEY:
        if (ch == CTRL_D) {
            // If the callback is set, do the operation and then exit,
            // as if Esc was entered.
            if (exec_ctrl_d_callback() && EditCmd)
                EditCmd->Terminate();
        }
        if (pe_buf.element(pe_column)->type() == HLrefEnd)
            return (true);
        pe_firstinsert = false;
        show_del(pe_column);
        return (true);

    case PAGEDN_KEY:
        {
            int col = pe_column + win_width(true)/2;
            int end = pe_buf.endcol();
            if (col > end)
                col = end;
            pe_firstinsert = false;
            draw_cursor(UNDRAW);
            set_col(col);
            draw_cursor(DRAW);
        }
        return (true);

    case PAGEUP_KEY:
        {
            int col = pe_column - win_width(true)/2;
            if (col < pe_colmin)
                col = pe_colmin;
            draw_cursor(UNDRAW);
            set_col(col);
            if (col == pe_colmin && pe_offset) {
                pe_offset = 0;
                int tmp = pe_column;
                set_col(0, true);
                draw_text(DRAW, TOEND, true);
                set_col(tmp, true);
            }
            draw_cursor(DRAW);
        }
        return (true);

    case HOME_KEY:
        return (false);
    case NUPLUS_KEY:
    case NUMINUS_KEY:
        // If the mouse pointer is over the prompt line, don't treat
        // these specially.
        if (EV()->IsFromPromptLine())
            break;
        return (false);
    case FUNC_KEY:
        return (true);
    }

    if (ch) {
        sHtxt hret;
        switch (ch) {
        case '\b':
            pe_firstinsert = false;
            if (pe_column <= pe_colmin)
                return (true);
            show_del(pe_column-1);
            break;
        case CTRL_U:
            if (mstate & GR_SHIFT_MASK) {
                // User is pressing Ctrl-Shift-u, signaling the start of
                // a Unicode character.

                pe_unichars = new sUni;
                return (true);
            }
            clear_cols_to_end(pe_colmin);
            pe_cwid = 1;
            if (pe_offset) {
                pe_offset = 0;
                redraw();
            }
            draw_cursor(DRAW);
            break;
        case CTRL_K:
            clear_cols_to_end(pe_column);
            draw_cursor(DRAW);
            break;
        case CTRL_A:
            draw_cursor(UNDRAW);
            set_col(pe_colmin);
            if (pe_offset) {
                pe_offset = 0;
                int tmp = pe_column;
                set_col(0, true);
                draw_text(DRAW, TOEND, true);
                set_col(tmp, true);
            }
            draw_cursor(DRAW);
            break;
        /* mapped to delete key
        case CTRL_D:
            // If the callback is set, do the operation and then exit,
            // as if Esc was entered.
            if (exec_ctrl_d_callback())
                esc();
            break;
        */
        case CTRL_E:
            pe_firstinsert = false;
            draw_cursor(UNDRAW);
            set_col(TOEND);
            draw_cursor(DRAW);
            break;
        case CTRL_P:
            // get primary
            get_selection(false);
            break;
        case CTRL_V:
            // get clipboard
            get_selection(true);
            break;
        case '\r':
        case '\n':
            if (mstate & GR_SHIFT_MASK) {
                hret.set_char('\n');
                insert(&hret);
            }
            else
                finish(false);
            break;
        case CTRL_T:  // long text block if active
            if (pe_column == pe_colmin && pe_long_text_mode) {
                hyList *hl = get_hyList();
                hret.set_lt(hyList::string(hl, HYcvPlain, true));
                hyList::destroy(hl);
                clear_cols_to_end(pe_colmin);
                insert(&hret);
                finish(false);
            }
            break;
        default:
            if (ch < ' ' || ch > '~')
                return (false);
            hret.set_char(ch);
            insert(&hret);
        }
    }
    return (true);
}


namespace {
    struct pe_pill
    {
        pe_pill(CDs *sd, hyEnt *ent)
            {
                sdesc = sd;
                hent = ent;
            }

        ~pe_pill()
            {
                delete hent;
            }

        CDs *sdesc;
        hyEnt *hent;
    };


    int subw_idle(void *arg)
    {
        pe_pill *p = (pe_pill*)arg;
        DSP()->OpenSubwin(p->sdesc, p->hent, true);
        delete p;
        return (false);
    }


    // Return true if the hyEnt references an object visible in the
    // window, The return from CD doesn't take into account actual
    // screen visibility.  We don't want to select things that aren't
    // visible.
    //
    bool is_visible(hyEnt *ent)
    {
        WindowDesc *wdesc = EV()->CurrentWin() ?
            EV()->CurrentWin() : DSP()->MainWdesc();
        DSPattrib *a = wdesc->Attrib();
        if (a->expand_level(Electrical) < 0)
            return (true);

        int elev = 0;
        for (hyParent *p = ent->parent(); p; p = p->next())
            elev++;
        if (a->expand_level(Electrical) >= elev)
            return (true);
        if (ent->parent() &&
                ent->parent()->cdesc()->has_flag(wdesc->DisplFlags()))
            return (true);
        return (false);
    }


    // Return a string naming the hypertext reference just pressed or
    // lifted over, and the hypertext entry struct.
    // int typemask;        return these types only
    // struct hyEnt **hent; point descriptor, if found
    //
    char *
    hy_string(WindowDesc *wdesc, int typemask, hyEnt **hent, bool up)
    {
        int delta = 1 + (int)(DSP()->PixelDelta()/wdesc->Ratio());

        const CursorDesc &curs = EV()->Cursor();
        int xr, yr;
        if (up) {
            if (curs.is_release_ok())
                curs.get_release(&xr, &yr);
            else if (curs.get_press_alt() && !curs.get_was_press_alt())
                curs.get_alt_up(&xr, &yr);
            else
                return (0);
            wdesc->Snap(&xr, &yr);
        }
        else {
            if (curs.get_press_alt())
                curs.get_alt_down(&xr, &yr);
            else
                curs.get_raw(&xr, &yr);
        }
        BBox BB(xr - delta, yr - delta, xr + delta, yr + delta);

        CDs *cursd = wdesc->CurCellDesc(wdesc->Mode());
        if (!cursd)
            return (0);
        cTfmStack stk;
        char *s = cursd->hyString(&stk, &BB, typemask, hent);
        if (s) {
            hyParent *pl = wdesc->ProxyList();
            if (pl) {
                (*hent)->set_owner(CurCell());
                (*hent)->set_proxy(pl);
                delete [] s;
                s = (*hent)->get_subname(true);
            }
        }
        return (s);
    }
}


// Button-1 press handler, for drawing windows.
//
void
cPromptEdit::button1_handler(bool up)
{
    unsigned int state = 0;
    DSPmainDraw(QueryPointer(0, 0, &state))
    int type;
    if (ScedIf()->doingPlot()) {
        type = HY_NODE | HY_BRAN;
        if (state & GR_SHIFT_MASK)
            type |= HY_DEVN;
    }
    else if (XM()->HtextCnamesOnly())
        type = HY_CELL;
    else
        type = HY_NODE | HY_BRAN | HY_DEVN | HY_LABEL;

    WindowDesc *wdesc = EV()->CurrentWin();
    if (!wdesc)
        return;
    if (wdesc->Mode() != DSP()->CurMode())
        return;
    if (wdesc != DSP()->MainWdesc()) {
        if (wdesc->CurCellName() != DSP()->MainWdesc()->CurCellName() &&
                !wdesc->HasProxy())
            return;
    }

    if (up) {
        if (pe_pxdesc) {
            // Initiate the idle procedure to pop up a sub-window. 
            // This is done only on a click, a drag voids.

            int x, y;
            if (EV()->Cursor().is_release_ok())
                EV()->Cursor().get_release(&x, &y);
            else if (EV()->Cursor().get_press_alt() &&
                    !EV()->Cursor().get_was_press_alt())
                EV()->Cursor().get_alt_up(&x, &y);
            else {
                pe_pxdesc = 0;
                delete pe_pxent;
                pe_pxent = 0;
                return;
            }
            int xp = pe_press_x;
            int yp = pe_press_y;
            wdesc->Snap(&xp, &yp);
            wdesc->Snap(&x, &y);
            if (x != xp || y != yp) {
                pe_pxdesc = 0;
                delete pe_pxent;
                pe_pxent = 0;
                return;
            }

            pe_pill *p = new pe_pill(pe_pxdesc, pe_pxent);
            pe_pxdesc = 0;
            pe_pxent = 0;
            dspPkgIf()->RegisterIdleProc(subw_idle, p);
        }
        else if (pe_in_select) {
            // Select labels only, this accelerates changing property
            // text while plotting.
            cEventHdlr::sel_b1up(0, "l", 0, true);
            pe_in_select = false;
        }
        else if (pe_down) {
            Gst()->SetGhost(GFnone);
            pe_down = false;

            int x, y;
            if (EV()->Cursor().is_release_ok())
                EV()->Cursor().get_release(&x, &y);
            else if (EV()->Cursor().get_press_alt() &&
                    !EV()->Cursor().get_was_press_alt())
                EV()->Cursor().get_alt_up(&x, &y);
            else
                return;

            wdesc->Snap(&x, &y);
            char *string = 0;
            hyEnt *ent = 0;
            if (x != pe_press_x || y != pe_press_y) {
                string = hy_string(wdesc, type, &ent, true);
                if (!string) {
                    // Drag to nowhere, ignore.
                    delete pe_last_string;
                    pe_last_string = 0;
                    delete pe_last_ent;
                    pe_last_ent = 0;
                    return;
                }
            }
            process_b1_text(string, ent);
        }
        return;
    }

    if (pe_buf.element(pe_colmin)->type() == HLrefLongText)
        return;

    hyEnt *ent;
    char *string = hy_string(wdesc, type, &ent, false);
    if (!string) {
        // Look for objects (cells return a string).
        if (!EV()->Cursor().get_press_alt()) {
            if (DSP()->CurMode() == Physical) {
                int xr, yr;
                EV()->Cursor().get_raw(&xr, &yr);
                BBox BB(xr, yr, xr, yr);
                CDs *cursd = CurCell(Physical);
                CDol *ol = Selections.selectItems(cursd, 0, &BB, PSELpoint);
                ol = Selections.filter(cursd, ol, &BB, false);
                CDo *od = 0;
                for (CDol *o = ol; o; o = o->next) {
                    if (XM()->IsBoundaryVisible(cursd, o->odesc)) {
                        od = o->odesc;
                        break;
                    }
                }
                CDol::destroy(ol);
                if (od)
                    EditIf()->prptyCallback(od);
            }
            else if (ScedIf()->doingPlot()) {
                cEventHdlr::sel_b1down();
                pe_in_select = true;
            }
        }
        return;
    }
    if (EditIf()->prptyCallback(ent->odesc())) {
        delete [] string;
        delete ent;
        return;
    }
    if (!ent) {
        // This shouldn't happen.
        delete [] string;
        return;
    }

    // If clicking with ctrl+shift on an instance in physical mode or
    // a subcircuit in electrical mode, we pop up a sub-window showing
    // the master, where we can select nodes in electrical mode.  The
    // master is shown as a schematic if symbolic.  The window pop-up
    // is called from an idle proc set on button-up.

    if ((state & GR_SHIFT_MASK) && (state & GR_CONTROL_MASK)) {
        CDc *cd = 0;
        hyParent *p = ent->parent();
        if (p)
            cd = p->cdesc();
        else if (ent->odesc() && ent->odesc()->type() == CDINSTANCE)
            cd = (CDc*)ent->odesc();
        if (cd) {
            CDs *ms = cd->masterCell();
            if (ms) {
                if (EV()->Cursor().get_press_alt())
                    EV()->Cursor().get_alt_down(&pe_press_x, &pe_press_y);
                else
                    EV()->Cursor().get_raw(&pe_press_x, &pe_press_y);
                if (ms->isElectrical() && ms->elecCellType() == CDelecSubc) {

                    // The ent already has the correct owner and proxy.
                    ent->set_ref_type(HYrefDevice);
                    ent->set_odesc(cd);
                    ent->set_pos_x(pe_press_x);
                    ent->set_pos_y(pe_press_y);
                    hyParent::destroy(ent->parent());
                    ent->set_parent(0);

                    pe_pxdesc = ms;
                    pe_pxent = ent;
                    ent = 0;
                }
                else if (!ms->isElectrical()) {
                    pe_pxent = 0;
                    pe_pxdesc = ms;
                }
            }
        }
        delete [] string;
        delete ent;
        return;
    }

    if (!is_visible(ent)) {
        delete [] string;
        delete ent;
        return;
    }

    if (ent->ref_type() == HYrefCell) {
        // Returned a cell name, enter as plain text.
        if (ent->odesc() && ent->odesc()->type() == CDINSTANCE) {
            CDc *cd = (CDc*)ent->odesc();
            insert(Tstring(cd->cellname()));
        }
        delete [] string;
        delete ent;
        return;
    }
    if (ent->ref_type() == HYrefLabel) {
        // Returned a label, enter hypertext.
        if (ent->odesc() && ent->odesc()->type() == CDLABEL) {
            CDla *la = (CDla*)ent->odesc();
            insert(la->label());
        }
        delete [] string;
        delete ent;
        return;
    }

    if (ScedIf()->doingPlot()) {
        pe_down = true;
        pe_last_string = string;
        pe_last_ent = ent;
        if (EV()->Cursor().get_press_alt()) {
            EV()->Cursor().get_alt_down(&pe_press_x, &pe_press_y);
            wdesc->Snap(&pe_press_x, &pe_press_y);
        }
        else
            EV()->Cursor().get_xy(&pe_press_x, &pe_press_y);
        Gst()->SetGhost(GFeterms);
        EV()->DownTimer(GFeterms);
    }
    else
        process_b1_text(string, ent);
}


// Return the index matching ent, or -1 if not found.
//
int
cPromptEdit::find_ent(hyEnt *ent)
{
    if (ent) {
        int delta = (int)(EV()->CurrentWin()->LogScaleToPix(10)/
            EV()->CurrentWin()->Ratio());
        for (int i = pe_colmin; i < pe_buf.size(); i++) {
            if (pe_buf.element(i)->type() == HLrefEnd)
                break;
            if (pe_buf.element(i)->type() == HLrefText)
                continue;
            if (!hyEnt::hy_strcmp(ent, pe_buf.element(i)->hyent())) {
                if (abs(ent->pos_x() -
                        pe_buf.element(i)->hyent()->pos_x()) <= delta &&
                        abs(ent->pos_y() -
                        pe_buf.element(i)->hyent()->pos_y()) <= delta)
                    return (i);
            }
        }
    }
    return (-1);
}


namespace {
    bool spacechar(const char *s)
    {
        if (s && !(*s & 0x80))
            return (isspace(*s));
        return (false);
    }
}


namespace {
    // Return true if the type corresponds to one of the property types.
    //
    inline bool is_prpref(HLrefType r)
    {
        return (r & (HLrefNode | HLrefBranch | HLrefDevice));
    }
}


// Do the work responding to a button click or drag in a drawing window.
//
void
cPromptEdit::process_b1_text(char *string, hyEnt *ent)
{
    if (ScedIf()->doingPlot()) {
        if (!string) {
            string = pe_last_string;
            pe_last_string = 0;
            ent = pe_last_ent;
            pe_last_ent = 0;
        }

        if (!pe_last_string) {
            // Processing a click, delete if matches existing, else insert
            // new;

            int i = find_ent(ent);
            if (i >= 0) {
                // found it
                show_del(i);
                if (pe_column > pe_colmin &&
                        pe_buf.element(pe_column-1)->type() == HLrefText &&
                        spacechar(pe_buf.element(pe_column-1)->chr()))
                    show_del(pe_column-1);
                delete [] string;
                delete ent;
                draw_cursor(UNDRAW);
                set_col(TOEND);
                draw_cursor(DRAW);
                return;
            }
        }
        else {
            // Drag and drop.
            int i_down = find_ent(pe_last_ent);
            if (i_down < 0) {
                // Dragging nothing, bail.
                delete [] pe_last_string;
                pe_last_string = 0;
                delete pe_last_ent;
                pe_last_ent = 0;
                delete [] string;
                delete ent;
                return;
            }

            int i_up = find_ent(ent);
            if (i_up >= 0) {
                // Dpopped on existing mark, swap the references.
                rotate_plotpts(i_down, i_up);

                delete [] string;
                delete ent;
                delete [] pe_last_string;
                pe_last_string = 0;
                delete pe_last_ent;
                pe_last_ent = 0;
            }
            else {
                // Dropped on empty location, update.
                sHtxt hret;
                hret.set_ent(string, ent);
                replace(i_down, &hret);
                draw_cursor(UNDRAW);
                set_col(TOEND);
                draw_cursor(DRAW);

                delete [] pe_last_string;
                pe_last_string = 0;
                delete pe_last_ent;
                pe_last_ent = 0;
            }
            return;
        }
    }

    if (!string)
        return;

    // Separate new item from other tokens with spaces.
    sHtxt hret;
    if (pe_column && (is_prpref(pe_buf.element(pe_column-1)->type()) ||
            (pe_buf.element(pe_column-1)->type() == HLrefText &&
            !spacechar(pe_buf.element(pe_column-1)->chr())))) {
        hret.set_char(' ');
        insert(&hret);
    }

    hret.set_ent(string, ent);
    insert(&hret);

    if (is_prpref(pe_buf.element(pe_column)->type()) ||
            (pe_buf.element(pe_column)->type() == HLrefText &&
            !spacechar(pe_buf.element(pe_column)->chr()))) {
        hret.set_char(' ');
        insert(&hret);
    }
}


// Handler for mouse button presses in the prompt line area.  If
// button 1, move the cursor.  If button 2 or 3, prepare to receive
// selection.
//
void
cPromptEdit::button_press_handler(int btn, int x, int y)
{
    if (XM()->IsDoingHelp()) {
        unsigned int state = 0;
        DSPmainDraw(QueryPointer(0, 0, &state))
        if (!(state & GR_SHIFT_MASK)) {
            DSPmainWbag(PopUpHelp("promptline"))
            return;
        }
    }
    if (pe_active == hyOFF) {
        // Non-editing mode, we support scrolling of long lines and
        // text selections.
        if (btn == 1) {
            pe_has_drag = true;
            pe_dragged = false;
            pe_drag_x = x;
            pe_drag_y = y;
        }
        return;
    }
    if (pe_buf.element(pe_colmin)->type() == HLrefLongText)
        return;
    if (btn == 1) {
        int cpos = (x - 2 - pe_offset)/pe_fntwid;
        if (cpos < pe_colmin)
            cpos = pe_colmin;
        int realcol = pe_colmin;
        int i;
        for (i = pe_colmin; i < pe_buf.size(); i++) {
            if (pe_buf.element(i)->type() == HLrefEnd)
                break;
            if (pe_buf.element(i)->type() == HLrefText)
                realcol++;
            else if (pe_buf.element(i)->type() == HLrefLongText)
                realcol += strlen(HY_LT_MSG);
            else
                realcol += numchars(pe_buf.element(i)->string());
            if (realcol > cpos)
                break;
        }
        draw_cursor(UNDRAW);
        set_col(i);
        draw_cursor(DRAW);
        pe_firstinsert = false;
    }
    else if (btn == 2) {
        // get primary
        get_selection(false);
    }
    else if (btn == 3) {
        // get clipboard
        get_selection(true);
    }
}


void
cPromptEdit::button_release_handler(int btn, int x, int)
{
    if (pe_active != hyOFF)
        return;
    if (btn != 1)
        return;

    pe_has_drag = false;
    if (!pe_dragged) {
        // Not dragging, no selection, so scroll the prompt line. 
        // This can only happen when the text is longer than the
        // display area.  Click near the left of the prompt line to
        // show the start of the string.  Click near the rignt to show
        // the end of the string.

        deselect();

        const char *str = PL()->GetLastPrompt();
        if (!str)
            return;
        int cwd = win_width(true);
        int len = strlen(str);
        if (len <= cwd)
            return;

        int wd = win_width();
        double r = (x-8)/(double)(wd-16);
        if (r < 0.0)
            r = 0.0;
        else if (r > 1.0)
            r = 1.0;
        int ofs = (int)(r*(len - cwd));

        int i;
        const char *t;
        for (i = 0, t = str + ofs; *t; i++, t++) {
            char c = *t;
            if (c < ' ')
                c = ' ';
            pe_buf.set_char(c, i);
        }
        pe_buf.set_ent(0, 0, i);
        set_col_min(-1);  // Force text in prompt edit color.
        draw_text(DRAW, TOEND, true);
        set_col_min(0);
        DSPmainDraw(Update())
    }
    pe_dragged = false;
}


// Handle pointer motion, called only when button 1 is pressed and held.
//
void
cPromptEdit::pointer_motion_handler(int x, int)
{
    if (pe_active != hyOFF)
        return;
    if (pe_has_drag) {
        select(pe_drag_x, x);
        pe_dragged = true;
    }
}


// Handler for 'L' (long text) button presses.
//
void
cPromptEdit::lt_btn_press_handler()
{
    if (XM()->IsDoingHelp()) {
        unsigned int state = 0;
        DSPmainDraw(QueryPointer(0, 0, &state))
        if (!(state & GR_SHIFT_MASK)) {
            DSPmainWbag(PopUpHelp("longtext"))
            return;
        }
    }
    sHtxt hret;
    hyList *hl = get_hyList();
    hret.set_lt(hyList::string(hl, HYcvPlain, true));
    hyList::destroy(hl);
    clear_cols_to_end(pe_colmin);
    insert(&hret);
    finish(false);
}


void
cPromptEdit::select(int x1, int x2)
{
    if (pe_active != hyOFF)
        return;
    if (x2 < x1) {
        int t = x2;
        x2 = x1;
        x1 = t;
    }
    if (!pe_fntwid)
        return;

    int start = (x1 - pe_xpos)/pe_fntwid;
    if (start >= pe_buf.endcol())
        return;
    pe_sel_start = start;
    pe_sel_end = (x2 - pe_xpos)/pe_fntwid + 1;
    set_col_min(-1); // Force text in prompt edit color.
    draw_text(DISPLAY, TOEND, true);
    set_col_min(0);
    DSPmainDraw(Update())
    init_selection(true);
}


void
cPromptEdit::deselect(bool no_init)
{
    if (pe_active != hyOFF)
        return;
    bool was_sel = (pe_sel_end > pe_sel_start);
    pe_sel_start = 0;
    pe_sel_end = 0;
    if (was_sel) {
        set_col_min(-1); // Force text in prompt edit color.
        draw_text(DISPLAY, TOEND, true);
        set_col_min(0);
        DSPmainDraw(Update())

        // For GTK, we have to skip the init_selection callback when
        // deselect is called from the selection-clear handler. 
        // Otherwise, we will invalidate the new selection in another
        // window that triggered the selection clear event.

        if (!no_init)
            init_selection(false);
    }
}


char *
cPromptEdit::get_sel()
{
    if (pe_active == hyOFF && pe_sel_end > pe_sel_start) {
        sLstr lstr;
        for (int i = 0; i < pe_buf.size(); i++) {
            if (pe_buf.element(i)->type() != HLrefText)
                break;
            if (i >= pe_sel_start && i < pe_sel_end)
                lstr.add(pe_buf.element(i)->chr());
        }
        return (lstr.string_trim());
    }
    return (0);
}
// End of cPromptEdit functions


//-----------------------------------------------------------------------------
// Prompt text buffer

void
sPromptBuffer::insert(sHtxt *h, int col)
{
    if (col < 0)
        return;
    int end = endcol();
    if (end < col)
        return;
    if (!h) {
        clear_to_end(col);
        return;
    }
    check_size(end+1);
    for (int i = end; i >= col; i--)
        pb_hbuf[i+1] = pb_hbuf[i];
    pb_hbuf[col] = *h;
}


void
sPromptBuffer::replace(sHtxt *h, int col)
{
    if (col < 0 || col >= pb_size)
        return;
    if (!h) {
        clear_to_end(col);
        return;
    }
    pb_hbuf[col].clear_free();
    if (h)
        pb_hbuf[col] = *h;
}


void
sPromptBuffer::swap(int from, int to)
{
    if (from < 0 || from >= pb_size)
        return;
    if (to < 0 || to >= pb_size)
        return;
    sHtxt ht = pb_hbuf[from];
    pb_hbuf[from] = pb_hbuf[to];
    pb_hbuf[to] = ht;
}


void
sPromptBuffer::remove(int col)
{
    if (col < 0 || col >= pb_size)
        return;
    pb_hbuf[col].clear_free();
    for (int i = col; i < pb_size; i++) {
        pb_hbuf[i] = pb_hbuf[i+1];
        if (pb_hbuf[i].type() == HLrefEnd)
            break;
    }
}


int
sPromptBuffer::endcol()
{
    for (int i = 0; i < pb_size; i++) {
        if (pb_hbuf[i].type() == HLrefEnd)
            return (i);
    }
    return (0);
}


void
sPromptBuffer::clear_to_end(int col)
{
    if (col < 0 || col >= pb_size)
        return;
    for (int i = col; i < pb_size; i++) {
        if (pb_hbuf[i].type() == HLrefEnd)
            break;
        pb_hbuf[i].clear_free();
    }
}


char *
sPromptBuffer::set_plain_text(const char *s)
{
    if (!s)
        *pb_tbuf = 0;
    else {
        int len = strlen(s) + 1;
        if (len > pb_tsize) {
            pb_tsize = len;
            delete [] pb_tbuf;
            pb_tbuf = new char[pb_tsize];
        }
        strcpy(pb_tbuf, s);
    }
    return (pb_tbuf);
}


char *
sPromptBuffer::get_plain_text(int colmin)
{
    sLstr lstr;
    for (int i = colmin; i < pb_size; i++) {
        if (pb_hbuf[i].type() == HLrefEnd)
            break;
        if (pb_hbuf[i].type() == HLrefText)
            lstr.add(pb_hbuf[i].chr());
        else {
            const char *tt = pb_hbuf[i].string();
            if (tt)
                lstr.add(tt);
        }
    }
    return (set_plain_text(lstr.string()));
}


hyList *
sPromptBuffer::get_hyList(int colmin, bool pre)
{
    hyList *h = 0, *h0 = 0;
    sLstr lstr;
    int k = 0;
    for (int i = (pre ? 0 : colmin); i < (pre ? colmin+1 : pb_size); i++) {

        if (pb_hbuf[i].type() == HLrefEnd || i == (pre ? colmin : pb_size-1)) {
            if (k) {
                if (!h0)
                    h0 = h = new hyList;
                else {
                    h->set_next(new hyList);
                    h = h->next();
                }
                h->set_ref_type(HLrefText);
                h->set_text(lstr.string_trim());
                lstr.free();
            }
            if (i == (pre ? 0 : colmin)) {
                // No text, return something to distinguish
                // from ESC.
                h0 = new hyList;
                h0->set_ref_type(HLrefText);
                h0->set_text(lstring::copy(""));
            }
            break;
        }
        if (pb_hbuf[i].type() == HLrefText) {
            lstr.add(pb_hbuf[i].chr());
            k++;
        }
        else {
            if (k) {
                k = 0;
                if (!h0)
                    h0 = h = new hyList;
                else {
                    h->set_next(new hyList);
                    h = h->next();
                }
                h->set_ref_type(HLrefText);
                h->set_text(lstr.string_trim());
                lstr.free();
            }
            if (!h0)
                h0 = h = new hyList;
            else {
                h->set_next(new hyList);
                h = h->next();
            }
            h->set_ref_type(pb_hbuf[i].type());
            if (pb_hbuf[i].type() == HLrefLongText)
                h->set_text(lstring::copy(pb_hbuf[i].string()));
            else {
                hyEnt *ent = pb_hbuf[i].hyent()->dup();
                ent->add();
                h->set_hent(ent); 
            }
        }
    }
    return (h0);
}
// End of sPromptBuffer functions.


//-----------------------------------------------------------------------------
// Prompt line message handling

// Append the string to the Stuff-Buf.
//
void
sPromptContext::stuff_string(const char *string)
{
    if (string) {
        if (!pc_stuff_buf)
            pc_stuff_buf = new stringlist(lstring::copy(string), 0);
        else {
            stringlist *w = pc_stuff_buf;
            while (w->next)
                w = w->next;
            w->next = new stringlist(lstring::copy(string), 0);
        }
    }
}


// Pop-off and return the head string of the Stuff-Buf.
//
char *
sPromptContext::pop_stuff()
{
    if (pc_stuff_buf) {
        stringlist *s = pc_stuff_buf;
        pc_stuff_buf = pc_stuff_buf->next;
        char *t = s->string;
        delete s;
        return (t);
    }
    return (0);
}


// Redirect a copy of the message.  The fp1 is either stderr or
// stdout, or a disk file pointer, if not 0.  fp2 is a pointer to a
// log file if not 0.
//
void
sPromptContext::redirect(const char *buf)
{
    if (pc_tee_fp1)
        fprintf(pc_tee_fp1, "%s\n", buf);
    if (pc_tee_fp2)
        fprintf(pc_tee_fp2, "%s\n", buf);
}


// Open fp1 by name (close it first if necessary).
//
void
sPromptContext::open_fp1(const char *name)
{
    char *t = lstring::getqtok(&name);
    if (!t) {
        if (pc_tee_fp1 && pc_tee_fp1 != stdout && pc_tee_fp1 != stderr)
            fclose(pc_tee_fp1);
        pc_tee_fp1 = 0;
        return;
    }
    if (!strcmp(t, "stdout"))
        pc_tee_fp1 = stdout;
    else if (!strcmp(t, "stderr"))
        pc_tee_fp1 = stderr;
    else {
        pc_tee_fp1 = filestat::open_file(t, "w");
        if (!pc_tee_fp1)
            Log()->ErrorLog(mh::Initialization, filestat::error_msg());
    }
    delete [] t;
}


// Set the fp2 pointer.
//
void
sPromptContext::set_fp2(FILE *fp)
{
    pc_tee_fp2 = fp;
}


// Cat out the prefix strings to buf.
//
char *
sPromptContext::cat_prompts(char *buf)
{
    char *s = buf;
    for (stringlist *sl = pc_prompt_stack; sl; sl = sl->next) {
        strcpy(s, sl->string);
        s += strlen(sl->string);
    }
    return (s);
}


// Push the old prefix list and null the pointer.  Add the current
// prompt line to the saved prompt list.
//
void
sPromptContext::save_prompt()
{
    pc_prompt_bak = new stringlist_list(pc_prompt_stack, pc_prompt_bak);
    pc_prompt_stack = 0;
    char *s = PL()->GetPrompt();
    if (s)
        pc_saved_prompt = new stringlist(lstring::copy(s), pc_saved_prompt);
}


// Delete the current prefix list, display the saved prompt and pop
// the list, and restore the previous prefix list.
//
void
sPromptContext::restore_prompt()
{
    stringlist::destroy(pc_prompt_stack);
    pc_prompt_stack = 0;
    if (pc_saved_prompt) {
        stringlist *sl = pc_saved_prompt;
        pc_saved_prompt = pc_saved_prompt->next;
        PL()->ShowPrompt(sl->string);
        delete [] sl->string;
        delete sl;
    }
    if (pc_prompt_bak) {
        pc_prompt_stack = pc_prompt_bak->list;
        stringlist_list *sll = pc_prompt_bak;
        pc_prompt_bak = pc_prompt_bak->next;
        delete sll;
    }
}


// Push buf to the end of the prefix list.
//
void
sPromptContext::push_prompt(const char *buf)
{
    if (!pc_prompt_stack)
        pc_prompt_stack = new stringlist(lstring::copy(buf), 0);
    else {
        stringlist *sl = pc_prompt_stack;
        while (sl->next)
            sl = sl->next;
        sl->next = new stringlist(lstring::copy(buf), 0);
    }
}


// Delete the last entry in the prefix list.
//
void
sPromptContext::pop_prompt()
{
    if (pc_prompt_stack) {
        if (!pc_prompt_stack->next) {
            delete [] pc_prompt_stack->string;
            delete pc_prompt_stack;
            pc_prompt_stack = 0;
        }
        else {
            stringlist *sl = pc_prompt_stack;
            while (sl->next->next)
                sl = sl->next;
            delete [] sl->next->string;
            delete [] sl->next;
            sl->next = 0;
        }
    }
}


// Save the string in the LastPrompt buffer.
//
void
sPromptContext::save_last(const char *string)
{
    if (!string)
        return;
    int len = strlen(string);
    if (len <= pc_last_prompt_len) {
        strcpy(pc_last_prompt, string);
        return;
    }
    delete [] pc_last_prompt;
    pc_last_prompt = lstring::copy(string);
    pc_last_prompt_len = len;
}


// Return the LastPrompt buffer.
//
const char *
sPromptContext::get_last()
{
    return (pc_last_prompt);
}
// End of sPromptContext functions


// Set the UTF8 encoding of the number.
//
// U+0000    U+007F     0xxxxxxx
// U+0080    U+07FF     110xxxxx 10xxxxxx
// U+0800    U+FFFF     1110xxxx 10xxxxxx 10xxxxxx
// U+10000   U+1FFFFF   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
// U+200000  U+3FFFFFF  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
// U+4000000 U+7FFFFFFF 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
//
const char *
sUni::utf8_encode()
{
    if (u_nchars == 0)
        return (0);
    u_buf[u_nchars] = 0;

    int val;
    if (sscanf(u_buf, "%x", &val) != 1)
        return (0);

    char *t = u_buf;
    if (val <= 0x7f) {
        *t++ = val;
        *t = 0;
    }
    else if (val <= 0x7ff) {
        *t++ = 0xc0 | ((val >> 6) & 0x1f);
        *t++ = 0x80 | (val & 0x3f);
        *t++ = 0;
    }
    else if (val <= 0xffff) {
        *t++ = 0xe0 | ((val >> 12) & 0xf);
        *t++ = 0x80 | ((val >> 6) & 0x3f);
        *t++ = 0x80 | (val & 0x3f);
        *t++ = 0;
    }
    else if (val <= 0x1fffff) {
        *t++ = 0xf0 | ((val >> 18) & 0x7);
        *t++ = 0x80 | ((val >> 12) & 0x3f);
        *t++ = 0x80 | ((val >> 6) & 0x3f);
        *t++ = 0x80 | (val & 0x3f);
        *t++ = 0;
    }
    else if (val <= 0x3ffffff) {
        *t++ = 0xf8 | ((val >> 24) & 0x3);
        *t++ = 0x80 | ((val >> 18) & 0x3f);
        *t++ = 0x80 | ((val >> 12) & 0x3f);
        *t++ = 0x80 | ((val >> 6) & 0x3f);
        *t++ = 0x80 | (val & 0x3f);
        *t++ = 0;
    }
    else if (val <= 0x7fffffff) {
        *t++ = 0xfc | ((val >> 30) & 0x1);
        *t++ = 0x80 | ((val >> 24) & 0x3f);
        *t++ = 0x80 | ((val >> 18) & 0x3f);
        *t++ = 0x80 | ((val >> 12) & 0x3f);
        *t++ = 0x80 | ((val >> 6) & 0x3f);
        *t++ = 0x80 | (val & 0x3f);
        *t++ = 0;
    }
    return (u_buf);
}


/********
// Here's a function to go from UTF-8 back to \u escapes, may be useful
// at some point.

namespace {
    // Return the string rep for the hex bytes of the UTF-8 character.
    //
    char *utf8_to_bytes(const char *s)
    {
        if (!s)
            return (0);
        sLstr lstr;
        while (*s) {
            unsigned int val;
            if ((*s & 0x80) && (*s & 0x40)) {
                if (!(*s & 0x20)) {
                    val = (s[0] & 0x1f) << 6;
                    val |= (s[1] & 0x3f);
                    s += 2;
                }
                else if (!(*s & 0x10)) {
                    val = (s[0] & 0x0f) << 12;
                    val |= (s[1] & 0x3f) << 6;
                    val |= (s[2] & 0x3f);
                    s += 3;
                }
                else if (!(*s & 0x08)) {
                    val = (s[0] & 0x07) << 18;
                    val |= (s[1] & 0x3f) << 12;
                    val |= (s[2] & 0x3f) << 6;
                    val |= (s[3] & 0x3f);
                    s += 4;
                }
                else if (!(*s & 0x04)) {
                    val = (s[0] & 0x03) << 24;
                    val |= (s[1] & 0x3f) << 18;
                    val |= (s[2] & 0x3f) << 12;
                    val |= (s[3] & 0x3f) << 6;
                    val |= (s[4] & 0x3f);
                    s += 5;
                }
                else if (!(*s & 0x02)) {
                    val = (s[0] & 0x01) << 30;
                    val |= (s[1] & 0x3f) << 24;
                    val |= (s[2] & 0x3f) << 18;
                    val |= (s[3] & 0x3f) << 12;
                    val |= (s[4] & 0x3f) << 6;
                    val |= (s[5] & 0x3f);
                    s += 6;
                }
                char buf[32];
                lstr.add_c('\\');
                if (val <= 0xffff) {
                    lstr.add_c('u');
                    sprintf(buf, "%04x", val);
                    lstr.add(buf);
                }
                else {
                    lstr.add_c('U');
                    sprintf(buf, "%08x", val);
                    lstr.add(buf);
                }
            }
            else {
                lstr.add_c(*s);
                s++;
            }
        }
        return (lstr.string_trim());
    }
}
********/

