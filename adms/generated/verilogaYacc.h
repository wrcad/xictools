/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_VERILOGA_Y_TAB_H_INCLUDED
# define YY_VERILOGA_Y_TAB_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int verilogadebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     PREC_IF_THEN = 258,
     tk_else = 259,
     tk_from = 260,
     tk_branch = 261,
     tk_number = 262,
     tk_nature = 263,
     tk_aliasparameter = 264,
     tk_output = 265,
     tk_anystring = 266,
     tk_dollar_ident = 267,
     tk_or = 268,
     tk_aliasparam = 269,
     tk_if = 270,
     tk_analog = 271,
     tk_parameter = 272,
     tk_discipline = 273,
     tk_char = 274,
     tk_anytext = 275,
     tk_for = 276,
     tk_while = 277,
     tk_real = 278,
     tk_op_shr = 279,
     tk_case = 280,
     tk_potential = 281,
     tk_endcase = 282,
     tk_disc_id = 283,
     tk_inf = 284,
     tk_exclude = 285,
     tk_ground = 286,
     tk_endmodule = 287,
     tk_begin = 288,
     tk_enddiscipline = 289,
     tk_domain = 290,
     tk_ident = 291,
     tk_op_shl = 292,
     tk_string = 293,
     tk_integer = 294,
     tk_module = 295,
     tk_endattribute = 296,
     tk_end = 297,
     tk_inout = 298,
     tk_and = 299,
     tk_bitwise_equr = 300,
     tk_default = 301,
     tk_function = 302,
     tk_input = 303,
     tk_beginattribute = 304,
     tk_endnature = 305,
     tk_endfunction = 306,
     tk_flow = 307
   };
#endif
/* Tokens.  */
#define PREC_IF_THEN 258
#define tk_else 259
#define tk_from 260
#define tk_branch 261
#define tk_number 262
#define tk_nature 263
#define tk_aliasparameter 264
#define tk_output 265
#define tk_anystring 266
#define tk_dollar_ident 267
#define tk_or 268
#define tk_aliasparam 269
#define tk_if 270
#define tk_analog 271
#define tk_parameter 272
#define tk_discipline 273
#define tk_char 274
#define tk_anytext 275
#define tk_for 276
#define tk_while 277
#define tk_real 278
#define tk_op_shr 279
#define tk_case 280
#define tk_potential 281
#define tk_endcase 282
#define tk_disc_id 283
#define tk_inf 284
#define tk_exclude 285
#define tk_ground 286
#define tk_endmodule 287
#define tk_begin 288
#define tk_enddiscipline 289
#define tk_domain 290
#define tk_ident 291
#define tk_op_shl 292
#define tk_string 293
#define tk_integer 294
#define tk_module 295
#define tk_endattribute 296
#define tk_end 297
#define tk_inout 298
#define tk_and 299
#define tk_bitwise_equr 300
#define tk_default 301
#define tk_function 302
#define tk_input 303
#define tk_beginattribute 304
#define tk_endnature 305
#define tk_endfunction 306
#define tk_flow 307



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 126 "verilogaYacc.y"

  p_lexval _lexval;
  p_yaccval _yaccval;


/* Line 2058 of yacc.c  */
#line 167 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE verilogalval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int verilogaparse (void *YYPARSE_PARAM);
#else
int verilogaparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int verilogaparse (void);
#else
int verilogaparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_VERILOGA_Y_TAB_H_INCLUDED  */
