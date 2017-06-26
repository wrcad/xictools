
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
 $Id: cd_strmdata.cc,v 5.8 2015/06/11 05:54:05 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_strmdata.h"
#include <ctype.h>


//
// Support for the layer/datatype mapping elements of layers.
//


// Print the layer specification either in lstr or to fp.  This is in the
// form  "n n-m ... , n n-m ..." i.e., numbers and ranges for layer and
// datatype, the tho separated by a comma.
//
void
strm_idata::print(FILE *fp, sLstr *lstr)
{
    if (!si_layer_list || !si_dtype_list)
        return;
    if (!fp && !lstr)
        return;

    sLstr tmplstr;
    sLstr *pstr = lstr ? lstr : &tmplstr;
    bool first = true;
    for (idata_list *l = si_layer_list; l; l = l->next) {
        if (first)
            first = false;
        else
            pstr->add_c(' ');
        pstr->add_i(l->min);
        if (l->max != l->min) {
            pstr->add_c('-');
            pstr->add_i(l->max);
        }
    }
    pstr->add_c(',');
    for (idata_list *l = si_dtype_list; l; l = l->next) {
        pstr->add_c(' ');
        pstr->add_i(l->min);
        if (l->max != l->min) {
            pstr->add_c('-');
            pstr->add_i(l->max);
        }
    }
    pstr->add_c('\n');
    if (lstr)
        return;
    fputs(pstr->string(), fp);
}


namespace {
    // Get the next word from str in tok, advance str.
    //
    bool
    get_word(const char **s, char **tok)
    {
        if (s == 0 || *s == 0)
            return (false);
        while (isspace(**s))
            (*s)++;
        if (!**s)
            return (false);
        const char *st = *s;
        while (**s && !isspace(**s) && **s != ',')
            (*s)++;
        if (tok) {
            *tok = new char[*s - st + 1];
            char *c = *tok;
            while (st < *s)
                *c++ = *st++;
            *c = 0;
        }
        while (isspace(**s))
            (*s)++;
        return (true);
    }
}


// Parse the description (as in the print description above) and set
// up the lists.
//
bool
strm_idata::parse_lspec(const char **string)
{
    // Do two passes, to catch errors before modifying anything.
    const char *stbak = *string;
    char *tok;
    while (get_word(string, &tok)) {
        unsigned int i1, i2;
        int k = sscanf(tok, "%u-%u", &i1, &i2);
        delete [] tok;
        if (k == 1) {
            if (i1 >= GDS_MAX_LAYERS)
                return (false);
        }
        else if (k == 2) {
            if (i1 >= GDS_MAX_LAYERS || i2 >= GDS_MAX_LAYERS)
                return (false);
        }
        else
            return (false);
        if (**string == ',')
            break;
    }
    if (**string == ',') {
        (*string)++;
        while (get_word(string, &tok)) {
            unsigned int i1, i2;
            int k = sscanf(tok, "%u-%u", &i1, &i2);
            delete [] tok;
            if (k == 1) {
                if (i1 >= GDS_MAX_DTYPES)
                    return (false);
            }
            else if (k == 2) {
                if (i1 >= GDS_MAX_DTYPES || i2 >= GDS_MAX_DTYPES)
                    return (false);
            }
            else
                return (false);
            if (**string == ',')
                break;
        }
    }
    *string = stbak;

    while (get_word(string, &tok)) {
        unsigned int i1, i2;
        int k = sscanf(tok, "%u-%u", &i1, &i2);
        delete [] tok;
        if (k == 1)
            set_lspec(i1, i1, false);
        else if (k == 2)
            set_lspec(i1, i2, false);
        if (**string == ',')
            break;
    }
    if (**string == ',') {
        (*string)++;
        while (get_word(string, &tok)) {
            unsigned int i1, i2;
            int k = sscanf(tok, "%u-%u", &i1, &i2);
            delete [] tok;
            if (k == 1)
                set_lspec(i1, i1, true);
            else if (k == 2)
                set_lspec(i1, i2, true);
            if (**string == ',')
                break;
        }
    }
    else
        enable_all_dtypes();
    return (true);
}


// Return true if the layer and datatype are in the mapped ranges.
//
bool
strm_idata::check(unsigned int layer, unsigned int dtype)
{
    {
        strm_idata *st = this;
        if (!st)
            return (false);
    }

    if (layer >= GDS_MAX_LAYERS || dtype >= GDS_MAX_DTYPES)
        return (false);

    bool ok = false;
    for (idata_list *l = si_layer_list; l; l = l->next) {
        if (l->min > layer)
            break;
        if (l->min <= layer && layer <= l->max) {
            ok = true;
            break;
        }
    }
    if (!ok)
        return (false);
    ok = false;
    for (idata_list *l = si_dtype_list; l; l = l->next) {
        if (l->min > dtype)
            break;
        if (l->min <= dtype && dtype <= l->max) {
            ok = true;
            break;
        }
    }
    return (ok);
}


// Function to add the range to the layers (dt false) or datatypes
// (dt true).  The ranges are kept in ascending order with no two
// ranges touching.
//
void
strm_idata::set_lspec(unsigned int min, unsigned int max, bool dt)
{
    if (dt) {
        if (min >= GDS_MAX_DTYPES || max >= GDS_MAX_DTYPES)
            return;
    }
    else {
        if (min >= GDS_MAX_LAYERS || max >= GDS_MAX_LAYERS)
            return;
    }
    if (min > max) {
        unsigned int t = min;
        min = max;
        max = t;
    }
    idata_list *list = dt ? si_dtype_list : si_layer_list;
    idata_list *lp = 0, *ln;
    idata_list *lx0 = 0;

    // Remove elements that touch the given range, at the same time
    // extend the range if an endpoint fallis in an existing range. 
    // If the given range is enclosed in a single existing range,
    // just return.

    for (idata_list *l = list; l; l = ln) {
        ln = l->next;
        if (max >= l->min && max <= l->max) {
            if (min >= l->min)
                return;
            max = l->max;
        }
        if (min >= l->min && min <= l->max) {
            if (max <= l->max)
                return;
            min = l->min;
        }
        if (min <= l->min && max >= l->max) {
            if (lp)
                lp->next = ln;
            else
                list = ln;
            l->next = lx0;
            lx0 = l;
            continue;
        }
        lp = l;
    }

    // If any elements were removed, keep only one, and reuse it. 
    // Otherwise create a new range element.

    if (lx0) {
        lx0->next->free();
        lx0->next = 0;
        lx0->min = min;
        lx0->max = max;
    }
    else
        lx0 = new idata_list(min, max);

    // Now insert it in the proper location.  We know that the range
    // does not touch existing ranges.

    if (!list)
        list = lx0;
    else if (lx0->max < list->min) {
        lx0->next = list;
        list = lx0;
    }
    else {
        for (idata_list *l = list; l; l = l->next) {
            if (lx0->min > l->max && (!l->next || lx0->max < l->next->min)) {
                lx0->next = l->next;
                l->next = lx0;
                break;
            }
        }
    }
    if (dt)
        si_dtype_list = list;
    else
        si_layer_list = list;
}
// End of strm_idata functions.


// Convert a token in the form "layer,datatype" to a hex encoding:  if
// layer, datatype are both in range the format is LLDD, otherwise the
// format is LLLLDDDD.  Either field can be '-' which maps to "XX" or
// "XXXX", a wildcard.
//
char *
strmdata::dec_to_hex(const char *dec)
{
    if (!dec)
        return (0);
    const char *p = dec;
    if (p[0] == '-' && p[1] == ',')
        p += 2;
    else {
        while (isdigit(*p))
            p++;
        if (*p != ',' || p == dec)
            return (0);
        p++;
    }
    const char *dtp = p;
    if (p[0] == '-' && !p[1])
        ;
    else {
        while (isdigit(*p))
            p++;
        if (*p || p == dtp)
            return (0);
    }
    int lyr = *dec == '-' ? -1 : atoi(dec);
    int dt = *dtp == '-' ? -1 : atoi(dtp);
    if (lyr >= GDS_MAX_LAYERS || dt >= GDS_MAX_DTYPES)
        return (0);
    char *h = new char[10];
    hexpr(h, lyr, dt);
    return (h);
}


// Convert a four or eight char hex encoding to "layer,datatype"
// format.  Wild card "XX" maps to "-".
//
char *
strmdata::hex_to_dec(const char *hex)
{
    int lyr, dt;
    if (!hextrn(hex, &lyr, &dt))
        return (0);
    char *tbuf = new char[12];
    if (lyr >= 0) {
        char *s = mmItoA(tbuf, lyr);
        *s++ = ',';
        if (dt >= 0)
            mmItoA(s, dt);
        else {
            *s++ = '-';
            *s = 0;
        }
    }
    else {
        tbuf[0] = '-';
        tbuf[1] = ',';
        if (dt >= 0)
            mmItoA(tbuf + 2, dt);
        else {
            tbuf[2] = '-';
            tbuf[3] = 0;
        }
    }
    return (tbuf);
}


// Create layer name.  If both layer and dtype are in range, the
// format is LLDD, otherwise the format is LLLLDDDD.  If either value
// is negative, the corresponding field is all 'X's.
//
void
strmdata::hexpr(char *buf, int layer, int dtype)
{
    const char xchars[] = "0123456789ABCDEF";
    if (layer < GDS_MAX_SPEC_LAYERS && dtype < GDS_MAX_SPEC_DTYPES) {
        if (layer >= 0) {
            buf[0] = xchars[(layer >> 4) & 0xf];
            buf[1] = xchars[layer & 0xf];
        }
        else {
            buf[0] = 'X';
            buf[1] = 'X';
        }
        if (dtype >= 0) {
            buf[2] = xchars[(dtype >> 4) & 0xf];
            buf[3] = xchars[dtype & 0xf];
        }
        else {
            buf[2] = 'X';
            buf[3] = 'X';
        }
        buf[4] = 0;
    }
    else {
        if (layer >= 0) {
            buf[0] = xchars[(layer >> 12) & 0xf];
            buf[1] = xchars[(layer >> 8) & 0xf];
            buf[2] = xchars[(layer >> 4) & 0xf];
            buf[3] = xchars[layer & 0xf];
        }
        else {
            buf[0] = 'X';
            buf[1] = 'X';
            buf[2] = 'X';
            buf[3] = 'X';
        }
        if (dtype >= 0) {
            buf[4] = xchars[(dtype >> 12) & 0xf];
            buf[5] = xchars[(dtype >> 8) & 0xf];
            buf[6] = xchars[(dtype >> 4) & 0xf];
            buf[7] = xchars[dtype & 0xf];
        }
        else {
            buf[4] = 'X';
            buf[5] = 'X';
            buf[6] = 'X';
            buf[7] = 'X';
        }
        buf[8] = 0;
    }
}


// Return true and fill in the values if hex is in the form LLDD or
// LLLLDDDD, where L,D are hex values or the entire L or D field is
// 'X' or 'x' (wildcard) which translates to -1.
//
bool
strmdata::hextrn(const char *hex, int *layer, int *dtype)
{
    if (!hex)
        return (false);
    if (strlen(hex) == 4) {
        if (isxdigit(hex[0]) && isxdigit(hex[1]))
            *layer = hexval(hex[1]) + 16*hexval(hex[0]);
        else if ((hex[0] == 'X' || hex[1] == 'X') ||
                (hex[0] == 'x' && hex[1] == 'x'))
            *layer = -1;
        else
            return (false);
        if (isxdigit(hex[2]) && isxdigit(hex[3]))
            *dtype = hexval(hex[3]) + 16*hexval(hex[2]);
        else if ((hex[2] == 'X' || hex[3] == 'X') ||
                (hex[2] == 'x' && hex[3] == 'x'))
            *dtype = -1;
        else
            return (false);
        return (true);
    }
    if (strlen(hex) == 8) {
        if (isxdigit(hex[0]) && isxdigit(hex[1]) &&
                isxdigit(hex[2]) && isxdigit(hex[3]))
            *layer = hexval(hex[3]) + 16*hexval(hex[2]) +
                256*hexval(hex[1]) + 4096*hexval(hex[0]);
        else if ((hex[0] == 'X' && hex[1] == 'X' &&
                hex[2] == 'X' && hex[3] == 'X') ||
                (hex[0] == 'x' && hex[1] == 'x' &&
                hex[2] == 'x' && hex[3] == 'x'))
            *layer = -1;
        else
            return (false);
        if (isxdigit(hex[4]) && isxdigit(hex[5]) &&
                isxdigit(hex[6]) && isxdigit(hex[7]))
            *dtype = hexval(hex[7]) + 16*hexval(hex[6]) +
                256*hexval(hex[5]) + 4096*hexval(hex[4]);
        else if ((hex[4] == 'X' && hex[5] == 'X' &&
                hex[6] == 'X' && hex[7] == 'X') ||
                (hex[4] == 'x' && hex[5] == 'x' &&
                hex[6] == 'x' && hex[7] == 'x'))
            *dtype = -1;
        else
            return (false);
        return (true);
    }
    return (false);
}

