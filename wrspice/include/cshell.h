
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Wayne A. Christopher, U. C. Berkeley CAD Group
**********/

#ifndef CSHELL_H
#define CSHELL_H

//
// General stuff for the C-shell parser.
//

#include "config.h"
#include "misc.h"
#include "ttyio.h"
#include "wlist.h"
#include "complete.h"

// references
struct variable;
struct tmpFileList;
struct sControl;
struct sVarDb;

// The Windows version used to need setjmp, make sure this is no longer
// true.  If so, remove all setjmp stuff and don't include setjmp.h above.
//#define USE_SETJMP_EXP
#if defined(USE_SETJMP_EXP) && defined(HAVE_SETJMP_H)
#include <setjmp.h>
extern jmp_buf msw_jbf[4];
extern int msw_jbf_sp;
#endif

#define MAXWORDS 512  // maximum global matches
#define CP_DefHistLen 1000  // default history list length


// The history list. Things get put here after the first (basic) parse.
// The word list will change later, so be sure to copy it.
//
struct sHistEnt
{
    sHistEnt(int ev, wordlist *wl)
        {
            hi_event = ev;
            hi_wlist = wordlist::copy(wl);
            hi_next = 0;
            hi_prev = 0;
        }

    ~sHistEnt()
        {
            wordlist::destroy(hi_wlist);
        }

    int event()                 { return (hi_event); }
    wordlist *text()            { return (hi_wlist); }
    sHistEnt *next()            { return (hi_next); }
    void set_next(sHistEnt *n)  { hi_next = n; }
    sHistEnt *prev()            { return (hi_prev); }
    void set_prev(sHistEnt *n)  { hi_prev = n; }

    void update(wordlist *wl)
        {
            wordlist::destroy(hi_wlist);
            hi_wlist = wordlist::copy(wl);
        }

private:
    int hi_event;         // Event number
    wordlist *hi_wlist;   // Command text
    sHistEnt *hi_next;    // Next command
    sHistEnt *hi_prev;    // Previous command
};


// Aliases. These will be expanded if the word is the first in an input
// line. The substitution string may contain arg selectors.
//
struct sAlias
{
    sAlias(const char *n, wordlist *wl)
        {
            al_name = lstring::copy(n);
            al_text = wordlist::copy(wl);
            al_next = 0;
            al_prev = 0;
        }

    ~sAlias()
        {
            delete [] al_name;
            wordlist::destroy(al_text);
        }

    const char *name()          { return (al_name); }
    wordlist *text()            { return (al_text); }
    sAlias *next()              { return (al_next); }
    void set_next(sAlias *n)    { al_next = n; }
    sAlias *prev()              { return (al_prev); }
    void set_prev(sAlias *n)    { al_prev = n; }

private:
    const char *al_name;  // The word to be substituted for
    wordlist *al_text;    // What to substitute for it
    sAlias *al_next;      // Next alias in list
    sAlias *al_prev;      // Previous alias in list
};

// CshPar boolean flag indices.
//
enum CPflag {
    CP_CWAIT,                   // True when parsing input.
    CP_DEBUG,                   // Debugging messages are printed.
    CP_DOUNIXCOM,               // Hash unix shell commands.
    CP_IGNOREEOF,               // Ignore EOF on input.
    CP_INTERACTIVE,             // Interactive mode.
    CP_INTRPT,                  // Interrupt received.
    CP_MESSAGE,                 // True when receiving IPC.
    CP_NOCC,                    // Disable command completion.
    CP_NOCLOBBER,               // Don't overwrite files.
    CP_NOEDIT,                  // Disable command line editing.
    CP_LOCK_NOEDIT,             // Don't let cp_noedit change.
    CP_NOGLOB,                  // Skip global matching.
    CP_NONOMATCH,               // Use literally if no match.
    CP_NOTTYIO,                 // Prevent changes to terminal.
    CP_WAITING,                 // True when waiting for input.
    CP_RAWMODE,                 // stdin is in raw mode.
    CP_NOBRKTOK,                // Lexer shouldn't take <>&; as token sep. 
    CP_NUMFLAGS
};

// For quoting individual characters. '' strings are all quoted, but `` and
// "" strings are maintained as single words with the quotes around them.
// Note that this won't work on non-ascii machines.
//
#define QUOTE(c)    ((c) | 0x80)
#define STRIP(c)    ((c) & 0x7f)

// This specifies the default concatenation character for shell
// variable expansion.  It terminates the shell variable token, and is
// needed if other than white space or punctuation follows.
//
#define DEF_VAR_CATCHAR '%'


// Csh-like shell interpreter class
//
struct CshPar
{
    friend struct sLx;

    CshPar();

    wordlist *LexString(const char *str)
        {
            wordlist *wl = Lexer(str);
            StripList(wl);
            return (wl);
        }

    wordlist *LexStringSub(const char *str)
        {
            wordlist *wl = Lexer(str);
            if (wl)
                VarSubst(&wl);
            if (wl)
                BackQuote(&wl);
            StripList(wl);
            return (wl);
        }

    // alias.cc
    void DoAlias(wordlist**);               // Perform alias substitution
    void SetAlias(const char*, wordlist*);  // Add new alias to list
    void SetAlias(const char*, const char*); // Add new alias to list
    void Unalias(const char*);              // Remove alias from list
    void PrintAliases(const char*);         // Print alias of arg
    sAlias *GetAlias(const char*);          // Return entry for word

    // backq.cc
    void BackQuote(wordlist**);             // Perform backquote substit.
    wordlist *BackEval(const char*);        // Evaluate backquote

    // complete.cc
    void Complete(wordlist*, const char*, bool); // Complete last word of input
    void AddKeyword(int, const char*);      // Add word to cc database
    void AddCommand(const char*, unsigned*); // Add command to cc database
    void RemKeyword(int, const char*);      // Remove word from cc database
    void CcRestart(bool);                   // Clear completion database
    sTrie **CcClass(int);                   // Return completion data class
    wordlist *FileC(const char*);           // Return filename matches

    // cshpar.cc
    wordlist *Parse(const char*);           // Parse wordlist from cmd line
    void Redirect(wordlist**);              // Set up IO redirection
    void SetReturnVal(double);              // Ser return value for function.

    // front.cc
    void AddBlock(const char*, wordlist*);  // Add codeblock to list
    void ExecBlock(const char*);            // Execute a codeblock
    void PrintBlock(const char*, FILE* = 0);// Print a codeblock
    bool IsBlockDef(const char*);           // See if named block exists
    void FreeBlock(const char*);            // Delete a codeblock
    int  EvLoop(const char*);               // Evaluate commands
    sControl *MakeControl(wordlist*);       // Make control structure
    void PrintControl(sControl*, FILE* = 0);// Print control structures
    void ExecControl(sControl*);            // Execute a control struct
    void FreeControl(sControl*);            // Delete a control struct
    void ResetControl();                    // Clear all control structs
    void PopControl();                      // Return from context switch
    void PushControl();                     // Switch to new control context
    void TopLevel();                        // Go to top context level
    void SetAltPrompt();                    // Set prompt for contest level
    wordlist *GetCommand(const char*);      // Get a command
    void DoCommand(wordlist*);              // Process a command
    void Reset(bool);                       // Reset after interrupt

    // glob.cc
    void DoGlob(wordlist**);                // Perform global matching
    bool GlobMatch(const char*, const char*); // Test if strings match
    char *TildeExpand(const char*);         // Expand tildes (user name)
    wordlist *GlobExpand(const char*);      // Expand *?[]
    wordlist *BracExpand(const char*);      // Expand {...}

    // history.cc
    void HistSubst(wordlist**);             // Perform history substitution
    void AddHistEnt(int, wordlist*);        // Add command to history list
    void HistPrint(int, int, bool);         // Print command from list

    // ipcomm.cc
    bool InitIPC();                         // Initialize IPC, call listen()
    bool InitIPCmessage();                  // Call accept()
    bool MessageHandler(int);               // Handle IPC messages

    // lexical.cc
    wordlist *PromptUser(const char*);      // Get string from user
    wordlist *Lexer(const char*);           // Lexically analyze input
    void Prompt();                          // Issue command prompt
    void SetupTty(int, bool);               // Initialize TTY
    int Getchar(int, bool, bool = false);   // Get a char
    int RawGetc(int);                       // Get a char, raw mode
    void StuffChars(const char*);           // do TIOCSTI

    // quote.c
    void Strip(char*);                      // Destructively remove 8'th bit
    void QuoteWord(char*);                  // Turn on 8'th bit
    void PrintWord(const char*, FILE*);     // Print quoted text
    void StripList(wordlist*);              // Strip a list of words
    void Unquote(char*);                    // Remove "..." around word

    // unixcom.cc
    void Rehash(const char*, bool);         // Rebuild command database
    bool UnixCom(wordlist*);                // Execute a shell command
    void HashStat();                        // Print stats for debugging
    bool ShellExec(const char*, char**);    // Exec shell command, give path
    bool IsExecutable(const char*, const char*); // Test if file is executable
    int System(const char*);                // Exec shell command

    // variable.cc
    wordlist *VarList();                    // Return list of variables set
    void VarSubst(wordlist**);              // Perform variable substitution
    void VarSubst(char**);                  // Perform variable substitution
    wordlist *VarEval(const char*);         // Evaluate a variable
    variable *RawVarGet(const char*);       // Get a shell variable
    void RawVarSet(const char*, bool, variable*); // Set a shell variable
    variable *ParseSet(wordlist*);          // Process the "set" args
    variable *GetList(wordlist**);          // Get a list of variables
    void PushArg(wordlist*);                // Push argc, argv variables
    void PopArg();                          // Pop argc, argv variables
    void ClearArg();                        // Clear arg stack

    const char *Program()                   { return (cp_program); }
    void SetProgram(const char *p)
        {
            char *s = lstring::copy(p);
            delete cp_program;
            cp_program = s;
        }

    const char *Display()                   { return (cp_display); }
    void SetDisplay(const char *d)
        {
            char *s = lstring::copy(d);
            delete cp_display;
            cp_display = s;
        }

    const char *PromptString()              { return (cp_promptstring); }
    void SetPromptString(const char *p)
        {
            char *s = lstring::copy(p);
            delete cp_promptstring;
            cp_promptstring = s;
        }

    bool Return()                           { return (cp_return); }
    void SetReturn(bool b)                  { cp_return = b; }

    double ReturnVal()                      { return (cp_return_val); }

    int Event()                             { return (cp_event); }

    int HistLength()                        { return (cp_histlength); }
    int MaxHistLength()                     { return (cp_maxhistlength); }
    void SetMaxHistLength(int l)            { cp_maxhistlength = l; }

    int NumDigits()                         { return (cp_numdgt); }
    void SetNumDigits(int n)                { cp_numdgt = n; }

    int AcctSocket()                        { return (cp_acct_sock); }
    void SetAcctSocket(int s)               { cp_acct_sock = s; }
    int MesgSocket()                        { return (cp_mesg_sock); }
    void SetMesgSocket(int s)               { cp_mesg_sock = s; }
    int Port()                              { return (cp_port); }
    void SetPort(int p)                     { cp_port = p; }

    bool GetFlag(CPflag f)                  { return (cp_flags[f]); }
    void SetFlag(CPflag f, bool b)          { cp_flags[f] = b; }

    char VarCatchar()                       { return (cp_var_catchar); }
    void SetVarCatchar(char c)              { cp_var_catchar = c; }

    void AddPendingSource(const char *fn)
        {
            cp_srcfiles = wordlist::append(cp_srcfiles, new wordlist(fn, 0));
        }

    void QueueInterrupt()                   { cp_queue_interrupt = true; }

    static sVarDb *VarDb()                  { return (cp_vardb); }

private:
    // cshpar.c
    void pwlist(wordlist*, const char*);

    // glob.cc
    wordlist *brac1(const char*);
    wordlist *brac2(const char*);
    bool noglobs(const char*);

    // history.cc
    wordlist *dohsubst(char*);
    wordlist *hpattern(const char*);
    wordlist *hprefix(const char*);
    wordlist *getevent(int);
    void freehist(int);

    // variable.cc
    char *dollartok(char*);

    char *cp_program;                       // Application name.
    char *cp_display;                       // X display name, if using X.
    char *cp_promptstring;                  // String used for cmd prompt.
    char *cp_altprompt;                     // Alt prompt for block.

    FILE *cp_input;                         // Override input pointer.
    sAlias *cp_aliases;                     // The alias list.
    sHistEnt *cp_lastone;                   // Last command, from history.

    wordlist *cp_srcfiles;                  // List of files to source.

    double cp_return_val;                   // Global return value.

    int cp_event;                           // History event count.
    int cp_histlength;                      // Length of history list.
    int cp_maxhistlength;                   // Maximum length of history list.
    int cp_numdgt;                          // Sig. figs in number print.

    int cp_acct_sock;                       // Socket for IPC control.
    int cp_mesg_sock;                       // Socket for IPC messages.
    int cp_port;                            // IPC port.

    bool cp_flags[CP_NUMFLAGS];             // Misc. flags.
    bool cp_return;                         // Return from script when set.

    char cp_amp;                            // '&'
    char cp_back;                           // '`'
    char cp_bang;                           // '!'
    char cp_cbrac;                          // ']'
    char cp_ccurl;                          // '}'
    char cp_comma;                          // ','
    char cp_csep;                           // ';'
    char cp_dol;                            // '$'
    char cp_gt;                             // '>'
    char cp_hash;                           // '#'
    char cp_hat;                            // '^'
    char cp_huh;                            // '?'
    char cp_lt;                             // '<'
    char cp_obrac;                          // '['
    char cp_ocurl;                          // '{'
    char cp_star;                           // '*'
    char cp_til;                            // '~'

    char cp_var_catchar;

    bool cp_didhsubst;                      // Did a history substitution.
    bool cp_bqflag;                         // True if processing backquote.
    bool cp_queue_interrupt;                // Waiting to safely handle ^C.

    static sVarDb *cp_vardb;                // Pointer to variables database.
};

extern CshPar CP;

#endif // CSHELL_H

