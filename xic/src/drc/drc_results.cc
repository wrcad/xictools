
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
 $Id: drc_results.cc,v 5.19 2015/07/08 04:44:22 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "fio.h"
#include "drc.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_layer.h"
#include "events.h"
#include "errorlog.h"
#include "promptline.h"
#include "select.h"
#include "ghost.h"
#include "menu.h"
#include "drc_menu.h"
#include "filestat.h"
#include "pathlist.h"
#include "tvals.h"
#include "miscutil.h"
#include "timedbg.h"
#include "timer.h"

#include <ctype.h>
#include <dirent.h>
#ifdef WIN32
#include "msw.h"
#endif

//
// Commands and functions for dealing with the results file.
//

//-----------------------------------------------------------------------------
// A parser for results files.
//-----------------------------------------------------------------------------


namespace drc_results {
    // Return value used by DRCresultParser functions.
    enum RPtype { RPerror, RPok, RPeof };

    struct DRCresultParser
    {
        DRCresultParser();
        ~DRCresultParser();

        RPtype openFile(const char* = 0);
        void closeFile();
        RPtype nextErr(DRCerrList*, char**);
        RPtype prevErr(DRCerrList*, char**);

        static stringlist *listFiles(const char* = 0);

        int curError()      { return (errNum); }
        int maxErrorNum()   { return (errCnt); }  

        void setCurError(int j)
            {
                if (j < 0)
                    j = 0;
                if (j > errCnt)
                    j = errCnt;
                if (j <= errNum && errFp)
                    rewind(errFp);
                errNum = j;
            }

        bool active()                       { return (errOpenOk); }
        void setNotify(void(*f)(DRCresultParser*, const char*))
                                            { errNotify = f; }

    private:
        RPtype open_file(const char*);
        bool find_err(char*, DRCerrList*, bool*, char** = 0);
        CDo *find_object(int, char*);
        CDo *find_odesc(CDl*, BBox*, int, CDs*, cTfmStack*);

        static void callback(const char*, void*);

        FILE *errFp;          // file pointer to error log file
        char *errFname;       // name of error log file opened
        void(*errNotify)(DRCresultParser*, const char*);
                              // notification callback
        int errNum;           // index of currently displayer error
        int errCnt;           // number of errors reported
        bool errOpenOk;       // true if open file succeeds
    };
}

using namespace drc_results;


DRCresultParser::DRCresultParser()
{
    errFp = 0;
    errFname = 0;
    errNotify = 0;
    errNum = -1;
    errCnt = 0;
    errOpenOk = false;
}


DRCresultParser::~DRCresultParser()
{
    if (errFp)
        fclose(errFp);
    delete [] errFname;
}


// Open the file for reading.  If there is ambiguity, launch a
// resolution pop-up and return RPeof.  This must be called before
// other operations.
//
RPtype
DRCresultParser::openFile(const char *fname)
{
    if (fname && *fname) {
        callback(fname, this);
        return (RPok);
    }
    stringlist *wl = listFiles();
    if (!wl) {
        Errs()->add_error("No %s file found for current cell.",
            DRC_EFILE_PREFIX);
        return (RPerror);
    }
    if (wl->next) {
        stringlist::sort(wl);
        DSPmainWbag(PopUpList(wl,
            "Multiple DRC error log files found - click to select",
            0, callback, this, false, true))
        stringlist::destroy(wl);
        return (RPeof);
    }
    callback(wl->string, this);
    stringlist::destroy(wl);
    return (RPok);
}


// Close file, reset context.
//
void
DRCresultParser::closeFile()
{
    if (errFp) {
        fclose(errFp);
        errFp = 0;
    }
    delete [] errFname;
    errFname = 0;
    errNum = -1;
    errCnt = 0;
    errOpenOk = false;
}


// Parse the next error and fill in the err pointer if it is not null. 
// Return a copy of the FAILED message inerrstr if not null.  If
// error, return RPerror with err untouched, with a message in the
// errors system.  Return RPeof on end of file.
//
RPtype
DRCresultParser::nextErr(DRCerrList *err, char **errstr)
{
    if (errstr)
        *errstr = 0;
    if (!errFp) {
        Errs()->add_error("nextErr: file not open.");
        return (RPerror);
    }
    int num;
    char *s, buf[512];
    for (;;) {
        num = -1;
        while ((s = fgets(buf, 512, errFp)) != 0) {
            if (!strncmp(buf, "DRC error", 9)) {
                if (sscanf(buf+10, "%d", &num) == 1 && num > errNum)
                    break;
            }
        }
        if (!s)
            return (RPeof);
        break;
    }

    char line[256];
    sprintf(line, "%d) ", num);
    char *t = line + strlen(line);

    for (;;) {
        s = strchr(s, 'c');
        if (!s)
            break;
        if (*(s+1) == 'e' && *(s+2) == 'l' && *(s+3) == 'l') {
            s += 4;
            while (isspace(*s))
                s++;
            *t++ = 'I';
            *t++ = 'n';
            *t++ = ' ';
            while (*s && *s != ':' && !isspace(*s))
                *t++ = *s++;
            *t++ = ' ';
            *t = 0;
            break;
        }
    }
    errNum = num;

    if (!fgets(t, 256 - (t-line), errFp)) {
        clearerr(errFp);
        Errs()->add_error("nextErr: premature end of file.");
        return (RPerror);
    }
    NTstrip(line, true);

    bool eof;
    if (!find_err(buf, err, &eof, errstr)) {
        if (eof) {
            clearerr(errFp);
            Errs()->add_error("nextErr: premature end of file.");
        }
        else
            Errs()->add_error("nextErr: parse error, bad file format.");
        return (RPerror);
    }
    if (errstr && *errstr) {
        t = line + strlen(line) - 1;
        while (t >= line && isspace(*t))
            t--;
        if (*t == ':')
            sprintf(t+1, " %s", *errstr);
        delete [] *errstr;
        *errstr = lstring::copy(line);
    }
    return (RPok);
}


// Parse the previous error and fill in the err pointer if it is not
// null.  Return a copy of the FAILED message inerrstr if not null. 
// If error, return RPerror with err untouched, with a message in the
// errors system.  Return RPeof on end of file.
//
RPtype
DRCresultParser::prevErr(DRCerrList *err, char **errstr)
{
    if (errstr)
        *errstr = 0;
    if (!errFp) {
        Errs()->add_error("prevErr: file not open.");
        return (RPerror);
    }
    if (errNum <= 1)
        return (RPeof);
    long ftold = ftell(errFp);
    ftold -= 1000;
    if (ftold < 0)
        ftold = 0;
    fseek(errFp, ftold, SEEK_SET);

    int num;
    char *t, line[256];
    char *s, buf[512];
    do {
        while ((s = fgets(buf, 512, errFp)) != 0) {
            if (!strncmp(buf, "DRC error", 9)) {
                if (sscanf(buf+10, "%d", &num) == 1 && num > 0)
                    break;
            }
        }
        if (!s) {
            clearerr(errFp);
            errNum = 0;
            return (RPeof);
        }
        sprintf(line, "%d) ", num);
        t = line + strlen(line);
        for (;;) {
            s = strchr(s, 'c');
            if (!s)
                break;
            if (*(s+1) == 'e' && *(s+2) == 'l' && *(s+3) == 'l') {
                s += 4;
                while (isspace(*s))
                    s++;
                *t++ = 'I';
                *t++ = 'n';
                *t++ = ' ';
                while (*s && *s != ':' && !isspace(*s))
                    *t++ = *s++;
                *t++ = ' ';
                *t = 0;
                break;
            }
        }
    } while (num < errNum - 1);
    if (num != errNum - 1)
        // shouldn't happen
        return (RPeof);
    errNum = num;

    if (!fgets(t, 256 - (t-line), errFp)) {
        clearerr(errFp);
        Errs()->add_error("prevErr: premature end of file.");
        return (RPerror);
    }
    NTstrip(line, true);

    bool eof;
    if (!find_err(buf, err, &eof, errstr)) {
        if (eof) {
            clearerr(errFp);
            Errs()->add_error("prevErr: premature end of file.");
        }
        else
            Errs()->add_error("prevErr: parse error, bad file format.");
        return (RPerror);
    }
    if (errstr && *errstr) {
        t = line + strlen(line) - 1;
        while (t >= line && isspace(*t))
            t--;
        if (*t == ':')
            sprintf(t+1, " %s", *errstr);
        delete [] *errstr;
        *errstr = lstring::copy(line);
    }
    return (RPok);
}


// Open a new file and reset context.
//
RPtype
DRCresultParser::open_file(const char *name)
{
    errOpenOk = false;
    if (!name || !*name) {
        Errs()->add_error("open_file: null or empty file name given.");
        return (RPerror);
    }
    FILE *fp = fopen(name, "rb");
    if (!fp) {
        Errs()->add_error("open_file: could not open %s.", name);
        Errs()->sys_error("name");
        return (RPerror);
    }

    char buf[512];
    char *s = fgets(buf, 512, fp);
    NTstrip(buf);
    if (!s) {
        Errs()->add_error("open_file: premature end of file.");
        fclose(fp);
        return (RPerror);
    }

    int error_count = -1;
    if (!strncmp(buf, DRC_EFILE_HEADER, strlen(DRC_EFILE_HEADER))) {
        s = buf + strlen(DRC_EFILE_HEADER) + 1;
        while (isspace(*s))
            s++;
        char *t = s;
        while (*t && !isspace(*t))
            t++;
        *t = '\0';
        if (strcmp(s, Tstring(DSP()->CurCellName()))) {
            // This can happen when background job finishes if user
            // switches edit cell.
            Errs()->add_error(
                "open_file: file found is not for current cell.");
            fclose(fp);
            return (RPerror);
        }
        // Look at end for error count.
        long posn = ftell(fp);

        // Find the "End check" line near the end of the file, this
        // will contain a number giving the error count.  We back up
        // by tha amount below, which may need to change if additional
        // stuff is printed at the end of the file.
#define BACKNCHARS -100

        fseek(fp, BACKNCHARS, SEEK_END);
        // Should be ahead of "End check" line, read to next newline.
        fgets(buf, 512, fp);
        while ((s = fgets(buf, 512, fp)) != 0) {
            if (lstring::prefix("End", buf)) {
                while (*s && !isdigit(*s))
                    s++;
                if (*s)
                    error_count = atoi(s);
                break;
            }
        }
        fseek(fp, posn, SEEK_SET);
        if (error_count < 0) {
            Log()->PopUpErr(
                "Unable to find error count at end of file,\n"
                "access by number won't work.");
        }
    }
    else {
        Errs()->add_error("open_file: format error in DRC results file.");
        fclose(fp);
        return (RPerror);
    }

    // Switch context to new file.
    if (errFp)
        fclose(errFp);
    errFp = fp;
    if (!errFname || strcmp(errFname, name)) {
        delete [] errFname;
        errFname = lstring::copy(name);
    }
    errNum = 0;
    errCnt = error_count;
    errOpenOk = true;
    return (RPok);
}


// Find and set the error object (if found) and error region in err,
// if err is not null.  Return true if success, false otherwise, and
// set eof if end of file.  If fmsg, return a copy of the "Failed..."
// line.
//
bool
DRCresultParser::find_err(char *buf, DRCerrList *err, bool *eof, char **fmsg)
{
    *eof = false;
    CDo *odesc = 0;
    char *s;
    while ((s = fgets(buf, 512, errFp)) != 0) {
        while (isspace(*s))
            s++;
        if (lstring::ciprefix("FAILED", s)) {
            if (fmsg) {
                NTstrip(s, true);
                *fmsg = lstring::copy(s);
            }
            break;
        }
        if (lstring::ciprefix("BOX", s))
            odesc = find_object(CDBOX, s+3);
        else if (lstring::ciprefix("POLYGON", s))
            odesc = find_object(CDPOLYGON, s+7);
        else if (lstring::ciprefix("WIRE", s))
            odesc = find_object(CDWIRE, s+4);
    }
    if (!s) {
        *eof = true;
        return (false);
    }

    // The "in poly..." starts on the next line after "Failed..." in
    // more recent releases, but was in the same line originally.
    for (int pass = 0; pass < 2; pass++) {
        if ((s = strrchr(buf, 'z')) != 0 && !strncmp(s, "zoid", 4)) {
            s += 5;
            double yl, yu, xll, xul, xlr, xur;
            if (sscanf(s, "%lf %lf, %lf %lf %lf %lf", &yl, &yu, &xll,
                    &xul, &xlr, &xur) == 6) {
                // For compatibility with old versions.
                if (err) {
                    err->set_pbad(0, INTERNAL_UNITS(xll), INTERNAL_UNITS(yl));
                    err->set_pbad(1, INTERNAL_UNITS(xul), INTERNAL_UNITS(yu));
                    err->set_pbad(2, INTERNAL_UNITS(xur), INTERNAL_UNITS(yu));
                    err->set_pbad(3, INTERNAL_UNITS(xlr), INTERNAL_UNITS(yl));
                    err->set_pbad(4, INTERNAL_UNITS(xll), INTERNAL_UNITS(yl));
                    err->set_pointer(odesc);
                }
                else
                    delete odesc;
                return (true);
            }
        }
        else if ((s = strrchr(buf, 'p')) != 0 && !strncmp(s, "poly", 4)) {
            s += 5;
            double x1, y1, x2, y2, x3, y3, x4, y4;
            if (sscanf(s, "%lf,%lf %lf,%lf %lf,%lf %lf,%lf", &x1, &y1,
                    &x2, &y2, &x3, &y3, &x4, &y4) == 8) {
                if (err) {
                    err->set_pbad(0, INTERNAL_UNITS(x1), INTERNAL_UNITS(y1));
                    err->set_pbad(1, INTERNAL_UNITS(x2), INTERNAL_UNITS(y2));
                    err->set_pbad(2, INTERNAL_UNITS(x3), INTERNAL_UNITS(y3));
                    err->set_pbad(3, INTERNAL_UNITS(x4), INTERNAL_UNITS(y4));
                    err->set_pbad(4, INTERNAL_UNITS(x1), INTERNAL_UNITS(y1));
                    err->set_pointer(odesc);
                }
                else
                    delete odesc;
                return (true);
            }
        }
        else if (pass == 0) {
            long place = ftell(errFp);
            // read the next line and try again
            if (fgets(buf, 512, errFp) != 0) {
                if (!strncmp(buf, "DRC error", 9)) {
                    fseek(errFp, place, SEEK_SET);
                    break;
                }
            }
            else {
                clearerr(errFp);
                break;
            }
        }
    }
    delete odesc;
    return (false);
}


// Given the string, find the odesc in the database with the matching
// bounding box on the layer given.  This must be the object which caused
// the error.
//
// The returned object is a copy with coordinates translated to top
// level.
//
CDo *
DRCresultParser::find_object(int type, char *str)
{
    // "on layer bounded by xx,xx xx,xx"
    char *layer = 0;
    char *tok = lstring::gettok(&str);
    if (!tok)
        return (0);
    if (!strcmp(tok, "on")) {
        delete [] tok;
        layer = lstring::gettok(&str);
        if (!layer)
            return (0);
    }
    else
        layer = tok;
    lstring::advtok(&str);  // "bounded"
    lstring::advtok(&str);  // "by"

    double l, b, r, t;
    if (sscanf(str, "%lf,%lf %lf,%lf", &l, &b, &r, &t) != 4)
        return (0);
    CDl *ld = CDldb()->findLayer(layer, Physical);
    delete [] layer;
    if (!ld)
        return (0);
    BBox BB(INTERNAL_UNITS(l), INTERNAL_UNITS(b),
        INTERNAL_UNITS(r), INTERNAL_UNITS(t));
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (0);
    cTfmStack stk;
    return (find_odesc(ld, &BB, type, cursdp, &stk));
}


// The returned object is a copy with coordinates translated to top
// level.
//
CDo *
DRCresultParser::find_odesc(CDl *ld, BBox *BB, int type, CDs *sdesc,
    cTfmStack *tstk)
{
    BBox tBB = *BB;
    tstk->TInverse();
    tstk->TInverseBB(&tBB, 0);
    CDg gdesc;
    tstk->TInitGen(sdesc, ld, BB, &gdesc);
    CDo *odesc;
    while ((odesc = gdesc.next()) != 0) {
        if (odesc->type() != type)
            continue;
        if (odesc->oBB() == tBB) {
            CDo *newdesc = odesc->copyObjectWithXform(tstk);
            return (newdesc);
        }
    }

    // Must be in a subcell.
    if (tstk->TFull()) {
        Errs()->add_error(
            "transform stack overflow, cell hierarchy recursive?");
        return (0);
    }
    tstk->TInitGen(sdesc, CellLayer(), BB, &gdesc);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        CDs *msdesc = cdesc->masterCell();
        tstk->TPush();
        unsigned int x1, x2, y1, y2;
        if (tstk->TOverlapInst(cdesc, BB, &x1, &x2, &y1, &y2)) {
            CDap ap(cdesc);
            int tx, ty;
            tstk->TGetTrans(&tx, &ty);
            xyg_t xyg(x1, x2, y1, y2);
            do {
                tstk->TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                CDo *newdesc = find_odesc(ld, BB, type, msdesc, tstk);
                if (newdesc) {
                    tstk->TPop();
                    return (newdesc);
                }
                tstk->TSetTrans(tx, ty);
            } while (xyg.advance());
        }
        tstk->TPop();
    }
    return (0);
}


// Static function.
// Return a list of files in dir whose names match the prefix, and which
// are for the current cell.
//
stringlist *
DRCresultParser::listFiles(const char *dir)
{
    if (!dir)
        dir = ".";
    const char *prefix = DRC_EFILE_PREFIX;
    DIR *wdir;
    if (!(wdir = opendir(dir)))
        return (0);
    char *t = 0;
    stringlist *wl = 0, *wl0 = 0;
    struct dirent *de;
    while ((de = readdir(wdir)) != 0) {
        if (strncmp(de->d_name, prefix, strlen(prefix)))
            continue;
        char *fpath = pathlist::mk_path(dir, de->d_name);
        FILE *fp = fopen(fpath, "r");
        if (!fp) {
            delete [] fpath;
            continue;
        }
        // The cell name is the last word on the line.
        char buf[256];
        if (fgets(buf, 256, fp) && (t = buf + strlen(buf) - 1) > buf) {
            *t-- = 0;  // null '\n'
            if (*t == '\r')
                *t-- = 0;
            while (t >= buf && !isspace(*t))
                t--;
            t++;
            if (!strcmp(t, Tstring(DSP()->CurCellName()))) {
                // avoid printing "./"
                t = strrchr(fpath, '/');
                char *fn;
                if (t == buf + 2 && *(buf+1) == '.')
                    fn = lstring::copy(t+1);
                else
                    fn = lstring::copy(fpath);
                if (wl0 == 0)
                    wl = wl0 = new stringlist(fn, 0);
                else {
                    wl->next = new stringlist(fn, 0);
                    wl = wl->next;
                }
            }
        }
        delete [] fpath;
        fclose(fp);
    }
    closedir(wdir);
    return (wl0);
}


// Static function.
// Read in the error log and initialize.
//
// The notify callback will be called whether or not the open succeeds,
// so the callback should check this.  Also, the notify callback will
// be called with name=null when the list widget is destroyed.
//
void
DRCresultParser::callback(const char *name, void *arg)
{
    DRCresultParser *prsr = (DRCresultParser*)arg;
    if (!prsr)
        return;
    if (name) {
        RPtype ret = prsr->open_file(name);
        if (ret == RPerror)
            Log()->ErrorLog(mh::DRC, Errs()->get_error());
    }
    if (prsr->errNotify)
        (*prsr->errNotify)(prsr, name);
}
// End of DRCresultParser functions.


//-----------------------------------------------------------------------------
// Sequential error viewing: the "next" command.
//-----------------------------------------------------------------------------

namespace {
    // This encapsulates the "Next" (sequential error view) command.
    //
    struct SeqErrView
    {
        SeqErrView()
            {
                sev_caller = 0;
                sev_parser = 0;
                sev_win = -1;
                sev_lockout = false;
            }

        bool active()
            {
                return (sev_parser && sev_parser->active());
            }

        int win_num()                   { return (sev_win); }
        DRCerrList *err_ptr()           { return (&sev_err); }
        void *caller()                  { return (sev_caller); }
        bool lockout()                  { return (sev_lockout); }

        void reset_file(const char *fname)
            {
                if (!active() || !fname)
                    return;
                sev_parser->openFile(fname);
            }

        void clear()
            {
                delete sev_parser;
                sev_parser = 0;
                delete sev_err.pointer();
                sev_err.set_pointer(0);
                sev_win = -1;
                sev_caller = 0;
                sev_lockout = false;
            }

        RPtype open(void*);
        void set_next_error(int, bool, bool);
        void next_error();
        void prev_error();

    private:
        void show_error();
        static void notify(DRCresultParser*, const char*);

        void *sev_caller;
        DRCresultParser *sev_parser;
        DRCerrList sev_err;
        int sev_win;
        bool sev_lockout;
    };

    SeqErrView SEV;


    RPtype
    SeqErrView::open(void *caller_btn)
    {
        if (active())
            return (RPok);
        delete sev_parser;
        sev_parser = new DRCresultParser;
        sev_parser->setNotify(notify);
        RPtype ret = sev_parser->openFile();
        if (ret == RPerror) {
            Log()->ErrorLog(mh::DRC, Errs()->get_error());
            delete sev_parser;
            sev_parser = 0;
        }
        else {
            sev_caller = caller_btn;
            if (ret == RPeof)
                sev_lockout = true;
        }
        return (ret);
    }


    void
    SeqErrView::set_next_error(int n, bool relative, bool negative)
    {
        if (!active())
            return;

        if (relative) {
            if (negative)
                n = sev_parser->curError() - n;
            else
                n = sev_parser->curError() + n;
        }
        n--;
        if (n < 0)
            n = 0;
        sev_parser->setCurError(n);
    }


    void
    SeqErrView::next_error()
    {
        if (!active())
            return;
        const CDo *od = sev_err.pointer();
        char *string;
        RPtype ret = sev_parser->nextErr(&sev_err, &string);
        if (ret == RPok) {
            show_error();
            PL()->ShowPrompt(string);
            delete [] string;
            delete od;
            return;
        }
        if (ret == RPeof) {
            PL()->ShowPrompt("No more errors.");
            return;
        }
        Log()->ErrorLog(mh::DRC, Errs()->get_error());
    }


    void
    SeqErrView::prev_error()
    {
        if (!active())
            return;
        const CDo *od = sev_err.pointer();
        char *string;
        RPtype ret = sev_parser->prevErr(&sev_err, &string);
        if (ret == RPok) {
            show_error();
            PL()->ShowPrompt(string);
            delete [] string;
            delete od;
            return;
        }
        if (ret == RPeof) {
            PL()->ShowPrompt("No more errors.");
            return;
        }
        Log()->ErrorLog(mh::DRC, Errs()->get_error());
    }


    // Actually display the current error in a pop-up window.
    //
    void
    SeqErrView::show_error()
    {
        int left, right, bottom, top;
        left = right = sev_err.pbad(0).x;
        bottom = top = sev_err.pbad(0).y;
        for (int i = 1; i < 4; i++) {
            if (left > sev_err.pbad(i).x)
                left = sev_err.pbad(i).x;
            if (bottom > sev_err.pbad(i).y)
                bottom = sev_err.pbad(i).y;
            if (right < sev_err.pbad(i).x)
                right = sev_err.pbad(i).x;
            if (top < sev_err.pbad(i).y)
                top = sev_err.pbad(i).y;
        }
        int wid = (right - left)*4;
        int ht = (top - bottom)*4;
        if (ht > wid)
            wid = ht;
        BBox AOI;
        AOI.left = (left + right - wid)/2;
        AOI.right = (left + right + wid)/2;
        AOI.bottom = (bottom + top - wid)/2;
        AOI.top = (bottom + top + wid)/2;
        if (sev_win < 0 || !DSP()->Window(sev_win) ||
                !DSP()->Window(sev_win)->IsSimilar(DSP()->MainWdesc()))
            sev_win = DSP()->OpenSubwin(&AOI);
        else {
            DSP()->Window(sev_win)->InitWindow(&AOI);
            DSP()->Window(sev_win)->Redisplay(0);
        }
    }


    // Static function.
    // This is called when a results file has been opened, and when the
    // ambiguity list is destroyed (with name=null).  Bail out if the
    // open failed, set the menu button and show the initial message if
    // ok.  This also lifts the lock which prevents restarting the command
    // when the ambiguity list is showing.
    //
    void
    SeqErrView::notify(DRCresultParser*, const char *name)
    {
        SEV.sev_lockout = false;
        if (!name)
            return;

        // If the command is already active, pop down the window to show
        // that the error list has changed.  This happens when a
        // background DRC job completes (which overwrites the results
        // file).
        if (SEV.sev_win > 0) {
            delete DSP()->Window(SEV.sev_win);
            SEV.sev_win = -1;
            // This now terminates the command, the next block will return.
        }
        if (!SEV.active()) {
            // Open failed.
            SEV.clear();
            return;
        }
        if (SEV.sev_caller)
            Menu()->SetStatus(SEV.sev_caller, true);

        if (SEV.sev_parser->maxErrorNum() == 0)
            PL()->ShowPrompt("No errors recorded.");
        else if (SEV.sev_parser->maxErrorNum() > 0)
            PL()->ShowPromptV(
                "%d errors recorded.  "
                "Press PageDn to view errors, PageUp for previous.",
                SEV.sev_parser->maxErrorNum());
        else
            PL()->ShowPrompt(
            "Press PageDn to view next error, PageUp for previous errors.");
    }
}


// Initiate the sequential error view mode.
//
void
cDRC::viewErrsSequentiallyExec(CmdDesc *cmd)
{
    if (SEV.lockout()) {
        // Note:  the caller button is deselected during ambiguity
        // resolution, and this function is locked out.
        if (SEV.caller())
            Menu()->SetStatus(SEV.caller(), false);
        return;
    }
    if (!Menu()->GetStatus(cmd->caller)) {
        SEV.clear();
        return;
    }
    if (SEV.active())
        // "can't happen"
        return;

    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Physical))
        return;
    if (!XM()->CheckCurCell(false, false, Physical))
        return;

    RPtype ret = SEV.open(cmd->caller);
    if (ret == RPerror) {
        SEV.clear();
        return;
    }
    if (ret == RPeof)
        return;
    ds.clear();
}


// Exit the sequential error view mode.
//
void
cDRC::cancelNext()
{
    if (SEV.active()) {
        Menu()->Deselect(SEV.caller());
        SEV.clear();
    }
}


// Interface callback called when a drawing window is destroyed. 
// Cancel sequential error display mode if the user destroys the
// display window.
//
void
cDRC::windowDestroyed(int wnum)
{
    if (SEV.active() && wnum == SEV.win_num())
        cancelNext();
}


// Callback from the keypress handler.  Display the next or previous
// error.  The text can indicate a number for "nxterr".
//
int
cDRC::errCb(int c, const char *text, int txtlen)
{
    if (SEV.active()) {
        switch (c) {
        case 6:  // ^F
            if (txtlen) {
                int j = 0;
                bool rel = false;
                if (*text == '-' || *text == '+') {
                    rel = true;
                    text++;
                    txtlen--;
                }
                for (int i = 0; i < txtlen; i++) {
                    if (isdigit(text[i])) {
                        j *= 10;
                        j += (text[i] - '0');
                    }
                    else
                        break;
                }
                SEV.set_next_error(j, rel, (*(text-1) == '-'));
            }
            SEV.next_error();
            return (1);
        case 2:  // ^B
        case 16: // ^P
            SEV.prev_error();
            return (1);
        }
    }
    return (0);
}


// This takes care of the case when the "next" command is active, and the
// user reruns the DRC creating a new file.
//
void
cDRC::errReset(const char *fname, int pid)
{
    if (!SEV.active())
        return;
    if (pid > 0 && !fname) {
        char *fn = errFilename(Tstring(DSP()->CurCellName()), pid);
        if (access(fn, F_OK) == 0)
            SEV.reset_file(fn);
        delete [] fn;
        return;
    }
    SEV.reset_file(fname);
}


// This function is called by DRCshowCurError().  Only the current error
// is shown in the pop up subwindow.
//
bool
cDRC::altShowError(WindowDesc *wdesc, bool display)
{
    if (!SEV.active() || SEV.win_num() < 0 ||
            wdesc != DSP()->Window(SEV.win_num()) ||
            !wdesc->IsSimilar(DSP()->MainWdesc()))
        return (false);
    DRC()->showError(wdesc, display, SEV.err_ptr());
    return (true);
}
// End of "next" command implementation.


namespace {
    // This prevents restarting the command while waiting for action
    // in the ambiguity list.
    bool elist_lockout;

    void update_errlist(DRCresultParser *prsr, const char *name)
    {
        if (name && prsr->active())
            DRC()->updateErrlist(prsr);
        if (!name) {
            delete prsr;
            elist_lockout = false;
        }
    }
}


// Rebuild the error highlighting list from a file.
//
void
cDRC::setErrlist()
{
    if (elist_lockout)
        return;
    if (!XM()->CheckCurMode(Physical))
        return;
    if (!XM()->CheckCurCell(false, false, Physical))
        return;

    DRCresultParser *prsr = new DRCresultParser;
    RPtype ret = prsr->openFile();
    if (ret == RPerror) {
        Log()->ErrorLog(mh::DRC, Errs()->get_error());
        return;
    }
    if (ret == RPeof) {
        prsr->setNotify(update_errlist);
        elist_lockout = true;
        return;
    }
    update_errlist(prsr, "");
}


void
cDRC::updateErrlist(DRCresultParser *prsr)
{
    if (!prsr->active())
        return;
    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wd = wgen.next()) != 0)
        showCurError(wd, ERASE);
    clearCurError();

    bool ok = true;
    DRCerrList *end = 0;
    for (;;) {
        DRCerrList *err = new DRCerrList;
        char *string;
        RPtype ret = prsr->nextErr(err, &string);
        if (ret == RPeof) {
            delete err;
            break;
        }
        if (ret == RPerror) {
            delete err;
            Log()->ErrorLog(mh::DRC, Errs()->get_error());
            ok = false;
            break;
        }
        err->set_message(string);
        delete [] string;
        if (!end)
            drc_err_list = end = err;
        else {
            end->set_next(err);
            end = err;
        }
    }

    wgen = WDgen(WDgen::MAIN, WDgen::CDDB);
    while ((wd = wgen.next()) != 0)
        showCurError(wd, DISPLAY);
    if (ok)
        PL()->ShowPrompt("DRC error highlighting updated from file.");
    else
        PL()->ShowPrompt("Error occurred, highlighting update incomplete.");
}


// Create a polygon on lname for each error region, and give the poly
// a property prpnum containing the drc error text, if prpnum > 0.
// The lname layer is cleared first.
//
bool
cDRC::errLayer(const char *lname, int prpnum)
{
    CDs *cursdp = CurCell(Physical);
    if (!drc_err_list || !cursdp)
        return (true);
    CDl *ld = CDldb()->findLayer(lname, Physical);
    if (!ld)
        ld = CDldb()->newLayer(lname, Physical);
    if (!ld) {
        Log()->ErrorLog(mh::DRC, Errs()->get_error());
        return (false);
    }

    int cnt = 0;
    sLstr lstr;
    char buf[256];

    cursdp->clearLayer(ld);

    for (DRCerrList *el = drc_err_list; el; el = el->next()) {
        const char *msg = el->message();
        if (!msg)
            continue;
        Point *pts = new Point[5];
        for (int i = 0; i < 5; i++)
            pts[i] = el->pbad(i);
        Poly poly(5, pts);
        CDpo *npoly;
        if (cursdp->makePolygon(ld, &poly, &npoly, 0, false) != CDok)
            continue;
        if (prpnum > 0) {
            if (lstring::prefix("In", msg)) {
                const char *t = msg;
                lstring::advtok(&t);
                char *cname = lstring::gettok(&t);
                if (!cname)
                    continue;
                const char *string = t;
                char *e = cname + strlen(cname) - 1;
                if (*e == ':')
                    *e = 0;
                sprintf(buf, "DRC error %d in cell %s:\n", cnt + 1, cname);
                lstr.add(buf);
                delete [] cname;
                lstr.add(string);
                if (string[strlen(string) - 1] != '\n' && el->descr())
                    lstr.add_c('\n');
            }
            else {
                lstr.add(msg);
                if (msg[strlen(msg) - 1] != '\n' && el->descr())
                    lstr.add_c('\n');
            }
            if (el->descr())
                lstr.add(el->descr());
            npoly->set_prpty_list(new CDp(lstr.string(), prpnum));
            lstr.clear();
        }
        cnt++;
    }
    return (true);
}


// Create a results file from the highlighting list.
//
void
cDRC::dumpResultsFile()
{
    if (!DRC()->isError()) {
        PL()->ShowPrompt("No errors in highlighting list, nothing to dump.");
        return;
    }
    char *in = PL()->EditPrompt("Enter file name, blank for a temp file: ",
        DRC_EFILE_PREFIX);
    in = lstring::strip_space(in);
    if (!in)
        return;
    bool tempf = false;
    char *fname;
    if (*in)
        fname = lstring::copy(in);
    else {
        fname = filestat::make_temp("dr");
        tempf = true;
    }
    int ret = DRC()->dumpCurError(fname);
    if (ret < 0) {
        PL()->ShowPromptV("Error: can't open file %s.", fname);
        delete [] fname;
        return;
    }
    if (ret == 0) {
        PL()->ShowPrompt("Nothing to dump, error list is empty!");
        delete [] fname;
        return;
    }
    if (tempf)
        filestat::queue_deletion(fname);
    in = PL()->EditPrompt("View file? ", "n");
    if (in && (*in == 'y' || *in == 'Y'))
        DSPmainWbag(PopUpFileBrowser(fname))
    delete fname;
    PL()->ShowPrompt("File written successfully.");
}

