
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
 $Id: ext_techif.cc,v 5.5 2017/03/19 01:58:39 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "tech.h"
#include "tech_extract.h"


//-----------------------------------------------------------------------------
// Techfile package configuration for extraction.
//

namespace {
    TCret device_parse(FILE *techfp)
    {
        if (Tech()->Matching(Ekw.DeviceTemplate())) {
            const char *t = Tech()->InBuf();
            char *tok = lstring::gettok(&t);
            if (EX()->parseDeviceTemplate(techfp, tok)) {
                if (!EX()->techDevTemplates())
                    EX()->setTechDevTemplates(new stringlist(tok, 0));
                else {
                    stringlist *sl = EX()->techDevTemplates();
                    while (sl->next)
                        sl = sl->next;
                    sl->next = new stringlist(tok, 0);
                }
            }
            return (TCmatch);
        }

        // Device block definition
        //
        if (Tech()->Matching(Ekw.Device())) {
            const char *t = Tech()->InBuf();
            char *tok = lstring::gettok(&t);
            if (tok) {
                // Device specification for a layer.
                //
                char *e = Tech()->SaveError(
                    "%s: keyword in layer block not supported, ignored.",
                    Ekw.Device());
                delete [] tok;
                return (e);
            }
            EX()->parseDevice(techfp, false);
            return (TCmatch);
        }
        return (TCnone);
    }


    void print_devices(FILE *techfp)
    {
        EX()->techPrintDeviceTemplates(techfp, true);
        EX()->techPrintDevices(techfp);
    }
}


void
cExt::setupTech()
{
    Tech()->RegisterDeviceParser(&device_parse);
    Tech()->RegisterPrintDevices(&print_devices);
}

