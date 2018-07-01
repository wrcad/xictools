
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

#ifndef OA_PROP_H
#define OA_PROP_H


// Name prefix for Xic properties.
#define OA_XICP_PFX "xic_"

struct PCellParam;
struct lispnode;
namespace { struct sPrmList; }


// Struct to hold info about devices and subcircuits obtained from the
// Cadence Virtuoso CDF and properties.  These are saved in a hash
// table by cell name.
//
class cOAelecInfo
{
public:
    // Array element for parameter lists.
    struct param
    {
        friend class cOAelecInfo;

        const char *name()  const { return (p_name); }
        const char *value() const { return (p_value); }

    private:
        char *p_name;
        char *p_value;
    };

    cOAelecInfo();
    ~cOAelecInfo();

    const char *name()              const { return (cdf_name); }
    const char *prefix()            const { return (cdf_prefix); }
    const char *simulator()         const { return (cdf_simulator); }
    const param *inst_params()      const { return (cdf_inst_params); }
    const param *all_params()       const { return (cdf_all_params); }
    const char *const *terms()      const { return (cdf_terms); }
    const char *const *ports()      const { return (cdf_ports); }
    const char *const *propmap()    const { return (cdf_map); }

    const param *find_param(const char *pname) const
        {
            if (cdf_all_params && pname) {
                for (param *p = cdf_all_params; p->p_name; p++) {
                    if (!strcmp(pname, p->p_name))
                        return (p);
                }
            }
            return (0);
        }

    static void set_prp_info(const char*, const oaProp*, const oaProp*);
    static void parse_CDF(lispnode*, const oaCell*);
    static const cOAelecInfo *find(const char*);

    void create_inst_param_string(sLstr&, const sPrmList*) const;
    void create_all_param_string(sLstr&) const;

private:
    void add();
    void parse_instParameters(lispnode*);
    void parse_termOrder(lispnode*);
    void parse_propMapping(lispnode*);
    void parse_simulator(const char*, lispnode*);
    void parse_parameters(lispnode*);
    void parse_propList(lispnode*);
    void parse_simInfo(lispnode*);

    char *cdf_name;             // The device part name.
    char *cdf_prefix;           // The namePrefix (SPICE key).
    char *cdf_simulator;        // The simulator name.
    param *cdf_inst_params;     // Instance param list and defaults.
    param *cdf_all_params;      // All parameters found, with defaults.
    char **cdf_terms;           // Zero-terminated ordered terminal name list.
    char **cdf_ports;           // Zero-terminated ordered port name list.
    char **cdf_map;             // The propMapping, zero terminated.
    const oaCell *cdf_cell;     // Used during parse.

    static SymTab *cdf_tab;     // Table of CDF info structs.
};


class cOAprop
{
public:
    cOAprop(const char *layv, const char *schv, const char *symv,
            const char *prpv, bool fxic)
        {
            // The default view names, can't be null.
            p_def_layout = layv;
            p_def_schematic = schv;
            p_def_symbol = symv;
            p_def_dev_prop = prpv;

            p_from_xic = fxic;
        }

    ~cOAprop()
        {
        }

    bool fromXic()          const { return (p_from_xic); }

    CDp *handleProperties(const oaObject*, DisplayMode);

    static SymTab *getPropTab(oaDesign*);
    static PCellParam *getPcParameters(const oaParamArray&, SymTab*);
    static void savePcParameters(const PCellParam*, oaParamArray&);
    static void checkFixProperties(CDc*);
    static bool addPrptyLabel(CDc*, CDp*);
    static const cOAelecInfo *getCDFinfo(const oaCell*, const char*,
        const char*);

private:
    const char *p_def_layout;
    const char *p_def_schematic;
    const char *p_def_symbol;
    const char *p_def_dev_prop;
    bool p_from_xic;
};

#endif

