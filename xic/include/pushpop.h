
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef PUSHPOP_H
#define PUSHPOP_H


// Interface to the undo list handling.
struct pp_state_t
{
    virtual ~pp_state_t() { }
    virtual void purge(const CDs*, const CDl*) = 0;
    virtual void purge(const CDs*, const CDo*) = 0;
    virtual void rotate() = 0;
};


// Save drawing window region.
struct win_state_t
{
    win_state_t() { width = 0.0; x = y = 0; sdesc = 0; }

    double width;
    int x;
    int y;
    const CDs *sdesc;
};


// Before a Push, the present values of a number of variables are
// stored in the structure below.
//
struct ContextDesc
{
    ContextDesc()
        {
            cNext = 0;
            cInst = 0;
            cIndX = 0;
            cIndY = 0;
            cParentDesc = 0;
            cState = 0;
        }

    ContextDesc(const CDc*, unsigned int, unsigned int);
    ~ContextDesc();

    void purge(const CDs*, const CDl*);
    ContextDesc *purge(const CDs*, const CDo*);
    void clear();

    int level()
        {
            int n = 0;
            for (ContextDesc *d = this; d; d = d->cNext, n++) ;
            return (n);
        }

    ContextDesc *cNext;     // pointer to parent context
    const CDc *cInst;       // pushed-to instance object desc
    unsigned int cIndX;     // instance array X index
    unsigned int cIndY;     // instance array Y index
    CDs *cParentDesc;       // pushed-from cell desc
    pp_state_t *cState;     // interface for undo/redo of edits
    CDtf cTF;               // transform structure

    win_state_t cWinState[DSP_NUMWINS];  // per-window regions
    wStack cWinViews[DSP_NUMWINS];       // per-window view stacks
};


// State storage for mode switch.
//
struct CXstate
{
    CXstate(CDcellName n, ContextDesc *c, ContextDesc *ch)
        {
            cellname = n;
            context = c;
            context_history = ch;
        }

    ~CXstate()
        {
            context->clear();
            context_history->clear();
        }

    CDcellName cellname;
    ContextDesc *context;
    ContextDesc *context_history;
};


inline class cPushPop *PP();

// Interface for Push/Pop of editing context.
//
class cPushPop
{
    static cPushPop *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cPushPop *PP() { return (cPushPop::ptr()); }

    cPushPop();
    void PushContext(const CDc*, unsigned int, unsigned int);
    void PopContext();
    void ClearContext(bool = false);
    void ClearLists();
    void InvalidateLayer(const CDs*, const CDl*);
    void InvalidateObject(const CDs*, const CDo*, bool);
    CXstate *PopState();
    void PushState(CXstate*);

private:
    // editcx_setif.cc
    void setupInterface();
public:

    const CDc *Instance() { return (context ? context->cInst : 0); }

    int Level() { return (context->level()); }

    void InstanceList(const CDc **list, int *szp)
        {
            int j = Level();
            *szp = j;
            for (ContextDesc *cx = context; cx && j > 0; cx = cx->cNext)
                list[--j] = cx->cInst;
        }

    ContextDesc *Context() { return (context); }

private:
    ContextDesc *context;           // Context storage for push/pop.
    ContextDesc *context_history;   // Reservoir for popped contexts.
    bool no_display;                // Suppress window redrawing.

    static cPushPop *instancePtr;
};

#endif

