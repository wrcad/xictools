
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
#include "errorlog.h"
#include "oa_if.h"
#include "pcell_params.h"
#include "oa.h"
#include "oa_tech_observer.h"
#include "oa_lib_observer.h"
#include "oa_errlog.h"
#include "reltag.h"


// Instantiate the error logger.
cOAerrLog OAerrLog;


// This is our function to get a cOA through dlsym.
//
extern "C" {
    cOA *oaptr()
    {
        return (new cOA);
    }
}


cOA::cOA()
{
    oa_tech_observer = 0;
    oa_lib_observer = 0;
    oa_open_lib_tab = 0;
    oa_idstr = OSNAME " " XIC_RELEASE_TAG;
    oa_api_major = 0;
    oa_initialized = false;
}


// Return the id string which is in the form OSNAME CVSTAG.  This must
// match the application.
//
const char *
cOA::id_string()
{
    return (oa_idstr);
}


// Initialize OpenAccess, call before first usage.  We determine the
// api major release number of the library here.  This is necessary as
// the api major can be 4 or 5.  For 22.04, 22.4X the api major is 4,
// for 22.5X the api major is 5.
//
// Normally, user code must be compiled against the correct api major. 
// The 22.50 that comes with Virtuoso IC617 is a "coalition release"
// so the source (even the includes) is unavailable unless we pay
// $thousands to join the coalition.  Even then, we would need
// separate plugins for api major 4 and 5.
//
// However, we have hacked around the problem (the main
// incompatibility is a change to class IBase, and the one place where
// this is used in the plugin code (oa_load.cc) has been fixed).
//
bool
cOA::initialize()
{
    if (oa_initialized)
        return (true);
    try {
        // Initialize OA with data model 4.  Our includes may be
        // ancient api-major 4.  To semi-support api-major 5/6, init
        // using the hard coded values and hope for the best.  We need
        // to find the api-major from the library, so we can init
        // accordingly.

        // This is safe to call before oaDesignInit.
        oaBuildInfo *bi = oaBuildInfo::find("oaBase");
        if (bi) {
            // Should never fail.  Get the second number in the
            // release triple.

            int amaj = bi->getMinorReleaseNum();
            if (amaj == 4 || (amaj >= 41 && amaj <= 43))
                oa_api_major = 4;
            else if (amaj >= 50 && amaj < 60)
                oa_api_major = 5;
            else if (amaj >= 60 && amaj < 70)
                oa_api_major = 6;
        }
        if (oacAPIMajorRevNumber == oa_api_major) {
            oaDesignInit(oacAPIMajorRevNumber, oacAPIMinorRevNumber, 4);
        }
        else if (oa_api_major >= 4) {
            oaDesignInit(oa_api_major, 0, 4);  // Hope for the best!
        }
        else {
            // We didn't determine the app-major value, or it is
            // something non-supported.
            Errs()->add_error("Unsupported OpenAccess version.");
            return (false);
        }

        // Setup an instance of the oaTech conflict observer.
        oa_tech_observer = new cOAtechObserver(1);

        // Setup an instance of the oaLibDefList observer.
        oa_lib_observer = new cOAlibObserver(1);
        oa_pcell_observer = new cOApcellObserver(1);

        // Read in the lib.defs file.
        oaLibDefList::openLibs();

        oa_initialized = true;
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
    catch (const char *a) {
        Errs()->add_error(a);
        return (false);
    }
    catch (...) {
        Errs()->add_error("OpenAccess init caused unexpected exception.");
        return (false);
    }
    return (true);
}


// Return a string giving the OpenAccess version number.
//
const char *
cOA::version()
{
    static char *version_str;
    if (version_str)
        return (version_str);

    oaBuildInfo *bi = oaBuildInfo::find("oaBase");
    if (bi)
        version_str = lstring::copy(bi->getBuildName());
    else
        version_str = lstring::copy("unknown");
    return (version_str);
}


// Static function.
// This function reads the feature list from an oaCompatibility error
// and formats an easy to read exception message.
//
void
cOA::handleFBCError(oaCompatibilityError &err)
{
    static const oaString domainFmt(
        "\t%s of the %s feature, which applies "
        "to %s objects in the %s portion of the "
        "%s database in the %s domain.\n");
    static const oaString noDomainFmt(
        "\t%s of the %s feature, which applies "
        "to %s objects in the %s portion of "
        "the %s database.\n");
    static const oaString errorMsg(
        "An incompatible OpenAccess database, %s, was "
        "encountered.\nException from OA: %s\nThis "
        "database contains the following unsupported "
        "features:\n%s");

    const oaFeatureArray features = err.getFeatures();
    oaString unsupported;
    for (oaUInt4 i = 0; i < features.getNumElements(); i++) {
        oaString tmp;
        oaString name;

        oaFeature *feature = features[i];
        feature->getName(name);

        if (feature->getDomain() == oacNoDomain) {
            tmp.format((const char*) noDomainFmt,
                       (const char*) feature->getDataModelModType().getName(),
                       (const char*) name,
                       (const char*) feature->getObjectType().getName(),
                       (const char*) feature->getCategory().getName(),
                       (const char*) feature->getDBType().getName());
        }
        else {
            tmp.format((const char*) domainFmt,
                       (const char*) feature->getDataModelModType().getName(),
                       (const char*) name,
                       (const char*) feature->getObjectType().getName(),
                       (const char*) feature->getCategory().getName(),
                       (const char*) feature->getDBType().getName(),
                       (const char*) feature->getDomain().getName());
        }
        unsupported += tmp;
    }

    oaString output;
    output.format(errorMsg,
                  (const char*) err.getDatabaseName(),
                  (const char*) err.getMsg(),
                  (const char*) unsupported);
    Errs()->add_error((const char*)output);
}

