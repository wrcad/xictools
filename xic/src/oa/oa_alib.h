
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

#ifndef OA_ALIB_H
#define OA_ALIB_H


// Class for converting the standard (analogLib and basic libraries)
// devices into a format that will work in WRspice.  First, these call
// names will be mapper into new names to avoid conflict with the Xic
// device library names.  Second, we build a value property string
// from the parameters list.  The value string provides the parameters
// in the proper order and syntax for WRspice.

// Static size for Alib hash table.
#define AL_HMASK 63

class cAlibFixup
{
public:
    typedef bool(*AlibFunc)(sLstr&);

    struct alib_elt
    {
        alib_elt(const char *n, AlibFunc f)
            {
                next = 0;
                name = n;
                func = f;
            }

        alib_elt *next;
        const char *name;
        AlibFunc func;
    };

    cAlibFixup()
        {
            for (int i = 0; i <= AL_HMASK; i++)
                al_array[i] = 0;
            al_linked = false;
        }

    // Return true if cname is a known device.
    bool is_alib_device(const char *cname)
        {
            return (alib_find(cname));
        }

    // Fix up the property strings of known devices, return true if
    // device found and fixed.
    bool prpty_fix(const char *cname, sLstr &lstr)
        {
            alib_elt *e = alib_find(cname);
            if (e)
                return ((*e->func)(lstr));
            return (false);
        }

private:
    alib_elt *alib_find (const char*);

    // These are the devices, most from analogLib.  The gpulse devices
    // are from Synopsys (imported from WRspice for SuperTools).
    static void dcac(const PCellParam*, sLstr&);
    static bool src_params(sLstr&, const char*, const char**);
    static bool alib_vdc(sLstr&);
    static bool alib_iprobe(sLstr&);
    static bool alib_vexp(sLstr&);
    static bool alib_vgpulse(sLstr&);
    static bool alib_vpulse(sLstr&);
    static bool alib_vpwl(sLstr&);
    static bool alib_vsffm(sLstr&);
    static bool alib_vsin(sLstr&);
    static bool alib_idc(sLstr&);
    static bool alib_iexp(sLstr&);
    static bool alib_igpulse(sLstr&);
    static bool alib_ipulse(sLstr&);
    static bool alib_ipwl(sLstr&);
    static bool alib_isffm(sLstr&);
    static bool alib_isin(sLstr&);
    static bool alib_cccs(sLstr&);
    static bool alib_ccvs(sLstr&);
    static bool alib_vccs(sLstr&);
    static bool alib_vcvs(sLstr&);
    static bool alib_cap(sLstr&);
    static bool alib_ind(sLstr&);
    static bool alib_mind(sLstr&);
    static bool alib_res(sLstr&);
    static bool alib_jj(sLstr&);
    static bool alib_tline(sLstr&);
    static bool alib_diode(sLstr&);
    static bool alib_npn(sLstr&);
    static bool alib_pnp(sLstr&);
    static bool alib_nbsim(sLstr&);
    static bool alib_nbsim4(sLstr&);
    static bool alib_nmos(sLstr&);
    static bool alib_nmos4(sLstr&);
    static bool alib_pbsim(sLstr&);
    static bool alib_pbsim4(sLstr&);
    static bool alib_pmos(sLstr&);
    static bool alib_pmos4(sLstr&);
    static bool alib_njfet(sLstr&);
    static bool alib_pjfet(sLstr&);
    static bool alib_nmes(sLstr&);
    static bool alib_pmes(sLstr&);

    // Terminals from the basic library.
/*XXX
    static bool alib_gnd(sLstr&);
    static bool alib_gnd_inherited(sLstr&);
    static bool alib_vdd(sLstr&);
    static bool alib_vdd_inherited(sLstr&);
*/

    alib_elt *al_array[AL_HMASK + 1];
    bool al_linked;

    static alib_elt al_list[];
};

extern cAlibFixup AlibFixup;

#endif

