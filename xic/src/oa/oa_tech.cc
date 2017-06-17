
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
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
 $Id: oa_tech.cc,v 1.19 2017/03/14 01:26:50 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "pcell.h"
#include "dsp.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "fio.h"
#include "fio_cvt_base.h"
#include "fio_library.h"
#include "cd_hypertext.h"
#include "cd_types.h"
#include "cd_objects.h"
#include "cd_strmdata.h"
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "tech_cds_out.h"
#include "errorlog.h"
#include "tech.h"
#include "texttf.h"
#include "oa_if.h"
#include "pcell_params.h"
#include "oa.h"
#include "oa_via.h"
#include <algorithm>


// Attach the tech library techlibname to the libname library.  This
// will fail if libname has a local tech database.  The local database
// should be destroyed first.
//
bool
cOA::attach_tech(const char *libname, const char *techlibname)
{
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    if (!libname || !*libname || !techlibname || !*techlibname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }

    try {
        oaScalarName libName(oaNativeNS(), libname);
        oaLib *lib = oaLib::find(libName);
        if (!lib) {
            Errs()->add_error("Library %s not found.", libname);
            return (false);
        }
        oaScalarName techLibName(oaNativeNS(), techlibname);
        if (!oaLib::find(techLibName)) {
            Errs()->add_error("Library %s not found.", techlibname);
            return (false);
        }
        oaTech::attach(lib, techLibName);
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


bool
cOA::destroy_tech(const char *libname, bool unattach_only)
{
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }

    try {
        oaScalarName libName(oaNativeNS(), libname);
        oaLib *lib = oaLib::find(libName);
        if (!lib) {
            Errs()->add_error("Library %s not found.", libname);
            return (false);
        }
        if (oaTech::hasAttachment(lib)) {
            oaTech::detach(lib);
            return (true);
        }
        if (unattach_only)
            return (true);
        if (oaTech::exists(lib, false)) {
            oaTech *tech = oaTech::find(lib);
            if (tech)
                tech->purge();
            oaTech::destroy(lib);
        }
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


bool
cOA::has_attached_tech(const char *libname, char **attached_lib_name)
{
    if (attached_lib_name)
        *attached_lib_name = 0;
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }

    try {
        oaScalarName libName(oaNativeNS(), libname);
        oaLib *lib = oaLib::find(libName);
        if (!lib) {
            Errs()->add_error("Library %s not found.", libname);
            return (false);
        }
        if (oaTech::hasAttachment(lib)) {
            oaScalarName attachLibName;
            oaTech::getAttachment(lib, attachLibName);
            oaString attachlibname;
            attachLibName.get(attachlibname);
            if (attached_lib_name)
                *attached_lib_name = lstring::copy(attachlibname);
        }
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


bool
cOA::has_local_tech(const char *libname, bool *hastech)
{
    if (!hastech)
        return (false);
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }

    try {
        oaScalarName libName(oaNativeNS(), libname);
        oaLib *lib = oaLib::find(libName);
        if (!lib) {
            Errs()->add_error("Library %s not found.", libname);
            return (false);
        }
        if (!oaTech::hasAttachment(lib)) {
            if (oaTech::exists(lib, false))
                *hastech = true;
        }
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


bool
cOA::create_local_tech(const char *libname)
{
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (false);
    }

    try {
        oaScalarName libName(oaNativeNS(), libname);
        oaLib *lib = oaLib::find(libName);
        if (!lib) {
            Errs()->add_error("Library %s not found.", libname);
            return (false);
        }
        if (oaTech::hasAttachment(lib))
            return (true);
        if (!oaTech::exists(lib, false))
            oaTech::create(lib);
        return (true);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
}


//
// A utility class for working with the OpenAcces technology database.
//

// Collections available from oaTech.
//
#define OATECH_PR_UNITS         0x1
#define OATECH_PR_ANLIBS        0x2
#define OATECH_PR_LAYERS        0x4
#define OATECH_PR_OPPTS         0x8
#define OATECH_PR_PURPS         0x10
#define OATECH_PR_STDEFS        0x20
#define OATECH_PR_VALUES        0x40
#define OATECH_PR_VIADEFS       0x80
#define OATECH_PR_VIASPECS      0x100
#define OATECH_PR_VIAVARS       0x200
#define OATECH_PR_CNSTGRPS      0x400
#define OATECH_PR_CNSTPRMS      0x800
#define OATECH_PR_DRVLPRMS      0x1000
#define OATECH_PR_APPOBJDEFS    0x2000
#define OATECH_PR_GROUPS        0x4000
 

class cOaTechIf
{
public:
    struct techLPP
    {
        techLPP()
            {
                layer = 0;
                purpose = 0;
                lnum = -1;
                pnum = -1;
                flags = 0;
            }
        oaLayer *layer;
        oaPurpose *purpose;
        int lnum;
        int pnum;
        int flags;
        oaString packetName;
    };

    cOaTechIf(oaTech *tech, FILE *fp)
        {
            if_tech = tech;
            if_fp = fp;
        }

    void printCdsTech();
    void printCdsControls();
    void printCdsLayerDefinitions();
    void printCdsLayerRules();
    void printCdsViaDefs();
    void printCdsConstraintGroups();
    void printCdsConstraintGroup(const oaConstraintGroup*);
    void printCdsConstraintGroupMem(const oaConstraintGroupMem*, const char*,
        int);
    void printCdsAntennaMods(const oaConstraintGroupMem**, int);
    void printCdsDevices();
    void printCdsMppRec(const oaGroup*);
    void printCdsViaSpecs();

    void printTech(const char*, const char*);
    void printViaDef(const oaViaDef*);
    void printGroup(const oaGroup*);
    void printConstraintGroup(const oaConstraintGroup*);
    void printProperties(const oaObject*, int);
    template <class T>void printAppDef(const T*);

private:
    bool getCdsLpp(techLPP&, const oaAppObject*);
    void getLppNames(const techLPP&, oaString&, oaString&, bool* = 0);
    void getConstraintGroupMemName(const oaConstraintGroupMem*, oaString&);
    bool hasAntennaMod(const oaConstraintGroupMem*, int);
    bool hasTable(const oaConstraintGroupMem*);
    char *getConstraintParamString(const oaConstraint*);
    char *getValueAsString(const oaValue*, int = 0);

    char *getAntennaRatioArrayValueAsString(const oaAntennaRatioArrayValue*,
        int);
    char *getAntennaRatioValueAsString(const oaAntennaRatioValue*);
    char *getBooleanValueAsString(const oaBooleanValue*);
    char *getBoxArrayValueAsString(const oaBoxArrayValue*);
    char *getDualInt1DTblValueAsString(const oaDualInt1DTblValue*);
    char *getDualIntValueAsString(const oaDualIntValue*, int);
    char *getFlt1DTblValueAsString(const oaFlt1DTblValue*);
    char *getFlt2DTblValueAsString(const oaFlt2DTblValue*);
    char *getFltIntFltTblValueAsString(const oaFltIntFltTblValue*);
    char *getFltValueAsString(const oaFltValue*);
    char *getInt1DTblValueAsString(const oaInt1DTblValue*, int);
    char *getInt2DTblValueAsString(const oaInt2DTblValue*, int);
    char *getIntDualIntArrayTblValueAsString(const oaIntDualIntArrayTblValue*);
    char *getIntFltTblValueAsString(const oaIntFltTblValue*, int);
    char *getIntRangeArray1DTblValueAsString(const oaIntRangeArray1DTblValue*);
    char *getIntRangeArray2DTblValueAsString(const oaIntRangeArray2DTblValue*);
    char *getIntRangeArrayValueAsString(const oaIntRangeArrayValue*);
    char *getIntRangeValueAsString(const oaIntRangeValue*);
    char *getIntValueAsString(const oaIntValue*, int);
    char *getLayerArrayValueAsString(const oaLayerArrayValue*);
    char *getLayerValueAsString(const oaLayerValue*);
    char *getPurposeValueAsString(const oaPurposeValue*);
    char *getUInt8RangeValueAsString(const oaUInt8RangeValue*);
    char *getUInt8ValueAsString(const oaUInt8Value*);
    char *getValueArrayValueAsString(const oaValueArrayValue*, int);
    char *getViaDef2DTblValueAsString(const oaViaDef2DTblValue*);
    char *getViaDefArrayValueAsString(const oaViaDefArrayValue*);
    char *getViaTopology2DTblValueAsString(const oaViaTopology2DTblValue*);
    char *getViaTopologyArrayValueAsString(const oaViaTopologyArrayValue*);

    int parse_token(const char*);

    double dbuToUU(int dbu)
        {
            return (if_tech->dbuToUU(oaViewType::get(oacMaskLayout), dbu));
        }

    const char *virtReservedLayer(int l)
        {
            if (l >= 200 && l <= 255)
                return (vreslyr[l - 200]);
            return (0);
        }

    const char *virtReservedPurpose(int p)
        {
            if (p >= 223 && p <= 255)
                return (vresprp[p - 223]);
            return (0);
        }

    oaTech  *if_tech;
    FILE    *if_fp;

    static const char *vreslyr[];
    static const char *vresprp[];
};

// The Virtuoso reserved layer and purpose names are listed in the
// tech.db string table, but I can't seem to find where any tables are
// stored.  There are no oaLayer or oaPurpose objects for these in the
// database, so we need to hack our own table to get layer names from
// numbers, which are given in the LPPs.

const char *cOaTechIf::vreslyr[] = {
    "Unrouted",         // 200
    "Row",              // 201
    "Group",            // 202
    "Cannotoccupy",     // 203
    "Canplace",         // 204
    "hardFence",        // 205
    "softFence",        // 206
    "y0",               // 207
    "y1",               // 208
    "y2",               // 209
    "y3",               // 210
    "y4",               // 211
    "y5",               // 212
    "y6",               // 213
    "y7",               // 214
    "y8",               // 215
    "y9",               // 216
    "designFlow",       // 217
    "stretch",          // 218
    "edgeLayer",        // 219
    "changedLayer",     // 220
    "unset",            // 221
    "unknown",          // 222
    "spike",            // 223
    "hiz",              // 224
    "resist",           // 225
    "drive",            // 226
    "supply",           // 227
    "wire",             // 228
    "pin",              // 229
    "text",             // 230
    "device",           // 231
    "border",           // 232
    "snap",             // 233
    "align",            // 234
    "prBoundary",       // 235
    "instance",         // 236
    "annotate",         // 237
    "marker",           // 238
    "select",           // 239
    "winActiveBanner",  // 240
    "winAttentionText", // 241
    "winBackground",    // 242
    "winBorder",        // 243
    "winBottomShadow",  // 244
    "winButton",        // 245
    "winError",         // 246
    "winForeground",    // 247
    "winInactiveBanner",// 248
    "winText",          // 249
    "winTopShadow",     // 250
    "grid",             // 251
    "axis",             // 252
    "hilite",           // 253
    "background"        // 254
};

const char *cOaTechIf::vresprp[] = {
    "fatal",            // 223
    "critical",         // 224
    "soCritical",       // 225
    "soError",          // 226
    "ackWarn",          // 227
    "info",             // 228
    "track",            // 229
    "blockage",         // 230
    "grid",             // 231
    0,                  // 232
    0,                  // 233

    // Legacy Virtuoso
    "warning",          // 234
    "tool1",            // 235
    "tool0",            // 236
    "label",            // 237
    "flight",           // 238
    "error",            // 239
    "annotate",         // 240
    "drawing1",         // 241
    "drawing2",         // 242
    "drawing3",         // 243
    "drawing4",         // 244
    "drawing5",         // 245
    "drawing6",         // 246
    "drawing7",         // 247
    "drawing8",         // 248
    "drawing9",         // 249
    "boundary",         // 250
    "pin",              // 251
    "drawing",          // 252
    "net",              // 253
    "cell",             // 254
    "all"               // 255
};


// A NO-OP for now.
bool
cOA::save_tech()
{
    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
        return (false);
    }
    return (true);
}


bool
cOA::print_tech(FILE *fp, const char *libname, const char *which,
    const char *prname)
{
    if (!fp) {
        Errs()->add_error("print_tech:  Null file pointer.");
        return (false);
    }
    if (!initialize()) {
        Errs()->add_error("print_tech:  OpenAccess initialization failed.");
        return (false);
    }

    oaNativeNS ns;
    oaScalarName libName(ns, libname);
    oaLib *lib = oaLib::find(libName);
    if (!lib) {
        Errs()->add_error("Library %s was not found in lib.defs.", libname);
        return (false);
    }
    oaTech *tech = oaTech::find(libName);
    if (!tech)
        tech = oaTech::open(libName, 'r');
    if (!tech) {
        Errs()->add_error("No tech database found in library %s.", libname);
        return (false);
    }

    cOaTechIf tif(tech, fp);
    if (!which || !*which)
        tif.printCdsTech();
    else
        tif.printTech(which, prname);
    return (true);
}


// Print an ASCII Cadence Virtuoso technology file.  This was created
// by reverse engineering the TSMC N65 PDK use of OpenAccess,
// comparing with their ASCII technology file.  This capability is
// most certainly not complete, and contains a few guesses about how
// things work where there was no sample.  How Virtuoso uses the
// OpenAccess database does not seem to be documented an any detail
// that I can find.
//
void
cOaTechIf::printCdsTech()
{
    if (!if_fp || !if_tech)
        return;

    fprintf(if_fp, "; Generated by %s\n", XM()->IdString());
    oaLib *lib = if_tech->getLib();
    oaString libpath;
    lib->getFullPath(libpath);
    fprintf(if_fp, "; Cadence Virtuoso-style technology file from OpenAccess "
        "library\n; %s\n\n", (const char*)libpath);

    const char *line = ";********************************\n";
    fputs(line, if_fp);
    fputs("; CONTROLS\n", if_fp);
    fputs(line, if_fp);
    printCdsControls();

    fputs("\n\n", if_fp);
    fputs(line, if_fp);
    fputs("; LAYER DEFINITION\n", if_fp);
    fputs(line, if_fp);
    printCdsLayerDefinitions();

    fputs("\n\n", if_fp);
    fputs(line, if_fp);
    fputs("; LAYER RULES\n", if_fp);
    fputs(line, if_fp);
    printCdsLayerRules();

    fputs("\n\n", if_fp);
    fputs(line, if_fp);
    fputs("; VIADEFS\n", if_fp);
    fputs(line, if_fp);
    printCdsViaDefs();

    fputs("\n\n", if_fp);
    fputs(line, if_fp);
    fputs("; CONSTRAINT GROUPS\n", if_fp);
    fputs(line, if_fp);
    printCdsConstraintGroups();

    fputs("\n\n", if_fp);
    fputs(line, if_fp);
    fputs("; DEVICES\n", if_fp);
    fputs(line, if_fp);
    printCdsDevices();

    fputs("\n\n", if_fp);
    fputs(line, if_fp);
    fputs("; VIASPECS\n", if_fp);
    fputs(line, if_fp);
    printCdsViaSpecs();

    fputc('\n', if_fp);
    fputs(line, if_fp);
    fputs("; END\n", if_fp);
}


void
cOaTechIf::printCdsControls()
{
    fputs("controls(\n", if_fp);
    fputs(" techParams(\n", if_fp);
    fputs(" ;( parameter           value )\n", if_fp);
    fputs(" ;( ---------           ----- )\n", if_fp);

    oaString gname("techParams");
    oaIter<oaGroup> iterGroup(if_tech->getGroupsByName(gname));
    oaGroup *group;
    while ((group = iterGroup.getNext()) != 0) {
        if (group->isEmpty())
            continue;

        oaIter<oaGroupMember> iterGM(group->getMembers());
        oaGroupMember *member;
        while ((member = iterGM.getNext()) != 0) {

            oaString name;
            switch (member->getObject()->getType()) {
            case oacIntPropType:
                {
                    oaIntProp *prp = (oaIntProp*)member->getObject();
                    prp->getName(name);
                    fprintf(if_fp, "  ( %-19s %d )\n", (const char*)name,
                        prp->getValue());
                }
                break;
            case oacFloatPropType:
                {
                    oaFloatProp *prp = (oaFloatProp*)member->getObject();
                    prp->getName(name);
                    fprintf(if_fp, "  ( %-19s %g )\n", (const char*)name,
                        prp->getValue());
                }
                break;
            case oacStringPropType:
                {
                    oaStringProp *prp = (oaStringProp*)member->getObject();
                    prp->getName(name);
                    oaString val;
                    prp->getValue(val);
                    fprintf(if_fp, "  ( %-19s \"%s\" )\n", (const char*)name,
                        (const char*)val);
                }
                break;
            case oacDoublePropType:
                {
                    oaDoubleProp *prp = (oaDoubleProp*)member->getObject();
                    prp->getName(name);
                    fprintf(if_fp, "  ( %-19s %g )\n", (const char*)name,
                        prp->getValue());
                }
                break;
            case oacAppPropType:
                {
                    oaAppProp *ap = (oaAppProp*)member->getObject();
                    ap->getName(name);
                    fprintf(if_fp, "  ( %-19s ", (const char*)name);
                    oaByteArray ary;
                    ap->getValue(ary);

                    for (unsigned int i = 0; i < ap->getSize(); i++) {
                        if (ary.get(i))
                            fputc(ary.get(i), if_fp);
                    }
                    fputs(" )\n", if_fp);
                }
                break;
            case oacAppObjectType:
                {
                    oaAppObject *ao = (oaAppObject*)member->getObject();
                    oaAppObjectDef *od = ao->getAppObjectDef();
                    od->getName(name);
                    fprintf(if_fp, "  ( %-19s )\n", (const char*)name);
                }
                break;
            case oacPhysicalLayerType:
                {
                    oaPhysicalLayer *lyr =
                        (oaPhysicalLayer*)member->getObject();
                    lyr->getName(name);
                    fprintf(if_fp, "  ( %s )\n", (const char*)name);
                }
                break;
            case oacDerivedLayerType:
                {
                    oaDerivedLayer *dl = (oaDerivedLayer*)member->getObject();
                    oaString l1name, l2name;
                    dl->getLayer1()->getName(l1name);
                    dl->getLayer2()->getName(l2name);
                    oaLayerOp op = dl->getOperation();
                    oaString opstr = op.getName();
                    fprintf(if_fp, "  ( %s %s %s )\n", (const char*)l1name,
                        (const char*)opstr, (const char*)l2name);
                }
                break;
            case oacGroupType:
                {
                    oaGroup *mgrp = (oaGroup*)member->getObject();
                    if (mgrp == group)
                    continue;
                    mgrp->getName(name);
                    fprintf(if_fp, "  ( %-19s )\n", (const char*)name);
                }
                break;
            default:
                {
                    name = member->getObject()->getType().getName();
                    fprintf(if_fp, "  ( %s (wtf) )\n", (const char*)name);
                }
                break;
            }
        }
    }
    fputs(" ) ;techParams\n\n", if_fp);

    fputs(" viewTypeUnits(\n", if_fp);
    fputs(" ;( viewType            userUnit       dbuperuu )\n", if_fp);
    fputs(" ;( --------            --------       -------- )\n", if_fp);

    // Can't seem to simply look through the collection from
    // oaViewType, since non-reserved types throw an exception, and
    // there seems to be no easy way to tell if the oaViewType is
    // reserved.

    for (int i = 0; i < 4; i++) {
        oaString name, type;
        oaViewType *vt;
        if (i == 0)
            vt = oaViewType::get(oaReservedViewType(oacMaskLayout));
        else if (i == 1)
            vt = oaViewType::get(oaReservedViewType(oacSchematic));
        else if (i == 2)
            vt = oaViewType::get(oaReservedViewType(oacSchematicSymbol));
        else
            vt = oaViewType::get(oaReservedViewType(oacNetlist));

        if (if_tech->isDBUPerUUSet(vt) && if_tech->isUserUnitsSet(vt)) {
            vt->getName(name);
            oaString type = if_tech->getUserUnits(vt).getName();

            fprintf(if_fp, "  ( %-19s %-14s %d )\n", (const char*)name,
                (const char*)type, if_tech->getDBUPerUU(vt));
        }
    }
    fputs(" ) ;viewTypeUnits\n\n", if_fp);

    fputs(" mfgGridResolution(\n", if_fp);
    int dist = if_tech->getDefaultManufacturingGrid();
    int sc = if_tech->getDBUPerUU(
        oaViewType::get(oaReservedViewType(oacMaskLayout)));

    fprintf(if_fp, " ( %.6f )\n", (double)dist/sc);
    fputs(" ) ;mfgGridResolution\n", if_fp);
    fputs(") ;controls\n", if_fp);
}


void
cOaTechIf::printCdsLayerDefinitions()
{
    fputs("layerDefinitions(\n", if_fp);
    fputs(" techPurposes(\n", if_fp);
    fputs(" ;( PurposeName         Purpose#   Abbreviation )\n", if_fp);
    fputs(" ;( -----------         --------   ------------ )\n", if_fp);
    {
        // Purposes list.
        oaIter<oaPurpose> iterPurpose(if_tech->getPurposes());
        oaPurpose *purpose;
        while ((purpose = iterPurpose.getNext()) != 0) {
            oaString name;
            purpose->getName(name);
            if (!purpose->isReserved()) {

                oaString abbrev(name);
                if (purpose->hasAppDef()) {
                    oaIter<oaAppDef> iterAppDef(purpose->getAppDefs());
                    oaAppDef *adef;
                    while ((adef = iterAppDef.getNext()) != 0) {
                        oaString aname;
                        adef->getName(aname);
                        if (aname == "purposeAbbreviation") {
                            ((oaStringAppDef<oaPurpose>*)adef)->get(purpose,
                                abbrev);
                            break;
                        }
                    }
                }
                fprintf(if_fp, "  ( %-19s %-10d %-3s )\n", (const char*)name,
                    purpose->getNumber(), (const char*)abbrev);
            }
        }
    }
    fputs(" ) ;techPurposes\n\n", if_fp);

    fputs(" techLayers(\n", if_fp);
    fputs(" ;( LayerName           Layer#     Abbreviation )\n", if_fp);
    fputs(" ;( ---------           ------     ------------ )\n", if_fp);
    {
        // Layers list.
        oaIter<oaLayer> iterLayer(if_tech->getLayers());
        oaLayer *layer;
        while ((layer = iterLayer.getNext()) != 0) {
            oaString name;
            layer->getName(name);
            if (layer->getType() == oacPhysicalLayerType) {

                oaString abbrev(name);
                if (layer->hasAppDef()) {
                    oaIter<oaAppDef> iterAppDef(layer->getAppDefs());
                    oaAppDef *adef;
                    while ((adef = iterAppDef.getNext()) != 0) {
                        oaString aname;
                        adef->getName(aname);
                        if (aname == "layerAbbreviation") {
                            ((oaStringAppDef<oaLayer>*)adef)->get(layer,
                                abbrev);
                            break;
                        }
                    }
                }
                fprintf(if_fp, "  ( %-19s %-10d %-3s )\n", (const char*)name,
                    layer->getNumber(), (const char*)abbrev);
            }
        }
    }
    fputs(" ) ;techLayers\n\n", if_fp);

    oaAppObjectDef *lppodef;
    {
        // Find an oaAppObjectDef named "techLPP" in the oaTech.  The
        // techLPP objects are what we want to list, already in order.

        oaString techLPP("techLPP");
        oaIter<oaAppObjectDef> iterObjectDef(if_tech->getAppObjectDefs());
        while ((lppodef = iterObjectDef.getNext()) != 0) {
            oaString name;
            lppodef->getName(name);
            if (name == techLPP)
                break;
        }
    }
    if (!lppodef) {
        // This isn't good, the oaTech was not set up for Virtuoso, so our
        // output will be missing LPP data.

        fputs(
            "; WARNING: The LPP data are not found, the technology database\n"
            "; is not set up for Virtuoso.  LPP data will be missing.\n",
            if_fp);
    }

    fputs(" techLayerPurposePriorities(\n", if_fp);
    fputs(" ;( LayerName           Purpose )\n", if_fp);
    fputs(" ;( ---------           ------- )\n", if_fp);
    if (lppodef) {
        // Layer purpose priority list.

        oaIter<oaAppObject> iterAppObject(if_tech->getAppObjects(lppodef));
        oaAppObject *lppobj;
        while ((lppobj = iterAppObject.getNext()) != 0) {

            oaIter<oaAppDef> iterAppDef(lppobj->getAppDefs());
            techLPP lpp;
            if (getCdsLpp(lpp, lppobj)) {
                oaString lname, pname;
                getLppNames(lpp, lname, pname);

                fprintf(if_fp, "  ( %-19s %s )\n", (const char*)lname,
                    (const char*)pname);
            }
        }
    }
    else
        fputs(" ; LPP data not available.\n", if_fp);
    fputs(" ) ;techLayerPurposePriorities\n\n", if_fp);

#define lppFlgCon2C 0x2
#define lppFlgOEnbl 0x4
#define lppFlgSel   0x8
#define lppFlgValid 0x10
#define lppFlgVis   0x20

    fputs(" techDisplays(\n", if_fp);
    fputs(" ;( LayerName    Purpose      Packet          Vis Sel Con2ChgLy "
        "DrgEnbl Valid )\n", if_fp);
    fputs(" ;( ---------    -------      ------          --- --- --------- "
        "------- ----- )\n", if_fp);
    if (lppodef) {
        // Tech displays list.

        // Run the techLPP list again, printing the display stuff.

        oaIter<oaAppObject> iterAppObject(if_tech->getAppObjects(lppodef));
        oaAppObject *lppobj;
        while ((lppobj = iterAppObject.getNext()) != 0) {

            techLPP lpp;
            if (getCdsLpp(lpp, lppobj)) {

                bool fixme;
                oaString lname, pname;
                getLppNames(lpp, lname, pname, &fixme);

                // The packetname seems to be wrong in some cases, e.g.
                // using "drawing" instead of the real purpose name, for
                // reserved values only.

                char buf[256];
                if (fixme)
                    sprintf(buf, "%s_%s", (const char*)lname, (const char*)pname);
                else
                    sprintf(buf, "%s", (const char*)lpp.packetName);

                fprintf(if_fp, "  ( %-12s %-12s %-20s", (const char*)lname,
                    (const char*)pname, buf);

                fprintf(if_fp, " %s", (lpp.flags & lppFlgVis) ? "t" : "nil");
                fprintf(if_fp, " %s", (lpp.flags & lppFlgSel) ? "t" : "nil");
                fprintf(if_fp, " %s", (lpp.flags & lppFlgCon2C) ? "t" : "nil");
                fprintf(if_fp, " %s", (lpp.flags & lppFlgOEnbl) ? "t" : "nil");
                fprintf(if_fp, " %s", (lpp.flags & lppFlgValid) ? "t" : "nil");
                fputs(" )\n", if_fp);
            }
        }
    }
    else
        fputs(" ; LPP data not available.\n", if_fp);
    fputs(" ) ;techDisplays\n\n", if_fp);

    fputs(" techLayerProperties(\n", if_fp);
    fputs(" ;( PropName            Layer1 [ Layer2 ]   PropValue )\n", if_fp);
    fputs(" ;( --------            ------ ----------   --------- )\n", if_fp);
    {
        // Layer properties list.

        oaIter<oaLayer> iterLayer(if_tech->getLayers());
        oaLayer *layer;
        while ((layer = iterLayer.getNext()) != 0) {
            oaString name;
            layer->getName(name);
            oaIter<oaProp> iterProp(layer->getProps());
            oaProp *prop;
            while ((prop = iterProp.getNext()) != 0) {

                // What to do with these?
                if (prop->getType() == oacHierPropType)
                    continue;

                oaString pname, value;
                prop->getName(pname);
                prop->getValue(value);
                if (!strcasecmp(pname, "resistance")) {
                    fprintf(if_fp,  "  ( %-19s %-19s %s )\n",
                        "sheetResistance", (const char *)name,
                        (const char*)value);
                }
                else if (!strcasecmp(pname, "capacitance")) {
                    fprintf(if_fp,  "  ( %-19s %-19s %s )\n",
                        "areaCapacitance", (const char *)name,
                        (const char*)value);
                }
                else if (!strcasecmp(pname, "edgecapacitance")) {
                    fprintf(if_fp,  "  ( %-19s %-19s %s )\n",
                        "edgeCapacitance", (const char *)name,
                        (const char*)value);
                }
                else if (!strcasecmp(pname, "thickness")) {
                    // This one needs conversion.
                    int t = ((oaIntProp*)prop)->getValue();
                    fprintf(if_fp,  "  ( %-19s %-19s %g )\n",
                        "thickness", (const char *)name, dbuToUU(t));
                }
                else {
                    fprintf(if_fp,  "  ( %-19s %-19s %s )\n",
                        (const char*)pname, (const char *)name,
                        (const char*)value);
                }
            }
        }
    }
    fputs(" ) ;techLayerProperties\n\n", if_fp);

    fputs(" techDerivedLayers(\n", if_fp);
    fputs(" ;( DerivedLayerName    #        composition )\n", if_fp);
    fputs(" ;( ----------------    -----    ----------- )\n", if_fp);
    {
        // Derived layers list.
        oaIter<oaLayer> iterLayer(if_tech->getLayers());
        oaLayer *layer;
        while ((layer = iterLayer.getNext()) != 0) {
            if (layer->getType() == oacDerivedLayerType) {
                oaDerivedLayer *dl = (oaDerivedLayer*)layer;

                oaString name;
                dl->getName(name);
                fprintf(if_fp, "  ( %-19s %-8d", (const char*)name,
                    dl->getNumber());

                oaLayer *l1 = dl->getLayer1();
                oaLayer *l2 = dl->getLayer2();
                oaLayerOp op = dl->getOperation();
                const char *otok = 0;
                switch (oaLayerOpEnum(op)) {
                case oacAndLayerOp:
                    otok = "'and";
                    break;
                case oacOrLayerOp:
                    otok = "\'or";
                    break;
                case oacNotLayerOp:
                    otok = "\'not";
                    break;
                case oacXorLayerOp:
                    otok = "\'xor";
                    break;
                case oacTouchingLayerOp:
                case oacButtOnlyLayerOp:
                case oacUserDefinedLayerOp:
                case oacInsideLayerOp:
                case oacOutsideLayerOp:
                case oacOverlappingLayerOp:
                case oacStraddlingLayerOp:
                case oacAvoidingLayerOp:
                case oacButtingLayerOp:
                case oacCoincidentLayerOp:
                case oacCoincidentOnlyLayerOp:
                case oacEnclosingLayerOp:
                case oacButtingOrCoincidentLayerOp:
                case oacButtingOrOverlappingLayerOp:
                case oacAreaLayerOp:
                case oacGrowLayerOp:
                case oacShrinkLayerOp:
                case oacGrowVerticalLayerOp:
                case oacGrowHorizontalLayerOp:
                case oacShrinkVerticalLayerOp:
                case oacShrinkHorizontalLayerOp:
                case oacSelectLayerOp:
                    otok = "\'unhandled";
                    break;
                }
                oaString lnm1, lnm2;
                if (l1)
                    l1->getName(lnm1);
                else {
                    const char *t = virtReservedLayer(dl->getLayer1Num());
                    if (t)
                        lnm1 = t;
                    else
                        lnm1.format("??? (%d)", dl->getLayer1Num());
                }
                if (l2)
                    l2->getName(lnm2);
                else {
                    const char *t = virtReservedLayer(dl->getLayer2Num());
                    if (t)
                        lnm2 = t;
                    else
                        lnm2.format("??? (%d)", dl->getLayer2Num());
                }
                l2->getName(lnm2);
                fprintf(if_fp, " ( %-10s %-7s %s ) )\n", (const char*)lnm1,
                    otok, (const char*)lnm2);
            }
        }
    }
    fputs(" ) ;techDerivedLayers\n", if_fp);
    fputs(") ;layerDefinitions\n", if_fp);
}


namespace {
    bool mncomp(const oaPhysicalLayer *l1, const oaPhysicalLayer *l2)
    {
        return (l1->getMaskNumber() <= l2->getMaskNumber());
    }
}


void
cOaTechIf::printCdsLayerRules()
{
    char buf[256];
    fputs("layerRules(\n", if_fp);
    fputs(" functions(\n", if_fp);
    fputs(" ;( layer                       function       [maskNumber] )\n",
        if_fp);
    fputs(" ;( -----                       --------       ------------ )\n",
        if_fp);
    {
        // Layer list, we need to save a list of layers, to sort by
        // mask number.
        struct llst
        {
            llst(const oaPhysicalLayer *l, llst *n)
                {
                    next = n;
                    layer = l;
                }

            llst *next;
            const oaPhysicalLayer *layer;
        };

        llst *layers = 0;
        oaIter<oaLayer> iterLayer(if_tech->getLayers());
        oaLayer *layer;
        while ((layer = iterLayer.getNext()) != 0) {
            if (layer->getType() == oacPhysicalLayerType) {
                oaPhysicalLayer *lyr = (oaPhysicalLayer*)layer;
                oaMaterial fcn = lyr->getMaterial();
                if (fcn == oacOtherMaterial)
                    continue;
                layers = new llst(lyr, layers);
            }
        }
        int num = 0;
        for (llst *l = layers; l; l = l->next)
            num++;
        if (num > 1) {
            const oaPhysicalLayer **ary = new const oaPhysicalLayer*[num];
            num = 0;
            for (llst *l = layers; l; l = l->next)
                ary[num++] = l->layer;
            std::sort(ary, ary + num, mncomp);
            num = 0;
            for (llst *l = layers; l; l = l->next)
                l->layer = ary[num++];
            delete [] ary;
        }
        llst *lnxt;
        for (llst *l = layers; l; l = lnxt) {
            lnxt = l->next;
            oaMaterial fcn = l->layer->getMaterial();
            oaString mname = fcn.getName();
            sprintf(buf, "\"%s\"", (const char*)mname);
            oaString lname;
            l->layer->getName(lname);
            fprintf(if_fp, "  ( %-27s %-14s %-4d )\n", (const char*)lname,
                buf, l->layer->getMaskNumber());
            delete l;
        }
    }
    fputs(" ) ;functions\n\n", if_fp);

    // Look for a group stampLabelLayers/stampLabelLayer, which we use
    // in the next two sections.

    oaGroup *stampGroup = 0;
    {
        oaIter<oaGroup> iterGroup(if_tech->getGroupsByName("stampLabelLayers"));
        oaGroup *group = iterGroup.getNext();
        if (group) {
            oaIter<oaGroupMember> iterGroupMember(group->getMembers());
            oaGroupMember *member;
            while ((member = iterGroupMember.getNext()) != 0) {
                oaObject *obj = member->getObject();
                if (obj->getType() == oacGroupType) {
                    oaGroup *g = (oaGroup*)obj;
                    oaString gname;
                    g->getName(gname);
                    if (gname == "stampLabelLayer") {
                        stampGroup = g;
                        break;
                    }
                }
            }
        }
    }

    fputs(" routingDirections(\n", if_fp);
    fputs(" ;( layer                       direction      )\n", if_fp);
    fputs(" ;( -----                       ---------      )\n", if_fp);
    if (stampGroup) {
        oaIter<oaGroupMember> iterGroupMember(stampGroup->getMembers());
        oaGroupMember *member;
        while ((member = iterGroupMember.getNext()) != 0) {
            oaObject *obj = member->getObject();

            // The stamp layer list seems to be the correct list for
            // the routing directions, i.e., same layers ane order in
            // my sample..

            if (obj->getType() == oacPhysicalLayerType) {
                oaPhysicalLayer *lyr = (oaPhysicalLayer*)obj;
                oaString lname;
                lyr->getName(lname);
                oaPrefRoutingDir dir = lyr->getPrefRoutingDir();
                oaString dname = dir.getName();
                sprintf(buf, "\"%s\"", (const char*)dname);
                fprintf(if_fp, "  ( %-27s %-14s )\n", (const char*)lname,
                    buf);
            }
        }
    }
    fputs(" ) ;routingDirections\n\n", if_fp);

    fputs(" stampLabelLayers(\n", if_fp);
    fputs(" ;( textLayer   layers        )\n", if_fp);
    fputs(" ;( ---------   -----------   )\n", if_fp);
    if (stampGroup) {
        sLstr lstr;
        lstr.add("  (");
        oaIter<oaGroupMember> iterGroupMember(stampGroup->getMembers());
        oaGroupMember *member;
        while ((member = iterGroupMember.getNext()) != 0) {
            oaObject *obj = member->getObject();

            if (obj->getType() == oacAppObjectType) {
                oaAppObject *ao = (oaAppObject*)obj;
                oaIter<oaAppDef> iterAppDef(ao->getAppDefs());
                oaAppDef *adef;
                const char *layer = 0, *purpose = 0;
                while ((adef = iterAppDef.getNext()) != 0) {
                    oaString aname;
                    adef->getName(aname);
                    if (aname == "techLayerRuleLayerNumbersDef") {
                        int i = ((oaIntAppDef<oaAppObject>*)adef)->get(ao);
                        layer = virtReservedLayer(i);
                    }
                    else if (aname == "techLayerRulePurposeNumbersDef") {
                        int i = ((oaIntAppDef<oaAppObject>*)adef)->get(ao);
                        if (i != 255)
                            purpose = virtReservedPurpose(i);
                    }
                }
                if (layer) {
                    lstr.add_c(' ');
                    if (purpose) {
                        lstr.add_c('(');
                        lstr.add(layer);
                        lstr.add_c(' ');
                        lstr.add(purpose);
                        lstr.add_c(')');
                    }
                    else
                        lstr.add(layer);
                }
            }
            else if (obj->getType() == oacPhysicalLayerType) {
                oaPhysicalLayer *lyr = (oaPhysicalLayer*)obj;
                oaString lname;
                lyr->getName(lname);
                lstr.add_c(' ');
                lstr.add((const char*)lname);
            }
        }
        lstr.add(" )");
        fprintf(if_fp, "%s\n", lstr.string());
    }
    fputs(" ) ;stampLabelLayers\n\n", if_fp);

    fputs(" currentDensityTables(\n", if_fp);
    fputs(" ;( rule                	layer1\n", if_fp);
    fputs(" ;  (( index1Definitions	[index2Definitions]) [defaultValue] )\n",
        if_fp);
    fputs(" ;  (table))\n", if_fp);
    fputs(" ;(----------------------------------------------------------)\n",
        if_fp);

    {
        oaIter<oaGroup> iterGroup(if_tech->getGroups());
        oaGroup *group;
        while ((group = iterGroup.getNext()) != 0) {
            oaString name;
            group->getName(name);
            if (name == "peakACCurrentDensity" ||
                    name == "rmsACCurrentDensity") {
                oaIter<oaGroupMember> iterGroupMember(group->getMembers());
                oaGroupMember *member;
                while ((member = iterGroupMember.getNext()) != 0) {
                    oaObject *obj = member->getObject();
                    double xval =  0.0;
                    if (obj->getType() == oacPhysicalLayerType) {
                        sprintf(buf, "\"%s\"", (const char*)name);
                        fprintf(if_fp, "  ( %-27s", buf);

                        oaPhysicalLayer *lyr = (oaPhysicalLayer*)obj;
                        oaString lname;
                        lyr->getName(lname);
                        fprintf(if_fp, " \"%s\"\n", (const char*)lname);
                        fputs(
                        "    ((\"frequency\" nil nil \"width\" nil nil ))\n",
                            if_fp);
                    }
                    else if (obj->getType() == oacGroupType) {
                        oaIter<oaProp> iterProp(member->getProps());
                        oaProp *prp;
                        while ((prp = iterProp.getNext()) != 0) {
                            oaString pnm;
                            prp->getName(pnm);
                            if (pnm == "index") {
                                xval = ((oaDoubleProp*)prp)->getValue();
                                fputs("    (\n", if_fp);
                            }
                        }

                        oaGroup *g = (oaGroup*)obj;
                        oaIter<oaGroupMember> iterGM(g->getMembers());
                        oaGroupMember *m;
                        double entry = 0.0;
                        double index = 0.0;
                        while ((m = iterGM.getNext()) != 0) {
                            oaObject *o = m->getObject();
                            if (o->getType() == oacGroupType) {
                                oaIter<oaProp> iterProp1(m->getProps());
                                oaProp *prp;
                                while ((prp = iterProp1.getNext()) != 0) {
                                    oaString pnm;
                                    prp->getName(pnm);
                                    oaDoubleProp *dprp = (oaDoubleProp*)prp;
                                    if (pnm == "entry")
                                        entry = dprp->getValue();
                                    else if (pnm == "index")
                                        index = dprp->getValue();
                                }
                                sprintf(buf, "(%-6g %-6g)", xval, index);
                                fprintf(if_fp, "        %-16s %g\n", buf,
                                    entry);
                            }
                        }
                    }
                }
                fputs("    )\n  )\n", if_fp);
            }
        }
    }
    fputs(" ) ;currentDensityTables\n", if_fp);

    fputs(") ;layerRules\n", if_fp);
}


void
cOaTechIf::printCdsViaDefs()
{
    fputs("viaDefs(\n", if_fp);
    fputs(" standardViaDefs(\n", if_fp);
    fputs(" ;( viaDefName	layer1	layer2	(cutLayer cutWidth cutHeight "
        "[resistancePerCut])\n", if_fp);
    fputs(" ;   (cutRows	cutCol	(cutSpace)) \n", if_fp);
    fputs(" ;   (layer1Enc) (layer2Enc)	(layer1Offset)	(layer2Offset)	"
        "(origOffset)\n", if_fp);
    fputs(" ;   [implant1	 (implant1Enc)	[implant2	(implant2Enc) "
        "[well/substrate]]])\n", if_fp);
    fputs(" ;(-----------------------------------------------------------)\n",
        if_fp);

    char buf[256];
    {
        oaIter<oaViaDef> iterViaDef(if_tech->getViaDefs());
        oaViaDef *viadef;
        while ((viadef = iterViaDef.getNext()) != 0) {
            if (viadef->getType() == oacStdViaDefType) {
                oaStdViaDef *stvd = (oaStdViaDef*)viadef;
                oaString name;
                stvd->getName(name);
                oaPhysicalLayer *l1 = stvd->getLayer1();
                oaString l1name;
                l1->getName(l1name);
                oaPhysicalLayer *l2 = stvd->getLayer2();
                oaString l2name;
                l2->getName(l2name);
                oaViaParam prm;
                stvd->getParams(prm);
                int clnum = prm.getCutLayer();
                oaLayer *cl = oaLayer::find(if_tech, clnum);
                oaString clname;
                cl->getName(clname);

                fprintf(if_fp, "  ( %-15s %-14s %-14s (\"%s\" %g %g)\n",
                    (const char*)name, (const char*)l1name, (const char*)l2name,
                    (const char*)clname, dbuToUU(prm.getCutWidth()),
                    dbuToUU(prm.getCutHeight()));

                fprintf(if_fp, "     (%d %d (%g %g))\n", prm.getCutRows(),
                    prm.getCutColumns(), dbuToUU(prm.getCutSpacing().x()),
                    dbuToUU(prm.getCutSpacing().y()));

                sprintf(buf, "(%g %g)", dbuToUU(prm.getLayer1Enc().x()),
                    dbuToUU(prm.getLayer1Enc().y()));
                fprintf(if_fp, "     %-14s", buf);
                sprintf(buf, "(%g %g)", dbuToUU(prm.getLayer2Enc().x()),
                    dbuToUU(prm.getLayer2Enc().y()));
                fprintf(if_fp, " %-14s", buf);
                sprintf(buf, "(%g %g)", dbuToUU(prm.getLayer1Offset().x()),
                    dbuToUU(prm.getLayer1Offset().y()));
                fprintf(if_fp, " %-14s", buf);
                sprintf(buf, "(%g %g)", dbuToUU(prm.getLayer2Offset().x()),
                    dbuToUU(prm.getLayer2Offset().y()));
                fprintf(if_fp, " %-14s", buf);
                sprintf(buf, "(%g %g)", dbuToUU(prm.getOriginOffset().x()),
                    dbuToUU(prm.getOriginOffset().y()));
                fprintf(if_fp, " %s\n", buf);

                oaPhysicalLayer *imp1 = stvd->getImplant1();
                if (imp1) {
                    sprintf(buf, "(%g %g)", dbuToUU(prm.getImplant1Enc().x()),
                        dbuToUU(prm.getImplant1Enc().y()));
                    oaString i1name;
                    imp1->getName(i1name);
                    fprintf(if_fp, "     %-14s %-14s", (const char*)i1name,
                        buf);
                    oaPhysicalLayer *imp2 = stvd->getImplant2();
                    if (imp2) {
                        sprintf(buf, "(%g %g)",
                            dbuToUU(prm.getImplant2Enc().x()),
                            dbuToUU(prm.getImplant2Enc().y()));
                        oaString i2name;
                        imp2->getName(i2name);
                        fprintf(if_fp, " %-14s %-14s", (const char*)i2name,
                            buf);
                    }
                    fputc('\n', if_fp);
                }
                fputs("  )\n", if_fp);
            }
        }
    }
    fputs(" ) ;standardViaDefs\n\n", if_fp);

    fputs(" customViaDefs(\n", if_fp);
    fputs(" ;( viaDefName libName cellName viewName layer1 layer2 "
        "resistancePerCut )\n", if_fp);
    fputs(" ;( ---------- ------- -------- -------- ------ ------ "
        "---------------- )\n", if_fp);
    {
        oaIter<oaViaDef> iterViaDef(if_tech->getViaDefs());
        oaViaDef *viadef;
        while ((viadef = iterViaDef.getNext()) != 0) {
            if (viadef->getType() == oacCustomViaDefType) {
                oaCustomViaDef *cvd = (oaCustomViaDef*)viadef;
                oaString name;
                cvd->getName(name);
                oaString libname, cellname, viewname;
                cvd->getLibName(oaNativeNS(), libname);
                cvd->getCellName(oaNativeNS(), cellname);
                cvd->getViewName(oaNativeNS(), viewname);
                oaString l1name, l2name;
                cvd->getLayer1()->getName(l1name);
                cvd->getLayer2()->getName(l2name);
                double rpc = cvd->getResistancePerCut();

                fprintf(if_fp, "   ( %-13s %-12s %-12s %-12s %-7s %-6s %g )\n",
                    (const char*)name, (const char*)libname,
                    (const char*)cellname, (const char*)viewname,
                    (const char*)l1name, (const char*)l2name, rpc);
            }
        }
    }
    fputs(" ) ;customViaDefs\n", if_fp);

    fputs(") ;viaDefs\n", if_fp);
}


namespace {
    // We need to map the OpenAccess constraint group settings into
    // the format expected by Virtuoso.  There are a few things to do:
    // 1.  We need to generate the second-level grouping found in the
    //     Virtuoso tech file, with names like "spacings", etc.
    // 2.  We need to map some of the OA keywords into the corresponding
    //     different Virtuoso keyword.
    // 3.  We need to change spatial property values to microns.
    //
    // Below is a table used to identify the appropriate keywords.
    // We ignore any element names not found in the table.
    //
    // The constraint group elements are already in the correct order
    // for the Virtuoso tech file.  We just need to keep track of the
    // current container.

// Virtuoso second-level container names used in constraints.
const char *v_containers[] = {
    "interconnect",         // 0
    "routingGrids",         // 1
    "placementGrids",       // 2
    "spacings",             // 3
    "spacingTables",        // 4
    "orderedSpacings",      // 5
    "viaStackingLimits",    // 6
    "antennaModels",        // 7
    "electrical"            // 8
};
#define C_INTERCONNECT      v_containers[0]
#define C_ROUTINGGRIDS      v_containers[1]
#define C_PLACEMENTGRIDS    v_containers[2]
#define C_SPACING           v_containers[3]
#define C_SPACINGTABLES     v_containers[4]
#define C_ORDEREDSPACINGS   v_containers[5]
#define C_VIASTACKINGLIMITS v_containers[6]
#define C_ANTENNAMODELS     v_containers[7]
#define C_ELECTRICAL        v_containers[8]

struct v_cgmap
{
    const char *cname;      // OpenAccess constraint name.
    const char *vname;      // Virtuoso name, if different.
    const char *container;  // Virtuoso container name.
    int        ccode;       // 1:  All integer values to UU (distance)
                            // 2:  Scalar integer to UU squared (area)
                            // 3:  In 1D tables, integer abscissa to UU.
} v_map[] = {
    // OA keyword                   Virtuoso keyword    Container
    { "validRoutingLayers",         "validLayers",      C_INTERCONNECT, 0 },
    { "validRoutingVias",           "validVias",        C_INTERCONNECT, 0 },
    { "maxRoutingDistance",         0,                  C_INTERCONNECT, 1 },

    { "verticalRouteGridPitch",     "verticalPitch",    C_ROUTINGGRIDS, 1 },
    { "horizontalRouteGridPitch",   "horizontalPitch",  C_ROUTINGGRIDS, 1 },
    { "verticalRouteGridOffset",    "verticalOffset",   C_ROUTINGGRIDS, 1 },
    { "horizontalRouteGridOffset",  "horizontalOffset", C_ROUTINGGRIDS, 1 },
    { "45RouteGridPitch",           "leftDiagPitch",    C_ROUTINGGRIDS, 1 },
    { "135RouteGridPitch",          "rightDiagPitch",   C_ROUTINGGRIDS, 1 },

    { "horizontalPlacementGridPitch","horizontalPitch", C_PLACEMENTGRIDS, 1 },
    { "verticalPlacementGridPitch", "verticalPitch",    C_PLACEMENTGRIDS, 1 },

    // These may be "spacingTables" if they point to a table, or
    // "spacings" otherwise.
    { "minWidth",                   0,                  C_SPACING, 1 },
    { "maxWidth",                   0,                  C_SPACING, 1 },
    { "minSpacing",                 0,                  C_SPACING, 1 },
    { "minAdjacentViaSpacing",      "viaSpacing",       C_SPACING, 1 },
    { "minProtrusionNumCut",        0,                  C_SPACING, 0 },
    { "minSameNetSpacing",          0,                  C_SPACING, 1 },
    { "minArea",                    0,                  C_SPACING, 2 },
    { "minEnclosedArea",            "minHoleArea",      C_SPACING, 2 },
    { "minBoundaryInteriorHalo",    "minPRBoundaryInteriorHalo", C_SPACING,1 },
    { "keepAlignedShapeAndBoundary","keepPRBoundarySharedEdges", C_SPACING,1 },
    { "minBoundaryExtension",       "minPRBoundaryExtension",    C_SPACING,1 },
    { "minNumCut",                  0,                  C_SPACING, 3 },
    { "minDensity",                 0,                  C_SPACING, 3 },
    { "maxDensity",                 0,                  C_SPACING, 3 },

    { "minDualExtension",           "minOppExtension",  C_ORDEREDSPACINGS, 1 },
    { "minExtension",               "minEnclosure",     C_ORDEREDSPACINGS, 1 },

    { "viaStackLimit",              0,                  C_VIASTACKINGLIMITS,0 },

    { "antenna",                    0,                  C_ANTENNAMODELS,0 },
    { "cumulativeMetalAntenna",     0,                  C_ANTENNAMODELS,0 },
    { "cumulativeViaAntenna",       0,                  C_ANTENNAMODELS,0 },

    { 0,                            0,                  0, 0 }
};

    v_cgmap *getMap(oaString &name)
    {
        for (v_cgmap *m = v_map; m->cname; m++) {
            if (name == m->cname)
                return (m);
        }
        return (0);
    }
} // namespace


void
cOaTechIf::printCdsConstraintGroups()
{
    fputs("constraintGroups(\n", if_fp);

    oaIter<oaConstraintGroup> iterCG(if_tech->getConstraintGroups());
    oaConstraintGroup *cgroup;
    while ((cgroup = iterCG.getNext()) != 0) {
        oaString name;
        cgroup->getName(name);

        // Skip the "default" group.
        if (name == "default")
            continue;
        // Skip the "foundry" group, we add this last.
        if (name == "foundry")
            continue;
        printCdsConstraintGroup(cgroup);
        fputc('\n', if_fp);
    }

    // Now do "foundry".
    printCdsConstraintGroup(if_tech->getFoundryRules());

    fputs(") ;constraintGroups\n", if_fp);
}


void
cOaTechIf::printCdsConstraintGroup(const oaConstraintGroup *cgroup)
{
    if (!cgroup)
        return;
    oaString name;
    cgroup->getName(name);
    fputs(" ;( group    [override] )\n", if_fp);
    fputs(" ;( -----    ---------- )\n", if_fp);
    fprintf(if_fp, "  ( \"%s\"   %s\n", (const char*)name,
        cgroup->override() ? "t" : "nil");

    oaIter<oaConstraintGroupMem> iterCGM(cgroup->getMembers());
    oaConstraintGroupMem *member;
    const char *last_cont = 0;

    // We have to loop through the antenna constraints to get
    // and sequentially output the models, so keep a list.
    const oaConstraintGroupMem *antennaMods[20];
    int acnt = 0;

    while ((member = iterCGM.getNext()) != 0) {
        getConstraintGroupMemName(member, name);
        v_cgmap *vm = getMap(name);
        if (!vm)
            continue;

        // The "spacings" and "spacingTables" are ambiguous, as
        // some keywords can appear in either.  Check if a table
        // is present, if so we use "spacingTables".

        const char *container = vm->container;
        if (container == C_SPACING) {
            if (hasTable(member))
                container = C_SPACINGTABLES;
        }
        if (container == C_ANTENNAMODELS)
            antennaMods[acnt++] = member;
        if (container != last_cont) {
            if (last_cont) {
                if (last_cont == C_ANTENNAMODELS)
                    printCdsAntennaMods(antennaMods, acnt);
                fprintf(if_fp, "    ) ;%s\n\n", last_cont);
            }
            last_cont = container;
            fprintf(if_fp, "    %s(\n", container);
        }
        if (container == C_ANTENNAMODELS)
            continue;
        printCdsConstraintGroupMem(member, vm->vname, vm->ccode);
    }
    if (last_cont) {
        if (last_cont == C_ANTENNAMODELS)
            printCdsAntennaMods(antennaMods, acnt);
        fprintf(if_fp, "    ) ;%s\n", last_cont);
    }

    // Look for an oaGroup with the same name as the constraint group. 
    // Expect to find some goodies in there.

    cgroup->getName(name);
    oaIter<oaGroup> iterGroup(if_tech->getGroupsByName(name));
    oaGroup *group = iterGroup.getNext();  // At most one exists.

    if (group) {
        oaIter<oaGroupMember> iterGM(group->getMembers());
        group = 0;
        oaGroupMember *member;
        while ((member = iterGM.getNext()) != 0) {
            oaObject *obj = member->getObject();
            if (obj->getType() == oacGroupType) {
                oaGroup *g = (oaGroup*)obj;
                oaString gname;
                g->getName(gname);
                if (gname == "electricalRules") {
                    group = g;
                    break;
                }
            }
        }
    }

    if (group) {
        // Ha! Found the "electrical" constraint.

        fputs("\n    electrical(\n", if_fp);

        oaIter<oaGroupMember> iterGM(group->getMembers());
        oaGroupMember *member;
        while ((member = iterGM.getNext()) != 0) {
            oaObject *obj = member->getObject();
            if (obj->getType() == oacGroupType) {
                oaGroup *g = (oaGroup*)obj;
                oaString gname;
                g->getName(gname);

                if (gname == "oneLayerElectricalRules") {

                    oaIter<oaGroupMember> iterGM1(g->getMembers());
                    oaGroupMember *m;
                    while ((m = iterGM1.getNext()) != 0) {
                        oaObject *o = m->getObject();

                        if (o->getType() == oacPhysicalLayerType) {
                            oaString lname;
                            ((oaLayer*)o)->getName(lname);
                            oaIter<oaProp> iterProp(m->getProps());
                            oaProp *prop;
                            while ((prop = iterProp.getNext()) != 0) {
                                oaString pname;
                                prop->getName(pname);
                                oaString value;
                                prop->getValue(value);
                                fprintf(if_fp,
                                    "     ( %-25s %-14s %s )\n",
                                    (const char*)pname,
                                    (const char*)lname,
                                    (const char*)value);
                            }
                        }
                    }
                }
            }
        }
        fputs("    ) ;electrical\n", if_fp);
    }
    fputs(" )\n", if_fp);
}


void
cOaTechIf::printCdsConstraintGroupMem(const oaConstraintGroupMem *member,
    const char *vname, int ccode)
{
    char buf[256];
    oaString name;
    oaString id, dsc;
    const char *valstr = 0;
    const char *paramstr = 0;

    int indent = 5;
    oaObject *obj = member->getObject();
    switch (obj->getType()) {
    case oacSimpleConstraintType:
        {
            oaSimpleConstraint *cs = (oaSimpleConstraint*)obj;
            valstr = getValueAsString(cs->getValue(), ccode);
            if (!valstr)
                break;
            paramstr = getConstraintParamString(cs);

            if (vname)
                name = vname;
            else
                ((oaSimpleConstraintDef*)cs->getDef())->getName(name);
            if (name == "cumulativeMetalAntenna") {
                indent = 7;
                fprintf(if_fp, "%*c%s(\n", indent, ' ', (const char*)name);
            }
            else if (paramstr)
                fprintf(if_fp, "%*c( %s\n", indent, ' ', (const char*)name);
            else
                fprintf(if_fp, "%*c( %-27s", indent, ' ', (const char*)name);

            cs->getID(id);
            cs->getDescription(dsc);
        }
        break;
    case oacLayerConstraintType:
        {
            oaLayerConstraint *cs = (oaLayerConstraint*)obj;
            valstr = getValueAsString(cs->getValue(), ccode);
            if (!valstr)
                break;
            paramstr = getConstraintParamString(cs);

            if (vname)
                name = vname;
            else
                ((oaLayerConstraintDef*)cs->getDef())->getName(name);
            oaLayer *lyr = oaLayer::find(if_tech, cs->getLayer());
            oaString lname;
            lyr->getName(lname);
            if (oaPurposeType::get(cs->getPurpose()) != oacNoPurposeType) {
                oaPurpose *prp = oaPurpose::find(if_tech, cs->getPurpose());
                oaString pname;
                prp->getName(pname);
                sprintf(buf, "(\"%s\" \"%s\")", (const char*)lname,
                    (const char*)pname);
            }
            else {
                sprintf(buf, "\"%s\"", (const char*)lname);
            }
            if (name == "antenna") {
                // The returned valstr is missing the layer token, we
                // have to insert these.

                sLstr lstr;
                const char *s = strstr(valstr, "areaRatio");
                if (s) {
                    s += 13;
                    for (const char *t = valstr; t < s; t++)
                        lstr.add_c(*t);
                    lstr.add_c(' ');
                    int l = strlen(buf);
                    lstr.add(buf);
                    while (l < 7) {
                        l++;
                        lstr.add_c(' ');
                    }
                    const char *p = s;
                    s = strstr(p, "diffAreaRatio");
                    if (s) {
                        s += 13;
                        for (const char *t = p; t < s; t++)
                            lstr.add_c(*t);
                        lstr.add_c(' ');
                        int l = strlen(buf);
                        lstr.add(buf);
                        while (l < 7) {
                            l++;
                            lstr.add_c(' ');
                        }
                        p = s;
                    }
                    lstr.add(p);
                    delete [] valstr;
                    valstr = lstr.string_trim();
                }

                indent = 7;
                fprintf(if_fp, "%*c%s(\n", indent, ' ', (const char*)name);
            }
            else if (paramstr) {
                fprintf(if_fp, "%*c( %-27s %s\n", indent, ' ',
                    (const char*)name, buf);
            }
            else {
                fprintf(if_fp, "%*c( %-27s %-9s", indent, ' ',
                    (const char*)name, buf);
            }

            cs->getID(id);
            cs->getDescription(dsc);
        }
        break;
    case oacLayerPairConstraintType:
        {
            oaLayerPairConstraint *cs = (oaLayerPairConstraint*)obj;
            valstr = getValueAsString(cs->getValue(), ccode);
            if (!valstr)
                break;
            paramstr = getConstraintParamString(cs);

            if (vname)
                name = vname;
            else
                ((oaLayerPairConstraintDef*)cs->getDef())->getName(name);
            oaLayer *lyr = oaLayer::find(if_tech, cs->getLayer1());
            oaString lname;
            lyr->getName(lname);
            if (oaPurposeType::get(cs->getPurpose1()) != oacNoPurposeType) {
                oaPurpose *prp = oaPurpose::find(if_tech, cs->getPurpose1());
                oaString pname;
                prp->getName(pname);
                sprintf(buf, "(\"%s\" \"%s\")", (const char*)lname,
                    (const char*)pname);
            }
            else {
                sprintf(buf, "\"%s\"", (const char*)lname);
            }
            fprintf(if_fp, "%*c( %-27s %-9s", indent, ' ',
                (const char*)name, buf);

            lyr = oaLayer::find(if_tech, cs->getLayer2());
            lyr->getName(lname);
            if (oaPurposeType::get(cs->getPurpose2()) != oacNoPurposeType) {
                oaPurpose *prp = oaPurpose::find(if_tech, cs->getPurpose2());
                oaString pname;
                prp->getName(pname);
                sprintf(buf, "(\"%s\" \"%s\")", (const char*)lname,
                    (const char*)pname);
            }
            else {
                sprintf(buf, "\"%s\"", (const char*)lname);
            }
            if (paramstr)
                fprintf(if_fp, " %s\n", buf);
            else
                fprintf(if_fp, " %-9s", buf);

            cs->getID(id);
            cs->getDescription(dsc);
        }
        break;
    case oacLayerArrayConstraintType:
        {
            oaLayerArrayConstraint *cs = (oaLayerArrayConstraint*)obj;
            valstr = getValueAsString(cs->getValue(), ccode);
            if (!valstr)
                break;
            paramstr = getConstraintParamString(cs);

            if (vname)
                name = vname;
            else
                ((oaLayerArrayConstraintDef*)cs->getDef())->getName(name);
            if (paramstr)
                fprintf(if_fp, "%*c( %s\n", indent, ' ', (const char*)name);
            else
                fprintf(if_fp, "%*c( %-27s", indent, ' ', (const char*)name);

            cs->getID(id);
            cs->getDescription(dsc);
        }
        break;
    case oacConstraintGroupHeaderType:
        // Don't list these, they are transient.
        break;
    case oacConstraintGroupType:
        // There doesn't seem to be any of these.
        break;
    default:
        {
            // Shouldn't get here.
            name = member->getObject()->getType().getName();
            fprintf(if_fp, "     ; ERROR, unhandled type %s\n",
                (const char*)name);
        }
        break;
    }

    if (valstr) {
        bool has_nl = strchr(valstr, '\n');  // likely a table
        if (paramstr) {
            fputs(paramstr, if_fp);
            delete [] paramstr;
            if (has_nl)
                fprintf(if_fp, " %s", valstr);
            else
                fprintf(if_fp, "        %s\n", valstr);
            if (!id.isEmpty())
                fprintf(if_fp, "        'ref %s\n", (const char*)id);
            fprintf(if_fp, "%*c)", indent, ' ');
        }
        else {
            if (!id.isEmpty()) {
                if (has_nl) {
                    fprintf(if_fp, " %s", valstr);
                    fprintf(if_fp, "%*c'ref %s )", indent, ' ',
                        (const char*)id);
                }
                else {
                    fprintf(if_fp, " %-12s 'ref %s )", valstr,
                        (const char*)id);
                }
            }
            else {
                if (has_nl) {
                    fprintf(if_fp, " %s", valstr);
                    fprintf(if_fp, "%*c)", indent, ' ');
                }
                else
                    fprintf(if_fp, " %s )", valstr);
            }
        }
        delete [] valstr;

        if (!dsc.isEmpty())
            fprintf(if_fp, " ; %s", (const char*)dsc);
        fputc('\n', if_fp);
    }
}


void
cOaTechIf::printCdsAntennaMods(const oaConstraintGroupMem **ary, int numobjs)
{
    fputs("    ;( model )\n", if_fp);
    for (int model = 0; model < 4; model++) {
        bool hasmod = false;
        for (int i = 0; i < numobjs; i++) {
            if (hasAntennaMod(ary[i], model)) {
                hasmod = true;
                break;
            }
        }
        if (!hasmod)
            break;

        if (model == 0)
            fputs("     ( \"default\"\n", if_fp);
        else if (model == 1)
            fputs("     ( \"second\"\n", if_fp);
        else if (model == 2)
            fputs("     ( \"third\"\n", if_fp);
        else if (model == 3)
            fputs("     ( \"fourth\"\n", if_fp);

        for (int i = 0; i < numobjs; i++)
            printCdsConstraintGroupMem(ary[i], 0, model);
        fputs("     )\n", if_fp);
    }
}


void
cOaTechIf::printCdsDevices()
{
    fputs("devices(\n", if_fp);
    fputs("tcCreateCDSDeviceClass()\n", if_fp);
    fputs("multipartPathTemplates(\n", if_fp);
    fputs("; ( name [masterPath] [offsetSubpaths] [encSubPaths] [subRects] "
        ")\n", if_fp);
    fputs(";\n", if_fp);
    fputs(";   masterPath:\n", if_fp);
    fputs(";   (layer [width] [choppable] [endType] [beginExt] [endExt] "
        "[justify] [offset]\n", if_fp);
    fputs(";   [connectivity])\n", if_fp);
    fputs(";\n", if_fp);
    fputs(";   offsetSubpaths:\n", if_fp);
    fputs(";   (layer [width] [choppable] [separation] [justification] "
        "[begOffset] [endOffset]\n", if_fp);
    fputs(";   [connectivity])\n", if_fp);
    fputs(";\n", if_fp);
    fputs(";   encSubPaths:\n", if_fp);
    fputs(";   (layer [enclosure] [choppable] [separation] [begOffset] "
        "[endOffset]\n", if_fp);
    fputs(";   [connectivity])\n", if_fp);
    fputs(";\n", if_fp);
    fputs(";   subRects:\n", if_fp);
    fputs(";   (layer [width] [length] [choppable] [separation] "
        "[justification] [space] [begOffset] [endOffset] [gap] \n", if_fp);
    fputs(";   [connectivity] [beginSegOffset] [endSegOffset])\n", if_fp);
    fputs(";\n", if_fp);
    fputs(";   connectivity:\n", if_fp);
    fputs(";   ([I/O type] [pin] [accDir] [dispPinName] [height] [ layer]\n",
        if_fp);
    fputs(";    [layer] [justification] [font] [textOptions] [orientation]\n",
        if_fp);
    fputs(";    [refHandle] [offset])\n", if_fp);
    fputs(";\n", if_fp);

    {
        oaIter<oaGroup> iterGroup(if_tech->getGroupsByName("lxMPPTemplates"));
        oaGroup *group = iterGroup.getNext();
        if (group) {
            oaIter<oaGroupMember> iterGroupMember(group->getMembers());
            oaGroupMember *member;
            while ((member = iterGroupMember.getNext()) != 0) {
                oaObject *obj = member->getObject();
                if (obj->getType() == oacGroupType) {
                    oaGroup *g = (oaGroup*)obj;
                    oaIter<oaProp> iterProp(member->getProps());
                    oaProp *prop;
                    while ((prop = iterProp.getNext()) != 0) {
                        oaString pn;
                        prop->getName(pn);
                        if (pn == "mppName")
                            break;
                    }
                    if (prop) {
                        oaString pv;
                        prop->getValue(pv);
                        fprintf(if_fp, "  (%s\n", (const char*)pv);
                        printCdsMppRec(g);
                    }
                }
            }
        }
    }
    fputs(")  ;multipartPathTemplates\n", if_fp);

    {
        bool pmos = false;
        bool pres = false;
        oaIter<oaGroup> iterGroup(if_tech->getGroupsByName("extractDevices"));
        oaGroup *group = iterGroup.getNext();
        if (group) {
            oaIter<oaGroupMember> iterGroupMember(group->getMembers());
            oaGroupMember *member;
            while ((member = iterGroupMember.getNext()) != 0) {
                oaObject *obj = member->getObject();
                if (obj->getType() == oacGroupType) {
                    oaGroup *g = (oaGroup*)obj;
                    oaString gname;
                    g->getName(gname);

                    if (gname == "extractMOS") {
                        if (!pmos) {
                            pmos = true;
                            fputs("\n;extractMOS(deviceName  recLayer "
                                "gateLayer sdLayer bulkLayer [spiceModel])\n",
                                if_fp);
                        }
                    }
                    else if (gname == "extractRES") {
                        if (!pres) {
                            pres = true;
                            fputs("\n;extractRES(deviceName  recLayer "
                                "termLayer[spiceModel])\n", if_fp);
                        }
                    }

                    fprintf(if_fp, "%s(", (const char*)gname);

                    oaIter<oaGroupMember> iterGroupMember1(g->getMembers());
                    oaGroupMember *m;
                    while ((m = iterGroupMember1.getNext()) != 0) {
                        oaObject *o = m->getObject();
                        if (o->getType() == oacDerivedLayerType) {
                            oaDerivedLayer *dl = (oaDerivedLayer*)o;
                            oaIter<oaProp> iterProp(m->getProps());
                            oaProp *prop;
                            while ((prop = iterProp.getNext()) != 0) {
                                oaString pn;
                                prop->getName(pn);
                                if (pn == "deviceName") {
                                    oaString pv;
                                    prop->getValue(pv);
                                    oaString lname;
                                    dl->getName(lname);
                                    fprintf(if_fp, "\"%s\" \"%s\"",
                                        (const char*)pv, (const char*)lname);
                                }
                            }
                        }
                        else if (o->getType() == oacPhysicalLayerType) {
                            oaPhysicalLayer *pl = (oaPhysicalLayer*)o;
                            oaString lname;
                            pl->getName(lname);
                            fprintf(if_fp, " \"%s\"", (const char*)lname);
                        }
                        else if (o->getType() == oacAppObjectType) {
                            fprintf(if_fp, " \"%s\"", "substrate");
                        }
                    }
                    fputs(")\n", if_fp);
                }
            }
        }
    }

    fputs(") ;devices\n", if_fp);
}


namespace {
    // Convert boolean values to LISP.
    //
    inline void bfix(oaString str)
    {
        if (str == "true")
            str = "t";
        else if (str == "false")
            str = "nil";
    }


    struct techMPP
    {
        void set(const oaObject *object)
            {
                oaIter<oaProp> iterProp(object->getProps());
                oaProp *p;
                while ((p = iterProp.getNext()) != 0) {
                    oaString pn;
                    p->getName(pn);
                    if (pn == "mppWidth") {
                        p->getValue(mppWidth);
                        bfix(mppWidth);
                    }
                    else if (pn == "mppChoppable") {
                        p->getValue(mppChoppable);
                        bfix(mppChoppable);
                    }
                    else if (pn == "mppEndType") {
                        p->getValue(mppEndType);
                        bfix(mppEndType);
                    }
                    else if (pn == "mppBeginExt") {
                        p->getValue(mppBeginExt);
                        bfix(mppBeginExt);
                    }
                    else if (pn == "mppEndExt") {
                        p->getValue(mppEndExt);
                        bfix(mppEndExt);
                    }
                    else if (pn == "mppJustification") {
                        p->getValue(mppJustification);
                        bfix(mppJustification);
                    }
                    else if (pn == "mppOffset") {
                        p->getValue(mppOffset);
                        bfix(mppOffset);
                    }
                    else if (pn == "mppEnclosure") {
                        p->getValue(mppEnclosure);
                        bfix(mppEnclosure);
                    }
                    else if (pn == "mppSeparation") {
                        p->getValue(mppSeparation);
                        bfix(mppSeparation);
                    }
                    else if (pn == "mppBeginOffset") {
                        p->getValue(mppBeginOffset);
                        bfix(mppBeginOffset);
                    }
                    else if (pn == "mppEndOffset") {
                        p->getValue(mppEndOffset);
                        bfix(mppEndOffset);
                    }
                    else if (pn == "mppLength") {
                        p->getValue(mppLength);
                        bfix(mppLength);
                    }
                    else if (pn == "mppSpacing") {
                        p->getValue(mppSpacing);
                        bfix(mppSpacing);
                    }
                    else if (pn == "mppGap") {
                        p->getValue(mppGap);
                        bfix(mppGap);
                    }
                    else if (pn == "mppBeginSegOffset") {
                        p->getValue(mppBeginSegOffset);
                        bfix(mppBeginSegOffset);
                    }
                    else if (pn == "mppEndSegOffset") {
                        p->getValue(mppEndSegOffset);
                        bfix(mppEndSegOffset);
                    }
                }
            }

        oaString mppWidth;
        oaString mppChoppable;
        oaString mppEndType;
        oaString mppBeginExt;
        oaString mppEndExt;
        oaString mppJustification;
        oaString mppOffset;
        oaString mppEnclosure;
        oaString mppSeparation;
        oaString mppBeginOffset;
        oaString mppEndOffset;
        oaString mppLength;
        oaString mppSpacing;
        oaString mppGap;
        oaString mppBeginSegOffset;
        oaString mppEndSegOffset;
    };
}


void
cOaTechIf::printCdsMppRec(const oaGroup *group)
{
    char buf[256];
    oaString gname;
    group->getName(gname);
    bool didMaster = false;
    bool didOffset = false;
    bool didEnc = false;
    oaIter<oaGroupMember> iterGroupMember(group->getMembers());
    oaGroupMember *member;
    while ((member = iterGroupMember.getNext()) != 0) {
        oaObject *obj = member->getObject();
        if (obj->getType() == oacAppObjectType) {
            oaAppObject *ao = (oaAppObject*)obj;

            techLPP lpp;
            if (!getCdsLpp(lpp, ao))
                continue;

            oaString lname, pname;
            getLppNames(lpp, lname, pname);
            sprintf(buf, "(\"%s\" \"%s\")",
                (const char*)lname, (const char*)pname);

            techMPP mpp;
            mpp.set(member);

            fprintf(if_fp, "    (%-19s", buf);

            if (gname == "mppTemplate") {
                didMaster = true;

                // masterpath
                if (!mpp.mppWidth.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppWidth);
                if (!mpp.mppChoppable.isEmpty())
                    fprintf(if_fp, " %-4s", (const char*)mpp.mppChoppable);
                if (!mpp.mppEndType.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppEndType);
                if (!mpp.mppBeginExt.isEmpty())
                    fprintf(if_fp, " %-4s", (const char*)mpp.mppBeginExt);
                if (!mpp.mppEndExt.isEmpty())
                    fprintf(if_fp, " %-4s", (const char*)mpp.mppEndExt);
                // justify offset connectivity
            }
            if (gname == "mppOffsetPath") {  // XXX keyword is guess
                // offsetSubPaths
                if (!mpp.mppWidth.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppWidth);
                if (!mpp.mppChoppable.isEmpty())
                    fprintf(if_fp, " %-4s", (const char*)mpp.mppChoppable);
                if (!mpp.mppSeparation.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppSeparation);
                if (!mpp.mppJustification.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppJustification);
                if (!mpp.mppBeginOffset.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppBeginOffset);
                if (!mpp.mppEndOffset.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppEndOffset);
                // connectivity
            }
            if (gname == "mppEncPath") {
                // encSubPaths
                if (!mpp.mppEnclosure.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppEnclosure);
                if (!mpp.mppChoppable.isEmpty())
                    fprintf(if_fp, " %-4s", (const char*)mpp.mppChoppable);
                if (!mpp.mppSeparation.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppSeparation);
                if (!mpp.mppBeginOffset.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppBeginOffset);
                if (!mpp.mppEndOffset.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppEndOffset);
                // connectivity
            }
            if (gname == "mppSubRect") {
                // subRects

                // In the model tech file, the values below need permuting.
                // Is this Cadence being "clever" or a bug?

                oaString temp = mpp.mppGap;
                mpp.mppGap = mpp.mppJustification;
                mpp.mppJustification = mpp.mppSpacing;
                mpp.mppSpacing = mpp.mppBeginOffset;
                mpp.mppBeginOffset = temp;

                if (!mpp.mppWidth.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppWidth);
                if (!mpp.mppLength.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppLength);
                if (!mpp.mppChoppable.isEmpty())
                    fprintf(if_fp, " %-4s", (const char*)mpp.mppChoppable);
                if (!mpp.mppSeparation.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppSeparation);
                if (!mpp.mppJustification.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppJustification);
                if (!mpp.mppSpacing.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppSpacing);
                if (!mpp.mppBeginOffset.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppBeginOffset);
                if (!mpp.mppEndOffset.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppEndOffset);
                if (!mpp.mppGap.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppGap);
                fprintf(if_fp, " %-4s", "nil"); // connectivity
                if (!mpp.mppBeginSegOffset.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppBeginSegOffset);
                if (!mpp.mppEndSegOffset.isEmpty())
                    fprintf(if_fp, " %-7s", (const char*)mpp.mppEndSegOffset);
            }

            fputs(" )\n", if_fp);
        }
        else if (obj->getType() == oacGroupType) {
            if ((oaGroup*)obj == group)
                continue;
            oaGroup *g = (oaGroup*)obj;
            if (gname == "mppTemplate") {
                oaString gn;
                g->getName(gn);
                if (gn == "mppOffsetPaths" || gn == "mppOffsetPath") {
                    if (!didMaster) {
                        fputs("   nil\n", if_fp);
                        didMaster = true;
                    }
                    didOffset = true;
                }
                else if (gn == "mppEncPaths" || gn == "mppEncPath") {
                    if (!didMaster) {
                        fputs("   nil\n", if_fp);
                        didMaster = true;
                    }
                    if (!didOffset) {
                        fputs("   nil\n", if_fp);
                        didOffset = true;
                    }
                    didEnc = true;
                }
                else if (gn == "mppSubRects" || gn == "mppSubRect") {
                    if (!didMaster) {
                        fputs("   nil\n", if_fp);
                        didMaster = true;
                    }
                    if (!didOffset) {
                        fputs("   nil\n", if_fp);
                        didOffset = true;
                    }
                    if (!didEnc) {
                        fputs("   nil\n", if_fp);
                        didEnc = true;
                    }
                }
                if (gn == "mppOffsetPaths" || gn == "mppEncPaths" ||
                        gn == "mppSubRects")
                    fputs("   (\n", if_fp);

                printCdsMppRec(g);
                if (gn == "mppOffsetPaths" || gn == "mppEndPaths" ||
                        gn == "mppSubRects")
                    fputs("   )\n", if_fp);
            }
            else
                printCdsMppRec(g);
        }
    }
}


void
cOaTechIf::printCdsViaSpecs()
{
    fputs("viaSpecs(\n", if_fp);
    fputs(" ;(layer1  layer2  (viaDefName ...)\n", if_fp);
    fputs(" ;   [(\n", if_fp);
    fputs(" ;	(layer1MinWidth layer1MaxWidth layer2MinWidth layer2MaxWidth\n", if_fp);
    fputs(" ;            (viaDefName ...))\n", if_fp);
    fputs(" ;	...\n", if_fp);
    fputs(" ;   )])\n", if_fp);

    oaIter<oaViaSpec> iterViaSpec(if_tech->getViaSpecs());
    oaViaSpec *viaspec;
    while ((viaspec = iterViaSpec.getNext()) != 0) {

        oaPhysicalLayer *l1 = viaspec->getLayer1();
        oaString l1name;
        l1->getName(l1name);
        oaPhysicalLayer *l2 = viaspec->getLayer2();
        oaString l2name;
        l2->getName(l2name);
        fprintf(if_fp, "  ( %-10s %-10s", (const char*)l1name,
            (const char*)l2name);

        oaViaDefArrayValue *av = viaspec->getDefaultValue();
        if (av) {
            oaViaDefArray a;
            av->get(a);
            int n = a.getNumElements();
            fputs(" (", if_fp);
            for (int i = 0; i < n; i++) {
                oaViaDef *vdb = a.get(i);
                oaString name;
                vdb->getName(name);
                fprintf(if_fp, " \"%s\"", (const char*)name);
            }
            fputs(" )\n", if_fp);
        }

        oaViaDef2DTblValue *a2d = viaspec->getValue();
        if (a2d) {
            oa2DLookupTbl<oaInt4, oaInt4, oaViaDefArrayValue*> adv;
            a2d->get(adv);
            int nr = adv.getNumRows();
            int nc = adv.getNumCols();
            for (int i = 0; i < nc; i++) {
                double min1 = i > 0 ? dbuToUU(adv.getColHeader(i-1)) : 0.0;
                double max1 = dbuToUU(adv.getColHeader(i));
                for (int j = 0; j < nr; j++) {
                    double min2 = j > 0 ? dbuToUU(adv.getRowHeader(j-1)) : 0.0;
                    double max2 = dbuToUU(adv.getRowHeader(j));
                    av = adv.getValue(j, i);
                    if (av) {
                        oaViaDefArray a;
                        av->get(a);
                        int n = a.getNumElements();
                        fprintf(if_fp, " %g %g %g %g (", min1, max1,
                            min2, max2);
                        for (int i = 0; i < n; i++) {
                            oaViaDef *vdb = a.get(i);
                            oaString name;
                            vdb->getName(name);
                            fprintf(if_fp, " \"%s\"", (const char*)name);
                        }
                        fputs(" )\n", if_fp);
                    }
                }
            }
        }
        fputs("  )\n", if_fp);
    }
    fputs(") ;viaSpecs\n", if_fp);
}
// End of Virtuoso format print functions.


// A general purpose print function, used mostly for hacking and
// debugging.  Where applicable, we follow the format of the Xic
// technology file.  Otherwise, there is no set format, we try to dump
// as much as possible.
//
// The which argument will specialize output to the type of data, see
// the parse_token function.  The prname will restrict to that data
// item name.  iF both are null, everything is printed.
//
//
void
cOaTechIf::printTech(const char *which, const char *prname)
{
    if (!if_tech || !if_fp)
        return;
    if (!which || !*which) {
        which = "all";
        prname = 0;
    }
    unsigned int wflag = parse_token(which);
    if (!wflag) {
        fprintf(if_fp, "Error: unrecognized designator %s.\n", which);
        return;
    }

    if (wflag == (unsigned int)-1) {
        fprintf(if_fp, "; Generated by %s\n", XM()->IdString());
        oaLib *lib = if_tech->getLib();
        oaString libpath;
        lib->getFullPath(libpath);
        fprintf(if_fp, "; Technology dump from OpenAccess library\n; %s\n\n",
            (const char*)libpath);

        if (if_tech->hasProcessFamily()) {
            oaString str;
            if_tech->getProcessFamily(str);
            fprintf(if_fp, "Process family: %s\n", (const char*)str);
        }

        oaScalarName libname;
        if_tech->getLibName(libname);
        if (oaLibDMData::exists(libname)) {
            fprintf(if_fp, "DM data library exists.\n");
            oaLibDMData *dm = oaLibDMData::open(libname, 'r');

            fprintf(if_fp, "DM groups:\n");
            oaIter<oaGroup> nIter(dm->getGroups());
            oaGroup *group;
            while ((group = nIter.getNext()) != 0) {
                if (prname) {
                    oaString name;
                    group->getName(name);
                    if (strcmp(name, prname))
                        continue;
                }
                printGroup(group);
            }

            fprintf(if_fp, "DM app object defs:\n");
            oaIter<oaAppObjectDef> nIter1(dm->getAppObjectDefs());
            oaAppObjectDef *appodef;
            while ((appodef = nIter1.getNext()) != 0) {

                oaString name;
                appodef->getName(name);
                fprintf(if_fp, "  %s\n", (const char*)name);
                printProperties(appodef, 4);
                printAppDef(appodef);
                if (appodef->hasConstraintGroup())
                    fputs("has constraint group\n", if_fp);

                oaIter<oaAppObject> nIter2(if_tech->getAppObjects(appodef));
                oaAppObject *appobj;
                while ((appobj = nIter2.getNext()) != 0) {
                    oaString name;
                    appobj->getAppObjectDef()->getName(name);
                    fprintf(if_fp, "    %s\n", (const char*)name);

                    printProperties(appobj, 4);
                    printAppDef(appobj);
                    if (appobj->hasConstraintGroup())
                        fputs("has constraint group\n", if_fp);
                }
            }
            fprintf(if_fp, "DM end\n\n");
        }


        // Print properties.
        fputs("Technology database properties:\n", if_fp);
        printProperties(if_tech, 0);
        printAppDef(if_tech);
        fputs("End of database properties.\n\n", if_fp);
    }

    if (wflag & OATECH_PR_UNITS) {
        fputs("View-type units:\n", if_fp);
        for (int i = 0; i < 4; i++) {
            oaString name, type;
            oaViewType *vt;
            if (i == 0)
                vt = oaViewType::get(oaReservedViewType(oacMaskLayout));
            else if (i == 1)
                vt = oaViewType::get(oaReservedViewType(oacSchematic));
            else if (i == 2)
                vt = oaViewType::get(oaReservedViewType(oacSchematicSymbol));
            else
                vt = oaViewType::get(oaReservedViewType(oacNetlist));

            if (if_tech->isDBUPerUUSet(vt) && if_tech->isUserUnitsSet(vt)) {
                vt->getName(name);
                oaString type = if_tech->getUserUnits(vt).getName();

                fprintf(if_fp, "  ( %-19s %-14s %d )\n", (const char*)name,
                    (const char*)type, if_tech->getDBUPerUU(vt));
            }
        }
        fputc('\n', if_fp);
    }

    if (wflag & OATECH_PR_ANLIBS) {
        fprintf(if_fp, "Analysis libs:\n");
        oaIter<oaAnalysisLib> nIter(if_tech->getAnalysisLibs());
        oaAnalysisLib *anlib;
        while ((anlib = nIter.getNext()) != 0) {
            oaString name;
            anlib->getName(name);
            if (prname && strcmp(name, prname))
                continue;
            fprintf(if_fp, "  %-16s\n", (const char*)name);
        }
        fputc('\n', if_fp);
    }

    if (wflag & OATECH_PR_LAYERS) {
        fprintf(if_fp, "Layers:\n");
        oaIter<oaLayer> nIter(if_tech->getLayers());
        oaLayer *layer;
        while ((layer = nIter.getNext()) != 0) {
            oaString name;
            layer->getName(name);
            if (prname && strcmp(name, prname))
                continue;
            if (layer->getType() == oacPhysicalLayerType) {
                fprintf(if_fp, "DefineLayer %-16s %d\n", (const char*)name,
                    layer->getNumber());
                printProperties(layer, 4);
                printAppDef(layer);
            }
        }
        fputc('\n', if_fp);
    }

    if (wflag & OATECH_PR_OPPTS) {
        fprintf(if_fp, "Op Points:\n");
        oaIter<oaOpPoint> nIter(if_tech->getOpPoints());
        oaOpPoint *oppoint;
        while ((oppoint = nIter.getNext()) != 0) {
            oaString name;
            oppoint->getName(name);
            if (prname && strcmp(name, prname))
                continue;
            fprintf(if_fp, "  %-16s\n", (const char*)name);
        }
        fputc('\n', if_fp);
    }

    if (wflag & OATECH_PR_PURPS) {
        fprintf(if_fp, "Purposes:\n");
        oaIter<oaPurpose> nIter(if_tech->getPurposes());
        oaPurpose *purpose;
        while ((purpose = nIter.getNext()) != 0) {
            oaString name;
            purpose->getName(name);
            if (prname && strcmp(name, prname))
                continue;
            if ((int)purpose->getNumber() >= 0) {
                fprintf(if_fp, "DefinePurpose %-16s %d\n", (const char*)name,
                    purpose->getNumber());
                printProperties(purpose, 4);
                printAppDef(purpose);
            }
        }
        fputc('\n', if_fp);
    }

    if (wflag & OATECH_PR_STDEFS) {
        fprintf(if_fp, "Site Defs:\n");
        oaIter<oaSiteDef> nIter(if_tech->getSiteDefs());
        oaSiteDef *sitedef;
        while ((sitedef = nIter.getNext()) != 0) {
            oaString name;
            sitedef->getName(name);
            if (prname && strcmp(name, prname))
                continue;
            fprintf(if_fp, "  %s\n", (const char*)name);
        }
        fputc('\n', if_fp);
    }

    if (wflag & OATECH_PR_VALUES) {
        fprintf(if_fp, "Values:\n");
        oaIter<oaValue> nIter(if_tech->getValues());
        oaValue *value;
        int cnt = 0;
        while ((value = nIter.getNext()) != 0) {
            cnt++;
        }
        fprintf(if_fp, "  %d\n\n", cnt);
    }

    if (wflag & OATECH_PR_VIADEFS) {
        fprintf(if_fp, "Via Defs:\n");
        oaIter<oaViaDef> nIter(if_tech->getViaDefs());
        oaViaDef *viadef;
        while ((viadef = nIter.getNext()) != 0) {
            oaString name;
            viadef->getName(name);
            if (prname && strcmp(name, prname))
                continue;
            printViaDef(viadef);
        }
        fputc('\n', if_fp);
    }

    if (wflag & OATECH_PR_VIASPECS) {
        fprintf(if_fp, "Via Specs:\n");
        oaIter<oaViaSpec> nIter(if_tech->getViaSpecs());
        oaViaSpec *viaspec;

        while ((viaspec = nIter.getNext()) != 0) {

            oaPhysicalLayer *l1 = viaspec->getLayer1();
            oaString l1name;
            l1->getName(l1name);
            oaPhysicalLayer *l2 = viaspec->getLayer2();
            oaString l2name;
            l2->getName(l2name);
            fprintf(if_fp, "  %s %s", (const char*)l1name, (const char*)l2name);

            oaViaDefArrayValue *av = viaspec->getDefaultValue();
            if (av) {
                oaViaDefArray a;
                av->get(a);
                int n = a.getNumElements();
                fputs(" (", if_fp);
                for (int i = 0; i < n; i++) {
                    oaViaDef *vdb = a.get(i);
                    oaString name;
                    vdb->getName(name);
                    fprintf(if_fp, " \"%s\"", (const char*)name);
                }
                fputs(" )\n", if_fp);
            }
            oaViaDef2DTblValue *a2d = viaspec->getValue();
            if (a2d) {
                oa2DLookupTbl<oaInt4, oaInt4, oaViaDefArrayValue*> adv;
                a2d->get(adv);
                int nr = adv.getNumRows();
                int nc = adv.getNumCols();
                for (int i = 0; i < nc; i++) {
                    for (int j = 0; j < nr; j++) {
                        av = adv.getValue(j, i);
                        if (av) {
                            oaViaDefArray a;
                            av->get(a);
                            int n = a.getNumElements();
                            fprintf(if_fp, " %d %d (", i, j);
                            for (int i = 0; i < n; i++) {
                                oaViaDef *vdb = a.get(i);
                                oaString name;
                                vdb->getName(name);
                                fprintf(if_fp, " \"%s\"", (const char*)name);
                            }
                            fputs(" )\n", if_fp);
                        }
                    }
                }
            }
        }
        fputc('\n', if_fp);
    }

    if (wflag & OATECH_PR_VIAVARS) {
        fprintf(if_fp, "Via Variants:\n");
        oaIter<oaViaVariant> nIter(if_tech->getViaVariants());
        oaViaVariant *viavar;
        int cnt = 0;
        while ((viavar = nIter.getNext()) != 0) {
            /*
            oaString name;
            viaspec->getName(name);
            char *nm = lstring::copy(name);
            fprintf(fp, "  %s\n", nm);
            delete [] nm;
            */
            cnt++;
        }
        fprintf(if_fp, "  %d\n\n", cnt);
    }

    if (wflag & OATECH_PR_CNSTGRPS) {
        fprintf(if_fp, "Constraint Groups:\n");
        oaIter<oaConstraintGroup> nIter(if_tech->getConstraintGroups());
        oaConstraintGroup *cgroup;
        while ((cgroup = nIter.getNext()) != 0) {
            if (prname) {
                oaString name;
                cgroup->getName(name);
                if (strcmp(name, prname))
                    continue;
            }
            printConstraintGroup(cgroup);
        }
        fputc('\n', if_fp);
    }

    if (wflag & OATECH_PR_CNSTPRMS) {
        fprintf(if_fp, "Constraint Params:\n");
        oaIter<oaConstraintParam> nIter(if_tech->getConstraintParams());
        oaConstraintParam *cparam;
        while ((cparam = nIter.getNext()) != 0) {
            oaConstraintParamDef *cdef = cparam->getDef();
            oaString name;
            cdef->getName(name);
            fprintf(if_fp, "  %s", (const char*)name);
            char *nm = getValueAsString(cparam->getValue());
            fprintf(if_fp, " %s\n", nm);
            delete [] nm;
        }
        fputc('\n', if_fp);
    }

    if (wflag & OATECH_PR_DRVLPRMS) {
        fprintf(if_fp, "Derived Layer Params:\n");
        oaIter<oaDerivedLayerParam> nIter(if_tech->getDerivedLayerParams());
        oaDerivedLayerParam *cparam;
        int cnt = 0;
        while ((cparam = nIter.getNext()) != 0) {
            /*
            oaString name;
            viaspec->getName(name);
            char *nm = lstring::copy(name);
            fprintf(fp, "  %s\n", nm);
            delete [] nm;
            */
            cnt++;
        }
        fprintf(if_fp, "  %d\n\n", cnt);
    }

    if (wflag & OATECH_PR_APPOBJDEFS) {
        fprintf(if_fp, "App object defs:\n");
        oaIter<oaAppObjectDef> nIter(if_tech->getAppObjectDefs());
        oaAppObjectDef *appodef;
        while ((appodef = nIter.getNext()) != 0) {

            oaString name;
            appodef->getName(name);
            fprintf(if_fp, "  %s\n", (const char*)name);
            printProperties(appodef, 4);
            printAppDef(appodef);
            if (appodef->hasConstraintGroup())
                fputs("has constraint group\n", if_fp);

            oaIter<oaAppObject> nIter1(if_tech->getAppObjects(appodef));
            oaAppObject *appobj;
            while ((appobj = nIter1.getNext()) != 0) {
                oaString name;
                appobj->getAppObjectDef()->getName(name);
                fprintf(if_fp, "    %s\n", (const char*)name);

                printProperties(appobj, 4);
                printAppDef(appobj);
                if (appobj->hasConstraintGroup())
                    fputs("has constraint group\n", if_fp);
            }
        }
        fputc('\n', if_fp);
    }

    if (wflag & OATECH_PR_GROUPS) {
        // oaCollection<oaGroup, oaTech> getGroups () const
        fprintf(if_fp, "Groups:\n");
        oaIter<oaGroup> nIter(if_tech->getGroups());
        oaGroup *group;
        while ((group = nIter.getNext()) != 0) {
            if (prname) {
                oaString name;
                group->getName(name);
                if (strcmp(name, prname))
                    continue;
            }
            printGroup(group);
        }
    }
}


void
cOaTechIf::printViaDef(const oaViaDef *viadef)
{
    char buf[256];
    if (viadef->getType() == oacStdViaDefType) {
        oaStdViaDef *stvd = (oaStdViaDef*)viadef;
        oaString name;
        stvd->getName(name);
        oaPhysicalLayer *l1 = stvd->getLayer1();
        oaString l1name;
        l1->getName(l1name);
        oaPhysicalLayer *l2 = stvd->getLayer2();
        oaString l2name;
        l2->getName(l2name);
        oaViaParam prm;
        stvd->getParams(prm);
        int clnum = prm.getCutLayer();
        oaLayer *cl = oaLayer::find(if_tech, clnum);
        oaString clname;
        cl->getName(clname);

        fprintf(if_fp, "%s %-16s %-10s %-10s %s \\\n", "StandardVia",
            (const char*)name, (const char*)l1name, (const char*)l2name,
            (const char*)clname);

        sprintf(buf, "%g %g", dbuToUU(prm.getCutWidth()),
            dbuToUU(prm.getCutHeight()));
        fprintf(if_fp, "    %-12s", buf);
        sprintf(buf, "%d %d", prm.getCutRows(), prm.getCutColumns());
        fprintf(if_fp, " %-5s", buf);
        sprintf(buf, "%g %g", dbuToUU(prm.getCutSpacing().x()),
            dbuToUU(prm.getCutSpacing().y()));
        fprintf(if_fp, " %-12s", buf);
        sprintf(buf, "%g %g", dbuToUU(prm.getLayer1Enc().x()),
            dbuToUU(prm.getLayer1Enc().y()));
        fprintf(if_fp, " %-12s", buf);
        sprintf(buf, "%g %g", dbuToUU(prm.getLayer2Enc().x()),
            dbuToUU(prm.getLayer2Enc().y()));
        fprintf(if_fp, " %s \\\n", buf);

        sprintf(buf, "%g %g", dbuToUU(prm.getLayer1Offset().x()),
            dbuToUU(prm.getLayer1Offset().y()));
        fprintf(if_fp, "    %-12s", buf);
        sprintf(buf, "%g %g", dbuToUU(prm.getLayer2Offset().x()),
            dbuToUU(prm.getLayer2Offset().y()));
        fprintf(if_fp, " %-12s", buf);
        sprintf(buf, "%g %g", dbuToUU(prm.getOriginOffset().x()),
            dbuToUU(prm.getOriginOffset().y()));
        fprintf(if_fp, " %s", buf);

        oaPhysicalLayer *imp1 = stvd->getImplant1();
        if (imp1) {
            sprintf(buf, "%g %g", dbuToUU(prm.getImplant1Enc().x()),
                dbuToUU(prm.getImplant1Enc().y()));
            oaString i1name;
            imp1->getName(i1name);
            fprintf(if_fp, " \\\n    %-16s %12s", (const char*)i1name, buf);
            oaPhysicalLayer *imp2 = stvd->getImplant2();
            if (imp2) {
                sprintf(buf, "%g %g",
                    dbuToUU(prm.getImplant2Enc().x()),
                    dbuToUU(prm.getImplant2Enc().y()));
                oaString i2name;
                imp2->getName(i2name);
                fprintf(if_fp, "  %-16s %12s", (const char*)i2name, buf);
            }
        }
        fputs("\n", if_fp);
    }
    // Else custom via, fixme
}


void
cOaTechIf::printGroup(const oaGroup *group)
{
    oaString name;
    group->getName(name);
    oaString dname;
    group->getDef()->getName(dname);
    fprintf(if_fp, "  %s %s\n", (const char*)name, (const char*)dname);

    printProperties(group, 4);
    printAppDef(group);
    printProperties(group->getDef(), 4);
    printAppDef(group->getDef());

    if (group->isEmpty())
        fprintf(if_fp, "EMPTY\n");

    oaIter<oaGroupMember> nIter1(group->getMembers());
    oaGroupMember *member;
    while ((member = nIter1.getNext()) != 0) {
        oaString prn = member->getObject()->getType().getName();

        switch (member->getObject()->getType()) {
        case oacIntPropType:
            {
                oaIntProp *prp = (oaIntProp*)member->getObject();
                prp->getName(name);
                fprintf(if_fp, "    (%s) %s %d\n", (const char*)prn,
                    (const char*)name, prp->getValue());
            }
            break;
        case oacFloatPropType:
            {
                oaFloatProp *prp = (oaFloatProp*)member->getObject();
                prp->getName(name);
                fprintf(if_fp, "    (%s) %s %g\n", (const char*)prn,
                    (const char*)name, prp->getValue());
            }
            break;
        case oacStringPropType:
            {
                oaStringProp *prp = (oaStringProp*)member->getObject();
                prp->getName(name);
                oaString val;
                prp->getValue(val);
                fprintf(if_fp, "    (%s) %s %s\n", (const char*)prn,
                    (const char*)name, (const char*)val);
            }
            break;
        case oacDoublePropType:
            {
                oaDoubleProp *prp = (oaDoubleProp*)member->getObject();
                prp->getName(name);
                fprintf(if_fp, "    (%s) %s %g\n", (const char*)prn,
                    (const char*)name, prp->getValue());
            }
            break;
        case oacAppPropType:
            {
                oaAppProp *ap = (oaAppProp*)member->getObject();
                ap->getName(name);
                fprintf(if_fp, "    (%s) %s ", (const char*)prn,
                    (const char*)name);
                oaByteArray ary;
                ap->getValue(ary);

                for (unsigned int i = 0; i < ap->getSize(); i++) {
                    if (ary.get(i))
                        fputc(ary.get(i), if_fp);
                }
                fputc('\n', if_fp);
            }
            break;
        case oacAppObjectType:
            {
                oaAppObject *ao = (oaAppObject*)member->getObject();
                oaAppObjectDef *od = ao->getAppObjectDef();
                od->getName(name);
                fprintf(if_fp, "    (%s) %s\n", (const char*)prn,
                    (const char*)name);
            }
            break;
        case oacPhysicalLayerType:
            {
                oaPhysicalLayer *lyr = (oaPhysicalLayer*)member->getObject();
                lyr->getName(name);
                fprintf(if_fp, "    (%s) %s\n", (const char*)prn,
                    (const char*)name);
            }
            break;
        case oacDerivedLayerType:
            {
                oaDerivedLayer *dl = (oaDerivedLayer*)member->getObject();
                oaString l1name, l2name;
                dl->getLayer1()->getName(l1name);
                dl->getLayer2()->getName(l2name);
                oaLayerOp op = dl->getOperation();
                oaString opstr = op.getName();
                fprintf(if_fp, "    (%s) %s %s %s\n", (const char*)prn,
                    (const char*)l1name, (const char*)opstr,
                    (const char*)l2name);
            }
            break;
        case oacGroupType:
            {
                oaGroup *mgrp = (oaGroup*)member->getObject();
                if (mgrp == group)
                    continue;
                mgrp->getName(name);
                fprintf(if_fp, "    (%s) %s\n", (const char*)prn,
                    (const char*)name);
            }
            break;
        default:
            {
                fprintf(if_fp, "    (%s) NOT HANDLED\n", (const char*)prn);
            }
            break;
        }

        printProperties(member, 6);
        printAppDef(member);
    }
}


void
cOaTechIf::printConstraintGroup(const oaConstraintGroup *cgroup)
{
    oaString name;
    cgroup->getName(name);
    fprintf(if_fp, "  %s\n", (const char*)name);

    oaIter<oaConstraintGroupMem> nIter1(cgroup->getMembers());
    oaConstraintGroupMem *member;
    while ((member = nIter1.getNext()) != 0) {

        switch (member->getObject()->getType()) {
        case oacSimpleConstraintType:
            {
                oaSimpleConstraint *cs =
                    (oaSimpleConstraint*)member->getObject();
                oaSimpleConstraintDef *csdef
                    = (oaSimpleConstraintDef*)cs->getDef();
                csdef->getName(name);
                fprintf(if_fp, "    %s", (const char*)name);

                char *nm = getValueAsString(cs->getValue());
                fprintf(if_fp, " %s", nm);
                delete [] nm;
                oaString id, dsc;
                cs->getID(id);
                cs->getDescription(dsc);
                if (!id.isEmpty())
                    fprintf(if_fp, " %s", (const char*)id);
                if (!dsc.isEmpty())
                    fprintf(if_fp, " %s", (const char*)dsc);
                fputc('\n', if_fp);
                char *prms = getConstraintParamString(cs);
                if (prms) {
                    fprintf(if_fp, "        params\n%s", prms);
                    delete prms;
                }
            }
            break;
        case oacLayerConstraintType:
            {
                oaLayerConstraint *cs =
                    (oaLayerConstraint*)member->getObject();
                oaLayerConstraintDef *csdef =
                    (oaLayerConstraintDef*)cs->getDef();

                csdef->getName(name);
                oaLayer *lyr = oaLayer::find(if_tech, cs->getLayer());
                oaString lname;
                lyr->getName(lname);
                if (oaPurposeType::get(cs->getPurpose()) != oacNoPurposeType) {
                    oaPurpose *prp = oaPurpose::find(if_tech, cs->getPurpose());
                    oaString pname;
                    prp->getName(pname);
                    fprintf(if_fp, "    %s %s:%s", (const char*)name,
                        (const char*)lname, (const char*)pname);
                }
                else {
                    fprintf(if_fp, "    %s %s", (const char*)name,
                        (const char*)lname);
                }

                char *nm = getValueAsString(cs->getValue());
                fprintf(if_fp, " %s", nm);
                delete [] nm;
                oaString id, dsc;
                cs->getID(id);
                cs->getDescription(dsc);
                if (!id.isEmpty())
                    fprintf(if_fp, " %s", (const char*)id);
                if (!dsc.isEmpty())
                    fprintf(if_fp, " %s", (const char*)dsc);
                fputc('\n', if_fp);
                char *prms = getConstraintParamString(cs);
                if (prms) {
                    fprintf(if_fp, "        params\n%s", prms);
                    delete prms;
                }
            }
            break;
        case oacLayerPairConstraintType:
            {
                oaLayerPairConstraint *cs =
                    (oaLayerPairConstraint*)member->getObject();
                oaLayerPairConstraintDef *csdef =
                    (oaLayerPairConstraintDef*)cs->getDef();

                csdef->getName(name);
                oaLayer *lyr = oaLayer::find(if_tech, cs->getLayer1());
                oaString lname;
                lyr->getName(lname);
                if (oaPurposeType::get(cs->getPurpose1()) != oacNoPurposeType) {
                    oaPurpose *prp = oaPurpose::find(if_tech,
                        cs->getPurpose1());
                    oaString pname;
                    prp->getName(pname);
                    fprintf(if_fp, "    %s %s:%s", (const char*)name,
                        (const char*)lname, (const char*)pname);
                }
                else {
                    fprintf(if_fp, "    %s %s", (const char*)name,
                        (const char*)lname);
                }
                lyr = oaLayer::find(if_tech, cs->getLayer2());
                lyr->getName(lname);
                if (oaPurposeType::get(cs->getPurpose2()) != oacNoPurposeType) {
                    oaPurpose *prp = oaPurpose::find(if_tech,
                        cs->getPurpose2());
                    oaString pname;
                    prp->getName(pname);
                    fprintf(if_fp, " %s:%s", (const char*)lname,
                        (const char*)pname);
                }
                else {
                    fprintf(if_fp, " %s", (const char*)lname);
                }

                char *nm = getValueAsString(cs->getValue());
                fprintf(if_fp, " %s", nm);
                delete [] nm;
                oaString id, dsc;
                cs->getID(id);
                cs->getDescription(dsc);
                if (!id.isEmpty())
                    fprintf(if_fp, " %s", (const char*)id);
                if (!dsc.isEmpty())
                    fprintf(if_fp, " %s", (const char*)dsc);
                fputc('\n', if_fp);
                char *prms = getConstraintParamString(cs);
                if (prms) {
                    fprintf(if_fp, "        params\n%s", prms);
                    delete prms;
                }
            }
            break;
        case oacLayerArrayConstraintType:
            {
                oaLayerArrayConstraint *cs =
                    (oaLayerArrayConstraint*)member->getObject();
                oaLayerArrayConstraintDef *csdef =
                    (oaLayerArrayConstraintDef*)cs->getDef();

                csdef->getName(name);
                fprintf(if_fp, "    %s", (const char*)name);

                char *nm = getValueAsString(cs->getValue());
                fprintf(if_fp, " %s", nm);
                delete [] nm;
                oaString id, dsc;
                cs->getID(id);
                cs->getDescription(dsc);
                if (!id.isEmpty())
                    fprintf(if_fp, " %s", (const char*)id);
                if (!dsc.isEmpty())
                    fprintf(if_fp, " %s", (const char*)dsc);
                fputc('\n', if_fp);
                char *prms = getConstraintParamString(cs);
                if (prms) {
                    fprintf(if_fp, "        params\n%s", prms);
                    delete prms;
                }
            }
            break;
        case oacConstraintGroupHeaderType:
            // Don't list these.
            break;
        case oacConstraintGroupType:
            {
                oaConstraintGroup *cg =
                    (oaConstraintGroup*)member->getObject();
                cg->getName(name);
                fprintf(if_fp, "    %s (cgroup)", (const char*)name);
            }
            break;
        default:
            {
                // Shouldn't get here.
                name = member->getObject()->getType().getName();
                fprintf(if_fp, "    %s\n", (const char*)name);
            }
            break;
        }
    }
}


void
cOaTechIf::printProperties(const oaObject *obj, int indent)
{
    oaIter<oaProp> iterProp(obj->getProps());
    oaProp *prop;
    while ((prop = iterProp.getNext()) != 0) {
        oaString name, value;
        if (prop->getType() == oacHierPropType) {
            prop->getName(name);
            fprintf(if_fp,  "%*c%s (hp)\n", indent, ' ', (const char*)name);
            printProperties(prop, indent + 2);
        }
        else {
            prop->getName(name);
            prop->getValue(value);
            fprintf(if_fp,  "%*c%s %s (p)\n", indent, ' ',
                (const char*)name, (const char*)value);
        }
    }
}


template <class T>
void
cOaTechIf::printAppDef(const T *obj)
{
    oaIter<oaAppDef> iterAppDef(obj->getAppDefs());
    oaAppDef *adef;
    while ((adef = iterAppDef.getNext()) != 0) {
        oaString name, value;
        adef->getName(name);
        switch (adef->getType()) {
        case oacBooleanAppDefType:
            {
                bool b = ((oaBooleanAppDef<T>*)adef)->get(obj);
                fprintf(if_fp,  "    %s %s (a)\n", (const char*)name,
                b ? "TRUE" : "FALSE");
            }
            break;
        case oacDataAppDefType:
            {
                value = adef->getType().getName();
                fprintf(if_fp,  "    %s %s (a)\n", (const char*)name,
                (const char*)value);
            }
            break;
        case oacDoubleAppDefType:
            {
                double d = ((oaDoubleAppDef<T>*)adef)->get(obj);
                fprintf(if_fp,  "    %s %g (a)\n", (const char*)name, d);
            }
            break;
        case oacFloatAppDefType:
            {
                float f = ((oaFloatAppDef<T>*)adef)->get(obj);
                fprintf(if_fp,  "    %s %g (a)\n", (const char*)name, f);
            }
            break;
        case oacIntAppDefType:
            {
                int i = ((oaIntAppDef<T>*)adef)->get(obj);
                fprintf(if_fp,  "    %s %d (a)\n", (const char*)name, i);
            }
            break;
        case oacInterPointerAppDefType:
            {
                oaObject *o = ((oaInterPointerAppDef<T>*)adef)->get(obj);
                value = o->getType().getName();
                fprintf(if_fp,  "    %s %s (a)\n", (const char*)name,
                    (const char*)value);
            }
            break;
        case oacIntraPointerAppDefType:
            {
                T *o = ((oaIntraPointerAppDef<T>*)adef)->get(obj);
                value = o->getType().getName();
                fprintf(if_fp,  "    %s %s (a)\n", (const char*)name,
                    (const char*)value);
            }
            break;
        case oacStringAppDefType:
            {
                oaString str;
                ((oaStringAppDef<T>*)adef)->get(obj, str);
                fprintf(if_fp,  "    %s %s (a)\n", (const char*)name,
                    (const char*)str);
            }
            break;
        case oacTimeAppDefType:
            {
                value = adef->getType().getName();
                fprintf(if_fp,  "    %s %s (a)\n", (const char*)name,
                    (const char*)value);
            }
            break;
        case oacVarDataAppDefType:
            {
                value = adef->getType().getName();
                fprintf(if_fp,  "    %s %s (a)\n", (const char*)name,
                    (const char*)value);
            }
            break;
        case oacVoidPointerAppDefType:
            {
                value = adef->getType().getName();
                fprintf(if_fp,  "    %s %s (a)\n", (const char*)name,
                    (const char*)value);
            }
            break;
        default:
            fprintf(if_fp,  "BAD TYPE\n");
        }

        printProperties(adef, 4);
    }
}


// Fill in the techLPP struct from the oaAppObject.  This is a Virtuoso
// extension.
//
bool
cOaTechIf::getCdsLpp(techLPP &lpp, const oaAppObject *lppobj)
{
    oaAppObjectDef *aodef = lppobj->getAppObjectDef();
    oaString aodname;
    aodef->getName(aodname);
    if (aodname != "techLPP")
        return (false);
    oaIter<oaAppDef> iterAppDef(lppobj->getAppDefs());
    oaAppDef *adef;
    while ((adef = iterAppDef.getNext()) != 0) {
        oaString name, value;
        adef->getName(name);
        if (name == "lppLayer") {
            lpp.layer = (oaLayer*)
                ((oaInterPointerAppDef<oaAppObject>*)adef)->get(lppobj);
        }
        else if (name == "lppLayerNum") {
            lpp.lnum = ((oaIntAppDef<oaAppObject>*)adef)->get(lppobj);
        }
        else if (name == "lppPurpose") {
            lpp.purpose = (oaPurpose*)
                ((oaInterPointerAppDef<oaAppObject>*)adef)->get(lppobj);
        }
        else if (name == "lppPurposeNum") {
            lpp.pnum = ((oaIntAppDef<oaAppObject>*)adef)->get(lppobj);
        }
        else if (name == "lppPacketName") {
            ((oaStringAppDef<oaAppObject>*)adef)->get(lppobj, lpp.packetName);
        }
        else if (name == "lppFlags") {
            lpp.flags = ((oaIntAppDef<oaAppObject>*)adef)->get(lppobj);
        }
    }
    return (true);
}


// Get the later and purpose names.  The oaLayer/oaPurpose may not
// exist for reserved numbers, set had_reserved in this case if a
// pointer is passed.
//
void
cOaTechIf::getLppNames(const techLPP &lpp, oaString &lname, oaString &pname,
    bool *had_reserved)
{
    if (had_reserved)
        *had_reserved = false;
    if (lpp.layer)
        lpp.layer->getName(lname);
    else {
        const char *t = virtReservedLayer(lpp.lnum);
        if (t) {
            lname = t;
            if (had_reserved)
                *had_reserved = true;
        }
        else
            lname.format("??? (%d)", lpp.lnum);
    }
    if (lpp.purpose)
        lpp.purpose->getName(pname);
    else {
        const char *t = virtReservedPurpose(lpp.pnum);
        if (t) {
            pname = t;
            if (had_reserved)
                *had_reserved = true;
        }
        else
            pname.format("??? (%d)", lpp.pnum);
    }
}


// Get the name of the constraint group member.
//
void
cOaTechIf::getConstraintGroupMemName(const oaConstraintGroupMem *member,
    oaString &name)
{
    oaObject *obj = member->getObject();
    switch (obj->getType()) {
    case oacSimpleConstraintType:
        {
            oaSimpleConstraint *cs = (oaSimpleConstraint*)obj;
            ((oaSimpleConstraintDef*)cs->getDef())->getName(name);
        }
        break;
    case oacLayerConstraintType:
        {
            oaLayerConstraint *cs = (oaLayerConstraint*)obj;
            ((oaLayerConstraintDef*)cs->getDef())->getName(name);
        }
        break;
    case oacLayerPairConstraintType:
        {
            oaLayerPairConstraint *cs = (oaLayerPairConstraint*)obj;
            ((oaLayerPairConstraintDef*)cs->getDef())->getName(name);
        }
        break;
    case oacLayerArrayConstraintType:
        {
            oaLayerArrayConstraint *cs = (oaLayerArrayConstraint*)obj;
            ((oaLayerArrayConstraintDef*)cs->getDef())->getName(name);
        }
        break;
    case oacConstraintGroupType:
        {
            ((oaConstraintGroup*)member->getObject())->getName(name);
        }
    default:
        break;
    }
}


// Return true if the member is an antenna array with a nonzero
// element for model.
//
bool
cOaTechIf::hasAntennaMod(const oaConstraintGroupMem *member, int model)
{
    oaObject *obj = member->getObject();
    oaValue *val = 0;
    switch (obj->getType()) {
    case oacSimpleConstraintType:
        {
            oaSimpleConstraint *cs = (oaSimpleConstraint*)obj;
            val = cs->getValue();
        }
        break;
    case oacLayerConstraintType:
        {
            oaLayerConstraint *cs = (oaLayerConstraint*)obj;
            val = cs->getValue();
        }
        break;
    default:
        return (false);
    }
    if (val && val->getType() == oacAntennaRatioArrayValueType) {
        oaAntennaRatioArrayValue *va = (oaAntennaRatioArrayValue*)val;
        switch (model) {
        case 0:
            return (va->get(oacDefaultAntennaModel) != 0);
        case 1:
            return (va->get(oacSecondAntennaModel) != 0);
            break;
        case 2:
            return (va->get(oacThirdAntennaModel) != 0);
            break;
        case 3:
            return (va->get(oacFourthAntennaModel) != 0);
        default:
            break;
        }
    }
    return (false);
}


// Return true is the constraint group member references a table.
//
bool
cOaTechIf::hasTable(const oaConstraintGroupMem *member)
{
    oaObject *obj = member->getObject();
    switch (obj->getType()) {
    case oacSimpleConstraintType:
    case oacLayerConstraintType:
    case oacLayerPairConstraintType:
    case oacLayerArrayConstraintType:
        {
            oaValue *v = ((oaConstraint*)obj)->getValue();
            switch (v->getType()) {
            case oacDualInt1DTblValueType:
            case oacFlt1DTblValueType:
            case oacFlt2DTblValueType:
            case oacFltIntFltTblValueType:
            case oacInt1DTblValueType:
            case oacInt2DTblValueType:
            case oacIntDualIntArrayTblValueType:
            case oacIntFltTblValueType:
            case oacIntRangeArray1DTblValueType:
            case oacIntRangeArray2DTblValueType:
            case oacViaDef2DTblValueType:
            case oacViaTopology2DTblValueType:
                return (true);
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
    return (false);
}


// Return a listing of parameters and values for the constraint, null
// if constraint has no parameters.
//
char *
cOaTechIf::getConstraintParamString(const oaConstraint *cns)
{
    if (!cns->hasParams())
        return (0);
    sLstr lstr;
    oaConstraintParamArray pa;
    cns->getParams(pa);
    int n = pa.getNumElements();
    for (int i = 0; i < n; i++) {
        oaConstraintParam *p = pa.get(i);
        oaConstraintParamDef *pd = p->getDef();
        oaString pnm;
        pd->getName(pnm);
        lstr.add("        '");
        lstr.add((const char*)pnm);
        int plen = pnm.getLength();
        while (plen < 19) {
            plen++;
            lstr.add_c(' ');
        }
        lstr.add_c(' ');
        int ccode = 0;
        if (pnm == "distance" || pnm == "length" || pnm == "width")
            ccode = 1;

        char *s = getValueAsString(p->getValue(), ccode);
        lstr.add(s);
        lstr.add_c('\n');
        delete [] s;
    }
    return (lstr.string_trim());
}


// Return a string giving the value.  If nonzero, the ccode causes
// integer values to be converted to from DBU to UU.
// ccode: 1:  All integer values to UU (distance)
//        2:  Scalar integer to UU squared (area)
//        3:  In 1D tables, integer abscissa to UU.
//
char *
cOaTechIf::getValueAsString(const oaValue *value, int ccode)
{
    switch (value->getType()) {
    case oacAntennaRatioArrayValueType:
        return (getAntennaRatioArrayValueAsString(
            (oaAntennaRatioArrayValue*)value, ccode));
    case oacAntennaRatioValueType:
        return (getAntennaRatioValueAsString((oaAntennaRatioValue*)value));
    case oacBooleanValueType:
        return (getBooleanValueAsString((oaBooleanValue*)value));
    case oacBoxArrayValueType:
        return (getBoxArrayValueAsString((oaBoxArrayValue*)value));
    case oacDualInt1DTblValueType:
        return (getDualInt1DTblValueAsString((oaDualInt1DTblValue*)value));
    case oacDualIntValueType:
        return (getDualIntValueAsString((oaDualIntValue*)value, ccode));
    case oacFlt1DTblValueType:
        return (getFlt1DTblValueAsString((oaFlt1DTblValue*)value));
    case oacFlt2DTblValueType:
        return (getFlt2DTblValueAsString((oaFlt2DTblValue*)value));
    case oacFltIntFltTblValueType:
        return (getFltIntFltTblValueAsString((oaFltIntFltTblValue*)value));
    case oacFltValueType:
        return (getFltValueAsString((oaFltValue*)value));
    case oacInt1DTblValueType:
        return (getInt1DTblValueAsString((oaInt1DTblValue*)value, ccode));
    case oacInt2DTblValueType:
        return (getInt2DTblValueAsString((oaInt2DTblValue*)value, ccode));
    case oacIntDualIntArrayTblValueType:
        return (getIntDualIntArrayTblValueAsString(
            (oaIntDualIntArrayTblValue*)value));
    case oacIntFltTblValueType:
        return (getIntFltTblValueAsString((oaIntFltTblValue*)value, ccode));
    case oacIntRangeArray1DTblValueType:
        return (getIntRangeArray1DTblValueAsString(
            (oaIntRangeArray1DTblValue*)value));
    case oacIntRangeArray2DTblValueType:
        return (getIntRangeArray2DTblValueAsString(
            (oaIntRangeArray2DTblValue*)value));
    case oacIntRangeArrayValueType:
        return (getIntRangeArrayValueAsString((oaIntRangeArrayValue*)value));
    case oacIntRangeValueType:
        return (getIntRangeValueAsString((oaIntRangeValue*)value));
    case oacIntValueType:
        return (getIntValueAsString((oaIntValue*)value, ccode));
    case oacLayerArrayValueType:
        return (getLayerArrayValueAsString((oaLayerArrayValue*)value));
    case oacLayerValueType:
        return (getLayerValueAsString((oaLayerValue*)value));
    case oacPurposeValueType:
        return (getPurposeValueAsString((oaPurposeValue*)value));
    case oacUInt8RangeValueType:
        return (getUInt8RangeValueAsString((oaUInt8RangeValue*)value));
    case oacUInt8ValueType:
        return (getUInt8ValueAsString((oaUInt8Value*)value));
    case oacValueArrayValueType:
        return (getValueArrayValueAsString((oaValueArrayValue*)value, ccode));
    case oacViaDef2DTblValueType:
        return (getViaDef2DTblValueAsString((oaViaDef2DTblValue*)value));
    case oacViaDefArrayValueType:
        return (getViaDefArrayValueAsString((oaViaDefArrayValue*)value));
    case oacViaTopology2DTblValueType:
        return (getViaTopology2DTblValueAsString(
            (oaViaTopology2DTblValue*)value));
    case oacViaTopologyArrayValueType:
        return (getViaTopologyArrayValueAsString(
            (oaViaTopologyArrayValue*)value));

    default:
        {
            oaString name;
            name = value->getType().getName();
            return (lstring::copy(name));
        }
        break;
    }
}


char *
cOaTechIf::getAntennaRatioArrayValueAsString(
    const oaAntennaRatioArrayValue *aav, int ccode)
{
    oaAntennaRatioValue *av = 0;
    switch (ccode) {
    case 0:
        av = aav->get(oacDefaultAntennaModel);
        break;
    case 1:
        av = aav->get(oacSecondAntennaModel);
        break;
    case 2:
        av = aav->get(oacThirdAntennaModel);
        break;
    case 3:
        av = aav->get(oacFourthAntennaModel);
    default:
        break;
    }
    if (av)
        return (getAntennaRatioValueAsString(av));
    return (0);
}

char *
cOaTechIf::getAntennaRatioValueAsString(const oaAntennaRatioValue *av)
{
    char buf[64];

    sLstr lstr;
    lstr.add("       ( areaRatio    ");
    lstr.add_c(' ');
    lstr.add_g(av->getGate());
    if (av->isSide()) {
        lstr.add_c(' ');
        lstr.add("'side");
    }
    lstr.add(" )\n");

    oa1DLookupTbl<oaInt8, oaFloat> tbl;
    av->getDiode(tbl);
    int tsize = tbl.getNumItems();
    if (tsize > 0 || tbl.getDefaultValue() != 0.0) {
        lstr.add("        ( diffAreaRatio");
        if (tsize == 0) {
            lstr.add_c(' ');
            lstr.add_g(tbl.getDefaultValue());
        }
        else {
            lstr.add(" (");
            for (int i = 0; i < tsize; i++) {
                sprintf(buf, "(%g %g)", dbuToUU(tbl.getHeader(i)),
                    tbl.getValue(i));
                if (i)
                    lstr.add_c(' ');
                lstr.add(buf);
            }
            lstr.add_c(')');
        }
        if (av->isSide())
            lstr.add(" 'side");
        lstr.add(" )\n");
    }
    return (lstr.string_trim());
}

char *
cOaTechIf::getBooleanValueAsString(const oaBooleanValue *b)
{
    return (lstring::copy(b->get() ? "t" : "nil"));
}

char *
cOaTechIf::getBoxArrayValueAsString(const oaBoxArrayValue *)
{
    return (lstring::copy(""));
}

char *
cOaTechIf::getDualInt1DTblValueAsString(const oaDualInt1DTblValue *)
{
    return (lstring::copy(""));
}

char *
cOaTechIf::getDualIntValueAsString(const oaDualIntValue *i2, int ccode)
{
    char buf[64];
    if (ccode == 1) {
        sprintf(buf, "(%g %g)", dbuToUU(i2->getFirst()),
            dbuToUU(i2->getSecond()));
    }
    else
        sprintf(buf, "(%d %d)", i2->getFirst(), i2->getSecond());
    return (lstring::copy(buf));
}

char *
cOaTechIf::getFlt1DTblValueAsString(const oaFlt1DTblValue *t)
{
    char buf[64];
    oa1DLookupTbl<oaFloat, oaFloat> tbl;
    t->get(tbl);
    int tsize = tbl.getNumItems();
    oaString name = tbl.getName();
    sprintf(buf, "( \"%s\" nil nil )", (const char*)name);
    sLstr lstr;
    lstr.add("\n        (");
    lstr.add(buf);
    lstr.add("   ");
    lstr.add_g(tbl.getDefaultValue());
    lstr.add(" )\n        (\n");
    for (int i = 0; i < tsize; i++) {
        sprintf(buf, "          %-8g %-8g\n", tbl.getHeader(i),
            tbl.getValue(i));
        lstr.add(buf);
    }
    lstr.add("        )\n");
    return (lstr.string_trim());
}

char *
cOaTechIf::getFlt2DTblValueAsString(const oaFlt2DTblValue *t)
{
    char buf[64];
    oa2DLookupTbl<oaFloat, oaFloat, oaFloat> tbl;
    t->get(tbl);
    int rows = tbl.getNumRows();
    int cols = tbl.getNumCols();
    oaString rowname = tbl.getRowName();
    oaString colname = tbl.getColName();
    sprintf(buf, "( \"%s\" nil nil \"%s\" nil nil )", (const char*)rowname,
        (const char*)colname);
    sLstr lstr;
    lstr.add("\n        (");
    lstr.add(buf);
    lstr.add("   ");
    lstr.add_g(tbl.getDefaultValue());
    lstr.add(" )\n        (\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            sprintf(buf, "          (%-8g %-8g) %-8g\n", tbl.getRowHeader(i),
                tbl.getColHeader(j), tbl.getValue(i, j));
            lstr.add(buf);
        }
    }
    lstr.add("        )\n");
    return (lstr.string_trim());
}

char *
cOaTechIf::getFltIntFltTblValueAsString(const oaFltIntFltTblValue *t)
{
    char buf[64];
    oa2DLookupTbl<oaFloat, oaInt4, oaFloat> tbl;
    t->get(tbl);
    int rows = tbl.getNumRows();
    int cols = tbl.getNumCols();
    oaString rowname = tbl.getRowName();
    oaString colname = tbl.getColName();
    sprintf(buf, "( \"%s\" nil nil \"%s\" nil nil )", (const char*)rowname,
        (const char*)colname);
    sLstr lstr;
    lstr.add("\n        (");
    lstr.add(buf);
    lstr.add("   ");
    lstr.add_g(tbl.getDefaultValue());
    lstr.add(" )\n        (\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            sprintf(buf, "          (%-8g %-8d) %-8g\n", tbl.getRowHeader(i),
                tbl.getColHeader(j), tbl.getValue(i, j));
            lstr.add(buf);
        }
    }
    lstr.add("        )\n");
    return (lstr.string_trim());
}

char *
cOaTechIf::getFltValueAsString(const oaFltValue *f)
{
    char buf[32];
    sprintf(buf, "%g", f->get());
    return (lstring::copy(buf));
}

char *
cOaTechIf::getInt1DTblValueAsString(const oaInt1DTblValue *t, int ccode)
{
    char buf[64];
    oa1DLookupTbl<oaInt4, oaInt4> tbl;
    t->get(tbl);
    int tsize = tbl.getNumItems();
    oaString name = tbl.getName();
    sprintf(buf, "( \"%s\" nil nil )", (const char*)name);
    sLstr lstr;
    lstr.add("\n        (");
    lstr.add(buf);
    lstr.add("   ");
    if (ccode == 1)
        lstr.add_g(dbuToUU(tbl.getDefaultValue()));
    else
        lstr.add_i(tbl.getDefaultValue());
    lstr.add(" )\n        (\n");
    for (int i = 0; i < tsize; i++) {
        if (ccode == 1) {
            sprintf(buf, "          %-8g %-8g\n", dbuToUU(tbl.getHeader(i)),
                dbuToUU(tbl.getValue(i)));
        }
        else if (ccode == 3) {
            sprintf(buf, "          %-8g %-8d\n", dbuToUU(tbl.getHeader(i)),
                tbl.getValue(i));
        }
        else {
            sprintf(buf, "          %-8d %-8d\n", tbl.getHeader(i),
                tbl.getValue(i));
        }
        lstr.add(buf);
    }
    lstr.add("        )\n");
    return (lstr.string_trim());
}

char *
cOaTechIf::getInt2DTblValueAsString(const oaInt2DTblValue *t, int ccode)
{
    char buf[64];
    oa2DLookupTbl<oaInt4, oaInt4, oaInt4> tbl;
    t->get(tbl);
    int rows = tbl.getNumRows();
    int cols = tbl.getNumCols();
    oaString rowname = tbl.getRowName();
    oaString colname = tbl.getColName();
    sprintf(buf, "( \"%s\" nil nil \"%s\" nil nil )", (const char*)rowname,
        (const char*)colname);
    sLstr lstr;
    lstr.add("\n        (");
    lstr.add(buf);
    lstr.add("   ");
    if (ccode == 1)
        lstr.add_g(dbuToUU(tbl.getDefaultValue()));
    else
        lstr.add_i(tbl.getDefaultValue());
    lstr.add(" )\n        (\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (ccode == 1) {
                sprintf(buf, "          (%-8g %-8g) %-8g\n",
                    dbuToUU(tbl.getRowHeader(i)), dbuToUU(tbl.getColHeader(j)),
                    dbuToUU(tbl.getValue(i, j)));
            }
            else {
                sprintf(buf, "          (%-8d %-8d) %-8d\n",
                    tbl.getRowHeader(i), tbl.getColHeader(j),
                    tbl.getValue(i, j));
            }
            lstr.add(buf);
        }
    }
    lstr.add("        )\n");
    return (lstr.string_trim());
}

char *
cOaTechIf::getIntDualIntArrayTblValueAsString(const oaIntDualIntArrayTblValue *)
{
    return (lstring::copy(""));
}

char *
cOaTechIf::getIntFltTblValueAsString(const oaIntFltTblValue *t, int ccode)
{
    char buf[64];
    oa1DLookupTbl<oaInt4, oaFloat> tbl;
    t->get(tbl);
    int tsize = tbl.getNumItems();
    oaString name = tbl.getName();
    sprintf(buf, "( \"%s\" nil nil )", (const char*)name);
    sLstr lstr;
    lstr.add("\n        (");
    lstr.add(buf);
    lstr.add("   ");
    lstr.add_g(tbl.getDefaultValue());
    lstr.add(" )\n        (\n");
    for (int i = 0; i < tsize; i++) {
        if (ccode == 1 || ccode == 3)
            sprintf(buf, "          %-8g %-8g\n", dbuToUU(tbl.getHeader(i)),
                tbl.getValue(i));
        else {
            sprintf(buf, "          %-8d %-8g\n", tbl.getHeader(i),
                tbl.getValue(i));
        }
        lstr.add(buf);
    }
    lstr.add("        )\n");
    return (lstr.string_trim());
}

char *
cOaTechIf::getIntRangeArray1DTblValueAsString(const oaIntRangeArray1DTblValue *)
{
    return (lstring::copy(""));
}

char *
cOaTechIf::getIntRangeArray2DTblValueAsString(const oaIntRangeArray2DTblValue *)
{
    return (lstring::copy(""));
}

char *
cOaTechIf::getIntRangeArrayValueAsString(const oaIntRangeArrayValue *)
{
    return (lstring::copy(""));
}

char *
cOaTechIf::getIntRangeValueAsString(const oaIntRangeValue *)
{
    return (lstring::copy(""));
}

char *
cOaTechIf::getIntValueAsString(const oaIntValue *i, int ccode)
{
    char buf[32];
    if (ccode == 1) {
        // Given DBU, return UU.
        double uu = dbuToUU(i->get());
        sprintf(buf, "%g", uu);
    }
    else if (ccode == 2) {
        // Given DBU^2, return UU^2
        double sc = dbuToUU(1);
        double uu = dbuToUU(i->get());
        sprintf(buf, "%g", sc*uu);
    }
    else
        sprintf(buf, "%d", i->get());
    return (lstring::copy(buf));
}

char *
cOaTechIf::getLayerArrayValueAsString(const oaLayerArrayValue *lav)
{
    oaLayerArray la;
    lav->get(la);
    sLstr lstr;
    lstr.add_c('(');
    for (unsigned int i = 0; i < la.getNumElements(); i++) {
        lstr.add_c(' ');

        oaString lname;
        oaLayer *layer = oaLayer::find(if_tech, la.get(i));
        if (layer)
            layer->getName(lname);
        else {
            const char *t = virtReservedLayer(la.get(i));
            if (t)
                lname = t;
            else 
                lname.format("??? (%d)", la.get(i));
        }
        lstr.add((const char*)lname);
    }
    lstr.add_c(' ');
    lstr.add_c(')');
    return (lstr.string_trim());
}

char *
cOaTechIf::getLayerValueAsString(const oaLayerValue *n)
{
    oaString lname;
    oaLayer *layer = oaLayer::find(if_tech, n->get());
    if (layer)
        layer->getName(lname);
    else {
        const char *t = virtReservedLayer(n->get());
        if (t)
            lname = t;
        else 
            lname.format("??? (%d)", n->get());
    }
    return (lstring::copy((const char*)lname));
}

char *
cOaTechIf::getPurposeValueAsString(const oaPurposeValue *n)
{
    oaString pname;
    oaPurpose *purpose = oaPurpose::find(if_tech, n->get());
    if (purpose)
        purpose->getName(pname);
    else {
        const char *t = virtReservedPurpose(n->get());
        if (t)
            pname = t;
        else 
            pname.format("??? (%d)", n->get());
    }
    return (lstring::copy((const char*)pname));
}

char *
cOaTechIf::getUInt8RangeValueAsString(const oaUInt8RangeValue *)
{
    return (lstring::copy(""));
}

char *
cOaTechIf::getUInt8ValueAsString(const oaUInt8Value *n)
{
    char buf[64];
    sprintf(buf, "%lld",  n->get());
    return (lstring::copy(buf));
}

char *
cOaTechIf::getValueArrayValueAsString(const oaValueArrayValue *n, int ccode)
{
    oaValueArray a;
    n->get(a);
    sLstr lstr;
    lstr.add_c('(');
    for (oaUInt4 i = 0; i < a.getNumElements(); i++) {
        lstr.add_c(' ');
        char *s = getValueAsString(a.get(i), ccode);
        lstr.add(s);
        delete [] s;
    }
    lstr.add(" )");
    return (lstr.string_trim());
}

char *
cOaTechIf::getViaDef2DTblValueAsString(const oaViaDef2DTblValue *)
{
    return (lstring::copy(""));
}

char *
cOaTechIf::getViaDefArrayValueAsString(const oaViaDefArrayValue *n)
{
    oaViaDefNameArray names;
    n->getNames(names);
    sLstr lstr;
    lstr.add_c('(');
    for (oaUInt4 i = 0; i < names.getNumElements(); i++) {
        lstr.add_c(' ');
        lstr.add((const char*)names.get(i));
    }
    lstr.add(" )");
    return (lstr.string_trim());
}

char *
cOaTechIf::getViaTopology2DTblValueAsString(const oaViaTopology2DTblValue *)
{
    return (lstring::copy(""));
}

char *
cOaTechIf::getViaTopologyArrayValueAsString(const oaViaTopologyArrayValue *)
{
    return (lstring::copy(""));
}


int
cOaTechIf::parse_token(const char *str)
{
    if (lstring::ciprefix("u", str))
        return (OATECH_PR_UNITS);
    if (lstring::ciprefix("an", str))
        return (OATECH_PR_ANLIBS);
    if (lstring::ciprefix("l", str))
        return (OATECH_PR_LAYERS);
    if (lstring::ciprefix("o", str))
        return (OATECH_PR_OPPTS);
    if (lstring::ciprefix("p", str))
        return (OATECH_PR_PURPS);
    if (lstring::ciprefix("si", str))
        return (OATECH_PR_STDEFS);
    if (lstring::ciprefix("va", str))
        return (OATECH_PR_VALUES);
    if (lstring::ciprefix("viad", str))
        return (OATECH_PR_VIADEFS);
    if (lstring::ciprefix("vias", str))
        return (OATECH_PR_VIASPECS);
    if (lstring::ciprefix("viav", str))
        return (OATECH_PR_VIAVARS);
    if (lstring::ciprefix("co", str))
        return (OATECH_PR_CNSTGRPS);
    if (lstring::ciprefix("cg", str))
        return (OATECH_PR_CNSTGRPS);
    if (lstring::ciprefix("cp", str))
        return (OATECH_PR_CNSTPRMS);
    if (lstring::ciprefix("d", str))
        return (OATECH_PR_DRVLPRMS);
    if (lstring::ciprefix("ap", str))
        return (OATECH_PR_APPOBJDEFS);
    if (lstring::ciprefix("g", str))
        return (OATECH_PR_GROUPS);

    if (lstring::ciprefix("al", str))
        return (-1);
    return (0);
}

