
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#include "fio.h"
#include "fio_chd.h"
#include "fio_oasis.h"
#include "fio_cgd.h"
#include "fio_cgd_lmux.h"
#include "cd_digest.h"
#include "si_parsenode.h"
#include "si_daemon.h"
#include "filestat.h"
#include "pathlist.h"
#include "services.h"
#include <sys/stat.h>
#include <algorithm>

#ifdef WIN32
#include <conio.h>
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif


//-----------------------------------------------------------------------------
// The database for per-cell/per-layer OASIS geometry streams.

namespace {
    // Parse the CGD remote specification string.  This has the form
    // "hostname[:port]/idname".
    //
    bool parse_remote_spec(const char *str, char **hostp, int *portp,
        char **idnamep)
    {
        *hostp = 0;
        *portp = -1;
        *idnamep = 0;
        while (isspace(*str))
            str++;
        if (!*str)
            return (false);

        const char *slsh = strrchr(str, '/');
        if (!slsh)
            return (false);
        if (slsh == str)
            return (false);
        if (!*(slsh+1) || isspace(*(slsh+1)))
            return (false);
        const char *col = strchr(str, ':');
        if (col) {
            if (col > slsh)
                return (false);
            if (col == str)
                return (false);
            for (const char *c = col+1; c != slsh; c++) {
                if (!isdigit(*c))
                    return (false);
            }
            if (sscanf(col+1, "%d", portp) != 1)
                return (false);
            char *t = new char[col - str + 1];
            strncpy(t, str, col-str);
            t[col-str] = 0;
            *hostp = t;
        }
        else {
            char *t = new char[slsh - str + 1];
            strncpy(t, str, slsh-str);
            t[slsh-str] = 0;
            *hostp = t;
        }

        *idnamep = lstring::copy(slsh + 1);
        char *t = *idnamep + strlen(*idnamep) - 1;
        while (isspace(*t) && t >= *idnamep)
           *t-- = 0;
        return (true);
    }
}


// Create a new CGD, which will be added to the CD table under
// access name idname.  If idname is null or empty, an idname will
// be assigned.  The string is a source file name, or a remote
// access spec in the form "hostname[:port]/idname".  The mode
// argument enforces the type of CGD to create.
//
// The chd argument applies only to mode CGDmemory.  If given, its
// filename will override the string, and its cellname aliasing will
// be propagated.
//
cCGD *
cFIO::NewCGD(const char *idname, const char *string, CgdType mode, cCHD *chd)
{
    if ((!string || !*string) && !chd) {
        Errs()->add_error("NewCGD: null or empty specification string.");
        return (0);
    }
    if (idname && *idname) {
        cCGD *cgd = CDcgd()->cgdRecall(idname, false);
        if (cgd) {
            if (cgd->refcnt()) {
                Errs()->add_error("NewCGD: access name %s in use.", idname);
                return (0);
            }
            delete cgd;
        }
        idname = lstring::copy(idname);
    }
    else
        idname = CDcgd()->newCgdName();
    GCarray<const char*> gc_idname(idname);

    if (mode == CGDmemory) {

        // The source string can be:
        // 0.  Anything, if a chd is passed.  It will be ignored.
        // 1.  A layout (archive) file.  The file will be read and
        //     geometry extracted.
        // 2.  A CHD name.  The CHD will be used to read the geometry
        //     from the file it references.
        // 3.  A CHD file name.  The file will be read, and a new CHD
        //     created in memory.  This CHD will be used to read the
        //     geometry from the file referenced.
        // 4.  A CGD file name.  The file will be read into a memory
        //     CGD.

        bool free_chd = false;
        char *infile = 0;
        FileType src_ft = Fnone;
        if (chd) {
            // A CHD was passed.
            infile = lstring::copy(chd->filename());
            src_ft = chd->filetype();
        }
        else {
            chd = CDchd()->chdRecall(string, false);
            if (chd) {
                // Source name was a CHD in memory, which is allowed.
                infile = lstring::copy(string);
                src_ft = chd->filetype();
            }
        }
        if (!chd) {
            infile = pathlist::expand_path(string, false, true);
            if (filestat::get_file_type(infile) != GFT_FILE) {
                Errs()->add_error("NewCGD: can't open source file.");
                delete [] infile;
                return (0);
            }

            char *realname;
            FILE *fp = FIO()->POpen(infile, "rb", &realname);
            if (!fp) {
                Errs()->add_error("NewCGD: can't open source file.");
                delete [] infile;
                return (0);
            }
            src_ft = FIO()->GetFileType(fp);
            fclose(fp);
            if (FIO()->IsSupportedArchiveFormat(src_ft)) {
                // Use full ptah to file.
                delete [] infile;
                infile = realname;
            }
            else {
                // Source file not a layout file.

                // Is it a CHD file? If so, create a CHD from it, and
                // use the CHD.
                sCHDin chd_in;
                if (chd_in.check(infile)) {
                    // Input was a CHD file.
                    chd = chd_in.read(infile, CHD_CGDmemory);
                    if (!chd) {
                        Errs()->add_error(
                            "NewCGD: CHD read error occurred.");
                        delete [] infile;
                        return (0);
                    }

                    // If the CHD file had geometry records, there will
                    // be a linked CGD, unlink and return it.
                    if (chd->hasCgd()) {
                        cCGD *cgd = chd->getCgd();
                        cgd->set_free_on_unlink(false);
                        chd->setCgd(0, 0);
                        delete chd;
                        // The CGD is still in storage, under a
                        // different name.
                        CDcgd()->cgdRecall(cgd->id_name(), true);
                        CDcgd()->cgdStore(idname, cgd);
                        delete [] infile;
                        return (cgd);
                    }

                    // Save the CHD for reading, will delete when done.
                    char *dbname = CDchd()->newChdName();
                    CDchd()->chdStore(dbname, chd);
                    delete [] infile;
                    infile = dbname;
                    src_ft = chd->filetype();
                    free_chd = true;
                }
                else {
                    // Is it a CGD file?  If so, read it and return.
                    cCGD *cgd = new cCGD(infile);
                    if (!cgd->read(infile, CGDmemory)) {
                        Errs()->add_error(
                            "NewCGD: not a supported file format.");
                        delete [] infile;
                        return (0);
                    }
                    CDcgd()->cgdStore(idname, cgd);
                    delete [] infile;
                    return (cgd);
                }
            }
        }

        FIOcvtPrms prms;
        if (chd) {
            // Pass the chd's aliasing to the reader.  This should
            // ensure that the cell names used in the chd and cgd
            // are the same.
            prms.set_alias_info(chd->aliasInfo());
        }
        else
            prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
        prms.set_allow_layer_mapping(true);
        prms.set_destination(idname, Fnone, true);

        bool ret = false;
        if (src_ft == Fgds)
            ret = FIO()->ConvertFromGds(infile, &prms);
        else if (src_ft == Foas)
            ret = FIO()->ConvertFromOas(infile, &prms);
        else if (src_ft == Fcgx)
            ret = FIO()->ConvertFromCgx(infile, &prms);
        else if (src_ft == Fcif)
            ret = FIO()->ConvertFromCif(infile, &prms);

        if (free_chd) {
            CDchd()->chdRecall(infile, true);
            delete chd;
        }
        delete [] infile;

        // Above creates a new CGD, listed in the CD table, and at
        // this point should have zero refcnt.

        cCGD *cgd = CDcgd()->cgdRecall(prms.destination(), false);
        if (ret) {
            if (!cgd) {
                // Successful return above but CGD not in table, can't
                // happen.
                Errs()->add_error("NewCGD: CGD name %s is unrsolved.",
                    prms.destination());
                return (0);
            }
        }
        else {
            // Failed for some reason, clean up.
            if (cgd) {
                // The refcnt should be zero, CGDs can be deleted
                // only when so.
                if (cgd->refcnt()) {
                    Errs()->add_error(
                "NewCGD: CGD %s creation failed, reference count nonzero.",
                        prms.destination());
                    cgd = 0;
                }
                else
                    delete cgd;
            }
        }
        return (cgd);
    }
    if (mode == CGDfile) {

        // The source string must be the name of a CGD file, or a CHD
        // file with geometry records.  The CGD will obtain geometry
        // via offsets into this file.

        char *infile = pathlist::expand_path(string, false, true);
        if (filestat::get_file_type(infile) != GFT_FILE) {
            Errs()->add_error("NewCGD: can't open source file.");
            delete [] infile;
            return (0);
        }

        FILE *fp = FIO()->POpen(infile, "rb");
        if (!fp) {
            Errs()->add_error("NewCGD: can't open source file.");
            delete [] infile;
            return (0);
        }
        fclose(fp);

        sCHDin chd_in;
        if (chd_in.check(infile)) {
            // Input was a CHD file.
            chd = chd_in.read(infile, CHD_CGDfile);
            if (!chd) {
                Errs()->add_error("NewCGD: CHD read error occurred.");
                delete [] infile;
                return (0);
            }

            // If the CHD file had geometry records, there will
            // be a linked CGD, unlink and return it.
            if (chd->hasCgd()) {
                cCGD *cgd = chd->getCgd();
                cgd->set_free_on_unlink(false);
                chd->setCgd(0, 0);
                delete chd;
                // The CGD is still in storage, under a
                // different name.
                CDcgd()->cgdRecall(cgd->id_name(), true);
                CDcgd()->cgdStore(idname, cgd);
                delete [] infile;
                return (cgd);
            }

            // No geometry, and error here.
            Errs()->add_error("NewCGD: no geometry found in CHD file.");
            delete [] infile;
            return (0);
        }

        cCGD *cgd = new cCGD(infile);
        if (!cgd->read(infile, CGDfile)) {
            Errs()->add_error("NewCGD: not a supported file format.");
            delete [] infile;
            return (0);
        }
        CDcgd()->cgdStore(idname, cgd);
        delete [] infile;
        return (cgd);
    }
    if (mode == CGDremote) {

        // The source string must be in the form
        //  hostname[:port]/idname

        char *hostname;
        int port;
        char *remid;
        if (!parse_remote_spec(string, &hostname, &port, &remid)) {
            Errs()->add_error("NewCGD: bad syntax in specification string.");
            return (0);
        }
        if (port < 0)
            port = XIC_PORT;

        cCGD *cgd = new cCGD(hostname, port);
        if (cgd->connection_socket() < 0) {
            delete [] hostname;
            delete [] remid;
            delete cgd;
            Errs()->add_error("NewCGD: connection to server failed.");
            return (0);
        }
        if (!cgd->set_remote_cgd_name(remid)) {
            delete [] hostname;
            delete [] remid;
            delete cgd;
            Errs()->add_error("NewCGD: unknown or invalid remote id name.");
            return (0);
        }
        CDcgd()->cgdStore(idname, cgd);
        return (cgd);
    }
    return (0);
}
// End of cFIO functions.


#ifdef WIN32
#define CLOSESOCKET(x) (shutdown(x, SD_SEND), closesocket(x))
#else
#define CLOSESOCKET(x) ::close(x)
#endif


sCHDout *cCGD::cg_chd_out_s;

// Constructor for local CGD, geometry obtained from local memory or
// file.
//
cCGD::cCGD(const char *nm)
{
    cg_sourcename = lstring::copy(nm);  // source file name
    cg_table = 0;
    cg_chd_out = cg_chd_out_s;
    cg_cur_stream = 0;
    cg_fp = 0;
    cg_unlisted = 0;

    cg_hostname = 0;
    cg_cgdname = 0;
    cg_port = 0;
    cg_skt = 0;
    cg_remote = false;

    cg_free_on_unlink = false;
    cg_refcnt = 0;
    cg_dbname = 0;
}


// Constructor for remote access CGD, geometry obtained from remote
// host throuth the Xic server.  The connection_socket() method should
// be used to test the validity of the new socket.  If it is bad,
// there will be an error message in the Errs system.
//
// This uses the "geom" command of an Xic server running on a remote
// system to provide geometry.  The connection is established in the
// constructor (if all goes well) and persists until the destructor is
// called.
//
cCGD::cCGD(const char *hostname, int port)
{
    if (!hostname)
        hostname = "localhost";
    if (port < 0)
        port = XIC_PORT;

    cg_sourcename = new char[strlen(hostname) + 10];
    sprintf(cg_sourcename, "%s:%d", hostname, port);
    cg_table = 0;
    cg_chd_out = 0;
    cg_cur_stream = 0;
    cg_fp = 0;
    cg_unlisted = 0;

    cg_hostname = lstring::copy(hostname);
    cg_cgdname = 0;
    cg_port = port;
    cg_skt = -1;
    cg_remote = true;

    cg_free_on_unlink = false;
    cg_refcnt = 0;
    cg_dbname = 0;

#ifdef WIN32
    // initialize winsock
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        Errs()->add_error(
            "Windows Socket Architecture initialization failed.\n");
        return;
    }
#endif

    hostent *hent = gethostbyname(hostname);
    if (!hent) {
        Errs()->sys_herror("gethostbyname");
        return;
    }

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        Errs()->sys_error("socket");
        return;
    }
    sockaddr_in skt;
    memset(&skt, 0, sizeof(sockaddr_in));
    memcpy(&skt.sin_addr, hent->h_addr, hent->h_length);
    skt.sin_family = AF_INET;
    skt.sin_port = htons(port);
    if (connect(sd, (sockaddr*)&skt, sizeof(skt)) < 0) {
        CLOSESOCKET(sd);
        Errs()->sys_error("connect");
        return;
    }

    // drain initial reply
    char buf[256];
    if (recv(sd, buf, 256, 0) < 0) {
        CLOSESOCKET(sd);
        Errs()->sys_error("recv");
        return;
    }
    cg_skt = sd;
}


cCGD::~cCGD()
{
    if (cg_dbname)
        CDcgd()->cgdRecall(cg_dbname, true);
    if (!cg_remote) {
        delete [] cg_sourcename;
        delete [] cg_cur_stream;
        if (cg_fp)
            fclose (cg_fp);
        tgen_t<cgd_cn_t> cgen(cg_table);
        cgd_cn_t *cn;
        while ((cn = cgen.next()) != 0) {
            tgen_t<cgd_lyr_t> lgen(cn->table);
            cgd_lyr_t *lyr;
            while ((lyr = lgen.next()) != 0)
                lyr->free_data();
            delete cn->table;
            cn->table = 0;
        }
        delete cg_table;
        delete cg_unlisted;
    }
    else {
        if (cg_skt > 0)
            CLOSESOCKET(cg_skt);
        delete [] cg_hostname;
        delete [] cg_cgdname;
    }
//    if (refcnt())
//        printf("FOO! refcount nonzero!\n");
}


// The following functions implement the interface for the CHD-CGD
// linking.  A linked CHD will provide access to the geometry obtained
// from the CGD, rather than through reading the original layout file.

// Return a list of the cells found in the database.
//
stringlist *
cCGD::cells_list()
{
    stringlist *s0 = 0;
    if (!cg_remote) {
        tgen_t<cgd_cn_t> gen(cg_table);
        cgd_cn_t *ct;
        while ((ct = gen.next()) != 0)
            s0 = new stringlist(lstring::copy(ct->name), s0);
        stringlist::sort(s0);
    }
    else {
        if (cg_skt < 0) {
            Errs()->add_error("cells_list: socket not open.");
            return (0);
        }

        char buf[256];
        sprintf(buf, "geom %s\r\n", cg_cgdname);
        if (send(cg_skt, buf, strlen(buf), 0) < 0) {
            Errs()->add_error("cells_list: socket write error.");
            return (0);
        }

        int id, size;
        unsigned char *msg;
        if (!daemon_client::read_msg(cg_skt, &id, &size, &msg))
            return (0);

        if (id == 4 && msg) {
            stringlist *se = 0;
            char *s = (char*)msg;
            char *tok;
            while ((tok = lstring::gettok(&s)) != 0) {
                if (!s0)
                    s0 = se = new stringlist(tok, 0);
                else {
                    se->next = new stringlist(tok, 0);
                    se = se->next;
                }
            }
        }
        delete [] msg;
    }
    return (s0);
}


// Return true if the cell is found in the table.
//
bool
cCGD::cell_test(const char *cname)
{
    if (!cname || !*cname)
        return (false);
    if (!cg_remote) {
        if (!cg_table)
            return (false);
        return (cg_table->find(cname) != 0);
    }
    if (cg_skt < 0)
        return (false);

    char buf[256];
    sprintf(buf, "geom %s ? %s\r\n", cg_cgdname, cname);
    if (send(cg_skt, buf, strlen(buf), 0) < 0)
        return (false);

    int id, size;
    unsigned char *msg;
    if (!daemon_client::read_msg(cg_skt, &id, &size, &msg))
        return (false);

    if (id == 4 && msg) {
        if (!strcmp((char*)msg, "y")) {
            delete [] msg;
            return (true);
        }
    }
    delete [] msg;
    return (false);
}


namespace {
    inline bool lyrsort(cgd_lyr_t *l1, cgd_lyr_t *l2)
    {
        return (l2->get_offset() > l1->get_offset());
    }
}


// Return a list of the layers used in the named cell.  If the layer
// records contain offsets, sort in ascending offset order.  This may
// speed up file access.
//
stringlist *
cCGD::layer_list(const char *cname)
{
    if (!cname || !*cname)
        return (0);
    stringlist *s0 = 0;
    if (!cg_remote) {
        if (!cg_table)
            return (0);
        cgd_cn_t *cgdcn = cg_table->find(cname);
        if (!cgdcn)
            return (0);
        tgen_t<cgd_lyr_t> gen(cgdcn->table);
        cgd_lyr_t *lyr;

        cgd_lyr_t **ary = 0;
        int sz = 0;
        int cnt = 0;

        while ((lyr = gen.next()) != 0) {
            if (lyr->has_local_data())
                s0 = new stringlist(lstring::copy(lyr->lname), s0);
            else {
                if (!ary) {
                    ary = new cgd_lyr_t*[32];
                    sz = 32;
                }
                else if (cnt == sz) {
                    cgd_lyr_t **tary = new cgd_lyr_t*[2*sz];
                    memcpy(tary, ary, sz*sizeof(cgd_lyr_t*));
                    delete [] ary;
                    ary = tary;
                    sz *= 2;
                }
                ary[cnt++] = lyr;
            }
        }
        if (ary) {
            std::sort(ary, ary + cnt, lyrsort);
            for (int i = cnt-1; i >= 0; i--)
                s0 = new stringlist(lstring::copy(ary[i]->tab_name()), s0);
            delete [] ary;
        }
    }
    else {
        if (cg_skt < 0) {
            Errs()->add_error("layer_list: socket not open.");
            return (0);
        }
        if (!cname || !*cname) {
            Errs()->add_error("layer_list: null or empty cell name.");
            return (0);
        }

        char buf[256];
        sprintf(buf, "geom %s %s\r\n", cg_cgdname, cname);
        if (send(cg_skt, buf, strlen(buf), 0) < 0) {
            Errs()->add_error("layer_list: socket write error.");
            return (0);
        }

        int id, size;
        unsigned char *msg;
        if (!daemon_client::read_msg(cg_skt, &id, &size, &msg))
            return (0);

        if (id == 4 && msg) {
            stringlist *se = 0;
            char *s = (char*)msg;
            char *tok;
            while ((tok = lstring::gettok(&s)) != 0) {
                if (!s0)
                    s0 = se = new stringlist(tok, 0);
                else {
                    se->next = new stringlist(tok, 0);
                    se = se->next;
                }
            }
        }
        delete [] msg;
    }
    return (s0);
}


// Return true if the layer is used in the cell.
//
bool
cCGD::layer_test(const char *cname, const char *lname)
{
    if (!cname || !*cname)
        return (false);
    if (!lname || !*lname)
        return (false);
    if (!cg_remote) {
        if (!cg_table)
            return (false);
        cgd_cn_t *cgdcn = cg_table->find(cname);
        if (!cgdcn)
            return (false);
        return (cgdcn->table->find(lname) != 0);
    }
    if (cg_skt < 0)
        return (false);

    char buf[256];
    sprintf(buf, "geom %s %s ? %s\r\n", cg_cgdname, cname, lname);
    if (send(cg_skt, buf, strlen(buf), 0) < 0)
        return (false);

    int id, size;
    unsigned char *msg;
    if (!daemon_client::read_msg(cg_skt, &id, &size, &msg))
        return (false);

    if (id == 4 && msg) {
        if (!strcmp((char*)msg, "y")) {
            delete [] msg;
            return (true);
        }
    }
    delete [] msg;
    return (false);
}


// Remove and destroy the given cell data from the database.  Return
// true if cell was found and removed, false otherwise.
//
bool
cCGD::remove_cell(const char *cname)
{
    if (!cname || !*cname)
        return (false);
    if (!cg_remote) {
        if (!cg_table)
            return (false);
        cgd_cn_t *cn = cg_table->remove(cname);
        if (!cn)
            return (false);
        tgen_t<cgd_lyr_t> lgen(cn->table);
        cgd_lyr_t *lyr;
        while ((lyr = lgen.next()) != 0)
            lyr->free_data();
        delete cn->table;
        cn->table = 0;

        // Put the dead cn into the unlisted table, so we can know that it
        // was originally included.
        if (!cg_unlisted)
            cg_unlisted = new table_t<cgd_cn_t>;
        cg_unlisted->link(cn);
        cg_unlisted = cg_unlisted->check_rehash();
        return (true);
    }
    if (cg_skt < 0)
        return (false);

    char buf[256];
    sprintf(buf, "geom %s - %s\r\n", cg_cgdname, cname);
    if (send(cg_skt, buf, strlen(buf), 0) < 0)
        return (false);

    int id, size;
    unsigned char *msg;
    if (!daemon_client::read_msg(cg_skt, &id, &size, &msg))
        return (false);

    if (id == 4 && msg) {
        if (!strcmp((char*)msg, "y")) {
            delete [] msg;
            return (true);
        }
    }
    delete [] msg;
    return (false);
}


// Return a list of the cells that have been removed from the
// database.
//
stringlist *
cCGD::unlisted_list()
{
    stringlist *s0 = 0;
    if (!cg_remote) {
        tgen_t<cgd_cn_t> gen(cg_unlisted);
        cgd_cn_t *ct;
        while ((ct = gen.next()) != 0)
            s0 = new stringlist(lstring::copy(ct->name), s0);
        stringlist::sort(s0);
    }
    else {
        if (cg_skt < 0)
            return (0);

        char buf[256];
        sprintf(buf, "geom %s -?\r\n", cg_cgdname);
        if (send(cg_skt, buf, strlen(buf), 0) < 0)
            return (0);

        int id, size;
        unsigned char *msg;
        if (!daemon_client::read_msg(cg_skt, &id, &size, &msg))
            return (0);

        if (id == 4 && msg) {
            stringlist *se = 0;
            char *s = (char*)msg;
            char *tok;
            while ((tok = lstring::gettok(&s)) != 0) {
                if (!s0)
                    s0 = se = new stringlist(tok, 0);
                else {
                    se->next = new stringlist(tok, 0);
                    se = se->next;
                }
            }
        }
        delete [] msg;
    }
    return (s0);
}


// Return true if cname was removed from the database.
//
bool
cCGD::unlisted_test(const char *cname)
{
    if (!cname || !*cname)
        return (false);
    if (!cg_remote) {
        if (!cg_unlisted)
            return (false);
        return (cg_unlisted->find(cname) != 0);
    }
    if (cg_skt < 0)
        return (false);

    char buf[256];
    sprintf(buf, "geom %s -? %s\r\n", cg_cgdname, cname);
    if (send(cg_skt, buf, strlen(buf), 0) < 0)
        return (false);

    int id, size;
    unsigned char *msg;
    if (!daemon_client::read_msg(cg_skt, &id, &size, &msg))
        return (false);

    if (id == 4 && msg) {
        if (!strcmp((char*)msg, "y")) {
            delete [] msg;
            return (true);
        }
    }
    delete [] msg;
    return (false);
}


// If the cellname/layername match a stream, return a reader object in
// pbs.  If there is no data found, set pbs to 0.  In both cases
// return true, return false if there is an error obtaining data.
//
bool
cCGD::get_byte_stream(const char *cname, const char *lname,
    oas_byte_stream **pbs)
{
    if (!pbs) {
        Errs()->add_error("get_byte_stream: null return address.");
        return (false);
    }
    *pbs = 0;

    if (!cname || !*cname) {
        Errs()->add_error("get_byte_stream: null or empty cell name.");
        return (false);
    }
    if (!lname || !*lname) {
        Errs()->add_error("get_byte_stream: null or empty layer name.");
        return (false);
    }
    if (!cg_remote) {
        if (!cg_table)
            return (true);
        cgd_cn_t *cgdcn = cg_table->find(cname);
        if (!cgdcn)
            return (true);
        cgd_lyr_t *lyr = cgdcn->table->find(lname);
        if (lyr) {
            if (lyr->has_local_data()) {
                if (lyr->get_data() && (lyr->get_csize() || lyr->get_usize()))
                    *pbs = new bstream_t(lyr->get_data(), lyr->get_csize(),
                        lyr->get_usize());
                return (true);
            }

            size_t size = lyr->get_csize();
            if (!size)
                size = lyr->get_usize();
            if (!size)
                return (true);
            if (!get_cur_stream(lyr->get_offset(), size))
                return (false);
            *pbs = new bstream_t(cg_cur_stream, lyr->get_csize(),
                lyr->get_usize());
        }
    }
    else {
        if (cg_skt < 0) {
            Errs()->add_error("get_byte_stream: bad socket index.");
            return (false);
        }

        char buf[256];
        sprintf(buf, "geom %s %s %s\r\n", cg_cgdname, cname, lname);
        if (send(cg_skt, buf, strlen(buf), 0) < 0) {
            Errs()->sys_error("get_byte_stream/send");
            return (false);
        }

        int id, size;
        unsigned char *msg;
        if (!daemon_client::read_msg(cg_skt, &id, &size, &msg)) {
            Errs()->add_error("get_byte_stream: daemon client error.");
            return (false);
        }

        if (id == 9 && msg) {
            char *p = (char*)msg;
            unsigned int csz = *(unsigned int*)p;
            csz = ntohl(csz);
            p += 4;
            unsigned int usz = *(unsigned int*)p;
            usz = ntohl(usz);
            p += 4;

            *pbs = new bstream_t(msg, csz, usz, true, 8);
        }
        else
            delete [] msg;
    }
    return (true);
}

// End of CHD interface functions.


namespace {
    inline char *format(char *buf, const char *n)
    {
        sprintf(buf, "%-20s: ", n);
        return (buf);
    }
}


// Return a string containing a formatted listing of CGD parameters. 
// If quick is true, return an encoding in the form
//   <type> [Y][*]
// where <type is 'M', 'F', or'R', the 'Y' appears if linked, and the
// '*' appears if destroy-on-unlink is set.  Caller should free
// returned string.
//
char *
cCGD::info(bool quick)
{
    char buf[256];
    if (quick) {
        if (cg_remote)
            strcpy(buf, "Rem ");
        else if (cg_fp)
            strcpy(buf, "File");
        else
            strcpy(buf, "Mem ");
        if (cg_refcnt > 0) {
            buf[4] = ' ';
            buf[5] = 'y';
            buf[6] = 'e';
            buf[7] = 's';
            buf[8] = cg_free_on_unlink ? '*' : 0;
            buf[9] = 0;
        }
        return (lstring::copy(buf));
    }

    sLstr lstr;

    if (cg_dbname) {
        lstr.add(format(buf, "Access Name"));
        lstr.add(cg_dbname);
        lstr.add_c('\n');
    }

    lstr.add(format(buf, "Type"));
    if (cg_remote)
        lstr.add("REMOTE");
    else if (cg_fp)
        lstr.add("FILE");
    else
        lstr.add("MEMORY");
    lstr.add_c('\n');
    
    if (cg_sourcename) {
        lstr.add(format(buf, "Source"));
        lstr.add(cg_sourcename);
        lstr.add_c('\n');
    }

    lstr.add(format(buf, "References"));
    lstr.add_i(cg_refcnt);
    lstr.add_c('\n');

    lstr.add(format(buf, "Destroy on Unlink"));
    lstr.add(cg_free_on_unlink ? "TRUE" : "FALSE");
    lstr.add_c('\n');

    if (cg_remote) {
        lstr.add(format(buf, "Host"));
        lstr.add(cg_hostname ? cg_hostname : "");
        lstr.add_c('\n');
        lstr.add(format(buf, "Port"));
        lstr.add_i(cg_port);
        lstr.add_c('\n');
        lstr.add(format(buf, "Id Name"));
        lstr.add(cg_cgdname ? cg_cgdname : "");
        lstr.add_c('\n');
        lstr.add(format(buf, "Socket"));
        lstr.add_i(cg_skt);
        lstr.add_c('\n');
    }
    else {
        lstr.add(format(buf, "Cells"));
        lstr.add_i(cg_table ? cg_table->allocated() : 0);
        lstr.add_c('\n');
        lstr.add(format(buf, "Removed Cells"));
        lstr.add_i(cg_unlisted ? cg_unlisted->allocated() : 0);
        lstr.add_c('\n');

        uint64_t mem = 0;
        uint64_t tot = 0;
        tgen_t<cgd_cn_t> gen(cg_table);
        cgd_cn_t *ct;
        while ((ct = gen.next()) != 0) {
            tgen_t<cgd_lyr_t> lgen(ct->table);
            cgd_lyr_t *lyr;
            while ((lyr = lgen.next()) != 0) {
                uint64_t sz = lyr->get_csize();
                if (!sz)
                    sz = lyr->get_usize();
                if (lyr->has_local_data())
                    mem += sz;
                tot += sz;
            }
        }
        lstr.add(format(buf, "Memory Use"));
        sprintf(buf, "%.3fKb\n", mem*1e-3);
        lstr.add(buf);
        lstr.add(format(buf, "Total Size"));
        sprintf(buf, "%.3fKb\n", tot*1e-3);
        lstr.add(buf);
    }
    return (lstr.string_trim());
}


// Retrieve a data block for a cell and layer.
//
bool
cCGD::find_block(const char *cname, const char *lname,
    size_t *cszp, size_t *uszp, const unsigned char **pdata)
{
    if (!pdata) {
        Errs()->add_error("find_block: null return address.");
        return (false);
    }
    *pdata = 0;

    if (!cszp) {
        Errs()->add_error("find_block: null compressed size pointer.");
        return (false);
    }
    *cszp = 0;

    if (!uszp) {
        Errs()->add_error("find_block: null uncompressed size pointer.");
        return (false);
    }
    *uszp = 0;

    if (!cname || !*cname) {
        Errs()->add_error("find_block: null or empty cell name.");
        return (false);
    }
    if (!lname || !*lname) {
        Errs()->add_error("find_block: null or empty layer name.");
        return (false);
    }
    if (!cg_remote) {
        if (!cg_table)
            return (true);
        cgd_cn_t *cgdcn = cg_table->find(cname);
        if (!cgdcn)
            return (true);
        cgd_lyr_t *cgdlyr = cgdcn->table->find(lname);
        if (!cgdlyr)
            return (true);

        *cszp = cgdlyr->get_csize();
        *uszp = cgdlyr->get_usize();
        if (cgdlyr->has_local_data()) {
            *pdata = cgdlyr->get_data();
            return (true);
        }
        size_t size = cgdlyr->get_csize();
        if (!size) 
            size = cgdlyr->get_usize(); 
        if (!size) 
            return (true);
        if (!get_cur_stream(cgdlyr->get_offset(), size))
            return (false);
        *pdata = cg_cur_stream;
    }
    else {
        if (cg_skt < 0) {
            Errs()->add_error("find_block: bad socket index.");
            return (false);
        }

        char buf[256];
        sprintf(buf, "geom %s %s %s\r\n", cg_cgdname, cname, lname);
        if (send(cg_skt, buf, strlen(buf), 0) < 0) {
            Errs()->sys_error("find_block/send");
            return (false);
        }

        int id, size;
        unsigned char *msg;
        if (!daemon_client::read_msg(cg_skt, &id, &size, &msg)) {
            Errs()->add_error("find_block: daemon client error.");
            return (false);
        }

        if (id == 9 && msg) {
            unsigned char *p = (unsigned char*)msg;
            unsigned int csz = *(unsigned int*)p;
            *cszp = ntohl(csz);
            p += 4;
            unsigned int usz = *(unsigned int*)p;
            *uszp = ntohl(usz);
            p += 4;
            *pdata = p;
            delete [] cg_cur_stream;
            cg_cur_stream = msg;
        }
        else
            delete [] msg;
    }
    return (true);
}


// Check and save the remote CGD name, remote mode only.
//
bool
cCGD::set_remote_cgd_name(const char *cgdname)
{
    if (!cg_remote)
        return (false);
    if (cg_skt < 0) {
        Errs()->add_error("set_remote_cgd_name: socket not open.");
        return (false);
    }

    char buf[256];
    sprintf(buf, "geom ? %s\r\n", cgdname);
    if (send(cg_skt, buf, strlen(buf), 0) < 0) {
        Errs()->add_error("set_remote_cgd_name: socket write error.");
        return (false);
    }

    int id, size;
    unsigned char *msg;
    if (!daemon_client::read_msg(cg_skt, &id, &size, &msg))
        return (false);

    if (id == 4 && msg && *msg == 'y') {
        delete [] cg_cgdname;
        cg_cgdname = lstring::copy(cgdname);
        if (cg_sourcename) {
            // Cat the id name, form will be "hostname:port/idname".
            char *tmp = new char[strlen(cg_sourcename) +
                strlen(cg_cgdname) + 2];
            char *e = lstring::stpcpy(tmp, cg_sourcename);
            *e++ = '/';
            strcpy(e, cg_cgdname);
            delete [] cg_sourcename;
            cg_sourcename = tmp;
        }
        return (true);
    }
    return (false);
}


// Return a list of CGD names found on the remote system, remote mode
// only.
//
stringlist *
cCGD::remote_cgd_name_list()
{
    if (!cg_remote)
        return (0);
    if (cg_skt < 0) {
        Errs()->add_error("remote_cgd_name_list: socket not open.");
        return (0);
    }

    char buf[256];
    strcpy(buf, "geom\r\n");
    if (send(cg_skt, buf, strlen(buf), 0) < 0) {
        Errs()->add_error("remote_cgd_name_list: socket write error.");
        return (0);
    }

    int id, size;
    unsigned char *msg;
    if (!daemon_client::read_msg(cg_skt, &id, &size, &msg))
        return (0);

    stringlist *s0 = 0;
    if (id == 4 && msg) {
        stringlist *se = 0;
        char *s = (char*)msg;
        char *tok;
        while ((tok = lstring::getqtok(&s)) != 0) {
            if (!s0)
                s0 = se = new stringlist(tok, 0);
            else {
                se->next = new stringlist(tok, 0);
                se = se->next;
            }
        }
    }
    delete [] msg;
    return (s0);
}
// End of remote mode functions.

// The remaining functions apply for local mode only.

// Using the CHD, read in a list of cells.
//
bool
cCGD::load_cells(cCHD *chd, stringlist *cellnames, double scale,
    bool allow_layer_mapping)
{
    if (!cellnames)
        return (true);
    if (cg_remote)
        return (false);

    if (!chd) {
        Errs()->add_error("cCGD::load_cells: null CHD pointer.");
        return (false);
    }
    if (scale < .001 || scale > 1000.0) {
        Errs()->add_error("cCGD::load_cells: bad scale.");
        return (false);
    }

    cv_in *in = chd->newInput(allow_layer_mapping);
    if (!in) {
        Errs()->add_error("cCGD::load_cells: input channel creation failed.");
        return (false);
    }

    // Set input aliasing to CHD aliasing.
    //
    in->assign_alias(new FIOaliasTab(true, false, chd->aliasInfo()));

    // Create output channel.
    //
    oas_out *out = new oas_out(this);
    in->setup_backend(out);

    bool ok = true;
    if (!in->chd_setup(chd, 0, 0, Physical, scale)) {
        Errs()->add_error("cCGD::load_cells: setup failed.");
        ok = false;
    }

    if (ok) {
        for (stringlist *sl = cellnames; sl; sl = sl->next) {
            symref_t *p = chd->findSymref(sl->string, Physical, false);
            if (!p || !p->get_defseen())
                continue;
            bool ret = in->chd_read_cell(p, false);
            if (!ret) {
                Errs()->add_error("cCGD::load_cells: cell %s read failed.",
                    Tstring(p->get_name()));
                ok = false;
                break;
            }
        }
    }

    in->chd_finalize();
    out->get_cgd();  // zero the pointer
    delete in;
    return (ok);
}


// Add a geometry data block for a cell and layer to the database.
//
bool
cCGD::add_data(const char *cname, const char *lname,
    const unsigned char *data, size_t csz, size_t usz)
{
    if (cg_remote)
        return (false);
    if (!cname) {
        Errs()->add_error("add_data: null cell name.");
        delete [] data;
        return (false);
    }
    if (!lname) {
        // If no layername and no data, record the cell name anyway. 
        // This will be used to avoid a fruitless access of the source
        // when reading geometry.

        if (data) {
            Errs()->add_error("add_data: null layer name.");
            delete [] data;
            return (false);
        }
        return (add_cn(cname) != 0);
    }
    if (!data || !usz) {
        delete [] data;
        return (true);
    }

    if (cg_chd_out) {
        // Here, we're writing geometry records at the end of a CHD
        // file.  A CGD is being produced in the process, but it is
        // UNUSABLE (no data saved) but has the tables.  It should
        // be freed when done.

        cgd_cn_t *cgdcn = add_cn(cname);
        if (!cgdcn) {
            delete [] data;
            return (false);
        }
        cg_chd_out->write_cell_record(cgdcn);

        cgd_lyr_t *cgdlyr = add_lyr(cgdcn, lname);
        if (!cgdlyr) {
            delete [] data;
            return (false);
        }
        cgdlyr->set_csize(csz);
        cgdlyr->set_usize(usz);
        cgdlyr->set_data(data);
        // This will free the data and save the file offset.
        if (!cg_chd_out->write_layer_record(cgdlyr))
            return (false);
    }
    else {
        // Add an in-memory record.
        cgd_cn_t *cgdcn = add_cn(cname);
        if (!cgdcn) {
            delete [] data;
            return (false);
        }

        cgd_lyr_t *cgdlyr = add_lyr(cgdcn, lname);
        if (!cgdlyr) {
            delete [] data;
            return (false);
        }
        cgdlyr->set_data(data);
        cgdlyr->set_csize(csz);
        cgdlyr->set_usize(usz);
    }
    return (true);
}


// Remove and destroy the named layer data associated with the given
// cell.  Return true if found and removed, false otherwise.
//
bool
cCGD::remove_cell_layer(const char *cname, const char *lname)
{
    if (cg_remote)
        return (false);
    if (!cname || !*cname)
        return (false);
    if (!lname || !*lname)
        return (false);
    if (!cg_table)
        return (false);
    cgd_cn_t *cn = cg_table->find(cname);
    if (!cn || !cn->table)
        return (false);
    cgd_lyr_t *lyr = cn->table->remove(lname);
    if (!lyr)
        return (false);
    lyr->free_data();
    return (true);
}


// Return a list of the layers used in the named cell, along with the
// data block size parameters.
//
stringlist *
cCGD::layer_info_list(const char *cname)
{
    if (cg_remote)
        return (0);
    if (!cname)
        return (0);
    if (!cg_table)
        return (0);
    cgd_cn_t *cgdcn = cg_table->find(cname);
    if (!cgdcn)
        return (0);
    tgen_t<cgd_lyr_t> gen(cgdcn->table);
    cgd_lyr_t *lyr;
    stringlist *s0 = 0;
    char buf[256];
    while ((lyr = gen.next()) != 0) {
        sprintf(buf, "%-4s c=%-5u u=%-5u", lyr->lname,
            (unsigned int)lyr->get_csize(), (unsigned int)lyr->get_usize());
        s0 = new stringlist(lstring::copy(buf), s0);
    }
    return (s0);
}


bool
cCGD::get_cur_stream(uint64_t offset, size_t size)
{
    if (!cg_fp) {
        Errs()->add_error("get_cur_stream: null file pointer.");
        return (false);
    }
    if (large_fseek(cg_fp, offset, SEEK_SET) < 0) {
        Errs()->sys_error("get_cur_stream/large_fseek");
        return (false);
    }
    delete [] cg_cur_stream;
    cg_cur_stream = new unsigned char[size];
    if (fread(cg_cur_stream, 1, size, cg_fp) != size) {
        Errs()->sys_error("get_cur_stream/fread");
        return (false);
    }
    return (true);
}


// Debugging aid, print records in text mode.
//
void
cCGD::dump()
{
    if (cg_remote)
        return;
    printf("DUMP\n");
    tgen_t<cgd_cn_t> gen(cg_table);
    cgd_cn_t *ct;
    while ((ct = gen.next()) != 0) {
        printf("%s\n", ct->name);
        tgen_t<cgd_lyr_t> lgen(ct->table);
        cgd_lyr_t *lt;
        while ((lt = lgen.next()) != 0) {
            printf("  %s\n", lt->lname);

            const unsigned char *data;
            if (lt->has_local_data())
                data = lt->get_data();
            else {
                size_t size = lt->get_csize();
                if (!size)
                    size = lt->get_usize();
                if (!size)
                    return;
                if (!get_cur_stream(lt->get_offset(), size)) {
                    printf("fatal error: %s\n", Errs()->get_error());
                    return;
                }
                data = cg_cur_stream;
            }
            if (!data)
                continue;
            bstream_t bs(data, lt->get_csize(), lt->get_usize());

            oas_in oas(false);
            oas.set_byte_stream(&bs);
            oas.setup_ascii_out("", 0, 0, -1, -1);
            oas.parse(Physical, false, 1.0, 0);
        }
    }
}



namespace {
    // Helper class for saving database to file.
    //
    struct sCGDout
    {
        sCGDout(FILE *fp) { dbo_fp = fp; }
        ~sCGDout() { if (dbo_fp) fclose(dbo_fp); }

        bool write_char(int c)
        {
            if (putc(c, dbo_fp) == EOF) {
                Errs()->add_error("OadDbOut::write_char: write failed!");
                return (false);
            }
            return (true);
        }

        bool write_n_string(const char*);
        bool write_unsigned(unsigned int);

    private:
        FILE *dbo_fp;
    };


    // Write an ascii text string.
    //
    bool
    sCGDout::write_n_string(const char *string)
    {
        if (!string)
            return (false);
        unsigned int len = strlen(string);
        if (!write_unsigned(len))
            return (false);
        const char *s = string;
        while (len--) {
            if (*s < 0x21 || *s > 0x7e) {
                Errs()->add_error(
                "sCGDout::write_n_string: forbidden character '0x%x'.", *s);
                return (false);
            }
            else if (!write_char(*s))
                return (false);
            s++;
        }
        return (true);
    }


    // Write an unsigned integer.
    //
    bool
    sCGDout::write_unsigned(unsigned i)
    {
        unsigned char b = i & 0x7f;
        i >>= 7;        // >> 7
        if (!i)
            return (write_char(b));
        b |= 0x80;
        if (!write_char(b))
            return (false);
        b = i & 0x7f;
        i >>= 7;        // >> 14
        if (!i)
            return (write_char(b));
        b |= 0x80;
        if (!write_char(b))
            return (false);
        b = i & 0x7f;
        i >>= 7;        // >> 21
        if (!i)
            return (write_char(b));
        b |= 0x80;
        if (!write_char(b))
            return (false);
        b = i & 0x7f;
        i >>= 7;        // >> 28
        if (!i)
            return (write_char(b));
        b |= 0x80;
        if (!write_char(b))
            return (false);
        b = i & 0xf;    // 4 bits left
        return (write_char(b));
    }
    // End of sCGDout functions.
}


// Dump a representation of the database to a file.
//
bool
cCGD::write(const char *fname)
{
    if (cg_remote)
        return (false);
    if (!fname || !*fname) {
        Errs()->add_error("cCGD::write: null or empty file name.");
        return (false);
    }
    if (!filestat::create_bak(fname)) {
        Errs()->add_error(filestat::error_msg());
        Errs()->add_error("cCGD::write: error backing up file.");
        return (false);
    }
    FILE *fp = fopen(fname, "wb");
    if (!fp) {
        Errs()->sys_error("fopen");
        Errs()->add_error("cCGD::write: failed to open %s for output.",
            fname);
        return (false);
    }
    return (write(fp, true));
}


bool
cCGD::write(FILE *fp, bool write_header)
{
    if (cg_remote)
        return (false);
    if (!fp) {
        Errs()->add_error("cCGD::write: null file pointer.");
        return (false);
    }
    sCGDout dbo(fp);
    if (write_header) {
        if (cg_sourcename && *cg_sourcename) {
            if (!dbo.write_n_string(CGD_MAGIC1))
                return (false);
            if (!dbo.write_n_string(cg_sourcename))
                return (false);
        }
        else {
            if (!dbo.write_n_string(CGD_MAGIC))
                return (false);
        }
    }

    tgen_t<cgd_cn_t> gen(cg_table);
    cgd_cn_t *ct;
    while ((ct = gen.next()) != 0) {

        if (!dbo.write_char(CGD_CNAME_REC))
            return (false);
        if (!dbo.write_n_string(ct->name))
            return (false);

        tgen_t<cgd_lyr_t> lgen(ct->table);
        cgd_lyr_t *lt;
        while ((lt = lgen.next()) != 0) {

            if (!dbo.write_char(CGD_LNAME_REC))
                return (false);
            if (!dbo.write_n_string(lt->lname))
                return (false);

            if (!dbo.write_unsigned(lt->get_csize()))
                return (false);
            if (!dbo.write_unsigned(lt->get_usize()))
                return (false);
            size_t sz = lt->get_csize();
            if (!sz)
                sz = lt->get_usize();
            const unsigned char *data;
            if (lt->has_local_data())
                data = lt->get_data();
            else {
                if (!get_cur_stream(lt->get_offset(), sz))
                    return (false);
                data = cg_cur_stream;
            }
            if (fwrite(data, 1, sz, fp) != sz) {
                Errs()->add_error(
                    "cCGD::write: fwrite returned short count.");
                return (false);
            }
        }
    }
    if (!dbo.write_char(CGD_END_REC))
        return (false);
    return (true);
}


namespace {
    // Helper class for reading geom data from file.
    //
    struct sCGDin
    {
        sCGDin(FILE *f)
            {
                dbi_fp = f;
                dbi_nogo = false;
            }

        int read_byte()
            {
                int c = getc(dbi_fp);
                if (c == EOF) {
                    dbi_nogo = true;
                    Errs()->add_error("sCGDin::read_byte: read EOF.");
                    return (0);
                }
                return (c);
            }

        FILE *fp()      { return (dbi_fp); }
        bool nogo()     { return (dbi_nogo); }

        char *read_n_string();
        unsigned int read_unsigned();
        uint64_t read_unsigned64();

    private:
        FILE *dbi_fp;
        bool dbi_nogo;
    };


    char *
    sCGDin::read_n_string()
    {
        int len = read_unsigned();
        if (len == 0)
            return (0);
        char *str = new char[len+1];
        char *s = str;
        while (len--) {
            int c = read_byte();
            if (dbi_nogo) {
                delete [] str;
                return (0);
            }
            *s++ = c;
        }
        *s = 0;
        return (str);
    }


    unsigned int
    sCGDin::read_unsigned()
    {
        unsigned int b = read_byte();
        if (dbi_nogo)
            return (0);
        unsigned int i = (b & 0x7f);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x7f) << 7);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x7f) << 14);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x7f) << 21);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0xf) << 28); // 4 bits left

        if (b & 0xf0) {
            Errs()->add_error("sCGDin::read_unsigned: int32 overflow.");
            dbi_nogo = true;
            return (0);
        }
        return (i);
    }


    uint64_t
    sCGDin::read_unsigned64()
    {
        uint64_t b = read_byte();
        if (dbi_nogo)
            return (0);
        uint64_t i = (b & 0x7f);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x7f) << 7);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x7f) << 14);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x7f) << 21);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x7f) << 28);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x7f) << 35);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x7f) << 42);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x7f) << 49);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x7f) << 56);
        if (!(b & 0x80))
            return (i);
        b = read_byte();
        if (dbi_nogo)
            return (0);
        i |= ((b & 0x1) << 63); // 1 bit left

        // Only 1 LSB can be set.
        if (b & 0xfe) {
            Errs()->add_error("sCGDin::read_unsigned64: int64 overflow.");
            dbi_nogo = true;
            return (0);
        }
        return (i);
    }
}


// Read in a file representation of the database.  'This' should be
// newly-created, i.e., empty.
//
bool
cCGD::read(const char *fname, CgdType mode)
{
    if (cg_remote || mode == CGDremote) {
        Errs()->add_error("cCGD::read: no support for remote mode.");
        return (false);
    }
    if (cg_table) {
        Errs()->add_error("cCGD::read: CGD not empty.");
        return (false);
    }
    if (!fname || !*fname) {
        Errs()->add_error("cCGD::read: null or empty file name.");
        return (false);
    }
    FILE *fp = large_fopen(fname, "rb");
    if (!fp) {
        Errs()->add_error(
            "cCGD::read: failed to open %s for input.", fname);
        return (false);
    }

    bool ret = read(fp, mode, true);
    fclose(fp);
    return (ret);
}


// Read in a file representation of the database.  'This' should be
// newly-created, i.e., empty.
//
bool
cCGD::read(FILE *fp, CgdType mode, bool expect_header)
{
    if (cg_remote || mode == CGDremote) {
        Errs()->add_error("cCGD::read: no support for remote mode.");
        return (false);
    }
    if (cg_table) {
        Errs()->add_error("cCGD::read: CGD not empty.");
        return (false);
    }
    if (!fp) {
        Errs()->add_error("cCGD::read: null file pointer.");
        return (false);
    }

    sCGDin dbi(fp);
    if (expect_header) {
        char *s = dbi.read_n_string();
        if (dbi.nogo() || !s) {
            Errs()->add_error("cCGD::read: no magic string!");
            return (false);
        }
        if (!strcmp(s, CGD_MAGIC1)) {
            delete [] s;
            cg_sourcename = dbi.read_n_string();
            if (dbi.nogo()) {
                Errs()->add_error("cCGD::read: source string read error!");
                return (false);
            }
        }
        else if (!strcmp(s, CGD_MAGIC))
            delete [] s;
        else {
            delete [] s;
            Errs()->add_error("cCGD::read: bad magic, incorrect file type.");
            return (false);
        }
    }

    uint64_t cpos = large_ftell(dbi.fp());
    int ix = dbi.read_unsigned();
    if (dbi.nogo()) {
        Errs()->add_error("cCGD::read:  read error.");
        return (false);
    }
    if (ix != CGD_CNAME_REC) {
        Errs()->add_error(
            "cCGD::read:  parse failed, bad initial record type.");
        return (false);
    }
    for (;;) {
        if (ix != CGD_CNAME_REC) {
            if (ix == CGD_END_REC)
                break;
            Errs()->add_error("cCGD::read:  parse failed, bad record type.");
            return (false);
        }

        char *cname = dbi.read_n_string();
        if (!cname) {
            Errs()->add_error(
                "cCGD::read: file format error, expecting CNAME.");
            return (false);
        }
        cgd_cn_t *cgdcn = add_cn(cname);
        delete [] cname;
        if (!cgdcn)
            return (false);
        for (;;) {
            cpos = large_ftell(dbi.fp());
            ix = dbi.read_unsigned();
            if (dbi.nogo()) {
                Errs()->add_error("cCGD::read:  read error.");
                return (false);
            }
            if (ix != CGD_LNAME_REC)
                break;
            char *lname = dbi.read_n_string();
            if (!lname) {
                Errs()->add_error(
                    "cCGD::read: file format error, expecting LNAME.");
                return (false);
            }
            uint64_t csize = dbi.read_unsigned64();
            uint64_t usize = dbi.read_unsigned64();
            if (dbi.nogo()) {
                Errs()->add_error(
                    "cCGD::read: parse failed, reading layer size.");
                delete [] lname;
                return (false);
            }
            uint64_t sz = csize;
            if (!sz)
                sz = usize;
            if (!sz) {
                Errs()->add_error(
                    "cCGD::read:  parse failed, bad record size.");
                return (false);
            }
            cpos = large_ftell(dbi.fp());
            if (mode == CGDfile)
                large_fseek(dbi.fp(), sz, SEEK_CUR);

            cgd_lyr_t *cgdlyr = add_lyr(cgdcn, lname);
            delete [] lname;
            if (!cgdlyr)
                return (false);

            cgdlyr->set_csize(csize);
            cgdlyr->set_usize(usize);
            if (mode == CGDfile) {
                cgdlyr->set_offset(cpos);
                // Offset to data string, not record.
            }
            else {
                unsigned char *data = new unsigned char[sz];
                // There is a 32/64 issue here if sizeof(size_t) == 4,
                // should be OK under Linux due to largefile.h

                if (fread(data, 1, sz, dbi.fp()) != sz) {
                    Errs()->add_error(
                        "cCGD::read: fread returned short count.");
                    delete [] data;
                    return (false);
                }
                cgdlyr->set_data(data);
            }
        }
    }
    if (mode == CGDfile) {
        // Duplicate the file pointer as a private copy for use in
        // accessing the file data.
        cg_fp = fdopen(dup(fileno(fp)), "rb");
        if (!cg_fp) {
            Errs()->add_error(
                "cCGD::read: failed to reopen file pointer.");
            return (false);
        }
    }
    return (true);
}


// Private function.
cgd_cn_t *
cCGD::add_cn(const char *cname)
{
    if (!cg_table)
        cg_table = new table_t<cgd_cn_t>;
    cgd_cn_t *cgdcn = cg_table->find(cname);
    if (!cgdcn) {
        cgdcn = cg_eltab_cn.new_element();
        cgdcn->set_tab_next(0);
        cgdcn->set_tab_name(cg_string_tab.add(cname));
        cgdcn->table = new table_t<cgd_lyr_t>;
        cg_table->link(cgdcn, false);
        cg_table = cg_table->check_rehash();
    }
    return (cgdcn);
}


// Private function.
cgd_lyr_t *
cCGD::add_lyr(cgd_cn_t *cgdcn, const char *lname)
{
    cgd_lyr_t *cgdlyr = cgdcn->table->find(lname);
    if (!cgdlyr) {
        cgdlyr = cg_eltab_lyr.new_element();
        cgdlyr->set_tab_next(0);
        cgdlyr->lname = cg_string_tab.add(lname);
        cgdcn->table->link(cgdlyr, false);
        cgdcn->table = cgdcn->table->check_rehash();
        return (cgdlyr);
    }
    Errs()->add_error("add_lyr: duplicate layer record.");
    return (0);
}
// End of cCGD functions.


accum_t::~accum_t()
{
    delete ac_zfp;
    if (ac_fp)
        fclose(ac_fp);
    if (ac_tempfile) {
        unlink(ac_tempfile);
        delete [] ac_tempfile;
    }
    delete [] ac_data;
};


// Accumulate a byte.  If the buffer overflows, we go to compression
// mode.  Short strings will remain uncompressed, usually these are
// smaller than the "compressed" version.
//
bool
accum_t::put_byte(int c)
{
    if (ac_count < CGD_COMPR_BUFSIZE) {
        ac_buf[ac_count++] = c;
        return (true);
    }
    if (!ac_fp) {
        ac_tempfile = filestat::make_temp("ac");
        ac_fp = large_fopen(ac_tempfile, "wb+");
        if (!ac_fp) {
            Errs()->add_error(
                "accum_t::put_byte: open failed for temp file.");
            return (false);
        }
        ac_zfp = zio_stream::zio_open(ac_fp, "wb");
        if (!ac_zfp) {
            Errs()->add_error("accum_t::put_byte: zio_open failed.");
            return (false);
        }

        for (int i = 0; i < CGD_COMPR_BUFSIZE; i++) {
            if (ac_zfp->zio_putc(ac_buf[i]) == EOF) {
                Errs()->add_error("accum_t::put_byte: write failed.");
                return (false);
            }
        }
    }
    if (ac_zfp->zio_putc(c) == EOF) {
        Errs()->add_error("accum_t::put_byte: write failed.");
        return (false);
    }
    ac_count++;
    return (true);
}


// End accumulation, clean up and prepare block.
//
bool
accum_t::finalize()
{
    if (ac_zfp) {
        delete ac_zfp;
        ac_zfp = 0;
        ac_csize = large_ftell(ac_fp);
        rewind(ac_fp);
        ac_data = new unsigned char[ac_csize];
        if ((size_t)fread(ac_data, 1, ac_csize, ac_fp) != ac_csize) {
            Errs()->add_error("accum_t::finalize: read failed.");
            return (false);
        }
        fclose(ac_fp);
        ac_fp = 0;
        if (ac_tempfile) {
            unlink(ac_tempfile);
            delete [] ac_tempfile;
            ac_tempfile = 0;
        }
        return (true);
    }
    if (ac_count <= CGD_COMPR_BUFSIZE)
        return (true);
    Errs()->add_error("accum_t::finalize: compression error.");
    return (false);
}
// End of accum_t functions.


cgd_layer_mux::~cgd_layer_mux()
{
    tgen_t<lm_t> gen(lm_table);
    lm_t *e;
    while ((e = gen.next()) != 0) {
        delete [] e->layername;
        delete e->accum;
        e->modal.reset();
    }
    delete [] lm_cellname;
    delete lm_table;
}


// Set the layer channel for the current output stream.  This creates the
// streams when needed.
//
oas_modal *
cgd_layer_mux::set_layer(int layer, int dtype)
{
    char lname[32];
    strmdata::hexpr(lname, layer, dtype);

    if (!lm_table)
        lm_table = new table_t<lm_t>;
    lm_t *e = lm_table->find(lname);
    if (!e) {
        e = lm_eltab.new_element();
        e->set_tab_next(0);
        e->set_tab_name(lstring::copy(lname));
        e->accum = new accum_t;
        e->modal.reset();
        lm_table->link(e, false);
        lm_table = lm_table->check_rehash();
    }
    lm_accum = e->accum;
    return (&e->modal);
}


// Terminate writing for the present cell, destroying the stream file
// pointers.
//
void
cgd_layer_mux::finalize()
{
    tgen_t<lm_t> gen(lm_table);
    lm_t *e;
    while ((e = gen.next()) != 0)
        e->accum->finalize();
}


// Read the temp file data into data blocks and add these to the
// database.  The temp files are unlinked.
//
bool
cgd_layer_mux::grab_results()
{
    if (!lm_cgd) {
        Errs()->add_error("grab_reaults: no CGD database.");
        return (false);
    }
    if (!lm_cellname) {
        Errs()->add_error("grab_reaults: null cellname.");
        return (false);
    }
    tgen_t<lm_t> gen(lm_table);
    lm_t *e;
    bool data_added = false;
    while ((e = gen.next()) != 0) {
        lm_cgd->add_data(lm_cellname, e->layername, e->accum->data(),
            e->accum->compr_size(), e->accum->uncompr_size());
        delete e->accum;
        e->accum = 0;
        data_added = true;
    }
    if (!data_added) {
        // The cell contained no geometry of interest.  Add the cell
        // to the database anyway, to prevent accessing the source
        // file when reading geometry.

        return (lm_cgd->add_data(lm_cellname, 0, 0, 0, 0));
    }
    return (true);
}


bool
cgd_layer_mux::put_byte(int c)
{
    if (!lm_accum)
        return (true);
    return (lm_accum->put_byte(c));
}
// End of cgd_layer_mux functions.


#define Z_BUFSIZE 16384

// If freedata is true, the data pointer is freed in the destructor.
// The offs is an offset into the data where reading starts, csz/usz
// do not include this offset.

bstream_t::bstream_t(const unsigned char *data, size_t csz, size_t usz,
    bool freedata, int offs)
{
    b_data = data;
    data += offs;
    b_buf = csz ? new unsigned char[Z_BUFSIZE] : data;
    b_errflg = false;
    b_csize = csz;
    b_usize = usz;
    b_nbytes = 0;
    b_offs = 0;
    b_freedata = freedata;

    if (b_csize) {
        b_stream.zalloc = 0;
        b_stream.zfree = 0;
        b_stream.opaque = 0;
        b_stream.next_in = (Bytef*)data;
        b_stream.next_out = (Bytef*)b_buf;
        b_stream.avail_in = csz;
        b_stream.avail_out = Z_BUFSIZE;
        b_stream.total_out = 0;
        b_stream.total_in = 0;

        int err = inflateInit2(&b_stream, -MAX_WBITS);
        if (err != Z_OK) {
            b_errflg = true;
            return;
        }
        err = inflate(&b_stream, Z_NO_FLUSH);
        if (err != Z_STREAM_END && err != Z_OK) {
            b_errflg = true;
            return;
        }
    }
}


int
bstream_t::get_byte()
{
    if (b_nbytes < b_usize) {
        if (b_csize && b_nbytes == b_stream.total_out) {
            b_stream.next_out = (Bytef*)b_buf;
            b_stream.avail_out = Z_BUFSIZE;
            b_offs = 0;
            int err = inflate(&b_stream, Z_NO_FLUSH);
            if (err != Z_STREAM_END && err != Z_OK) {
                b_errflg = true;
                return (0);
            }
        }
        b_nbytes++;
        return (b_buf[b_offs++]);
    }
    return (0);
}
// End of bstream_t functions.

