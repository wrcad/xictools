
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2013 Whiteley Research Inc, all rights reserved.        *
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
 $Id: ext_duality.h,v 5.4 2015/06/11 05:54:06 stevew Exp $
 *========================================================================*/

#ifndef EXT_DUALITY_H
#define EXT_DUALITY_H


//========================================================================
//
// Structs to record associations after an imposed symmetry breaking. 
// This allows the actions to be undone, another of the symmetry
// choices applied, and the associations redone.  The structures are
// linked, allowing the context to be nested.
//
//========================================================================

namespace ext_duality {

    // List element for group/node associations.
    //
    struct sSymGrp
    {
        sSymGrp(int g, int n, sSymGrp *nx = 0)
            {
                sg_group = g;
                sg_node = n;
                sg_next = nx;
            }

        static void destroy(const sSymGrp *s)
            {
                while (s) {
                    const sSymGrp *x = s;
                    s = s->next();
                    delete x;
                }
            }

        int group()             const { return (sg_group); }
        int node()              const { return (sg_node); }
        sSymGrp *next()         const { return (sg_next); }
        void set_next(sSymGrp *n)     { sg_next = n; }

    private:
        int                     sg_group;
        int                     sg_node;
        sSymGrp                 *sg_next;
    };

    // List element for device associations.
    //
    struct sSymDev
    {
        sSymDev(sDevInst *d, sEinstList *e, sSymDev *nx = 0)
            {
                sd_pdev = d;
                sd_edev = e;
                sd_next = nx;
            }

        static void destroy(const sSymDev *s)
            {
                while (s) {
                    const sSymDev *x = s;
                    s = s->next();
                    delete x;
                }
            }

        sDevInst *phys_dev()    const { return (sd_pdev); }
        sEinstList *elec_dev()  const { return (sd_edev); }
        sSymDev *next()         const { return (sd_next); }
        void set_next(sSymDev *n)     { sd_next = n; }

    private:
        sDevInst                *sd_pdev;
        sEinstList              *sd_edev;
        sSymDev                 *sd_next;
    };

    // List element for subcircuit associations.
    //
    struct sSymSubc
    {
        sSymSubc(sSubcInst *s, sEinstList *e, sSymSubc *nx = 0)
            {
                ss_psub = s;
                ss_esub = e;
                ss_next = nx;
            }

        static void destroy(const sSymSubc *s)
            {
                while (s) {
                    const sSymSubc *x = s;
                    s = s->next();
                    delete x;
                }
            }

        sSubcInst *phys_subc()  const { return (ss_psub); }
        sEinstList *elec_subc() const { return (ss_esub); }
        sSymSubc *next()        const { return (ss_next); }
        void set_next(sSymSubc *n)    { ss_next = n; }

    private:
        sSubcInst               *ss_psub;
        sEinstList              *ss_esub;
        sSymSubc                *ss_next;
    };

    // List element for sEinstList objects.
    //
    struct sSymCll
    {
        sSymCll(sEinstList *c, sSymCll *nx = 0)
            {
                sc_clist = c;
                sc_next = nx;
            }

        static void destroy(const sSymCll *s)
            {
                while (s) {
                    const sSymCll *x = s;
                    s = s->next();
                    delete x;
                }
            }

        sEinstList *inst_elem() const { return (sc_clist); }
        sSymCll *next()         const { return (sc_next); }
        void set_next(sSymCll *n)     { sc_next = n; }

    private:
        sEinstList              *sc_clist;
        sSymCll                 *sc_next;
    };

    // The main context scruct.  This is created when symmetry
    // breaking is imposed.
    //
    struct sSymBrk
    {
        sSymBrk(sDevInst *di, sSymCll *el, sSymBrk *n)
            {
                sb_dev = di;
                sb_subc = 0;
                sb_elist = el;
                sb_grp_assoc = 0;
                sb_dev_assoc = 0;
                sb_subc_assoc = 0;
                sb_next = n;
            }

        sSymBrk(sSubcInst *s, sSymCll *el, sSymBrk *n)
            {
                sb_dev = 0;
                sb_subc = s;
                sb_elist = el;
                sb_grp_assoc = 0;
                sb_dev_assoc = 0;
                sb_subc_assoc = 0;
                sb_next = n;
            }

        sSymBrk(const sSymBrk&);

        ~sSymBrk()
            {
                sSymCll::destroy(sb_elist);
                sSymGrp::destroy(sb_grp_assoc);
                sSymDev::destroy(sb_dev_assoc);
                sSymSubc::destroy(sb_subc_assoc);
            }

        static void destroy(const sSymBrk *s)
            {
                while (s) {
                    const sSymBrk *x = s;
                    s = s->next();
                    delete x;
                }
            }

        sSymBrk *dup() const;

        void new_grp_assoc(int g, int n)
            {
                sSymBrk *sbt = this;
                if (!sbt)
                    return;
                sb_grp_assoc = new sSymGrp(g, n, sb_grp_assoc);
            }

        void new_dev_assoc(sDevInst *di, sEinstList *e)
            {
                sSymBrk *sbt = this;
                if (!sbt)
                    return;
                sb_dev_assoc = new sSymDev(di, e, sb_dev_assoc);
            }

        void new_subc_assoc(sSubcInst *s, sEinstList *e)
            {
                sSymBrk *sbt = this;
                if (!sbt)
                    return;
                sb_subc_assoc = new sSymSubc(s, e, sb_subc_assoc);
            }

        sDevInst *device()          const { return (sb_dev); }
        sSubcInst *subckt()         const { return (sb_subc); }
        sSymCll *elec_insts()       const { return (sb_elist); }
        void set_elec_insts(sSymCll *s)   { sb_elist = s; }
        sSymGrp *grp_assoc()        const { return (sb_grp_assoc); }
        void set_grp_assoc(sSymGrp *s)    { sb_grp_assoc = s; }
        sSymDev *dev_assoc()        const { return (sb_dev_assoc); }
        void set_dev_assoc(sSymDev *s)    { sb_dev_assoc = s; }
        sSymSubc *subc_assoc()      const { return (sb_subc_assoc); }
        void set_subc_assoc(sSymSubc *s)  { sb_subc_assoc = s; }
        sSymBrk *next()             const { return (sb_next); }
        void set_next(sSymBrk *n)         { sb_next = n; }

    private:
        sDevInst *sb_dev;           // device associated by symmetry breaking
        sSubcInst *sb_subc;         // subckt associated by symmetry breaking
        sSymCll *sb_elist;          // list of associations not made
        sSymGrp *sb_grp_assoc;      // groups associated since symmetry_break
        sSymDev *sb_dev_assoc;      // devs associated since symmetry break
        sSymSubc *sb_subc_assoc;    // subs associated since symmetry break
        sSymBrk *sb_next;
    };


    sSymBrk::sSymBrk(const sSymBrk &s)
    {
        sb_dev = s.sb_dev;
        sb_subc = s.sb_subc;

        sb_elist = 0;
        sSymCll *ce = 0;
        for (sSymCll *p = s.sb_elist; p; p = p->next()) {
            if (!sb_elist)
                sb_elist = ce = new sSymCll(p->inst_elem());
            else {
                ce->set_next(new sSymCll(p->inst_elem()));
                ce = ce->next();
            }
        }

        sb_grp_assoc = 0;
        sSymGrp *ge = 0;
        for (sSymGrp *p = s.sb_grp_assoc; p; p = p->next()) {
            if (!sb_grp_assoc)
                sb_grp_assoc = ge = new sSymGrp(p->group(), p->node());
            else {
                ge->set_next(new sSymGrp(p->group(), p->node()));
                ge = ge->next();
            }
        }

        sb_dev_assoc = 0;
        sSymDev *de = 0;
        for (sSymDev *p = s.sb_dev_assoc; p; p = p->next()) {
            if (!sb_dev_assoc)
                sb_dev_assoc = de = new sSymDev(p->phys_dev(), p->elec_dev());
            else {
                de->set_next(new sSymDev(p->phys_dev(), p->elec_dev()));
                de = de->next();
            }
        }

        sb_subc_assoc = 0;
        sSymSubc *se = 0;
        for (sSymSubc *p = s.sb_subc_assoc; p; p = p->next()) {
            if (!sb_subc_assoc)
                sb_subc_assoc = se =
                    new sSymSubc(p->phys_subc(), p->elec_subc());
            else {
                se->set_next(new sSymSubc(p->phys_subc(), p->elec_subc()));
                se = se->next();
            }
        }

        sb_next = 0;
    }


    sSymBrk *
    sSymBrk::dup() const
    {
        sSymBrk *s0 = 0, *se = 0;
        for (const sSymBrk *s = this; s; s = s->next()) {
            if (!s0)
                s0 = se = new sSymBrk(*s);
            else {
                se->set_next(new sSymBrk(*s));
                se = se->next();
            }
        }
        return (s0);
    }
}

using namespace ext_duality;

#endif

