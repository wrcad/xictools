
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
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
 * This software is available for non-commercial use under the terms of   *
 * the GNU General Public License as published by the Free Software       *
 * Foundation; either version 2 of the License, or (at your option) any   *
 * later version.                                                         *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program; if not, write to the Free Software            *
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              *
 *========================================================================*
 *                                                                        *
 * Verilog Support Files                                                  *
 *                                                                        *
 *========================================================================*
 $Id: verilog.l,v 1.9 2013/11/10 21:30:15 stevew Exp $
 *========================================================================*/

/*========================================================================*
 *  vl2mv: Verilog to BLIF-MV Translator Distribution
 *
 *  Copyright (c) 1992, 1993
 *        Regents of the University of California
 *  All rights reserved.
 *
 *  Use and copying of this software and preparation of derivative works
 *  based upon this software are permitted.  However, any distribution of
 *  this software or derivative works must include the above copyright 
 *  notice.
 *
 *  This software is made available AS IS, and neither the Electronics
 *  Research Laboratory or the Universify of California make any
 *  warranty about the software, its performance or its conformity to
 *  any specification.
 *
 * Lexical Scanner for Verilog
 *
 * Author: Szu-Tsung Cheng, stcheng@ic.berkeley.edu
 *
 * Date: September 18, 1992
 *========================================================================*/

%{
#include <stdio.h>
#include <unistd.h>
#include "vl_st.h"
#include "vl_list.h"
#include "vl_defs.h"
#include "vl_types.h"
#include "verilog_yacc.h"

extern bitexp_parse yybit;
extern char yyid[];

static int scan_table = 0;
static void skipcommentblock();
static void skipcommentline();
static void lex_panic(const char*);
static void memorize_macro(char*, table<char*>*);
static int find_macro(char*);
static int find_not_macro(char*);
static void expand_macro(char*, table<char*>*);
static void push_open_file(char*);
static void pop_close_file();
static void pushd(char*);
static void popd();

#ifdef FLEX_SCANNER
#ifndef YY_FLEX_LEX_COMPAT
#define YYLMAX YY_BUF_SIZE
#endif
#define MAX_INCLUDE_DEPTH 20
YY_BUFFER_STATE buffer_stack[MAX_INCLUDE_DEPTH];
int buffer_stack_ptr = 0;
#endif

#define YY_SKIP_YYWRAP
#define yywrap() (VP.file_stack->length() ? (pop_close_file(), 0) : 1)

// Have to export yytext to verilog.y, sometimes this is a pointer,
// sometimes a char*, sometimes an unsigned char*, this avoids having
// to know a-priori
//
char *yy_textbuf() { return ((char*)yytext); }

%}

%a 3000
%e 1700
%k 1000
%n 700
%o 4000
%p 5000

%Start Snormal Stable Sskip

Space       [\f\n\r\t\b ]
Alpha       [a-zA-Z]
AlphaU      [a-zA-Z_]
AlphaNum    [a-zA-Z0-9]
AlphaNumU   [a-zA-Z0-9_]
Digit       [0-9]
DigitU      [0-9_]
Number      {Digit}{DigitU}*
Decimal     ({Number})?'[dD][ ]*{Number}
Octal       ({Number})?'[oO][ ]*[0-7xXzZ?][0-7xXzZ?_]*
Hexdecimal  ({Number})?'[hH][ ]*[0-9a-fA-FxXzZ?][0-9a-fA-FxXzZ?_]*
Binary      ({Number})?'[bB][ ]*[01xXzZ?][01xXzZ?_]*

%%
    
        strcpy(VP.last_macro, VP.curr_macro);
        VP.curr_macro[0]='\0';
        if (VP.ifelseSP == 0 || VP.ifelse_stack[VP.ifelseSP-1]) {
            if (scan_table)
                BEGIN Stable; 
            else
                BEGIN Snormal;
        }

<Sskip,Snormal,Stable>{Space}+ { 
#ifdef FLEX_SCANNER
        for (int i = 0; i < (int)yyleng; i++)
            if (yytext[i] == '\n')
                yylineno++;
#endif
        continue; }

<Sskip,Snormal,Stable>`ifdef[^\n]*\n {
        if (find_macro(yytext))
            VP.ifelse_stack[VP.ifelseSP++] = 1;
        else {
            VP.ifelse_stack[VP.ifelseSP++] = 0;
            BEGIN Sskip;
        }
        if (VP.ifelseSP == MAXSTRLEN) 
            lex_panic("nested `if/`else too deep"); }

<Sskip,Snormal,Stable>`ifndef[^\n]*\n {
        if (find_not_macro(yytext))
            VP.ifelse_stack[VP.ifelseSP++] = 1;
        else {
            VP.ifelse_stack[VP.ifelseSP++] = 0;
            BEGIN Sskip;
        }
        if (VP.ifelseSP == MAXSTRLEN) 
            lex_panic("nested `if/`else too deep"); }

<Sskip,Snormal,Stable>`else {
        if (VP.ifelseSP > 0) {
            if (VP.ifelse_stack[VP.ifelseSP-1]) {
                VP.ifelse_stack[VP.ifelseSP-1] = 0;
                BEGIN Sskip;
            }
            else {
                VP.ifelse_stack[VP.ifelseSP-1] = 1;
                if (scan_table)
                    BEGIN Stable;
                else
                    BEGIN Snormal;
            }
        }
        else
            lex_panic("misplaced `else"); }

<Sskip,Snormal,Stable>`endif {
        if (VP.ifelseSP > 0) {
            if (!VP.ifelse_stack[--VP.ifelseSP]) {
                if (scan_table)
                    BEGIN Stable;
                else
                    BEGIN Snormal;
            }
        }
        else
            lex_panic("misplaced `endif"); }

<Sskip>[^\f\n\r\t\b ]+      { continue; }

<Snormal,Stable>"/*"        { skipcommentblock(); continue; }
<Snormal,Stable>"//"        { skipcommentline();  continue; }

<Snormal,Stable>`timescale[^\n]*\n {
        if (!VP.parse_timescale(yytext))
            VP.error(ERR_WARN, "syntax error in timescale: %s", yytext); }

<Snormal,Stable>`define[^\n]*\n { 
#ifdef FLEX_SCANNER
        yylineno++;
#endif
        memorize_macro(yytext, VP.macros); 
        continue; }

<Snormal,Stable>`include[^\n]*\n {
#ifdef FLEX_SCANNER
        yylineno++;
#endif
        yytext[strlen(yytext)-1] = '\0';
        char *cp = strstr(yytext, "`include");
        cp += strlen("`include");
        while (*cp==' ' || *cp=='\t' || *cp=='\r')
            cp++;
        push_open_file(cp); }

<Snormal>">="               { return YYGEQ; }
<Snormal>"=<"               { return YYLEQ; }
<Snormal>"&&"               { return YYLOGAND; }
<Snormal>"||"               { return YYLOGOR; }
<Snormal>"==="              { return YYCASEEQUALITY; }
<Snormal>"=="               { return YYLOGEQUALITY; }
<Snormal>"!=="              { return YYCASEINEQUALITY; }
<Snormal>"!="               { return YYLOGINEQUALITY; }
<Snormal>"^~"               { return YYLOGXNOR; }
<Snormal>"~^"               { return YYLOGXNOR; }
<Snormal>"~&"               { return YYLOGNAND; }
<Snormal>"~|"               { return YYLOGNOR; }
<Snormal>"<<"               { return YYLSHIFT; }
<Snormal>">>"               { return YYRSHIFT; }
<Snormal>"?:"               { return YYCONDITIONAL; }

<Snormal>\"[^"]*\"          { return YYSTRING; }

<Snormal>always             { return YYALWAYS; }
<Snormal>"*>"               { return YYALLPATH; }
<Snormal>and                { return YYAND; }
<Snormal>assign             { return YYASSIGN; }
<Snormal>begin              { return YYBEGIN; }
<Snormal>buf                { return YYBUF; }
<Snormal>bufif0             { return YYBUFIF0; }
<Snormal>bufif1             { return YYBUFIF1; }
<Snormal>case               { return YYCASE; }
<Snormal>casex              { return YYCASEX; }
<Snormal>casez              { return YYCASEZ; }
<Snormal>cmos               { return YYCMOS; }
<Snormal>deassign           { return YYDEASSIGN; }
<Snormal>default            { return YYDEFAULT; }
<Snormal>defparam           { return YYDEFPARAM; }
<Snormal>disable            { return YYDISABLE; }
<Snormal>else               { return YYELSE; }
<Snormal>end                { return YYEND; }
<Snormal>endcase            { return YYENDCASE; }
<Snormal>endfunction        { return YYENDFUNCTION; }
<Snormal>endmodule          { return YYENDMODULE; }
<Snormal>endprimitive       { return YYENDPRIMITIVE; }
<Snormal>endspecify         { return YYENDSPECIFY; }
<Stable>endtable            { scan_table = 0; return YYENDTABLE; }
<Snormal>endtask            { return YYENDTASK; }
<Snormal>enum               { return YYENUM; }
<Snormal>event              { return YYEVENT; }
<Snormal>for                { return YYFOR; }
<Snormal>force              { return YYFORCE; }
<Snormal>forever            { return YYFOREVER; }
<Snormal>fork               { return YYFORK; }
<Snormal>function           { return YYFUNCTION; }
<Snormal>highz0             { return YYHIGHZ0; }
<Snormal>highz1             { return YYHIGHZ1; }
<Snormal>if                 { return YYIF; }
<Snormal>initial            { return YYINITIAL; }
<Snormal>inout              { return YYINOUT; }
<Snormal>input              { return YYINPUT; }
<Snormal>integer            { return YYINTEGER; }
<Snormal>join               { return YYJOIN; }
<Snormal>large              { return YYLARGE; }
<Snormal>"=>"               { return YYLEADTO; }
<Snormal>medium             { return YYMEDIUM; }
<Snormal>module             { return YYMODULE; }
<Snormal>"<="               { return YYNBASSIGN;  }
<Snormal>nand               { return YYNAND; }
<Snormal>negedge            { return YYNEGEDGE; }
<Snormal>nmos               { return YYNMOS; }
<Snormal>nor                { return YYNOR; }
<Snormal>not                { return YYNOT; }
<Snormal>notif0             { return YYNOTIF0; }
<Snormal>notif1             { return YYNOTIF1; }
<Snormal>or                 { return YYOR; }
<Snormal>output             { return YYOUTPUT; }
<Snormal>parameter          { return YYPARAMETER; }
<Snormal>pmos               { return YYPMOS; }
<Snormal>posedge            { return YYPOSEDGE; }
<Snormal>primitive          { return YYPRIMITIVE; }
<Snormal>pull0              { return YYPULL0; }
<Snormal>pull1              { return YYPULL1; }
<Snormal>pulldown           { return YYPULLDOWN; }
<Snormal>pullup             { return YYPULLUP; }
<Snormal>rcmos              { return YYRCMOS; }
<Snormal>real               { return YYREAL; }
<Snormal>reg                { return YYREG; }
<Snormal>release            { return YYRELEASE; }
<Snormal>repeat             { return YYREPEAT; }
<Snormal>"->"               { return YYRIGHTARROW; }
<Snormal>rnmos              { return YYRNMOS; }
<Snormal>rpmos              { return YYRPMOS; }
<Snormal>rtran              { return YYRTRAN; }
<Snormal>rtranif0           { return YYRTRANIF0; }
<Snormal>rtranif1           { return YYRTRANIF1; }
<Snormal>scalered           { return YYSCALARED; }
<Snormal>small              { return YYSMALL; }
<Snormal>specify            { return YYSPECIFY; }
<Snormal>specparam          { return YYSPECPARAM; }
<Snormal>strong0            { return YYSTRONG0; }
<Snormal>strong1            { return YYSTRONG1; }
<Snormal>supply0            { return YYSUPPLY0; }
<Snormal>supply1            { return YYSUPPLY1; }
<Snormal>table              { scan_table = 1; return YYTABLE; }
<Snormal>task               { return YYTASK; }
<Snormal>time               { return YYTIME; }
<Snormal>tran               { return YYTRAN; }
<Snormal>tranif0            { return YYTRANIF0; }
<Snormal>tranif1            { return YYTRANIF1; }
<Snormal>tri                { return YYTRI; }
<Snormal>tri0               { return YYTRI0; }
<Snormal>tri1               { return YYTRI1; }
<Snormal>triand             { return YYTRIAND; }
<Snormal>trior              { return YYTRIOR; }
<Snormal>trireg             { return YYTRIREG; }
<Snormal>vectored           { return YYVECTORED; }
<Snormal>wait               { return YYWAIT; }
<Snormal>wand               { return YYWAND; }
<Snormal>weak0              { return YYWEAK0; }
<Snormal>weak1              { return YYWEAK1; }
<Snormal>while              { return YYWHILE; }
<Snormal>wire               { return YYWIRE; }
<Snormal>wor                { return YYWOR; }
<Snormal>xnor               { return YYXNOR; }
<Snormal>xor                { return YYXOR; }

<Snormal>\$setup            { return YYsysSETUP; }

<Snormal>\${AlphaU}{AlphaNumU}* { strcpy(yyid, yytext); return YYsysID; }

<Snormal>`{AlphaU}{AlphaNumU}* {
        strcpy(yyid, yytext); 
        char *macro;
        if (VP.macros->lookup(yyid+1, &macro)) {
            expand_macro(yyid+1, VP.macros);
            continue;
        }
        return YYNAME; }

<Snormal>{AlphaU}{AlphaNumU}* { strcpy(yyid, yytext); return YYNAME; }

<Snormal>\\[^\n\t\b\r\f ]*  { strcpy(yyid, yytext); return YYNAME; }

<Snormal>{Digit}*\.{Digit}+ { return YYRNUMBER; }
<Snormal>{Digit}+\.{Digit}* { return YYRNUMBER; }
<Snormal>{Digit}+[eE]{Digit}+ { return YYRNUMBER; }
<Snormal>{Digit}*\.{Digit}+[eE]{Digit}+ { return YYRNUMBER; }
<Snormal>{Digit}+\.{Digit}*[eE]{Digit}+ { return YYRNUMBER; }

<Snormal>{Number}           { return YYINUMBER; }
<Snormal>{Binary}           { yybit.bin(yytext); return YYINUMBER; }
<Snormal>{Octal}            { yybit.oct(yytext); return YYINUMBER; }
<Snormal>{Decimal}          { yybit.dec(yytext); return YYINUMBER; }
<Snormal>{Hexdecimal}       { yybit.hex(yytext); return YYINUMBER; }

<Stable>\(\?\?\)            { return '*'; }
<Stable>\(01\)              { return 'r'; }
<Stable>\(10\)              { return 'f'; }
<Snormal,Stable>.           { return yytext[0]; }

%%    

static void
skipcommentblock()
{
    int level = 0;
    for (bool done = false; !done;
            yyleng = (yyleng > YYLMAX-2) ? YYLMAX-2 : yyleng) {
        int c;
#ifdef __cplusplus
        if ((c = yyinput()) == '*') {
#else
        if ((c = input()) == '*') {
#endif
            yytext[yyleng++] = c;
#ifdef __cplusplus
            if ((c = yyinput()) == '/') {
#else
            if ((c = input()) == '/') {
#endif
                done = (level-- == 0); 
            }
            else
                unput(c);
        }
        else if (c == '/') {
            yytext[yyleng++] = c;
#ifdef __cplusplus
            if ((c = yyinput()) == '*') {
#else
            if ((c = input()) == '*') {
#endif
                level++;
            }
            else
                unput(c);
        }
        else if (c == 0) {
            vl_error("incomplete comment (line %d)", yylineno);
            longjmp(VP.jbuf, 1);
        }
        else if (c == EOF) {
            vl_error("end of file in comment (line %d)", yylineno);
            longjmp(VP.jbuf, 1);
        }
        else {
            if (c == '\n')
                yylineno++;
            yytext[yyleng++] = c;
        }
    }
}


static void
skipcommentline()
{
    for (bool done = false; !done;
            yyleng = (yyleng > YYLMAX-2) ? YYLMAX-2 : yyleng) {
        char c;
#ifdef __cplusplus
        if ((c = yyinput()) == '\n') {
#else
        if ((c = input()) == '\n') {
#endif
            done = true;
            yylineno++;
        }
        else
            yytext[yyleng++] = c;
    }
    yytext[yyleng] = '\0';
}


static void
lex_panic(const char *mesg)
{
    vl_error(mesg);
    longjmp(VP.jbuf, 1);
}


/*
 * memorize_macro
 *
 *     insert a argumentless macro into macro table
 */
static void
memorize_macro(char *instr, table<char*> *macros)
{
    char mname[MAXSTRLEN];
    char *cp = strstr(instr, MACRO_DEFINE) + strlen(MACRO_DEFINE);
    sscanf(cp, "%s", mname);
    cp = strstr(cp, mname) + strlen(mname);
    char *cp1 = strstr(cp, "//");
    char *cp2 = strstr(cp, "/*");
    if (cp1 || cp2) {
        if (cp1)
            *cp1 = '\0';
        if (cp2)
            *cp2 = '\0';
    }
    macros->insert(vl_strdup(mname), vl_strdup(cp));
}


static int
find_macro(char *str)
{
    char mname[MAXSTRLEN];
    char *cp = strstr(str, MACRO_IFDEF) + strlen(MACRO_IFDEF);
    sscanf(cp, "%s", mname);
    char *dummy;
    return (VP.macros->lookup(mname, &dummy));
}


static int
find_not_macro(char *str)
{
    char mname[MAXSTRLEN];
    char *cp = strstr(str,MACRO_IFNDEF) + strlen(MACRO_IFNDEF);
    sscanf(cp, "%s", mname);
    char *dummy;
    return (!VP.macros->lookup(mname, &dummy));
}


/*
 * expand_macro
 *
 *     expand an argumentless macro into input stream (using unput)
 */
static void
expand_macro(char *mname, table<char*> *macros)
{
    char *mbody;
    if (macros->lookup(mname, &mbody)) {
        for (int i = strlen(mbody)-1; i >= 0; i--)
            unput(mbody[i]);
        strcpy(VP.curr_macro, mname);
    }
}


// Return True if the string represents an absolute path
//
static bool
is_rooted(const char *string)
{
    if (!string)
        return (false);
    if (*string == '/')
        return (true);
#ifdef WIN32
    if (*string == '\\')
        return (true);
    if (strlen(string) >= 3 && isalpha(string[0]) && string[1] == ':' &&
            (string[2] == '/' || string[2] == '\\'))
        return (true);
#endif
    return (false);
}


// Return True if c is a directory separator character
//
static bool
is_dirsep(char c)
{
    if (c == '/')
        return (true);
#ifdef WIN32
    if (c == '\\')
        return (true);
#endif
    return (false);
}


// Return directory separation character
//
static char
dirsep()
{
#ifdef WIN32
    return ('\\');
#else
    return ('/');
#endif
}


static void
push_open_file(char *fname)
{
    char *filename = vl_strdup(fname);
    if (*filename == '"') {
        int i;
        filename[strlen(filename)-1] = '\0';
        for(i = 0; i<(int)strlen(filename); i++) filename[i] = filename[i+1];
    }
    // Look for the file relative to the CWD, and if not found look
    // in the directory of the last opened file
    FILE *infile = fopen(filename, "r");
    if (infile)
        pushd(filename);
    else {
        char *cwd;
        if (VP.dir_stack->length() > 0) {
            VP.dir_stack->top(&cwd);
            char *s = new char[strlen(cwd) + strlen(filename) + 2];
            strcpy(s, cwd);
            char *t = s + strlen(s) - 1;
            *t++ = dirsep();
            strcpy(t, filename);
            infile = fopen(s, "r");
            if (infile)
                pushd(s);
            delete [] s;
        }
    }
    if (!infile) {
        vl_error("failed to open file '%s'", filename);
        longjmp(VP.jbuf, 1);
    }
    VP.file_stack->push(yyin);
    VP.fname_stack->push(VP.filename);
    VP.lineno_stack->push(yylineno);
    
#ifdef FLEX_SCANNER
    buffer_stack[buffer_stack_ptr++] = YY_CURRENT_BUFFER;
    yy_switch_to_buffer(yy_create_buffer(infile, YY_BUF_SIZE));
#else
    yyin = infile;
#endif
    yylineno = 1;
    VP.filename = filename;
}


static void
pop_close_file()
{
    fclose(yyin);
    FILE *infile;
    VP.file_stack->pop(&infile);
    char *filename;
    VP.fname_stack->pop(&filename);
    int old_yylineno;
    VP.lineno_stack->pop(&old_yylineno);
    popd();
#ifdef FLEX_SCANNER
    yy_delete_buffer(YY_CURRENT_BUFFER);
    yy_switch_to_buffer(buffer_stack[--buffer_stack_ptr]);
#else
    yyin = infile;
#endif
    yylineno = old_yylineno;
    delete [] VP.filename;
    VP.filename = filename;
}


static void
pushd(char *filename)
{
    if (is_rooted(filename)) {
        char *cwd = vl_strdup(filename);
        char *s = cwd + strlen(cwd) - 1;
        while (!is_dirsep(*s) && s >= cwd)
            s--;
        *s = 0;
        if (!is_rooted(cwd)) {
            *s++ = dirsep();
            *s = 0;
        }
        VP.dir_stack->push(cwd);
    }
    else {
        char *pwd = getcwd(0, 0);
        if (pwd) {
            char *cwd = new char[strlen(pwd) + strlen(filename) + 2];
            strcpy(cwd, pwd);
            delete [] pwd;
            char *s = cwd + strlen(cwd) - 1;
            *s++ = dirsep();
            strcpy(s, filename);
            s = cwd + strlen(cwd) - 1;
            while (!is_dirsep(*s) && s >= cwd)
                s--;
            *s = 0;
            if (!is_rooted(cwd)) {
                *s++ = dirsep();
                *s = 0;
            }
            VP.dir_stack->push(cwd);
        }
        else
            VP.dir_stack->push(vl_strdup("."));
    }
}


static void
popd()
{
    char *cwd;
    VP.dir_stack->pop(&cwd);
    delete [] cwd;
}
