
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
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
 $Id: tech_cds_out.h,v 5.7 2014/11/17 05:17:38 stevew Exp $
 *========================================================================*/

#ifndef TECH_CDS_OUT_H
#define TECH_CDS_OUT_H


//
//  Definitions for Cadence tech/drf file writer.
//

//-----------------------------------------------------------------------------
// cTecCdsOut  Cadence tect/drf file writer

class cTechCdsOut
{
public:
    cTechCdsOut()
        {
            tco_fp = 0;
            tco_stip_cnt = 100;
        }

    bool write_tech(const char*);
    bool write_drf(const char*);
    bool write_lmap(const char*);

private:
    bool dump_tech();
    bool dump_controls();
    bool dump_layerDefinitions();
    bool dump_techPurposes();
    bool dump_techLayers();
    bool dump_techLayerPurposePriorities();
    bool dump_techDisplays();
    bool dump_techLayerProperties();
    bool dump_techDerivedLayers();
    bool dump_layerRules();
    bool dump_viaDefs();
    bool dump_constraintGroups();
    bool dump_devices();
    bool dump_viaSpecs();

    bool dump_drf();
    bool dump_drDefineColor();
    bool dump_drDefineStipple();
    bool dump_drDefineLineStyle();
    bool dump_drDefinePacket();

    void dspstrings(const CDl*, const char**, const char**, char**);
    char *mk_colorname(const CDl*);
    char *mk_stipplename(const CDl*);
    char *mk_packetname(const char*, const char*, const char*);

    FILE *tco_fp;
    unsigned int tco_stip_cnt;
};

#endif

