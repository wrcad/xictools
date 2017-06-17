
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
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: ftehelp.h,v 2.24 2013/06/08 04:05:26 stevew Exp $
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

