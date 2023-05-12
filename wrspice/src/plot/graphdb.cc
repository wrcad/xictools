
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
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 UCB
         1992 Stephen R. Whiteley
****************************************************************************/

#include "graph.h"
#include "output.h"
#include "cshell.h"
#include "simulator.h"
#include "runop.h"


//
// Manage graph data structure.
//

// linked list of graphs
//
struct sGlist : public sGraph
{
    sGlist(int gid)
        {
            newid(gid);
            next = 0;
        };

    sGlist *next;
};

#define NUMGBUCKETS 16

struct sGbucket
{
    sGlist *list;
};
namespace { sGbucket GBucket[NUMGBUCKETS]; }


// Returns 0 on error.
//
sGraph *
SPgraphics::NewGraph()
{
    int BucketId = spg_running_id % NUMGBUCKETS;
    sGlist *list = new sGlist(spg_running_id);
    if (!GBucket[BucketId].list)
        GBucket[BucketId].list = list;
    else {
        // insert at front of current list
        list->next = GBucket[BucketId].list;
        GBucket[BucketId].list = list;
    }
    spg_running_id++;
    return (list);
}


sGraph *
SPgraphics::NewGraph(int type, const char *name)
{
    sGraph *graph = NewGraph();
    if (!graph->gr_setup_dev(type, name)) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "can't open viewport for graphics.\n");
        DestroyGraph(graph->id());
        return (0);
    }
    return (graph);
}


// Given graph id, return graph.
//
sGraph *
SPgraphics::FindGraph(int id)
{
    sGlist *list;
    for (list = GBucket[id % NUMGBUCKETS].list; list && list->id() != id;
            list = list->next) ;
    return (list);
}


bool
SPgraphics::DestroyGraph(int id)
{
    sGlist *list = GBucket[id % NUMGBUCKETS].list;
    sGlist *lastlist = 0;
    while (list) {
        if (list->id() == id) {  // found it

            // Fix the iplot list
            sRunopIplot *d = OP.runops()->iplots();
            while (d && d->graphid() != id)
                d = d->next();
            if (!d) {
                for (sFtCirc *f = Sp.CircuitList(); f; f = f->next()) {
                    d = f->iplots();
                    for ( ; d && d->graphid() != id; d = d->next()) ;
                    if (d)
                        break;
                }
            }

            if (d && d->type() == RO_IPLOT) {
                d->set_type(RO_DEADIPLOT);
                // Delete this later
                return (0);
            }

            // adjust bucket pointers
            if (lastlist)
                lastlist->next = list->next;
            else
                GBucket[id % NUMGBUCKETS].list = list->next;

            list->gr_reset();
            list->halt();
            delete list;
            return (true);
        }
        lastlist = list;
        list = list->next;
    }

    GRpkg::self()->ErrPrintf(ET_INTERR,
        "DestroyGraph: tried to destroy non-existent graph.\n");
    return (false);
}


// free up all dynamically allocated data structures
//
void
SPgraphics::FreeGraphs()
{
    for (sGbucket *gbucket = GBucket; gbucket < &GBucket[NUMGBUCKETS];
            gbucket++) {
        for (sGlist *list = gbucket->list; list; list = gbucket->list) {
            gbucket->list = list->next;
            delete list;
        }
    }
}


void
SPgraphics::SetGraphContext(int graphid)
{
    spg_cur = FindGraph(graphid);
}


struct sGstack
{
    sGstack()
        {
            pgraph = 0;
            next = 0;
        }

    static void push(sGraph *graph, sGraph **curptr)
        {
            sGstack *gcstack = new sGstack;
            if (!gcstacktop)
                gcstacktop = gcstack;
            else {
                gcstack->next = gcstacktop;
                gcstacktop = gcstack;
            }
            gcstacktop->pgraph = *curptr;
            *curptr = graph;
        }

    static void pop(sGraph **curptr)
        {
            if (gcstacktop) {
                *curptr = gcstacktop->pgraph;
                sGstack *dead = gcstacktop;
                gcstacktop = gcstacktop->next;
                delete dead;
            }
        }

private:
    sGraph *pgraph;
    sGstack *next;

    static sGstack *gcstacktop;
};

sGstack *sGstack::gcstacktop;



// The push and pop have two uses: 1) while drawing a plot (including an
// iplot), the context is pushed so that interrupt and error handlers can
// gain access, and 2) while writing ghost-mode figures, a push is used to
// tell the rendering functions which graph to draw on.

void
SPgraphics::PushGraphContext(sGraph *graph)
{
    sGstack::push(graph, &spg_cur);
}


void
SPgraphics::PopGraphContext()
{
    sGstack::pop(&spg_cur);
}


#define GXorig 200;
#define GYorig 100;

namespace {
    void default_position(int i, int *x, int *y)
    {
        *x = GXorig;
        *y = GYorig;
        *y += i*25;
        *x += i*25;
    }


    int count_plots()
    {
        int count = 0;
        for (int i = 0; i < NUMGBUCKETS; i++) {
            sGlist *list = GBucket[i].list;
            for (; list; count++, list = list->next) ;
        }
        return (count);
    }
}


// Return the position in screen coordinates of the next plot window.
// This can be set with a "plotposnN" variable, or through the default
// positioning routine.  The default positioning assumes that the
// coordinates are referenced to the upper left corner of the screen
// and plot window (as in X).
//
void
SPgraphics::PlotPosition(int *x, int *y)
{
    int xx, yy;
    char name[40];
    static int count;

    if (count_plots() == 1)
        count = 0;
    int i = count;
    count++;
    i %= NUMGBUCKETS;
    sprintf(name, "plotposn%d", i);
    variable *v = Sp.GetRawVar(name);
    if (!v) {
        default_position(i, x, y);
        return;
    }
    if (v->type() == VTYP_STRING) {
        int j = sscanf(v->string(), "%d%d", &xx, &yy);
        if (j == 2 && xx >= 0 && xx <= 4000 && yy >= 0 && yy <= 3000) {
            *x = xx;
            *y = yy;
        }
        else
            default_position(i, x, y);
        return;
    }
    if (v->type() == VTYP_LIST) {
        v = v->list();
        xx = yy = -1;
        if (v && v->next()) {
            if (v->type() == VTYP_NUM)
                xx = v->integer();
            else if (v->type() == VTYP_REAL)
                xx = (int)v->real();
            v = v->next();
            if (v->type() == VTYP_NUM)
                yy = v->integer();
            else if (v->type() == VTYP_REAL)
                yy = (int)v->real();
        }
        if (xx >= 0 && xx <= 4000 && yy >= 0 && yy <= 3000) {
            *x = xx;
            *y = yy;
            return;
        }
    }
    default_position(i, x, y);
}


// Initialize and return a generator.
//
struct sGgen *
SPgraphics::InitGgen()
{
    for (int i = 0; i < NUMGBUCKETS; i++) {
        sGlist *list = GBucket[i].list;
        if (list)
            return (new sGgen(i, list));
    }
    return (0);
}


// Generator: return a graph on each call
//
sGraph *
sGgen::next()
{
    const sGgen *thisgen = this;
    if (!thisgen)
        return (0);

    if (!graph) {
        delete this;
        return (0);
    }
    sGraph *g = graph;
    graph = 0;
    sGlist *l = (sGlist*)g;
    if (l->next)
        graph = l->next;
    else {
        for (int i = num+1; i < NUMGBUCKETS; i++) {
            sGlist *list = GBucket[i].list;
            if (list) {
                graph = list;
                num = i;
                break;
            }
        }
    }
    return (g);
}

