
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
 $Id: verilog.y,v 1.5 2008/09/22 02:47:43 stevew Exp $
 *========================================================================*/

/*========================================================================*
  Copyright (c) 1992, 1993
        Regents of the University of California
  All rights reserved.

  Use and copying of this software and preparation of derivative works
  based upon this software are permitted.  However, any distribution of
  this software or derivative works must include the above copyright 
  notice.

  This software is made available AS IS, and neither the Electronics
  Research Laboratory or the Universify of California make any
  warranty about the software, its performance or its conformity to
  any specification.

  Author: Szu-Tsung Cheng, stcheng@ic.Berkeley.EDU
          10/92
          10/93
 *========================================================================*/

%{

#include <stdio.h>
#include "vl_list.h"
#include "vl_st.h"
#include "vl_defs.h"
#include "vl_types.h"

int YYTrace = 0;
char yyid[MAXSTRLEN];
bitexp_parse yybit;

static vl_strength *drive_strength(int, int);
static unsigned char setprim(unsigned char, unsigned char);

%}

%union {
    int                   ival;
    char                  *charp;
    vl_stmt               *stmt;

    lsList<void*>               *l_vp;
    lsList<int>                 *l_int;
    lsList<char*>               *l_chr;
    lsList<vl_module*>          *l_mod;
    lsList<vl_var*>             *l_var;
    lsList<vl_port*>            *l_por;
    lsList<vl_port_connect*>    *l_pc;
    lsList<vl_decl*>            *l_dcl;
    lsList<vl_bassign_stmt*>    *l_bas;
    lsList<vl_case_item*>       *l_ci;
    lsList<vl_expr*>            *l_exp;
    lsList<vl_gate_inst*>       *l_gti;
    lsList<vl_mp_inst*>         *l_mpi;
    lsList<vl_prim_entry*>      *l_pe;
    lsList<vl_stmt*>            *l_stm;
    lsList<vl_specify_item*>    *l_spi;
    lsList<vl_spec_term_desc*>  *l_std;

    vl_bassign_stmt       *basgn;
    vl_begin_end_stmt     *seqstmt;
    vl_case_item          *caseitem;
    vl_cont_assign        *casgn;
    vl_decl               *bascdcl;
    vl_dlstr              *dlstr;
    vl_strength           *stren;
    vl_delay              *dlay;
    vl_desc               *desc;
    vl_event_expr         *evnt;
    vl_expr               *expr;
    vl_fork_join_stmt     *parstmt;
    vl_function           *func;
    vl_gate_inst          *gtinst;
    vl_mp_inst            *mpinst;
    vl_gate_inst_list     *gtlist;
    vl_var                *var;
    vl_module             *modu;
    vl_port               *port;
    vl_port_connect       *connect;
    vl_prim_entry         *ntry;
    vl_primitive          *prim;
    vl_procstmt           *prcstmt;
    vl_range              *rang;
    vl_range_or_type      *rngtyp;
    vl_task               *task;
    vl_task_enable_stmt   *taskenablestmt;
    multi_concat          *mconcat;
    vl_sys_task_stmt      *systask;
    vl_specify_block      *specblk;
    vl_specify_item       *specitm;
    vl_path_desc          *pathdesc;
    vl_spec_term_desc     *termdesc;
}


%start source_text

%token YYNAME                      
%token YYINUMBER                 
%token YYRNUMBER                 
%token YYSTRING                  
%token YYALLPATH                 
%token YYALWAYS                  
%token YYAND                     
%token YYASSIGN                  
%token YYBEGIN                   
%token YYBUF                     
%token YYBUFIF0                  
%token YYBUFIF1                  
%token YYCASE                    
%token YYCASEX                   
%token YYCASEZ                   
%token YYCMOS                    
%token YYCONDITIONAL             
%token YYDEASSIGN                
%token YYDEFAULT                 
%token YYDEFPARAM                
%token YYDISABLE                 
%token YYELSE                    
%token YYEND                     
%token YYENDCASE                 
%token YYENDMODULE               
%token YYENDFUNCTION             
%token YYENDPRIMITIVE            
%token YYENDSPECIFY              
%token YYENDTABLE                
%token YYENDTASK                 
%token YYENUM                    
%token YYEVENT                   
%token YYFOR                     
%token YYFOREVER                 
%token YYFORCE
%token YYFORK                    
%token YYFUNCTION                
%token YYGEQ                     
%token YYHIGHZ0                  
%token YYHIGHZ1                  
%token YYIF                      
%token YYINITIAL                 
%token YYINOUT                   
%token YYINPUT                   
%token YYINTEGER                 
%token YYJOIN                    
%token YYLARGE                   
%token YYLEADTO                  
%token YYLEQ                     
%token YYLOGAND                  
%token YYCASEEQUALITY            
%token YYCASEINEQUALITY          
%token YYLOGNAND                 
%token YYLOGNOR                  
%token YYLOGOR                   
%token YYLOGXNOR                 
%token YYLOGEQUALITY             
%token YYLOGINEQUALITY           
%token YYLSHIFT                  
%token YYMEDIUM                  
%token YYMODULE                  
%token YYNAND                    
%token YYNBASSIGN                
%token YYNEGEDGE                 
%token YYNMOS                    
%token YYNOR                     
%token YYNOT                     
%token YYNOTIF0                  
%token YYNOTIF1                  
%token YYOR                      
%token YYOUTPUT                  
%token YYPARAMETER               
%token YYPMOS                    
%token YYPOSEDGE                 
%token YYPRIMITIVE               
%token YYPULL0                   
%token YYPULL1                   
%token YYPULLUP                  
%token YYPULLDOWN                
%token YYRCMOS                   
%token YYREAL                    
%token YYREG                     
%token YYRELEASE
%token YYREPEAT                  
%token YYRIGHTARROW              
%token YYRNMOS                   
%token YYRPMOS                   
%token YYRSHIFT                  
%token YYRTRAN                   
%token YYRTRANIF0                
%token YYRTRANIF1                
%token YYSCALARED                
%token YYSMALL                   
%token YYSPECIFY                 
%token YYSPECPARAM               
%token YYSTRONG0                 
%token YYSTRONG1                 
%token YYSUPPLY0                 
%token YYSUPPLY1                 
%token YYTABLE                   
%token YYTASK                    
%token YYTIME                    
%token YYTRAN                    
%token YYTRANIF0                 
%token YYTRANIF1                 
%token YYTRI                     
%token YYTRI0                    
%token YYTRI1                    
%token YYTRIAND                  
%token YYTRIOR                   
%token YYTRIREG                  
%token YYVECTORED                
%token YYWAIT                    
%token YYWAND                    
%token YYWEAK0                   
%token YYWEAK1                   
%token YYWHILE                   
%token YYWIRE                    
%token YYWOR                     
%token YYXNOR                    
%token YYXOR                     
%token YYsysSETUP                
%token YYsysID                   

%right YYCONDITIONAL
%right '?' ':'
%left YYOR
%left YYLOGOR
%left YYLOGAND
%left '|'
%left '^' YYLOGXNOR
%left '&'
%left YYLOGEQUALITY YYLOGINEQUALITY YYCASEEQUALITY YYCASEINEQUALITY
%left '<' YYLEQ '>' YYGEQ YYNBASSIGN
%left YYLSHIFT YYRSHIFT
%left '+' '-'
%left '*' '/' '%'
%right '~' '!' YYUNARYOPERATOR

%type <l_por> port_list_opt
%type <l_por> port_list
%type <l_var> port_expression_opt
%type <l_var> port_expression
%type <l_var> port_ref_list
%type <l_stm> module_item_clr
%type <l_var> variable_list
%type <l_dcl> primitive_declaration_eclr
%type <l_pe> table_definition
%type <basgn> prim_initial
%type <l_pe> table_entries
%type <l_pe> combinational_entry_eclr
%type <l_pe> sequential_entry_eclr
%type <l_int> input_list
%type <l_dcl> tf_declaration_clr
%type <l_dcl> tf_declaration_eclr
%type <l_stm> statement_opt
%type <l_int> level_symbol_or_edge_eclr
%type <l_bas> assignment_list
%type <l_bas> id_assignment_list
%type <l_var> register_variable_list
%type <l_chr> name_of_event_list
%type <l_gti> gate_instance_list
%type <l_exp> terminal_list
%type <l_mpi> module_or_primitive_instance_list
%type <l_pc> module_connection_list
%type <l_pc> module_port_connection_list
%type <l_pc> named_port_connection_list
%type <l_stm> statement_clr
%type <l_ci> case_item_eclr
%type <l_exp> expression_list
%type <l_dcl> block_declaration_clr
%type <l_exp> concatenation 
%type <l_exp> mintypmax_expression_list
%type <l_exp> systask_args
%type <desc> source_text
%type <desc> description
%type <modu> module
%type <prim> primitive
%type <port> port
%type <var> port_reference
%type <rang> port_reference_arg
%type <stmt> module_item
%type <bascdcl> parameter_declaration
%type <bascdcl> input_declaration output_declaration inout_declaration
%type <bascdcl> reg_declaration
%type <bascdcl> net_declaration
%type <bascdcl> time_declaration event_declaration
%type <bascdcl> integer_declaration real_declaration
%type <gtlist>  gate_instantiation
%type <stmt> module_or_primitive_instantiation
%type <bascdcl> parameter_override
%type <casgn>   continuous_assign
%type <prcstmt> initial_statement always_statement
%type <task> task
%type <func> function
%type <ntry> combinational_entry sequential_entry
%type <ival> output_symbol state next_state 
%type <ival> level_symbol edge level_symbol_or_edge edge_symbol
%type <bascdcl> primitive_declaration
%type <rngtyp> range_or_type_opt range_or_type
%type <rang> range
%type <bascdcl> tf_declaration
%type <rang> range_opt
%type <expr> expression
%type <dlstr> drive_delay_clr
%type <dlstr> module_or_primitive_option_clr
%type <stren> drive_strength_opt drive_strength 
%type <stren> charge_strength_opt charge_strength
%type <ival> nettype
%type <rang> expandrange_opt expandrange
%type <dlay> delay_opt delay
%type <charp> identifier
%type <charp> id_token
%type <var> register_variable
%type <charp> name_of_register
%type <charp> name_of_event
%type <ival> strength0 strength1
%type <basgn> assignment
%type <basgn> id_assignment
%type <ival> gatetype
%type <gtinst> gate_instance
%type <charp> name_of_gate_instance name_of_module_or_primitive
%type <expr> terminal
%type <dlay> delay_or_parameter_value_assignment
%type <mpinst> module_or_primitive_instance
%type <connect> module_port_connection named_port_connection
%type <stmt> statement
%type <caseitem> case_item
%type <dlay> delay_control
%type <evnt> event_control
%type <evnt> repeated_event_control
%type <var> lvalue
%type <seqstmt> seq_block
%type <parstmt> par_block
%type <charp> name_of_block
%type <bascdcl> block_declaration
%type <taskenablestmt>  task_enable
%type <expr> mintypmax_expression
%type <expr> primary
%type <expr> function_call
%type <evnt> event_expression ored_event_expression
%type <mconcat> multiple_concatenation
%type <charp> name_of_system_task;
%type <charp> name_of_identifier;
%type <systask> system_task_enable
%type <charp> system_identifier
%type <specblk> specify_block
%type <l_spi> specify_item_clr
%type <specitm> specify_item
%type <specitm> specparam_declaration
%type <specitm> path_declaration
%type <specitm> level_sensitive_path_declaration
%type <specitm> edge_sensitive_path_declaration
%type <specitm> system_timing_check
%type <l_exp> path_delay_value
%type <expr> path_delay_expression
%type <pathdesc> path_description
%type <termdesc> specify_terminal_descriptor
%type <l_std> path_list
%type <l_std> path_list_or_edge_sensitive_path_list
%type <ival> polarity_operator
%type <ival> polarity_operator_opt
%type <ival> edge_identifier
%%

source_text
        :
          {
              YYTRACE("source_text:");
              if (!VP.description)
                  VP.description = new vl_desc();
              $$ = VP.description;
          }
        | source_text description
          {
              YYTRACE("source_text: source_text description");
          }
        ;

description 
        : module
          {
              YYTRACE("description: module");
          }
        | primitive
          {
              YYTRACE("description: primitive");
          }
        ;

module 
        : YYMODULE identifier 
          {
              vl_module *currentModule;
              if (!VP.description->mp_st->lookup($2, (vl_mp**)&currentModule))
                  currentModule = new vl_module(VP.description, $2, 0, 0); 
              VP.context = VP.context->push(currentModule);
              VP.description->mp_undefined->set_eliminate($2);
              currentModule->tunit = VP.tunit;
              currentModule->tprec = VP.tprec;
          } 
          port_list_opt ';'
          {
              VP.context->currentModule()->ports = $4;
          }
          module_item_clr
          YYENDMODULE
          {
              YYTRACE("module: YYMODULE YYNAME port_list_opt ';' "
                  "module_item_clr YYENDMODULE");
              VP.context->currentModule()->mod_items = $7;
              $$ = VP.context->currentModule();
              VP.context = VP.context->pop();
          }
        ;

port_list_opt
        :
          {
              YYTRACE("port_list_opt:");
              $$ = 0;
          }
        | '(' port_list ')'
          {
              YYTRACE("port_list_opt: '(' port_list ')'");
              if ($2->length() == 1) {
                  // throw out a single empty port
                  vl_port *p;
                  $2->firstItem(&p);
                  if (!p || !p->port_exp)  {
                      delete $2;
                      $2 = 0;
                  }
              }
              $$ = $2;
          }
        ;

port_list
        : port
          {
              YYTRACE("port_list: port");
              $$ = new lsList<vl_port*>;
              $$->newEnd($1);
          }
        | port_list ',' port
          {
              YYTRACE("port_list: port_list ',' port");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

port
        : port_expression_opt
          {
              YYTRACE("port: port_expression_opt");
              $$ = new vl_port(ModulePort, 0, $1);
          }
        | '.' identifier '(' port_expression_opt ')'
          {
              YYTRACE("port: '.' identifier '(' port_expression_opt ')'");
              $$ = new vl_port(NamedPort, $2, $4);
          }
        ;

port_expression_opt
        :
          {
              YYTRACE("port_expression_opt:");
              $$ = 0;
          }
        |  port_expression
          {
              YYTRACE("port_expression_opt: port_expression");
              $$ = $1;
          }
        ;

port_expression
        : port_reference
          {
              YYTRACE("port_expression: port_reference");
              $$ = new lsList<vl_var*>;
              $$->newEnd($1);
          }
        | '{' port_ref_list '}'
          {
              YYTRACE("port_expression: '{' port_ref_list '}'");
              $$ = $2;
          }
        ;

port_ref_list
        : port_reference
          {
              YYTRACE("port_ref_list: port_reference");
              $$ = new lsList<vl_var*>;
              $$->newEnd($1);
          }
        | port_ref_list ',' port_reference
          {
              YYTRACE("port_ref_list: port_ref_list ',' port_reference");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

port_reference
        : identifier port_reference_arg
          {
              YYTRACE("port_reference: YYNAME port_reference_arg");
              $$ = new vl_var($1, $2);
          }
        ;

port_reference_arg
        :
          {
              YYTRACE("port_reference_arg:");
              $$ = 0;
          }
        | '[' expression ']' 
          {
              YYTRACE("port_reference_arg: '[' expression ']'");
              $$ = new vl_range($2, 0);
          }
        | '[' expression ':' expression ']' 
          {
              YYTRACE("port_reference_arg: '[' expression ':' expression ']'");
              $$ = new vl_range($2, $4);
          }
        ;

module_item_clr
        :
          {
              YYTRACE("module_item_clr:");
              $$ = new lsList<vl_stmt*>;
          }
        | module_item_clr module_item
          {
              YYTRACE("module_item_clr: module_item_clr module_item");
              $1->newEnd($2);
              $$ = $1;
          }
        ;

module_item
        : parameter_declaration
          {
              YYTRACE("module_item: parameter_declaration");
              $$ = $1;
          }
        | input_declaration
          {
              YYTRACE("module_item: input_declaration");
              $$ = $1;
          }
        | output_declaration
          {
              YYTRACE("module_item: output_declaration");
              $$ = $1;
          }
        | inout_declaration
          {
              YYTRACE("module_item: inout_declaration");
              $$ = $1;
          }
        | net_declaration
          {
              YYTRACE("module_item: net_declaration");
              $$ = $1;
          }
        | reg_declaration
          {
              YYTRACE("module_item: reg_declaration");
              $$ = $1;
          }
        | time_declaration
          {
              YYTRACE("module_item: time_declaration");
              $$ = $1;
          }
        | integer_declaration
          {
              YYTRACE("module_item: integer_declaration");
              $$ = $1;
          }
        | real_declaration
          {
              YYTRACE("module_item: real_declaration");
              $$ = $1;
          }
        | event_declaration
          {
              YYTRACE("module_item: event_declaration");
              $$ = $1;
          }
        | gate_instantiation
          {
              YYTRACE("module_item: gate_instantiation");
              $$ = $1;
          }
        | module_or_primitive_instantiation
          {
              YYTRACE("module_item: module_or_primitive_instantiation");
              $$ = $1;
          }
        | parameter_override
          {
              YYTRACE("module_item: parameter_override");
              $$ = $1;
          }
        | continuous_assign
          {
              YYTRACE("module_item: continous_assign");
              $$ = $1;
          }
        | specify_block
          {
              YYTRACE("module_item: specify_block");
              $$ = $1;
          }
        | initial_statement
          {
              YYTRACE("module_item: initial_statement");
              $$ = $1;
          }
        | always_statement
          {
              YYTRACE("module_item: always_statement");
              $$ = $1;
          }
        | task
          {
              YYTRACE("module_item: task");
              $$ = $1;
          }
        | function
          {
              YYTRACE("module_item: function");
              $$ = $1;
          }
        ;

primitive
        : YYPRIMITIVE identifier 
          {
              vl_primitive *currentPrimitive =
                  new vl_primitive(VP.description, $2);
              VP.context = VP.context->push(currentPrimitive);
              VP.description->mp_undefined->set_eliminate($2);
          }
          '(' port_list ')' ';' primitive_declaration_eclr prim_initial
                table_definition YYENDPRIMITIVE
          {
              YYTRACE("primitive: YYPRMITIVE YYNAME '(' variable_list ')' ';' "
                  "primitive_declaration_eclr prim_initial table_definition "
                  "YYENDPRIMITIVE");
              VP.context->currentPrimitive()->init_table($5, $8, $9, $10);
              $$ = VP.context->currentPrimitive();
              VP.context = VP.context->pop();
          }
        ;

primitive_declaration_eclr
        : primitive_declaration
          {
              YYTRACE("primitive_declaration_eclr: primitive_declaration");
              $$ = new lsList<vl_decl*>;
              $$->newEnd($1);
          }
        | primitive_declaration_eclr primitive_declaration
          {
              YYTRACE("primitive_declaration_eclr: primitive_declaration_eclr "
                  "primitive_declaration");
              $1->newEnd($2);
              $$ = $1;
          }
        ;

primitive_declaration
        : output_declaration
          {
              YYTRACE("primitive_declaration: output_declaration");
              $$ = $1;
          }
        | reg_declaration
          {
              YYTRACE("primitive_decalration: reg_declaration");
              $$ = $1;
          }
        | input_declaration
          {
              YYTRACE("primitive_decalration: input_declaration");
              $$ = $1;
          }
        ;

prim_initial
        : YYINITIAL assignment ';'
          {
              YYTRACE("primitive_initial: assignment");
              $$ = $2;
          }
        |
          {
              $$ = 0;
          }
        ;

table_definition
        : YYTABLE table_entries YYENDTABLE
          {
              YYTRACE("table_definition: YYTABLE table_entries YYENDTABLE");
              $$ = $2;
          }
        ;

table_entries
        : combinational_entry_eclr
          {
              YYTRACE("table_definition: combinational_entry_eclr");
          }
        | sequential_entry_eclr
          {
              YYTRACE("table_definition: sequential_entry_eclr");
              VP.context->currentPrimitive()->type = SeqPrimDecl;
          }
        ;

combinational_entry_eclr
        : combinational_entry
          {
              YYTRACE("combinational_entry_eclr: combinational_entry");
              $$ = new lsList<vl_prim_entry*>;
              $$->newEnd($1);
          }
        | combinational_entry_eclr combinational_entry
          {
              YYTRACE("combinational_entry_eclr: combinational_entry_eclr "
                  "combinational_entry");
              $1->newEnd($2);
              $$ = $1;
          }
        ;

combinational_entry
        : input_list ':' output_symbol ';'
          {
              YYTRACE("combinational_entry: input_list ':' output_symbol ';'");
              $$ = new vl_prim_entry($1, PrimNone, $3);
          }
        ;

sequential_entry_eclr
        : sequential_entry
          {
              YYTRACE("sequential_entry_eclr: sequential_entry");
              $$ = new lsList<vl_prim_entry*>;
              $$->newEnd($1);
          }
        | sequential_entry_eclr sequential_entry
          {
              YYTRACE("sequential_entry_eclr: sequential_entry_eclr sequential_entry");
              $1->newEnd($2);
              $$ = $1;
          }
        ;

sequential_entry
        : input_list ':' state ':' next_state ';'
          {
              YYTRACE("sequential_entry: input_list ':' state ':' "
                  "next_state ';'");
              $$ = new vl_prim_entry($1, $3, $5);
          }
        ;

input_list
        : level_symbol_or_edge_eclr
          {
              YYTRACE("input_list: level_symbol_or_edge_eclr");
          }
        ;

level_symbol_or_edge_eclr
        : level_symbol_or_edge
          {
              YYTRACE("level_symbol_or_edge_eclr: level_symbol_or_edge");
              $$ = new lsList<int>(true);
              $$->newEnd($1);
          }
        | level_symbol_or_edge_eclr level_symbol_or_edge
          {
              YYTRACE("level_symbol_or_edge_eclr: level_symbol_or_edge_eclr "
                  "level_symbol_or_edge");
              $1->newEnd($2);
              $$ = $1;
          }
        ;

level_symbol_or_edge
        : level_symbol
          {
              YYTRACE("level_symbol_or_edge: level_symbol");
          }
        | edge
          {
              YYTRACE("level_symbol_or_edge: edge");
          }
        ;

edge
        : '(' level_symbol level_symbol ')'
          {
              YYTRACE("edge: '(' level_symbol level_symbol ')'");
              $$ = setprim($2, $3);
          }
        | edge_symbol
          {
              YYTRACE("edge: edge_symbol");
          }
        ;

state
        : level_symbol
          {
              YYTRACE("state: level_symbol");
          }
        ;

next_state
        : output_symbol
          {
              YYTRACE("next_state: output_symbol");
          }
        | '-'
          {
              YYTRACE("next_state: '_'");
              $$ = PrimM;
          }
        ;

output_symbol
        : '0'
          {
              YYTRACE("output_symbol: '0'");
              $$ = Prim0;
          }
        | '1'
          {
              YYTRACE("output_symbol: '1'");
              $$ = Prim1;
          }
        | 'x'
          {
              YYTRACE("output_symbol: 'x'");
              $$ = PrimX;
          }
        | 'X'
          {
              YYTRACE("output_symbol: 'X'");
              $$ = PrimX;
          }
        ;

level_symbol
        : '0'
          {
              YYTRACE("level_symbol: '0'");
              $$ = Prim0;
          }
        | '1'
          {
              YYTRACE("level_symbol: '1'");
              $$ = Prim1;
          }
        | 'x'
          {
              YYTRACE("level_symbol: 'x'");
              $$ = PrimX;
          }
        | 'X'
          {
              YYTRACE("level_symbol: 'X'");
              $$ = PrimX;
          }
        | '?'
          {
              YYTRACE("level_symbol: '?'");
              $$ = PrimQ;
          }
        | 'b'
          {
              YYTRACE("level_symbol: 'b'");
              $$ = PrimB;
          }
        | 'B'
          {
              YYTRACE("level_symbol: 'B'");
              $$ = PrimB;
          }
        ;

edge_symbol
        : 'r'
          {
              YYTRACE("edge_symbol: 'r'");
              $$ = PrimR;
          }
        | 'R'
          {
              YYTRACE("edge_symbol: 'R'");
              $$ = PrimR;
          }
        | 'f'
          {
              YYTRACE("edge_symbol: 'f'");
              $$ = PrimF;
          }
        | 'F'
          {
              YYTRACE("edge_symbol: 'F'");
              $$ = PrimF;
          }
        | 'p'
          {
              YYTRACE("edge_symbol: 'p'");
              $$ = PrimP;
          }
        | 'P'
          {
              YYTRACE("edge_symbol: 'P'");
              $$ = PrimP;
          }
        | 'n'
          {
              YYTRACE("edge_symbol: 'n'");
              $$ = PrimN;
          }
        | 'N'
          {
              YYTRACE("edge_symbol: 'N'");
              $$ = PrimN;
          }
        | '*'
          {
              YYTRACE("edge_symbol: '*'");
              $$ = PrimS;
          }
        ;

task
        : YYTASK identifier  ';' tf_declaration_clr statement_opt YYENDTASK
          {
              YYTRACE("YYTASK YYNAME ';' tf_declaration_clr statement_opt "
                  "YYENDTASK");
              $$ = new vl_task($2, $4, $5);
          }
        ;

function
        : YYFUNCTION range_or_type_opt identifier 
          {
             vl_range *range = 0;
             int type = RangeFuncDecl;
             if ($2) {
                 if ($2->type == Integer_Dcl)
                     type = IntFuncDecl;
                 else if ($2->type == Real_Dcl)
                     type = RealFuncDecl;
                 else if ($2->type == Range_Dcl)
                     type = RangeFuncDecl;
                 else
                     VP.error(ERR_INTERNAL, "bad type for function %s", $3);
                 range = $2->range;
                 $2->range = 0;
                 delete $2;
             }
             else if (VP.verbose)
                 VP.error(ERR_WARN, "no type/range for function %s", $3);
             vl_function *currentFunction = new vl_function(type, range, $3,
                 0, 0);
             VP.context = VP.context->push(currentFunction);
          }
          ';' tf_declaration_eclr statement_opt YYENDFUNCTION
          {
              YYTRACE("YYFUNCTION range_or_type_opt YYNAME ';' "
                  "tf_declaration_eclr statement_opt YYENDFUNCTION");
              $$ = VP.context->currentFunction();
              $$->decls = $6;
              $$->stmts = $7;
              VP.context = VP.context->pop();
          }
        ;

range_or_type_opt
        :
          {
              YYTRACE("range_or_type_opt:");
              $$ = 0;
          }
        | range_or_type
          {
              YYTRACE("range_or_type_opt: range_or_type");
              $$ = $1;
          }
        ;

range_or_type
        : range
          {
              YYTRACE("range_or_type: range");
              $$ = new vl_range_or_type(Range_Dcl, $1);
          }
        | YYINTEGER
          {
              YYTRACE("range_or_type: YYINTEGER");
              $$ = new vl_range_or_type(Integer_Dcl, 0);
          }
        | YYREAL
          {
              YYTRACE("range_or_type: YYREAL");
              $$ = new vl_range_or_type(Real_Dcl, 0);
          }
        ;

tf_declaration_clr
        :
          {
              YYTRACE("tf_declaration_clr:");
              $$ = new lsList<vl_decl*>;
          }
        | tf_declaration_clr tf_declaration
          {
              YYTRACE("tf_declaration_clr: tf_declaration_clr tf_declaration");
              $1->newEnd($2);
              $$ = $1;
          }
        ;

tf_declaration_eclr
        : tf_declaration
          {
              YYTRACE("tf_declaration_eclr: tf_declaration");
              $$ = new lsList<vl_decl*>;
              $$->newEnd($1);
          }
        | tf_declaration_eclr tf_declaration
          {
              YYTRACE("tf_declaration_eclr: tf_decalration_eclr "
                  "tf_declaration");
              $1->newEnd($2);
              $$ = $1;
          }
        ;

tf_declaration
        : parameter_declaration
          {
              YYTRACE("tf_declaration: parameter_decalration");
              $$ = $1;
          }
        | input_declaration
          {
              YYTRACE("tf_declaration: input_declaration");
              $$ = $1;
          }
        | output_declaration
          {
              YYTRACE("tf_declaration: output_declaration");
              $$ = $1;
          }
        | inout_declaration
          {
              YYTRACE("tf_declaration: inout_declaration");
              $$ = $1;
          }
        | reg_declaration
          {
              YYTRACE("tf_declaration: reg_declaration");
              $$ = $1;
          }
        | time_declaration
          {
              YYTRACE("tf_declaration: time_declaration");
              $$ = $1;
          }
        | integer_declaration
          {
              YYTRACE("tf_declaration: integer_declaration");
              $$ = $1;
          }
        | real_declaration
          {
              YYTRACE("tf_declaration: real_declaration");
              $$ = $1;
          }
        | event_declaration
          {
              YYTRACE("tf_declaration: event_declaration");
              $$ = $1;
          }
        ;

parameter_declaration
        : YYPARAMETER range_opt assignment_list ';'
          {
              YYTRACE("parameter_declaration: YYPARAMETER range_opt "
                  "assignment_list ';'");
              $$ = new vl_decl(ParamDecl, $2, $3);
          }
        ;

input_declaration
        : YYINPUT range_opt variable_list ';'
          {
              YYTRACE("input_declaration: YYINPUT range_opt "
                  "variable_list ';'");
              $$ = new vl_decl(InputDecl, $2, $3);
          }
        ;

output_declaration
        : YYOUTPUT range_opt variable_list ';'
          {
              YYTRACE("output_declaration: YYOUTPUT range_opt "
                  "variable_list ';'");
              $$ = new vl_decl(OutputDecl, $2, $3);
          }
        ;

inout_declaration
        : YYINOUT range_opt variable_list ';'
          {
              YYTRACE("inout_declaration: YYINOUT range_opt "
                  "variable_list ';'");
              $$ = new vl_decl(InoutDecl, $2, $3);
          }
        ;

net_declaration
        : nettype drive_strength_opt expandrange_opt delay_opt 
            assignment_list ';'
          {
              YYTRACE("net_declaration: nettype drive_strength_opt "
                  "expandrange_opt delay_opt assignment_list ';'");
              $$ = new vl_decl($1, *$2, $3, $4, $5, 0);
          }
        | nettype drive_strength_opt expandrange_opt delay_opt 
            variable_list ';'
          {
              YYTRACE("net_declaration: nettype drive_strength_opt "
                  "expandrange_opt delay_opt variable_list ';'");
              $$ = new vl_decl($1, *$2, $3, $4, 0, $5);
          }
        | YYTRIREG charge_strength_opt expandrange_opt delay_opt 
            variable_list ';'
          {
              YYTRACE("net_declaration: YYTRIREG charge_strength_opt "
                  "expandrange_opt delay_opt variable_list ';'");
              $$ = new vl_decl(TriregDecl, *$2, $3, $4, 0, $5);
          }
        ;

nettype
        : YYWIRE
          {
              YYTRACE("nettype: YYWIRE");
              $$ = WireDecl;
          }
        | YYTRI
          {
              YYTRACE("nettype: YYTRI");
              $$ = TriDecl;
          }
        | YYTRI1
          {
              YYTRACE("nettype: YYTRI1");
              $$ = Tri1Decl;
          }
        | YYSUPPLY0
          {
              YYTRACE("nettype: YYSUPPLY0");
              $$ = Supply0Decl;
          }
        | YYWAND
          {
              YYTRACE("nettype: YYWAND");
              $$ = WandDecl;
          }
        | YYTRIAND
          {
              YYTRACE("nettype: YYTRIAND");
              $$ = TriandDecl;
          }
        | YYTRI0
          {
              YYTRACE("nettype: YYTRI0");
              $$ = Tri0Decl;
          }
        | YYSUPPLY1
          {
              YYTRACE("nettype: YYSUPPLY1");
              $$ = Supply1Decl;
          } 
        | YYWOR
          {
              YYTRACE("nettype: YYWOR");
              $$ = WorDecl;
          }
        | YYTRIOR
          {
              YYTRACE("nettype: YYTRIOR");
              $$ = TriorDecl;
          }
        ;

expandrange_opt
        :
          {
              YYTRACE("expandrange_opt:");
              $$ = 0;
          }
        | expandrange
          {
              YYTRACE("expandrange_opt: expandrange");
              $$ = $1;
          }
        ;

expandrange
        : range
          {
              YYTRACE("expandrange: range");
          }
        | YYSCALARED range
          {
              YYTRACE("expandrange: YYSCALARED range");
              $$ = $2;
          }
        | YYVECTORED range
          {
              YYTRACE("expandrange: YYVECTORED range");
              $$ = $2;
          }
        ;

reg_declaration
        : YYREG range_opt register_variable_list ';'
          {
              YYTRACE("reg_declaration: YYREG range_opt "
                  "register_variable_list ';'");
              $$ = new vl_decl(RegDecl, $2, $3);
          }
        | YYREG range_opt id_assignment_list ';'
          {
              // Extension: assignment in declaration
              YYTRACE("reg_declaration: YYREG range_opt "
                  "id_assignment_list ';'");
              vl_strength s;
              $$ = new vl_decl(RegDecl, s, $2, 0, $3, 0);
          }
        ;

time_declaration
        : YYTIME register_variable_list ';'
          {
              YYTRACE("time_declaration: YYTIME register_variable_list ';'");
              $$ = new vl_decl(TimeDecl, 0, $2);
          }
        | YYTIME id_assignment_list ';'
          {
              // Extension: assignment in declaration
              YYTRACE("time_declaration: YYTIME id_assignment_list ';'");
              vl_strength s;
              $$ = new vl_decl(TimeDecl, s, 0, 0, $2, 0);
          }
        ;

integer_declaration
        : YYINTEGER register_variable_list ';'
          {
              YYTRACE("integer_declaration: YYINTEGER "
                  "register_variable_list ';'");
              $$ = new vl_decl(IntDecl, 0, $2);
          }
        | YYINTEGER id_assignment_list ';'
          {
              // Extension: assignment in declaration
              YYTRACE("integer_declaration: YYINTEGER id_assignment_list ';'");
              vl_strength s;
              $$ = new vl_decl(IntDecl, s, 0, 0, $2, 0);
          }
        ;

real_declaration
        : YYREAL variable_list ';'
          {
              YYTRACE("real_declaration: YYREAL variable_list ';'");
              $$ = new vl_decl(RealDecl, 0, $2);
          }
        | YYREAL id_assignment_list ';'
          {
              // Extension: assignment in declaration
              YYTRACE("real_declaration: YYREAL id_assignment_list ';'");
              vl_strength s;
              $$ = new vl_decl(RealDecl, s, 0, 0, $2, 0);
          }
        ;

event_declaration
        : YYEVENT name_of_event_list ';'
          {
              YYTRACE("event_declaration: YYEVENT name_of_event_list ';'");
              $$ = new vl_decl(EventDecl, $2);
          }
        ;

continuous_assign
        : YYASSIGN drive_strength_opt delay_opt assignment_list ';'
          {
              YYTRACE("continuous_assign: YYASSIGN drive_strength_opt "
                  "delay_opt assignment_list ';'");
              $$ = new vl_cont_assign(*$2, $3, $4);
          }
        ;

parameter_override
        : YYDEFPARAM assignment_list ';'
          {
              YYTRACE("parameter_override: YYDEFPARAM assign_list ';'");
              $$ = new vl_decl(DefparamDecl, 0, $2);
          }
        ;

variable_list
        : identifier
          {
              YYTRACE("variable_list: identifier");
              $$ = new lsList<vl_var*>;
              $$->newEnd(new vl_var($1, 0));
          }
        | variable_list ',' identifier
          {
              YYTRACE("variable_list: variable_list ',' identifier");
              $1->newEnd(new vl_var($3, 0));
              $$ = $1;
          }
        ;

register_variable_list
        : register_variable
          {
              YYTRACE("register_variable_list: register_variable");
              $$ = new lsList<vl_var*>;
              $$->newEnd($1);
          }
        | register_variable_list ',' register_variable
          {
              YYTRACE("register_variable_list: register_variable_list ',' "
                  "register_variable");
              $1->newEnd($3);
              $$ = $1;
          }
        ;
        
register_variable
        : name_of_register
          {
              YYTRACE("register_variable: name_of_register");
              $$ = new vl_var($1, 0);
          }
        | name_of_register '[' expression ':' expression ']'
          {
              YYTRACE("register_variable: name_of_register '[' expression "
                  "':' expression ']'");
              $$ = new vl_var($1, new vl_range($3, $5));
          }
        ;

name_of_register
        : id_token
          {
              YYTRACE("name_of_register: YYNAME");
              $$ = $1;
          }
        ;

name_of_event_list
        : name_of_event
          {
              YYTRACE("name_of_event_list: name_of_event");
              $$ = new lsList<char*>;
              $$->newEnd($1);
          }
        | name_of_event_list ',' name_of_event
          {
              YYTRACE("name_of_event_list: name_of_event_list ',' "
                  "name_of_event");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

name_of_event
        : id_token
          {
              YYTRACE("name_of_event: YYNAME");
              $$ = $1;
          }
        ;

charge_strength_opt
        :
          {
              YYTRACE("charge_strength_opt:");
              $$ = drive_strength(YYSMALL, YYSMALL);
              $$->str0 = STRnone;
              $$->str1 = STRnone;
          }
        | charge_strength
          {
              YYTRACE("charge_strength_opt: charge_strength");
              $$ = $1;
          }
        ;

charge_strength
        : '(' YYSMALL ')'
          {
              YYTRACE("charge_strength: '(' YYSMALL ')'");
              $$ = drive_strength(YYSMALL, 0);
          }
        | '(' YYMEDIUM ')'
          {
              YYTRACE("charge_strength: '(' YYMEDIUM ')'");
              $$ = drive_strength(YYMEDIUM, 0);
          }
        | '(' YYLARGE ')'
          {
              YYTRACE("charge_strength: '(' YYLARGE ')'");
              $$ = drive_strength(YYLARGE, 0);
          }
        ;

drive_strength_opt
        :
          {
              YYTRACE("drive_strength_opt:");
              $$ = drive_strength(YYSMALL, 0);
              $$->str0 = STRnone;
              $$->str1 = STRnone;
          }
        | drive_strength
          {
              YYTRACE("drive_strength_opt: drive_strength");
              $$ = $1;
          }
        ;

drive_strength
        : '(' strength0 ',' strength1 ')'
          {
              YYTRACE("drive_strength: '(' strength0 ',' strength1 ')'");
              $$ = drive_strength($2, $4);
          }
        | '(' strength1 ',' strength0 ')'
          {
              YYTRACE("drive_strength: '(' strength1 ',' strength0 ')'");
              $$ = drive_strength($4, $2);
          }
        ;

strength0
        : YYSUPPLY0
          {
              YYTRACE("strength0: YYSUPPLY0");
              $$ = YYSUPPLY0;
          }
        | YYSTRONG0
          {
              YYTRACE("strength0: YYSTRONG0");
              $$ = YYSTRONG0;
          }
        | YYPULL0
          {
              YYTRACE("strength0: YYPULL0");
              $$ = YYPULL0;
          }
        | YYWEAK0
          {
              YYTRACE("strength0: YYWEAK0");
              $$ = YYWEAK0;
          }
        | YYHIGHZ0
          {
              YYTRACE("strength0: YYHIGHZ0");
              $$ = YYHIGHZ0;
          }
        ;

strength1
        : YYSUPPLY1
          {
              YYTRACE("strength1: YYSUPPLY1");
              $$ = YYSUPPLY1;
          }
        | YYSTRONG1
          {
              YYTRACE("strength1: YYSTRONG1");
              $$ = YYSTRONG1;
          }
        | YYPULL1
          {
              YYTRACE("strength1: YYPULL1");
              $$ = YYPULL1;
          }
        | YYWEAK1
          {
              YYTRACE("strength1: YYWEAK1");
              $$ = YYWEAK1;
          }
        | YYHIGHZ1
          {
              YYTRACE("strength1: YYHIGHZ1");
              $$ = YYHIGHZ1;
          }
        ;

range_opt
        :
          {
              YYTRACE("range_opt:");
              $$ = 0;
          }
        | range
          {
              YYTRACE("range_opt: range");
              $$ = $1;
          }
        ;

range
        : '[' expression ':' expression ']'
          {
              YYTRACE("range: '[' expression ':' expression ']'");
              $$ = new vl_range($2, $4);
          }
        ;

assignment_list
        : assignment
          {
              YYTRACE("assignment_list: assignment");
              $$ = new lsList<vl_bassign_stmt*>;
              $$->newEnd($1);
          }
        | assignment_list ',' assignment
          {
              YYTRACE("assignment_list: assignment_list ',' assignment");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

id_assignment_list
        : id_assignment
          {
              YYTRACE("id_assignment_list: id_assignment");
              $$ = new lsList<vl_bassign_stmt*>;
              $$->newEnd($1);
          }
        | id_assignment_list ',' id_assignment
          {
              YYTRACE("id_assignment_list: id_assignment_list ',' "
                  "id_assignment");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

id_assignment
        : identifier '=' expression
          {
              YYTRACE("id_assignment: identifier '=' expression");
              vl_var *v = new vl_var($1, 0);
              $$ = new vl_bassign_stmt(BassignStmt, v, 0, 0, $3);
          }
        ;


gate_instantiation
        : gatetype drive_delay_clr gate_instance_list ';'
          {
              YYTRACE("gate_instantiation: gatetype drive_delay_clr "
                  "gate_instance_list ';'");
              $$ = VP.description->add_gate_inst($1, $2, $3);
              delete $2;
          }
        ;

drive_delay_clr
        :
          {
              $$ = new vl_dlstr;
          }
        | drive_delay_clr drive_strength
          {
              $1->strength = *$2;
              $$ = $1;
          }
        | drive_delay_clr delay
          {
              $1->delay = $2;
              $$ = $1;
          }
        ;

gatetype
        : YYAND
          {
              YYTRACE("gatetype: YYAND");
              $$ = AndGate;
          }
        | YYNAND
          {
              YYTRACE("gatetype: YYNAND");
              $$ = NandGate;
          }
        | YYOR
          {
              YYTRACE("gatetype: YYOR");
              $$ = OrGate;
          }
        | YYNOR
          {
              YYTRACE("gatetype: YYNOR");
              $$ = NorGate;
          }
        | YYXOR
          {
              YYTRACE("gatetype: YYXOR");
              $$ = XorGate;
          }
        | YYXNOR
          {
              YYTRACE("gatetype: YYXNOR");
              $$ = XnorGate;
          }
        | YYBUF
          {
              YYTRACE("gatetype: YYBUF");
              $$ = BufGate;
          }
        | YYBUFIF0
          {
              YYTRACE("gatetype: YYBIFIF0");
              $$ = Bufif0Gate;
          }
        | YYBUFIF1
          {
              YYTRACE("gatetype: YYBIFIF1");
              $$ = Bufif1Gate;
          }
        | YYNOT
          {
              YYTRACE("gatetype: YYNOT");
              $$ = NotGate;
          }
        | YYNOTIF0
          {
              YYTRACE("gatetype: YYNOTIF0");
              $$ = Notif0Gate;
          }
        | YYNOTIF1
          {
              YYTRACE("gatetype: YYNOTIF1");
              $$ = Notif1Gate;
          }
        | YYPULLDOWN
          {
              YYTRACE("gatetype: YYPULLDOWN");
              $$ = PulldownGate;
          }
        | YYPULLUP
          {
              YYTRACE("gatetype: YYPULLUP");
              $$ = PullupGate;
          }
        | YYNMOS
          {
              YYTRACE("gatetype: YYNMOS");
              $$ = NmosGate;
          }
        | YYPMOS
          {
              YYTRACE("gatetype: YYPMOS");
              $$ = PmosGate;
          }
        | YYRNMOS
          {
              YYTRACE("gatetype: YYRNMOS");
              $$ = RnmosGate;
          }
        | YYRPMOS
          {
              YYTRACE("gatetype: YYRPMOS");
              $$ = RpmosGate;
          }
        | YYCMOS
          {
              YYTRACE("gatetype: YYCMOS");
              $$ = CmosGate;
          }
        | YYRCMOS
          {
              YYTRACE("gatetype: YYRCMOS");
              $$ = RcmosGate;
          }
        | YYTRAN
          {
              YYTRACE("gatetype: YYTRAN");
              $$ = TranGate;
          }
        | YYRTRAN
          {
              YYTRACE("gatetype: YYRTRAN");
              $$ = RtranGate;
          }
        | YYTRANIF0
          {
              YYTRACE("gatetype: YYTRANIF0");
              $$ = Tranif0Gate;
          }
        | YYRTRANIF0
          {
              YYTRACE("gatetype: YYRTRANIF0");
              $$ = Rtranif0Gate;
          }
        | YYTRANIF1
          {
              YYTRACE("gatetype: YYTRANIF1");
              $$ = Tranif1Gate;
          }
        | YYRTRANIF1
          {
              YYTRACE("gatetype: YYRTRANIF1");
              $$ = Rtranif1Gate;
          }
        ;

gate_instance_list
        : gate_instance
          {
              YYTRACE("gate_instance_list: gate_instance");
              $$ = new lsList<vl_gate_inst*>;
              $$->newEnd($1);
          }
        | gate_instance_list ',' gate_instance
          {
              YYTRACE("gate_instance_list: gate_instance_list ',' "
                  "gate_instance");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

gate_instance
        : '(' terminal_list ')'
          {
              YYTRACE("gate_instance: '(' terminal_list ')'");
              $$ = new vl_gate_inst(0, $2, 0);
          }
        | name_of_gate_instance '(' terminal_list ')'
          {
              YYTRACE("gate_instance: name_of_gate_instance '(' "
                  "terminal_list ')'");
              $$ = new vl_gate_inst($1, $3, 0);
          }
        | name_of_gate_instance '[' expression ':' expression ']'
            '(' terminal_list ')'
          {
              YYTRACE("name_of_gate_instance: YYNAME '[' expression ':' "
                  "expression ']' '(' terminal_list ')'");
              $$ = new vl_gate_inst($1, $8, new vl_range($3, $5));
          }
        ;

name_of_gate_instance
        : id_token
          {
              YYTRACE("name_of_gate_instance: YYNAME");
              $$ = $1;
          }
        ;

terminal_list
        : terminal
          {
              YYTRACE("terminal_list: terminal");
              $$ = new lsList<vl_expr*>;
              $$->newEnd($1);
          }
        | terminal_list ',' terminal
          {
              YYTRACE("terminal_list: terminal_list ',' terminal");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

terminal
        : expression
          {
              YYTRACE("terminal: expression");
          }
        ;

module_or_primitive_instantiation
        : name_of_module_or_primitive module_or_primitive_option_clr
             module_or_primitive_instance_list ';'
          {
              YYTRACE("module_or_primitive_instantiation: "
                  "name_of_module_or_primitive module_or_primitive_option_clr "
                  "module_or_primitive_instance_list ';'");
              $$ = VP.description->add_mp_inst(VP.description, $1, $2, $3);
              delete $2;
          }
        ;

name_of_module_or_primitive
        : id_token
          {
              YYTRACE("name_of_module_or_primitive: YYNAME");
              $$ = $1;
          }
        ;

module_or_primitive_option_clr
        :
          {
              $$ = new vl_dlstr;
          }
        | module_or_primitive_option_clr drive_strength
          {
              $1->strength = *$2;
              $$ = $1;
          }
        | module_or_primitive_option_clr delay_or_parameter_value_assignment
          {
              $1->delay = $2;
              $$ = $1;
          }
        ;

delay_or_parameter_value_assignment
        : '#' YYINUMBER    
          {
              YYTRACE("delay_or_parameter_value_assignment: '#' YYINUMBER");
              vl_expr *expr = new vl_expr(IntExpr,
                  atoi(yy_textbuf()), 0.0, 0, 0, 0);
              $$ = new vl_delay(expr);
          }
        | '#' YYRNUMBER    
          {
              YYTRACE("delay_or_parameter_value_assignment: '#' YYRNUMBER");
              vl_expr *expr = new vl_expr(IntExpr, 0,
                  atof(yy_textbuf()), 0, 0, 0);
              $$ = new vl_delay(expr);
          }
        | '#' identifier   
          {
              YYTRACE("delay_or_parameter_value_assignment: '#' identifier");
              vl_expr *expr = new vl_expr(IDExpr, 0, 0.0, $2, 0, 0);
              $$ = new vl_delay(expr);
          }
        | '#' '(' mintypmax_expression_list ')' 
          {
              YYTRACE("delay_or_parameter_value_assignment: '#' '(' "
                  "mintypmax_expression_list ')'");
              $$ = new vl_delay($3);
          }
        ;

module_or_primitive_instance_list
        : module_or_primitive_instance
          {
              YYTRACE("module_or_primitive_instance_list: "
                  "module_or_primitive_instance");
              $$ = new lsList<vl_mp_inst*>;
              $$->newEnd($1);
          }
        | module_or_primitive_instance_list ',' module_or_primitive_instance
          {
              YYTRACE("module_or_primitive_instance_list: "
                  "module_or_primitive_instance_list ',' "
                  "module_or_primitive_instance");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

module_or_primitive_instance
        : '(' module_connection_list ')'
          {
              YYTRACE("module_or_primitive_instance: '(' "
                  "module_connection_list ')'");
              $$ = new vl_mp_inst(0, $2);
          }
        | identifier '(' module_connection_list ')'
          {
              YYTRACE("module_or_primitive_instance: '(' "
                  "module_connection_list ')'");
              $$ = new vl_mp_inst($1, $3);
          }
        ;

module_connection_list
        : module_port_connection_list
          {
              YYTRACE("module_connection_list: module_port_connection_list");
          }
        | named_port_connection_list
          {
              YYTRACE("module_connection_list: named_port_connection_list");
          }
        ;

module_port_connection_list
        : module_port_connection
          {
              YYTRACE("module_port_connection_list: module_port_connection");
              $$ = new lsList<vl_port_connect*>;
              $$->newEnd($1);
          }
        | module_port_connection_list ',' module_port_connection
          {
              YYTRACE("module_port_connection_list: "
                  "module_port_connection_list ',' module_port_connection");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

named_port_connection_list
        : named_port_connection
          {
              YYTRACE("named_port_connection_list: named_port_connection");
              $$ = new lsList<vl_port_connect*>;
              $$->newEnd($1);
          }
        | named_port_connection_list ',' named_port_connection
          {
              YYTRACE("named_port_connection_list: named_port_connection_list "
                  "',' name_port_connection");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

module_port_connection
        :
          {
              YYTRACE("module_port_connection:");
              $$ = new vl_port_connect(ModuleConnect, 0, 0);
          }
        | expression
          {
              YYTRACE("module_port_connection: expression");
              $$ = new vl_port_connect(ModuleConnect, 0, $1);
          }
        ;

named_port_connection
        : '.' identifier '(' ')'
          {
              YYTRACE("named_port_connection: '.' identifier '(' ')'");
              $$ = new vl_port_connect(NamedConnect, $2, 0);
          }
        | '.' identifier '(' expression ')'
          {
              YYTRACE("named_port_connection: '.' identifier '(' "
                  "expression ')'");
              $$ = new vl_port_connect(NamedConnect, $2, $4);
          }
        ;

initial_statement
        : YYINITIAL { } statement
          {
              YYTRACE("initial_statement: YYINITIAL statement");
              $$ = new vl_procstmt(InitialStmt, $3);
          }
        ;

always_statement
        : YYALWAYS statement
          {
              YYTRACE("always_statement: YYALWAYS statement");
              $$ = new vl_procstmt(AlwaysStmt, $2);
          }
        ;

statement_opt
        :
          {
              YYTRACE("statement_opt:");
              $$ = new lsList<vl_stmt*>;
          }
        | statement
          {
              YYTRACE("statement_opt: statement");
              $$ = new lsList<vl_stmt*>;
              $$->newEnd($1);
          }
        ;        

statement_clr
        :
          {
              YYTRACE("statement_clr:");
              $$ = new lsList<vl_stmt*>;
          }
        | statement_clr statement
          {
              YYTRACE("statement_clr: statement_clr statement");
              $1->newEnd($2);
              $$ = $1;
          }
        ;

statement
        : ';' 
          {
              YYTRACE("statement: ';'");
              $$ = 0;
          }
        | assignment ';'
          {
              YYTRACE("statement: assignment ';'");
          }
        | YYIF '(' expression ')' statement
          {
              YYTRACE("statement: YYIF '(' expression ')' statement");
              $$ = new vl_if_else_stmt($3, $5, 0);
          }
        | YYIF '(' expression ')' statement YYELSE statement
          {
              YYTRACE("statement: YYIF '(' expression ')' statement "
                  "YYELSE statement");   
              $$ = new vl_if_else_stmt($3, $5, $7);
          } 
        | YYCASE '(' expression ')' case_item_eclr YYENDCASE
          {
              YYTRACE("statement: YYCASE '(' expression ')' case_item_eclr "
                  "YYENDCASE");
              $$ = new vl_case_stmt(CaseStmt, $3, $5);
          }
        | YYCASEZ '(' expression ')' case_item_eclr YYENDCASE
          {
              YYTRACE("statement: YYCASEZ '(' expression ')' case_item_eclr "
                  "YYENDCASE"); 
              $$ = new vl_case_stmt(CasezStmt, $3, $5);
          }
        | YYCASEX '(' expression ')' case_item_eclr YYENDCASE
          {
              YYTRACE("statement: YYCASEX '(' expression ')' case_item_eclr "
                  "YYENDCASE");
              $$ = new vl_case_stmt(CasexStmt, $3, $5);
          }
        | YYFOREVER statement
          {
              YYTRACE("statement: YYFOREVER statement");
              $$ = new vl_forever_stmt($2);
          }
        | YYREPEAT '(' expression ')' statement
          {
              YYTRACE("statement: YYREPEAT '(' expression ')' statement");
              $$ = new vl_repeat_stmt($3, $5);
          } 
        | YYWHILE '(' expression ')' statement
          {
              YYTRACE("statement: YYWHILE '(' expression ')' statement");
              $$ = new vl_while_stmt($3, $5);
          }
        | YYFOR '(' assignment ';' expression ';' assignment ')' statement
          {
              YYTRACE("statement: YYFOR '(' assignment ';' expression ';' "
                  "assignment ')' statement");
              $$ = new vl_for_stmt($3, $5, $7, $9);
          }
        | delay_control statement
          {
              YYTRACE("statement: delay_control statement");
              $$ = new vl_delay_control_stmt($1, $2);
          }
        | event_control statement
          {
              YYTRACE("statement: event_control statement");
              $$ = new vl_event_control_stmt($1, $2);
          }
        | lvalue '=' delay_control expression ';'
          {
              YYTRACE("statement: lvalue '=' delay_control expression ';'");
              $$ = new vl_bassign_stmt(DelayBassignStmt, $1, 0, $3, $4);
          }
        | lvalue '=' repeated_event_control expression ';'
          {
              YYTRACE("statement: lvalue '=' repeated_event_control "
                  "expression ';'");
              $$ = new vl_bassign_stmt(EventBassignStmt, $1, $3, 0, $4);
          }
        | lvalue YYNBASSIGN delay_control expression ';'
          {
              YYTRACE("statement: lvalue YYNBASSIGN delay_control "
                  "expression ';'");
              $$ = new vl_bassign_stmt(DelayNbassignStmt, $1, 0, $3, $4);
          }
        | lvalue YYNBASSIGN event_control expression ';'
          {
              YYTRACE("statement: lvalue YYNBASSIGN event_control "
                  "expression ';'");
              $$ = new vl_bassign_stmt(EventNbassignStmt, $1, $3, 0, $4);
          }
        | YYWAIT '(' expression ')' statement
          {
              YYTRACE("statement: YYWAIT '(' expression ')' statement");
              $$ = new vl_wait_stmt($3, $5);
          }
        | YYRIGHTARROW name_of_event ';'
          {
              YYTRACE("statement: YYRIGHTARROW name_of_event ';'");
              $$ = new vl_send_event_stmt($2);
          }
        | seq_block
          {
              YYTRACE("statement: seq_block");
          }
        | par_block
          {
              YYTRACE("statement: par_block");
          }
        | task_enable
          {
              YYTRACE("statement: task_enable");
          }
        | system_task_enable
          {
              YYTRACE("statement: system_task_enable");
          }
        | YYDISABLE identifier ';'  
          {
              YYTRACE("statement: YYDISABLE identifier ';'");
              $$ = new vl_disable_stmt($2);
          }
        | YYASSIGN assignment ';'
          {
              YYTRACE("statement: YYASSIGN assignment ';'");
              $2->type = AssignStmt;
              $$ = $2;
          }
        | YYDEASSIGN lvalue ';'
          {
              YYTRACE("statement: YYDEASSIGN lvalue ';'");
              $$ = new vl_deassign_stmt(DeassignStmt, $2);
          }
        | YYFORCE assignment ';'
          {
              YYTRACE("statement: YYFORCE assignment ';'");
              $2->type = ForceStmt;
              $$ = $2;
          }
        | YYRELEASE lvalue ';'
          {
              YYTRACE("statement: YYRELEASE lvalue ';'");
              $$ = new vl_deassign_stmt(ReleaseStmt, $2);
          }
        ;

assignment
        : lvalue '=' expression
          {
              YYTRACE("assignment: lvalue '=' expression");
              $$ = new vl_bassign_stmt(BassignStmt, $1, 0, 0, $3);
          }
        | lvalue YYNBASSIGN expression
          {
              YYTRACE("assignment: lvalue YYNBASSIGN expression");
              $$ = new vl_bassign_stmt(NbassignStmt, $1, 0, 0, $3);
          }
        ;

case_item_eclr
        : case_item
          {
              YYTRACE("case_item_eclr: case_item");
              $$ = new lsList<vl_case_item*>;
              $$->newEnd($1);
          }
        | case_item_eclr case_item
          {
              YYTRACE("case_item_eclr: case_item_eclr case_item");
              $1->newEnd($2);
              $$ = $1;
          }
        ;

case_item
        : expression_list ':' statement
          {
              YYTRACE("case_item: expression_list ':' statement");
              $$ = new vl_case_item(CaseItem, $1, $3);
          }
        | YYDEFAULT ':' statement
          {
              YYTRACE("case_item: YYDEFAULT ':' statement");
              $$ = new vl_case_item(DefaultItem, 0, $3);
          }
        | YYDEFAULT statement
          {
              YYTRACE("case_item: YYDEFAULT statement");
              $$ = new vl_case_item(DefaultItem, 0, $2);
          }
        ;

seq_block
        : YYBEGIN statement_clr YYEND
          {
              YYTRACE("seq_block: YYBEGIN statement_clr YYEND");
              $$ = new vl_begin_end_stmt(0, 0, $2);
          }
        | YYBEGIN ':' name_of_block block_declaration_clr statement_clr YYEND
          {
              YYTRACE("seq_block: YYBEGIN ':' name_of_block "
                  "block_declaration_clr statement_clr YYEND");
              $$ = new vl_begin_end_stmt($3, $4, $5);
          }
        ;

par_block
        : YYFORK statement_clr YYJOIN
          {
              YYTRACE("par_block: YYFORK statement_clr YYJOIN");
              $$ = new vl_fork_join_stmt(0, 0, $2);
          }
        | YYFORK ':' name_of_block block_declaration_clr statement_clr YYJOIN
          {
              YYTRACE("par_block: YYFORK ':' name_of_block "
                  "block_declaration_clr statement_clr YYJOIN");
              $$ = new vl_fork_join_stmt($3, $4, $5);
          }
        ;

name_of_block
        : id_token
          {
              YYTRACE("name_of_block: YYNAME");
              $$ = $1;
          }
        ;

block_declaration_clr
        :
          {
              YYTRACE("block_declaration_clr:");
              $$ = new lsList<vl_decl*>;
          }
        | block_declaration_clr block_declaration
          {
              YYTRACE("block_declaration_clr: block_declaration_clr "
                  "block_declaration");
              $1->newEnd($2);
              $$ = $1;
          }
        ;

block_declaration
        : parameter_declaration
          {
              YYTRACE("block_declaration: parameter_declaration");
              $$ = $1;
          }
        | reg_declaration
          {
              YYTRACE("block_declaration: reg_declaration");
              $$ = $1;
          }
        | integer_declaration
          {
              YYTRACE("block_declaration: integer_declaration");
              $$ = $1;
          }
        | real_declaration
          {
              YYTRACE("block_declaration: real_declaration");
              $$ = $1;
          }
        | time_declaration
          {
              YYTRACE("block_delcaration: time_declaration");
              $$ = $1;
          }
        | event_declaration
          {
              YYTRACE("block_declaration: event_declaration");
              $$ = $1;
          }
        ;

task_enable
        : name_of_identifier ';'
          {
              YYTRACE("task_enable: identifier ';'");
              $$ = new vl_task_enable_stmt(TaskEnableStmt, $1, 0);
          }
        | name_of_identifier '(' expression_list ')' ';'
          {
              YYTRACE("task_enable: identifier '(' expression_list ')' ';'");
              $$ = new vl_task_enable_stmt(TaskEnableStmt, $1, $3);
          }
        ;

name_of_identifier
        : identifier
          {
              YYTRACE("name_of_identifier: identifier");
          }
        ;

system_task_enable
        : name_of_system_task ';'
          {
              YYTRACE("system_task_enable: name_of_system_task ';'");
              $$ = new vl_sys_task_stmt($1, 0);
          }
        | name_of_system_task '(' systask_args ')' ';'
          {
              YYTRACE("system_task_enable: name_of_system_task '(' "
                  "systask_args ')'");
              $$ = new vl_sys_task_stmt($1, $3);
          }
        ;

name_of_system_task
        : system_identifier
          {
              YYTRACE("name_of_system_task: system_identifier");
          }
        ;

specify_block
        : YYSPECIFY specify_item_clr YYENDSPECIFY
          {
              YYTRACE("specify_block: YYSPECIFY specify_item_clr "
                  "YYENDSPECIFY");
              $$ = new vl_specify_block($2);
          } 
        ;

specify_item_clr
        :
          {
              YYTRACE("specify_item_clr:");
              $$ = new lsList<vl_specify_item*>;
          }
        | specify_item_clr specify_item
          {
              YYTRACE("specify_item_clr: specify_item_clr specify_item");
              $1->newEnd($2);
              $$ = $1;
          }
        ;

specify_item
        : specparam_declaration
          {
              YYTRACE("specify_item: specparm_declaration");
              $$ = $1;
          }
        | path_declaration
          {
              YYTRACE("specify_item: path_declaration");
              $$ = $1;
          }
        | level_sensitive_path_declaration
          {
              YYTRACE("specify_item: level_sensitive_path_declaration");
              $$ = $1;
          }
        | edge_sensitive_path_declaration
          {
              YYTRACE("specify_item: edge_sensitive_path_declaration");
              $$ = $1;
          }
        | system_timing_check
          {
              YYTRACE("specify_item: system_timing_check");
              $$ = $1;
          }
        ;

specparam_declaration
        : YYSPECPARAM assignment_list ';'
          {
              YYTRACE("specparam_declaration: YYSPECPARAM assignment_list ';'");
              $$ = new vl_specify_item(SpecParamDecl, $2);
          }
        ;

path_declaration
        : path_description '=' path_delay_value ';'
          {
              YYTRACE("path_declaration: path_description '=' "
                  "path_delay_value ';'");
              $$ = new vl_specify_item(SpecPathDecl, $1, $3);
          }
        ;

path_description
        : '(' specify_terminal_descriptor YYLEADTO specify_terminal_descriptor
          ')'
          {
              YYTRACE("path_description: '(' specify_terminal_descriptor "
                  "YYLEADTO specify_terminal_descriptor ')'");
              $$ = new vl_path_desc($2, $4);
          }
        | '(' path_list YYALLPATH path_list_or_edge_sensitive_path_list ')'
          {
              YYTRACE("path_description: '(' path_list YYALLPATH "
                  "path_list_or_edge_sensitive_path_list ')'");
              $$ = new vl_path_desc($2, $4);
          }
        ;

path_list
        : specify_terminal_descriptor
          {
              YYTRACE("path_list: specify_terminal_descriptor");
              $$ = new lsList<vl_spec_term_desc*>;
              $$->newEnd($1);
          }
        | path_list ',' specify_terminal_descriptor
          {
              YYTRACE("path_list: path_list ',' specify_terminal_descriptor");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

specify_terminal_descriptor
        : identifier
          {
              YYTRACE("specify_terminal_descriptor: YYNAME");
              $$ = new vl_spec_term_desc(vl_strdup($1), 0, 0);
          }
        | identifier '[' expression ']'
          {
              YYTRACE("specify_terminal_descriptor: YYNAME '[' expression ']'");
              $$ = new vl_spec_term_desc(vl_strdup($1), $3, 0);
          }
        | identifier '[' expression ':' expression ']'
          {
              YYTRACE("specify_terminal_descriptor: YYNAME '[' expression ':' "
                  "expression ']'");
              $$ = new vl_spec_term_desc(vl_strdup($1), $3, $5);
          }
        ;

path_list_or_edge_sensitive_path_list
        : path_list
          {
              YYTRACE("path_list_or_edge_sensitive_path_list: path_list");
              $$ = $1;
          }
        | '(' path_list ',' specify_terminal_descriptor
              polarity_operator YYCONDITIONAL 
              expression ')'
          {
              YYTRACE("path_list_or_edge_sensitive_path_list: '(' path_list "
                  "',' specify_terminal_descriptor polarity_operator "
                  "YYCONDITIONAL expression ')'");
              $$ = $2;
              $$->newEnd($4);
              $$->newEnd(new vl_spec_term_desc($5, $7));
          }
        ;

path_delay_value
        : path_delay_expression
          {
              YYTRACE("path_delay_value: path_delay_expression");
              $$ = new lsList<vl_expr*>;
              $$->newEnd($1);
          }
        | '(' path_delay_expression ',' path_delay_expression ')'
          {
              YYTRACE("path_delay_value: '(' path_delay_expression ',' "
                  "path_delay_expression ')'");
              $$ = new lsList<vl_expr*>;
              $$->newEnd($2);
              $$->newEnd($4);
          }
        | '(' path_delay_expression ',' path_delay_expression ',' 
              path_delay_expression ')'
          {
              YYTRACE("path_delay_value: '(' path_delay_expression ',' "
                  "path_delay_expression ',' path_delay_expression ')'");
              $$ = new lsList<vl_expr*>;
              $$->newEnd($2);
              $$->newEnd($4);
              $$->newEnd($6);
          }
        | '(' path_delay_expression ',' path_delay_expression ','
              path_delay_expression ',' path_delay_expression ','
              path_delay_expression ',' path_delay_expression ')'
          {
              YYTRACE("path_delay_value: '(' path_delay_expression ',' "
                  "path_delay_expression ',' path_delay_expression ',' "
                  "path_delay_expression ',' path_delay_expression ',' "
                  "path_delay_expression ')'");
              $$ = new lsList<vl_expr*>;
              $$->newEnd($2);
              $$->newEnd($4);
              $$->newEnd($6);
              $$->newEnd($8);
              $$->newEnd($10);
              $$->newEnd($12);
          }
        ;

path_delay_expression
        : expression
          {
              YYTRACE("path_delay_expression: expression");
              $$ = $1;
          }
        ;

system_timing_check
        : YYsysSETUP '(' ')' ';'
          {
              YYTRACE("system_timing_check: YYsysSetup '(' ')' ';'");
              $$ = new vl_specify_item(SpecTiming);
          }
        ;

level_sensitive_path_declaration
        : YYIF '(' expression ')'
            '(' specify_terminal_descriptor polarity_operator_opt YYLEADTO
                specify_terminal_descriptor ')' '=' path_delay_value ';'
          {
              YYTRACE("level_sensitive_path_declaration: YYIF '(' expression "
                  "')' '(' specify_terminal_descriptor polarity_operator_opt "
                  "YYLEADTO specify_terminal_descriptor" "')' "
                  "'=' path_delay_value ';'");
              $$ = new vl_specify_item(SpecLSPathDecl1, $3,
                  new lsList<vl_spec_term_desc*>, $7,
                  new lsList<vl_spec_term_desc*>, $12);
              $$->list1->newEnd($6);
              $$->list2->newEnd($9);
          }
        | YYIF '(' expression ')'
            '(' path_list ',' specify_terminal_descriptor 
                polarity_operator_opt YYALLPATH path_list ')'
                '=' path_delay_value ';'
          {
              YYTRACE("level_sensitive_path_declaration: YYIF '(' expression "
                  "')' '(' path_list ',' specify_terminal_descriptor "
                  "polarity_operator_opt YYALLPATH path_list ')' "
                  "'=' path_delay_value ';'");
              $$ = new vl_specify_item(SpecLSPathDecl2, $3, $6, $9, $11, $14);
              $6->newEnd($8);
          }
        ;

polarity_operator_opt
        : {
              $$ = 0;
          }
        | polarity_operator
          {
              YYTRACE("polarity_operator_opt: polarity_operator");
              $$ = $1;
          }
        ;

polarity_operator
        : '+' { $$ = '+'; }
        | '-' { $$ = '-'; }
        ;

edge_sensitive_path_declaration
        : '(' specify_terminal_descriptor YYLEADTO
            '(' specify_terminal_descriptor polarity_operator YYCONDITIONAL
                expression ')' ')' '=' path_delay_value ';'
          {
              YYTRACE("edge_sensitive_path_declaration: "
                  "'(' specify_terminal_descriptor polarity_operator "
                  "YYCONDITIONAL expression ')' ')' '=' path_delay_value ';'");
              $$ = new vl_specify_item(SpecESPathDecl1, 0, 0,
                   new lsList<vl_spec_term_desc*>,
                   new lsList<vl_spec_term_desc*>, $6, $8, $12);
              $$->list1->newEnd($2);
              $$->list2->newEnd($5);
          }
        | '(' edge_identifier specify_terminal_descriptor YYLEADTO
            '(' specify_terminal_descriptor polarity_operator YYCONDITIONAL
                expression ')' ')' '=' path_delay_value ';'
          {
              YYTRACE("edge_sensitive_path_declaration: "
                  "'(' edge_identifier specify_terminal_descriptor YYLEADTO "
                  "'(' specify_terminal_descriptor polarity_operator "
                  "YYCONDITIONAL expression ')' ')' '=' path_delay_value ';'");
              $$ = new vl_specify_item(SpecESPathDecl1, 0, $2,
                  new lsList<vl_spec_term_desc*>,
                  new lsList<vl_spec_term_desc*>, $7, $9, $13);
              $$->list1->newEnd($3);
              $$->list2->newEnd($6);
          }
        | YYIF '(' expression ')'
            '(' edge_identifier specify_terminal_descriptor YYLEADTO
              '(' specify_terminal_descriptor polarity_operator YYCONDITIONAL
                  expression ')' ')' '=' path_delay_value ';'
          {
              YYTRACE("edge_sensitive_path_declaration: "
                  "YYIF '(' expression ')' "
                  "'(' edge_identifier specify_terminal_descriptor YYLEADTO "
                  "'(' specify_terminal_descriptor polarity_operator "
                  "YYCONDITIONAL expression ')' ')' '=' path_delay_value ';'");
              $$ = new vl_specify_item(SpecESPathDecl1, $3, $6,
                  new lsList<vl_spec_term_desc*>,
                  new lsList<vl_spec_term_desc*>, $11, $13, $17);
              $$->list1->newEnd($7);
              $$->list2->newEnd($10);
          }
        | '(' edge_identifier specify_terminal_descriptor YYALLPATH
            '(' path_list ',' specify_terminal_descriptor
                polarity_operator YYCONDITIONAL 
                expression ')' ')' '=' path_delay_value ';'
          {
              YYTRACE("edge_sensitive_path_declaration: "
                  "'(' edge_identifier specify_terminal_descriptor YYALLPATH "
                  "'(' path_list ',' specify_terminal_descriptor "
                  "polarity_operator YYCONDITIONAL "
                  "expression ')' ')' '=' path_delay_value ';'");
              $$ = new vl_specify_item(SpecESPathDecl2, 0, $2,
                  new lsList<vl_spec_term_desc*>, $6, $9, $11, $15);
              $$->list1->newEnd($3);
              $$->list2->newEnd($8);
          }
        | YYIF '(' expression ')'
            '(' specify_terminal_descriptor YYALLPATH
            '(' path_list ',' specify_terminal_descriptor
                polarity_operator YYCONDITIONAL 
                expression ')' ')' '=' path_delay_value ';'
          {
              YYTRACE("edge_sensitive_path_declaration: "
                  "YYIF '(' expression ')' "
                  "'(' specify_terminal_descriptor YYALLPATH "
                  "'(' path_list ',' specify_terminal_descriptor "
                  "polarity_operator YYCONDITIONAL "
                  "expression ')' ')' '=' path_delay_value ';'");
              $$ = new vl_specify_item(SpecESPathDecl2, $3, 0,
                  new lsList<vl_spec_term_desc*>, $9, $12, $14, $18);
              $$->list1->newEnd($6);
              $$->list2->newEnd($11);
          }
        | YYIF '(' expression ')'
            '(' edge_identifier specify_terminal_descriptor YYALLPATH
            '(' path_list ',' specify_terminal_descriptor
                polarity_operator YYCONDITIONAL 
                expression ')' ')' '=' path_delay_value ';'
          {
              YYTRACE("edge_sensitive_path_declaration: "
                  "YYIF '(' expression ')' "
                  "'(' edge_identifier specify_terminal_descriptor YYALLPATH "
                  "'(' path_list ',' specify_terminal_descriptor "
                  "polarity_operator YYCONDITIONAL "
                  "expression ')' ')' '=' path_delay_value ';'");
              $$ = new vl_specify_item(SpecESPathDecl2, $3, $6,
                  new lsList<vl_spec_term_desc*>, $10, $13, $15, $19);
              $$->list1->newEnd($7);
              $$->list2->newEnd($12);
          }
        ;

edge_identifier
        : YYPOSEDGE { $$ = PosedgeEventExpr; }
        | YYNEGEDGE { $$ = NegedgeEventExpr; }
        ;

lvalue
        : identifier
          {
              YYTRACE("lvalue: identifier");
              $$ = new vl_var($1, 0);
          }
        | identifier '[' expression ']'
          {
              YYTRACE("lvalue: identifier '[' expression ']'");
              $$ = new vl_var($1, new vl_range($3, 0));
          }
        | identifier '[' expression ':' expression ']'
          {
              YYTRACE("lvalue: identifier'[' expression ':' expression ']'");
              $$ = new vl_var($1, new vl_range($3, $5));
          }
        | concatenation
          {
              YYTRACE("lvalue: concatenation");
              $$ = new vl_var(0, 0, $1);
          }
        ;

mintypmax_expression_list
        : mintypmax_expression
          {
              YYTRACE("mintypmax_expression_list: mintypmax_expression");
              $$ = new lsList<vl_expr*>;
              $$->newEnd($1);
          }
        | mintypmax_expression_list ',' mintypmax_expression
          {
              YYTRACE("mintypmax_expression_list: mintypmax_expression_list "
                  "',' mintypmax_expression");
              $1->newEnd($3);
              $$ = $1;
          }
        ;

mintypmax_expression
        : expression
          {
              YYTRACE("mintypmax_expression: expression");
              $$ = new vl_expr(MinTypMaxExpr, 0, 0.0, $1, 0, 0);
          }
        | expression ':' expression
          {
              YYTRACE("mintypmax_expression: expression ':' expression");
              $$ = new vl_expr(MinTypMaxExpr, 0, 0.0, $1, $3, 0);
          }
        | expression ':' expression ':' expression
          {
              YYTRACE("mintypmax_expression: expression ':' expression ':' "
                  "expression");
              $$ = new vl_expr(MinTypMaxExpr, 0, 0.0, $1, $3, $5);
          }
        ;

expression_list
        : expression
          {
              YYTRACE("expression_list: expression");
              $$ = new lsList<vl_expr*>;
              $$->newEnd($1);
          }
        | expression_list ',' expression
          {
              YYTRACE("expression_list: expression_list ',' expression");
              $$ = new lsList<vl_expr*>;
              $1->newEnd($3);
              $$ = $1;
          }
        ;

expression
        : primary
          {
              YYTRACE("expression: primary");
          }
        | '+' primary %prec YYUNARYOPERATOR
          {
              YYTRACE("expression: '+' primary %prec YYUNARYOPERATOR");
              $$ = new vl_expr(UplusExpr, 0, 0.0, $2, 0, 0);
          } 
        | '-' primary %prec YYUNARYOPERATOR
          {
              YYTRACE("expression: '-' primary %prec YYUNARYOPERATOR");
              $$ = new vl_expr(UminusExpr, 0, 0.0, $2, 0, 0);
          }
        | '!' primary %prec YYUNARYOPERATOR
          {
              YYTRACE("expression: '!' primary %prec YYUNARYOPERATOR");
              $$ = new vl_expr(UnotExpr, 0, 0.0, $2, 0, 0);
          }
        | '~' primary %prec YYUNARYOPERATOR
          {
              YYTRACE("expression: '~' primary %prec YYUNARYOPERATOR");
              $$ = new vl_expr(UcomplExpr, 0, 0.0, $2, 0, 0);
          }
        | '&' primary %prec YYUNARYOPERATOR
          {
              YYTRACE("expression: '&' primary %prec YYUNARYOPERATOR");
              $$ = new vl_expr(UandExpr, 0, 0.0, $2, 0, 0);
          }
        | '|' primary %prec YYUNARYOPERATOR
          {
              YYTRACE("expression: '|' primary %prec YYUNARYOPERATOR");
              $$ = new vl_expr(UorExpr, 0, 0.0, $2, 0, 0);
          }
        | '^' primary %prec YYUNARYOPERATOR
          {
              YYTRACE("expression: '^' primary %prec YYUNARYOPERATOR");
              $$ = new vl_expr(UxorExpr, 0, 0.0, $2, 0, 0);
          }
        | YYLOGNAND primary %prec YYUNARYOPERATOR
          {
              YYTRACE("expression: YYLOGNAND primary %prec YYUNARYOPERATOR");
              $$ = new vl_expr(UnandExpr, 0, 0.0, $2, 0, 0);
          }
        | YYLOGNOR primary %prec YYUNARYOPERATOR
          {
              YYTRACE("expression: YYLOGNOR primary %prec YYUNARYOPERATOR");
              $$ = new vl_expr(UnorExpr, 0, 0.0, $2, 0, 0);
          }
        | YYLOGXNOR primary %prec YYUNARYOPERATOR
          {
              YYTRACE("expression: YYLOGXNOR primary %prec YYUNARYOPERATOR");
              $$ = new vl_expr(UxnorExpr, 0, 0.0, $2, 0, 0);
          }
        | expression '+' expression
          {
              YYTRACE("expression: expression '+' expression");
              $$ = new vl_expr(BplusExpr, 0, 0.0, $1, $3, 0);
          }
        | expression '-' expression
          {
              YYTRACE("expression: expressio '-' expression");
              $$ = new vl_expr(BminusExpr, 0, 0.0, $1, $3, 0);
          }
        | expression '*' expression
          {
              YYTRACE("expression: expression '*' expression");
              $$ = new vl_expr(BtimesExpr, 0, 0.0, $1, $3, 0);
          }
        | expression '/' expression
          {
              YYTRACE("expression: expression '/' expression");
              $$ = new vl_expr(BdivExpr, 0, 0.0, $1, $3, 0);
          }
        | expression '%' expression
          {
              YYTRACE("expression: expression '%' expression");
              $$ = new vl_expr(BremExpr, 0, 0.0, $1, $3, 0);
          }
        | expression YYLOGEQUALITY expression
          {
              YYTRACE("expression: expression YYLOGEQUALITY expression");
              $$ = new vl_expr(Beq2Expr, 0, 0.0, $1, $3, 0);
          }
        | expression YYLOGINEQUALITY expression
          {
              YYTRACE("expression: expression YYLOGINEQUALITY expression");
              $$ = new vl_expr(Bneq2Expr, 0, 0.0, $1, $3, 0);
          }
        | expression YYCASEEQUALITY expression
          {
              YYTRACE("expression: expression YYCASEEQUALITY expression");
              $$ = new vl_expr(Beq3Expr, 0, 0.0, $1, $3, 0);
          }
        | expression YYCASEINEQUALITY expression
          {
              YYTRACE("expression: expression YYCASEINEQUALITY expression");
              $$ = new vl_expr(Bneq3Expr, 0, 0.0, $1, $3, 0);
          }
        | expression YYLOGAND expression
          {
              YYTRACE("expression: expression YYLOGAND expression");
              $$ = new vl_expr(BlandExpr, 0, 0.0, $1, $3, 0);
          }
        | expression YYLOGOR expression
          {
              YYTRACE("expression: expression YYLOGOR expression");
              $$ = new vl_expr(BlorExpr, 0, 0.0, $1, $3, 0);
          }
        | expression '<' expression
          {
              YYTRACE("expression: expression '<' expression");
              $$ = new vl_expr(BltExpr, 0, 0.0, $1, $3, 0);
          }
        | expression '>' expression
          {
              YYTRACE("expression: expression '>' expression");
              $$ = new vl_expr(BgtExpr, 0, 0.0, $1, $3, 0);
          }
        | expression '&' expression
          {
              YYTRACE("expression: expression '&' expression");
              $$ = new vl_expr(BandExpr, 0, 0.0, $1, $3, 0);
          }
        | expression '|' expression
          {
              YYTRACE("expression: expression '|' expression");
              $$ = new vl_expr(BorExpr, 0, 0.0, $1, $3, 0);
          }
        | expression '^' expression
          {
              YYTRACE("expression: expression '^' expression");
              $$ = new vl_expr(BxorExpr, 0, 0.0, $1, $3, 0);
          }
        | expression YYLEQ expression
          {
              YYTRACE("expression: expression YYLEQ expression");
              $$ = new vl_expr(BleExpr, 0, 0.0, $1, $3, 0);
          }
        | expression YYNBASSIGN expression
          {
              YYTRACE("expression: expression YYLEQ expression");
              $$ = new vl_expr(BleExpr, 0, 0.0, $1, $3, 0);
          }
        | expression YYGEQ expression
          {
              YYTRACE("expression: expression YYGEQ expression");
              $$ = new vl_expr(BgeExpr, 0, 0.0, $1, $3, 0);
          }
        | expression YYLSHIFT expression
          {
              YYTRACE("expression: expression YYLSHIFT expression");
              $$ = new vl_expr(BlshiftExpr, 0, 0.0, $1, $3, 0);
          }
        | expression YYRSHIFT expression
          {
              YYTRACE("expression: expression YYRSHIFT expression");
              $$ = new vl_expr(BrshiftExpr, 0, 0.0, $1, $3, 0);
          }
        | expression YYLOGXNOR expression
          {
              YYTRACE("expression: expression YYLOGXNOR expression");
              $$ = new vl_expr(BxnorExpr, 0, 0.0, $1, $3, 0);
          }
        | expression '?' expression ':' expression
          {
              YYTRACE("expression: expression '?' expression ':' expression");
              $$ = new vl_expr(TcondExpr, 0, 0.0, $1, $3, $5);
          }
        | YYSTRING
          {
              YYTRACE("expression: YYSTRING");
              $$ = new vl_expr(StringExpr, 0, 0.0,
                  vl_strdup(yy_textbuf()), 0, 0);
          }
        ;

primary
        : YYINUMBER
          {
              YYTRACE("primary: YYINUMBER");
              if (strchr(yy_textbuf(), '\'')) {
                  $$ = new vl_expr(BitExpr, 0, 0.0, &yybit, 0, 0);
              }
              else {
                  $$ = new vl_expr(IntExpr, atoi(yy_textbuf()), 0.0, 0, 0, 0);
              }
          }
        | YYRNUMBER
          {
              YYTRACE("primary: YYRNUMBER");
              $$ = new vl_expr(RealExpr, 0, atof(yy_textbuf()), 0, 0, 0);
          }
        | identifier
          {
              YYTRACE("primary: identifier");
              $$ = new vl_expr(IDExpr, 0, 0.0, $1, 0, 0);
          }
        | identifier '[' expression ']'
          {
              YYTRACE("primary: identifier '[' expression ']'");
              $$ = new vl_expr(BitSelExpr, 0, 0.0, $1,
                  new vl_range($3, 0), 0);
          }
        | identifier '[' expression ':'  expression ']'
          {
              YYTRACE("primary: identifier '[' expression ':' expression ']'");
              $$ = new vl_expr(BitSelExpr, 0, 0.0, $1,
                  new vl_range($3, $5), 0);
          }
        | concatenation
          {
              YYTRACE("primary: concatenation");
              $$ = new vl_expr(ConcatExpr, 0, 0.0, $1, 0, 0);
          }
        | multiple_concatenation
          {
              YYTRACE("primary: multiple_concatenation");
              $$ = new vl_expr(ConcatExpr, 0, 0.0, $1->concat, $1->rep, 0);
              delete $1;
          }
        | function_call
          {
              YYTRACE("primary: function_call");
          }
        | '(' mintypmax_expression ')'
          {
              YYTRACE("primary: '(' mintypmax_expression ')'");
              $$ = $2;
          }
          
        | system_identifier
          {
              YYTRACE("primary: system_identifier");
              $$ = new vl_expr(SysExpr, 0, 0.0, 0, $1, 0);
          }
        | system_identifier '(' systask_args ')'
          {
              YYTRACE("primary: system_identifier '(' systask_args ')'");
              $$ = new vl_expr(SysExpr, 0, 0.0, $3, $1, 0);
          }
        ;

systask_args
        : expression
          {
              YYTRACE("systask_args : expression");
              $$ = new lsList<vl_expr*>;
              $$->newEnd($1);
          }
        | systask_args ','
          {
          }

        | systask_args ',' expression
          {
              YYTRACE("systask_args : systask_args ',' expression");
              $$->newEnd($3);
          }
        ;

concatenation
        : '{' expression_list '}'
          {
              YYTRACE("concatenation: '{' expression_list '}'");
              $$ = $2;
          }
        ;

multiple_concatenation
        : '{' expression '{' expression_list '}' '}'
            {
                YYTRACE("multiple_concatenation: '{' expression '{' "
                    "expression_list '}' '}'");
                $$ = new multi_concat($2, $4);
            }
        ;

function_call
        : identifier '(' expression_list ')'
          {
              YYTRACE("function_call: identifier '(' expression_list ')'");
              $$ = new vl_expr(FuncExpr, 0, 0.0, $1, $3, 0);
          }
        | identifier '(' ')'
          {
              YYTRACE("function_call: identifier '(' ')'");
              $$ = new vl_expr(FuncExpr, 0, 0.0, $1, 0, 0);
          }
        ;

system_identifier
        : YYsysID
          {
              YYTRACE("system identifier: YYsysID");
              $$ = vl_strdup(yyid);
          }
        ;

identifier
        : id_token
          {
              YYTRACE("identifier: id_token");
              $$ = $1;
          }
        | identifier '.' id_token
          {
              YYTRACE("identifier: identifier '.' id_token");
              $$ = new char[strlen($1) + strlen(yyid) + 3];
              strcpy($$, $1);
              strcat($$, ".");
              strcat($$, yyid);
              if (*$$ == '\\')
                  strcat($$, " ");
          }
        ;

id_token
        : YYNAME
          {
              YYTRACE("id_token: YYNAME");
              if (*yyid == '\\') {
                  $$ = new char[strlen(yyid) + 2];
                  strcpy($$, yyid);
                  strcat($$, " ");
              }
              else
                  $$ = vl_strdup(yyid);
          }
        ;

delay_opt
        :
          {
              YYTRACE("delay_opt:");
              $$ = 0;
          }
        | delay
          {
              YYTRACE("delay_opt: delay");
          }
        ;

delay
        : '#' YYINUMBER
          {
              YYTRACE("delay: '#' YYINUMBER");
              vl_expr *expr = new vl_expr(IntExpr,
                  atoi(yy_textbuf()), 0.0, 0, 0, 0);
              $$ = new vl_delay(expr);
          }
        | '#' YYRNUMBER
          {
              YYTRACE("delay: '#' YYRNUMBER");
              vl_expr *expr = new vl_expr(RealExpr, 0,
                  atof(yy_textbuf()), 0, 0, 0);
              $$ = new vl_delay(expr);
          }  
        | '#' identifier
          {
              YYTRACE("delay: '#' identifier");
              vl_expr *expr = new vl_expr(IDExpr, 0, 0.0, $2, 0, 0);
              $$ = new vl_delay(expr);
          }
        | '#' '(' mintypmax_expression_list ')'
          {
              YYTRACE("delay_control: '#' '(' mintypmax_expression_list ')'");
              $$ = new vl_delay($3);
          }
        ;

delay_control
        : delay
          {
              $$ = $1;
          }
        ;

repeated_event_control
        : event_control
          {
              $$ = $1;
          }
        | YYREPEAT '(' expression ')' event_control
          {
              YYTRACE("repeated_event_control: YYREPEAT '(' expression ')' "
                  "event_control");
              $$ = $5;
              $$->repeat = $3;
          }
        ;

event_control
        : '@' identifier
          {
              YYTRACE("event_control: '@' identifier");
              vl_expr *expr = new vl_expr(IDExpr, 0, 0.0, $2, 0, 0);
              $$ = new vl_event_expr(EdgeEventExpr, expr);
          }
        | '@' '(' event_expression ')'
          {
              YYTRACE("event_control: '@' '(' event_expression ')'");
              $$ = $3;
          }
        | '@' '(' ored_event_expression ')'
          {
              YYTRACE("event_control: '@' '(' ored_event_expression ')'");
              $$ = $3;
          }
        ;


event_expression
        : expression
          {
              YYTRACE("event_expression: expression");
              $$ = new vl_event_expr(EdgeEventExpr, $1);
          }
        | YYPOSEDGE expression
          {
              YYTRACE("event_expression: YYPOSEDGE expression");
              $$ = new vl_event_expr(PosedgeEventExpr, $2);
          }
        | YYNEGEDGE expression
          {
              YYTRACE("event_expression: YYNEGEDGE expression");
              $$ = new vl_event_expr(NegedgeEventExpr, $2);
          }
        ;

ored_event_expression
        : event_expression YYOR event_expression
          {
              YYTRACE("ored_event_expression: event_expression YYOR "
                  "event_expression");
              lsList<vl_event_expr*> *event_list = new lsList<vl_event_expr*>;
              event_list->newEnd($1);
              event_list->newEnd($3);
              $$ = new vl_event_expr(OrEventExpr, 0);
              $$->list = event_list;
          }
        | ored_event_expression YYOR event_expression
          {
              YYTRACE("ored_event_expression: ored_event_expression YYOR "
                  "event_expression");
              $1->list->newEnd($3);
              $$ = $1;
          }
        ; 

%%

void
yyerror(const char *str)
{
    vl_error("%s: (line: %d, file: %s) token: '%s', ", str, yylineno,
        VP.filename, yy_textbuf());
    vl_error("yacc token: '%s'", yy_textbuf());
    if (VP.context->currentModule()) 
        vl_error("unexpected token '%s' seen in module '%s'",
            yy_textbuf(), VP.context->currentModule()->name);
    longjmp(VP.jbuf, 1);
}


static vl_strength *
drive_strength(int strength0, int strength1)
{
    static vl_strength retval;
    switch (strength0) {
    case YYSUPPLY0: 
        retval.str0 = STRsupply;
        break;
    case YYSTRONG0:
        retval.str0 = STRstrong;
        break;
    case YYPULL0:
        retval.str0 = STRpull;
        break;
    case YYWEAK0:
        retval.str0 = STRweak;
        break;
    case YYHIGHZ0:
        retval.str0 = STRhiZ;
        break;
    case YYSMALL:
        retval.str0 = STRsmall;
        break;
    case YYMEDIUM:
        retval.str0 = STRmed;
        break;
    case YYLARGE:
        retval.str0 = STRlarge;
        break;
    }
    switch (strength1) {
    case YYSUPPLY1: 
        retval.str1 = STRsupply;
        break;
    case YYSTRONG1:
        retval.str1 = STRstrong;
        break;
    case YYPULL1:
        retval.str1 = STRpull;
        break;
    case YYWEAK1:
        retval.str1 = STRweak;
        break;
    case YYHIGHZ1:
        retval.str1 = STRhiZ;
        break;
    }
    if (retval.str0 == STRnone || (retval.str1 == STRnone &&
            (retval.str0 != STRsmall &&
            retval.str0 != STRmed &&
            retval.str0 != STRlarge))) {
        VP.error(ERR_COMPILE, "illegal strength0/strength1");
        retval.str0 = retval.str1 = STRnone;
    }
    return (&retval);
}


static void
edge_err(unsigned char a, unsigned char b)
{
    const char *as = "<>";
    switch (a) {
    case Prim0:
        as = "0";
        break;
    case Prim1:
        as = "1";
        break;
    case PrimX:
        as = "x";
        break;
    case PrimB:
        as = "b";
        break;
    case PrimQ:
        as = "?";
        break;
    }

    const char *bs = "<>";
    switch (b) {
    case Prim0:
        bs = "0";
        break;
    case Prim1:
        bs = "1";
        break;
    case PrimX:
        bs = "x";
        break;
    case PrimB:
        bs = "b";
        break;
    case PrimQ:
        bs = "?";
        break;
    }

    char mesg[MAXSTRLEN];
    sprintf(mesg, "Unknown edge symbol (%s%s)", as, bs);
    VP.error(ERR_COMPILE, mesg);
}


static unsigned char
setprim(unsigned char a, unsigned char b)
{
    switch (a) {
    case Prim0:
        switch (b) {
        case Prim1: return (PrimR);
        case PrimX: return (Prim0X);
        case PrimB: return (PrimR);
        case PrimQ: return (Prim0Q);
        }
        break;
    case Prim1:
        switch (b) {
        case Prim0: return (PrimF);
        case PrimX: return (Prim1X);
        case PrimB: return (PrimF);
        case PrimQ: return (Prim1Q);
        }
        break;
    case PrimX: 
        switch (b) {
        case Prim0: return (PrimX0);
        case Prim1: return (PrimX1);
        case PrimB: return (PrimXB);
        case PrimQ: return (PrimXB);
        }
        break;
    case PrimB:
        switch (b) {
        case Prim0: return (PrimF);
        case Prim1: return (PrimR);
        case PrimX: return (PrimBX);
        case PrimB: return (PrimBB);
        case PrimQ: return (PrimBQ);
        }
        break;
    case PrimQ: 
        switch (b) {
        case Prim0: return (PrimQ0);
        case Prim1: return (PrimQ1);
        case PrimX: return (PrimBX);
        case PrimB: return (PrimQB);
        case PrimQ: return (PrimS);
        }
        break;
    }
    edge_err(a, b);
    return (0);
}
