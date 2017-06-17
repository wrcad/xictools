
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
 $Id: vl_defs.h,v 1.5 2006/02/28 01:39:35 stevew Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern int yylineno;
extern FILE *yyin;
extern char *yy_textbuf();

// This must be supplied by the application.  If it returns 0, an
// fopen call is made in vl to resolve the file.  The application can
// return a FILE* obtained via a search path, for example.
//
extern FILE *vl_file_open(const char*, const char*);

// verilog.l
extern int yylex();

// verilog.y
extern int yyparse();
extern void yyerror(const char*);


// max columns in prim table
#define MAXPRIMLEN   16            

#define MAXSTRLEN    (8*BUFSIZ)
#define MAXBITNUM    32

#define YYTRACE(str) if (YYTrace) fprintf(stderr, "%s\n", str);

#define MACRO_DEFINE  "define"
#define MACRO_IFDEF   "ifdef"
#define MACRO_IFNDEF  "ifndef"
#define MACRO_ELSE    "else"
#define MACRO_ENDIF   "endif"

// flags for vl_simulator::dbg_flags
//
#define DBG_action    1   // print action before evaluation
#define DBG_assign    2   // print variable assignment
#define DBG_cx        4   // not used
#define DBG_modname   8   // not used
#define DBG_tslot     16  // print time slot action list before evaluation
#define DBG_desc      32  // print some things about the description
#define DBG_mod_dmp   64  // dump module info
#define DBG_ptrace    128 // print yacc parser trace

// Defined data types
//
#define EnumType            1

//---------------------------------------------------------------------------
//  Data variables and expressions
//---------------------------------------------------------------------------

// Expression types (vl_expr)
//
#define BitExpr             100
#define IntExpr             101
#define RealExpr            102
#define StringExpr          103
#define IDExpr              104
#define BitSelExpr          105
#define PartSelExpr         106
#define ConcatExpr          107
#define MinTypMaxExpr       108
#define FuncExpr            109

#define UplusExpr           110  
#define UminusExpr          111 
#define UnotExpr            112 
#define UcomplExpr          113 

#define UandExpr            114 
#define UnandExpr           115 
#define UorExpr             116 
#define UnorExpr            117 
#define UxorExpr            118 
#define UxnorExpr           119 

#define BplusExpr           120 
#define BminusExpr          121 
#define BtimesExpr          123 
#define BdivExpr            124 
#define BremExpr            125 
#define Beq2Expr            126 
#define Bneq2Expr           127 
#define Beq3Expr            128 
#define Bneq3Expr           129 
#define BlandExpr           130 
#define BlorExpr            131 
#define BltExpr             132 
#define BleExpr             133 
#define BgtExpr             134 
#define BgeExpr             135 
#define BandExpr            136 
#define BorExpr             137 
#define BxorExpr            138 
#define BxnorExpr           139 
#define BlshiftExpr         140 
#define BrshiftExpr         141 

#define TcondExpr           142 
#define SysExpr             143

// vl_event_expr
#define OrEventExpr         145
#define NegedgeEventExpr    146
#define PosedgeEventExpr    147
#define EdgeEventExpr       148
#define LevelEventExpr      149

//---------------------------------------------------------------------------
//  Parser objects
//---------------------------------------------------------------------------
// Declaration types
//
// vl_range_or_type
#define Range_Dcl           1
#define Integer_Dcl         2
#define Real_Dcl            3

//---------------------------------------------------------------------------
//  Simulator objects
//---------------------------------------------------------------------------

// vl_action_item
#define ActionItem          100

// Object types
// These are the values of the type field of the various objects
//
//---------------------------------------------------------------------------
//  Verilog description objects
//---------------------------------------------------------------------------

// vl_module
#define ModDecl             1

// vl_primitive
#define CombPrimDecl        2
#define SeqPrimDecl         3

// Primitive entries (vl_prim_entry)
//
// The prim0, Prim1, and PrimX entries must match the values for
// BitL, BitH, and BitDC.
//
#define PrimNone            255
#define Prim0               0
#define Prim1               1
#define PrimX               2
#define PrimB               3
#define PrimQ               4   

// sequential
#define PrimR               5
#define PrimF               6
#define PrimP               7
#define PrimN               8
#define PrimS               9   
#define PrimM               10
#define Prim0X              11
#define Prim1X              12
#define PrimX0              13
#define PrimX1              14
#define PrimXB              15
#define PrimBX              16
#define PrimBB              17
#define PrimQ0              19
#define PrimQ1              20
#define PrimQB              21
#define Prim0Q              22
#define Prim1Q              23
#define PrimBQ              24

// vl_port
#define ModulePort          4
#define NamedPort           5

// vl_port_connect
#define ModuleConnect       6
#define NamedConnect        7

//---------------------------------------------------------------------------
//  Module items
//---------------------------------------------------------------------------

// vl_decl
#define RealDecl            10
#define IntDecl             11
#define TimeDecl            12
#define EventDecl           13
#define InputDecl           14
#define OutputDecl          15
#define InoutDecl           16
#define RegDecl             17
#define ParamDecl           18
#define DefparamDecl        19
#define WireDecl            20
#define TriDecl             21
#define Tri0Decl            22
#define Tri1Decl            23
#define Supply0Decl         24
#define Supply1Decl         25
#define WandDecl            26
#define TriandDecl          27
#define WorDecl             28
#define TriorDecl           29
#define TriregDecl          30

// vl_procstmt
#define AlwaysStmt          31
#define InitialStmt         32

// vl_cont_assign
#define ContAssign          33

// vl_specify_block
#define SpecBlock           34

// vl_specify_item
#define SpecParamDecl       1
#define SpecPathDecl        2
#define SpecLSPathDecl1     3
#define SpecLSPathDecl2     4
#define SpecESPathDecl1     5
#define SpecESPathDecl2     6
#define SpecTiming          7

// vl_path_desc
#define PathLeadTo          1
#define PathAll             2

// vl_task
#define TaskDecl            35

// vl_function
#define IntFuncDecl         36
#define RealFuncDecl        37
#define RangeFuncDecl       38

// vl_gate_inst_list
#define AndGate             39
#define NandGate            40
#define OrGate              41
#define NorGate             42
#define XorGate             43
#define XnorGate            44
#define BufGate             45
#define Bufif0Gate          46
#define Bufif1Gate          47
#define NotGate             48
#define Notif0Gate          49
#define Notif1Gate          50
#define PulldownGate        51
#define PullupGate          52
#define NmosGate            53
#define RnmosGate           54
#define PmosGate            55
#define RpmosGate           56
#define CmosGate            57
#define RcmosGate           58
#define TranGate            59
#define RtranGate           60
#define Tranif0Gate         61
#define Rtranif0Gate        62
#define Tranif1Gate         63
#define Rtranif1Gate        64

// vl_mp_inst_list
#define ModPrimInst         65

// subtype for vl_mp_inst_list
enum { MPundef, MPmod, MPprim };
typedef unsigned short MPtype;

//---------------------------------------------------------------------------
//  Statements
//---------------------------------------------------------------------------

// vl_begin_end_stmt
#define BeginEndStmt        70

// vl_if_else_stmt
#define IfElseStmt          71

//vl_case_stmt
#define CaseStmt            72
#define CasexStmt           73
#define CasezStmt           74

// vl_case_item
#define CaseItem            1
#define DefaultItem         2

// vl_forever_stmt
#define ForeverStmt         75

// vl_repeat_stmt
#define RepeatStmt          76

// vl_while_stmt
#define WhileStmt           77

// vl_for_stmt
#define ForStmt             78

// vl_delay_control_stmt
#define DelayControlStmt    79

// vl_event_control_stmt
#define EventControlStmt    80

// vl_send_event_stmt
#define SendEventStmt       81

// vl_bassign_stmt
#define BassignStmt         82
#define NbassignStmt        83
#define DelayBassignStmt    84
#define DelayNbassignStmt   85
#define EventBassignStmt    86
#define EventNbassignStmt   87
#define AssignStmt          88
#define ForceStmt           89

// vl_wait_stmt
#define WaitStmt            90

// vl_fork_join_stmt
#define ForkJoinStmt        91

// vl_fj_break
#define ForkJoinBreak       92

// vl_task_enable_stmt
#define TaskEnableStmt      93

// vl_sys_task_stmt
#define SysTaskEnableStmt   94

// vl_disable_stmt
#define DisableStmt         95

// vl_deassign_stmt
#define DeassignStmt        96
#define ReleaseStmt         97

