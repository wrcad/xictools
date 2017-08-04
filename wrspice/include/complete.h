
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1986 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef COMPLETE_H
#define COMPLETE_H


// command/keyword completion data struct
//
// The data structure for the commands is as follows: every node has a pointer
// to its leftmost child, where the children of a node are those of which
// the node is a prefix. This means that for a word like "ducks", there
// must be nodes "d", "du", "duc", etc (which are all marked "invalid", 
// of course).  This data structure is called a "trie".
//

struct sTrie
{
    sTrie(const char *s)
        {
            cc_name = lstring::copy(s);
            cc_kwords[0] = 0;
            cc_kwords[1] = 0;
            cc_kwords[2] = 0;
            cc_kwords[3] = 0;
            cc_child = 0;
            cc_sibling = 0;
            cc_ysibling = 0;
            cc_parent = 0;
            cc_invalid = false;
        }

    ~sTrie()
        {
            delete cc_child;
            delete cc_sibling;
            delete [] cc_name;
        }

    void free();
    wordlist *match(const char*);
    wordlist *wl(bool);
    sTrie *lookup(const char*, bool, bool);

    unsigned int keyword(int i)     { return (cc_kwords[i]); }
    void set_keyword(int i, unsigned int k) { cc_kwords[i] = k; }

    bool invalid()                  { return (cc_invalid); }
    void set_invalid(bool b)        { cc_invalid = b; }

private:
    const char *cc_name;            // Command or keyword name
    unsigned cc_kwords[4];          // What this command takes
    sTrie *cc_child;                // Left-most child
    sTrie *cc_sibling;              // Right (alph. greater) sibling
    sTrie *cc_ysibling;             // Left (alph. less) sibling
    sTrie *cc_parent;               // Parent node
    bool cc_invalid;                // This node has been deleted
};

// The types for command completion keywords. Note that these constants
// are built into cmdtab.c, so DON'T change them unless you want to
// change all of the bitmasks in cp_coms.
//
#define CT_FILENAME     0
#define CT_COMMANDS     1
#define CT_ALIASES      2
#define CT_RUSEARGS     3
#define CT_OPTARGS      4
#define CT_STOPARGS     5
#define CT_LISTINGARGS  6
#define CT_PLOTKEYWORDS 7
#define CT_VARIABLES    8
#define CT_UDFUNCS      9
#define CT_CKTNAMES     10
#define CT_PLOT         11
#define CT_TYPENAMES    12
#define CT_VECTOR       13
#define CT_DEVNAMES     14
#define CT_MODNAMES     15
#define CT_NODENAMES    16

#endif

