
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
 $Id: main_scriptif.h,v 5.8 2015/03/06 04:57:43 stevew Exp $
 *========================================================================*/

#ifndef MAIN_SCRIPTIF_H
#define MAIN_SCRIPTIF_H


inline struct SIlocalContext *SIlcx();

// The local script execution context for Xic.  This maintains state
// for various script functions.
//
struct SIlocalContext : public SIlocal_context
{
    static SIlocalContext *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

    friend inline SIlocalContext *SIlcx() { return (SIlocalContext::ptr()); }

    SIlocalContext();               // create and register
    void clear();                   // called on script exit
    void setFrozen(bool);
    void pushVar(const char*, const char*);
    void popVar(const char*);

    PolyList *ghostList()           const { return (lc_ghostList); }
    void setGhostList(PolyList *gl) { lc_ghostList = gl; }

    int ghostCount()                const { return (lc_ghostCount); }
    void incGhostCount()            { lc_ghostCount++; }
    void decGhostCount()            { lc_ghostCount--; }

    int curGhost()                  const { return (lc_curGhost); }
    void setCurGhost(int c)         { lc_curGhost = c; }

    int lastX()                     const { return (lc_lastX); }
    int lastY()                     const { return (lc_lastY); }
    void setLastXY(int x, int y)    { lc_lastX = x; lc_lastY = y; }

    int indentLevel()               const { return (lc_indentLevel); }
    void setIndentLevel(int l)      { lc_indentLevel = l; }
 
    int maxArrayPts()               const { return (lc_maxArrayPts); }
    void setMaxArrayPts(int m)      { lc_maxArrayPts = m; }
    int maxZoids()                  const { return (lc_maxZoids); }
    void setMaxZoids(int m)         { lc_maxZoids = m; }

    bool applyTransform()           const { return (lc_applyTransform); }
    void setApplyTransform(bool a, int x, int y)
        {
            lc_applyTransform = a;
            lc_transformX = x;
            lc_transformY = y;
        }
    int transformX()                const { return (lc_transformX); }
    int transformY()                const { return (lc_transformY); }

    bool doingPCell()               const { return (lc_doingPCell); }
    void setDoingPCell(bool b)      { lc_doingPCell = b; }

private:
    SymTab *lc_pushvarTab;          // table of pushed variables

    PolyList *lc_ghostList;         // list of ghosting polygons
    int lc_ghostCount;              // keep track of ghost status for cleanup
    int lc_curGhost;                // ghost parameters
    int lc_lastX;
    int lc_lastY;

    int lc_indentLevel;             // printing indentation
    int lc_maxArrayPts;             // how many things to print
    int lc_maxZoids;

    int lc_transformX;              // current transform reference
    int lc_transformY;
    bool lc_applyTransform;

    bool lc_noRedisplayBak;         // prior freeze state
    bool lc_frozen;                 // display is frozen
    bool lc_doingPCell;             // executing a pcell script

    static SIlocalContext *instancePtr;
};

#endif

