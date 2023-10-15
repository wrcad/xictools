
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
#include "keymap.h"
#include "menu.h"
#include "promptline.h"
#include "dsp_tkif.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"


/*========================================================================
  Keyboard Mapping
 ========================================================================*/

// keyboard mapping file
#define MAPFILE_BASE "xic_keymap"

namespace {
    // Define/initialize key mapping main class.
    cKsMap _km;

    namespace main_keymap {
        struct KsState : public CmdState
        {
            KsState(const char*, const char*);
            virtual ~KsState();

            void setCaller(GRobject c)  { Caller = c; }

            void esc();
            void getks();

            int inc_ptr()
                {
                    KsPtr++;
                    return (KsPtr - 1);
                }

        private:
            GRobject Caller;
            int KsPtr;
            bool Finished;
        };

        KsState *KsCmd;
    }
}

using namespace main_keymap;


// Command to get the keysyms from keys to be mapped in the
// application.
//
void
cMain::KeyMapExec(CmdDesc *cmd)
{
    if (KsCmd) {
        KsCmd->esc();
        return;
    }
    Deselector ds(cmd);
    KsCmd = new KsState("KEYMAP", "xic:keymp");
    KsCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(KsCmd)) {
        delete KsCmd;
        return;
    }
    KsCmd->getks();
    ds.clear();
}


KsState::KsState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    KsPtr = 0;
    Finished = false;
}


KsState::~KsState()
{
    KsCmd = 0;
}


void
KsState::esc()
{
    EV()->PopCallback(this);
    if (!Finished)
        PL()->ErasePrompt();
    MainMenu()->Deselect(Caller);
    delete this;
}


// Main control code for keyboard mapping function.
//
void
KsState::getks()
{
    const char *name = Kmap()->GetMapName(KsPtr);
    if (name)
        PL()->ShowPromptV("Press the key for: %s", name);
    else {
        for (unsigned int i = 0; Kmap()->GetMapName(i); i++)
            Kmap()->SetMap(Kmap()->GetMapCode(i), Kmap()->GetMapKeysym(i));

        Finished = true;
        esc();
        char *in = PL()->EditPrompt("Enter map file name to create: ",
            MAPFILE_BASE);
        in = lstring::strip_space(in);
        if (in) {
            if (Kmap()->SaveMapFile(in))
                PL()->ShowPromptV("Key map written to %s.", in);
            else
                PL()->ShowPrompt("Couldn't save map, write failed.");
        }
        else
            PL()->ShowPrompt("Aborted.");
    }
}
// End of KsState methods.


namespace {
    const char *codestr(eKeyCode);
    const char *actionstr(eKeyAction);
    const char *statestr(unsigned);
    const char *actioncodestr(int);
}


// Map system button index to application button index.
//
// The GTK system buttons basically match the application buttons. 
// Buttons 1-3 are left, center, right physical mouse buttons. 
// Buttons 4 and 5 are actually mouse wheel events.  Other button
// numbers are taken as the "no-op" button, which updates the
// coordinate display but nothing else.
//
namespace { char btn_map[] = { 6, 1, 2, 3, 4, 5, 6, 6 }; }

cKsMap *cKsMap::instancePtr = 0;

cKsMap::cKsMap()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cKsMap already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    // The graphics package supplies this.
    init();
}


// Private static error exit.
//
void
cKsMap::on_null_ptr()
{
    fprintf(stderr, "Singleton class cKsMap used before instantiated.\n");
    exit(1);
}


int
cKsMap::ButtonMap(int ix)
{
    if (ix < 0)
        ix = 0;
    else if (ix > 7)
        ix = 7;
    return (btn_map[ix]);
}


void
cKsMap::SetButtonMap(int ix, int b)
{
    if (ix < 0 || ix > 7)
        return;
    btn_map[ix] = b;
}


// Dump a file containing the key mapping and action mapping info.
//
bool
cKsMap::SaveMapFile(const char *fname)
{
    if (!filestat::create_bak(fname)) {
        DSPpkg::self()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        return (false);
    }
    FILE *fp = fopen(fname, "w");
    if (!fp)
        return (false);

    fprintf(fp, "# Xic Key/Action Mapping\n");
    for (int i = 0; kmKeyTab[i].name; i++) {
        fprintf(fp, "%-20s: %x\n", kmKeyTab[i].name,
            (int)kmKeyTab[i].keysym);
    }
    fprintf(fp, "#\n");

    fprintf(fp, "<Macro-inhibit> '%c'\n", SuppressChar());
    fprintf(fp, "#\n");

    int cnt = 0;
    for (keymap *k = KeymapDown(); k->keyval; k++, cnt++) ;
    fprintf(fp, "<Keymap> %d\n", cnt);
    for (keymap *k = KeymapDown(); k->keyval; k++)
        fprintf(fp, "  %-20s %-16s %d\n", KeyvalToString(k->keyval),
            codestr(k->code), k->subcode);
    fprintf(fp, "<End> Keymap\n");
    fprintf(fp, "#\n");
    cnt = 0;
    for (keyaction *k = ActionMapPre(); k->code; k++, cnt++) ;
    fprintf(fp, "<Actions-pre> %d\n", cnt);
    for (keyaction *k = ActionMapPre(); k->code; k++)
        fprintf(fp, "  %-12s %-16s %s\n", actioncodestr(k->code),
            statestr(k->state), actionstr(k->action));
    fprintf(fp, "<End> Actions-pre\n");
    fprintf(fp, "#\n");
    cnt = 0;
    for (keyaction *k = ActionMapPost(); k->code; k++, cnt++) ;
    fprintf(fp, "<Actions-post> %d\n", cnt);
    for (keyaction *k = ActionMapPost(); k->code; k++)
        fprintf(fp, "  %-12s %-16s %s\n", actioncodestr(k->code),
            statestr(k->state), actionstr(k->action));
    fprintf(fp, "<End> Actions-post\n");
    fprintf(fp, "#\n");
    fprintf(fp, "<Buttons> %d %d %d %d %d %d %d %d\n",
        btn_map[0], btn_map[1], btn_map[2], btn_map[3],
        btn_map[4], btn_map[5], btn_map[6], btn_map[7]);
    fclose(fp);
    return (true);
}


namespace {
    const char *codestr(eKeyCode code)
    {
        switch (code) {
        case NO_KEY:
            return ("NO_KEY");
        case RETURN_KEY:
            return ("RETURN_KEY");
        case ESCAPE_KEY:
            return ("ESCAPE_KEY");
        case TAB_KEY:
            return ("TAB_KEY");
        case UNDO_KEY:
            return ("UNDO_KEY");
        case REDO_KEY:
            return ("REDO_KEY");
        case BREAK_KEY:
            return ("BREAK_KEY");
        case DELETE_KEY:
            return ("DELETE_KEY");
        case BSP_KEY:
            return ("BSP_KEY");
        case LEFT_KEY:
            return ("LEFT_KEY");
        case UP_KEY:
            return ("UP_KEY");
        case RIGHT_KEY:
            return ("RIGHT_KEY");
        case DOWN_KEY:
            return ("DOWN_KEY");
        case SHIFTDN_KEY:
            return ("SHIFTDN_KEY");
        case SHIFTUP_KEY:
            return ("SHIFTUP_KEY");
        case CTRLDN_KEY:
            return ("CTRLDN_KEY");
        case CTRLUP_KEY:
            return ("CTRLUP_KEY");
        case HOME_KEY:
            return ("HOME_KEY");
        case NUPLUS_KEY:
            return ("NUPLUS_KEY");
        case NUMINUS_KEY:
            return ("NUMINUS_KEY");
        case PAGEDN_KEY:
            return ("PAGEDN_KEY");
        case PAGEUP_KEY:
            return ("PAGEUP_KEY");
        case FUNC_KEY:
            return ("FUNC_KEY");
        }
        return ("unknown");
    }


    const char *actionstr(eKeyAction action)
    {
        switch (action) {
        case No_action:
            return ("No_action");
        case Iconify_action:
            return ("Iconify_action");
        case Interrupt_action:
            return ("Interrupt_action");
        case Escape_action:
            return ("Escape_action");
        case Redisplay_action:
            return ("Redisplay_action");
        case Delete_action:
            return ("Delete_action");
        case Bsp_action:
            return ("Bsp_action");
        case CodeUndo_action:
            return ("CodeUndo_action");
        case CodeRedo_action:
            return ("CodeRedo_action");
        case Undo_action:
            return ("Undo_action");
        case Redo_action:
            return ("Redo_action");
        case Expand_action:
            return ("Expand_action");
        case Grid_action:
            return ("Grid_action");
        case ClearKeys_action:
            return ("ClearKeys_action");
        case DRCb_action:
            return ("DRCb_action");
        case DRCf_action:
            return ("DRCf_action");
        case DRCp_action:
            return ("DRCp_action");
        case SetNextView_action:
            return ("SetNextView_action");
        case SetPrevView_action:
            return ("SetPrevView_action");
        case FullView_action:
            return ("FullView_action");
        case DecRot_action:
            return ("DecRot_action");
        case IncRot_action:
            return ("IncRot_action");
        case FlipY_action:
            return ("FlipY_action");
        case FlipX_action:
            return ("FlipX_action");
        case PanLeft_action:
            return ("PanLeft_action");
        case PanDown_action:
            return ("PanDown_action");
        case PanRight_action:
            return ("PanRight_action");
        case PanUp_action:
            return ("PanUp_action");
        case ZoomIn_action:
            return ("ZoomIn_action");
        case ZoomOut_action:
            return ("ZoomOut_action");
        case ZoomInFine_action:
            return ("ZoomInFine_action");
        case ZoomOutFine_action:
            return ("ZoomOutFine_action");
        case Command_action:
            return ("Command_action");
        case Help_action:
            return ("Help_action");
        case Coord_action:
            return ("Coord_action");
        case SaveView_action:
            return ("SaveView_action");
        case NameView_action:
            return ("NameView_action");
        case Version_action:
            return ("Version_action");
        case PanLeftFine_action:
            return ("PanLeftFine_action");
        case PanDownFine_action:
            return ("PanDownFine_action");
        case PanRightFine_action:
            return ("PanRightFine_action");
        case PanUpFine_action:
            return ("PanUpFine_action");
        case IncExpand_action:
            return ("IncExpand_action");
        case DecExpand_action:
            return ("DecExpand_action");
        }
        return ("unknown");
    }


    const char *statestr(unsigned state)
    {
        static char buf[16];
        if (!(state & (GR_ALT_MASK | GR_CONTROL_MASK | GR_SHIFT_MASK)))
            strcpy(buf, "0");
        else {
            char *t = buf;
            if (state & GR_ALT_MASK) {
                strcpy(buf, "ALT");
                t += strlen(buf);
            }
            if (state & GR_CONTROL_MASK) {
                if (t == buf) {
                    strcpy(buf, "CTRL");
                    t += strlen(buf);
                }
                else {
                    strcpy(t, "-CTRL");
                    t += strlen(t);
                }
            }
            if (state & GR_SHIFT_MASK) {
                if (t == buf)
                    strcpy(buf, "SHIFT");
                else
                    strcpy(t, "-SHIFT");
            }
        }
        return (buf);
    }


    const char *actioncodestr(int code)
    {
        static char buf[16];
        if (code < 0x20)
            return (codestr((eKeyCode)code));
        if (code >= 0x20 && code <= 0x7e) {
            buf[0] = '\'';
            buf[1] = code;
            buf[2] = '\'';
            buf[3] = 0;
            return (buf);
        }
        snprintf(buf, sizeof(buf), "%x", code);
        return (buf);
    }
}


// This is called after each key press when defining mapping, from
// MacroExpand().
//
bool
cKsMap::DoMap(unsigned keysym)
{
    if (!KsCmd)
        return (false);
    int ret = filter_key(keysym);
    if (ret < 0) {
        KsCmd->esc();
        return (true);
    }
    if (ret == 0)
        return (true);
    int i = KsCmd->inc_ptr();
    if (ret == 2)
        SetMapKeysym(i, keysym);
    KsCmd->getks();
    return (true);
}


namespace {
    eKeyCode strcode(const char*);
    eKeyAction straction(const char*);
    unsigned int strstate(const char*);
    int stractioncode(const char*);
}

// Read the map file.  A null return indicates that the file was not
// found (or readable).  Returns "ok" on success, an error message
// otherwise.  The returned string should be freed.
//
char *
cKsMap::ReadMapFile(const char *fname)
{
    FILE *fp = fopen(fname, "r");
    if (!fp && !lstring::is_rooted(fname)) {
        const char *home = XM()->HomeDir();
        if (home && strlen(home)) {
            char *fn = pathlist::mk_path(home, fname);
            if (!access(fn, F_OK))
                fp = fopen(fn, "r");
            delete [] fn;
        }
        if (!fp)
            fp = pathlist::open_path_file(fname,
                CDvdb()->getVariable(VA_LibPath), "r", 0, true);
    }
    if (!fp)
        return (0);
    char supchar = SuppressChar();
    keymap *kmap = 0;
    keyaction *ka_pre = 0, *ka_post = 0;
    int cnt = 0;
    int linecnt = 0;
    enum { rmf_none, rmf_kmap, rmf_actns_pre, rmf_actns_post } mode = rmf_none;
    char buf[256];
    while (fgets(buf, 256, fp) != 0) {
        linecnt++;
        if (*buf == '#')
            continue;
        char *s = buf;
        char *tok = lstring::gettok(&s);
        if (!tok)
            continue;
        if (lstring::cieq(tok, "<end>")) {
            delete [] tok;
            if (mode == rmf_none) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf), "misplaced \"end\", line %d.",
                    linecnt);
                return (lstring::copy(buf));
            }
            cnt = 0;
            mode = rmf_none;
            continue;
        }
        if (lstring::cieq(tok, "<keymap>")) {
            delete [] tok;
            if (kmap) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "misplaced \"keymap\", line %d.", linecnt);
                return (lstring::copy(buf));
            }
            int sz = atoi(s);
            if (sz <= 0 || sz > 256) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "bad keymap size, line %d.", linecnt);
                return (lstring::copy(buf));
            }
            kmap = new keymap[sz+1];
            memset(kmap, 0, (sz+1)*sizeof(keymap));
            mode = rmf_kmap;
            continue;
        }
        if (lstring::cieq(tok, "<actions-pre>")) {
            delete [] tok;
            if (ka_pre) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "misplaced \"actions-pre\", line %d.", linecnt);
                return (lstring::copy(buf));
            }
            int sz = atoi(s);
            if (sz <= 0 || sz > 256) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "bad actions-pre size, line %d.", linecnt);
                return (lstring::copy(buf));
            }
            ka_pre = new keyaction[sz+1];
            memset(ka_pre, 0, (sz+1)*sizeof(keyaction));
            mode = rmf_actns_pre;
            continue;
        }
        if (lstring::cieq(tok, "<actions-post>")) {
            delete [] tok;
            if (ka_post) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "misplaced \"actions-post\", line %d.", linecnt);
                return (lstring::copy(buf));
            }
            int sz = atoi(s);
            if (sz <= 0 || sz > 256) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "bad actions-post size, line %d.", linecnt);
                return (lstring::copy(buf));
            }
            ka_post = new keyaction[sz+1];
            memset(ka_post, 0, (sz+1)*sizeof(keyaction));
            mode = rmf_actns_post;
            continue;
        }
        if (lstring::cieq(tok, "<macro-inhibit>")) {
            delete [] tok;
            tok = lstring::gettok(&s);
            if (tok && *tok == '\'' && isprint(tok[1])) {
                supchar = tok[1];
                delete [] tok;
            }
            else {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "bad macro-inhibit char, line %d.", linecnt);
                delete [] tok;
                return (lstring::copy(buf));
            }
            continue;
        }
        if (lstring::cieq(tok, "<buttons>")) {
            delete [] tok;
            int i0, i1, i2, i3, i4, i5, i6, i7;
            if (sscanf(s, "%d %d %d %d %d %d %d %d", &i0, &i1, &i2, &i3, &i4,
                    &i5, &i6, &i7) == 8) {
                btn_map[0] = i0;
                btn_map[1] = i1;
                btn_map[2] = i2;
                btn_map[3] = i3;
                btn_map[4] = i4;
                btn_map[5] = i5;
                btn_map[6] = i6;
                btn_map[7] = i7;
            }
            continue;
        }
        if (mode == rmf_kmap) {
            kmap[cnt].keyval = StringToKeyval(tok);
            if (!kmap[cnt].keyval) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "unknown keysym %s, line %d.", tok, linecnt);
                delete [] tok;
                return (lstring::copy(buf));
            }
            delete [] tok;
            tok = lstring::gettok(&s);
            if (!tok) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "syntax error, line %d.", linecnt);
                return (lstring::copy(buf));
            }
            kmap[cnt].code = strcode(tok);
            if ((int)kmap[cnt].code < 0) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "unknown keycode %s, line %d.", tok, linecnt);
                delete [] tok;
                return (lstring::copy(buf));
            }
            delete [] tok;
            kmap[cnt].subcode = atoi(s);
            cnt++;
            continue;
        }
        if (mode == rmf_actns_pre) {
            ka_pre[cnt].code = stractioncode(tok);
            if ((int)ka_pre[cnt].code < 0) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "unknown keycode %s, line %d.", tok, linecnt);
                delete [] tok;
                return (lstring::copy(buf));
            }
            delete [] tok;
            tok = lstring::gettok(&s);
            if (!tok) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "syntax error, line %d.", linecnt);
                return (lstring::copy(buf));
            }
            ka_pre[cnt].state = strstate(tok);
            delete [] tok;
            tok = lstring::gettok(&s);
            if (!tok) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "syntax error, line %d.", linecnt);
                return (lstring::copy(buf));
            }
            ka_pre[cnt].action = straction(tok);
            if ((int)ka_pre[cnt].action < 0) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "unknown action %s, line %d.", tok, linecnt);
                delete [] tok;
                return (lstring::copy(buf));
            }
            delete [] tok;
            cnt++;
            continue;
        }
        if (mode == rmf_actns_post) {
            ka_post[cnt].code = stractioncode(tok);
            if ((int)ka_post[cnt].code < 0) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "unknown keycode %s, line %d.", tok, linecnt);
                delete [] tok;
                return (lstring::copy(buf));
            }
            delete [] tok;
            tok = lstring::gettok(&s);
            if (!tok) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "syntax error, line %d.", linecnt);
                return (lstring::copy(buf));
            }
            ka_post[cnt].state = strstate(tok);
            delete [] tok;
            tok = lstring::gettok(&s);
            if (!tok) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "syntax error, line %d.", linecnt);
                return (lstring::copy(buf));
            }
            ka_post[cnt].action = straction(tok);
            if ((int)ka_post[cnt].action < 0) {
                delete [] kmap;
                delete [] ka_pre;
                delete [] ka_post;
                snprintf(buf, sizeof(buf),
                    "unknown action %s, line %d.", tok, linecnt);
                delete [] tok;
                return (lstring::copy(buf));
            }
            delete [] tok;
            cnt++;
            continue;
        }
        delete [] tok;
        s = strchr(buf, ':');
        if (!s)
            continue;
        *s++ = '\0';
        char *t = s - 2;
        while (t >= buf && isspace(*t))
            *t-- = '\0';
        while (isspace(*s))
            s++;
        for (int i = 0; kmKeyTab[i].name; i++) {
            if (!strcmp(kmKeyTab[i].name, buf)) {
                unsigned ks;
                if (sscanf(s, "%x", &ks) == 1)
                    kmKeyTab[i].keysym = ks;
            }
        }
    }
    fclose(fp);
    if (kmap)
        kmKeyMapDn = kmap;
    else {
        // old format file
        for (sKsMapElt *k = kmKeyTab; k->name; k++)
            SetMap(k->code, k->keysym);
    }
    if (ka_pre)
        kmActionsPre = ka_pre;
    if (ka_post)
        kmActionsPost = ka_post;
    kmSuppressChar = supchar;
    return (lstring::copy("ok"));
}


namespace {
    eKeyCode strcode(const char *string)
    {
        if (lstring::cieq(string, "NO_KEY"))
            return (NO_KEY);
        if (lstring::cieq(string, "RETURN_KEY"))
            return (RETURN_KEY);
        if (lstring::cieq(string, "ESCAPE_KEY"))
            return (ESCAPE_KEY);
        if (lstring::cieq(string, "TAB_KEY"))
            return (TAB_KEY);
        if (lstring::cieq(string, "UNDO_KEY"))
            return (UNDO_KEY);
        if (lstring::cieq(string, "REDO_KEY"))
            return (REDO_KEY);
        if (lstring::cieq(string, "BREAK_KEY"))
            return (BREAK_KEY);
        if (lstring::cieq(string, "DELETE_KEY"))
            return (DELETE_KEY);
        if (lstring::cieq(string, "BSP_KEY"))
            return (BSP_KEY);
        if (lstring::cieq(string, "LEFT_KEY"))
            return (LEFT_KEY);
        if (lstring::cieq(string, "UP_KEY"))
            return (UP_KEY);
        if (lstring::cieq(string, "RIGHT_KEY"))
            return (RIGHT_KEY);
        if (lstring::cieq(string, "DOWN_KEY"))
            return (DOWN_KEY);
        if (lstring::cieq(string, "SHIFTDN_KEY"))
            return (SHIFTDN_KEY);
        if (lstring::cieq(string, "SHIFTUP_KEY"))
            return (SHIFTUP_KEY);
        if (lstring::cieq(string, "CTRLDN_KEY"))
            return (CTRLDN_KEY);
        if (lstring::cieq(string, "CTRLUP_KEY"))
            return (CTRLUP_KEY);
        if (lstring::cieq(string, "HOME_KEY"))
            return (HOME_KEY);
        if (lstring::cieq(string, "NUPLUS_KEY"))
            return (NUPLUS_KEY);
        if (lstring::cieq(string, "NUMINUS_KEY"))
            return (NUMINUS_KEY);
        if (lstring::cieq(string, "PAGEDN_KEY"))
            return (PAGEDN_KEY);
        if (lstring::cieq(string, "PAGEUP_KEY"))
            return (PAGEUP_KEY);
        if (lstring::cieq(string, "FUNC_KEY"))
            return (FUNC_KEY);
        return ((eKeyCode)-1);
    }


    eKeyAction straction(const char *string)
    {
        if (lstring::cieq(string, "No_action"))
            return (No_action);
        if (lstring::cieq(string, "Iconify_action"))
            return (Iconify_action);
        if (lstring::cieq(string, "Interrupt_action"))
            return (Interrupt_action);
        if (lstring::cieq(string, "Escape_action"))
            return (Escape_action);
        if (lstring::cieq(string, "Redisplay_action"))
            return (Redisplay_action);
        if (lstring::cieq(string, "Delete_action"))
            return (Delete_action);
        if (lstring::cieq(string, "Bsp_action"))
            return (Bsp_action);
        if (lstring::cieq(string, "CodeUndo_action"))
            return (CodeUndo_action);
        if (lstring::cieq(string, "CodeRedo_action"))
            return (CodeRedo_action);
        if (lstring::cieq(string, "Undo_action"))
            return (Undo_action);
        if (lstring::cieq(string, "Redo_action"))
            return (Redo_action);
        if (lstring::cieq(string, "Expand_action"))
            return (Expand_action);
        if (lstring::cieq(string, "Grid_action"))
            return (Grid_action);
        if (lstring::cieq(string, "ClearKeys_action"))
            return (ClearKeys_action);
        if (lstring::cieq(string, "DRCb_action"))
            return (DRCb_action);
        if (lstring::cieq(string, "DRCf_action"))
            return (DRCf_action);
        if (lstring::cieq(string, "DRCp_action"))
            return (DRCp_action);
        if (lstring::cieq(string, "SetNextView_action"))
            return (SetNextView_action);
        if (lstring::cieq(string, "SetPrevView_action"))
            return (SetPrevView_action);
        if (lstring::cieq(string, "FullView_action"))
            return (FullView_action);
        if (lstring::cieq(string, "DecRot_action"))
            return (DecRot_action);
        if (lstring::cieq(string, "IncRot_action"))
            return (IncRot_action);
        if (lstring::cieq(string, "FlipY_action"))
            return (FlipY_action);
        if (lstring::cieq(string, "FlipX_action"))
            return (FlipX_action);
        if (lstring::cieq(string, "PanLeft_action"))
            return (PanLeft_action);
        if (lstring::cieq(string, "PanDown_action"))
            return (PanDown_action);
        if (lstring::cieq(string, "PanRight_action"))
            return (PanRight_action);
        if (lstring::cieq(string, "PanUp_action"))
            return (PanUp_action);
        if (lstring::cieq(string, "ZoomIn_action"))
            return (ZoomIn_action);
        if (lstring::cieq(string, "ZoomOut_action"))
            return (ZoomOut_action);
        if (lstring::cieq(string, "ZoomInFine_action"))
            return (ZoomInFine_action);
        if (lstring::cieq(string, "ZoomOutFine_action"))
            return (ZoomOutFine_action);
        if (lstring::cieq(string, "Command_action"))
            return (Command_action);
        if (lstring::cieq(string, "Help_action"))
            return (Help_action);
        if (lstring::cieq(string, "Coord_action"))
            return (Coord_action);
        if (lstring::cieq(string, "SaveView_action"))
            return (SaveView_action);
        if (lstring::cieq(string, "NameView_action"))
            return (NameView_action);
        if (lstring::cieq(string, "Version_action"))
            return (Version_action);
        if (lstring::cieq(string, "PanLeftFine_action"))
            return (PanLeftFine_action);
        if (lstring::cieq(string, "PanDownFine_action"))
            return (PanDownFine_action);
        if (lstring::cieq(string, "PanRightFine_action"))
            return (PanRightFine_action);
        if (lstring::cieq(string, "PanUpFine_action"))
            return (PanUpFine_action);
        if (lstring::cieq(string, "IncExpand_action"))
            return (IncExpand_action);
        if (lstring::cieq(string, "DecExpand_action"))
            return (DecExpand_action);
        return ((eKeyAction)-1);
    }


    unsigned int
    strstate(const char *string)
    {
        unsigned state = 0;
        while (string && *string) {
            if (lstring::ciprefix("alt", string))
                state |= GR_ALT_MASK;
            else if (lstring::ciprefix("ctrl", string))
                state |= GR_CONTROL_MASK;
            else if (lstring::ciprefix("shift", string))
                state |= GR_SHIFT_MASK;
            string = strchr(string, '-');
            if (string)
                string++;
        }
        return (state);
    }


    int
    stractioncode(const char *string)
    {
        if (*string == '\'')
            return (string[1]);
        int k = strcode(string);
        if (k >= 0)
            return (k);
        if (sscanf(string, "%x", &k) == 1)
            return (k);
        return (-1);
    }
}

