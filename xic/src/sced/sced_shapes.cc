
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
 $Id: sced_shapes.cc,v 5.35 2015/02/17 05:56:34 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "sced.h"
#include "edit.h"
#include "undolist.h"
#include "dsp_inlines.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "ebtn_menu.h"
#include "promptline.h"
#include "ghost.h"


#ifndef M_PI
#define	M_PI  3.14159265358979323846  // pi
#endif

// The pull-down menu entries.
// These keywords are the menu labels, and are also used as help system
// tags "shapes:xxx".
// The order MUST match the ShapeType enum!
//

namespace {
    const char * Shapes[] = {
        "box",
        "poly",
        "arc",
        "dot",
        "tri",
        "ttri",
        "and",
        "or",
        "sides",
        0
    };
}


const char *const *
cSced::shapesList()
{
    return (Shapes);
}


// This is just a repository for shape templates, with a constructor
// that is called on startup for analytical initialization.
//
namespace {
    struct sShapeTemplates
    {
        sShapeTemplates();

        short *DotP;
        int DotL;
        short *TriP;
        int TriL;
        short *TtriP;
        int TtriL;
        short *AndP;
        int AndL;
        short *OrP;
        int OrL;
    };

    sShapeTemplates ShapeTemplates;

    sShapeTemplates::sShapeTemplates()
    {
        int DotD = (30*CDelecResolution)/100;
        DotL = 9;
        DotP = new short[2*DotL];
        DotP[0] = 0;            DotP[1] = DotD;
        DotP[2] = (2*DotD)/3;   DotP[3] = (2*DotD)/3;
        DotP[4] = DotD;         DotP[5] = 0;
        DotP[6] = (2*DotD)/3,   DotP[7] = -(2*DotD)/3;
        DotP[8] = 0;            DotP[9] = -DotD;
        DotP[10] = -(2*DotD)/3; DotP[11] = -(2*DotD)/3;
        DotP[12] = -DotD;       DotP[13] = 0;
        DotP[14] = -(2*DotD)/3; DotP[15] = (2*DotD)/3;
        DotP[16] = 0;           DotP[17] = DotD;

        int TriH = 6*CDelecResolution;
        int TriW = 5*CDelecResolution;
        TriL = 4;
        TriP = new short[2*TriL];
        TriP[0] = 0;     TriP[1] = 0;
        TriP[2] = 0;     TriP[3] = TriH;
        TriP[4] = TriW;  TriP[5] = TriH/2;
        TriP[6] = 0;     TriP[7] = 0;

        int TtriD = 1*CDelecResolution;
        TtriL = 5;
        TtriP = new short[2*TtriL];
        TtriP[0] = 0;           TtriP[1] = 0;
        TtriP[2] = 0;           TtriP[3] = TriH;
        TtriP[4] = TriW-TtriD;  TtriP[5] = TriH-TtriD;
        TtriP[6] = TriW-TtriD;  TtriP[7] = TtriD;
        TtriP[8] = 0;           TtriP[9] = 0;

        int AndR = 3*CDelecResolution;
        AndL = 15;
        AndP = new short[2*AndL];
        AndP[0] = AndP[1] = 0;
        int nth = AndL - 3;
        int cx = AndR;
        int cy = AndR;
        double dth = M_PI/(nth-1);
        double th = 0;
        for (int i = 0; i < nth; i++) {
            AndP[2*i+2] = (int)(AndR*sin(th)) + cx;
            AndP[2*i+3] = -(int)(AndR*cos(th)) + cy;
            th += dth;
        }
        AndP[2*AndL-4] = 0;
        AndP[2*AndL-3] = 2*AndR;
        AndP[2*AndL-2] = 0;
        AndP[2*AndL-1] = 0;

        int r = CDelecResolution/10;
        OrL = 21;
        OrP = new short[2*OrL];
        OrP[0] = 0;     OrP[1] = 0;
        OrP[2] = 23*r;  OrP[3] = r;
        OrP[4] = 38*r;  OrP[5] = 4*r;
        OrP[6] = 46*r;  OrP[7] = 8*r;
        OrP[8] = 52*r;  OrP[9] = 12*r;
        OrP[10] = 58*r; OrP[11] = 18*r;
        OrP[12] = 62*r; OrP[13] = 24*r;
        OrP[14] = 65*r; OrP[15] = 30*r;
        OrP[16] = 62*r; OrP[17] = 36*r;
        OrP[18] = 58*r; OrP[19] = 42*r;
        OrP[20] = 52*r; OrP[21] = 48*r;
        OrP[22] = 46*r; OrP[23] = 52*r;
        OrP[24] = 38*r; OrP[25] = 56*r;
        OrP[26] = 24*r; OrP[27] = 59*r;
        OrP[28] = 0;    OrP[29] = 60*r;
        OrP[30] = 2*r;  OrP[31] = 51*r;
        OrP[32] = 4*r;  OrP[33] = 41*r;
        OrP[34] = 5*r;  OrP[35] = 30*r;
        OrP[36] = 4*r;  OrP[37] = 19*r;
        OrP[38] = 2*r;  OrP[39] = 9*r;
        OrP[40] = 0;    OrP[41] = 0;
    };
}


namespace {
    namespace sced_shapes {
        struct ShState : public CmdState
        {
            ShState(const char*, const char*);
            virtual ~ShState();

            void setup(int c)
                {
                    switch (c) {
                    case ShDot:
                    default:
                        Shape = ShDot;
                        break;
                    case ShTri:
                        Shape = ShTri;
                        break;
                    case ShTtri:
                        Shape = ShTtri;
                        break;
                    case ShAnd:
                        Shape = ShAnd;
                        break;
                    case ShOr:
                        Shape = ShOr;
                        break;
                    }
                }

            ShapeType shape()   { return (Shape); }
            double scale_x()    { return (Scalx); }
            double scale_y()    { return (Scaly); }
            void halt()         { esc(); }

            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);

        private:
            ShapeType Shape;
            double Scalx, Scaly;
            bool Shift;
            bool Ctrl;
        };

        ShState *ShCmd;
    }
}

using namespace sced_shapes;


// This is called in response to the pull-down menu of shape templates.
//
void
cSced::addShape(int shape)
{
    if (ShCmd)
        ShCmd->halt();
    switch (shape) {
    case ShBox:
        ED()->makeBoxesExec(0);
        break;
    case ShArc:
        SCD()->makeArcPathExec(0);
        break;
    case ShPoly:
        ED()->makePolygonsExec(0);
        break;
    default:
        if (!XM()->CheckCurMode(Electrical))
            return;
        if (!XM()->CheckCurLayer())
            return;
        if (!XM()->CheckCurCell(true, true, Electrical))
            return;

        Ulist()->ListCheck("shape", CurCell(), false);
        ShCmd = new ShState("SHAPE", "xic:shapes");
        ShCmd->setup(shape);
        if (!EV()->PushCallback(ShCmd)) {
            delete ShCmd;
            return;
        }
        Gst()->SetGhost(GFshape);
        break;
    }
}


namespace {
    struct sShape : public Poly
    {
        sShape();
        ~sShape();
        void modify();
        void transform(const cTfmStack*);
    };


    sShape::sShape()
    {
        int i;
        switch (ShCmd->shape()) {
        case ShDot:
        default:
            points = new Point[ShapeTemplates.DotL];
            numpts = ShapeTemplates.DotL;
            for (i = 0; i < numpts; i++)
                points[i].set(ShapeTemplates.DotP[2*i],
                    ShapeTemplates.DotP[2*i+1]);
            break;
        case ShTri:
            points = new Point[ShapeTemplates.TriL];
            numpts = ShapeTemplates.TriL;
            for (i = 0; i < numpts; i++)
                points[i].set(ShapeTemplates.TriP[2*i],
                    ShapeTemplates.TriP[2*i+1]);
            break;
        case ShTtri:
            points = new Point[ShapeTemplates.TtriL];
            numpts = ShapeTemplates.TtriL;
            for (i = 0; i < numpts; i++)
                points[i].set(ShapeTemplates.TtriP[2*i],
                    ShapeTemplates.TtriP[2*i+1]);
            break;
        case ShAnd:
            points = new Point[ShapeTemplates.AndL];
            numpts = ShapeTemplates.AndL;
            for (i = 0; i < numpts; i++)
                points[i].set(ShapeTemplates.AndP[2*i],
                    ShapeTemplates.AndP[2*i+1]);
            break;
        case ShOr:
            points = new Point[ShapeTemplates.OrL];
            numpts = ShapeTemplates.OrL;
            for (i = 0; i < numpts; i++)
                points[i].set(ShapeTemplates.OrP[2*i],
                    ShapeTemplates.OrP[2*i+1]);
            break;
        }
    }


    sShape::~sShape()
    {
        delete [] points;
    }


    void
    sShape::modify()
    {
        for (int i = 0; i < numpts; i++)
            points[i].set(mmRnd(points[i].x * ShCmd->scale_x()),
                mmRnd(points[i].y * ShCmd->scale_y()));
    }


    void
    sShape::transform(const cTfmStack *tstk)
    {
        tstk->TPath(numpts, points);
    }
}


ShState::ShState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Shape = ShBox;
    Scalx = Scaly = 1.0;
    Shift = Ctrl = false;
}


ShState::~ShState()
{
    ShCmd = 0;
}


void
ShState::b1down()
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    int x, y;
    EV()->Cursor().get_xy(&x, &y);
    cTfmStack stk;
    stk.TPush();
    stk.TSetMagn(GEO()->curTx()->magn());
    GEO()->applyCurTransform(&stk, 0, 0, x, y);
    sShape *s = new sShape;
    s->modify();
    s->transform(&stk);
    stk.TPop();

    Gst()->SetGhost(GFnone);

    CDl *ld = LT()->CurLayer();
    if (ld)
        cursd->newPoly(0, s, ld, 0, true);
    delete s;
}


void
ShState::b1up()
{
    if (!LT()->CurLayer()) {
        PL()->ShowPrompt("No current layer!");
        esc();
    }
    Ulist()->CommitChanges(true);
    Gst()->SetGhost(GFshape);
}


void
ShState::esc()
{
    PL()->ErasePrompt();
    Gst()->SetGhost(GFnone);
    EV()->PopCallback(this);
    delete this;
}


bool
ShState::key(int code, const char*, int)
{
    switch (code) {
    case SHIFTDN_KEY:
        Shift = true;
        break;
    case SHIFTUP_KEY:
        Shift = false;
        break;
    case CTRLDN_KEY:
        Ctrl = true;
        break;
    case CTRLUP_KEY:
        Ctrl = false;
        break;
    case LEFT_KEY:
        if (Ctrl)
            return (false);
        Gst()->SetGhost(GFnone);
        if (!Shift) {
            Scalx *= 0.9;
            Scaly *= 0.9;
        }
        else
            Scalx *= 0.9;
        Gst()->SetGhost(GFshape);
        Gst()->RepaintGhost();
        break;
    case UP_KEY:
        if (Ctrl)
            return (false);
        Gst()->SetGhost(GFnone);
        if (!Shift) {
            Scalx *= 2.0;
            Scaly *= 2.0;
        }
        else
            Scaly *= 1.1;
        Gst()->SetGhost(GFshape);
        Gst()->RepaintGhost();
        break;
    case RIGHT_KEY:
        if (Ctrl)
            return (false);
        Gst()->SetGhost(GFnone);
        if (!Shift) {
            Scalx *= 1.1;
            Scaly *= 1.1;
        }
        else
            Scalx *= 1.1;
        Gst()->SetGhost(GFshape);
        Gst()->RepaintGhost();
        break;
    case DOWN_KEY:
        if (Ctrl)
            return (false);
        Gst()->SetGhost(GFnone);
        if (!Shift) {
            Scalx *= 0.5;
            Scaly *= 0.5;
        }
        else
            Scaly *= 0.9;
        Gst()->SetGhost(GFshape);
        Gst()->RepaintGhost();
        break;
    default:
        return (false);
    }
    return (true);
}
// End of sShape functions.


// Render the ghost-drawn figure
//
void
cScedGhost::showGhostShape(int x, int y, int, int)
{
    if (!ShCmd)
        return;
    DSP()->TPush();
    DSP()->TSetMagn(GEO()->curTx()->magn());
    GEO()->applyCurTransform(DSP(), 0, 0, x, y);

    sShape *s = new sShape;
    s->modify();
    Gst()->ShowGhostPath(s->points, s->numpts);
    delete s;

    DSP()->TPop();
}

