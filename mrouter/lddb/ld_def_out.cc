
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA 2016, http://wrcad.com       *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY OR WHITELEY     *
 *   RESEARCH INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,   *
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 *   DEALINGS IN THE SOFTWARE.                                            *
 *                                                                        *
 *   Licensed under the Apache License, Version 2.0 (the "License");      *
 *   you may not use this file except in compliance with the License.     *
 *   You may obtain a copy of the License at                              *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *   Unless required by applicable law or agreed to in writing, software  *
 *   distributed under the License is distributed on an "AS IS" BASIS,    *
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      *
 *   implied. See the License for the specific language governing         *
 *   permissions and limitations under the License.                       *
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
 $Id: ld_def_out.cc,v 1.11 2017/02/15 01:39:36 stevew Exp $
 *========================================================================*/

#include "lddb_prv.h"
#include "defwWriter.hpp"
#include "miscutil/tvals.h"
#include <errno.h>


//
// LEF/DEF Database.
//
// Functions to generate DEF output, and access the routed nets.
//


// Set the dbu per micron to use in the DEF file.
//
bool
cLDDB::defOutResolSet(u_int resol)
{
    if (!resol) {
        db_def_out_resol = db_def_resol;
        return (LD_OK);
    }

    // DEF only allows certain values, check this.
    switch (resol) {
    case 100:
    case 200:
    case 400:
    case 800:
    case 1000:
    case 2000:
    case 4000:
    case 8000:
    case 10000:
    case 20000:
        break;
    default:
        emitErrMesg(
            "Error: DEF dbu/micron %d is not an accepted value.\n", resol);
        return (LD_BAD);
    }

    if (db_lef_resol % resol != 0) {
        emitErrMesg(
            "Error: DEF dbu/micron %d is numerically incompatible "
            "with LEF\ndbu/micron %d.\n",
            resol, db_lef_resol);
        return (LD_BAD);
    }
    db_def_out_resol = resol;

    return (LD_OK);
}


// Dump a DEF file from the content of the database.
//
bool
cLDDB::defWrite(const char *fname)
{
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        emitErrMesg("Cannot open output file: %s\n", strerror(errno));
        return (LD_BAD);
    }
    if (!db_def_out_resol)
        db_def_out_resol = db_def_resol;

    long time0 = Tvals::millisec();

    // Write DEF header.
    defwInit(fp,
        5,              // int vers1
        8,              // int vers2
        0,              // const char *caseSensitive (not used)
        "/",            // const char *dividerChar
        "<>",           // const char *busBitChars
        db_design,      // const char *designName
        db_technology,  // const char *technology
        0,              // const char *array
        0,              // const char *floorplan
        (double)db_def_out_resol // double units
    );

    // DIEAREA
    defwNewLine();
    {
        int xl = lefToDef(db_xLower);
        int yl = lefToDef(db_yLower);
        int xh = lefToDef(db_xUpper);
        int yh = lefToDef(db_yUpper);

        defwDieArea(xl, yl, xh, yh);
    }

    // TRACKS
    defwNewLine();
    for (u_int i = 0; i < db_numLayers; i++) {
        dbLayer *l = &db_layers[i];
        const char *ll[1];
        ll[0] = l->lid.lname;
        if (l->vert) {
            int strt = lefToDef(db_xLower);
            int step = lefToDef(l->pitchX);

            defwTracks(
                "X",                                // const char *master
                strt,                               // int doStart
                l->numChanX,                        // int doCount
                step,                               // int doStep
                1,                                  // int numLayers
                ll                                  // const char **layers
            );
        }
        else {
            int strt = lefToDef(db_yLower);
            int step = lefToDef(l->pitchY);

            defwTracks(
                "Y",                                // const char *master
                strt,                               // int doStart
                l->numChanY,                        // int doCount
                step,                               // int doStep
                1,                                  // int numLayers
                ll                                  // const char **layers
            );
        }
    }

    // BLOCKAGES
    int nblk = 0;
    for (dbDseg *sg = db_userObs; sg; sg = sg->next)
        nblk++;
    if (nblk) {
        defwNewLine();
        defwStartBlockages(nblk);
        for (dbDseg *sg = db_userObs; sg; sg = sg->next) {
            const char *lname =
                sg->lefId >= 0 ? db_lef_objects[sg->lefId]->lefName : 0;
            if (!lname) {
                if (sg->layer >= 0) {
                    // This should already be set.
                    lefRouteLayer *rl = getLefRouteLayer(sg->layer);
                    int id = rl ? rl->lefId : -1;
                    lname = db_lef_objects[id]->lefName;
                }
                if (!lname)
                    continue;
            }
            defwBlockagesLayer(lname);
            defwBlockagesRect(lefToDef(sg->x1), lefToDef(sg->y1),
                lefToDef(sg->x2), lefToDef(sg->y2));
        }
        defwEndBlockages();
    }

    // COMPONENTS
    if (db_numGates) {
        if (!nblk)
            defwNewLine();
        defwStartComponents(db_numGates);
        for (u_int k = 0; k < db_numGates; k++) {
            dbGate *g = db_nlGates[k];
            int xd = lefToDef(g->placedX);
            int yd = lefToDef(g->placedY);

            const char *status = 0;
            switch (g->placed) {
            case LD_NOLOC:
                break;
            case LD_COVER:
                status = "COVER";
                break;
            case LD_FIXED:
                status = "FIXED";
                break;
            case LD_PLACED:
                status = "PLACED";
                break;
            }

            // The Cadence doc is horribly out-of-date here, arg list
            // was obtained from header file.
            defwComponent(
                g->gatename,            // const char *name
                g->gatetype->gatename,  // const char *master
                0,                      // int numNetName
                0,                      // const char **netNames
                0,                      // const char *eeq
                0,                      // const char *genName
                0,                      // const char *genParameters
                0,                      // const char *source
                0,                      // int numForeign
                0,                      // const char **foreigns
                0,                      // int *foreignX
                0,                      // int *foreignY
                0,                      // int *foreignOrients
                status,                 // const char *status
                xd,                     // int statusX
                yd,                     // int statusY
                g->orient,              // int statusOrient
                0.0,                    // double weight
                0,                      // const char *region
                0,                      // int xl
                0,                      // int yl
                0,                      // int xh
                0                       // int yh
            );
        }
        defwEndComponents();
    }

    // PINS
    if (db_numPins) {
        if (!db_numGates)
            defwNewLine();
        defwStartPins(db_numPins);
        for (u_int k = 0; k < db_numPins; k++) {
            dbGate *g = db_nlPins[k];
            int xd = lefToDef(g->placedX);
            int yd = lefToDef(g->placedY);

            const char *status = 0;
            switch (g->placed) {
            case LD_NOLOC:
                break;
            case LD_COVER:
                status = "COVER";
                break;
            case LD_FIXED:
                status = "FIXED";
                break;
            case LD_PLACED:
                status = "PLACED";
                break;
            }

            defwPin(
                g->gatename,            // const char *pinName
                g->gatename,            // const char *netName
                0,                      // int special
                0,                      // const char *direction
                0,                      // const char *use
                status,                 // const char *status
                xd,                     // int statusX
                yd,                     // int statusY
                g->orient,              // int orient
                0,                      // const char *layer
                0,                      // int xl
                0,                      // int yl
                0,                      // int xh
                0                       // int yh
            );

            for (dbDseg *sg = g->taps[0]; sg; sg = sg->next) {
                if (sg->layer >= 0) {
                    const char *lname = db_lef_objects[sg->lefId]->lefName;
                    int xld = lefToDef(sg->x1 - g->placedX);
                    int yld = lefToDef(sg->y1 - g->placedY);
                    int xhd = lefToDef(sg->x2 - g->placedX);
                    int yhd = lefToDef(sg->y2 - g->placedY);

                    defwPinLayer(
                        lname,              // const char *layerName
                        0,                  // int spacing
                        0,                  // int designRuleWidth
                        xld,                // int xl
                        yld,                // int yl
                        xhd,                // int xh
                        yhd                 // int yh
                    );
                }
            }
        }
        defwEndPins();
    }

    // NETS
    defwStartNets(db_numNets);
    for (u_int i = 0; i < db_numNets; i++) {
        dbNet *net = db_nlNets[i];
        defwNet(net->netname);
        for (dbNode *node = net->netnodes; node; node = node->next) {
            dbGate *g = getGateOrPinByNum(node->gorpnum);
            if (g) {
                if (node->gorpnum & 1) {
                    // A pin.
                    defwNetConnection(
                        "PIN",              // const char *compName
                        g->gatename,        // const char *pinName
                        0                   // int synthesized
                    );
                }
                else {
                    lefMacro *ginfo = g->gatetype;
                    if (ginfo) {
                        int n = 1;
                        lefPin *pin = ginfo->pins;
                        for ( ; pin; pin = pin->next, n++) {
                            if (node->pinindx == n)
                                break;
                        }
                        if (pin) {
                            defwNetConnection(
                                g->gatename,        // const char *compName
                                pin->name,          // const char *pinName
                                0                   // int synthesized
                            );
                        }
                        else {
                            emitErrMesg(
                                "Warning: net %s, node of %s has unknown "
                                "pin index %d.\n", node->pinindx);
                        }
                    }
                    else {
                        emitErrMesg(
                            "Warning: net %s, node of %s has null master.\n",
                            g->gatename);
                    }
                }
            }
            else {
                emitErrMesg(
                    "Warning: net %s, unknown gate/pin id %d in node.\n",
                    node->gorpnum);
            }
        }

        // Write the physical routes, if any.
        fprintf(fp, "\n   ");
        writeDefNetRoutes(fp, net, false);

        defwNetEndOneNet();
    }
    defwEndNets();

    // SPECIALNETS
    int specnets = 0;
    for (u_int i = 0; i < db_numNets; i++) {
        if (db_nlNets[i]->spath)
            specnets++;
    }
    if (specnets > 0) {
        defwStartSpecialNets(specnets);
        for (u_int i = 0; i < db_numNets; i++) {
            dbNet *net = db_nlNets[i];
            if (net->spath) {
                defwSpecialNet(net->netname);
                fprintf(fp, "\n   ");
                writeDefNetRoutes(fp, net, true);
                defwSpecialNetEndOneNet();
            }
        }
        defwEndSpecialNets();
    }

    defwEnd();
    fclose(fp);
    if (db_verbose > 0) {
        long elapsed = Tvals::millisec() - time0;
        emitMesg("DEF write: Processed %d lines in %ld milliseconds.\n",
            defwCurrentLineNumber, elapsed);
    }
    return (LD_OK);
}


// Maximum length fixed by LEF/DEF specifications.
#define LD_LINE_MAX             2048

// Cell name (and other names) max length.
#define LD_MAX_NAME_LEN         1024

// writeDefRoutes
//
// Add in the physical routes to an existing DEF file.  Reads the
// in_def_file and rewrites to out_def_file, where each net definition
// has the physical route appended.
//
// AUTHOR and DATE: steve beccue      Mon Aug 11 2003
//
bool
cLDDB::writeDefRoutes(const char *in_def_file, const char *out_def_file)
{
    if (!in_def_file || !out_def_file) {
        emitErrMesg("writeDefRoutes: Error, null file name encountered.\n");
        return (LD_BAD);
    }
    char line[LD_LINE_MAX + 1], *lptr = 0;
    char netname[LD_MAX_NAME_LEN];
    dbNet *net = 0;
    bool errcond = false;

    FILE *infp;
    if (!strcmp(in_def_file, "stdin"))
        infp = stdin;
    else
        infp = fopen(in_def_file, "r");
    if (!infp) {
        emitErrMesg(
            "writeDefRoutes: Error, cannot open DEF file %s for reading.\n",
            lstring::strip_path(in_def_file));
        return (LD_BAD);
    } 

    FILE *outfp;
    if (!strcmp(out_def_file, "stdout"))
        outfp = stdout;
    else
        outfp = fopen(out_def_file, "w");
    if (!outfp) {
        emitErrMesg(
            "writeDefRoutes: Error, couldn't open output (routed) DEF "
            "file %s.\n",
            lstring::strip_path(out_def_file));
        if (infp != stdin)
            fclose(infp);
        return (LD_BAD);
    }

    // Temporarily set the out resolution to the in resolution, will
    // revert on exit.
    int resol_bk = db_def_out_resol;
    db_def_out_resol = db_def_resol;

    // Copy DEF file up to NETS line.
    u_int numnets = 0;
    while (fgets(line, LD_LINE_MAX, infp) != 0) {
        lptr = line;
        while (isspace(*lptr))
            lptr++;
        if (!strncmp(lptr, "NETS", 4)) {
            sscanf(lptr + 4, "%u", &numnets);
            break;
        }
        fputs(line, outfp);
    }
    fputs(line, outfp);   // Write the NETS line.

    // NOTE:  May want to remove this message.  It may merely reflect
    // that the DEF file defined one or more SPECIALNETS.

    if (numnets != db_numNets) {
        flushMesg();
        emitErrMesg(
            "writeDefRoutes:  Warning, DEF file has %d nets, but we want "
            "to write %d.\n",
            numnets, db_numNets);
    }

    for (u_int i = 0; i < numnets; i++) {
        if (errcond)
            break;
        while (fgets(line, LD_LINE_MAX, infp) != 0) {
            if ((lptr = strchr(line, ';')) != 0) {
                *lptr = '\n';
                *(lptr + 1) = '\0';
                break;
            }
            else {
                lptr = line;
                while (isspace(*lptr))
                    lptr++;
                if (*lptr == '-') {
                    lptr++;
                    while (isspace(*lptr))
                        lptr++;
                    sscanf(lptr, "%s", netname);
                    fputs(line, outfp);
                }
                else if (*lptr == '+') {
                    lptr++;
                    while (isspace(*lptr))
                        lptr++;
                    if (!strncmp(lptr, "ROUTED", 6)) {
                        // This net is being handled by qrouter, so
                        // remove the original routing information.
                        while (fgets(line, LD_LINE_MAX, infp) != 0) {
                            if ((lptr = strchr(line, ';')) != 0) {
                                *lptr = '\n';
                                *(lptr + 1) = '\0';
                                break;
                            }
                        }
                        break;
                    }
                    else
                        fputs(line, outfp);
                }
                else if (!strncmp(lptr, "END", 3)) {
                    // This should not happen.
                    fputs(line, outfp);
                    errcond = true;
                    break;
                }
                else
                    fputs(line, outfp);
            }
        }

        // Find this net.

        net = getNet(netname);
        if (!net) {
            emitErrMesg("writeDefRoutes:  Warning, net %s cannot be found.\n",
                netname);

            // Dump rest of net and continue---no routing information.
            *(lptr) = ';';
            fputs(line, outfp);
            continue;
        }
        else {
            // Add last net terminal, without the semicolon.
            fputs(line, outfp);

            writeDefNetRoutes(outfp, net, false);
            fprintf(outfp, " ;\n");
        }
    }

    // Finish copying the rest of the NETS section.
    if (!errcond) {
        while (fgets(line, LD_LINE_MAX, infp) != 0) {
            lptr = line;
            while (isspace(*lptr))
                lptr++;
            fputs(line, outfp);
            if (!strncmp(lptr, "END", 3)) {
                break;
            }
        }
    }

    // Add the SPECIALNETS block for stubs, if any.
    writeDefStubs(outfp);

    // Finish copying the rest of the file.
    while (fgets(line, LD_LINE_MAX, infp) != 0) {
        fputs(line, outfp);
    }
    if (infp != stdin)
        fclose(infp);
    if (outfp != stdout)
        fclose(outfp);
    db_def_out_resol = resol_bk;
    return (LD_OK);
}


// writeDefNetRoutes
//
// Dump the DEF format for a complete net route to fp.  If "special"
// is true, then it looks only for stub routes between a grid point
// and an off-grid terminal, and dumps only the stub route geometry as
// a SPECIALNET, which takes a width parameter.  This allows the stub
// routes to be given the same width as a via, when the via is larger
// than a route width, to avoid DRC notch errors between the via and
// the terminal.
//
// If special is not true, all routes other than the stubs are output. 
// Originally, the stubs were routed too with the standard route
// width.  so that the SPECIALNETS were connectively redundant, this
// is no longer the case.
//
// The text requires " ;\n" termination.
//
bool
cLDDB::writeDefNetRoutes(FILE *fp, dbNet *net, bool special)
{
    if (!fp) {
        emitErrMesg("writeDefNetRoutes: Error, null file pointer.\n");
        return (LD_BAD);
    }
    if (!net) {
        emitErrMesg("writeDefNetRoutes: Error, null net pointer.\n");
        return (LD_BAD);
    }

    dbPath *p = special ? net->spath : net->path;
    if (!p)
        return (LD_OK);

    lefu_t lx = 0;
    lefu_t ly = 0;
    bool pset = false;
    bool first = true;
    for ( ; p; p = p->next) {
        if (p->layer >= 0) {
            if (first) {
                first = false;
                fprintf(fp, "+ ROUTED");
            }
            else
                fprintf(fp, "\n  NEW");
            lx = p->x;
            ly = p->y;
            fprintf(fp, " %s", layerName(p->layer));
            if (p->width > 0)
                fprintf(fp, " %d", lefToDef(p->width));
            fprintf(fp, " ( %d %d )", lefToDef(p->x), lefToDef(p->y));
            pset = true;
            if (p->vid >= 0) {
                fprintf(fp, " %s", db_lef_objects[p->vid]->lefName);
                pset = false;
            }
            continue;
        }
        if (pset) {
            if (p->x == lx) {
                if (p->y != ly)
                    fprintf(fp, " ( * %d )", lefToDef(p->y));
            }
            else if (p->y == ly)
                fprintf(fp, " ( %d * )", lefToDef(p->x));
            else {
                fprintf(fp, " ( %d %d )",
                    lefToDef(p->x), lefToDef(p->y));  // BAD
                emitErrMesg(
                    "Warning: non-Manhattan segment (%d,%d -- %d,%d),\n",
                    lx, ly, p->x, p->y);
            }
            lx = p->x;
            ly = p->y;
            if (p->vid >= 0) {
                fprintf(fp, " %s", db_lef_objects[p->vid]->lefName);
                pset = false;
            }
        }
    }
    return (LD_OK);
}


// Write SPECIALNETS block which contains stubs that need to be output
// with specified width.
//
bool
cLDDB::writeDefStubs(FILE *fp)
{
    // Determine how many stub routes we will write to SPECIALNETS.
//XXX problem here if file already has SPECIALNETS block

    int stubroutes = 0;
    for (u_int i = 0; i < db_numNets; i++) {
        if (db_nlNets[i]->flags & NET_STUB)
            stubroutes++;
    }

    // If there were stub routes, write them in SPECIALNETS at the
    // proper width.

    if (stubroutes > 0) {
        fprintf(fp, "\nSPECIALNETS %d ;\n", stubroutes);
        for (u_int i = 0; i < db_numNets; i++) {
            dbNet *net = db_nlNets[i];
            if (net->flags & NET_STUB) {
                fprintf(fp, "- %s\n", net->netname);
                writeDefNetRoutes(fp, net, true);
                fprintf(fp, " ;\n");
            }
        }
        fprintf(fp, "END SPECIALNETS\n");
    }    
    return (LD_OK);
}

