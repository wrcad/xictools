
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

#ifndef CD_PROPNUM_H
#define CD_PROPNUM_H


// Electrical Mode Properties
// See cd_property.h for listing and descriptions.


// These are values written by old releases that are still recognized
// in input but never written.
//
#define OLD_GDS_LABEL_SIZE_PROP 94
#define OLD_NODRC_PROP          (GDSII_PROPERTY_BASE + 14)
#define OLD_PATHTYPE_PROP       (GDSII_PROPERTY_BASE + 33)


// Keywords used in the XIC_CHD_REF property.
//
#define CHDKW_FILENAME  "filename"
#define CHDKW_CELLNAME  "cellname"
#define CHDKW_BOUND     "bound"
#define CHDKW_SCALE     "scale"
#define CHDKW_AFLAGS    "aflags"
#define CHDKW_APREFIX   "aprefix"
#define CHDKW_ASUFFIX   "asuffix"

class cCHD;

// Parser/composer for the XICP_CHD_REF property string.
//
// The string is composed of name=value pairs.  The known names are
//   CHDKW_FILENAME
//     Full path name to layour file.
//   CHDKW_CELLNAME
//     Top-level cell name.
//   CHDKW_BOUND
//     Bounding box (l,b,r,t)
//   CHDKW_SCALE
//     CHD scale factor, floating point value
//   CHDKW_AFLAGs
//     alias flags, integer value
//   CHDKW_APREFIX
//     alias prefix
//   CHDKW_ASUFFIX
//     alias suffix
//
struct sChdPrp
{
    sChdPrp(const cCHD *chd, const char*, const char*, const BBox*BB);
    sChdPrp(const char*);
    ~sChdPrp();

    char *compose();
    void scale_bb(double);
    unsigned int hash(unsigned int);
    bool operator==(const sChdPrp&) const;

    const char *get_path()          const { return (cp_path); }
    const char *get_cellname()      const { return (cp_cellname); }
    const char *get_alias_prefix()  const { return (cp_alias_prefix); }
    const char *get_alias_suffix()  const { return (cp_alias_suffix); }
    const BBox *get_bb()            const { return (&cp_BB); }
    double get_scale()              const { return (cp_scale); }
    int get_alias_flags()           const { return (cp_alias_flags); }

private:
    char *cp_path;
    char *cp_cellname;
    char *cp_alias_prefix;
    char *cp_alias_suffix;
    BBox cp_BB;
    double cp_scale;
    int cp_alias_flags;
};


// Xic reserves properties in the range 7000 - 7299 for internal use.
//
// 7000 - 7099   GDSII mappings
//               We use the property list of symbols to save library
//               information during conversion from GDSII.  The value
//               of the property is the numeric value of the GDSII
//               record type offset by 7000 (e.g. 7000 is the property
//               value describing the GDSII version number, 7002 is
//               the property value describing the GDSII library name,
//               etc.)
//
// 7100 - 7199   Internal properties
//               These are used to store information particular to Xic
//               such as grid parameters.
//
// 7200 - 7299   Pseudo-properties
//               These are not actually saved as properties, but the
//               values trigger geometrical changes if set or provide
//               geometrical data when read.  Properties with these
//               values are ignored in input files.

// The base values for the reserved properties are defined here.
//
#define GDSII_PROPERTY_BASE     7000
#define INTERNAL_PROPERTY_BASE  7100
#define PSEUDO_PROPERTY_BASE    7200


// Test for any internally-used property value.
//
inline bool
prpty_internal(int num)
{
    return (num >= GDSII_PROPERTY_BASE && num < GDSII_PROPERTY_BASE + 300);
}


// Test for gdsii property values.
//
inline bool
prpty_gdsii(int num)
{
    return (num >= GDSII_PROPERTY_BASE && num < GDSII_PROPERTY_BASE + 100);
}


// The Internal Properties.
//
enum
{
    // These are the "global properties" that are applied to top-level
    // cells to save certain attributes.
    //
    // Grid property, physical only: grid %d (spacing) %d (snap).
    //   XICP_GRID
    //
    // Plot and analysis commands, electrical only, arb. strings.
    //   XICP_RUN
    //   XICP_PLOT
    //   XICP_IPLOT
    //
    XICP_GRID = INTERNAL_PROPERTY_BASE,
    XICP_RUN,
    XICP_PLOT,
    XICP_IPLOT,

    // These are misc. internal properties.
    //
    // Instance name property (used by extract).
    //   XICP_INST
    //
    // Misc. flags for cells.
    //   XICP_FLAGS
    //
    // Cached measure data from extraction.
    //   XICP_MEASURES
    //
    // Conductor number from FastCap interface.
    //   XICP_CNDR
    //
    XICP_INST,
    XICP_FLAGS,
    XICP_MEASURES,
    XICP_CNDR,

    // This indicates that the cell is a reference to a cell hierarchy
    // digest.  This is used internally, and can be read from/written to
    // native cell files only.
    XICP_CHD_REF = INTERNAL_PROPERTY_BASE + 50,

    // This can be applied to physical cells, and indicates that the
    // cell should be flattened into its parent for extraction/LVS,
    // as if listed in the FlattenPrefix variable.
    XICP_EXT_FLATTEN = INTERNAL_PROPERTY_BASE + 51,

    // This can be added to physical boxes, polys, and wires in a
    // device body.  If found in the body, the device won't be merged.
    XICP_NOMERGE = INTERNAL_PROPERTY_BASE + 52,

    // Properties for instances obtained from OpenAccess standard and
    // custom vias.  OA Standard vias are converted to Xic standard vias.
    //
    XICP_STDVIA = INTERNAL_PROPERTY_BASE + 60,
    XICP_CSTMVIA,

    // Property from OpenAccess the we don't recognize.  We keep these
    // anyway.
    XICP_OA_UNKN = INTERNAL_PROPERTY_BASE + 64,

    // This is set to a space-separated list of net names, and can be
    // applied to physical cells.  This provides the connection
    // terminal names and ordering when electrical data are absent. 
    // The names must match name labels placed in the layout.
    XICP_TERM_ORDER = INTERNAL_PROPERTY_BASE + 68,

    // Specifies a terminal to a physical cell, there can be any number
    // of these,
    XICP_TERM,

    // This is written in all output from boxes, polygons, or wires
    // with the NoDrc flag set.  It is not applied on input but used
    // to set the NoDrc flag in boxes, polygons, and wires.
    // Also: OLD_NODRC_PROP
    //
    XICP_NODRC = INTERNAL_PROPERTY_BASE + 78,

    // This is written to labels in GDSII to save the width/height, not
    // applied on input but used when reading GDSII labels.
    // Also: OLD_GDS_LABEL_SIZE_PROP
    //
    XICP_GDS_LABEL_SIZE = INTERNAL_PROPERTY_BASE + 80,

    // Start of reserved properties
    //
    // The remaining properties are reserved for strictly internal use,
    // and are never written, read, or copied.
    //
    // Added to physical objects that are targets of a terminal's oset field
    //   XICP_TERM_OSET
    //
    // Temporary property added to cells in delete list
    //   XICP_CLABEL
    //
    XICP_TERM_OSET = INTERNAL_PROPERTY_BASE + 81,
    XICP_CLABEL,
    XICP_OA_ORIG,
    //
    // End of reserved properties (base + 84)

    // Abutment control properties.  These are similar to the Ciranova
    // pycAbutClass, pycAbutRules, pycAbutDirections, pycShapeName and
    // pycPinSize properties.
    //
    XICP_AB_CLASS = INTERNAL_PROPERTY_BASE + 85,
    XICP_AB_RULES,
    XICP_AB_DIRECTS,
    XICP_AB_SHAPENAME,
    XICP_AB_PINSIZE,
    XICP_AB_INST,
    XICP_AB_PRIOR,
    XICP_AB_COPY,

    // Properties for PCells (parameterized cells).
    // Property        Contents    Applies to: super sub inst objects
    // XICP_PC         name of super                 x   x
    // XICP_PC_SCRIPT  script text             x
    // XICP_PC_PARAMS  parameters              x     x   x
    // XICP_GRIP       stretch handle desc                    x
    //
    // The super-master PCell contains a script, default parameter
    // values optionally with parameter constraint strings.  When the
    // super-master is instantiated, the script is executed producing
    // a sub-master under a modified name, plus an instance of the
    // sub-master.  The instance contains the name of the super-master
    // and a copy of the parameters.
    //
    // Objects in a PCell may contain a Ciranova-style "grip" property
    // for stretch handles.  This may have other uses as well, so the
    // XICP_GRIP property is not exclusively a "PCell" property.
    //
    XICP_GRIP = INTERNAL_PROPERTY_BASE + 95,
    XICP_PC = INTERNAL_PROPERTY_BASE + 97,
    XICP_PC_PARAMS,
    XICP_PC_SCRIPT
};


// Test function for "global" properties.
//
inline bool
prpty_global(int num)
{
    return (num >= XICP_GRID && num <= XICP_IPLOT);
}


// Test function, returns true for properties that should be discarded on
// input or output.
//
inline bool
prpty_reserved(int num)
{
    return (num >= INTERNAL_PROPERTY_BASE + 81 &&
        num < INTERNAL_PROPERTY_BASE + 85);
}


// The Pseudo-Properties
//
// These are special property values that when applied to physical
// objects will modify the object, or when interrogated will return a
// parameter value.  They are not saved as properties in objects.
//
enum XprpVal
{
    XprpType = PSEUDO_PROPERTY_BASE,
    XprpBB,
    XprpLayer,
    XprpFlags,
    XprpState,
    XprpGroup,
    XprpCoords,
    XprpMagn,
    XprpWwidth,
    XprpWstyle,
    XprpText,
    XprpXform,
    XprpArray,
    XprpTransf,
    XprpName,
    XprpXY,
    // Add new properties to end so as not to change the numbers above,
    // to avoid screwing up user's scripts.
    XprpWidth,
    XprpHeight,
    XprpEnd
};

// XprpType               All objects, returns object type, not settable
// XprpBB                 All objects, returns bounding box component,
//                         settable for objects, not subcells.  When set,
//                         will stretch to object to new value
// XprpLayer              All objects, returns layer name, settable for
//                         objects, not subcells.  Moves object to new layer
// XprpFlags              All objects, returns flags set, settable
// XprpState              All objects, returns state, setable
// XprpGroup              All objects, return group number, settable
// XprpCoords             All objects, return coordinate list or BB, settable
// XprpMagn               All objects, returns magnification of instance,
//                         1.0 other objects, settable (sets instance magn,
//                         scales other objects
// XprpWwidth             Wires, returns wire width, settable
// XprpWstyle             Wires, returns wire end style, settable
// XprpText               Labels, returns label text, settable
// XprpXform              Labels, returns transform code, settable
// XprpArray              Instances, returns array params, settable
// XprpTransf             Instances, returns transform, settable
// XprpName               Instances, returns cell name, settable (replaces
//                         instance)
// XprpXY                 Labels and instances, get/set x,y.  Wires, and
//                         polys, get/set first vertex.  Boxes, LL vertex.
// XprpWidth              Similar to XprpBB, but width only
// XprpHeight             Similar to XprpBB, but height only


// Test function fpr pseudo-property number.
//
inline bool
prpty_pseudo(int num)
{
    return (num >= PSEUDO_PROPERTY_BASE && num < PSEUDO_PROPERTY_BASE + 100);
}

#endif

