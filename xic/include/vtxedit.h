
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

#ifndef VTXEDIT_H
#define VTXEDIT_H


//
// Support for the vertex editing function of polygons and wires.
//

// List element for object vertices.
//
struct Vertex
{
    Vertex(const Point &tx) : v_p(tx.x, tx.y), v_next(0), v_movable(false) { }

    static void destroy(Vertex *v)
        {
            while (v) {
                Vertex *vx = v;
                v = v->v_next;
                delete vx;
            }
        }

    Point *point()              { return (&v_p); }
    int px()                    const { return (v_p.x); }
    int py()                    const { return (v_p.y); }
    const Vertex *cnext()       const { return (v_next); }
    Vertex *next()              { return (v_next); }
    void set_next(Vertex *n)    { v_next = n; }
    bool movable()              const { return (v_movable); }
    void set_movable(bool b)    { v_movable = b; }

private:
    Point_c v_p;
    Vertex *v_next;
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
            Vertex::destroy(o_pts);
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

    Vertex *points()            { return (o_pts); }
    void set_points(Vertex *v)  { o_pts = v; }

    sObj *next_obj()            { return (o_next); }
    void set_next_obj(sObj *n)  { o_next = n; }

    // vtxedit.cc
    static bool empty(const sObj*);
    static sObj *mklist(sObj*, CDol*, BBox*);
    static void mark_vertices(const sObj*, bool);
    bool get_wire_ref(int*, int*, int*, int*);

private:
    CDo *o_obj;
    Vertex *o_pts;
    sObj *o_next;
};
#endif

