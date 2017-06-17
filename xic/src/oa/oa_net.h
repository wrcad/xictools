
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
 $Id: oa_net.h,v 1.10 2016/02/15 05:35:55 stevew Exp $
 *========================================================================*/

#ifndef OA_NET_H
#define OA_NET_H


class cOAelecInfo;

// Table for port-order list.
//
struct sOAportTab
{
public:
    sOAportTab()
        {
            pt_tab = 0;
        }

    ~sOAportTab()
        {
            delete pt_tab;
        }

    bool port_setup(const oaBlock*, const cOAelecInfo*);
    bool find(const char*, unsigned int* = 0, unsigned int* = 0) const;

    static int def_order(const char*, int);

private:
    void add(const char*, unsigned int, unsigned int);

    SymTab *pt_tab;
};

// This sets up the electrical properties expected by Xic in
// electrical mode.
//
class cOAnetHandler
{
public:
    cOAnetHandler(const oaBlock *bl, CDs *sd, int sc, const char *symv,
            const char *prpv)
        {
            nh_block = bl;
            nh_sdesc = sd;
            nh_def_symbol = symv;       // default symbol view name
            nh_def_dev_prop = prpv;     // default simulator view name
            nh_scale = sc;
        }

    bool setupNets(bool);
    bool debugPrint();

private:
    bool find_pin_coords(const oaPin*, int, bool, int*, int*);
    bool implement_bit(const sOAportTab&, const oaTerm*, const oaString&,
        bool, int, int, int, int);
    int port_order(const cOAelecInfo*, int, const oaString&, int);
    bool check_vertex(const BBox*, int*, int*);
    void setup_wire_labels(const oaNet*);
    bool setup_wire_label(const oaText*);
    bool add_terminal(int, int, const char*);

    const oaBlock *nh_block;
    CDs *nh_sdesc;
    const char *nh_def_symbol;
    const char *nh_def_dev_prop;
    int nh_scale;
};

#endif

