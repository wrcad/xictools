/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_VERILOGA_Y_TAB_H_INCLUDED
# define YY_VERILOGA_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int verilogadebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    PREC_IF_THEN = 258,            /* PREC_IF_THEN  */
    tk_aliasparam = 259,           /* tk_aliasparam  */
    tk_aliasparameter = 260,       /* tk_aliasparameter  */
    tk_analog = 261,               /* tk_analog  */
    tk_and = 262,                  /* tk_and  */
    tk_anystring = 263,            /* tk_anystring  */
    tk_anytext = 264,              /* tk_anytext  */
    tk_begin = 265,                /* tk_begin  */
    tk_beginattribute = 266,       /* tk_beginattribute  */
    tk_bitwise_equr = 267,         /* tk_bitwise_equr  */
    tk_branch = 268,               /* tk_branch  */
    tk_case = 269,                 /* tk_case  */
    tk_char = 270,                 /* tk_char  */
    tk_default = 271,              /* tk_default  */
    tk_disc_id = 272,              /* tk_disc_id  */
    tk_discipline = 273,           /* tk_discipline  */
    tk_dollar_ident = 274,         /* tk_dollar_ident  */
    tk_domain = 275,               /* tk_domain  */
    tk_else = 276,                 /* tk_else  */
    tk_end = 277,                  /* tk_end  */
    tk_endattribute = 278,         /* tk_endattribute  */
    tk_endcase = 279,              /* tk_endcase  */
    tk_enddiscipline = 280,        /* tk_enddiscipline  */
    tk_endfunction = 281,          /* tk_endfunction  */
    tk_endmodule = 282,            /* tk_endmodule  */
    tk_endnature = 283,            /* tk_endnature  */
    tk_exclude = 284,              /* tk_exclude  */
    tk_flow = 285,                 /* tk_flow  */
    tk_for = 286,                  /* tk_for  */
    tk_from = 287,                 /* tk_from  */
    tk_function = 288,             /* tk_function  */
    tk_ground = 289,               /* tk_ground  */
    tk_ident = 290,                /* tk_ident  */
    tk_if = 291,                   /* tk_if  */
    tk_inf = 292,                  /* tk_inf  */
    tk_inout = 293,                /* tk_inout  */
    tk_input = 294,                /* tk_input  */
    tk_integer = 295,              /* tk_integer  */
    tk_module = 296,               /* tk_module  */
    tk_nature = 297,               /* tk_nature  */
    tk_number = 298,               /* tk_number  */
    tk_op_shl = 299,               /* tk_op_shl  */
    tk_op_shr = 300,               /* tk_op_shr  */
    tk_or = 301,                   /* tk_or  */
    tk_output = 302,               /* tk_output  */
    tk_parameter = 303,            /* tk_parameter  */
    tk_potential = 304,            /* tk_potential  */
    tk_real = 305,                 /* tk_real  */
    tk_string = 306,               /* tk_string  */
    tk_while = 307                 /* tk_while  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define PREC_IF_THEN 258
#define tk_aliasparam 259
#define tk_aliasparameter 260
#define tk_analog 261
#define tk_and 262
#define tk_anystring 263
#define tk_anytext 264
#define tk_begin 265
#define tk_beginattribute 266
#define tk_bitwise_equr 267
#define tk_branch 268
#define tk_case 269
#define tk_char 270
#define tk_default 271
#define tk_disc_id 272
#define tk_discipline 273
#define tk_dollar_ident 274
#define tk_domain 275
#define tk_else 276
#define tk_end 277
#define tk_endattribute 278
#define tk_endcase 279
#define tk_enddiscipline 280
#define tk_endfunction 281
#define tk_endmodule 282
#define tk_endnature 283
#define tk_exclude 284
#define tk_flow 285
#define tk_for 286
#define tk_from 287
#define tk_function 288
#define tk_ground 289
#define tk_ident 290
#define tk_if 291
#define tk_inf 292
#define tk_inout 293
#define tk_input 294
#define tk_integer 295
#define tk_module 296
#define tk_nature 297
#define tk_number 298
#define tk_op_shl 299
#define tk_op_shr 300
#define tk_or 301
#define tk_output 302
#define tk_parameter 303
#define tk_potential 304
#define tk_real 305
#define tk_string 306
#define tk_while 307

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 126 "verilogaYacc.y"

  p_lexval _lexval;
  p_yaccval _yaccval;

#line 176 "y.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE verilogalval;


int verilogaparse (void);


#endif /* !YY_VERILOGA_Y_TAB_H_INCLUDED  */
