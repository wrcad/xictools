
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
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: devlib.cc,v 2.26 2016/02/13 22:09:47 stevew Exp $
 *========================================================================*/

//
// Support functions for device models
//

#include <ctype.h>
#include <stdio.h>
#include "distdefs.h"
#include "device.h"
#include "input.h"
#include "src/srcdefs.h"
#include "ind/inddefs.h"


// Load a new device descriptor, its allocation function is passed.  The
// return value is the number of device slots allocated:
//  0:  something went wrong
//  1:  all devices except inductors with mutual inductor support
//  2:  inductor with mutual inductor support
//
int
sDevLib::loadDev(NewDevFunc f)
{
    if (!f)
        return (0);
    if (dl_numdevs + 2 >= dl_size) {
        IFdevice **tmp = new IFdevice*[dl_size + 40];
        if (dl_numdevs > 0)
            memcpy(tmp, dl_devices, dl_numdevs*sizeof(IFdevice*));
        delete [] dl_devices;
        dl_devices = tmp;
        dl_size += 40;
    }
    int i = dl_numdevs;
    (*f)(dl_devices + i, &dl_numdevs);
    return (dl_numdevs - i);
}


// Remove the device at index i and free it.  Shift down the remaining
// devices to fill the "hole".
//
bool
sDevLib::unloadDev(const IFdevice *dv)
{
    for (int i = 0; i < dl_numdevs; i++) {
        if (dl_devices[i] == dv) {
            IFdevice *tmp = dl_devices[i];
            dl_numdevs--;
            for (int j = i; j < dl_numdevs; j++)
                dl_devices[j] = dl_devices[j+1];
            dl_devices[dl_numdevs] = 0;
            delete tmp;
            return (true);
        }
    }
    return (false);
}


// Given the character device key, return the device code, or -1 if
// key is bad.  Level is the device level to return.
//
int
sDevLib::keyCode(int key, int level)
{
    char c = key;
    if (!isalpha(c))
        return (-1);
    if (isupper(c))
        c = tolower(c);
    if (level < 0)
        level = 0;
    for (int i = 0; i < numdevs(); i++) {
        for (int j = 0; ; j++) {
            const IFkeys *k = device(i)->key(j);
            if (!k)
                break;
            char d = k->key;
            if (c == (isupper(d) ? tolower(d) : d)) {
                if (device(i)->level(0) <= 1 && level <= 1)
                    return (i);
                if (device(i)->level(0)) {
                    for (int l = 0; ; l++) {
                        int lev = device(i)->level(l);
                        if (lev == 0)
                            break;
                        if (lev == level)
                            return (i);
                    }
                }
                break;
            }
        }
    }
    return (-1);
}


namespace {
    struct TmpNodes
    {
        TmpNodes(int mn, int mx)
            {
                nodes = new sCKTnode*[mx + 1];
                ndnames = new char*[mx + 1];
                posns = new const char*[mx + 2 - mn];
            }

        ~TmpNodes()
            {
                delete [] nodes;
                delete [] ndnames;
                delete [] posns;
            }

        void free(int i)
            {
                delete [] ndnames[i];
                ndnames[i] = 0;
            }

        void termInsert(sCKT *ckt, int i)
            {
                ckt->termInsert(&ndnames[i], &nodes[i]);
            }

        char **ndnames;
        sCKTnode **nodes;
        const char **posns;
    };
}


// Generic parser for device lines
//  current         string containing device specification
//  type            index of device
//  minnodes        minimum number of nodes
//  maxnodes        maximum number of nodes
//  hasmod          true if device accepts a model
//  leadname        parameter name for leading number, or 0
//
// This handles parsing for most devices, including mosfets.  For variable
// node count devices, unused nodes are set to -1.  The SOI models expect
// this, but this may not be true in general.
//
void
sDevLib::parse(sCKT *ckt, sLine *current, int type, int minnodes, int maxnodes,
    bool hasmod, const char *leadname)
{
    const char *toofewmsg = "Error: too few nodes given for device.";

    const char *line = current->line();
    char *dname = IP.getTok(&line, true);
    if (!dname)
        return;
    ckt->insert(&dname);
    char dvkey[2];
    dvkey[0] = islower(*dname) ? toupper(*dname) : *dname;
    dvkey[1] = 0;

    // If the node count is variable, we assume that the device
    // requires a model.
    bool reqmod = (minnodes != maxnodes);
    if (reqmod && !hasmod) {
        IP.logError(current, "Error: variable node count but no model.");
        ckt->CKTnogo = true;
        return;
    }

    // Error handling:
    // If ckt->CKTnogo is set true, a fatal error is indicated.  The
    // circuit will be destroyed after printing the error messages. 
    // Otherwise, messages are simply warnings, and WRspice will
    // continue the operations.

    TmpNodes T(minnodes, maxnodes);

    int ncnt = 0;
    for ( ; ncnt < minnodes; ncnt++) {
        char *tok = IP.getTok(&line, true);
        if (!tok) {
            IP.logError(current, toofewmsg);
            ckt->CKTnogo = true;
            for (int i = 0; i < ncnt; i++)
                T.free(i);
            return;
        }
        T.ndnames[ncnt] = tok;
    }

    char *model = 0;         // the name of the model

    if (!hasmod) {
        for (int i = 0; i < ncnt; i++)
            T.termInsert(ckt, i);
        T.posns[0] = line;
    }
    else {
        for ( ; ncnt <= maxnodes; ncnt++) {
            T.posns[ncnt - minnodes] = line;
            char *tok = IP.getTok(&line, true);
            if (!tok)
                break;
            T.ndnames[ncnt] = tok;
        }
        T.posns[ncnt - minnodes] = line;

        // Look for the model.  Read back-to-front to allow the odd
        // case where a node name is a model name too.
        for (int i = ncnt - 1; i >= 2; i--) {

            if (IP.lookMod(T.ndnames[i])) {
                if (i < minnodes) {
                    IP.logError(current, toofewmsg);
                    ckt->CKTnogo = true;
                    for (int j = 0; j < ncnt; j++)
                        T.free(j);
                    return;
                }
                model = T.ndnames[i];
                T.ndnames[i] = 0;
                ckt->insert(&model);
                for (int j = i+1; j < ncnt; j++)
                    T.free(j);
                ncnt = i;
                break;
            }
        }

        const char *pstart = 0;
        if (model) {
            for (int i = 0; i < ncnt; i++)
                T.termInsert(ckt, i);
        }
        else {
            const char *badnodes[5];
            badnodes[0] = T.ndnames[ncnt-1];
            badnodes[1] = 0;
            badnodes[2] = 0;
            badnodes[3] = 0;
            badnodes[4] = 0;

            // The token following the last legit node was not a model
            // name, look ahead and see if a model can be found.  The
            // user may have too many nodes on the line for the
            // device.

            for (int n = 0; n < 3; n++) {
                char *tok = IP.getTok(&line, true);
                if (!tok)
                    break;
                if (IP.lookMod(tok)) {
                    // Ha!  found it.  We'll just ignore the extra
                    // nodes, but add a warning.
                    const char *msg =
                        "Warning: too many nodes given, ignoring";

                    int len = strlen(msg);
                    for (int i = 0; i < 5 && badnodes[i]; i++)
                        len += strlen(badnodes[i]) + 1;
                    char *tmp = new char[len + 1];
                    strcpy(tmp, msg);
                    char *t = tmp + strlen(tmp);
                    for (int i = 0; i < 5 && badnodes[i]; i++) {
                        *t++ = ' ';
                        strcpy(t, badnodes[i]);
                        while (*t)
                            t++;
                    }
                    current->errcat(tmp);
                    delete [] tmp;

                    model = tok;
                    ckt->insert(&model);
                    pstart = line;
                    for (int i = 0; i < maxnodes; i++)
                        T.termInsert(ckt, i);
                    break;
                }
                badnodes[n+1] = tok;
            }
            if (model) {
                ncnt--;
                delete [] T.ndnames[ncnt];
                T.ndnames[ncnt] = 0;
            }
            for (int i = 1; i < 5 && badnodes[i]; i++)
                delete [] badnodes[i];
        }

        // If a model wasn't found, we can't really tell if the tokens
        // beyond minnodes are node names or parameters.  We will
        // assume that they are parameters, i.e., parse only minnodes
        // nodes.

        if (model) {
            if (!pstart)
                line = T.posns[ncnt + 1 - minnodes];
        }
        else {
            if (reqmod) {
                // We need a model and didn't find one, fatal
                // error.

                IP.logError(current, "Error: undefined model.");
                ckt->CKTnogo = true;
                return;
            }
            line = T.posns[0];
            for (int i = 0; i < minnodes; i++)
                T.termInsert(ckt, i);
            for (int i = minnodes; i < ncnt; i++)
                T.free(i);
            ncnt = minnodes;
        }
    }

    sINPmodel *thismodel;
    bool def_ok = IP.getMod(current, ckt, model, line, &thismodel);
    int error;
    sGENmodel *mdfast;   // pointer to the actual model
    if (thismodel != 0) {
        int ntype = thismodel->modType;
        if (ntype != type && !IP.checkKey(*dname, ntype)) {
            current->errcat("Error: Incorrect model type.");
            ckt->CKTnogo = true;
            return;
        }
        char *mname = new char[strlen(thismodel->modName) + 1];
        strcpy(mname, thismodel->modName);
        ckt->insert(&mname);
        sGENmodel *mdf = 0;
        error = ckt->findModl(&ntype, &mdf, mname);
        if (error == 0)
            mdfast = mdf;
        else {
            IP.logError(current, error);
            mdfast = 0;
        }
    }
    else {
        if (def_ok) {
            sCKTmodItem *mx = ckt->CKTmodels.find(type);
            if (mx && mx->default_model)
                mdfast = mx->default_model;
            else {
                // create default model
                IFuid uid;
                ckt->newUid(&uid, 0, dvkey, UID_MODEL);
                sGENmodel *m;
                error = ckt->newModl(type, &m, uid);
                IP.logError(current, error);
                if (!mx) {
                    // newModl should have created this
                    mx = ckt->CKTmodels.find(type);
                }
                if (mx)
                    mx->default_model = m;
                mdfast = m;
            }
        }
        else {
            current->errcat("Error: Unable to resolve model.");
            ckt->CKTnogo = true;
            return;
        }
    }
    sGENinstance *fast;  // pointer to the actual instance
    IP.logError(current, ckt->newInst(mdfast, &fast, dname));

    for (int i = 0; i < ncnt; i++)
        IP.logError(current, T.nodes[i]->bind(fast, i+1));

    // set unused nodes to -1
    int *nodeptr = fast->nodeptr(1);
    if (!nodeptr) {
        IP.logError(current, "Internal error: can't bind nodes!");
        ckt->CKTnogo = true;
        return;
    }
    for (int i = ncnt; i < maxnodes; i++)
        nodeptr[i] = -1;

    IP.devParse(current, &line, ckt, fast, leadname);
}


// Return true on version mismatch if the given version is newer than
// ref version.  Both are expected to be in the form
// "major.minor.release".
//
bool
sDevLib::checkVersion(const char *refVers, const char *givenVers)
{
    if (!givenVers)
        return (false);
    int major, minor, release;
    int n = sscanf(refVers, "%d.%d.%d", &major, &minor, &release);

    int gmajor, gminor, grelease;
    int gn = sscanf(givenVers, "%d.%d.%d", &gmajor, &gminor, &grelease);

    if (n >= 1 && gn >= 1) {
        if (gmajor > major)
            return (true);
        if (gmajor < major)
            return (false);
    }
    if (n >= 2 && gn >= 2) {
        if (gn == 2 && gminor >= 10) {
            // Deal with things like "version 4.30" which should actually
            // be 4.3.0 (IBM 9sf).
            gn = 3;
            grelease = gminor % 10;
            gminor /= 10;
        }
        if (gminor > minor)
            return (true);
        if (gminor < minor)
            return (false);
    }
    if (n == 3 && gn == 3) {
        if (grelease > release)
            return (true);
        if (grelease < release)
            return (false);
    }
    return (false);
}


// Limit the per-iteration change of VDS.
//
double
sDevLib::limvds(double vnew, double vold)
{
    if (vold >= 3.5) {
        if(vnew > vold)
            vnew = SPMIN(vnew, 3*vold + 2);
        else {
            if (vnew < 3.5)
                vnew = SPMAX(vnew, 2);
        }
    }
    else {
        if(vnew > vold)
            vnew = SPMIN(vnew, 4);
        else
            vnew = SPMAX(vnew, -.5);
    }
    return (vnew);
}


// Limit the per-iteration change of PN junction voltages.
//
double
sDevLib::pnjlim(double vnew, double vold, double vt, double vcrit, int *icheck)
{
    // This code has been fixed by Alan Gillespie adding limiting
    // for negative voltages.

    if (vnew > vcrit && fabs(vnew - vold) > (vt + vt)) {
        if (vold > 0) {
            double arg = (vnew - vold)/vt;
            if (arg > 0)
                // AG had 2.0 here, worry about too close to log(0).
                vnew = vold + vt*(2.0 + log(arg - 1.8));
            else
                vnew = vold - vt*(2.0 + log(-arg + 2.0));
        }
        else
            vnew = vt*log(vnew/vt);
        *icheck = 1;
    }
    else {
        if (vnew < 0) {
            double arg;
            if (vold > 0)
                arg = -1*vold - 1;
            else
                arg = 2*vold - 1;
            if (vnew < arg) {
                vnew = arg;
                *icheck = 1;
            }
            else
                *icheck = 0;
        }
        else
            *icheck = 0;
    }
    return (vnew);

/* Original Spice3 code
    double dt = vnew - vold;
    if (dt < 0.0)
        dt = -dt;
    if (vnew > vcrit && dt > (vt+vt)) {
        if (vold > 0) {
            double arg = 1.0 + (vnew - vold)/vt;
            if (arg > 0)
                vnew = vold + vt*log(arg);
            else
                vnew = vcrit;
        }
        else
            vnew = vt*log(vnew/vt);
        *icheck = 1;
    }
    else
        *icheck = 0;
    return (vnew);
*/
}


// Limit the per-iteration change of FET voltages.
//
double
sDevLib::fetlim(double vnew, double vold, double vto)
{
    // This code has been fixed by Alan Gillespie: a new
    // definition for vtstlo.

    double tmp = (vold - vto);
    if (tmp < 0.0)
        tmp = -tmp;
    double vtsthi = tmp + tmp + 2.0;
    double vtstlo = tmp + 1.0;
/*
    double vtstlo = vtsthi/2.0 + 2.0;
*/
    double vtox = vto + 3.5;
    double delv = vnew - vold;
    if (vold >= vto) {
        if (vold >= vtox) {
            if (delv <= 0.0) {
                // going off
                if (vnew >= vtox) {
                    if (-delv >vtstlo)
                        vnew =  vold - vtstlo;
                }
                else
                    vnew = SPMAX(vnew,vto+2);
            }
            else {
                // staying on
                if (delv >= vtsthi)
                    vnew = vold + vtsthi;
            }
        }
        else {
            // middle region
            if (delv <= 0)
                // decreasing
                vnew = SPMAX(vnew, vto - 0.5);
            else
                // increasing
                vnew = SPMIN(vnew, vto + 4.0);
        }
    }
    else {
        // off
        if (delv <= 0) {
            if (-delv > vtsthi)
                vnew = vold - vtsthi;
        }
        else {
            double vtemp = vto + .5;
            if (vnew <= vtemp) {
                if (delv > vtstlo)
                    vnew = vold + vtstlo;
            }
            else
                vnew = vtemp;
        }
    }
    return (vnew);
}


// Support core for the limexp/$limexp Verilog-A function.
//
double
sDevLib::limexp(double vnew, double vold, int *plimit)
{
// sqrt(e) - 0.5, so log(SQRT_E_MP5 + 0.5) = 0.5
#define SQRT_E_MP5   1.148721271

    *plimit = false;
    if ((vnew > 1.0) && (fabs(vnew - vold) > 0.5)) {
        if (vold >= 0.0) {
            if (vold < 0.5)
                vnew = 0.9;
            else {
                double arg = vnew - vold;
                if (arg > 0)
                    vnew = vold + log(SQRT_E_MP5 + arg);
                else
                    vnew = vold - log(SQRT_E_MP5 - arg);
            }
        }
        else
            vnew = log(vnew);
        *plimit = true;
    }
    return (vnew);
}


// Special interface functions called from WRspice.  These avoid using
// device header files in WRspice.


// Find the highest and lowest power supply voltages for clipping to
// aid convergence in dcop.
//
bool
sDevLib::getMinMax(sCKT *ckt, double *vmin, double *vmax)
{
    sGENmodel *gen_model = 0;
    if (ckt->typelook("Source", &gen_model) >= 0) {

        double amax = 0.0;
        double amin = 0.0;
        double afloat = 0.5;
        for (sSRCmodel *model = (sSRCmodel*)gen_model; model;
                model = model->next()) {
            sSRCinstance *inst;
            for (inst = model->inst(); inst; inst = inst->next()) {
                if (inst->SRCtype != SRC_V || inst->SRCccCoeffGiven ||
                        inst->SRCvcCoeffGiven ||
                        (inst->SRCtree && inst->SRCtree->num_vars() > 0))
                    return (false);
                if (inst->SRCdcValue == 0.0)
                    continue;

                double vs;
                if (inst->SRCnegNode == 0 && inst->SRCposNode > 0)
                    vs = inst->SRCdcValue;
                else if (inst->SRCnegNode > 0 && inst->SRCposNode == 0)
                    vs = -inst->SRCdcValue;
                else {
                    vs = inst->SRCdcValue;
                    if (vs < 0.0)
                        vs = -vs;
                    // assume worst case
                    afloat += vs;
                    continue;
                }
                if (amax < vs)
                    amax = vs;
                if (amin > vs)
                    amin = vs;
            }
        }
        *vmax = amax + afloat;
        *vmin = amin - afloat;
        return (true);
    }
    return (false);
}


// Set up source distortion parameters.
//
void
sDevLib::distoSet(sCKT *ckt, int mode, sDISTOAN *cv)
{
    sGENmodel *gen_model = 0;
    if (ckt->typelook("Source", &gen_model) >= 0) {
        for (sSRCmodel *model = (sSRCmodel*)gen_model; model;
                model = model->next()) {
            sSRCinstance *inst;
            for (inst = model->inst(); inst; inst = inst->next()) {
        
                // check if the source has a distortion input   

                if (inst->SRCdGiven) {
                    if (inst->SRCdF2given)
                        cv->Df2given = 1;
                    double mag = 0, phase = 0;
                    if ((inst->SRCdF1given) && (mode == D_RHSF1)) {
                        mag = inst->SRCdF1mag;  
                        phase = inst->SRCdF1phase;
                    }
                    else if ((inst->SRCdF2given) && (mode == D_RHSF2)) {
                        mag = inst->SRCdF2mag;
                        phase = inst->SRCdF2phase;
                    }
                    if (((inst->SRCdF1given) && (mode == D_RHSF1)) || 
                            ((inst->SRCdF2given) && (mode == D_RHSF2))) {
                        if (inst->SRCtype == SRC_V) {
                            *(ckt->CKTrhs + inst->SRCbranch) =
                                0.5*mag* cos(M_PI*phase/180.0);
                            *(ckt->CKTirhs + inst->SRCbranch) =
                                0.5*mag*sin(M_PI*phase/180.0);
                        }
                        else {
                            *(ckt->CKTrhs + inst->SRCposNode) =
                                - 0.5*mag*cos(M_PI*phase/180.0);
                            *(ckt->CKTrhs + inst->SRCnegNode) =
                                0.5*mag*cos(M_PI*phase/180.0);
                            *(ckt->CKTirhs + inst->SRCposNode) =
                                - 0.5*mag*sin(M_PI*phase/180.0);
                            *(ckt->CKTirhs + inst->SRCnegNode) =
                                0.5*mag*sin(M_PI*phase/180.0);
                        }
                    }
                }
            }
        }
    }
}


// Zero all inductor currents in CKTrhsOld.
//
void
sDevLib::zeroInductorCurrent(sCKT *ckt)
{
    sGENmodel *gen_model = 0;
    if (ckt->typelook("Inductor", &gen_model) >= 0) {
        for (sINDmodel *model = (sINDmodel*)gen_model; model;
                model = model->next()) {
            sINDinstance *inst;
            for (inst = model->inst(); inst; inst = inst->next())
                ckt->CKTrhsOld[inst->INDbrEq] = 0.0;
        }   
    }
}

