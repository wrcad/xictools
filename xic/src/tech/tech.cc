
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

#include "dsp.h"
#include "tech.h"
#include "tech_kwords.h"
#include "tech_extract.h"


//
// Tech package constructor, initialization, and misc.
//

// Instantiate keyword table.
sTKW Tkw;

cTech *cTech::instancePtr = 0;


cTech::cTech()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cTech is already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    tc_technology_name              = 0;
    tc_vendor_name                  = 0;
    tc_process_name                 = 0;
    tc_tech_ext                     = 0;
    tc_tech_filename                = 0;
    tc_kwbuf                        = 0;
    tc_inbuf                        = 0;
    tc_origbuf                      = 0;

    tc_std_vias                     = 0;
    tc_last_layer                   = 0;

    tc_phys_hc_format               = 0;
    tc_elec_hc_format               = 0;
    tc_hcopy_driver                 = -1;
    tc_default_phys_resol           = CDphysResolution;
    tc_default_phys_snap            = 1;

    tc_angle_mode                   = tAllAngles;

    memset(tc_phys_layer_palettes, 0, TECH_NUM_PALETTES*sizeof(char*));
    memset(tc_elec_layer_palettes, 0, TECH_NUM_PALETTES*sizeof(char*));

    tc_variable_tab                 = 0;
    memset(tc_fkey_strs, 0, TECH_NUM_FKEYS*sizeof(char*));
    tc_tech_macros                  = 0;
    tc_comments                     = 0;
    tc_cmts_end                     = 0;
    tc_bangset_lines                = 0;
    tc_battr_tab                    = 0;
    tc_sattr_tab                    = 0;

    tc_attr_array                   = 0;
    tc_attr_array_size              = 0;
    tc_print_mode                   = TCPRnondef;

    tc_parse_eval                   = 0;
    tc_parse_user_rule              = 0;
    tc_parse_lyr_blk                = 0;
    tc_parse_device                 = 0;
    tc_parse_script                 = 0;
    tc_parse_attribute              = 0;
    tc_layer_check                  = 0;

    tc_print_user_rules             = 0;
    tc_print_rules                  = 0;
    tc_print_devices                = 0;
    tc_print_scripts                = 0;
    tc_print_attributes             = 0;

    tc_has_std_via                  = 0;

    tc_c45_callback                 = 0;

    tc_techfile_read                = false;
    tc_no_line_num                  = false;
    tc_constrain45                  = false;
    tc_lyr_no_planarize             = false;
    tc_lyr_reord_mode               = tReorderNone;
    tc_parse_level                  = 0;

    tc_ext_antenna_total            = 0.0;
    tc_ext_substrate_eps            = SUBSTRATE_EPS;
    tc_ext_substrate_thick          = SUBSTRATE_THICKNESS;
    tc_ext_ground_plane             = 0;
    tc_ext_gp_mode                  = GPI_PLACE;
    tc_ext_invert_ground_plane      = false;
    tc_ext_ground_plane_global      = false;

    setupInterface();
    setupVariables();
}


// Private static error exit.
//
void
cTech::on_null_ptr()
{
    fprintf(stderr, "Singleton class cTech used before instantiated.\n");
    exit(1);
}


namespace {
    bool same_state(const char *s1, const char *s2)
    {
        if ((strchr(s1, 's') || strchr(s1, 'S')) !=
                (strchr(s2, 's') || strchr(s2, 'S')))
            return (false);
        if ((strchr(s1, 'c') || strchr(s1, 'C')) !=
                (strchr(s2, 'c') || strchr(s2, 'C')))
            return (false);
        if ((strchr(s1, 'a') || strchr(s1, 'A')) !=
                (strchr(s2, 'a') || strchr(s2, 'A')))
            return (false);
        return (true);
    }
}


// Return a function key command, or 0 if not set.
// The saved string is of the form
//   [<tok>] cmd [<tok> cmd] ...
// where the tok is composed of the letters s, c, and a with presence
// indicating the pressed state of the Shift, Ctrl, and Alt modifiers. 
// It the tok is missing, the cmd applies when no modifiers are
// pressed.  The cmd is always a single lexical token, it should be
// quoted if it contains white space..
//
// Note that the return should be freed.
//
char *
cTech::GetFkey(int n, unsigned int state)
{
    if (n < 0 || n >= TECH_NUM_FKEYS)
        return (0);
    const char *str = tc_fkey_strs[n];
    if (!str)
        return (0);
    char st[4];
    char *c = st;
    if (state & GR_SHIFT_MASK)
        *c++ = 's';
    if (state & GR_CONTROL_MASK)
        *c++ = 'c';
    if (state & GR_ALT_MASK)
        *c++ = 'a';
    *c = 0;

    char *tok;
    while ((tok = lstring::getqtok(&str)) != 0) {
        if (*tok == '<') {
            if (same_state(st, tok)) {
                delete [] tok;
                return (lstring::getqtok(&str));
            }
            delete [] tok;
            delete [] lstring::getqtok(&str);
            continue;
        }
        if (!*st)
            return (tok);
        delete [] tok;
    }
    return (0);
}

