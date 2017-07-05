
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
 $Id: si_parsenode.h,v 5.43 2017/05/01 17:12:05 stevew Exp $
 *========================================================================*/

#ifndef SI_PARSENODE_H
#define SI_PARSENODE_H

#include "lstring.h"
#include "si_variable.h"


// These definitions apply to the script parser and control structures.

// Variables used in this module.
#define VA_LogIsLog10   "LogIsLog10"

struct stringlist;
struct SymTab;
class MacroHandler;
struct CDcbin;
struct sExpr;
struct CDs;
struct Zlist;
struct sLspec;
struct CDl;
struct CDo;
struct CDp;
struct sBlk;
struct SIifel;
struct Variable;
struct ParseNode;
struct SIfunc;

// Netlist formatting
#define EX_NUM_FORMATS 3
#define EX_PNET_FORMAT 0
#define EX_ENET_FORMAT 1
#define EX_LVS_FORMAT  2


//-----------------------------------------------------------------------------
// User Menu definitions

// Script files/libraries must have these suffixes.
#define SCR_SUFFIX  ".scr"
#define LIB_SUFFIX  ".scm"

// Prefix to distinguish lib path in scOpen()
#define SCR_LIBCODE "@@/"

// Struct to hold user-given range values for arrays and strings,
// used in debugger support functions.
//
struct rvals
{
    rvals() { rmin = rmax = -1; d1 = d2 = 0; minus = false; }
    void parse_range(const char*);

    int rmin, rmax; // range min/max
    int d1;         // dimension 1
    int d2;         // dimension 2
    bool minus;     // true is '-max' found
};


//-----------------------------------------------------------------------------
// ParseNode definition

// Polarity designator (for geometric returns)
//
enum PolarityType { PolarityClear, PolarityDark };

// These are the token types the lexer returns.  Order is important,
// as it must match the static array which dispatches the operators,
// and the precedence table.
//
enum
{
    TOK_END    =    0,
    TOK_PLUS   =    1,
    TOK_MINUS  =    2,
    TOK_TIMES  =    3,
    TOK_MOD    =    4,
    TOK_DIVIDE =    5,
    TOK_POWER  =    6,
    TOK_EQ     =    7,
    TOK_GT     =    8,
    TOK_LT     =    9,
    TOK_GE     =    10,
    TOK_LE     =    11,
    TOK_NE     =    12,
    TOK_AND    =    13,
    TOK_OR     =    14,
    TOK_COMMA  =    15,
    TOK_COND   =    16,
    TOK_COLON  =    17,
    TOK_ASSIGN =    18,
    TOK_UMINUS =    19,
    TOK_NOT    =    20,
    TOK_LINCR  =    21,
    TOK_LDECR  =    22,
    TOK_RINCR  =    23,
    TOK_RDECR  =    24,
    TOK_VALUE  =    25,
    TOK_LPAREN =    26,
    TOK_RPAREN =    27,
    TOK_LBRAC  =    28,
    TOK_RBRAC  =    29
};
typedef unsigned char PTokenType;

#define IS_BOP(x) ((x) > TOK_END && (x) <= TOK_ASSIGN)
#define IS_UOP(x) ((x) > TOK_ASSIGN && (x) <= TOK_RDECR)

// for ParseNode allocator (ParseNode::allocate_pnode).
//
enum PallocMode { P_ALLOC, P_CLEAR, P_RESET };

struct SIlexprCx;

struct PTconstant
{
    const char *name;
    double value;
};

struct ParseNode
{
    ParseNode()
        {
            left = 0;
            right = 0;
            next = 0;
            type = PT_BOGUS;
            optype = TOK_END;
            lexpr_string = false;
            evfunc = 0;
            memset(&data, 0, sizeof(data));
        }

    // si_parsenode.cc
    void print(FILE*);
    void string(sLstr&, bool = false);
    siVariable *getVar();
    bool istrue(void*);
    bool check();

    static void destroy(const ParseNode*);

    // allocator
    static ParseNode *allocate_pnode(PallocMode);
    static int allocated()  { return (pn_nodes); }

    // These are exclusive to geometric trees.
    XIrt evalTree(SIlexprCx*, Zlist**, PolarityType);
    bool checkTree();
    bool checkExpandTree(ParseNode**);
    CDll *findLayersInTree();
    bool isLayerInTree(const CDl*);
    char *checkLayersInTree();
    char *checkCellsInTree(BBox*);
    void getBloat(int*);

    ParseNode *left;        // Left operand, or single operand.
    ParseNode *right;       // Right operand, if there is one.
    ParseNode *next;        // Function args.
    union {
        siVariable *v;          // Variable.
        PTconstant constant;    // If constant.
        struct {
            const char *funcname;   // Function or variable name.
            bool (*function)(Variable*, Variable*, void*);
            SIfunc *userfunc;       // User-defined function.
            int numargs;            // Argument count.
        } f;
    } data;
    PNodeType type;         // Type of node.
    PTokenType optype;      // If operator node, the type.
    bool lexpr_string;      // In layer expressions, interpret string
                            // as a string, not a layer name.
    bool (*evfunc)(ParseNode*, siVariable*, void*);

private:
    // Allocation stuff.
    static ParseNode **pn_pnodes;
    static int pn_nodes;
    static int pn_size;
};

#endif

