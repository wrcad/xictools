
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
            c_next = 0;
            c_inst = 0;
            c_indx = 0;
            c_indy = 0;
            c_parent = 0;
            c_state = 0;
        }

    ContextDesc(const CDc*, unsigned int, unsigned int);
    ~ContextDesc();

    static void purge(ContextDesc*, const CDs*, const CDl*);
    static ContextDesc *purge(ContextDesc*, const CDs*, const CDo*);
    static void clear(ContextDesc*);

    ContextDesc *next()             { return (c_next); }
    void set_next(ContextDesc *n)   { c_next = n; }
    const CDc *instance()           const { return (c_inst); }
    unsigned int indX()             { return (c_indx); }
    unsigned int indY()             { return (c_indy); }
    CDs *parent()                   { return (c_parent); }
    void set_parent(CDs *s)         { c_parent = s; }
    pp_state_t *state()             { return (c_state); }
    CDtf *tf()                      { return (&c_tf); }
    win_state_t *win_state(int i)   { return (c_winstate + i); }
    wStack *win_views(int i)        { return (c_winviews + i); }

    int level()
        {
            int n = 0;
            for (ContextDesc *d = this; d; d = d->c_next, n++) ;
            return (n);
        }

private:
    ContextDesc *c_next;    // pointer to parent context
    const CDc *c_inst;      // pushed-to instance object desc
    unsigned int c_indx;    // instance array X index
    unsigned int c_indy;    // instance array Y index
    CDs *c_parent;          // pushed-from cell desc
    pp_state_t *c_state;    // interface for undo/redo of edits
    CDtf c_tf;              // transform structure

    win_state_t c_winstate[DSP_NUMWINS];    // per-window regions
    wStack c_winviews[DSP_NUMWINS];         // per-window view stacks
};


// State storage for mode switch.
//
struct CXstate
{
    CXstate(CDcellName n, ContextDesc *c, ContextDesc *ch)
        {
            cx_cellname = n;
            cx_context = c;
            cx_context_history = ch;
        }

    ~CXstate()
        {
            ContextDesc::clear(cx_context);
            ContextDesc::clear(cx_context_history);
        }

    CDcellName cellname()               { return (cx_cellname); }
    ContextDesc *context()              { return (cx_context); }
    void set_context(ContextDesc *c)    { cx_context = c; }
    ContextDesc *context_history()      { return (cx_context_history); }
    void set_context_history(ContextDesc *c) { cx_context_history = c; }

private:
    CDcellName cx_cellname;
    ContextDesc *cx_context;
    ContextDesc *cx_context_history;
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

    ContextDesc *Context()  { return (pp_cx); }
    const CDc *Instance()   { return (pp_cx ? pp_cx->instance() : 0); }
    int Level()             { return (pp_cx ? pp_cx->level() : 0); }

    void InstanceList(const CDc *list[CDMAXCALLDEPTH], int *szp)
        {
            int j = Level();
            *szp = j;
            for (ContextDesc *cx = pp_cx; cx && j > 0; cx = cx->next())
                list[--j] = cx->instance();
        }

private:
    // editcx_setif.cc
    void setupInterface();

    ContextDesc *pp_cx;             // Context storage for push/pop.
    ContextDesc *pp_cx_history;     // Reservoir for popped contexts.
    bool pp_no_display;             // Suppress window redrawing.

    static cPushPop *instancePtr;
};

#endif

