
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

#ifndef KWORDS_FTE_H
#define KWORDS_FTE_H


/*************************************************************************
 *  Plot Keywords
 *************************************************************************/

// the plotstyle arguments
extern const char *kw_linplot;
extern const char *kw_pointplot;
extern const char *kw_combplot;

// the gridstyle arguments
extern const char *kw_lingrid;
extern const char *kw_xlog;
extern const char *kw_ylog;
extern const char *kw_loglog;
extern const char *kw_polar;
extern const char *kw_smith;
extern const char *kw_smithgrid;

// the scaletype arguments
extern const char *kw_multi;
extern const char *kw_single;
extern const char *kw_group;

// plot window geometry
extern const char *kw_plotgeom;

// the plot keywords
extern const char *kw_curplot;
extern const char *kw_device;
extern const char *kw_gridsize;
extern const char *kw_gridstyle;
extern const char *kw_nogrid;
extern const char *kw_noplotlogo;
extern const char *kw_nointerp;
extern const char *kw_plotstyle;
extern const char *kw_pointchars;
extern const char *kw_polydegree;
extern const char *kw_polysteps;
extern const char *kw_scaletype;
extern const char *kw_ticmarks;
extern const char *kw_title;
extern const char *kw_xcompress;
extern const char *kw_xdelta;
extern const char *kw_xindices;
extern const char *kw_xlabel;
extern const char *kw_xlimit;
extern const char *kw_ydelta;
extern const char *kw_ylabel;
extern const char *kw_ylimit;
extern const char *kw_ysep;

// 'no_record' keywords
extern const char *kw_curanalysis;
extern const char *kw_curplotdate;
extern const char *kw_curplotname;
extern const char *kw_curplottitle;
extern const char *kw_plots;

// asciiplot keywords
extern const char *kw_noasciiplotvalue;
extern const char *kw_nobreak;

// hardcopy keywords
extern const char *kw_hcopydriver;
extern const char *kw_hcopycommand;
extern const char *kw_hcopyresol;
extern const char *kw_hcopywidth;
extern const char *kw_hcopyheight;
extern const char *kw_hcopyxoff;
extern const char *kw_hcopyyoff;
extern const char *kw_hcopylandscape;
extern const char *kw_hcopyrmdelay;

// source keywords
extern const char *kw_noprtitle;

// xgraph keywords
extern const char *kw_xglinewidth;
extern const char *kw_xgmarkers;

/*************************************************************************
 *  Color Keywords
 *************************************************************************/

// the colorN keywords
extern const char *kw_color0;
extern const char *kw_color1;
extern const char *kw_color2;
extern const char *kw_color3;
extern const char *kw_color4;
extern const char *kw_color5;
extern const char *kw_color6;
extern const char *kw_color7;
extern const char *kw_color8;
extern const char *kw_color9;
extern const char *kw_color10;
extern const char *kw_color11;
extern const char *kw_color12;
extern const char *kw_color13;
extern const char *kw_color14;
extern const char *kw_color15;
extern const char *kw_color16;
extern const char *kw_color17;
extern const char *kw_color18;
extern const char *kw_color19;

/*************************************************************************
 *  Debug Keywords
 *************************************************************************/

// the debug arguments
extern const char *kw_async;
extern const char *kw_control;
extern const char *kw_cshpar;
extern const char *kw_eval;
extern const char *kw_ginterface;
extern const char *kw_helpsys;
extern const char *kw_plot;
extern const char *kw_parser;
extern const char *kw_siminterface;
extern const char *kw_vecdb;

// the debug keywords
extern const char *kw_debug;
extern const char *kw_display;
extern const char *kw_dontplot;
extern const char *kw_noparse;
extern const char *kw_nosubckt;
extern const char *kw_program;
extern const char *kw_strictnumparse;
extern const char *kw_units_catchar;
extern const char *kw_subc_catchar;
extern const char *kw_subc_catmode;
extern const char *kw_plot_catchar;
extern const char *kw_spec_catchar;
extern const char *kw_var_catchar;
extern const char *kw_term;
extern const char *kw_trantrace;
extern const char *kw_fpemode;

/*************************************************************************
 *  Command Keywords
 *************************************************************************/

// filetype arguments
extern const char *kw_ascii;
extern const char *kw_binary;

// level arguments
extern const char *kw_i;
extern const char *kw_b;
extern const char *kw_a;

// specwindow arguments
extern const char *kw_bartlet;
extern const char *kw_blackman;
extern const char *kw_cosine;
extern const char *kw_gaussian;
extern const char *kw_hamming;
extern const char *kw_hanning;
extern const char *kw_none;
extern const char *kw_rectangular;
extern const char *kw_triangle;

// units arguments
extern const char *kw_radians;
extern const char *kw_degrees;

// the command configuration keywords
extern const char *kw_appendwrite;
extern const char *kw_checkiterate;
extern const char *kw_diff_abstol;
extern const char *kw_diff_reltol;
extern const char *kw_diff_vntol;
extern const char *kw_dollarcmt;
extern const char *kw_dpolydegree;
extern const char *kw_editor;
extern const char *kw_errorlog;
extern const char *kw_filetype;
extern const char *kw_fourgridsize;
extern const char *kw_helpinitxpos;
extern const char *kw_helpinitypos;
extern const char *kw_helppath;
extern const char *kw_installcmdfmt;
extern const char *kw_level;
extern const char *kw_modpath;
extern const char *kw_mplot_cur;
extern const char *kw_nfreqs;
extern const char *kw_nocheckupdate;
extern const char *kw_nomodload;
extern const char *kw_nopadding;
extern const char *kw_nopage;
extern const char *kw_numdgt;
extern const char *kw_printautowidth;
extern const char *kw_printnoheader;
extern const char *kw_printnoindex;
extern const char *kw_printnopageheader;
extern const char *kw_printnoscale;
extern const char *kw_random;
extern const char *kw_rawfile;
extern const char *kw_rawfileprec;
extern const char *kw_rhost;
extern const char *kw_rprogram;
extern const char *kw_spectrace;
extern const char *kw_specwindow;
extern const char *kw_specwindoworder;
extern const char *kw_spicepath;
extern const char *kw_units;
extern const char *kw_xicpath;

/*************************************************************************
 *  Shell Keywords
 *************************************************************************/

// the shell keywords
extern const char *kw_cktvars;
extern const char *kw_height;
extern const char *kw_history;
extern const char *kw_ignoreeof;
extern const char *kw_noaskquit;
extern const char *kw_nocc;
extern const char *kw_noclobber;
extern const char *kw_noedit;
extern const char *kw_noerrwin;
extern const char *kw_noglob;
extern const char *kw_nomoremode;
extern const char *kw_nonomatch;
extern const char *kw_nosort;
extern const char *kw_prompt;
extern const char *kw_sourcepath;
extern const char *kw_unixcom;
extern const char *kw_width;
extern const char *kw_wmfocusfix;

/*************************************************************************
 *  Simulator Keywords
 *************************************************************************/

// parser keywords
extern const char *kw_modelcard;
extern const char *kw_pexnodes;
extern const char *kw_nobjthack;
extern const char *kw_subend;
extern const char *kw_subinvoke;
extern const char *kw_substart;

/*************************************************************************
 *  Misc.
 *************************************************************************/

// breakp.cc
extern const char *kw_stop;
extern const char *kw_trace;
extern const char *kw_iplot;
extern const char *kw_save;
extern const char *kw_after;
extern const char *kw_at;
extern const char *kw_before;
extern const char *kw_when;
extern const char *kw_active;
extern const char *kw_inactive;

// initialize.cc
extern const char *kw_all;
extern const char *kw_everything;

// resource.cc
extern const char *kw_elapsed;
extern const char *kw_totaltime;
extern const char *kw_space;
extern const char *kw_faults;
extern const char *kw_stats;

// hardcopy.cc
extern const char *kw_hcopyfilename;

#endif

