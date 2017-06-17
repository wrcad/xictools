
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
 $Id: dsp_setif.cc,v 1.23 2013/09/25 00:57:24 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "geo_zoid.h"
#include "fio.h"
#include "grfont.h"


//
// Setup interface callbacks in the CD, GEO, and FIO libraries.
//

//-----------------------------------------------------------------------------
// CD configuration

namespace {
    // Compute the label object database size and return true, or return
    // false and use default sizing.  Applies to all modes.
    //
    int
    ifDefaultLabelSize(const char *text, DisplayMode mode, double *width,
        double *height)
    {
        int lwid, lhei, numlines;
        FT.textExtent(text, &lwid, &lhei, &numlines);
        if (mode == Physical) {
            *width = (CDphysDefTextWidth * lwid)/FT.cellWidth();
            *height = (CDphysDefTextHeight * lhei)/FT.cellHeight();
        }
        else {
            *width = (CDelecDefTextWidth * lwid)/FT.cellWidth();
            *height = (CDelecDefTextHeight * lhei)/FT.cellHeight();
        }
        return (numlines);
    }

    // The layer is being created, set up the application-specific stuff.
    //
    void
    ifNewLayer(CDl *ld)
    {
        ld->setDspData(new DspLayerParams(ld));
    }

    // The layer descriptor ld is being destroyed.  This should destroy
    // the user information (if any) in ld->dsp_data.
    //
    void
    ifInvalidateLayer(CDl *ld)
    {
        delete dsp_prm(ld);
    }

    // The symbol is being destroyed.
    //
    void
    ifInvalidateSymbol(CDcellName sname)
    {
        for (int i = 0; i < DSP_NUMWINS; i++) {
            if (DSP()->Window(i) && DSP()->Window(i)->TopCellName() == sname) {
                DSP()->Window(i)->SetTopCellName(0);
                DSP()->Window(i)->SetCurCellName(0);
            }
        }
    }

    // The terminal is being destroyed.
    //
    void
    ifInvalidateTerminal(CDterm *t)
    {
        DSP()->ClearTerminal(t);
    }
}


//-----------------------------------------------------------------------------
// GEO configuration

namespace {
    // Render the trapezoid in a drawing window using highlighting.
    //
    void
    ifDrawZoid(const Zoid *z)
    {
        DSPmainDraw(SetColor(DSP()->Color(HighlightingColor)))
        DSP()->MainWdesc()->ShowLine(z->xll, z->yl, z->xul, z->yu);
        DSP()->MainWdesc()->ShowLine(z->xul, z->yu, z->xur, z->yu);
        DSP()->MainWdesc()->ShowLine(z->xur, z->yu, z->xlr, z->yl);
        DSP()->MainWdesc()->ShowLine(z->xlr, z->yl, z->xll, z->yl);
    }
}


//-----------------------------------------------------------------------------
// FIO configuration

namespace {
    // Return the current display mode of the main window,
    // Physical or Electrical.
    //
    DisplayMode
    ifCurrentDisplayMode()
    {
        return (DSP()->CurMode());
    }
}


void
cDisplay::setupInterface()
{
    CD()->RegisterIfDefaultLabelSize(ifDefaultLabelSize);
    CD()->RegisterIfNewLayer(ifNewLayer);
    CD()->RegisterIfInvalidateLayer(ifInvalidateLayer);
    CD()->RegisterIfInvalidateSymbol(ifInvalidateSymbol);
    CD()->RegisterIfInvalidateTerminal(ifInvalidateTerminal);

    GEO()->RegisterIfDrawZoid(ifDrawZoid);

    FIO()->RegisterIfCurrentDisplayMode(ifCurrentDisplayMode);
}

