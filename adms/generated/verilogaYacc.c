/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison implementation for Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         verilogaparse
#define yylex           verilogalex
#define yyerror         verilogaerror
#define yylval          verilogalval
#define yychar          verilogachar
#define yydebug         verilogadebug
#define yynerrs         veriloganerrs

/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 6 "verilogaYacc.y"

#define YYDEBUG 1
#include "admsVeriloga.h"

static e_verilogactx ctx;
e_verilogactx verilogactx (){return ctx;}
void set_context(e_verilogactx x){ctx=x;}

#define NEWVARIABLE(l) p_variableprototype myvariableprototype=adms_variableprototype_new(gModule,l,(p_adms)gBlockList->data);

inline static void   Y (p_yaccval myyaccval,p_adms myusrdata) {myyaccval->_usrdata=myusrdata;}
inline static p_adms YY(p_yaccval myyaccval)                  {return myyaccval->_usrdata;}
static char* gNatureAccess=NULL;
static p_number gNatureAbsTol=NULL;
static char* gNatureUnits=NULL;
static char* gNatureidt=NULL;
static char* gNatureddt=NULL;
static char* gDisc=NULL;
static p_discipline gDiscipline=NULL;
static p_module gModule=NULL;
static p_analogfunction gAnalogfunction=NULL;
static p_module gInstanceModule=NULL;
static p_node gGND=NULL;
static p_source gSource=NULL;
static p_lexval gLexval=NULL;
static p_contribution gContribution=NULL;
static admse gVariableType=admse_real;
static admse gNodeDirection;
int uid=0;
static p_slist gVariableDeclarationList=NULL;
static p_slist gInstanceVariableList=NULL;
static p_slist gTerminalList=NULL;
static p_slist gBranchAliasList=NULL;
static p_slist gRangeList=NULL;
static p_slist gNodeList=NULL;
static p_slist gAttributeList=NULL;
static p_slist gGlobalAttributeList=NULL;
static p_slist gBlockList=NULL;
static p_slist gBlockVariableList=NULL;
static p_branchalias gBranchAlias=NULL;

static void adms_veriloga_message_fatal_continue(const p_lexval mytoken)
{
  adms_message_fatal_continue(("[%s:%i:%i]: at '%s':\n",mytoken->_f,mytoken->_l,mytoken->_c,mytoken->_string))
}
static void adms_veriloga_message_fatal (const char* message,const p_lexval mytoken)
{
  adms_veriloga_message_fatal_continue(mytoken);
  adms_message_fatal((message))
}
/*
inline static p_variableprototype variableprototype_recursive_lookup_by_id (p_adms myadms,p_lexval mylexval)
{
  if(myadms==(p_adms)gModule)
    return adms_module_list_variable_lookup_by_id(gModule,gModule,mylexval,(p_adms)gModule);
  else if(myadms==(p_adms)gAnalogfunction)
    return adms_analogfunction_list_variable_lookup_by_id(gAnalogfunction,gModule,mylexval,(p_adms)gAnalogfunction);
  else
  {
    p_slist l;
    for(l=((p_block)myadms)->_variable;l;l=l->next)
      if(!strcmp(((p_variableprototype)l->data)->_lexval->_string,mylexval->_string))
        return (p_variableprototype)l->data;
    return variableprototype_recursive_lookup_by_id((p_adms)((p_block)myadms)->_block,mylexval);
  }
}
*/
inline static p_variable variable_recursive_lookup_by_id (p_adms myadms,p_lexval mylexval)
{
  if(myadms==(p_adms)gModule)
  {
    p_variable myvariable=NULL;
    p_variableprototype myvariableprototype;
    if((myvariableprototype=adms_module_list_variable_lookup_by_id(gModule,gModule,mylexval,(p_adms)gModule)))
    {
      myvariable=adms_variable_new(myvariableprototype);
      adms_slist_push(&myvariableprototype->_instance,(p_adms)myvariable);
    }
    return myvariable;
  }
  else if(myadms==(p_adms)gAnalogfunction)
  {
    p_variable myvariable=NULL;
    p_variableprototype myvariableprototype;
    if((myvariableprototype=adms_analogfunction_list_variable_lookup_by_id(gAnalogfunction,gModule,mylexval,(p_adms)gAnalogfunction)))
    {
      myvariable=adms_variable_new(myvariableprototype);
      adms_slist_push(&myvariableprototype->_instance,(p_adms)myvariable);
    }
    return myvariable;
  }
  else
  {
    p_slist l;
    for(l=((p_block)myadms)->_variable;l;l=l->next)
      if(!strcmp(((p_variableprototype)l->data)->_lexval->_string,mylexval->_string))
      {
        p_variableprototype myvariableprototype=(p_variableprototype)l->data;
        p_variable myvariable=adms_variable_new(myvariableprototype);
        adms_slist_push(&myvariableprototype->_instance,(p_adms)myvariable);
        return myvariable;
      }
    return variable_recursive_lookup_by_id((p_adms)((p_block)myadms)->_block,mylexval);
  }
}
static p_nature lookup_nature(const char *myname)
{
  p_slist l;
  for(l=root()->_nature;l;l=l->next)
    if(!strcmp(((p_nature)l->data)->_name,myname))
      return (p_nature)l->data;
  return NULL;
}


/* Line 371 of yacc.c  */
#line 191 "y.tab.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
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
/* Line 387 of yacc.c  */
#line 126 "verilogaYacc.y"

  p_lexval _lexval;
  p_yaccval _yaccval;


/* Line 387 of yacc.c  */
#line 344 "y.tab.c"
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

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 372 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  20
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   653

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  79
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  122
/* YYNRULES -- Number of rules.  */
#define YYNRULES  263
/* YYNRULES -- Number of states.  */
#define YYNSTATES  514

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   307

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    74,     2,    67,     2,    78,    73,     2,
      55,    56,    76,    64,    57,    63,    68,    77,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    61,    53,
      66,    54,    75,    69,    65,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    60,     2,    62,    71,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    58,    72,    59,    70,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,    10,    12,    14,    16,    21,
      23,    25,    28,    32,    36,    40,    42,    47,    49,    52,
      57,    63,    68,    73,    74,    76,    80,    83,    86,    88,
      91,    95,    96,    97,   106,   107,   109,   111,   114,   116,
     118,   121,   124,   128,   130,   133,   139,   140,   142,   144,
     148,   150,   152,   155,   157,   160,   162,   164,   166,   167,
     173,   174,   179,   180,   185,   187,   189,   191,   192,   197,
     198,   203,   204,   209,   211,   213,   215,   217,   221,   223,
     227,   230,   233,   237,   239,   243,   245,   252,   257,   262,
     267,   273,   279,   281,   283,   286,   290,   294,   298,   302,
     306,   308,   312,   314,   318,   320,   324,   326,   330,   332,
     336,   338,   340,   342,   343,   347,   349,   351,   355,   357,
     361,   368,   370,   372,   375,   378,   383,   390,   392,   399,
     400,   402,   404,   407,   410,   413,   419,   425,   431,   437,
     439,   441,   444,   446,   448,   451,   452,   456,   458,   460,
     462,   466,   469,   471,   474,   476,   478,   480,   482,   488,
     493,   496,   498,   500,   503,   511,   514,   519,   521,   525,
     527,   530,   535,   539,   545,   548,   550,   553,   557,   561,
     565,   567,   571,   573,   580,   584,   589,   596,   601,   607,
     617,   624,   626,   629,   633,   637,   640,   641,   646,   654,
     656,   658,   662,   668,   672,   677,   684,   692,   698,   706,
     708,   710,   714,   716,   718,   720,   726,   728,   732,   737,
     739,   743,   745,   749,   751,   755,   757,   761,   763,   767,
     769,   774,   779,   781,   785,   790,   794,   799,   801,   805,
     809,   811,   815,   819,   821,   825,   829,   833,   835,   838,
     841,   844,   847,   849,   852,   854,   857,   859,   861,   863,
     865,   870,   875,   880
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      80,     0,    -1,    81,    -1,    82,    -1,    81,    82,    -1,
      95,    -1,    83,    -1,    88,    -1,    18,    84,    85,    34,
      -1,    36,    -1,    86,    -1,    85,    86,    -1,    26,    87,
      53,    -1,    52,    87,    53,    -1,    35,    36,    53,    -1,
      36,    -1,     8,    36,    89,    50,    -1,    90,    -1,    89,
      90,    -1,    36,    54,     7,    53,    -1,    36,    54,     7,
      36,    53,    -1,    36,    54,    11,    53,    -1,    36,    54,
      36,    53,    -1,    -1,    92,    -1,    49,    93,    41,    -1,
      49,    20,    -1,    49,    41,    -1,    94,    -1,    93,    94,
      -1,    36,    54,    11,    -1,    -1,    -1,    91,    40,    36,
      96,   101,    97,    98,    32,    -1,    -1,   105,    -1,    99,
      -1,   105,    99,    -1,   153,    -1,   100,    -1,   100,   153,
      -1,   153,   100,    -1,   100,   153,   100,    -1,   177,    -1,
     100,   177,    -1,    55,   102,    56,    91,    53,    -1,    -1,
     103,    -1,   104,    -1,   103,    57,   104,    -1,    36,    -1,
     106,    -1,   105,   106,    -1,   108,    -1,   107,   108,    -1,
      92,    -1,   112,    -1,   121,    -1,    -1,    17,   109,   136,
     139,   138,    -1,    -1,    17,   110,   139,   138,    -1,    -1,
     136,   111,   140,   138,    -1,   141,    -1,   125,    -1,    53,
      -1,    -1,   116,   113,   117,    53,    -1,    -1,    31,   114,
     118,    53,    -1,    -1,    28,   115,   118,    53,    -1,    48,
      -1,    10,    -1,    43,    -1,   119,    -1,   117,    57,   119,
      -1,   120,    -1,   118,    57,   120,    -1,    36,    91,    -1,
      36,    91,    -1,     6,   124,    53,    -1,   123,    -1,   122,
      57,   123,    -1,    36,    -1,    55,    36,    57,    36,    56,
     122,    -1,    55,    36,    56,   122,    -1,   126,   128,   158,
      51,    -1,    16,    47,   127,    53,    -1,    16,    47,    39,
     127,    53,    -1,    16,    47,    23,   127,    53,    -1,    36,
      -1,   129,    -1,   128,   129,    -1,    48,   130,    53,    -1,
      10,   131,    53,    -1,    43,   132,    53,    -1,    39,   133,
      53,    -1,    23,   134,    53,    -1,    36,    -1,   130,    57,
      36,    -1,    36,    -1,   131,    57,    36,    -1,    36,    -1,
     132,    57,    36,    -1,    36,    -1,   133,    57,    36,    -1,
      36,    -1,   134,    57,    36,    -1,    39,    -1,    23,    -1,
      38,    -1,    -1,   135,   137,    91,    -1,    53,    -1,   143,
      -1,   139,    57,   143,    -1,   144,    -1,   140,    57,   144,
      -1,   142,    36,    54,    36,    91,    53,    -1,     9,    -1,
      14,    -1,   145,    91,    -1,   146,    91,    -1,   146,    54,
     183,   147,    -1,   146,    54,    58,   156,    59,   147,    -1,
      36,    -1,    36,    60,     7,    61,     7,    62,    -1,    -1,
     148,    -1,   149,    -1,   148,   149,    -1,     5,   150,    -1,
      30,   150,    -1,    55,   151,    61,   152,    56,    -1,    55,
     151,    61,   152,    62,    -1,    60,   151,    61,   152,    56,
      -1,    60,   151,    61,   152,    62,    -1,   183,    -1,   183,
      -1,    63,    29,    -1,   183,    -1,    29,    -1,    64,    29,
      -1,    -1,    16,   154,   155,    -1,   157,    -1,   158,    -1,
     183,    -1,   156,    57,   183,    -1,    91,   165,    -1,   168,
      -1,   181,    53,    -1,   182,    -1,   171,    -1,   173,    -1,
     172,    -1,    12,    55,   156,    56,    53,    -1,    12,    55,
      56,    53,    -1,    12,    53,    -1,    53,    -1,   162,    -1,
     159,   162,    -1,    65,    55,    36,    55,   160,    56,    56,
      -1,    65,    36,    -1,    65,    55,    36,    56,    -1,   161,
      -1,   160,    57,   161,    -1,    11,    -1,   163,    42,    -1,
     163,    61,    36,    42,    -1,   163,   164,    42,    -1,   163,
      61,    36,   164,    42,    -1,    91,    33,    -1,   155,    -1,
     164,   155,    -1,    39,   166,    53,    -1,    23,   166,    53,
      -1,    38,   166,    53,    -1,   167,    -1,   166,    57,   167,
      -1,    36,    -1,    36,    60,     7,    61,     7,    62,    -1,
     169,    91,    53,    -1,   170,    66,    64,   183,    -1,    36,
      55,    36,    57,    36,    56,    -1,    36,    55,    36,    56,
      -1,    22,    55,   183,    56,   155,    -1,    21,    55,   181,
      53,   183,    53,   181,    56,   155,    -1,    25,    55,   183,
      56,   174,    27,    -1,   175,    -1,   174,   175,    -1,   184,
      61,   155,    -1,    46,    61,   155,    -1,    46,   155,    -1,
      -1,    67,    55,   179,    56,    -1,   178,   176,    36,    55,
     118,    56,    53,    -1,    36,    -1,   180,    -1,   179,    57,
     180,    -1,    68,    36,    55,   183,    56,    -1,    36,    54,
     183,    -1,    92,    36,    54,   183,    -1,    36,    60,   186,
      62,    54,   183,    -1,    92,    36,    60,   186,    62,    54,
     183,    -1,    15,    55,   183,    56,   155,    -1,    15,    55,
     183,    56,   155,     4,   155,    -1,   186,    -1,   185,    -1,
     184,    57,   185,    -1,   186,    -1,   187,    -1,   188,    -1,
     188,    69,   188,    61,   188,    -1,   189,    -1,   188,    45,
     189,    -1,   188,    70,    71,   189,    -1,   190,    -1,   189,
      71,   190,    -1,   191,    -1,   190,    72,   191,    -1,   192,
      -1,   191,    73,   192,    -1,   193,    -1,   192,    13,   193,
      -1,   194,    -1,   193,    44,   194,    -1,   195,    -1,   194,
      54,    54,   195,    -1,   194,    74,    54,   195,    -1,   196,
      -1,   195,    66,   196,    -1,   195,    66,    54,   196,    -1,
     195,    75,   196,    -1,   195,    75,    54,   196,    -1,   197,
      -1,   196,    24,   197,    -1,   196,    37,   197,    -1,   198,
      -1,   197,    64,   198,    -1,   197,    63,   198,    -1,   199,
      -1,   198,    76,   199,    -1,   198,    77,   199,    -1,   198,
      78,   199,    -1,   200,    -1,    64,   200,    -1,    63,   200,
      -1,    74,   200,    -1,    70,   200,    -1,    39,    -1,    39,
      36,    -1,     7,    -1,     7,    36,    -1,    19,    -1,    11,
      -1,    36,    -1,    12,    -1,    36,    60,   186,    62,    -1,
      12,    55,   184,    56,    -1,    36,    55,   184,    56,    -1,
      55,   186,    56,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   295,   295,   300,   303,   308,   311,   314,   319,   326,
     333,   336,   341,   345,   349,   361,   372,   397,   400,   405,
     416,   451,   463,   490,   492,   497,   500,   508,   513,   516,
     521,   533,   548,   532,   559,   561,   564,   567,   572,   575,
     578,   581,   584,   589,   592,   597,   607,   609,   614,   617,
     622,   632,   635,   640,   644,   651,   658,   661,   665,   664,
     672,   671,   679,   678,   685,   688,   691,   697,   696,   708,
     707,   719,   718,   735,   739,   743,   749,   752,   757,   760,
     765,   783,   797,   802,   805,   810,   817,   842,   865,   873,
     879,   887,   895,   906,   909,   914,   917,   920,   923,   926,
     931,   938,   947,   954,   963,   971,   981,   993,  1007,  1019,
    1033,  1037,  1041,  1048,  1047,  1058,  1067,  1070,  1075,  1078,
    1083,  1099,  1102,  1107,  1118,  1129,  1136,  1148,  1157,  1172,
    1174,  1179,  1182,  1187,  1195,  1205,  1213,  1221,  1229,  1237,
    1247,  1251,  1264,  1268,  1279,  1293,  1292,  1303,  1307,  1313,
    1320,  1329,  1339,  1343,  1347,  1351,  1355,  1359,  1363,  1374,
    1382,  1390,  1397,  1401,  1409,  1413,  1426,  1441,  1444,  1449,
    1454,  1460,  1467,  1473,  1482,  1496,  1500,  1506,  1519,  1532,
    1547,  1550,  1555,  1561,  1572,  1582,  1594,  1611,  1636,  1645,
    1654,  1664,  1671,  1680,  1688,  1695,  1705,  1707,  1712,  1735,
    1746,  1749,  1754,  1774,  1789,  1810,  1827,  1852,  1860,  1871,
    1880,  1887,  1896,  1902,  1908,  1912,  1923,  1927,  1935,  1945,
    1949,  1959,  1963,  1973,  1977,  1987,  1991,  2001,  2005,  2015,
    2019,  2027,  2037,  2041,  2049,  2057,  2065,  2075,  2079,  2087,
    2097,  2101,  2109,  2119,  2123,  2131,  2139,  2149,  2153,  2160,
    2167,  2174,  2183,  2190,  2221,  2226,  2256,  2260,  2266,  2284,
    2290,  2298,  2307,  2359
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "PREC_IF_THEN", "tk_else", "tk_from",
  "tk_branch", "tk_number", "tk_nature", "tk_aliasparameter", "tk_output",
  "tk_anystring", "tk_dollar_ident", "tk_or", "tk_aliasparam", "tk_if",
  "tk_analog", "tk_parameter", "tk_discipline", "tk_char", "tk_anytext",
  "tk_for", "tk_while", "tk_real", "tk_op_shr", "tk_case", "tk_potential",
  "tk_endcase", "tk_disc_id", "tk_inf", "tk_exclude", "tk_ground",
  "tk_endmodule", "tk_begin", "tk_enddiscipline", "tk_domain", "tk_ident",
  "tk_op_shl", "tk_string", "tk_integer", "tk_module", "tk_endattribute",
  "tk_end", "tk_inout", "tk_and", "tk_bitwise_equr", "tk_default",
  "tk_function", "tk_input", "tk_beginattribute", "tk_endnature",
  "tk_endfunction", "tk_flow", "';'", "'='", "'('", "')'", "','", "'{'",
  "'}'", "'['", "':'", "']'", "'-'", "'+'", "'@'", "'<'", "'#'", "'.'",
  "'?'", "'~'", "'^'", "'|'", "'&'", "'!'", "'>'", "'*'", "'/'", "'%'",
  "$accept", "R_admsParse", "R_l.admsParse", "R_s.admsParse",
  "R_discipline_member", "R_discipline_name", "R_l.discipline_assignment",
  "R_s.discipline_assignment", "R_discipline.naturename",
  "R_nature_member", "R_l.nature_assignment", "R_s.nature_assignment",
  "R_d.attribute.0", "R_d.attribute", "R_l.attribute", "R_s.attribute",
  "R_d.module", "$@1", "$@2", "R_modulebody", "R_netlist", "R_l.instance",
  "R_d.terminal", "R_l.terminal.0", "R_l.terminal", "R_s.terminal",
  "R_l.declaration", "R_s.declaration.withattribute",
  "R_d.attribute.global", "R_s.declaration", "$@3", "$@4", "$@5",
  "R_d.node", "$@6", "$@7", "$@8", "R_node.type", "R_l.terminalnode",
  "R_l.node", "R_s.terminalnode", "R_s.node", "R_d.branch",
  "R_l.branchalias", "R_s.branchalias", "R_s.branch", "R_d.analogfunction",
  "R_d.analogfunction.proto", "R_d.analogfunction.name",
  "R_l.analogfunction.declaration", "R_s.analogfunction.declaration",
  "R_l.analogfunction.input.variable",
  "R_l.analogfunction.output.variable",
  "R_l.analogfunction.inout.variable",
  "R_l.analogfunction.integer.variable",
  "R_l.analogfunction.real.variable", "R_variable.type.set",
  "R_variable.type", "$@9", "R_d.variable.end", "R_l.parameter",
  "R_l.variable", "R_d.aliasparameter", "R_d.aliasparameter.token",
  "R_s.parameter", "R_s.variable", "R_s.parameter.name",
  "R_s.variable.name", "R_s.parameter.range", "R_l.interval",
  "R_s.interval", "R_d.interval", "R_interval.inf", "R_interval.sup",
  "R_analog", "$@10", "R_analogcode", "R_l.expression",
  "R_analogcode.atomic", "R_analogcode.block",
  "R_analogcode.block.atevent", "R_l.analysis", "R_s.analysis",
  "R_d.block", "R_d.block.begin", "R_l.blockitem", "R_d.blockvariable",
  "R_l.blockvariable", "R_s.blockvariable", "R_d.contribution",
  "R_contribution", "R_source", "R_d.while", "R_d.for", "R_d.case",
  "R_l.case.item", "R_s.case.item", "R_s.paramlist.0", "R_s.instance",
  "R_instance.module.name", "R_l.instance.parameter",
  "R_s.instance.parameter", "R_s.assignment", "R_d.conditional",
  "R_s.expression", "R_l.enode", "R_s.function_expression", "R_expression",
  "R_e.conditional", "R_e.bitwise_equ", "R_e.bitwise_xor",
  "R_e.bitwise_or", "R_e.bitwise_and", "R_e.logical_or", "R_e.logical_and",
  "R_e.comp_equ", "R_e.comp", "R_e.bitwise_shift", "R_e.arithm_add",
  "R_e.arithm_mult", "R_e.unary", "R_e.atomic", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,    59,    61,    40,    41,    44,   123,   125,
      91,    58,    93,    45,    43,    64,    60,    35,    46,    63,
     126,    94,   124,    38,    33,    62,    42,    47,    37
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    79,    80,    81,    81,    82,    82,    82,    83,    84,
      85,    85,    86,    86,    86,    87,    88,    89,    89,    90,
      90,    90,    90,    91,    91,    92,    92,    92,    93,    93,
      94,    96,    97,    95,    98,    98,    98,    98,    99,    99,
      99,    99,    99,   100,   100,   101,   102,   102,   103,   103,
     104,   105,   105,   106,   106,   107,   108,   108,   109,   108,
     110,   108,   111,   108,   108,   108,   108,   113,   112,   114,
     112,   115,   112,   116,   116,   116,   117,   117,   118,   118,
     119,   120,   121,   122,   122,   123,   124,   124,   125,   126,
     126,   126,   127,   128,   128,   129,   129,   129,   129,   129,
     130,   130,   131,   131,   132,   132,   133,   133,   134,   134,
     135,   135,   135,   137,   136,   138,   139,   139,   140,   140,
     141,   142,   142,   143,   144,   145,   145,   146,   146,   147,
     147,   148,   148,   149,   149,   150,   150,   150,   150,   150,
     151,   151,   152,   152,   152,   154,   153,   155,   155,   156,
     156,   157,   157,   157,   157,   157,   157,   157,   157,   157,
     157,   157,   158,   158,   159,   159,   159,   160,   160,   161,
     162,   162,   162,   162,   163,   164,   164,   165,   165,   165,
     166,   166,   167,   167,   168,   169,   170,   170,   171,   172,
     173,   174,   174,   175,   175,   175,   176,   176,   177,   178,
     179,   179,   180,   181,   181,   181,   181,   182,   182,   183,
     184,   184,   185,   186,   187,   187,   188,   188,   188,   189,
     189,   190,   190,   191,   191,   192,   192,   193,   193,   194,
     194,   194,   195,   195,   195,   195,   195,   196,   196,   196,
     197,   197,   197,   198,   198,   198,   198,   199,   199,   199,
     199,   199,   200,   200,   200,   200,   200,   200,   200,   200,
     200,   200,   200,   200
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     1,     1,     4,     1,
       1,     2,     3,     3,     3,     1,     4,     1,     2,     4,
       5,     4,     4,     0,     1,     3,     2,     2,     1,     2,
       3,     0,     0,     8,     0,     1,     1,     2,     1,     1,
       2,     2,     3,     1,     2,     5,     0,     1,     1,     3,
       1,     1,     2,     1,     2,     1,     1,     1,     0,     5,
       0,     4,     0,     4,     1,     1,     1,     0,     4,     0,
       4,     0,     4,     1,     1,     1,     1,     3,     1,     3,
       2,     2,     3,     1,     3,     1,     6,     4,     4,     4,
       5,     5,     1,     1,     2,     3,     3,     3,     3,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     1,     1,     0,     3,     1,     1,     3,     1,     3,
       6,     1,     1,     2,     2,     4,     6,     1,     6,     0,
       1,     1,     2,     2,     2,     5,     5,     5,     5,     1,
       1,     2,     1,     1,     2,     0,     3,     1,     1,     1,
       3,     2,     1,     2,     1,     1,     1,     1,     5,     4,
       2,     1,     1,     2,     7,     2,     4,     1,     3,     1,
       2,     4,     3,     5,     2,     1,     2,     3,     3,     3,
       1,     3,     1,     6,     3,     4,     6,     4,     5,     9,
       6,     1,     2,     3,     3,     2,     0,     4,     7,     1,
       1,     3,     5,     3,     4,     6,     7,     5,     7,     1,
       1,     3,     1,     1,     1,     5,     1,     3,     4,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       4,     4,     1,     3,     4,     3,     4,     1,     3,     3,
       1,     3,     3,     1,     3,     3,     3,     1,     2,     2,
       2,     2,     1,     2,     1,     2,     1,     1,     1,     1,
       4,     4,     4,     3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
      23,     0,     0,     0,     0,     2,     3,     6,     7,     0,
      24,     5,     0,     9,     0,    26,     0,    27,     0,    28,
       1,     4,     0,     0,     0,    17,     0,     0,     0,     0,
      10,     0,    25,    29,    31,     0,    16,    18,    15,     0,
       0,     0,     8,    11,    30,     0,     0,     0,     0,    12,
      14,    13,    46,    32,     0,    19,    21,    22,    50,     0,
      47,    48,    34,    20,    23,     0,     0,   121,    74,   122,
     145,    58,   111,    71,    69,   199,   112,   110,    75,    73,
      66,    55,     0,    36,    39,    35,    51,     0,    53,    56,
      67,    57,    65,     0,   113,    62,    64,     0,    38,    43,
     196,     0,    49,     0,     0,     0,    23,     0,     0,     0,
       0,    33,   145,    40,    44,    37,    52,     0,    54,     0,
       0,     0,     0,     0,     0,    23,    93,    23,     0,     0,
      41,     0,     0,    45,     0,    82,     0,    92,     0,     0,
       0,     0,     0,     0,     0,     0,   161,     0,     0,    24,
     146,   147,   148,    23,   162,    23,   152,    23,     0,   155,
     157,   156,     0,   154,     0,   127,     0,   116,    23,     0,
      23,     0,    78,     0,    42,    23,     0,    76,   102,     0,
     108,     0,   106,     0,   104,     0,   100,     0,     0,    94,
       0,   114,     0,   118,    23,     0,     0,     0,     0,     0,
       0,     0,    89,   160,     0,     0,     0,     0,     0,     0,
       0,     0,   165,     0,     0,   174,     0,     0,   151,     0,
     163,   170,     0,   175,    23,     0,     0,   153,     0,     0,
     115,     0,    61,   123,     0,    81,    72,     0,    70,    80,
      68,     0,    96,     0,    99,     0,    98,     0,    97,     0,
      95,     0,    88,     0,    63,   124,    23,     0,     0,   200,
       0,    85,    87,    83,     0,    91,    90,   254,   257,   259,
     256,   258,   252,     0,     0,     0,     0,     0,     0,     0,
     149,   209,   213,   214,   216,   219,   221,   223,   225,   227,
     229,   232,   237,   240,   243,   247,     0,     0,     0,     0,
       0,     0,   203,     0,     0,     0,   182,     0,   180,     0,
       0,     0,     0,    23,   172,   176,   184,     0,    59,     0,
     117,     0,   129,    79,    77,   103,   109,   107,   105,   101,
     119,     0,     0,   197,     0,     0,     0,     0,   255,     0,
       0,     0,   253,     0,   159,   249,   248,   251,   250,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      23,     0,    23,     0,   187,     0,     0,     0,   166,     0,
     178,     0,   179,   177,   204,     0,   171,    23,   185,     0,
       0,     0,     0,   125,   130,   131,   120,     0,   201,     0,
      84,    86,     0,   210,   212,     0,     0,   263,   158,   150,
     217,     0,     0,   220,   222,   224,   226,   228,     0,     0,
       0,   233,     0,   235,   238,   239,   242,   241,   244,   245,
     246,   207,     0,   188,    23,     0,   191,     0,     0,     0,
     169,     0,   167,     0,   181,     0,   173,     0,   129,     0,
       0,   133,   139,   134,   132,     0,   198,   261,     0,   262,
     260,     0,   218,   230,   231,   234,   236,    23,     0,    23,
     195,   190,   192,    23,   186,   205,     0,     0,     0,     0,
     128,   126,     0,     0,   140,   209,     0,   202,   211,   215,
     208,     0,   194,   193,   164,   168,     0,   206,   141,     0,
       0,    23,   183,   143,     0,     0,   142,     0,   189,   144,
     135,   136,   137,   138
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     4,     5,     6,     7,    14,    29,    30,    39,     8,
      24,    25,   148,    10,    18,    19,    11,    45,    62,    82,
      83,    84,    53,    59,    60,    61,    85,    86,    87,    88,
     107,   108,   128,    89,   119,   110,   109,    90,   176,   171,
     177,   172,    91,   262,   263,   104,    92,    93,   139,   125,
     126,   187,   179,   185,   183,   181,    94,    95,   127,   232,
     166,   192,    96,    97,   167,   193,   168,   169,   393,   394,
     395,   451,   483,   505,    98,   106,   223,   279,   151,   152,
     153,   441,   442,   154,   155,   224,   218,   307,   308,   156,
     157,   158,   159,   160,   161,   435,   436,   132,    99,   100,
     258,   259,   162,   163,   280,   437,   403,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -338
static const yytype_int16 yypact[] =
{
      37,    22,    98,   117,    48,    35,  -338,  -338,  -338,   -22,
    -338,  -338,   147,  -338,    69,  -338,   137,  -338,   145,  -338,
    -338,  -338,   167,   162,    93,  -338,   189,   191,   189,    12,
    -338,   241,  -338,  -338,  -338,    26,  -338,  -338,  -338,   202,
     224,   252,  -338,  -338,  -338,   232,    70,   254,   256,  -338,
    -338,  -338,   275,  -338,   265,  -338,  -338,  -338,  -338,   238,
     269,  -338,   559,  -338,   285,   275,   292,  -338,  -338,  -338,
     277,   312,  -338,  -338,  -338,  -338,  -338,  -338,  -338,  -338,
    -338,  -338,   322,  -338,    64,   559,  -338,   600,  -338,  -338,
    -338,  -338,  -338,    18,  -338,  -338,  -338,   317,   327,  -338,
     291,   314,  -338,   333,   318,    73,   334,   125,   338,   339,
     339,  -338,  -338,   327,  -338,  -338,  -338,   277,  -338,   341,
     342,   343,   344,   345,   346,   172,  -338,   285,   338,   332,
     327,   337,   352,  -338,   187,  -338,   353,  -338,   353,   347,
      99,   349,   359,   360,   361,   130,  -338,    63,   128,   358,
    -338,  -338,  -338,   285,  -338,   411,  -338,   285,   331,  -338,
    -338,  -338,   348,  -338,   338,   362,    78,  -338,   285,   365,
     285,   112,  -338,   156,   327,   285,   157,  -338,  -338,   171,
    -338,   173,  -338,   176,  -338,   182,  -338,   188,   388,  -338,
     374,  -338,   196,  -338,   285,   393,   363,   379,   399,   402,
     389,   390,  -338,  -338,    15,   354,    97,   354,   354,   354,
     405,   354,  -338,   408,   409,  -338,   409,   409,  -338,   -18,
    -338,  -338,   412,  -338,   437,   397,   387,  -338,    78,   450,
    -338,   338,  -338,  -338,    62,  -338,  -338,   339,  -338,  -338,
    -338,   341,  -338,   427,  -338,   430,  -338,   431,  -338,   433,
    -338,   434,  -338,   338,  -338,  -338,   285,   438,   207,  -338,
     339,  -338,   418,  -338,   424,  -338,  -338,   446,  -338,   428,
    -338,    50,   448,   354,   432,   401,   401,   401,   401,   216,
    -338,  -338,  -338,    46,   416,   417,   415,   478,   449,   -35,
      28,   118,   217,   215,  -338,  -338,   439,   116,   358,   443,
     444,   445,  -338,   227,   452,   234,   451,   197,  -338,   205,
     213,   354,   354,   456,  -338,  -338,  -338,   354,  -338,   447,
    -338,   354,    30,  -338,  -338,  -338,  -338,  -338,  -338,  -338,
    -338,   453,   455,  -338,   363,   245,   399,   399,  -338,   354,
     354,   354,  -338,   459,  -338,  -338,  -338,  -338,  -338,   464,
     354,   354,   354,   441,   354,   354,   354,   354,   354,   465,
     466,   138,   168,   354,   354,   354,   354,   354,   354,   354,
     334,   354,   334,   249,  -338,   463,   471,   516,  -338,   521,
    -338,   409,  -338,  -338,  -338,   468,  -338,   482,  -338,   525,
     139,   267,   267,  -338,    30,  -338,  -338,   354,  -338,   483,
    -338,   418,   258,  -338,  -338,   260,   476,  -338,  -338,  -338,
     416,    52,   354,   417,   415,   478,   449,   -35,   354,   354,
     354,   118,   354,   118,   217,   217,   215,   215,  -338,  -338,
    -338,   535,   487,  -338,   501,    13,  -338,   214,   486,   354,
    -338,   264,  -338,   484,  -338,   489,  -338,   491,    30,   391,
     391,  -338,  -338,  -338,  -338,   488,  -338,  -338,   354,  -338,
    -338,   354,   416,    28,    28,   118,   118,   334,    97,   334,
    -338,  -338,  -338,   334,  -338,  -338,   490,   516,   541,   354,
    -338,  -338,   522,   494,  -338,   459,   498,  -338,  -338,    57,
    -338,   493,  -338,  -338,  -338,  -338,   505,  -338,  -338,   321,
     321,   334,  -338,  -338,   545,   143,  -338,   144,  -338,  -338,
    -338,  -338,  -338,  -338
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -338,  -338,  -338,   555,  -338,  -338,  -338,   534,   542,  -338,
    -338,   547,     3,   -62,  -338,   554,  -338,  -338,  -338,  -338,
     495,   -48,  -338,  -338,  -338,   513,  -338,   500,  -338,   492,
    -338,  -338,  -338,  -338,  -338,  -338,  -338,  -338,  -338,  -101,
     350,   351,  -338,   246,   250,  -338,  -338,  -338,    81,  -338,
     467,  -338,  -338,  -338,  -338,  -338,  -338,   496,  -338,  -136,
     425,  -338,  -338,  -338,   368,   340,  -338,  -112,   146,  -338,
     210,   204,   151,   105,   527,  -338,  -105,   294,  -338,   497,
    -338,  -338,   136,   472,  -338,   305,  -338,   119,   239,  -338,
    -338,  -338,  -338,  -338,  -338,  -338,   184,  -338,   -67,  -338,
    -338,   287,  -199,  -338,  -203,    -1,   166,  -201,  -338,  -337,
    -330,   272,   274,   271,   273,   276,   -76,  -332,   -19,   -14,
     -69,  -264
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -61
static const yytype_int16 yytable[] =
{
      81,   150,   296,     9,   300,   301,   302,   299,     9,   173,
     304,   345,   346,   347,   348,   411,   194,   114,    22,   359,
     267,   410,   267,    81,   268,   269,   268,   269,   120,   421,
     423,   322,   270,    46,   270,   391,   311,    47,    26,   360,
     471,   121,   312,     1,   149,     1,    42,    27,    20,   271,
     130,   271,   272,     2,   272,     2,   254,   122,    12,   434,
     392,   123,    48,   114,    28,   174,   124,   101,   273,   267,
     273,   274,   343,   268,   269,   -23,   275,   276,   275,   276,
     112,   270,   462,   277,     3,   277,     3,   278,   465,   278,
     466,   351,   318,   149,   361,    26,   136,   351,   271,   212,
      75,   272,   351,   362,    27,   340,    54,   114,   384,   137,
     341,   385,   138,   461,   388,   352,   353,   273,   213,   315,
     321,    28,   353,    55,   489,   275,   276,   353,   188,    23,
     191,   230,   277,   297,    13,   231,   278,    15,   404,   404,
     406,   194,   363,    36,   298,   267,     3,   409,    72,   268,
     269,   214,   203,    16,   204,   364,   188,   270,    17,   335,
     225,   215,   149,    76,    77,   236,   216,   217,   432,   237,
     209,   233,   404,   235,   271,   267,   211,   272,   239,   268,
     269,    16,   120,    23,   209,   210,    32,   270,   452,   452,
     211,    31,   420,   273,   455,   121,   350,   255,   448,   510,
     512,   275,   276,    34,   271,   511,   513,   272,   277,   238,
     240,   122,   278,   237,   241,   123,    35,   200,   345,   201,
     124,     3,   422,   273,   242,    38,   244,    40,   243,   246,
     245,   275,   276,   247,   404,   248,   475,   147,   277,   249,
     346,   250,   278,   198,   199,   251,   484,   484,   485,   230,
     380,   149,    44,   253,   381,    49,   267,   404,   382,   331,
     268,   269,   381,   333,   334,   431,   383,   433,   270,   491,
     381,   458,   349,   350,   267,   473,   497,    50,   268,   269,
     365,   366,   315,   374,   375,   271,   270,    52,   272,   377,
     378,   367,   368,   369,    64,   434,   506,   506,   428,   429,
     430,   399,   237,   271,   273,    51,   272,    56,   149,    57,
     149,    58,   275,   276,   457,   458,   459,   458,    63,   277,
     476,   477,   449,   278,   105,   149,    65,   450,   267,   470,
     275,   276,   268,   269,     3,   309,   310,   277,   402,   405,
     270,   278,   463,   464,   424,   425,   140,   103,   -60,   141,
     503,   426,   427,   129,   111,   142,   143,   271,   131,   144,
     272,   267,   490,    75,   492,   268,   269,   133,   493,   134,
     145,   135,   149,   270,   165,   170,   273,   175,   178,   180,
     182,   184,   186,     3,   275,   504,   195,   146,   197,   137,
     271,   277,   196,   272,   219,   278,   508,   226,   267,   147,
     202,   227,   268,   269,   205,   149,   298,   149,   267,   273,
     270,   149,   268,   269,   206,   207,   208,   275,   276,   234,
     270,   215,   229,   140,   277,   252,   141,   271,   278,   256,
     272,   257,   142,   143,   260,   261,   144,   271,   264,   149,
     272,   303,   265,   266,   305,   306,   273,   145,   313,   140,
     316,   317,   141,   221,   482,   276,   273,   319,   142,   143,
       3,   277,   144,   325,   146,   278,   326,   327,   140,   328,
     329,   141,   222,   145,   332,   336,   147,   142,   143,   314,
     337,   144,   338,   339,   342,   344,     3,   354,   356,   355,
     146,   357,   145,   358,   140,   370,   371,   141,   386,   438,
     372,   373,   147,   142,   143,     3,   396,   144,   389,   146,
     397,   379,   412,   140,   376,   407,   141,   408,   145,   418,
     419,   147,   142,   143,   446,   439,   144,   440,   443,   267,
     445,     3,   447,   268,   269,   146,   456,   145,   460,   467,
     468,   270,   474,   479,   487,   478,   494,   147,   496,   501,
       3,   498,   267,   480,   146,   499,   268,   269,   271,   500,
      21,   272,   469,    43,   270,    66,   147,   502,    67,    68,
      41,    37,    33,    69,   509,    70,    71,   273,   102,   118,
     115,   271,    72,   401,   272,   116,   400,    73,   323,   228,
      74,   324,   189,   330,   481,    75,   453,    76,    77,   320,
     273,   486,    78,   164,   454,   507,    66,    79,     3,    67,
      68,   113,    80,   495,    69,   390,   117,    71,   387,   472,
     444,   398,   190,    72,   488,   220,   413,   415,    73,   414,
     416,    74,     0,     0,   417,     0,     0,     0,    76,    77,
       0,     0,     0,    78,     0,     0,     0,     0,    79,     0,
       0,     0,     0,    80
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-338)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      62,   106,   205,     0,   207,   208,   209,   206,     5,   110,
     211,   275,   276,   277,   278,   352,   128,    84,    40,    54,
       7,   351,     7,    85,    11,    12,    11,    12,    10,   361,
     362,   234,    19,     7,    19,     5,    54,    11,    26,    74,
      27,    23,    60,     8,   106,     8,    34,    35,     0,    36,
      98,    36,    39,    18,    39,    18,   192,    39,    36,    46,
      30,    43,    36,   130,    52,   113,    48,    64,    55,     7,
      55,    56,   273,    11,    12,    40,    63,    64,    63,    64,
      16,    19,   412,    70,    49,    70,    49,    74,   420,    74,
     422,    45,   228,   155,    66,    26,    23,    45,    36,    36,
      36,    39,    45,    75,    35,    55,    36,   174,   311,    36,
      60,   312,    39,    61,   317,    69,    70,    55,    55,   224,
      58,    52,    70,    53,   461,    63,    64,    70,   125,    36,
     127,    53,    70,    36,    36,    57,    74,    20,   339,   340,
     341,   253,    24,    50,   206,     7,    49,   350,    23,    11,
      12,    23,    53,    36,    55,    37,   153,    19,    41,   260,
     157,    33,   224,    38,    39,    53,    38,    39,   371,    57,
      54,   168,   373,   170,    36,     7,    60,    39,   175,    11,
      12,    36,    10,    36,    54,    55,    41,    19,   391,   392,
      60,    54,    54,    55,   397,    23,    57,   194,    59,    56,
      56,    63,    64,    36,    36,    62,    62,    39,    70,    53,
      53,    39,    74,    57,    57,    43,    54,   136,   482,   138,
      48,    49,    54,    55,    53,    36,    53,    36,    57,    53,
      57,    63,    64,    57,   435,    53,   439,    65,    70,    57,
     504,    53,    74,    56,    57,    57,   449,   450,   449,    53,
      53,   313,    11,    57,    57,    53,     7,   458,    53,   256,
      11,    12,    57,    56,    57,   370,    53,   372,    19,   468,
      57,    57,    56,    57,     7,    61,   479,    53,    11,    12,
      63,    64,   387,    56,    57,    36,    19,    55,    39,    55,
      56,    76,    77,    78,    56,    46,   499,   500,   367,   368,
     369,    56,    57,    36,    55,    53,    39,    53,   370,    53,
     372,    36,    63,    64,    56,    57,    56,    57,    53,    70,
      56,    57,    55,    74,    47,   387,    57,    60,     7,   434,
      63,    64,    11,    12,    49,   216,   217,    70,   339,   340,
      19,    74,   418,   419,   363,   364,    12,    55,    36,    15,
      29,   365,   366,    36,    32,    21,    22,    36,    67,    25,
      39,     7,   467,    36,   469,    11,    12,    53,   473,    36,
      36,    53,   434,    19,    36,    36,    55,    36,    36,    36,
      36,    36,    36,    49,    63,    64,    54,    53,    36,    36,
      36,    70,    55,    39,    36,    74,   501,    66,     7,    65,
      53,    53,    11,    12,    55,   467,   468,   469,     7,    55,
      19,   473,    11,    12,    55,    55,    55,    63,    64,    54,
      19,    33,    60,    12,    70,    51,    15,    36,    74,    36,
      39,    68,    21,    22,    55,    36,    25,    36,    36,   501,
      39,    36,    53,    53,    36,    36,    55,    36,    36,    12,
      53,    64,    15,    42,    63,    64,    55,     7,    21,    22,
      49,    70,    25,    36,    53,    74,    36,    36,    12,    36,
      36,    15,    61,    36,    36,    57,    65,    21,    22,    42,
      56,    25,    36,    55,    36,    53,    49,    71,    73,    72,
      53,    13,    36,    44,    12,    56,    53,    15,    42,    36,
      56,    56,    65,    21,    22,    49,    53,    25,    61,    53,
      55,    60,    71,    12,    62,    56,    15,    53,    36,    54,
      54,    65,    21,    22,    42,    54,    25,    11,     7,     7,
      62,    49,     7,    11,    12,    53,    53,    36,    62,     4,
      53,    19,    56,    54,    56,    61,    56,    65,     7,    56,
      49,    29,     7,    62,    53,    61,    11,    12,    36,    61,
       5,    39,    61,    29,    19,     6,    65,    62,     9,    10,
      28,    24,    18,    14,    29,    16,    17,    55,    65,    87,
      85,    36,    23,   337,    39,    85,   336,    28,   237,   164,
      31,   241,   125,   253,   448,    36,   392,    38,    39,   231,
      55,   450,    43,   107,   394,   500,     6,    48,    49,     9,
      10,    84,    53,   477,    14,   321,    16,    17,   313,   435,
     381,   334,   125,    23,   458,   153,   354,   356,    28,   355,
     357,    31,    -1,    -1,   358,    -1,    -1,    -1,    38,    39,
      -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    48,    -1,
      -1,    -1,    -1,    53
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     8,    18,    49,    80,    81,    82,    83,    88,    91,
      92,    95,    36,    36,    84,    20,    36,    41,    93,    94,
       0,    82,    40,    36,    89,    90,    26,    35,    52,    85,
      86,    54,    41,    94,    36,    54,    50,    90,    36,    87,
      36,    87,    34,    86,    11,    96,     7,    11,    36,    53,
      53,    53,    55,   101,    36,    53,    53,    53,    36,   102,
     103,   104,    97,    53,    56,    57,     6,     9,    10,    14,
      16,    17,    23,    28,    31,    36,    38,    39,    43,    48,
      53,    92,    98,    99,   100,   105,   106,   107,   108,   112,
     116,   121,   125,   126,   135,   136,   141,   142,   153,   177,
     178,    91,   104,    55,   124,    47,   154,   109,   110,   115,
     114,    32,    16,   153,   177,    99,   106,    16,   108,   113,
      10,    23,    39,    43,    48,   128,   129,   137,   111,    36,
     100,    67,   176,    53,    36,    53,    23,    36,    39,   127,
      12,    15,    21,    22,    25,    36,    53,    65,    91,    92,
     155,   157,   158,   159,   162,   163,   168,   169,   170,   171,
     172,   173,   181,   182,   136,    36,   139,   143,   145,   146,
      36,   118,   120,   118,   100,    36,   117,   119,    36,   131,
      36,   134,    36,   133,    36,   132,    36,   130,    91,   129,
     158,    91,   140,   144,   146,    54,    55,    36,    56,    57,
     127,   127,    53,    53,    55,    55,    55,    55,    55,    54,
      55,    60,    36,    55,    23,    33,    38,    39,   165,    36,
     162,    42,    61,   155,   164,    91,    66,    53,   139,    60,
      53,    57,   138,    91,    54,    91,    53,    57,    53,    91,
      53,    57,    53,    57,    53,    57,    53,    57,    53,    57,
      53,    57,    51,    57,   138,    91,    36,    68,   179,   180,
      55,    36,   122,   123,    36,    53,    53,     7,    11,    12,
      19,    36,    39,    55,    56,    63,    64,    70,    74,   156,
     183,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   183,    36,    92,   181,
     183,   183,   183,    36,   186,    36,    36,   166,   167,   166,
     166,    54,    60,    36,    42,   155,    53,    64,   138,     7,
     143,    58,   183,   120,   119,    36,    36,    36,    36,    36,
     144,    91,    36,    56,    57,   118,    57,    56,    36,    55,
      55,    60,    36,   186,    53,   200,   200,   200,   200,    56,
      57,    45,    69,    70,    71,    72,    73,    13,    44,    54,
      74,    66,    75,    24,    37,    63,    64,    76,    77,    78,
      56,    53,    56,    56,    56,    57,    62,    55,    56,    60,
      53,    57,    53,    53,   183,   186,    42,   164,   183,    61,
     156,     5,    30,   147,   148,   149,    53,    55,   180,    56,
     123,   122,   184,   185,   186,   184,   186,    56,    53,   183,
     189,   188,    71,   190,   191,   192,   193,   194,    54,    54,
      54,   196,    54,   196,   197,   197,   198,   198,   199,   199,
     199,   155,   183,   155,    46,   174,   175,   184,    36,    54,
      11,   160,   161,     7,   167,    62,    42,     7,    59,    55,
      60,   150,   183,   150,   149,   183,    53,    56,    57,    56,
      62,    61,   189,   195,   195,   196,   196,     4,    53,    61,
     155,    27,   175,    61,    56,   183,    56,    57,    61,    54,
      62,   147,    63,   151,   183,   186,   151,    56,   185,   188,
     155,   181,   155,   155,    56,   161,     7,   183,    29,    61,
      61,    56,    62,    29,    64,   152,   183,   152,   155,    29,
      56,    62,    56,    62
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
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
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
        break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
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


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}




/* The lookahead symbol.  */
int yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
/* Line 1792 of yacc.c  */
#line 296 "verilogaYacc.y"
    {
          }
    break;

  case 3:
/* Line 1792 of yacc.c  */
#line 301 "verilogaYacc.y"
    {
          }
    break;

  case 4:
/* Line 1792 of yacc.c  */
#line 304 "verilogaYacc.y"
    {
          }
    break;

  case 5:
/* Line 1792 of yacc.c  */
#line 309 "verilogaYacc.y"
    {
          }
    break;

  case 6:
/* Line 1792 of yacc.c  */
#line 312 "verilogaYacc.y"
    {
          }
    break;

  case 7:
/* Line 1792 of yacc.c  */
#line 315 "verilogaYacc.y"
    {
          }
    break;

  case 8:
/* Line 1792 of yacc.c  */
#line 320 "verilogaYacc.y"
    {
            adms_admsmain_list_discipline_prepend_once_or_abort(root(),gDiscipline);
            gDiscipline=NULL;
          }
    break;

  case 9:
/* Line 1792 of yacc.c  */
#line 327 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (1)]._lexval))->_string;
            gDiscipline=adms_discipline_new(mylexval1);
          }
    break;

  case 10:
/* Line 1792 of yacc.c  */
#line 334 "verilogaYacc.y"
    {
          }
    break;

  case 11:
/* Line 1792 of yacc.c  */
#line 337 "verilogaYacc.y"
    {
          }
    break;

  case 12:
/* Line 1792 of yacc.c  */
#line 342 "verilogaYacc.y"
    {
            gDiscipline->_potential=(p_nature)YY((yyvsp[(2) - (3)]._yaccval));
          }
    break;

  case 13:
/* Line 1792 of yacc.c  */
#line 346 "verilogaYacc.y"
    {
            gDiscipline->_flow=(p_nature)YY((yyvsp[(2) - (3)]._yaccval));
          }
    break;

  case 14:
/* Line 1792 of yacc.c  */
#line 350 "verilogaYacc.y"
    {
            char* mylexval2=((p_lexval)(yyvsp[(2) - (3)]._lexval))->_string;
            if(!strcmp(mylexval2,"discrete"))
              gDiscipline->_domain=admse_discrete;
            else if(!strcmp(mylexval2,"continuous"))
              gDiscipline->_domain=admse_continuous;
            else
             adms_veriloga_message_fatal("domain: bad value given - should be either 'discrete' or 'continuous'\n",(yyvsp[(2) - (3)]._lexval));
          }
    break;

  case 15:
/* Line 1792 of yacc.c  */
#line 362 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (1)]._lexval))->_string;
            p_nature mynature=lookup_nature(mylexval1);
            if(!mynature)
              adms_veriloga_message_fatal("can't find nature definition\n",(yyvsp[(1) - (1)]._lexval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mynature);
          }
    break;

  case 16:
/* Line 1792 of yacc.c  */
#line 373 "verilogaYacc.y"
    {
            char* mylexval2=((p_lexval)(yyvsp[(2) - (4)]._lexval))->_string;
            p_nature mynature=NULL;
            if(gNatureAccess) 
              mynature=adms_admsmain_list_nature_prepend_by_id_once_or_abort(root(),gNatureAccess);
            else
             adms_veriloga_message_fatal("attribute 'access' in nature definition not found\n",(yyvsp[(2) - (4)]._lexval));
            adms_nature_valueto_name(mynature,mylexval2);
            if(gNatureidt) 
              adms_nature_valueto_idt_name(mynature,gNatureidt);
            if(gNatureddt) 
              adms_nature_valueto_ddt_name(mynature,gNatureddt);
            if(gNatureUnits)
              mynature->_units=gNatureUnits;
            if(gNatureAbsTol)
              mynature->_abstol=gNatureAbsTol;
            gNatureAccess=NULL;
            gNatureAbsTol=NULL;
            gNatureUnits=NULL;
            gNatureidt=NULL;
            gNatureddt=NULL;
          }
    break;

  case 17:
/* Line 1792 of yacc.c  */
#line 398 "verilogaYacc.y"
    {
          }
    break;

  case 18:
/* Line 1792 of yacc.c  */
#line 401 "verilogaYacc.y"
    {
          }
    break;

  case 19:
/* Line 1792 of yacc.c  */
#line 406 "verilogaYacc.y"
    {
            if(!strcmp((yyvsp[(1) - (4)]._lexval)->_string,"abstol"))
            {
              if(gNatureAbsTol)
                adms_veriloga_message_fatal("nature attribute defined more than once\n",(yyvsp[(1) - (4)]._lexval));
              gNatureAbsTol=adms_number_new((yyvsp[(3) - (4)]._lexval));
            }
            else
             adms_veriloga_message_fatal("unknown nature attribute\n",(yyvsp[(1) - (4)]._lexval));
          }
    break;

  case 20:
/* Line 1792 of yacc.c  */
#line 417 "verilogaYacc.y"
    {
            char* mylexval4=((p_lexval)(yyvsp[(4) - (5)]._lexval))->_string;
            admse myunit=admse_1;
            if(!strcmp((yyvsp[(1) - (5)]._lexval)->_string,"abstol"))
            {
              if(gNatureAbsTol)
                adms_veriloga_message_fatal("nature attribute defined more than once\n",(yyvsp[(1) - (5)]._lexval));
              gNatureAbsTol=adms_number_new((yyvsp[(3) - (5)]._lexval));
            }
            else
             adms_veriloga_message_fatal("unknown nature attribute\n",(yyvsp[(1) - (5)]._lexval));
            if(0) {}
            else if(!strcmp(mylexval4,"E")) myunit=admse_E;
            else if(!strcmp(mylexval4,"P")) myunit=admse_P;
            else if(!strcmp(mylexval4,"T")) myunit=admse_T;
            else if(!strcmp(mylexval4,"G")) myunit=admse_G;
            else if(!strcmp(mylexval4,"M")) myunit=admse_M;
            else if(!strcmp(mylexval4,"K")) myunit=admse_K;
            else if(!strcmp(mylexval4,"k")) myunit=admse_k;
            else if(!strcmp(mylexval4,"h")) myunit=admse_h;
            else if(!strcmp(mylexval4,"D")) myunit=admse_D;
            else if(!strcmp(mylexval4,"d")) myunit=admse_d;
            else if(!strcmp(mylexval4,"c")) myunit=admse_c;
            else if(!strcmp(mylexval4,"m")) myunit=admse_m;
            else if(!strcmp(mylexval4,"u")) myunit=admse_u;
            else if(!strcmp(mylexval4,"n")) myunit=admse_n;
            else if(!strcmp(mylexval4,"A")) myunit=admse_A;
            else if(!strcmp(mylexval4,"p")) myunit=admse_p;
            else if(!strcmp(mylexval4,"f")) myunit=admse_f;
            else if(!strcmp(mylexval4,"a")) myunit=admse_a;
            else
              adms_veriloga_message_fatal("can not convert symbol to valid unit\n",(yyvsp[(4) - (5)]._lexval));
            gNatureAbsTol->_scalingunit=myunit;
          }
    break;

  case 21:
/* Line 1792 of yacc.c  */
#line 452 "verilogaYacc.y"
    {
            char* mylexval3=((p_lexval)(yyvsp[(3) - (4)]._lexval))->_string;
            if(!strcmp((yyvsp[(1) - (4)]._lexval)->_string,"units"))
            {
              if(gNatureUnits)
                adms_veriloga_message_fatal("nature attribute defined more than once\n",(yyvsp[(1) - (4)]._lexval));
              gNatureUnits=adms_kclone(mylexval3);
            }
            else
             adms_veriloga_message_fatal("unknown nature attribute\n",(yyvsp[(1) - (4)]._lexval));
          }
    break;

  case 22:
/* Line 1792 of yacc.c  */
#line 464 "verilogaYacc.y"
    {
            char* mylexval3=((p_lexval)(yyvsp[(3) - (4)]._lexval))->_string;
            if(!strcmp((yyvsp[(1) - (4)]._lexval)->_string,"access"))
            {
              if(gNatureAccess)
                adms_veriloga_message_fatal("nature attribute defined more than once\n",(yyvsp[(1) - (4)]._lexval));
              gNatureAccess=adms_kclone(mylexval3);
            }
            else if(!strcmp((yyvsp[(1) - (4)]._lexval)->_string,"idt_nature"))
            {
              if(gNatureidt)
                adms_veriloga_message_fatal("idt_nature attribute defined more than once\n",(yyvsp[(1) - (4)]._lexval));
              gNatureidt=adms_kclone(mylexval3);
            }
            else if(!strcmp((yyvsp[(1) - (4)]._lexval)->_string,"ddt_nature"))
            {
              if(gNatureddt)
                adms_veriloga_message_fatal("ddt_nature attribute defined more than once\n",(yyvsp[(1) - (4)]._lexval));
              gNatureddt=adms_kclone(mylexval3);
            }
            else
             adms_veriloga_message_fatal("unknown nature attribute\n",(yyvsp[(1) - (4)]._lexval));
          }
    break;

  case 23:
/* Line 1792 of yacc.c  */
#line 490 "verilogaYacc.y"
    {
          }
    break;

  case 24:
/* Line 1792 of yacc.c  */
#line 493 "verilogaYacc.y"
    {
          }
    break;

  case 25:
/* Line 1792 of yacc.c  */
#line 498 "verilogaYacc.y"
    {
          }
    break;

  case 26:
/* Line 1792 of yacc.c  */
#line 501 "verilogaYacc.y"
    {
            char* mylexval2=((p_lexval)(yyvsp[(2) - (2)]._lexval))->_string;
            p_attribute myattribute=adms_attribute_new("ibm");
            p_admst myconstant=adms_admst_newks(adms_kclone(mylexval2));
            myattribute->_value=(p_adms)myconstant;
            adms_slist_push(&gAttributeList,(p_adms)myattribute);
          }
    break;

  case 27:
/* Line 1792 of yacc.c  */
#line 509 "verilogaYacc.y"
    {
          }
    break;

  case 28:
/* Line 1792 of yacc.c  */
#line 514 "verilogaYacc.y"
    {
          }
    break;

  case 29:
/* Line 1792 of yacc.c  */
#line 517 "verilogaYacc.y"
    {
          }
    break;

  case 30:
/* Line 1792 of yacc.c  */
#line 522 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (3)]._lexval))->_string;
            char* mylexval3=((p_lexval)(yyvsp[(3) - (3)]._lexval))->_string;
            p_attribute myattribute=adms_attribute_new(mylexval1);
            p_admst myconstant=adms_admst_newks(adms_kclone(mylexval3));
            myattribute->_value=(p_adms)myconstant;
            adms_slist_push(&gAttributeList,(p_adms)myattribute);
          }
    break;

  case 31:
/* Line 1792 of yacc.c  */
#line 533 "verilogaYacc.y"
    {
            char* mylexval3=((p_lexval)(yyvsp[(3) - (3)]._lexval))->_string;
            p_slist l;
            p_nodealias mynodealias;
            gModule=adms_admsmain_list_module_prepend_by_id_once_or_abort(root(),mylexval3); 
            adms_slist_push(&gBlockList,(p_adms)gModule);
            mynodealias=adms_module_list_nodealias_prepend_by_id_once_or_abort(gModule,gModule,"0"); 
            gGND=adms_module_list_node_prepend_by_id_once_or_abort(gModule,gModule,"GND"); 
            mynodealias->_node=gGND;
            gGND->_location=admse_ground;
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&gModule->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
          }
    break;

  case 32:
/* Line 1792 of yacc.c  */
#line 548 "verilogaYacc.y"
    {
            set_context(ctx_moduletop);
          }
    break;

  case 33:
/* Line 1792 of yacc.c  */
#line 552 "verilogaYacc.y"
    {
            adms_slist_pull(&gBlockList);
            adms_slist_inreverse(&gModule->_assignment);
          }
    break;

  case 34:
/* Line 1792 of yacc.c  */
#line 559 "verilogaYacc.y"
    {
          }
    break;

  case 35:
/* Line 1792 of yacc.c  */
#line 562 "verilogaYacc.y"
    {
          }
    break;

  case 36:
/* Line 1792 of yacc.c  */
#line 565 "verilogaYacc.y"
    {
          }
    break;

  case 37:
/* Line 1792 of yacc.c  */
#line 568 "verilogaYacc.y"
    {
          }
    break;

  case 38:
/* Line 1792 of yacc.c  */
#line 573 "verilogaYacc.y"
    {
          }
    break;

  case 39:
/* Line 1792 of yacc.c  */
#line 576 "verilogaYacc.y"
    {
          }
    break;

  case 40:
/* Line 1792 of yacc.c  */
#line 579 "verilogaYacc.y"
    {
          }
    break;

  case 41:
/* Line 1792 of yacc.c  */
#line 582 "verilogaYacc.y"
    {
          }
    break;

  case 42:
/* Line 1792 of yacc.c  */
#line 585 "verilogaYacc.y"
    {
          }
    break;

  case 43:
/* Line 1792 of yacc.c  */
#line 590 "verilogaYacc.y"
    {
          }
    break;

  case 44:
/* Line 1792 of yacc.c  */
#line 593 "verilogaYacc.y"
    {
          }
    break;

  case 45:
/* Line 1792 of yacc.c  */
#line 598 "verilogaYacc.y"
    {
            p_slist l;
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&gModule->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
          }
    break;

  case 46:
/* Line 1792 of yacc.c  */
#line 607 "verilogaYacc.y"
    {
          }
    break;

  case 47:
/* Line 1792 of yacc.c  */
#line 610 "verilogaYacc.y"
    {
          }
    break;

  case 48:
/* Line 1792 of yacc.c  */
#line 615 "verilogaYacc.y"
    {
          }
    break;

  case 49:
/* Line 1792 of yacc.c  */
#line 618 "verilogaYacc.y"
    {
          }
    break;

  case 50:
/* Line 1792 of yacc.c  */
#line 623 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (1)]._lexval))->_string;
            p_nodealias mynodealias=adms_module_list_nodealias_prepend_by_id_once_or_abort(gModule,gModule,mylexval1); 
            p_node mynode=adms_module_list_node_prepend_by_id_once_or_abort(gModule,gModule,mylexval1); 
            mynodealias->_node=mynode;
            mynode->_location=admse_external;
          }
    break;

  case 51:
/* Line 1792 of yacc.c  */
#line 633 "verilogaYacc.y"
    {
          }
    break;

  case 52:
/* Line 1792 of yacc.c  */
#line 636 "verilogaYacc.y"
    {
          }
    break;

  case 53:
/* Line 1792 of yacc.c  */
#line 641 "verilogaYacc.y"
    {
            set_context(ctx_moduletop);
          }
    break;

  case 54:
/* Line 1792 of yacc.c  */
#line 645 "verilogaYacc.y"
    {
            adms_slist_free(gGlobalAttributeList); gGlobalAttributeList=NULL;
            set_context(ctx_moduletop);
          }
    break;

  case 55:
/* Line 1792 of yacc.c  */
#line 652 "verilogaYacc.y"
    {
            gGlobalAttributeList=gAttributeList;
            gAttributeList=NULL;
          }
    break;

  case 56:
/* Line 1792 of yacc.c  */
#line 659 "verilogaYacc.y"
    {
          }
    break;

  case 57:
/* Line 1792 of yacc.c  */
#line 662 "verilogaYacc.y"
    {
          }
    break;

  case 58:
/* Line 1792 of yacc.c  */
#line 665 "verilogaYacc.y"
    {
            set_context(ctx_any);
          }
    break;

  case 59:
/* Line 1792 of yacc.c  */
#line 669 "verilogaYacc.y"
    {
          }
    break;

  case 60:
/* Line 1792 of yacc.c  */
#line 672 "verilogaYacc.y"
    {
            set_context(ctx_any);
          }
    break;

  case 61:
/* Line 1792 of yacc.c  */
#line 676 "verilogaYacc.y"
    {
          }
    break;

  case 62:
/* Line 1792 of yacc.c  */
#line 679 "verilogaYacc.y"
    {
            set_context(ctx_any);
          }
    break;

  case 63:
/* Line 1792 of yacc.c  */
#line 683 "verilogaYacc.y"
    {
          }
    break;

  case 64:
/* Line 1792 of yacc.c  */
#line 686 "verilogaYacc.y"
    {
          }
    break;

  case 65:
/* Line 1792 of yacc.c  */
#line 689 "verilogaYacc.y"
    {
          }
    break;

  case 66:
/* Line 1792 of yacc.c  */
#line 692 "verilogaYacc.y"
    {
          }
    break;

  case 67:
/* Line 1792 of yacc.c  */
#line 697 "verilogaYacc.y"
    {
            set_context(ctx_any);
          }
    break;

  case 68:
/* Line 1792 of yacc.c  */
#line 701 "verilogaYacc.y"
    {
            p_slist l;
            for(l=gTerminalList;l;l=l->next)
              ((p_node)l->data)->_direction=gNodeDirection;
            adms_slist_free(gTerminalList); gTerminalList=NULL;
          }
    break;

  case 69:
/* Line 1792 of yacc.c  */
#line 708 "verilogaYacc.y"
    {
            set_context(ctx_any);
          }
    break;

  case 70:
/* Line 1792 of yacc.c  */
#line 712 "verilogaYacc.y"
    {
            p_slist l;
            for(l=gNodeList;l;l=l->next)
              ((p_node)l->data)->_location=admse_ground;
            adms_slist_free(gNodeList); gNodeList=NULL;
          }
    break;

  case 71:
/* Line 1792 of yacc.c  */
#line 719 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (1)]._lexval))->_string;
            set_context(ctx_any);
            gDisc=mylexval1;
          }
    break;

  case 72:
/* Line 1792 of yacc.c  */
#line 725 "verilogaYacc.y"
    {
            char* mydisciplinename=gDisc;
            p_discipline mydiscipline=adms_admsmain_list_discipline_lookup_by_id(root(),mydisciplinename);
            p_slist l;
            for(l=gNodeList;l;l=l->next)
              ((p_node)l->data)->_discipline=mydiscipline;
            adms_slist_free(gNodeList); gNodeList=NULL;
          }
    break;

  case 73:
/* Line 1792 of yacc.c  */
#line 736 "verilogaYacc.y"
    {
            gNodeDirection=admse_input;
          }
    break;

  case 74:
/* Line 1792 of yacc.c  */
#line 740 "verilogaYacc.y"
    {
            gNodeDirection=admse_output;
          }
    break;

  case 75:
/* Line 1792 of yacc.c  */
#line 744 "verilogaYacc.y"
    {
            gNodeDirection=admse_inout;
          }
    break;

  case 76:
/* Line 1792 of yacc.c  */
#line 750 "verilogaYacc.y"
    {
          }
    break;

  case 77:
/* Line 1792 of yacc.c  */
#line 753 "verilogaYacc.y"
    {
          }
    break;

  case 78:
/* Line 1792 of yacc.c  */
#line 758 "verilogaYacc.y"
    {
          }
    break;

  case 79:
/* Line 1792 of yacc.c  */
#line 761 "verilogaYacc.y"
    {
          }
    break;

  case 80:
/* Line 1792 of yacc.c  */
#line 766 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (2)]._lexval))->_string;
            p_slist l;
            p_node mynode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval1);
            if(!mynode)
             adms_veriloga_message_fatal("terminal not found\n",(yyvsp[(1) - (2)]._lexval));
            if(mynode->_location!=admse_external)
             adms_veriloga_message_fatal("node not a terminal\n",(yyvsp[(1) - (2)]._lexval));
            adms_slist_push(&gTerminalList,(p_adms)mynode);
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&mynode->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&mynode->_attribute,l->data);
          }
    break;

  case 81:
/* Line 1792 of yacc.c  */
#line 784 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (2)]._lexval))->_string;
            p_slist l;
            p_node mynode=adms_module_list_node_prepend_by_id_once_or_ignore(gModule,gModule,mylexval1);
            adms_slist_push(&gNodeList,(p_adms)mynode);
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&mynode->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&mynode->_attribute,l->data);
          }
    break;

  case 82:
/* Line 1792 of yacc.c  */
#line 798 "verilogaYacc.y"
    {
          }
    break;

  case 83:
/* Line 1792 of yacc.c  */
#line 803 "verilogaYacc.y"
    {
          }
    break;

  case 84:
/* Line 1792 of yacc.c  */
#line 806 "verilogaYacc.y"
    {
          }
    break;

  case 85:
/* Line 1792 of yacc.c  */
#line 811 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (1)]._lexval))->_string;
            adms_slist_push(&gBranchAliasList,(p_adms)mylexval1);
          }
    break;

  case 86:
/* Line 1792 of yacc.c  */
#line 818 "verilogaYacc.y"
    {
            char* mylexval2=((p_lexval)(yyvsp[(2) - (6)]._lexval))->_string;
            char* mylexval4=((p_lexval)(yyvsp[(4) - (6)]._lexval))->_string;
            p_slist l;
            p_branch mybranch; 
            p_node pnode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval2);
            p_node nnode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval4);
            mybranch=adms_module_list_branch_prepend_by_id_once_or_ignore(gModule,gModule,pnode,nnode); 
            if(!pnode)
             adms_veriloga_message_fatal("node never declared\n",(yyvsp[(2) - (6)]._lexval));
            if(!nnode)
             adms_veriloga_message_fatal("node never declared\n",(yyvsp[(4) - (6)]._lexval));
            for(l=gBranchAliasList;l;l=l->next)
            {
              char*aliasname=(char*)l->data;
              p_branchalias mybranchalias; 
              mybranchalias=adms_module_list_branchalias_prepend_by_id_once_or_abort(gModule,gModule,aliasname); 
              if(mybranchalias) mybranchalias->_branch=mybranch;
            }
            adms_slist_free(gBranchAliasList);
            gBranchAliasList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&mybranch->_attribute,l->data);
          }
    break;

  case 87:
/* Line 1792 of yacc.c  */
#line 843 "verilogaYacc.y"
    {
            char* mylexval2=((p_lexval)(yyvsp[(2) - (4)]._lexval))->_string;
            p_slist l;
            p_branch mybranch;
            p_node pnode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval2);
            if(!pnode)
             adms_veriloga_message_fatal("node never declared\n",(yyvsp[(2) - (4)]._lexval));
            mybranch=adms_module_list_branch_prepend_by_id_once_or_ignore(gModule,gModule,pnode,gGND); 
            for(l=gBranchAliasList;l;l=l->next)
            {
              char*aliasname=(char*)l->data;
              p_branchalias mybranchalias; 
              mybranchalias=adms_module_list_branchalias_prepend_by_id_once_or_abort(gModule,gModule,aliasname); 
              if(mybranchalias) mybranchalias->_branch=mybranch;
            }
            adms_slist_free(gBranchAliasList);
            gBranchAliasList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&mybranch->_attribute,l->data);
          }
    break;

  case 88:
/* Line 1792 of yacc.c  */
#line 866 "verilogaYacc.y"
    {
            adms_slist_pull(&gBlockList);
            gAnalogfunction->_tree=YY((yyvsp[(3) - (4)]._yaccval));
            gAnalogfunction=NULL;
          }
    break;

  case 89:
/* Line 1792 of yacc.c  */
#line 874 "verilogaYacc.y"
    {
            NEWVARIABLE(gAnalogfunction->_lexval)
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_output=admse_yes;
          }
    break;

  case 90:
/* Line 1792 of yacc.c  */
#line 880 "verilogaYacc.y"
    {
            NEWVARIABLE(gAnalogfunction->_lexval)
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_output=admse_yes;
            myvariableprototype->_type=admse_integer;
            gAnalogfunction->_type=admse_integer; 
          }
    break;

  case 91:
/* Line 1792 of yacc.c  */
#line 888 "verilogaYacc.y"
    {
            NEWVARIABLE(gAnalogfunction->_lexval)
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_output=admse_yes;
          }
    break;

  case 92:
/* Line 1792 of yacc.c  */
#line 896 "verilogaYacc.y"
    {
            p_slist l;
            gAnalogfunction=adms_analogfunction_new(gModule,(yyvsp[(1) - (1)]._lexval));
            adms_slist_push(&gBlockList,(p_adms)gAnalogfunction);
            adms_module_list_analogfunction_prepend_once_or_abort(gModule,gAnalogfunction); 
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&gAnalogfunction->_attribute,l->data);
          }
    break;

  case 93:
/* Line 1792 of yacc.c  */
#line 907 "verilogaYacc.y"
    {
          }
    break;

  case 94:
/* Line 1792 of yacc.c  */
#line 910 "verilogaYacc.y"
    {
          }
    break;

  case 95:
/* Line 1792 of yacc.c  */
#line 915 "verilogaYacc.y"
    {
          }
    break;

  case 96:
/* Line 1792 of yacc.c  */
#line 918 "verilogaYacc.y"
    {
          }
    break;

  case 97:
/* Line 1792 of yacc.c  */
#line 921 "verilogaYacc.y"
    {
          }
    break;

  case 98:
/* Line 1792 of yacc.c  */
#line 924 "verilogaYacc.y"
    {
          }
    break;

  case 99:
/* Line 1792 of yacc.c  */
#line 927 "verilogaYacc.y"
    {
          }
    break;

  case 100:
/* Line 1792 of yacc.c  */
#line 932 "verilogaYacc.y"
    {
            NEWVARIABLE((yyvsp[(1) - (1)]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_input=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
    break;

  case 101:
/* Line 1792 of yacc.c  */
#line 939 "verilogaYacc.y"
    {
            NEWVARIABLE((yyvsp[(3) - (3)]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_input=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
    break;

  case 102:
/* Line 1792 of yacc.c  */
#line 948 "verilogaYacc.y"
    {
            NEWVARIABLE((yyvsp[(1) - (1)]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_output=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
    break;

  case 103:
/* Line 1792 of yacc.c  */
#line 955 "verilogaYacc.y"
    {
            NEWVARIABLE((yyvsp[(3) - (3)]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_output=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
    break;

  case 104:
/* Line 1792 of yacc.c  */
#line 964 "verilogaYacc.y"
    {
            NEWVARIABLE((yyvsp[(1) - (1)]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_input=admse_yes;
            myvariableprototype->_output=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
    break;

  case 105:
/* Line 1792 of yacc.c  */
#line 972 "verilogaYacc.y"
    {
            NEWVARIABLE((yyvsp[(3) - (3)]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_input=admse_yes;
            myvariableprototype->_output=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
    break;

  case 106:
/* Line 1792 of yacc.c  */
#line 982 "verilogaYacc.y"
    {
            p_variableprototype myvariableprototype=adms_analogfunction_list_variable_lookup_by_id(gAnalogfunction,gModule,(yyvsp[(1) - (1)]._lexval),(p_adms)gAnalogfunction);
            if(myvariableprototype)
              myvariableprototype->_type=admse_integer;
            else
            {
              NEWVARIABLE((yyvsp[(1) - (1)]._lexval))
              adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
              myvariableprototype->_type=admse_integer;
            }
          }
    break;

  case 107:
/* Line 1792 of yacc.c  */
#line 994 "verilogaYacc.y"
    {
            p_variableprototype myvariableprototype=adms_analogfunction_list_variable_lookup_by_id(gAnalogfunction,gModule,(yyvsp[(3) - (3)]._lexval),(p_adms)gAnalogfunction);
            if(myvariableprototype)
              myvariableprototype->_type=admse_integer;
            else
            {
              NEWVARIABLE((yyvsp[(3) - (3)]._lexval))
              adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
              myvariableprototype->_type=admse_integer;
            }
          }
    break;

  case 108:
/* Line 1792 of yacc.c  */
#line 1008 "verilogaYacc.y"
    {
            p_variableprototype myvariableprototype=adms_analogfunction_list_variable_lookup_by_id(gAnalogfunction,gModule,(yyvsp[(1) - (1)]._lexval),(p_adms)gAnalogfunction);
            if(myvariableprototype)
              myvariableprototype->_type=admse_real;
            else
            {
              NEWVARIABLE((yyvsp[(1) - (1)]._lexval))
              adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
              myvariableprototype->_type=admse_real;
            }
          }
    break;

  case 109:
/* Line 1792 of yacc.c  */
#line 1020 "verilogaYacc.y"
    {
            p_variableprototype myvariableprototype=adms_analogfunction_list_variable_lookup_by_id(gAnalogfunction,gModule,(yyvsp[(3) - (3)]._lexval),(p_adms)gAnalogfunction);
            if(myvariableprototype)
              myvariableprototype->_type=admse_real;
            else
            {
              NEWVARIABLE((yyvsp[(3) - (3)]._lexval))
              adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
              myvariableprototype->_type=admse_real;
            }
          }
    break;

  case 110:
/* Line 1792 of yacc.c  */
#line 1034 "verilogaYacc.y"
    {
            gVariableType=admse_integer;
          }
    break;

  case 111:
/* Line 1792 of yacc.c  */
#line 1038 "verilogaYacc.y"
    {
            gVariableType=admse_real;
          }
    break;

  case 112:
/* Line 1792 of yacc.c  */
#line 1042 "verilogaYacc.y"
    {
            gVariableType=admse_string;
          }
    break;

  case 113:
/* Line 1792 of yacc.c  */
#line 1048 "verilogaYacc.y"
    {
            set_context(ctx_any);
          }
    break;

  case 114:
/* Line 1792 of yacc.c  */
#line 1052 "verilogaYacc.y"
    {
            adms_slist_concat(&gGlobalAttributeList,gAttributeList);
            gAttributeList=NULL;
          }
    break;

  case 115:
/* Line 1792 of yacc.c  */
#line 1059 "verilogaYacc.y"
    {
            p_slist l;
            for(l=gVariableDeclarationList;l;l=l->next)
              ((p_variableprototype)l->data)->_type=gVariableType;
            adms_slist_free(gVariableDeclarationList); gVariableDeclarationList=NULL;
          }
    break;

  case 116:
/* Line 1792 of yacc.c  */
#line 1068 "verilogaYacc.y"
    {
          }
    break;

  case 117:
/* Line 1792 of yacc.c  */
#line 1071 "verilogaYacc.y"
    {
          }
    break;

  case 118:
/* Line 1792 of yacc.c  */
#line 1076 "verilogaYacc.y"
    {
          }
    break;

  case 119:
/* Line 1792 of yacc.c  */
#line 1079 "verilogaYacc.y"
    {
          }
    break;

  case 120:
/* Line 1792 of yacc.c  */
#line 1084 "verilogaYacc.y"
    {
            char* mylexval2=((p_lexval)(yyvsp[(2) - (6)]._lexval))->_string;
            p_slist l;
            p_variableprototype myvariableprototype=adms_module_list_variable_lookup_by_id(gModule,gModule,(yyvsp[(4) - (6)]._lexval),(p_adms)gModule);
            if(!myvariableprototype)
             adms_veriloga_message_fatal("variable never declared\n",(yyvsp[(4) - (6)]._lexval));
            adms_variableprototype_list_alias_prepend_once_or_abort(myvariableprototype,adms_kclone(mylexval2));
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&myvariableprototype->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&myvariableprototype->_attribute,l->data);
          }
    break;

  case 121:
/* Line 1792 of yacc.c  */
#line 1100 "verilogaYacc.y"
    {
          }
    break;

  case 122:
/* Line 1792 of yacc.c  */
#line 1103 "verilogaYacc.y"
    {
          }
    break;

  case 123:
/* Line 1792 of yacc.c  */
#line 1108 "verilogaYacc.y"
    {
            p_slist l;
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&((p_variableprototype)gVariableDeclarationList->data)->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&((p_variableprototype)gVariableDeclarationList->data)->_attribute,l->data);
          }
    break;

  case 124:
/* Line 1792 of yacc.c  */
#line 1119 "verilogaYacc.y"
    {
            p_slist l;
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&((p_variableprototype)gVariableDeclarationList->data)->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&((p_variableprototype)gVariableDeclarationList->data)->_attribute,l->data);
          }
    break;

  case 125:
/* Line 1792 of yacc.c  */
#line 1130 "verilogaYacc.y"
    {
            ((p_variableprototype)gVariableDeclarationList->data)->_input=admse_yes;
            ((p_variableprototype)gVariableDeclarationList->data)->_default=((p_expression)YY((yyvsp[(3) - (4)]._yaccval)));
            ((p_variableprototype)gVariableDeclarationList->data)->_range=adms_slist_reverse(gRangeList);
            gRangeList=NULL;
          }
    break;

  case 126:
/* Line 1792 of yacc.c  */
#line 1137 "verilogaYacc.y"
    {
            p_slist myArgs=(p_slist)YY((yyvsp[(4) - (6)]._yaccval));
            adms_slist_inreverse(&myArgs);
            ((p_variableprototype)gVariableDeclarationList->data)->_input=admse_yes;
            ((p_variableprototype)gVariableDeclarationList->data)->_default=((p_expression)myArgs->data);
            ((p_variableprototype)gVariableDeclarationList->data)->_arraydefault=myArgs;
            ((p_variableprototype)gVariableDeclarationList->data)->_range=adms_slist_reverse(gRangeList);
            gRangeList=NULL;
          }
    break;

  case 127:
/* Line 1792 of yacc.c  */
#line 1149 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (1)]._lexval))->_string;
            NEWVARIABLE((yyvsp[(1) - (1)]._lexval))
            if(adms_module_list_node_lookup_by_id(gModule,gModule,mylexval1))
             adms_veriloga_message_fatal("variable already defined as node\n",(yyvsp[(1) - (1)]._lexval));
            adms_module_list_variable_prepend_once_or_abort(gModule,myvariableprototype); 
            adms_slist_push(&gVariableDeclarationList,(p_adms)myvariableprototype);
          }
    break;

  case 128:
/* Line 1792 of yacc.c  */
#line 1158 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (6)]._lexval))->_string;
            NEWVARIABLE((yyvsp[(1) - (6)]._lexval))
            if(adms_module_list_node_lookup_by_id(gModule,gModule,mylexval1))
             adms_veriloga_message_fatal("variable already defined as node\n",(yyvsp[(1) - (6)]._lexval));
            adms_module_list_variable_prepend_once_or_abort(gModule,myvariableprototype); 
            adms_slist_push(&gVariableDeclarationList,(p_adms)myvariableprototype);
            myvariableprototype->_sizetype=admse_array;
            myvariableprototype->_minsize=adms_number_new((yyvsp[(3) - (6)]._lexval));
            myvariableprototype->_maxsize=adms_number_new((yyvsp[(5) - (6)]._lexval));
          }
    break;

  case 129:
/* Line 1792 of yacc.c  */
#line 1172 "verilogaYacc.y"
    {
          }
    break;

  case 130:
/* Line 1792 of yacc.c  */
#line 1175 "verilogaYacc.y"
    {
          }
    break;

  case 131:
/* Line 1792 of yacc.c  */
#line 1180 "verilogaYacc.y"
    {
          }
    break;

  case 132:
/* Line 1792 of yacc.c  */
#line 1183 "verilogaYacc.y"
    {
          }
    break;

  case 133:
/* Line 1792 of yacc.c  */
#line 1188 "verilogaYacc.y"
    {
            if(((p_range)YY((yyvsp[(2) - (2)]._yaccval)))->_infboundtype==admse_range_bound_value)
              ((p_range)YY((yyvsp[(2) - (2)]._yaccval)))->_type=admse_include_value;
            else
              ((p_range)YY((yyvsp[(2) - (2)]._yaccval)))->_type=admse_include;
            adms_slist_push(&gRangeList,YY((yyvsp[(2) - (2)]._yaccval)));
          }
    break;

  case 134:
/* Line 1792 of yacc.c  */
#line 1196 "verilogaYacc.y"
    {
            if(((p_range)YY((yyvsp[(2) - (2)]._yaccval)))->_infboundtype==admse_range_bound_value)
              ((p_range)YY((yyvsp[(2) - (2)]._yaccval)))->_type=admse_exclude_value;
            else
              ((p_range)YY((yyvsp[(2) - (2)]._yaccval)))->_type=admse_exclude;
            adms_slist_push(&gRangeList,YY((yyvsp[(2) - (2)]._yaccval)));
          }
    break;

  case 135:
/* Line 1792 of yacc.c  */
#line 1206 "verilogaYacc.y"
    {
            p_range myrange=adms_module_list_range_prepend_by_id_once_or_abort(gModule,gModule,(p_expression)YY((yyvsp[(2) - (5)]._yaccval)),(p_expression)YY((yyvsp[(4) - (5)]._yaccval))); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myrange->_infboundtype=admse_range_bound_exclude;
            myrange->_supboundtype=admse_range_bound_exclude;
            Y((yyval._yaccval),(p_adms)myrange);
          }
    break;

  case 136:
/* Line 1792 of yacc.c  */
#line 1214 "verilogaYacc.y"
    {
            p_range myrange=adms_module_list_range_prepend_by_id_once_or_abort(gModule,gModule,(p_expression)YY((yyvsp[(2) - (5)]._yaccval)),(p_expression)YY((yyvsp[(4) - (5)]._yaccval))); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myrange->_infboundtype=admse_range_bound_exclude;
            myrange->_supboundtype=admse_range_bound_include;
            Y((yyval._yaccval),(p_adms)myrange);
          }
    break;

  case 137:
/* Line 1792 of yacc.c  */
#line 1222 "verilogaYacc.y"
    {
            p_range myrange=adms_module_list_range_prepend_by_id_once_or_abort(gModule,gModule,(p_expression)YY((yyvsp[(2) - (5)]._yaccval)),(p_expression)YY((yyvsp[(4) - (5)]._yaccval))); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myrange->_infboundtype=admse_range_bound_include;
            myrange->_supboundtype=admse_range_bound_exclude;
            Y((yyval._yaccval),(p_adms)myrange);
          }
    break;

  case 138:
/* Line 1792 of yacc.c  */
#line 1230 "verilogaYacc.y"
    {
            p_range myrange=adms_module_list_range_prepend_by_id_once_or_abort(gModule,gModule,(p_expression)YY((yyvsp[(2) - (5)]._yaccval)),(p_expression)YY((yyvsp[(4) - (5)]._yaccval))); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myrange->_infboundtype=admse_range_bound_include;
            myrange->_supboundtype=admse_range_bound_include;
            Y((yyval._yaccval),(p_adms)myrange);
          }
    break;

  case 139:
/* Line 1792 of yacc.c  */
#line 1238 "verilogaYacc.y"
    {
            p_range myrange=adms_module_list_range_prepend_by_id_once_or_abort(gModule,gModule,(p_expression)YY((yyvsp[(1) - (1)]._yaccval)),(p_expression)YY((yyvsp[(1) - (1)]._yaccval))); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myrange->_infboundtype=admse_range_bound_value;
            myrange->_supboundtype=admse_range_bound_value;
            Y((yyval._yaccval),(p_adms)myrange);
          }
    break;

  case 140:
/* Line 1792 of yacc.c  */
#line 1248 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 141:
/* Line 1792 of yacc.c  */
#line 1252 "verilogaYacc.y"
    {
            p_number mynumber=adms_number_new((yyvsp[(2) - (2)]._lexval)); 
            p_expression myexpression=adms_expression_new(gModule,(p_adms)mynumber); 
            mynumber->_lexval->_string=adms_kclone("-inf");
            adms_slist_push(&gModule->_expression,(p_adms)myexpression); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myexpression->_infinity=admse_minus;
            myexpression->_hasspecialnumber=adms_kclone("YES");
            Y((yyval._yaccval),(p_adms)myexpression);
          }
    break;

  case 142:
/* Line 1792 of yacc.c  */
#line 1265 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 143:
/* Line 1792 of yacc.c  */
#line 1269 "verilogaYacc.y"
    {
            p_number mynumber=adms_number_new((yyvsp[(1) - (1)]._lexval)); 
            p_expression myexpression=adms_expression_new(gModule,(p_adms)mynumber); 
            mynumber->_lexval->_string=adms_kclone("+inf");
            adms_slist_push(&gModule->_expression,(p_adms)myexpression); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myexpression->_infinity=admse_plus;
            myexpression->_hasspecialnumber=adms_kclone("YES");
            Y((yyval._yaccval),(p_adms)myexpression);
          }
    break;

  case 144:
/* Line 1792 of yacc.c  */
#line 1280 "verilogaYacc.y"
    {
            p_number mynumber=adms_number_new((yyvsp[(2) - (2)]._lexval)); 
            p_expression myexpression=adms_expression_new(gModule,(p_adms)mynumber); 
            mynumber->_lexval->_string=adms_kclone("+inf");
            adms_slist_push(&gModule->_expression,(p_adms)myexpression); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myexpression->_infinity=admse_plus;
            myexpression->_hasspecialnumber=adms_kclone("YES");
            Y((yyval._yaccval),(p_adms)myexpression);
          }
    break;

  case 145:
/* Line 1792 of yacc.c  */
#line 1293 "verilogaYacc.y"
    {
            set_context(ctx_any); // from here, don't recognize node declarations.
                                  // they are not permitted anyway.
          }
    break;

  case 146:
/* Line 1792 of yacc.c  */
#line 1298 "verilogaYacc.y"
    {
            gModule->_analog=adms_analog_new(YY((yyvsp[(3) - (3)]._yaccval)));
          }
    break;

  case 147:
/* Line 1792 of yacc.c  */
#line 1304 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 148:
/* Line 1792 of yacc.c  */
#line 1308 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 149:
/* Line 1792 of yacc.c  */
#line 1314 "verilogaYacc.y"
    {
            p_slist myArgs=NULL;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_push(&myArgs,YY((yyvsp[(1) - (1)]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
    break;

  case 150:
/* Line 1792 of yacc.c  */
#line 1321 "verilogaYacc.y"
    {
            p_slist myArgs=(p_slist)YY((yyvsp[(1) - (3)]._yaccval));
            (yyval._yaccval)=(yyvsp[(1) - (3)]._yaccval);
            adms_slist_push(&myArgs,YY((yyvsp[(3) - (3)]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
    break;

  case 151:
/* Line 1792 of yacc.c  */
#line 1330 "verilogaYacc.y"
    {
            p_slist l;
            p_slist lv;
            for(l=gAttributeList;l;l=l->next)
              for(lv=((p_blockvariable)YY((yyvsp[(2) - (2)]._yaccval)))->_variable;lv;lv=lv->next)
                adms_slist_push(&((p_variableprototype)lv->data)->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            (yyval._yaccval)=(yyvsp[(2) - (2)]._yaccval);
          }
    break;

  case 152:
/* Line 1792 of yacc.c  */
#line 1340 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 153:
/* Line 1792 of yacc.c  */
#line 1344 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (2)]._yaccval);
          }
    break;

  case 154:
/* Line 1792 of yacc.c  */
#line 1348 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 155:
/* Line 1792 of yacc.c  */
#line 1352 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 156:
/* Line 1792 of yacc.c  */
#line 1356 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 157:
/* Line 1792 of yacc.c  */
#line 1360 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 158:
/* Line 1792 of yacc.c  */
#line 1364 "verilogaYacc.y"
    {
            p_function myfunction=adms_function_new((yyvsp[(1) - (5)]._lexval),uid++);
            p_slist myArgs=(p_slist)YY((yyvsp[(3) - (5)]._yaccval));
            p_callfunction mycallfunction=adms_callfunction_new(gModule,myfunction);
            adms_slist_push(&gModule->_callfunction,(p_adms)mycallfunction);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_inreverse(&myArgs);
            myfunction->_arguments=myArgs;
            Y((yyval._yaccval),(p_adms)mycallfunction);
          }
    break;

  case 159:
/* Line 1792 of yacc.c  */
#line 1375 "verilogaYacc.y"
    {
            p_function myfunction=adms_function_new((yyvsp[(1) - (4)]._lexval),uid++);
            p_callfunction mycallfunction=adms_callfunction_new(gModule,myfunction);
            adms_slist_push(&gModule->_callfunction,(p_adms)mycallfunction);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mycallfunction);
          }
    break;

  case 160:
/* Line 1792 of yacc.c  */
#line 1383 "verilogaYacc.y"
    {
            p_function myfunction=adms_function_new((yyvsp[(1) - (2)]._lexval),uid++);
            p_callfunction mycallfunction=adms_callfunction_new(gModule,myfunction);
            adms_slist_push(&gModule->_callfunction,(p_adms)mycallfunction);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mycallfunction);
          }
    break;

  case 161:
/* Line 1792 of yacc.c  */
#line 1391 "verilogaYacc.y"
    {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)adms_nilled_new(gModule));
          }
    break;

  case 162:
/* Line 1792 of yacc.c  */
#line 1398 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 163:
/* Line 1792 of yacc.c  */
#line 1402 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(2) - (2)]._yaccval);
            adms_lexval_free(((p_block)YY((yyval._yaccval)))->_lexval);
            ((p_block)YY((yyval._yaccval)))->_lexval=(p_lexval)YY((yyvsp[(1) - (2)]._yaccval));
          }
    break;

  case 164:
/* Line 1792 of yacc.c  */
#line 1410 "verilogaYacc.y"
    {
            adms_veriloga_message_fatal("@ control not supported\n",(yyvsp[(3) - (7)]._lexval));
          }
    break;

  case 165:
/* Line 1792 of yacc.c  */
#line 1414 "verilogaYacc.y"
    {
            char* mylexval2=((p_lexval)(yyvsp[(2) - (2)]._lexval))->_string;
            char* mypartitionning=adms_kclone(mylexval2);
            if(strcmp(mypartitionning,"initial_model")
              && strcmp(mypartitionning,"initial_instance")
              && strcmp(mypartitionning,"noise")
              && strcmp(mypartitionning,"initial_step")
              && strcmp(mypartitionning,"final_step"))
              adms_veriloga_message_fatal(" @() control not supported\n",(yyvsp[(2) - (2)]._lexval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)(yyvsp[(2) - (2)]._lexval));
          }
    break;

  case 166:
/* Line 1792 of yacc.c  */
#line 1427 "verilogaYacc.y"
    {
            char* mylexval3=((p_lexval)(yyvsp[(3) - (4)]._lexval))->_string;
            char* mypartitionning=adms_kclone(mylexval3);
            if(strcmp(mypartitionning,"initial_model")
              && strcmp(mypartitionning,"initial_instance")
              && strcmp(mypartitionning,"noise")
              && strcmp(mypartitionning,"initial_step")
              && strcmp(mypartitionning,"final_step"))
              adms_veriloga_message_fatal(" @() control not supported\n",(yyvsp[(3) - (4)]._lexval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)(yyvsp[(3) - (4)]._lexval));
          }
    break;

  case 167:
/* Line 1792 of yacc.c  */
#line 1442 "verilogaYacc.y"
    {
          }
    break;

  case 168:
/* Line 1792 of yacc.c  */
#line 1445 "verilogaYacc.y"
    {
          }
    break;

  case 169:
/* Line 1792 of yacc.c  */
#line 1450 "verilogaYacc.y"
    {
          }
    break;

  case 170:
/* Line 1792 of yacc.c  */
#line 1455 "verilogaYacc.y"
    {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),gBlockList->data);
            adms_slist_pull(&gBlockList);
          }
    break;

  case 171:
/* Line 1792 of yacc.c  */
#line 1461 "verilogaYacc.y"
    {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),gBlockList->data);
            adms_slist_pull(&gBlockList);
            ((p_block)YY((yyval._yaccval)))->_lexval->_string=(yyvsp[(3) - (4)]._lexval)->_string;
          }
    break;

  case 172:
/* Line 1792 of yacc.c  */
#line 1468 "verilogaYacc.y"
    {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),gBlockList->data);
            adms_slist_pull(&gBlockList);
          }
    break;

  case 173:
/* Line 1792 of yacc.c  */
#line 1474 "verilogaYacc.y"
    {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),gBlockList->data);
            adms_slist_pull(&gBlockList);
            ((p_block)YY((yyval._yaccval)))->_lexval->_string=(yyvsp[(3) - (5)]._lexval)->_string;
          }
    break;

  case 174:
/* Line 1792 of yacc.c  */
#line 1483 "verilogaYacc.y"
    {
            p_slist l;
            p_block myblock=adms_block_new(gModule,(yyvsp[(2) - (2)]._lexval),gBlockList?((p_block)gBlockList->data):NULL,NULL);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myblock->_lexval->_string=adms_kclone("");
            adms_slist_push(&gBlockList,(p_adms)myblock);
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&myblock->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            adms_slist_push(&gModule->_block,gBlockList->data);
          }
    break;

  case 175:
/* Line 1792 of yacc.c  */
#line 1497 "verilogaYacc.y"
    {
            adms_slist_push(&((p_block)gBlockList->data)->_item,YY((yyvsp[(1) - (1)]._yaccval)));
          }
    break;

  case 176:
/* Line 1792 of yacc.c  */
#line 1501 "verilogaYacc.y"
    {
            adms_slist_push(&((p_block)gBlockList->data)->_item,YY((yyvsp[(2) - (2)]._yaccval)));
          }
    break;

  case 177:
/* Line 1792 of yacc.c  */
#line 1507 "verilogaYacc.y"
    {
            p_slist l;
            p_blockvariable myblockvariable=adms_blockvariable_new(((p_block)gBlockList->data)); 
            adms_slist_push(&gModule->_blockvariable,(p_adms)myblockvariable); 
            for(l=gBlockVariableList;l;l=l->next)
              ((p_variableprototype)l->data)->_type=admse_integer;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_inreverse(&gBlockVariableList);
            myblockvariable->_variable=gBlockVariableList;
            gBlockVariableList=NULL;
            Y((yyval._yaccval),(p_adms)myblockvariable);
          }
    break;

  case 178:
/* Line 1792 of yacc.c  */
#line 1520 "verilogaYacc.y"
    {
            p_slist l;
            p_blockvariable myblockvariable=adms_blockvariable_new(((p_block)gBlockList->data)); 
            adms_slist_push(&gModule->_blockvariable,(p_adms)myblockvariable); 
            for(l=gBlockVariableList;l;l=l->next)
              ((p_variableprototype)l->data)->_type=admse_real;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_inreverse(&gBlockVariableList);
            myblockvariable->_variable=gBlockVariableList;
            gBlockVariableList=NULL;
            Y((yyval._yaccval),(p_adms)myblockvariable);
          }
    break;

  case 179:
/* Line 1792 of yacc.c  */
#line 1533 "verilogaYacc.y"
    {
            p_slist l;
            p_blockvariable myblockvariable=adms_blockvariable_new(((p_block)gBlockList->data)); 
            adms_slist_push(&gModule->_blockvariable,(p_adms)myblockvariable); 
            for(l=gBlockVariableList;l;l=l->next)
              ((p_variableprototype)l->data)->_type=admse_string;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_inreverse(&gBlockVariableList);
            myblockvariable->_variable=gBlockVariableList;
            gBlockVariableList=NULL;
            Y((yyval._yaccval),(p_adms)myblockvariable);
          }
    break;

  case 180:
/* Line 1792 of yacc.c  */
#line 1548 "verilogaYacc.y"
    {
          }
    break;

  case 181:
/* Line 1792 of yacc.c  */
#line 1551 "verilogaYacc.y"
    {
          }
    break;

  case 182:
/* Line 1792 of yacc.c  */
#line 1556 "verilogaYacc.y"
    {
            NEWVARIABLE((yyvsp[(1) - (1)]._lexval))
            adms_block_list_variable_prepend_once_or_abort(((p_block)gBlockList->data),myvariableprototype); 
            adms_slist_push(&gBlockVariableList,(p_adms)myvariableprototype);
          }
    break;

  case 183:
/* Line 1792 of yacc.c  */
#line 1562 "verilogaYacc.y"
    {
            NEWVARIABLE((yyvsp[(1) - (6)]._lexval))
            adms_block_list_variable_prepend_once_or_abort(((p_block)gBlockList->data),myvariableprototype); 
            adms_slist_push(&gVariableDeclarationList,(p_adms)myvariableprototype);
            myvariableprototype->_sizetype=admse_array;
            myvariableprototype->_minsize=adms_number_new((yyvsp[(3) - (6)]._lexval));
            myvariableprototype->_maxsize=adms_number_new((yyvsp[(5) - (6)]._lexval));
          }
    break;

  case 184:
/* Line 1792 of yacc.c  */
#line 1573 "verilogaYacc.y"
    {
            p_slist l;
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&gContribution->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            gContribution=NULL;
          }
    break;

  case 185:
/* Line 1792 of yacc.c  */
#line 1583 "verilogaYacc.y"
    {
            p_source mysource=(p_source)YY((yyvsp[(1) - (4)]._yaccval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            gContribution=adms_contribution_new(gModule,mysource,(p_expression)YY((yyvsp[(4) - (4)]._yaccval)),gLexval);
            adms_slist_push(&gModule->_contribution,(p_adms)gContribution);
            Y((yyval._yaccval),(p_adms)gContribution);
            gContribution->_branchalias=gBranchAlias;
            gBranchAlias=NULL;
          }
    break;

  case 186:
/* Line 1792 of yacc.c  */
#line 1595 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (6)]._lexval))->_string;
            char* mylexval3=((p_lexval)(yyvsp[(3) - (6)]._lexval))->_string;
            char* mylexval5=((p_lexval)(yyvsp[(5) - (6)]._lexval))->_string;
            p_node Pnode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval3);
            p_node Nnode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval5);
            char* natureID=mylexval1;
            p_nature mynature=adms_admsmain_list_nature_lookup_by_id(root(),natureID);
            p_branch mybranch=adms_module_list_branch_prepend_by_id_once_or_ignore(gModule,gModule,Pnode,Nnode);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            if(!mynature)
             adms_message_fatal(("[source:error] there is no nature with access %s, missing discipline.h file?\n",natureID))
            gSource=adms_module_list_source_prepend_by_id_once_or_ignore(gModule,gModule,mybranch,mynature);
            gLexval=(yyvsp[(1) - (6)]._lexval);
            Y((yyval._yaccval),(p_adms)gSource);
          }
    break;

  case 187:
/* Line 1792 of yacc.c  */
#line 1612 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (4)]._lexval))->_string;
            char* mylexval3=((p_lexval)(yyvsp[(3) - (4)]._lexval))->_string;
            char* natureID=mylexval1;
            p_nature mynature=adms_admsmain_list_nature_lookup_by_id(root(),natureID);
            p_branchalias branchalias=adms_module_list_branchalias_lookup_by_id(gModule,gModule,mylexval3);
            p_node pnode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval3);
            p_branch mybranch=NULL;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            if(!mynature)
             adms_message_fatal(("[source:error] there is no nature with access %s, please, include discipline.h file\n",natureID))
            if(pnode)
              mybranch=adms_module_list_branch_prepend_by_id_once_or_ignore(gModule,gModule,pnode,gGND);
            else if(branchalias)
              mybranch=branchalias->_branch;
            else
              adms_veriloga_message_fatal("undefined branch or node\n",(yyvsp[(1) - (4)]._lexval));
            gSource=adms_module_list_source_prepend_by_id_once_or_ignore(gModule,gModule,mybranch,mynature);
            gLexval=(yyvsp[(1) - (4)]._lexval);
            gBranchAlias=branchalias;
            Y((yyval._yaccval),(p_adms)gSource);
          }
    break;

  case 188:
/* Line 1792 of yacc.c  */
#line 1637 "verilogaYacc.y"
    {
            p_whileloop mywhileloop=adms_whileloop_new(gModule,(p_expression)YY((yyvsp[(3) - (5)]._yaccval)),YY((yyvsp[(5) - (5)]._yaccval)));
            adms_slist_push(&gModule->_whileloop,(p_adms)mywhileloop);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mywhileloop);
          }
    break;

  case 189:
/* Line 1792 of yacc.c  */
#line 1646 "verilogaYacc.y"
    {
            p_forloop myforloop=adms_forloop_new(gModule,(p_assignment)YY((yyvsp[(3) - (9)]._yaccval)),(p_expression)YY((yyvsp[(5) - (9)]._yaccval)),(p_assignment)YY((yyvsp[(7) - (9)]._yaccval)),YY((yyvsp[(9) - (9)]._yaccval)));
            adms_slist_push(&gModule->_forloop,(p_adms)myforloop);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myforloop);
          }
    break;

  case 190:
/* Line 1792 of yacc.c  */
#line 1655 "verilogaYacc.y"
    {
            p_case mycase=adms_case_new(gModule,(p_expression)YY((yyvsp[(3) - (6)]._yaccval)));
            adms_slist_push(&gModule->_case,(p_adms)mycase);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            mycase->_caseitem=adms_slist_reverse((p_slist)YY((yyvsp[(5) - (6)]._yaccval)));
            Y((yyval._yaccval),(p_adms)mycase);
          }
    break;

  case 191:
/* Line 1792 of yacc.c  */
#line 1665 "verilogaYacc.y"
    {
            p_slist myArgs=NULL;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_push(&myArgs,YY((yyvsp[(1) - (1)]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
    break;

  case 192:
/* Line 1792 of yacc.c  */
#line 1672 "verilogaYacc.y"
    {
            p_slist myArgs=(p_slist)YY((yyvsp[(1) - (2)]._yaccval));
            (yyval._yaccval)=(yyvsp[(1) - (2)]._yaccval);
            adms_slist_push(&myArgs,YY((yyvsp[(2) - (2)]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
    break;

  case 193:
/* Line 1792 of yacc.c  */
#line 1681 "verilogaYacc.y"
    {
            p_slist myArgs=(p_slist)YY((yyvsp[(1) - (3)]._yaccval));
            p_caseitem mycaseitem=adms_caseitem_new(YY((yyvsp[(3) - (3)]._yaccval)));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            mycaseitem->_condition=adms_slist_reverse(myArgs);
            Y((yyval._yaccval),(p_adms)mycaseitem);
          }
    break;

  case 194:
/* Line 1792 of yacc.c  */
#line 1689 "verilogaYacc.y"
    {
            p_caseitem mycaseitem=adms_caseitem_new(YY((yyvsp[(3) - (3)]._yaccval)));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            mycaseitem->_defaultcase=admse_yes;
            Y((yyval._yaccval),(p_adms)mycaseitem);
          }
    break;

  case 195:
/* Line 1792 of yacc.c  */
#line 1696 "verilogaYacc.y"
    {
            p_caseitem mycaseitem=adms_caseitem_new(YY((yyvsp[(2) - (2)]._yaccval)));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            mycaseitem->_defaultcase=admse_yes;
            Y((yyval._yaccval),(p_adms)mycaseitem);
          }
    break;

  case 196:
/* Line 1792 of yacc.c  */
#line 1705 "verilogaYacc.y"
    {
          }
    break;

  case 197:
/* Line 1792 of yacc.c  */
#line 1708 "verilogaYacc.y"
    {
          }
    break;

  case 198:
/* Line 1792 of yacc.c  */
#line 1713 "verilogaYacc.y"
    {
            char* mylexval3=((p_lexval)(yyvsp[(3) - (7)]._lexval))->_string;
            p_instance myinstance;
            p_slist l1;
            p_slist l2;
            myinstance=adms_module_list_instance_prepend_by_id_once_or_abort(gModule,gModule,gInstanceModule,mylexval3);
            adms_slist_inreverse(&gInstanceModule->_node);
            l2=gInstanceModule->_node;
            l2=l2->next; /*GND ignored*/
            for(l1=adms_slist_reverse(gNodeList);l1;l1=l1->next)
            {
              adms_instance_list_terminal_prepend_once_or_abort(myinstance,adms_instancenode_new(((p_node)l1->data),(p_node)l2->data));
              l2=l2->next;
            }
            for(l1=gInstanceVariableList;l1;l1=l1->next)
              adms_instance_list_parameterset_prepend_once_or_abort(myinstance,(p_instanceparameter)l1->data);
            adms_slist_inreverse(&gInstanceModule->_node);
            adms_slist_free(gNodeList);gNodeList=NULL;
            adms_slist_free(gInstanceVariableList);gInstanceVariableList=NULL;
          }
    break;

  case 199:
/* Line 1792 of yacc.c  */
#line 1736 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (1)]._lexval))->_string;
            set_context(ctx_any); // from here, don't recognize node declarations.
                                  // they are not permitted anyway.
            gInstanceModule=adms_admsmain_list_module_lookup_by_id(root(),mylexval1);
            if(!gInstanceModule)
              adms_message_fatal(("module '%s' not found\n",mylexval1));
          }
    break;

  case 200:
/* Line 1792 of yacc.c  */
#line 1747 "verilogaYacc.y"
    {
          }
    break;

  case 201:
/* Line 1792 of yacc.c  */
#line 1750 "verilogaYacc.y"
    {
          }
    break;

  case 202:
/* Line 1792 of yacc.c  */
#line 1755 "verilogaYacc.y"
    {
            char* mylexval2=((p_lexval)(yyvsp[(2) - (5)]._lexval))->_string;
            p_variableprototype myvariableprototype=adms_module_list_variable_lookup_by_id(gInstanceModule,gInstanceModule,(yyvsp[(2) - (5)]._lexval),(p_adms)gInstanceModule);
            if(myvariableprototype)
            {
              p_instanceparameter myinstanceparameter;
              myinstanceparameter=adms_instanceparameter_new(myvariableprototype);
              adms_slist_push(&gInstanceVariableList,(p_adms)myinstanceparameter);
              myinstanceparameter->_value=((p_expression)YY((yyvsp[(4) - (5)]._yaccval)));
            }
            else
            {
              adms_veriloga_message_fatal_continue((yyvsp[(2) - (5)]._lexval));
              adms_message_fatal(("[%s.%s.%s]: undefined variable (instance declaration)",
                adms_module_uid(gModule),adms_module_uid(gInstanceModule),mylexval2))
            }
          }
    break;

  case 203:
/* Line 1792 of yacc.c  */
#line 1775 "verilogaYacc.y"
    {
            p_assignment myassignment;
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[(1) - (3)]._lexval));
            p_variableprototype myvariableprototype;
            if(!myvariable)
              adms_veriloga_message_fatal("undefined variable\n",(yyvsp[(1) - (3)]._lexval));
            myvariableprototype=myvariable->_prototype;
            myassignment=adms_assignment_new(gModule,(p_adms)myvariable,(p_expression)YY((yyvsp[(3) - (3)]._yaccval)),(yyvsp[(1) - (3)]._lexval));
            adms_slist_push(&gModule->_assignment,(p_adms)myassignment);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myassignment);
            myvariableprototype->_vcount++;
            myvariableprototype->_vlast=myassignment;
          }
    break;

  case 204:
/* Line 1792 of yacc.c  */
#line 1790 "verilogaYacc.y"
    {
            p_assignment myassignment;
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[(2) - (4)]._lexval));
            p_variableprototype myvariableprototype;
            if(!myvariable)
              adms_veriloga_message_fatal("undefined variable\n",(yyvsp[(2) - (4)]._lexval));
            myvariableprototype=myvariable->_prototype;
            myassignment=adms_assignment_new(gModule,(p_adms)myvariable,(p_expression)YY((yyvsp[(4) - (4)]._yaccval)),(yyvsp[(2) - (4)]._lexval));
            adms_slist_push(&gModule->_assignment,(p_adms)myassignment);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myassignment);
            {
              p_slist l;
              for(l=gAttributeList;l;l=l->next)
                adms_slist_push(&myassignment->_attribute,l->data);
            }
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            myvariableprototype->_vcount++;
            myvariableprototype->_vlast=myassignment;
          }
    break;

  case 205:
/* Line 1792 of yacc.c  */
#line 1811 "verilogaYacc.y"
    {
            p_assignment myassignment;
            p_array myarray;
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[(1) - (6)]._lexval));
            p_variableprototype myvariableprototype;
            if(!myvariable)
              adms_veriloga_message_fatal("undefined variable\n",(yyvsp[(1) - (6)]._lexval));
            myvariableprototype=myvariable->_prototype;
            myarray=adms_array_new(myvariable,YY((yyvsp[(3) - (6)]._yaccval)));
            myassignment=adms_assignment_new(gModule,(p_adms)myarray,(p_expression)YY((yyvsp[(6) - (6)]._yaccval)),(yyvsp[(1) - (6)]._lexval));
            adms_slist_push(&gModule->_assignment,(p_adms)myassignment);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myassignment);
            myvariableprototype->_vcount++;
            myvariableprototype->_vlast=myassignment;
          }
    break;

  case 206:
/* Line 1792 of yacc.c  */
#line 1828 "verilogaYacc.y"
    {
            p_assignment myassignment;
            p_array myarray;
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[(2) - (7)]._lexval));
            p_variableprototype myvariableprototype;
            if(!myvariable)
              adms_veriloga_message_fatal("undefined variable\n",(yyvsp[(2) - (7)]._lexval));
            myvariableprototype=myvariable->_prototype;
            myarray=adms_array_new(myvariable,YY((yyvsp[(4) - (7)]._yaccval)));
            myassignment=adms_assignment_new(gModule,(p_adms)myarray,(p_expression)YY((yyvsp[(7) - (7)]._yaccval)),(yyvsp[(2) - (7)]._lexval));
            adms_slist_push(&gModule->_assignment,(p_adms)myassignment);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myassignment);
            {
              p_slist l;
              for(l=gAttributeList;l;l=l->next)
                adms_slist_push(&myassignment->_attribute,l->data);
            }
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            myvariableprototype->_vcount++;
            myvariableprototype->_vlast=myassignment;
          }
    break;

  case 207:
/* Line 1792 of yacc.c  */
#line 1853 "verilogaYacc.y"
    {
            p_expression myexpression=(p_expression)YY((yyvsp[(3) - (5)]._yaccval));
            p_adms mythen=YY((yyvsp[(5) - (5)]._yaccval));
            p_conditional myconditional=adms_conditional_new(gModule,myexpression,mythen,NULL);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myconditional);
          }
    break;

  case 208:
/* Line 1792 of yacc.c  */
#line 1861 "verilogaYacc.y"
    {
            p_expression myexpression=(p_expression)YY((yyvsp[(3) - (7)]._yaccval));
            p_adms mythen=YY((yyvsp[(5) - (7)]._yaccval));
            p_adms myelse=YY((yyvsp[(7) - (7)]._yaccval));
            p_conditional myconditional=adms_conditional_new(gModule,myexpression,mythen,myelse);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myconditional);
          }
    break;

  case 209:
/* Line 1792 of yacc.c  */
#line 1872 "verilogaYacc.y"
    {
            p_expression myexpression=adms_expression_new(gModule,YY((yyvsp[(1) - (1)]._yaccval))); 
            adms_slist_push(&gModule->_expression,(p_adms)myexpression); 
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
            Y((yyval._yaccval),(p_adms)myexpression);
          }
    break;

  case 210:
/* Line 1792 of yacc.c  */
#line 1881 "verilogaYacc.y"
    {
            p_slist myArgs=NULL;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_push(&myArgs,YY((yyvsp[(1) - (1)]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
    break;

  case 211:
/* Line 1792 of yacc.c  */
#line 1888 "verilogaYacc.y"
    {
            p_slist myArgs=(p_slist)YY((yyvsp[(1) - (3)]._yaccval));
            (yyval._yaccval)=(yyvsp[(1) - (3)]._yaccval);
            adms_slist_push(&myArgs,YY((yyvsp[(3) - (3)]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
    break;

  case 212:
/* Line 1792 of yacc.c  */
#line 1897 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 213:
/* Line 1792 of yacc.c  */
#line 1903 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 214:
/* Line 1792 of yacc.c  */
#line 1909 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 215:
/* Line 1792 of yacc.c  */
#line 1913 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (5)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (5)]._yaccval));
            p_adms m3=YY((yyvsp[(5) - (5)]._yaccval));
            p_mapply_ternary myop=adms_mapply_ternary_new(admse_conditional,m1,m2,m3);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 216:
/* Line 1792 of yacc.c  */
#line 1924 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 217:
/* Line 1792 of yacc.c  */
#line 1928 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_bw_equr,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 218:
/* Line 1792 of yacc.c  */
#line 1936 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (4)]._yaccval));
            p_adms m2=YY((yyvsp[(4) - (4)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_bw_equl,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 219:
/* Line 1792 of yacc.c  */
#line 1946 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 220:
/* Line 1792 of yacc.c  */
#line 1950 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_bw_xor,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 221:
/* Line 1792 of yacc.c  */
#line 1960 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 222:
/* Line 1792 of yacc.c  */
#line 1964 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_bw_or,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 223:
/* Line 1792 of yacc.c  */
#line 1974 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 224:
/* Line 1792 of yacc.c  */
#line 1978 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_bw_and,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 225:
/* Line 1792 of yacc.c  */
#line 1988 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 226:
/* Line 1792 of yacc.c  */
#line 1992 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_or,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 227:
/* Line 1792 of yacc.c  */
#line 2002 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 228:
/* Line 1792 of yacc.c  */
#line 2006 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_and,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 229:
/* Line 1792 of yacc.c  */
#line 2016 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 230:
/* Line 1792 of yacc.c  */
#line 2020 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (4)]._yaccval));
            p_adms m2=YY((yyvsp[(4) - (4)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_equ,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 231:
/* Line 1792 of yacc.c  */
#line 2028 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (4)]._yaccval));
            p_adms m2=YY((yyvsp[(4) - (4)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_notequ,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 232:
/* Line 1792 of yacc.c  */
#line 2038 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 233:
/* Line 1792 of yacc.c  */
#line 2042 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_lt,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 234:
/* Line 1792 of yacc.c  */
#line 2050 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (4)]._yaccval));
            p_adms m2=YY((yyvsp[(4) - (4)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_lt_equ,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 235:
/* Line 1792 of yacc.c  */
#line 2058 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_gt,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 236:
/* Line 1792 of yacc.c  */
#line 2066 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (4)]._yaccval));
            p_adms m2=YY((yyvsp[(4) - (4)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_gt_equ,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 237:
/* Line 1792 of yacc.c  */
#line 2076 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 238:
/* Line 1792 of yacc.c  */
#line 2080 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_shiftr,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 239:
/* Line 1792 of yacc.c  */
#line 2088 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_shiftl,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 240:
/* Line 1792 of yacc.c  */
#line 2098 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 241:
/* Line 1792 of yacc.c  */
#line 2102 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_addp,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 242:
/* Line 1792 of yacc.c  */
#line 2110 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_addm,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 243:
/* Line 1792 of yacc.c  */
#line 2120 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 244:
/* Line 1792 of yacc.c  */
#line 2124 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_multtime,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 245:
/* Line 1792 of yacc.c  */
#line 2132 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_multdiv,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 246:
/* Line 1792 of yacc.c  */
#line 2140 "verilogaYacc.y"
    {
            p_adms m1=YY((yyvsp[(1) - (3)]._yaccval));
            p_adms m2=YY((yyvsp[(3) - (3)]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_multmod,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
    break;

  case 247:
/* Line 1792 of yacc.c  */
#line 2150 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(1) - (1)]._yaccval);
          }
    break;

  case 248:
/* Line 1792 of yacc.c  */
#line 2154 "verilogaYacc.y"
    {
            p_adms m=YY((yyvsp[(2) - (2)]._yaccval));
            p_mapply_unary mymathapply=adms_mapply_unary_new(admse_plus,m);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mymathapply);
          }
    break;

  case 249:
/* Line 1792 of yacc.c  */
#line 2161 "verilogaYacc.y"
    {
            p_adms m=YY((yyvsp[(2) - (2)]._yaccval));
            p_mapply_unary mymathapply=adms_mapply_unary_new(admse_minus,m);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mymathapply);
          }
    break;

  case 250:
/* Line 1792 of yacc.c  */
#line 2168 "verilogaYacc.y"
    {
            p_adms m=YY((yyvsp[(2) - (2)]._yaccval));
            p_mapply_unary mymathapply=adms_mapply_unary_new(admse_not,m);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mymathapply);
          }
    break;

  case 251:
/* Line 1792 of yacc.c  */
#line 2175 "verilogaYacc.y"
    {
            p_adms m=YY((yyvsp[(2) - (2)]._yaccval));
            p_mapply_unary mymathapply=adms_mapply_unary_new(admse_bw_not,m);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mymathapply);
          }
    break;

  case 252:
/* Line 1792 of yacc.c  */
#line 2184 "verilogaYacc.y"
    {
            p_number mynumber=adms_number_new((yyvsp[(1) - (1)]._lexval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            mynumber->_cast=admse_i;
            Y((yyval._yaccval),(p_adms)mynumber);
          }
    break;

  case 253:
/* Line 1792 of yacc.c  */
#line 2191 "verilogaYacc.y"
    {
            char* mylexval2=((p_lexval)(yyvsp[(2) - (2)]._lexval))->_string;
            p_number mynumber=adms_number_new((yyvsp[(1) - (2)]._lexval));
            int myunit=admse_1;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            if(0) {}
            else if(!strcmp(mylexval2,"E")) myunit=admse_E;
            else if(!strcmp(mylexval2,"P")) myunit=admse_P;
            else if(!strcmp(mylexval2,"T")) myunit=admse_T;
            else if(!strcmp(mylexval2,"G")) myunit=admse_G;
            else if(!strcmp(mylexval2,"M")) myunit=admse_M;
            else if(!strcmp(mylexval2,"K")) myunit=admse_K;
            else if(!strcmp(mylexval2,"k")) myunit=admse_k;
            else if(!strcmp(mylexval2,"h")) myunit=admse_h;
            else if(!strcmp(mylexval2,"D")) myunit=admse_D;
            else if(!strcmp(mylexval2,"d")) myunit=admse_d;
            else if(!strcmp(mylexval2,"c")) myunit=admse_c;
            else if(!strcmp(mylexval2,"m")) myunit=admse_m;
            else if(!strcmp(mylexval2,"u")) myunit=admse_u;
            else if(!strcmp(mylexval2,"n")) myunit=admse_n;
            else if(!strcmp(mylexval2,"A")) myunit=admse_A;
            else if(!strcmp(mylexval2,"p")) myunit=admse_p;
            else if(!strcmp(mylexval2,"f")) myunit=admse_f;
            else if(!strcmp(mylexval2,"a")) myunit=admse_a;
            else
              adms_veriloga_message_fatal(" can not convert symbol to valid unit\n",(yyvsp[(2) - (2)]._lexval));
            mynumber->_scalingunit=myunit;
            mynumber->_cast=admse_i;
            Y((yyval._yaccval),(p_adms)mynumber);
          }
    break;

  case 254:
/* Line 1792 of yacc.c  */
#line 2222 "verilogaYacc.y"
    {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)adms_number_new((yyvsp[(1) - (1)]._lexval)));
          }
    break;

  case 255:
/* Line 1792 of yacc.c  */
#line 2227 "verilogaYacc.y"
    {
            char* mylexval2=((p_lexval)(yyvsp[(2) - (2)]._lexval))->_string;
            p_number mynumber=adms_number_new((yyvsp[(1) - (2)]._lexval));
            int myunit=admse_1;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            if(0) {}
            else if(!strcmp(mylexval2,"E")) myunit=admse_E;
            else if(!strcmp(mylexval2,"P")) myunit=admse_P;
            else if(!strcmp(mylexval2,"T")) myunit=admse_T;
            else if(!strcmp(mylexval2,"G")) myunit=admse_G;
            else if(!strcmp(mylexval2,"M")) myunit=admse_M;
            else if(!strcmp(mylexval2,"K")) myunit=admse_K;
            else if(!strcmp(mylexval2,"k")) myunit=admse_k;
            else if(!strcmp(mylexval2,"h")) myunit=admse_h;
            else if(!strcmp(mylexval2,"D")) myunit=admse_D;
            else if(!strcmp(mylexval2,"d")) myunit=admse_d;
            else if(!strcmp(mylexval2,"c")) myunit=admse_c;
            else if(!strcmp(mylexval2,"m")) myunit=admse_m;
            else if(!strcmp(mylexval2,"u")) myunit=admse_u;
            else if(!strcmp(mylexval2,"n")) myunit=admse_n;
            else if(!strcmp(mylexval2,"A")) myunit=admse_A;
            else if(!strcmp(mylexval2,"p")) myunit=admse_p;
            else if(!strcmp(mylexval2,"f")) myunit=admse_f;
            else if(!strcmp(mylexval2,"a")) myunit=admse_a;
            else
              adms_veriloga_message_fatal(" can not convert symbol to valid unit\n",(yyvsp[(2) - (2)]._lexval));
            mynumber->_scalingunit=myunit;
            Y((yyval._yaccval),(p_adms)mynumber);
          }
    break;

  case 256:
/* Line 1792 of yacc.c  */
#line 2257 "verilogaYacc.y"
    {
            adms_veriloga_message_fatal("%s: character are not handled\n",(yyvsp[(1) - (1)]._lexval));
          }
    break;

  case 257:
/* Line 1792 of yacc.c  */
#line 2261 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (1)]._lexval))->_string;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)adms_string_new(mylexval1));
          }
    break;

  case 258:
/* Line 1792 of yacc.c  */
#line 2267 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (1)]._lexval))->_string;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[(1) - (1)]._lexval));
            if(myvariable)
              Y((yyval._yaccval),(p_adms)myvariable);
            else if (!gAnalogfunction)
            {
                p_branchalias mybranchalias=adms_module_list_branchalias_lookup_by_id(gModule,gModule,mylexval1);
                p_node mynode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval1);
                if(mynode) Y((yyval._yaccval),(p_adms)mynode);
                if(mybranchalias)
                  Y((yyval._yaccval),(p_adms)mybranchalias->_branch);
            }
            if(!YY((yyval._yaccval)))
              adms_veriloga_message_fatal("identifier never declared\n",(yyvsp[(1) - (1)]._lexval));
          }
    break;

  case 259:
/* Line 1792 of yacc.c  */
#line 2285 "verilogaYacc.y"
    {
            p_function myfunction=adms_function_new((yyvsp[(1) - (1)]._lexval),uid++);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myfunction);
          }
    break;

  case 260:
/* Line 1792 of yacc.c  */
#line 2291 "verilogaYacc.y"
    {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[(1) - (4)]._lexval));
            if(!myvariable)
               adms_veriloga_message_fatal("undefined array variable\n",(yyvsp[(1) - (4)]._lexval));
            Y((yyval._yaccval),(p_adms)adms_array_new(myvariable,YY((yyvsp[(3) - (4)]._yaccval))));
          }
    break;

  case 261:
/* Line 1792 of yacc.c  */
#line 2299 "verilogaYacc.y"
    {
            p_function myfunction=adms_function_new((yyvsp[(1) - (4)]._lexval),uid++);
            p_slist myArgs=(p_slist)YY((yyvsp[(3) - (4)]._yaccval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_inreverse(&myArgs);
            myfunction->_arguments=myArgs;
            Y((yyval._yaccval),(p_adms)myfunction);
          }
    break;

  case 262:
/* Line 1792 of yacc.c  */
#line 2308 "verilogaYacc.y"
    {
            char* mylexval1=((p_lexval)(yyvsp[(1) - (4)]._lexval))->_string;
            char* myfunctionname=mylexval1;
            p_slist myArgs=(p_slist)YY((yyvsp[(3) - (4)]._yaccval));
            int narg=adms_slist_length(myArgs);
            p_probe myprobe=NULL;
            p_nature mynature=adms_admsmain_list_nature_lookup_by_id(root(),myfunctionname);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            if(mynature && narg==1)
            {
              p_adms mychild0=(p_adms)adms_slist_nth_data(myArgs,0);
              if(mychild0->_datatypename==admse_node)
              {
                p_branch mybranch=adms_module_list_branch_prepend_by_id_once_or_ignore(gModule,gModule,(p_node)mychild0,gGND);
                myprobe=adms_module_list_probe_prepend_by_id_once_or_ignore(gModule,gModule,mybranch,mynature);
              }
              else if(mychild0->_datatypename==admse_branch)
              {
                myprobe=adms_module_list_probe_prepend_by_id_once_or_ignore(gModule,gModule,(p_branch)mychild0,mynature);
              }
              else
                adms_veriloga_message_fatal("bad argument (expecting node or branch)\n",(yyvsp[(1) - (4)]._lexval));
            }
            else if(mynature && narg==2)
            {
              p_adms mychild0=(p_adms)adms_slist_nth_data(myArgs,0);
              p_adms mychild1=(p_adms)adms_slist_nth_data(myArgs,1);
              p_branch mybranch;
              if(mychild0->_datatypename!=admse_node)
                adms_veriloga_message_fatal("second argument of probe is not a node\n",(yyvsp[(1) - (4)]._lexval));
              if(mychild1->_datatypename!=admse_node)
                adms_veriloga_message_fatal("first argument of probe is not a node\n",(yyvsp[(1) - (4)]._lexval));
              mybranch=adms_module_list_branch_prepend_by_id_once_or_ignore(gModule,gModule,(p_node)mychild1,((p_node)mychild0));
              myprobe=adms_module_list_probe_prepend_by_id_once_or_ignore(gModule,gModule,mybranch,mynature);
            }
            if(myprobe)
              Y((yyval._yaccval),(p_adms)myprobe);
            else
            {
              p_slist l;
              p_function myfunction=adms_function_new((yyvsp[(1) - (4)]._lexval),uid++);
              for(l=gModule->_analogfunction;l&&(myfunction->_definition==NULL);l=l->next)
              {
                p_analogfunction myanalogfunction=(p_analogfunction)l->data;
                if(!strcmp((yyvsp[(1) - (4)]._lexval)->_string,myanalogfunction->_lexval->_string))
                  myfunction->_definition=myanalogfunction;
              }
              myfunction->_arguments=adms_slist_reverse(myArgs);
              Y((yyval._yaccval),(p_adms)myfunction);
            }
          }
    break;

  case 263:
/* Line 1792 of yacc.c  */
#line 2360 "verilogaYacc.y"
    {
            (yyval._yaccval)=(yyvsp[(2) - (3)]._yaccval);
          }
    break;


/* Line 1792 of yacc.c  */
#line 5003 "y.tab.c"
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
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


/* Line 2055 of yacc.c  */
#line 2364 "verilogaYacc.y"

void adms_veriloga_setint_yydebug(const int val)
{
  yydebug=val;
}
