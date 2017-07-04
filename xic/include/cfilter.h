
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
 $Id: cfilter.h,v 1.3 2015/03/21 19:50:24 stevew Exp $
 *========================================================================*/

#ifndef CFILTER_H
#define CFILTER_H


//
// cfilter_t:  A struct to filter cell listings.
//

#define CF_VIASUBM      0x1
#define CF_NOTVIASUBM   0x2
#define CF_PCELLSUBM    0x4
#define CF_NOTPCELLSUBM 0x8
#define CF_PCELLSUPR    0x10
#define CF_NOTPCELLSUPR 0x20
#define CF_IMMUTABLE    0x40
#define CF_NOTIMMUTABLE 0x80
#define CF_DEVICE       0x100
#define CF_NOTDEVICE    0x200
#define CF_LIBRARY      0x400
#define CF_NOTLIBRARY   0x800
#define CF_MODIFIED     0x1000
#define CF_NOTMODIFIED  0x2000
#define CF_REFERENCE    0x4000
#define CF_NOTREFERENCE 0x8000
#define CF_TOPLEV       0x10000
#define CF_NOTTOPLEV    0x20000
#define CF_WITHALT      0x40000
#define CF_NOTWITHALT   0x80000
#define CF_PARENT       0x100000
#define CF_NOTPARENT    0x200000
#define CF_SUBCELL      0x400000
#define CF_NOTSUBCELL   0x800000
#define CF_LAYER        0x1000000
#define CF_NOTLAYER     0x2000000
#define CF_FLAG         0x4000000
#define CF_NOTFLAG      0x8000000
#define CF_FTYPE        0x10000000
#define CF_NOTFTYPE     0x20000000

struct CDll;
struct stringlist;

struct cfilter_t
{
    struct cnlist_t
    {
        cnlist_t(CDcellName cn, cnlist_t *nx)
            {
                next = nx;
                cname = cn;
            }

        static void destroy(const cnlist_t *l)
            {
                while (l) {
                    const cnlist_t *x = l;
                    l = l->next;
                    delete x;
                }
            }

        cnlist_t *next;
        CDcellName cname;
    };

    cfilter_t(DisplayMode);
    ~cfilter_t();

    void set_default();
    bool inlist(const CDcbin*) const;

    char *prnt_list() const;
    char *not_prnt_list() const;
    char *subc_list() const;
    char *not_subc_list() const;
    char *layer_list() const;
    char *not_layer_list() const;
    char *flag_list() const;
    char *not_flag_list() const;
    char *ftype_list() const;
    char *not_ftype_list() const;
    char *string() const;

    void parse_parent(bool, const char*, stringlist**);
    void parse_subcell(bool, const char*, stringlist**);
    void parse_layers(bool, const char*, stringlist**);
    void parse_flags(bool, const char*, stringlist**);
    void parse_ftypes(bool, const char*, stringlist**);

    static cfilter_t *parse(const char*, DisplayMode, stringlist**);

    DisplayMode mode()                  const { return (cf_mode); }
    unsigned int flags()                const { return (cf_flags); }
    void set_flags(unsigned int f)      { cf_flags = f; check_flags(); }

private:
    void check_flags();

    cnlist_t *cf_p_list;
    cnlist_t *cf_np_list;
    cnlist_t *cf_s_list;
    cnlist_t *cf_ns_list;
    CDll *cf_l_list;
    CDll *cf_nl_list;
    unsigned int cf_flags;
    unsigned int cf_f_bits;
    unsigned int cf_nf_bits;
    unsigned int cf_ft_mask;
    unsigned int cf_nft_mask;
    DisplayMode cf_mode;
};

#endif

