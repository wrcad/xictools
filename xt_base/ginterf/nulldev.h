
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
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef NULLDEV_H
#define NULLDEV_H

#include "graphics.h"

//
// A NULL graphics device.  This is used as the main graphics device
// when graphics should be suppressed.
//

namespace ginterf
{
    struct NULLdraw : public HCdraw
    {
        void Halt() { }

        void ResetViewport(int, int)                    { }
        void DefineViewport()                           { }
        void Dump(int)                                  { }
        void Pixel(int, int)                            { }
        void Pixels(GRmultiPt*, int)                    { }
        void Line(int, int, int, int)                   { }
        void PolyLine(GRmultiPt*, int)                  { }
        void Lines(GRmultiPt*, int)                     { }
        void Box(int, int, int, int)                    { }
        void Boxes(GRmultiPt*, int)                     { }
        void Arc(int, int, int, int, double, double)    { }
        void Polygon(GRmultiPt*, int)                   { }
        void Zoid(int, int, int, int, int, int)         { }
        void Text(const char*, int, int, int, int = -1, int = -1) { }
        void TextExtent(const char*, int*, int*)        { }
        void SetColor(int)                              { }
        void SetLinestyle(const GRlineType*)            { }
        void SetFillpattern(const GRfillType*)          { }
        void DisplayImage(const GRimage*, int, int, int, int) { }
        double Resolution()                             { return (1.0); }
    };

    // NULL widget bag class.
    //
    struct NULLwbag : virtual public GRwbag
    {
        void Title(const char*, const char*)                    { }
        void ClearPopups()                                      { }

        GReditPopup *PopUpTextEditor(const char*,
            bool(*)(const char*, void*, XEtype), void*, bool)   { return (0); }
        GReditPopup *PopUpFileBrowser(const char*)          { return (0); }
        GReditPopup *PopUpStringEditor(const char*,
            bool(*)(const char*, void*, XEtype), void*)     { return (0); }
        GReditPopup *PopUpMail(const char*, const char*,
            void(*)(GReditPopup*) = 0, GRloc = GRloc())     { return (0); }

        GRfilePopup *PopUpFileSelector(FsMode, GRloc,
            void(*)(const char*, void*),
            void(*)(GRfilePopup*, void*),
            void*, const char*)                             { return (0); }

        void PopUpFontSel(GRobject, GRloc, ShowMode,
            void(*)(const char*, const char*, void*),
            void*, int, const char** = 0, const char* = 0)      { }

        void PopUpPrint(GRobject, HCcb*, HCmode, GRdraw* = 0)   { }
        void HCupdate(HCcb*, GRobject)                          { }
        void HCsetFormat(int)                                   { }

        bool PopUpHelp(const char*)                         { return (false); }

        GRlistPopup *PopUpList(stringlist*, const char*,
            const char*, void(*)(const char*, void*),
            void*, bool, bool)                              { return (0); }

        GRmcolPopup *PopUpMultiCol(stringlist*, const char*,
            void(*)(const char*, void*), void*, const char**,
            int = 0, bool = false)                          { return (0); }

        GRaffirmPopup *PopUpAffirm(GRobject, GRloc,
            const char*, void(*)(bool, void*), void*)       { return (0); }
        GRnumPopup *PopUpNumeric(GRobject, GRloc, const char*,
            double, double, double, double, int,
            void(*)(double, bool, void*), void*)            { return (0); }
        GRledPopup *PopUpEditString(GRobject, GRloc, const char*,
            const char*, ESret(*)(const char *, void*), void*,
            int, void(*)(bool), bool = false,
            const char* = 0)                                { return (0); }
        void PopUpInput(const char*, const char*, const char*,
            void(*)(const char*, void*), void*, int = 0)        { }
        GRmsgPopup *PopUpMessage(const char*, bool, bool = false,
            bool = false, GRloc = GRloc())                  { return (0); }
        int PopUpWarn(ShowMode, const char*, STYtype = STY_NORM,
            GRloc = GRloc())                                { return (-100); }
        int PopUpErr(ShowMode, const char*, STYtype = STY_NORM,
            GRloc = GRloc())                                { return (-100); }
        GRtextPopup *PopUpErrText(const char*, STYtype = STY_NORM,
            GRloc = GRloc(LW_UL))                           { return (0); }
        int PopUpInfo(ShowMode, const char*, STYtype = STY_NORM,
            GRloc = GRloc(LW_LL))                           { return (-100); }
        int PopUpInfo2(ShowMode, const char*, bool(*)(bool, void*),
            void*, STYtype = STY_NORM, GRloc = GRloc(LW_LL)){ return (-100); }
        int PopUpHTMLinfo(ShowMode, const char*,
            GRloc = GRloc(LW_LL))                           { return (-100); }

        GRledPopup *ActiveInput()                           { return (0); }
        GRmsgPopup *ActiveMessage()                         { return (0); }
        GRtextPopup *ActiveInfo()                           { return (0); }
        GRtextPopup *ActiveInfo2()                          { return (0); }
        GRtextPopup *ActiveHtinfo()                         { return (0); }
        GRtextPopup *ActiveError()                          { return (0); }
        GRfontPopup *ActiveFontsel()                        { return (0); }

        void SetErrorLogName(const char*)                   { }
    };

    class NULLdev : public GRscreenDev
    {
    public:
        NULLdev()
            {
                name = "NULL";
                ident = _devNULL_;
                x_pixels = false;
            }

        bool Init(int*, char**)                         { return (false); }

        GRdraw *NewDraw(int)
            {
                return (new NULLdraw);
            }

        GRwbag *NewWbag(const char*, GRwbag *w)
            {
                if (!w)
                    w = new NULLwbag;
                return (w);
            }

        bool InitColormap(int, int, bool)               { return (false); }
        int AllocateColor(int*, int, int, int)          { return (0); }
        int NameColor(const char*)                      { return (0); }
        bool NameToRGB(const char*, int *rgb)
                              { rgb[0] = rgb[1] = rgb[2] = 0; return (true); }

        int AddTimer(int, int(*)(void*), void*)         { return (0); }
        void RemoveTimer(int)                           { }
        void (* RegisterSigintHdlr(void(*)()) )()       { return (0); }
        bool CheckForEvents()                           { return (false); }
        int Input(int, int, int*)                       { return (0); }
        void MainLoop(bool=false)                       { }
        int LoopLevel()                                 { return (0); }
        void BreakLoop()                                { }
        void HCmessage(const char*)                     { }
        int UseSHM()                                    { return (0); }

        // This is used to resolve pixel values in the hard copy drivers.
        // If x_pixels is set, then we have an X server connection, and
        // pixel values are defined within the context of the current
        // visual.  Otherwise, the pixels are set as RGB triples
        //
        void RGBofPixel(int pixel, int *r, int *g, int *b)
        {
            if (!x_pixels) {
                *r = pixel & 0xff;
                *g = (pixel >> 8) & 0xff;
                *b = (pixel >> 16) & 0xff;
            }
            else {
                *r = 0;
                *g = 0;
                *b = 0;
            }
        }

    private:
        bool x_pixels;
    };
}

#endif

