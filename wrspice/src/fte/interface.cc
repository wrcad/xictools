
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
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

#include "simulator.h"
#include "circuit.h"
#include "datavec.h"
#include "runop.h"
#include "output.h"
#include "optdefs.h"
#include "statdefs.h"
#include "variable.h"
#include "spnumber/hash.h"
#include "ginterf/graphics.h"


//
// Misc. interface support functions.
//

// Find and return the instance parameter struct matching the given
// name and mask.  The name matching is case-insensitive.  The mask
// should be IF_ASK, IF_SET, or the OR of both.
//
// This builds a hash table on the first call, for subsequent speed.
//
IFparm *
IFdevice::findInstanceParm(const char *pname, int mask)
{
    if (!pname || !dv_numInstanceParms)
        return (0);
    if (dv_numInstanceParms > 10 && !dv_instTab) {
        dv_instTab = new sHtab(true);
        for (int i = 0; i < dv_numInstanceParms; i++) {
            dv_instTab->add(
                dv_instanceParms[i].keyword, &dv_instanceParms[i]);
        }
    }
    if (dv_instTab) {
        IFparm *prm = (IFparm*)sHtab::get(dv_instTab, pname);
        if (prm && (prm->dataType & mask))
            return (prm);
        return (0);
    }

    for (int i = 0; i < dv_numInstanceParms; i++) {
        if ((dv_instanceParms[i].dataType & mask) &&
                lstring::cieq(dv_instanceParms[i].keyword, pname))
            return (dv_instanceParms + i);
    }
    return (0);
}


// Find and return the model parameter struct matching the given
// name and mask.  The name matching is case-insensitive.  The mask
// should be IF_ASK, IF_SET, or the OR of both.
//
// This builds a hash table on the first call, for subsequent speed.
//
IFparm *
IFdevice::findModelParm(const char *pname, int mask)
{
    if (!pname || !dv_numModelParms)
        return (0);
    if (dv_numModelParms > 10 && !dv_modlTab) {
        dv_modlTab = new sHtab(true);
        for (int i = 0; i < dv_numModelParms; i++) {
            dv_modlTab->add(
                dv_modelParms[i].keyword, &dv_modelParms[i]);
        }
    }
    if (dv_modlTab) {
        IFparm *prm = (IFparm*)sHtab::get(dv_modlTab, pname);
        if (prm && (prm->dataType & mask))
            return (prm);
        return (0);
    }

    for (int i = 0; i < dv_numModelParms; i++) {
        if ((dv_modelParms[i].dataType & mask) &&
                lstring::cieq(dv_modelParms[i].keyword, pname))
            return (dv_modelParms + i);
    }
    return (0);
}
// End of IFdevice functions.


// Get the option called 'name'.  If this is null or empty get all
// appropriate options available.  A match is assumed if name is a
// case insensitive prefix of the keyword.
//
variable *
IFanalysis::getOpt(const sCKT *ckt, const sJOB *job, const char *pname,
    IFspecial *sp) const
{
    int i;
    IFdata parm;
    if (pname && *pname) {
        for (i = 0; i < numParms; i++) {
            if (lstring::ciprefix(pname, analysisParms[i].keyword))
                break;
        }
        if (i == numParms)
            return (0);
        if (!(analysisParms[i].dataType & IF_ASK))
            return (0);
        if (!askQuest(ckt, job, analysisParms[i].id, &parm)) {
            if (sp) {
                sp->sp_inst = 0;
                sp->sp_mod = 0;
                sp->sp_job = job;
                sp->sp_an = this;
                sp->sp_parm = &analysisParms[i];
                sp->sp_isset = true;
            }
            return (analysisParms[i].tovar(&parm.v));
        }
    }
    else {
        variable *v, *vars;
        for (i = 0, vars = v = 0; i < numParms; i++) {
            if (!(analysisParms[i].dataType & IF_ASK))
                continue;
            if (!askQuest(ckt, job, analysisParms[i].id, &parm)) {
                if (v) {
                    v->set_next(analysisParms[i].tovar(&parm.v));
                    v = v->next();
                }
                else
                    vars = v = analysisParms[i].tovar(&parm.v);
            }
        }
        return (vars);
    }
    return (0);
}


// As above, but return an IFdata struct, and don't handle lists.
//
int
IFanalysis::getOpt(const sCKT *ckt, const sJOB *job, const char *pname,
    IFdata* data, IFspecial *sp) const
{
    int i;
    IFdata parm;
    if (pname && *pname) {
        for (i = 0; i < numParms; i++) {
            if (lstring::ciprefix(pname, analysisParms[i].keyword))
                break;
        }
        if (i == numParms)
            return (E_BADPARM);
        if (!(analysisParms[i].dataType & IF_ASK))
            return (E_BADPARM);
        int err = askQuest(ckt, job, analysisParms[i].id, &parm);
        if (err == OK) {
            if (sp) {
                sp->sp_inst = 0;
                sp->sp_mod = 0;
                sp->sp_job = job;
                sp->sp_an = this;
                sp->sp_parm = &analysisParms[i];
                sp->sp_isset = true;
            }
            data->type = analysisParms[i].dataType;
            data->v = parm.v;
        }
        return (err);
    }
    return (E_BADPARM);
}
// End of IFanalysis functions.


variable *
IFparm::p_to_v(const IFvalue *pv, int type)
{
    variable *vv = new variable(keyword);
    vv->set_reference(description);
    int err = false;
    int vec = type & IF_VECTOR;
    type &= IF_VARTYPES;
    type &= ~IF_VECTOR;
    if (type == IF_INTEGER) {
        if (vec) {
            variable *v0 = 0, *tv = 0;
            for (int i = 0; i < pv->v.numValue; i++) {
                if (!v0)
                    v0 = tv = new variable;
                else {
                    tv->set_next(new variable);
                    tv = tv->next();
                }
                tv->set_integer((pv->v.vec.iVec)[i]);
            }
            vv->set_list(v0);
        }
        else
            vv->set_integer(pv->iValue);
    }
    else if (type == IF_REAL) {
        if (vec) {
            variable *v0 = 0, *tv = 0;
            for (int i = 0; i < pv->v.numValue; i++) {
                if (!v0)
                    v0 = tv = new variable;
                else {
                    tv->set_next(new variable);
                    tv = tv->next();
                }
                tv->set_real((pv->v.vec.rVec)[i]);
            }
            vv->set_list(v0);
        }
        else
            vv->set_real(pv->rValue);
    }
    else if (type == IF_COMPLEX) {
        if (vec)
            err = true;
        else {
            variable *tv = new variable;
            variable *v0 = tv;
            tv->set_real(pv->cValue.real);
            tv->set_next(new variable);
            tv = tv->next();
            tv->set_real(pv->cValue.imag);
            vv->set_list(v0);
        }
    }
    else if (type == IF_STRING) {
        if (vec)
            err = true;
        else
            vv->set_string(pv->sValue);
    }
    else if (type == IF_INSTANCE) {
        if (vec)
            err = true;
        else
            vv->set_string((const char*)pv->uValue);
    }
    else if (type == IF_FLAG) {
        if (vec)
            err = true;
        else
            vv->set_boolean(pv->iValue ? true : false);
    }
    else if (type == IF_PARSETREE) {
        if (vec)
            err = true;
        else
            vv->set_string(pv->tValue ? pv->tValue->line() : 0);
    }
    else
        err = true;

    if (err) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "can't handle data type %d.\n",
            type);
        delete vv;
        return (0);
    }
    return (vv);
}
// End of IFparm functions.


// Return the corresponding UU_* code for the data type.
//
int
IFdata::toUU()
{
    int code;
    switch (type & IF_UNITS) {
    default:
        code = UU_NOTYPE; break;
    case IF_VOLT:
        code = UU_VOLTAGE; break;
    case IF_AMP:
        code = UU_CURRENT; break;
    case IF_CHARGE:
        code = UU_CHARGE; break;
    case IF_FLUX:
        code = UU_FLUX; break;
    case IF_CAP:
        code = UU_CAP; break;
    case IF_IND:
        code = UU_IND; break;
    case IF_RES:
        code = UU_RES; break;
    case IF_COND:
        code = UU_COND; break;
    case IF_LEN:
        code = UU_LEN; break;
    case IF_AREA:
        code = UU_AREA; break;
    case IF_TEMP:
        code = UU_TEMP; break;
    case IF_FREQ:
        code = UU_FREQUENCY; break;
    case IF_TIME:
        code = UU_TIME; break;
    case IF_POWR:
        code = UU_POWER; break;
    }
    return (code);
}


// The receiver of vector data should call this.  This will free heap
// allocated data, the sender has set the v.freeVec flag.
//
void
IFdata::cleanup()
{
    if ((type & IF_VECTOR) && v.v.freeVec) {
        switch (type & IF_VARTYPES) {
        case IF_FLAGVEC:
        case IF_INTVEC:
            delete [] v.v.vec.iVec;
            break;
        case IF_REALVEC:
            delete [] v.v.vec.rVec;
            break;
        case IF_CPLXVEC:
            delete [] v.v.vec.cVec;
            break;
        case IF_NODEVEC:
        case IF_STRINGVEC:
        case IF_INSTVEC:
            // These aren't used.
            break;
        }
        v.v.vec.iVec = 0;
        type = IF_NOTYPE;
    }
}
// End of IFdata functions.


// Evaluate the special parameter named in string, and return the value
// in data.  If the parameter is a list, use the list_ind arg to index.
// If error, set the error flag and return the error.
//
int
IFspecial::evaluate(const char *string, sCKT *ckt, IFdata *data, int list_ind)
{
    if (sp_error)
        return (sp_error);
    if (!ckt)
        return (E_NOTFOUND);
    if (sp_isset) {
        // The instance and model pointers are only valid for the
        // primary thread.  For other threads we have to search.
        if (sp_inst) {
#ifdef WITH_THREADS
            if (ckt->CKTthreadId == 0)
                sp_error = sp_inst->askParam(ckt, sp_parm->id, data);
            else {
                int type = sp_inst->GENmodPtr->GENmodType;
                sGENinstance *inst = 0;
                if (ckt->findInst(&type, &inst, sp_inst->GENname, 0, 0) == OK)
                    sp_error = inst->askParam(ckt, sp_parm->id, data);
            }
#else
            sp_error = sp_inst->askParam(ckt, sp_parm->id, data);
#endif
        }
        else if (sp_mod) {
#ifdef WITH_THREADS
            if (ckt->CKTthreadId == 0)
                sp_error = sp_mod->askParam(sp_parm->id, data);
            else {
                int type = sp_mod->GENmodType;
                sGENmodel *mod = 0;
                if (ckt->findModl(&type, &mod, sp_mod->GENmodName) == OK)
                    sp_error = mod->askParam(sp_parm->id, data);
            }
#else
            sp_error = sp_mod->askParam(sp_parm->id, data);
#endif
        }
        else {
            // this takes care of options, resources, analysis params
            data->type = sp_parm->dataType & IF_VARTYPES;
            sp_error = sp_an->askQuest(ckt, sp_job, sp_parm->id, data);
        }
    }
    else if (string && *string == '@') {
        char name[64], param[64];
        const char *s = string + 1;
        char *t = name;
        while (*s && *s != '[')
            *t++ = *s++;
        *t = '\0';
        t = param;
        if (*s) {
            s++;
            while (*s && *s != ']' && *s != ',')
                *t++ = *s++;
        }
        *t = '\0';

        // The param name can be the name of a .measure result, with
        // an index.  If so, return the value, or 0 if the measure
        // has not been performed.

        ROgen<sRunopMeas> mgen(OP.runops()->measures(),
            ckt->CKTbackPtr->measures());
        for (sRunopMeas *m = mgen.next(); m; m = mgen.next()) {
            if (m->result() && lstring::cieq(name, m->result())) {
                sDataVec *d = OP.vecGet(name, ckt);
                if (d) {
                    int ix = atoi(param);
                    data->type = IF_REAL;
                    if (ix >= d->length())
                        data->v.rValue = 0.0;
                    else
                        data->v.rValue = d->realval(ix);
                }
                else {
                    data->type = IF_REAL;
                    data->v.rValue = 0.0;
                }
                return (sp_error);
            }
        }

        // passing 'this' to these functions fills in the cached parameters
        // and sets 'isset'
        if (*param) {
            sp_error = ckt->getParam(name, param, data, this);
            if (sp_error)
                sp_error = ckt->getAnalParam(name, param, data, this);
        }
        else {
            sp_error = OPTinfo.getOpt(ckt, 0, name, data, this);
            if (sp_error)
                sp_error = STATinfo.getOpt(ckt, 0, name, data, this);
        }
    }
    if (sp_error) {
        GRpkg::self()->ErrPrintf(ET_WARN, "evaluation error for %s %s.\n",
            string, Sp.ErrorShort(sp_error));
        return (sp_error);
    }

    // can only handle scalars
    if (data->type & IF_VECTOR) {
        if (list_ind >= 0 && list_ind < data->v.v.numValue) {
            data->type &= ~IF_VECTOR;
            if ((data->type & IF_VARTYPES) == IF_INTEGER)
                data->v.iValue = data->v.v.vec.iVec[list_ind];
            else if ((data->type & IF_VARTYPES) == IF_REAL)
                data->v.rValue = data->v.v.vec.rVec[list_ind];
            else if ((data->type & IF_VARTYPES) == IF_COMPLEX)
                data->v.cValue = data->v.v.vec.cVec[list_ind];
            else
                sp_error = E_BADPARM;
        }
        else
            sp_error = E_BADPARM;
    }
    return (sp_error);
}
// End of IFspecial functions.


