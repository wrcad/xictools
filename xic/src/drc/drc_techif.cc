
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
 $Id: drc_techif.cc,v 5.4 2017/03/15 03:06:06 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "drc.h"
#include "drc_kwords.h"
#include "tech.h"


//-----------------------------------------------------------------------------
// Techfile package configuration for DRC.
//

namespace {
    TCret user_rule_parse(FILE *techfp)
    {
        if (Tech()->Matching(Dkw.DrcTest())) {
            DRC()->techParseUserRule(techfp);
            return (TCmatch);
        }
        return (TCnone);
    }

    TCret layer_parse(CDl *lastlayer)
    {
        TCret tcret = DRC()->techParseLayer(lastlayer);
        if (tcret != TCnone)
            return (tcret);

        return (TCnone);
    }


    void print_user_rules(FILE *techfp)
    {
        DRC()->techPrintUserRules(techfp);
    }

    void print_rules(FILE *techfp, sLstr *lstr, bool cmts, const CDl *ld)
    {
        DRC()->techPrintLayer(techfp, lstr, cmts, ld, false, true);
    }
}


void
cDRC::setupTech()
{
    Tech()->RegisterUserRuleParser(&user_rule_parse);
    Tech()->RegisterLayerParser(&layer_parse);
    Tech()->RegisterPrintUserRules(&print_user_rules);
    Tech()->RegisterPrintRules(&print_rules);
}

