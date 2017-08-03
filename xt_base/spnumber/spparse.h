
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef EXPPARSE_H
#define EXPPARSE_H


//
// The core of the expression parsing system.
//

enum TokenType
{
    TT_END,
    TT_PLUS,
    TT_MINUS,
    TT_TIMES,
    TT_MOD,
    TT_DIVIDE,
    TT_POWER,
    TT_UMINUS,
    TT_LPAREN,
    TT_RPAREN,
    TT_COMMA,
    TT_COLON,
    TT_COND,
    TT_VALUE,
    TT_EQ,
    TT_GT,
    TT_LT,
    TT_GE,
    TT_LE,
    TT_NE,
    TT_AND,
    TT_OR,
    TT_NOT,
    TT_INDX,
    TT_RANGE,
    TT_PLACEHOLDER
};

enum DatumType
{
    DT_BOGUS,
    DT_NUM,
    DT_STRING,
    DT_PNODE,
    DT_USTRING
};


// The Parser class is designed to be used with derived versions of
// Element, and with user defined parse nodes.  To avoid using templates
// which produce redundant code, we pass an Element array of size STACKSIZE
// to the Parser constructor, which provides the elements needed internally.
// The Element produces the parse nodes. The user's parse nodes should be
// typedefed to "ParseNode" in the scope of this header.

struct sUnits;

// Parser elements
//
struct Element
{
    Element() { clear(); }

    virtual ~Element() { }

    void clear()
        {
            token = TT_END;
            type = DT_BOGUS;
            units = 0;
            vu.real = 0.0;
        }

    // The makeSnode and makeFnode functions are expected to use or free
    // the vu.string, and clear it.
    //
    virtual ParseNode *makeNode(void*) = 0;
    virtual ParseNode *makeBnode(ParseNode*, ParseNode*, void*) = 0;
    virtual ParseNode *makeFnode(ParseNode*, void*) = 0;
    virtual ParseNode *makeUnode(ParseNode*, void*) = 0;
    virtual ParseNode *makeSnode(void*) = 0;
    virtual ParseNode *makeNnode(void*) = 0;
    virtual char *userString(const char**, bool) = 0;

    TokenType token;
    DatumType type;
    sUnits *units;
    union {
        char *string;
        double real;
        ParseNode *node;
    } vu;
};

enum ErrorCode
{
    ER_OK,
    ER_OVERF,
    ER_SYNTAX,
    ER_BADNOD,
};

#define STACKSIZE 200

// Mode flags.
//
#define PRSR_UNITS      0x1
    // Use pass the element units field to the number parser.
#define PRSR_AMPHACK    0x2
    // In forms like @xxx[yyy], zzz.@xxx[yyy], delimit at '[', otherwise
    // always delimit at ']'.
#define PRSR_NODEHACK   0x4
    // Take <integer>: as a string prefix (for node names).  Otherwise,
    // delimit at ':'.
#define PRSR_USRSTR     0x8
    // Use callback to resolve strings.
#define PRSR_SOURCE     0x10
    // Parsing a V/I source function.

// Operator precedence codes.
enum PRECtype { PRECnone, PRECgt, PREClt, PRECeq, PRECerr };

class Parser
{
public:
    Parser(Element*, unsigned int);
    ~Parser();

    static bool parenTable(TokenType, TokenType, bool);

    void init(const char*, void*);
    ParseNode *parse();
    Element *lexer();
    const char *getErrMesg();

    const char *residue()   { return (prsr_sptr); }
    ErrorCode getError()    { return (prsr_errorCode); }

    static bool Debug;      // debugging mode when true

    static const char *default_specials() { return (def_specials); }

private:

    bool is_special(char c)
        {
            return (strchr(prsr_specials, c) ? true : false);
        }

    void finish();
    void handle_string();
    void clear_stack(int);

    const char *prsr_string;     // string being parsed
    void *prsr_arg;              // user arg to pass thru to parse nodes

    TokenType prsr_lastToken;    // last token type seen by lexer
    DatumType prsr_lastType;     // last value type seen by lexer
    bool prsr_bracFlag;          // true when processing inside brackets
    const char *prsr_specials;   // delimiting characters for lexer
    Element *prsr_el;            // current element for lexer
    Element *prsr_stack;         // parser's stack
    const char *prsr_sptr;       // character position in string
    ErrorCode prsr_errorCode;    // error code if problems
    unsigned int prsr_flags;     // options
    unsigned int prsr_trinest;   // depth in x ? y : z
    bool prsr_in_source;         // true when parsing args to source funcs

    static const char *def_specials;
                                 // default special characters
};

#endif

