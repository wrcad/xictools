
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1986 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include "frontend.h"
#include "fteparse.h"
#include "ftedata.h"
#include "measure.h"
#include "outdata.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "device.h"
#include "input.h"
#include "spglobal.h"
#include "spnumber/spnumber.h"
#include "spnumber/hash.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "ginterf/graphics.h"
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#ifdef WIN32
#include <windows.h>
#include <libiberty.h>  // provides vasprintf
#else
#include <dlfcn.h>
#endif


//
// Functions to load, query and alter devices.
//

// Instantiate the device library error reporting interface.
sDevOut DVO;

// These two locations are set when a module is dynamically loaded.
// They provide the module name and build version.
//
const char *WRS_ModuleName;
const char *WRS_ModuleVersion;


namespace {

    // Database of loaded shared device model objects.
    //
    struct sMdb
    {
        struct mdb_t
        {
            mdb_t(const char *n, void *h, const IFdevice *d)
                {
                    path = n;
                    handle = h;
                    device = d;
                }

            const char *path;
            void *handle;
            const IFdevice *device;
        };

        sMdb() { m_tab = 0; }

        bool add_module(const char*, void*, NewDevFunc);
        bool rem_module(const char*);
        const char *find_path(const IFdevice*);
        void list_modules();
        void check();

    private:

        sHtab *m_tab;
    };
    sMdb DevmodDB;


    // Add a module to the database.
    //
    bool
    sMdb::add_module(const char *path, void *handle, NewDevFunc f)
    {
        if (!path || !handle)
            return (false);

        // Probably won't see this error, as dlopen will have failed.
        if (sHtab::get(m_tab, path))
            return (false);

        if (!m_tab)
            m_tab = new sHtab(true);

        int i = DEV.numdevs();
        DEV.loadDev(f);
        if (DEV.numdevs() == i) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "failed to allocate device.\n");
#ifdef WIN32
            FreeLibrary((HINSTANCE)handle);
#else
            dlclose(handle);
#endif
            return (false);;
        }
        TTY.printf("Loading module %s.\n", DEV.device(i)->name());

        mdb_t *m = new mdb_t(lstring::copy(path), handle, DEV.device(i));
        m_tab->add(m->path, m);
        check();
        return (true);
    }


    // Remove the module keyed by path, if any.
    //
    bool
    sMdb::rem_module(const char *path)
    {
        if (!path)
            return (false);
        if (!m_tab)
            return (true);
        mdb_t *m = (mdb_t*)m_tab->remove(path);
        if (m) {
            DEV.unloadDev(m->device);
#ifdef WIN32
            FreeLibrary((HINSTANCE)m->handle);
#else
            dlclose(m->handle);
#endif
        }
        return (true);
    }


    // Brute-force search for dv, return the path if found.
    //
    const char *
    sMdb::find_path(const IFdevice *dv)
    {
        if (!m_tab || !dv)
            return (0);
        sHgen gen(m_tab);
        sHent *h;
        while ((h = gen.next()) != 0) {
            mdb_t *m = (mdb_t*)h->data();
            if (m->device == dv)
                return (m->path);
        }
        return (0);
    }


    void
    sMdb::list_modules()
    {
        wordlist *wl = sHtab::wl(m_tab);
        if (!wl) {
            TTY.printf("No device modules loaded.\n");
            return;
        }
        for (wordlist *w = wl; w; w = w->wl_next) {
            const char *t = lstring::strrdirsep(w->wl_word);
            if (t) {
                t++;
                char *nn = lstring::copy(t);
                delete [] w->wl_word;
                w->wl_word = nn;
            }
        }
        wordlist::sort(wl);
        TTY.printf("Device modules loaded:\n");
        for (wordlist *w = wl; w; w = w->wl_next)
            TTY.printf("    %s\n", w->wl_word);
        TTY.printf("\n");
    }


    // Check for key/level clashes in devices list.  If a clash exists
    // for a loadable module, silently replace the module.  Otherwise,
    // warn the user that a module is inaccessible.  I module is
    // inaccessible only if it clashes with an internal device model.
    //
    void
    sMdb::check()
    {
        for (int i1 = 1; i1 < DEV.numdevs(); i1++) {
start_again:
            IFdevice *dv1 = DEV.device(i1);
            if (!dv1)
                continue;
            for (int i2 = 0; ; i2++) {
                const IFkeys *k1 = dv1->key(i2);
                if (!k1)
                    break;
                for (int i3 = 0; ; i3++) {
                    int l1 = dv1->level(i3);
                    if (!l1)
                        break;
                    for (int j1 = 0; j1 < i1; j1++) {
                        IFdevice *dv2 = DEV.device(j1);
                        if (!dv2)
                            continue;
                        for (int j2 = 0; ; j2++) {
                            const IFkeys *k2 = dv2->key(j2);
                            if (!k2)
                                break;
                            if (k2->key != k1->key)
                                continue;
                            for (int j3 = 0; ; j3++) {
                                int l2 = dv2->level(j3);
                                if (!l2)
                                    break;
                                if (l2 == l1) {
                                    const char *path = find_path(dv2);
                                    if (path) {
                                        rem_module(path);
                                        goto start_again;
                                    }
                                    TTY.printf(
                                        "Warning:  device %d (%s) is not "
                                        "accessible due to clash\n"
                                        "with %d (%s) for key=%c, level=%d\n",
                                        i1, dv1->name(), j1, dv2->name(),
                                        k1->key, l1);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


// Load a shared memory device object.
// Usage: devload [path_to_device.so]
//
// The argument can be a directory path, in which case all modules
// found in the directory will be loaded.  It can also be "all" in
// which case known modules are loaded, as at program startup.
//
void
CommandTab::com_devload(wordlist *wl)
{
    const char *path = wordlist::flatten(wl);
    if (!path || !*path) {
        delete [] path;
        DevmodDB.list_modules();
        return;
    }
    Sp.LoadModules(path);
    delete [] path;
}


namespace {
    // Device matching criteria: key letter and model level range.
    //
    struct ls_t
    {
        ls_t(int c)
            {
                next = 0;
                key = isupper(c) ? tolower(c) : c;
                minlev = -1;
                maxlev = -1;
            }

        static void destroy(ls_t *l)
            {
                while (l) {
                    ls_t *lx = l;
                    l = l->next;
                    delete lx;
                }
            }

        bool match(IFdevice*);

        ls_t *next;
        int key;
        int minlev;
        int maxlev;
    };


    // Return true if dev matches this.
    //
    bool ls_t::match(IFdevice *dev)
    {
        bool found = false;
        for (int j = 0; ; j++) {
            const IFkeys *k = dev->key(j);
            if (!k)
                break;
            int c = isupper(k->key) ? tolower(k->key) : k->key;
            if (c == key) {
                found = true;
                break;
            }
        }
        if (!found)
            return (false);
        if (minlev < 0)
            return (true);

        found = false;
        for (int j = 0; ; j++) {
            int l = dev->level(j);
            if (!l)
                break;
            if (l >= minlev && l <= maxlev) {
                found = true;
                break;
            }
        }
        return (found);
    }


    void list_dev(int i, IFdevice *dv)
    {
        TTY.printf("%d %s : %s\n", i, dv->name(), dv->description());
        for (int j = 0; ; j++) {
            const IFkeys *k = dv->key(j);
            if (!k)
                break;
            TTY.printf("  Key: %c  Terminals:", k->key);
            for (int n = 0; n < k->minTerms; n++)
                TTY.printf(" %s", k->termNames[n]);
            if (k->maxTerms > k->minTerms) {
                TTY.printf(" [");
                for (int n = k->minTerms; n < k->maxTerms; n++)
                    TTY.printf(" %s", k->termNames[n]);
                TTY.printf(" ]");
            }
            TTY.printf("\n");
        }

        bool found = false;
        int cnt = 0;
        for (int j = 0; ; j++) {
            int l = dv->level(j);
            if (!l)
                break;
            cnt++;
            found = true;
        }
        TTY.printf("  Model level");
        if (cnt > 1)
            TTY.printf("s:");
        else
            TTY.printf(":");
        for (int j = 0; ; j++) {
            int l = dv->level(j);
            if (!l)
                break;
            TTY.printf(" %d", l);
        }
        if (found)
            TTY.printf("\n");
        else
            TTY.printf(" 0\n");

        cnt = 0;
        found = false;
        for (int j = 0; ; j++) {
            const char *t = dv->modelKey(j);
            if (!t)
                break;
            cnt++;
            found = true;
        }

        TTY.printf("  Model name");
        if (cnt > 1)
            TTY.printf("s:");
        else
            TTY.printf(":");
        for (int j = 0; ; j++) {
            const char *t = dv->modelKey(j);
            if (!t)
                break;
            TTY.printf(" %s", t);
        }
        if (found)
            TTY.printf("\n");
        else
            TTY.printf(" none\n");
    }
}


// List known devices.
// Usage: devls [c[mmin[-mmax]]] ...
//  c = device key letter
//  mmin = min model level
//  mmax = max model level
// 
void
CommandTab::com_devls(wordlist *wl)
{
    ls_t *ls0 = 0, *lend = 0;
    while (wl) {
        const char *t = wl->wl_word;
        if (!isalpha(*t))
            continue;
        ls_t *ls = new ls_t(*t);
        int n, x;
        int i = sscanf(t+1, "%d-%d", &n, &x);
        if (i == 1) {
            ls->minlev = n;
            ls->maxlev = n;
        }
        else if (i > 1) {
            ls->minlev = SPMIN(n, x);
            ls->maxlev = SPMAX(n, x);
        }
        if (!ls0)
            ls0 = lend = ls;
        else {
            lend->next = ls;
            lend = lend->next;
        }
        wl = wl->wl_next;
    }

    TTY.init_more();
    TTY.printf("\n");
    for (int i = 0; i < DEV.numdevs(); i++) {
        IFdevice *dv = DEV.device(i);
        if (!dv)
            continue;

        if (ls0) {
            bool found = false;
            for (ls_t *ls = ls0; ls; ls = ls->next) {
                if (ls->match(dv)) {
                    found = true;
                    break;
                }
            }
            if (!found)
                continue;
        }
        list_dev(i, dv);
    }
    ls_t::destroy(ls0);
}


// devmod N [M1 ...]
void
CommandTab::com_devmod(wordlist *wl)
{
    if (!wl) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "No device index given to devmod.\n");
        return;
    }
    int index;
    if (sscanf(wl->wl_word, "%u", &index) != 1) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "Bad device index given to devmod.\n");
        return;
    }
    if (index >= DEV.numdevs()) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "Out of range device index given to devmod.\n");
        return;
    }
    IFdevice *dv = DEV.device(index);
    if (!dv) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "No device for index given to devmod.\n");
        return;
    }

    int models[NUM_DEV_LEVELS];
    memset(models, 0, NUM_DEV_LEVELS*sizeof(int));
    int levels = 0;
    wl = wl->wl_next;
    while (wl) {
        if (levels == NUM_DEV_LEVELS) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "Too many model levels given to devmod.\n");
            return;
        }
        if (sscanf(wl->wl_word, "%u", models + levels) != 1) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "Bad model level given to devmod.\n");
            return;
        }
        if (models[levels] < 1 || models[levels] > 255) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "Out of range model level given to devmod.\n");
            return;
        }
        levels++;
        wl = wl->wl_next;
    }
    if (levels) {
        if (dv->flags() & DV_NOLEVCHG) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "levels of device index %d can not be changed.\n", index);
            return;
        }
        for (int i = 0; i < NUM_DEV_LEVELS; i++)
            dv->setLevel(i, models[i]);
    }
    list_dev(index, dv);
    if (levels)
        DevmodDB.check();
}


namespace {
    inline bool pmatch(wordlist *plist, const char *kw)
    {
        while (plist) {
            if (CP.GlobMatch(plist->wl_word, kw))
                return (true);
            plist = plist->wl_next;
        }
        return (false);
    }

    void wl_tolower(wordlist *wl)
    {
        for (wordlist *w = wl; w; w = w->wl_next)
            lstring::strtolower(w->wl_word);
    }

    // Return a list of instance counts.
    //
    wordlist *devcnt(const sCKT *ckt, wordlist *matches)
    {
        char buf[80];
        wordlist *w0 = 0, *we = 0;
        sCKTmodGen mgen(ckt->CKTmodels);
        for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
            for (sGENmodel *dm = m; dm; dm = dm->GENnextModel) {
                if (matches) {
                    strcpy(buf, (const char*)dm->GENmodName);
                    lstring::strtolower(buf);
                    if (!pmatch(matches, buf))
                        continue;
                }

                int cnt = 0;
                sGENinstance *d = dm->GENinstances;
                for ( ; d; d = d->GENnextInstance)
                    cnt++;
                if (cnt) {
                    sprintf(buf, "%-14s %d", (const char*)dm->GENmodName, cnt);
                    char *t = buf + strlen(buf);
                    while (t - buf < 28)
                        *t++ = ' ';
                    *t++ = ' ';
                    strcpy(t, DEV.device(dm->GENmodType)->name());
                    if (!w0)
                        w0 = we = new wordlist(buf, 0);
                    else {
                        we->wl_next = new wordlist(buf, we);
                        we = we->wl_next;
                    }
                }
            }
        }
        wordlist::sort(w0);
        return (w0);
    }
}


void
CommandTab::com_devcnt(wordlist *wl)
{
    if (!Sp.CurCircuit()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no current circuit.\n");
        return;
    }
    bool freeckt = false;
    sCKT *ckt = Sp.CurCircuit()->runckt();
    if (!ckt) {
        int err = Sp.CurCircuit()->newCKT(&ckt, 0);
        if (err != OK) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "circuit parse failed: %s.\n",
                IP.errMesgShort(err));
            return;
        }
        if (!ckt)
            return;
    }

    // Need to lower-case for glob matching.
    wordlist *w0 = wordlist::copy(wl);
    wl_tolower(w0);

    wordlist *dvs = devcnt(ckt, w0);
    wordlist::destroy(w0);
    TTY.init_more();
    TTY.printf("\n");
    for (wordlist *w = dvs; w; w = w->wl_next)
        TTY.printf("%s\n", w->wl_word);
    TTY.printf("\n");
    wordlist::destroy(dvs);
    if (freeckt)
        delete ckt;
}


// show [-r|-d|-n node|-m|-D[M]|-M|-o|-O]  args [, params]
// -r, show resources
// -d, show devices
// -m, show models
// -o, show options
// -n node, show devices conneected to node 
// -D[M], show keywords for devices keyed by args, models too if -DM
// -M, show model keywords for devices keyed by args
//
void
CommandTab::com_show(wordlist *wl)
{
    if (!wl) {
        // default action
        Sp.Show(0, 0, false, -1);
        return;
    }
    if (*wl->wl_word == '-') {
        switch (*(wl->wl_word + 1)) {
        case 'n':
            {
                if (!wl->wl_next) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "no node name given.\n");
                    return;
                }
                if (!Sp.CurCircuit()) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "no current circuit.\n");
                    return;
                }
                if (!Sp.CurCircuit()->runckt()) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "no current simulation.\n");
                    return;
                }
                sCKT *ckt = Sp.CurCircuit()->runckt();

                char *nodename = lstring::copy(wl->wl_next->wl_word);
                sCKTnode *node;
                ckt->findTerm(&nodename, &node);
                if (!node) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "can't find node named \"%s\".\n", nodename);
                    return;
                }
                wl = wl->wl_next->wl_next;
                Sp.Show(wl, 0, false, node->number());
            }
            break;
        case 'r':
            wl = wl->wl_next;
            Sp.ShowResource(wl, 0);
            break;
        case 'd':
            wl = wl->wl_next;
            Sp.Show(wl, 0, false, -1);
            break;
        case 'D':
            {
                bool domod =
                    (*(wl->wl_word + 2) == 'M' || *(wl->wl_word + 2) == 'm');
                wl = wl->wl_next;
                sFtCirc::showDevParms(wl, true, domod);
            }
            break;
        case 'm':
            wl = wl->wl_next;
            Sp.Show(wl, 0, true, -1);
            break;
        case 'M':
            wl = wl->wl_next;
            sFtCirc::showDevParms(wl, false, true);
            break;
        case 'o':
            wl = wl->wl_next;
            Sp.ShowOption(wl, 0);
            break;
        case 'O':
            {
                wordlist xl;
                xl.wl_word = (char*)"all";
                xl.wl_next = wl->wl_next;
                Sp.ShowOption(&xl, 0);
            }
            break;
        default:
            GRpkgIf()->ErrPrintf(ET_ERROR, "bad option %c.\n",
                *(wl->wl_word + 1));
        }
    }
    else
        // default action
        Sp.Show(wl, 0, false, -1);
}


// Alter a device parameter.  The syntax here is
//   alter devicelist , parmname value [parmname value] ...
// where devicelist is as above, parmname is the name of the desired parm
// and value is a string, numeric, or bool value.
//
void
CommandTab::com_alter(wordlist *wl)
{
    if (!Sp.CurCircuit()) {
        Sp.Error(E_NOCURCKT);
        return;
    }
    wordlist *devs, *parms;
    if (!sFtCirc::parseDevParams(wl, &devs, &parms, false)) {
        wordlist::destroy(parms);
        GRpkgIf()->ErrPrintf(ET_ERROR, "no matching devices found.\n");
        return;
    }

    if (!devs && !parms) {
        Sp.CurCircuit()->printAlter();
        return;
    }
    if (!devs) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no devices in list.\n");
        wordlist::destroy(parms);
        return;
    }
    if (!parms) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no parameters in list.\n");
        wordlist::destroy(devs);
        return;
    }

    for (wordlist *tw = devs; tw; tw = tw->wl_next)
        Sp.CurCircuit()->alter(tw->wl_word, parms);

    wordlist::destroy(devs);
    wordlist::destroy(parms);
}
// End of CommandTab functions.


// Make sure dynamic linked device library is compatible.
//
void
IFsimulator::CheckDevlib()
{
    int i = 0;
    int req_major, req_minor, req_release;
    if (Global.DevlibVersion())
        i = sscanf(Global.DevlibVersion(), "%d.%d.%d", &req_major,
            &req_minor, &req_release);
    if (i != 3)
        // don't know our required version, skip test
        return;

    i = 0;
    int major, minor, release;
    if (DEV.version())
        i = sscanf(DEV.version(), "%d.%d.%d", &major, &minor,
            &release);
    if (i != 3 || req_major != major || req_minor != minor) {
        // old version or major/minor mismatch, exit
        fprintf(stderr,
"Expecting device library %s, found library %s which is incompatible.\n",
            Global.DevlibVersion(), DEV.version());
        exit(1);
    }
    if (req_release != release) {
        // release mismatch is assumed compatible, but warn
        fprintf(stderr,
"Expecting device library %s, found library %s, continuing anyway.\n",
            Global.DevlibVersion(), DEV.version());
    }
}


namespace {

    // Load modules found in the passed directory path.
    //
    void load_modules(const char *mpath)
    {
        DIR *wdir = opendir(mpath);
        if (!wdir)
            return;

        char *filepath = new char[strlen(mpath) + 128];
        char *t = lstring::stpcpy(filepath, mpath);
        if (t > filepath && !lstring::is_dirsep(*(t-1)))
            *t++ = '/';
        *t = 0;

#ifdef WIN32
        const char *sfx = "dll";
#else
#ifdef __APPLE__
        const char *sfx = "dylib";
#else
        const char *sfx = "so";
#endif
#endif

        struct dirent *de;
        while ((de = readdir(wdir)) != 0) {
            char *name = de->d_name;
            if (name[0] == '.') {
                if (!name[1] || (name[1] == '.' && !name[2]))
                    continue;
            }
            const char *dsfx = strrchr(de->d_name, '.');
            if (!dsfx || !lstring::cieq(dsfx+1, sfx))
                continue;
            strcpy(t, de->d_name);
            Sp.LoadModules(filepath);
        }
        delete [] filepath;
        closedir(wdir);
    }
}


// Load all of the loadable device models in the module path, or the
// startup_dir/devices directory if no module path.
//
void
IFsimulator::LoadModules(const char *str)
{
    // If given "all", load all modules found in the search areas. 
    // This is normally done on program startup.
    if (lstring::cieq(str, "all")) {
        LoadModules(0);
        return;
    }
    char *path = lstring::getqtok(&str);
    if (path && *path) {
        GFTtype tp = filestat::get_file_type(path);
        if (tp == GFT_NONE) {
            // If missing, append the shared library extension for the
            // OS and try again.
            const char *p = strrchr(path, '.');
#ifdef WIN32
            const char *slext = ".dll";
#else
#ifdef __APPLE__
            const char *slext = ".dylib";
#else
            const char *slext = ".so";
#endif
#endif
            if (!p || !lstring::cieq(p, slext)) {
                char *t = new char[strlen(path) + strlen(slext) + 1];
                strcpy(t, path);
                strcat(t, slext);
                delete [] path;
                path = t;
                tp = filestat::get_file_type(path);
            }
        }
        if (tp == GFT_FILE) {
            // Path to a regular file, assume it is a loadable module
            // and do the load.

            if (!lstring::is_rooted(path)) {
                // dlopen needs a full path.
                const char *p = path;
                path = pathlist::mk_path(".", p);
                delete [] p;
                p = path;
                path = pathlist::expand_path(p, true, true);
                delete [] p;
            }

            // Remove a module from the same path.  This must be done before
            // calling dlopen.
            DevmodDB.rem_module(path);

            WRS_ModuleName = 0;
            WRS_ModuleVersion = 0;

#ifdef WIN32
            HINSTANCE handle = LoadLibrary(path);
            if ((unsigned long)handle <= HINSTANCE_ERROR) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                "failed to dynamically load %s.\n", lstring::strip_path(path));
                delete [] path;
                return;
            }
#else
            void *handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
            if (!handle) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "failed to dynamically load module %s.\n%s\n",
                    lstring::strip_path(path), dlerror());
                delete [] path;
                return;
            }
#endif

            if (!WRS_ModuleVersion) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "failed to identify module %s build version number.\n",
                    lstring::strip_path(path));
#ifdef WIN32
                FreeLibrary(handle);
#else
                dlclose(handle);
#endif
                delete [] path;
                return;
            }
            if (!lstring::eq(WRS_ModuleVersion, Global.DevlibVersion())) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                "incompatible module %s, built for %s, this release is %s.\n",
                    lstring::strip_path(path), WRS_ModuleVersion,
                    Global.DevlibVersion());
#ifdef WIN32
                FreeLibrary(handle);
#else
                dlclose(handle);
#endif
                delete [] path;
                return;
            }
            if (!WRS_ModuleName) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "failed to identify module %s device name.\n",
                    lstring::strip_path(path));
#ifdef WIN32
                FreeLibrary(handle);
#else
                dlclose(handle);
#endif
                delete [] path;
                return;
            }
            // The name of the allocation function is the module name with
            // "_c" appended.
            char buf[128];
            strcpy(buf, WRS_ModuleName);
            strcat(buf, "_c");

#ifdef WIN32
            NewDevFunc f = (NewDevFunc)GetProcAddress(handle, buf);
#else
            // Should use dlfunc here, but Apple doesn't have it!
            NewDevFunc f = (NewDevFunc)dlsym(handle, buf);
#endif
            if (!f) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "failed to find module %s allocator function %s.\n",
                    lstring::strip_path(path), buf);
#ifdef WIN32
                FreeLibrary(handle);
#else
                dlclose(handle);
#endif
                delete [] path;
                return;
            }

            DevmodDB.add_module(path, handle, f);
        }
        else if (tp == GFT_DIR) {
            // Load all modules found in the directory.
            load_modules(path);
        }
        else {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "failed to find module or open directory in given path.\n");
        }
        delete [] path;
        return;
    }

    // Load all modules in startup or search path directories.
    VTvalue vv;
    if (Sp.GetVar(kw_modpath, VTYP_LIST, &vv)) {
        variable *v = vv.get_list();
        while (v) {
            if (v->type() == VTYP_STRING)
                load_modules(v->string());
            v = v->next();
        }
    }
    else if (Sp.GetVar(kw_modpath, VTYP_STRING, &vv)) {
        pathgen pgen(vv.get_string());
        char *p;
        while ((p = pgen.nextpath(true)) != 0) {
            load_modules(p);
            delete [] p;
        }
    }
    else {
        if (Global.StartupDir() && *Global.StartupDir()) {
            char *mpath = pathlist::mk_path(Global.StartupDir(), "devices");
            load_modules(mpath);
            delete [] mpath;
        }
    }
}


// Display various device parameters.  The input syntax is
//   [devicelist] [, parmlist]
// where devicelist can be "all", device names, or glob-matched subnames
// using wildcards *, ?, [].  The parms are names of parameters that should
// be valid for all the named devices, or "all".  Defaults to all, all.
// If contact_node is non-negative, show only devices connected to this
// node number.
//
void
IFsimulator::Show(wordlist *wl, char**, bool mod, int contact_node)
{
    if (!ft_curckt || !ft_curckt->runckt()) {
        Sp.Error(E_NOCURCKT);
        return;
    }
    wordlist *objs, *parms;
    if (!sFtCirc::parseDevParams(wl, &objs, &parms, mod)) {
        wordlist::destroy(parms);
        GRpkgIf()->ErrPrintf(ET_ERROR, "no matching devices found.\n");
        return;
    }

    if (parms == 0)
        parms = new wordlist("all", 0);

    if (objs == 0) {
        objs = mod ? (*ft_curckt->models())->wl(true) :
                (*ft_curckt->devices())->wl(true);
    }

    wordlist::sort(objs);
    TTY.init_more();
    TTY.printf("\n");

    sCKT *ckt = ft_curckt->runckt();
    for (wordlist *tw = objs; tw; tw = tw->wl_next) {
        char *name = lstring::copy(tw->wl_word);
        ckt->insert(&name);
        if (mod) {
            TTY.printf("%s: ", tw->wl_word);
            int typecode = ckt->finddev(name, 0, 0);
            if (typecode >= 0 && DEV.device(typecode) &&
                    DEV.device(typecode)->description())
                TTY.printf("%s\n", DEV.device(typecode)->description());
            else
                TTY.printf("\n");
        }
        else {
            sGENinstance *dev = 0;
            int typecode = ckt->finddev(name, &dev, 0);

            bool found = false;
            if (contact_node >= 0) {
                if (typecode < 0 || !dev)
                    continue;
                IFdevice *ifd = DEV.device(typecode);
                if (!ifd)
                    continue;
                IFkeys *k = ifd->keyMatch(*name);
                if (k) {
                    for (int j = 1; j <= k->maxTerms; j++) {
                        if (contact_node == *dev->nodeptr(j)) {
                            found = true;
                            break;
                        }
                    }
                }
                if (!found)
                    continue;
            }
                
            TTY.printf("%s: ", tw->wl_word);
            if (typecode >= 0 && DEV.device(typecode) &&
                    DEV.device(typecode)->name())
                TTY.printf("%s", DEV.device(typecode)->name());

            if (dev && dev->GENmodPtr) {
                sGENmodel *tmod = dev->GENmodPtr;
                if (tmod->GENmodName && *(char*)tmod->GENmodName)
                    TTY.printf(" (model %s)", (char*)tmod->GENmodName);
            }
            TTY.printf("\n");
        }

        bool foundp = false;
        for (wordlist *pw = parms; pw; pw = pw->wl_next) {
            variable *vv = ckt->getParam(tw->wl_word, pw->wl_word);
            for (variable *v = vv; v; v = v->next()) {
                int len = 14 + strlen(v->name());
                TTY.printf("  %s =", v->name());
                wordlist *w0 = v->varwl();
                for (wordlist *ww = w0; ww; ww = ww->wl_next) {
                    len += strlen(ww->wl_word) + 1;
                    TTY.printf(" %s", ww->wl_word);
                }
                wordlist::destroy(w0);

                for (; len < 40; len++)
                    TTY.send(" ");
                TTY.printf(" %s\n", v->reference());
                foundp = true;
            }
            variable::destroy(vv);
        }
        if (foundp)
            TTY.send("\n");
    }
    TTY.send("\n");

    wordlist::destroy(objs);
    wordlist::destroy(parms);
}
// End of IFsimulator functions.


namespace {
    bool check_prefix(const char *pfx, const char *string)
    {
        if (!pfx)
            return (true);
        if (!string)
            return (false);
        while (isalpha(*pfx)) {
            if ((isupper(*pfx) ? tolower(*pfx) : *pfx) !=
                    (isupper(*string) ? tolower(*string) : *string))
                return (false);
            pfx++;
            string++;
        }
        return (true);
    }
}


// This is exported to the device library, to avoid exporting the
// IFoutput class.  Basically the same as IFoutput::error.
//
void
sDevOut::textOut(OUTerrType flag, const char *fmt, ...)
{
    va_list args;
    if (flag == OUT_INFO && !Sp.GetFlag(FT_SIMDB))
        return;
    const char *pfx = 0;
    for (sMsg *m = OP.msgs(); m->string; m++) {
        if (    (flag == OUT_INFO && m->flag == ERR_INFO) ||
                (flag == OUT_WARNING && m->flag == ERR_WARNING) ||
                (flag == OUT_FATAL && m->flag == ERR_FATAL) ||
                (flag == OUT_PANIC && m->flag == ERR_PANIC)) {
            pfx = m->string;
            break;
        }
    }
    sLstr lstr;
    if (!check_prefix(pfx, fmt))
        lstr.add(pfx);

    va_start(args, fmt);
#ifdef HAVE_VASPRINTF
    char *str = 0;
    if (vasprintf(&str, fmt, args) >= 0) {
        lstr.add(str);
        delete [] str;
    }
#else
    char buf[1024];
    vsnprintf(buf, 1024, fmt, args);
    lstr.add(buf);
#endif
    va_end(args);

    const char *s = lstr.string();
    if (!s || *(s + strlen(s)-1) != '\n')
        lstr.add_c('\n');
    GRpkgIf()->ErrPrintf(ET_MSG, lstr.string());
}


// Print a header for a device error message.  To use this in
// model/instance loops, the inst pointer should be explicitly zeroed
// when out of scope.
//
void
sDevOut::printModDev(sGENmodel *mod, sGENinstance *inst, bool *chk)
{
    if (!chk || !*chk) {
        if (inst)
            textOut(OUT_NONE, "Device %s (model %s):", (const char*)inst->GENname,
                (const char*)mod->GENmodName);
        else if (mod)
            textOut(OUT_NONE, "Model %s:", (const char*)mod->GENmodName);
        if (chk)
            *chk = true;
    }
}


// Save a message in a queue.
//
void
sDevOut::pushMsg(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *str = 0;
#ifdef HAVE_VASPRINTF
    if (vasprintf(&str, fmt, args) < 0)
        return;
#else
    char buf[1024];
    vsnprintf(buf, 1024, fmt, args);
    str = lstring::copy(buf);
#endif
    va_end(args);

    wordlist *w = new wordlist;
    w->wl_word = str;
    if (dvo_msgs) {
        wordlist *wl = dvo_msgs;
        while (wl->wl_next)
            wl = wl->wl_next;
        wl->wl_next = w;
        w->wl_prev = wl;
    }
    else
        dvo_msgs = w;
}


// If there are messages in the queue, print them.
//
void
sDevOut::checkMsg(sGENmodel *mod, sGENinstance *inst)
{
    if (dvo_msgs) {
        printModDev(mod, inst);
        while (dvo_msgs) {
            wordlist *wl = dvo_msgs;
            dvo_msgs = dvo_msgs->wl_next;
            textOut(OUT_WARNING, "%s", wl->wl_word);
            delete [] wl->wl_word;
            delete wl;
        }
    }
}


//
// The following implement Verilog-A output functions.
//
namespace {
    // Add support for Verilog-A %-escapes.
    //
    // %h or %H display in hexadecimal format
    // %d or %D display in decimal format
    // %o or %O display in octal format
    // %b or %B display in binary format
    // %c or %C display in ASCII character format
    // %m or %M display hierarchical name
    // %s or %S display as a string
    // %e or %E display real in an exponential format
    // %f or %F display real in a decimal format
    // %g or %G display real in exponential or decimal format,
    //          whichever format results in the shorter printed output
    //
    char *fix_format(sGENmodel *model, sGENinstance *inst, const char *fmt)
    {
        const char *uctl = "DOBCMS";
        sLstr lstr;
        const char *s = fmt;
        if (!s)
            s = "";
        while (*s) {
            if (*s == '%') {
                char c = s[1];
                if (strchr(uctl, c))
                    c = tolower(c);
                if (c == 'h') {
                    lstr.add_c('%');
                    lstr.add_c('x');
                    s += 2;
                    continue;
                }
                if (c == 'H') {
                    lstr.add_c('%');
                    lstr.add_c('X');
                    s += 2;
                    continue;
                }
                if (c == 'm') {
                    if (inst)
                        lstr.add((const char*)inst->GENname);
                    else if (model)
                        lstr.add((const char*)model->GENmodName);
                    else
                        lstr.add("???");
                    s += 2;
                    continue;
                }
                lstr.add_c('%');
                lstr.add_c(c);
                s += 2;
                continue;
            }
            lstr.add_c(*s);
            s++;
        }
        lstr.add_c('\n');
        char *newfmt = lstr.string_trim();
        char *e = newfmt + strlen(newfmt) - 1;
        if (e > newfmt && e[-1] == '\n')
            *e = 0;
        return (newfmt);
    }


    // Get the string for $display, etc.  If newline, add a trailing
    // newline if not already there.  In any case, if the string would
    // otherwise be null or empty, add a newline.
    //
    char *get_string(sGENmodel *model, sGENinstance *inst, const char *fmt,
        va_list &args, bool newline)
    {
        char *nfmt = fix_format(model, inst, fmt);
        sLstr lstr;
#ifdef HAVE_VASPRINTF
        char *str = 0;
        if (vasprintf(&str, nfmt, args) >= 0) {
            lstr.add(str);
            delete [] str;
        }
#else
        char buf[1024];
        vsnprintf(buf, 1024, nfmt, args);
        lstr.add(buf);
#endif
        va_end(args);
        delete [] nfmt;
        if (!lstr.string() || !*lstr.string())
            lstr.add_c('\n');
        else if (newline) {
            const char *e = lstr.string();
            if (*(e + strlen(e) - 1) != '\n')
                lstr.add_c('\n');
        }
        return (lstr.string_trim());
    }
}


// Call when enalysis ends.
//
void
sDevOut::cleanup()
{
    for (int i = 0; i < OUT_MAX_FDS; i++) {
        if (dvo_fds[i] > 1)
            close(dvo_fds[i]);
        dvo_fds[i] = 0;
    }
    while (dvo_strobes) {
        sStrobeOut *s = dvo_strobes;
        dvo_strobes = dvo_strobes->next;
        delete s;
    }
    while (dvo_monitors) {
        sStrobeOut *s = dvo_monitors;
        dvo_monitors = dvo_monitors->next;
        delete s;
    }
}


int
sDevOut::fopen(const char *fname)
{
    int fd = open(fname, (O_CREAT|O_TRUNC|O_WRONLY), 0644);
    if (fd > 0) {
        for (int i = 0; i < OUT_MAX_FDS; i++) {
            if (dvo_fds[i] == 0) {
                dvo_fds[i] = fd;
                return (fd);
            }
        }
        textOut(OUT_WARNING, "too many open file descriptors.\n");
        return (fd);
    }
    textOut(OUT_WARNING, "failed to open *s.\n", fname);
    return (fd);
}


void
sDevOut::fclose(int fd)
{
    if (fd > 1) {
        close(fd);
        for (int i = 0; i < OUT_MAX_FDS; i++) {
            if (dvo_fds[i] == fd) {
                dvo_fds[i] = 0;
                break;
            }
        }
    }
}


namespace {
    void output(int fd, const char *str)
    {
        if (fd == 0)
            GRpkgIf()->ErrPrintf(ET_MSG, str);
        else if (fd <= 1)
            write(fileno(stdout), str, strlen(str));
        else
            write(fd, str, strlen(str));
    }
}


// The display functions immediately print the string.  A trailing
// newline will be added if none exists.
//
void
sDevOut::display(sGENmodel *model, sGENinstance *inst, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *str = get_string(model, inst, fmt, args, true);
    va_end(args);

    output(1, str);
    delete [] str;
}


void
sDevOut::fdisplay(sGENmodel *model, sGENinstance *inst, int fd,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *str = get_string(model, inst, fmt, args, true);
    va_end(args);

    output(fd, str);
    delete [] str;
}


// The write functions immediately print the string.
//
void
sDevOut::write(sGENmodel *model, sGENinstance *inst, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *str = get_string(model, inst, fmt, args, false);
    va_end(args);

    output(1, str);
    delete [] str;
}


void
sDevOut::fwrite(sGENmodel *model, sGENinstance *inst, int fd,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *str = get_string(model, inst, fmt, args, false);
    va_end(args);

    output(fd, str);
    delete [] str;
}


// The moditor functions print only when a variable changes, meaning
// that the string changes.
//
void
sDevOut::monitor(sGENmodel *model, sGENinstance *inst, int id,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *str = get_string(model, inst, fmt, args, true);
    va_end(args);

    if (!dvo_monitors) {
        dvo_monitors = new sStrobeOut(str, -1, id);
        output(1, str);
        return;
    }
    sStrobeOut *s = dvo_monitors;
    for ( ;; s = s->next) {
        if (s->id == id) {
            if (strcmp(s->string, str)) {
                delete [] s->string;
                s->string = str;
                output(1, str);
            }
            else
                delete [] str;
            return;
        }
        if (!s->next) {
            s->next = new sStrobeOut(str, -1, id);
            break;
        }
    }
    output(1, str);
}


void
sDevOut::fmonitor(sGENmodel *model, sGENinstance *inst, int id, int fd,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *str = get_string(model, inst, fmt, args, true);
    va_end(args);

    if (!dvo_monitors) {
        dvo_monitors = new sStrobeOut(str, fd, id);
        output(fd, str);
        return;
    }
    sStrobeOut *s = dvo_monitors;
    for ( ;; s = s->next) {
        if (s->id == id) {
            if (strcmp(s->string, str)) {
                delete [] s->string;
                s->string = str;
                s->fd = fd;
                output(fd, str);
            }
            else
                delete [] str;
            return;
        }
        if (!s->next) {
            s->next = new sStrobeOut(str, fd, id);
            break;
        }
    }
    output(fd, str);
}


void
sDevOut::strobe(sGENmodel *model, sGENinstance *inst, int id,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *str = get_string(model, inst, fmt, args, true);
    va_end(args);

    if (!dvo_strobes) {
        dvo_strobes = new sStrobeOut(str, -1, id);
        return;
    }
    sStrobeOut *s = dvo_strobes;
    for ( ;; s = s->next) {
        if (s->id == id) {
            delete [] s->string;
            s->string = str;
            return;
        }
        if (!s->next) {
            s->next = new sStrobeOut(str, -1, id);
            break;
        }
    }
}


void
sDevOut::fstrobe(sGENmodel *model, sGENinstance *inst, int id, int fd,
    const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *str = get_string(model, inst, fmt, args, true);
    va_end(args);

    if (!dvo_strobes) {
        dvo_strobes = new sStrobeOut(str, fd, id);
        return;
    }
    sStrobeOut *s = dvo_strobes;
    for ( ;; s = s->next) {
        if (s->id == id) {
            delete [] s->string;
            s->string = str;
            s->fd = fd;
            return;
        }
        if (!s->next) {
            s->next = new sStrobeOut(str, fd, id);
            break;
        }
    }
}


// Dump the strobe messages, clear the list.  This is called at the
// end of every analysis point (from sCKT::NIiter, sCKT::NIacIter).
//
void
sDevOut::dumpStrobe()
{
    while (dvo_strobes) {
        sStrobeOut *s = dvo_strobes;
        dvo_strobes = dvo_strobes->next;
        output(s->fd, s->string);
        delete s;
    }
}


void
sDevOut::warning(sGENmodel *model, sGENinstance *inst, const char *fmt, ...)
{
    va_list args;
    char *nfmt = fix_format(model, inst, fmt);
    const char *pfx = "Warning: ";
    sLstr lstr;
    if (!check_prefix(pfx, fmt))
        lstr.add(pfx);
    va_start(args, fmt);
#ifdef HAVE_VASPRINTF
    char *str = 0;
    if (vasprintf(&str, nfmt, args) >= 0) {
        lstr.add(str);
        delete [] str;
    }
#else
    char buf[1024];
    vsnprintf(buf, 1024, nfmt, args);
    lstr.add(buf);
#endif
    va_end(args);
    delete [] nfmt;
    GRpkgIf()->ErrPrintf(ET_MSG, lstr.string());
}


void
sDevOut::error(sGENmodel *model, sGENinstance *inst, const char *fmt, ...)
{
    va_list args;
    char *nfmt = fix_format(model, inst, fmt);
    const char *pfx = "Fatal: ";
    sLstr lstr;
    if (!check_prefix(pfx, fmt))
        lstr.add(pfx);
    va_start(args, fmt);
#ifdef HAVE_VASPRINTF
    char *str = 0;
    if (vasprintf(&str, nfmt, args) >= 0) {
        lstr.add(str);
        delete [] str;
    }
#else
    char buf[1024];
    vsnprintf(buf, 1024, nfmt, args);
    lstr.add(buf);
#endif
    va_end(args);
    delete [] nfmt;
    GRpkgIf()->ErrPrintf(ET_MSG, lstr.string());
}


// Support for the Verilog-A $finish function.  Print message according
// to argument.  The analysis should be terminated immediately after this
// call.
// 
//  n <= 0 prints nothing
//  n == 1 prints simulation time and location (default)
//  n >= 2 prints additional stats
//
int
sDevOut::finish(sGENmodel *model, sGENinstance *inst, double time, int n)
{
    // This pauses the analysis (i.e., it can be resumed).
    DVO.dumpStrobe();
    if (n > 0) {
        if (model) {
            if (inst)
                GRpkgIf()->ErrPrintf(ET_MSG,
                    "$finish called at time=%g from instance %s of model %s.\n",
                    time, (const char*)model->GENmodName,
                    (const char*)inst->GENname);
            else
                GRpkgIf()->ErrPrintf(ET_MSG,
                    "$finish called at time=%g from model %s.\n",
                    time, (const char*)model->GENmodName);
        }
        else
            GRpkgIf()->ErrPrintf(ET_MSG, "$finish called at time=%g.\n", time);
    }
    if (n > 1) {
        wordlist wl;
        wl.wl_word = (char*)"all";
        CommandTab::com_rusage(&wl);
    }
    return (E_PANIC);
}


// Support for the Verilog-A $stop function, n as above.  The analysis
// should be paused (i.e., can be resumed) after this call.
//
int
sDevOut::stop(sGENmodel *model, sGENinstance *inst, double time, int n)
{
    DVO.dumpStrobe();
    if (n > 0) {
        if (model) {
            if (inst)
                GRpkgIf()->ErrPrintf(ET_MSG,
                    "$stop called at time=%g from instance %s of model %s.\n",
                    time, (const char*)model->GENmodName,
                    (const char*)inst->GENname);
            else
                GRpkgIf()->ErrPrintf(ET_MSG,
                    "$stop called at time=%g from model %s.\n",
                    time, (const char*)model->GENmodName);
        }
        else
            GRpkgIf()->ErrPrintf(ET_MSG, "$stop called at time=%g.\n", time);
    }
    if (n > 1) {
        wordlist wl;
        wl.wl_word = (char*)"all";
        CommandTab::com_rusage(&wl);
    }
    raise(SIGINT);
    return (E_PAUSE);
}
// End of sDevOut functions.


dfrdlist *
sFtCirc::findDeferred(const char *dname, const char *param)
{
    dfrdlist *dl = ci_use_trial_deferred ? ci_trial_deferred : ci_deferred;
    while (dl) {
        if (lstring::cieq(dname, dl->dname) && lstring::cieq(param, dl->param))
            return (dl);
        dl = dl->next;
    }
    return (0);
}


// Add a deferred device or model parameter assignment.  These will be
// applied to the next analysis.
//
void
sFtCirc::addDeferred(const char *dname, const char *param, const char *rhs)
{
    dfrdlist *dl = ci_use_trial_deferred ? ci_trial_deferred : ci_deferred;
    dfrdlist *dp = 0;
    for (; dl; dp = dl, dl = dl->next) {
        if (lstring::cieq(dname, dl->dname) &&
                lstring::cieq(param, dl->param)) {
            char *r = lstring::copy(rhs);
            delete [] dl->rhs;
            dl->rhs = r;
            return;
        }
    }
    dl = new dfrdlist(dname, param, rhs);
    if (dp) {
        dp->next = dl;
        return;
    }
    if (ci_use_trial_deferred)
        ci_trial_deferred = dl;
    else
        ci_deferred = dl;
}


// Free the deferred lists.
//
void
sFtCirc::clearDeferred()
{
    dfrdlist::destroy(ci_deferred);
    ci_deferred = 0;
    dfrdlist::destroy(ci_trial_deferred);
    ci_trial_deferred = 0;
    ci_keep_deferred = false;
    ci_use_trial_deferred = false;
}


namespace {
    void apply_dfrd(sCKT *ckt, dfrdlist *dl)
    {
        sDataVec *t = 0;
        const char *rhs = dl->rhs;
        pnode *nn = Sp.GetPnode(&rhs, true);
        if (nn) {
            t = Sp.Evaluate(nn);
            delete nn;
        }
        if (t) {
            IFdata data;
            data.type = IF_REAL;
            data.v.rValue = t->realval(0);

            int err = ckt->setParam(dl->dname, dl->param, &data);
            if (err) {
                const char *msg = Sp.ErrorShort(err);
                GRpkgIf()->ErrPrintf(ET_ERROR, "could not set @%s[%s]: %s.\n",
                    dl->dname, dl->param, msg);
            }
        }
        else
            GRpkgIf()->ErrPrintf(ET_ERROR, "evaluation of %s failed.\n", rhs);
    }
}


// Apply the deferred device/model parameter setting, which clears the
// list.  This is done just before analysis, after any call to reset.
//
// There are actually two lists, for loop and range analysis support. 
// The trial list contains the changes for each trial (if any).  The
// normal list contains changes that were in effect before the
// analysis.  The trial list, applied after the normal list, is always
// cleared.  The normal list is kept in this case.
//
void
sFtCirc::applyDeferred(sCKT *ckt)
{
    if (!ci_deferred && !ci_trial_deferred)
        return;
    for (dfrdlist *dl = ci_deferred; dl; dl = dl->next)
        apply_dfrd(ckt, dl);
    for (dfrdlist *dl = ci_trial_deferred; dl; dl = dl->next)
        apply_dfrd(ckt, dl);
    dfrdlist::destroy(ci_trial_deferred);
    ci_trial_deferred = 0;
    if (!ci_keep_deferred) {
        dfrdlist::destroy(ci_deferred);
        ci_deferred = 0;
    }
}


// Apply the parameter values given to the named device.
//
void
sFtCirc::alter(const char *dname, wordlist *dparams)
{
    // The dparams list is a space-tokenized list of name [=] value
    // pairs.  Have to check all possibilities for association of the
    // '=', if it appears.  The value must be a number.

    for (wordlist *wl = dparams; wl; wl = wl->wl_next) {
        char *pname = lstring::copy(wl->wl_word);
        char *val = 0;
        char *t = strchr(pname, '=');
        if (t) {
            *t++ = 0;;
            if (*t)
                val = lstring::copy(t);
        }
        if (!val) {
            wl = wl->wl_next;
            if (!wl) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "no value given for %s.\n", pname);
                delete [] pname;
                return;
            }
            const char *ct = wl->wl_word;
            if (*ct == '=')
               ct++;
            if (*ct)
               val = lstring::copy(ct);
        }
        if (!val) {
            wl = wl->wl_next;
            if (!wl) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "no value given for %s.\n", pname);
                delete [] pname;
                return;
            }
            val = lstring::copy(wl->wl_word);
        }
        if (lstring::cieq(pname, "all")) {
            GRpkgIf()->ErrPrintf(ET_WARN,
                "invalid parameter name \"%s\".\n", pname);
            delete [] pname;
            delete [] val;
            continue;
        }

        const char *ct = val;
        if (!SPnum.parse(&ct, false)) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "non-numeric value given for %s.\n", pname);
            delete [] pname;
            delete [] val;
            return;
        }

        if (Sp.GetFlag(FT_SIMDB))
            GRpkgIf()->ErrPrintf(ET_MSGS,
                "adding deferred: device=%s param=%s value=%s\n",
                dname, pname, val);

        addDeferred(dname, pname, val);
        delete [] pname;
        delete [] val;
    }
}


void
sFtCirc::printAlter(FILE *fp)
{
    if (fp) {
        for (dfrdlist *dl = ci_deferred; dl; dl = dl->next)
            fprintf(fp, "%-16s %-16s %s\n", dl->dname, dl->param, dl->rhs);
        for (dfrdlist *dl = ci_trial_deferred; dl; dl = dl->next)
            fprintf(fp, "%-16s %-16s %s\n", dl->dname, dl->param, dl->rhs);
    }
    else {
        for (dfrdlist *dl = ci_deferred; dl; dl = dl->next)
            TTY.printf("%-16s %-16s %s\n", dl->dname, dl->param, dl->rhs);
        for (dfrdlist *dl = ci_trial_deferred; dl; dl = dl->next)
            TTY.printf("%-16s %-16s %s\n", dl->dname, dl->param, dl->rhs);
    }
}
// End of sFtCirc functions.


// Static function.
// Return lists of the device and model parameters for the device type
// associated with code.  Returns false if code is bad.
//
bool
sFtCirc::devParams(int code, wordlist **dwl, wordlist **mwl, bool parmstoo)
{
    char buf[BSIZE_SP];
    if (dwl)
        *dwl = 0;
    if (mwl)
        *mwl = 0;
    if (code < 0 || code >= DEV.numdevs())
        return (false);
    IFdevice *device = DEV.device(code);
    if (!device)
        return (true);
    if (dwl) {
        sprintf(buf, "device: %s (%s)", device->description(),
            device->name());
        wordlist *wl0 = new wordlist;
        wordlist *ww = wl0;
        ww->wl_word = lstring::copy(buf);

        for (int j = 0; ; j++) {
            const IFkeys *k = device->key(j);
            if (!k)
                break;
            if (k->maxTerms <= 0)
                sprintf(buf, "key: %c", k->key);
            else {
                sprintf(buf, "key: %c  terminals:", k->key);
                for (int i = 0; i < k->minTerms; i++)
                    sprintf(buf + strlen(buf), " %s", k->termNames[i]);
                if (k->minTerms != k->maxTerms) {
                    sprintf(buf + strlen(buf), " %s", "[");
                    for (int i = k->minTerms; i < k->maxTerms; i++)
                        sprintf(buf + strlen(buf), " %s", k->termNames[i]);
                    sprintf(buf + strlen(buf), " %s", "]");
                }
            }
            ww->wl_next = new wordlist;
            ww = ww->wl_next;
            ww->wl_word = lstring::copy(buf);
        }

        if (parmstoo) {
            for (int i = 0; ; i++) {
                IFparm *opt = device->instanceParm(i);
                if (!opt)
                    break;
                sprintf(buf, "%-18s ", opt->keyword);
                if (!(opt->dataType & IF_ASK))
                    strcat(buf, "NR ");
                else if (!(opt->dataType & IF_SET))
                    strcat(buf, "RO ");
                else
                    strcat(buf, "   ");
                if (opt->description)
                    strcat(buf, opt->description);
                else
                    strcat(buf, " (alias)");
                ww->wl_next = new wordlist;
                ww = ww->wl_next;
                ww->wl_word = lstring::copy(buf);
            }
        }

        ww->wl_next = new wordlist;
        ww = ww->wl_next;
        ww->wl_word = lstring::copy("");
        *dwl = wl0;
    }
    if (mwl) {
        if (!device->modelParm(0))
            return (true);
        wordlist *wl0 = new wordlist;
        wordlist *ww = wl0;
        if (device->description())
            ww->wl_word = lstring::copy(device->description());
        else {
            sprintf(buf, "%s model", device->name());
            ww->wl_word = lstring::copy(buf);
        }
        ww->wl_next = new wordlist;
        ww = ww->wl_next;

        if (device->level(0)) {
            int lcnt = 0;
            for ( ; ; lcnt++) {
                if (device->level(lcnt) == 0)
                    break;
            }
            sprintf(buf, "%s %s:", device->name(),
                lcnt > 1 ? "levels" : "level");
            for (lcnt = 0; ; lcnt++) {
                if (device->level(lcnt) == 0)
                    break;
                sprintf(buf + strlen(buf), " %d", device->level(lcnt));
            }
        }
        else 
            strcpy(buf, device->name());
        int ncnt = 0;
        for (int i = 0; ; i++) {
            const char *kstr = device->modelKey(i);
            if (!kstr)
                break;
            ncnt++;
        }
        if (ncnt) {
            sprintf(buf + strlen(buf), "  model %s:",
                ncnt > 1 ? "names" : "name");
            for (int i = 0; ; i++) {
                const char *kstr = device->modelKey(i);
                if (!kstr)
                    break;
                sprintf(buf + strlen(buf), " %s", kstr);
            }
        }
        ww->wl_word = lstring::copy(buf);

        if (parmstoo) {
            for (int i = 0; ; i++) {
                IFparm *opt = device->modelParm(i);
                if (!opt)
                    break;
                sprintf(buf, "%-18s ", opt->keyword);
                if (!(opt->dataType & IF_ASK))
                    strcat(buf, "NR ");
                else if (!(opt->dataType & IF_SET))
                    strcat(buf, "RO ");
                else
                    strcat(buf, "   ");
                if (opt->description)
                    strcat(buf, opt->description);
                else
                    strcat(buf, " (alias)");
                ww->wl_next = new wordlist;
                ww = ww->wl_next;
                ww->wl_word = lstring::copy(buf);
            }
        }

        ww->wl_next = new wordlist;
        ww = ww->wl_next;
        ww->wl_word = lstring::copy("");
        *mwl = wl0;
    }
    return (true);
}


// Static function.
// Print the parameter keywords and descriptions for devices
// and/or models.  If wl is 0 or "all", print for all devices
// known.  Otherwise, take the first character of each word as the
// key for the device to print.  The optional second character is
// the model level.
//
void
sFtCirc::showDevParms(wordlist *wl, bool devs, bool mods)
{
    if (!devs && !mods)
        return;
    TTY.init_more();
    if (!wl || lstring::cieq(wl->wl_word, "all")) {
        for (int i = 0; i < DEV.numdevs(); i++) {
            wordlist *dwl = 0, *mwl = 0;
            if (devs && mods)
                devParams(i, &dwl, &mwl, false);
            else if (devs && !mods)
                devParams(i, &dwl, 0, false);
            else
                devParams(i, 0, &mwl, false);
            wordlist *ww;
            for (ww = dwl; ww; ww = ww->wl_next)
                TTY.printf("%s\n", ww->wl_word);
            for (ww = mwl; ww; ww = ww->wl_next)
                TTY.printf("%s\n", ww->wl_word);
            wordlist::destroy(dwl);
            wordlist::destroy(mwl);
        }
        return;
    }
    char *doneit = new char[DEV.numdevs()];  // print entries once only
    memset(doneit, 0, DEV.numdevs());
    while (wl && wl->wl_word) {
        int lev = 1;
        bool onepass = false;
        if (isdigit(wl->wl_word[1])) {
            lev = atoi(wl->wl_word + 1);
            if (!lev)
                lev++;
            onepass = true;
        }
        for (;;) {
            int code = DEV.keyCode(*wl->wl_word, lev);
            if (code < 0 || doneit[code])
                break;
            doneit[code] = 1;
            wordlist *dwl = 0, *mwl = 0;
            if (devs && mods)
                devParams(code, &dwl, &mwl, true);
            else if (devs && !mods)
                devParams(code, &dwl, 0, true);
            else
                devParams(code, 0, &mwl, true);
            wordlist *ww;
            for (ww = dwl; ww; ww = ww->wl_next)
                TTY.printf("%s\n", ww->wl_word);
            for (ww = mwl; ww; ww = ww->wl_next)
                TTY.printf("%s\n", ww->wl_word);
            wordlist::destroy(dwl);
            wordlist::destroy(mwl);
            if (onepass)
                break;
            lev++;
        }
        wl = wl->wl_next;
    }
    delete [] doneit;
}


namespace {
    // Return true if s contains global substitution characters.
    //
    inline bool isglob(const char *s)
    {
        return (strchr(s, '?') || strchr(s, '*') || strchr(s, '[') ||
            strchr(s, '{'));
    }
}


// Static function.
// Given a device name, possibly with wildcards, return the matches.
//
wordlist *
sFtCirc::devExpand(const char *name, bool mod)
{
    // Model names are always case-insensitive.  The CSE_NODE flag
    // controls device name case sensitivity.
    bool ci = mod ? true : sHtab::get_ciflag(CSE_NODE);

    char buf[128];
    wordlist *wl = 0;
    if (isglob(name)) {
        if (!Sp.CurCircuit())
            return (0);
        wordlist *objs = mod ? (*Sp.CurCircuit()->models())->wl(true) :
            (*Sp.CurCircuit()->devices())->wl(true);
        wordlist *nlist = CP.BracExpand(name);
        if (ci)
            wl_tolower(nlist);
        for (wordlist *nl = nlist; nl; nl = nl->wl_next) {
            for (wordlist *d = objs; d; d = d->wl_next) {
                strcpy(buf, d->wl_word);
                if (ci)
                    lstring::strtolower(buf);
                if (CP.GlobMatch(nl->wl_word, buf)) {
                    wordlist *tw = new wordlist(d->wl_word, 0);
                    if (wl) {
                        wl->wl_prev = tw;
                        tw->wl_next = wl;
                        wl = tw;
                    }
                    else
                        wl = tw;
                }
            }
        }
        wordlist::destroy(objs);
        wordlist::destroy(nlist);
    }
    else if (lstring::cieq(name, "all")) {
        if (!Sp.CurCircuit())
            return (0);
        wl = mod ? (*Sp.CurCircuit()->models())->wl(true) :
            (*Sp.CurCircuit()->devices())->wl(true);
    }
    else
        wl = new wordlist(name, 0);
    wordlist::sort(wl);
    return (wl);
}


// Static function.
// Split out the device and parameter lists.  Do glob expansion of
// the device list.  Return false only if devices were given but
// none was resolved.
//
bool
sFtCirc::parseDevParams(wordlist *line, wordlist **dlist, wordlist **plist,
    bool mod)
{
    *dlist = 0;
    *plist = 0;
    if (!line)
        return (true);
    char *cl = wordlist::flatten(line);
    if (!cl)
        return (true);
    bool ret = parseDevParams(cl, dlist, plist, mod);
    delete [] cl;
    return (ret);
}


bool
sFtCirc::parseDevParams(const char *list, wordlist **dlist, wordlist **plist,
    bool mod)
{
    *dlist = 0;
    *plist = 0;
    if (!list)
        return (true);

    char *cl = lstring::copy(list);
    char *pl = 0;
    int ccnt = 0;
    // Find the field separator comma, if any.  Don't be confused by
    // commas in curly brackets (globbing)
    for (char *s = cl; *s; s++) {
        if (*s == '{') {
            ccnt++;
            continue;
        }
        if (*s == '}') {
            if (ccnt)
                ccnt--;
            continue;
        }
        if (*s == ',' && !ccnt) {
            *s++ = 0;
            while (isspace(*s))
                s++;
            if (*s)
                pl = lstring::copy(s);
            break;
        }
    }

    // Now expand the devicelist...
    wordlist *w0 = 0;
    char *tok;
    char *s = cl;
    bool gotdev = false;
    while ((tok = lstring::gettok(&s)) != 0) {
        w0 = wordlist::append(w0, devExpand(tok, mod));
        delete [] tok;
        gotdev = true;
    }
    *dlist = w0;
    *plist = pl ? new wordlist(pl) : 0;
    delete [] cl;
    delete [] pl;

    return (!gotdev || *dlist);
}

