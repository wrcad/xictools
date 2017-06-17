
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
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
 $Id: cd_memmgr.cc,v 5.18 2016/02/19 02:09:07 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "cd.h"
#include "cd_types.h"
#include "cd_hypertext.h"
#include "cd_memmgr.h"
#include "cd_memmgr_cfg.h"


// Print function from templates, breakpoint here for debugging.
//
void
mm_err_hook(const char *s, const char *type_name)
{
    fprintf(stderr, "%s: %s\n", type_name ? type_name : "MMGR", s);
}


//-----------------------------------------------------------------------------
// cCDmmgr:
// Memory management of CDo and derived, CDs, CDm, CDol, CDcl, and SymTabEnt.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Setup for RTelem subclassing.

namespace {
    void destroy_o(RTelem *e)   { ((CDo*)e)->destroy(); }
    void destroy_po(RTelem *e)  { ((CDpo*)e)->destroy(); }
    void destroy_w(RTelem *e)   { ((CDw*)e)->destroy(); }
    void destroy_la(RTelem *e)  { ((CDla*)e)->destroy(); }
    void destroy_c(RTelem *e)   { ((CDo*)e)->destroy(); }

#ifdef CD_USE_MANAGER
    void delete_o(RTelem *e)    { CDmmgr()->CDo_db.delElem(e); }
    void delete_po(RTelem *e)   { CDmmgr()->CDpo_db.delElem(e); }
    void delete_w(RTelem *e)    { CDmmgr()->CDw_db.delElem(e); }
    void delete_la(RTelem *e)   { CDmmgr()->CDla_db.delElem(e); }
    void delete_c(RTelem *e)    { CDmmgr()->CDc_db.delElem(e); }
#endif

    void registerObjTypes()
    {
#ifdef CD_USE_MANAGER
        RTelem::registerType(CDBOX, destroy_o, delete_o);
        RTelem::registerType(CDPOLYGON, destroy_po, delete_po);
        RTelem::registerType(CDWIRE, destroy_w, delete_w);
        RTelem::registerType(CDLABEL, destroy_la, delete_la);
        RTelem::registerType(CDINSTANCE, destroy_c, delete_c);
#else
        RTelem::registerType(CDBOX, destroy_o, 0);
        RTelem::registerType(CDPOLYGON, destroy_po, 0);
        RTelem::registerType(CDWIRE, destroy_w, 0);
        RTelem::registerType(CDLABEL, destroy_la, 0);
        RTelem::registerType(CDINSTANCE, destroy_c, 0);
#endif
    }
}


//-----------------------------------------------------------------------------
// sCDmmgr functions.

cCDmmgr *cCDmmgr::instancePtr = 0;

cCDmmgr::cCDmmgr() :
    CDo_db("CDo"), CDpo_db("CDpo"), CDw_db("CDw"), CDla_db("CDla"), 
    CDc_db("CDc"), CDs_db("CDs"), CDm_db("CDm"), CDol_db("CDol"),
    CDcl_db("CDcl"), SymTabEnt_db("SymTabEnt")
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cCDmmgr already instantiated.\n");
        exit (1);
    }
    instancePtr = this;

    registerObjTypes();
}


// Function to print allocation statistics.
//
void
cCDmmgr::stats(int *inuse, int *not_inuse)
{
#ifdef HAS_ATOMIC_STACK
    printf("Atomic free list: yes\n");
#else
    printf("Atomic free list: no\n");
#endif

    mmgrstat_t st;
    CDo_db.stats(&st);
    st.print("CDo", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    CDpo_db.stats(&st);
    st.print("CDpo", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    CDw_db.stats(&st);
    st.print("CDw", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    CDla_db.stats(&st);
    st.print("CDla", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    CDc_db.stats(&st);
    st.print("CDc", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    CDs_db.stats(&st);
    st.print("CDs", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    CDm_db.stats(&st);
    st.print("CDm", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    CDol_db.stats(&st);
    st.print("CDol", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    CDcl_db.stats(&st);
    st.print("CDcl", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    SymTabEnt_db.stats(&st);
    st.print("SymTabEnt", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;
}


//-----------------------------------------------------------------------------
// Allocation operators

#ifdef CD_USE_MANAGER

void *
CDo::operator new(size_t)
    { return (CDmmgr()->CDo_db.newElem()); }


void *
CDpo::operator new(size_t)
    { return (CDmmgr()->CDpo_db.newElem()); }


void *
CDw::operator new(size_t)
    { return (CDmmgr()->CDw_db.newElem()); }


void *
CDla::operator new(size_t)
    { return (CDmmgr()->CDla_db.newElem()); }


void *
CDc::operator new(size_t)
    { return (CDmmgr()->CDc_db.newElem()); }


//---------------------------------------
// CDdb (really CDs)

void *
CDdb::operator new(size_t)
    { return (CDmmgr()->CDs_db.newElem()); }

void
CDdb::operator delete(void *p, size_t)
    { if (p) CDmmgr()->CDs_db.delElem(p); }


//---------------------------------------
// CDm

void *
CDm::operator new(size_t)
    { return (CDmmgr()->CDm_db.newElem()); }

void
CDm::operator delete(void *p, size_t)
    { if (p) CDmmgr()->CDm_db.delElem(p); }


//---------------------------------------
// CDol

void *
CDol::operator new(size_t)
    { return (CDmmgr()->new_CDol()); }

void
CDol::operator delete(void *p, size_t)
    { if (p) CDmmgr()->del_CDol((CDol*)p); }


//---------------------------------------
// CDcl

void *
CDcl::operator new(size_t)
    { return (CDmmgr()->new_CDcl()); }

void
CDcl::operator delete(void *p, size_t)
    { if (p) CDmmgr()->del_CDcl((CDcl*)p); }


//---------------------------------------
// SymTabEnt

void *
SymTabEnt::operator new(size_t)
    { return (CDmmgr()->SymTabEnt_db.newElem()); }

void
SymTabEnt::operator delete(void *p, size_t)
    { if (p) CDmmgr()->SymTabEnt_db.delElem(p); }

#endif

