
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id: gr_pkg_setup.h,v 2.6 2010/02/15 04:02:39 stevew Exp $
 *========================================================================*/

#ifndef GR_PKG_SETUP_H
#define GR_PKG_SETUP_H

// This must included in the application once, it provides the package
// initialization function.  The variable GR_CONFIG must be defined to
// an ORed list of the tokens listed at the top of graphics.h, before
// this file is included:  For example,
//
//     #include "graphics.h"
//      . . .
//     #define GR_CONFIG  (GR_ALL_PKGS | GR_ALL_DRIVERS)
//     #include "gr_pkg_setup.h"
//

#include "nulldev.h"
#if (GR_CONFIG & _devPS_)
#include "pslindrw.h"
#endif
#if (GR_CONFIG & _devPSBM_)
#include "psbm.h"
#endif
#if (GR_CONFIG & _devPSBC_)
#include "psbc.h"
#endif
#if (GR_CONFIG & _devHP_)
#include "hpgl.h"
#endif
#if (GR_CONFIG & _devPCL_)
#include "pcl.h"
#endif
#if (GR_CONFIG & _devXF_)
#include "xfig.h"
#endif
#if (GR_CONFIG & _devX_)
#include "xdraw.h"
#endif

// Initialize the package list and the screen package.  Return false
// on success, true if error.  The first argument represents the
// initial driver set, which should be a subset of thise given in
// GR_CONFIG.
//
bool
GRpkg::InitPkg(unsigned int cfg, int *argc, char **argv)
{
#if ((GR_CONFIG & GR_ALL_PKGS) == _devNULL_)
    RegisterDevice(new NULLdev);
#else
    if ((cfg & GR_ALL_PKGS) != _devNULL_)
        DevDepInit(GR_CONFIG);
    else
        RegisterDevice(new NULLdev);
#endif
#if (GR_CONFIG & _devPS_)
    if (cfg & _devPS_) {
        RegisterDevice(new PSdev);
        RegisterHcopyDesc(&PSdesc_m);
        RegisterHcopyDesc(&PSdesc_c);
    }
#endif
#if (GR_CONFIG & _devPSBM_)
    if (cfg & _devPSBM_) {
        RegisterDevice(new PSBMdev);
        RegisterHcopyDesc(&PSBMdesc);
        RegisterHcopyDesc(&PSBMdesc_e);
    }
#endif
#if (GR_CONFIG & _devPSBC_)
    if (cfg & _devPSBC_) {
        RegisterDevice(new PSBCdev);
        RegisterHcopyDesc(&PSBCdesc);
        RegisterHcopyDesc(&PSBCdesc_e);
    }
#endif
#if (GR_CONFIG & _devHP_)
    if (cfg & _devHP_) {
        RegisterDevice(new HPdev);
        RegisterHcopyDesc(&HPdesc);
    }
#endif
#if (GR_CONFIG & _devPCL_)
    if (cfg & _devPCL_) {
        RegisterDevice(new PCLdev);
        RegisterHcopyDesc(&PCLdesc);
    }
#endif
#if (GR_CONFIG & _devXF_)
    if (cfg & _devXF_) {
        RegisterDevice(new XFdev);
        RegisterHcopyDesc(&XFdesc);
    }
#endif
#if (GR_CONFIG & _devX_)
    if (cfg & _devX_) {
        RegisterDevice(new Xdev);
        RegisterHcopyDesc(&Xdesc);
    }
#endif

    // pkg_device[0] is always the screen device
    if (pkg_devices[0]->Init(argc, argv))
        return (true);
    pkg_cur_dev = pkg_devices[0];
    pkg_main_dev = dynamic_cast<GRscreenDev*>(pkg_cur_dev);
    return (false);
}

#endif

