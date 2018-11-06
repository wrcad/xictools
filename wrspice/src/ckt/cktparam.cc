
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

#include "device.h"
#include "variable.h"

#include "frontend.h"
#include "cshell.h"
#include "output.h"


namespace {
    // Return true is either string is a case-insensitive prefix of
    // the other.
    //
    bool cmpstr(const char *s1, const char *s2)
    {
        int c = 0;
        while (*s1 && *s2 && (isupper(*s1) ? tolower(*s1) : *s1) ==
                (isupper(*s2) ? tolower(*s2) : *s2)) {
            s1++;
            s2++;
            c++;
        }
        return (c > 0 && (!*s1 || !*s2));
    }
}


// Look up the 'type' in the device description struct and return the
// appropriate index for the device found, or -1 for not found.  Also
// return the model list head if an address is given.
//
int
sCKT::typelook(const char *name, sGENmodel **mod) const
{
    if (mod)
        *mod = 0;
    if (!name)
        return (-1);

    int type = -1, cnt = 0;
    for (int i = 0; i < DEV.numdevs(); i++) {
        if (cmpstr(name, DEV.device(i)->name())) {
            type = i;
            cnt++;
        }
    }
    if (cnt == 1) {
        // Found unambiguous name match, return it.
        if (mod) {
            sCKTmodItem *mx = CKTmodels.find(type);
            if (mx)
                *mod = mx->head;
        }
        return (type);
    }
    return (-1);
}


// Return the model head for the given device type if a pointer is
// passed, -1 is returned if the type is bad, 0 if ok.
//
int
sCKT::typelook(int type, sGENmodel **mod) const
{
    sCKTmodItem *mx = CKTmodels.find(type);
    if (!mx)
        return (-1);
    if (mod)
        *mod = mx->head;
    return (0);
}


// Perform the CKTask call.  Return in `data' if successful and
// return true.
//
int
sCKT::doask(IFdata *data, const sGENinstance *dev, const sGENmodel *mod,
    const IFparm *opt) const
{
    if (!data || !opt)
        return (E_BADPARM);
    data->v.cValue.real = 0.0;
    data->v.cValue.imag = 0.0;
    data->type = IF_NOTYPE;

    int err;
    if (dev)
        err = dev->askParam(this, opt->id, data);
    else
        err = mod->askParam(opt->id, data);
    return (err);
}


// Get pointers to a device, its model, and its type number given
// the name.  If there is no such device, try to find a model with
// that name.
//
int
sCKT::finddev(IFuid name, sGENinstance **devptr, sGENmodel **modptr) const
{
    int type = -1;
    int err = findInst(&type, devptr, name, 0, 0);
    if (err == OK)
        return (type);

    type = -1;
    if (devptr)
        *devptr = 0;

    err = findModl(&type, modptr, name);
    if (err == OK)
        return (type);

    if (modptr)
        *modptr = 0;
    return (-1);
}


namespace {
    // Return true if s contains global substitution characters.
    //
    inline bool isglob(const char *s)
    {
        return (strchr(s, '?') || strchr(s, '*') || strchr(s, '[') ||
            strchr(s, '{'));
    }


    inline bool pmatch(wordlist *plist, const char *kw)
    {
        while (plist) {
            if (CP.GlobMatch(plist->wl_word, kw))
                return (true);
            plist = plist->wl_next;
        }
        return (false);
    }


    void tolower(wordlist *wl)
    {
        for (wordlist *w = wl; w; w = w->wl_next)
            lstring::strtolower(w->wl_word);
    }
}


// Get a parameter value from the circuit.  If sp is passed, fill it in,
// unless the param is "all".
//
variable *
sCKT::getParam(const char *word, const char *param, IFspecial *sp) const
{
    if (!word || !param)
        return (0);
    sGENinstance *dev = 0;
    sGENmodel *mod = 0;
    bool all = lstring::cieq(param, "all");
    if (all || isglob(param)) {
        char *nm = lstring::copy(word);
        insert(&nm);
        int typecode = finddev(nm, &dev, &mod);
        if (typecode < 0) {
            // This could be an unused bin model, suppress error message in
            // this case.
            const char *t = strrchr(word, '.');
            if (!t || !isdigit(t[1])) {
                OP.error(ERR_WARNING,
                    "no such device or model name %s.\n", nm);
            }
            return (0);
        }
        IFdevice *device = DEV.device(typecode);
        variable *vv = 0, *tv = 0;
        char buf[128];

        if (dev) {
            // We use the CP to do glob matching.  Since we want case
            // insensitive matching for parameters, we convert the
            // strings being compared to lower case.

            wordlist *plist = all ? 0 : CP.BracExpand(param);
            tolower(plist);

            for (int i = 0; ; i++) {
                IFparm *opt = device->instanceParm(i);
                if (!opt)
                    break;
                if (!(opt->dataType & IF_ASK))
                    continue;
                strcpy(buf, opt->keyword);
                lstring::strtolower(buf);
                if (all || pmatch(plist, buf)) {
                    IFdata data;
                    if (doask(&data, dev, mod, opt) == OK) {
                        if (tv) {
                            tv->set_next(opt->tovar(&data));
                            tv = tv->next();
                        }
                        else
                            vv = tv = opt->tovar(&data); 
                    }
                }
            }
            wordlist::destroy(plist);
            return (vv);
        }
        else if (mod) {
            wordlist *plist = all ? 0 : CP.BracExpand(param);
            tolower(plist);
            for (int i = 0; ; i++) {
                IFparm *opt = device->modelParm(i);
                if (!opt)
                    break;
                if (!(opt->dataType & IF_ASK))
                    continue;
                strcpy(buf, opt->keyword);
                lstring::strtolower(buf);
                if (all || pmatch(plist, buf)) {
                    IFdata data;
                    if (doask(&data, dev, mod, opt) == OK) {
                        if (tv) {
                            tv->set_next(opt->tovar(&data));
                            tv = tv->next();
                        }
                        else
                            vv = tv = opt->tovar(&data); 
                    }
                }
            }
            wordlist::destroy(plist);
            return (vv);
        }
        return (0);
    }
    char *nm = lstring::copy(word);
    insert(&nm);
    int typecode = finddev(nm, &dev, &mod);
    if (typecode < 0)
        return (0);
    IFdevice *device = DEV.device(typecode);
    IFparm *opt = 0;
    if (dev)
        opt = device->findInstanceParm(param, IF_ASK);
    else if (mod)
        opt = device->findModelParm(param, IF_ASK);
    if (!opt)
        return (0);
    IFdata data;
    variable *vv = 0;
    if (doask(&data, dev, mod, opt) == OK)
        vv = opt->tovar(&data);
#ifdef WITH_THREADS
    if (sp && CKTthreadId == 0) {
        // Mod/dev change with thread, save only primary thread
        // values.
#else
    if (sp) {
#endif

        sp->sp_inst = dev;
        sp->sp_mod = mod;
        sp->sp_job = 0;
        sp->sp_an = 0;
        sp->sp_parm = opt;
        sp->sp_isset = true;
        // caller should set error field
    }
    return (vv);
}


// As above, but doesn't handle "all" and returns an IFdata struct.
//
int
sCKT::getParam(const char *word, const char *param, IFdata *data,
    IFspecial *sp) const
{
    if (!word || !*word)
        return (E_NODEV);
    if (!param || !*param || lstring::cieq(param, "all"))
        return (E_BADPARM);
    sGENinstance *dev = 0;
    sGENmodel *mod = 0;
    char *nm = lstring::copy(word);
    insert(&nm);
    int typecode = finddev(nm, &dev, &mod);
    if (typecode < 0)
        return (E_NODEV);
    IFdevice *device = DEV.device(typecode);
    if (!device)
        return (E_NODEV);
    IFparm *opt = 0;
    if (dev)
        opt = device->findInstanceParm(param, IF_ASK);
    else if (mod)
        opt = device->findModelParm(param, IF_ASK);
    if (!opt)
        return (E_BADPARM);
    if (doask(data, dev, mod, opt) == OK) {
        // fill in the rest of the type field
        data->type |= opt->dataType & ~IF_VARTYPES;
#ifdef WITH_THREADS
        if (sp && CKTthreadId == 0) {
            // Mod/dev change with thread, save only primary thread
            // values.
#else
    if (sp) {
#endif

            sp->sp_inst = dev;
            sp->sp_mod = mod;
            sp->sp_job = 0;
            sp->sp_an = 0;
            sp->sp_parm = opt;
            sp->sp_isset = true;
            // caller should set error field
        }
        return (OK);
    }
    return (E_BADPARM);
}


int
sCKT::setParam(const char *word, const char *param, IFdata *data)
{
    sGENinstance *dev = 0;
    sGENmodel *mod = 0;
    if (!word || !*word)
        return (E_NODEV);
    if (!param || !*param || lstring::cieq(param, "all"))
        return (E_BADPARM);
    char *nm = lstring::copy(word);
    insert(&nm);
    int typecode = finddev(nm, &dev, &mod);
    if (typecode < 0)
        return (E_NODEV);
    IFdevice *device = DEV.device(typecode);
    if (!device)
        return (E_NODEV);
    IFparm *opt = 0;
    if (dev)
        opt = device->findInstanceParm(param, IF_ASK);
    else if (mod)
        opt = device->findModelParm(param, IF_ASK);
    if (!opt)
        return (E_BADPARM);

    // Need to ensure data type.
    if ((opt->dataType & IF_VARTYPES) != data->type) {
        if ((opt->dataType & IF_VARTYPES) == IF_PARSETREE &&
                data->type == IF_REAL) {
            // Assume that we can always pass a real instead of a
            // parse tree.
        }
        else if (((opt->dataType & IF_VARTYPES) == IF_REAL ||
                (opt->dataType & IF_VARTYPES) == IF_PARSETREE) &&
                data->type == IF_INTEGER) {
            double d = (double)data->v.iValue;
            data->v.rValue = d;
            data->type = IF_REAL;
        }
        else if ((opt->dataType & IF_VARTYPES) == IF_INTEGER &&
                data->type == IF_REAL) {
            int i = (int)data->v.rValue;
            data->v.iValue = i;
            data->type = IF_INTEGER;
        }
        else
            return (E_BADPARM);
    }

    if (dev) {
        int err = dev->setParam(opt->id, data);
        if (err)
            return (err);
    }
    else if (mod) {
        int err = mod->setParam(opt->id, data);
        if (err)
            return (err);
    }
    else
        return (E_BADPARM);

    return (OK);
}


variable *
sCKT::getAnalParam(const char *word, const char *param, IFspecial *sp) const
{
    if (!word || !*word)
        return (0);
    for (int i = 0; ; i++) {
        IFanalysis *an = IFanalysis::analysis(i);
        if (!an)
            break;
        if (lstring::cieq(an->name, word)) {
            sJOB *job = CKTcurTask->TSKjobs;
            for ( ; job; job = job->JOBnextJob) {
                if (job->JOBtype == i)
                    return (an->getOpt(this, job, param, sp));
            }
            break;
        }
    }
    return (0);
}


int
sCKT::getAnalParam(const char *word, const char *param, IFdata *data,
    IFspecial *sp) const
{
    if (!word || !*word)
        return (E_BADPARM);
    for (int i = 0; ; i++) {
        IFanalysis *an = IFanalysis::analysis(i);
        if (!an)
            return (E_BADPARM);
        if (lstring::cieq(an->name, word)) {
            sJOB *job = CKTcurTask->TSKjobs;
            for ( ; job; job = job->JOBnextJob) {
                if (job->JOBtype == i)
                    return (an->getOpt(this, job, param, data, sp));
            }
            break;
        }
    }
    return (0);
}
// End of sCKT functions.


// Set an instance parameter identified by number.
//
int
sGENinstance::setParam(int param, IFdata *data)
{
    if (!data)
        return (E_BADPARM);
    int type = GENmodPtr->GENmodType;
    if (type < 0 || type >= DEV.numdevs() || !DEV.device(type))
        return (E_NODEV);
    return (DEV.device(type)->setInst(param, data, this));
}


// Set an instance parameter identified by name.
//
int
sGENinstance::setParam(const char *pname, IFdata *data)
{
    if (!pname || !data)
        return (E_BADPARM);
    int type = GENmodPtr->GENmodType;
    if (type < 0 || type >= DEV.numdevs())
        return (E_NODEV);
    IFdevice *dev = DEV.device(type);
    if (!dev)
        return (E_NODEV);

    IFparm *p = dev->findInstanceParm(pname, IF_SET);
    if (p)
        return (dev->setInst(p->id, data, this));
    return (E_BADPARM);
}


// Obtain an instance parameter identified by number.
//
int
sGENinstance::askParam(const sCKT *ckt, int param, IFdata *data) const
{
    if (!data)
        return (E_BADPARM);
    int type = GENmodPtr->GENmodType;
    if (type < 0 || type >= DEV.numdevs() || !DEV.device(type))
        return (E_NODEV);
    return (DEV.device(type)->askInst(ckt, this, param, data));
}


// Obtain an instance parameter identified by name.
//
int
sGENinstance::askParam(const sCKT *ckt, const char *pname, IFdata *data) const
{
    if (!pname || !data)
        return (E_BADPARM);
    int type = GENmodPtr->GENmodType;
    if (type < 0 || type >= DEV.numdevs() || !DEV.device(type))
        return (E_NODEV);

    IFparm *p = DEV.device(type)->findInstanceParm(pname, IF_ASK);
    if (p)
        return (DEV.device(type)->askInst(ckt, this, p->id, data));
    return (E_BADPARM);
}
// End of sGENinstance functions.


// Set a model parameter identified by number.
//
int
sGENmodel::setParam(int param, IFdata *data)
{
    if (!data)
        return (E_BADPARM);
    int type = GENmodType;
    if (type < 0 || type >= DEV.numdevs() || !DEV.device(type))
        return (E_NOMOD);
    return (DEV.device(type)->setModl(param, data, this));
}


// Set a model parameter identified by name;
//
int
sGENmodel::setParam(const char *pname, IFdata *data)
{
    if (!pname || !data)
        return (E_BADPARM);

    int type = GENmodType;
    IFdevice *dev = DEV.device(type);
    if (dev) {
        IFparm *p = dev->findModelParm(pname, IF_SET);
        if (p)
            return (dev->setModl(p->id, data, this));
        return (E_BADPARM);
    }
    return (E_NOMOD);
}


// Obtain a model parameter identified by number;
//
int
sGENmodel::askParam(int param, IFdata *data) const
{
    if (!data)
        return (E_BADPARM);
    int type = GENmodType;
    if (type < 0 || type >= DEV.numdevs() || !DEV.device(type))
        return (E_NOMOD);
    return (DEV.device(type)->askModl(this, param, data));
}


// Obtain a model parameter identified by name;
//
int
sGENmodel::askParam(const char *pname, IFdata *data) const
{
    if (!pname || !data)
        return (E_BADPARM);
    int type = GENmodType;
    if (type < 0 || type >= DEV.numdevs() || !DEV.device(type))
        return (E_NOMOD);

    IFparm *p = DEV.device(type)->findModelParm(pname, IF_ASK);
    if (p)
        return (DEV.device(type)->askModl(this, p->id, data));
    return (E_BADPARM);
}
// End of sGENmodel functions.

