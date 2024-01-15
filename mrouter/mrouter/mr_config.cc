
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
 *                                                                        *
 * LEF/DEF Database and Maze Router.                                      *
 *                                                                        *
 * Stephen R. Whiteley (stevew@wrcad.com)                                 *
 * Whiteley Research Inc. (wrcad.com)                                     *
 *                                                                        *
 * Portions adapted from Qrouter by Tim Edwards,                          *
 * (www.opencircuitdesign.com) which used code by Steve Beccue.           *
 * See original headers where applicable.                                 *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/*--------------------------------------------------------------*/
/* qconfig.c -- .cfg file read/write for route                  */
/*--------------------------------------------------------------*/
/* Written by Steve Beccue 2003                                 */
/*--------------------------------------------------------------*/
/* Modified by Tim Edwards, June 2011.  The route.cfg file is   */
/* no longer the main configuration file but is supplementary   */
/* to the LEF and DEF files.  Various configuration items that  */
/* do not appear in the LEF and DEF formats, such as route      */
/* costing, appear in this file, as well as a filename pointer  */
/* to the LEF file with information on standard cell macros.    */
/*--------------------------------------------------------------*/

#include <stdarg.h>
#include <errno.h>
#include <math.h>
#include <algorithm>
#include "config.h"
#include "mrouter_prv.h"


//
// Maze Router.
//
// This reads a Qrouter config file.
//
// DEPRECATED.  This is kept around for now for Qrouter compatibility,
// but has been replaced by the scripting functionality.
//

#define MAX_LINE_LEN    2048


#ifndef HAVE_STRCASESTR
// MinGW doesn't have this.
namespace {
    const char *strcasestr(const char *big, const char *little)
    {
        int n = strlen(little);
        int b = strlen(big);
        if (b < n)
            return (0);
        const char *e = big + b - n;
        for (const char *s = big; s <= e; s++) {
            if (!strncasecmp(s, little, n))
                return (s);
        }
        return (0);
    }
}
#endif


// readConfig
//
// Read in the config file.
//
// ARGS:            The filename (normally route.cfg).
// RETURNS:         Number of lines successfully read.
// SIDE EFFECTS:    Loads Global configuration variables.
//
// Arg "is_info" indicates if router was called with the -i option in
// which case the config file read should stop before any def file is
// read.
//
// When incr is true, there is no initialization performed. 
// Otherwise, the database is cleared and reset before reading the
// file.
//
int
cMRouter::readConfig(const char *filename, bool is_info, bool incr)
{
    if (!filename)
        filename = MR_CONFIG_FILENAME;
    FILE *fconfig = fopen(filename, "r");
    if (!fconfig) {
        db->emitErrMesg("Cannot open config file: %s\n", strerror(errno));
        return (-1);
    }
    if (!incr)
        reset(true);

    char sarg[MAX_LINE_LEN];
    char line[MAX_LINE_LEN];
    lefMacro *gateinfo = 0;       // Gate information, pin location, etc.
    int count = 0;

    while (!feof(fconfig)) {
        fgets(line, MAX_LINE_LEN, fconfig);
        char *lineptr = line;
        while (isspace(*lineptr))
            lineptr++;

        bool OK = false;
        if (!strncasecmp(lineptr, "lef", 3) ||
                !strncmp(lineptr, "read_lef", 8)) {
            if (sscanf(lineptr, "%*s %s\n", sarg) == 1) {
                // Argument is a filename of a LEF file from which we
                // should get the information about gate pins &
                // obstructions.

                OK = true;
                db->lefRead(sarg);
            }
        }

        // The remainder of the statements is not case sensitive.

        for (int i = 0; line[i] && i < MAX_LINE_LEN - 1; i++) {
            if (isupper(line[i]))
                line[i] = tolower(line[i]);
        }

        int iarg;
        if (sscanf(lineptr, "num_layers %d", &iarg) == 1) {
            OK = true;
            db->setNumLayers(iarg);
        }
        else if (sscanf(lineptr, "layers %d", &iarg) == 1) {
            OK = true;
            db->setNumLayers(iarg);
        }

        int iarg2;
        if (sscanf(lineptr, "layer_%d_name %s", &iarg2, sarg) == 2) {
            if (iarg2 > 0 && iarg2 <= (int)numLayers()) {
                OK = true;
                db->setLayerName(iarg2 - 1, sarg);
            }
        }

        if (sscanf(lineptr, "gds_layer_%d %d", &iarg2, &iarg) == 2) {
            if (iarg2 > 0 && iarg2 <= (int)numLayers()) {
                OK = true;
                db->setLayerNumber(iarg2 - 1, iarg);
            }
        }
        if (sscanf(lineptr, "gds_datatype_%d %d", &iarg2, &iarg) == 2) {
            if (iarg2 > 0 && iarg2 <= (int)numLayers()) {
                OK = true;
                db->setPurposeNumber(iarg2 - 1, iarg);
            }
        }

        if (sscanf(lineptr, "comment_layer_name %s", sarg) == 1) {
            OK = true;
            db->setCommentLayerName(sarg);
        }
        if (sscanf(lineptr, "gds_comment_layer %d", &iarg) == 1) {
            OK = true;
            db->setCommentLayerNumber(iarg);
        }
        if (sscanf(lineptr, "gds_comment_datatype %d", &iarg) == 1) {
            OK = true;
            db->setCommentLayerPurpose(iarg);
        }

        double darg;
        if (sscanf(lineptr, "layer_1_width %lf", &darg) == 1) {
            OK = true;
            db->setPathWidth(0, db->micToLefGrid(darg));
        }
        if (sscanf(lineptr, "layer_2_width %lf", &darg) == 1) {
            OK = true;
            db->setPathWidth(1, db->micToLefGrid(darg));
        }
        if (sscanf(lineptr, "layer_3_width %lf", &darg) == 1) {
            OK = true;
            db->setPathWidth(2, db->micToLefGrid(darg));
        }
        if (sscanf(lineptr, "layer_4_width %lf", &darg) == 1) {
            OK = true;
            db->setPathWidth(3, db->micToLefGrid(darg));
        }
        if (sscanf(lineptr, "layer_5_width %lf", &darg) == 1) {
            OK = true;
            db->setPathWidth(4, db->micToLefGrid(darg));
        }
        if (sscanf(lineptr, "layer_6_width %lf", &darg) == 1) {
            OK = true;
            db->setPathWidth(5, db->micToLefGrid(darg));
        }
        if (sscanf(lineptr, "layer_7_width %lf", &darg) == 1) {
            OK = true;
            db->setPathWidth(6, db->micToLefGrid(darg));
        }
        if (sscanf(lineptr, "layer_8_width %lf", &darg) == 1) {
            OK = true;
            db->setPathWidth(7, db->micToLefGrid(darg));
        }
        if (sscanf(lineptr, "layer_9_width %lf", &darg) == 1) {
            OK = true;
            db->setPathWidth(8, db->micToLefGrid(darg));
        }

        if (sscanf(lineptr, "x lower bound %lf", &darg) == 1) {
            OK = true;
            db->setXlower(db->micToLefGrid(darg));
        }
        if (sscanf(lineptr, "x upper bound %lf", &darg) == 1) {
            OK = true;
            db->setXupper(db->micToLefGrid(darg));
        }
        if (sscanf(lineptr, "y lower bound %lf", &darg) == 1) {
            OK = true;
            db->setYlower(db->micToLefGrid(darg));
        }
        if (sscanf(lineptr, "y upper bound %lf", &darg) == 1) {
            OK = true;
            db->setYupper(db->micToLefGrid(darg));
        }

        int i;
        char carg;
        if ((i = sscanf(lineptr, "layer %d wire pitch %lf\n", &iarg, &darg))
                == 2) {
            if (iarg > 0 && iarg <= (int)numLayers()) {
                OK = true;
                db->setPitchX(iarg-1, db->micToLefGrid(darg));
            }
        }
        else if (i == 1) {
            if ((i = sscanf(lineptr, "layer %*d vertical %d\n", &iarg2))
                    == 1) {
                if (iarg > 0 && iarg <= (int)numLayers()) {
                    OK = true;
                    db->setVert(iarg - 1, iarg2);
                }
            }
            else if ((i = sscanf(lineptr, "layer %*d %c\n", &carg)) == 1) {
                if (iarg > 0 && iarg <= (int)numLayers()) {
                    if (tolower(carg) == 'v') {
                        OK = true;
                        db->setVert(iarg - 1, true);
                    }
                    else if (tolower(carg) == 'h') {
                        OK = true;
                        db->setVert(iarg - 1, false);
                    }
                }
            }
        }

        if (sscanf(lineptr, "num passes %d\n", &iarg) == 1) {
            OK = true;
            if (iarg > 0 && iarg <= MR_MAX_PASSES)
                setNumPasses(iarg);
        }
        else if (sscanf(lineptr, "passes %d\n", &iarg) == 1) {
            OK = true;
            if (iarg > 0 && iarg <= MR_MAX_PASSES)
                setNumPasses(iarg);
        }
        
        if (sscanf(lineptr, "route segment cost %d", &iarg) == 1) {
            OK = true;
            setSegCost(iarg);
        }
        
        if (sscanf(lineptr, "route via cost %d", &iarg) == 1) {
            OK = true;
            setViaCost(iarg);
        }
        
        if (sscanf(lineptr, "route jog cost %d", &iarg) == 1) {
            OK = true;
            setJogCost(iarg);
        }
        
        if (sscanf(lineptr, "route crossover cost %d", &iarg) == 1) {
            OK = true;
            setXverCost(iarg);
        }

        if (sscanf(lineptr, "route offset cost %d", &iarg) == 1) {
            OK = true;
            setOffsetCost(iarg);
        }
        if (sscanf(lineptr, "route block cost %d", &iarg) == 1) {
            OK = true;
            setBlockCost(iarg);
        }

        if (sscanf(lineptr, "do not route node %s\n", sarg) == 1) {
            OK = true; 
            db->dontRoute(sarg);
        }
        
        if (sscanf(lineptr, "route priority %s\n", sarg) == 1) {
            OK = true; 
            db->criticalNet(sarg);
        }
        
        if (sscanf(lineptr, "critical net %s\n", sarg) == 1) {
            OK = true; 
            db->criticalNet(sarg);
        }

        // Search for "no stack".  This allows variants like "no
        // stacked contacts", "no stacked vias", or just "no
        // stacking", "no stacks", etc.

        if (strcasestr(lineptr, "no stack") != 0) {
            OK = true;
            setStackedVias(1);
        }

        // Search for "stack N", where "N" is the largest number of
        // vias that can be stacked upon each other.  Values 0 and 1
        // are both equivalent to specifying "no stack".

        if (sscanf(lineptr, "stack %d", &iarg) == 1) {
            OK = true;
            setStackedVias(iarg);
        }
        else if (sscanf(lineptr, "via stack %d", &iarg) == 1) {
            OK = true;
            setStackedVias(iarg);
        }

        // Look for via patterning specifications.
        if (strcasestr(lineptr, "via pattern")) {
            if (strcasestr(lineptr + 12, "normal"))
                setViaPattern(VIA_PATTERN_NORMAL);
            else if (strcasestr(lineptr + 12, "invert"))
                setViaPattern(VIA_PATTERN_INVERT);
        }

        double darg2, darg3, darg4;
        if (sscanf(lineptr, "obstruction %lf %lf %lf %lf %s\n",
                &darg, &darg2, &darg3, &darg4, sarg) == 5) {

            iarg = getLayer(sarg);
            if (iarg >= 0) {
                OK = true;
                db->addObstruction(
                    db->micToLefGrid(darg), db->micToLefGrid(darg2),
                    db->micToLefGrid(darg3), db->micToLefGrid(darg4), iarg);
            }
        }

        if (sscanf(lineptr, "gate %s %lf %lf\n", sarg, &darg, &darg2) == 3) {
            OK = true; 
            gateinfo = getLefGate(sarg);
            if (gateinfo) {
                gateinfo->width = db->micToLefGrid(darg);
                gateinfo->height = db->micToLefGrid(darg2);
            }
            else {
                gateinfo = new lefMacro(lstring::copy(sarg), darg, darg2);
                db->lefAddGate(gateinfo);
            }
        }
        
        if (sscanf(lineptr, "endgate %s\n", sarg) == 1) {
            OK = true; 
            if (gateinfo) {
                int cnt = 0;
                for (lefPin *p = gateinfo->pins; p; p = p->next)
                    cnt++;
                gateinfo->nodes = cnt;
                gateinfo->obs = 0;
                // This syntax does not include declaration of obstructions.
                gateinfo = 0;
            }
        }

        if (sscanf(lineptr, "pin %s %lf %lf\n", sarg, &darg, &darg2) == 3) {
            if (gateinfo) {
                OK = true; 
                // These style gates have only one tap per gate; LEF
                // file reader allows multiple taps per gate node.

                // This syntax always defines pins on layer 0; LEF file
                // reader allows pins on all layers.

                lefPin *pnew = new lefPin(lstring::copy(sarg),
                    new dbDseg(
                        db->micToLefGrid(darg), db->micToLefGrid(darg2),
                        db->micToLefGrid(darg), db->micToLefGrid(darg2),
                        0, -1, 0),
                    PORT_CLASS_UNSET, PORT_USE_UNSET, PORT_SHAPE_UNSET, 0);
                if (!gateinfo->pins)
                    gateinfo->pins = pnew;
                else {
                    lefPin *p = gateinfo->pins;
                    while (p->next)
                        p = p->next;
                    p->next = pnew;
                }
            }
        }

        if (!OK) {
            if (!(lineptr[0] == '\n' || lineptr[0] == '#' || lineptr[0] == 0)) {
                if (!is_info)   // Don't report errors on info file generation.
                    db->emitErrMesg("line not understood: %s\n", line);
            }
        }
        OK = false;
        line[0] = line[1] = '\0';

    }
    fclose(fconfig);
    return (count);
}

