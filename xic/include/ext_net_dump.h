
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
 $Id: ext_net_dump.h,v 5.5 2009/06/30 07:31:58 stevew Exp $
 *========================================================================*/

#ifndef EXT_NET_DUMP_H
#define EXT_NET_DUMP_H


struct emrec_t;
struct oas_out;
struct cv_in;
struct sGroup;

// Flags:
//
// EN_FLAT
//   When set, an additional processing step flattens the netlist
//   subcells.  This will require more memory and processing time than
//   non-flat mode, which keeps connected subnets as subcells in the
//   primary net cell.
//
// EN_COMP
//   Use compression, string tables, and the repetition finder when
//   generating OASIS output.  this produces smaller files, but takes a
//   bit more time.
//
// EN_EXTR
//   If set, perform device extraction after grouping when identifying
//   nets.  This may break up nets that connect to/through devices.  It
//   is somewhat more compute intensive than simple grouping, which
//   takes no account of device structures.
//
// EN_LFLT
//   When set, the run will use whatever layer filtering was set up
//   externally before the run, when reading the sections into memory. 
//   When not set and grouping only (EN_EXTR not set), layer filtering
//   will be applied so that only CONDUCTOR and VIA layers are read. 
//   When not set and extracting, layer filtering is turned off so all
//   layers are read, since they may be needed for device recognition.
//
// EN_VIAS
//   Include vias in the net cells.  Ovjects on via layers that are
//   recognized as vias are retained.
//
// EN_VTRE
//   When EN_VIAS is given, also save objects on layers needed for via
//   recognition, when the via layer provides an expression for this
//   purpose.  The objects are clipped to the via object.  When this
//   and EN_VIAS are set and EN_EXTR is not set, the tree layers will
//   be read into memory in stage 1.
//
// --- Debugging Flags ---
//
// EN_EQVB
//   If set, print the layer and overlap areas in the equivalence file,
//   for debugging.
//
// EN_KEEP
//   Don't unlink work files (OASIS grid files, edge files, equivalence
//   file).
//
// EN_STP1
//   Stop after Stage 1.
//
// EN_STP2
//   Stop after Stage 2.
//
#define EN_FLAT 0x1
#define EN_COMP 0x2
#define EN_EXTR 0x4
#define EN_LFLT 0x8
#define EN_VIAS 0x10
#define EN_VTRE 0x20
//
#define EN_EQVB 0x100
#define EN_KEEP 0x200
#define EN_STP1 0x400
#define EN_STP2 0x800


class cExtNets
{
public:
    cExtNets(cCHD*, const char*, const char*, unsigned int);
    ~cExtNets() { delete [] en_basename; }

    // Stage 1
    bool dump_nets_grid(int);
    bool dump_nets(const BBox*, int, int);

    // Stage 2
    bool stage2();
    bool parse_edge_file(FILE*, SymTab**);
    bool reduce(FILE*, SymTab*, SymTab*, int, int, int, int);
    bool reduce(emrec_t*, emrec_t*, FILE*, int, int, int, int, const char*);

    // Stage 3
    bool stage3();
    unsigned int net_count() const { return (en_netcnt); }

private:
    bool write_metal_file(const CDs*, int, int) const;
    bool write_vias(const CDs*, const sGroup*, oas_out*) const;
    bool write_edge_map(const CDs*, const BBox*, int, int) const;
    bool add_listed_nets(bool, stringlist*, oas_out*, cCHD**, cv_in*) const;

    cCHD *en_chd;
    const char *en_cellname;
    char *en_basename;
    int en_nx;
    int en_ny;
    unsigned int en_flags;
    unsigned int en_netcnt;
};


// Edge mapping record list element.
//
struct emrec_t
{
    emrec_t(const char *ln, int s, int e, int g, emrec_t *n)
        {
            next = n;
            lname = ln;
            start = s;
            end = e;
            group = g;
        }

    static void destroy(emrec_t *em)
        {
            while (em) {
                emrec_t *ex = em;
                em = em->next;
                delete ex;
            }
        }

    const char *layer_name() const { return (lname); }
    int cmpval() const { return (start); }
    int grpval() const { return (group); }
    int startval() const { return (start); }
    int endval() const { return (end); }
    emrec_t *next_rec() { return (next); }
    void set_next_rec(emrec_t *r) { next = r; }

    emrec_t *sort_edge();
    void print(FILE*, int);

private:
    emrec_t *sort();

    emrec_t *next;
    const char *lname;
    int start, end;
    int group;
};

#endif

