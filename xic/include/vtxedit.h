
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
 $Id: edit.h,v 5.139 2016/09/01 16:42:27 stevew Exp $
 *========================================================================*/

#ifndef VTXEDIT_H
#define VTXEDIT_H


//
// Support for the vertex editing function of polygons and wires.
//

// List element for object vertices.
//
struct Vtex
{
    Vtex(const Point &px) : v_p(px.x, px.y)
        {
            v_next = 0;
            v_movable = false;
        }

    static void destroy(Vtex *v)
        {
            while (v) {
                Vtex *vx = v;
                v = v->v_next;
                delete vx;
            }
        }

    Point_c v_p;
    Vtex *v_next;
    bool v_movable;
};

// A selected object with a vertex to move.
//
struct sObj
{
    sObj(CDo *p, sObj *n)
        {
            o_obj = p;
            o_pts = 0;
            o_next = n;
        }

    ~sObj()
        {
            Vtex::destroy(o_pts);
        }

    static void destroy(sObj *o)
        {
            while (o) {
                sObj *ox = o;
                o = o->o_next;
                delete ox;
            }
        }

    CDo *object()               { return (o_obj); }
    void set_object(CDo *o)     { o_obj = o; }

    Vtex *points()              { return (o_pts); }
    void set_points(Vtex *v)    { o_pts = v; }

    sObj *next_obj()            { return (o_next); }
    void set_next_obj(sObj *n)  { o_next = n; }

    // vtxedit.cc
    static bool empty(const sObj*);
    static sObj *mklist(sObj*, CDol*, BBox*);
    static void mark_vertices(const sObj*, bool);
    bool get_wire_ref(int*, int*, int*, int*);

private:
    CDo *o_obj;
    Vtex *o_pts;
    sObj *o_next;
};
#endif

