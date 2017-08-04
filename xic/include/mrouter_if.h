
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

