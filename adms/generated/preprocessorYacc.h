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

#ifndef YY_PREPROCESSOR_Y_TAB_H_INCLUDED
# define YY_PREPROCESSOR_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int preprocessordebug;
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
    TK_PRAGMA_NAME = 258,          /* TK_PRAGMA_NAME  */
    TK_IDENT = 259,                /* TK_IDENT  */
    TK_STRING = 260,               /* TK_STRING  */
    TK_NOT_IDENT = 261,            /* TK_NOT_IDENT  */
    TK_ARG = 262,                  /* TK_ARG  */
    TK_ARG_NULL = 263,             /* TK_ARG_NULL  */
    TK_SUBSTITUTOR_NOARG = 264,    /* TK_SUBSTITUTOR_NOARG  */
    TK_SUBSTITUTOR_NULLARG = 265,  /* TK_SUBSTITUTOR_NULLARG  */
    TK_SUBSTITUTOR_NULLARG_ALONE = 266, /* TK_SUBSTITUTOR_NULLARG_ALONE  */
    TK_SUBSTITUTOR_WITHARG = 267,  /* TK_SUBSTITUTOR_WITHARG  */
    TK_SUBSTITUTOR_WITHARG_ALONE = 268, /* TK_SUBSTITUTOR_WITHARG_ALONE  */
    TK_CONTINUATOR = 269,          /* TK_CONTINUATOR  */
    TK_NOPRAGMA_CONTINUATOR = 270, /* TK_NOPRAGMA_CONTINUATOR  */
    TK_EOL = 271,                  /* TK_EOL  */
    TK_EOF = 272,                  /* TK_EOF  */
    TK_COMMENT = 273,              /* TK_COMMENT  */
    TK_INCLUDE = 274,              /* TK_INCLUDE  */
    TK_SPACE = 275,                /* TK_SPACE  */
    TK_ERROR_PRAGMA_DEFINITION = 276, /* TK_ERROR_PRAGMA_DEFINITION  */
    TK_ERROR_PRAGMA_NOT_FOUND = 277, /* TK_ERROR_PRAGMA_NOT_FOUND  */
    TK_ERROR_UNEXPECTED_CHAR = 278, /* TK_ERROR_UNEXPECTED_CHAR  */
    TK_ERROR_FILE_OPEN = 279,      /* TK_ERROR_FILE_OPEN  */
    TK_DEFINE = 280,               /* TK_DEFINE  */
    TK_DEFINE_END = 281,           /* TK_DEFINE_END  */
    TK_UNDEF = 282,                /* TK_UNDEF  */
    TK_IFDEF = 283,                /* TK_IFDEF  */
    TK_IFNDEF = 284,               /* TK_IFNDEF  */
    TK_ELSE = 285,                 /* TK_ELSE  */
    TK_ENDIF = 286                 /* TK_ENDIF  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define TK_PRAGMA_NAME 258
#define TK_IDENT 259
#define TK_STRING 260
#define TK_NOT_IDENT 261
#define TK_ARG 262
#define TK_ARG_NULL 263
#define TK_SUBSTITUTOR_NOARG 264
#define TK_SUBSTITUTOR_NULLARG 265
#define TK_SUBSTITUTOR_NULLARG_ALONE 266
#define TK_SUBSTITUTOR_WITHARG 267
#define TK_SUBSTITUTOR_WITHARG_ALONE 268
#define TK_CONTINUATOR 269
#define TK_NOPRAGMA_CONTINUATOR 270
#define TK_EOL 271
#define TK_EOF 272
#define TK_COMMENT 273
#define TK_INCLUDE 274
#define TK_SPACE 275
#define TK_ERROR_PRAGMA_DEFINITION 276
#define TK_ERROR_PRAGMA_NOT_FOUND 277
#define TK_ERROR_UNEXPECTED_CHAR 278
#define TK_ERROR_FILE_OPEN 279
#define TK_DEFINE 280
#define TK_DEFINE_END 281
#define TK_UNDEF 282
#define TK_IFDEF 283
#define TK_IFNDEF 284
#define TK_ELSE 285
#define TK_ENDIF 286

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 44 "preprocessorYacc.y"

  p_slist slist;
  char* mystr;

#line 134 "y.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE preprocessorlval;


int preprocessorparse (void);


#endif /* !YY_PREPROCESSOR_Y_TAB_H_INCLUDED  */
