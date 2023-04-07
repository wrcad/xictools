
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTINTERF_H
#define QTINTERF_H

#include "ginterf/graphics.h"
#include <string.h>
#include <QEventLoop>
#include "draw_if.h"

//
//  Main header for the QT-4 library
//

class QCursor;
class QWidget;
class QTextEdit;

namespace qtinterf { }
using namespace qtinterf;

#define NUMITEMS(x)  sizeof(x)/sizeof(x[0])

namespace qtinterf
{
    class interval_timer;
    class QTfilePopup;
    class QTfontPopup;
    class QTlistPopup;
    class QTaffirmPopup;
    class QTnumPopup;
    class QTledPopup;
    class QTmsgPopup;
    class QTprintPopup;

    typedef QTmsgPopup QTtextPopup;

    // Graphical context, may be used by multiple windows
    struct sGbag
    {
        sGbag() {
            cursor = 0;
            drawghost = 0;
            firstghost = false;
            showghost = false;
            noclear = false;
            ghostcxcnt = 0;
            refx = refy = 0;
            lastx = lasty = 0;
            backg = 0;
            foreg = 0;
            lastlinestyle = 0;
        }

        static sGbag *default_gbag();

        QCursor *cursor;
        void (*drawghost)(int, int, int, int);
        bool firstghost;
        bool showghost;
        bool noclear;
        int ghostcxcnt;
        int refx, refy;
        int lastx, lasty;
        unsigned long backg;
        unsigned long foreg;
        int lastlinestyle;
    };

    struct qt_draw : virtual public GRdraw
    {
        qt_draw();
        virtual ~qt_draw() { }

        // GRdraw virtual overrides
        unsigned long WindowID()                { return (0); }
        virtual void Halt();
        void Clear()                    { if (viewport) viewport->clear(); }
        int SwathHeight(int*)                   { return (0); }
        void SetSwath(int, int)                 { }
        void ResetViewport(int, int)            { }
        void DefineViewport()                   { }
        void Dump(int)                          { }

        void Pixel(int x, int y)
            { if (viewport) viewport->draw_pixel(x, y); }
        void Pixels(GRmultiPt *p, int n)
            { if (viewport) viewport->draw_pixels(p, n); }
        void Line(int x1, int y1, int x2, int y2)
            { if (viewport) viewport->draw_line(x1, y1, x2, y2); }
        void PolyLine(GRmultiPt *p, int n)
            { if (viewport) viewport->draw_polyline(p, n); }
        void Lines(GRmultiPt *p, int n)
            { if (viewport) viewport->draw_lines(p, n); }
        void Box(int x1, int y1, int x2, int y2)
            { if (viewport) viewport->draw_box(x1, y1, x2, y2); }
        void Boxes(GRmultiPt *p, int n)
            { if (viewport) viewport->draw_boxes(p, n); }
        void Arc(int x, int y, int rx, int ry, double a1, double a2)
            { if (viewport) viewport->draw_arc(x, y, rx, ry, a1, a2); }
        void Polygon(GRmultiPt *p, int n)
            { if (viewport) viewport->draw_polygon(p, n); }
        void Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
            { if (viewport) viewport->draw_zoid(yl, yu, xll, xul, xlr, xur); }
        void Text(const char *str, int x, int y, int, int = -1, int = -1)
            { if (viewport) viewport->draw_text(x, y, str, -1); }
        void TextExtent(const char *str, int *w, int *h)
            { if (viewport) viewport->text_extent(str, w, h); }
        void MovePointer(int, int, bool);
        void SetGhost(void(*)(int, int, int, int), int, int);
        void ShowGhost(int);
        void UndrawGhost();
        void DrawGhost(int, int);
        void QueryPointer(int*, int*, unsigned*);
        void DefineColor(int*, int, int, int);
        void SetBackground(int pix)
            { if (viewport) viewport->set_background(pix); }
        void SetWindowBackground(int pix)
            { if (viewport) viewport->set_background(pix); }
        void SetGhostColor(int); 
        void SetColor(int pix)
            { if (viewport) viewport->set_foreground(pix); }  
        void SetLinestyle(const GRlineType *lt)
            { if (viewport) viewport->set_linestyle(lt); }
        void DefineLinestyle(GRlineType*)       { }
        void DefineFillpattern(GRfillType *fp)
            { if (viewport) viewport->define_fillpattern(fp); }
        void SetFillpattern(const GRfillType *fp)
            { if (viewport) viewport->set_fillpattern(fp); }
        void Update()
            { if (viewport) viewport->update(); }

        void Input(int*, int*, int*, int*);
        void SetXOR(int);
        void ShowGlyph(int, int, int);
        GRobject GetRegion(int, int, int, int);
        void PutRegion(GRobject, int, int, int, int);
        void FreeRegion(GRobject);
        void DisplayImage(const GRimage *im, int x, int y, int w, int h)
            { if (viewport) viewport->draw_image(im, x, y, w, h); }
        double Resolution() { return (1.0); }

        sGbag *xbag;                // graphics rendering context
        draw_if *viewport;          // drawing area
    };

    struct qt_bag : virtual public GRwbag
    {
        qt_bag(QWidget*);
        virtual ~qt_bag();

        // QT-SPECIFIC
        QWidget *ItemFromTicket(GRobject);

        // Pass a title for the window and icon.
        //
        void Title(const char*, const char*);

        // editor
        GReditPopup *PopUpTextEditor(const char*,
            bool(*)(const char*, void*, XEtype), void*, bool);
        GReditPopup *PopUpFileBrowser(const char*);
        GReditPopup *PopUpStringEditor(const char*,
            bool(*)(const char*, void*, XEtype), void*);
        GReditPopup *PopUpMail(const char*, const char*,
            void(*)(GReditPopup*) = 0, GRloc = GRloc());

        // file selection
        GRfilePopup *PopUpFileSelector(FsMode, GRloc,
            void(*)(const char*, void*),
            void(*)(GRfilePopup*, void*), void*, const char*);

        // font selector
        void PopUpFontSel(GRobject, GRloc, ShowMode,
            void(*)(const char*, const char*, void*),
            void*, int, const char** = 0, const char* = 0);

        // printing
        void PopUpPrint(GRobject, HCcb*, HCmode, GRdraw* = 0);
        void HCupdate(HCcb*, GRobject);
        void HcopyDisableMsgs();
        bool HcopyLocate(int, int, int*, int*);

        // Override these when printing support for a widget collection
        // is needed.
        virtual char *GetPostscriptText(int, const char*, const char*, bool,
            bool)
            { return (0); }  // see htm/htm_text.cc
        virtual char *GetPlainText()
            { return (0); }
        virtual char *GetHtmlText()
            { return (0); }

        // help
        bool PopUpHelp(const char*);

        // list
        GRlistPopup *PopUpList(stringlist*, const char*, const char*,
            void(*)(const char*, void*), void*, bool);

        // qtmcol.cc
        GRmcolPopup *PopUpMultiCol(stringlist*, const char*,
            void(*)(const char*, void*), void*, const char**,
            int = 0, bool = false);

        // utilities
        void ClearPopups();

        GRaffirmPopup *PopUpAffirm(GRobject, GRloc, const char*,
            void(*)(bool, void*), void*);
        GRnumPopup *PopUpNumeric(GRobject, GRloc, const char*,
            double initd, double, double, double, int,
            void(*)(double, bool, void*), void*);
        GRledPopup *PopUpEditString(GRobject, GRloc, const char*,
            const char*, ESret(*)(const char*, void*), void*, int,
            void(*)(bool), bool = false, const char* = 0);
        void PopUpInput(const char*, const char*, const char*,
            void(*)(const char*, void*), void*, int=0);
        GRmsgPopup *PopUpMessage(const char*, bool, bool = false,
            bool = false, GRloc = GRloc());
        int PopUpErr(ShowMode, const char*, STYtype = STY_NORM,
            GRloc = GRloc());
        GRtextPopup *PopUpErrText(const char*, STYtype = STY_NORM,
            GRloc = GRloc(LW_UL));
        int PopUpInfo(ShowMode, const char*, STYtype = STY_NORM,
            GRloc = GRloc(LW_LL));
        int PopUpInfo2(ShowMode, const char*, bool(*)(bool, void*),
            void*, STYtype = STY_NORM, GRloc = GRloc(LW_LL));
        int PopUpHTMLinfo(ShowMode, const char*, GRloc = GRloc(LW_LL));

        GRledPopup *ActiveInput();
        GRmsgPopup *ActiveMessage();
        GRtextPopup *ActiveInfo();
        GRtextPopup *ActiveInfo2();
        GRtextPopup *ActiveHtinfo();
        GRtextPopup *ActiveError();
        GRfontPopup *ActiveFontsel();

        // QT-SPECIFIC
        QWidget *shell_widget() { return (shell); }

    public:
        QWidget *shell;             // top level widget
        QTledPopup *input;          // dialog input popup
        QTmsgPopup *message;        // message popup
        QTmsgPopup *info;           // info popup
        QTmsgPopup *info2;          // two button info popup
        QTmsgPopup *htinfo;         // html info popup
        QTmsgPopup *error;          // error popup
        QTfontPopup *fontsel;       // font select popup

        QTprintPopup *hc;           // hard copy parameters and widget
        GRmonList monitor;          // certain popups use this
        void *call_data;            // internal data
        void (*sens_set)(qt_bag*, bool);
                                    // sensitivity change callback
        int err_cnt;                // counters for window id
        int info_cnt;
        int info2_cnt;
        int htinfo_cnt;
    };

    // An event loop that can be linked into a list.
    //
    class event_loop : public QEventLoop
    {
    public:
        event_loop(event_loop *nx) : QEventLoop(0)
        {
            next = nx;
        }
        event_loop *next;
    };

    // QT driver class
    //
    class QTdev : public GRscreenDev
    {
    public:
        QTdev();
        ~QTdev();

        // virtual overrides
        // qtinterf.cc
        virtual bool Init(int*, char**);
        bool InitColormap(int, int, bool);
        void RGBofPixel(int, int*, int*, int*);
        int AllocateColor(int*, int, int, int);
        int NameColor(const char*);
        bool NameToRGB(const char*, int*);
        GRdraw *NewDraw(int);
        GRwbag *NewWbag(const char*, GRwbag*);
        int AddTimer(int, int(*)(void*), void*);
        void RemoveTimer(int);
        void (* RegisterSigintHdlr(void(*)()) )();
        bool CheckForEvents();
        int Input(int, int, int*);
        void MainLoop(bool=false);
        int LoopLevel()                     { return (loop_level); }
        void BreakLoop();
        void HCmessage(const char*);

        // Remaining functions are unique to class.

        // This can be set by the application to the main frame widget
        // bag, in which case certain pop-ups called from other pop-ups
        // will be rooted in the main window, rather than the pop-up's
        // window
        //
        void RegisterMainFrame(qt_bag *w) { main_bag = w; }
        qt_bag *main_frame() { return (main_bag); }

        // qtinterf.cc
        void SetPopupLocation(GRloc, QWidget*, QWidget*);
        void ComputePopupLocation(GRloc, QWidget*, QWidget*, int*, int*);

    private:
        int minx, miny;
        int loop_level;             // loop level
        event_loop *loop;           // event loop stack
        qt_bag *main_bag;           // top level bag
        interval_timer *timers;     // list of timers
    };
}

// Global access, set in constructor
extern qtinterf::QTdev *GRX;

#endif // GTKINTERF_H

