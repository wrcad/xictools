
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
 * proc2mod -- Create bsim1/bsim2 model from process data.                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
**********/
// convert .process file to set of .model cards

#include <stdio.h>
#include "misc.h"
#include <math.h>

void getdata(double*,int,int);
double evaluate(char**, int*, int);

struct nmod {
    nmod *nnext;
    char *nname;
    double nparms[69];
};

struct pmod {
    pmod *pnext;
    char *pname;
    double pparms[69];
};

struct dmod {
    dmod *dnext;
    char *dname;
    double dparms[10];
};

struct ymod {
    ymod *ynext;
    char *yname;
    double yparms[10];
};

struct mmod {
    mmod *mnext;
    char *mname;
    double mparms[10];
};

FILE *m = 0;
FILE *p = 0;
char *dataline;


// Convert str to lower case.
//
void
str_tolower(char *str)
{ 
    if (str) {
        while (*str) {
            if (isupper(*str))
                *str = tolower(*str);
            str++;
        }
    }
}


int
main() {
    char *typeline;
    char *prname;
    nmod *nlist=0, *ncur;
    pmod *plist=0, *pcur;
    dmod *dlist=0, *dcur;
    ymod *ylist=0, *ycur;
    mmod *mlist=0, *mcur;
    char *filename;

    filename = (char *)malloc(1024);
    typeline = (char *)malloc(1024);
    dataline = (char *)malloc(1024);

    while(p == 0) {
        printf("name of process file (input): ");
        if(scanf("%s",filename)!=1) {
            printf("error reading process file name\n");
            exit(1);
        }
        p = fopen(filename,"r");
        if(p==0) {
            printf("can't open %s:",filename);
            perror("");
        }
    }
    while(m == 0) {
        printf("name of .model file (output): ");
        if(scanf("%s",filename)!=1) {
            printf("error reading model file name\n");
            exit(1);
        }
        m = fopen(filename,"w");
        if(m==0) {
            printf("can't open %s:",filename);
            perror("");
        }
    }
    printf("process name : ");
    if(scanf("%s",filename)!=1) {
        printf("error reading process name\n");
        exit(1);
    }
    prname = filename;
    if(fgets(typeline,1023,p)==0) {
        printf("error reading input description line\n");
        exit(1);
    }
    str_tolower(typeline);
    while(1) {
        while(*typeline == ' ' || *typeline == '\t' || *typeline == ',' ||
                *typeline == '\n' ) {
            typeline ++;
        }
        if(*typeline == '\0') break;
        if(strncmp("nm",typeline,2) == 0) {
            ncur = (nmod *)malloc(sizeof(nmod));
            ncur->nnext = 0;
            ncur->nname = typeline;
            *(typeline+3) = 0;
            typeline += 4;
            getdata(ncur->nparms,69,3);
            ncur->nnext = nlist;
            nlist = ncur;
        } else if(strncmp("pm",typeline,2) == 0) {
            pcur = (pmod *)malloc(sizeof(pmod));
            pcur->pnext = 0;
            pcur->pname = typeline;
            *(typeline+3) = 0;
            typeline += 4;
            getdata(pcur->pparms,69,3);
            pcur->pnext = plist;
            plist = pcur;
        } else if(strncmp("py",typeline,2) == 0) {
            ycur = (ymod *)malloc(sizeof(ymod));
            ycur->ynext = 0;
            ycur->yname = typeline;
            *(typeline+3) = 0;
            typeline += 4;
            getdata(ycur->yparms,10,5);
            ycur->ynext = ylist;
            ylist = ycur;
        } else if(strncmp("du",typeline,2) == 0) {
            dcur = (dmod *)malloc(sizeof(dmod));
            dcur->dnext = 0;
            dcur->dname = typeline;
            *(typeline+3) = 0;
            typeline += 4;
            getdata(dcur->dparms,10,5);
            dcur->dnext = dlist;
            dlist = dcur;
        } else if(strncmp("ml",typeline,2) == 0) {
            mcur = (mmod *)malloc(sizeof(mmod));
            mcur->mnext = 0;
            mcur->mname = typeline;
            *(typeline+3) = 0;
            typeline += 4;
            getdata(mcur->mparms,10,5);
            mcur->mnext = mlist;
            mlist = mcur;
        } else {
            printf(" illegal header line in process file:  run terminated\n");
            printf(" error occurred while parsing %s\n",typeline);
            exit(1);
        }
    }
    for(dcur=dlist;dcur;dcur=dcur->dnext) {
        fprintf(m,".model %s_%s r rsh = %g defw = %g narrow = %g\n",
            prname,dcur->dname,dcur->dparms[0],dcur->dparms[8],dcur->dparms[9]);
        fprintf(m,".model %s_%s c cj = %g cjsw = %g defw = %g narrow = %g\n",
            prname,dcur->dname,dcur->dparms[1],dcur->dparms[2],dcur->dparms[8],
            dcur->dparms[9]);
    }
    for(ycur=ylist;ycur;ycur=ycur->ynext) {
        fprintf(m,".model %s_%s r rsh = %g defw = %g narrow = %g\n",
            prname,ycur->yname,ycur->yparms[0],ycur->yparms[8],ycur->yparms[9]);
        fprintf(m,".model %s_%s c cj = %g cjsw = %g defw = %g narrow = %g\n",
            prname,ycur->yname,ycur->yparms[1],ycur->yparms[2],ycur->yparms[8],
            ycur->yparms[9]);
    }
    for(mcur=mlist;mcur;mcur=mcur->mnext) {
        fprintf(m,".model %s_%s r rsh = %g defw = %g narrow = %g\n",
            prname,mcur->mname,mcur->mparms[0],mcur->mparms[8],mcur->mparms[9]);
        fprintf(m,".model %s_%s c cj = %g cjsw = %g defw = %g narrow = %g\n",
            prname,mcur->mname,mcur->mparms[1],mcur->mparms[2],mcur->mparms[8],
            mcur->mparms[9]);
    }
    for(pcur=plist;pcur;pcur=pcur->pnext) {
        for(dcur=dlist;dcur;dcur=dcur->dnext) {
            fprintf(m,".model %s_%s_%s pmos level=4\n",prname,pcur->pname,
                    dcur->dname);
            fprintf(m,"+ vfb = %g lvfb = %g wvfb = %g\n",
                    pcur->pparms[0],pcur->pparms[1],pcur->pparms[2]);
            fprintf(m,"+ phi = %g lphi = %g wphi = %g\n",
                    pcur->pparms[3],pcur->pparms[4],pcur->pparms[5]);
            fprintf(m,"+ k1 = %g lk1 = %g wk1 = %g\n",
                    pcur->pparms[6],pcur->pparms[7],pcur->pparms[8]);
            fprintf(m,"+ k2 = %g lk2 = %g wk2 = %g\n",
                    pcur->pparms[9],pcur->pparms[10],pcur->pparms[11]);
            fprintf(m,"+ eta = %g leta = %g weta = %g\n",
                    pcur->pparms[12],pcur->pparms[13],pcur->pparms[14]);
            fprintf(m,"+ muz = %g dl = %g dw = %g\n",
                    pcur->pparms[15],pcur->pparms[16],pcur->pparms[17]);
            fprintf(m,"+ u0 = %g lu0 = %g wu0 = %g\n",
                    pcur->pparms[18],pcur->pparms[19],pcur->pparms[20]);
            fprintf(m,"+ u1 = %g lu1 = %g wu1 = %g\n",
                    pcur->pparms[21],pcur->pparms[22],pcur->pparms[23]);
            fprintf(m,"+ x2mz = %g lx2mz = %g wx2mz = %g\n",
                    pcur->pparms[24],pcur->pparms[25],pcur->pparms[26]);
            fprintf(m,"+ x2e = %g lx2e = %g wx2e = %g\n",
                    pcur->pparms[27],pcur->pparms[28],pcur->pparms[29]);
            fprintf(m,"+ x3e = %g lx3e = %g wx3e = %g\n",
                    pcur->pparms[30],pcur->pparms[31],pcur->pparms[32]);
            fprintf(m,"+ x2u0 = %g lx2u0 = %g wx2u0 = %g\n",
                    pcur->pparms[33],pcur->pparms[34],pcur->pparms[35]);
            fprintf(m,"+ x2u1 = %g lx2u1 = %g wx2u1 = %g\n",
                    pcur->pparms[36],pcur->pparms[37],pcur->pparms[38]);
            fprintf(m,"+ mus = %g lmus = %g wmus = %g\n",
                    pcur->pparms[39],pcur->pparms[40],pcur->pparms[41]);
            fprintf(m,"+ x2ms = %g lx2ms = %g wx2ms = %g\n",
                    pcur->pparms[42],pcur->pparms[43],pcur->pparms[44]);
            fprintf(m,"+ x3ms = %g lx3ms = %g wx3ms = %g\n",
                    pcur->pparms[45],pcur->pparms[46],pcur->pparms[47]);
            fprintf(m,"+ x3u1 = %g lx3u1 = %g wx3u1 = %g\n",
                    pcur->pparms[48],pcur->pparms[49],pcur->pparms[50]);
            fprintf(m,"+ tox = %g temp = %g vdd = %g\n",
                    pcur->pparms[51],pcur->pparms[52],pcur->pparms[53]);
            fprintf(m,"+ cgdo = %g cgso = %g cgbo = %g\n",
                    pcur->pparms[54],pcur->pparms[55],pcur->pparms[56]);
            fprintf(m,"+ xpart = %g \n",
                    pcur->pparms[57]);
            fprintf(m,"+ n0 = %g ln0 = %g wn0 = %g\n",
                    pcur->pparms[60],pcur->pparms[61],pcur->pparms[62]);
            fprintf(m,"+ nb = %g lnb = %g wnb = %g\n",
                    pcur->pparms[63],pcur->pparms[64],pcur->pparms[65]);
            fprintf(m,"+ nd = %g lnd = %g wnd = %g\n",
                    pcur->pparms[66],pcur->pparms[67],pcur->pparms[68]);
            fprintf(m,"+ rsh = %g cj = %g cjsw = %g\n",
                dcur->dparms[0], dcur->dparms[1], dcur->dparms[2]);
            fprintf(m,"+ js = %g pb = %g pbsw = %g\n",
                dcur->dparms[3], dcur->dparms[4], dcur->dparms[5]);
            fprintf(m,"+ mj = %g mjsw = %g wdf = %g\n",
                dcur->dparms[6], dcur->dparms[7], dcur->dparms[8]);
            fprintf(m,"+ dell = %g\n",
                dcur->dparms[9]);
        }
    }
    for(ncur=nlist;ncur;ncur=ncur->nnext) {
        for(dcur=dlist;dcur;dcur=dcur->dnext) {
            fprintf(m,".model %s_%s_%s nmos level=4\n",prname,ncur->nname,
                    dcur->dname);
            fprintf(m,"+ vfb = %g lvfb = %g wvfb = %g\n",
                    ncur->nparms[0],ncur->nparms[1],ncur->nparms[2]);
            fprintf(m,"+ phi = %g lphi = %g wphi = %g\n",
                    ncur->nparms[3],ncur->nparms[4],ncur->nparms[5]);
            fprintf(m,"+ k1 = %g lk1 = %g wk1 = %g\n",
                    ncur->nparms[6],ncur->nparms[7],ncur->nparms[8]);
            fprintf(m,"+ k2 = %g lk2 = %g wk2 = %g\n",
                    ncur->nparms[9],ncur->nparms[10],ncur->nparms[11]);
            fprintf(m,"+ eta = %g leta = %g weta = %g\n",
                    ncur->nparms[12],ncur->nparms[13],ncur->nparms[14]);
            fprintf(m,"+ muz = %g dl = %g dw = %g\n",
                    ncur->nparms[15],ncur->nparms[16],ncur->nparms[17]);
            fprintf(m,"+ u0 = %g lu0 = %g wu0 = %g\n",
                    ncur->nparms[18],ncur->nparms[19],ncur->nparms[20]);
            fprintf(m,"+ u1 = %g lu1 = %g wu1 = %g\n",
                    ncur->nparms[21],ncur->nparms[22],ncur->nparms[23]);
            fprintf(m,"+ x2mz = %g lx2mz = %g wx2mz = %g\n",
                    ncur->nparms[24],ncur->nparms[25],ncur->nparms[26]);
            fprintf(m,"+ x2e = %g lx2e = %g wx2e = %g\n",
                    ncur->nparms[27],ncur->nparms[28],ncur->nparms[29]);
            fprintf(m,"+ x3e = %g lx3e = %g wx3e = %g\n",
                    ncur->nparms[30],ncur->nparms[31],ncur->nparms[32]);
            fprintf(m,"+ x2u0 = %g lx2u0 = %g wx2u0 = %g\n",
                    ncur->nparms[33],ncur->nparms[34],ncur->nparms[35]);
            fprintf(m,"+ x2u1 = %g lx2u1 = %g wx2u1 = %g\n",
                    ncur->nparms[36],ncur->nparms[37],ncur->nparms[38]);
            fprintf(m,"+ mus = %g lmus = %g wmus = %g\n",
                    ncur->nparms[39],ncur->nparms[40],ncur->nparms[41]);
            fprintf(m,"+ x2ms = %g lx2ms = %g wx2ms = %g\n",
                    ncur->nparms[42],ncur->nparms[43],ncur->nparms[44]);
            fprintf(m,"+ x3ms = %g lx3ms = %g wx3ms = %g\n",
                    ncur->nparms[45],ncur->nparms[46],ncur->nparms[47]);
            fprintf(m,"+ x3u1 = %g lx3u1 = %g wx3u1 = %g\n",
                    ncur->nparms[48],ncur->nparms[49],ncur->nparms[50]);
            fprintf(m,"+ tox = %g temp = %g vdd = %g\n",
                    ncur->nparms[51],ncur->nparms[52],ncur->nparms[53]);
            fprintf(m,"+ cgdo = %g cgso = %g cgbo = %g\n",
                    ncur->nparms[54],ncur->nparms[55],ncur->nparms[56]);
            fprintf(m,"+ xpart = %g \n",
                    ncur->nparms[57]);
            fprintf(m,"+ n0 = %g ln0 = %g wn0 = %g\n",
                    ncur->nparms[60],ncur->nparms[61],ncur->nparms[62]);
            fprintf(m,"+ nb = %g lnb = %g wnb = %g\n",
                    ncur->nparms[63],ncur->nparms[64],ncur->nparms[65]);
            fprintf(m,"+ nd = %g lnd = %g wnd = %g\n",
                    ncur->nparms[66],ncur->nparms[67],ncur->nparms[68]);
            fprintf(m,"+ rsh = %g cj = %g cjsw = %g\n",
                dcur->dparms[0], dcur->dparms[1], dcur->dparms[2]);
            fprintf(m,"+ js = %g pb = %g pbsw = %g\n",
                dcur->dparms[3], dcur->dparms[4], dcur->dparms[5]);
            fprintf(m,"+ mj = %g mjsw = %g wdf = %g\n",
                dcur->dparms[6], dcur->dparms[7], dcur->dparms[8]);
            fprintf(m,"+ dell = %g\n",
                dcur->dparms[9]);
        }
    }
}


void
getdata(double vals[], int count, int width) 
    //int width; /* maximum number of values to accept per line */
{
    int i;
    int start;

    do {
        if(fgets(dataline,1023,p)==0) {
            printf("premature end of file getting input data line\n");
            exit(1);
        }
        start=0;
    } while (*dataline == '*') ;
    char *c = dataline;
    for(i=0;i<count;i++) {
retry:

        int error;
        vals[i] = evaluate(&c, &error, 1);
        start++;
        if(error || (start>width)) { /* end of line, so read another one */
            do {
                if(fgets(dataline,1023,p)==0) {
                    printf("premature end of file reading input data line \n");
                    exit(1);
                }
                start=0;
            } while (*dataline == '*') ;
            c = dataline;
            goto retry;
        }
    }
}


void
fatal(int x)
{
    exit(x);
}

#define issep(c) ((c)==' '||(c)=='\t'||(c)=='='||(c)=='('||(c)==')'||(c)==',')
 
 
// Get input token from 'line', and return a pointer to it in 'token'.
// int gobble; eat non-whitespace trash AFTER token?
//
int
getTok(char **line, char **token, int gobble)
{
    // scan along throwing away garbage characters
    char *point;
    for (point = *line; *point != '\0'; point++) {
        if (issep(*point)) continue;
        break;
    }
 
    // mark beginning of token
    *line = point;
 
    // now find all good characters
    for (point = *line; *point != '\0'; point++)
        if (issep(*point))
            break;
    *token = new char[1 + point - *line];
    (void) strncpy(*token, *line, point - *line);
    *(*token + (point - *line)) = '\0';
    *line = point;

    // gobble garbage to next token
    for ( ; **line != '\0'; (*line)++) {
        if (**line == ' ') continue;
        if (**line == '\t') continue;
        if ((**line == '=') && gobble) continue;
        if ((**line == ',') && gobble) continue;
        break;
    }
    return (0);
}


double
evaluate(char **line, int *error, int gobble)

// int gobble; /* non-zero to gobble rest of token, zero to leave it alone */
{
    double mantis;
    char *token;
    char *here;
    char *tmpline;
    int expo1;
    int expo2;
    int sign;
    int expsgn;

    /* setup */
    tmpline = *line;
    if (gobble) {
        *error = getTok(line,&token,1);
        if (*error)
            return ((double)0.0);
    }
    else {
        token = *line;
        *error = 0;
    }
    mantis = 0;
    expo1 = 0;
    expo2 = 0;
    sign = 1;
    expsgn = 1;

    /* loop through all of the input token */
    here = token;
    if (*here == '+')
        /* plus, so do nothing except skip it */
        here++;
    if (*here == '-') {
        /* minus, so skip it, and change sign */
        here++;
        sign = -1;
    }
    if  (*here == '\0' || ((!(isdigit(*here))) && (*here != '.'))) {
        /* number looks like just a sign! */
        *error = 1;
        /* back out the 'gettok' operation */
        *line = tmpline;
        if (gobble) {
            delete [] token;
        }
        else {
            *line = here;
        }
        return (0);
    }
    while (isdigit(*here)) {
        /* digit, so accumulate it. */
        mantis = 10*mantis + (*here - '0');
        here++;
    }
    if (*here == '\0') {
        /* reached the end of token - done. */
        if (gobble) {
            delete [] token;
        }
        else {
            *line = here;
        }
        return ((double)mantis*sign);
    }
    if (*here == ':') {
            /* hack for subcircuit node numbering */
            *error = 1;
            return (0.0);
    }
    /* after decimal point! */
    if (*here == '.') {
        /* found a decimal point! */
        here++; /* skip to next character */
        if (*here == '\0') {
            /* number ends in the decimal point */
            if (gobble) {
                delete [] token;
            }
            else {
                *line = here;
            }
            return ((double)mantis*sign);
        }
        while (isdigit(*here)) {
            /* digit, so accumulate it. */
            mantis = 10*mantis + (*here - '0');
            expo1--;
            if (*here == '\0') {
                /* reached the end of token - done. */
                if (gobble) {
                    delete [] token;
                }
                else {
                    *line = here;
                }
                return (mantis*sign*pow(10.0,(double)expo1));
            }
            here++;
        }
    }
    /* now look for "E","e",etc to indicate an exponent */
    if ((*here == 'E') || (*here == 'e') ||
            (*here == 'D') || (*here == 'd')) {
        /* have an exponent, so skip the e */
        here++;
        /* now look for exponent sign */
        if (*here == '+')
            /* just skip + */
            here++;
        if (*here == '-') {
            /* skip over minus sign */
            here++;
            /* and make a negative exponent */
            expsgn = -1;
        }
        /* now look for the digits of the exponent */
        while (isdigit(*here)) {
            expo2 = 10*expo2 + (*here - '0');
            here++;
        }
    }
    /* now we have all of the numeric part of the number, time to
     * look for the scale factor (alphabetic)
     */
    switch(*here) {
        case 't':
        case 'T':
            expo1 += 12;
            here++;
            break;
        case 'g':
        case 'G':
            expo1 += 9;
            here++;
            break;
        case 'k':
        case 'K':
            expo1 += 3;
            here++;
            break;
        case 'u':
        case 'U':
            expo1 -= 6;
            here++;
            break;
        case 'n':
        case 'N':
            expo1 -= 9;
            here++;
            break;
        case 'p':
        case 'P':
            expo1 -= 12;
            here++;
            break;
        case 'f':
        case 'F':
            expo1 -= 15;
            here++;
            break;
        case 'm':
        case 'M':
            /* special case for m - may be m or mil or meg */
            if (*(here+1) != '\0' && *(here+2) != '\0') {
                /* at least 2 characters, so check them. */
                if ((*(here+1) == 'E') || (*(here+1) == 'e')) {
                    if ((*(here+2) == 'G') || (*(here+2) == 'g')) {
                        expo1 += 6;
                        here += 3;
                        if (gobble) {
                            delete [] token;
                        }
                        else {
                            *line = here;
                        }
                        return (sign*mantis*
                            pow(10.0,(double)(expo1 + expsgn*expo2)));
                    }
                }
                else if ((*(here+1) == 'I') || (*(here+1) == 'i')) {
                    if ( (*(here+2) == 'L') || (*(here+2) == 'l')) {
                        expo1 -= 6;
                        here += 3;
                        mantis = mantis*25.4;
                        if (gobble) {
                            delete [] token;
                        }
                        else {
                            *line = here;
                        }
                        return (sign*mantis*
                            pow(10.0,(double)(expo1 + expsgn*expo2)));
                    }
                }
            }
            /* not either special case, so just m => 1e-3 */
            expo1 -= 3;
            here++;
            break;
        default:
            break;
    }
    if (gobble) {
        delete [] token;
    }
    else {
        *line = here;
    }
    return (sign*mantis*pow(10.0,(double)(expo1 + expsgn*expo2)));
}
