
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

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#ifndef KWORDS_ANALYSIS_H
#define KWORDS_ANALYSIS_H


// AC keywords
extern const char *ackw_start;
extern const char *ackw_stop;
extern const char *ackw_numsteps;
extern const char *ackw_dec;
extern const char *ackw_oct;
extern const char *ackw_lin;

// DC keywords
extern const char *dckw_name1;
extern const char *dckw_start1;
extern const char *dckw_stop1;
extern const char *dckw_step1;
extern const char *dckw_name2;
extern const char *dckw_start2;
extern const char *dckw_stop2;
extern const char *dckw_step2;

// Distortion keywords
extern const char *dokw_start;
extern const char *dokw_stop;
extern const char *dokw_numsteps;
extern const char *dokw_dec;
extern const char *dokw_oct;
extern const char *dokw_lin;
extern const char *dokw_f2overf1;

// Noise keywords
extern const char *nokw_output;
extern const char *nokw_outputref;
extern const char *nokw_input;
extern const char *nokw_ptspersum;

// PZ keywords
extern const char *pzkw_nodei;
extern const char *pzkw_nodeg;
extern const char *pzkw_nodej;
extern const char *pzkw_nodek;
extern const char *pzkw_v;
extern const char *pzkw_i;
extern const char *pzkw_pol;
extern const char *pzkw_zer;
extern const char *pzkw_pz;

// Sens keywords
extern const char *snkw_deftol;
extern const char *snkw_defperturb;
extern const char *snkw_outpos;
extern const char *snkw_outneg;
extern const char *snkw_outsrc;
extern const char *snkw_outname;

// TF keywords
extern const char *tfkw_outpos;
extern const char *tfkw_outneg;
extern const char *tfkw_outname;
extern const char *tfkw_outsrc;
extern const char *tfkw_insrc;

// Tran keywords
extern const char *trkw_part;
extern const char *trkw_tstart;
extern const char *trkw_tmax;
extern const char *trkw_uic;
extern const char *trkw_scroll;
extern const char *trkw_segment;
extern const char *trkw_segwidth;

// Spice option keywords
// reals
extern const char *spkw_abstol;
extern const char *spkw_chgtol;
extern const char *spkw_dcmu;
extern const char *spkw_defad;
extern const char *spkw_defas;
extern const char *spkw_defl;
extern const char *spkw_defw;
extern const char *spkw_delmin;
extern const char *spkw_dphimax;
extern const char *spkw_jjdphimax;
extern const char *spkw_gmax;
extern const char *spkw_gmin;
extern const char *spkw_maxdata;
extern const char *spkw_minbreak;
extern const char *spkw_pivrel;
extern const char *spkw_pivtol;
extern const char *spkw_rampup;
extern const char *spkw_reltol;
extern const char *spkw_temp;
extern const char *spkw_tnom;
extern const char *spkw_trapratio;
extern const char *spkw_trtol;
extern const char *spkw_vntol;
extern const char *spkw_xmu;

// integers
extern const char *spkw_bypass;
extern const char *spkw_fpemode;
extern const char *spkw_gminsteps;
extern const char *spkw_interplev;
extern const char *spkw_itl1;
extern const char *spkw_itl2;
extern const char *spkw_itl2gmin;
extern const char *spkw_itl2src;
extern const char *spkw_itl4;
extern const char *spkw_loadthrds;
extern const char *spkw_loopthrds;
extern const char *spkw_maxord;
extern const char *spkw_srcsteps;
extern const char *spkw_itl6;

// bools
extern const char *spkw_dcoddstep;
extern const char *spkw_extprec;
extern const char *spkw_forcegmin;
extern const char *spkw_gminfirst;
extern const char *spkw_hspice;
extern const char *spkw_jjaccel;
extern const char *spkw_noiter;
extern const char *spkw_nojjtp;
extern const char *spkw_noklu;
extern const char *spkw_nomatsort;
extern const char *spkw_noopiter;
extern const char *spkw_noshellopts;
extern const char *spkw_oldlimit;
extern const char *spkw_oldsteplim;
extern const char *spkw_renumber;
extern const char *spkw_savecurrent;
extern const char *spkw_spice3;
extern const char *spkw_trapcheck;
extern const char *spkw_trytocompact;
extern const char *spkw_useadjoint;

// strings
extern const char *spkw_method;
extern const char *spkw_optmerge;
extern const char *spkw_parhier;
extern const char *spkw_steptype;

// "external" options
extern const char *spkw_acct;
extern const char *spkw_dev;
extern const char *spkw_list;
extern const char *spkw_mod;
extern const char *spkw_node;
extern const char *spkw_nopage;
extern const char *spkw_numdgt;
extern const char *spkw_opts;
extern const char *spkw_post;

// obsolete and/or unsupported
extern const char *spkw_cptime;
extern const char *spkw_itl3;
extern const char *spkw_itl5;
extern const char *spkw_limpts;
extern const char *spkw_limtim;
extern const char *spkw_lvlcod;
extern const char *spkw_lvltim;
extern const char *spkw_nomod;

// string option values
extern const char *spkw_trap;
extern const char *spkw_gear;
extern const char *spkw_global;
extern const char *spkw_local;
extern const char *spkw_noshell;
extern const char *spkw_interpolate;
extern const char *spkw_hitusertp;
extern const char *spkw_nousertp;
extern const char *spkw_fixedstep;
extern const char *spkw_delta;

// Resource keywords
extern const char *stkw_accept;
extern const char *stkw_cvchktime;
extern const char *stkw_equations;
extern const char *stkw_fillin;
extern const char *stkw_involcxswitch;
extern const char *stkw_loadtime;
extern const char *stkw_lutime;
extern const char *stkw_matsize;
extern const char *stkw_nonzero;
extern const char *stkw_pagefaults;
extern const char *stkw_rejected;
extern const char *stkw_reordertime;
extern const char *stkw_solvetime;
extern const char *stkw_threads;
extern const char *stkw_time;
extern const char *stkw_totiter;
extern const char *stkw_trancuriters;
extern const char *stkw_traniter;
extern const char *stkw_tranitercut;
extern const char *stkw_tranlutime;
extern const char *stkw_tranouttime;
extern const char *stkw_tranpoints;
extern const char *stkw_transolvetime;
extern const char *stkw_trantime;
extern const char *stkw_trantrapcut;
extern const char *stkw_trantstime;
extern const char *stkw_volcxswitch;

extern const char *stat_print_array[];

#endif

