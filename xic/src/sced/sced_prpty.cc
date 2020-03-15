
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
#include "sced.h"
#include "edit.h"
#include "undolist.h"
#include "dsp_inlines.h"
#include "cd_netname.h"
#include "cd_hypertext.h"
#include "errorlog.h"


// Set a device instance property.  The name is of the form
// "devname.property" or "devname property".
//
bool
cSced::setDevicePrpty(const char *name, const char *string)
{
    if (!DSP()->CurCellName())
        return (false);
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);
    while (isspace(*name))
        name++;
    char devn[32];
    char *s = devn;
    while (*name && !isspace(*name) && *name != '.')
        *s++ = *name++;
    *s = '\0';
    while (isspace(*name) || *name == '.')
        name++;
    if (!*name)
        return (false);
    char key[32];
    s = key;
    while (*name && !isspace(*name))
        *s++ = *name++;
    *s = '\0';

    int type = ScedIf()->whichPrpty(key, 0);
    if (!type)
        return (0);

    while (isspace(*string))
        string++;
    if (!*string)
        string = 0;
    CDc *cdesc = cursde->findInstance(devn);
    if (cdesc) {
        if (cdesc->isDevice()) {
            switch (type) {
            case P_MODEL:
            case P_VALUE:
            case P_PARAM:
            case P_OTHER:
            case P_NOPHYS:
            case P_RANGE:
            case P_NAME:
            case P_DEVREF:
                break;
            default:
                return (false);
            }
        }
        else {
            switch (type) {
            case P_PARAM:
            case P_OTHER:
            case P_NOPHYS:
            case P_FLATTEN:
            case P_RANGE:
            case P_NAME:
            case P_SYMBLC:
                break;
            default:
                return (false);
            }
        }
        Ulist()->ListCheckPush("setdev", cursde, false, true);
        prptyModify(cdesc, 0, type, string, 0);
        Ulist()->ListPop();
        return (true);
    }
    return (setMutParam(devn, type, string));
}


// Return a device property as a string (copied).  The name is of the
// form "devname.property" or "devname property".
//
char *
cSced::getDevicePrpty(const char *name)
{
    if (!DSP()->CurCellName())
        return (0);
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (0);
    while (isspace(*name))
        name++;
    char devn[64];
    char *s = devn;
    while (*name && !isspace(*name) && *name != '.')
        *s++ = *name++;
    *s = '\0';
    while (isspace(*name) || *name == '.')
        name++;
    if (!*name)
        return (0);
    char key[32];
    s = key;
    while (*name && !isspace(*name))
        *s++ = *name++;
    *s = '\0';

    int type = ScedIf()->whichPrpty(key, 0);
    if (!type)
        return (0);

    char *retstr = 0;
    CDc *cdesc = cursde->findInstance(devn);
    if (cdesc) {
        if (cdesc->isDevice()) {
            switch (type) {
            case P_MODEL:
            case P_VALUE:
            case P_PARAM:
            case P_OTHER:
            case P_NOPHYS:
            case P_RANGE:
            case P_NAME:
            case P_DEVREF:
                break;
            default:
                return (0);
            }
        }
        else {
            switch (type) {
            case P_PARAM:
            case P_OTHER:
            case P_NOPHYS:
            case P_FLATTEN:
            case P_RANGE:
            case P_NAME:
            case P_SYMBLC:
                break;
            default:
                return (0);
            }
        }
        for (CDp *pd = cdesc->prpty_list(); pd; pd = pd->next_prp()) {
            if (pd->value() == type) {
                if (type == P_RANGE) {
                    sLstr lstr;
                    CDp_range *pr = (CDp_range*)pd;
                    lstr.add_i(pr->beg_range());
                    lstr.add_c(' ');
                    lstr.add_i(pr->end_range());
                    return (lstr.string_trim());
                }
                else {
                    bool copied;
                    hyList *hs = pd->label_text(&copied, cdesc);
                    retstr = hyList::string(hs, HYcvPlain, false);
                    if (copied)
                        hyList::destroy(hs);
                }
                return (retstr);
            }
        }
    }
    else
        retstr = getMutParam(devn, type);
    return (retstr);
}


// Add/replace the property specified, taking care of pointers and
// redisplay, electrical mode only.  Return the new property if one is
// created.
//
CDp *
cSced::prptyModify(CDc *cdesc, CDp *oldp, int value, const char *string,
    hyList *hystr)
{
    if (!cdesc)
        return (0);
    CDs *sdesc = cdesc->parent();
    if (!sdesc || !sdesc->isElectrical())
        return (0);
    if (!cdesc->masterCell())
        return (0);

    bool iscur = false;
    if (sdesc->cellname() == DSP()->CurCellName())
        iscur = true;
    if (value == P_NAME) {
        if (!oldp)
            oldp = cdesc->prpty(value);
        if (!oldp) {
            Log()->ErrorLog(mh::Properties,
                "Can't reassign name of unnamed device.");
            // Can't add name to gnd device.
            return (0);
        }
        CDp_cname *pn = (CDp_cname*)oldp;
        if (pn->key() == P_NAME_TERM) {
            // Just update label of terminal.
            if (hystr)
                addDeviceLabel(cdesc, oldp, oldp, hystr, false, iscur);
            else {
                hyList *hy = string ? new hyList(0, string, HYcvAscii) : 0;
                addDeviceLabel(cdesc, oldp, oldp, hy, false, iscur);
                if (hy)
                    hyList::destroy(hy);
            }
            return (0);
        }

        char *nstr = 0;
        if (hystr)
            nstr = hyList::string(hystr, HYcvPlain, false);
        else if (string)
            nstr = lstring::copy(string);

        bool new_range = false;
        if (nstr) {
            if (*nstr && isalpha(*nstr)) {
                // Look for a range suffix.  If found, strip it from
                // the name string, and modify the range property if
                // necessary.

                unsigned int rbeg, rend;
                const char *e = CDp_range::str_rspec(nstr, &rbeg, &rend);
                CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
                if (e) {
                    nstr[e-nstr] = 0;
                    if (!pr || rbeg != pr->beg_range() ||
                            rend != pr->end_range()) {
                        char trange[64];
                        sprintf(trange, "%u %u", rbeg, rend);
                        prptyModify(cdesc, pr, P_RANGE, trange, 0);
                        new_range = true;
                    }
                }
                else if (pr) {
                    // No range suffix, delete any range property.

                    prptyModify(cdesc, pr, P_RANGE, 0, 0);
                    new_range = true;
                }
            }

            // filter out delimiters
            char *s;
            if ((s = strchr(nstr, '.')) != 0)
                *s = '\0';
            if ((s = strchr(nstr, ';')) != 0)
                *s = '\0';

            if (*nstr && isalpha(*nstr)) {
                // If the name hasn't changed, we're done.
                const char *instname = cdesc->getElecInstBaseName(pn);
                if (!strcasecmp(instname, nstr)) {
                    delete nstr;
                    return (0);
                }

                if (!cdesc->nameOK(nstr)) {
                    Log()->ErrorLogV(mh::Properties,
                "Can't assign name %s, conflicts with another assignment.",
                        nstr);
                    delete nstr;
                    return (0);
                }
            }
            else {
                Log()->ErrorLogV(mh::Properties,
                    "Can't assign name %s, first char must be alpha.",
                    nstr);
                return (0);
            }
        }
        CDp_cname *pnnew = (CDp_cname*)pn->dup();
        pnnew->set_assigned_name(nstr);

        // When editing the label, have to get out of this to avoid
        // munging the label size.
        if (new_range)
            ED()->setLabelOverride(0, 0, 0, 0, 0);

        addDeviceLabel(cdesc, pnnew, oldp, 0, false, iscur);
        Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, pnnew);
        return (pnnew);
    }
    else if (value == P_MODEL || value == P_VALUE ||
            value == P_PARAM || value == P_OTHER || value == P_DEVREF) {
        if (value != P_OTHER) {
            // P_OTHER properties are added, not replaced
            if (!oldp)
                oldp = cdesc->prpty(value);
        }

        // Find name.
        CDp_cname *pn = (CDp_cname*)cdesc->prpty(P_NAME);
        // Allow only "other" properties on gnd and terminal
        // devices.
        //
        if (!pn) {
            CDp_node *pnd = (CDp_node*)cdesc->prpty(P_NODE);
            if (pnd && !pnd->next()) {
                // A ground device.
                if (value != P_OTHER) {
                    Log()->ErrorLog(mh::Properties,
                        "Only `other' properties allowed on gnd terminal.");
                    return (0);
                }
            }
        }
        else if (!pn->name_string() || pn->key() == P_NAME_NULL) {
            if (value != P_OTHER) {
                Log()->ErrorLog(mh::Properties,
                    "Only `other' properties allowed on inactive cell.");
                return (0);
            }
        }
        else if (pn->key() == P_NAME_TERM) {
            // A terminal.
            if (value != P_OTHER) {
                Log()->ErrorLog(mh::Properties,
                    "Only `other' properties allowed on terminal.");
                return (0);
            }
        }
        if (string || hystr) {
            CDp_user *pu = new CDp_user(value);
            if (hystr)
                pu->set_data(hyList::dup(hystr));
            else
                pu->set_data(string ? new hyList(0, string, HYcvAscii) : 0);
            if (pu->value() != P_OTHER)
                addDeviceLabel(cdesc, pu, oldp, 0, false, iscur);
            Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, pu);
            return (pu);
        }
        if (value != P_OTHER) {
            // the property was destroyed
            Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, 0);
            addDeviceLabel(cdesc, 0, oldp, 0, false, iscur);
        }
    }
    else if (value == P_NOPHYS) {
        if (!oldp)
            oldp = cdesc->prpty(value);
        if (string || hystr) {
            bool shorted = false;
            if (string) {
                if (*string == 's' || *string == 'S')
                    shorted = true;
            }
            else if (hystr) {
                char *str = hyList::string(hystr, HYcvPlain, false);
                if (*str == 's' || *str == 'S')
                    shorted = true;
                delete str;
            }
            const char *pstr = shorted ? "shorted" : "nophys";
            CDp *pnew = new CDp(pstr, P_NOPHYS);
            sdesc->unsetConnected();
            Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, pnew);
            return (pnew);
        }
        // the property was destroyed
        sdesc->unsetConnected();
        Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, 0);
    }
    else if (value == P_FLATTEN) {
        if (!oldp)
            oldp = cdesc->prpty(value);
        if (string || hystr) {
            CDp *pnew = new CDp("flatten", P_FLATTEN);
            sdesc->unsetConnected();
            Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, pnew);
            return (pnew);
        }
        // the property was destroyed
        sdesc->unsetConnected();
        Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, 0);
    }
    else if (value == P_SYMBLC) {
        if (!oldp)
            oldp = cdesc->prpty(value);
        // If the string is "0", create a new property with all fields
        // null and active false.  This will force this subckt
        // instance to not be shown symbolically.  Otherwise, remove
        // any existing property, the instance will be shown as
        // specified in its master.

        if (!sdesc->isDevice()) {
            bool create = false;
            if (string || hystr) {
                if (string) {
                    if (*string == '0')
                        create = true;
                }
                else if (hystr) {
                    char *str = hyList::string(hystr, HYcvPlain, false);
                    if (*str == '0')
                        create = true;
                    delete str;
                }
            }
            if (create) {
                CDp_sym *pnew = new CDp_sym;
                pnew->set_active(false);
                sdesc->unsetConnected();
                Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, pnew);
                return (pnew);
            }
        }
        // the property was destroyed
        sdesc->unsetConnected();
        Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, 0);
    }
    else if (value == P_RANGE) {
        if (!oldp)
            oldp = cdesc->prpty(value);

        if (!hystr && !string) {
            // Destroy existing property.
            if (oldp) {
                sdesc->unsetConnected();
                Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, 0);
            }
            return (0);
        }

        char *str;
        if (hystr)
            str = hyList::string(hystr, HYcvPlain, true);
        else
            str = lstring::copy(string);
        if (!str)
            str = lstring::copy("");

        char *s = str;
        while (isspace(*s))
            s++;
        bool ok = false;
        int beg = 0, end = 0;
        if (isdigit(*s)) {
            beg = atoi(s);
            while (isdigit(*s))
                s++;
            while (*s && !isdigit(*s))
                s++;
            if (isdigit(*s)) {
                end = atoi(s);
                ok = true;
            }
        }
        delete [] str;
        if (!ok) {
            Log()->ErrorLog(mh::Properties,
                "Parse error in range, expected two integers.");
            return (0);
        }
        CDp_range *pr = new CDp_range;
        pr->set_range(beg, end);
        sdesc->unsetConnected();
        Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, pr);
        return (pr);
    }
    return (0);
}


// Function to obtain the default layer for internally generated
// property labels.
//
CDl *
cSced::defaultLayer(CDp *pdesc)
{
    CDl *ld = 0;
    if (pdesc) {
        switch (pdesc->value()) {
        case P_NAME:
            {
                CDp_cname *pn = (CDp_cname*)pdesc;
                int k = pn->key();
                if (isalpha(k))
                    ld = CDldb()->findLayer("NAME", Electrical);
                else
                    ld = CDldb()->findLayer("NODE", Electrical);
            }
            break;
        case P_MODEL:
            ld = CDldb()->findLayer("MODL", Electrical);
            break;
        case P_VALUE:
        case P_NEWMUT:
            ld = CDldb()->findLayer("VALU", Electrical);
            break;
        case P_PARAM:
            ld = CDldb()->findLayer("PARM", Electrical);
            if (!ld)
                ld = CDldb()->findLayer("INIT", Electrical);
            break;
        case P_DEVREF:
            ld = CDldb()->findLayer("ETC1", Electrical);
            break;
        }
    }
    if (!ld)
        ld = CDldb()->findLayer("ETC1", Electrical);
    if (!ld)
        ld = CDldb()->findLayer("ETC2", Electrical);
    if (!ld)
        ld = CDldb()->findLayer("SCED", Electrical);
    return (ld);
}

