
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
 $Id: sensgen.h,v 2.54 2015/07/30 17:27:11 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#ifndef SENSGEN_H
#define SENSGEN_H


struct sgen
{
    sgen(sCKT*, bool);
    ~sgen() { delete [] sg_devlist; }

    sgen *next();
    bool set_inst();
    bool set_dev();
    bool set_param();
    void suspend();
    void restore();
    int get_param(IFdata*);
    int set_param(IFdata*);
    int load_new(bool);
    void dbg_print(const char*, double);

    double sg_value;
    sCKT *sg_ckt;
    sGENmodel **sg_devlist;
    sGENmodel *sg_model;
    sGENmodel *sg_next_model;
    sGENmodel *sg_first_model;
    sGENinstance *sg_instance;
    sGENinstance *sg_next_instance;
    sGENinstance *sg_first_instance;
    IFparm *sg_ptable;
    int sg_devsz;
    int sg_dev;
    int sg_istate;
    int sg_param;
    int sg_max_param;
    bool sg_is_dc;
    bool sg_is_instparam;
    bool sg_is_q;
    bool sg_is_principle;
    bool sg_is_zerook;
};

#endif //SENSGEN_H

