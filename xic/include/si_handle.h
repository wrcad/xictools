
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
 $Id: si_handle.h,v 5.22 2016/06/21 17:26:50 stevew Exp $
 *========================================================================*/

#ifndef SI_HANDLE_H
#define SI_HANDLE_H


//
// Handles
//
// Note: this include must follow xdraw.h and regex.h for the xdraw and
// regex handles to be defined.  These are entirely inline.

enum HDLtype
{
    HDLgeneric,
    HDLstring,
    HDLobject,
    HDLfd,
    HDLprpty,
    HDLnode,
    HDLterminal,
    HDLdevice,
    HDLdcontact,
    HDLsubckt,
    HDLscontact,
    HDLgen,
    HDLgraph,
    HDLregex,
    HDLajob,
    HDLbstream
};

// enum:          datatype:       iterator returns:  deletes data?
// HDLgeneric     no data type    data               no
// HDLstring      stringlist      char*              yes (not string)
// HDLobject      CDol            CDo*               yes (object too if copy)
// HDLfd          int             data               no
// HDLprpty       CDpl            CDp*               yes (not property)
// HDLnode        tlist2<>        CDp_nodeEx*        yes (not property)
// HDLterminal    tlist<>         CDterm*            yes (not terminal)
// HDLdevice      tlist<>         sDevInst*          yes (not device)
// HDLdcontact    tlist<>         sDevContactInst*   yes (not contact)
// HDLsubckt      tlist<>         sSubcInst*         yes (not subckt)
// HDLscontact    tlist<>         sSubcContactInst*  yes (not contact)
// HDLgen         generator       object handle      no
// HDLgraph       Xdraw           data               yes
// HDLregex       regex_t         data               yes
// HDLajob        ajob_t          data               yes
// HDLbstream     cv_incr_reader  CDo*               yes

class cGroupDesc;
struct CDo;
struct CDol;
struct CDp;
struct CDp_nodeEx;
struct CDpl;
struct CDterm;
struct sDevInst;
struct sDevContactInst;
struct sSubcInst;
struct sSubcContactInst;
struct ajob_t;
struct sPF;
struct cv_incr_reader;

template<class T> struct tlist
{
    tlist(T *e, tlist *n)   { elt = e; next = n; }

    static void destroy(tlist *s)
        {
            while (s) {
                tlist *sx = s;
                s = s->next;
                delete sx;
            }
        }

    static int count(const tlist *thistl)
        {
            int cnt = 0;
            for (const tlist *t = thistl; t; t = t->next, cnt++) ;
            return (cnt);
        }

    T *elt;
    tlist *next;
};

template<class T> struct tlist2
{
    tlist2(T *e, void *x, tlist2 *n) { elt = e; xtra = x; next = n; }

    static void destroy(tlist2 *s)
        {
            while (s) {
                tlist2 *sx = s;
                s = s->next;
                delete sx;
            }
        }

    static int count(const tlist2 *thistl) {
        int cnt = 0;
        for (const tlist2 *t = thistl; t; t = t->next, cnt++) ;
        return (cnt);
    }

    T *elt;
    tlist2 *next;
    void *xtra;
};

// The sHdlUniq is a hash table wrapper for efficiently testing if an
// object is already in a list.

struct SymTab;

struct sHdlUniq
{
    sHdlUniq(struct sHdl*);
    ~sHdlUniq();

    bool test(struct sHdl*);

    bool active()   { return (hu_tab != 0); }
    bool copies()   { return (hu_copies); }

private:
    SymTab *hu_tab;
    HDLtype hu_type;
    bool hu_copies;
};


// Base type for handles to lists, objects, etc.
//
struct sHdl
{
    sHdl(void*, int = 0);
    virtual ~sHdl();
    virtual void *iterator() { return (data); }
    virtual int close(int) { delete this; return (0); }
    virtual const char *string() { return ("generic data"); }

    int id;             // integer id of handle
    HDLtype type;       // type of object(s) referenced
    void *data;         // storage for object(s)

    static SymTab *HdlTable;    // table of handles, referenced by id
    static int HdlIndex;        // id assignment counter

    static sHdl *get(int);
    static void update(CDo*, CDo*);
    static void update(CDo*, CDp*, CDp*);
    static void update(CDs*);
    static void update(cGroupDesc*);
    static void update();
    static void clear();
    static CDpl *prp_list(CDp*);
    static CDol *sel_list(CDol*);
    static sHdlUniq *new_uniq(sHdl*);
};

// Handle type for stringlist.
//
struct sHdlString : public sHdl
{
    sHdlString(stringlist *s) : sHdl(s) { type = HDLstring; }
    ~sHdlString();

    void *iterator();
    const char *string() { return ("string list"); }
};

// Handle type for objects.
//
struct sHdlObject : public sHdl
{
    sHdlObject(CDol *ol, CDs *sd, bool cp = false) : sHdl(ol)
        { type = HDLobject; sdesc = sd; copies = cp; }
    ~sHdlObject();

    void *iterator();
    const char *string() { return ("object list"); }

    CDs *sdesc;     // cell containing objects
    bool copies;    // object descs are copies
};

// Handle type for file descriptors.  This keeps three types of descriptor:
//  1) common file descriptors    (id = fd, data is open mode, fp = 0)
//  2) popen descriptor           (id = fd, data is open mode, fp used)
//  3) sockets                    (id = sd, data = 0, fp = 0)
// Win32 support requires keeping sockets separate.
//
struct sHdlFd : public sHdl
{
    sHdlFd(int hd, const char *mode) : sHdl(lstring::copy(mode), hd)
        { fp = 0; type = HDLfd; }
    sHdlFd(FILE *f, const char *mode) : sHdl(lstring::copy(mode), fileno(f))
        { fp = f; type = HDLfd; }
    ~sHdlFd();

    int close(int);
    const char *string() { return ("file descriptor"); }

    FILE *fp;  // used only for popen()
};

// Handle type for properties.
//
struct sHdlPrpty : public sHdl
{
    sHdlPrpty(CDpl *prp, CDo *od, CDs *sd) : sHdl(prp)
        { type = HDLprpty; odesc = od; sdesc = sd; }
    ~sHdlPrpty();

    void *iterator();
    const char *string() { return ("properties list"); }

    CDo *odesc; // object containing properties
    CDs *sdesc; // cell containing object;
};

// Handle type for cell node properties.
//
struct sHdlNode : public sHdl
{
    sHdlNode(tlist2<CDp_nodeEx> *ct) : sHdl(ct)
        { type = HDLnode; }
    ~sHdlNode();

    void *iterator();
    const char *string() { return ("cell/instance node property list"); }
};

// Handle type for terminals.
//
struct sHdlTerminal : public sHdl
{
    sHdlTerminal(tlist<CDterm> *ct) : sHdl(ct)
        { type = HDLterminal; }
    ~sHdlTerminal();

    void *iterator();
    const char *string() { return ("physical terminal list"); }
};

// Handle type for devices.
//
struct sHdlDevice : public sHdl
{
    sHdlDevice(tlist<sDevInst> *dv, cGroupDesc *gd) : sHdl(dv)
        { type = HDLdevice; gdesc = gd; }
    ~sHdlDevice();

    void *iterator();
    const char *string() { return ("device instance list"); }

    cGroupDesc *gdesc;
};

// Handle type for device contacts.
//
struct sHdlDevContact : public sHdl
{
    sHdlDevContact(tlist<sDevContactInst> *ct, cGroupDesc *gd) : sHdl(ct)
        { type = HDLdcontact; gdesc = gd; }
    ~sHdlDevContact();

    void *iterator();
    const char *string() { return ("device instance contact list"); }

    cGroupDesc *gdesc;
};

// Handle type for subckts.
//
struct sHdlSubckt : public sHdl
{
    sHdlSubckt(tlist<sSubcInst> *su, cGroupDesc *gd) : sHdl(su)
        { type = HDLsubckt; gdesc = gd; }
    ~sHdlSubckt();

    void *iterator();
    const char *string() { return ("subcircuit instance list"); }

    cGroupDesc *gdesc;
};

// Handle type for subckt contacts.
//
struct sHdlSubcContact : public sHdl
{
    sHdlSubcContact(tlist<sSubcContactInst> *ct, cGroupDesc *gd) : sHdl(ct)
        { type = HDLscontact; gdesc = gd; }
    ~sHdlSubcContact();

    void *iterator();
    const char *string() { return ("subcircuit instance contact list"); }

    cGroupDesc *gdesc;
};


// This is a tack-on so that the generator below can iterate over
// multiple layers.
//
struct gdrec
{
    gdrec(const BBox *BB, int d, DisplayMode m, stringlist *s)
        { AOI = *BB; depth = d; mode = m; names = s; }
    ~gdrec() { stringlist::destroy(names); }

    static gdrec *dup(const gdrec *gt) {
        if (!gt)
            return (0);
        return (new gdrec(&gt->AOI, gt->depth, gt->mode,
            stringlist::dup(gt->names)));
    }

    BBox AOI;
    int depth;
    DisplayMode mode;
    stringlist *names;
};

// Handle type for pseudo-flat generator.
//
struct sHdlGen : public sHdl
{
    sHdlGen(sPF *g, CDs *sd, gdrec *r) : sHdl(g)
        { type = HDLgen; sdesc = sd; rec = r; }
    ~sHdlGen();

    void *iterator();
    const char *string() { return ("hierarchy traversal generator"); }

    CDs *sdesc;
    gdrec *rec;
};

#ifdef XDRAW_H
// defined in xdraw.h

// Handle type for graphical interface.
//
struct sHdlGraph : public sHdl
{
    sHdlGraph(Xdraw *xd) : sHdl(xd) { type = HDLgraph; }
    ~sHdlGraph() { delete (Xdraw*)data; }

    const char *string() { return ("graphical interface"); }
};
#endif

#ifdef REG_ICASE
// defined in regex.h

// Handle type for regular expressions.
//
struct sHdlRegex : public sHdl
{
    sHdlRegex(regex_t *px) : sHdl(px) { type = HDLregex; }
    ~sHdlRegex()
        {
            regfree((regex_t*)data);
            delete (regex_t*)data;
        }

    const char *string() { return ("compiled regular expression"); }
};
#endif


// Handle type for "assemble" control struct.
//
struct sHdlAjob : public sHdl
{
    sHdlAjob(ajob_t *job) : sHdl(job) { type = HDLajob; }
    ~sHdlAjob();

    const char *string() { return ("assemble job"); }
};


// Handle type for the incremental parser of compressed geometry streams.
//
struct sHdlBstream : public sHdl
{
    sHdlBstream(cv_incr_reader *irdr) : sHdl(irdr) { type = HDLbstream; }
    ~sHdlBstream();

    const char *string() { return ("incremental geometry stream parser"); }
};

#endif

