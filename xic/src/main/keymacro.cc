
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
 $Id: keymacro.cc,v 5.4 2017/04/16 20:28:12 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "dsp_tkif.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "events.h"
#include "keymap.h"
#include "keymacro.h"
#include "promptline.h"
#include "tech.h"
#include "pathlist.h"
#include "filestat.h"


/*========================================================================
  Keyboard Macros
 ========================================================================*/

cKbMacro *cKbMacro::instancePtr = 0;

// Instantiate the class.
namespace { cKbMacro _km; }


cKbMacro::cKbMacro()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cKbMacro already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    km_key_list = 0;
    km_last_btn = 0;
    km_last_menu = 0;
    km_mqueue = 0;
    km_mex = 0;
    km_context = 0;
    km_context_pop = false;
}


// Private static error exit.
//
void
cKbMacro::on_null_ptr()
{
    fprintf(stderr, "Singleton class cKbMacro used before instantiated.\n");
    exit(1);
}


// Begin recording events for a macro definition.
//
void
cKbMacro::GetMacro()
{
    sKeyMap *nkey = getKeyToMap();
    if (!nkey)
        return;
    char string[64];
    SprintKey(string, nkey->key, nkey->state, nkey->text);
    nkey->begin_recording(string);
}


// Save or delete a recorded macro.
//
void
cKbMacro::SaveMacro(sKeyMap *nkey, bool success)
{
    if (!success) {
        delete nkey;
        PL()->ShowPrompt("Macro not recorded.");
    }
    else {
        if (nkey->response) {
            nkey->next = km_key_list;
            km_key_list = nkey;
        }
        MacroFileUpdate(XM()->MacroFileName());
        if (nkey->response)
            PL()->ShowPrompt("Macro recorded.");
        else
            PL()->ShowPrompt("Macro deleted.");
    }
}


// Return the mapping if the given key event is already mapped.
// Remove the structure from the list.
//
sKeyMap *
cKbMacro::already_mapped(unsigned key, unsigned state)
{
    sKeyMap *kp = 0;
    for (sKeyMap *kl = km_key_list; kl; kl = kl->next) {
        if (kl->key == key && kl->state == state) {
            if (kp)
                kp->next = kl->next;
            else
                km_key_list = kl->next;
            return (kl);
        }
        kp = kl;
    }
    return (0);
}


namespace {
    char *
    name_tok(char*s)
    {
        while (isspace(*s) || *s == '\"' || *s == ',')
            s++;
        char *t = s;
        if ((t = strrchr(s, '\"')) != 0)   // strip quote
            *t = '\0';
        else if((t = strrchr(s, ')')) != 0)   // strip )
            *t = '\0';
        return (s);
    }
}


// Parse and save a macro specification.  Only one of sfp, wl, line
// should be nonzero, which represents the input stream.  The
// linecnt in incremented for each line read.
//
void
cKbMacro::MacroParse(SIfile *sfp, stringlist **wl, const char **line,
    int *linecnt)
{
    // the "#macro" line is stripped
    bool badness = false;
    sEvent *end = 0;
    sKeyMap *map = 0;
    int lcnt = 0;
    char *s;
    while ((s = SI()->NextLine(linecnt, sfp, wl, line)) != 0) {
        char *string = s;
        lcnt++;
        sEvent *ev;
        unsigned key, btn, state;
        while (isspace(*s))
            s++;
        if (!strncmp(s, "#end", 4)) {
            delete [] string;
            break;
        }
        if (badness) {
            delete [] string;
            continue;
        }
        if (!strncmp(s, "KeyDown", 7)) {
            s = strchr(s, '(');
            if (s)
                s++;
            else {
                badness = true;
                delete [] string;
                continue;
            }
            int nc = 0;
            int n = sscanf(s, "%x, %d, %n", &key, &state, &nc);
            if (n != 2) {
                badness = true;
                delete [] string;
                continue;
            }
            if (lcnt == 1)
                map = new sKeyMap(key, state, 0);
            else {
                if (!map) {
                    badness = true;
                    delete [] string;
                    continue;
                }
                s = name_tok(s + nc);
                ev = new sKeyEvent(lstring::copy(s), state, KEY_PRESS, key);
                ((sKeyEvent*)ev)->set_text();
                if (!map->response)
                    map->response = end = ev;
                else {
                    end->next = ev;
                    end = end->next;
                }
            }
        }
        else if (!strncmp(s, "KeyUp", 5)) {
            s = strchr(s, '(');
            if (s)
                s++;
            else {
                badness = true;
                delete [] string;
                continue;
            }
            int nc = 0;
            int n = sscanf(s, "%x, %d, %n", &key, &state, &nc);
            if (n < 2) {
                badness = true;
                delete [] string;
                continue;
            }
            if (!map) {
                badness = true;
                delete [] string;
                continue;
            }
            s = name_tok(s + nc);
            ev = new sKeyEvent(lstring::copy(s), state, KEY_RELEASE, key);
            if (!map->response)
                map->response = end = ev;
            else {
                end->next = ev;
                end = end->next;
            }
        }
        else if (!strncmp(s, "BtnDown", 7)) {
            s = strchr(s, '(');
            if (s)
                s++;
            else {
                badness = true;
                delete [] string;
                continue;
            }
            int x, y, nc = 0;
            int n = sscanf(s, "%d, %d, %d, %d, %n", &btn, &state, &x, &y, &nc);
            if (n < 4) {
                badness = true;
                delete [] string;
                continue;
            }
            s = name_tok(s + nc);
            ev = new sBtnEvent(lstring::copy(s), state, BUTTON_PRESS, btn,
                x, y);
            if (!map->response)
                map->response = end = ev;
            else {
                end->next = ev;
                end = end->next;
            }
        }
        else if (!strncmp(s, "BtnUp", 5)) {
            s = strchr(s, '(');
            if (s)
                s++;
            else {
                badness = true;
                delete [] string;
                continue;
            }
            int x, y, nc = 0;
            int n = sscanf(s, "%d, %d, %d, %d, %n", &btn, &state, &x, &y, &nc);
            if (n < 4) {
                badness = true;
                delete [] string;
                continue;
            }
            s = name_tok(s + nc);
            ev = new sBtnEvent(lstring::copy(s), state, BUTTON_RELEASE, btn,
                x, y);
            if (!map->response)
                map->response = end = ev;
            else {
                end->next = ev;
                end = end->next;
            }
        }
        else {
            // error
            badness = true;
        }
        delete [] string;
    }
    if (map && map->response) {
        map->next = km_key_list;
        km_key_list = map;
    }
}


// Function to exec a macro, called from an idle proc.
//
bool
cKbMacro::DoMacro()
{
    if (!km_mqueue)
        return (false);
    // Do one macro per call, events are handled between calls.  Save
    // the current macro while running in the km_mex list, for
    // recursion detection.  This list will probably never have more
    // than one element.

    sMqueue *m = km_mqueue;
    km_mqueue = m->next;
    m->next = km_mex;
    km_mex = m;
    MacroPush(m->kl->response);
    km_mex = km_mex->next;
    delete m;
    return (km_mqueue != 0);
}


namespace {
    // Idle function to execute macros queued.
    //
    int
    do_macro(void*)
    {
        return (KbMac()->DoMacro());
    }
}


bool
cKbMacro::MacroExpand(unsigned keysym, unsigned state, bool up)
{
    if (!up && Kmap()->DoMap(keysym))
        return (true);

    // No macro expansion while text prompting.
    if (PL()->IsEditing())
        return (false);

    if (!up) {
        state &= (GR_SHIFT_MASK | GR_CONTROL_MASK | GR_ALT_MASK);
        for (sKeyMap *kl = km_key_list; kl; kl = kl->next) {
            if (kl->key == keysym && kl->state == state) {

                // Bail if recursion detected.  This doesn't work very
                // well, or possibly at all in some cases.  Need to
                // implement something better.
                for (sMqueue *m = km_mex; m; m = m->next) {
                    if (m->kl == kl)
                        return (false);
                }

                sMqueue *m0 = new sMqueue(keysym, state, kl);
                if (!km_mqueue) {
                    km_mqueue = m0;
                    dspPkgIf()->RegisterIdleProc(do_macro, 0);
                }
                else {
                    sMqueue *m = km_mqueue;
                    while (m->next)
                        m = m->next;
                    m->next = m0;
                }
                return (true);
            }
        }
    }
    return (false);
}


// Start to execute a macro (ev != 0) or push the macro context (ev == 0)
// return true if a macro is or was in progress.  See gtkhtext.cc for
// usage example.
//
bool
cKbMacro::MacroPush(sEvent *ev)
{
    if (km_context) {
        if (ev) {
            // deal with nested macros (not allowed yet, clear)
            for (sContext *cx = km_context; cx; cx = km_context) {
                km_context = cx->next;
                delete cx;
            }
            km_context = new sContext(ev, 0);
        }
        else
            km_context = new sContext(km_context->event, km_context);
    }
    else {
        if (ev)
            km_context = new sContext(ev, 0);
        else
            return (false);
    }
    while (km_context && km_context->event) {
        sEvent *ex = km_context->event;
        km_context->event = km_context->event->next;
        if (km_context->event == 0) {
            for (sContext *cx = km_context; cx; cx = km_context) {
                km_context = cx->next;
                delete cx;
            }
        }

        ex->exec();
        if (km_context_pop) {
            // MacroPop() called in exec
            km_context_pop = false;
            if (km_context) {
                if (km_context->next)
                    km_context->next->event = km_context->event;
                sContext *tmp = km_context;
                km_context = km_context->next;
                delete tmp;
                return (true);
            }
        }
    }
    return (false);
}


bool
cKbMacro::MacroPop()
{
    if (km_context) {
        km_context_pop = true;
        return (true);
    }
    return (false);
}


// Dump the macro list to filename, creating backup file first.
//
void
cKbMacro::MacroFileUpdate(const char *filename)
{
    if (!filename || !*filename) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "MacroFileUpdate:  null or empty file name.");
        return;
    }
    FILE *fp = 0;
    if (Tech()->TechExtension() && *Tech()->TechExtension()) {
        char *tbf = new char[strlen(filename) +
            strlen(Tech()->TechExtension()) + 2];
        sprintf(tbf, "%s.%s", filename, Tech()->TechExtension());
        if (!access(tbf, F_OK)) {
            if (filestat::create_bak(tbf))
                fp = fopen(tbf, "w");
            else
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        }
        if (!fp) {
            const char *home = XM()->HomeDir();
            if (home && strlen(home)) {
                char *pth = pathlist::mk_path(home, tbf);
                if (!access(pth, F_OK)) {
                    if (filestat::create_bak(pth))
                        fp = fopen(pth, "w");
                    else
                        GRpkgIf()->ErrPrintf(ET_ERROR, "%s",
                            filestat::error_msg());
                }
                delete [] pth;
            }
        }
        delete [] tbf;
    }
    if (!fp) {
        if (!access(filename, F_OK)) {
            if (filestat::create_bak(filename))
                fp = fopen(filename, "w");
            else
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        }
        if (!fp) {
            const char *home = XM()->HomeDir();
            if (home && strlen(home)) {
                char *pth = pathlist::mk_path(home, filename);
                if (!access(pth, F_OK)) {
                    if (filestat::create_bak(pth))
                        fp = fopen(pth, "w");
                    else
                        GRpkgIf()->ErrPrintf(ET_ERROR, "%s",
                            filestat::error_msg());
                }
                delete [] pth;
            }
        }
    }
    if (!fp)
        fp = fopen(filename, "w");
    if (!fp)
        return;
    fprintf(fp, "# %s\n", XM()->IdString());
    fprintf(fp, "# keyboard macros\n");
    for (sKeyMap *k = km_key_list; k; k = k->next)
        k->print_macro(fp);
    fclose(fp);
}


// Print a representation of the key event.
//
void
cKbMacro::SprintKey(char *buf, unsigned key, unsigned state, const char *text)
{
    if (isModifier(key))
        return;
    if (state & GR_ALT_MASK) {
        *buf++ = 'A';
        *buf++ = 'l';
        *buf++ = 't';
        *buf++ = '-';
    }
    if (state & GR_CONTROL_MASK) {
        *buf++ = 'C';
        *buf++ = 't';
        *buf++ = 'r';
        *buf++ = 'l';
        *buf++ = '-';
    }
    if (text && isprint(*text) && !isspace(*text)) {
        if (ispunct(*text)) {
            *buf++ = *text;
            if (*text == '%')  // PL()->ShowPrompt() uses sprintf()
                *buf++ = *text;
        }
        else if (isupper(*text) && (state & (GR_CONTROL_MASK | GR_ALT_MASK))) {
            *buf++ = 'S';
            *buf++ = 'h';
            *buf++ = 'i';
            *buf++ = 'f';
            *buf++ = 't';
            *buf++ = '-';
            *buf++ = tolower(*text);
        }
        else
            *buf++ = *text;
        *buf++ = 0;
    }
    else {
        char tbuf[32];
        keyName(key, tbuf);
        if (!*(tbuf+1)) {
            if (isalpha(*tbuf)) {
                if (state & GR_SHIFT_MASK) {
                    if (islower(*tbuf))
                        *tbuf = toupper(*tbuf);
                }
                else {
                    if (isupper(*tbuf))
                        *tbuf = tolower(*tbuf);
                }
                sprintf(buf, "%s", tbuf);
            }
            else {
                if (state & GR_SHIFT_MASK) {
                    *buf++ = 'S';
                    *buf++ = 'h';
                    *buf++ = 'i';
                    *buf++ = 'f';
                    *buf++ = 't';
                    *buf++ = '-';
                }
                sprintf(buf, "%s", tbuf);
            }
        }
        else {
            if (state & GR_SHIFT_MASK) {
                *buf++ = 'S';
                *buf++ = 'h';
                *buf++ = 'i';
                *buf++ = 'f';
                *buf++ = 't';
                *buf++ = '-';
            }
            sprintf(buf, "<%s>", tbuf);
        }
    }
}


// Print a representation of the button event.
//
void
cKbMacro::SprintBtn(char *buf, int btn, unsigned state)
{
    char tbuf[32];
    char *s = tbuf;
    if (state & GR_ALT_MASK)
        *s++ = 'A';
    if (state & GR_SHIFT_MASK)
        *s++ = 'S';
    if (state & GR_CONTROL_MASK)
        *s++ = '^';
    strcpy(s, "btn");
    sprintf(buf, "<%s%d>", tbuf, btn);
}
// End of cKbMacro functions.


sKeyEvent::sKeyEvent(char *wn, unsigned s, unsigned t, unsigned k)
{
    widget_name = wn;
    state = s & (GR_SHIFT_MASK | GR_CONTROL_MASK | GR_ALT_MASK);
    type = t;
    key = k;
    text[0] = 0;
    text[1] = 0;
    text[2] = 0;
    text[3] = 0;
}


void
sKeyEvent::print(FILE *fp)
{
    fprintf(fp, "%s(%x, %d, \"%s\")\n",
        type == KEY_PRESS ? "KeyDown" : "KeyUp", (int)key,
        state, widget_name);
}


void
sKeyEvent::print(char *buf)
{
    char *s = buf;
    if (type == KEY_RELEASE)
        strcpy(s, "up ");
    else
        strcpy(s, "down ");
    s += strlen(s);
    if (KbMac()->isModifier(key)) {
        if (KbMac()->isControl(key)) {
            strcpy(s, "ctrl ");
            s += 5;
        }
        if (KbMac()->isShift(key)) {
            strcpy(s, "shift ");
            s += 6;
        }
        if (KbMac()->isAlt(key)) {
            strcpy(s, "alt ");
            s += 4;
        }
    }
    else
        KbMac()->SprintKey(s, key, state, text);
    sprintf(buf + strlen(buf), " %x", state);
}


// Set the text field from the keysym.
//
void
sKeyEvent::set_text()
{
    char *s = KbMac()->keyText(key, state);
    text[0] = s ? s[0] : 0;
    text[1] = 0;
}


bool
sKeyEvent::exec()
{
    return (KbMac()->execKey(this));
}
// End of sKeyEvent functions.


sBtnEvent::sBtnEvent(char *wn, unsigned s, unsigned t, unsigned b,
    int xx, int yy)
{
    widget_name = wn;
    state = s & (GR_SHIFT_MASK | GR_CONTROL_MASK | GR_ALT_MASK);
    type = t;
    button = b;
    x = xx;
    y = yy;
}


void
sBtnEvent::print(FILE *fp)
{
    fprintf(fp, "%s(%d, %d, %d, %d, \"%s\")\n",
        type == BUTTON_PRESS ? "BtnDown" : "BtnUp", button,
        state, x, y, widget_name);
}


void
sBtnEvent::print(char *buf)
{
    KbMac()->SprintBtn(buf, button, state);
}


bool
sBtnEvent::exec()
{
    return (KbMac()->execBtn(this));
}
// End of sBtnEvent functions.


sKeyMap::sKeyMap(unsigned k, unsigned s, const char *str)
{
    state = s & (GR_SHIFT_MASK | GR_CONTROL_MASK | GR_ALT_MASK);
    key = k;
    if (str)
        strncpy(text, str, 4);
    else
        *text = 0;
    forstr = 0;
    response = end = 0;
    lastkey = 0;
    grabber = 0;
    next = 0;
};


void
sKeyMap::show()
{
    char buf[256];
    sprintf(buf, "Record actions for %s, Enter to complete: ", forstr);
    for (sEvent *r = response; r; r = r->next) {
        if (r->type == KEY_PRESS) {
            sKeyEvent *rk = (sKeyEvent*)r;
            KbMac()->SprintKey(buf + strlen(buf), rk->key, rk->state, rk->text);
        }
        else if (r->type ==  BUTTON_PRESS) {
            sBtnEvent *rb = (sBtnEvent*)r;
            KbMac()->SprintBtn(buf + strlen(buf), rb->button, rb->state);
        }
    }
    PL()->ShowPrompt(buf);
}


void
sKeyMap::print_macro(FILE *fp)
{
    print_macro_text(fp);
    fprintf(fp, "#macro\n");
    fprintf(fp, "KeyDown(%x, %d, NULL)\n", (unsigned)key, state);
    for (sEvent *ev = response; ev; ev = ev->next)
        ev->print(fp);
    fprintf(fp, "#end macro\n");
}


void
sKeyMap::print_macro_text(FILE *fp)
{
    char buf[256];
    KbMac()->SprintKey(buf, key, state, 0);
    strcat(buf, " : ");
    for (sEvent *r = response; r; r = r->next) {
        if (r->type == KEY_PRESS) {
            sKeyEvent *rk = (sKeyEvent*)r;
            KbMac()->SprintKey(buf + strlen(buf), rk->key, rk->state, rk->text);
        }
        else if (r->type ==  BUTTON_PRESS) {
            sBtnEvent *rb = (sBtnEvent*)r;
            KbMac()->SprintBtn(buf + strlen(buf), rb->button, rb->state);
        }
    }
    fprintf(fp, "# %s\n", buf);
}


void
sKeyMap::add_response(sEvent *ev)
{
    if (!response)
        response = end = ev;
    else {
        end->next = ev;
        end = end->next;
    }
}


void
sKeyMap::clear_response()
{
    sEvent *ev;
    for ( ; response; response = ev) {
        ev = response->next;
        delete response;
    }
}


// Remove the last button or key press/release.
//
void
sKeyMap::clear_last_event()
{
    sEvent *e0 = 0, *en, *e;
    // reverse the event list
    //
    for (e = response; e; e = en) {
        en = e->next;
        e->next = e0;
        e0 = e;
    }
    e = e0;
    e0 = 0;
    bool found = false;
    // Delete events up through the first button or non-modifier key
    // press.
    //
    for ( ; e; e = en) {
        en = e->next;
        switch (e->type) {
        case KEY_PRESS:
            if (KbMac()->isModifier(((sKeyEvent*)e)->key)) {
                e->next = e0;
                e0 = e;
                continue;
            } // fall thru
        case BUTTON_PRESS:
            if (found) {
                e->next = e0;
                e0 = e;
                continue;
            }
            else {
                found = true;
                delete e;
            }
            break;
        case KEY_RELEASE:
            if (KbMac()->isModifier(((sKeyEvent*)e)->key)) {
                e->next = e0;
                e0 = e;
                continue;
            } // fall thru
        case BUTTON_RELEASE:
            if (found) {
                e->next = e0;
                e0 = e;
                continue;
            }
            else
                delete e;
            break;
        }
    }
    end = response = e0;
    if (end) {
        while (end->next)
            end = end->next;
    }
}


// Remove modifier keypress/release that does not enclose a button
// event or normal key event.
//
void
sKeyMap::fix_modif()
{
    sEvent *r, *rn;
    for (r = response; r; r = rn) {
        rn = r->next;
        if (r->type == KEY_PRESS &&
                KbMac()->isModifier(((sKeyEvent*)r)->key)) {
            sEvent *rr;
            for (rr = r->next; rr; rr = rr->next) {
                if (rr->type == KEY_PRESS &&
                        KbMac()->isModifier(((sKeyEvent*)rr)->key))
                    continue;
                if (rr->type == KEY_RELEASE &&
                        KbMac()->isModifier(((sKeyEvent*)rr)->key) &&
                        ((sKeyEvent*)rr)->key != ((sKeyEvent*)r)->key)
                    continue;
                // break on release, or modifiable event
                break;
            }
            // there is nothing important enclosed, so delete the pair
            if (rr && rr->type == KEY_RELEASE &&
                    ((sKeyEvent*)rr)->key == ((sKeyEvent*)r)->key) {
                remove(r);
                remove(rr);
                rn = response; // start over
            }
        }
    }
}


// Remove e from the list.
//
void
sKeyMap::remove(sEvent *e)
{
    sEvent *rp = 0;
    for (sEvent *r = response; r; r = r->next) {
        if (r == e) {
            if (rp)
                rp->next = r->next;
            else
                response = r->next;
            delete r; // e is bogus!
            return;
        }
        rp = r;
    }
}
// End of sKeyMap functions.

