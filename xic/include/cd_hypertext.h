
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
 $Id: cd_hypertext.h,v 5.32 2017/04/13 17:06:24 stevew Exp $
 *========================================================================*/

#ifndef CD_HYPERTEXT_H
#define CD_HYPERTEXT_H

// Request flags.  Used as argument to CDs::hy_string, etc.
// Warning, the numerical values associated with node/bran/devn are
// used in files - don't change these (or lose compatibility).  These
// are the true "hypertext" requests, HY_CELL will load a cell name as
// plain text, HY_LABEL will return label text.
//
#define HY_NODE     0x1
#define HY_BRAN     0x2
#define HY_DEVN     0x4
#define HY_CELL     0x8
#define HY_LABEL    0x10

// Type for hypertext entry.
//
enum HYrefType
{
    HYrefBogus      = 0,
    HYrefNode       = HY_NODE,
    HYrefBranch     = HY_BRAN,
    HYrefDevice     = HY_DEVN,
    HYrefCell       = HY_CELL,
    HYrefLabel      = HY_LABEL
};

// Orientatons for hypertext entries.
//
enum HYorType
{
    HYorNone,
    HYorDn,
    HYorRt,
    HYorUp,
    HYorLt
};

// Text conversion modes.
//
enum HYcvType
{
    HYcvPlain,
    HYcvAscii
};

// Type for hypertext list.
//
enum HLrefType
{
    HLrefEnd        = 0,
    HLrefNode       = 1,
    HLrefBranch     = 2,
    HLrefDevice     = 4,
    HLrefText       = 8,
    HLrefLongText   = 16
};


// Tokens for ascii encoding.
#define HYtokPre "(||"
#define HYtokSuf "||)"
#define HYtokLT  "text"
#define HYtokSC  "sc"

// This is the format used to start a reference definition.  There can
// be multiple coordinate pairs that follow (4.2.9 and later).  Note
// that there is no white space around the ':'.
//
#define HYfmtRefBegin   "%d:%d %d"

// This is shown on-screen for "long text" labels
//
#define HY_LT_MSG "[text]"

// This is the prefix for script labels
//
#define HY_SCRPREFIX "!!script"

// This is the default script name
//
#define HY_SCRNAME "script"

// Linked list for keeping track of coordinate reference through a
// cell hierarchy.
//
struct hyParent
{
    hyParent(CDc *cd, int x, int y, hyParent *n)
        {
            pNext = n;
            pCdesc = cd;
            pX = x;
            pY = y;
        }

    static void destroy(const hyParent *p)
        {
            while (p) {
                const hyParent *px = p;
                p = p->pNext;
                delete px;
            }
        }

    // Copy the stack.
    //
    static hyParent *dup(const hyParent *thisp)
        {
            hyParent *p0 = 0, *pe = 0;
            for (const hyParent *p = thisp; p; p = p->pNext) {
                if (!p0)
                    p0 = pe = new hyParent(p->pCdesc, p->posx(), p->posy(), 0);
                else {
                    pe->pNext =
                        new hyParent(p->pCdesc, p->posx(), p->posy(), 0);
                    pe = pe->pNext;
                }
            }
            return (p0);
        }

    // Return true if the stacks are identical, or both null.
    //
    static bool cmp(const hyParent *thisp, const hyParent *pc)
        {
            const hyParent *p = thisp;
            while (p) {
                if (!pc)
                    return (false);
                if (p->pCdesc != pc->pCdesc)
                    return (false);
                p = p->pNext;
                pc = pc->pNext;
            }
            return (!pc);
        }

    hyParent *next()            const { return (pNext); }
    void set_next(hyParent *p)        { pNext = p; }

    CDc *cdesc()                const { return (pCdesc); }
    void set_cdesc(CDc *c)            { pCdesc = c; }

    int posx()                  const { return (pX); }
    int posy()                  const { return (pY); }
    void set_posx(int x)              { pX = x; }
    void set_posy(int y)              { pY = y; }

private:
    hyParent *pNext;            // link
    CDc *pCdesc;                // parent of reference
    int pX;                     // coordinate to select pCdesc in its
    int pY;                     //  parent, used only in proxy lists
};

// The data for the hyEnt, without a destructor.  This is also used as
// backup storage for hyEnts in the undo system, where we want to
// destroy without calling the destructor.
//
struct hyEntData
{
    hyEntData()
        {
            hyX = hyY = 0;
            hySdesc = 0;
            hyOdesc = 0;
            hyPrnt = 0;
            hyPrxy = 0;
            hyTindex = 0;
            hyRefType = HYrefBogus;
            hyOrient = HYorNone;
            hyIsTerm = false;
            hyLinked = false;
            hyFixProxy = false;
        }

    // Set the reference to a no-op.
    //
    void set_noref()
    {
        hyOdesc = 0;
        hyParent::destroy(hyPrnt);
        hyPrnt = 0;
        hyParent::destroy(hyPrxy);
        hyPrxy = 0;
        hyRefType = HYrefBogus;
        hyOrient = HYorNone;
        hyFixProxy = false;
    }

    // Return the cell that contains hyOdesc.
    //
    CDs *container()
        {
            if (hyPrxy) {
                CDc *cd = hyPrxy->cdesc();
                if (!cd)
                    return (0);
                return (cd->masterCell(true));
            }
            return (hySdesc);
        }

    // Return the top-level context cell.
    //
    CDs *owner()                const { return (hySdesc); }

    int pos_x()                 const { return (hyX); }
    int pos_y()                 const { return (hyY); }
    CDo *odesc()                const { return (hyOdesc); }
    hyParent *parent()          const { return (hyPrnt); }
    hyParent *proxy()           const { return (hyPrxy); }
    HYrefType ref_type()        const { return (hyRefType); }
    HYorType orient()           const { return (hyOrient); }

    void set_pos_x(int x)             { hyX = x; }
    void set_pos_y(int y)             { hyY = y; }
    void set_owner(CDs *sd)           { hySdesc = sd; }
    void set_odesc(CDo *od)           { hyOdesc = od; }
    void set_parent(hyParent *p)      { hyPrnt = p; }
    void set_proxy(hyParent *p)       { hyPrxy = p; }
    void set_ref_type(HYrefType rt)   { hyRefType = rt; }
    void set_orient(HYorType t)       { hyOrient = t; }
    void set_fix_proxy(bool b)        { hyFixProxy = b; }

protected:
    int hyX;                    // Reference oordinate.
    int hyY;
    CDs *hySdesc;               // Top level context cell, where this
                                //  will be linked.  If there is a hyPrxy
                                //  list, then hyOdesc is found in
                                //  hyPrxy->cdesc()->masterCell(), NOT
                                //  hySdesc!

    CDo *hyOdesc;               // Object referenced.

    hyParent *hyPrnt;           // List of parent subckts for sub ref.
    hyParent *hyPrxy;           // List of parent subckts for proxy.
    //
    // The hyPrnt list is a path to a reference object contained in a
    // subckt, whose coordinates translate to a location in the
    // displaying window.  I.e., the object is visible in the display
    // window.  The hyPrxy allows a proxy window to be associated with
    // a subckt in another window.  For example, the subcircuit may be
    // shown as a symbol.  We can use hyPrxy to associate a second
    // window displaying the schematic representation of the symbol.

    unsigned int hyTindex;      // Index of cell terminal referenced.
    HYrefType hyRefType;        // Type of reference.
    HYorType hyOrient;          // Orientation of branch current.
    bool hyIsTerm;              // Reference a terminal (nil hyOdesc).
    bool hyLinked;              // True if add() was called.
    bool hyFixProxy;            // hyPrxy is a list of coords, need to
                                //  build actual list, after cell read.
};

// Hypertext entry, represents a clickable reference.
//
struct hyEnt : public hyEntData
{
    hyEnt() { }

    hyEnt(CDs*, int, int, CDo*, HYrefType, HYorType);
    hyEnt(CDs*, int, int, int);
    ~hyEnt();

    // Refresh reference.
    //
    void check_ref()
        {
            switch (hyRefType) {
            case HYrefNode:
            case HYrefBranch:
            case HYrefDevice:
                if (!hyOdesc && !hyIsTerm)
                    delete [] stringUpdate(0);
                if (!hyOdesc && !hyIsTerm)
                    // bad reference
                    set_noref();
                break;
            default:
                break;
            }
        }

    bool add();
    bool remove();
    hyEnt *dup() const;
    int nodenum() const;
    void get_tfpoint(int*, int*) const;
    char *stringUpdate(cTfmStack*);
    char *get_subname(bool) const;
    char *parse_bstring() const;
    char *get_devname() const;
    char *get_nodename(int) const;

    static int hy_strcmp(hyEnt*, hyEnt*);
};

namespace { struct LTdlist; }

// "Long text" helper.
//
struct HYlt
{
    static void *lt_new(hyList*, void(*)(hyList*, void*), void*);
    static void lt_clear();
    static void lt_update(void*, const char*);
    static void lt_copy(const hyList*, hyList*);
    static void lt_free(const hyList*);

private:
    static LTdlist *LtList;
};

// Hypertext string list.
//
struct hyList
{
    hyList()
        {
            hlNext = 0;
            hlRefType = HLrefEnd;
            hlText = 0;
            hlEnt = 0;
        }

    hyList(HLrefType t)
        {
            hlNext = 0;
            hlRefType = t;
            hlText = 0;
            hlEnt = 0;
        }

    hyList(CDs*, const char*, HYcvType);

    ~hyList()
        {
            if (hlRefType == HLrefLongText)
                HYlt::lt_free(this);
            delete [] hlText;
            delete hlEnt;
        }

    static void destroy(const hyList *h)
        {
            while (h) {
                const hyList *hx = h;
                h = h->hlNext;
                delete hx;
            }
        }

    bool is_label_script() const
        {
            if ((hlRefType == HLrefText || hlRefType == HLrefLongText) &&
                    hlText && lstring::ciprefix(HY_SCRPREFIX, hlText))
                return (true);
            return (false);
        }

    static hyList *dup(const hyList*);

    static char *string(hyList *hyl, HYcvType tp, bool allow_long)
        {
            return (hyl ? hyl->string_prv(tp, allow_long) : 0);
        }

    static char *get_entry_string(hyList *hyl)
        {
            return (hyl ? hyl->get_entry_string_prv() : 0);
        }

    // Return the character length of the equivalent text.
    //
    static int length(hyList *hyl)
        {
            if (!hyl)
                return (0);
            char *s = hyl->string_prv(HYcvPlain, true);
            if (!s)
                return (0);
            int i = strlen(s);
            delete [] s;
            return (i);
        }

private:
    char *string_prv(HYcvType, bool);
    char *get_entry_string_prv();

public:
    void trim_white_space();

    static const char *hy_token(const char*, const char**, char**);
    static char *hy_scale(char*, double);
    static int hy_strcmp(hyList*, hyList*);
    static char *hy_strip(const char*);

    hyList *next()                    { return (hlNext); }
    HLrefType ref_type()        const { return (hlRefType); }
    const char *text()          const { return (hlText); }
    const hyEnt *hent()         const { return (hlEnt); }

    char *etext()                     { return (hlText); }

    void set_next(hyList *n)          { hlNext = n; }
    void set_ref_type(HLrefType t)    { hlRefType = t; }
    void set_text(char *t)            { hlText = t; }
    void set_hent(hyEnt *e)           { hlEnt = e; }

private:
    hyList *hlNext;             // link
    HLrefType hlRefType;        // reference type
    char *hlText;               // text, if HLrefText
    hyEnt *hlEnt;               // hyEnt struct if not HLrefText
};

#endif

