/* A Bison parser, made by GNU Bison 3.7.6.  */

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
#define YYBISON 30706

/* Bison version string.  */
#define YYBISON_VERSION "3.7.6"

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
#define yydebug         verilogadebug
#define yynerrs         veriloganerrs
#define yylval          verilogalval
#define yychar          verilogachar

/* First part of user prologue.  */
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


#line 194 "y.tab.c"

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

#line 356 "y.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE verilogalval;

int verilogaparse (void);

#endif /* !YY_VERILOGA_Y_TAB_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_PREC_IF_THEN = 3,               /* PREC_IF_THEN  */
  YYSYMBOL_tk_aliasparam = 4,              /* tk_aliasparam  */
  YYSYMBOL_tk_aliasparameter = 5,          /* tk_aliasparameter  */
  YYSYMBOL_tk_analog = 6,                  /* tk_analog  */
  YYSYMBOL_tk_and = 7,                     /* tk_and  */
  YYSYMBOL_tk_anystring = 8,               /* tk_anystring  */
  YYSYMBOL_tk_anytext = 9,                 /* tk_anytext  */
  YYSYMBOL_tk_begin = 10,                  /* tk_begin  */
  YYSYMBOL_tk_beginattribute = 11,         /* tk_beginattribute  */
  YYSYMBOL_tk_bitwise_equr = 12,           /* tk_bitwise_equr  */
  YYSYMBOL_tk_branch = 13,                 /* tk_branch  */
  YYSYMBOL_tk_case = 14,                   /* tk_case  */
  YYSYMBOL_tk_char = 15,                   /* tk_char  */
  YYSYMBOL_tk_default = 16,                /* tk_default  */
  YYSYMBOL_tk_disc_id = 17,                /* tk_disc_id  */
  YYSYMBOL_tk_discipline = 18,             /* tk_discipline  */
  YYSYMBOL_tk_dollar_ident = 19,           /* tk_dollar_ident  */
  YYSYMBOL_tk_domain = 20,                 /* tk_domain  */
  YYSYMBOL_tk_else = 21,                   /* tk_else  */
  YYSYMBOL_tk_end = 22,                    /* tk_end  */
  YYSYMBOL_tk_endattribute = 23,           /* tk_endattribute  */
  YYSYMBOL_tk_endcase = 24,                /* tk_endcase  */
  YYSYMBOL_tk_enddiscipline = 25,          /* tk_enddiscipline  */
  YYSYMBOL_tk_endfunction = 26,            /* tk_endfunction  */
  YYSYMBOL_tk_endmodule = 27,              /* tk_endmodule  */
  YYSYMBOL_tk_endnature = 28,              /* tk_endnature  */
  YYSYMBOL_tk_exclude = 29,                /* tk_exclude  */
  YYSYMBOL_tk_flow = 30,                   /* tk_flow  */
  YYSYMBOL_tk_for = 31,                    /* tk_for  */
  YYSYMBOL_tk_from = 32,                   /* tk_from  */
  YYSYMBOL_tk_function = 33,               /* tk_function  */
  YYSYMBOL_tk_ground = 34,                 /* tk_ground  */
  YYSYMBOL_tk_ident = 35,                  /* tk_ident  */
  YYSYMBOL_tk_if = 36,                     /* tk_if  */
  YYSYMBOL_tk_inf = 37,                    /* tk_inf  */
  YYSYMBOL_tk_inout = 38,                  /* tk_inout  */
  YYSYMBOL_tk_input = 39,                  /* tk_input  */
  YYSYMBOL_tk_integer = 40,                /* tk_integer  */
  YYSYMBOL_tk_module = 41,                 /* tk_module  */
  YYSYMBOL_tk_nature = 42,                 /* tk_nature  */
  YYSYMBOL_tk_number = 43,                 /* tk_number  */
  YYSYMBOL_tk_op_shl = 44,                 /* tk_op_shl  */
  YYSYMBOL_tk_op_shr = 45,                 /* tk_op_shr  */
  YYSYMBOL_tk_or = 46,                     /* tk_or  */
  YYSYMBOL_tk_output = 47,                 /* tk_output  */
  YYSYMBOL_tk_parameter = 48,              /* tk_parameter  */
  YYSYMBOL_tk_potential = 49,              /* tk_potential  */
  YYSYMBOL_tk_real = 50,                   /* tk_real  */
  YYSYMBOL_tk_string = 51,                 /* tk_string  */
  YYSYMBOL_tk_while = 52,                  /* tk_while  */
  YYSYMBOL_53_ = 53,                       /* ';'  */
  YYSYMBOL_54_ = 54,                       /* '='  */
  YYSYMBOL_55_ = 55,                       /* '('  */
  YYSYMBOL_56_ = 56,                       /* ')'  */
  YYSYMBOL_57_ = 57,                       /* ','  */
  YYSYMBOL_58_ = 58,                       /* '{'  */
  YYSYMBOL_59_ = 59,                       /* '}'  */
  YYSYMBOL_60_ = 60,                       /* '['  */
  YYSYMBOL_61_ = 61,                       /* ':'  */
  YYSYMBOL_62_ = 62,                       /* ']'  */
  YYSYMBOL_63_ = 63,                       /* '-'  */
  YYSYMBOL_64_ = 64,                       /* '+'  */
  YYSYMBOL_65_ = 65,                       /* '@'  */
  YYSYMBOL_66_ = 66,                       /* '<'  */
  YYSYMBOL_67_ = 67,                       /* '#'  */
  YYSYMBOL_68_ = 68,                       /* '.'  */
  YYSYMBOL_69_ = 69,                       /* '?'  */
  YYSYMBOL_70_ = 70,                       /* '~'  */
  YYSYMBOL_71_ = 71,                       /* '^'  */
  YYSYMBOL_72_ = 72,                       /* '|'  */
  YYSYMBOL_73_ = 73,                       /* '&'  */
  YYSYMBOL_74_ = 74,                       /* '!'  */
  YYSYMBOL_75_ = 75,                       /* '>'  */
  YYSYMBOL_76_ = 76,                       /* '*'  */
  YYSYMBOL_77_ = 77,                       /* '/'  */
  YYSYMBOL_78_ = 78,                       /* '%'  */
  YYSYMBOL_YYACCEPT = 79,                  /* $accept  */
  YYSYMBOL_R_admsParse = 80,               /* R_admsParse  */
  YYSYMBOL_81_R_l_admsParse = 81,          /* R_l.admsParse  */
  YYSYMBOL_82_R_s_admsParse = 82,          /* R_s.admsParse  */
  YYSYMBOL_R_discipline_member = 83,       /* R_discipline_member  */
  YYSYMBOL_R_discipline_name = 84,         /* R_discipline_name  */
  YYSYMBOL_85_R_l_discipline_assignment = 85, /* R_l.discipline_assignment  */
  YYSYMBOL_86_R_s_discipline_assignment = 86, /* R_s.discipline_assignment  */
  YYSYMBOL_87_R_discipline_naturename = 87, /* R_discipline.naturename  */
  YYSYMBOL_R_nature_member = 88,           /* R_nature_member  */
  YYSYMBOL_89_R_l_nature_assignment = 89,  /* R_l.nature_assignment  */
  YYSYMBOL_90_R_s_nature_assignment = 90,  /* R_s.nature_assignment  */
  YYSYMBOL_91_R_d_attribute_0 = 91,        /* R_d.attribute.0  */
  YYSYMBOL_92_R_d_attribute = 92,          /* R_d.attribute  */
  YYSYMBOL_93_R_l_attribute = 93,          /* R_l.attribute  */
  YYSYMBOL_94_R_s_attribute = 94,          /* R_s.attribute  */
  YYSYMBOL_95_R_d_module = 95,             /* R_d.module  */
  YYSYMBOL_96_1 = 96,                      /* $@1  */
  YYSYMBOL_97_2 = 97,                      /* $@2  */
  YYSYMBOL_R_modulebody = 98,              /* R_modulebody  */
  YYSYMBOL_R_netlist = 99,                 /* R_netlist  */
  YYSYMBOL_100_R_l_instance = 100,         /* R_l.instance  */
  YYSYMBOL_101_R_d_terminal = 101,         /* R_d.terminal  */
  YYSYMBOL_102_R_l_terminal_0 = 102,       /* R_l.terminal.0  */
  YYSYMBOL_103_R_l_terminal = 103,         /* R_l.terminal  */
  YYSYMBOL_104_R_s_terminal = 104,         /* R_s.terminal  */
  YYSYMBOL_105_R_l_declaration = 105,      /* R_l.declaration  */
  YYSYMBOL_106_R_s_declaration_withattribute = 106, /* R_s.declaration.withattribute  */
  YYSYMBOL_107_R_d_attribute_global = 107, /* R_d.attribute.global  */
  YYSYMBOL_108_R_s_declaration = 108,      /* R_s.declaration  */
  YYSYMBOL_109_3 = 109,                    /* $@3  */
  YYSYMBOL_110_4 = 110,                    /* $@4  */
  YYSYMBOL_111_5 = 111,                    /* $@5  */
  YYSYMBOL_112_R_d_node = 112,             /* R_d.node  */
  YYSYMBOL_113_6 = 113,                    /* $@6  */
  YYSYMBOL_114_7 = 114,                    /* $@7  */
  YYSYMBOL_115_8 = 115,                    /* $@8  */
  YYSYMBOL_116_R_node_type = 116,          /* R_node.type  */
  YYSYMBOL_117_R_l_terminalnode = 117,     /* R_l.terminalnode  */
  YYSYMBOL_118_R_l_node = 118,             /* R_l.node  */
  YYSYMBOL_119_R_s_terminalnode = 119,     /* R_s.terminalnode  */
  YYSYMBOL_120_R_s_node = 120,             /* R_s.node  */
  YYSYMBOL_121_R_d_branch = 121,           /* R_d.branch  */
  YYSYMBOL_122_R_l_branchalias = 122,      /* R_l.branchalias  */
  YYSYMBOL_123_R_s_branchalias = 123,      /* R_s.branchalias  */
  YYSYMBOL_124_R_s_branch = 124,           /* R_s.branch  */
  YYSYMBOL_125_R_d_analogfunction = 125,   /* R_d.analogfunction  */
  YYSYMBOL_126_R_d_analogfunction_proto = 126, /* R_d.analogfunction.proto  */
  YYSYMBOL_127_R_d_analogfunction_name = 127, /* R_d.analogfunction.name  */
  YYSYMBOL_128_R_l_analogfunction_declaration = 128, /* R_l.analogfunction.declaration  */
  YYSYMBOL_129_R_s_analogfunction_declaration = 129, /* R_s.analogfunction.declaration  */
  YYSYMBOL_130_R_l_analogfunction_input_variable = 130, /* R_l.analogfunction.input.variable  */
  YYSYMBOL_131_R_l_analogfunction_output_variable = 131, /* R_l.analogfunction.output.variable  */
  YYSYMBOL_132_R_l_analogfunction_inout_variable = 132, /* R_l.analogfunction.inout.variable  */
  YYSYMBOL_133_R_l_analogfunction_integer_variable = 133, /* R_l.analogfunction.integer.variable  */
  YYSYMBOL_134_R_l_analogfunction_real_variable = 134, /* R_l.analogfunction.real.variable  */
  YYSYMBOL_135_R_variable_type_set = 135,  /* R_variable.type.set  */
  YYSYMBOL_136_R_variable_type = 136,      /* R_variable.type  */
  YYSYMBOL_137_9 = 137,                    /* $@9  */
  YYSYMBOL_138_R_d_variable_end = 138,     /* R_d.variable.end  */
  YYSYMBOL_139_R_l_parameter = 139,        /* R_l.parameter  */
  YYSYMBOL_140_R_l_variable = 140,         /* R_l.variable  */
  YYSYMBOL_141_R_d_aliasparameter = 141,   /* R_d.aliasparameter  */
  YYSYMBOL_142_R_d_aliasparameter_token = 142, /* R_d.aliasparameter.token  */
  YYSYMBOL_143_R_s_parameter = 143,        /* R_s.parameter  */
  YYSYMBOL_144_R_s_variable = 144,         /* R_s.variable  */
  YYSYMBOL_145_R_s_parameter_name = 145,   /* R_s.parameter.name  */
  YYSYMBOL_146_R_s_variable_name = 146,    /* R_s.variable.name  */
  YYSYMBOL_147_R_s_parameter_range = 147,  /* R_s.parameter.range  */
  YYSYMBOL_148_R_l_interval = 148,         /* R_l.interval  */
  YYSYMBOL_149_R_s_interval = 149,         /* R_s.interval  */
  YYSYMBOL_150_R_d_interval = 150,         /* R_d.interval  */
  YYSYMBOL_151_R_interval_inf = 151,       /* R_interval.inf  */
  YYSYMBOL_152_R_interval_sup = 152,       /* R_interval.sup  */
  YYSYMBOL_R_analog = 153,                 /* R_analog  */
  YYSYMBOL_154_10 = 154,                   /* $@10  */
  YYSYMBOL_R_analogcode = 155,             /* R_analogcode  */
  YYSYMBOL_156_R_l_expression = 156,       /* R_l.expression  */
  YYSYMBOL_157_R_analogcode_atomic = 157,  /* R_analogcode.atomic  */
  YYSYMBOL_158_R_analogcode_block = 158,   /* R_analogcode.block  */
  YYSYMBOL_159_R_analogcode_block_atevent = 159, /* R_analogcode.block.atevent  */
  YYSYMBOL_160_R_l_analysis = 160,         /* R_l.analysis  */
  YYSYMBOL_161_R_s_analysis = 161,         /* R_s.analysis  */
  YYSYMBOL_162_R_d_block = 162,            /* R_d.block  */
  YYSYMBOL_163_R_d_block_begin = 163,      /* R_d.block.begin  */
  YYSYMBOL_164_R_l_blockitem = 164,        /* R_l.blockitem  */
  YYSYMBOL_165_R_d_blockvariable = 165,    /* R_d.blockvariable  */
  YYSYMBOL_166_R_l_blockvariable = 166,    /* R_l.blockvariable  */
  YYSYMBOL_167_R_s_blockvariable = 167,    /* R_s.blockvariable  */
  YYSYMBOL_168_R_d_contribution = 168,     /* R_d.contribution  */
  YYSYMBOL_R_contribution = 169,           /* R_contribution  */
  YYSYMBOL_R_source = 170,                 /* R_source  */
  YYSYMBOL_171_R_d_while = 171,            /* R_d.while  */
  YYSYMBOL_172_R_d_for = 172,              /* R_d.for  */
  YYSYMBOL_173_R_d_case = 173,             /* R_d.case  */
  YYSYMBOL_174_R_l_case_item = 174,        /* R_l.case.item  */
  YYSYMBOL_175_R_s_case_item = 175,        /* R_s.case.item  */
  YYSYMBOL_176_R_s_paramlist_0 = 176,      /* R_s.paramlist.0  */
  YYSYMBOL_177_R_s_instance = 177,         /* R_s.instance  */
  YYSYMBOL_178_R_instance_module_name = 178, /* R_instance.module.name  */
  YYSYMBOL_179_R_l_instance_parameter = 179, /* R_l.instance.parameter  */
  YYSYMBOL_180_R_s_instance_parameter = 180, /* R_s.instance.parameter  */
  YYSYMBOL_181_R_s_assignment = 181,       /* R_s.assignment  */
  YYSYMBOL_182_R_d_conditional = 182,      /* R_d.conditional  */
  YYSYMBOL_183_R_s_expression = 183,       /* R_s.expression  */
  YYSYMBOL_184_R_l_enode = 184,            /* R_l.enode  */
  YYSYMBOL_185_R_s_function_expression = 185, /* R_s.function_expression  */
  YYSYMBOL_R_expression = 186,             /* R_expression  */
  YYSYMBOL_187_R_e_conditional = 187,      /* R_e.conditional  */
  YYSYMBOL_188_R_e_bitwise_equ = 188,      /* R_e.bitwise_equ  */
  YYSYMBOL_189_R_e_bitwise_xor = 189,      /* R_e.bitwise_xor  */
  YYSYMBOL_190_R_e_bitwise_or = 190,       /* R_e.bitwise_or  */
  YYSYMBOL_191_R_e_bitwise_and = 191,      /* R_e.bitwise_and  */
  YYSYMBOL_192_R_e_logical_or = 192,       /* R_e.logical_or  */
  YYSYMBOL_193_R_e_logical_and = 193,      /* R_e.logical_and  */
  YYSYMBOL_194_R_e_comp_equ = 194,         /* R_e.comp_equ  */
  YYSYMBOL_195_R_e_comp = 195,             /* R_e.comp  */
  YYSYMBOL_196_R_e_bitwise_shift = 196,    /* R_e.bitwise_shift  */
  YYSYMBOL_197_R_e_arithm_add = 197,       /* R_e.arithm_add  */
  YYSYMBOL_198_R_e_arithm_mult = 198,      /* R_e.arithm_mult  */
  YYSYMBOL_199_R_e_unary = 199,            /* R_e.unary  */
  YYSYMBOL_200_R_e_atomic = 200            /* R_e.atomic  */
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
typedef yytype_int16 yy_state_t;

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

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
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
#define YYFINAL  20
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   634

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  79
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  122
/* YYNRULES -- Number of rules.  */
#define YYNRULES  263
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  514

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   307


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
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
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
  "\"end of file\"", "error", "\"invalid token\"", "PREC_IF_THEN",
  "tk_aliasparam", "tk_aliasparameter", "tk_analog", "tk_and",
  "tk_anystring", "tk_anytext", "tk_begin", "tk_beginattribute",
  "tk_bitwise_equr", "tk_branch", "tk_case", "tk_char", "tk_default",
  "tk_disc_id", "tk_discipline", "tk_dollar_ident", "tk_domain", "tk_else",
  "tk_end", "tk_endattribute", "tk_endcase", "tk_enddiscipline",
  "tk_endfunction", "tk_endmodule", "tk_endnature", "tk_exclude",
  "tk_flow", "tk_for", "tk_from", "tk_function", "tk_ground", "tk_ident",
  "tk_if", "tk_inf", "tk_inout", "tk_input", "tk_integer", "tk_module",
  "tk_nature", "tk_number", "tk_op_shl", "tk_op_shr", "tk_or", "tk_output",
  "tk_parameter", "tk_potential", "tk_real", "tk_string", "tk_while",
  "';'", "'='", "'('", "')'", "','", "'{'", "'}'", "'['", "':'", "']'",
  "'-'", "'+'", "'@'", "'<'", "'#'", "'.'", "'?'", "'~'", "'^'", "'|'",
  "'&'", "'!'", "'>'", "'*'", "'/'", "'%'", "$accept", "R_admsParse",
  "R_l.admsParse", "R_s.admsParse", "R_discipline_member",
  "R_discipline_name", "R_l.discipline_assignment",
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
  "R_e.arithm_mult", "R_e.unary", "R_e.atomic", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
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
#endif

#define YYPACT_NINF (-344)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-61)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      17,    29,    45,    60,   113,    98,  -344,  -344,  -344,    80,
    -344,  -344,  -344,  -344,   122,    66,  -344,  -344,     2,   171,
    -344,  -344,   196,   244,  -344,  -344,   220,   241,   241,   212,
    -344,   209,    15,  -344,  -344,  -344,   216,  -344,   236,   251,
    -344,  -344,    39,  -344,  -344,   259,  -344,  -344,  -344,   266,
     276,    52,   262,  -344,  -344,  -344,   281,  -344,  -344,   286,
     265,  -344,   493,  -344,   337,   262,  -344,  -344,   319,   301,
    -344,  -344,  -344,  -344,  -344,  -344,  -344,   330,  -344,  -344,
    -344,  -344,   342,  -344,    77,   493,  -344,   532,  -344,  -344,
    -344,  -344,  -344,   246,  -344,  -344,  -344,   336,   339,  -344,
     317,   332,  -344,   137,   465,   354,   343,   355,   355,   101,
     357,  -344,  -344,   339,  -344,  -344,  -344,   319,  -344,   360,
     362,   369,   370,   374,   381,    16,  -344,   337,   357,   363,
     339,   364,   383,  -344,  -344,   385,   385,   371,   367,    69,
     368,   134,   373,   375,  -344,    62,    21,   396,  -344,  -344,
    -344,   337,  -344,   160,  -344,   337,   372,  -344,  -344,  -344,
     389,  -344,    47,  -344,   337,    76,  -344,   108,   357,   394,
     111,  -344,   337,   391,   339,   337,   133,  -344,  -344,   173,
    -344,   207,  -344,   221,  -344,   222,  -344,   234,   439,  -344,
     426,  -344,   242,  -344,   337,   424,   393,   408,   416,   417,
    -344,   351,  -344,    22,    35,   351,   436,   351,   351,   351,
    -344,   437,  -344,   438,   438,   438,  -344,   103,  -344,  -344,
     442,  -344,   188,   425,   419,  -344,   450,   451,  -344,  -344,
     355,  -344,   111,   448,  -344,   357,  -344,  -344,    92,  -344,
    -344,   360,  -344,   454,  -344,   456,  -344,   457,  -344,   458,
    -344,   460,  -344,   357,  -344,  -344,   337,   467,    81,  -344,
     355,  -344,  -344,  -344,  -344,   452,    65,   468,   470,   351,
      91,    91,    91,    91,   453,  -344,  -344,    24,   440,   441,
     435,   466,   507,    44,   -22,   104,   170,   139,  -344,  -344,
     462,   250,  -344,   149,   396,   463,  -344,   269,   459,   464,
     469,   189,   474,   245,  -344,   252,   264,   351,   351,   415,
    -344,  -344,  -344,   351,  -344,   472,  -344,   479,  -344,  -344,
     461,  -344,   351,   172,  -344,  -344,  -344,  -344,  -344,  -344,
    -344,   471,   484,  -344,   393,   284,   351,   351,   351,  -344,
    -344,   486,  -344,  -344,  -344,  -344,   165,   351,   351,   455,
     351,   351,   351,   351,   351,   494,   496,   203,   273,   351,
     351,   351,   351,   351,   351,   351,  -344,   498,   351,   351,
    -344,   488,   499,   465,   465,   511,  -344,   512,  -344,   438,
    -344,  -344,  -344,   485,  -344,   422,  -344,   450,   450,   514,
     126,   275,   275,  -344,   172,  -344,  -344,   351,  -344,   502,
     294,  -344,  -344,   298,   497,  -344,   429,     5,  -344,   263,
     440,   495,   351,   441,   435,   466,   507,    44,   351,   351,
     351,   104,   351,   104,   170,   170,   139,   139,  -344,  -344,
    -344,  -344,  -344,   505,   504,   351,   540,  -344,  -344,   304,
    -344,   501,  -344,   509,  -344,  -344,   472,   503,   172,   392,
     392,  -344,  -344,  -344,  -344,   513,  -344,  -344,   351,  -344,
    -344,   465,  -344,  -344,  -344,   465,   351,   440,   -22,   -22,
     104,   104,    35,  -344,  -344,   465,   517,   511,   535,   351,
    -344,  -344,   549,   515,  -344,   486,   516,  -344,  -344,  -344,
    -344,  -344,   525,  -344,  -344,  -344,   526,  -344,  -344,   338,
     338,   465,  -344,  -344,   559,   192,  -344,   206,  -344,  -344,
    -344,  -344,  -344,  -344
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int16 yydefact[] =
{
      23,     0,     0,     0,     0,     2,     3,     6,     7,     0,
      24,     5,    26,    27,     0,     0,    28,     9,     0,     0,
       1,     4,     0,     0,    25,    29,     0,     0,     0,     0,
      10,     0,     0,    17,    31,    30,     0,    15,     0,     0,
       8,    11,     0,    16,    18,     0,    14,    13,    12,     0,
       0,     0,    46,    32,    21,    22,     0,    19,    50,     0,
      47,    48,    34,    20,    23,     0,   122,   121,   145,     0,
      71,    69,   199,    75,    73,   110,    74,    58,   111,   112,
      66,    55,     0,    36,    39,    35,    51,     0,    53,    56,
      67,    57,    65,     0,   113,    62,    64,     0,    38,    43,
     196,     0,    49,     0,    23,     0,     0,     0,     0,     0,
       0,    33,   145,    40,    44,    37,    52,     0,    54,     0,
       0,     0,     0,     0,     0,    23,    93,    23,     0,     0,
      41,     0,     0,    45,    92,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   161,     0,     0,    24,   146,   147,
     148,    23,   162,    23,   152,    23,     0,   155,   157,   156,
       0,   154,     0,    82,    23,     0,    78,     0,     0,   127,
       0,   116,    23,     0,    42,    23,     0,    76,   104,     0,
     100,     0,   106,     0,   102,     0,   108,     0,     0,    94,
       0,   114,     0,   118,    23,     0,     0,     0,     0,     0,
      89,     0,   160,     0,     0,     0,     0,     0,     0,     0,
     165,     0,   174,     0,     0,     0,   151,     0,   163,   170,
       0,   175,    23,     0,     0,   153,     0,     0,    81,    72,
       0,    70,     0,     0,   115,     0,    61,   123,     0,    80,
      68,     0,    97,     0,    95,     0,    98,     0,    96,     0,
      99,     0,    88,     0,    63,   124,    23,     0,     0,   200,
       0,    90,    91,   257,   256,   259,   258,   252,   254,     0,
       0,     0,     0,     0,     0,   209,   213,   214,   216,   219,
     221,   223,   225,   227,   229,   232,   237,   240,   243,   247,
       0,     0,   149,     0,     0,     0,   203,     0,     0,     0,
       0,     0,   182,     0,   180,     0,     0,     0,     0,    23,
     172,   176,   184,     0,    85,    87,    83,     0,    79,    59,
       0,   117,     0,   129,    77,   105,   101,   107,   103,   109,
     119,     0,     0,   197,     0,     0,     0,     0,     0,   253,
     255,     0,   249,   248,   251,   250,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   159,     0,     0,     0,
     187,     0,     0,    23,    23,     0,   166,     0,   177,     0,
     178,   179,   204,     0,   171,    23,   185,     0,     0,     0,
       0,     0,     0,   125,   130,   131,   120,     0,   201,     0,
       0,   210,   212,     0,     0,   263,    23,     0,   191,     0,
     217,     0,     0,   220,   222,   224,   226,   228,     0,     0,
       0,   233,     0,   235,   239,   238,   242,   241,   244,   245,
     246,   158,   150,     0,     0,     0,   207,   188,   169,     0,
     167,     0,   181,     0,   173,    84,    86,     0,   129,     0,
       0,   134,   139,   133,   132,     0,   198,   261,     0,   262,
     260,    23,   195,   190,   192,    23,     0,   218,   230,   231,
     234,   236,     0,   186,   205,    23,     0,     0,     0,     0,
     128,   126,     0,     0,   140,   209,     0,   202,   211,   194,
     193,   215,     0,   208,   164,   168,     0,   206,   141,     0,
       0,    23,   183,   143,     0,     0,   142,     0,   189,   144,
     135,   136,   137,   138
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -344,  -344,  -344,   582,  -344,  -344,  -344,   561,   563,  -344,
    -344,   565,     3,   -62,  -344,   578,  -344,  -344,  -344,  -344,
     510,   -64,  -344,  -344,  -344,   533,  -344,   518,  -344,   519,
    -344,  -344,  -344,  -344,  -344,  -344,  -344,  -344,  -344,  -101,
     359,   377,  -344,   213,   218,  -344,  -344,  -344,   228,  -344,
     483,  -344,  -344,  -344,  -344,  -344,  -344,   500,  -344,  -159,
     443,  -344,  -344,  -344,   378,   365,  -344,  -117,   162,  -344,
     223,   224,   169,   112,   531,  -344,  -103,   299,  -344,   506,
    -344,  -344,   143,   473,  -344,   313,  -344,   153,   247,  -344,
    -344,  -344,  -344,  -344,  -344,  -344,   225,  -344,   -72,  -344,
    -344,   289,  -200,  -344,  -199,    40,   167,  -193,  -343,  -344,
    -328,   277,   278,   282,   280,   274,   -39,  -332,    23,    26,
     -32,  -255
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     4,     5,     6,     7,    18,    29,    30,    38,     8,
      32,    33,   146,    10,    15,    16,    11,    45,    62,    82,
      83,    84,    53,    59,    60,    61,    85,    86,    87,    88,
     109,   110,   128,    89,   119,   108,   107,    90,   176,   165,
     177,   166,    91,   315,   316,   106,    92,    93,   137,   125,
     126,   181,   185,   179,   183,   187,    94,    95,   127,   236,
     170,   192,    96,    97,   171,   193,   172,   173,   393,   394,
     395,   451,   483,   505,    98,   104,   221,   291,   149,   150,
     151,   439,   440,   152,   153,   222,   216,   303,   304,   154,
     155,   156,   157,   158,   159,   407,   408,   132,    99,   100,
     258,   259,   160,   161,   292,   409,   401,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      81,   148,   274,     9,   295,   411,   296,   167,     9,   299,
     300,   194,   114,   263,   298,   342,   343,   344,   345,   410,
     264,   406,    26,    81,   265,   421,   423,     1,     1,   463,
     263,   212,    27,   254,   130,     2,   347,   264,    12,   323,
     266,   265,   147,    43,   357,   267,     1,    49,   268,   174,
      31,    28,    13,   358,   120,   121,   122,   266,   114,     3,
     269,   213,   267,   123,    14,   268,   124,   101,   270,   271,
     293,   214,   215,   319,    50,   272,   341,   269,   290,   273,
      17,   145,    51,   112,   467,   270,   271,    56,   470,    24,
     471,   147,   272,   348,   349,    19,   273,   210,   355,   263,
     263,    14,   114,   226,   227,    57,   264,   264,   382,     1,
     265,   265,    72,    20,   386,   383,     2,   211,   356,   311,
     337,    22,   202,   491,   203,   338,   266,   266,   188,   229,
     191,   267,   267,   230,   268,   268,   194,   333,   334,   -23,
       3,    75,   294,   402,   402,   404,   269,   269,   359,   360,
     322,    78,    79,   402,   188,   270,   271,   307,   223,   335,
     147,   231,   272,   308,   234,   230,   273,   228,   235,   432,
     433,     1,   134,   263,   138,   237,    23,   135,   239,   139,
     264,   406,   219,   368,   265,   448,   240,   136,   205,   206,
     241,   140,   452,   452,   207,   141,   142,   255,   455,     1,
     266,   391,   138,   205,   392,   267,    31,   139,   268,   207,
     310,   263,   143,   144,   402,   363,   364,   365,   264,   140,
     269,   220,   265,   141,   142,   145,   242,   342,   270,   271,
     243,    34,    26,   361,   362,   272,   474,    40,   266,   273,
     143,   144,    27,   267,   375,   376,   268,   147,   510,   343,
     484,   484,    35,   145,   511,    36,   485,   420,   269,   331,
     244,    28,   512,    42,   245,   402,   270,   271,   513,    46,
     436,   437,   492,   272,   246,   248,    37,   273,   247,   249,
     497,   263,   311,   263,   120,   121,   122,   250,   264,    47,
     264,   251,   265,   123,   265,   234,   124,    58,   378,   253,
     506,   506,   379,   462,    48,   380,   367,   368,   266,   379,
     266,   147,   147,   267,    52,   267,   268,   381,   268,    54,
     458,   379,    65,   147,   465,   370,   371,   422,   269,    55,
     449,   428,   429,   430,    63,   450,   270,   271,   270,   271,
     399,   230,    64,   272,   147,   272,   263,   273,     1,   273,
     457,   458,   103,   264,   459,   458,   105,   265,   489,   263,
     476,   477,   490,   198,   199,   -60,   264,   305,   306,   111,
     265,   129,   493,   266,    72,   503,   400,   403,   267,   468,
     469,   268,   424,   425,   131,   133,   266,   426,   427,   162,
     164,   267,   169,   269,   268,   175,   163,   178,   508,   147,
     263,   270,   504,   147,   180,   182,   269,   264,   272,   184,
     294,   265,   273,   147,   270,   271,   186,   195,   197,   196,
     134,   272,   201,   204,   200,   273,     1,   266,   208,   138,
     209,   217,   267,     1,   139,   268,   138,   384,   224,   147,
       1,   139,   225,   138,   444,   238,   140,   269,   139,   212,
     141,   142,   252,   140,   233,   482,   271,   141,   142,   256,
     140,   257,   272,   260,   141,   142,   273,   143,   144,   261,
     262,   297,   301,   302,   143,   144,     1,   309,   312,   138,
     145,   143,   144,   313,   139,   314,   317,   145,   320,   325,
     461,   326,   327,   328,   145,   329,   140,    66,    67,    68,
     141,   142,   332,   339,     1,   340,    69,   336,   352,   346,
      70,   350,   353,   351,   354,   366,   369,   143,   144,   438,
     373,   372,   389,   434,   396,   374,   412,    71,    72,   387,
     145,    73,    74,    75,   377,   388,    66,    67,   117,   397,
      76,    77,   405,    78,    79,    69,    80,   443,   418,    70,
     419,   431,   441,   435,   447,   456,   466,   263,   472,   460,
     473,   475,   478,   479,   264,   480,    71,   263,   265,   487,
      73,    74,    75,   494,   264,   496,   499,   500,   265,    76,
      77,   501,    78,    79,   266,    80,   498,    21,   502,   267,
      41,    39,   268,    25,   266,   115,   509,    44,   102,   267,
     324,   446,   268,   116,   269,   445,   118,   318,   189,   168,
     481,   232,   507,   321,   269,   113,   453,   454,   330,   486,
     495,   390,   385,   398,   218,   488,   442,   413,   417,   414,
       0,   190,   464,   416,   415
};

static const yytype_int16 yycheck[] =
{
      62,   104,   201,     0,   204,   348,   205,   108,     5,   208,
     209,   128,    84,     8,   207,   270,   271,   272,   273,   347,
      15,    16,    20,    85,    19,   357,   358,    11,    11,    24,
       8,    10,    30,   192,    98,    18,    12,    15,     9,   238,
      35,    19,   104,    28,    66,    40,    11,     8,    43,   113,
      35,    49,    23,    75,    38,    39,    40,    35,   130,    42,
      55,    40,    40,    47,    35,    43,    50,    64,    63,    64,
      35,    50,    51,   232,    35,    70,   269,    55,    56,    74,
      35,    65,    43,     6,   412,    63,    64,    35,   420,    23,
     422,   153,    70,    69,    70,    35,    74,    35,    54,     8,
       8,    35,   174,    56,    57,    53,    15,    15,   307,    11,
      19,    19,    35,     0,   313,   308,    18,    55,    74,   222,
      55,    41,    53,   466,    55,    60,    35,    35,   125,    53,
     127,    40,    40,    57,    43,    43,   253,    56,    57,    41,
      42,    40,   204,   336,   337,   338,    55,    55,    44,    45,
      58,    50,    51,   346,   151,    63,    64,    54,   155,   260,
     222,    53,    70,    60,    53,    57,    74,   164,    57,   368,
     369,    11,    35,     8,    14,   172,    54,    40,   175,    19,
      15,    16,    22,    57,    19,    59,    53,    50,    54,    55,
      57,    31,   391,   392,    60,    35,    36,   194,   397,    11,
      35,    29,    14,    54,    32,    40,    35,    19,    43,    60,
      22,     8,    52,    53,   407,    76,    77,    78,    15,    31,
      55,    61,    19,    35,    36,    65,    53,   482,    63,    64,
      57,    35,    20,    63,    64,    70,   435,    25,    35,    74,
      52,    53,    30,    40,    55,    56,    43,   309,    56,   504,
     449,   450,     8,    65,    62,    35,   449,    54,    55,   256,
      53,    49,    56,    54,    57,   458,    63,    64,    62,    53,
     373,   374,   472,    70,    53,    53,    35,    74,    57,    57,
     479,     8,   385,     8,    38,    39,    40,    53,    15,    53,
      15,    57,    19,    47,    19,    53,    50,    35,    53,    57,
     499,   500,    57,   406,    53,    53,    56,    57,    35,    57,
      35,   373,   374,    40,    55,    40,    43,    53,    43,    53,
      57,    57,    57,   385,    61,    56,    57,    54,    55,    53,
      55,   363,   364,   365,    53,    60,    63,    64,    63,    64,
      56,    57,    56,    70,   406,    70,     8,    74,    11,    74,
      56,    57,    33,    15,    56,    57,    55,    19,   461,     8,
      56,    57,   465,   135,   136,    35,    15,   214,   215,    27,
      19,    35,   475,    35,    35,    37,   336,   337,    40,   418,
     419,    43,   359,   360,    67,    53,    35,   361,   362,    35,
      35,    40,    35,    55,    43,    35,    53,    35,   501,   461,
       8,    63,    64,   465,    35,    35,    55,    15,    70,    35,
     472,    19,    74,   475,    63,    64,    35,    54,    35,    55,
      35,    70,    55,    55,    53,    74,    11,    35,    55,    14,
      55,    35,    40,    11,    19,    43,    14,    22,    66,   501,
      11,    19,    53,    14,    22,    54,    31,    55,    19,    10,
      35,    36,    26,    31,    60,    63,    64,    35,    36,    35,
      31,    68,    70,    55,    35,    36,    74,    52,    53,    53,
      53,    35,    35,    35,    52,    53,    11,    35,    53,    14,
      65,    52,    53,    64,    19,    35,    35,    65,    40,    35,
      61,    35,    35,    35,    65,    35,    31,     4,     5,     6,
      35,    36,    35,    35,    11,    35,    13,    55,    73,    56,
      17,    71,    46,    72,     7,    53,    53,    52,    53,     8,
      56,    62,    61,    35,    53,    56,    71,    34,    35,    57,
      65,    38,    39,    40,    60,    56,     4,     5,     6,    55,
      47,    48,    56,    50,    51,    13,    53,    62,    54,    17,
      54,    53,    40,    54,    40,    53,    61,     8,    53,    62,
      56,    21,    61,    54,    15,    62,    34,     8,    19,    56,
      38,    39,    40,    56,    15,    40,    61,    61,    19,    47,
      48,    56,    50,    51,    35,    53,    37,     5,    62,    40,
      29,    28,    43,    15,    35,    85,    37,    32,    65,    40,
     241,   388,    43,    85,    55,   387,    87,   230,   125,   109,
     448,   168,   500,   235,    55,    84,   392,   394,   253,   450,
     477,   322,   309,   334,   151,   458,   379,   350,   354,   351,
      -1,   125,   407,   353,   352
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    11,    18,    42,    80,    81,    82,    83,    88,    91,
      92,    95,     9,    23,    35,    93,    94,    35,    84,    35,
       0,    82,    41,    54,    23,    94,    20,    30,    49,    85,
      86,    35,    89,    90,    35,     8,    35,    35,    87,    87,
      25,    86,    54,    28,    90,    96,    53,    53,    53,     8,
      35,    43,    55,   101,    53,    53,    35,    53,    35,   102,
     103,   104,    97,    53,    56,    57,     4,     5,     6,    13,
      17,    34,    35,    38,    39,    40,    47,    48,    50,    51,
      53,    92,    98,    99,   100,   105,   106,   107,   108,   112,
     116,   121,   125,   126,   135,   136,   141,   142,   153,   177,
     178,    91,   104,    33,   154,    55,   124,   115,   114,   109,
     110,    27,     6,   153,   177,    99,   106,     6,   108,   113,
      38,    39,    40,    47,    50,   128,   129,   137,   111,    35,
     100,    67,   176,    53,    35,    40,    50,   127,    14,    19,
      31,    35,    36,    52,    53,    65,    91,    92,   155,   157,
     158,   159,   162,   163,   168,   169,   170,   171,   172,   173,
     181,   182,    35,    53,    35,   118,   120,   118,   136,    35,
     139,   143,   145,   146,   100,    35,   117,   119,    35,   132,
      35,   130,    35,   133,    35,   131,    35,   134,    91,   129,
     158,    91,   140,   144,   146,    54,    55,    35,   127,   127,
      53,    55,    53,    55,    55,    54,    55,    60,    55,    55,
      35,    55,    10,    40,    50,    51,   165,    35,   162,    22,
      61,   155,   164,    91,    66,    53,    56,    57,    91,    53,
      57,    53,   139,    60,    53,    57,   138,    91,    54,    91,
      53,    57,    53,    57,    53,    57,    53,    57,    53,    57,
      53,    57,    26,    57,   138,    91,    35,    68,   179,   180,
      55,    53,    53,     8,    15,    19,    35,    40,    43,    55,
      63,    64,    70,    74,   183,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
      56,   156,   183,    35,    92,   181,   183,    35,   186,   183,
     183,    35,    35,   166,   167,   166,   166,    54,    60,    35,
      22,   155,    53,    64,    35,   122,   123,    35,   120,   138,
      40,   143,    58,   183,   119,    35,    35,    35,    35,    35,
     144,    91,    35,    56,    57,   118,    55,    55,    60,    35,
      35,   186,   200,   200,   200,   200,    56,    12,    69,    70,
      71,    72,    73,    46,     7,    54,    74,    66,    75,    44,
      45,    63,    64,    76,    77,    78,    53,    56,    57,    53,
      56,    57,    62,    56,    56,    55,    56,    60,    53,    57,
      53,    53,   183,   186,    22,   164,   183,    57,    56,    61,
     156,    29,    32,   147,   148,   149,    53,    55,   180,    56,
     184,   185,   186,   184,   186,    56,    16,   174,   175,   184,
     189,   187,    71,   190,   191,   192,   193,   194,    54,    54,
      54,   196,    54,   196,   197,   197,   198,   198,   199,   199,
     199,    53,   183,   183,    35,    54,   155,   155,     8,   160,
     161,    40,   167,    62,    22,   123,   122,    40,    59,    55,
      60,   150,   183,   150,   149,   183,    53,    56,    57,    56,
      62,    61,   155,    24,   175,    61,    61,   189,   195,   195,
     196,   196,    53,    56,   183,    21,    56,    57,    61,    54,
      62,   147,    63,   151,   183,   186,   151,    56,   185,   155,
     155,   187,   181,   155,    56,   161,    40,   183,    37,    61,
      61,    56,    62,    37,    64,   152,   183,   152,   155,    37,
      56,    62,    56,    62
};

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

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
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


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


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

/* This macro is provided for backward compatibility. */
# ifndef YY_LOCATION_PRINT
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif


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
# ifdef YYPRINT
  if (yykind < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yykind], *yyvaluep);
# endif
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
    goto yyexhaustedlab;
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
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
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
  case 2: /* R_admsParse: R_l.admsParse  */
#line 296 "verilogaYacc.y"
          {
          }
#line 1970 "y.tab.c"
    break;

  case 3: /* R_l.admsParse: R_s.admsParse  */
#line 301 "verilogaYacc.y"
          {
          }
#line 1977 "y.tab.c"
    break;

  case 4: /* R_l.admsParse: R_l.admsParse R_s.admsParse  */
#line 304 "verilogaYacc.y"
          {
          }
#line 1984 "y.tab.c"
    break;

  case 5: /* R_s.admsParse: R_d.module  */
#line 309 "verilogaYacc.y"
          {
          }
#line 1991 "y.tab.c"
    break;

  case 6: /* R_s.admsParse: R_discipline_member  */
#line 312 "verilogaYacc.y"
          {
          }
#line 1998 "y.tab.c"
    break;

  case 7: /* R_s.admsParse: R_nature_member  */
#line 315 "verilogaYacc.y"
          {
          }
#line 2005 "y.tab.c"
    break;

  case 8: /* R_discipline_member: tk_discipline R_discipline_name R_l.discipline_assignment tk_enddiscipline  */
#line 320 "verilogaYacc.y"
          {
            adms_admsmain_list_discipline_prepend_once_or_abort(root(),gDiscipline);
            gDiscipline=NULL;
          }
#line 2014 "y.tab.c"
    break;

  case 9: /* R_discipline_name: tk_ident  */
#line 327 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[0]._lexval))->_string;
            gDiscipline=adms_discipline_new(mylexval1);
          }
#line 2023 "y.tab.c"
    break;

  case 10: /* R_l.discipline_assignment: R_s.discipline_assignment  */
#line 334 "verilogaYacc.y"
          {
          }
#line 2030 "y.tab.c"
    break;

  case 11: /* R_l.discipline_assignment: R_l.discipline_assignment R_s.discipline_assignment  */
#line 337 "verilogaYacc.y"
          {
          }
#line 2037 "y.tab.c"
    break;

  case 12: /* R_s.discipline_assignment: tk_potential R_discipline.naturename ';'  */
#line 342 "verilogaYacc.y"
          {
            gDiscipline->_potential=(p_nature)YY((yyvsp[-1]._yaccval));
          }
#line 2045 "y.tab.c"
    break;

  case 13: /* R_s.discipline_assignment: tk_flow R_discipline.naturename ';'  */
#line 346 "verilogaYacc.y"
          {
            gDiscipline->_flow=(p_nature)YY((yyvsp[-1]._yaccval));
          }
#line 2053 "y.tab.c"
    break;

  case 14: /* R_s.discipline_assignment: tk_domain tk_ident ';'  */
#line 350 "verilogaYacc.y"
          {
            char* mylexval2=((p_lexval)(yyvsp[-1]._lexval))->_string;
            if(!strcmp(mylexval2,"discrete"))
              gDiscipline->_domain=admse_discrete;
            else if(!strcmp(mylexval2,"continuous"))
              gDiscipline->_domain=admse_continuous;
            else
             adms_veriloga_message_fatal("domain: bad value given - should be either 'discrete' or 'continuous'\n",(yyvsp[-1]._lexval));
          }
#line 2067 "y.tab.c"
    break;

  case 15: /* R_discipline.naturename: tk_ident  */
#line 362 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[0]._lexval))->_string;
            p_nature mynature=lookup_nature(mylexval1);
            if(!mynature)
              adms_veriloga_message_fatal("can't find nature definition\n",(yyvsp[0]._lexval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mynature);
          }
#line 2080 "y.tab.c"
    break;

  case 16: /* R_nature_member: tk_nature tk_ident R_l.nature_assignment tk_endnature  */
#line 373 "verilogaYacc.y"
          {
            char* mylexval2=((p_lexval)(yyvsp[-2]._lexval))->_string;
            p_nature mynature=NULL;
            if(gNatureAccess) 
              mynature=adms_admsmain_list_nature_prepend_by_id_once_or_abort(root(),gNatureAccess);
            else
             adms_veriloga_message_fatal("attribute 'access' in nature definition not found\n",(yyvsp[-2]._lexval));
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
#line 2107 "y.tab.c"
    break;

  case 17: /* R_l.nature_assignment: R_s.nature_assignment  */
#line 398 "verilogaYacc.y"
          {
          }
#line 2114 "y.tab.c"
    break;

  case 18: /* R_l.nature_assignment: R_l.nature_assignment R_s.nature_assignment  */
#line 401 "verilogaYacc.y"
          {
          }
#line 2121 "y.tab.c"
    break;

  case 19: /* R_s.nature_assignment: tk_ident '=' tk_number ';'  */
#line 406 "verilogaYacc.y"
          {
            if(!strcmp((yyvsp[-3]._lexval)->_string,"abstol"))
            {
              if(gNatureAbsTol)
                adms_veriloga_message_fatal("nature attribute defined more than once\n",(yyvsp[-3]._lexval));
              gNatureAbsTol=adms_number_new((yyvsp[-1]._lexval));
            }
            else
             adms_veriloga_message_fatal("unknown nature attribute\n",(yyvsp[-3]._lexval));
          }
#line 2136 "y.tab.c"
    break;

  case 20: /* R_s.nature_assignment: tk_ident '=' tk_number tk_ident ';'  */
#line 417 "verilogaYacc.y"
          {
            char* mylexval4=((p_lexval)(yyvsp[-1]._lexval))->_string;
            admse myunit=admse_1;
            if(!strcmp((yyvsp[-4]._lexval)->_string,"abstol"))
            {
              if(gNatureAbsTol)
                adms_veriloga_message_fatal("nature attribute defined more than once\n",(yyvsp[-4]._lexval));
              gNatureAbsTol=adms_number_new((yyvsp[-2]._lexval));
            }
            else
             adms_veriloga_message_fatal("unknown nature attribute\n",(yyvsp[-4]._lexval));
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
              adms_veriloga_message_fatal("can not convert symbol to valid unit\n",(yyvsp[-1]._lexval));
            gNatureAbsTol->_scalingunit=myunit;
          }
#line 2175 "y.tab.c"
    break;

  case 21: /* R_s.nature_assignment: tk_ident '=' tk_anystring ';'  */
#line 452 "verilogaYacc.y"
          {
            char* mylexval3=((p_lexval)(yyvsp[-1]._lexval))->_string;
            if(!strcmp((yyvsp[-3]._lexval)->_string,"units"))
            {
              if(gNatureUnits)
                adms_veriloga_message_fatal("nature attribute defined more than once\n",(yyvsp[-3]._lexval));
              gNatureUnits=adms_kclone(mylexval3);
            }
            else
             adms_veriloga_message_fatal("unknown nature attribute\n",(yyvsp[-3]._lexval));
          }
#line 2191 "y.tab.c"
    break;

  case 22: /* R_s.nature_assignment: tk_ident '=' tk_ident ';'  */
#line 464 "verilogaYacc.y"
          {
            char* mylexval3=((p_lexval)(yyvsp[-1]._lexval))->_string;
            if(!strcmp((yyvsp[-3]._lexval)->_string,"access"))
            {
              if(gNatureAccess)
                adms_veriloga_message_fatal("nature attribute defined more than once\n",(yyvsp[-3]._lexval));
              gNatureAccess=adms_kclone(mylexval3);
            }
            else if(!strcmp((yyvsp[-3]._lexval)->_string,"idt_nature"))
            {
              if(gNatureidt)
                adms_veriloga_message_fatal("idt_nature attribute defined more than once\n",(yyvsp[-3]._lexval));
              gNatureidt=adms_kclone(mylexval3);
            }
            else if(!strcmp((yyvsp[-3]._lexval)->_string,"ddt_nature"))
            {
              if(gNatureddt)
                adms_veriloga_message_fatal("ddt_nature attribute defined more than once\n",(yyvsp[-3]._lexval));
              gNatureddt=adms_kclone(mylexval3);
            }
            else
             adms_veriloga_message_fatal("unknown nature attribute\n",(yyvsp[-3]._lexval));
          }
#line 2219 "y.tab.c"
    break;

  case 23: /* R_d.attribute.0: %empty  */
#line 490 "verilogaYacc.y"
          {
          }
#line 2226 "y.tab.c"
    break;

  case 24: /* R_d.attribute.0: R_d.attribute  */
#line 493 "verilogaYacc.y"
          {
          }
#line 2233 "y.tab.c"
    break;

  case 25: /* R_d.attribute: tk_beginattribute R_l.attribute tk_endattribute  */
#line 498 "verilogaYacc.y"
          {
          }
#line 2240 "y.tab.c"
    break;

  case 26: /* R_d.attribute: tk_beginattribute tk_anytext  */
#line 501 "verilogaYacc.y"
          {
            char* mylexval2=((p_lexval)(yyvsp[0]._lexval))->_string;
            p_attribute myattribute=adms_attribute_new("ibm");
            p_admst myconstant=adms_admst_newks(adms_kclone(mylexval2));
            myattribute->_value=(p_adms)myconstant;
            adms_slist_push(&gAttributeList,(p_adms)myattribute);
          }
#line 2252 "y.tab.c"
    break;

  case 27: /* R_d.attribute: tk_beginattribute tk_endattribute  */
#line 509 "verilogaYacc.y"
          {
          }
#line 2259 "y.tab.c"
    break;

  case 28: /* R_l.attribute: R_s.attribute  */
#line 514 "verilogaYacc.y"
          {
          }
#line 2266 "y.tab.c"
    break;

  case 29: /* R_l.attribute: R_l.attribute R_s.attribute  */
#line 517 "verilogaYacc.y"
          {
          }
#line 2273 "y.tab.c"
    break;

  case 30: /* R_s.attribute: tk_ident '=' tk_anystring  */
#line 522 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[-2]._lexval))->_string;
            char* mylexval3=((p_lexval)(yyvsp[0]._lexval))->_string;
            p_attribute myattribute=adms_attribute_new(mylexval1);
            p_admst myconstant=adms_admst_newks(adms_kclone(mylexval3));
            myattribute->_value=(p_adms)myconstant;
            adms_slist_push(&gAttributeList,(p_adms)myattribute);
          }
#line 2286 "y.tab.c"
    break;

  case 31: /* $@1: %empty  */
#line 533 "verilogaYacc.y"
          {
            char* mylexval3=((p_lexval)(yyvsp[0]._lexval))->_string;
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
#line 2305 "y.tab.c"
    break;

  case 32: /* $@2: %empty  */
#line 548 "verilogaYacc.y"
          {
            set_context(ctx_moduletop);
          }
#line 2313 "y.tab.c"
    break;

  case 33: /* R_d.module: R_d.attribute.0 tk_module tk_ident $@1 R_d.terminal $@2 R_modulebody tk_endmodule  */
#line 552 "verilogaYacc.y"
          {
            adms_slist_pull(&gBlockList);
            adms_slist_inreverse(&gModule->_assignment);
          }
#line 2322 "y.tab.c"
    break;

  case 34: /* R_modulebody: %empty  */
#line 559 "verilogaYacc.y"
          {
          }
#line 2329 "y.tab.c"
    break;

  case 35: /* R_modulebody: R_l.declaration  */
#line 562 "verilogaYacc.y"
          {
          }
#line 2336 "y.tab.c"
    break;

  case 36: /* R_modulebody: R_netlist  */
#line 565 "verilogaYacc.y"
          {
          }
#line 2343 "y.tab.c"
    break;

  case 37: /* R_modulebody: R_l.declaration R_netlist  */
#line 568 "verilogaYacc.y"
          {
          }
#line 2350 "y.tab.c"
    break;

  case 38: /* R_netlist: R_analog  */
#line 573 "verilogaYacc.y"
          {
          }
#line 2357 "y.tab.c"
    break;

  case 39: /* R_netlist: R_l.instance  */
#line 576 "verilogaYacc.y"
          {
          }
#line 2364 "y.tab.c"
    break;

  case 40: /* R_netlist: R_l.instance R_analog  */
#line 579 "verilogaYacc.y"
          {
          }
#line 2371 "y.tab.c"
    break;

  case 41: /* R_netlist: R_analog R_l.instance  */
#line 582 "verilogaYacc.y"
          {
          }
#line 2378 "y.tab.c"
    break;

  case 42: /* R_netlist: R_l.instance R_analog R_l.instance  */
#line 585 "verilogaYacc.y"
          {
          }
#line 2385 "y.tab.c"
    break;

  case 43: /* R_l.instance: R_s.instance  */
#line 590 "verilogaYacc.y"
          {
          }
#line 2392 "y.tab.c"
    break;

  case 44: /* R_l.instance: R_l.instance R_s.instance  */
#line 593 "verilogaYacc.y"
          {
          }
#line 2399 "y.tab.c"
    break;

  case 45: /* R_d.terminal: '(' R_l.terminal.0 ')' R_d.attribute.0 ';'  */
#line 598 "verilogaYacc.y"
          {
            p_slist l;
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&gModule->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
          }
#line 2410 "y.tab.c"
    break;

  case 46: /* R_l.terminal.0: %empty  */
#line 607 "verilogaYacc.y"
          {
          }
#line 2417 "y.tab.c"
    break;

  case 47: /* R_l.terminal.0: R_l.terminal  */
#line 610 "verilogaYacc.y"
          {
          }
#line 2424 "y.tab.c"
    break;

  case 48: /* R_l.terminal: R_s.terminal  */
#line 615 "verilogaYacc.y"
          {
          }
#line 2431 "y.tab.c"
    break;

  case 49: /* R_l.terminal: R_l.terminal ',' R_s.terminal  */
#line 618 "verilogaYacc.y"
          {
          }
#line 2438 "y.tab.c"
    break;

  case 50: /* R_s.terminal: tk_ident  */
#line 623 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[0]._lexval))->_string;
            p_nodealias mynodealias=adms_module_list_nodealias_prepend_by_id_once_or_abort(gModule,gModule,mylexval1); 
            p_node mynode=adms_module_list_node_prepend_by_id_once_or_abort(gModule,gModule,mylexval1); 
            mynodealias->_node=mynode;
            mynode->_location=admse_external;
          }
#line 2450 "y.tab.c"
    break;

  case 51: /* R_l.declaration: R_s.declaration.withattribute  */
#line 633 "verilogaYacc.y"
          {
          }
#line 2457 "y.tab.c"
    break;

  case 52: /* R_l.declaration: R_l.declaration R_s.declaration.withattribute  */
#line 636 "verilogaYacc.y"
          {
          }
#line 2464 "y.tab.c"
    break;

  case 53: /* R_s.declaration.withattribute: R_s.declaration  */
#line 641 "verilogaYacc.y"
          {
            set_context(ctx_moduletop);
          }
#line 2472 "y.tab.c"
    break;

  case 54: /* R_s.declaration.withattribute: R_d.attribute.global R_s.declaration  */
#line 645 "verilogaYacc.y"
          {
            adms_slist_free(gGlobalAttributeList); gGlobalAttributeList=NULL;
            set_context(ctx_moduletop);
          }
#line 2481 "y.tab.c"
    break;

  case 55: /* R_d.attribute.global: R_d.attribute  */
#line 652 "verilogaYacc.y"
          {
            gGlobalAttributeList=gAttributeList;
            gAttributeList=NULL;
          }
#line 2490 "y.tab.c"
    break;

  case 56: /* R_s.declaration: R_d.node  */
#line 659 "verilogaYacc.y"
          {
          }
#line 2497 "y.tab.c"
    break;

  case 57: /* R_s.declaration: R_d.branch  */
#line 662 "verilogaYacc.y"
          {
          }
#line 2504 "y.tab.c"
    break;

  case 58: /* $@3: %empty  */
#line 665 "verilogaYacc.y"
          {
            set_context(ctx_any);
          }
#line 2512 "y.tab.c"
    break;

  case 59: /* R_s.declaration: tk_parameter $@3 R_variable.type R_l.parameter R_d.variable.end  */
#line 669 "verilogaYacc.y"
          {
          }
#line 2519 "y.tab.c"
    break;

  case 60: /* $@4: %empty  */
#line 672 "verilogaYacc.y"
          {
            set_context(ctx_any);
          }
#line 2527 "y.tab.c"
    break;

  case 61: /* R_s.declaration: tk_parameter $@4 R_l.parameter R_d.variable.end  */
#line 676 "verilogaYacc.y"
          {
          }
#line 2534 "y.tab.c"
    break;

  case 62: /* $@5: %empty  */
#line 679 "verilogaYacc.y"
          {
            set_context(ctx_any);
          }
#line 2542 "y.tab.c"
    break;

  case 63: /* R_s.declaration: R_variable.type $@5 R_l.variable R_d.variable.end  */
#line 683 "verilogaYacc.y"
          {
          }
#line 2549 "y.tab.c"
    break;

  case 64: /* R_s.declaration: R_d.aliasparameter  */
#line 686 "verilogaYacc.y"
          {
          }
#line 2556 "y.tab.c"
    break;

  case 65: /* R_s.declaration: R_d.analogfunction  */
#line 689 "verilogaYacc.y"
          {
          }
#line 2563 "y.tab.c"
    break;

  case 66: /* R_s.declaration: ';'  */
#line 692 "verilogaYacc.y"
          {
          }
#line 2570 "y.tab.c"
    break;

  case 67: /* $@6: %empty  */
#line 697 "verilogaYacc.y"
          {
            set_context(ctx_any);
          }
#line 2578 "y.tab.c"
    break;

  case 68: /* R_d.node: R_node.type $@6 R_l.terminalnode ';'  */
#line 701 "verilogaYacc.y"
          {
            p_slist l;
            for(l=gTerminalList;l;l=l->next)
              ((p_node)l->data)->_direction=gNodeDirection;
            adms_slist_free(gTerminalList); gTerminalList=NULL;
          }
#line 2589 "y.tab.c"
    break;

  case 69: /* $@7: %empty  */
#line 708 "verilogaYacc.y"
          {
            set_context(ctx_any);
          }
#line 2597 "y.tab.c"
    break;

  case 70: /* R_d.node: tk_ground $@7 R_l.node ';'  */
#line 712 "verilogaYacc.y"
          {
            p_slist l;
            for(l=gNodeList;l;l=l->next)
              ((p_node)l->data)->_location=admse_ground;
            adms_slist_free(gNodeList); gNodeList=NULL;
          }
#line 2608 "y.tab.c"
    break;

  case 71: /* $@8: %empty  */
#line 719 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[0]._lexval))->_string;
            set_context(ctx_any);
            gDisc=mylexval1;
          }
#line 2618 "y.tab.c"
    break;

  case 72: /* R_d.node: tk_disc_id $@8 R_l.node ';'  */
#line 725 "verilogaYacc.y"
          {
            char* mydisciplinename=gDisc;
            p_discipline mydiscipline=adms_admsmain_list_discipline_lookup_by_id(root(),mydisciplinename);
            p_slist l;
            for(l=gNodeList;l;l=l->next)
              ((p_node)l->data)->_discipline=mydiscipline;
            adms_slist_free(gNodeList); gNodeList=NULL;
          }
#line 2631 "y.tab.c"
    break;

  case 73: /* R_node.type: tk_input  */
#line 736 "verilogaYacc.y"
          {
            gNodeDirection=admse_input;
          }
#line 2639 "y.tab.c"
    break;

  case 74: /* R_node.type: tk_output  */
#line 740 "verilogaYacc.y"
          {
            gNodeDirection=admse_output;
          }
#line 2647 "y.tab.c"
    break;

  case 75: /* R_node.type: tk_inout  */
#line 744 "verilogaYacc.y"
          {
            gNodeDirection=admse_inout;
          }
#line 2655 "y.tab.c"
    break;

  case 76: /* R_l.terminalnode: R_s.terminalnode  */
#line 750 "verilogaYacc.y"
          {
          }
#line 2662 "y.tab.c"
    break;

  case 77: /* R_l.terminalnode: R_l.terminalnode ',' R_s.terminalnode  */
#line 753 "verilogaYacc.y"
          {
          }
#line 2669 "y.tab.c"
    break;

  case 78: /* R_l.node: R_s.node  */
#line 758 "verilogaYacc.y"
          {
          }
#line 2676 "y.tab.c"
    break;

  case 79: /* R_l.node: R_l.node ',' R_s.node  */
#line 761 "verilogaYacc.y"
          {
          }
#line 2683 "y.tab.c"
    break;

  case 80: /* R_s.terminalnode: tk_ident R_d.attribute.0  */
#line 766 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[-1]._lexval))->_string;
            p_slist l;
            p_node mynode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval1);
            if(!mynode)
             adms_veriloga_message_fatal("terminal not found\n",(yyvsp[-1]._lexval));
            if(mynode->_location!=admse_external)
             adms_veriloga_message_fatal("node not a terminal\n",(yyvsp[-1]._lexval));
            adms_slist_push(&gTerminalList,(p_adms)mynode);
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&mynode->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&mynode->_attribute,l->data);
          }
#line 2703 "y.tab.c"
    break;

  case 81: /* R_s.node: tk_ident R_d.attribute.0  */
#line 784 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[-1]._lexval))->_string;
            p_slist l;
            p_node mynode=adms_module_list_node_prepend_by_id_once_or_ignore(gModule,gModule,mylexval1);
            adms_slist_push(&gNodeList,(p_adms)mynode);
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&mynode->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&mynode->_attribute,l->data);
          }
#line 2719 "y.tab.c"
    break;

  case 82: /* R_d.branch: tk_branch R_s.branch ';'  */
#line 798 "verilogaYacc.y"
          {
          }
#line 2726 "y.tab.c"
    break;

  case 83: /* R_l.branchalias: R_s.branchalias  */
#line 803 "verilogaYacc.y"
          {
          }
#line 2733 "y.tab.c"
    break;

  case 84: /* R_l.branchalias: R_l.branchalias ',' R_s.branchalias  */
#line 806 "verilogaYacc.y"
          {
          }
#line 2740 "y.tab.c"
    break;

  case 85: /* R_s.branchalias: tk_ident  */
#line 811 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[0]._lexval))->_string;
            adms_slist_push(&gBranchAliasList,(p_adms)mylexval1);
          }
#line 2749 "y.tab.c"
    break;

  case 86: /* R_s.branch: '(' tk_ident ',' tk_ident ')' R_l.branchalias  */
#line 818 "verilogaYacc.y"
          {
            char* mylexval2=((p_lexval)(yyvsp[-4]._lexval))->_string;
            char* mylexval4=((p_lexval)(yyvsp[-2]._lexval))->_string;
            p_slist l;
            p_branch mybranch; 
            p_node pnode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval2);
            p_node nnode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval4);
            mybranch=adms_module_list_branch_prepend_by_id_once_or_ignore(gModule,gModule,pnode,nnode); 
            if(!pnode)
             adms_veriloga_message_fatal("node never declared\n",(yyvsp[-4]._lexval));
            if(!nnode)
             adms_veriloga_message_fatal("node never declared\n",(yyvsp[-2]._lexval));
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
#line 2778 "y.tab.c"
    break;

  case 87: /* R_s.branch: '(' tk_ident ')' R_l.branchalias  */
#line 843 "verilogaYacc.y"
          {
            char* mylexval2=((p_lexval)(yyvsp[-2]._lexval))->_string;
            p_slist l;
            p_branch mybranch;
            p_node pnode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval2);
            if(!pnode)
             adms_veriloga_message_fatal("node never declared\n",(yyvsp[-2]._lexval));
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
#line 2803 "y.tab.c"
    break;

  case 88: /* R_d.analogfunction: R_d.analogfunction.proto R_l.analogfunction.declaration R_analogcode.block tk_endfunction  */
#line 866 "verilogaYacc.y"
          {
            adms_slist_pull(&gBlockList);
            gAnalogfunction->_tree=YY((yyvsp[-1]._yaccval));
            gAnalogfunction=NULL;
          }
#line 2813 "y.tab.c"
    break;

  case 89: /* R_d.analogfunction.proto: tk_analog tk_function R_d.analogfunction.name ';'  */
#line 874 "verilogaYacc.y"
          {
            NEWVARIABLE(gAnalogfunction->_lexval)
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_output=admse_yes;
          }
#line 2823 "y.tab.c"
    break;

  case 90: /* R_d.analogfunction.proto: tk_analog tk_function tk_integer R_d.analogfunction.name ';'  */
#line 880 "verilogaYacc.y"
          {
            NEWVARIABLE(gAnalogfunction->_lexval)
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_output=admse_yes;
            myvariableprototype->_type=admse_integer;
            gAnalogfunction->_type=admse_integer; 
          }
#line 2835 "y.tab.c"
    break;

  case 91: /* R_d.analogfunction.proto: tk_analog tk_function tk_real R_d.analogfunction.name ';'  */
#line 888 "verilogaYacc.y"
          {
            NEWVARIABLE(gAnalogfunction->_lexval)
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_output=admse_yes;
          }
#line 2845 "y.tab.c"
    break;

  case 92: /* R_d.analogfunction.name: tk_ident  */
#line 896 "verilogaYacc.y"
          {
            p_slist l;
            gAnalogfunction=adms_analogfunction_new(gModule,(yyvsp[0]._lexval));
            adms_slist_push(&gBlockList,(p_adms)gAnalogfunction);
            adms_module_list_analogfunction_prepend_once_or_abort(gModule,gAnalogfunction); 
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&gAnalogfunction->_attribute,l->data);
          }
#line 2858 "y.tab.c"
    break;

  case 93: /* R_l.analogfunction.declaration: R_s.analogfunction.declaration  */
#line 907 "verilogaYacc.y"
          {
          }
#line 2865 "y.tab.c"
    break;

  case 94: /* R_l.analogfunction.declaration: R_l.analogfunction.declaration R_s.analogfunction.declaration  */
#line 910 "verilogaYacc.y"
          {
          }
#line 2872 "y.tab.c"
    break;

  case 95: /* R_s.analogfunction.declaration: tk_input R_l.analogfunction.input.variable ';'  */
#line 915 "verilogaYacc.y"
          {
          }
#line 2879 "y.tab.c"
    break;

  case 96: /* R_s.analogfunction.declaration: tk_output R_l.analogfunction.output.variable ';'  */
#line 918 "verilogaYacc.y"
          {
          }
#line 2886 "y.tab.c"
    break;

  case 97: /* R_s.analogfunction.declaration: tk_inout R_l.analogfunction.inout.variable ';'  */
#line 921 "verilogaYacc.y"
          {
          }
#line 2893 "y.tab.c"
    break;

  case 98: /* R_s.analogfunction.declaration: tk_integer R_l.analogfunction.integer.variable ';'  */
#line 924 "verilogaYacc.y"
          {
          }
#line 2900 "y.tab.c"
    break;

  case 99: /* R_s.analogfunction.declaration: tk_real R_l.analogfunction.real.variable ';'  */
#line 927 "verilogaYacc.y"
          {
          }
#line 2907 "y.tab.c"
    break;

  case 100: /* R_l.analogfunction.input.variable: tk_ident  */
#line 932 "verilogaYacc.y"
          {
            NEWVARIABLE((yyvsp[0]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_input=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
#line 2918 "y.tab.c"
    break;

  case 101: /* R_l.analogfunction.input.variable: R_l.analogfunction.input.variable ',' tk_ident  */
#line 939 "verilogaYacc.y"
          {
            NEWVARIABLE((yyvsp[0]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_input=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
#line 2929 "y.tab.c"
    break;

  case 102: /* R_l.analogfunction.output.variable: tk_ident  */
#line 948 "verilogaYacc.y"
          {
            NEWVARIABLE((yyvsp[0]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_output=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
#line 2940 "y.tab.c"
    break;

  case 103: /* R_l.analogfunction.output.variable: R_l.analogfunction.output.variable ',' tk_ident  */
#line 955 "verilogaYacc.y"
          {
            NEWVARIABLE((yyvsp[0]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_output=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
#line 2951 "y.tab.c"
    break;

  case 104: /* R_l.analogfunction.inout.variable: tk_ident  */
#line 964 "verilogaYacc.y"
          {
            NEWVARIABLE((yyvsp[0]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_input=admse_yes;
            myvariableprototype->_output=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
#line 2963 "y.tab.c"
    break;

  case 105: /* R_l.analogfunction.inout.variable: R_l.analogfunction.inout.variable ',' tk_ident  */
#line 972 "verilogaYacc.y"
          {
            NEWVARIABLE((yyvsp[0]._lexval))
            adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
            myvariableprototype->_input=admse_yes;
            myvariableprototype->_output=admse_yes;
            myvariableprototype->_parametertype=admse_analogfunction;
          }
#line 2975 "y.tab.c"
    break;

  case 106: /* R_l.analogfunction.integer.variable: tk_ident  */
#line 982 "verilogaYacc.y"
          {
            p_variableprototype myvariableprototype=adms_analogfunction_list_variable_lookup_by_id(gAnalogfunction,gModule,(yyvsp[0]._lexval),(p_adms)gAnalogfunction);
            if(myvariableprototype)
              myvariableprototype->_type=admse_integer;
            else
            {
              NEWVARIABLE((yyvsp[0]._lexval))
              adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
              myvariableprototype->_type=admse_integer;
            }
          }
#line 2991 "y.tab.c"
    break;

  case 107: /* R_l.analogfunction.integer.variable: R_l.analogfunction.integer.variable ',' tk_ident  */
#line 994 "verilogaYacc.y"
          {
            p_variableprototype myvariableprototype=adms_analogfunction_list_variable_lookup_by_id(gAnalogfunction,gModule,(yyvsp[0]._lexval),(p_adms)gAnalogfunction);
            if(myvariableprototype)
              myvariableprototype->_type=admse_integer;
            else
            {
              NEWVARIABLE((yyvsp[0]._lexval))
              adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
              myvariableprototype->_type=admse_integer;
            }
          }
#line 3007 "y.tab.c"
    break;

  case 108: /* R_l.analogfunction.real.variable: tk_ident  */
#line 1008 "verilogaYacc.y"
          {
            p_variableprototype myvariableprototype=adms_analogfunction_list_variable_lookup_by_id(gAnalogfunction,gModule,(yyvsp[0]._lexval),(p_adms)gAnalogfunction);
            if(myvariableprototype)
              myvariableprototype->_type=admse_real;
            else
            {
              NEWVARIABLE((yyvsp[0]._lexval))
              adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
              myvariableprototype->_type=admse_real;
            }
          }
#line 3023 "y.tab.c"
    break;

  case 109: /* R_l.analogfunction.real.variable: R_l.analogfunction.real.variable ',' tk_ident  */
#line 1020 "verilogaYacc.y"
          {
            p_variableprototype myvariableprototype=adms_analogfunction_list_variable_lookup_by_id(gAnalogfunction,gModule,(yyvsp[0]._lexval),(p_adms)gAnalogfunction);
            if(myvariableprototype)
              myvariableprototype->_type=admse_real;
            else
            {
              NEWVARIABLE((yyvsp[0]._lexval))
              adms_analogfunction_list_variable_prepend_once_or_abort(gAnalogfunction,myvariableprototype); 
              myvariableprototype->_type=admse_real;
            }
          }
#line 3039 "y.tab.c"
    break;

  case 110: /* R_variable.type.set: tk_integer  */
#line 1034 "verilogaYacc.y"
          {
            gVariableType=admse_integer;
          }
#line 3047 "y.tab.c"
    break;

  case 111: /* R_variable.type.set: tk_real  */
#line 1038 "verilogaYacc.y"
          {
            gVariableType=admse_real;
          }
#line 3055 "y.tab.c"
    break;

  case 112: /* R_variable.type.set: tk_string  */
#line 1042 "verilogaYacc.y"
          {
            gVariableType=admse_string;
          }
#line 3063 "y.tab.c"
    break;

  case 113: /* $@9: %empty  */
#line 1048 "verilogaYacc.y"
          {
            set_context(ctx_any);
          }
#line 3071 "y.tab.c"
    break;

  case 114: /* R_variable.type: R_variable.type.set $@9 R_d.attribute.0  */
#line 1052 "verilogaYacc.y"
          {
            adms_slist_concat(&gGlobalAttributeList,gAttributeList);
            gAttributeList=NULL;
          }
#line 3080 "y.tab.c"
    break;

  case 115: /* R_d.variable.end: ';'  */
#line 1059 "verilogaYacc.y"
          {
            p_slist l;
            for(l=gVariableDeclarationList;l;l=l->next)
              ((p_variableprototype)l->data)->_type=gVariableType;
            adms_slist_free(gVariableDeclarationList); gVariableDeclarationList=NULL;
          }
#line 3091 "y.tab.c"
    break;

  case 116: /* R_l.parameter: R_s.parameter  */
#line 1068 "verilogaYacc.y"
          {
          }
#line 3098 "y.tab.c"
    break;

  case 117: /* R_l.parameter: R_l.parameter ',' R_s.parameter  */
#line 1071 "verilogaYacc.y"
          {
          }
#line 3105 "y.tab.c"
    break;

  case 118: /* R_l.variable: R_s.variable  */
#line 1076 "verilogaYacc.y"
          {
          }
#line 3112 "y.tab.c"
    break;

  case 119: /* R_l.variable: R_l.variable ',' R_s.variable  */
#line 1079 "verilogaYacc.y"
          {
          }
#line 3119 "y.tab.c"
    break;

  case 120: /* R_d.aliasparameter: R_d.aliasparameter.token tk_ident '=' tk_ident R_d.attribute.0 ';'  */
#line 1084 "verilogaYacc.y"
          {
            char* mylexval2=((p_lexval)(yyvsp[-4]._lexval))->_string;
            p_slist l;
            p_variableprototype myvariableprototype=adms_module_list_variable_lookup_by_id(gModule,gModule,(yyvsp[-2]._lexval),(p_adms)gModule);
            if(!myvariableprototype)
             adms_veriloga_message_fatal("variable never declared\n",(yyvsp[-2]._lexval));
            adms_variableprototype_list_alias_prepend_once_or_abort(myvariableprototype,adms_kclone(mylexval2));
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&myvariableprototype->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&myvariableprototype->_attribute,l->data);
          }
#line 3137 "y.tab.c"
    break;

  case 121: /* R_d.aliasparameter.token: tk_aliasparameter  */
#line 1100 "verilogaYacc.y"
          {
          }
#line 3144 "y.tab.c"
    break;

  case 122: /* R_d.aliasparameter.token: tk_aliasparam  */
#line 1103 "verilogaYacc.y"
          {
          }
#line 3151 "y.tab.c"
    break;

  case 123: /* R_s.parameter: R_s.parameter.name R_d.attribute.0  */
#line 1108 "verilogaYacc.y"
          {
            p_slist l;
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&((p_variableprototype)gVariableDeclarationList->data)->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&((p_variableprototype)gVariableDeclarationList->data)->_attribute,l->data);
          }
#line 3164 "y.tab.c"
    break;

  case 124: /* R_s.variable: R_s.variable.name R_d.attribute.0  */
#line 1119 "verilogaYacc.y"
          {
            p_slist l;
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&((p_variableprototype)gVariableDeclarationList->data)->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            for(l=gGlobalAttributeList;l;l=l->next)
              adms_slist_push(&((p_variableprototype)gVariableDeclarationList->data)->_attribute,l->data);
          }
#line 3177 "y.tab.c"
    break;

  case 125: /* R_s.parameter.name: R_s.variable.name '=' R_s.expression R_s.parameter.range  */
#line 1130 "verilogaYacc.y"
          {
            ((p_variableprototype)gVariableDeclarationList->data)->_input=admse_yes;
            ((p_variableprototype)gVariableDeclarationList->data)->_default=((p_expression)YY((yyvsp[-1]._yaccval)));
            ((p_variableprototype)gVariableDeclarationList->data)->_range=adms_slist_reverse(gRangeList);
            gRangeList=NULL;
          }
#line 3188 "y.tab.c"
    break;

  case 126: /* R_s.parameter.name: R_s.variable.name '=' '{' R_l.expression '}' R_s.parameter.range  */
#line 1137 "verilogaYacc.y"
          {
            p_slist myArgs=(p_slist)YY((yyvsp[-2]._yaccval));
            adms_slist_inreverse(&myArgs);
            ((p_variableprototype)gVariableDeclarationList->data)->_input=admse_yes;
            ((p_variableprototype)gVariableDeclarationList->data)->_default=((p_expression)myArgs->data);
            ((p_variableprototype)gVariableDeclarationList->data)->_arraydefault=myArgs;
            ((p_variableprototype)gVariableDeclarationList->data)->_range=adms_slist_reverse(gRangeList);
            gRangeList=NULL;
          }
#line 3202 "y.tab.c"
    break;

  case 127: /* R_s.variable.name: tk_ident  */
#line 1149 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[0]._lexval))->_string;
            NEWVARIABLE((yyvsp[0]._lexval))
            if(adms_module_list_node_lookup_by_id(gModule,gModule,mylexval1))
             adms_veriloga_message_fatal("variable already defined as node\n",(yyvsp[0]._lexval));
            adms_module_list_variable_prepend_once_or_abort(gModule,myvariableprototype); 
            adms_slist_push(&gVariableDeclarationList,(p_adms)myvariableprototype);
          }
#line 3215 "y.tab.c"
    break;

  case 128: /* R_s.variable.name: tk_ident '[' tk_integer ':' tk_integer ']'  */
#line 1158 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[-5]._lexval))->_string;
            NEWVARIABLE((yyvsp[-5]._lexval))
            if(adms_module_list_node_lookup_by_id(gModule,gModule,mylexval1))
             adms_veriloga_message_fatal("variable already defined as node\n",(yyvsp[-5]._lexval));
            adms_module_list_variable_prepend_once_or_abort(gModule,myvariableprototype); 
            adms_slist_push(&gVariableDeclarationList,(p_adms)myvariableprototype);
            myvariableprototype->_sizetype=admse_array;
            myvariableprototype->_minsize=adms_number_new((yyvsp[-3]._lexval));
            myvariableprototype->_maxsize=adms_number_new((yyvsp[-1]._lexval));
          }
#line 3231 "y.tab.c"
    break;

  case 129: /* R_s.parameter.range: %empty  */
#line 1172 "verilogaYacc.y"
          {
          }
#line 3238 "y.tab.c"
    break;

  case 130: /* R_s.parameter.range: R_l.interval  */
#line 1175 "verilogaYacc.y"
          {
          }
#line 3245 "y.tab.c"
    break;

  case 131: /* R_l.interval: R_s.interval  */
#line 1180 "verilogaYacc.y"
          {
          }
#line 3252 "y.tab.c"
    break;

  case 132: /* R_l.interval: R_l.interval R_s.interval  */
#line 1183 "verilogaYacc.y"
          {
          }
#line 3259 "y.tab.c"
    break;

  case 133: /* R_s.interval: tk_from R_d.interval  */
#line 1188 "verilogaYacc.y"
          {
            if(((p_range)YY((yyvsp[0]._yaccval)))->_infboundtype==admse_range_bound_value)
              ((p_range)YY((yyvsp[0]._yaccval)))->_type=admse_include_value;
            else
              ((p_range)YY((yyvsp[0]._yaccval)))->_type=admse_include;
            adms_slist_push(&gRangeList,YY((yyvsp[0]._yaccval)));
          }
#line 3271 "y.tab.c"
    break;

  case 134: /* R_s.interval: tk_exclude R_d.interval  */
#line 1196 "verilogaYacc.y"
          {
            if(((p_range)YY((yyvsp[0]._yaccval)))->_infboundtype==admse_range_bound_value)
              ((p_range)YY((yyvsp[0]._yaccval)))->_type=admse_exclude_value;
            else
              ((p_range)YY((yyvsp[0]._yaccval)))->_type=admse_exclude;
            adms_slist_push(&gRangeList,YY((yyvsp[0]._yaccval)));
          }
#line 3283 "y.tab.c"
    break;

  case 135: /* R_d.interval: '(' R_interval.inf ':' R_interval.sup ')'  */
#line 1206 "verilogaYacc.y"
          {
            p_range myrange=adms_module_list_range_prepend_by_id_once_or_abort(gModule,gModule,(p_expression)YY((yyvsp[-3]._yaccval)),(p_expression)YY((yyvsp[-1]._yaccval))); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myrange->_infboundtype=admse_range_bound_exclude;
            myrange->_supboundtype=admse_range_bound_exclude;
            Y((yyval._yaccval),(p_adms)myrange);
          }
#line 3295 "y.tab.c"
    break;

  case 136: /* R_d.interval: '(' R_interval.inf ':' R_interval.sup ']'  */
#line 1214 "verilogaYacc.y"
          {
            p_range myrange=adms_module_list_range_prepend_by_id_once_or_abort(gModule,gModule,(p_expression)YY((yyvsp[-3]._yaccval)),(p_expression)YY((yyvsp[-1]._yaccval))); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myrange->_infboundtype=admse_range_bound_exclude;
            myrange->_supboundtype=admse_range_bound_include;
            Y((yyval._yaccval),(p_adms)myrange);
          }
#line 3307 "y.tab.c"
    break;

  case 137: /* R_d.interval: '[' R_interval.inf ':' R_interval.sup ')'  */
#line 1222 "verilogaYacc.y"
          {
            p_range myrange=adms_module_list_range_prepend_by_id_once_or_abort(gModule,gModule,(p_expression)YY((yyvsp[-3]._yaccval)),(p_expression)YY((yyvsp[-1]._yaccval))); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myrange->_infboundtype=admse_range_bound_include;
            myrange->_supboundtype=admse_range_bound_exclude;
            Y((yyval._yaccval),(p_adms)myrange);
          }
#line 3319 "y.tab.c"
    break;

  case 138: /* R_d.interval: '[' R_interval.inf ':' R_interval.sup ']'  */
#line 1230 "verilogaYacc.y"
          {
            p_range myrange=adms_module_list_range_prepend_by_id_once_or_abort(gModule,gModule,(p_expression)YY((yyvsp[-3]._yaccval)),(p_expression)YY((yyvsp[-1]._yaccval))); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myrange->_infboundtype=admse_range_bound_include;
            myrange->_supboundtype=admse_range_bound_include;
            Y((yyval._yaccval),(p_adms)myrange);
          }
#line 3331 "y.tab.c"
    break;

  case 139: /* R_d.interval: R_s.expression  */
#line 1238 "verilogaYacc.y"
          {
            p_range myrange=adms_module_list_range_prepend_by_id_once_or_abort(gModule,gModule,(p_expression)YY((yyvsp[0]._yaccval)),(p_expression)YY((yyvsp[0]._yaccval))); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myrange->_infboundtype=admse_range_bound_value;
            myrange->_supboundtype=admse_range_bound_value;
            Y((yyval._yaccval),(p_adms)myrange);
          }
#line 3343 "y.tab.c"
    break;

  case 140: /* R_interval.inf: R_s.expression  */
#line 1248 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 3351 "y.tab.c"
    break;

  case 141: /* R_interval.inf: '-' tk_inf  */
#line 1252 "verilogaYacc.y"
          {
            p_number mynumber=adms_number_new((yyvsp[0]._lexval)); 
            p_expression myexpression=adms_expression_new(gModule,(p_adms)mynumber); 
            mynumber->_lexval->_string=adms_kclone("-inf");
            adms_slist_push(&gModule->_expression,(p_adms)myexpression); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myexpression->_infinity=admse_minus;
            myexpression->_hasspecialnumber=adms_kclone("YES");
            Y((yyval._yaccval),(p_adms)myexpression);
          }
#line 3366 "y.tab.c"
    break;

  case 142: /* R_interval.sup: R_s.expression  */
#line 1265 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 3374 "y.tab.c"
    break;

  case 143: /* R_interval.sup: tk_inf  */
#line 1269 "verilogaYacc.y"
          {
            p_number mynumber=adms_number_new((yyvsp[0]._lexval)); 
            p_expression myexpression=adms_expression_new(gModule,(p_adms)mynumber); 
            mynumber->_lexval->_string=adms_kclone("+inf");
            adms_slist_push(&gModule->_expression,(p_adms)myexpression); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myexpression->_infinity=admse_plus;
            myexpression->_hasspecialnumber=adms_kclone("YES");
            Y((yyval._yaccval),(p_adms)myexpression);
          }
#line 3389 "y.tab.c"
    break;

  case 144: /* R_interval.sup: '+' tk_inf  */
#line 1280 "verilogaYacc.y"
          {
            p_number mynumber=adms_number_new((yyvsp[0]._lexval)); 
            p_expression myexpression=adms_expression_new(gModule,(p_adms)mynumber); 
            mynumber->_lexval->_string=adms_kclone("+inf");
            adms_slist_push(&gModule->_expression,(p_adms)myexpression); 
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myexpression->_infinity=admse_plus;
            myexpression->_hasspecialnumber=adms_kclone("YES");
            Y((yyval._yaccval),(p_adms)myexpression);
          }
#line 3404 "y.tab.c"
    break;

  case 145: /* $@10: %empty  */
#line 1293 "verilogaYacc.y"
          {
            set_context(ctx_any); // from here, don't recognize node declarations.
                                  // they are not permitted anyway.
          }
#line 3413 "y.tab.c"
    break;

  case 146: /* R_analog: tk_analog $@10 R_analogcode  */
#line 1298 "verilogaYacc.y"
          {
            gModule->_analog=adms_analog_new(YY((yyvsp[0]._yaccval)));
          }
#line 3421 "y.tab.c"
    break;

  case 147: /* R_analogcode: R_analogcode.atomic  */
#line 1304 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 3429 "y.tab.c"
    break;

  case 148: /* R_analogcode: R_analogcode.block  */
#line 1308 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 3437 "y.tab.c"
    break;

  case 149: /* R_l.expression: R_s.expression  */
#line 1314 "verilogaYacc.y"
          {
            p_slist myArgs=NULL;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_push(&myArgs,YY((yyvsp[0]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
#line 3448 "y.tab.c"
    break;

  case 150: /* R_l.expression: R_l.expression ',' R_s.expression  */
#line 1321 "verilogaYacc.y"
          {
            p_slist myArgs=(p_slist)YY((yyvsp[-2]._yaccval));
            (yyval._yaccval)=(yyvsp[-2]._yaccval);
            adms_slist_push(&myArgs,YY((yyvsp[0]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
#line 3459 "y.tab.c"
    break;

  case 151: /* R_analogcode.atomic: R_d.attribute.0 R_d.blockvariable  */
#line 1330 "verilogaYacc.y"
          {
            p_slist l;
            p_slist lv;
            for(l=gAttributeList;l;l=l->next)
              for(lv=((p_blockvariable)YY((yyvsp[0]._yaccval)))->_variable;lv;lv=lv->next)
                adms_slist_push(&((p_variableprototype)lv->data)->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 3473 "y.tab.c"
    break;

  case 152: /* R_analogcode.atomic: R_d.contribution  */
#line 1340 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 3481 "y.tab.c"
    break;

  case 153: /* R_analogcode.atomic: R_s.assignment ';'  */
#line 1344 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[-1]._yaccval);
          }
#line 3489 "y.tab.c"
    break;

  case 154: /* R_analogcode.atomic: R_d.conditional  */
#line 1348 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 3497 "y.tab.c"
    break;

  case 155: /* R_analogcode.atomic: R_d.while  */
#line 1352 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 3505 "y.tab.c"
    break;

  case 156: /* R_analogcode.atomic: R_d.case  */
#line 1356 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 3513 "y.tab.c"
    break;

  case 157: /* R_analogcode.atomic: R_d.for  */
#line 1360 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 3521 "y.tab.c"
    break;

  case 158: /* R_analogcode.atomic: tk_dollar_ident '(' R_l.expression ')' ';'  */
#line 1364 "verilogaYacc.y"
          {
            p_function myfunction=adms_function_new((yyvsp[-4]._lexval),uid++);
            p_slist myArgs=(p_slist)YY((yyvsp[-2]._yaccval));
            p_callfunction mycallfunction=adms_callfunction_new(gModule,myfunction);
            adms_slist_push(&gModule->_callfunction,(p_adms)mycallfunction);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_inreverse(&myArgs);
            myfunction->_arguments=myArgs;
            Y((yyval._yaccval),(p_adms)mycallfunction);
          }
#line 3536 "y.tab.c"
    break;

  case 159: /* R_analogcode.atomic: tk_dollar_ident '(' ')' ';'  */
#line 1375 "verilogaYacc.y"
          {
            p_function myfunction=adms_function_new((yyvsp[-3]._lexval),uid++);
            p_callfunction mycallfunction=adms_callfunction_new(gModule,myfunction);
            adms_slist_push(&gModule->_callfunction,(p_adms)mycallfunction);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mycallfunction);
          }
#line 3548 "y.tab.c"
    break;

  case 160: /* R_analogcode.atomic: tk_dollar_ident ';'  */
#line 1383 "verilogaYacc.y"
          {
            p_function myfunction=adms_function_new((yyvsp[-1]._lexval),uid++);
            p_callfunction mycallfunction=adms_callfunction_new(gModule,myfunction);
            adms_slist_push(&gModule->_callfunction,(p_adms)mycallfunction);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mycallfunction);
          }
#line 3560 "y.tab.c"
    break;

  case 161: /* R_analogcode.atomic: ';'  */
#line 1391 "verilogaYacc.y"
          {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)adms_nilled_new(gModule));
          }
#line 3569 "y.tab.c"
    break;

  case 162: /* R_analogcode.block: R_d.block  */
#line 1398 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 3577 "y.tab.c"
    break;

  case 163: /* R_analogcode.block: R_analogcode.block.atevent R_d.block  */
#line 1402 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
            adms_lexval_free(((p_block)YY((yyval._yaccval)))->_lexval);
            ((p_block)YY((yyval._yaccval)))->_lexval=(p_lexval)YY((yyvsp[-1]._yaccval));
          }
#line 3587 "y.tab.c"
    break;

  case 164: /* R_analogcode.block.atevent: '@' '(' tk_ident '(' R_l.analysis ')' ')'  */
#line 1410 "verilogaYacc.y"
          {
            adms_veriloga_message_fatal("@ control not supported\n",(yyvsp[-4]._lexval));
          }
#line 3595 "y.tab.c"
    break;

  case 165: /* R_analogcode.block.atevent: '@' tk_ident  */
#line 1414 "verilogaYacc.y"
          {
            char* mylexval2=((p_lexval)(yyvsp[0]._lexval))->_string;
            char* mypartitionning=adms_kclone(mylexval2);
            if(strcmp(mypartitionning,"initial_model")
              && strcmp(mypartitionning,"initial_instance")
              && strcmp(mypartitionning,"noise")
              && strcmp(mypartitionning,"initial_step")
              && strcmp(mypartitionning,"final_step"))
              adms_veriloga_message_fatal(" @() control not supported\n",(yyvsp[0]._lexval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)(yyvsp[0]._lexval));
          }
#line 3612 "y.tab.c"
    break;

  case 166: /* R_analogcode.block.atevent: '@' '(' tk_ident ')'  */
#line 1427 "verilogaYacc.y"
          {
            char* mylexval3=((p_lexval)(yyvsp[-1]._lexval))->_string;
            char* mypartitionning=adms_kclone(mylexval3);
            if(strcmp(mypartitionning,"initial_model")
              && strcmp(mypartitionning,"initial_instance")
              && strcmp(mypartitionning,"noise")
              && strcmp(mypartitionning,"initial_step")
              && strcmp(mypartitionning,"final_step"))
              adms_veriloga_message_fatal(" @() control not supported\n",(yyvsp[-1]._lexval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)(yyvsp[-1]._lexval));
          }
#line 3629 "y.tab.c"
    break;

  case 167: /* R_l.analysis: R_s.analysis  */
#line 1442 "verilogaYacc.y"
          {
          }
#line 3636 "y.tab.c"
    break;

  case 168: /* R_l.analysis: R_l.analysis ',' R_s.analysis  */
#line 1445 "verilogaYacc.y"
          {
          }
#line 3643 "y.tab.c"
    break;

  case 169: /* R_s.analysis: tk_anystring  */
#line 1450 "verilogaYacc.y"
          {
          }
#line 3650 "y.tab.c"
    break;

  case 170: /* R_d.block: R_d.block.begin tk_end  */
#line 1455 "verilogaYacc.y"
          {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),gBlockList->data);
            adms_slist_pull(&gBlockList);
          }
#line 3660 "y.tab.c"
    break;

  case 171: /* R_d.block: R_d.block.begin ':' tk_ident tk_end  */
#line 1461 "verilogaYacc.y"
          {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),gBlockList->data);
            adms_slist_pull(&gBlockList);
            ((p_block)YY((yyval._yaccval)))->_lexval->_string=(yyvsp[-1]._lexval)->_string;
          }
#line 3671 "y.tab.c"
    break;

  case 172: /* R_d.block: R_d.block.begin R_l.blockitem tk_end  */
#line 1468 "verilogaYacc.y"
          {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),gBlockList->data);
            adms_slist_pull(&gBlockList);
          }
#line 3681 "y.tab.c"
    break;

  case 173: /* R_d.block: R_d.block.begin ':' tk_ident R_l.blockitem tk_end  */
#line 1474 "verilogaYacc.y"
          {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),gBlockList->data);
            adms_slist_pull(&gBlockList);
            ((p_block)YY((yyval._yaccval)))->_lexval->_string=(yyvsp[-2]._lexval)->_string;
          }
#line 3692 "y.tab.c"
    break;

  case 174: /* R_d.block.begin: R_d.attribute.0 tk_begin  */
#line 1483 "verilogaYacc.y"
          {
            p_slist l;
            p_block myblock=adms_block_new(gModule,(yyvsp[0]._lexval),gBlockList?((p_block)gBlockList->data):NULL,NULL);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            myblock->_lexval->_string=adms_kclone("");
            adms_slist_push(&gBlockList,(p_adms)myblock);
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&myblock->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            adms_slist_push(&gModule->_block,gBlockList->data);
          }
#line 3708 "y.tab.c"
    break;

  case 175: /* R_l.blockitem: R_analogcode  */
#line 1497 "verilogaYacc.y"
          {
            adms_slist_push(&((p_block)gBlockList->data)->_item,YY((yyvsp[0]._yaccval)));
          }
#line 3716 "y.tab.c"
    break;

  case 176: /* R_l.blockitem: R_l.blockitem R_analogcode  */
#line 1501 "verilogaYacc.y"
          {
            adms_slist_push(&((p_block)gBlockList->data)->_item,YY((yyvsp[0]._yaccval)));
          }
#line 3724 "y.tab.c"
    break;

  case 177: /* R_d.blockvariable: tk_integer R_l.blockvariable ';'  */
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
#line 3741 "y.tab.c"
    break;

  case 178: /* R_d.blockvariable: tk_real R_l.blockvariable ';'  */
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
#line 3758 "y.tab.c"
    break;

  case 179: /* R_d.blockvariable: tk_string R_l.blockvariable ';'  */
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
#line 3775 "y.tab.c"
    break;

  case 180: /* R_l.blockvariable: R_s.blockvariable  */
#line 1548 "verilogaYacc.y"
          {
          }
#line 3782 "y.tab.c"
    break;

  case 181: /* R_l.blockvariable: R_l.blockvariable ',' R_s.blockvariable  */
#line 1551 "verilogaYacc.y"
          {
          }
#line 3789 "y.tab.c"
    break;

  case 182: /* R_s.blockvariable: tk_ident  */
#line 1556 "verilogaYacc.y"
          {
            NEWVARIABLE((yyvsp[0]._lexval))
            adms_block_list_variable_prepend_once_or_abort(((p_block)gBlockList->data),myvariableprototype); 
            adms_slist_push(&gBlockVariableList,(p_adms)myvariableprototype);
          }
#line 3799 "y.tab.c"
    break;

  case 183: /* R_s.blockvariable: tk_ident '[' tk_integer ':' tk_integer ']'  */
#line 1562 "verilogaYacc.y"
          {
            NEWVARIABLE((yyvsp[-5]._lexval))
            adms_block_list_variable_prepend_once_or_abort(((p_block)gBlockList->data),myvariableprototype); 
            adms_slist_push(&gVariableDeclarationList,(p_adms)myvariableprototype);
            myvariableprototype->_sizetype=admse_array;
            myvariableprototype->_minsize=adms_number_new((yyvsp[-3]._lexval));
            myvariableprototype->_maxsize=adms_number_new((yyvsp[-1]._lexval));
          }
#line 3812 "y.tab.c"
    break;

  case 184: /* R_d.contribution: R_contribution R_d.attribute.0 ';'  */
#line 1573 "verilogaYacc.y"
          {
            p_slist l;
            for(l=gAttributeList;l;l=l->next)
              adms_slist_push(&gContribution->_attribute,l->data);
            adms_slist_free(gAttributeList); gAttributeList=NULL;
            gContribution=NULL;
          }
#line 3824 "y.tab.c"
    break;

  case 185: /* R_contribution: R_source '<' '+' R_s.expression  */
#line 1583 "verilogaYacc.y"
          {
            p_source mysource=(p_source)YY((yyvsp[-3]._yaccval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            gContribution=adms_contribution_new(gModule,mysource,(p_expression)YY((yyvsp[0]._yaccval)),gLexval);
            adms_slist_push(&gModule->_contribution,(p_adms)gContribution);
            Y((yyval._yaccval),(p_adms)gContribution);
            gContribution->_branchalias=gBranchAlias;
            gBranchAlias=NULL;
          }
#line 3838 "y.tab.c"
    break;

  case 186: /* R_source: tk_ident '(' tk_ident ',' tk_ident ')'  */
#line 1595 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[-5]._lexval))->_string;
            char* mylexval3=((p_lexval)(yyvsp[-3]._lexval))->_string;
            char* mylexval5=((p_lexval)(yyvsp[-1]._lexval))->_string;
            p_node Pnode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval3);
            p_node Nnode=adms_module_list_node_lookup_by_id(gModule,gModule,mylexval5);
            char* natureID=mylexval1;
            p_nature mynature=adms_admsmain_list_nature_lookup_by_id(root(),natureID);
            p_branch mybranch=adms_module_list_branch_prepend_by_id_once_or_ignore(gModule,gModule,Pnode,Nnode);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            if(!mynature)
             adms_message_fatal(("[source:error] there is no nature with access %s, missing discipline.h file?\n",natureID))
            gSource=adms_module_list_source_prepend_by_id_once_or_ignore(gModule,gModule,mybranch,mynature);
            gLexval=(yyvsp[-5]._lexval);
            Y((yyval._yaccval),(p_adms)gSource);
          }
#line 3859 "y.tab.c"
    break;

  case 187: /* R_source: tk_ident '(' tk_ident ')'  */
#line 1612 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[-3]._lexval))->_string;
            char* mylexval3=((p_lexval)(yyvsp[-1]._lexval))->_string;
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
              adms_veriloga_message_fatal("undefined branch or node\n",(yyvsp[-3]._lexval));
            gSource=adms_module_list_source_prepend_by_id_once_or_ignore(gModule,gModule,mybranch,mynature);
            gLexval=(yyvsp[-3]._lexval);
            gBranchAlias=branchalias;
            Y((yyval._yaccval),(p_adms)gSource);
          }
#line 3886 "y.tab.c"
    break;

  case 188: /* R_d.while: tk_while '(' R_s.expression ')' R_analogcode  */
#line 1637 "verilogaYacc.y"
          {
            p_whileloop mywhileloop=adms_whileloop_new(gModule,(p_expression)YY((yyvsp[-2]._yaccval)),YY((yyvsp[0]._yaccval)));
            adms_slist_push(&gModule->_whileloop,(p_adms)mywhileloop);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mywhileloop);
          }
#line 3897 "y.tab.c"
    break;

  case 189: /* R_d.for: tk_for '(' R_s.assignment ';' R_s.expression ';' R_s.assignment ')' R_analogcode  */
#line 1646 "verilogaYacc.y"
          {
            p_forloop myforloop=adms_forloop_new(gModule,(p_assignment)YY((yyvsp[-6]._yaccval)),(p_expression)YY((yyvsp[-4]._yaccval)),(p_assignment)YY((yyvsp[-2]._yaccval)),YY((yyvsp[0]._yaccval)));
            adms_slist_push(&gModule->_forloop,(p_adms)myforloop);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myforloop);
          }
#line 3908 "y.tab.c"
    break;

  case 190: /* R_d.case: tk_case '(' R_s.expression ')' R_l.case.item tk_endcase  */
#line 1655 "verilogaYacc.y"
          {
            p_case mycase=adms_case_new(gModule,(p_expression)YY((yyvsp[-3]._yaccval)));
            adms_slist_push(&gModule->_case,(p_adms)mycase);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            mycase->_caseitem=adms_slist_reverse((p_slist)YY((yyvsp[-1]._yaccval)));
            Y((yyval._yaccval),(p_adms)mycase);
          }
#line 3920 "y.tab.c"
    break;

  case 191: /* R_l.case.item: R_s.case.item  */
#line 1665 "verilogaYacc.y"
          {
            p_slist myArgs=NULL;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_push(&myArgs,YY((yyvsp[0]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
#line 3931 "y.tab.c"
    break;

  case 192: /* R_l.case.item: R_l.case.item R_s.case.item  */
#line 1672 "verilogaYacc.y"
          {
            p_slist myArgs=(p_slist)YY((yyvsp[-1]._yaccval));
            (yyval._yaccval)=(yyvsp[-1]._yaccval);
            adms_slist_push(&myArgs,YY((yyvsp[0]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
#line 3942 "y.tab.c"
    break;

  case 193: /* R_s.case.item: R_l.enode ':' R_analogcode  */
#line 1681 "verilogaYacc.y"
          {
            p_slist myArgs=(p_slist)YY((yyvsp[-2]._yaccval));
            p_caseitem mycaseitem=adms_caseitem_new(YY((yyvsp[0]._yaccval)));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            mycaseitem->_condition=adms_slist_reverse(myArgs);
            Y((yyval._yaccval),(p_adms)mycaseitem);
          }
#line 3954 "y.tab.c"
    break;

  case 194: /* R_s.case.item: tk_default ':' R_analogcode  */
#line 1689 "verilogaYacc.y"
          {
            p_caseitem mycaseitem=adms_caseitem_new(YY((yyvsp[0]._yaccval)));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            mycaseitem->_defaultcase=admse_yes;
            Y((yyval._yaccval),(p_adms)mycaseitem);
          }
#line 3965 "y.tab.c"
    break;

  case 195: /* R_s.case.item: tk_default R_analogcode  */
#line 1696 "verilogaYacc.y"
          {
            p_caseitem mycaseitem=adms_caseitem_new(YY((yyvsp[0]._yaccval)));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            mycaseitem->_defaultcase=admse_yes;
            Y((yyval._yaccval),(p_adms)mycaseitem);
          }
#line 3976 "y.tab.c"
    break;

  case 196: /* R_s.paramlist.0: %empty  */
#line 1705 "verilogaYacc.y"
          {
          }
#line 3983 "y.tab.c"
    break;

  case 197: /* R_s.paramlist.0: '#' '(' R_l.instance.parameter ')'  */
#line 1708 "verilogaYacc.y"
          {
          }
#line 3990 "y.tab.c"
    break;

  case 198: /* R_s.instance: R_instance.module.name R_s.paramlist.0 tk_ident '(' R_l.node ')' ';'  */
#line 1713 "verilogaYacc.y"
          {
            char* mylexval3=((p_lexval)(yyvsp[-4]._lexval))->_string;
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
#line 4015 "y.tab.c"
    break;

  case 199: /* R_instance.module.name: tk_ident  */
#line 1736 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[0]._lexval))->_string;
            set_context(ctx_any); // from here, don't recognize node declarations.
                                  // they are not permitted anyway.
            gInstanceModule=adms_admsmain_list_module_lookup_by_id(root(),mylexval1);
            if(!gInstanceModule)
              adms_message_fatal(("module '%s' not found\n",mylexval1));
          }
#line 4028 "y.tab.c"
    break;

  case 200: /* R_l.instance.parameter: R_s.instance.parameter  */
#line 1747 "verilogaYacc.y"
          {
          }
#line 4035 "y.tab.c"
    break;

  case 201: /* R_l.instance.parameter: R_l.instance.parameter ',' R_s.instance.parameter  */
#line 1750 "verilogaYacc.y"
          {
          }
#line 4042 "y.tab.c"
    break;

  case 202: /* R_s.instance.parameter: '.' tk_ident '(' R_s.expression ')'  */
#line 1755 "verilogaYacc.y"
          {
            char* mylexval2=((p_lexval)(yyvsp[-3]._lexval))->_string;
            p_variableprototype myvariableprototype=adms_module_list_variable_lookup_by_id(gInstanceModule,gInstanceModule,(yyvsp[-3]._lexval),(p_adms)gInstanceModule);
            if(myvariableprototype)
            {
              p_instanceparameter myinstanceparameter;
              myinstanceparameter=adms_instanceparameter_new(myvariableprototype);
              adms_slist_push(&gInstanceVariableList,(p_adms)myinstanceparameter);
              myinstanceparameter->_value=((p_expression)YY((yyvsp[-1]._yaccval)));
            }
            else
            {
              adms_veriloga_message_fatal_continue((yyvsp[-3]._lexval));
              adms_message_fatal(("[%s.%s.%s]: undefined variable (instance declaration)",
                adms_module_uid(gModule),adms_module_uid(gInstanceModule),mylexval2))
            }
          }
#line 4064 "y.tab.c"
    break;

  case 203: /* R_s.assignment: tk_ident '=' R_s.expression  */
#line 1775 "verilogaYacc.y"
          {
            p_assignment myassignment;
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[-2]._lexval));
            p_variableprototype myvariableprototype;
            if(!myvariable)
              adms_veriloga_message_fatal("undefined variable\n",(yyvsp[-2]._lexval));
            myvariableprototype=myvariable->_prototype;
            myassignment=adms_assignment_new(gModule,(p_adms)myvariable,(p_expression)YY((yyvsp[0]._yaccval)),(yyvsp[-2]._lexval));
            adms_slist_push(&gModule->_assignment,(p_adms)myassignment);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myassignment);
            myvariableprototype->_vcount++;
            myvariableprototype->_vlast=myassignment;
          }
#line 4083 "y.tab.c"
    break;

  case 204: /* R_s.assignment: R_d.attribute tk_ident '=' R_s.expression  */
#line 1790 "verilogaYacc.y"
          {
            p_assignment myassignment;
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[-2]._lexval));
            p_variableprototype myvariableprototype;
            if(!myvariable)
              adms_veriloga_message_fatal("undefined variable\n",(yyvsp[-2]._lexval));
            myvariableprototype=myvariable->_prototype;
            myassignment=adms_assignment_new(gModule,(p_adms)myvariable,(p_expression)YY((yyvsp[0]._yaccval)),(yyvsp[-2]._lexval));
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
#line 4108 "y.tab.c"
    break;

  case 205: /* R_s.assignment: tk_ident '[' R_expression ']' '=' R_s.expression  */
#line 1811 "verilogaYacc.y"
          {
            p_assignment myassignment;
            p_array myarray;
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[-5]._lexval));
            p_variableprototype myvariableprototype;
            if(!myvariable)
              adms_veriloga_message_fatal("undefined variable\n",(yyvsp[-5]._lexval));
            myvariableprototype=myvariable->_prototype;
            myarray=adms_array_new(myvariable,YY((yyvsp[-3]._yaccval)));
            myassignment=adms_assignment_new(gModule,(p_adms)myarray,(p_expression)YY((yyvsp[0]._yaccval)),(yyvsp[-5]._lexval));
            adms_slist_push(&gModule->_assignment,(p_adms)myassignment);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myassignment);
            myvariableprototype->_vcount++;
            myvariableprototype->_vlast=myassignment;
          }
#line 4129 "y.tab.c"
    break;

  case 206: /* R_s.assignment: R_d.attribute tk_ident '[' R_expression ']' '=' R_s.expression  */
#line 1828 "verilogaYacc.y"
          {
            p_assignment myassignment;
            p_array myarray;
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[-5]._lexval));
            p_variableprototype myvariableprototype;
            if(!myvariable)
              adms_veriloga_message_fatal("undefined variable\n",(yyvsp[-5]._lexval));
            myvariableprototype=myvariable->_prototype;
            myarray=adms_array_new(myvariable,YY((yyvsp[-3]._yaccval)));
            myassignment=adms_assignment_new(gModule,(p_adms)myarray,(p_expression)YY((yyvsp[0]._yaccval)),(yyvsp[-5]._lexval));
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
#line 4156 "y.tab.c"
    break;

  case 207: /* R_d.conditional: tk_if '(' R_s.expression ')' R_analogcode  */
#line 1853 "verilogaYacc.y"
          {
            p_expression myexpression=(p_expression)YY((yyvsp[-2]._yaccval));
            p_adms mythen=YY((yyvsp[0]._yaccval));
            p_conditional myconditional=adms_conditional_new(gModule,myexpression,mythen,NULL);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myconditional);
          }
#line 4168 "y.tab.c"
    break;

  case 208: /* R_d.conditional: tk_if '(' R_s.expression ')' R_analogcode tk_else R_analogcode  */
#line 1861 "verilogaYacc.y"
          {
            p_expression myexpression=(p_expression)YY((yyvsp[-4]._yaccval));
            p_adms mythen=YY((yyvsp[-2]._yaccval));
            p_adms myelse=YY((yyvsp[0]._yaccval));
            p_conditional myconditional=adms_conditional_new(gModule,myexpression,mythen,myelse);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myconditional);
          }
#line 4181 "y.tab.c"
    break;

  case 209: /* R_s.expression: R_expression  */
#line 1872 "verilogaYacc.y"
          {
            p_expression myexpression=adms_expression_new(gModule,YY((yyvsp[0]._yaccval))); 
            adms_slist_push(&gModule->_expression,(p_adms)myexpression); 
            (yyval._yaccval)=(yyvsp[0]._yaccval);
            Y((yyval._yaccval),(p_adms)myexpression);
          }
#line 4192 "y.tab.c"
    break;

  case 210: /* R_l.enode: R_s.function_expression  */
#line 1881 "verilogaYacc.y"
          {
            p_slist myArgs=NULL;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_push(&myArgs,YY((yyvsp[0]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
#line 4203 "y.tab.c"
    break;

  case 211: /* R_l.enode: R_l.enode ',' R_s.function_expression  */
#line 1888 "verilogaYacc.y"
          {
            p_slist myArgs=(p_slist)YY((yyvsp[-2]._yaccval));
            (yyval._yaccval)=(yyvsp[-2]._yaccval);
            adms_slist_push(&myArgs,YY((yyvsp[0]._yaccval)));
            Y((yyval._yaccval),(p_adms)myArgs);
          }
#line 4214 "y.tab.c"
    break;

  case 212: /* R_s.function_expression: R_expression  */
#line 1897 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4222 "y.tab.c"
    break;

  case 213: /* R_expression: R_e.conditional  */
#line 1903 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4230 "y.tab.c"
    break;

  case 214: /* R_e.conditional: R_e.bitwise_equ  */
#line 1909 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4238 "y.tab.c"
    break;

  case 215: /* R_e.conditional: R_e.bitwise_equ '?' R_e.conditional ':' R_e.conditional  */
#line 1913 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-4]._yaccval));
            p_adms m2=YY((yyvsp[-2]._yaccval));
            p_adms m3=YY((yyvsp[0]._yaccval));
            p_mapply_ternary myop=adms_mapply_ternary_new(admse_conditional,m1,m2,m3);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4251 "y.tab.c"
    break;

  case 216: /* R_e.bitwise_equ: R_e.bitwise_xor  */
#line 1924 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4259 "y.tab.c"
    break;

  case 217: /* R_e.bitwise_equ: R_e.bitwise_equ tk_bitwise_equr R_e.bitwise_xor  */
#line 1928 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_bw_equr,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4271 "y.tab.c"
    break;

  case 218: /* R_e.bitwise_equ: R_e.bitwise_equ '~' '^' R_e.bitwise_xor  */
#line 1936 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-3]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_bw_equl,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4283 "y.tab.c"
    break;

  case 219: /* R_e.bitwise_xor: R_e.bitwise_or  */
#line 1946 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4291 "y.tab.c"
    break;

  case 220: /* R_e.bitwise_xor: R_e.bitwise_xor '^' R_e.bitwise_or  */
#line 1950 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_bw_xor,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4303 "y.tab.c"
    break;

  case 221: /* R_e.bitwise_or: R_e.bitwise_and  */
#line 1960 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4311 "y.tab.c"
    break;

  case 222: /* R_e.bitwise_or: R_e.bitwise_or '|' R_e.bitwise_and  */
#line 1964 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_bw_or,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4323 "y.tab.c"
    break;

  case 223: /* R_e.bitwise_and: R_e.logical_or  */
#line 1974 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4331 "y.tab.c"
    break;

  case 224: /* R_e.bitwise_and: R_e.bitwise_and '&' R_e.logical_or  */
#line 1978 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_bw_and,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4343 "y.tab.c"
    break;

  case 225: /* R_e.logical_or: R_e.logical_and  */
#line 1988 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4351 "y.tab.c"
    break;

  case 226: /* R_e.logical_or: R_e.logical_or tk_or R_e.logical_and  */
#line 1992 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_or,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4363 "y.tab.c"
    break;

  case 227: /* R_e.logical_and: R_e.comp_equ  */
#line 2002 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4371 "y.tab.c"
    break;

  case 228: /* R_e.logical_and: R_e.logical_and tk_and R_e.comp_equ  */
#line 2006 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_and,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4383 "y.tab.c"
    break;

  case 229: /* R_e.comp_equ: R_e.comp  */
#line 2016 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4391 "y.tab.c"
    break;

  case 230: /* R_e.comp_equ: R_e.comp_equ '=' '=' R_e.comp  */
#line 2020 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-3]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_equ,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4403 "y.tab.c"
    break;

  case 231: /* R_e.comp_equ: R_e.comp_equ '!' '=' R_e.comp  */
#line 2028 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-3]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_notequ,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4415 "y.tab.c"
    break;

  case 232: /* R_e.comp: R_e.bitwise_shift  */
#line 2038 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4423 "y.tab.c"
    break;

  case 233: /* R_e.comp: R_e.comp '<' R_e.bitwise_shift  */
#line 2042 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_lt,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4435 "y.tab.c"
    break;

  case 234: /* R_e.comp: R_e.comp '<' '=' R_e.bitwise_shift  */
#line 2050 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-3]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_lt_equ,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4447 "y.tab.c"
    break;

  case 235: /* R_e.comp: R_e.comp '>' R_e.bitwise_shift  */
#line 2058 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_gt,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4459 "y.tab.c"
    break;

  case 236: /* R_e.comp: R_e.comp '>' '=' R_e.bitwise_shift  */
#line 2066 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-3]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_gt_equ,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4471 "y.tab.c"
    break;

  case 237: /* R_e.bitwise_shift: R_e.arithm_add  */
#line 2076 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4479 "y.tab.c"
    break;

  case 238: /* R_e.bitwise_shift: R_e.bitwise_shift tk_op_shr R_e.arithm_add  */
#line 2080 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_shiftr,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4491 "y.tab.c"
    break;

  case 239: /* R_e.bitwise_shift: R_e.bitwise_shift tk_op_shl R_e.arithm_add  */
#line 2088 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_shiftl,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4503 "y.tab.c"
    break;

  case 240: /* R_e.arithm_add: R_e.arithm_mult  */
#line 2098 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4511 "y.tab.c"
    break;

  case 241: /* R_e.arithm_add: R_e.arithm_add '+' R_e.arithm_mult  */
#line 2102 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_addp,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4523 "y.tab.c"
    break;

  case 242: /* R_e.arithm_add: R_e.arithm_add '-' R_e.arithm_mult  */
#line 2110 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_addm,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4535 "y.tab.c"
    break;

  case 243: /* R_e.arithm_mult: R_e.unary  */
#line 2120 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4543 "y.tab.c"
    break;

  case 244: /* R_e.arithm_mult: R_e.arithm_mult '*' R_e.unary  */
#line 2124 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_multtime,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4555 "y.tab.c"
    break;

  case 245: /* R_e.arithm_mult: R_e.arithm_mult '/' R_e.unary  */
#line 2132 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_multdiv,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4567 "y.tab.c"
    break;

  case 246: /* R_e.arithm_mult: R_e.arithm_mult '%' R_e.unary  */
#line 2140 "verilogaYacc.y"
          {
            p_adms m1=YY((yyvsp[-2]._yaccval));
            p_adms m2=YY((yyvsp[0]._yaccval));
            p_mapply_binary myop=adms_mapply_binary_new(admse_multmod,m1,m2);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myop);
          }
#line 4579 "y.tab.c"
    break;

  case 247: /* R_e.unary: R_e.atomic  */
#line 2150 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[0]._yaccval);
          }
#line 4587 "y.tab.c"
    break;

  case 248: /* R_e.unary: '+' R_e.atomic  */
#line 2154 "verilogaYacc.y"
          {
            p_adms m=YY((yyvsp[0]._yaccval));
            p_mapply_unary mymathapply=adms_mapply_unary_new(admse_plus,m);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mymathapply);
          }
#line 4598 "y.tab.c"
    break;

  case 249: /* R_e.unary: '-' R_e.atomic  */
#line 2161 "verilogaYacc.y"
          {
            p_adms m=YY((yyvsp[0]._yaccval));
            p_mapply_unary mymathapply=adms_mapply_unary_new(admse_minus,m);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mymathapply);
          }
#line 4609 "y.tab.c"
    break;

  case 250: /* R_e.unary: '!' R_e.atomic  */
#line 2168 "verilogaYacc.y"
          {
            p_adms m=YY((yyvsp[0]._yaccval));
            p_mapply_unary mymathapply=adms_mapply_unary_new(admse_not,m);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mymathapply);
          }
#line 4620 "y.tab.c"
    break;

  case 251: /* R_e.unary: '~' R_e.atomic  */
#line 2175 "verilogaYacc.y"
          {
            p_adms m=YY((yyvsp[0]._yaccval));
            p_mapply_unary mymathapply=adms_mapply_unary_new(admse_bw_not,m);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)mymathapply);
          }
#line 4631 "y.tab.c"
    break;

  case 252: /* R_e.atomic: tk_integer  */
#line 2184 "verilogaYacc.y"
          {
            p_number mynumber=adms_number_new((yyvsp[0]._lexval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            mynumber->_cast=admse_i;
            Y((yyval._yaccval),(p_adms)mynumber);
          }
#line 4642 "y.tab.c"
    break;

  case 253: /* R_e.atomic: tk_integer tk_ident  */
#line 2191 "verilogaYacc.y"
          {
            char* mylexval2=((p_lexval)(yyvsp[0]._lexval))->_string;
            p_number mynumber=adms_number_new((yyvsp[-1]._lexval));
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
              adms_veriloga_message_fatal(" can not convert symbol to valid unit\n",(yyvsp[0]._lexval));
            mynumber->_scalingunit=myunit;
            mynumber->_cast=admse_i;
            Y((yyval._yaccval),(p_adms)mynumber);
          }
#line 4677 "y.tab.c"
    break;

  case 254: /* R_e.atomic: tk_number  */
#line 2222 "verilogaYacc.y"
          {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)adms_number_new((yyvsp[0]._lexval)));
          }
#line 4686 "y.tab.c"
    break;

  case 255: /* R_e.atomic: tk_number tk_ident  */
#line 2227 "verilogaYacc.y"
          {
            char* mylexval2=((p_lexval)(yyvsp[0]._lexval))->_string;
            p_number mynumber=adms_number_new((yyvsp[-1]._lexval));
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
              adms_veriloga_message_fatal(" can not convert symbol to valid unit\n",(yyvsp[0]._lexval));
            mynumber->_scalingunit=myunit;
            Y((yyval._yaccval),(p_adms)mynumber);
          }
#line 4720 "y.tab.c"
    break;

  case 256: /* R_e.atomic: tk_char  */
#line 2257 "verilogaYacc.y"
          {
            adms_veriloga_message_fatal("%s: character are not handled\n",(yyvsp[0]._lexval));
          }
#line 4728 "y.tab.c"
    break;

  case 257: /* R_e.atomic: tk_anystring  */
#line 2261 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[0]._lexval))->_string;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)adms_string_new(mylexval1));
          }
#line 4738 "y.tab.c"
    break;

  case 258: /* R_e.atomic: tk_ident  */
#line 2267 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[0]._lexval))->_string;
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[0]._lexval));
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
              adms_veriloga_message_fatal("identifier never declared\n",(yyvsp[0]._lexval));
          }
#line 4760 "y.tab.c"
    break;

  case 259: /* R_e.atomic: tk_dollar_ident  */
#line 2285 "verilogaYacc.y"
          {
            p_function myfunction=adms_function_new((yyvsp[0]._lexval),uid++);
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            Y((yyval._yaccval),(p_adms)myfunction);
          }
#line 4770 "y.tab.c"
    break;

  case 260: /* R_e.atomic: tk_ident '[' R_expression ']'  */
#line 2291 "verilogaYacc.y"
          {
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            p_variable myvariable=variable_recursive_lookup_by_id(gBlockList->data,(yyvsp[-3]._lexval));
            if(!myvariable)
               adms_veriloga_message_fatal("undefined array variable\n",(yyvsp[-3]._lexval));
            Y((yyval._yaccval),(p_adms)adms_array_new(myvariable,YY((yyvsp[-1]._yaccval))));
          }
#line 4782 "y.tab.c"
    break;

  case 261: /* R_e.atomic: tk_dollar_ident '(' R_l.enode ')'  */
#line 2299 "verilogaYacc.y"
          {
            p_function myfunction=adms_function_new((yyvsp[-3]._lexval),uid++);
            p_slist myArgs=(p_slist)YY((yyvsp[-1]._yaccval));
            (yyval._yaccval)=adms_yaccval_new("unknown source file");
            adms_slist_inreverse(&myArgs);
            myfunction->_arguments=myArgs;
            Y((yyval._yaccval),(p_adms)myfunction);
          }
#line 4795 "y.tab.c"
    break;

  case 262: /* R_e.atomic: tk_ident '(' R_l.enode ')'  */
#line 2308 "verilogaYacc.y"
          {
            char* mylexval1=((p_lexval)(yyvsp[-3]._lexval))->_string;
            char* myfunctionname=mylexval1;
            p_slist myArgs=(p_slist)YY((yyvsp[-1]._yaccval));
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
                adms_veriloga_message_fatal("bad argument (expecting node or branch)\n",(yyvsp[-3]._lexval));
            }
            else if(mynature && narg==2)
            {
              p_adms mychild0=(p_adms)adms_slist_nth_data(myArgs,0);
              p_adms mychild1=(p_adms)adms_slist_nth_data(myArgs,1);
              p_branch mybranch;
              if(mychild0->_datatypename!=admse_node)
                adms_veriloga_message_fatal("second argument of probe is not a node\n",(yyvsp[-3]._lexval));
              if(mychild1->_datatypename!=admse_node)
                adms_veriloga_message_fatal("first argument of probe is not a node\n",(yyvsp[-3]._lexval));
              mybranch=adms_module_list_branch_prepend_by_id_once_or_ignore(gModule,gModule,(p_node)mychild1,((p_node)mychild0));
              myprobe=adms_module_list_probe_prepend_by_id_once_or_ignore(gModule,gModule,mybranch,mynature);
            }
            if(myprobe)
              Y((yyval._yaccval),(p_adms)myprobe);
            else
            {
              p_slist l;
              p_function myfunction=adms_function_new((yyvsp[-3]._lexval),uid++);
              for(l=gModule->_analogfunction;l&&(myfunction->_definition==NULL);l=l->next)
              {
                p_analogfunction myanalogfunction=(p_analogfunction)l->data;
                if(!strcmp((yyvsp[-3]._lexval)->_string,myanalogfunction->_lexval->_string))
                  myfunction->_definition=myanalogfunction;
              }
              myfunction->_arguments=adms_slist_reverse(myArgs);
              Y((yyval._yaccval),(p_adms)myfunction);
            }
          }
#line 4851 "y.tab.c"
    break;

  case 263: /* R_e.atomic: '(' R_expression ')'  */
#line 2360 "verilogaYacc.y"
          {
            (yyval._yaccval)=(yyvsp[-1]._yaccval);
          }
#line 4859 "y.tab.c"
    break;


#line 4863 "y.tab.c"

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
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;
#endif


/*-------------------------------------------------------.
| yyreturn -- parsing is finished, clean up and return.  |
`-------------------------------------------------------*/
yyreturn:
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

#line 2364 "verilogaYacc.y"

void adms_veriloga_setint_yydebug(const int val)
{
  yydebug=val;
}
