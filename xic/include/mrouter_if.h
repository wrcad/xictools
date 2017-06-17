
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2016 Whiteley Research Inc, all rights reserved.        *
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
 $Id: mrouter_if.h,v 5.11 2017/02/15 18:33:54 stevew Exp $
 *========================================================================*/

#ifndef MROUTER_IF_H
#define MROUTER_IF_H

//
// Access to the router via plug-in.
//


class cMRif;
class cLDDBif;
struct dbNet;
struct lefRouteLayer;

inline class cMRcmdIf *MRcmd();

class cMRcmdIf
{
    static cMRcmdIf *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cMRcmdIf *MRcmd() { return (cMRcmdIf::ptr()); }

    cMRcmdIf();

    bool openRouter();
    bool createRouter();
    cLDDBif *newLDDB();
    cMRif *newRouter(cLDDBif*);
    void destroyRouter();

    bool doCmd(const char*);

    bool cmdReadTech(const char*);
    bool cmdWriteTech(const char*);
    bool cmdSource(const char*);
    bool cmdPlace(const char*);
    bool cmdImplementRoutes(const char*);

    // Message printed on prompt line after command returns,
    // caller should free.
    char *doneMsg()
        {
            char *dm = if_donemsg;
            if_donemsg = 0;
            return (dm);
        }

private:
    bool findLayer(const char*, lefRouteLayer**, CDl**);
    bool findLayer(int, lefRouteLayer**, CDl**);
    bool findLefLayer(const CDl*, lefRouteLayer**);
    bool createVias();
    bool placeVia(const char*, int, int);
    bool removeVia(const char*, int, int);
    bool newWire(const char*, Plist*, int);
    bool removeWire(const char*, Plist*, int);
    bool implementRoute(dbNet*);
    bool setLayers();
    void loadMRouterFuncs();

    cLDDBif *if_l;
    cMRif   *if_r;
    cLDDBif*  (*if_new_db)();
    cMRif*  (*if_new_router)(cLDDBif*);
    bool    if_error;
    int     if_stepnet;
    char    *if_donemsg;
    const char *if_version;

    CDl     **if_layers;
    int     if_numlayers;

    static cMRcmdIf *instancePtr;
};

#endif

