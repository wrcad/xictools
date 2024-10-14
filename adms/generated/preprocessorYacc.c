/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         preprocessorparse
#define yylex           preprocessorlex
#define yyerror         preprocessorerror
#define yydebug         preprocessordebug
#define yynerrs         preprocessornerrs
#define yylval          preprocessorlval
#define yychar          preprocessorchar

/* First part of user prologue.  */
#line 28 "preprocessorYacc.y"


#include "admsPreprocessor.h"

#define YYDEBUG 1
#define KS(s) adms_k2strconcat(&message,s);
#define KI(i) adms_k2strconcat(&message,adms_integertostring(i));
#define K0 KS("[") KS(pproot()->cr_scanner->filename) KS(":") \
  KI(adms_preprocessor_get_line_position(pproot()->cr_scanner,0)) KS("]: ")
#define DONT_SKIPP (pproot()->skipp_text->data==INT2ADMS(0))

p_slist continuatorList=NULL;
p_slist condistrue=NULL;


#line 94 "y.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
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

#line 214 "y.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE preprocessorlval;


int preprocessorparse (void);


#endif /* !YY_PREPROCESSOR_Y_TAB_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_TK_PRAGMA_NAME = 3,             /* TK_PRAGMA_NAME  */
  YYSYMBOL_TK_IDENT = 4,                   /* TK_IDENT  */
  YYSYMBOL_TK_STRING = 5,                  /* TK_STRING  */
  YYSYMBOL_TK_NOT_IDENT = 6,               /* TK_NOT_IDENT  */
  YYSYMBOL_TK_ARG = 7,                     /* TK_ARG  */
  YYSYMBOL_TK_ARG_NULL = 8,                /* TK_ARG_NULL  */
  YYSYMBOL_TK_SUBSTITUTOR_NOARG = 9,       /* TK_SUBSTITUTOR_NOARG  */
  YYSYMBOL_TK_SUBSTITUTOR_NULLARG = 10,    /* TK_SUBSTITUTOR_NULLARG  */
  YYSYMBOL_TK_SUBSTITUTOR_NULLARG_ALONE = 11, /* TK_SUBSTITUTOR_NULLARG_ALONE  */
  YYSYMBOL_TK_SUBSTITUTOR_WITHARG = 12,    /* TK_SUBSTITUTOR_WITHARG  */
  YYSYMBOL_TK_SUBSTITUTOR_WITHARG_ALONE = 13, /* TK_SUBSTITUTOR_WITHARG_ALONE  */
  YYSYMBOL_TK_CONTINUATOR = 14,            /* TK_CONTINUATOR  */
  YYSYMBOL_TK_NOPRAGMA_CONTINUATOR = 15,   /* TK_NOPRAGMA_CONTINUATOR  */
  YYSYMBOL_TK_EOL = 16,                    /* TK_EOL  */
  YYSYMBOL_TK_EOF = 17,                    /* TK_EOF  */
  YYSYMBOL_TK_COMMENT = 18,                /* TK_COMMENT  */
  YYSYMBOL_TK_INCLUDE = 19,                /* TK_INCLUDE  */
  YYSYMBOL_TK_SPACE = 20,                  /* TK_SPACE  */
  YYSYMBOL_TK_ERROR_PRAGMA_DEFINITION = 21, /* TK_ERROR_PRAGMA_DEFINITION  */
  YYSYMBOL_TK_ERROR_PRAGMA_NOT_FOUND = 22, /* TK_ERROR_PRAGMA_NOT_FOUND  */
  YYSYMBOL_TK_ERROR_UNEXPECTED_CHAR = 23,  /* TK_ERROR_UNEXPECTED_CHAR  */
  YYSYMBOL_TK_ERROR_FILE_OPEN = 24,        /* TK_ERROR_FILE_OPEN  */
  YYSYMBOL_TK_DEFINE = 25,                 /* TK_DEFINE  */
  YYSYMBOL_TK_DEFINE_END = 26,             /* TK_DEFINE_END  */
  YYSYMBOL_TK_UNDEF = 27,                  /* TK_UNDEF  */
  YYSYMBOL_TK_IFDEF = 28,                  /* TK_IFDEF  */
  YYSYMBOL_TK_IFNDEF = 29,                 /* TK_IFNDEF  */
  YYSYMBOL_TK_ELSE = 30,                   /* TK_ELSE  */
  YYSYMBOL_TK_ENDIF = 31,                  /* TK_ENDIF  */
  YYSYMBOL_32_ = 32,                       /* '('  */
  YYSYMBOL_33_ = 33,                       /* ')'  */
  YYSYMBOL_34_ = 34,                       /* ','  */
  YYSYMBOL_YYACCEPT = 35,                  /* $accept  */
  YYSYMBOL_R_description = 36,             /* R_description  */
  YYSYMBOL_R_list_of_conditional = 37,     /* R_list_of_conditional  */
  YYSYMBOL_R_conditional = 38,             /* R_conditional  */
  YYSYMBOL_R_if = 39,                      /* R_if  */
  YYSYMBOL_R_ifn = 40,                     /* R_ifn  */
  YYSYMBOL_R_ifdef = 41,                   /* R_ifdef  */
  YYSYMBOL_R_ifndef = 42,                  /* R_ifndef  */
  YYSYMBOL_R_else = 43,                    /* R_else  */
  YYSYMBOL_R_endif = 44,                   /* R_endif  */
  YYSYMBOL_R_include = 45,                 /* R_include  */
  YYSYMBOL_R_undef = 46,                   /* R_undef  */
  YYSYMBOL_R_alternative = 47,             /* R_alternative  */
  YYSYMBOL_R_pragma = 48,                  /* R_pragma  */
  YYSYMBOL_R_notpragma = 49,               /* R_notpragma  */
  YYSYMBOL_R_define_notpragma = 50,        /* R_define_notpragma  */
  YYSYMBOL_R_substitutor = 51,             /* R_substitutor  */
  YYSYMBOL_R_substitutor_nullarg = 52,     /* R_substitutor_nullarg  */
  YYSYMBOL_R_substitutor_witharg = 53,     /* R_substitutor_witharg  */
  YYSYMBOL_R_arg_null = 54,                /* R_arg_null  */
  YYSYMBOL_R_substitutor_list_of_arg = 55, /* R_substitutor_list_of_arg  */
  YYSYMBOL_R_list_of_arg = 56,             /* R_list_of_arg  */
  YYSYMBOL_R_list_of_arg_with_comma = 57,  /* R_list_of_arg_with_comma  */
  YYSYMBOL_R_arg = 58,                     /* R_arg  */
  YYSYMBOL_R_other = 59,                   /* R_other  */
  YYSYMBOL_R_define_alternative = 60,      /* R_define_alternative  */
  YYSYMBOL_R_define = 61,                  /* R_define  */
  YYSYMBOL_R_define_list_of_arg = 62,      /* R_define_list_of_arg  */
  YYSYMBOL_R_define_text = 63              /* R_define_text  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  46
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   432

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  35
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  29
/* YYNRULES -- Number of rules.  */
#define YYNRULES  83
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  113

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   286


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      32,    33,     2,     2,    34,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   122,   122,   129,   133,   141,   156,   168,   183,   195,
     202,   209,   216,   237,   259,   273,   281,   290,   297,   301,
     308,   312,   316,   321,   325,   334,   347,   351,   357,   367,
     371,   384,   388,   395,   405,   409,   422,   427,   432,   437,
     446,   456,   465,   510,   514,   521,   525,   532,   538,   542,
     550,   554,   562,   566,   575,   580,   587,   592,   600,   608,
     615,   620,   625,   630,   637,   642,   647,   652,   657,   662,
     667,   672,   677,   685,   691,   697,   703,   709,   715,   724,
     731,   735,   743,   747
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "TK_PRAGMA_NAME",
  "TK_IDENT", "TK_STRING", "TK_NOT_IDENT", "TK_ARG", "TK_ARG_NULL",
  "TK_SUBSTITUTOR_NOARG", "TK_SUBSTITUTOR_NULLARG",
  "TK_SUBSTITUTOR_NULLARG_ALONE", "TK_SUBSTITUTOR_WITHARG",
  "TK_SUBSTITUTOR_WITHARG_ALONE", "TK_CONTINUATOR",
  "TK_NOPRAGMA_CONTINUATOR", "TK_EOL", "TK_EOF", "TK_COMMENT",
  "TK_INCLUDE", "TK_SPACE", "TK_ERROR_PRAGMA_DEFINITION",
  "TK_ERROR_PRAGMA_NOT_FOUND", "TK_ERROR_UNEXPECTED_CHAR",
  "TK_ERROR_FILE_OPEN", "TK_DEFINE", "TK_DEFINE_END", "TK_UNDEF",
  "TK_IFDEF", "TK_IFNDEF", "TK_ELSE", "TK_ENDIF", "'('", "')'", "','",
  "$accept", "R_description", "R_list_of_conditional", "R_conditional",
  "R_if", "R_ifn", "R_ifdef", "R_ifndef", "R_else", "R_endif", "R_include",
  "R_undef", "R_alternative", "R_pragma", "R_notpragma",
  "R_define_notpragma", "R_substitutor", "R_substitutor_nullarg",
  "R_substitutor_witharg", "R_arg_null", "R_substitutor_list_of_arg",
  "R_list_of_arg", "R_list_of_arg_with_comma", "R_arg", "R_other",
  "R_define_alternative", "R_define", "R_define_list_of_arg",
  "R_define_text", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-77)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     133,   -77,   -77,   -77,   -77,   -14,   -77,     2,   -77,   -77,
     -77,   -77,   -77,   -77,   -77,   -77,   -77,   -77,   -77,    21,
      28,   -77,   -77,   -77,   -77,   -77,    32,   133,   -77,    31,
      34,   -77,   -77,   -77,   -77,   -77,   -77,     7,    13,   -77,
     -77,   164,   -77,   -77,   -77,   -77,   -77,   -77,   -77,   133,
     -77,   133,   350,   -77,   350,   -77,   -77,   226,   -77,   -77,
     -77,   -77,   -77,   -77,   -77,   195,   257,    71,    71,   -77,
     -77,   -77,   -77,   -77,   -77,   -77,   375,   -77,   -77,   -25,
     400,   -77,   -21,   -77,   288,   -77,   -77,   319,   -77,   -77,
     -77,   -77,   133,   -77,   133,   -77,   -77,   400,    -4,   -77,
     400,   -77,   -77,   -77,   -77,   102,   102,   -77,   400,   400,
     -77,   -77,   400
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    67,    69,    68,    36,    44,    37,    46,    40,    27,
      28,    72,    71,    16,    70,    25,    30,    23,    24,     0,
       0,    10,    11,    64,    65,    66,     0,     2,     3,     0,
       0,    20,    22,     9,    18,    19,    26,     0,     0,    29,
      21,     0,    43,    45,    79,    17,     1,     4,    12,     0,
      13,     0,     0,    38,     0,    41,    80,     0,    32,    33,
      35,    73,    82,    31,    34,     0,     0,     0,     0,    60,
      61,    62,    55,    57,    56,    54,     0,    47,    63,     0,
      48,    50,     0,    75,     0,    81,    77,     0,    74,    83,
      14,    15,     0,     6,     0,     8,    59,    52,     0,    39,
       0,    51,    42,    76,    78,     0,     0,    58,     0,    49,
       5,     7,    53
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -77,   -77,   -44,   -27,   -77,   -77,   -77,   -77,   -17,   -53,
     -77,   -77,   -77,   -77,   -77,   -64,   -38,   -77,   -77,    16,
       1,   -65,   -77,   -76,   -40,   -77,   -77,   -77,   -47
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    26,    27,    28,    29,    30,    49,    51,    92,    93,
      31,    32,    33,    34,    35,    62,    36,    37,    38,    53,
      79,    80,    98,    81,    39,    40,    41,    65,    66
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      47,    64,    89,    63,   101,    67,    42,    68,    99,   100,
      84,    97,   102,   100,    78,    95,    78,    64,    87,    63,
      89,   101,    43,    89,    44,    64,    64,    63,    63,   107,
     108,    45,    46,   101,    48,   109,   101,    50,    78,    52,
      47,    47,    78,   112,    64,    54,    63,    64,   105,    63,
     106,    94,   110,   111,    55,    82,     0,     0,     0,    78,
       0,     0,    78,     0,     0,     0,     0,     0,     0,     0,
      78,    78,     0,     0,    78,     1,     2,     3,    47,    47,
       4,     5,     6,     7,     8,     0,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,     0,    20,    21,
      22,    90,    91,    23,    24,    25,     1,     2,     3,     0,
       0,     4,     5,     6,     7,     8,     0,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,     0,    20,
      21,    22,     0,    91,    23,    24,    25,     1,     2,     3,
       0,     0,     4,     5,     6,     7,     8,     0,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,     0,
      20,    21,    22,     0,     0,    23,    24,    25,     1,     2,
       3,    56,    57,     4,     5,     6,     7,     8,    58,     0,
      59,    11,    12,     0,    14,     0,    60,     0,     0,     0,
      61,     0,     0,     0,     0,     0,    23,    24,    25,     1,
       2,     3,    85,     0,     4,     5,     6,     7,     8,    58,
       0,    59,    11,    12,     0,    14,     0,    60,     0,     0,
       0,    86,     0,     0,     0,     0,     0,    23,    24,    25,
       1,     2,     3,     0,     0,     4,     5,     6,     7,     8,
      58,     0,    59,    11,    12,     0,    14,     0,    60,     0,
       0,     0,    83,     0,     0,     0,     0,     0,    23,    24,
      25,     1,     2,     3,     0,     0,     4,     5,     6,     7,
       8,    58,     0,    59,    11,    12,     0,    14,     0,    60,
       0,     0,     0,    88,     0,     0,     0,     0,     0,    23,
      24,    25,     1,     2,     3,     0,     0,     4,     5,     6,
       7,     8,    58,     0,    59,    11,    12,     0,    14,     0,
      60,     0,     0,     0,   103,     0,     0,     0,     0,     0,
      23,    24,    25,     1,     2,     3,     0,     0,     4,     5,
       6,     7,     8,    58,     0,    59,    11,    12,     0,    14,
       0,    60,     0,     0,     0,   104,     0,     0,     0,     0,
       0,    23,    24,    25,    69,    70,    71,     0,     0,     4,
       5,     6,     7,     8,    72,     0,    73,     0,    74,     0,
      75,     0,     0,     0,     0,     0,     0,     0,     0,    69,
      70,    71,    76,    77,     4,     5,     6,     7,     8,    72,
       0,    73,     0,    74,     0,    75,     0,     0,     0,     0,
       0,     0,     0,     0,    69,    70,    71,    76,    96,     4,
       5,     6,     7,     8,    72,     0,    73,     0,    74,     0,
      75,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    76
};

static const yytype_int8 yycheck[] =
{
      27,    41,    66,    41,    80,    49,    20,    51,    33,    34,
      57,    76,    33,    34,    52,    68,    54,    57,    65,    57,
      84,    97,    20,    87,     3,    65,    66,    65,    66,    33,
      34,     3,     0,   109,     3,   100,   112,     3,    76,    32,
      67,    68,    80,   108,    84,    32,    84,    87,    92,    87,
      94,    68,   105,   106,    38,    54,    -1,    -1,    -1,    97,
      -1,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     108,   109,    -1,    -1,   112,     4,     5,     6,   105,   106,
       9,    10,    11,    12,    13,    -1,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    -1,    27,    28,
      29,    30,    31,    32,    33,    34,     4,     5,     6,    -1,
      -1,     9,    10,    11,    12,    13,    -1,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    -1,    27,
      28,    29,    -1,    31,    32,    33,    34,     4,     5,     6,
      -1,    -1,     9,    10,    11,    12,    13,    -1,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    -1,
      27,    28,    29,    -1,    -1,    32,    33,    34,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    -1,
      16,    17,    18,    -1,    20,    -1,    22,    -1,    -1,    -1,
      26,    -1,    -1,    -1,    -1,    -1,    32,    33,    34,     4,
       5,     6,     7,    -1,     9,    10,    11,    12,    13,    14,
      -1,    16,    17,    18,    -1,    20,    -1,    22,    -1,    -1,
      -1,    26,    -1,    -1,    -1,    -1,    -1,    32,    33,    34,
       4,     5,     6,    -1,    -1,     9,    10,    11,    12,    13,
      14,    -1,    16,    17,    18,    -1,    20,    -1,    22,    -1,
      -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    32,    33,
      34,     4,     5,     6,    -1,    -1,     9,    10,    11,    12,
      13,    14,    -1,    16,    17,    18,    -1,    20,    -1,    22,
      -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,     4,     5,     6,    -1,    -1,     9,    10,    11,
      12,    13,    14,    -1,    16,    17,    18,    -1,    20,    -1,
      22,    -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,
      32,    33,    34,     4,     5,     6,    -1,    -1,     9,    10,
      11,    12,    13,    14,    -1,    16,    17,    18,    -1,    20,
      -1,    22,    -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,
      -1,    32,    33,    34,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    13,    14,    -1,    16,    -1,    18,    -1,
      20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     4,
       5,     6,    32,    33,     9,    10,    11,    12,    13,    14,
      -1,    16,    -1,    18,    -1,    20,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     4,     5,     6,    32,    33,     9,
      10,    11,    12,    13,    14,    -1,    16,    -1,    18,    -1,
      20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    32
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     4,     5,     6,     9,    10,    11,    12,    13,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      27,    28,    29,    32,    33,    34,    36,    37,    38,    39,
      40,    45,    46,    47,    48,    49,    51,    52,    53,    59,
      60,    61,    20,    20,     3,     3,     0,    38,     3,    41,
       3,    42,    32,    54,    32,    54,     7,     8,    14,    16,
      22,    26,    50,    51,    59,    62,    63,    37,    37,     4,
       5,     6,    14,    16,    18,    20,    32,    33,    51,    55,
      56,    58,    55,    26,    63,     7,    26,    63,    26,    50,
      30,    31,    43,    44,    43,    44,    33,    56,    57,    33,
      34,    58,    33,    26,    26,    37,    37,    33,    34,    56,
      44,    44,    56
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    35,    36,    37,    37,    38,    38,    38,    38,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    47,
      48,    48,    48,    48,    48,    48,    49,    49,    49,    49,
      49,    50,    50,    50,    50,    50,    51,    51,    51,    51,
      51,    51,    51,    52,    52,    53,    53,    54,    55,    55,
      56,    56,    57,    57,    58,    58,    58,    58,    58,    58,
      58,    58,    58,    58,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    60,    60,    60,    60,    60,    60,    61,
      62,    62,    63,    63
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     2,     6,     4,     6,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     4,
       1,     2,     4,     2,     1,     2,     1,     2,     1,     3,
       1,     2,     1,     3,     1,     1,     1,     1,     3,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     3,     4,     3,     4,     2,
       1,     2,     1,     2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* R_description: R_list_of_conditional  */
#line 123 "preprocessorYacc.y"
          {
            pproot()->Text=(yyvsp[0].slist);
          }
#line 1410 "y.tab.c"
    break;

  case 3: /* R_list_of_conditional: R_conditional  */
#line 130 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1418 "y.tab.c"
    break;

  case 4: /* R_list_of_conditional: R_list_of_conditional R_conditional  */
#line 134 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
            adms_slist_concat(&((yyval.slist)),(yyvsp[-1].slist));
          }
#line 1427 "y.tab.c"
    break;

  case 5: /* R_conditional: R_if R_ifdef R_list_of_conditional R_else R_list_of_conditional R_endif  */
#line 142 "preprocessorYacc.y"
          {
            if(condistrue->data==INT2ADMS(1))
            {
              (yyval.slist)=(yyvsp[0].slist);
              adms_slist_concat(&((yyval.slist)),(yyvsp[-3].slist));
            }
            else if(condistrue->data==INT2ADMS(0))
            {
              (yyval.slist)=(yyvsp[-1].slist);
              adms_slist_concat(&((yyval.slist)),(yyvsp[-2].slist));
            }
            adms_slist_pull(&pproot()->skipp_text);
            adms_slist_pull(&condistrue);
          }
#line 1446 "y.tab.c"
    break;

  case 6: /* R_conditional: R_if R_ifdef R_list_of_conditional R_endif  */
#line 157 "preprocessorYacc.y"
          {
            if(condistrue->data==INT2ADMS(1))
            {
              (yyval.slist)=(yyvsp[-1].slist);
              adms_slist_concat(&((yyval.slist)),(yyvsp[-3].slist));
            }
            else if(condistrue->data==INT2ADMS(0))
              (yyval.slist)=(yyvsp[0].slist);
            adms_slist_pull(&pproot()->skipp_text);
            adms_slist_pull(&condistrue);
          }
#line 1462 "y.tab.c"
    break;

  case 7: /* R_conditional: R_ifn R_ifndef R_list_of_conditional R_else R_list_of_conditional R_endif  */
#line 169 "preprocessorYacc.y"
          {
            if(condistrue->data==INT2ADMS(1))
            {
               (yyval.slist)=(yyvsp[0].slist);
               adms_slist_concat(&((yyval.slist)),(yyvsp[-3].slist));
            }
            else if(condistrue->data==INT2ADMS(0))
            {
              (yyval.slist)=(yyvsp[-1].slist);
              adms_slist_concat(&((yyval.slist)),(yyvsp[-2].slist));
            }
            adms_slist_pull(&pproot()->skipp_text);
            adms_slist_pull(&condistrue);
          }
#line 1481 "y.tab.c"
    break;

  case 8: /* R_conditional: R_ifn R_ifndef R_list_of_conditional R_endif  */
#line 184 "preprocessorYacc.y"
          {
            if(condistrue->data==INT2ADMS(1))
            {
              (yyval.slist)=(yyvsp[-1].slist);
              adms_slist_concat(&((yyval.slist)),(yyvsp[-3].slist));
            }
            else if(condistrue->data==INT2ADMS(0))
              (yyval.slist)=(yyvsp[0].slist);
            adms_slist_pull(&pproot()->skipp_text);
            adms_slist_pull(&condistrue);
          }
#line 1497 "y.tab.c"
    break;

  case 9: /* R_conditional: R_alternative  */
#line 196 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1505 "y.tab.c"
    break;

  case 10: /* R_if: TK_IFDEF  */
#line 203 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 1514 "y.tab.c"
    break;

  case 11: /* R_ifn: TK_IFNDEF  */
#line 210 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 1523 "y.tab.c"
    break;

  case 12: /* R_ifdef: TK_PRAGMA_NAME  */
#line 217 "preprocessorYacc.y"
          {
            (yyval.mystr)=(yyvsp[0].mystr);
            if(!DONT_SKIPP)
            {
              adms_slist_push(&pproot()->skipp_text,INT2ADMS(1));
              adms_slist_push(&condistrue,INT2ADMS(-1));
            }
            else if(adms_preprocessor_identifier_is_def((yyvsp[0].mystr)))
            {
              adms_slist_push(&condistrue,INT2ADMS(1));
              adms_slist_push(&pproot()->skipp_text,INT2ADMS(0));
            }
            else
            {
              adms_slist_push(&condistrue,INT2ADMS(0));
              adms_slist_push(&pproot()->skipp_text,INT2ADMS(1));
            }
          }
#line 1546 "y.tab.c"
    break;

  case 13: /* R_ifndef: TK_PRAGMA_NAME  */
#line 238 "preprocessorYacc.y"
          {
            (yyval.mystr)=(yyvsp[0].mystr);
            if(!DONT_SKIPP)
            {
              adms_slist_push(&pproot()->skipp_text,INT2ADMS(1));
              adms_slist_push(&condistrue,INT2ADMS(-1));
            }
            else if(adms_preprocessor_identifier_is_ndef((yyvsp[0].mystr)))
            {
              adms_slist_push(&condistrue,INT2ADMS(1));
              adms_slist_push(&pproot()->skipp_text,INT2ADMS(0));
            }
            else
            {
              adms_slist_push(&condistrue,INT2ADMS(0));
              adms_slist_push(&pproot()->skipp_text,INT2ADMS(1));
            }
          }
#line 1569 "y.tab.c"
    break;

  case 14: /* R_else: TK_ELSE  */
#line 260 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
            if(condistrue->data==INT2ADMS(0))
              pproot()->skipp_text->data=INT2ADMS(0);
            else if(condistrue->data==INT2ADMS(1))
              pproot()->skipp_text->data=INT2ADMS(1);
            else
              pproot()->skipp_text->data=INT2ADMS(1);
          }
#line 1584 "y.tab.c"
    break;

  case 15: /* R_endif: TK_ENDIF  */
#line 274 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 1593 "y.tab.c"
    break;

  case 16: /* R_include: TK_INCLUDE  */
#line 282 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext;
            newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 1603 "y.tab.c"
    break;

  case 17: /* R_undef: TK_UNDEF TK_PRAGMA_NAME  */
#line 291 "preprocessorYacc.y"
          {
            (yyval.mystr)=(yyvsp[0].mystr);
          }
#line 1611 "y.tab.c"
    break;

  case 18: /* R_alternative: R_pragma  */
#line 298 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1619 "y.tab.c"
    break;

  case 19: /* R_alternative: R_notpragma  */
#line 302 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1627 "y.tab.c"
    break;

  case 20: /* R_pragma: R_include  */
#line 309 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1635 "y.tab.c"
    break;

  case 21: /* R_pragma: R_define_alternative  */
#line 313 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1643 "y.tab.c"
    break;

  case 22: /* R_pragma: R_undef  */
#line 317 "preprocessorYacc.y"
          {
            (yyval.slist)=NULL;
            if(DONT_SKIPP) adms_preprocessor_identifer_set_undef((yyvsp[0].mystr));
          }
#line 1652 "y.tab.c"
    break;

  case 23: /* R_pragma: TK_ERROR_UNEXPECTED_CHAR  */
#line 322 "preprocessorYacc.y"
          {
            (yyval.slist)=NULL;
          }
#line 1660 "y.tab.c"
    break;

  case 24: /* R_pragma: TK_ERROR_FILE_OPEN  */
#line 326 "preprocessorYacc.y"
          {
            char*message=NULL;
            (yyval.slist)=NULL;
            K0 KS(pproot()->cr_scanner->cur_message) KS("\n") 
            adms_preprocessor_add_message(message);
            free(pproot()->cr_scanner->cur_message);
            pproot()->cr_scanner->cur_message=NULL;
          }
#line 1673 "y.tab.c"
    break;

  case 25: /* R_pragma: TK_ERROR_PRAGMA_DEFINITION  */
#line 335 "preprocessorYacc.y"
          {
            char*message=NULL;
            (yyval.slist)=NULL;
            K0 KS("macro ") KS(pproot()->cr_scanner->cur_message) KS(" badly formed\n")
            adms_preprocessor_add_message(message);
            pproot()->error += 1;
            free(pproot()->cr_scanner->cur_message);
            pproot()->cr_scanner->cur_message=NULL;
          }
#line 1687 "y.tab.c"
    break;

  case 26: /* R_notpragma: R_substitutor  */
#line 348 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1695 "y.tab.c"
    break;

  case 27: /* R_notpragma: TK_NOPRAGMA_CONTINUATOR  */
#line 352 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string("\n");
            adms_slist_push(&continuatorList,(p_adms)newtext);
            (yyval.slist)=NULL;
          }
#line 1705 "y.tab.c"
    break;

  case 28: /* R_notpragma: TK_EOL  */
#line 358 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string("\n");
            (yyval.slist)=adms_slist_new((p_adms)newtext);
            adms_slist_concat(&((yyval.slist)),continuatorList);
            continuatorList=NULL;
            ++pproot()->cr_scanner->cur_line_position;
            pproot()->cr_scanner->cur_char_position=1;
            pproot()->cr_scanner->cur_continuator_position=NULL;
          }
#line 1719 "y.tab.c"
    break;

  case 29: /* R_notpragma: R_other  */
#line 368 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1727 "y.tab.c"
    break;

  case 30: /* R_notpragma: TK_ERROR_PRAGMA_NOT_FOUND  */
#line 372 "preprocessorYacc.y"
          {
            char*message=NULL;
            (yyval.slist)=NULL;
            K0 KS("macro ") KS(pproot()->cr_scanner->cur_message) KS(" is undefined\n")
            adms_preprocessor_add_message(message);
            pproot()->error += 1;
            free(pproot()->cr_scanner->cur_message);
            pproot()->cr_scanner->cur_message=NULL;
          }
#line 1741 "y.tab.c"
    break;

  case 31: /* R_define_notpragma: R_substitutor  */
#line 385 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1749 "y.tab.c"
    break;

  case 32: /* R_define_notpragma: TK_CONTINUATOR  */
#line 389 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext1=adms_preprocessor_new_text_as_string("\n");
            p_preprocessor_text newtext2=adms_preprocessor_new_text_as_string("");
            adms_slist_push(&continuatorList,(p_adms)newtext1);
            (yyval.slist)=adms_slist_new((p_adms)newtext2);
          }
#line 1760 "y.tab.c"
    break;

  case 33: /* R_define_notpragma: TK_EOL  */
#line 396 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
            adms_slist_concat(&((yyval.slist)),continuatorList);
            continuatorList=NULL;
            ++pproot()->cr_scanner->cur_line_position;
            pproot()->cr_scanner->cur_char_position=1;
            pproot()->cr_scanner->cur_continuator_position=NULL;
          }
#line 1774 "y.tab.c"
    break;

  case 34: /* R_define_notpragma: R_other  */
#line 406 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1782 "y.tab.c"
    break;

  case 35: /* R_define_notpragma: TK_ERROR_PRAGMA_NOT_FOUND  */
#line 410 "preprocessorYacc.y"
          {
            char*message=NULL;
            (yyval.slist)=NULL;
            K0 KS("macro ") KS(pproot()->cr_scanner->cur_message) KS(" is undefined\n")
            adms_preprocessor_add_message(message);
            pproot()->error += 1;
            free(pproot()->cr_scanner->cur_message);
            pproot()->cr_scanner->cur_message=NULL;
          }
#line 1796 "y.tab.c"
    break;

  case 36: /* R_substitutor: TK_SUBSTITUTOR_NOARG  */
#line 423 "preprocessorYacc.y"
          {
            p_preprocessor_pragma_define Define=adms_preprocessor_pragma_define_exists((yyvsp[0].mystr));
            (yyval.slist)=adms_preprocessor_new_text_as_substitutor(Define,NULL);
          }
#line 1805 "y.tab.c"
    break;

  case 37: /* R_substitutor: TK_SUBSTITUTOR_NULLARG_ALONE  */
#line 428 "preprocessorYacc.y"
          {
            p_preprocessor_pragma_define Define=adms_preprocessor_pragma_define_exists((yyvsp[0].mystr));
            (yyval.slist)=adms_preprocessor_new_text_as_substitutor(Define,NULL);
          }
#line 1814 "y.tab.c"
    break;

  case 38: /* R_substitutor: R_substitutor_nullarg R_arg_null  */
#line 433 "preprocessorYacc.y"
          {
            p_preprocessor_pragma_define Define=adms_preprocessor_pragma_define_exists((yyvsp[-1].mystr));
            (yyval.slist)=adms_preprocessor_new_text_as_substitutor(Define,NULL);
          }
#line 1823 "y.tab.c"
    break;

  case 39: /* R_substitutor: R_substitutor_nullarg '(' R_substitutor_list_of_arg ')'  */
#line 438 "preprocessorYacc.y"
          {
            char*message=NULL;
            p_preprocessor_pragma_define Define=adms_preprocessor_pragma_define_exists((yyvsp[-3].mystr));
            (yyval.slist)=adms_preprocessor_new_text_as_substitutor(Define, (yyvsp[-1].slist));
            K0 KS("arguments given to macro `") KS( Define->name) KS("\n")
            adms_preprocessor_add_message(message);
            pproot()->error += 1;
          }
#line 1836 "y.tab.c"
    break;

  case 40: /* R_substitutor: TK_SUBSTITUTOR_WITHARG_ALONE  */
#line 447 "preprocessorYacc.y"
          {
            char*message=NULL;
            p_preprocessor_pragma_define Define=adms_preprocessor_pragma_define_exists((yyvsp[0].mystr));
            (yyval.slist)=adms_preprocessor_new_text_as_substitutor(Define,NULL);
            K0 KS("macro `") KS(Define->name) KS(" has no argument [") KI(adms_slist_length(Define->arg)) KS(" expected]\n")
            adms_preprocessor_add_message(message);
            pproot()->error += 1;
            adms_slist_push(&((yyval.slist)),(p_adms)(yyvsp[0].mystr));
          }
#line 1850 "y.tab.c"
    break;

  case 41: /* R_substitutor: R_substitutor_witharg R_arg_null  */
#line 457 "preprocessorYacc.y"
          {
            char*message=NULL;
            p_preprocessor_pragma_define Define=adms_preprocessor_pragma_define_exists((yyvsp[-1].mystr));
            (yyval.slist)=adms_preprocessor_new_text_as_substitutor(Define,NULL);
            K0 KS("macro `") KS(Define->name) KS(" has no argument [") KI(adms_slist_length(Define->arg)) KS(" expected]\n")
            adms_preprocessor_add_message(message);
            pproot()->error += 1;
          }
#line 1863 "y.tab.c"
    break;

  case 42: /* R_substitutor: R_substitutor_witharg '(' R_substitutor_list_of_arg ')'  */
#line 466 "preprocessorYacc.y"
          {
            p_preprocessor_pragma_define Define=adms_preprocessor_pragma_define_exists((yyvsp[-3].mystr));
            {
              if(adms_slist_length((yyvsp[-1].slist)) == adms_slist_length(Define->arg))
              {
              }
              else if(adms_slist_length((yyvsp[-1].slist)) > adms_slist_length(Define->arg))
              {
                if(adms_slist_length((yyvsp[-1].slist)) == 1)
                {
                  char*message=NULL;
                  K0 KS("macro `") KS(Define->name) KS(" has one argument [") KI(adms_slist_length(Define->arg)) KS(" expected]\n")
                  adms_preprocessor_add_message(message);
                }
                else
                {
                  char*message=NULL;
                  K0 KS("macro `") KS(Define->name) KS(" has too many (") KI(adms_slist_length((yyvsp[-1].slist))) KS(") arguments\n") 
                  adms_preprocessor_add_message(message);
                }
                pproot()->error += 1;
              }
              else
              {
                if(adms_slist_length((yyvsp[-1].slist)) == 1)
                {
                  char*message=NULL;
                  K0 KS("macro `") KS(Define->name) KS(" has one argument [") KI(adms_slist_length(Define->arg)) KS(" expected]\n")
                  adms_preprocessor_add_message(message);
                }
                else
                {
                  char*message=NULL;
                  K0 KS("macro `") KS(Define->name) KS(" has too few (") KI(adms_slist_length((yyvsp[-1].slist))) KS(") arguments\n")
                  adms_preprocessor_add_message(message);
                }
                pproot()->error += 1;
              }
            }
            (yyval.slist)=adms_preprocessor_new_text_as_substitutor(Define, (yyvsp[-1].slist));
          }
#line 1909 "y.tab.c"
    break;

  case 43: /* R_substitutor_nullarg: TK_SUBSTITUTOR_NULLARG TK_SPACE  */
#line 511 "preprocessorYacc.y"
          {
            (yyval.mystr)=(yyvsp[-1].mystr);
          }
#line 1917 "y.tab.c"
    break;

  case 44: /* R_substitutor_nullarg: TK_SUBSTITUTOR_NULLARG  */
#line 515 "preprocessorYacc.y"
          {
            (yyval.mystr)=(yyvsp[0].mystr);
          }
#line 1925 "y.tab.c"
    break;

  case 45: /* R_substitutor_witharg: TK_SUBSTITUTOR_WITHARG TK_SPACE  */
#line 522 "preprocessorYacc.y"
          {
            (yyval.mystr)=(yyvsp[-1].mystr);
          }
#line 1933 "y.tab.c"
    break;

  case 46: /* R_substitutor_witharg: TK_SUBSTITUTOR_WITHARG  */
#line 526 "preprocessorYacc.y"
          {
            (yyval.mystr)=(yyvsp[0].mystr);
          }
#line 1941 "y.tab.c"
    break;

  case 47: /* R_arg_null: '(' ')'  */
#line 533 "preprocessorYacc.y"
          {
          }
#line 1948 "y.tab.c"
    break;

  case 48: /* R_substitutor_list_of_arg: R_list_of_arg  */
#line 539 "preprocessorYacc.y"
          {
            (yyval.slist)=adms_slist_new((p_adms)(yyvsp[0].slist));
          }
#line 1956 "y.tab.c"
    break;

  case 49: /* R_substitutor_list_of_arg: R_substitutor_list_of_arg ',' R_list_of_arg  */
#line 543 "preprocessorYacc.y"
          {
            adms_slist_push(&((yyvsp[-2].slist)),(p_adms)(yyvsp[0].slist));
            (yyval.slist)=(yyvsp[-2].slist);
          }
#line 1965 "y.tab.c"
    break;

  case 50: /* R_list_of_arg: R_arg  */
#line 551 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1973 "y.tab.c"
    break;

  case 51: /* R_list_of_arg: R_list_of_arg R_arg  */
#line 555 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
            adms_slist_concat(&((yyval.slist)),(yyvsp[-1].slist));
          }
#line 1982 "y.tab.c"
    break;

  case 52: /* R_list_of_arg_with_comma: R_list_of_arg  */
#line 563 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 1990 "y.tab.c"
    break;

  case 53: /* R_list_of_arg_with_comma: R_list_of_arg_with_comma ',' R_list_of_arg  */
#line 567 "preprocessorYacc.y"
          {
            p_preprocessor_text comma=adms_preprocessor_new_text_as_string(",");
            adms_slist_push(&((yyvsp[-2].slist)),(p_adms)comma);
            (yyval.slist)=(yyvsp[0].slist);
            adms_slist_concat(&((yyval.slist)),(yyvsp[-2].slist));
          }
#line 2001 "y.tab.c"
    break;

  case 54: /* R_arg: TK_SPACE  */
#line 576 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2010 "y.tab.c"
    break;

  case 55: /* R_arg: TK_CONTINUATOR  */
#line 581 "preprocessorYacc.y"
          {
            /* SRW -- fix parse problem, string macro arg broken by
             * continuator caused 'unexpected end of line' error.
             */
            (yyval.slist)=0;
          }
#line 2021 "y.tab.c"
    break;

  case 56: /* R_arg: TK_COMMENT  */
#line 588 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2030 "y.tab.c"
    break;

  case 57: /* R_arg: TK_EOL  */
#line 593 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string("\n");
            ++pproot()->cr_scanner->cur_line_position;
            pproot()->cr_scanner->cur_char_position=1;
            pproot()->cr_scanner->cur_continuator_position=NULL;
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2042 "y.tab.c"
    break;

  case 58: /* R_arg: '(' R_list_of_arg_with_comma ')'  */
#line 601 "preprocessorYacc.y"
          {
            p_preprocessor_text lparen=adms_preprocessor_new_text_as_string("(");
            p_preprocessor_text rparen=adms_preprocessor_new_text_as_string(")");
            (yyval.slist)=(yyvsp[-1].slist);
            adms_slist_concat(&((yyval.slist)),adms_slist_new((p_adms)lparen));
            adms_slist_push(&((yyval.slist)),(p_adms)rparen);
          }
#line 2054 "y.tab.c"
    break;

  case 59: /* R_arg: '(' ')'  */
#line 609 "preprocessorYacc.y"
          {
            p_preprocessor_text lparen=adms_preprocessor_new_text_as_string("(");
            p_preprocessor_text rparen=adms_preprocessor_new_text_as_string(")");
            (yyval.slist)=adms_slist_new((p_adms)lparen);
            adms_slist_push(&((yyval.slist)),(p_adms)rparen);
          }
#line 2065 "y.tab.c"
    break;

  case 60: /* R_arg: TK_IDENT  */
#line 616 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2074 "y.tab.c"
    break;

  case 61: /* R_arg: TK_STRING  */
#line 621 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2083 "y.tab.c"
    break;

  case 62: /* R_arg: TK_NOT_IDENT  */
#line 626 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2092 "y.tab.c"
    break;

  case 63: /* R_arg: R_substitutor  */
#line 631 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 2100 "y.tab.c"
    break;

  case 64: /* R_other: '('  */
#line 638 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string("(");
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2109 "y.tab.c"
    break;

  case 65: /* R_other: ')'  */
#line 643 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string(")");
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2118 "y.tab.c"
    break;

  case 66: /* R_other: ','  */
#line 648 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string(",");
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2127 "y.tab.c"
    break;

  case 67: /* R_other: TK_IDENT  */
#line 653 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2136 "y.tab.c"
    break;

  case 68: /* R_other: TK_NOT_IDENT  */
#line 658 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2145 "y.tab.c"
    break;

  case 69: /* R_other: TK_STRING  */
#line 663 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2154 "y.tab.c"
    break;

  case 70: /* R_other: TK_SPACE  */
#line 668 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2163 "y.tab.c"
    break;

  case 71: /* R_other: TK_COMMENT  */
#line 673 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2172 "y.tab.c"
    break;

  case 72: /* R_other: TK_EOF  */
#line 678 "preprocessorYacc.y"
          {
            p_preprocessor_text newtext=adms_preprocessor_new_text_as_string((yyvsp[0].mystr));
            (yyval.slist)=adms_slist_new((p_adms)newtext);
          }
#line 2181 "y.tab.c"
    break;

  case 73: /* R_define_alternative: R_define TK_DEFINE_END  */
#line 686 "preprocessorYacc.y"
          {
            p_preprocessor_pragma_define Define;
            if(DONT_SKIPP) Define=adms_preprocessor_define_add((yyvsp[-1].mystr));
            (yyval.slist)=NULL;
          }
#line 2191 "y.tab.c"
    break;

  case 74: /* R_define_alternative: R_define R_define_text TK_DEFINE_END  */
#line 692 "preprocessorYacc.y"
          {
            p_preprocessor_pragma_define Define;
            if(DONT_SKIPP) Define=adms_preprocessor_define_add_with_text((yyvsp[-2].mystr), (yyvsp[-1].slist));
            (yyval.slist)=NULL;
          }
#line 2201 "y.tab.c"
    break;

  case 75: /* R_define_alternative: R_define TK_ARG_NULL TK_DEFINE_END  */
#line 698 "preprocessorYacc.y"
          {
            p_preprocessor_pragma_define Define;
            if(DONT_SKIPP) Define=adms_preprocessor_define_add_with_arg((yyvsp[-2].mystr), NULL);
            (yyval.slist)=NULL;
          }
#line 2211 "y.tab.c"
    break;

  case 76: /* R_define_alternative: R_define TK_ARG_NULL R_define_text TK_DEFINE_END  */
#line 704 "preprocessorYacc.y"
          {
            p_preprocessor_pragma_define Define;
            if(DONT_SKIPP) Define=adms_preprocessor_define_add_with_arg_and_text((yyvsp[-3].mystr), NULL, (yyvsp[-1].slist));
            (yyval.slist)=NULL;
          }
#line 2221 "y.tab.c"
    break;

  case 77: /* R_define_alternative: R_define R_define_list_of_arg TK_DEFINE_END  */
#line 710 "preprocessorYacc.y"
          {
            p_preprocessor_pragma_define Define;
            if(DONT_SKIPP) Define=adms_preprocessor_define_add_with_arg((yyvsp[-2].mystr), (yyvsp[-1].slist));
            (yyval.slist)=NULL;
          }
#line 2231 "y.tab.c"
    break;

  case 78: /* R_define_alternative: R_define R_define_list_of_arg R_define_text TK_DEFINE_END  */
#line 716 "preprocessorYacc.y"
          {
            p_preprocessor_pragma_define Define;
            if(DONT_SKIPP) Define=adms_preprocessor_define_add_with_arg_and_text((yyvsp[-3].mystr), (yyvsp[-2].slist), (yyvsp[-1].slist));
            (yyval.slist)=NULL;
          }
#line 2241 "y.tab.c"
    break;

  case 79: /* R_define: TK_DEFINE TK_PRAGMA_NAME  */
#line 725 "preprocessorYacc.y"
          {
            (yyval.mystr)=(yyvsp[0].mystr);
          }
#line 2249 "y.tab.c"
    break;

  case 80: /* R_define_list_of_arg: TK_ARG  */
#line 732 "preprocessorYacc.y"
          {
            (yyval.slist)=adms_slist_new((p_adms)(yyvsp[0].mystr));
          }
#line 2257 "y.tab.c"
    break;

  case 81: /* R_define_list_of_arg: R_define_list_of_arg TK_ARG  */
#line 736 "preprocessorYacc.y"
          {
            adms_slist_push(&((yyvsp[-1].slist)),(p_adms)(yyvsp[0].mystr));
            (yyval.slist)=(yyvsp[-1].slist);
          }
#line 2266 "y.tab.c"
    break;

  case 82: /* R_define_text: R_define_notpragma  */
#line 744 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
          }
#line 2274 "y.tab.c"
    break;

  case 83: /* R_define_text: R_define_text R_define_notpragma  */
#line 748 "preprocessorYacc.y"
          {
            (yyval.slist)=(yyvsp[0].slist);
            adms_slist_concat(&((yyval.slist)),(yyvsp[-1].slist));
          }
#line 2283 "y.tab.c"
    break;


#line 2287 "y.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 754 "preprocessorYacc.y"


int adms_preprocessor_getint_yydebug(void)
  {
    return yydebug;
  }
void adms_preprocessor_setint_yydebug(const int val)
  {
    yydebug=val;
  }
