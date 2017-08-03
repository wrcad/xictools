
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
Author: 1987 Jeffrey M. Hsu
**********/

#ifndef FTEHELP_H
#define FTEHELP_H

#include "help/help_defs.h"
#include "help/help_topic.h"


//
// Defines for help.
//

#define E_HASPLOTS  1
#define E_NOPLOTS   2
#define E_HASGRAPHS 4
#define E_MENUMODE  8

#define E_BEGINNING 4096
#define E_INTERMED  8192
#define E_ADVANCED  16384
#define E_ALWAYS    32768

// Default is intermediate level.
#define E_DEFHMASK  8192


// Text-mode help handler.
//
struct text_help : public TextHelp
{
    text_help()
        {
            curtop = 0;
            quitflag = false;
        }

    bool display(HLPtopic*);
    void show(HLPtopic*);
    HLPtopList *handle(HLPtopic*, HLPtopic**);
    int putlist(HLPtopic*, HLPtopList*, int);

    HLPtopic *curtop;
    bool quitflag;
};

#endif // FTEHELP_H

