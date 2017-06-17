
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
 $Id: ext_ep_comp.h,v 5.2 2013/11/24 02:34:12 stevew Exp $
 *========================================================================*/

#ifndef EXT_EP_COMP_H
#define EXT_EP_COMP_H


// This is a scale factor for comparison.
//
#define CMP_SCALE 1000


// This is used for device comparisons when finding duals.
//
struct sDevComp
{
    sDevComp()
        {
            dc_pdev = 0;
            dc_edev = 0;
            dc_conts = 0;
            dc_nodes = 0;
            dc_conts_sz = 0;
            dc_nodes_sz = 0;
            dc_pix1 = -1;
            dc_pix2 = -1;
            dc_gix = -1;
        }

    ~sDevComp()
        {
            delete [] dc_conts;
            delete [] dc_nodes;
        }

    sDevInst *pdev()        const { return (dc_pdev); }

    bool nogood()           const { return (!dc_conts_sz); }

    // ext_ep_comp.cc
    bool set(sDevInst*);
    bool set(sEinstList*);
    int score(cGroupDesc*);
    void associate(cGroupDesc*);
    bool is_parallel(const sEinstList*);
    bool is_mos_tpeq(cGroupDesc*, const sEinstList*);

private:
    sDevInst        *dc_pdev;       // Reference physical device.
    sEinstList      *dc_edev;       // Current electrical device.
    sDevContactInst **dc_conts;     // Physical device contacts.
    CDp_cnode       **dc_nodes;     // Electrical device node properties.
    unsigned int    dc_conts_sz;    // Contacts array size.
    unsigned int    dc_nodes_sz;    // Nodes array size.
    int             dc_pix1;        // Node property index of permutation 1.
    int             dc_pix2;        // Node property index of permutation 2.
    int             dc_gix;         // Node property index of MOS gate.
};

#endif

