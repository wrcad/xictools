
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
#include "cd_lgen.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_library.h"
#include "miscutil/filestat.h"
#include "miscutil/pathlist.h"


namespace {
    bool linematch(const char*, const char*);
    bool endmatch(const char*);
}

// Place the current cell in a copy of the device library as a cell
// named 'name', or save to file Read in the new library and update
// internals.
//
bool
cSced::saveAsDev(const char *name, bool tofile)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde) {
        Errs()->add_error("No current electrical cell!");
        return (false);
    }
    CDm_gen mgen(cursde, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        if (mdesc->hasInstances()) {
            Errs()->add_error(
                "Current cell contains instantiations, can't be a device.");
            return (false);
        }
    }
    CDs *cursdp = CurCell(Physical);
    if (cursdp && !cursdp->isEmpty()) {
        Errs()->add_error(
            "Current cell contains physical data, can't be a device.");
        return (false);
    }

    // Make sure external node is set to be undefined.
    for (CDp *pd = cursde->prptyList(); pd; pd = pd->next_prp()) {
        if (pd->value() == P_NODE)
            ((CDp_node*)pd)->set_enode(-1);
    }

#ifdef NEWNMP
#else
    CDp_name *pn = (CDp_name*)cursde->prpty(P_NAME);
    if (pn) {
        // Devices never have "set" name.
        pn->set_assigned_name(0);

        // Devices never have "subname".  This field is used only for
        // explicitly placed subcells.
        pn->set_subckt(false);
    }
#endif

    // Remove any odesc properties.  Geometry in library cell
    // has no electrical significance.
    CDl *ldesc;
    CDsLgen lgen(cursde);
    while ((ldesc = lgen.next()) != 0) {
        CDg gdesc;
        gdesc.init_gen(cursde, ldesc);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0)
            odesc->prptyFreeList();
    }

    if (tofile) {
        XM()->SetSavingDev(true);
        bool ret = XM()->SaveCellAs(0, false);
        XM()->SetSavingDev(false);
        if (!ret) {
            Errs()->add_error("Error occurred when writing cell file.");
            return (false);
        }
    }
    else {
        FILE *fp = pathlist::open_path_file(XM()->DeviceLibName(),
            CDvdb()->getVariable(VA_LibPath), "r", 0, true);
        if (!fp) {
            // Hmmmm, no device library, create one in cwd.
            fp = fopen(XM()->DeviceLibName(), "w");
            if (!fp) {
                Errs()->add_error("Can't create %s in current directory.",
                    XM()->DeviceLibName());
                return (false);
            }

            bool ret = FIO()->WriteFile(cursde, name, fp);
            fclose(fp);
            if (!ret) {
                Errs()->add_error(
                    "Error occurred when writing new library file.");
                return (false);
            }
        }
        else {
            char *tf = filestat::make_temp("zz");
            FILE *gp = fopen(tf, "w");
            if (!gp) {
                Errs()->add_error("Failed to open temporary file %s.", tf);
                delete [] tf;
                return (false);
            }

            // Update to temp file.
            char *s, buf[256];
            bool indev = false;
            bool found = false;
            while ((s = fgets(buf, 256, fp)) != 0) {
                if (linematch(buf, name)) {
                    if (!FIO()->WriteFile(cursde, name, gp)) {
                        fclose(fp);
                        fclose(gp);
                        unlink(tf);
                        delete [] tf;
                        Errs()->add_error(
                            "Failed to write current cell into new library.");
                        return (false);
                    }
                    indev = true;
                    found = true;
                    continue;
                }
                if (indev && endmatch(buf)) {
                    indev = false;
                    continue;
                }
                if (!indev) {
                    for ( ; *s; s++)
                        putc(*s, gp);
                }
            }
            if (!found) {
                gp = freopen(tf, "a", gp);
                if (!gp) {
                    fclose(fp);
                    unlink(tf);
                    delete [] tf;
                    Errs()->add_error("Can't reopen temp. file.");
                    return (false);
                }
                putc('\n', gp);
                if (!FIO()->WriteFile(cursde, name, gp)) {
                    fclose(fp);
                    fclose(gp);
                    unlink(tf);
                    delete [] tf;
                    Errs()->add_error(
                        "Failed to write current cell into new library.");
                    return (false);
                }
            }
            fclose(fp);
            fp = freopen(tf, "r", gp);
            if (!fp) {
                unlink(tf);
                delete [] tf;
                Errs()->add_error("Can't reopen temp. file.");
                return (false);
            }

            // If device.lib exists in current directory, back up.
            if (!filestat::create_bak(XM()->DeviceLibName())) {
                fclose(fp);
                unlink(tf);
                delete [] tf;
                Errs()->add_error(filestat::error_msg());
                return (false);
            }
            gp = fopen(XM()->DeviceLibName(), "w");
            if (!gp) {
                fclose(fp);
                unlink(tf);
                delete [] tf;
                Errs()->add_error("Can't create %s in current directory.",
                    XM()->DeviceLibName());
                return (false);
            }
            int c;
            while ((c = getc(fp)) != EOF)
                putc(c, gp);
            fclose(gp);

            fclose(fp);
            unlink(tf);
            delete [] tf;
        }
    }

    // Update internals from new library.  We do this even when
    // writing as a cell file since the file or containing directory
    // may be referenced in the device library (device.lib) file.

    CDcellName cname = DSP()->CurCellName();
    FIO()->CloseLibrary(XM()->DeviceLibName(), LIBdevice);
    FIO()->OpenLibrary(CDvdb()->getVariable(VA_LibPath),
        XM()->DeviceLibName());
    DSP()->SetCurCellName(0);
    DSP()->SetTopCellName(0);

    bool wok = true;
    CDcbin cbin;
    if (OIfailed(CD()->OpenExisting(Tstring(cname), &cbin))) {
        Errs()->add_error("Huh?  The cell disappeared, this can' happen!");
        wok = false;
    }
    else
        XM()->SetNewContext(&cbin, true);
    XM()->ShowParameters();
    SCD()->PopUpDevs(0, MODE_UPD);
    return (wok);
}


namespace {
    // Return true if buf matches "(Symbol <name>);".
    //
    bool
    linematch(const char *s, const char *name)
    {
        char buf[64];
        while (isspace(*s))
            s++;
        if (*s != '(')
            return (false);
        s++;
        while (isspace(*s) || *s == 'S' || *s == 's')
            s++;
        if (*s++ != 'y')
            return (false);
        if (*s++ != 'm')
            return (false);
        if (*s++ != 'b')
            return (false);
        if (*s++ != 'o')
            return (false);
        if (*s++ != 'l')
            return (false);
        while (isspace(*s))
            s++;
        int i = 0;
        while (*s && !isspace(*s) && *s != ')')
            buf[i++] = *s++;
        buf[i] = '\0';
        if (!strcmp(buf, name))
            return (true);
        return (false);
    }


    // Return true if buf matches "E".
    //
    bool
    endmatch(const char *s)
    {
        if (*s != 'E')
            return (false);
        s++;
        while (isspace(*s))
            s++;
        if (!*s)
            return (true);
        return (false);
    }
}

