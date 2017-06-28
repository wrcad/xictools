
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: si_variable.cc,v 5.48 2017/04/13 17:06:28 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "geo_zlist.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_handle.h"
#include "si_daemon.h"
#include "si_lexpr.h"
#include "si_lspec.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// This may already be typedefed, but not in mingw and Solaris 2.6,
// at least
typedef int int32_t;

//
// Variables and arrays
//

namespace {
    void
    clear__string(Variable *v)
    {
        if (v->flags & VF_ORIGINAL)
            delete [] v->content.string;
    }

    void
    clear__array(Variable *v)
    {
        if (v->flags & VF_ORIGINAL)
            delete v->content.a;
    }

    void
    clear__zlist(Variable *v)
    {
        v->content.zlist->free();
    }

    void
    clear__lexpr(Variable *v)
    {
        delete v->content.lspec;
    }

    void
    clear__ldorig(Variable *v)
    {
        delete v->content.ldorig;
    }
}

void(*Variable::clear_tab[NUM_TYPES])(Variable*) =
{
    clear__string,          // TYP_NOTYPE
    clear__string,          // TYP_STRING
    0,                      // TYP_SCALAR
    clear__array,           // TYP_ARRAY
    0,                      // TYP_CMPLX
    clear__zlist,           // TYP_ZLIST
    clear__lexpr,           // TYP_LEXPR
    0,                      // TYP_HANDLE
    clear__ldorig           // TYP_LDORIG
};


// For TYP_HANDLE:  return false if the variable is "0", used for
// conditionals.
//
bool
Variable::istrue__handle()
{
    int id = (int)content.value;
    if (id <= 0)
        return (false);
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (false);
    if (hdl->data || hdl->type == HDLfd)
        return (true);
    hdl->close(id);
    return (false);
}


void
Variable::incr__handle(Variable *res)
{
    int id = (int)content.value;
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        // warning: hdl may be freed in iterator
        if (hdl->type == HDLstring) {
            char *s = (char*)hdl->iterator();
            delete [] s;
        }
        else if (hdl->type == HDLobject) {
            bool cp = ((sHdlObject*)hdl)->copies;
            CDo *obj = (CDo*)hdl->iterator();
            if (cp)
                delete obj;
        }
        else if (hdl->type == HDLgen) {
            int tid = (long)hdl->iterator();
            sHdl *nhdl = sHdl::get(tid);
            if (nhdl)
                nhdl->close(tid);
        }
        else
            hdl->iterator();

        if (sHdl::get(id)) {
            res->type = TYP_HANDLE;
            res->content.value = id;
        }
    }
}


void
Variable::cat__handle(Variable *v, Variable *res)
{
    int id1 = (int)content.value;
    int id2 = (int)v->content.value;

    sHdl *hdl1 = sHdl::get(id1);
    sHdl *hdl2 = sHdl::get(id2);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl1 && hdl2 && hdl1->type == hdl2->type) {

        if (hdl1->type == HDLstring) {
            stringlist *s0 = (stringlist*)hdl2->data;
            s0 = s0->dup();
            if (!hdl1->data)
                hdl1->data = s0;
            else {
                stringlist *s = (stringlist*)hdl1->data;
                while (s->next)
                    s = s->next;
                s->next = s0;
            }
            res->type = TYP_HANDLE;
            res->content.value = id1;
        }
        else if (hdl1->type == HDLobject) {
            bool copies1 = ((sHdlObject*)hdl1)->copies;
            bool copies2 = ((sHdlObject*)hdl2)->copies;
            if (copies1 != copies2)
                return;

            // Make sure that there are no duplicate objects, as these
            // can cause trouble.
            sHdlUniq *hu = sHdl::new_uniq(hdl1);

            CDol *ol0 = 0, *ole = 0;
            CDol *o2 = (CDol*)hdl2->data;
            for (CDol *o = o2; o; o = o->next) {
                hdl2->data = o;
                if (!hu || !hu->test(hdl2)) {
                    if (!ol0)
                        ol0 = ole = new CDol(o->odesc, 0);
                    else {
                        ole->next = new CDol(o->odesc, 0);
                        ole = ole->next;
                    }
                    if (copies2)
                        ole->odesc = ole->odesc->copyObject();
                }
            }
            hdl2->data = o2;
            delete hu;

            if (!hdl1->data)
                hdl1->data = ol0;
            else {
                CDol *o = (CDol*)hdl1->data;
                while (o->next)
                    o = o->next;
                o->next = ol0;
            }
            res->type = TYP_HANDLE;
            res->content.value = id1;
        }
        else if (hdl1->type == HDLprpty) {
            sHdlUniq *hu = sHdl::new_uniq(hdl1);

            CDpl *pl0 = 0, *ple = 0;
            CDpl *p2 = (CDpl*)hdl2->data;
            for (CDpl *p = p2; p; p = p->next) {
                hdl2->data = p;
                if (!hu || !hu->test(hdl2)) {
                    if (!pl0)
                        pl0 = ple = new CDpl(p->pdesc, 0);
                    else {
                        ple->next = new CDpl(p->pdesc, 0);
                        ple = ple->next;
                    }
                }
            }
            hdl2->data = p2;
            delete hu;

            if (!hdl1->data)
                hdl1->data = pl0;
            else {
                CDpl *p = (CDpl*)hdl1->data;
                while (p->next)
                    p = p->next;
                p->next = pl0;
            }
            res->type = TYP_HANDLE;
            res->content.value = id1;
        }
        else if (
                hdl1->type == HDLnode   || hdl1->type == HDLterminal ||
                hdl1->type == HDLdevice || hdl1->type == HDLdcontact ||
                hdl1->type == HDLsubckt || hdl1->type == HDLscontact) {
            sHdlUniq *hu = sHdl::new_uniq(hdl1);

            tlist<void> *t0 = 0, *te = 0;
            tlist<void> *t2 = (tlist<void>*)hdl2->data;
            for (tlist<void> *t = t2; t; t = t->next) {
                hdl2->data = t;
                if (!hu || !hu->test(hdl2)) {
                    if (!t0)
                        t0 = te = new tlist<void>(t->elt, 0);
                    else {
                        te->next = new tlist<void>(t->elt, 0);
                        te = te->next;
                    }
                }
            }
            hdl2->data = t2;
            delete hu;

            if (!hdl1->data)
                hdl1->data = t0;
            else {
                tlist<void> *t = (tlist<void>*)hdl1->data;
                while (t->next)
                    t = t->next;
                t->next = t0;
            }
            res->type = TYP_HANDLE;
            res->content.value = id1;
        }
    }
}
// End of Variable functions.


//---- Start of dispatch functions ----

namespace {
    void
    safe_del__string(Variable *v)
    {
        bool found_alias = false;
        char *start = v->content.string;
        if (start) {
            char *end = start + strlen(start);
            for (Variable *vv = SIparse()->getVariables(); vv;
                    vv = vv->next) {
                if (vv->type != TYP_STRING || vv == v)
                    continue;
                if (vv->content.string == start) {
                    // vv is an alias of v, keep it
                    if (v->flags & VF_ORIGINAL) {
                        vv->flags |= VF_ORIGINAL;
                        v->flags &= ~VF_ORIGINAL;
                    }
                    found_alias = true;
                    break;
                }
            }
            if (!found_alias) {
                for (Variable *vv = SIparse()->getVariables(); vv;
                        vv = vv->next) {
                    if (vv->type != TYP_STRING || vv == v)
                        continue;
                    if (vv->content.string > start &&
                            vv->content.string <= end) {
                        // vv is a pointer into v, initialize vv
                        vv->type = TYP_NOTYPE;
                        vv->content.string = vv->name;
                    }
                }
            }
            if (v->flags & VF_ORIGINAL)
                delete [] v->content.string;
        }
        v->type = TYP_NOTYPE;
        v->flags = 0;
        v->content.string = v->name;
    }

    void
    safe_del__scalar(Variable *v)
    {
        v->type = TYP_NOTYPE;
        v->flags = 0;
        v->content.string = v->name;
    }

    void
    safe_del__array(Variable *v)
    {
        if (v->content.a) {
            if (v->content.a->refptr()) {
                // v is a pointer
                v->content.a->refptr()->decref();
                delete v->content.a;
            }
            else {
                bool found_alias = false;
                for (Variable *vv = SIparse()->getVariables(); vv;
                        vv = vv->next) {
                    if (vv->type != TYP_ARRAY || vv == v)
                        continue;
                    if (vv->content.a == v->content.a) {
                        // vv is an alias of v, keep it
                        if (v->flags & VF_ORIGINAL) {
                            vv->flags |= VF_ORIGINAL;
                            v->flags &= ~VF_ORIGINAL;
                        }
                        found_alias = true;
                        break;
                    }
                }
                if (!found_alias) {
                    // Since there wasn't an alias, we need to init
                    // any pointers to v.
                    for (Variable *vv = SIparse()->getVariables(); vv;
                            vv = vv->next) {
                        if (vv->type != TYP_ARRAY || vv == v)
                            continue;

                        if (vv->content.a->refptr() == v->content.a) {
                            // vv is a pointer to v, initialize vv
                            delete vv->content.a;
                            vv->type = TYP_NOTYPE;
                            vv->content.string = vv->name;
                        }
                    }
                }
                if (v->flags & VF_ORIGINAL)
                    delete v->content.a;
            }
        }
        v->type = TYP_NOTYPE;
        v->flags = 0;
        v->content.string = v->name;
    }

    void
    safe_del__cmplx(Variable *v)
    {
        v->type = TYP_NOTYPE;
        v->flags = 0;
        v->content.string = v->name;
    }

    void
    safe_del__zlist(Variable *v)
    {
        v->content.zlist->free();
        v->type = TYP_NOTYPE;
        v->flags = 0;
        v->content.string = v->name;
    }

    void
    safe_del__lexpr(Variable *v)
    {
        delete v->content.lspec;
        v->type = TYP_NOTYPE;
        v->flags = 0;
        v->content.string = v->name;
    }

    void
    safe_del__handle(Variable *v)
    {
        // Same as calling Close(), except variable becomes undefined
        // rather than scalar 0.
        int id = (int)v->content.value;
        sHdl *hdl = sHdl::get(id);
        if (hdl)
            hdl->close(id);
        v->type = TYP_NOTYPE;
        v->flags = 0;
        v->content.string = v->name;
    }

    void
    safe_del__ldorig(Variable *v)
    {
        delete v->content.ldorig;
        v->type = TYP_NOTYPE;
        v->flags = 0;
        v->content.string = v->name;
    }
}

//---- End safe_delete ----

namespace {
    void
    gc_argv__string(Variable *v, Variable *res, PNodeType)
    {
        // The variable returned from a PT_VAR never has
        // VF_ORIGINAL set.  However, some function or operator
        // nodes may create new data, in which case the
        // VF_ORIGINAL flag is set.  In this case, since the scope
        // has ended, the data should be freed.

        if (!(v->flags & VF_ORIGINAL) || !v->content.string)
            return;  // not original or no data
        // Don't free if string passed to result
        char *start = v->content.string;
        char *end = start + strlen(start);
        if (res->type == TYP_STRING && res->content.string >= start &&
                res->content.string <= end) {
            // The res is a pointer into the string.  This can come from
            // operator returns like res = string_func() + N.  Make the
            // result VF_ORIGINAL
            if (v->content.string != res->content.string) {
                res->content.string = lstring::copy(res->content.string);
                delete [] v->content.string;
            }
            res->flags |= VF_ORIGINAL;
        }
        else
            delete [] v->content.string;
        v->content.string = 0;
    }

    void
    gc_argv__zlist(Variable *v, Variable *res, PNodeType)
    {
        // Every operator or function return is a new list, or one
        // of the arguments.  A return from a variable access, or
        // from an assignemnt return (z = (z1 = z2)) has VF_NAMED
        // set.  The rule is that if VF_NAMED is not set, and the
        // argument differs from the return, it is freed.

        if (v->flags & VF_NAMED)
            return;
        if (res->type == TYP_ZLIST && res->content.zlist == v->content.zlist)
            return;
        v->content.zlist->free();
        v->content.zlist = 0;
    }

    void
    gc_argv__handle(Variable *v, Variable *res, PNodeType ntype)
    {
        // Argument is a function call returning a handle
        if (ntype == PT_FUNCTION) {
            int id = (int)v->content.value;
            if (id > 0) {
                if (res->type != TYP_HANDLE || res->content.value != id) {
                    sHdl *hdl = sHdl::get(id);
                    if (hdl)
                        hdl->close(id);
                }
                v->content.value = 0;
            }
        }
    }
}

//---- End gc_argv ----

namespace {
    void
    gc_result__string(Variable *v)
    {
        if (v->flags & VF_ORIGINAL) {
            delete [] v->content.string;
            v->content.string = 0;
        }
    }

    void
    gc_result__zlist(Variable *v)
    {
        if (!(v->flags & VF_NAMED)) {
            v->content.zlist->free();
            v->content.zlist = 0;
        }
    }

    void
    gc_result__lexpr(Variable *v)
    {
        if (!(v->flags & VF_NAMED)) {
            delete v->content.lspec;
            v->content.lspec = 0;
        }
    }
}

//---- End gc_result ----

namespace {
    // We are deleting oldstr, and replacing it with newstr if newstr is
    // not nil.  Search through the variables for pointers into oldstr,
    // and update them to newstr, or invalidate them.  This should be
    // called after a string variable with the "orginal" flag set has
    // been given a new string.
    //
    void
    update_string(char *oldstr, char *newstr)
    {
        if (!oldstr)
            return;
        char *start = oldstr;
        char *end = start + strlen(start);
        for (Variable *v = SIparse()->getVariables(); v; v = v->next) {
            if (v->type == TYP_STRING && v->content.string >= start &&
                    v->content.string <= end) {
                if (v->flags & VF_ORIGINAL)
                    // caller should have already done the replacement, but
                    // I suppose it is ok to defer
                    continue;
                if (newstr) {
                    int offset = v->content.string - start;
                    int newlen = strlen(newstr);
                    if (offset > newlen)
                        offset = newlen;
                    v->content.string = newstr + offset;
                }
                else {
                    // invalidate
                    v->type = TYP_NOTYPE;
                    v->content.string = v->name;
                }
            }
        }
    }

    bool
    assign__string(siVariable *v, ParseNode *p, Variable *res, void *datap)
    {
        siVariable r1, r2;
        if (p->left->left) {
            bool err = (*p->left->left->evfunc)(p->left->left, &r1, datap);
            if (err)
                return (err);
            if (r1.type != TYP_SCALAR) {
                SIparse()->pushError(
                    "-enon-scalar string subscript in assignment");
                return (BAD);
            }
            int indx = (int)r1.content.value;
            if (indx < 0)
                indx = 0;
            if (indx >= (int)strlen(v->content.string)) {
                SIparse()->pushError(
                    "-estring subscript out of range in assignment");
                return (BAD);
            }
            err = (*p->right->evfunc)(p->right, &r2, datap);
            if (err != OK)
                return (err);
            if (r2.type != TYP_SCALAR) {
                SIparse()->pushError("-enon-scalar character assignment");
                return (BAD);
            }
            *(v->content.string + indx) = (char)r2.content.value;
            res->set_result(&r2);
            res->flags |= VF_NAMED;
            return (OK);
        }
        bool err = (*p->right->evfunc)(p->right, &r2, datap);
        if (err != OK)
            return (err);
        if (v->type == TYP_NOTYPE) {
            v->type = (r2.type == TYP_NOTYPE ? (VarType)TYP_STRING : r2.type);
            v->content = r2.content;
            v->assign_fix(&r2);
        }
        else if (v->type == TYP_STRING) {
            if (r2.type == TYP_STRING || r2.type == TYP_NOTYPE) {
                char *oldstr = v->content.string;
                char *newstr = r2.content.string;
                v->content.string = newstr;
                if (oldstr && (v->flags & VF_ORIGINAL)) {
                    if (oldstr <= newstr && newstr <=
                            oldstr + strlen(oldstr)) {
                        // copy is string is assigned into itself
                        newstr = lstring::copy(v->content.string);
                        v->content.string = newstr;
                    }
                    // fix any pointers to oldstr
                    update_string(oldstr, newstr);
                    delete [] oldstr;
                }
                v->assign_fix(&r2);
                // r2->flags & VF_ORIGINAL is set only from a function return,
                // including overloaded operators
            }
            else {
                SIparse()->pushError(
                    "-eillegal type conversion: non-string to string");
                return (BAD);
            }
        }
        res->set_result(&r2);
        res->flags |= VF_NAMED;
        return (OK);
    }

    bool
    assign__scalar(siVariable *v, ParseNode *p, Variable *res, void *datap)
    {
        if (p->left->left) {
            SIparse()->pushError("-esubscript used on scalar value");
            return (BAD);
        }
        siVariable r2;
        bool err = (*p->right->evfunc)(p->right, &r2, datap);
        if (err != OK)
            return (err);
        if (r2.type == TYP_HANDLE)
            v->type = TYP_HANDLE;
        else if (r2.type != TYP_SCALAR) {
            SIparse()->pushError(
                "-eillegal type conversion: nonscalar to scalar");
            return (BAD);
        }
        v->content.value = r2.content.value;
        res->set_result(&r2);
        res->flags |= VF_NAMED;
        return (OK);
    }

    bool
    assign__array(siVariable *v, ParseNode *p, Variable *res, void *datap)
    {
        if (p->left->left) {
            bool ret = ADATA(v->content.a)->assign(res, p->left->left, p->right,
                datap);
            res->flags |= VF_NAMED;
            return (ret);
        }
        siVariable r2;
        bool err = (*p->right->evfunc)(p->right, &r2, datap);
        if (err != OK)
            return (err);
        if (v->flags & VF_ORIGINAL) {
            SIparse()->pushError(
                "-eillegal type conversion: array invariant");
            return (BAD);
        }
        if (r2.type == TYP_SCALAR) {
            // Setting to 0 dereferences pointer
            if (to_boolean(r2.content.value)) {
                SIparse()->pushError(
                    "-eillegal type conversion: scalar to array pointer");
                return (BAD);
            }
            if (v->content.a->refptr()) {
                v->content.a->refptr()->decref();
                delete v->content.a;
            }
            v->content.string = v->name;
            v->type = TYP_NOTYPE;
        }
        else if (r2.type == TYP_ARRAY) {
            if (v->content.a->refptr()) {
                v->content.a->refptr()->decref();
                delete v->content.a;
            }
            v->content.a = r2.content.a;
        }
        else {
            SIparse()->pushError(
                "-eillegal type conversion: string to array pointer");
            return (BAD);
        }

        res->set_result(&r2);
        res->flags |= VF_NAMED;
        return (OK);
    }

    bool
    assign__cmplx(siVariable *v, ParseNode *p, Variable *res, void *datap)
    {
        if (p->left->left) {
            SIparse()->pushError("-esubscript used on complex value");
            return (BAD);
        }
        siVariable r2;
        bool err = (*p->right->evfunc)(p->right, &r2, datap);
        if (err != OK)
            return (err);
        if (r2.type != TYP_CMPLX && r2.type != TYP_SCALAR) {
            SIparse()->pushError(
                "-eillegal type conversion: non-numeric to complex");
            return (BAD);
        }
        v->content.cx.real = r2.content.cx.real;
        v->content.cx.imag = (r2.type == TYP_CMPLX ? r2.content.cx.imag : 0.0);
        res->set_result(&r2);
        res->flags |= VF_NAMED;
        return (OK);
    }

    bool
    assign__zlist(siVariable *v, ParseNode *p, Variable *res, void *datap)
    {
        if (p->left->left) {
            SIparse()->pushError("-esubscript used on scalar value");
            return (BAD);
        }
        siVariable r2;
        bool err = (*p->right->evfunc)(p->right, &r2, datap);
        if (err != OK)
            return (err);
        // Dealing with zoid lists
        // zl from function or operator: pass the pointer
        // zl from variable: copy the list
        // zl from assignment (zl = (zx = ...)): copy the list
        // convenently, VF_NAMED is set when we should copy

        if (r2.type != TYP_ZLIST) {
            SIparse()->pushError(
                "-eillegal type conversion: non-zlist to zlist");
            return (BAD);
        }
        v->content.zlist->free();
        if (r2.flags & VF_NAMED)
            v->content.zlist = r2.content.zlist->copy();
        else
            v->content.zlist = r2.content.zlist;

        res->set_result(&r2);
        res->flags |= VF_NAMED;
        return (OK);
    }

    bool
    assign__lexpr(siVariable*, ParseNode *p, Variable*, void*)
    {
        if (p->left->left) {
            SIparse()->pushError("-esubscript used on scalar value");
            return (BAD);
        }
        SIparse()->pushError(
            "-eassignment to layer expression type not allowed");
        return (BAD);
    }

    bool
    assign__handle(siVariable *v, ParseNode *p, Variable *res, void *datap)
    {
        if (p->left->left) {
            SIparse()->pushError("-esubscript used on handle value");
            return (BAD);
        }
        siVariable r2;
        bool err = (*p->right->evfunc)(p->right, &r2, datap);
        if (err != OK)
            return (err);
        if (r2.type != TYP_HANDLE) {
            if (r2.type != TYP_SCALAR || to_boolean(r2.content.value)) {
                SIparse()->pushError(
                    "-eillegal type conversion: nonhandle to handle");
                return (BAD);
            }
        }
        v->content.value = r2.content.value;
        res->set_result(&r2);
        res->flags |= VF_NAMED;
        return (OK);
    }
}

//---- End assign ----

namespace {
    void
    assign_fix__string(Variable *v, Variable *r)
    {
        if (v->flags & (VF_STATIC | VF_GLOBAL)) {
            v->content.string = lstring::copy(v->content.string);
            v->flags |= VF_ORIGINAL;
        }
        else if (r->flags & VF_ORIGINAL)
            v->flags |= VF_ORIGINAL;
        else
            v->flags &= ~VF_ORIGINAL;
    }

    void
    assign_fix__array(Variable *v, Variable *r)
    {
        if (v->content.a->refptr()) {
            // just made a reference to a pointer, copy the siAryData so
            // we can safely free it if the variable is reassigned
            siAryData *a = new siAryData;
            *a = *ADATA(v->content.a);
            v->content.a = a;
            a->refptr()->incref();
        }
        if (r->flags & VF_ORIGINAL)
            v->flags |= VF_ORIGINAL;
    }

    void
    assign_fix__zlist(Variable *v, Variable *r)
    {
        if (r->flags & VF_NAMED)
            // the source was another named variable
            v->content.zlist = v->content.zlist->copy();
    }
}

//---- End assign_fix ----

namespace {
    bool
    get_var__string(Variable *v, ParseNode *p, Variable *res)
    {
        if (p->left) {
            siVariable r1;
            SIlexprCx cx;
            bool err = (*p->left->evfunc)(p->left, &r1, &cx);
            if (err)
                return (err);
            if (r1.type != TYP_SCALAR) {
                SIparse()->pushError("-enon-scalar string subscript");
                return (BAD);
            }
            int indx = (int)r1.content.value;
            if (indx < 0)
                indx = 0;
            if (indx > (int)strlen(v->content.string)) {
                SIparse()->pushError("-estring subscript out of range");
                return (BAD);
            }
            res->type = TYP_SCALAR;
            res->content.value = v->content.string[indx];
            res->flags |= VF_NAMED;
            return (OK);
        }
        res->set_result(v);
        res->flags |= VF_NAMED;
        return (OK);
    }

    bool
    get_var__scalar(Variable *v, ParseNode *p, Variable *res)
    {
        if (p->left) {
            SIparse()->pushError("-esubscript used on scalar value");
            return (BAD);
        }
        res->set_result(v);
        res->flags |= VF_NAMED;
        return (OK);
    }

    bool
    get_var__array(Variable *v, ParseNode *p, Variable *res)
    {
        if (p->left) {
            bool ret = ADATA(v->content.a)->resolve(res, p->left);
            res->flags |= VF_NAMED;
            return (ret);
        }
        res->set_result(v);
        res->flags |= VF_NAMED;
        return (OK);
    }

    bool
    get_var__cmplx(Variable *v, ParseNode *p, Variable *res)
    {
        if (p->left) {
            SIparse()->pushError("-esubscript used on complex value");
            return (BAD);
        }
        res->set_result(v);
        res->flags |= VF_NAMED;
        return (OK);
    }

    bool
    get_var__zlist(Variable *v, ParseNode *p, Variable *res)
    {
        if (p->left) {
            SIparse()->pushError("-esubscript used on scalar value");
            return (BAD);
        }
        res->set_result(v);
        res->flags |= VF_NAMED;
        return (OK);
    }

    bool
    get_var__lexpr(Variable *v, ParseNode *p, Variable *res)
    {
        if (p->left) {
            SIparse()->pushError("-esubscript used on scalar value");
            return (BAD);
        }
        res->set_result(v);
        res->flags |= VF_NAMED;
        return (OK);
    }

    bool
    get_var__handle(Variable *v, ParseNode *p, Variable *res)
    {
        if (p->left) {
            SIparse()->pushError("-esubscript used on handle value");
            return (BAD);
        }
        res->set_result(v);
        res->flags |= VF_NAMED;
        return (OK);
    }
}

//---- End get_var ----

namespace {
    bool
    bop_look_ahead__scalar(Variable *v, ParseNode *p, Variable *res)
    {
        if (!to_boolean(v[0].content.value)) {
            if (p->optype == TOK_AND ||
                    p->optype == TOK_TIMES ||
                    p->optype == TOK_MOD ||
                    p->optype == TOK_DIVIDE) {
                res->type = TYP_SCALAR;
                res->content.value = 0.0;
                return (true);
            }
        }
        else {
            if (p->optype == TOK_OR) {
                res->type = TYP_SCALAR;
                res->content.value = 1.0;
                return (true);
            }
        }
        return (false);
    }

    bool
    bop_look_ahead__cmplx(Variable *v, ParseNode *p, Variable *res)
    {
        if (!(to_boolean(v[0].content.cx.real) ||
                to_boolean(v[0].content.cx.imag))) {
            if (p->optype == TOK_AND ||
                    p->optype == TOK_TIMES ||
                    p->optype == TOK_MOD ||
                    p->optype == TOK_DIVIDE) {
                res->type = TYP_SCALAR;
                res->content.value = 0.0;
                return (true);
            }
        }
        else {
            if (p->optype == TOK_OR) {
                res->type = TYP_SCALAR;
                res->content.value = 1.0;
                return (true);
            }
        }
        return (false);
    }

    bool
    bop_look_ahead__zlist(Variable *v, ParseNode *p, Variable *res)
    {
        if (v[0].content.zlist == 0) {
            if (p->optype == TOK_AND || p->optype == TOK_TIMES) {
                res->type = TYP_ZLIST;
                res->content.zlist = 0;
                return (true);
            }
        }
        return (false);
    }
}

//---- End bop_look_ahead ----

namespace {
    void
    print_var__string(Variable *v, rvals *rv, char *buf)
    {
        if (!rv) {
            char *s = SIparser::toPrintable(v->content.string);
            if (!s) {
                strcpy(buf, "(null)");
                return;
            }
            int len = strlen(s);
            if (len > 254) {
                strcpy(s + 251 , " ...");
                len = 255;
            }
            strcpy(buf, s);
            delete [] s;
        }
        else {
            if (!v->content.string) {
                strcpy(buf, "(null)");
                return;
            }
            if (rv->rmax < 0 && rv->d1 > 0)
                rv->rmax = rv->d1;
            int len = strlen(v->content.string);
            if (rv->rmin >= 0 && rv->rmax >= 0) {
                if (rv->rmax >= rv->rmin) {
                    if (rv->rmin > len - 1)
                        strcpy(buf, "range error");
                    else {
                        if (rv->rmax > len - 1)
                            rv->rmax = len - 1;
                        char *s = new char [rv->rmax - rv->rmin + 2];
                        char *t = s;
                        for (int j = rv->rmin; j <= rv->rmax; j++)
                            *s++ = v->content.string[j];
                        *s = 0;
                        s = SIparser::toPrintable(t);
                        delete [] t;
                        if (strlen(s) > 254)
                            strcpy(s + 251 , " ...");
                        strcpy(buf, s);
                        delete [] s;
                    }
                }
                else {
                    if (rv->rmax > len - 1)
                        strcpy(buf, "range error");
                    else {
                        if (rv->rmin > len - 1)
                            rv->rmin = len - 1;
                        char *s = new char [rv->rmin - rv->rmax + 2];
                        char *t = s;
                        for (int j = rv->rmin; j >= rv->rmax; j--)
                            *s++ = v->content.string[j];
                        *s = 0;
                        s = SIparser::toPrintable(t);
                        delete [] t;
                        if (strlen(s) > 254)
                            strcpy(s + 251 , " ...");
                        strcpy(buf, s);
                        delete [] s;
                    }
                }
            }
            else if (rv->rmin >= 0) {
                if (rv->rmin > len - 1) {
                    strcpy(buf, "range error");
                    return;
                }
                if (rv->minus) {
                    char *s =
                        SIparser::toPrintable(v->content.string + rv->rmin);
                    if (strlen(s) > 254)
                        strcpy(s + 251 , " ...");
                    strcpy(buf, s);
                    delete [] s;
                }
                else {
                    buf[0] = v->content.string[rv->rmin];
                    buf[1] = 0;
                    char *s = SIparser::toPrintable(buf);
                    strcpy(buf, s);
                    delete [] s;
                }
            }
            else if (rv->rmax > 0 && rv->minus) {
                if (rv->rmax > len - 1)
                    rv->rmax = len - 1;
                strncpy(buf, v->content.string, rv->rmax);
                buf[rv->rmax] = 0;
                char *s = SIparser::toPrintable(buf);
                strcpy(buf, s);
                delete [] s;
            }
            else
                strcpy(buf, "range error");
        }
    }

    void
    print_var__scalar(Variable *v, rvals *rv, char *buf)
    {
        if (!rv)
            sprintf(buf, "%g", v->content.value);
        else
            strcpy(buf, "range error");
    }

    void
    print_var__array(Variable *v, rvals *rv, char *buf)
    {
        if (!rv) {
            int len = v->content.a->length();
            if (len > 5)
                len = 5;
            char *s = buf;
            for (int j = 0; j < len; j++) {
                sprintf(s, "%g ", v->content.a->values()[j]);
                while (*s)
                    s++;
            }
            if (v->content.a->length() > len)
                strcpy(s, "...");
        }
        else {
            int *dims = v->content.a->dims();
            bool err = false;
            int os = 0;
            if (rv->d2 > 0) {
                if (dims[2] > 0) {
                    if (rv->d2 < dims[2])
                        os += rv->d2*dims[1]*dims[0];
                    else
                        err = true;
                }
                else
                    err = true;
            }
            if (rv->d1 > 0) {
                if (dims[1] > 0) {
                    if (rv->d1 < dims[1])
                        os += rv->d1*dims[0];
                    else
                        err = true;
                }
                else
                    err = true;
            }
            double *vals = v->content.a->values() + os;

            if (err)
                strcpy(buf, "range error");
            else if (rv->rmin >= 0 && rv->rmax >= 0) {
                if (rv->rmax >= rv->rmin) {
                    if (rv->rmin > dims[0] - 1)
                        strcpy(buf, "range error");
                    else {
                        if (rv->rmax > dims[0] - 1)
                            rv->rmax = dims[0] - 1;
                        int i = 0;
                        char *s = buf;
                        for (int j = rv->rmin; j <= rv->rmax; j++) {
                            if (i == 5) {
                                strcpy(s, "...");
                                break;
                            }
                            sprintf(s, "%g ", vals[j]);
                            while (*s)
                                s++;
                            i++;
                        }
                    }
                }
                else {
                    if (rv->rmax > dims[0] - 1)
                        strcpy(buf, "range error");
                    else {
                        if (rv->rmin > dims[0] - 1)
                            rv->rmin = dims[0] - 1;
                        int i = 0;
                        char *s = buf;
                        for (int j = rv->rmin; j >= rv->rmax; j--) {
                            if (i == 5) {
                                strcpy(s, "...");
                                break;
                            }
                            sprintf(s, "%g ", vals[j]);
                            while (*s)
                                s++;
                            i++;
                        }
                    }
                }
            }
            else if (rv->rmin >= 0) {
                if (rv->rmin > dims[0] - 1) {
                    strcpy(buf, "range error");
                    return;
                }
                if (rv->minus) {
                    int i = 0;
                    char *s = buf;
                    for (int j = rv->rmin; j < dims[0]; j++) {
                        if (i == 5) {
                            strcpy(s, "...");
                            return;
                        }
                        sprintf(s, "%g ", vals[j]);
                        while (*s)
                            s++;
                        i++;
                    }
                }
                else
                    sprintf(buf, "%g", vals[rv->rmin]);
            }
            else if (rv->rmax >= 0 && rv->minus) {
                if (rv->rmax > dims[0] - 1)
                    rv->rmax = dims[0] - 1;
                int i = 0;
                char *s = buf;
                for (int j = 0; j <= rv->rmax; j++) {
                    if (i == 5) {
                        strcpy(s, "...");
                        return;
                    }
                    sprintf(s, "%g ", vals[j]);
                    while (*s)
                        s++;
                    i++;
                }
            }
            else
                strcpy(buf, "range error");
        }
    }

    void
    print_var__cmplx(Variable *v, rvals *rv, char *buf)
    {
        if (!rv)
            sprintf(buf, "(%g,%g)", v->content.cx.real, v->content.cx.imag);
        else
            strcpy(buf, "range error");
    }

    void
    print_var__handle(Variable *v, rvals *rv, char *buf)
    {
        if (!rv)
            sprintf(buf, "%g", v->content.value);
        else
            strcpy(buf, "range error");
    }

    void
    print_var__ldorig(Variable *v, rvals *rv, char *buf)
    {
        if (!rv)
            strcpy(buf, v->content.ldorig->origname());
        else
            strcpy(buf, "range error");
    }
}

//---- End print_var ----

namespace {
    char *
    set_var__string(Variable *v, rvals *rv, const char *val)
    {
        if (!rv) {
            if (!v->content.string) {
                v->content.string = SIparser::fromPrintable(val);
                v->flags |= VF_ORIGINAL;
            }
            else {
                char *s = SIparser::fromPrintable(val);
                if (strlen(v->content.string) <= strlen(s)) {
                    strcpy(v->content.string, s);
                    delete [] s;
                }
                else {
                    if (v->flags & VF_ORIGINAL)
                        delete [] v->content.string;
                    v->content.string = s;
                    v->flags |= VF_ORIGINAL;
                }
            }
            v->type = TYP_STRING;
        }
        else {
            if (!v->content.string)
                return (lstring::copy("String is null, can't assign range."));
            if (rv->rmax < 0 && rv->d1 > 0)
                rv->rmax = rv->d1;
            int len = strlen(v->content.string);
            if (rv->rmin >= 0 && rv->rmax >= 0) {
                if (rv->rmax >= rv->rmin) {
                    if (rv->rmin > len - 1)
                        return (lstring::copy("Assignment out of range."));
                    if (rv->rmax > len - 1)
                        rv->rmax = len - 1;
                    char *s = SIparser::fromPrintable(val);
                    char *t = s;
                    for (int j = rv->rmin; j <= rv->rmax; j++) {
                        if (!*s)
                            break;
                        v->content.string[j] = *s;
                        s++;
                    }
                    delete [] t;
                    v->type = TYP_STRING;
                }
                else {
                    if (rv->rmax > len - 1)
                        return (lstring::copy("Assignment out of range."));
                    if (rv->rmin > len - 1)
                        rv->rmin = len - 1;
                    char *s = SIparser::fromPrintable(val);
                    char *t = s;
                    for (int j = rv->rmin; j >= rv->rmax; j--) {
                        if (!*s)
                            break;
                        v->content.string[j] = *s;
                        s++;
                    }
                    delete [] t;
                    v->type = TYP_STRING;
                }
            }
            else if (rv->rmin >= 0) {
                if (rv->rmin > len - 1)
                    return (lstring::copy("Assignment out of range."));
                if (rv->minus) {
                    char *s = SIparser::fromPrintable(val);
                    char *t = s;
                    for (int j = rv->rmin; j < len; j++) {
                        if (!*s)
                            break;
                        v->content.string[j] = *s;
                        s++;
                    }
                    delete [] t;
                    v->type = TYP_STRING;
                }
                else {
                    char *s = SIparser::fromPrintable(val);
                    v->content.string[rv->rmin] = *s;
                    delete [] s;
                    v->type = TYP_STRING;
                }
            }
            else if (rv->rmax >= 0 && rv->minus) {
                if (rv->rmax > len - 1)
                    rv->rmax = len - 1;
                char *s = SIparser::fromPrintable(val);
                char *t = s;
                for (int j = 0; j <= rv->rmax; j++) {
                    if (!*s)
                        break;
                    v->content.string[j] = *s;
                    s++;
                }
                delete [] t;
                v->type = TYP_STRING;
            }
            else
                return (lstring::copy("Parse error in range."));
        }
        return (0);
    }

    char *
    set_var__scalar(Variable *v, rvals *rv, const char *val)
    {
        if (!rv) {
            double dval;
            if (sscanf(val, "%lf", &dval)) {
                v->content.value = dval;
                return (0);
            }
            else
                return (lstring::copy(
                    "Bad scalar data, variable not changed."));
        }
        else
            return (lstring::copy(
                "Error, subscript on scalar, variable not changed."));
    }

    char *
    set_var__array(Variable *v, rvals *rv, const char *val)
    {
        char buf[256];
        double dval;
        if (!rv) {
            char *tok;
            int i = 0;
            while ((tok = lstring::gettok(&val)) != 0) {
                if (!strcmp(tok, "...")) {
                    delete [] tok;
                    break;
                }
                int ss = sscanf(tok, "%lf", &dval);
                delete [] tok;
                if (i >= v->content.a->length())
                    break;
                if (ss == 1)
                    v->content.a->values()[i] = dval;
                else {
                    sprintf(buf, "Bad vector data, index %d.", i);
                    return (lstring::copy(buf));
                }
                i++;
            }
        }
        else {
            int *dims = v->content.a->dims();
            bool err = false;
            int os = 0;
            if (rv->d2 > 0) {
                if (dims[2] > 0) {
                    if (rv->d2 < dims[2])
                        os += rv->d2*dims[1]*dims[0];
                    else
                        err = true;
                }
                else
                    err = true;
            }
            if (rv->d1 > 0) {
                if (dims[1] > 0) {
                    if (rv->d1 < dims[1])
                        os += rv->d1*dims[0];
                    else
                        err = true;
                }
                else
                    err = true;
            }
            double *vals = v->content.a->values() + os;

            if (err)
                return (lstring::copy("Assignment out of range."));
            else if (rv->rmin >= 0 && rv->rmax >= 0) {
                if (rv->rmax >= rv->rmin) {
                    if (rv->rmin > dims[0] - 1)
                        return (lstring::copy("Assignment out of range."));
                    if (rv->rmax > dims[0] - 1)
                        rv->rmax = dims[0] - 1;

                    int i = rv->rmin;
                    char *tok;
                    while ((tok = lstring::gettok(&val)) != 0) {
                        if (!strcmp(tok, "...")) {
                            delete [] tok;
                            break;
                        }
                        int ss = sscanf(tok, "%lf", &dval);
                        delete [] tok;
                        if (ss == 1)
                            vals[i] = dval;
                        else {
                            sprintf(buf, "Bad vector data, index %d.", i);
                            return (lstring::copy(buf));
                        }
                        i++;
                        if (i > rv->rmax)
                            break;
                    }
                }
                else {
                    if (rv->rmax > dims[0] - 1)
                        return (lstring::copy("Assignment out of range."));
                    if (rv->rmin > dims[0] - 1)
                        rv->rmin = dims[0] - 1;

                    int i = rv->rmin;
                    char *tok;
                    while ((tok = lstring::gettok(&val)) != 0) {
                        if (!strcmp(tok, "...")) {
                            delete [] tok;
                            break;
                        }
                        int ss = sscanf(tok, "%lf", &dval);
                        delete [] tok;
                        if (ss == 1)
                            vals[i] = dval;
                        else {
                            sprintf(buf, "Bad vector data, index %d.", i);
                            return (lstring::copy(buf));
                        }
                        i--;
                        if (i < rv->rmax)
                            break;
                    }
                }
            }
            else if (rv->rmin >= 0) {
                if (rv->rmin > dims[0] - 1)
                    return (lstring::copy("Assignment out of range."));
                if (rv->minus) {
                    int i = rv->rmin;
                    char *tok;
                    while ((tok = lstring::gettok(&val)) != 0) {
                        if (!strcmp(tok, "...")) {
                            delete [] tok;
                            break;
                        }
                        int ss = sscanf(tok, "%lf", &dval);
                        delete [] tok;
                        if (ss == 1)
                            vals[i] = dval;
                        else {
                            sprintf(buf, "Bad vector data, index %d.", i);
                            return (lstring::copy(buf));
                        }
                        i++;
                        if (i >= dims[0])
                            break;
                    }
                }
                else {
                    if (sscanf(val, "%lf", &dval)) {
                        vals[rv->rmin] = dval;
                        return (0);
                    }
                    else
                        return (lstring::copy(
                            "Bad scalar data, variable not changed."));
                }
            }
            else if (rv->rmax >= 0 && rv->minus) {
                if (rv->rmax > dims[0] - 1)
                    rv->rmax = dims[0] - 1;
                int i = 0;
                char *tok;
                while ((tok = lstring::gettok(&val)) != 0) {
                    if (!strcmp(tok, "...")) {
                        delete [] tok;
                        break;
                    }
                    int ss = sscanf(tok, "%lf", &dval);
                    delete [] tok;
                    if (ss == 1)
                        vals[i] = dval;
                    else {
                        sprintf(buf, "Bad vector data, index %d.", i);
                        return (lstring::copy(buf));
                    }
                    i++;
                    if (i > rv->rmax)
                        break;
                }
            }
            else
                return (lstring::copy("Parse error in range."));
        }
        return (0);
    }

    char *
    set_var__cmplx(Variable *v, rvals *rv, const char *val)
    {
        if (!rv) {
            double rval, ival;
            if (sscanf(val, "(%lf,%lf)", &rval, &ival) == 2) {
                v->content.cx.real = rval;
                v->content.cx.imag = ival;
                return (0);
            }
            else
                return (lstring::copy(
                    "Bad complex data, variable not changed."));
        }
        else
            return (lstring::copy(
                "Error, subscript on complex, variable not changed."));
    }

    char *
    set_var__handle(Variable *v, rvals *rv, const char *val)
    {
        if (!rv) {
            int indx;
            if (sscanf(val, "%d", &indx) && indx >= 0) {
                v->content.value = indx;
                return (0);
            }
            else
                return (lstring::copy(
                    "Bad handle value, variable not changed."));
        }
        else
            return (lstring::copy(
                "Error, subscript on handle, variable not changed."));
    }
}

//---- End set_var ----

namespace {
    // Response functions for server mode

    // Put the double into "network byte order" which is defined here as
    // the same order Sun (sparc) uses (reverse that of I386).
    //
    void
    dton(char *s, double d)
    {
        union { double d; unsigned char s[8]; } u;
        u.d = 1.0;
        if (u.s[7]) {
            // This means MSB's are at top address, reverse bytes
            u.d = d;
            for (int i = 7; i >= 0; i--)
                *s++ = u.s[i];
        }
        else
            memcpy(s, &d, 8);
    }


    int
    respond__notype(Variable*, int g, bool)
    {
        // |ok(4)|
        int32_t o = htonl(RSP_OK);
        int ret = send(g, (char*)&o, 4, 0);
        return (ret);
    }

    int
    respond__string(Variable *v, int g, bool longform)
    {
        int ret;
        if (longform) {
            const char *string = v->content.string;
            if (!string)
                string = "";
            int len = strlen(string) + 1;

            // |string(4)|len(4)|"..."|
            char *s = new char[len + 8];
            int32_t *ii = (int32_t*)&s[0];
            ii[0] = htonl(RSP_STRING | LONGFORM_FLAG);
            ii[1] = htonl(len);
            strcpy(s+8, string);
            ret = send(g, s, len + 8, 0);
            delete [] s;
        }
        else {
            // |string(4)|
            int32_t o = htonl(RSP_STRING);
            ret = send(g, (char*)&o, 4, 0);
        }
        return (ret);
    }

    int
    respond__scalar(Variable *v, int g, bool longform)
    {
        int ret;
        if (longform) {
            // |scalar(4)|val(8)|
            char s[12];
            int32_t *ii = (int32_t*)&s[0];
            *ii = htonl(RSP_SCALAR | LONGFORM_FLAG);
            dton(s+4, v->content.value);
            ret = send(g, s, 12, 0);
        }
        else {
            // |scalar(4)|
            int32_t o = htonl(RSP_SCALAR);
            ret = send(g, (char*)&o, 4, 0);
        }
        return (ret);
    }

    int
    respond__array(Variable *v, int g, bool longform)
    {
        int ret;
        if (longform) {
            // |array(4)|len(4)|...(len*8)|
            int len = v->content.a->length();
            int sz = (1 + len)*8;
            char *s = new char[sz];
            int32_t *ii = (int32_t*)&s[0];
            ii[0] = htonl(RSP_ARRAY | LONGFORM_FLAG);
            ii[1] = htonl(len);
            double *dd = (double*)&s[8];
            for (int i = 0; i < len; i++)
                dton((char*)&dd[i], v->content.a->values()[i]);
            ret = send(g, s, sz, 0);
            delete [] s;
        }
        else {
            // |array(4)|
            int32_t o = htonl(RSP_ARRAY);
            ret = send(g, (char*)&o, 4, 0);
        }
        return (ret);
    }

    int
    respond__cmplx(Variable *v, int g, bool longform)
    {
        int ret;
        if (longform) {
            // |CMPLX(4)|val(8)val(8)|
            char s[20];
            int32_t *ii = (int32_t*)&s[0];
            *ii = htonl(RSP_CMPLX | LONGFORM_FLAG);
            dton(s+4, v->content.cx.real);
            dton(s+12, v->content.cx.imag);
            ret = send(g, s, 20, 0);
        }
        else {
            // |cmplx(4)|
            int32_t o = htonl(RSP_CMPLX);
            ret = send(g, (char*)&o, 4, 0);
        }
        return (ret);
    }

    int
    respond__zlist(Variable *v, int g, bool longform)
    {
        int ret;
        if (longform) {
            // |zlist(4)|len(4)|...(len*6*4)|
            // order: xll xlr yl xul xur yu
            Zlist *zl = v->content.zlist;
            int len = Zlist::length(zl);
            int32_t *vals = new int32_t[len*6 + 2];
            vals[0] = htonl(RSP_ZLIST | LONGFORM_FLAG);
            vals[1] = htonl(len);
            int32_t *vv = vals + 2;
            for ( ; zl; zl = zl->next) {
                *vv++ = htonl(zl->Z.xll);
                *vv++ = htonl(zl->Z.xlr);
                *vv++ = htonl(zl->Z.yl);
                *vv++ = htonl(zl->Z.xul);
                *vv++ = htonl(zl->Z.xur);
                *vv++ = htonl(zl->Z.yu);
            }
            ret = send(g, (char*)vals, (len*6+2)*4, 0);
            delete [] vals;
        }
        else {
            // |zlist(4)|
            int32_t o = htonl(RSP_ZLIST);
            ret = send(g, (char*)&o, 4, 0);
        }
        return (ret);
    }

    int
    respond__lexpr(Variable *v, int g, bool longform)
    {
        int ret;
        if (longform) {
            // |lexpr|string\r\n|
            char *le = v->content.lspec->string();
            int sz = strlen(le) + 9;
            char *s = new char[sz];
            int32_t *ii = (int32_t*)&s[0];
            ii[0] = htonl(RSP_LEXPR | LONGFORM_FLAG);
            ii[1] = htonl(strlen(le) + 1);
            strcpy(s+8, le);
            delete [] le;
            ret = send(g, s, sz, 0);
            delete [] s;
        }
        else {
            // |lexpr(4)|
            int32_t o = htonl(RSP_LEXPR);
            ret = send(g, (char*)&o, 4, 0);
        }
        return (ret);
    }

    int
    respond__handle(Variable *v, int g, bool longform)
    {
        int ret;
        if (longform) {
            // |handle(4)|id(4)|
            char s[8];
            int32_t *ii = (int32_t*)&s[0];
            ii[0] = htonl(RSP_HANDLE | LONGFORM_FLAG);
            ii[1] = htonl((int)v->content.value);
            ret = send(g, s, 8, 0);
        }
        else {
            // |handle(4)|
            int32_t o = htonl(RSP_HANDLE);
            ret = send(g, (char*)&o, 4, 0);
        }
        return (ret);
    }
}
//---- End of dispatch functions ----

// Initialization of tables.  The ordering must match the enum values.

void(*siVariable::safe_del_tab[NUM_TYPES])(Variable*) =
{
    safe_del__scalar,       // TYP_NOTYPE
    safe_del__string,       // TYP_STRING
    safe_del__scalar,       // TYP_SCALAR
    safe_del__array,        // TYP_ARRAY
    safe_del__cmplx,        // TYP_CMPLX
    safe_del__zlist,        // TYP_ZLIST
    safe_del__lexpr,        // TYP_LEXPR
    safe_del__handle,       // TYP_HANDLE
    safe_del__ldorig,       // TYP_LDORIG
};

void(*siVariable::gc_argv_tab[NUM_TYPES])(Variable*, Variable*, PNodeType) =
{
    gc_argv__string,        // TYP_NOTYPE
    gc_argv__string,        // TYP_STRING
    0,                      // TYP_SCALAR
    0,                      // TYP_ARRAY
    0,                      // TYP_CMPLX
    gc_argv__zlist,         // TYP_ZLIST
    0,                      // TYP_LEXPR
    gc_argv__handle,        // TYP_HANDLE
    0                       // TYP_LDORIG
};

void(*siVariable::gc_result_tab[NUM_TYPES])(Variable*) =
{
    gc_result__string,      // TYP_NOTYPE
    gc_result__string,      // TYP_STRING
    0,                      // TYP_SCALAR
    0,                      // TYP_ARRAY
    0,                      // TYP_CMPLX
    gc_result__zlist,       // TYP_ZLIST
    gc_result__lexpr,       // TYP_LEXPR
    0,                      // TYP_HANDLE
    0                       // TYP_LDORIG
};

bool(*siVariable::assign_tab[NUM_TYPES])(siVariable*, ParseNode*, Variable*,
    void*) =
{
    assign__string,         // TYP_NOTYPE
    assign__string,         // TYP_STRING
    assign__scalar,         // TYP_SCALAR
    assign__array,          // TYP_ARRAY
    assign__cmplx,          // TYP_CMPLX
    assign__zlist,          // TYP_ZLIST
    assign__lexpr,          // TYP_LEXPR
    assign__handle,         // TYP_HANDLE
    0                       // TYP_LDORIG
};

void(*siVariable::assign_fix_tab[NUM_TYPES])(Variable*, Variable*) =
{
    assign_fix__string,     // TYP_NOTYPE
    assign_fix__string,     // TYP_STRING
    0,                      // TYP_SCALAR
    assign_fix__array,      // TYP_ARRAY
    0,                      // TYP_CMPLX
    assign_fix__zlist,      // TYP_ZLIST
    0,                      // TYP_LEXPR
    0,                      // TYP_HANDLE
    0                       // TYP_LDORIG
};

bool(*siVariable::get_var_tab[NUM_TYPES])(Variable*, ParseNode*, Variable*) =
{
    get_var__string,        // TYP_NOTYPE
    get_var__string,        // TYP_STRING
    get_var__scalar,        // TYP_SCALAR
    get_var__array,         // TYP_ARRAY
    get_var__cmplx,         // TYP_CMPLX
    get_var__zlist,         // TYP_ZLIST
    get_var__lexpr,         // TYP_LEXPR
    get_var__handle,        // TYP_HANDLE
    0                       // TYP_LDORIG
};

bool(*siVariable::bop_look_ahead_tab[NUM_TYPES])(Variable*, ParseNode*,
    Variable*) =
{
    0,                      // TYP_NOTYPE
    0,                      // TYP_STRING
    bop_look_ahead__scalar, // TYP_SCALAR
    0,                      // TYP_ARRAY
    bop_look_ahead__cmplx,  // TYP_CMPLX
    bop_look_ahead__zlist,  // TYP_ZLIST
    0,                      // TYP_LEXPR
    0,                      // TYP_HANDLE
    0                       // TYP_LDORIG
};

void(*siVariable::print_var_tab[NUM_TYPES])(Variable*, rvals*, char*) =
{
    print_var__string,      // TYP_NOTYPE
    print_var__string,      // TYP_STRING
    print_var__scalar,      // TYP_SCALAR
    print_var__array,       // TYP_ARRAY
    print_var__cmplx,       // TYP_CMPLX
    0,                      // TYP_ZLIST
    0,                      // TYP_LEXPR
    print_var__handle,      // TYP_HANDLE
    print_var__ldorig       // TYP_LDORIG
};

char*(*siVariable::set_var_tab[NUM_TYPES])(Variable*, rvals*, const char*) =
{
    set_var__string,        // TYP_NOTYPE
    set_var__string,        // TYP_STRING
    set_var__scalar,        // TYP_SCALAR
    set_var__array,         // TYP_ARRAY
    set_var__cmplx,         // TYP_CMPLX
    0,                      // TYP_ZLIST
    0,                      // TYP_LEXPR
    set_var__handle,        // TYP_HANDLE
    0                       // TYP_LDORIG
};

int(*siVariable::respond_tab[NUM_TYPES])(Variable*, int, bool) =
{
    respond__notype,        // TYP_NOTYPE
    respond__string,        // TYP_STRING
    respond__scalar,        // TYP_SCALAR
    respond__array,         // TYP_ARRAY
    respond__cmplx,         // TYP_CMPLX
    respond__zlist,         // TYP_ZLIST
    respond__lexpr,         // TYP_LEXPR
    respond__handle,        // TYP_HANDLE
    0                       // TYP_LDORIG
};


siVariable::siVariable(char *n, int *dims)
{
    name = (n ? n : lstring::copy(""));
    type = TYP_NOTYPE;
    flags = 0;
    next = 0;
    if (dims && dims[0] > 0) {
        type = TYP_ARRAY;
        content.a = new siAryData;
        if (ADATA(content.a)->init(dims) == OK)
            flags |= VF_ORIGINAL;
    }
    else {
        if (*name == '"') {
            type = TYP_STRING;
            content.string = new char[strlen(name)];
            strcpy(content.string, name + 1);
            if (content.string[strlen(content.string) - 1] == '"')
                content.string[strlen(content.string) - 1] = 0;
            flags |= VF_ORIGINAL;
        }
        else
            content.string = name;
    }
}
// End of siVariable functions.


// Initialize the array specification.
//
bool
siAryData::init(int *d)
{
    memcpy(ad_dims, d, MAXDIMS*sizeof(int));
    int size = length() + 1;
    allocate(size);
    if (!ad_values) {
        SIparse()->pushError("-eout of memory, array allocation failed");
        return (BAD);
    }
    return (OK);
}


// Fill in d[MAXDIMS] with the dimensions passed.  Expand the array if
// necessary and possible.  If nd, return the dimensionality of the
// index.  If vnd, return the dimensionality of the vector.  Expand
// the array to handle icnt new entries.
//
bool
siAryData::dimensions(ParseNode *pl, int *d, int *nd, int *vnd, int icnt)
{
    if (!ad_values)
        return (BAD);
    int ndims = 1;
    for ( ; ndims < MAXDIMS && ad_dims[ndims]; ndims++) ;
    if (vnd)
        *vnd = ndims;
    memset(d, 0, MAXDIMS*sizeof(int));
    {
        int i = 0;
        while (pl) {
            if (i == ndims) {
                SIparse()->pushError(
                    "-etoo many dimensions in array subscript");
                return (BAD);
            }
            siVariable r1;
            SIlexprCx cx;
            bool err = (*pl->evfunc)(pl, &r1, &cx);
            if (err)
                return (err);
            if (r1.type != TYP_SCALAR) {
                SIparse()->pushError(
                    "-enon-scalar array subscript in assignment");
                return (BAD);
            }
            d[i] = (int)r1.content.value;
            if (d[i] < 0) {
                SIparse()->pushError("-enegative array subscript");
                return (BAD);
            }
            pl = pl->next;
            i++;
        }
        if (nd)
            *nd = i;
    }

    int xd[MAXDIMS];
    memcpy(xd, d, MAXDIMS*sizeof(int));

    bool needs_resize = false;
    if (icnt > 1) {

        int offs = xd[0];
        int sz = ad_dims[0];
        for (int i = 1; i < ndims; i++) {
            if (xd[i])
                offs += xd[i]*sz;
            sz *= ad_dims[i];
        }
        offs += icnt-1;

        if (offs >= sz) {
            int tsz = sz/ad_dims[ndims-1];
            xd[ndims-1] = ad_dims[ndims-1] - 1;
            while (offs  >= sz) {
                xd[ndims-1]++;
                sz += tsz;
            }
            needs_resize = true;
        }
    }
    if (!needs_resize) {
        for (int i = 0; i < ndims; i++) {
            if (xd[i] >= ad_dims[i]) {
                needs_resize = true;
                break;
            }
        }
    }
    if (needs_resize) {
        if (ad_refptr || ad_refcnt) {
            SIparse()->pushError(
                "-earray subscript out of range in assignment, can't resize");
            return (BAD);
        }
        if (expand(xd) == BAD)
            return (BAD);
    }
    return (OK);
}


// Return OK if the subscripting in p is within the current array bounds.
//
bool
siAryData::check(ParseNode *p)
{
    int d[MAXDIMS], nd, ndims;
    if (dimensions(p, d, &nd, &ndims, 0) == BAD)
        return (BAD);

    if (nd < ndims) {
        // sub-array return
        int k = ndims - nd;
        for (int j = 0; k < ndims; k++, j++) {
            if (d[j] && (d[j] < 0 || d[j] >= ad_dims[k]))
                return (BAD);
        }
    }
    else {
        if (d[0] < 0 || d[0] >= ad_dims[0])
            return (BAD);
        for (int i = 1; i < ndims; i++) {
            if (d[i] && (d[i] < 0 || d[i] >= ad_dims[i]))
                return (BAD);
        }
    }
    return (OK);
}


// Set the result to the accessed component of the array.  If the number
// of dimensions supplied is less than the number of dimensions of the
// array, the return is a sub-array.  For example, given array[2,3,4],
// then [1] is &array[0,0,1], [1,2] is &array[0,1,2].
//
bool
siAryData::resolve(Variable *res, ParseNode *p)
{
    int d[MAXDIMS], nd, ndims;
    if (dimensions(p, d, &nd, &ndims, 0) == BAD)
        return (BAD);

    if (nd < ndims) {
        // return sub-array
        siAryData *a = new siAryData;
        int sz = 1;
        int k;
        for (k = 0; k < ndims - nd; k++) {
            a->ad_dims[k] = ad_dims[k];
            sz *= ad_dims[k];
        }
        int os = 0;
        for (int j = 0; k < ndims; k++, j++) {
            if (d[j]) {
                if (d[j] < 0 || d[j] >= ad_dims[k]) {
                    SIparse()->pushError("-earray subscript out of range");
                    delete a;
                    return (BAD);
                }
                os += d[j]*sz;
            }
            sz *= ad_dims[k];
        }
        a->ad_values = ad_values + os;

        // reference original data
        siAryData *rp = this;
        while (rp->ad_refptr)
            rp = (siAryData*)rp->ad_refptr;
        a->ad_refptr = rp;
        rp->ad_refcnt++;

        res->type = TYP_ARRAY;
        res->content.a = a;
    }
    else {
        int offs = d[0];
        int sz = ad_dims[0];
        if (offs < 0 || offs >= sz) {
            SIparse()->pushError("-earray subscript out of range");
            return (BAD);
        }
        for (int i = 1; i < ndims; i++) {
            if (d[i]) {
                if (d[i] < 0 || d[i] >= ad_dims[i]) {
                    SIparse()->pushError("-earray subscript out of range");
                    return (BAD);
                }
                offs += d[i]*sz;
            }
            sz *= ad_dims[i];
        }
        res->type = TYP_SCALAR;
        res->content.value = ad_values[offs];
    }
    return (OK);
}


// Set the value in pr to the array element given in pl, resizing the
// array if necessary and possible.  Unspecified dimensions are taken
// as 0.
//
bool
siAryData::assign(Variable *res, ParseNode *pl, ParseNode *pr, void *datap)
{
    int icnt = 0;
    for (ParseNode *p = pr; p; p = p->next)
        icnt++;

    int d[MAXDIMS], ndims;
    if (dimensions(pl, d, 0, &ndims, icnt) == BAD)
        return (BAD);

    int offs = d[0];
    int sz = ad_dims[0];
    for (int i = 1; i < ndims; i++) {
        if (d[i])
            offs += d[i]*sz;
        sz *= ad_dims[i];
    }

    while (pr) {
        siVariable r2;
        bool err = (*pr->evfunc)(pr, &r2, datap);
        if (err != OK)
            return (err);
        if (r2.type != TYP_SCALAR && r2.type != TYP_HANDLE) {
            SIparse()->pushError("-enon-scalar array element assignment");
            return (BAD);
        }
        ad_values[offs] = r2.content.value;
        offs++;
        res->set_result(&r2);
        pr = pr->next;
    }
    return (OK);
}


// Handle array expansion, preserving original data.
//
bool
siAryData::expand(int *d)
{
    int nd[MAXDIMS];
    memset(nd, 0, sizeof(nd));
    int sz = 1;
    for (int i = 0; i < MAXDIMS && ad_dims[i]; i++) {
        int x = d[i] + 1;
        if (x < ad_dims[i])
            x = ad_dims[i];
        sz *= x;
        nd[i] = x;
    }
    double *newvals = (double*)calloc(sz, sizeof(double));
    if (!newvals) {
        SIparse()->pushError("-eout of memory, array resize failed");
        return (BAD);
    }
    int osz = length()/ad_dims[0];
    double *vo = ad_values;
    double *vn = newvals;
    for (int i = 0; i < osz; i++) {
        memcpy(vn, vo, ad_dims[0]*sizeof(double));
        vo += ad_dims[0];
        vn += nd[0];
    }
    memcpy(ad_dims, nd, sizeof(nd));
    delete [] ad_values;
    ad_values = newvals;
    return (OK);
}


// Resize the array if necessary and possible, to handle size elements.
//
bool
siAryData::resize(int size)
{
    const char *msg = "-eout of memory, array resize failed";
    if (size < length())
        return (OK);
    if (ad_refptr || ad_refcnt)
        return (BAD);
    if (!ad_dims[1]) {
        // simple linear array
        allocate(size + 1);
        if (!ad_values) {
            SIparse()->pushError(msg);
            return (BAD);
        }
        ad_dims[0] = size;
        return (OK);
    }
    int ndims = 1;
    for ( ; ndims < MAXDIMS && ad_dims[ndims]; ndims++) ;
    ndims--;
    int sz = ad_dims[0];
    for (int i = 1; i < ndims; i++)
        sz *= ad_dims[i];
    while (sz*ad_dims[ndims] < size)
        ad_dims[ndims]++;
    allocate(sz*ad_dims[ndims] + 1);
    if (!ad_values) {
        SIparse()->pushError(msg);
        return (BAD);
    }
    return (OK);
}


// Free the array if possible and reset to size 1.
//
bool
siAryData::reset()
{
    if (ad_refptr || ad_refcnt)
        return (BAD);
    allocate(1);
    if (!ad_values) {
        SIparse()->pushError("-eout of memory, array resize failed");
        return (BAD);
    }
    memset(ad_dims, 0, MAXDIMS*sizeof(int));
    ad_dims[0] = 1;
    return (OK);
}


// Create a pointer to a sub-array or an offset into the data.  If out
// of range, return BAD.
//
bool
siAryData::mkpointer(Variable *res, int indx)
{
    int ndims = 1;
    for ( ; ndims < MAXDIMS && ad_dims[ndims]; ndims++) ;
    ndims--;
    if (indx >= ad_dims[ndims])
        return (BAD);
    siAryData *ad = new siAryData;
    res->content.a = ad;

    // reference original data
    siAryData *rp = this;
    while (rp->ad_refptr)
        rp = (siAryData*)rp->ad_refptr;
    ad->ad_refptr = rp;
    rp->ad_refcnt++;

    if (!ndims) {
        if (indx < 0) {
            int nn = ad_values - rp->ad_values;
            if (nn + indx < 0)
                return (BAD);
        }
        ad->ad_dims[0] = ad_dims[0] - indx;
        ad->ad_values = ad_values + indx;
    }
    else {
        int sz = 1;
        for (int i = 0; i < ndims; i++) {
            ad->ad_dims[i] = ad_dims[i];
            sz *= ad_dims[i];
        }
        if (indx < 0) {
            int nn = ad_values - rp->ad_values;
            if (nn + indx*sz < 0)
                return (BAD);
        }
        ad->ad_values = ad_values + sz*indx;
    }
    res->type = TYP_ARRAY;
    return (OK);
}

