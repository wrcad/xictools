
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

