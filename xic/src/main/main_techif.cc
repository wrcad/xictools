
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
 $Id: main_techif.cc,v 5.26 2017/03/16 05:19:38 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "tech.h"
#include "tech_extract.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "cvrt_variables.h"


//-----------------------------------------------------------------------------
// Main techfile package configuration.
//

namespace {
    // Extract and evaluate expression, write output in outbuf.  Return
    // false if error.
    //
    bool parse_eval(char **strp, char *outbuf)
    {
        char buf[512];
        char *s = *strp + 1;
        char *t = buf;
        int cnt = 1;
        for ( ; *s && cnt; s++) {
            if (*s == '(')
                cnt++;
            else if (*s == ')')
                cnt--;
            if (cnt)
                *t++ = *s;
        }
        *strp = s;
        *t = '\0';
        if (cnt || SIparse()->evaluate(buf, outbuf, 512)) {
            *outbuf = '0';
            *(outbuf+1) = '\0';
            return (false);
        }
        return (true);
    }

    // A dummy Device block parser, just skip over the block and warn the
    // user.  This will be overridden if extraction is present.
    //
    TCret device_parse(FILE *techfp)
    {
        if (Tech()->Matching(Ekw.Device())) {
            char *nm = 0;
            Tech()->SetCmtType(tBlkDev, 0);
            const char *kw;
            while ((kw = Tech()->GetKeywordLine(techfp)) != 0) {
                const char *line = Tech()->InBuf();
                if (Tech()->Matching(Ekw.Name()))
                    nm = lstring::gettok(&line);
                else if (Tech()->Matching(Ekw.End()))
                    break;
            }
            char *e = Tech()->SaveError(
                "Device %s block ignored, extraction system not present.",
                nm ? nm : "");
            delete [] nm;
            return (e);
        }
        return (TCnone);
    }

    TCret script_parse(FILE *techfp)
    {
        if (Tech()->Matching("Script")) {
            XM()->SaveScript(techfp);
            return (TCmatch);
        }
        return (TCnone);
    }

    TCret attribute_parse()
    {
        const char *inbuf = Tech()->InBuf();
        if (Tech()->Matching("DeviceLibrary")) {
            char *tt;
            if (Tech()->GetWord(&inbuf, &tt)) {
                char *s = lstring::strip_path(tt);
                if (s != tt) {
                    s = lstring::copy(s);
                    delete [] tt;
                    tt = s;
                }
                XM()->SetDeviceLibName(tt);
            }
            return (TCmatch);
        }
        if (Tech()->Matching("ModelLibrary")) {
            char *tt;
            if (Tech()->GetWord(&inbuf, &tt)) {
                char *s = lstring::strip_path(tt);
                if (s != tt) {
                    s = lstring::copy(s);
                    delete [] tt;
                    tt = s;
                }
                XM()->SetModelLibName(tt);
            }
            return (TCmatch);
        }
        if (Tech()->Matching("ModelSubdir")) {
            char *tt;
            if (Tech()->GetWord(&inbuf, &tt))
                XM()->SetModelSubdirName(tt);
            return (TCmatch);
        }
        if (Tech()->Matching("MergeOnRead")) {
            // This used to be a techfile param that simply set
            // MergeInput.  It is no longer defined or documented, but
            // catch it here anyway.

            if (Tech()->GetBoolean(inbuf))
                CDvdb()->setVariable(VA_MergeInput, "");
            return (TCmatch);
        }
        return (TCnone);
    }


    void print_scripts(FILE *techfp)
    {
        XM()->DumpScripts(techfp);
    }
}


void
cMain::setupTech()
{
    Tech()->RegisterEvaluator(&parse_eval);
    Tech()->RegisterDeviceParser(&device_parse);
    Tech()->RegisterScriptParser(&script_parse);
    Tech()->RegisterAttributeParser(&attribute_parse);
    Tech()->RegisterPrintScripts(&print_scripts);
}

