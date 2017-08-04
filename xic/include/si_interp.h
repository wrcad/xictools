
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

#ifndef SI_INTERP_H
#define SI_INTERP_H


class cGroupDesc;
class SIinterp;
struct PCellParam;
struct sCrypt;
struct SIlexp_list;
struct SImacroHandler;

//-----------------------------------------------------------------------------
// Control Blocks

// Control block types
//
enum
{
    CO_UNFILLED  = 0,
    CO_LABEL     = 1,
    CO_GOTO      = 2,
    CO_STATEMENT = 3,
    CO_DELETE    = 4,
    CO_REPEAT    = 5,
    CO_WHILE     = 6,
    CO_DOWHILE   = 7,
    CO_IF        = 8,
    CO_ELIF      = 9,
    CO_ELSE      = 10,
    CO_END       = 11,
    CO_BREAK     = 12,
    CO_CONTINUE  = 13,
    CO_RETURN    = 14,
    CO_FUNCTION  = 15,
    CO_ENDFUNC   = 16,
    CO_STATIC    = 17,
    CO_GLOBAL    = 18,
    CO_EXEC      = 19
};
typedef unsigned short CblkType;

// Struct to hold results from block evaluation.
//
// CS_NORMAL indicates a normal termination
// CS_BROKEN indicates a break -- if the caller is a breakable loop,
//      terminate it, otherwise pass the break upwards
// CS_CONTINUED indicates a continue -- if the caller is a continuable loop,
//      continue, else pass the continue upwards
// The 'num' is the break/continue argument.
// CS_LABEL indicates a pointer to a string which is a label somewhere
//      if this label is present in the block, goto it, otherwise pass it up.
// Note that this prevents jumping into a loop, which is good.
// CS_RETURN indicates a return statement, halt everything and exit, passing
//      value.
//
enum Cstatus { CS_NORMAL, CS_BROKEN, CS_CONTINUED, CS_LABEL, CS_RETURN };

struct SIretinfo
{
    SIretinfo()
        {
            type = CS_NORMAL;
            rt.value = 0.0;
        }

    Cstatus type;
    union {
        char *label;
        int num;
        double value;
    } rt;
};

// The control block struct.
//
struct SIcontrol
{
    SIcontrol(SIcontrol *p)
        {
            co_type = CO_UNFILLED;
            co_lineno = 0;
            co_content.text = 0;
            co_parent = p;
            co_children = 0;
            co_elseblock = 0;
            co_next = 0;
            co_prev = 0;
        }

    SIcontrol *findlabel(const char*);
    void print(int, FILE*);
    static void destroy(SIcontrol*);

    CblkType co_type;             // Block type
    unsigned short co_lineno;     // Source file line number
    union {
        ParseNode *text;          // Executable
        SIifel *ifcond;           // For if/elif
        char *label;              // goto, label
        int count;                // break & continue levels
    } co_content;                 // Type dependent content
    SIcontrol *co_parent;         // If this is inside a block
    SIcontrol *co_children;       // The contents of this block
    SIcontrol *co_elseblock;      // For if-then-else
    SIcontrol *co_next;           // Next in list for child block
    SIcontrol *co_prev;           // Prev in list for child block
};

// List of if-then clauses for CO_IF block, for if and elif.
//
struct SIifel
{
    SIifel(SIcontrol *p)
        {
            text = 0;
            children = 0;
            parent = p;
            next = 0;
        }

    ~SIifel()
        {
            ParseNode::destroy(text);
            SIcontrol::destroy(children);
        }

    static void destroy(SIifel *n)
        {
            while (n) {
                SIifel *nx = n;
                n = n->next;
                delete nx;
            }
        }

    ParseNode *text;
    SIcontrol *children;
    SIcontrol *parent;
    SIifel *next;
};


//-----------------------------------------------------------------------------
// Functions

// Since we add variables to the end of the list, define a separate
// reference mechanism for argument variables.  If wariables were
// added to the beginning, this would not be necessary.
//
struct SIarg
{
    SIarg(Variable *v)
        {
            var = v;
            next = 0;
        }

    static void destroy(SIarg *a)
        {
            while (a) {
                SIarg *ax = a;
                a = a->next;
                delete ax;
            }
        }

    Variable *var;
    SIarg *next;
};

// Context description for a function.
//
struct SIfunc
{
    SIfunc()
        {
            sf_name = 0;
            sf_args = 0;
            sf_variables = 0;
            sf_varinit = 0;
            sf_text = 0;
            sf_end = 0;
            sf_exprs = 0;
            sf_refcnt = 0;
        }

    SIfunc(char *n)
        {
            sf_name = n;
            sf_args = 0;
            sf_variables = 0;
            sf_varinit = 0;
            sf_text = new SIcontrol(0);
            sf_end = sf_text;
            sf_exprs = 0;
            sf_refcnt = 0;
        }

    ~SIfunc();
    void clear();
    void parse_args(const char**, bool*);
    void init_local_vars();
    bool save_vars(siVariable**);
    bool restore_vars(Variable*, Variable*);

    char *sf_name;              // function name
    SIarg *sf_args;             // formal arguments
    siVariable *sf_variables;   // local variables
    siVariable *sf_varinit;     // copy of local variables
    SIcontrol *sf_text;         // parse tree
    SIcontrol *sf_end;          // end pointer, for building tree
    SIlexp_list *sf_exprs;      // related layer expressions
    int sf_refcnt;              // number of referencing ParseNodes
};

// Call stack element.
//
struct SIcstack
{
    SIcstack()
        {
            sf = 0;
            vars = 0;
            funcvars = 0;
            stack = 0;
        }

    SIfunc *sf;                 // current block
    siVariable *vars;           // parent variables
    siVariable *funcvars;       // previous variables of function
    sBlk *stack;                // control blocks
};

// Returned from SIfile::create.
enum sif_err { sif_ok, sif_open, sif_crypt };

// Special "file descriptor" to deal with decryption.
//
struct SIfile
{
    SIfile(FILE *f, sCrypt *c, char *fn)
        {
            fp = f;
            cr = c;
            filename = fn;
        }

    // si_support.cc
    static SIfile *create(const char*, FILE*, sif_err*);
    ~SIfile();
    int sif_getc();

    FILE *fp;
    sCrypt *cr;
    char *filename;
};


// This is a stack element used for control block evaluation.
//
struct sBlk
{
    sBlk(SIcontrol *c, sBlk *n)
        {
            bcur = c;
            bnext = n;
            bcount = 0;
            bchp = 0;
        }

    SIcontrol *bcur;        // the control block
    SIcontrol *bchp;        // sub-block (children) pointer
    sBlk *bnext;            // stack link pointer
    int bcount;             // flag for do-while, counter for repeat
};


// Context storage, to make interpreter reentrant.
//
struct sCx
{
    sCx(sCx*);

    SIfunc *sfunc()             { return (&cx_sfunc); }
    int line_cnt()              { return (cx_line_cnt); }
    int ex_line()               { return (cx_ex_line); }
    int level()                 { return (cx_level); }
    sBlk *stack()               { return (cx_stack); }
    siVariable *variables()     { return (cx_variables); }
    SImacroHandler *defines()   { return (cx_defines); }
    sCx *next()                 { return (cx_next); }

private:
    SIfunc          cx_sfunc;
    int             cx_line_cnt;
    int             cx_ex_line;
    int             cx_level;
    sBlk            *cx_stack;
    siVariable      *cx_variables;
    SImacroHandler  *cx_defines;
    sCx             *cx_next;
};

#define CALLSTACKSIZE 50

// We also keep a list of SIlexprCx contexts.  When possible, the
// context is instantiated on the stack at the top level of execution,
// which is passed down to lower levels during execution.  This
// provides thread independence.  However, for things like
// single-stepping, and the Tcl/Python interpreters, we need to
// allocate and export a context.  This provides a stack of allocated
// contexts.  We push/pop around script execution functions.  This is
// obviously not multi-thread safe.

struct sLCx
{
    sLCx(sLCx*);
    ~sLCx();

    sLCx *next;
    SIlexprCx *lcx;
    Zlist *zlbak;
};


inline class SIinterp *SI();

// The main class for the script interpreter.
//
class SIinterp
{
    static SIinterp *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline SIinterp *SI() { return (SIinterp::ptr()); }

    friend struct SIcontrol;
    friend struct sCx;

    // si_interp.cc
    SIinterp();
    static char *NextLine(int*, SIfile*, stringlist**, const char**);
    int Interpret(SIfile*, stringlist*, const char**, siVariable*,
        bool = false, const PCellParam* = 0);
    int LineInterp(const char*, siVariable*, bool = false,
        const PCellParam* = 0);
    SIfunc *GetBlock(SIfile*, stringlist*, const char**, siVariable**);
    XIrt EvalFunc(SIfunc*, void*, siVariable* = 0, siVariable* = 0);
    void Init();
    void Clear();
    bool ShowSubfuncs();
    SIfunc *GetSubfunc(const char*, int*);
    SIfunc *NewSubfunc(const char*);
    stringlist *GetSubfuncList();
    void FreeSubfunc(const char*);
    void ClearSubfuncs();
    void LineError(const char*, ...);

    // si_support.cc
    void RegisterLocalContext(SIlocal_context*);
    void Cleanup();
    void UpdateObject(CDo*, CDo*);
    void UpdatePrpty(CDo*, CDp*, CDp*);
    void UpdateCell(CDs*);
    void ClearGroups(cGroupDesc*);

    bool IsInBlock() { return (siBlockStepping); }

    SIcontrol *CurBlock()
        {
            return (siStackPtr > 0 ?
                 siCallStack[siStackPtr].sf->sf_text : siMain.sf_text);
        }

    SymTab *SetFuncTab(SymTab *t)
        {
            SymTab *tab = siFuncTab;
            siFuncTab = t;
            return (tab);
        }

    SymTab *GetFuncTab() { return (siFuncTab); }

    void PresetMacros(SImacroHandler *mh) { siMacrosPreset = mh; }

    void SetKey(const char *k)
        {
            if (!cryptKey)
                cryptKey = new char[13];
            memcpy(cryptKey, k, 13);
        }

    bool GetKey(char *k)
        {
            if (!cryptKey)
                 return (false);
            memcpy(k, cryptKey, 13);
            return (true);
        }

    // Return 1 if the script crashed. -1 if halted due to interrupt.
    // Clear the flags.
    //
    int GetError()
        {
            int e = siError ? 1 : (siInterrupt ? -1 : 0);
            siError = false;
            siInterrupt = false;
            return (e);
        }

    void ClearInterrupt()           { siHalt = false; siInterrupt = false; }
    void SetInterrupt()             { siHalt = true; siInterrupt = true; }
    void Halt()                     { siHalt = true; }
    bool IsHalted()                 { return (siHalt); }

    static bool LogIsLog10()        { return (siLogIsLog10); }
    static void SetLogIsLog10(bool b)   { siLogIsLog10 = b; }


    void PushLexprCx()              { siLCx = new sLCx(siLCx); }
    void PopLexprCx()
        {
            if (siLCx) {
                sLCx *lcx = siLCx;
                siLCx = siLCx->next;
                delete lcx;
            }
        }
    SIlexprCx *LexprCx()            { return (siLCx ? siLCx->lcx : 0); }

private:

    void push(SIcontrol *c) { if (c) siStack = new sBlk(c, siStack); }

    void pop()
        {
            if (siStack) {
                sBlk *s = siStack;
                siStack = siStack->bnext;
                delete s;
            }
        }

    void clear()
        {
            while (siStack) {
                sBlk *b = siStack->bnext;
                delete siStack;
                siStack = b;
            }
        }

    // si_interp.cc
    void eval_stmt(siVariable*, siVariable*, void*);
    void eval(siVariable*, siVariable*, void*);
    void eval_break(int);
    void eval_continue(int);
    void eval_goto(const char*);
    void postfunc();
    void push_cx();
    void pop_cx();
    void handle_keyword(SIfile*, stringlist**, const char**, const char*);
    void skip_block(SIfile*, stringlist**, const char**);
    void set_block(const char**, int);
    int gettokval(const char**);

    sCx *siContext;                 // execution context stack
    sLCx *siLCx;                    // layer expression context stack
    sBlk *siStack;                  // control block stack
    SIfunc *siCurFunc;              // current function
    SymTab *siFuncTab;              // saved sub-functions

    SImacroHandler *siDefines;      // #define macro handler
    SImacroHandler *siMacrosPreset; // moved to siDefines when siClear called
                                    // registered callback to predefine macros

    int siStackPtr;                 // execution block context
    int siBlockLineref;             // line number starting block
    int siLineCount;                // count of processed lines
    int siExecLine;                 // line being executing
    int siLevel;                    // level in #ifdef/#else/#endif
    bool siBlockStepping;           // single stepping through a block
    bool siSkipJunk;                // skip leading comments single stepping
    bool siError;                   // LineError called
    bool siInterrupt;               // interrupt signal received
    bool siHalt;                    // halt script execution immediately

    char *cryptKey;                 // decryption key
    SIlocal_context *siLocalContext; // user's data

    SIfunc siMain;                  // current context
    SIretinfo siRetinfo;            // evaluation return status
    SIcstack siCallStack[CALLSTACKSIZE]; // context stack

    static SIinterp *instancePtr;
    static bool siLogIsLog10;
};

#endif

