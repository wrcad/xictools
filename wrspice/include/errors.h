
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1986 Thomas L. Quarles
**********/

#ifndef ERRORS_H
#define ERRORS_H


// Input processing error message descriptions
//
#define E_INTRPT     -2 // interrupt received, pausing
#define E_PAUSE      -1 // condition met, pausing
#define OK            0 // satisfactory conclusion
#define E_NOMEM       1 // insufficient memory available - VERY FATAL
#define E_PANIC       2 // vague internal error for "can't get here" cases
#define E_FAILED      3 // general operation failure
#define E_NOCKT       4 // null reference to a circuit
#define E_NODEV       5 // attempt to modify a non-existant instance
#define E_NOMOD       6 // attempt to modify a non-existant model
#define E_NOANAL      7 // attempt to modify a non-existant analysis
#define E_NOTERM      8 // attempt to bind to a non-existant terminal
#define E_NOVEC       9 // no vector vound
#define E_NOTFOUND   10 // simulator can't find something it was looking for
#define E_BADPARM    11 // attempt to specify a non-existant parameter
#define E_PARMVAL    12 // the parameter value specified is illegal
#define E_UNSUPP     13 // the specified operation is unsupported
#define E_EXISTS     14 // warning/error - attempt to create duplicate
#define E_NOTEMPTY   15 // deleted still referenced item
#define E_NOCHANGE   16 // simulator can't tolerate any more topology changes
#define E_BAD_DOMAIN 17 // output interface begin/end domain calls mismatched
#define E_NODECON    18 // warning/error - bad node
#define E_SYNTAX     19 // syntax error
#define E_TOOMUCH    20 // some limit was exceeded

// Simulator run-time errors
//
#define E_INTERN E_PANIC
#define E_NOCURCKT     100 // no current circuit
#define E_SINGULAR     101 // matrix is singular
#define E_BADMATRIX    102 // ill-formed matrix can't be decomposed
#define E_ITERLIM      103 // iteration limit reached
#define E_ORDER        104 // integration order not supported
#define E_METHOD       105 // integration method not supported
#define E_TIMESTEP     106 // timestep too small
#define E_XMISSIONLINE 107 // transmission line in pz analysis
#define E_MAGEXCEEDED  108 // pole-zero magnitude too large
#define E_SHORT        109 // pole-zero input or output shorted
#define E_INISOUT      110 // pole-zero input is output
#define E_ASKPOWER     111 // ac powers cannot be ASKed
#define E_NODUNDEF     112 // node not defined in noise analysis
#define E_NOACINPUT    113 // no ac input source specified for noise
#define E_NOF2SRC      114 // no source at F2 for IM dist analysis
#define E_NODISTO      115 // no disto analysis - NODISTO defined
#define E_NONOISE      116 // no noise analysis - NONOISE defined
#define E_MATHDBZ      117 // math error, divide by zero
#define E_MATHINV      118 // math error, invalid result
#define E_MATHOVF      119 // math error, overflow
#define E_MATHUNF      120 // math error, underflow

#endif // ERRORS_H

