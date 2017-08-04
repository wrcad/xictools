
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

#ifndef TECH_H
#define TECH_H

#include "dsp_grid.h"
#include "tech_ret.h"

// We keep this here, to support keyword handling.
#define VA_Constrain45          "Constrain45"

#define TECH_BUFSIZE 2048
#define KEYBUFSIZ 40

#define TECH_DEFAULT "scmos"

// Name of file containing default fillpattern maps
#define TECH_STIPPLE_FILE "xic_stipples"

// Number of layer palette registers.
#define TECH_NUM_PALETTES 8

// Number of grid registers.
#define TECH_NUM_GRIDS 8

// Number of saved function key specifications.
#define TECH_NUM_FKEYS 12

// Number of default fillpattern maps.
#define TECH_MAP_SIZE 64

enum tBlkType { tBlkNone, tBlkElyr, tBlkPlyr, tBlkRule, tBlkDev, tBlkHcpy };

// Keep track of the context, so we can put comment lines in something
// close to the right place on update.
//
struct sTcx
{
    sTcx()
        {
            cx_subkey = 0;
            cx_key = 0;
            cx_blkname = 0;
            cx_type = tBlkNone;
        }

    sTcx(sTcx &cx)
        {
            cx_subkey = (cx.cx_subkey ? lstring::copy(cx.cx_subkey) : 0);
            cx_key = (cx.cx_key ? lstring::copy(cx.cx_key) : 0);
            cx_blkname = (cx.cx_blkname ? lstring::copy(cx.cx_blkname) : 0);
            cx_type = cx.cx_type;
        }

    void set_subkey(const char *kw)
        {
            char *s = lstring::copy(kw);
            delete [] cx_subkey;
            cx_subkey = s;
        }

    void set_key(const char *kw)
        {
            char *s = lstring::copy(kw);
            delete [] cx_key;
            cx_key = s;
        }

    void set_type(tBlkType t, const char *nm)
        {
            cx_type = t;
            char *s = lstring::copy(nm);
            delete [] cx_blkname;
            cx_blkname = s;
            delete [] cx_subkey;
            cx_subkey = 0;
        }

    void clear()
        {
            delete [] cx_subkey;
            cx_subkey = 0;
            delete [] cx_key;
            cx_key = 0;
            delete [] cx_blkname;
            cx_blkname = 0;
            cx_type = tBlkNone;
        }

    const char *subkey()        { return (cx_subkey); }
    const char *key()           { return (cx_key); }
    const char *blkname()       { return (cx_blkname); }
    tBlkType type()             { return (cx_type); }

private:
    char *cx_subkey;
    char *cx_key;
    char *cx_blkname;
    tBlkType cx_type;
};

// Context-sensitive storage for comment lines.
//
struct sTcomment
{
    sTcomment(sTcx *c, char *t) { next = 0; cx = c; text = t; dumped = false; }

    sTcomment *next;
    sTcx *cx;
    char *text;
    bool dumped;
};

// For fill patterns.
//
struct sTpmap
{
    sTpmap()                        { nx = ny = 0; map = 0; }

    sTpmap &operator=(const unsigned char c[8])
    {
        nx = ny = 8;
        map = new unsigned char[8];
        memcpy(map, c, 8);
        return (*this);
    }

    sTpmap &operator=(const unsigned short s[16])
    {
        nx = ny = 16;
        map = new unsigned char[32];
        memcpy(map, s, 32);
        return (*this);
    }

    short nx, ny;
    unsigned char *map;
};

// Layer stack reordering.
//
// None:    No reordering, strict layer table order.
// Vabove:  Move Via layers to follow largest index lower reference.
// Vbelow:  Move Via layers to just before smallest index upper reference.
//
enum tReorderMode
{
    tReorderNone,
    tReorderVabove,
    tReorderVbelow
};

// Ground plane inversion method.
enum { GPI_PLACE, GPI_TOP, GPI_ALL };
typedef unsigned char GPItype;

// Prototypes for application hooks.
typedef bool(*techEvalFunc)(char**, char*);
typedef TCret(*techAttrFunc)();
typedef TCret(*techBlkFunc)(FILE*);
typedef TCret(*techLyrBlkFunc)(CDl*);
typedef char*(*techLayerCheckFunc)(CDl*);
typedef void(*techPrintFunc)(FILE*);
typedef void(*techPrintLayerFunc)(FILE*, sLstr*, bool, const CDl*);
typedef void(*techHasStdViaFunc)(bool);

// How to print techfile keywords:
//   non-default values only,
//   comment default values,
//   print non-default and default values.
enum TCPRtype { TCPRnondef, TCPRcmt, TCPRall };

// Angle support (values are important!)
enum tAngleMode { tManhattan = 0, t45s = 1, tAllAngles = 2 };

struct sAttrContext;
struct sLayerAttr;
struct sTderivedLyr;
struct sStdVia;
struct sVia;
struct ParseNode;
struct SImacroHandler;

// List element for standard via descriptions.
//
struct sStdViaList
{
    sStdViaList(const sStdVia *v, sStdViaList *n)
        {
            next = n;
            std_via = v;
        }

    static void destroy(sStdViaList *vl)
        {
            while (vl) {
                sStdViaList *vx = vl;
                vl = vl->next;
                delete vx;
            }
        }

    sStdViaList *next;
    const sStdVia *std_via;
};


// How to read a line with GetKeywordLine.
enum tReadMode { tReadNormal, tNoKeyword, tVerbatim };

inline class cTech *Tech();

// Main class for technology file management.
//
class cTech
{
    static cTech *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cTech *Tech() { return (cTech::ptr()); }

    // Table element,
    struct tc_nm_t
    {
        tc_nm_t(const char *n)
            {
                name = n;
                next = 0;
            }

        const char *tab_name()          { return (name); }
        tc_nm_t *tab_next()             { return (next); }
        tc_nm_t *tgen_next(bool)        { return (next); }
        void set_tab_next(tc_nm_t *n)   { next = n; }

        const char *name;
        tc_nm_t *next;
    };

    // Callback registrations.
    //
    techEvalFunc RegisterEvaluator(techEvalFunc e)
        {
            techEvalFunc last = tc_parse_eval;
            tc_parse_eval = e;
            return (last);
        }

    techBlkFunc RegisterUserRuleParser(techBlkFunc h)
        {
            techBlkFunc last = tc_parse_user_rule;
            tc_parse_user_rule = h;
            return (last);
        }

    techLyrBlkFunc RegisterLayerParser(techLyrBlkFunc h)
        {
            techLyrBlkFunc last = tc_parse_lyr_blk;
            tc_parse_lyr_blk = h;
            return (last);
        }

    techBlkFunc RegisterDeviceParser(techBlkFunc h)
        {
            techBlkFunc last = tc_parse_device;
            tc_parse_device = h;
            return (last);
        }

    techBlkFunc RegisterScriptParser(techBlkFunc h)
        {
            techBlkFunc last = tc_parse_script;
            tc_parse_script = h;
            return (last);
        }

    techAttrFunc RegisterAttributeParser(techAttrFunc h)
        {
            techAttrFunc last = tc_parse_attribute;
            tc_parse_attribute = h;
            return (last);
        }

    techLayerCheckFunc RegisterLayerCheck(techLayerCheckFunc c)
        {
            techLayerCheckFunc last = tc_layer_check;
            tc_layer_check = c;
            return (last);
        }

    techPrintFunc RegisterPrintUserRules(techPrintFunc p)
        {
            techPrintFunc last = tc_print_user_rules;
            tc_print_user_rules = p;
            return (last);
        }

    techPrintLayerFunc RegisterPrintRules(techPrintLayerFunc p)
        {
            techPrintLayerFunc last = tc_print_rules;
            tc_print_rules = p;
            return (last);
        }

    techPrintFunc RegisterPrintDevices(techPrintFunc p)
        {
            techPrintFunc last = tc_print_devices;
            tc_print_devices = p;
            return (last);
        }

    techPrintFunc RegisterPrintScripts(techPrintFunc p)
        {
            techPrintFunc last = tc_print_scripts;
            tc_print_scripts = p;
            return (last);
        }

    techPrintFunc RegisterPrintAttributes(techPrintFunc p)
        {
            techPrintFunc last = tc_print_attributes;
            tc_print_attributes = p;
            return (last);
        }

    techHasStdViaFunc RegisterHasStdVia(techHasStdViaFunc p)
        {
            techHasStdViaFunc last = tc_has_std_via;
            tc_has_std_via = p;
            return (last);
        }

    // Back-end for Print functions.
    //
    static void PutStr(FILE *fp, sLstr *lstr, const char *s)
        {
            if (lstr)
                lstr->add(s);
            else if (fp)
                fputs(s, fp);
        }

    static void PutChr(FILE *fp, sLstr *lstr, char c)
        {
            if (lstr)
                lstr->add_c(c);
            else if (fp)
                fputc(c, fp);
        }

    // Layer test function.
    //
    TCret CheckLD(bool no_elec = false) const
        {
            if (!tc_last_layer) {
                return (SaveError(
                    "%s: keyword invalid, no associated layer.", tc_kwbuf));
            }
            if (no_elec && tc_last_layer->elecIndex() >= 0) {
                return (SaveError(
                    "%s: keyword invalid in electrical layer.", tc_kwbuf));
            }
            return (TCnone);
        }

    // tech.cc
    cTech();
    char *GetFkey(int, unsigned int);

    // tech_attrib.cc
    TCret ParseAttributes(sAttrContext* = 0, bool = false);
    void RegisterBooleanAttribute(const char*);
    bool FindBooleanAttribute(const char*, const char**);
    void RegisterStringAttribute(const char*);
    bool FindStringAttribute(const char*, const char**);
    void PrintAttributes(FILE*, sAttrContext* = 0, const char* = 0);
    void PrintAttrVars(FILE*);

    // tech_convert.cc
    TCret ParseCvtLayerBlock();
    void PrintCvtLayerBlock(FILE*, sLstr*, bool, const CDl*, DisplayMode);

    // tech_extract.cc
    TCret ParseExtLayerBlock();
    void PrintExtLayerBlock(FILE*, sLstr*, bool, const CDl*);
    static sVia *AddVia(CDl*, const char*, const char*, ParseNode*);
    static bool AddContact(CDl*, const char*, ParseNode*);
    static char *ExtCheckLayerKeywords(CDl*);
    static double GetLayerRsh(const CDl*);
    static bool GetLayerTline(const CDl*, double*);
    bool ParseRouting(const CDl*, const char*);
    void WriteRouting(const CDl*, FILE*, sLstr*);

    // tech_hardcopy.cc
    void SetHardcopyFuncs(
        bool(*)(bool, int, bool, GRdraw*),
        int(*)(HCorientFlags, HClegType, GRdraw*),
        bool(*)(HCframeMode, GRobject, int*, int*, int*, int*, GRdraw*));
    TCret ParseHardcopy(FILE*);
    void PrintHardcopy(FILE*);
    sAttrContext *GetAttrContext(int, bool);
    sAttrContext *GetHCbakAttrContext();

    // tech_layer.cc
    CDll *sequenceLayers(bool(*)(const CDl*));

    // tech_parse.cc
    void InitPreParse();
    void InitPostParse();
    void BeginParse();
    void EndParse();
    void Parse(FILE*);
    bool ParseLine(const char*);
    const char *GetKeywordLine(FILE*, tReadMode = tReadNormal);
    bool GetBoolean(const char*);
    int GetInt(const char*);
    bool GetWord(const char**, char**);
    bool GetRgb(int*);
    bool GetFilled(CDl*);
    bool GetFilled(sLayerAttr*);
    bool GetPmap(const char**, unsigned char*, int*, int*);
    void Comment(const char*);
    int LineCount() const;
    void SetLineCount(int);
    void IncLineCount();
    char *SaveError(const char*, ...) const;
    bool ExpandLine(FILE*, tReadMode);

    // tech_stipples.cc
    void ReadDefaultStipples();
    const char *DumpDefaultStipples();

    // tech_variables.cc
    void SetTechVariable(const char*, const char*);
    void ClearTechVariable(const char*);
    stringlist *VarList();
    bool VarSubst(char*, char** = 0, int* = 0);
    bool EvaluateEval(char*, int* = 0);

    // tech_via.cc
    sStdVia *AddStdVia(const sStdVia&);
    sStdVia *FindStdVia(const char*);
    CDs *OpenViaSubMaster(const char*);
    sStdViaList *StdViaList();
    void StdViaReset(bool);

    // tech_write.cc
    void Print(FILE*);
    void PrintLayerBlock(FILE*, sLstr*, bool, const CDl*, DisplayMode);
    bool PrintPmap(FILE*, sLstr*, unsigned char*, int, int);
    void CommentDump(FILE*, sLstr*, tBlkType, const char*, const char*,
        const char* = 0);

    bool Matching(const char *str)
        {
            if (lstring::cimatch(str, tc_kwbuf)) {
                strcpy(tc_kwbuf, str);
                return (true);
            }
            return (false);
        }

    bool pcheck(FILE *fp, bool cond)
    {
        if (PrintMode() == TCPRall || cond)
            return (true);
        if (PrintMode() == TCPRcmt) {
            fputc('#', fp);
            fputc(' ', fp);
            return (true);
        }
        return (false);
    }

    // Misc. inlines for private member access.
    //
    void SetCmtSubkey(const char *kw)
        {
            tc_comment_cx.set_subkey(kw);
        }

    void SetCmtKey(const char *kw)
        {
            tc_comment_cx.set_key(kw);
        }

    void SetCmtType(tBlkType t, const char *nm)
        {
            tc_comment_cx.set_type(t, nm);
        }

    sTpmap *GetDefaultMap(int num)
        {
            if (num < 0 || num >= TECH_MAP_SIZE)
                return (0);
            return (&tc_default_maps[num]);
        }

    double MfgGrid()        const { return (GridDesc::mfg_grid(Physical)); }
    bool SetMfgGrid(double d)
        {
            if (d < 0.0 || d > GRD_MFG_GRID_MAX)
                return (false);
            GridDesc::set_mfg_grid(d, Physical);
            return (true);
        }

    const char *TechnologyName()    const { return (tc_technology_name); }

    const char *TechExtension()     const { return (tc_tech_ext); }
    void SetTechExtension(const char *e)
        {
            delete [] tc_tech_ext;
            tc_tech_ext = lstring::copy(e);
        }

    const char *TechFilename()      const { return (tc_tech_filename); }
    void SetTechFilename(const char *f)
        {
            delete [] tc_tech_filename;
            tc_tech_filename = lstring::copy(f);
        }

    const char *KwBuf()         const { return (tc_kwbuf); }
    const char *InBuf()         const { return (tc_inbuf); }
    const char *OrigBuf()       const { return (tc_origbuf); }

    void WriteBuf(const char *kw, const char *in, const char *org)
        {
            if (kw && tc_kwbuf)
                strcpy(tc_kwbuf, kw);
            if (in && tc_inbuf)
                strcpy(tc_inbuf, in);
            if (org && tc_origbuf)
                strcpy(tc_origbuf, org);
        }

    table_t<sStdVia> *StdViaTab() const { return (tc_std_vias); }

    const char *LayerPaletteReg(int i, DisplayMode m) const
        {
            if (i >= 0 && i < TECH_NUM_PALETTES)
                return (m == Physical ?
                    tc_phys_layer_palettes[i] : tc_elec_layer_palettes[i]);
            return (0);
        }
    void SetLayerPaletteReg(int i, DisplayMode m, const char *s)
        {
            if (i >= 0 && i < TECH_NUM_PALETTES) {
                char *nstr = lstring::copy(s);
                if (m == Physical) {
                    delete [] tc_phys_layer_palettes[i];
                    tc_phys_layer_palettes[i] = nstr;
                }
                else {
                    delete [] tc_elec_layer_palettes[i];
                    tc_elec_layer_palettes[i] = nstr;
                }
            }
        }

    int HcopyDriver()               const { return (tc_hcopy_driver); }
    void SetHcopyDriver(int d)            { tc_hcopy_driver = d; }

    int PhysHcFormat()              const { return (tc_phys_hc_format); }
    void SetPhysHcFormat(int f)           { tc_phys_hc_format = f; }
    int ElecHcFormat()              const { return (tc_elec_hc_format); }
    void SetElecHcFormat(int f)           { tc_elec_hc_format = f; }

    int DefaultPhysResol()          const { return (tc_default_phys_resol); }
    int DefaultPhysSnap()           const { return (tc_default_phys_snap); }

    TCPRtype PrintMode()            const { return (tc_print_mode); }
    void SetPrintMode(TCPRtype t)         { tc_print_mode = t; }

    tAngleMode AngleSupport()       const { return (tc_angle_mode); }
    void SetAngleSupport(tAngleMode m)    { tc_angle_mode = m; }

    const char *GetFkeyString(int n) const
        {
            if (n >= 0 && n < TECH_NUM_FKEYS)
                return (tc_fkey_strs[n]);
            return (0);
        }

    void SetFkeyString(int n, const char *str)
        {
            if (n >= 0 && n < TECH_NUM_FKEYS) {
                char *nstr = lstring::copy(str);
                delete [] tc_fkey_strs[n];
                tc_fkey_strs[n] = nstr;
            }
        }

    GridDesc *GridReg(int i, DisplayMode m)
        {
            if (i >= 0 && i < TECH_NUM_GRIDS)
                return (m == Physical ? &tc_phys_grids[i] : &tc_elec_grids[i]);
            return (0);
        }

    void SetGridReg(int i, const GridDesc &g, DisplayMode m)
        {
            if (i >= 0 && i < TECH_NUM_GRIDS) {
                if (m == Physical)
                    tc_phys_grids[i] = g;
                else
                    tc_elec_grids[i] = g;
            }
        }

    bool HaveTechfile()             const { return (tc_techfile_read); }

    bool IsConstrain45()                { return (tc_constrain45); }
    void SetConstrain45(bool b)
        {
            tc_constrain45 = b;
            if (tc_c45_callback)
                (*tc_c45_callback)();
        }
    void SetC45Callback(void(*cb)())    { tc_c45_callback = cb; }

    bool IsNoPlanarize()                { return (tc_lyr_no_planarize); }
    void SetNoPlanarize(bool b)         { tc_lyr_no_planarize = b; }
    tReorderMode ReorderMode()          { return (tc_lyr_reord_mode); }
    void SetReorderMode(tReorderMode m) { tc_lyr_reord_mode = m; }

    // Access functions, extraction package support.
    double AntennaTotal()               { return (tc_ext_antenna_total); }
    void SetAntennaTotal(double d)      { tc_ext_antenna_total = d; }
    double SubstrateEps()               { return (tc_ext_substrate_eps); }
    void SetSubstrateEps(double d)      { tc_ext_substrate_eps = d; }
    double SubstrateThickness()         { return (tc_ext_substrate_thick); }
    void SetSubstrateThickness(double d){ tc_ext_substrate_thick = d; }
    GPItype GroundPlaneMode()           { return (tc_ext_gp_mode); }
    void SetGroundPlaneMode(GPItype t)  { tc_ext_gp_mode = t; }
    bool IsInvertGroundPlane()          { return (tc_ext_invert_ground_plane); }
    void SetInvertGroundPlane(bool b)   { tc_ext_invert_ground_plane = b; }
    bool IsGroundPlaneGlobal()          { return (tc_ext_ground_plane_global); }
    void SetGroundPlaneGlobal(bool b)   { tc_ext_ground_plane_global = b; }

    static struct HCcb &HC() { return (tc_hccb); }

private:
    // tech_attrib.cc
    TCret parse_gridreg(const char*, int, DisplayMode);
    void print_boolean_attributes(FILE*);
    void print_string_attributes(FILE*);
    void print_grid_registers(FILE*);
    void print_func_keys(FILE*);
    void print_layer_palettes(FILE*);
    void print_fonts(FILE*);

    // tech_hardcopy.cc
    bool read_driver(FILE*);
    TCret dispatch_drvr(int, sAttrContext*, sLayerAttr**);
    void print_driver_layer_block(FILE*, const HCdesc*, const sLayerAttr*,
        DisplayMode);

    // tech_parse.cc
    TCret dispatch(FILE*);
    TCret dispatchLayerBlock();
    TCret checkLayerFinal(CDl*);
    bool getLineFromFile(FILE*, tReadMode);

    // tech_setif.cc
    void setupInterface();

    // tech_variables.cc
    void setupVariables();
    bool vset(const char*);
    bool bangvset(const char*);


    char *tc_technology_name;   // Name of technology.
    char *tc_tech_ext;          // Tech file extension.
    char *tc_tech_filename;     // Tech file path.
    char *tc_kwbuf;             // Keyword buffer for parser.
    char *tc_inbuf;             // Line buffer for parser.
    char *tc_origbuf;           // Line backup for parser.

    table_t<sStdVia> *tc_std_vias;
    CDl *tc_last_layer;         // Layer last read.

    int tc_phys_hc_format;      // Hard copy default driver index.
    int tc_elec_hc_format;      // Hard copy default driver index.
    int tc_hcopy_driver;        // The current driver index in hard copy mode.
    int tc_default_phys_resol;  // Default physical mode grid space parameter.
    int tc_default_phys_snap;   // Default physical mode grid snap parameter.

    tAngleMode tc_angle_mode;   // Angle support enum value.

    char *tc_phys_layer_palettes[TECH_NUM_PALETTES];
    char *tc_elec_layer_palettes[TECH_NUM_PALETTES];
                                // Saved layer lists for palette.

    SymTab *tc_variable_tab;    // Tech file variables defined.
    char *tc_fkey_strs[TECH_NUM_FKEYS];
                                    // Function key strings.
    SImacroHandler *tc_tech_macros; // Tech file macros.
    sTcomment *tc_comments;         // Comments list.
    sTcomment *tc_cmts_end;
    stringlist *tc_bangset_lines;   // !Set lines.
    ctable_t<tc_nm_t> *tc_battr_tab; // Binary attributes.
    ctable_t<tc_nm_t> *tc_sattr_tab; // String attributes.

    sAttrContext **tc_attr_array;   // HC driver attribute struct array.
    int tc_attr_array_size;         // Size of array.
    TCPRtype tc_print_mode;         // Print mode for techfile keywords.

    // Function pointers for caller additions.
    techEvalFunc tc_parse_eval;         // Evaluate expressions.
    techBlkFunc tc_parse_user_rule;     // Parse user design rule.
    techLyrBlkFunc tc_parse_lyr_blk;    // Parse layer block keyword.
    techBlkFunc tc_parse_device;        // Parse device definition.
    techBlkFunc tc_parse_script;        // Parse script definition.
    techAttrFunc tc_parse_attribute;    // Parse attribute keyword.
    techLayerCheckFunc tc_layer_check;  // Check layer, called at end.

    techPrintFunc tc_print_user_rules;  // Print user design rules (b4 phys layers).
    techPrintLayerFunc tc_print_rules;  // Print design rules in layer block.
    techPrintFunc tc_print_devices;     // Print after phys layers.
    techPrintFunc tc_print_scripts;     // Print after phys layers.
    techPrintFunc tc_print_attributes;  // Print attributes.

    techHasStdViaFunc tc_has_std_via;   // Call back when standard via added.

    void(*tc_c45_callback)();       // Notification when constrain45 changes.

    bool tc_techfile_read;          // True if techfile has been read.
    bool tc_no_line_num;            // Line numbers invalid.
    bool tc_constrain45;            // Poly/wire construction constraint.
    bool tc_lyr_no_planarize;       // Don't assume layers are planarizing.
    tReorderMode tc_lyr_reord_mode; // Layer sequence options.
    int  tc_parse_level;            // Parser nesting depth;

    // Extraction system support
    double tc_ext_antenna_total;    // Antenna ratio total.
    double tc_ext_substrate_eps;    // Substrate diel. const.
    double tc_ext_substrate_thick;  // Substrate thickness in microns
    CDl *tc_ext_ground_plane;       // Ground plane layer.
    GPItype tc_ext_gp_mode;         // Ground plane inversion method
    bool tc_ext_invert_ground_plane;// Use inverted ground plane
    bool tc_ext_ground_plane_global;// Clear-field ground plane layer


    GridDesc tc_phys_grids[TECH_NUM_GRIDS];
    GridDesc tc_elec_grids[TECH_NUM_GRIDS];
                                    // Grid storage registers.


    sTcx tc_comment_cx;             // Keep context for comment line placement.
    sTpmap tc_default_maps[TECH_MAP_SIZE];  // Default fillpatterns.

    static struct HCcb tc_hccb;

    static cTech *instancePtr;
};

#endif

