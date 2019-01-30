
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "cshell.h"
#include "commands.h"
#include "simulator.h"
#include "parser.h"
#include "spnumber/hash.h"
#include "spnumber/spnumber.h"
#include "ginterf/graphics.h"


//
// The front-end command loop.
//

// Stuff to do control structures. We keep a history (seperate from the
// cshpar history, for now at least) of commands and their event numbers, 
// with a block considered as a statement. In a goto, the first word in
// co_text is where to go, likewise for label. For conditional controls, 
// we have to call ft_getpnames and ft_evaluate each time, since the
// dvec pointers will change... Also we should do variable and backquote
// substitution each time...

enum RItype
{
    NORMAL,
    BROKEN,
    CONTINUED,
    LABEL,
    RETURN
};

struct retinfo
{
    retinfo()
        {
            ri_type = NORMAL;
            ri_num = 0;
            ri_label = 0;
        }

    ~retinfo()
        {
            delete [] ri_label;
        }

    RItype type()               { return (ri_type); }
    void set_type(RItype t)     { ri_type = t; }

    int num()                   { return (ri_num); }
    void set_num(int i)         { ri_num = i; }

    const char *label()         { return (ri_label); }
    void set_label(const char *l)
        {
            char *s = lstring::copy(l);
            delete [] ri_label;
            ri_label = s;
        }

private:
    RItype ri_type;
    int ri_num;
    const char *ri_label;
};


enum COtype
{
    CO_UNFILLED,
    CO_STATEMENT,
    CO_WHILE,
    CO_DOWHILE,
    CO_IF,
    CO_FOREACH,
    CO_BREAK,
    CO_CONTINUE,
    CO_LABEL,
    CO_GOTO,
    CO_REPEAT,
    CO_RETURN
};

struct sControl
{
    sControl()
        {
            co_type = CO_UNFILLED;
            co_cond = 0;
            co_foreachvar = 0;
            co_text = 0;
            co_parent = 0;
            co_children = 0;
            co_elseblock = 0;
            co_next = 0;
            co_prev = 0;
            co_numtimes = 0;
        }

    sControl *newblock()
        {
            co_children = new sControl;
            co_children->co_parent = this;
            return (co_children);
        }

    static void destroy(sControl *cc)
        {
            while (cc) {
                sControl *cx = cc;
                cc = cc->co_next;

                wordlist::destroy(cx->co_cond);
                wordlist::destroy(cx->co_text);
                delete [] cx->co_foreachvar;
                destroy(cx->co_children);
                destroy(cx->co_elseblock);
            }
        }
  
    static void set_block(wordlist*);

    static void eval_block(sControl*);
    static sControl *find_label(sControl*, const char*);
    void doblock(retinfo *info);
    void dodump(FILE* = 0);

    sControl *next()                { return (co_next); }
    sControl *parent()              { return (co_parent); }

    static void init_indent()       { co_indent = 0; }

private:
    COtype co_type;                 // One of CO_* ...
    wordlist *co_cond;              // If, while, dowhile.
    char *co_foreachvar;            // Foreach.
    wordlist *co_text;              // Ordinary text and foreach values.
    sControl *co_parent;            // If this is inside a block.
    sControl *co_children;          // The contents of this block.
    sControl *co_elseblock;         // For if-then-else.
    sControl *co_next;
    sControl *co_prev;
    int co_numtimes;                // Repeat, break & continue levels.

    static int co_indent;           // For printing.
};

int sControl::co_indent = 0;


// Named control blocks.
//
struct block
{
    block(const char *n)
        {
            bl_name = lstring::copy(n);
            bl_cont = 0;
        }

    ~block()
        {
            delete [] bl_name;
            sControl::destroy(bl_cont);
        }

    const char *name()              { return (bl_name); }
    sControl *control()             { return (bl_cont); }
    void set_control(sControl *c)
        {
            sControl::destroy(bl_cont);
            bl_cont = c;
        }

private:
    const char *bl_name;
    sControl *bl_cont;
};


// We have to keep the control structures in a stack, so that when we do
// a 'source', we can push a fresh set onto the top...  Actually there have
// to be two stacks -- one for the pointer to the list of control structs, 
// and one for the 'current command' pointer...

#define CS_SIZE 256

struct ControlStack
{
    char *set_alt_prompt();
    void add_block(const char*);

    void set_no_exec(bool b)            { cs_no_exec = b; }

    sHtab *block_tab()                  { return (cs_block_tab); }

    void push_stack()                   { cs_stackp++; }
    void pop_stack()                    { cs_stackp--; }
    sControl *cur_control()             { return (cs_control[cs_stackp]); }
    void set_cur_control(sControl *c)   { cs_control[cs_stackp] = c; }
    sControl *cur_cend()                { return (cs_cend[cs_stackp]); }
    void set_cur_cend(sControl *c)      { cs_cend[cs_stackp] = c; }

    void evl_check_exec()
        {
            if (cs_cend[cs_stackp] && !cs_cend[cs_stackp]->parent() &&
                    !cs_no_exec) {
                sControl::eval_block(cs_cend[cs_stackp]);
                sControl::destroy(cs_control[cs_stackp]);
                cs_control[cs_stackp] = 0;
                cs_cend[cs_stackp] = 0;
            }
        }

    void set_top_level()
        {
            cs_stackp = 0;
            if (cs_cend[cs_stackp]) {
                while (cs_cend[cs_stackp]->parent())
                    cs_cend[cs_stackp] = cs_cend[cs_stackp]->parent();
            }
        }

    void reset_control()
        {
            if (cs_cend[cs_stackp] && cs_cend[cs_stackp]->parent())
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "EOF before block terminated.\n");
            for (cs_stackp = 0; cs_stackp < CS_SIZE; cs_stackp++) {
                if (!cs_control[cs_stackp])
                    break;
                sControl::destroy(cs_control[cs_stackp]);
                cs_control[cs_stackp] = 0;
                cs_cend[cs_stackp] = 0;
            }
            cs_stackp = 0;
        }

    void push_control()
        {
            if (CP.GetFlag(CP_DEBUG))
                GRpkgIf()->ErrPrintf(ET_MSGS, "push: stackp: %d -> %d\n",
                    cs_stackp, cs_stackp + 1);
            if (cs_stackp > CS_SIZE - 2) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "stack overflow -- max depth = %d.\n", CS_SIZE);
                cs_stackp = 0;
            }
            else {
                cs_stackp++;
                sControl::destroy(cs_control[cs_stackp]);
                cs_control[cs_stackp] = 0;
                cs_cend[cs_stackp] = 0;
            }
        }

    void pop_control()
        {
            if (CP.GetFlag(CP_DEBUG))
                GRpkgIf()->ErrPrintf(ET_MSGS, "pop: stackp: %d -> %d\n",
                    cs_stackp, cs_stackp - 1);
            if (cs_stackp < 1)
                GRpkgIf()->ErrPrintf(ET_INTERR, "PopControl: stack empty.\n");
            else
                cs_stackp--;
        }

private:
    sHtab *cs_block_tab;            // Hash table for control blocks.
    bool cs_no_exec;                // Supress command execution.
    int cs_stackp;                  // Stack pointer.
    sControl *cs_control[CS_SIZE];  // Context arrays.
    sControl *cs_cend[CS_SIZE];
    char cs_prompt_buf[64];         // Buffer for alt prompt.
};

namespace { ControlStack CS; }


// Manipulation of named control codeblocks.  Usage is
// codeblock [-fpab] [names]
//  -f,d      delete named blocks
//  -p,t      print named blocks
//  -a        add listed file as a block
//  -b[e,c]   bind exec, control(default) codeblock to current ckt
//  -c        list bound blocks of current circuit
//
void
CommandTab::com_codeblock(wordlist *wl)
{
    char *fname = 0;
    bool remove = false;
    bool print = false;
    bool add = false;
    bool bind = false;
    bool bindexec = false;
    bool listblks = false;
    block *b = 0;
    if (wl) {
        wl = wordlist::copy(wl);
        wordlist *ww, *wn;
        for (ww = wl; wl; wl = wn) {
            wn = wl->wl_next;
            if (*wl->wl_word != '-')
                continue;
            if (!wl->wl_word[1]) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "syntax, missing options.\n");
                wordlist::destroy(wl);
                return;
            }
            for (char *s = wl->wl_word + 1; *s; s++) {
                if (*s == 'f' || *s == 'd')
                    remove = true;
                else if (*s == 'p' || *s == 't')
                    print = true;
                else if (*s == 'a')
                    add = true;
                else if (*s == 'b') {
                    bind = true;
                    if (*(s+1) == 'e') {
                        bindexec = true;
                        s++;
                    }
                }
                else if (*s == 'c')
                    listblks = true;
                else {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "unknown option character %c.\n", *s);
                    wordlist::destroy(wl);
                    return;
                }
            }

            if (wl->wl_prev)
                wl->wl_prev->wl_next = wl->wl_next;
            if (wl->wl_next)
                wl->wl_next->wl_prev = wl->wl_prev;
            if (ww == wl)
                ww = wl->wl_next;
            delete [] wl->wl_word;
            delete wl;
        }
        if (ww) {
            fname = lstring::copy(ww->wl_word);
            CP.Unquote(fname);
            wordlist::destroy(ww);
        }
    }

    if (fname == 0) {
        TTY.init_more();

        if (bind) {
            Sp.Bind(0, bindexec);
            if (listblks)
                Sp.ListBound();
            return;
        }

        wordlist *ww, *wn;
        ww = wn = sHtab::wl(CS.block_tab());
        while (ww) {
            TTY.printf("%s", ww->wl_word);
            if (print || remove)
                b = (block*)sHtab::get(CS.block_tab(), ww->wl_word);
            if (print) {
                TTY.send(":\n");
                for (sControl *c = b->control(); c; c = c->next())
                    c->dodump();
            }
            else
                TTY.send("\n");
            if (remove) {
                Sp.Unbind(b->name());
                delete b;
                CS.block_tab()->remove(ww->wl_word);
            }
            ww = ww->wl_next;
        }
        wordlist::destroy(wn);
        if (listblks)
            Sp.ListBound();
        return;
    }

    if (!print && !remove && !add && !bind)
        add = true;

    if (print && !add) {
        b = (block*)sHtab::get(CS.block_tab(), fname);
        if (!b) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "codeblock %s not found.\n",
                fname);
            return;
        }
        TTY.init_more();
        TTY.printf("%s:\n", b->name());
        CP.PrintBlock(fname);
    }
    if (remove) {
        Sp.Unbind(fname);
        CP.FreeBlock(fname);
    }
    if (add) {
        CS.set_no_exec(true);
        bool success = Sp.Block(fname);
        CS.set_no_exec(false);
        if (success)
            CS.add_block(fname);
        else
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s not found, no block added.\n",
                fname);
    }
    if (print && add) {
        b = (block*)sHtab::get(CS.block_tab(), fname);
        if (!b) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "codeblock %s not found.\n",
                fname);
            return;
        }
        TTY.init_more();
        TTY.printf("%s:\n", b->name());
        CP.PrintBlock(fname);
    }
    if (bind)
        Sp.Bind(fname, bindexec);
    if (listblks)
        Sp.ListBound();
    delete [] fname;
}


void
CommandTab::com_cdump(wordlist *)
{
    TTY.init_more();
    sControl::init_indent();
    for (sControl *c = CS.cur_control(); c; c = c->next())
        c->dodump();
}
// End of CommandTab functions.


// Parse and add a named block.
//
void
CshPar::AddBlock(const char *name, wordlist *wl)
{
    bool inter = cp_flags[CP_INTERACTIVE];
    cp_flags[CP_INTERACTIVE] = false;
    PushControl();
    CS.set_no_exec(true);
    for ( ; wl; wl = wl->wl_next)
        EvLoop(wl->wl_word);
    CS.set_no_exec(false);
    PopControl();
    cp_flags[CP_INTERACTIVE] = inter;
    CS.add_block(name);
}


// Execute the named codeblock, or the one just above in the stack
// if name is 0.
//
void
CshPar::ExecBlock(const char *name)
{
    CP.SetReturnVal(0.0);
    block *b = 0;
    CS.push_stack();
    if (name) {
        if (CS.block_tab())
            b = (block*)sHtab::get(CS.block_tab(), name);
        if (!b) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "named block %s not found.\n",
                name);
            CS.pop_stack();
            return;
        }
    }
    if (b) {
        sControl *x = CS.cur_control();
        CS.set_cur_control(b->control());
        sControl::eval_block(CS.cur_control());
        CS.set_cur_control(x);
    }
    else
        sControl::eval_block(CS.cur_control());
    CS.pop_stack();
}



// Print the named codeblock.
//
void
CshPar::PrintBlock(const char *name, FILE *fp)
{
    block *b = (block*)sHtab::get(CS.block_tab(), name);
    if (b) {
        for (sControl *c = b->control(); c; c = c->next())
            c->dodump(fp);
    }
}


// Return true if name corresponds to a codeblock name.
//
bool
CshPar::IsBlockDef(const char *name)
{

    if (sHtab::get(CS.block_tab(), name))
        return (true);
    return (false);
}


// Delete the named codeblock from the list.
//
void
CshPar::FreeBlock(const char *name)
{
    if (!name || *name == '\0')
        return;
    block *b = (block*)sHtab::get(CS.block_tab(), name);
    if (b) {
        delete b;
        CS.block_tab()->remove(name);
    }
}


// Prevent redirection for listed commands.
namespace {
    const char *const noredirect[] = { "stop", 0 } ;  // Only one??
}

// If there is an argument, give this to cshpar to use instead of stdin. In
// a few places, we call EvLoop again if it returns 1 and exit (or close
// a file) if it returns 0... Because of the way sources are done, we can't
// allow the control structures to get blown away every time we return --
// probably every time we type source at the keyboard and every time a
// source returns to keyboard input is ok though -- use ft_controlreset.

// Main routine for evaluating shell input.
//
int
CshPar::EvLoop(const char *string)
{
    for (;;) {
        if (cp_flags[CP_INTRPT] && string)
            return (1);
        cp_flags[CP_INTRPT] = false;
        wordlist *wlist = GetCommand(string);
        cp_flags[CP_INTRPT] = false;
        if (wlist == 0) {
            // End of file or end of user input, could also be a syntax error
            if (!string) {
                if (CS.cur_cend() && CS.cur_cend()->parent())
                    ResetControl();
                cp_event--;
                continue;
            }
            return (0);
        }
        if ((wlist->wl_word == 0) || (*wlist->wl_word == '\0')) {
            // User just typed return
            wordlist::destroy(wlist);
            if (string)
                return (1);
            cp_event--;
            continue;
        }

        // Just a check...
        for (wordlist *ww = wlist; ww; ww = ww->wl_next) {
            if (!ww->wl_word) {
                GRpkgIf()->ErrPrintf(ET_INTERR,
                    "EvLoop: null word pointer.\n");
                continue;
            }
        }

        sControl::set_block(wlist);
        wordlist::destroy(wlist);
        CS.evl_check_exec();
        if (string)
            return (1); // The return value is irrelevant
    }
}


// Create and return a control struct for wl.
//
sControl *
CshPar::MakeControl(wordlist *wl)
{
    bool inter = cp_flags[CP_INTERACTIVE];
    cp_flags[CP_INTERACTIVE] = false;
    PushControl();
    CS.set_no_exec(true);
    for ( ; wl; wl = wl->wl_next)
        EvLoop(wl->wl_word);
    CS.set_no_exec(false);
    PopControl();
    cp_flags[CP_INTERACTIVE] = inter;

    CS.push_stack();
    sControl *b = CS.cur_control();
    CS.set_cur_control(0);
    CS.set_cur_cend(0);
    CS.pop_stack();
    return (b);
}


// Print the control struct c.
//
void
CshPar::PrintControl(sControl *c, FILE *fp)
{
    for (; c; c = c->next())
        c->dodump(fp);
}


// Execute the control struct c, or the one in the stack just above
// if 0.
//
void
CshPar::ExecControl(sControl *c)
{
    CP.SetReturnVal(0.0);
    CS.push_stack();
    if (c) {
        sControl *x = CS.cur_control();
        CS.set_cur_control(c);
        sControl::eval_block(CS.cur_control());
        CS.set_cur_control(x);
    }
    else
        sControl::eval_block(CS.cur_control());
    CS.pop_stack();
}


// Free the control structure given.
//
void
CshPar::FreeControl(sControl *cntrl)
{
    sControl::destroy(cntrl);
}


// This blows away the control structures...
//
void
CshPar::ResetControl()
{
    CS.reset_control();
}


// Push or pop a new control structure set...
//
void
CshPar::PopControl()
{
    CS.pop_control();
}


void
CshPar::PushControl()
{
    CS.push_control();
}


// Return to the top level (for use in the interrupt handlers)
//
void
CshPar::TopLevel()

{
    CS.set_top_level();
}


void
CshPar::SetAltPrompt()
{
    cp_altprompt = CS.set_alt_prompt();
}


// Get a command. This does all the bookkeeping things like turning
// command completion on and off...
//
wordlist *
CshPar::GetCommand(const char *string)
{
    if (cp_flags[CP_DEBUG])
        GRpkgIf()->ErrPrintf(ET_MSGS, "calling GetCommand: (%s)\n",
            string ? string : "stdin");
    SetAltPrompt();
    cp_flags[CP_CWAIT] = true;
    wordlist *wlist = Parse(string);
    cp_flags[CP_CWAIT] = false;
    if (cp_flags[CP_DEBUG]) {
        GRpkgIf()->ErrPrintf(ET_MSGS, "GetCommand: ");
        for (wordlist *wl = wlist; wl; wl = wl->wl_next) {
            char *s = lstring::copy(wl->wl_word);
            Strip(s);
            GRpkgIf()->ErrPrintf(ET_MSGS, "%s ", s);
            delete s;
        }
        GRpkgIf()->ErrPrintf(ET_MSGS, "\n");
    }
    return (wlist);
}


// Note that we only do io redirection when we get to here - we also
// postpone some other things until now.
// NOTE! wlist is freed.
//
void
CshPar::DoCommand(wordlist *wlist)
{
    if (cp_flags[CP_DEBUG]) {
        GRpkgIf()->ErrPrintf(ET_MSGS, "DoCommand: ");
        for (wordlist *wl = wlist; wl; wl = wl->wl_next) {
            char *s = lstring::copy(wl->wl_word);
            Strip(s);
            GRpkgIf()->ErrPrintf(ET_MSGS, "%s ", s);
            delete [] s;
        }
        GRpkgIf()->ErrPrintf(ET_MSGS, "\n");
    }

    // Do periodic sorts of things.
    Sp.Periodic();

    // Do all the things that used to be done by cshpar when the line
    // was read...
    //
    VarSubst(&wlist);
    pwlist(wlist, "After variable substitution");

    BackQuote(&wlist);
    pwlist(wlist, "After backquote substitution");

    DoGlob(&wlist);
    pwlist(wlist, "After globbing");

    if (!wlist || !wlist->wl_word)
        return;

    // Now loop through all of the commands given
    wordlist *rwlist = wlist, *nextc;
    do {
        int qcnt = 0;
        for (nextc = wlist; nextc; nextc = nextc->wl_next) {
            // Don't break if ';' in double quotes.  This can happen if a
            // backquote evaluation is inside a double quoted substring.
            //
            char *q = nextc->wl_word;
            while ((q = strchr(q, '"')) != 0) {
                qcnt++;
                q++;
            }
            if (qcnt & 1)
                continue;
            if (*nextc->wl_word == cp_csep && !nextc->wl_word[1])
                break;
        }

        // Temporarily hide the rest of the command...
        if (nextc && nextc->wl_prev)
            nextc->wl_prev->wl_next = 0;
        wordlist *ee = wlist->wl_prev;
        if (ee)
            wlist->wl_prev = 0;

        if (nextc != wlist) {
            // There was text...

            // do the redirection
            TTY.ioReset();
            int i;
            for (i = 0; noredirect[i]; i++)
                if (lstring::eq(wlist->wl_word, noredirect[i]))
                    break;
            if (!noredirect[i]) {
                Redirect(&wlist);
                if (wlist == 0) {
                    TTY.ioReset();
                    return;
                }
            }

            // Get rid of all the 8th bits now...
            StripList(wlist);

            // initialize the more'd output
            if (cp_flags[CP_INTERACTIVE])
                TTY.init_more();

            // First check the named blocks
            block *b = 0;
            if (CS.block_tab())
                b = (block*)sHtab::get(CS.block_tab(), wlist->wl_word);
            if (b) {
                if (b->control()) {
                    PushArg(wlist);
                    TTY.ioPush();
                    bool tmpintr = cp_flags[CP_INTERACTIVE];
                    cp_flags[CP_INTERACTIVE] = false;
                    ExecControl(b->control());
                    cp_flags[CP_INTERACTIVE] = tmpintr;
                    TTY.ioPop();
                    PopArg();
                }
            }
            else {
                sCommand *command = Cmds.FindCommand(wlist->wl_word);
                if (!command) {
                    if (!Sp.ImplicitCommand(wlist) &&
                            !(cp_flags[CP_DOUNIXCOM] && UnixCom(wlist)))
                        GRpkgIf()->ErrPrintf(ET_MSG,
                            "%s: no such command available in %s.\n", 
                            wlist->wl_word, cp_program);
                }
                else {
                    int nargs = 0;
                    for (wordlist *wl = wlist->wl_next; wl; wl = wl->wl_next)
                        nargs++;
                    if (command->co_stringargs) {
                        char *lcom = wordlist::flatten(wlist->wl_next);
                        (*command->co_func) ((wordlist*)lcom);
                        delete [] lcom;
                    }
                    else {
                        if (nargs < command->co_minargs) {
                            if (command->co_argfn)
                                (*command->co_argfn) (wlist->wl_next, command);
                            else
                                GRpkgIf()->ErrPrintf(ET_MSG,
                                    "%s: too few args.\n", wlist->wl_word);
                        }
                        else if (nargs > command->co_maxargs)
                            GRpkgIf()->ErrPrintf(ET_MSG,
                                "%s: too many args.\n", wlist->wl_word);
                        else
                            (*command->co_func) (wlist->wl_next);
                    }
                }
            }
        }

        // Now fix the pointers and advance wlist
        wlist->wl_prev = ee;
        if (nextc) {
            if (nextc->wl_prev)
                nextc->wl_prev->wl_next = nextc;
            wlist = nextc->wl_next;
        }
    } while (nextc && wlist);

    wordlist::destroy(rwlist);
    TTY.ioReset();
}


// Reset the shell after an interrupt in interactive mode.
//
void
CshPar::Reset(bool intr)
{
    cp_flags[CP_INTERACTIVE] = intr;
    cp_flags[CP_INTRPT] = false;
    cp_bqflag = false;
    cp_input = 0;
    ClearArg();
    TTY.ioTop();
}
// End of CshPar functions.


char *
ControlStack::set_alt_prompt()
{
    if (cs_cend[cs_stackp]) {
        int i = 0;
        for (sControl *c = cs_cend[cs_stackp]->parent(); c; c = c->parent())
            i++;
        if (i) {
            int j;
            for (j = 0; j < i; j++)
                cs_prompt_buf[j] = '>';
            cs_prompt_buf[j] = ' ';
            cs_prompt_buf[j + 1] = '\0';
            return (cs_prompt_buf);
        }
    }
    return (0);
}


// Add the new block to the list of named blocks.
//
void
ControlStack::add_block(const char *name)
{
    if (!name || !*name)
        return;
    if (!cs_control[cs_stackp+1]) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "block is empty, not added.\n");
        return;
    }

    if (cs_block_tab == 0)
        cs_block_tab = new sHtab(sHtab::get_ciflag(CSE_CBLK));
    block *b = (block*)sHtab::get(cs_block_tab, name);
    if (!b) {
        b = new block(name);
        cs_block_tab->add(name, b);
    }
    b->set_control(cs_control[cs_stackp+1]);
    cs_control[cs_stackp+1] = cs_cend[cs_stackp+1] = 0;
}
// End of ControlStack functions.


namespace {
    // Little hack to take care of various input substitutions.
    //
    wordlist *quicksub(wordlist *wl)
    {
        wordlist *nwl = wordlist::copy(wl);
        CP.DoGlob(&nwl);
        CP.BackQuote(&nwl);
        CP.VarSubst(&nwl);
        return (nwl);
    }
}


// Static function.
// Add wl to the control structure list.  If cs_cend->co_type is
// CO_UNFILLED, the last line was the beginning of a block, and this
// is the unfilled first statement.
//
void
sControl::set_block(wordlist *wl)
{
    sControl *cur = CS.cur_cend();

    if (cur && (cur->co_type != CO_UNFILLED)) {
        cur->co_next = new sControl;
        cur->co_next->co_prev = cur;
        cur->co_next->co_parent = cur->co_parent;
        cur = cur->co_next;
    }
    else if (!cur) {
        cur = new sControl;
        CS.set_cur_control(cur);
    }

    if (lstring::eq(wl->wl_word, "while")) {
        cur->co_type = CO_WHILE;
        cur->co_cond = wordlist::copy(wl->wl_next);
        if (!cur->co_cond) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "missing while condition.\n");
        }
        cur = cur->newblock();
    }
    else if (lstring::eq(wl->wl_word, "dowhile")) {
        cur->co_type = CO_DOWHILE;
        cur->co_cond = wordlist::copy(wl->wl_next);
        if (!cur->co_cond) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "missing dowhile condition.\n");
        }
        cur = cur->newblock();
    }
    else if (lstring::eq(wl->wl_word, "repeat")) {
        cur->co_type = CO_REPEAT;
        if (!wl->wl_next)
            cur->co_numtimes = -1;
        else {
            const char *s = 0;
            wordlist *ww = quicksub(wl);
            if (ww && ww->wl_next)
                s = ww->wl_next->wl_word;
            double *dd = SPnum.parse(&s, false);
            if (dd) {
                if (*dd < 0) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "can't repeat a negative number of times.\n");
                    *dd = 0.0;
                }
                cur->co_numtimes = (int) *dd;
            }
            else
                GRpkgIf()->ErrPrintf(ET_ERROR, 
                    "bad repeat argument %s.\n", s ? s : "");
            if (ww)
                wordlist::destroy(ww);
        }
        cur = cur->newblock();
    }
    else if (lstring::eq(wl->wl_word, "return")) {
        cur->co_type = CO_RETURN;
        cur->co_text = wordlist::copy(wl->wl_next);
    }
    else if (lstring::eq(wl->wl_word, "if")) {
        cur->co_type = CO_IF;
        cur->co_cond = wordlist::copy(wl->wl_next);
        if (!cur->co_cond)
            GRpkgIf()->ErrPrintf(ET_ERROR, "missing if condition.\n");
        cur = cur->newblock();
    }
    else if (lstring::eq(wl->wl_word, "foreach")) {
        cur->co_type = CO_FOREACH;
        wordlist *ww = wl;
        if (ww->wl_next) {
            ww = ww->wl_next;
            cur->co_foreachvar = lstring::copy(ww->wl_word);
            ww = ww->wl_next;
        }
        else
            GRpkgIf()->ErrPrintf(ET_ERROR, "missing foreach variable.\n");
        ww = wordlist::copy(ww);
        CP.DoGlob(&ww);
        cur->co_text = ww;
        cur = cur->newblock();
    }
    else if (lstring::eq(wl->wl_word, "label")) {
        cur->co_type = CO_LABEL;
        if (wl->wl_next) {
            cur->co_text = wordlist::copy(wl->wl_next);
            if (wl->wl_next->wl_next)
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "ignored extra junk after label.\n");
        }
        else
            GRpkgIf()->ErrPrintf(ET_ERROR, "missing label.\n");
    }
    else if (lstring::eq(wl->wl_word, "goto")) {
        // Incidentally, this won't work if the values 1 and
        // 2 ever get to be valid character pointers -- I
        // think it's reasonably safe to assume they aren't...
        //
        cur->co_type = CO_GOTO;
        if (wl->wl_next) {
            cur->co_text = wordlist::copy(wl->wl_next);
            if (wl->wl_next->wl_next)
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "ignored extra junk after goto.\n");
        }
        else
            GRpkgIf()->ErrPrintf(ET_ERROR, "missing label.\n");
    }
    else if (lstring::eq(wl->wl_word, "continue")) {
        cur->co_type = CO_CONTINUE;
        if (wl->wl_next) {
            cur->co_numtimes = lstring::scannum(wl->wl_next->wl_word);
            if (wl->wl_next->wl_next)
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "ignored extra junk after continue %d.\n", 
                    cur->co_numtimes);
        }
        else
            cur->co_numtimes = 1;
    }
    else if (lstring::eq(wl->wl_word, "break")) {
        cur->co_type = CO_BREAK;
        if (wl->wl_next) {
            cur->co_numtimes = lstring::scannum(wl->wl_next->wl_word);
            if (wl->wl_next->wl_next)
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "ignored extra junk after break %d.\n", 
                    cur->co_numtimes);
        }
        else
            cur->co_numtimes = 1;
    }
    else if (lstring::eq(wl->wl_word, "end")) {
        // Throw away this thing
        if (!cur->co_parent) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "no block to end.\n");
            cur->co_type = CO_UNFILLED;
        }
        else if (cur->co_prev) {
            cur->co_prev->co_next = 0;
            sControl *x = cur;
            cur = cur->co_parent;
            delete x;
        }
        else {
            sControl *x = cur;
            cur = cur->co_parent;
            cur->co_children = 0;
            delete x;
        }
    }
    else if (lstring::eq(wl->wl_word, "else")) {
        if (!cur->co_parent ||
                (cur->co_parent->co_type != CO_IF)) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "misplaced else.\n");
            cur->co_type = CO_UNFILLED;
        }
        else {
            if (cur->co_prev)
                cur->co_prev->co_next = 0;
            else
                cur->co_parent->co_children = 0;
            cur->co_parent->co_elseblock = cur;
            cur->co_prev = 0;
        }
    }
    else {
        cur->co_type = CO_STATEMENT;
        cur->co_text = wordlist::copy(wl);
    }
    CS.set_cur_cend(cur);
}


// Static function.
// Evaluate the statement block.
//
void
sControl::eval_block(sControl *x)
{
    // We have to toss this do-while loop in here so
    // that gotos at the top level will work.
    //
    do {
        retinfo ri;
        x->doblock(&ri);
        switch (ri.type()) {

            case NORMAL:
                break;

            case BROKEN:
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "break not in loop or too many break levels given.\n");
                break;

            case CONTINUED:
                GRpkgIf()->ErrPrintf(ET_ERROR,
                "continue not in loop or too many continue levels given.\n");
                break;

            case LABEL:
                x = sControl::find_label(CS.cur_control(), ri.label());
                if (!x)
                    GRpkgIf()->ErrPrintf(ET_ERROR, "label %s not found.\n",
                        ri.label());
                ri.set_label(0);
                break;

            case RETURN:
                x = 0;
                break;
        }
        if (x)
            x = x->co_next;
    } while (x);
}


// Static function.
sControl *
sControl::find_label(sControl *ct, const char *s)
{
    while (ct) {
        if (ct->co_type == CO_LABEL && lstring::eq(s, ct->co_text->wl_word))
            break;
        ct = ct->co_next;
    }
    return (ct);
}


// Execute a block.  There can be a number of return values from this
// function.
//
// NORMAL indicates a normal termination.
//
// BROKEN indicates a break -- if the caller is a breakable loop, 
//      terminate it, otherwise pass the break upwards.
//
// CONTINUED indicates a continue -- if the caller is a continuable loop, 
//      continue, else pass the continue upwards.
//
// RETURN indicates function termination due to a return statement.
//
// Any other return code is considered a pointer to a string which is
// a label somewhere -- if this label is present in the block, goto
// it, otherwise pass it up.  Note that this prevents jumping into a
// loop, which is good.  Note that here is where we expand variables,
// ``, and globs for controls.  The 'num' argument is used by break n
// and continue n.
//
void
sControl::doblock(retinfo *info)
{
    if (CP.GetFlag(CP_INTRPT)) {
        info->set_type(NORMAL);
        return;
    }
    sControl *ch, *cn = 0;
    wordlist *wl, *ww;
    retinfo ri;
    switch (co_type) {
    case CO_WHILE:
        while (co_cond && Sp.IsTrue(co_cond)) {
            for (ch = co_children; ch; ch = cn) {
                cn = ch->co_next;
                if (CP.GetFlag(CP_INTRPT)) {
                    info->set_type(NORMAL);
                    return;
                }
                ch->doblock(&ri);
                switch (ri.type()) {
                case NORMAL:
                    break;

                case BROKEN:    // Break
                    if (ri.num() < 2)
                        info->set_type(NORMAL);
                    else {
                        info->set_num(ri.num() - 1);
                        info->set_type(BROKEN);
                    }
                    return;

                case CONTINUED: // Continue
                    if (ri.num() < 2) {
                        cn = 0;
                        break;
                    }
                    else {
                        info->set_num(ri.num() - 1);
                        info->set_type(CONTINUED);
                        return;
                    }

                case LABEL:
                    cn = find_label(co_children, ri.label());
                    if (!cn) {
                        info->set_type(LABEL);
                        info->set_label(ri.label());
                        return;
                    }
                    ri.set_label(0);
                    break;

                case RETURN:
                    info->set_type(RETURN);
                    return;
                }
            }
        }
        break;

    case CO_DOWHILE:
        do {
            for (ch = co_children; ch; ch = cn) {
                cn = ch->co_next;
                if (CP.GetFlag(CP_INTRPT)) {
                    info->set_type(NORMAL);
                    return;
                }
                ch->doblock(&ri);
                switch (ri.type()) {
                case NORMAL:
                    break;

                case BROKEN:    // Break
                    if (ri.num() < 2)
                        info->set_type(NORMAL);
                    else {
                        info->set_num(ri.num() - 1);
                        info->set_type(BROKEN);
                    }
                    return;

                case CONTINUED: // Continue
                    if (ri.num() < 2) {
                        cn = 0;
                        break;
                    }
                    else {
                        info->set_num(ri.num() - 1);
                        info->set_type(CONTINUED);
                        return;
                    }

                case LABEL:
                    cn = find_label(co_children, ri.label());
                    if (!cn) {
                        info->set_type(LABEL);
                        info->set_label(ri.label());
                        return;
                    }
                    ri.set_label(0);
                    break;

                case RETURN:
                    info->set_type(RETURN);
                    return;
                }
            }
        } while (co_cond && Sp.IsTrue(co_cond));
        break;

    case CO_REPEAT:
        while ((co_numtimes > 0) ||
                (co_numtimes == -1)) {
            if (co_numtimes != -1)
                co_numtimes--;
            for (ch = co_children; ch; ch = cn) {
                cn = ch->co_next;
                if (CP.GetFlag(CP_INTRPT)) {
                    info->set_type(NORMAL);
                    return;
                }
                ch->doblock(&ri);
                switch (ri.type()) {
                case NORMAL:
                    break;

                case BROKEN:    // Break
                    if (ri.num() < 2)
                        info->set_type(NORMAL);
                    else {
                        info->set_num(ri.num() - 1);
                        info->set_type(BROKEN);
                    }
                    return;

                case CONTINUED: // Continue
                    if (ri.num() < 2) {
                        cn = 0;
                        break;
                    }
                    else {
                        info->set_num(ri.num() - 1);
                        info->set_type(CONTINUED);
                        return;
                    }

                case LABEL:
                    cn = find_label(co_children, ri.label());
                    if (!cn) {
                        info->set_type(LABEL);
                        info->set_label(ri.label());
                        return;
                    }
                    ri.set_label(0);
                    break;

                case RETURN:
                    info->set_type(RETURN);
                    return;
                }
            }
        }
        break;

    case CO_IF:
        if (co_cond && Sp.IsTrue(co_cond)) {
            for (ch = co_children; ch; ch = cn) {
                cn = ch->co_next;
                ch->doblock(&ri);
                if (ri.type() == BROKEN || ri.type() == CONTINUED) {
                    info->set_type(ri.type());
                    info->set_num(ri.num());
                    return;
                }
                if (ri.type() == LABEL) {
                    cn = find_label(co_children, ri.label());
                    if (!cn) {
                        info->set_type(LABEL);
                        info->set_label(ri.label());
                        return;
                    }
                    ri.set_label(0);
                }
                else if (ri.type() == RETURN) {
                    info->set_type(RETURN);
                    return;
                }
            }
        }
        else {
            for (ch = co_elseblock; ch; ch = cn) {
                cn = ch->co_next;
                ch->doblock(&ri);
                if (ri.type() == BROKEN || ri.type() == CONTINUED) {
                    info->set_type(ri.type());
                    info->set_num(ri.num());
                    return;
                }
                if (ri.type() == LABEL) {
                    cn = find_label(co_elseblock, ri.label());
                    if (!cn) {
                        info->set_type(LABEL);
                        info->set_label(ri.label());
                        return;
                    }
                    ri.set_label(0);
                }
                else if (ri.type() == RETURN) {
                    info->set_type(RETURN);
                    return;
                }
            }
        }
        break;

    case CO_FOREACH:
        ww = quicksub(co_text);
        for (wl = ww; wl; wl = wl->wl_next) {
            variable v;
            v.set_string(wl->wl_word);
            CP.RawVarSet(co_foreachvar, true, &v);
            for (ch = co_children; ch; ch = cn) {
                cn = ch->co_next;
                if (CP.GetFlag(CP_INTRPT)) {
                    info->set_type(NORMAL);
                    return;
                }
                ch->doblock(&ri);
                switch (ri.type()) {
                case NORMAL:
                    break;

                case BROKEN:    // Break
                    if (ri.num() < 2)
                        info->set_type(NORMAL);
                    else {
                        info->set_num(ri.num() - 1);
                        info->set_type(BROKEN);
                    }
                    return;

                case CONTINUED: // Continue
                    if (ri.num() < 2) {
                        cn = 0;
                        break;
                    }
                    else {
                        info->set_num(ri.num() - 1);
                        info->set_type(CONTINUED);
                        return;
                    }

                case LABEL:
                    cn = find_label(co_children, ri.label());
                    if (!cn) {
                        info->set_type(LABEL);
                        info->set_label(ri.label());
                        return;
                    }
                    ri.set_label(0);
                    break;

                case RETURN:
                    info->set_type(RETURN);
                    return;
                }
            }
        }
        wordlist::destroy(ww);
        break;

    case CO_BREAK:
        if (co_numtimes > 0) {
            info->set_num(co_numtimes);
            info->set_type(BROKEN);
            return;
        }
        GRpkgIf()->ErrPrintf(ET_WARN, "break %d a no-op.\n", co_numtimes);
        break;

    case CO_CONTINUE:
        if (co_numtimes > 0) {
            info->set_num(co_numtimes);
            info->set_type(CONTINUED);
            return;
        }
        GRpkgIf()->ErrPrintf(ET_WARN, "continue %d a no-op.\n",
            co_numtimes);
        break;

    case CO_GOTO:
        wl = quicksub(co_text);
        if (wl) {
            if (wl->wl_word) {
                info->set_type(LABEL);
                info->set_label(wl->wl_word);
                return;
            }
            wordlist::destroy(wl);
        }
        break;

    case CO_LABEL:
        // Do nothing
        break;

    case CO_STATEMENT:
        CP.DoCommand(wordlist::copy(co_text));
        break;

    case CO_RETURN:
        info->set_type(RETURN);
        if (co_text) {
            wordlist *w = wordlist::copy(co_text);
            char *str = wordlist::flatten(w);
            const char *wp = str;
            pnode *nn = Sp.GetPnode(&wp, true);
            if (!nn) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "evaluation failed: %s.\n", str);
                return;
            }
            sDataVec *t = Sp.Evaluate(nn);
            delete nn;
            if (!t)
                GRpkgIf()->ErrPrintf(ET_ERROR, "evaluation failed: %s.\n", str);
            else
                CP.SetReturnVal(t->realval(0));
        }
        return;

    case CO_UNFILLED:
        // There was probably an error here...
        GRpkgIf()->ErrPrintf(ET_WARN, "ignoring previous error.\n");
        break;

    default:
        GRpkgIf()->ErrPrintf(ET_INTERR, "doblock: bad block type %d.\n", 
            co_type);
    }
    info->set_type(NORMAL);
}


#define tabo(num)    for (i = 0; i < num; i++) TTY.send(" ")
#define tabf(num)    for (i = 0; i < num; i++) putc(' ', fp)

void
sControl::dodump(FILE *fp)
{
    bool useout = !fp;
    int i;
    sControl *tc;
    switch (co_type) {
    case CO_UNFILLED:
        if (useout) {
            tabo(co_indent);
            TTY.printf( "(unfilled)\n");
        }
        else {
            tabf(co_indent);
            fprintf(fp, "(unfilled)\n");
        }
        break;
    case CO_STATEMENT:
        if (useout) {
            tabo(co_indent);
            TTY.wlprint(co_text);
            TTY.send("\n");
        }
        else {
            tabf(co_indent);
            wordlist::print(co_text, fp);
            putc('\n', fp);
        }
        break;
    case CO_WHILE:
        if (useout) {
            tabo(co_indent);
            TTY.printf( "while ");
            TTY.wlprint(co_cond);
            TTY.send("\n");
        }
        else {
            tabf(co_indent);
            fprintf(fp, "while ");
            wordlist::print(co_cond, fp);
            putc('\n', fp);
        }
        co_indent += 8;
        for (tc = co_children; tc; tc = tc->co_next)
            tc->dodump(fp);
        co_indent -= 8;
        if (useout) {
            tabo(co_indent);
            TTY.printf( "end\n");
        }
        else {
            tabf(co_indent);
            fprintf(fp, "end\n");
        }
        break;
    case CO_REPEAT:
        if (useout) {
            tabo(co_indent);
            TTY.printf( "repeat ");
            if (co_numtimes != -1)
                TTY.printf( "%d\n", co_numtimes);
            else
                TTY.send("\n");
        }
        else {
            tabf(co_indent);
            fprintf(fp, "repeat ");
            if (co_numtimes != -1)
                fprintf(fp, "%d\n", co_numtimes);
            else
                putc('\n', fp);
        }
        co_indent += 8;
        for (tc = co_children; tc; tc = tc->co_next)
            tc->dodump(fp);
        co_indent -= 8;
        if (useout) {
            tabo(co_indent);
            TTY.printf( "end\n");
        }
        else {
            tabf(co_indent);
            fprintf(fp, "end\n");
        }
        break;
    case CO_DOWHILE:
        if (useout) {
            tabo(co_indent);
            TTY.printf( "dowhile ");
            TTY.wlprint(co_cond);
            TTY.send("\n");
        }
        else {
            tabf(co_indent);
            fprintf(fp, "dowhile ");
            wordlist::print(co_cond, fp);
            putc('\n', fp);
        }
        co_indent += 8;
        for (tc = co_children; tc; tc = tc->co_next)
            tc->dodump(fp);
        co_indent -= 8;
        if (useout) {
            tabo(co_indent);
            TTY.printf( "end\n");
        }
        else {
            tabf(co_indent);
            fprintf(fp, "end\n");
        }
        break;
    case CO_IF:
        if (useout) {
            tabo(co_indent);
            TTY.printf( "if ");
            TTY.wlprint(co_cond);
            TTY.send("\n");
        }
        else {
            tabf(co_indent);
            fprintf(fp, "if ");
            wordlist::print(co_cond, fp);
            putc('\n', fp);
        }
        co_indent += 8;
        for (tc = co_children; tc; tc = tc->co_next)
            tc->dodump(fp);
        co_indent -= 8;
        if (useout) {
            tabo(co_indent);
            TTY.printf( "end\n");
        }
        else {
            tabf(co_indent);
            fprintf(fp, "end\n");
        }
        break;
    case CO_FOREACH:
        if (useout) {
            tabo(co_indent);
            TTY.printf( "foreach %s ", co_foreachvar);
            TTY.wlprint(co_text);
            TTY.send("\n");
        }
        else {
            tabf(co_indent);
            fprintf(fp, "foreach %s ", co_foreachvar);
            wordlist::print(co_text, fp);
            putc('\n', fp);
        }
        co_indent += 8;
        for (tc = co_children; tc; tc = tc->co_next)
            tc->dodump(fp);
        co_indent -= 8;
        if (useout) {
            tabo(co_indent);
            TTY.printf( "end\n");
        }
        else {
            tabf(co_indent);
            fprintf(fp, "end\n");
        }
        break;
    case CO_BREAK:
        if (useout) {
            tabo(co_indent);
            if (co_numtimes != 1)
                TTY.printf( "break %d\n", co_numtimes);
            else
                TTY.printf( "break\n");
        }
        else {
            tabf(co_indent);
            if (co_numtimes != 1)
                fprintf(fp, "break %d\n", co_numtimes);
            else
                fprintf(fp, "break\n");
        }
        break;
    case CO_CONTINUE:
        if (useout) {
            tabo(co_indent);
            if (co_numtimes != 1)
                TTY.printf( "continue %d\n", co_numtimes);
            else
                TTY.printf( "continue\n");
        }
        else {
            tabf(co_indent);
            if (co_numtimes != 1)
                fprintf(fp, "continue %d\n", co_numtimes);
            else
                fprintf(fp, "continue\n");
        }
        break;
    case CO_LABEL:
        if (useout) {
            tabo(co_indent);
            TTY.printf( "label %s\n", co_text->wl_word);
        }
        else {
            tabf(co_indent);
            fprintf(fp, "label %s\n", co_text->wl_word);
        }
        break;
    case CO_GOTO:
        if (useout) {
            tabo(co_indent);
            TTY.printf( "goto %s\n", co_text->wl_word);
        }
        else {
            tabf(co_indent);
            fprintf(fp, "goto %s\n", co_text->wl_word);
        }
        break;
    default:
        if (useout) {
            tabo(co_indent);
            TTY.printf( "bad type %d\n", co_type);
        }
        else {
            tabf(co_indent);
            fprintf(fp, "bad type %d\n", co_type);
        }
        break;
    }
}
// End of sControl functions.

