
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
 $Id: select.h,v 5.80 2013/02/14 00:58:10 stevew Exp $
 *========================================================================*/

#ifndef SELECT_H
#define SELECT_H

// The selections list.

// Theory of Operation
//
// Objects are saved in doubly-linked lists, the lists are keyed by a
// cell struct (CDs).  Empty list are given back to the pool by
// zeroing the CDs pointer.  This occurs when a new list is requested,
// and possibly at other times.  In addition to the linked list,
// objects key a hash table for rapid access by address.
//
// If a listed object has the oState value CDSelected, then the object
// will be shown as blinking-highlighted in windows showing the cell
// whose list contains the object.  If the object has oState set to
// some other value, it is "invisible", but may be used for such
// things as ghosting and temporary storage within commands.
//
// When no command is in force, all objects should have oState set to
// CDSelected.  This is checked and enforced by the check() method,
// which can be called between commands and at other times to check
// consistency.
//
// Several of the functions take a types string argument which
// determines which object types are acted on.  A null or empty string
// is an "all" wildcard.  Otherwise the string consists of one or more
// of the characters c, b, p, w, l for instances, boxes, polygons,
// wires, and labels.  If an object oType character is found in the
// string, the object will be included in the operation.

// Default threshold for selection list blinking.  If there are more
// than the threshold of selected objects, blinking is disabled in
// true-color display modes.  If DSP()->NoPixmapStore() is set, a
// fraction (1/8) of the threshold is used.
//
#define DEF_MAX_BLINKING_OBJECTS 25000

enum PTRmode { PTRnormal, PTRselect, PTRmodify };
//
//  Enum to specify how button operations are to be interpreted:
//  PTRnormal       Normal mode
//  PTRselect       Selection operations only
//  PTRmodify       Move/Copy/Stretch operations only

enum SELmode { SELnormal, SELtoggle, SELselect, SELdesel };
//
//  Enum to specify how selections are performed:
//  SELnormal       Normal mode for selections
//  SELtoggle       Switch selected/deselected status
//  SELselect       Add unselected to selections
//  SELdesel        Remove selected from selections

enum ASELmode { ASELnormal, ASELenclosed, ASELall };
//
//  Enum to specify area-select defaults:
//  ASELnormal      Normal area select
//  ASELenclosed    Objects must be contained in box
//  ASELall         Select anything that intersects box

enum PSELmode { PSELpoint, PSELstrict_area, PSELstrict_area_nt };
//
//  Special enum passed to selectItems():
//  PSELpoint       Normal point selections
//  PSELstrict_area Don't compensate for screen resol, take box as given
//  PSELstrict_area_nt As above, but objects must overlap, not merely touch


// Selection queue element.  We maintain a doubly-linked list of
// objects, with the most recently inserted object at the list head. 
// In addition, a hash table is maintained, for rapid access to list
// objects for removal, and to ensure uniqueness of list objects.
//
struct sqel_t
{
    unsigned long tab_key()         { return ((unsigned long)odesc); }
    sqel_t *tab_next()              { return (tabnext); }
    void set_tab_next(sqel_t *n)    { tabnext = n; }

    sqel_t *next;                   // Next object in list.
    sqel_t *prev;                   // Previous object in list, 0 for head.
    CDo *odesc;                     // Object pointer.
private:
    sqel_t *tabnext;                // Table next pointer.
};

#define SQF_BLSIZE 256

// A factory for sqel_t elements.  These are bulk-allocated.
//
struct sqelfct_t
{
    struct sqfblock_t
    {
        sqfblock_t(sqfblock_t *n)   { next = n; }

        sqfblock_t *next;
        sqel_t elements[SQF_BLSIZE];
    };

    sqelfct_t()
        {
            sqf_deleted = 0;
            sqf_blocks = 0;
            sqf_alloc = 0;
        }

    ~sqelfct_t()
        {
            while (sqf_blocks) {
                sqfblock_t *b = sqf_blocks;
                sqf_blocks = sqf_blocks->next;
                delete b;
            }
        }

    void clear()
        {
            while (sqf_blocks) {
                sqfblock_t *b = sqf_blocks;
                sqf_blocks = sqf_blocks->next;
                delete b;
            }
            zero();
        }

    void zero()
        {
            sqf_deleted = 0;
            sqf_blocks = 0;
            sqf_alloc = 0;
        }

    sqel_t *new_element();
    void delete_element(sqel_t*);

private:
    sqel_t *sqf_deleted;        // Deleted elements, for reuse.
    sqfblock_t *sqf_blocks;     // Blocks of allocated elements.
    int sqf_alloc;              // count of allocated elements in block.
};

struct Zlist;

// Argument to selqueue_t::insert_object.
enum SQinsMode { SQinsNoShow, SQinsProvShow, SQinsShow };

// Selected item list for container cell.
//
struct selqueue_t
{
    selqueue_t()
        {
            sq_sdesc = 0;
            sq_list = 0;
            sq_tab = 0;
        }

    const CDs *celldesc()               const { return (sq_sdesc); }
    void set_celldesc(const CDs *sd)          { sq_sdesc = sd; }

    const sqel_t *queue()               const { return (sq_list); }

    void reset()
        {
            sq_fct.clear();
            delete sq_tab;
            sq_tab = 0;
            sq_list = 0;
            sq_sdesc = 0;
        }

    void zero()
        {
            sq_fct.zero();
            sq_tab = 0;
            sq_list = 0;
            sq_sdesc = 0;
        }

    unsigned int queue_length()
        {
            if (sq_tab)
                return (sq_tab->allocated());
            return (0);
        }

    bool insert_object(CDo*, SQinsMode);
    bool replace_object(CDo*, CDo*);
    void insert_and_show(CDol*);
    bool remove_object(CDo*);
    void remove_types(const char*);
    void remove_layer(const CDl*);
    void deselect_types(const char*);
    void deselect_layer(const CDl*);
    bool deselect_last();

    bool compute_bb(BBox*, bool) const;
    void count_queue(unsigned int*, unsigned int*) const;
    void set_show_selected(const char*, bool);
    unsigned int show(WindowDesc*) const;
    bool has_types(const char*, bool) const;
    bool in_queue(CDo*) const;
    CDo *first_object(const char*, bool, bool) const;

    void purge_deleted();
    void add_labels();
    void purge_labels(bool);
    void inst_to_front();
    Zlist *get_zlist(const CDl*, const Zlist*) const;
    bool check();

    static void show_selected(const CDs*, CDo*);
    static void show_unselected(const CDs*, CDo*);
    static void redisplay_list(const CDs*, CDol*);

private:
    const CDs *sq_sdesc;
    sqel_t *sq_list;
    itable_t<sqel_t> *sq_tab;
    sqelfct_t sq_fct;
};


// Number of queues that are available.
#define SEL_NUMQUEUES 10

// Main application class for selections.
//
class cSelections
{
public:
    friend struct sSelGen;

    cSelections()
        {
            reset();
            s_display_count = 0;
        }

    void reset()
        {
            s_ptr_mode = PTRnormal;
            s_sel_mode = SELnormal;
            s_area_mode = ASELnormal;
            memset(s_types, 0, sizeof(s_types));
            s_types[0] = CDINSTANCE;
            s_types[1] = CDPOLYGON;
            s_types[2] = CDLABEL;
            s_types[3] = CDWIRE;
            s_types[4] = CDBOX;
            s_search_up = false;
        }

    // Return true is the selections can be shown blinking.  If there
    // are too many selections, they will be shown highlighted but
    // won't blink.
    //
    bool blinking()
        {
            if (DSP()->NoPixmapStore())
                return (s_display_count < (blink_thresh >> 3));
            return (s_display_count < blink_thresh);
        }

    // Deselect and clear all lists, or all list other than the list
    // for keepme, if passed.
    //
    void deselectAll(CDs *keepme = 0)
        {
            for (int i = 0; i < SEL_NUMQUEUES; i++) {
                if (s_queues[i].celldesc() &&
                        s_queues[i].celldesc() != keepme) {
                    s_queues[i].deselect_types(0);
                    s_queues[i].remove_types(0);
                    s_queues[i].reset();
                }
            }
        }

    // Deselect and remove all objects on ld, from all lists.
    //
    void deselectAllLayer(const CDl *ld)
        {
            if (!ld)
                return;
            for (int i = 0; i < SEL_NUMQUEUES; i++) {
                selqueue_t *sq = s_queues + i;
                if (sq->celldesc() &&
                        ld->index(sq->celldesc()->displayMode()) >= 0) {
                    sq->deselect_layer(ld);
                    sq->remove_layer(ld);
                }
            }
        }

    // Remove all objects on ld, from all lists.  There is no redisplay.
    //
    void removeAllLayer(const CDl *ld)
        {
            if (!ld)
                return;
            for (int i = 0; i < SEL_NUMQUEUES; i++) {
                selqueue_t *sq = s_queues + i;
                if (sq->celldesc() &&
                        ld->index(sq->celldesc()->displayMode()) >= 0) {
                    sq->remove_layer(ld);
                }
            }
        }

    // Insert od into the list, return true if od is in the list.  If
    // noshow is false, it will be displayed as selected if its state
    // was CDVanilla.  If noshow is true, this will be skipped.  In
    // either case, the object state is set to CDSelected.
    //
    bool insertObject(const CDs *sd, CDo *od, bool noshow = false)
        {
            bool ret = false;
            selqueue_t *sq = findQueue(sd, true);
            if (sq)
                ret = sq->insert_object(od,
                    noshow ? SQinsNoShow : SQinsProvShow);
            return (ret);
        }

    // Insert each of the objects into the list.  The state of each
    // object will be retained,
    //
    void insertList(const CDs *sd, CDol *list)
        {
            selqueue_t *sq = 0;
            for (CDol *ol = list; ol; ol = ol->next) {
                if (ol->odesc) {
                    if (!sq) {
                        sq = findQueue(sd, true);
                        if (!sq)
                            return;
                    }
                    int st = ol->odesc->state();
                    sq->insert_object(ol->odesc, SQinsNoShow);
                    ol->odesc->set_state(st);
                }
            }
        }

    // Transfer the queue for sd into sqout, if found, and zero the
    // queue.
    //
    void getQueue(const CDs *sd, selqueue_t *sqout)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq) {
                *sqout = *sq;
                sq->zero();
            }
            else
                sqout->zero();
        }

    // Transfer sqin into the queue list.  This will clear any
    // previous content.  The sqin returns zeroed.
    //
    void setQueue(selqueue_t *sqin)
        {
            if (sqin->celldesc()) {
                if (sqin->has_types(0, true)) {
                    selqueue_t *sq = findQueue(sqin->celldesc(), true);
                    if (sq) {
                        sq->reset();
                        *sq = *sqin;
                        sqin->zero();
                        sq->purge_deleted();
                    }
                }
                else {
                    selqueue_t *sq = findQueue(sqin->celldesc(), false);
                    if (sq)
                        sq->reset();
                    sqin->reset();
                }
            }
        }

    // Replace od, if found, with odnew.  Return true if replacement
    // done.  No redisplay is done.
    //
    bool replaceObject(const CDs *sd, CDo *od, CDo *odnew)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                return (sq->replace_object(od, odnew));
            return (false);
        }

    // Remove od from the list, if found.  If oState is CDSelected, it
    // will be shown as deselected and set to CDVanilla.  Explicitly
    // set oState to CDVanilla to suppress the redisplay.
    //
    void removeObject(const CDs *sd, CDo *od)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq) {
                selqueue_t::show_unselected(sd, od);
                sq->remove_object(od);
            }
        }

    // Remove all objects matching types from the list, setting the
    // state to CDVanilla.  There is no redisplay.
    //
    void removeTypes(const CDs *sd, const char *types)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                sq->remove_types(types);
        }

    // Remove all objects on ld from the list, set the states to
    // CDVanilla.  there is no redisplay.
    //
    void removeLayer(const CDs *sd, const CDl *ld)
        {
            if (!ld)
                return;
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                sq->remove_layer(ld);
        }

    // For each object in the list that matches types and has state
    // CDSelected, remove the object from the list, set the state to
    // CDVanilla, and redisplay the object.
    //
    void deselectTypes(const CDs *sd, const char *types)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                sq->deselect_types(types);
        }

    // Remove the list head object.  If it had state CDSelected, it
    // will be redisplayed as unselected, and state set to CDVanilla.
    //
    bool deselectLast(const CDs *sd)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                return (sq->deselect_last());
            return (false);
        }

    // Compute the bounding box of objects in the list.  If all is
    // false of not given, include only objects with state CDSelected. 
    // Otherwise, include all objects.
    //
    bool computeBB(const CDs *sd, BBox *nBB, bool all = true)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                return (sq->compute_bb(nBB, all));
            if (nBB)
                *nBB = CDnullBB;
            return (false);
        }

    // Return the total number of elements in the list, from the table
    // so very efficient.
    //
    unsigned int queueLength(const CDs *sd)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                return (sq->queue_length());
            return (0);
        }

    // Return a count of objects in the list, separate counts for
    // CDSelected objects and other objects.  Requires list traversal.
    //
    void countQueue(const CDs *sd, unsigned int *ns, unsigned int *nu)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                sq->count_queue(ns, nu);
            else {
                if (ns)
                    *ns = 0;
                if (nu)
                    *nu = 0;
            }
        }

    // If on is true, set the state of all objects in the list that
    // match types to CDSelected and redraw.  If on is false, set the
    // state of all objects in the list that match types to CDVanilla
    // and redraw.
    //
    void setShowSelected(const CDs *sd, const char *types, bool on)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                sq->set_show_selected(types, on);
        }

    // Return true if there is an object in the list that matches types
    // and is selected or all is set.
    //
    bool hasTypes(const CDs *sd, const char *types, bool all = true)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                return (sq->has_types(types, all));
            return (false);
        }

    // Return true if od is in the list.
    //
    bool inQueue(const CDs *sd, CDo *od)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                return (sq->in_queue(od));
            return (false);
        }

    // Return a pointer to the first matching object found in the
    // list.
    //
    CDo *firstObject(const CDs *sd, const char *types, bool all = true,
        bool array_only = false)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                return (sq->first_object(types, all, array_only));
            return (0);
        }

    // Remove any objects with state CDDeleted from the list.  There
    // is no redisplay.
    //
    void purgeDeleted(const CDs *sd)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                sq->purge_deleted();
        }

    // In electrical mode, for each instance in the list, bring in the
    // associated labels that have state CDVanilla.  These will be
    // set to CDSelected, added to the list, and displayed.
    //
    void addLabels(const CDs *sd)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                sq->add_labels();
        }

    // Remove from the list any property labels that are bound to
    // instances in the list, or all property labels if all is set. The
    // labels will have state changed to CDVanilla and will be
    // redisplayed if the previous state was CDSelected.
    //
    void purgeLabels(const CDs *sd, bool all = false)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                sq->purge_labels(all);
        }

    // Move all of the instances to the front of the list.  This is
    // needed in electrical mode, we need to add instances before
    // their bound labels.
    //
    void instanceToFront(const CDs *sd)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                sq->inst_to_front();
        }

    // Return a Zlist representing all objects on ld, clipped to zl.
    //
    Zlist *getZlist(const CDs *sd, const CDl *ld, const Zlist *zl)
        {
            selqueue_t *sq = findQueue(sd, false);
            if (sq)
                return (sq->get_zlist(ld, zl));
            return (0);
        }

    // Static function.
    // If od has state CDVanilla, change the state to CDSelected and
    // render as selected.  The object may or may not be in any list. 
    // The cell pointer determines the windows where redisplay takes
    // place.
    //
    void showSelected(const CDs *sd, CDo *od)
        {
            selqueue_t::show_selected(sd, od);
        }

    // Static function.
    // If od has state CDSelected, set the state to CDVanilla and
    // redisplay.  The od may or may not be in any list, the cell
    // pointer determines the windows for redisplay.
    //
    void showUnselected(const CDs *sd, CDo *od)
        {
            selqueue_t::show_unselected(sd, od);
        }

    void setPtrMode(PTRmode p)      { s_ptr_mode = p; }
    PTRmode ptrMode()               { return (s_ptr_mode); }

    void setSelMode(SELmode a)      { s_sel_mode = a; }
    SELmode selMode()               { return (s_sel_mode); }

    void setAreaMode(ASELmode a)    { s_area_mode = a; }
    ASELmode areaMode()             { return (s_area_mode); }

    bool typeSelected(const CDo *od) { return (od &&
                                     strchr(s_types, od->type())); }
    const char *selectTypes()       { return (s_types); }

    void setSelectType(int c, bool sel)
        {
            char bf[8];
            strcpy(bf, s_types);
            char *s = bf, *t = s_types;
            while (*s) {
                if (*s != c)
                    *t++ = *s;
                s++;
            }
            if (sel)
                *t++ = c;
            *t = 0;
        }

    void setLayerSearchUp(bool b)   { s_search_up = b; }
    bool layerSearchUp()            { return (s_search_up); }

    void setMaxBlinkingObjects(unsigned int i)
                                    { blink_thresh = i; }

    bool selection(const CDs*, const char*, const BBox*, bool = false);
    CDol *selectItems(const CDs*, const char*, const BBox*, PSELmode);
    CDol *filter(const CDs*, CDol*, const BBox*, bool);
    CDol *listQueue(const CDs*, const char* = 0, bool = true);
    void show(WindowDesc*);
    void check();
    void parseSelections(const CDs *sd, const char*, bool);

    static bool processSelect(CDo*, const BBox*, PSELmode, ASELmode, int);

private:
    selqueue_t *findQueue(const CDs *sd, bool alloc)
        {
            if (!sd)
                return (0);
            for (int i = 0; i < SEL_NUMQUEUES; i++) {
                if (s_queues[i].celldesc() == sd)
                    return (&s_queues[i]);
            }
            if (alloc) {
                // First run the list and zero out any empty queues.
                for (int i = 0; i < SEL_NUMQUEUES; i++) {
                    if (!s_queues[i].has_types(0, true))
                        s_queues[i].reset();
                }
                for (int i = 0; i < SEL_NUMQUEUES; i++) {
                    if (!s_queues[i].celldesc()) {
                        s_queues[i].set_celldesc(sd);
                        return (&s_queues[i]);
                    }
                }
            }
            return (0);
        }

    void pmatch(const CDs*, int, const char*, bool);
    void cmatch(const CDs*, const char*, bool, CDol** = 0);
    void lmatch(const CDs*, const char*, bool, CDol** = 0);

    selqueue_t s_queues[SEL_NUMQUEUES]; // The available queues.

    PTRmode s_ptr_mode;             // Modes.
    SELmode s_sel_mode;
    ASELmode s_area_mode;

    unsigned int s_display_count;   // Count selected objects.
    char s_types[8];                // Which types of object will be selected.
    bool s_search_up;

    static unsigned int blink_thresh;  // Don't blink if too many selections.
};

extern cSelections Selections;

// Generator for list element retrieval.
//
struct sSelGen
{
    sSelGen(cSelections&, const CDs*, const char* = 0, bool = true);

    CDo *next();
    void remove();
    void replace(CDo*);

private:
    selqueue_t *sg_sq;
    const sqel_t *sg_el;
    CDo *sg_obj;
    const char *sg_types;
    bool sg_all;
};

#endif

