
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

#include "cd.h"
#include "cd_types.h"
#include "cd_sdb.h"
#include "fio.h"
#include "fio_crgen.h"
#include <signal.h>


//-----------------------------------------------------------------------------
// The instance attributes database.
//
// The "attributes" (transformation and array parameters) are
// consolidated to save memory, since attribute sets may be used many
// times.
//
// The attributes are described in a CDattr struct, and saved under a
// ticket_t.  Cell instance descriptions save the ticket_t instead of the
// struct.
//
// The 4 lsb's of the ticket identify the rotations and reflection.  The
// rotations are encoded in the first three bits as:
//
// ax ay
//  1  0        000
//  0 -1        001
// -1  0        010
//  0  1        011
//  1  1        100
//  1 -1        101
// -1 -1        110
// -1  1        111
//
// The fourth bit is the y-reflection flag.  The remaining bits are a
// "real" ticket into a database of compressed strings that represent the
// array parameters and magnification.

//-----------------------------------------------------------------------------
// Compact hash table for attribute entries
//-----------------------------------------------------------------------------

namespace cd_attrdb {
    struct atitem_t
    {
        const unsigned char *ptr;
        atitem_t *next;
        ticket_t ticket;
    };

    struct attable_t
    {
        attable_t()
            {
                count = 0;
                hashmask = 0;
                array = 0;
            }

        ~attable_t()
            {
                delete [] array;
            }

        ticket_t find(const unsigned char*);
        void add(const unsigned char*, ticket_t);

        int allocated() { return (count); }
        int hashwidth() { return (hashmask+1); }

    private:

        // Comparison function for byte streams, returns true if match.
        //
        bool
        compare(const unsigned char *s1, const unsigned char *s2)
            {
                if (*s1 != *s2)
                    return (false);
                int n = *s1 >> 3;  // encoded total length
                return (!memcmp(s1, s2, n));
            }

        unsigned int hash(const unsigned char*);
        void check_rehash();

        unsigned int count;             // count of elements added
        unsigned int hashmask;          // variable width hash mask
        atitem_t **array;               // array of list heads
        tGEOfact<atitem_t> factory;     // allocator for atitem_t
    };


    // Return the ticket corresponding to the byte stream, 0 if not found.
    //
    ticket_t
    attable_t::find(const unsigned char *str)
    {
        if (array && str) {
            unsigned int i = hash(str);
            for (atitem_t *e = array[i]; e; e = e->next) {
                if (compare(str, e->ptr))
                    return (e->ticket);
            }
        }
        return (0);
    }


    // Add a byte stream and corresponding ticket.  The ticket is an
    // arbitrary payload.  The first byte of the stream is special:
    // first_byte >> 3 is the total length of the stream.
    //
    void
    attable_t::add(const unsigned char *str, ticket_t tkt)
    {
        if (!str)
            return;
        if (!array) {
            array = new atitem_t*[hashmask + 1];
            for (unsigned int i = 0; i <= hashmask; i++)
                array[i] = 0;
        }
        unsigned int i = hash(str);
        atitem_t *e = factory.new_obj();
        e->ptr = str;
        e->ticket = tkt;
        e->next = array[i];
        array[i] = e;
        count++;
        check_rehash();
    }


    // Hash generator for byte stream.
    //
    unsigned int
    attable_t::hash(const unsigned char *str)
    {
        if (!hashmask || !str)
            return (0);
        int num = *str >> 3;  // encoded total length
        unsigned int nhash = 5381;
        for ( ; num > 0; num--, str++)
            nhash = ((nhash << 5) + nhash) ^ *str;
        return (nhash & hashmask);
    }


    // Hash table resizing function, called after adding each element.
    //
    void
    attable_t::check_rehash()
    {
        if (count/(hashmask+1) <= ST_MAX_DENS)
            return;

        unsigned int newmask = (hashmask << 1) | 1;
        atitem_t **atmp = new atitem_t*[newmask + 1];
        for (unsigned int i = 0; i <= newmask; i++)
            atmp[i] = 0;
        unsigned int oldmask = hashmask;
        hashmask = newmask;
        for (unsigned int i = 0;  i <= oldmask; i++) {
            atitem_t *en;
            for (atitem_t *e = array[i]; e; e = en) {
                en = e->next;
                unsigned int j = hash(e->ptr);
                e->next = atmp[j];
                atmp[j] = e;
            }
        }
        delete [] array;
        array = atmp;
    }
}


//-----------------------------------------------------------------------------
// Database for attributes
//-----------------------------------------------------------------------------

// The three LSBs of the first character of the byte stream, specifies
// what follows.  The remaining bits contain the string length.
#define TF_NX   0x1
#define TF_NY   0x2
#define TF_MG   0x4

namespace cd_attrdb {
    struct adb_t
    {
        adb_t() : reader(0), writer(0, 0, false) { }

        ticket_t save(CDattr*);
        const CDattr *recall(ticket_t);
        void print_stats();

    private:
        ticket_t new_tkt(CDattr*);

        bytefact_t factory;         // allocator for variable-length strings
        CDattr cdattr;              // return values container
        ts_reader reader;           // compressed stream reader
        ts_writer writer;           // compressed stream writer
        attable_t table;            // uniqueness table
        unsigned char scratch[32];  // buffer area
    };


    // Return a ticket corresponding to the attribute data.  The four LSBs
    // encode the rotation and reflection.  The remaining bits, if
    // nonzero, are a ticket for the array params and magnification.
    //
    ticket_t
    adb_t::save(CDattr *at)
    {
        if (!at)
            return (0);
        unsigned int c = at->encode_transform();
        if (at->nx <= 1 && at->ny <= 1 && at->magn == 1.0)
            return (c);

        ticket_t tk = new_tkt(at);
        return ((tk << 4) | c);
    }


    // Return the attribute data for the ticket.
    //
    const CDattr *
    adb_t::recall(ticket_t t)
    {
        cdattr.decode_transform(t);
        if (t < 16) {
            cdattr.nx = 1;
            cdattr.ny = 1;
            cdattr.dx = 0;
            cdattr.dy = 0;
            cdattr.magn = 1.0;
        }
        else {
            ticket_t tk = t >> 4;
            unsigned char *s = factory.find(tk);
            if (!s)
                // error, bogus ticket
                return (0);
            reader.init(s);
            unsigned char c = reader.read_uchar();
            if (c & TF_NX) {
                cdattr.nx = reader.read_unsigned();
                cdattr.dx = reader.read_signed();
            }
            else {
                cdattr.nx = 1;
                cdattr.dx = 0;
            }
            if (c & TF_NY) {
                cdattr.ny = reader.read_unsigned();
                cdattr.dy = reader.read_signed();
            }
            else {
                cdattr.ny = 1;
                cdattr.dy = 0;
            }
            if (c & TF_MG)
                cdattr.magn = reader.read_real();
            else
                cdattr.magn = 1.0;
        }
        return (&cdattr);
    }


    // Print database statistics.
    //
    void
    adb_t::print_stats()
    {
        printf("Ticket count: %d\n", table.allocated());
        printf("Bytes used: %llu\n", (unsigned long long)(
            factory.bytes_inuse() + table.allocated() * sizeof(atitem_t)));
        printf("Hash width: %d\n", table.hashwidth());
    }


    // Return a ticket for the array and magnification attributes.
    //
    ticket_t
    adb_t::new_tkt(CDattr *at)
    {
        if (!at)
            return (0);
        writer.init(scratch);
        unsigned char c = 0;
        if (at->nx > 1)
            c |= TF_NX;
        if (at->ny > 1)
            c |= TF_NY;
        if (at->magn != 1.0)
            c |= TF_MG;

        writer.add_uchar(c);
        if (at->nx > 1) {
            writer.add_unsigned(at->nx);
            writer.add_signed(at->dx);
        }
        if (at->ny > 1) {
            writer.add_unsigned(at->ny);
            writer.add_signed(at->dy);
        }
        if (c & TF_MG)
            writer.add_real(at->magn);
        unsigned long len = writer.bytes_used();

        // The length of the string is saved in the 5 free bits of the
        // header byte.
        *scratch |= (len << 3);

        ticket_t tk = table.find(scratch);
        if (tk)
            return (tk);
        tk = factory.get_space_nonvolatile(len);
        if (tk) {
            unsigned char *s = factory.find(tk);
            memcpy(s, scratch, len);
            table.add(s, tk);
        }
        else {
            fprintf(stderr, "Instance parameter ticket allocation failed!\n");
            fprintf(stderr, "This \"can't happen\", I have to exit.  Bye!\n");
            raise(SIGTERM);
        }
        return (tk);
    }
}
// End of adb_t functions.


// Return the CDattr given a ticket_t.
//
bool
cCD::FindAttr(ticket_t t, CDattr *at)
{
    if (!cdAttrDB && t < 16)
        // We need a database even if empty, to resolve the "standard"
        // tickets.
        cdAttrDB = new cd_attrdb::adb_t;
    const CDattr *atp = cdAttrDB ? cdAttrDB->recall(t) : 0;
    if (!atp)
        return (false);
    if (at)
        *at = *atp;
    return (true);
}


// Record a CDattr in the database, and return a ticket_t to it.
//
ticket_t
cCD::RecordAttr(CDattr *at)
{
    if (!cdAttrDB)
        cdAttrDB = new cd_attrdb::adb_t;
#ifdef ATTR_DBG
    ticket_t t = cdAttrDB->save(at);
    const CDattr *tmp = cdAttrDB->recall(t);
    if (tmp->ax != at->ax || tmp->ay != at->ay || tmp->refly != at->refly)
        printf("ax %d/%d ay %d/%d refly %d/%d\n", tmp->ax, at->ax,
            tmp->ay, at->ay, tmp->refly, at->refly);
    if (tmp->nx != at->nx || tmp->ny != at->ny)
        printf("nx %d/%d ny %d/%d\n", tmp->nx, at->nx, tmp->ny, at->ny);
    else if ((at->nx > 1 && tmp->dx != at->dx) ||
            (at->ny > 1 && tmp->dy != at->dy))
        printf("%d dx %d/%d  %d dy %d/%d\n", at->nx, tmp->dx, at->dx,
            at->ny, tmp->dy, at->dy);
    if (tmp->magn != at->magn)
        printf("magn %g/%g\n", tmp->magn, at->magn);
    return (t);
#endif
    return (cdAttrDB->save(at));
}


// Print database statistics (debugging).
//
void
cCD::PrintAttrStats()
{
    if (!cdAttrDB)
        printf("cdAttrDB null\n");
    else
        cdAttrDB->print_stats();
}


// Destroy the attributes database.
//
void
cCD::ClearAttrDB()
{
    delete cdAttrDB;
    cdAttrDB = 0;
}

