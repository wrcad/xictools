
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

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

// Enable TJM device support.
#define TJM_IF

#include <math.h>
#include "config.h"
#include "simulator.h"
#include "output.h"
#include "device.h"
#include "inpptree.h"
#include "input.h"
#include "ttyio.h"
#include "wlist.h"
#include "uidhash.h"
#include "verilog.h"
#include "commands.h"
#include "graph.h"
#include "sparse/spmatrix.h"
#include "spnumber/hash.h"
#include "miscutil/errorrec.h"
#include "miscutil/pathlist.h"
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifdef HAVE_FENV_H
#include <fenv.h>
#endif
#ifndef DBL_MAX
#define DBL_MAX     1.79769313486231e+308
#endif
#ifdef HAVE_GETRUSAGE
#include <sys/resource.h>
#endif
#include "fpe_check.h"  // for check_fpe()

#ifdef WITH_THREADS

// A complicated but more efficient thread job queue system. 
// Uncomment for the original system.
#define NEW_THREAD_QUEUE

// #define TDEBUG
#include "miscutil/threadpool.h"
#ifdef __APPLE__
os_unfair_lock_s sCKT::CKTloadLock2;
#else
pthread_spinlock_t sCKT::CKTloadLock2;
#endif
#ifndef WITH_ATOMIC
pthread_spinlock_t sCKT::CKTloadLock1;
pthread_spinlock_t sCKT::CKTloadLock3;
#endif
#ifndef THREAD_SAFE_EVAL
pthread_mutex_t sCKT::CKTloadLock4 = PTHREAD_MUTEX_INITIALIZER;
#endif


namespace {
    struct _stupid_init
    {
        _stupid_init()
            {
#ifdef __APPLE__
#else
                pthread_spin_init(&sCKT::CKTloadLock2, 0);
#endif
#ifndef WITH_ATOMIC
                pthread_spin_init(&sCKT::CKTloadLock1, 0);
                pthread_spin_init(&sCKT::CKTloadLock3, 0);
#endif
            }
    } _stupid;

#ifdef NEW_THREAD_QUEUE
    struct sInstBatch
    {
        // Variable length, no constructor/destructor.

        static sInstBatch *new_batch(sCKT *c, int sz)
            {
                if (sz < 0)
                    return (0);
                int size = sizeof(sInstBatch) + (sz-1)*sizeof(sGENinstance*);
                sInstBatch *b = (sInstBatch*)malloc(size);
                memset(b, 0, size);
                b->b_ckt = c;
                return (b);
            }

        void append(sGENinstance *d)
            {
                b_list[b_count++] = d;
            }

        sCKT *ckt()                 { return (b_ckt); }
        int count()                 { return (b_count); }
        sGENinstance *list(int i)   { return (b_list[i]); }

    private:
        sCKT *b_ckt;
        int b_count;
        sGENinstance *b_list[1];
    };

    struct sBatchList
    {
        sBatchList(sInstBatch *b, sBatchList *n)
            {
                next = n;
                batch = b;
            }

        sBatchList *next;
        sInstBatch *batch;
    };
#else

#define BATCHNO 16

    struct sInstBatch
    {
        sInstBatch(sCKT *c)
            {
                b_ckt = c;
                memset(b_list, 0, BATCHNO*sizeof(sGENinstance*));
            };

        sCKT *ckt()                 { return (b_ckt); }
        int count()                 { return (BATCHNO); }
        sGENinstance *list(int i)   { return (b_list[i]); }
        void set_list(sGENinstance *g, int i)   { b_list[i] = g; }

    private:
        sCKT *b_ckt;
        sGENinstance *b_list[BATCHNO];
    };
#endif


    // Thread work procedure.
    //
    int thread_proc(sTPthreadData*, void *arg)
    {
        sInstBatch *b = (sInstBatch*)arg;
        for (int i = 0; i < b->count(); i++) {
            sGENinstance *d = b->list(i);
            if (!d)
                break;
            sGENmodel *m = d->GENmodPtr;
            int error = DEV.device(m->GENmodType)->load(d, b->ckt());
            if (error != OK && error != LOAD_SKIP_FLAG) {
                // Shouldn't see the skip flag.
                return (error);
            }
        }
        return (0);
    }


    // Cleanup procedure for thread user data.
    //
    void destroy_proc(void *arg)
    {
        free((sInstBatch*)arg);
    }
}

#endif // WITH_THREADS


sGENmodel::~sGENmodel()
{
    delete GENinstTab;
}
// End of sGENmodel functions.

//#define CKT_DEBUG

int sCKT::CKTstepDebug = 0;

sCKT::sCKT() : sCKTPOD()
{
    CKTsrcFact = 1.0;
    CKTorder = 1;
    CKTstat = new sSTATS();
    CKTnodeTab.reset();
    NIinit();
#ifdef CKT_DEBUG
    printf("new ckt %lx\n", (unsigned long)this);
#endif

    extern bool CKThasVA;
    CKThasVA = true;
}


// Destructor
//
sCKT::~sCKT()
{
#ifdef CKT_DEBUG
    printf("delete ckt %lx\n", (unsigned long)this);
#endif
    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next())
        DEV.device(m->GENmodType)->destroy(&m);
    for (int i = 0; i < 8; i++)
        delete [] CKTstates[i];

    delete [] CKTtemps;
    delete [] CKTtimePoints;
    clrTable();
    delete CKTvblk;
    delete CKTstat;
    NIdestroy();

    sHgen gen(CKTmacroTab, true);
    sHent *ent;
    while ((ent = gen.next()) != 0) {
        delete (IFmacro*)ent->data();
        delete ent;
    }
    delete CKTmacroTab;

    delete CKTloadPool;

    if (CKTbackPtr && CKTbackPtr->runckt() == this)
        CKTbackPtr->set_runckt(0);
}


// This is a simple program to dump the complex rhs vector into the
// rawfile.
//
void
sCKT::acDump(double freq, sRunDesc *run)
{
    if (!CKTcurJob)
        return;
    sOUTdata *outd = CKTcurJob->JOBoutdata;
    if (!outd)
        return;

    double *rhsold = CKTrhsOld;
    double *irhsold = CKTirhsOld;
    IFvalue freqData;
    freqData.rValue = freq;
    IFvalue valueData;
    int topeq = CKTnodeTab.numNodes() - 1;
    valueData.v.numValue = topeq;
    IFcomplex *data = new IFcomplex[topeq];
    valueData.v.vec.cVec = data;
    for (int i = 0; i < topeq; i++) {
        data[i].real = rhsold[i+1];
        data[i].imag = irhsold[i+1];
    }

    if (outd->cycle > 0) {
        // Analysis is multi-threaded, we're computing one cycle. 
        // Compute the actual offset and insert data at that location.

        unsigned int os = (outd->cycle - 1)*outd->numPts + outd->count;
        OP.insertData(this, run, &freqData, &valueData, os);
    }
    else {
        // Single-thread.

        OP.appendData(run, &freqData, &valueData);
        OP.checkRunops(run, freq);
    }
    delete [] data;
    outd->count++;
}


// This is a driver program to iterate through all the various ac load
// functions provided for the circuit elements in the given circuit.
//
int
sCKT::acLoad()
{
    int size = CKTmatrix->spGetSize(1);
    for (int i = 0; i <= size; i++) {
        *(CKTrhs+i) = 0;
        *(CKTirhs+i) = 0;
    }

    CKTmatrix->spSetComplex();
    if (CKTmatrix->spDataAddressChange()) {
        int error = resetup();
        if (error)
            return (error);
    }
    CKTmatrix->spClear();

    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        int error = DEV.device(m->GENmodType)->acLoad(m, this);
        if (error)
            return (error);
    }
    return (OK);
}


// This is a driver program to iterate through all the various accept
// functions provided for the circuit elements in the given circuit.
//
int
sCKT::accept()
{
    // This is optionally set in the device code to limit next step
    // size.  This is used by the JJ model (set in accept()), and by
    // the $bound_step() Verilog-A function (set in load()).  The
    // value is used to limit the next time step.
    //
    CKTdevMaxDelta = 0.0;

    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        int error = DEV.device(m->GENmodType)->accept(this, m);
        if (error)
            return (error);
    }

    // Rotate the sols vectors.
    double *tmp = CKTsols[7];
    int i;
    for (i = 7; i > 0; i--)
        CKTsols[i] = CKTsols[i-1];
    CKTsols[0] = tmp;
    int size = CKTmatrix->spGetSize(1);
    // CKTrhsOld contains the last solution.
    for (i = 0; i <= size; i++)
        CKTsols[0][i] = CKTrhsOld[i];
    return (OK);
}


// This method simply hides the sparse matrix system from the device
// library.
//
double *
sCKT::alloc(int i, int j)
{
    return (CKTmatrix->spGetElement(i, j));
}


// Delete the first time from the breakpoint table for the given
// circuit.
//
int
sCKT::breakClr()
{
    CKTlattice.clear_break(CKTtime, CKTcurTask->TSKminBreak, CKTbreaks);
    if (Sp.GetFlag(FT_SIMDB))
        TTY.err_printf("clear breakpoint: now %g, %g, num = %d\n", *CKTbreaks,
            *(CKTbreaks+1), CKTlattice.numbreaks());
    return (OK);
}


// Initialize the breakpoint table.
//
int
sCKT::breakInit()
{
    CKTlattice.init();
    breakSet(0.0);
    return (OK);
}


// Add the given time to the breakpoint table for the given circuit.
//
int
sCKT::breakSet(double time)
{
    bool added = CKTlattice.set_break(time, CKTtime, CKTcurTask->TSKminBreak,
        CKTbreaks);
    if (added && Sp.GetFlag(FT_SIMDB)) {
        TTY.err_printf("adding breakpoint %g: num = %d\n", time,
            CKTlattice.numbreaks());
    }
    return (OK);
}


// Set up a periodic breakpoint.
//
int
sCKT::breakSetLattice(double offs, double per)
{
    CKTlattice.set_lattice(offs, per);
    if (Sp.GetFlag(FT_SIMDB))
        TTY.err_printf("adding breakpoint lattice %g, %g\n", offs, per);
    return (OK);
}


// Clear the tables.
//
void
sCKT::clrTable()
{
    sCKTtable *t, *tn;
    for (t = (sCKTtable*)CKTtableHead; t; t = tn) {
        tn = t->next();
        delete t;
    }
    CKTtableHead = 0;
}


// This is a driver program to iterate through all the various
// convTest functions provided for the circuit elements in the given
// circuit.
//
int
sCKT::convTest()
{
    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        int error = DEV.device(m->GENmodType)->convTest(m, this);
        if (error)
            return (error);
        if (CKTnoncon)
            return (OK);
    }
    return (OK);
}


// Delete an instance.
//
int
sCKT::delInst(sGENmodel *model, IFuid uid, sGENinstance *inst)
{
    if (!inst && !uid)
        return (E_BADPARM);
    if (!model) {
        if (!inst) {
            int type = -1;
            int err = findInst(&type, &inst, uid, 0, 0);
            if (err)
                return (err);
        }
        model = inst->GENmodPtr;
    }
    return (DEV.device(model->GENmodType)->delInst(model, uid, inst));
}


// Delete a device model, and all of its instances.  At least one of
// fast or moduid must be given.  If modelp is given, it will be used
// to search for the model to delete.
//
int
sCKT::delModl(sGENmodel **modelp, IFuid moduid, sGENmodel *fast)
{
    if (!fast && !moduid)
        return (E_BADPARM);
    if (!modelp) {
        if (!fast) {
            int type = -1;
            int err = findModl(&type, &fast, moduid);
            if (err != OK)
                return (err);
        }
        sCKTmodItem *mx = CKTmodels.find(fast->GENmodType);
        if (mx)
            modelp = &mx->head;
    }
    return (DEV.device((*modelp)->GENmodType)->delModl(modelp, moduid, fast));
}


// Perform the analyses set in the CKTcurTask field.
//
int
sCKT::doTask(bool reset)
{
    double startTime = OP.seconds();
    CKTtroubleNode = 0;
    CKTtroubleElt = 0;
    CKTtranTrace = Sp.GetTranTrace();

    if (!CKTcurTask) {
        // Nothing to do, just return.
        return (OK);
    }
    if (CKTnodeTab.numNodes() <= 1) {
        // Circuit has nothing in it! Just return.
        return (OK);
    }

    // The mutual inductor instances must be loaded before the inductors.
    // Cache the mod head.
    typelook("mutual", &CKTmutModels);

    for (int i = 0; i < IFanalysis::numAnalyses(); i++) {
        for (sJOB *job = CKTcurTask->TSKjobs; job; job = job->JOBnextJob) {
            if (job->JOBtype == i) {
                int error;
                if (reset) {
                    CKTcurJob = job;
                    try {
                        error = doTaskSetup();
                    }
                    catch (int e) {
                        error = e;
                    }
                    if (error != OK)
                        return (error);
                    GP.Checkup();
                    if (Sp.GetFlag(FT_INTERRUPT))
                        return (E_PAUSE);
                }
                else if (CKTcurJob != job)
                    continue;

                // Set the "curanalysis" variable.
                Sp.SetCurAnalysis(IFanalysis::analysis(i));

                // Set the FPE mode to the value set in the .options,
                // if set there.
                FPEmode fpemode_bak = Sp.SetCircuitFPEmode();
#ifdef HAVE_GETRUSAGE
                struct rusage ruse, ruse2;
                getrusage(RUSAGE_SELF, &ruse);
#endif
#ifdef WITH_THREADS
                CKTstat->STATloadThreads = 0;
                CKTstat->STATloopThreads = 0;
#endif
                CKTstat->STATruns++;
                try {
                    error = IFanalysis::analysis(i)->anFunc(this, reset);
                }
                catch (int e) {
                    // The Verilog finish call sends e = E_PANIC.
                    error = e;
                }
#ifdef HAVE_GETRUSAGE
                getrusage(RUSAGE_SELF, &ruse2);
                CKTstat->STATpageFaults = ruse2.ru_majflt - ruse.ru_majflt;
                CKTstat->STATvolCxSwitch = ruse2.ru_nvcsw - ruse.ru_nvcsw;
                CKTstat->STATinvolCxSwitch = ruse2.ru_nivcsw - ruse.ru_nivcsw;
#endif
#ifdef WITH_THREADS
                delete CKTloadPool;
                CKTloadPool = 0;
#endif

                // Reset FPE mode.
                Sp.SetFPEmode(fpemode_bak);

                if (error != E_PAUSE)
                    DVO.cleanup();
                if (error != OK) {
                    CKTstat->STATtotAnalTime += OP.seconds() - startTime;
                    return (error);
                }
                if (CKTbackPtr && CKTbackPtr->postrunBlk().text()) {
                    // Execute the .postrun commands.

                    Sp.ExecCmds(CKTbackPtr->postrunBlk().text());
                }
                if (!reset)
                    reset = true;
            }
        }
    }
    CKTstat->STATtotAnalTime += OP.seconds() - startTime;
    return (OK);
}


int
sCKT::doTaskSetup()
{
    CKTdelta = 0.0;
    CKTtime = 0.0;

    CKTpreload = 1; // do preload
    // normal reset
    DVO.cleanup();
    int error = setup();
    if (error)
        return (error);
    error = temp();
    if (error)
        return (error);

    // Check to make sure that the node has a mapping
    // to the matrix, and check the diagonal element. 
    // Unconnected current sources can produce nodes
    // without a mapping.  A node used as a reference
    // input to a controlled source (for example) that
    // is not tied to anything will not have a
    // diagonal element.  However, series connected
    // voltage sources and/or inductors also produce
    // nodes without digonals, so the checking has to
    // account for this.

    const sCKTnode *node = CKTnodeTab.find(1);
    for ( ; node; node = CKTnodeTab.nextNode(node)) {
        if (!CKTmatrix->spCheckNode(node->number(), this)) {
            OP.error(ERR_FATAL, "Node %s: %s",
                (const char*)node->name(),
                Errs()->get_error());
            return (E_NODECON);
        }
    }

    CKTmatrix->spSaveForInitialization();
    if (CKTmatrix->spDataAddressChange()) {
        // We're using KLU, so all of the pointers into the
        // original matrix are bogus.  Call resetup to
        // get the new pointers.

        error = resetup();
        if (error)
            return (error);
    }
    return (OK);
}


// This is a simple function to dump the rhs vector to stdout.
//
void
sCKT::dump(double ref, sRunDesc *run)
{
    if (!CKTcurJob)
        return;
    sOUTdata *outd = CKTcurJob->JOBoutdata;
    if (!outd)
        return;

    IFvalue refData;
    refData.rValue = ref;
    IFvalue valData;
    valData.v.numValue = CKTnodeTab.numNodes() - 1;
    valData.v.vec.rVec = CKTrhsOld+1;

    if (outd->cycle > 0) {
        // Analysis is multi-threaded, we're computing one cycle. 
        // Compute the actual offset and insert data at that location.

        unsigned int os = (outd->cycle - 1)*outd->numPts + outd->count;
        OP.insertData(this, run, &refData, &valData, os);
    }
    else {
        // Single-thread.

        OP.appendData(run, &refData, &valData);
        OP.checkRunops(run, ref);
    }
    outd->count++;
}


// Output a list of all node name UIDs.
//
int
sCKT::names(int *numNames, IFuid **nameList)
{
    *numNames = CKTnodeTab.numNodes() - 1;
    *nameList = new IFuid[*numNames];
    int i = 0;
    sCKTnode *node = CKTnodeTab.find(1);
    for ( ; node; node = CKTnodeTab.nextNode(node))
        *((*nameList) + i++) = node->name();
    return (OK);
}


// Return a listing of the nodes and their values from rhsOld.  User must
// free the returned array.
//
int
sCKT::nodeVals(sCKTnodeVal **nvp) const
{
    unsigned int vsize = 256;
    sCKTnodeVal *nds = new sCKTnodeVal[vsize];
    unsigned int nv = 0;
    const sCKTnode *node = CKTnodeTab.find(1);
    for ( ; node; node = CKTnodeTab.nextNode(node)) {
        if (nv == vsize) {
            sCKTnodeVal *tmp = new sCKTnodeVal[vsize + vsize];
            memcpy(tmp, nds, vsize*sizeof(sCKTnodeVal));
            delete [] nds;
            nds = tmp;
            vsize += vsize;
        }
        nds[nv].name = (const char*)node->name();
        nds[nv].value = CKTrhsOld ? CKTrhsOld[node->number()] : 0.0;
        nv++;
    }

    if (nv) {
        *nvp = nds;
        return (nv);
    }
    delete [] nds;
    return (0);
}


// This is a driver program to iterate through all the various
// findBranch functions provided for the circuit elements in the given
// circuit.
//
int
sCKT::findBranch(IFuid name)
{
    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        int error = DEV.device(m->GENmodType)->findBranch(this, m, name);
        if (error)
            return (error);
    }
    return (0);
}


// Find a device.
//
int
sCKT::findInst(int *type, sGENinstance **fast, IFuid name, sGENmodel *modfast,
    IFuid modname) const
{
    if (fast && *fast) {
        // already have  fast, so nothing much to do
        // just get & set type
        if (type)
            *type = (*fast)->GENmodPtr->GENmodType;
        return (OK);
    } 
    if (fast)
        *fast = 0;
    if (modfast) {
        // Have model, just need device.
        if (modfast->GENinstTab) {
            sGENinstance *inst = modfast->GENinstTab->find(name);
            if (inst) {
                if (fast)
                    *fast = inst;
                if (type)
                    *type = modfast->GENmodType;
                return (OK);
            }
        }
        return(E_NODEV);
    }
    if (type && *type >= 0 && *type < DEV.numdevs()) {
        // have device type, need to find model & device
        sCKTmodItem *mx = CKTmodels.find(*type);
        if (mx) {
            // look through all models...
            for (sGENmodel *mod = mx->head; mod; mod = mod->GENnextModel) {
                // ...and all instances
                if (modname == 0 || mod->GENmodName == modname) {

                    if (mod->GENinstTab) {
                        sGENinstance *inst = mod->GENinstTab->find(name);
                        if (inst) {
                            if (fast)
                                *fast = inst;
                            return (OK);
                        }
                    }
                    if (mod->GENmodName == modname)
                        return (E_NODEV);
                }
            }
        }
        return (E_NODEV);
    }
    if (!type || *type == -1) {
        // need to find model & device
        // look through all types (worst case)
        sCKTmodGen mgen(CKTmodels);
        for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
            // look through all models...
            for (sGENmodel *mod = m; mod; mod = mod->GENnextModel) {
                // ...and all instances
                if (modname == 0 || mod->GENmodName == modname) {

                    if (mod->GENinstTab) {
                        sGENinstance *inst = mod->GENinstTab->find(name);
                        if (inst) {
                            if (fast)
                                *fast = inst;
                            if (type)
                                *type = mod->GENmodType;
                            return (OK);
                        }
                    }
                    if (mod->GENmodName == modname)
                        return (E_NODEV);
                }
            }
        }
        return (E_NODEV);
    }
    return (E_BADPARM);
}


// Find a model.
//
int
sCKT::findModl(int *type, sGENmodel **modfast, IFuid modname) const
{
    if (modfast && *modfast) {
        // already have  modfast, so nothing to do
        if (type)
            *type = (*modfast)->GENmodType;
        return (OK);
    } 
    if (type && *type >= 0 && *type < DEV.numdevs()) {
        // have device type, need to find model
        // look through all models
        sCKTmodItem *mx = CKTmodels.find(*type);
        if (mx) {
            for (sGENmodel *mod = mx->head; mod; mod = mod->GENnextModel) {
                if (mod->GENmodName == modname) {
                    if (modfast)
                        *modfast = mod;
                    return (OK);
                }
            }
        }
        return (E_NOMOD);
    }
    if (!type || *type == -1) {
        // need to find model & device
        // look through all types (worst case)
        sCKTmodGen mgen(CKTmodels);
        for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
            for (sGENmodel *mod = m; mod; mod = mod->GENnextModel) {
                if (mod->GENmodName == modname) {
                    if (modfast)
                        *modfast = mod;
                    if (type)
                        *type = mod->GENmodType;
                    return (OK);
                }
            }
        }
        return (E_NOMOD);
    }
    return (E_BADPARM);
}


// This is called just before op() from the analysis driver functions.
// The RHS values are used only in transient analysis when UIC is set.
//
int
sCKT::ic()
{
    int size = CKTmatrix->spGetSize(1);
    for (int i = 0; i <= size; i++)
        *(CKTrhs + i) = 0;
    const sCKTnode *node = CKTnodeTab.find(1);
    for ( ; node; node = CKTnodeTab.nextNode(node)) {
        if (node->nsGiven()) {
            CKThadNodeset = 1;
            *(CKTrhs + node->number()) = node->nodeset();
        }
        if (node->icGiven())
            *(CKTrhs + node->number()) = node->ic();
    }
    return (OK);
}


// Get the name and node pointer for a node given a device it is
// bound to and the terminal of the device.
//
int
sCKT::inst2Node(sGENinstance *instPtr, int term, sCKTnode **node,
    IFuid *node_name) const
{
    int type = instPtr->GENmodPtr->GENmodType;
    char key = *(char*)instPtr->GENname;

    IFkeys *keys = DEV.device(type)->keyMatch(key);
    if (!keys)
        return (E_NOTERM);

    if (keys->maxTerms >= term && term > 0) {
        int *nodeptr = instPtr->nodeptr(term);
        if (!nodeptr)
            return (E_NOTFOUND);
        // ok, now we know its number, so we just have to find it
        sCKTnode *n = CKTnodeTab.find(*nodeptr);
        if (n) {
            *node = n;
            *node_name = n->name();
            return (OK);
        }
        return (E_NOTFOUND);
    }
    return (E_NOTERM);
}


// This is a driver program to iterate through all the various load
// functions provided for the circuit elements in the given circuit.
//
int
sCKT::load(bool noclear)
{
    // This is optionally set in the device code to limit next step
    // size.  This is used by the JJ model (set in accept()), and by
    // the $bound_step() Verilog-A function (set in load()).  The
    // value is used to limit the next time step.
    //
    CKTdevMaxDelta = 0.0;

    double startTime = OP.seconds();
    int size = CKTmatrix->spGetSize(1);
    memset(CKTrhs, 0, (size+1)*sizeof(double));

    // Reset the real part of the matrix by loading the init
    // part, in which things might be cached.
    //
    if (!noclear)
        CKTmatrix->spLoadInitialization();

    CKTchargeCompNeeded =
        ((CKTmode & (MODEAC | MODETRAN | MODEINITSMSIG)) ||
            ((CKTmode & MODETRANOP) && (CKTmode & MODEUIC))) ? 1 : 0;
    CKTextPrec = CKTcurTask->TSKextPrec;

    bool tchk = CKTtrapCheck;
    CKTtrapCheck = tchk && (CKTmode & MODEINITFLOAT) && (CKTmode & MODETRAN);
    CKTtrapBad = false;

    // Mutual inductors must be loaded before the inductor devices. 
    // We'll load them now.  The order sensitivity could probably be
    // fixed, but there is a thread sync issue that would need to be
    // fixed as well.

    int muttype = -1;
    if (CKTmutModels) {
        muttype = CKTmutModels->GENmodType;
        for (sGENmodel *dm = CKTmutModels; dm; dm = dm->GENnextModel) {
            for (sGENinstance *d = dm->GENinstances; d;
                    d = d->GENnextInstance) {
                DEV.device(muttype)->load(d, this);
            }
        }
    }

#ifdef WITH_THREADS
#ifdef NEW_THREAD_QUEUE
    // The new thread queue, seems a bit faster than the original. 
    // Here there is one element per thread, holding Ndevs/Nthreads
    // devices.  The devices are scattered across the elements to even
    // the work.

    if (CKTcurTask->TSKloadThreads > 0) {
        if (!CKTloadPool ||
                ((int)CKTloadPool->num_threads() != CKTloadThreads)) {
            CKTloadThreads = CKTcurTask->TSKloadThreads;

            // Count the number of devices to load.
            int dcnt = 0;
            sCKTmodGen mgen(CKTmodels);
            for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
                if (m->GENmodType == muttype)
                    continue;
                for (sGENmodel *dm = m; dm; dm = dm->GENnextModel) {
                    for (sGENinstance *d = dm->GENinstances; d; 
                            d = d->GENnextInstance) {
                        int ret = DEV.device(m->GENmodType)->loadTest(d, this);
                        if (ret == LOAD_SKIP_FLAG)
                            break;
                        dcnt++;
                    }
                }
            }
            int njobs = CKTloadThreads + 1;  // worker threads plus primary
            int batchno = dcnt/njobs + (dcnt%njobs != 0);

            // Allocate the job structs.
            sBatchList *batch = 0;
            for (int i = 0; i < njobs; i++) {
                batch = new sBatchList(
                    sInstBatch::new_batch(this, batchno), batch);
            }

            // Fill the job structs, ordering to spread each device type
            // between jobs for good balance.
            sBatchList *b = batch;
            mgen = sCKTmodGen(CKTmodels);
            for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
                if (m->GENmodType == muttype)
                    continue;
                for (sGENmodel *dm = m; dm; dm = dm->GENnextModel) {
                    for (sGENinstance *d = dm->GENinstances; d; 
                            d = d->GENnextInstance) {
                        int ret = DEV.device(m->GENmodType)->loadTest(d, this);
                        if (ret == LOAD_SKIP_FLAG)
                            break;
                        b->batch->append(d);
                        b = b->next;
                        if (!b)
                            b = batch;
                    }
                }
            }

            // Update the thread pool.
            delete CKTloadPool;
            CKTloadPool = new cThreadPool(CKTloadThreads);
            CKTstat->STATloadThreads = CKTloadThreads;

            // Add the tasks, clear the list.
            while (batch) {
                CKTloadPool->submit(thread_proc, batch->batch, destroy_proc);
                b = batch;
                batch = batch->next;
                delete b;
            }
        }
    }
    if (CKTloadThreads > 0) {
        int error = CKTloadPool->run(0);
        if (error) {
            CKTtrapCheck = tchk;
            return (error);
        }
    }
    else

#else
    // The original thread queue.  This is fixed size, with each
    // element holding BATCHNO devices for loading.  There would
    // typically be many more elements than threads, so each thread
    // will process many elements until the queue is exhausted.  This
    // tends to balance the work done per thread, which is important
    // since there is no attempt to scatter devices across elements,
    // elements simply take the devices in order.

    CKTloadThreads = CKTcurTask->TSKloadThreads;
    if (CKTloadThreads > 0) {
        if (!CKTloadPool ||
                (CKTloadPool->num_threads() != (unsigned int)CKTloadThreads)) {
            delete CKTloadPool;
            CKTloadPool = new cThreadPool(CKTloadThreads);
            CKTstat->STATloadThreads = CKTloadThreads;

            sInstBatch *batch = 0;
            int j = 0;
            sCKTmodGen mgen(CKTmodels);
            for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
                if (m->GENmodType == muttype)
                    continue;
                for (sGENmodel *dm = m; dm; dm = dm->GENnextModel) {
                    for (sGENinstance *d = dm->GENinstances; d;
                            d = d->GENnextInstance) {
                        int ret = DEV.device(m->GENmodType)->loadTest(d, this);
                        if (ret == LOAD_SKIP_FLAG)
                            break;
                        if (!batch)
                            batch = new sInstBatch(this);
                        if (j == BATCHNO) {
                            j = 0;
                            CKTloadPool->submit(thread_proc, batch,
                                destroy_proc);
                            batch = new sInstBatch(this);
                        }
                        batch->set_list(d, j++);
                    }
                }
            }
            if (batch && batch->list(0)) {
                CKTloadPool->submit(thread_proc, batch);
                batch = 0;
            }
            delete batch;
        }
        int error = CKTloadPool->run(0);
        if (error) {
            CKTtrapCheck = tchk;
            return (error);
        }
    }
    else
#endif
#endif
    {
        sCKTmodGen mgen(CKTmodels);
        for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
            if (m->GENmodType == muttype)
                continue;
            for (sGENmodel *dm = m; dm; dm = dm->GENnextModel) {
                int noncon = CKTnoncon;
                for (sGENinstance *d = dm->GENinstances; d;
                        d = d->GENnextInstance) {
                    int error = DEV.device(m->GENmodType)->load(d, this);
                    if (error == LOAD_SKIP_FLAG)
                        break;
                    if (error) {
                        CKTtrapCheck = tchk;
                        return (error);
                    }
                }
                if (CKTstepDebug) {
                    if (noncon != CKTnoncon) {
                        TTY.err_printf(
                            "device type %s nonconvergence (%d tests)\n",
                            DEV.device(m->GENmodType)->name(),
                            CKTnoncon - noncon);
                        noncon = CKTnoncon;
                    }
                }
            }
        }
    }

    CKTtrapCheck = tchk;

    if (CKTmode & MODEDC) {
        // consider doing nodeset & ic assignments
        if (CKTmode & (MODEINITJCT | MODEINITFIX)) {
            // do nodesets
            const sCKTnode *node = CKTnodeTab.find(1);
            for ( ; node; node = CKTnodeTab.nextNode(node)) {
                if (node->nsGiven()) {
                    *(CKTrhs + node->number()) += node->nodeset();
                    double *diag = CKTmatrix->spGetElement(node->number(),
                        node->number());
                    if (diag)
                        ldadd(diag, 1.0);
                }
            }
        }
        if ((CKTmode & MODETRANOP) && !(CKTmode & MODEUIC)) {
            const sCKTnode *node = CKTnodeTab.find(1);
            for ( ; node; node = CKTnodeTab.nextNode(node)) {
                if (node->icGiven()) {
                    *(CKTrhs + node->number()) += node->ic();
                    double *diag = CKTmatrix->spGetElement(node->number(),
                        node->number());
                    if (diag)
                        ldadd(diag, 1.0);
                }
            }
        }
    }
    CKTstat->STATloadTime += OP.seconds() - startTime;
    return (OK);
}


// This function adds diagGmin to voltage nodes when diagGmin is
// nonzero.  Otherwise, if enabled, it will ensure that all of these
// (diagonal) elements have a minimum value of the circuit gmin.  This
// will keep the matrix solvable in the DC case with series
// capacitors, or capacitors or other devices connected on one end
// only.  Also if enabled, it will limit the magnitude of diagonal
// voltage node matrix entries.  This can avoid non-convergence in
// some cases, where matrix entries become so large as to cause a
// singular matrix.
//
int
sCKT::loadGmin()
{
    if (CKTdiagGmin != 0.0) {
        sCKTnode *node = CKTnodeTab.find(1);
        for ( ; node; node = CKTnodeTab.nextNode(node)) {
            if (node->type() == SP_VOLTAGE) {
                if (!CKTmatrix->spLoadGmin(node->number(), CKTdiagGmin, 0.0,
                        false, false)) {
                    // Hmmm, no diagonal entry in matrix.
                    OP.error(ERR_FATAL, "Badly connected node found: %s",
                        (const char*)node->name());
                    return (E_NODECON);
                }
            }
        }
        return (OK);
    }

    // The forcegmin option may resolve convergence issues, and will
    // allow DCOP of capacitor networks.  However, it causes
    // "surprising" things like finite MOS gate or capacitor current
    // when expecting none.  It is off by default.  The gmax checking
    // is always on.

    sCKTnode *node = CKTnodeTab.find(1);
    for ( ; node; node = CKTnodeTab.nextNode(node)) {
        if (node->type() == SP_VOLTAGE) {
            if (!CKTmatrix->spLoadGmin(node->number(), CKTcurTask->TSKgmin,
                    CKTcurTask->TSKgmax, CKTcurTask->TSKforceGmin, true)) {
                // Hmmm, no diagonal entry in matrix.
                OP.error(ERR_FATAL, "Badly connected node found: %s",
                    (const char*)node->name());
                return (E_NODECON);
            }
        }
    }

    // NOTE:  A current source in series with an inductor exclusively
    // can cause convergence problems.  May want someday to check for this,
    // and add gmin around the current source.

    return (OK);
}


// This is experimental.  As delta appears in the denominator of
// reactive terms, delta can't be arbitrarily small without causing
// numerical problems.  We attempt here to keep the largest such entry
// less than gmax by limiting the min delta.  It was noted that for
// some circuits, particularly one with a lot of breakpoints, setting
// delmin to a higher value allowed convergence.  Unfortunately, the
// results with this are mixed, it fixed convergence trouble in some
// circuits, but caused trouble in others.
//
double
sCKT::computeMinDelta()
{
    CKTtime = CKTdelta;
    CKTdeltaOld[0] = CKTdelta;
    NIcomCof();
    CKTmatrix->spClear();
    load(true);
    // Matrix now has terms a + b/delta.

    CKTmatrix->spNegate();
    double dt = CKTdelta;
    CKTdelta += dt;
    CKTtime = CKTdelta;
    CKTdeltaOld[0] = CKTdelta;
    NIcomCof();
    load(true);
    // Matrix new has terms -a - b/delta + a + b/2delta = -b/2delta.
    double maxelt = CKTmatrix->spLargest() * CKTdelta;

    double dmin = 0.0;
    if (maxelt > 0.0) {
        load(true);
        // Matrix now has terms -b/2delta + a + b/2delta = a.
        double minelt = CKTmatrix->spSmallest();

        // For IEEE double precision numbers, one has about 12 digit
        // resolution.
        double maxratio = 1e12*CKTcurTask->TSKreltol;

        dmin = maxelt/(maxratio*minelt);
        printf("computeMinDelta: delmin=%g minelt=%g maxelt=%g\n",
            dmin, minelt, maxelt);
    }

    CKTdelta = dt;
    CKTtime = 0.0;
    return (dmin);
}


// This saves/restores known-good device struct states.  Some BSIM
// models, for example, keep past history state in the device struct,
// which can wad up if there is nonconvergence, preventing convergence
// even if the external conditions are restored to a previous good
// state.  In particular, in gmin stepping, if we lose convergence, we
// need to restore the MOS device states before trying again.

int
sCKT::backup(DEV_BKMODE bmd)
{
    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next())
        DEV.device(m->GENmodType)->backup(m, bmd);
    if (bmd == DEV_SAVE) {
        CKTneedsRevertResetup = false;
        int sz = CKTmatrix->spGetSize(1) + 1;
        if (!CKToldSol)
            CKToldSol = new double[sz];
        if (!CKToldState0)
            CKToldState0 = new double[CKTnumStates + 1];
        memcpy(CKToldSol, CKTrhsOld, sz*sizeof(double));
        memcpy(CKToldState0, CKTstate0, CKTnumStates*sizeof(double));
    }
    else if (bmd == DEV_RESTORE) {
        int sz = CKTmatrix->spGetSize(1) + 1;
        if (CKToldSol)
            memcpy(CKTrhsOld, CKToldSol, sz*sizeof(double));
        if (CKToldState0)
            memcpy(CKTstate0, CKToldState0, CKTnumStates*sizeof(double));
        if (CKTneedsRevertResetup) {
            // The matrix pointers have changed!  Need to fix this.
            CKTneedsRevertResetup = false;
            int error = resetup();
            if (error)
                return (error);
        }
    }
    else {
        // DEV_CLEAR
        delete [] CKToldSol;
        CKToldSol = 0;
        delete [] CKToldState0;
        CKToldState0 = 0;
        CKTneedsRevertResetup = false;
    }
    return (OK);
}


// Create a device of the specified type, with the given name, using
// the specified model in the named circuit.
//
int
sCKT::newInst(sGENmodel *modPtr, sGENinstance **inInstPtr, IFuid name)
{
    if (inInstPtr)
        *inInstPtr = 0;
    if (modPtr == 0)
        return(E_NOMOD);
    int type = modPtr->GENmodType;
    sGENinstance *instPtr = 0;
    int error = findInst(&type, &instPtr, name, modPtr, 0);
    if (error == OK) { 
        if (inInstPtr)
            *inInstPtr = instPtr;
        return (E_EXISTS);
    }
    else if (error != E_NODEV)
        return (error);
    instPtr = DEV.device(type)->newInst();
    instPtr->GENname = name;
    instPtr->GENmodPtr = modPtr;
    instPtr->GENnextInstance = modPtr->GENinstances;
    modPtr->GENinstances = instPtr;

    if (!modPtr->GENinstTab)
        modPtr->GENinstTab = new sGENinstTable;
    modPtr->GENinstTab->link(instPtr);

    if (inInstPtr != 0)
        *inInstPtr = instPtr;
    return (OK);
}


// Create a device model of the specified type, with the given name
// in the named circuit.
//
int
sCKT::newModl(int type, sGENmodel **modfast, IFuid name)
{
    if (modfast)
        *modfast = 0;
    if (type < 0 || type > DEV.numdevs())
        return (E_BADPARM);
    sGENmodel *mymodfast = 0;
    int error = findModl(&type, &mymodfast, name);
    if (error == E_NOMOD) {
        mymodfast = DEV.device(type)->newModl();
        mymodfast->GENmodType = type;
            mymodfast->GENmodName = name;
        sCKTmodItem *mx = CKTmodels.find(type);
        if (mx) {
            mymodfast->GENnextModel = mx->head;
            mx->head = mymodfast;
        }
        else
            CKTmodels.insert(mymodfast);
        if (modfast)
            *modfast = mymodfast;
        return (OK);
    }
    else if (error == 0) {
        if (modfast)
            *modfast = mymodfast;
        return (E_EXISTS);
    }
    return (error);
}


int
sCKT::newTask(const char *what, const char *args, sTASK **pt)
{
    *pt = 0;
    sLstr lstr;
    lstr.add_c('.');
    if (args) {
        if (lstring::ciprefix(what, args))
            lstr.add(args);
        else {
            lstr.add(what);
            lstr.add_c(' ');
            lstr.add(args);
        }
    }
    else
        lstr.add(what);

    sLine cdeck;
    cdeck.set_line(lstr.string());

    IFuid specUid;
    int err = newUid(&specUid, 0, "special", UID_TASK);
    if (err) {
        Sp.Error(err, "newUid");
        return (E_PANIC);
    }
    sTASK *task = new sTASK(specUid);

    IP.parseDeck(this, &cdeck, task, true);
    if (cdeck.error()) {
        const char *s = cdeck.error();
        // If a line does not start with "Warning" assume a fatal error.
        while (s && *s) {
            if (!lstring::ciprefix("Warning", s)) {
                GRpkg::self()->ErrPrintf(ET_ERROR, "%s\n", cdeck.error());
                delete task;
                return (E_PANIC);
            }
            s = strchr(s, '\n');
            if (s) {
                while(isspace(*s))
                    s++;
            }
        }
        GRpkg::self()->ErrPrintf(ET_MSG, "%s\n", cdeck.error());
    }
    *pt = task;
    return (OK);
}


// Dynamic gmin stepping.
// Algorithm by Alan Gillespie, from ngspice.
//
int
sCKT::dynamic_gmin(int firstmode, int continuemode, int iterlim, bool srcstep)
{
    CKTmode = firstmode;

    int trace = Sp.GetTranTrace();

    memset(CKTstate0, 0, CKTnumStates*sizeof(double));

    const double gshunt = 0.0;
    const double Factor = 10.0;

    double factor = Factor;
    double OldGmin = 1e-2;
    CKTdiagGmin = OldGmin/factor;
    double gtarget = SPMAX(CKTcurTask->TSKgmin, gshunt);
    if (gtarget < 1e-15)
        gtarget = 1e-15;

    if (trace || Sp.GetFlag(FT_SIMDB)) {
        if (srcstep)
            TTY.err_printf("Starting gmin steping for first source step.\n");
        else
            TTY.err_printf("Starting gmin steping.\n");
    }

    bool succeeded = false;
    int nonconv = OK;
    for (;;) {

        int iters = CKTstat->STATnumIter;
        CKTnoncon = 1;
        nonconv = NIiter(CKTcurTask->TSKdcOpGminMaxIter);
        iters = (CKTstat->STATnumIter) - iters;

        if (nonconv) {
            if (trace || Sp.GetFlag(FT_SIMDB)) {
                TTY.err_printf("Gmin step failed, Gshunt=%g iters=%d %s\n",
                    CKTdiagGmin, iters, Sp.ErrorShort(nonconv));
            }

            if (nonconv != E_ITERLIM && nonconv != E_MATHDBZ &&
                    nonconv != E_MATHOVF && nonconv != E_MATHUNF &&
                    nonconv != E_MATHINV) {

                OP.error(ERR_FATAL, "Gmin iteration failed: %s.",
                    Sp.ErrorShort(nonconv));
                CKTdiagGmin = gshunt;
                backup(DEV_CLEAR);
                return (nonconv);
            }
            if (factor < 1.00005) {
                // Failed.
                break;
            }
            factor = sqrt(sqrt(factor));
            CKTdiagGmin = OldGmin/factor;

            backup(DEV_RESTORE);
        }
        else {
            if (trace || Sp.GetFlag(FT_SIMDB)) {
                TTY.err_printf("Gmin step succeeded, Gshunt=%g iters=%d\n",
                    CKTdiagGmin, iters);
            }

            CKTmode = continuemode;
            if (CKTdiagGmin <= gtarget) {
                // Success.
                succeeded = true;
                break;
            }
            backup(DEV_SAVE);

            if (iters <= CKTcurTask->TSKdcOpGminMaxIter/4) {
               factor *= sqrt(factor);
               if (factor > Factor)
                   factor = Factor;
            }
            else if (iters > 3*CKTcurTask->TSKdcOpGminMaxIter/4)
                factor = sqrt(factor);

            OldGmin = CKTdiagGmin;
            if ((CKTdiagGmin) < (factor * gtarget)) {
                factor = CKTdiagGmin/gtarget;
                CKTdiagGmin = gtarget;
            }
            else
                CKTdiagGmin /= factor;
        }
    }

    CKTdiagGmin = gshunt;
    backup(DEV_CLEAR);

    if (succeeded) {
        nonconv = NIiter(iterlim);
        if (!nonconv) {
            if (trace || Sp.GetFlag(FT_SIMDB))
                TTY.err_printf("Gmin stepping succeeded.\n");
            return (0);
        }
        if (trace || Sp.GetFlag(FT_SIMDB)) {
            TTY.err_printf("Final gmin iteration failed, %s\n",
                Sp.ErrorShort(nonconv));
        }
    }
    if (Sp.GetFlag(FT_SIMDB)) {
        if (srcstep) {
            TTY.err_printf("Gmin stepping for first source step failed, %s\n",
                Sp.ErrorShort(nonconv));
        }
        else {
            TTY.err_printf("Gmin stepping failed, %s.",
                Sp.ErrorShort(nonconv));
        }
    }
    return (nonconv);
}


// The Spice3 gmin stepping algorithm.
//
int
sCKT::spice3_gmin(int firstmode, int continuemode, int iterlim)
{
    CKTmode = firstmode;
    CKTdiagGmin = CKTcurTask->TSKgmin;
    for (int i = 0; i < CKTcurTask->TSKnumGminSteps; i++)
        CKTdiagGmin *= 10;

    bool succeeded = true;
    for (int i = 0; i <= CKTcurTask->TSKnumGminSteps; i++) {
        if (Sp.GetFlag(FT_SIMDB)) {
            TTY.err_printf("Starting gmin step, gmin = %12.4E.\n",
                CKTdiagGmin);
        }
        CKTnoncon = 1;
        int nonconv = NIiter(iterlim);
        if (nonconv) {
            if (Sp.GetFlag(FT_SIMDB))
                TTY.err_printf("Gmin step failed.\n");
            if (nonconv != E_ITERLIM) {
                OP.error(ERR_FATAL, "Iteration failed: %s.",
                    Sp.ErrorShort(nonconv));
                if (Sp.GetFlag(FT_SIMDB))
                    TTY.err_printf("Gmin iteration failed.\n");
            }
            CKTdiagGmin = 0;
            succeeded = false;
            break;
        }
        CKTdiagGmin /= 10.0;
        CKTmode = continuemode;
        if (Sp.GetFlag(FT_SIMDB))
            TTY.err_printf("Gmin step succeeded.\n");
    }

    CKTdiagGmin = 0;

    int ret = E_ITERLIM;
    if (succeeded) {
        if (Sp.GetFlag(FT_SIMDB))
            TTY.err_printf("Starting final gmin step.\n");
        int nonconv = NIiter(iterlim);
        if (!nonconv) {
            if (Sp.GetFlag(FT_SIMDB))
                TTY.err_printf("Gmin stepping succeeded.\n");
            return (0);
        }
        if (Sp.GetFlag(FT_SIMDB))
            TTY.err_printf("Final gmin step failed.\n");
        if (nonconv != E_ITERLIM) {
            if (Sp.GetFlag(FT_SIMDB))
                TTY.err_printf("Final gmin iteration failed, %s\n",
                    Sp.ErrorShort(nonconv));
            ret = nonconv;
        }
    }
    if (Sp.GetFlag(FT_SIMDB))
        TTY.err_printf("Gmin stepping failed.\n");
    return (ret);
}


// Based on the source stepping algorithm by Alan Gillespie, from
// ngspice.
//
int
sCKT::dynamic_src(int firstmode, int continuemode, int iterlim)
{
    CKTmode = firstmode;

    int trace = Sp.GetTranTrace();
    if (trace || Sp.GetFlag(FT_SIMDB))
        TTY.err_printf("Starting source stepping.\n");

    // Probably don't want to start at exactly zero, some device
    // models (hicum2.32) produce FPEs.
    // CKTsrcFact = 0;
    CKTsrcFact = 0.001;
    double raise = 0.001;
    double last_raise = raise;
    double ConvFact = 0;

    int nonconv = dynamic_gmin(firstmode, continuemode, iterlim, true);

    // If we've got convergence, then try stepping up the sources.
    if (!nonconv) {
        backup(DEV_SAVE);
        CKTsrcFact = ConvFact + raise;

        do {
            int iters = CKTstat->STATnumIter;
            nonconv = NIiter(CKTcurTask->TSKdcOpSrcMaxIter);
            iters = (CKTstat->STATnumIter) - iters;
            CKTmode = continuemode;

            if (nonconv) {
                if (trace || Sp.GetFlag(FT_SIMDB))
                    TTY.err_printf(
                        "Source step failed, factor=%g iters=%d %s\n",
                        CKTsrcFact, iters, Sp.ErrorShort(nonconv));

                // Don't give up if the matrix is singular!
                if (nonconv != E_ITERLIM && nonconv != E_SINGULAR &&
                        nonconv != E_MATHDBZ && nonconv != E_MATHOVF &&
                        nonconv != E_MATHUNF && nonconv != E_MATHINV) {

                    OP.error(ERR_FATAL, "Source iteration failed: %s.",
                        Sp.ErrorShort(nonconv));
                    break;
                }
                if (CKTsrcFact - ConvFact < 1e-8)
                    break;

                raise = 0.1 * last_raise;
                if (raise > 0.01)
                    raise = 0.01;

                CKTsrcFact = ConvFact + raise;
                last_raise = raise;
                backup(DEV_RESTORE);
                if (trace || Sp.GetFlag(FT_SIMDB))
                    TTY.err_printf("Reverting to factor=%g\n", CKTsrcFact);
            }
            else {
                ConvFact = CKTsrcFact;

                backup(DEV_SAVE);

                if (trace || Sp.GetFlag(FT_SIMDB))
                    TTY.err_printf(
                        "Source step succeeded, factor=%g iters=%d\n",
                        CKTsrcFact, iters);

                CKTsrcFact = ConvFact + raise;
                last_raise = raise;

                if (iters <= CKTcurTask->TSKdcOpSrcMaxIter/4)
                    raise *= 1.5;
                else if (iters > 3*CKTcurTask->TSKdcOpSrcMaxIter/4)
                    raise *= 0.5;
            }

            if (CKTsrcFact > 1)
                CKTsrcFact = 1;
        }
        while (raise >= 1e-7 && ConvFact < 1);
    }

    backup(DEV_CLEAR);
    CKTsrcFact = 1;

    if (ConvFact != 1) {
        if (trace || Sp.GetFlag(FT_SIMDB))
            TTY.err_printf("Source stepping failed, %s.\n",
                Sp.ErrorShort(nonconv));
        return (nonconv);
    }
    if (trace || Sp.GetFlag(FT_SIMDB))
        TTY.err_printf("Source stepping succeeded.\n");

    return (0);
}


// Spice3 source-stepping algorithm.
//
int
sCKT::spice3_src(int firstmode, int continuemode, int iterlim)
{
    CKTmode = firstmode;
    if (Sp.GetFlag(FT_SIMDB))
        TTY.err_printf("Starting source stepping.\n");

    for (int i = 0; i <= CKTcurTask->TSKnumSrcSteps; i++) {
        CKTsrcFact = ((double)i)/CKTcurTask->TSKnumSrcSteps;

        if (Sp.GetFlag(FT_SIMDB))
            TTY.err_printf("Start source step, supplies reduced to %8.4f%%.",
                CKTsrcFact*100);
        int nonconv = NIiter(iterlim);
        CKTmode = continuemode;
        if (nonconv) {
            CKTsrcFact = 1;
            if (nonconv != E_ITERLIM) {
                OP.error(ERR_FATAL, "Iteration failed: %s.",
                    Sp.ErrorShort(nonconv));
                if (Sp.GetFlag(FT_SIMDB))
                    TTY.err_printf("Source iteration failed.\n");
            }
            if (Sp.GetFlag(FT_SIMDB))
                TTY.err_printf("Source step failed (stepping failed).\n");
            return (nonconv);
        }
        if (Sp.GetFlag(FT_SIMDB))
            TTY.err_printf("Step succeeded.\n");
    }
    if (Sp.GetFlag(FT_SIMDB))
        TTY.err_printf("Source stepping succeeded.\n");
    CKTsrcFact = 1;
    return (0);
}


namespace {
    void print_nodes()
    {
        if (Sp.GetFlag(FT_BATCHMODE) && Sp.GetFlag(FT_NODESPRNT))
            CommandTab::com_dumpnodes(0);
    }

    bool err_return(sCKT *ckt, int err, bool first)
    {
        if (err == OK) {
            print_nodes();
            return (true);
        }
        if (err == E_SINGULAR) {
            ckt->warnSingular();
            return (true);
        }
        if (err == E_ITERLIM) {
            if (!first) {
                if (!Sp.GetFlag(FT_DCOSILENT)) {
                    OP.error(ERR_FATAL,
                        "No DCOP convergence, stepping failed.");
                    print_nodes();
                }
                return (true);
            }
            return (false);
        }
        if (err == E_PAUSE)
            OP.error(ERR_WARNING, "DCOP interrupted, simulation aborted.");
        else 
            OP.error(ERR_FATAL, "DCOP iteration error: %s.",
                Sp.ErrorShort(err));
        return (true);
    }
}


// Compute the operating point.
//
int
sCKT::op(int firstmode, int continuemode, int iterlim)
{
    // The WRspice defaults are:
    //
    // noopiter (ignored)   (Spice3: false)
    //   The initial iteration converges only for simple circuits,
    //   otherwise it is a waste of time.  For complex circuits that
    //   don't converge, the delay can be quite long.  For simple
    //   circuits, the time saved by avoiding continuation methods is
    //   tiny, not noticable to the user.
    //
    // gminsteps 0          (Spice3: 10)
    //   When 0, the dynamic algorithm is used, which seems much more
    //   effective than the Spice3 algorithm.
    //
    // srcsteps 0           (Spice3: 10)
    //   When 0, the dynamic algorithm is used, which seems much more
    //   effective than the Spice3 algorithm.  It uses the dynamic
    //   gmin algorithm for the first step, and seems more effective
    //   than pure gmin stepping as well.

    if ((firstmode & MODETRANOP) && CKTcurTask->TSKrampUpTime > 0.0) {
        // All sources are 0.

        CKTsrcFact = 0.0;
        CKTmode = firstmode;
        int err = NIiter(iterlim);
        if (!err) {
            CKTmode = continuemode;
            print_nodes();
            return (0);
        }
        err_return(this, err, false);
        return (err);
    }

    if (CKTcurTask->TSKnumGminSteps < 0 && CKTcurTask->TSKnumSrcSteps < 0) {
        // No stepping, but try a direct solution whether or not
        // noopiter is given.

        CKTmode = firstmode;
        int err = NIiter(iterlim);
        if (!err) {
            CKTmode = continuemode;
            print_nodes();
            return (0);
        }
        err_return(this, err, false);
        return (err);
    }
    if (CKTcurTask->TSKnumGminSteps <= 0 && CKTcurTask->TSKnumSrcSteps <= 0) {
        // This is the default, works well for large MOS circuits with
        // flip/flops, etc., which provide convergence challenges.

        if (CKTcurTask->TSKgminFirst) {
            if (CKTcurTask->TSKnumGminSteps >= 0) {
                int err = dynamic_gmin(firstmode, continuemode, iterlim);
                if (err_return(this, err, true))
                    return (err);
            }
            if (CKTcurTask->TSKnumSrcSteps >= 0) {
                int err = dynamic_src(firstmode, continuemode, iterlim);
                if (err_return(this, err, false))
                    return (err);
            }
        }
        else {
            if (CKTcurTask->TSKnumSrcSteps >= 0) {
                int err = dynamic_src(firstmode, continuemode, iterlim);
                if (err_return(this, err, true))
                    return (err);
            }
            if (CKTcurTask->TSKnumGminSteps >= 0) {
                int err = dynamic_gmin(firstmode, continuemode, iterlim);
                if (err_return(this, err, false))
                    return (err);
            }
        }
    }
    else {
        // The original Spice algorithm, sort of.  We use the dynamic
        // steping if the num steps is 0, skipping if -1.  We start
        // with source stepping, the reverse of Spice3, unless
        // gminfirst is given.

        if (!CKTcurTask->TSKnoOpIter) {
            CKTmode = firstmode;
            int err = NIiter(iterlim);
            if (!err) {
                CKTmode = continuemode;
                print_nodes();
                return (0);
            }
            if (err_return(this, err, true))
                return (err);
        }

        if (CKTcurTask->TSKgminFirst) {
            if (CKTcurTask->TSKnumGminSteps >= 0) {
                if (CKTcurTask->TSKnumGminSteps > 0) {
                    int err = spice3_gmin(firstmode, continuemode, iterlim);
                    if (err_return(this, err, true))
                        return (err);
                }
                else {
                    int err = dynamic_gmin(firstmode, continuemode, iterlim);
                    if (err_return(this, err, true))
                        return (err);
                }
            }
            if (CKTcurTask->TSKnumSrcSteps >= 0) {
                if (CKTcurTask->TSKnumSrcSteps > 0) {
                    int err = spice3_src(firstmode, continuemode, iterlim);
                    if (err_return(this, err, false))
                        return (err);
                }
                else {
                    int err = dynamic_src(firstmode, continuemode, iterlim);
                    if (err_return(this, err, false))
                        return (err);
                }
            }
        }
        else {
            if (CKTcurTask->TSKnumSrcSteps >= 0) {
                if (CKTcurTask->TSKnumSrcSteps > 0) {
                    int err = spice3_src(firstmode, continuemode, iterlim);
                    if (err_return(this, err, true))
                        return (err);
                }
                else {
                    int err = dynamic_src(firstmode, continuemode, iterlim);
                    if (err_return(this, err, true))
                        return (err);
                }
            }
            if (CKTcurTask->TSKnumGminSteps >= 0) {
                if (CKTcurTask->TSKnumGminSteps > 0) {
                    int err = spice3_gmin(firstmode, continuemode, iterlim);
                    if (err_return(this, err, false))
                        return (err);
                }
                else {
                    int err = dynamic_gmin(firstmode, continuemode, iterlim);
                    if (err_return(this, err, false))
                        return (err);
                }
            }
        }
    }
    // not reached
    return (OK);
}


// Compute the Lagrange factors for extrapolation to new time point.
// Sets ckt->CKTpred[4] and returns pointer: diff = DEVpredNew(ckt).
// Use: predicted = diff[0]*state1 + diff[1]*state2 + diff[2]*state3
//                  + dif[3]*state0.
//
double *
sCKT::predict()
{
    double *diff = CKTpred; // double CKTpred[4]
    if (CKTmode & MODETRAN) {
        double d1 = CKTdelta;

        /*
         * Seems that the linear extrapolation provides the lowest iteration
         * count
         *
        if (CKTstat->STATaccepted >= 4) {

            // third order extrapolation factors

            d1 = 1/d1;
            double c1 = CKTdeltaOld[1]*d1;
            double c2 = CKTdeltaOld[2]*d1;
            double c3 = CKTdeltaOld[3]*d1;
            double c12 = c1 + c2;
            double c23 = c2 + c3;
            double c123 = c12 + c3;

            double d2 = 1 + c1;
            double d3 = 1 + c12;
            double d4 = 1 + c123;

            double c  = c1*c2*c3*c12*c23*c123;
            c = 1/c;

            double dd1 = d3*d4*c*c3;
            double dd2 = d2*c*c1;

            diff[0] =  dd1 * d2 * c2  * c23;
            diff[1] = -dd1      * c12 * c123;
            diff[2] =  dd2 * d4 * c23 * c123;
            diff[3] = -dd2 * d3 * c2  * c12;
        }
        else
            if (CKTstat->STATaccepted >= 3) {

            // second order extrapolation factors
            double d2 = d1 + CKTdeltaOld[1];
            double d3 = d2 + CKTdeltaOld[2];

            diff[0] = d2*d3/((d1-d2)*(d1-d3));
            diff[1] = d1*d3/((d2-d1)*(d2-d3));
            diff[2] = d1*d2/((d3-d1)*(d3-d2));
            diff[3] = 0;
        }

        else {
         */
            // first order extrapolation factors
            diff[1] = -d1/CKTdeltaOld[1];
            diff[0] = 1 - diff[1];
            diff[2] = diff[3] = 0;
        /*
        }
         */
    }
    else {
        diff[0] = 1.0;
        diff[1] = 0.0;
        diff[2] = 0.0;
        diff[3] = 0.0;
    }
    return (diff);
}


// Set up the device initial conditions.  This is called from the
// transient analysis deriver if UIC is set.
//
int
sCKT::setic()
{
    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        int error = DEV.device(m->GENmodType)->getic(m, this);
        if (error)
            return (error);
    }
    return (OK);
}


// This is a driver program to iterate through all the various setup
// functions provided for the circuit elements in the given circuit.
//
int
sCKT::setup()
{
    // Save the existing top node number for the LoadGmin() function.
    if (!CKTmaxUserNodenum)
        CKTmaxUserNodenum = CKTnodeTab.numNodes() - 1;

    int error = unsetup();
    if (error)
        return (error);
    CKTnumStates = 0;
    NIdestroy();
    error = NIinit();
    if (error)
        return (error);

    // Redundant.
    CKTmatrix->spSetReal();
    if (CKTmatrix->spDataAddressChange()) {
        error = resetup();
        if (error)
            return (error);
    }
    CKTmatrix->spSetBuildState(0);

    // Set up Josephson junction support flags.
    CKTjjPresent = false;   // Circuit contains a Josephson junction.
#ifdef NEWJJDC
    CKTjjDCphase = false;   // Phase-mode DC analysis to be used.
#endif
    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        if (DEV.device(m->GENmodType)->flags() & (DV_JJSTEP | DV_JJPMDC)) {

            // Logic:
            // Call setup for JJs before other devices.  If the device
            // supports it, and phase-mode DC analysis is not disabled
            // (nopmdc option), then setup will set the phase flag of
            // all connected nodes.  If this is the case, set the
            // CKTjjDCphase flag, which enables phase-mode DC
            // analysis.
            //
            // If CKTjjDCphase is set, set also CKTjjPresent. 
            // Otherwise, look for the CCT model parameter and if not
            // found, or found and nonzero, set CKTjjPresent.

            error = DEV.device(m->GENmodType)->setup(m, this, &CKTnumStates);
            if (error)
                return (error);

#ifdef NEWJJDC
            if (!CKTjjDCphase) {
                for (sGENinstance *i = m->GENinstances; i;
                        i = i->GENnextInstance) {
                    if (DEV.device(m->GENmodType)->flags() & DV_JJPMDC) {

                        int nn = i->numnodes();
                        for (int j = 1; j <= nn; j++) {
                            int nodenum = *i->nodeptr(j);
                            if (nodenum > 0) {
                                sCKTnode *node = CKTnodeTab.find(nodenum);
                                if (node && node->phase()) {
                                    CKTjjDCphase = true;
                                    CKTjjPresent = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
#endif
            if (!CKTjjPresent) {
                CKTjjPresent = true;
                IFparm *p = DEV.device(m->GENmodType)->findModelParm("cct",
                    IF_ASK);
                if (p) {
                    // Model has a CCT parameter.  Take this as a
                    // Whiteley Research JJ model, so there is no
                    // overriding instance parameter.  If CCT is 0,
                    // reset CKTjjPrsent since there is no critical
                    // current.

                    IFdata data;
                    int err = DEV.device(m->GENmodType)->askModl(m, p->id,
                        &data);
                    if (err == OK && data.v.iValue == 0)
                        CKTjjPresent = false;
                }
            }
        }
    }

    mgen = sCKTmodGen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        // We've' already done the JJs. if any.
        if (DEV.device(m->GENmodType)->flags() & (DV_JJSTEP | DV_JJPMDC))
            continue;
        error = DEV.device(m->GENmodType)->setup(m, this, &CKTnumStates);
        if (error)
            return (error);
    }
    CKTmatrix->spSetBuildState(1);

    CKTstateSize = CKTnumStates;
    for (int i = 0; i < 8; i++) {
        delete [] CKTstates[i];
        CKTstates[i] = 0;
        if (i <= CKTcurTask->TSKmaxOrder + 1) {
            CKTstates[i] = new double[CKTnumStates];
            memset(CKTstates[i], 0, CKTnumStates*sizeof(double));
        }
    }
    error = NIreinit();
    if (error)
        return (error);

    return (OK);
}


int
sCKT::unsetup()
{
    // We need to remove all nodes and corresponding terminals that
    // were created in setup.  The devices that create internal nodes
    // otherwise fail on an E_EXISTS return from the terminal
    // allocator.  The legacy code would loop through the devices and
    // call delNode for each internal node (unsetup methods), and zero
    // the node value in the device.
    //
    // Here, delNode is a no-op.  We still have to call unsetup to
    // zero the device node values (or the nodes won't be recreated). 
    // We deallocate the nodes by decrementing the allocation count,
    // and removing and destroying the corresponding terminals.

    if (CKTmaxUserNodenum > 0)
        CKTnodeTab.dealloc(CKTmaxUserNodenum);

    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        int error = DEV.device(m->GENmodType)->unsetup(m, this);
        if (error)
            return (error);
    }
    return (OK);
}


// This updates the data pointers to the matrix elements of each
// device.  It must be called in the event that the sparse matrix
// package moves the element structs to new locations.  One could as
// well call unsetup/setup, however I'm not sure that this is without
// side effects or core leaks.
//
int
sCKT::resetup()
{
    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        int error = DEV.device(m->GENmodType)->resetup(m, this);
        if (error)
            return (error);
    }
    return (OK);
}


// This initializes tran function nodes in parse trees with the
// transient analysis parameters.  If arguments are zero, tran funcs
// will be disabled, appropriate when not doing transient analysis.
// This should be called when a new analysis begins.
//
int
sCKT::initTranFuncs(double step, double finaltime)
{
    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next())
        DEV.device(m->GENmodType)->initTranFuncs(m, step, finaltime);
    return (OK);
}


// This is a driver program to iterate through all the various
// temperature dependency functions provided for the circuit elements
// in the given circuit.
//
int
sCKT::temp()
{
    CKTvt = wrsCONSTKoverQ * CKTcurTask->TSKtemp;
    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        int error = DEV.device(m->GENmodType)->temperature(m, this);
        if (error)
            return (error);
    }
    return (OK);
}


namespace {
    inline double true_time(double tmp, int order)
    {
        if (order == 2)
            return (sqrt(tmp));
        if (order == 1)
            return (tmp);
        if (order == 3)
            return (cbrt(tmp));
        if (order == 4)
            return (sqrt(sqrt(tmp)));
        return (exp(log(tmp)/order));
    }
}


void
sCKT::terr(int qcap, double *timeStep)
{ 
    static double gearCoeff[] = {
        .5,
        .2222222222,
        .1363636364,
        .096,
        .07299270073,
        .05830903790
    };
    static double trapCoeff[] = {
        .5,
        .08333333333
    };

#define ccap (qcap+1)

    double y0 = fabs(*(CKTstate0 + ccap));
    double y1 = fabs(*(CKTstate1 + ccap));
    double volttol = CKTcurTask->TSKabstol +
        CKTcurTask->TSKreltol*SPMAX(y0,y1);

    y0 = fabs(*(CKTstate0 + qcap));
    y1 = fabs(*(CKTstate1 + qcap));
    double chargetol = SPMAX(y0,y1);
    if (chargetol < CKTcurTask->TSKchgtol)
        chargetol = CKTcurTask->TSKchgtol;
    chargetol *= CKTcurTask->TSKreltol/CKTdelta;

    double tol = SPMAX(volttol, chargetol);

    double diff[8];
    double deltmp[8];

    // Now divided differences.
    for (int i = CKTorder + 1; i >= 0; i--)
        diff[i] = *(CKTstates[i] + qcap);
    for (int i = 0; i <= CKTorder; i++)
        deltmp[i] = CKTdeltaOld[i];
    int j = CKTorder;
    for (;;) {
        for (int i = 0; i <= j; i++)
            diff[i] = (diff[i] - diff[i+1])/deltmp[i];
        if (--j < 0)
            break;
        for (int i = 0; i <= j; i++)
            deltmp[i] = deltmp[i+1] + CKTdeltaOld[i];
    }

    double del;
    if (CKTcurTask->TSKintegrateMethod == TRAPEZOIDAL)
        del = fabs(diff[0]*trapCoeff[CKTorder-1]);
    else
        del = fabs(diff[0]*gearCoeff[CKTorder-1]);

    del = CKTcurTask->TSKtrtol*tol/SPMAX(CKTcurTask->TSKabstol, del);

    // Normalization is now done in trunc().
    // del = true_time(del, CKTorder);

    if (del < *timeStep)
        *timeStep = del;
}


char *
sCKT::trouble(const char *optmsg)
{
    char msg_buf[513];
    if (!optmsg)
        optmsg = "";
    if (CKTtroubleNode) {
        snprintf(msg_buf, sizeof(msg_buf),
            "%s%sTime: %g, Timestep %g: trouble with %s\n",
            optmsg, optmsg && *optmsg ? "; " : "",
            CKTtime, CKTdelta, (char*)nodeName(CKTtroubleNode));
    }
    else if (CKTtroubleElt) {
        snprintf(msg_buf, sizeof(msg_buf),
            "Time: %g, Timestep %g: trouble with %s:%s:%s\n",
            CKTtime, CKTdelta,
            DEV.device(CKTtroubleElt->GENmodPtr->GENmodType)->name(),
                (char*)CKTtroubleElt->GENmodPtr->GENmodName,
                (char*)CKTtroubleElt->GENname);
    }
    else {
        snprintf(msg_buf, sizeof(msg_buf),
            "Time: %g, Timestep %g: Non convergence problem detected.\n",
            CKTtime, CKTdelta);
    }

    char *emsg = new char[strlen(msg_buf)+1];
    strcpy(emsg, msg_buf);
    return (emsg);
}


// This is a driver program to iterate through all the various
// truncation error functions provided for the circuit elements in the
// given circuit.
//
int
sCKT::trunc(double *timeStep)
{
    double timetemp = DBL_MAX;

    sCKTmodGen mgen(CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {

        double tmp = timetemp;
        int error = DEV.device(m->GENmodType)->trunc(m, this, &timetemp);
        if (error)
            return (error);
        if (CKTstepDebug) {
            if (tmp != timetemp) {
                TTY.err_printf(
                    "timestep cut by device type %s from %g to %g\n",
                    DEV.device(m->GENmodType)->name(),
                    true_time(tmp, CKTorder), true_time(timetemp, CKTorder));
            }
        }
    }

    // Here we normalize to integration order ( not in CKTterr() ) to
    // avoid the math operation at each call.  This works directly for
    // all devices which simply call CKTterr() in the trunc functions.
    //
    // !!!> The LTRAtrunc() code needs fixing if you change this <!!!
    //
    *timeStep = true_time(timetemp, CKTorder);

    // Spice3 passed the current delta as *timeStep, and limited the
    // return to twice this value.  The caller now takes care of the
    // limiting.

    return (OK);
}


IFmacro *
sCKT::find_macro(const char *name, int numargs)
{
    if (!CKTmacroTab || !name)
        return (0);

    char buf[256];
    snprintf(buf, sizeof(buf), "%s:%d", name, numargs);
    return ((IFmacro*)sHtab::get(CKTmacroTab, buf));
}


void
sCKT::save_macro(IFmacro *macro)
{
    if (!macro)
        return;
    char buf[256];
    snprintf(buf, sizeof(buf), "%s:%d", macro->name(), macro->numargs());
    if (!CKTmacroTab)
        CKTmacroTab = new sHtab(true);
    CKTmacroTab->add(buf, macro);
}


// If the matrix reorder fails with E_SINGULAR, call this to pop-up a
// message.
//
void
sCKT::warnSingular()
{
    // Check for inductor/voltage source loops.
    checkLVloops();
    checkCIcutSets();

    int i, j;
    CKTmatrix->spWhereSingular(&i, &j);
    OP.error(ERR_FATAL,
        "singular matrix:  check nodes %s and %s",
        (char*)nodeName(i), (char*)nodeName(j));
}


namespace {
    // Instance list, used in next two functions.
    struct sdlist_t
    {
        sdlist_t(sGENinstance *i, sdlist_t *n)
            {
                inst = i;
                next = n;
            }

        static void destroy(sdlist_t *s)
            {
                while (s) {
                    sdlist_t *x = s;
                    s = s->next;
                    delete x;
                }
            }

        sGENinstance *inst;
        sdlist_t *next;
    };
}


// Check inductors and voltage sources for topological loops, which cause
// a singular matrix in DCOP analysis.
//
int
sCKT::checkLVloops()
{
    int sz = CKTmatrix->spGetSize(1);
    sdlist_t **heads = new sdlist_t *[sz+1];
    memset(heads, 0, (sz+1)*sizeof(sdlist_t*));

    // Each head is indexed by the node number and consists of a
    // sdlist_t list of connected devices.

    sGENmodel *gen_model = 0;
#ifdef NEWJJDC
    if (CKTjjDCphase) {
        // Inductor loops are allowed here.
        // May want to add a check for "floating" phase-mode subnets,
        // and voltage-mode devices directly connected to phase nodes.
    }
    else {
#else
    {
#endif
        int type = typelook("Inductor", &gen_model);
        if (type >= 0) {
            for (sGENmodel *model = gen_model; model;
                    model = model->GENnextModel) {
                for (sGENinstance *inst = model->GENinstances; inst;
                        inst = inst->GENnextInstance) {
                    heads[*inst->nodeptr(1)] =
                        new sdlist_t(inst, heads[*inst->nodeptr(1)]);
                    heads[*inst->nodeptr(2)] =
                        new sdlist_t(inst, heads[*inst->nodeptr(2)]);
                }
            }
        }
    }

    int type = typelook("Source", &gen_model);
    if (type >= 0) {
        IFparm *p = DEV.device(type)->findInstanceParm("branch", IF_ASK);
        if (!p) {
            // This "can't happen" with our src device.
            OP.error(ERR_WARNING,
            "Source device has no \"branch\" keyword, skipping loop test.\n");
            return (OK);
        }
        for (sGENmodel *model = gen_model; model;
                model = model->GENnextModel) {
            for (sGENinstance *inst = model->GENinstances; inst;
                    inst = inst->GENnextInstance) {

                // Ask the branch node number, voltage sources will have
                // this set to a positive value.
                IFdata data;
                if (inst->askParam(this, p->id, &data))
                    continue;
                if (data.v.iValue < 1)
                    continue;

                heads[*inst->nodeptr(1)] =
                    new sdlist_t(inst, heads[*inst->nodeptr(1)]);
                heads[*inst->nodeptr(2)] =
                    new sdlist_t(inst, heads[*inst->nodeptr(2)]);
            }
        }
    }

    // Iteratively look through the heads, removing devices with one
    // node that appears exactly once in its head list (so the
    // connection is dangling).  Do this until no such devices are
    // found.  Any devices that are left in the list are connected to
    // each other forming loops.

    for (;;) {
        int removed = 0;
        for (int i = 0; i <= sz; i++) {
            if (!heads[i])
                continue;
            if (!heads[i]->next) {
                sdlist_t *sd1 = heads[i];
                heads[i] = 0;
                int n = *sd1->inst->nodeptr(1);
                if (n == i)
                    n = *sd1->inst->nodeptr(2);

                sdlist_t *sp = 0, *sn;
                for (sdlist_t *sd = heads[n]; sd; sd = sn) {
                    sn = sd->next;
                    if (sd->inst == sd1->inst) {
                        if (sp)
                            sp->next = sn;
                        else
                            heads[n] = sn;
                        delete sd;
                        break;
                    }
                    sp = sd;
                }
                delete sd1;
                removed++;
            }
        }
        if (!removed)
            break;
    }

    // Hash the names of remaining devices.
    //
    sHtab *tab = new sHtab(true);
    for (int i = 0; i <= sz; i++) {
        for (sdlist_t *sd = heads[i]; sd; sd = heads[i]) {
            heads[i] = sd->next;
            if (!sHtab::get(tab, (const char*)sd->inst->GENname))
                tab->add((const char*)sd->inst->GENname, sd->inst);
            delete sd;
        }
    }
    delete [] heads;

    wordlist *wl = sHtab::wl(tab);
    delete tab;

    // Output a listing of any looped devices.
    //
    if (wl) {
        wordlist::sort(wl);
        sLstr lstr;
        lstr.add("Voltage source and/or inductor loop(s) detected.\n");
        lstr.add("Devices involved: ");
        int ccnt = 18;
        for (wordlist *w = wl; w; w = w->wl_next) {
            int len = strlen(w->wl_word);
            if (len + ccnt > 70) {
                lstr.add_c('\n');
                lstr.add_c(' ');
                ccnt = 1;
            }
            lstr.add_c(' ');
            lstr.add(w->wl_word);
        }
        wordlist::destroy(wl);

        OP.error(ERR_FATAL, lstr.string());
        return (E_SINGULAR);
    }
    return (OK);
}


// Placeholder, maybe do something with this one day.  This is hard,
// have to identify "open" branches, and nodes connecting only open
// branches.
//
int
sCKT::checkCIcutSets()
{
    return (OK);
}


// Evaluate the tran function in string using the scale of length len.
// If successful, return vector in rvec and return true.
//
int
sCKT::evalTranFunc(double **rvec, const char *string, double *scale, int len)
{
    if (len > 1) {
        IFparseTree *tree = IFparseTree::getTree(&string, this, 0);
        if (!tree)
            return (false);
        IFparseNode *p = tree->tree();
        double t0 = scale[0];
        double t1 = scale[len-1];
        double dt = (t1 - t0)/(len - 1);
        p->p_init_func(dt, t1, true);
        double t;
        t1 += dt/2;
        double *vec = new double[len];
        int i;
        double tm = CKTtime;
        for (i = 0, t = t0; t < t1; i++, t += dt) {
            CKTtime = t;
            (p->*p->p_evfunc)(vec+i, 0, 0);
        }
        CKTtime = tm;
        *rvec = vec;
        delete tree;
        return (true);
    }
    return (false);
}


// Static function.
// Floating point exception checking.  Exceptions can be enabled
// in wrspice.cc for debugging.  This is for use by the device
// library, the same functionality is inlined elsewhere.
//
int
sCKT::checkFPE(bool noerrret)
{
    return (check_fpe(noerrret));
}


// Static function.
// For devices, temporarily disable FPE checking.
//
int
sCKT::disableFPE()
{
    int state = Sp.GetFPEmode();
    Sp.SetFPEmode(FPEnoCheck);
    return (state);
}


// Static function.
// For devices, re-enable FPE checking to previous state.
//
void
sCKT::enableFPE(int state)
{
    checkFPE(true);
    Sp.SetFPEmode((FPEmode)state);
}
// End of sCKT functions.


#ifdef TJM_IF
// This is used in the TJM device library model.
//
FILE *tjm_fopen(const char *nm, char **pfp)
{
    const char *tjm_path = "( . ~/.mmjco )";
    VTvalue vv;
    if (Sp.GetVar("tjm_path", VTYP_STRING, &vv, Sp.CurCircuit()))
        tjm_path = vv.get_string();
    return (pathlist::open_path_file(nm, tjm_path, "r", pfp, false));
}
#endif

