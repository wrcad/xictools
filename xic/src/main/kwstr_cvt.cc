
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
 $Id: kwstr_cvt.cc,v 5.3 2014/11/06 06:01:08 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "kwstr_cvt.h"
#include "cd_strmdata.h"
#include "promptline.h"
#include "events.h"

//
// Handling for layer block keywords: conversion-related keywords.
//


// Load the keyword list and the widget text (sorted).
//
void
cvtKWstruct::load_keywords(const CDl *ld, const char *string)
{
    clear_undo_list();
    kw_list->free();
    kw_list = 0;
    char *localstr = 0;
    if (!string) {
        localstr = get_settings(ld);
        string = localstr;
    }
    if (string) {
        while (*string) {
            string = lstring::strip_space(string);
            const char *t = string;
            while (*t && *t != '\n')
                t++;
            if (*string) {
                char *ns = new char[t - string + 1];
                strncpy(ns, string, t - string);
                ns[t - string] = 0;
                kw_list = new stringlist(ns, kw_list);
            }
            if (*t)
                t++;
            string = t;
        }
    }
    delete [] localstr;
}


// Insert the keyword string into the list.  The return is a status
// string (usually nil).
//
char *
cvtKWstruct::insert_keyword_text(const char *str, const char*, const char*)
{
    cvKW type = kwtype(str);
    if (type == cvNil)
        return (lstring::copy("Unrecognized keyword."));

    clear_undo_list();
    remove_keyword_text(type, false, 0, 0);
    kw_newstr = lstring::copy(str);
    kw_list = new stringlist(kw_newstr, kw_list);
    return (0);
}


void
cvtKWstruct::remove_keyword_text(int type, bool, const char *string,
    const char*)
{
    stringlist *lp = 0;
    for (stringlist *l = kw_list; l; l = l->next) {
        if (type == kwtype(l->string) &&
                (!string || !strcmp(string, l->string))) {
            remove(lp, l);
            return;
        }
        lp = l;
    }
}


// Return a char string, with each keyword/value taking one line.
//
char *
cvtKWstruct::list_keywords()
{
    sort();
    char *s = kw_list->flatten("\n");
    if (!s)
        s = lstring::copy("");
    return (s);
}


// Return true if type is in list.
//
bool
cvtKWstruct::inlist(cvKW type)
{
    for (stringlist *l = kw_list; l; l = l->next) {
        if (type == kwtype(l->string))
            return (true);
    }
    return (false);
}


namespace {
    // Sort comparison, order is same as enum.
    //
    bool sortcmp(const char *a, const char *b)
    {
        int i1 = cvtKWstruct::kwtype(a);
        int i2 = cvtKWstruct::kwtype(b);
        if (i1 != i2)
            return (i1 < i2);
        return (strcmp(a, b) < 0);
    }
}


void
cvtKWstruct::sort()
{
    kw_list->sort(sortcmp);
}


// Get text input from user.
//
char *
cvtKWstruct::prompt(const char *inittext, const char *deftext)
{
    kw_editing = true;
    char *s = PL()->EditPrompt(inittext, deftext);
    kw_editing = false;
    if (!s)
        PL()->ErasePrompt();
    return (lstring::strip_space(s));
}


namespace {
    // Return false if a token is out of range.
    //
    bool
    checklist(char *s, int maxv)
    {
        char *tok;
        while ((tok = lstring::gettok(&s)) != 0) {
            char *t2 = strchr(tok, '-');
            int d;
            if (t2) {
                *t2++ = 0;
                if (!isdigit(*t2)) {
                    delete [] tok;
                    return (false);
                }
                d = atoi(t2);
                if (d < 0 || d >= maxv) {
                    delete [] tok;
                    return (false);
                }
            }
            d = atoi(tok);
            if (d < 0 || d >= maxv) {
                delete [] tok;
                return (false);
            }
            delete [] tok;
        }
        return (true);
    }


    // Get the two sep separated arguments that follow a keyword.
    //
    void
    separate(const char *string, char sep, char *first, char *second)
    {
        if (first)
            *first = 0;
        if (second)
            *second = 0;
        if (!string)
            return;
        lstring::advtok(&string);
        const char *s = strchr(string, sep);
        if (first) {
            if (s) {
                strncpy(first, string, s - string);
                first[s - string] = 0;
            }
            else
                strcpy(first, string);
            char *t = first + strlen(first) - 1;
            while (t >= first && isspace(*t))
                *t-- = 0;
        }
        if (second && s) {
            s++;
            while (isspace(*s))
                s++;
            strcpy(second, s);
            char *t = second + strlen(second) - 1;
            while (t >= second && isspace(*t))
                *t-- = 0;
        }
    }
}


// Return the string for the keyword indicated by which.
//
char *
cvtKWstruct::get_string_for(int type, const char *orig)
{
    EV()->InitCallback();
    char first[128], second[128];
    char *in, buf[256];
    switch (type) {
    case cvStreamIn:
        separate(orig, ',', first, second);
        strcpy(buf, Tkw.StreamIn());
        in = prompt("Enter layer number list: ", first);
        for (;;) {
            if (!in)
                return (0);
            bool bad = false;
            bool hasdigit = false;
            for (char *s = in; *s; s++) {
                if (isdigit(*s)) {
                    hasdigit = true;
                    continue;
                }
                if (!isspace(*s) && *s != '-') {
                    bad = true;
                    break;
                }
            }
            if (!bad && hasdigit) {
                if (checklist(in, GDS_MAX_LAYERS)) {
                    sprintf(buf + strlen(buf), " %s", in);
                    break;
                }
            }
            in = prompt("Bad input, reenter layer number list: ", first);
        }
        in = prompt("Enter datatype list: ", second);
        for (;;) {
            if (!in)
                return (0);
            bool bad = false;
            bool hasdigit = false;
            for (char *s = in; *s; s++) {
                if (isdigit(*s)) {
                    hasdigit = true;
                    continue;
                }
                if (!isspace(*s) && *s != '-') {
                    bad = true;
                    break;
                }
            }
            if (!bad && hasdigit) {
                if (checklist(in, GDS_MAX_DTYPES)) {
                    sprintf(buf + strlen(buf), ", %s", in);
                    break;
                }
            }
            in = prompt("Bad input, reenter datatype list: ", second);
        }
        break;
    case cvStreamOut:
        strcpy(buf, Tkw.StreamOut());
        separate(orig, ' ', first, second);
        in = prompt("Enter output layer number: ", first);
        for (;;) {
            if (!in)
                return (0);
            bool hasdigit = false;
            for (char *s = in; *s; s++) {
                if (isdigit(*s)) {
                    hasdigit = true;
                    continue;
                }
                *s = 0;
                break;
            }
            if (hasdigit) {
                int d = atoi(in);
                if (d >= 0 && d < GDS_MAX_LAYERS) {
                    sprintf(buf + strlen(buf), " %s", in);
                    break;
                }
            }
            in = prompt("Bad input, reenter output layer number [0-65535]: ",
                first);
        }
        in = prompt("Enter output datatype number: ", second);
        for (;;) {
            if (!in)
                return (0);
            bool hasdigit = false;
            for (char *s = in; *s; s++) {
                if (isdigit(*s)) {
                    hasdigit = true;
                    continue;
                }
                *s = 0;
                break;
            }
            if (hasdigit) {
                int d = atoi(in);
                if (d >= 0 && d < GDS_MAX_DTYPES) {
                    sprintf(buf + strlen(buf), " %s", in);
                    break;
                }
            }
            in = prompt("Bad input, reenter output datatype number [0-65535]: ",
                second);
        }
        break;
    case cvNoDrcDataType:
        strcpy(buf, Tkw.NoDrcDataType());
        separate(orig, ' ', first, 0);
        in = prompt("Enter datatype number to exclude in DRC: ", first);
        for (;;) {
            if (!in)
                return (0);
            bool hasdigit = false;
            for (char *s = in; *s; s++) {
                if (isdigit(*s)) {
                    hasdigit = true;
                    continue;
                }
                *s = 0;
                break;
            }
            if (hasdigit) {
                int d = atoi(in);
                if (d >= 0 && d < GDS_MAX_DTYPES) {
                    sprintf(buf + strlen(buf), " %s", in);
                    break;
                }
            }
            in = prompt(
            "Bad input, reenter datatype number for DRC exclusion [0-65535]: ",
                first);
        }
        break;
    }
    PL()->ErasePrompt();
    return (lstring::copy(buf));
}


// Static function.
// Return the type of the keyword string.
//
cvKW
cvtKWstruct::kwtype(const char *str)
{
    char *tok = lstring::gettok(&str);
    cvKW ret = cvNil;
    if (tok) {
        if (lstring::cieq(tok, Tkw.StreamIn()))
            ret = cvStreamIn;
        else if (lstring::cieq(tok, Tkw.StreamOut()))
            ret = cvStreamOut;
        else if (lstring::cieq(tok, Tkw.NoDrcDataType()))
            ret = cvNoDrcDataType;
    }
    delete [] tok;
    return (ret);
}


char *
cvtKWstruct::get_settings(const CDl *ld)
{
    sLstr lstr;
    if (ld) {
        if (ld->strmIn()) {
            lstr.add(Tkw.StreamIn());
            lstr.add_c(' ');
            ld->strmIn()->print(0, &lstr);
        }
        for (strm_odata *so = ld->strmOut(); so; so = so->next()) {
            lstr.add(Tkw.StreamOut());
            lstr.add_c(' ');
            lstr.add_i(so->layer());
            lstr.add_c(' ');
            lstr.add_i(so->dtype());
            lstr.add_c('\n');
        }
        if (ld->isNoDRC()) {
            lstr.add(Tkw.NoDrcDataType());
            lstr.add_c(' ');
            lstr.add_i(ld->datatype(CDNODRC_DT));
            lstr.add_c('\n');
        }
    }

    lstr.add_c('\n');
    char *str = lstr.string_trim();
    return (str);
}


namespace {
    int set_line(CDl *ld, const char **line)
    {
        if (!ld)
            return (false);
        char kwbuf[128];
        char inbuf[256];
        *kwbuf = '\0';
        *inbuf = '\0';
        {
            const char *s = *line;
            char *bptr = kwbuf;
            while (isspace(*s))
                s++;
            while (*s && !isspace(*s))
                *bptr++ = *s++;
            *bptr = '\0';
            while (isspace(*s) && *s != '\n')
                s++;
            if (*s && *s != '\n') {   
                bptr = inbuf;
                while (*s && *s != '\n')
                    *bptr++ = *s++;
                *bptr = '\0';
            }
            while (isspace(*s))
                s++;
            *line = s;
            if (!*kwbuf)
                return (false);
            if (!*inbuf)
                return (false);
        }

        if (lstring::cieq(kwbuf, Tkw.StreamIn()))
            return (ld->setStrmIn(inbuf));
        if (lstring::cieq(kwbuf, Tkw.StreamOut())) {
            int lnum = 0;
            int dtyp = 0;
            int k =  sscanf(inbuf, "%d %d", &lnum, &dtyp);
            if (k >= 1 && lnum >= 0 && dtyp >= 0 &&
                    lnum < GDS_MAX_LAYERS && dtyp < GDS_MAX_DTYPES) {
                ld->addStrmOut(lnum, dtyp);
                return (true);
            }
        }
        else if (lstring::cieq(kwbuf, Tkw.StreamData())) {
            int lnum = 0;
            int dtyp = -1;
            int k =  sscanf(inbuf, "%d %d", &lnum, &dtyp);
            if (k >= 1 && lnum >= 0 && dtyp >= -1 &&
                    lnum < GDS_MAX_LAYERS && dtyp < GDS_MAX_DTYPES) {
                ld->addStrmOut(lnum, dtyp >= 0 ? dtyp : 0);
                return (ld->setStrmIn(inbuf));
            }
        }
        else if (lstring::cieq(kwbuf, Tkw.NoDrcDataType())) {
            int i;
            if (sscanf(inbuf, "%d", &i) == 1 && i >= -1 &&
                    i < GDS_MAX_DTYPES) {
                if (i < 0) {
                    ld->setNoDRC(false);
                    ld->setDatatype(CDNODRC_DT, 0);
                }
                else {
                    ld->setNoDRC(true);
                    ld->setDatatype(CDNODRC_DT, i);
                }
                return (true);
            }
        }
        return (false);
    }

    struct cvbak_t
    {
        cvbak_t(CDl *ld)
            {
                strmIn = ld->strmIn();
                ld->setStrmIn((strm_idata*)0);

                strmOut = ld->strmOut();
                ld->setStrmOut(0);

                nodrc = ld->isNoDRC();
                nodrc_dtype = ld->datatype(CDNODRC_DT);
                ld->setNoDRC(false);
                ld->setDatatype(CDNODRC_DT, 0);

                ldesc = ld;
            }

        ~cvbak_t()
            {
                delete strmIn;
                strmOut->free();
            }

        void revert()
            {
                delete ldesc->strmIn();
                ldesc->setStrmIn(strmIn);
                strmIn = 0;

                ldesc->strmOut()->free();
                ldesc->setStrmOut(strmOut);
                strmOut = 0;

                if (nodrc) {
                    ldesc->setNoDRC(true);
                    ldesc->setDatatype(CDNODRC_DT, nodrc_dtype);
                }
                else {
                    ldesc->setNoDRC(false);
                    ldesc->setDatatype(CDNODRC_DT, 0);
                }
            }

    private:
        CDl *ldesc;
        strm_idata *strmIn;
        strm_odata *strmOut;
        int nodrc_dtype;
        bool nodrc;
    };
}


char *
cvtKWstruct::set_settings(CDl *ld, const char *string)
{
    if (!ld)
        return (lstring::copy("No current layer!"));

    // save and clear relevant info
    cvbak_t cvbak(ld);

    if (string) {
        int linecnt = 1;
        const char *str = string;
        while (*str) {
            if (!set_line(ld, &str)) {

                // Failed, revert
                cvbak.revert();

                char buf[256];
                sprintf(buf, "Retry: error on line %d", linecnt);
                return (lstring::copy(buf));
            }
            linecnt++;
        }
    }
    return (0);
}

