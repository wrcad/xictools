
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
 $Id: sensgen.cc,v 2.16 2016/09/26 01:48:46 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: UCB CAD Group
         1993 Stephen R. Whiteley
****************************************************************************/

#include "sensdefs.h"
#include "sensgen.h"
#include "device.h"
#include "misc.h"
#include "ttyio.h"
#include "spmatrix.h"

char *Sfilter = 0;


sgen::sgen(sCKT *ckt, bool is_dc)
{
    sg_value = 0.0;
    sg_ckt = ckt;

    int num = 0;
    sCKTmodGen mgen(ckt->CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        if (m->GENmodType > num)
            num = m->GENmodType;
    }
    sg_devsz = num + 1;
    sg_devlist = new sGENmodel*[sg_devsz];
    memset(sg_devlist, 0, sg_devsz*sizeof(sGENmodel*));

    mgen = sCKTmodGen(ckt->CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next())
        sg_devlist[m->GENmodType] = m;

    sg_model = 0;
    sg_next_model = 0;
    sg_first_model = 0;
    sg_instance = 0;
    sg_next_instance = 0;
    sg_first_instance = 0;
    sg_ptable = 0;

    sg_dev = -1;
    sg_istate = 0;
    sg_param = 99999;
    sg_is_dc = is_dc;
    sg_is_instparam = false;
    sg_is_q = false;
    sg_is_principle = false;
    sg_is_zerook = false;
}


sgen *
sgen::next()
{
    bool done = false;
    int i = sg_dev;
    enum { INST_PRMS, MODL_PRMS };

    do {
        if (sg_instance) {
            if (sg_ptable) {
                do {
                    sg_param += 1;
                }
                while (sg_param < sg_max_param && !set_param());
            }
            else
                sg_max_param = -1;

            if (sg_param < sg_max_param) {
#ifdef SENS_DEBUG
                dbg_print("next", sg_value);
#endif
                done = true;
            }
            else if (!sg_is_instparam) {
                // Try instance parameters now.
                sg_is_instparam = true;
                sg_param = -1;
                DEV.device(i)->getPtable(&sg_max_param, &sg_ptable, INST_PRMS);
            }
            else {
                sg_is_principle = false;
                sg_instance->GENnextInstance = sg_next_instance;
                sg_instance->GENstate = sg_istate;
                sg_instance = 0;
            }
        }
        else if (sg_model) {

            // Find the first/next good instance for this model.
            bool good = false;
            for ( ; !good && sg_next_instance; good = set_inst()) {
                sg_instance = sg_next_instance;
                sg_next_instance = sg_instance->GENnextInstance;
            }

            if (good) {
                sg_is_principle = false;
                sg_istate = sg_instance->GENstate;
                sg_instance->GENnextInstance = 0;
                sg_model->GENinstances = sg_instance;
                DEV.device(i)->getPtable(&sg_max_param, &sg_ptable, MODL_PRMS);
                sg_param = -1;
                sg_is_instparam = false;
            }
            else {
                // No good instances of this model.
                sg_model->GENinstances = sg_first_instance;
                sg_model->GENnextModel = sg_next_model;
                sg_model = 0;
            }

        }
        else if (i >= 0) {

            // Find the first/next good model for this device.
            bool good = false;
            for ( ; !good && sg_next_model; good = set_inst()) {
                sg_model = sg_next_model;
                sg_next_model = sg_model->GENnextModel;
            }

            if (good) {
                sg_model->GENnextModel = 0;
                sg_devlist[i] = sg_model;
                DEV.device(i)->getPtable(&sg_max_param, &sg_ptable, MODL_PRMS);
                sg_next_instance = sg_first_instance = sg_model->GENinstances;
            }
            else {
                // No more good models for this device.
                sg_devlist[i] = sg_first_model;
                i = -1; // Try the next good device.
            }
        }
        else if (i < sg_devsz && sg_dev < sg_devsz) {

            // Find the next good device in this circuit.

            do {
                sg_dev++;
            }
            while (sg_dev < sg_devsz && sg_devlist[sg_dev] && !set_dev());

            i = sg_dev;

            if (i >= sg_devsz) {
                done = true;
                break;
            }
            sg_first_model = sg_next_model = sg_devlist[i];

        }
        else
            done = true;

    } while (!done);

    if (sg_dev >= sg_devsz) {
        delete this;
        return (0);
    }
    return (this);
}


bool
sgen::set_inst()
{
    return (true);
}


bool
sgen::set_dev()
{
    return (true);
}


bool
sgen::set_param()
{
    if (!sg_ptable[sg_param].keyword)
        return (false);
    if (Sfilter && strncmp(sg_ptable[sg_param].keyword, Sfilter,
            strlen(Sfilter)))
        return (false);
    if ((sg_ptable[sg_param].dataType &
            (IF_SET|IF_ASK|IF_REAL|IF_VECTOR|IF_REDUNDANT|IF_NONSENSE))
            != (IF_SET|IF_ASK|IF_REAL)) {

        // If the parameter is IF_PRINCIPAL and IF_PARSETREE, it can
        // also be treated as a real value (resistor/capacitor value).
        if (!(sg_ptable[sg_param].dataType & IF_PRINCIPAL) ||
                !(sg_ptable[sg_param].dataType & IF_PARSETREE))
            return (false);
    }
    if (sg_is_dc &&
        (sg_ptable[sg_param].dataType & (IF_AC | IF_AC_ONLY)))
        return (false);
    if ((sg_ptable[sg_param].dataType & IF_CHKQUERY) && !sg_is_q)
        return (false);

    IFdata ifdata;
    if (get_param(&ifdata))
        return (false);

    if (fabs(ifdata.v.rValue) < 1e-30) {
        if (sg_ptable[sg_param].dataType & IF_SETQUERY)
            sg_is_q = false;

        if (!sg_is_zerook && !(sg_ptable[sg_param].dataType & IF_PRINCIPAL))
            return (false);
    }
    else if (sg_ptable[sg_param].dataType & (IF_SETQUERY|IF_ORQUERY))
        sg_is_q = true;

    if (sg_ptable[sg_param].dataType & IF_PRINCIPAL)
        sg_is_principle = true;

    sg_value = ifdata.v.rValue;

#ifdef SENS_DEBUG
    dbg_print("iset", sg_value);
#endif

    return (true);
}


void
sgen::suspend()
{
    sg_devlist[sg_dev] = sg_first_model;
    sg_model->GENnextModel = sg_next_model;
    sg_instance->GENnextInstance = sg_next_instance;
    sg_model->GENinstances = sg_first_instance;
}


void
sgen::restore()
{
    sg_devlist[sg_dev] = sg_model;
    sg_model->GENnextModel = 0;
    sg_instance->GENnextInstance = 0;
    sg_model->GENinstances = sg_instance;
}


// Get parameter value.
//
int
sgen::get_param(IFdata *data)
{
    int pid;
    int error = 0;
    if (sg_is_instparam) {
        pid = DEV.device(sg_dev)->instanceParm(sg_param)->id;
        error = DEV.device(sg_dev)->askInst(sg_ckt, sg_instance, pid, data);
#ifdef SENS_DEBUG
        dbg_print("get", d.v.rValue);
#endif
    }
    else {
        pid = DEV.device(sg_dev)->modelParm(sg_param)->id;
        IFdata d;
        error = DEV.device(sg_dev)->askModl(sg_model, pid, data);
#ifdef SENS_DEBUG
        dbg_print("get", val->rValue);
#endif
    }

    if (error) {
        if (sg_is_instparam)
            TTY.err_printf("GET ERROR: %s:%s:%s -> param %s (%d)\n",
                DEV.device(sg_dev)->name(),
                (char*)sg_model->GENmodName,
                (char*)sg_instance->GENname,
                sg_ptable[sg_param].keyword, pid);
        else
            TTY.err_printf("GET ERROR: %s:%s:%s -> mparam %s (%d)\n",
                DEV.device(sg_dev)->name(),
                (char*)sg_model->GENmodName,
                (char*)sg_instance->GENname,
                sg_ptable[sg_param].keyword, pid);
    }
    return (error);
}


// Set parameter value.
//
int
sgen::set_param(IFdata *data)
{
    int pid;
    int error = 0;
    if (sg_is_instparam) {
#ifdef SENS_DEBUG
        dbg_print("set", data->v.rValue);
#endif
        pid = DEV.device(sg_dev)->instanceParm(sg_param)->id;
        error =
            DEV.device(sg_dev)->setInst(pid, data, sg_instance);
    }
    else {
#ifdef SENS_DEBUG
        dbg_print("set", data->v.rValue);
#endif
        pid = DEV.device(sg_dev)->modelParm(sg_param)->id;
        error = DEV.device(sg_dev)->setModl(pid, data, sg_model);
    }

    if (error) {
        if (sg_is_instparam)
            TTY.err_printf("SET ERROR: %s:%s:%s -> param %s (%d)\n",
                DEV.device(sg_dev)->name(),
                (char*)sg_model->GENmodName,
                (char*)sg_instance->GENname,
                sg_ptable[sg_param].keyword, pid);
        else
            TTY.err_printf("SET ERROR: %s:%s:%s -> mparam %s (%d)\n",
                DEV.device(sg_dev)->name(),
                (char*)sg_model->GENmodName,
                (char*)sg_instance->GENname,
                sg_ptable[sg_param].keyword, pid);
    }
    return (error);
}


int
sgen::load_new(bool is_dc)
{
    // CKT preload is a speed optimization whereby the constant part
    // of the A matrix is loaded in setup() or temp(), for some
    // devices.  In the DC case, it is *not* optional, as some
    // devices, such as resistors, don't even have a load function. 
    // We have to turn it off temporarily here in the AC case to avoid
    // double-loading.

    if (!is_dc)
        sg_ckt->CKTpreload = 0;

    // call setup
    sg_ckt->CKTnumStates = sg_istate;
    DEV.device(sg_dev)->setup(sg_model, sg_ckt, &sg_ckt->CKTnumStates);

    // call temp
    DEV.device(sg_dev)->temperature(sg_model, sg_ckt);
    if (!is_dc)
        sg_ckt->CKTpreload = 1;
    else
        sg_ckt->CKTmatrix->spSaveForInitialization();  // cache real part

    // call load
    int error;
    if (!is_dc)
        error = DEV.device(sg_dev)->acLoad(sg_model, sg_ckt);
    else {
        error = OK;
        for (sGENmodel *dm = sg_model; dm; dm = dm->GENnextModel) {
            for (sGENinstance *d = dm->GENinstances; d;
                    d = d->GENnextInstance) {
                error = DEV.device(sg_dev)->load(d, sg_ckt);
                if (error == LOAD_SKIP_FLAG) {
                    error = OK;
                    break;
                }
                if (error != OK)
                    break;
            }
        }
    }
    return (error);
}


void
sgen::dbg_print(const char *str, double dd)
{
    if (sg_is_instparam) {
        TTY.err_printf("%s %d dev %s %s %g\n", str, sg_dev,
            DEV.device(sg_dev)->name(),
            DEV.device(sg_dev)->instanceParm(sg_param)->keyword, dd);
    }
    else {
        TTY.err_printf("%s %d mod %s %s %g\n", str, sg_dev,
            DEV.device(sg_dev)->name(),
            DEV.device(sg_dev)->modelParm(sg_param)->keyword, dd);
    }
}

