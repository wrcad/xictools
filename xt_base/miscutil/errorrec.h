
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef ERRORREC_H
#define ERRORREC_H

#include "lstring.h"
#include "stdlib.h"


inline class ErrRec *Errs();

// Error string recording
//
class ErrRec
{
    static ErrRec *ptr()
        {
            if (!instancePtr) {
                fprintf(stderr,
                    "Singleton class ErrRec used before instantiated.\n");
                exit(1);
            }
            return (instancePtr);
        }

public:
    friend inline ErrRec *Errs() { return (ErrRec::ptr()); }

    ErrRec()
        {
            if (instancePtr) {
                fprintf(stderr,
                    "Singleton class ErrRec already instantiated.\n");
                exit(1);
            }
            instancePtr = this;

            erMsgs = 0;
            erStack = 0;
            lastMsg = 0;
            warnings = 0;
            warnings_flag = false;
        }

    ~ErrRec()
        {
            instancePtr = 0;
            clear();
        }

    void clear()
        {
            stringlist::destroy(erMsgs);
            erMsgs = 0;
            while (erStack) {
                stringlist *s = (stringlist*)erStack->string;
                stringlist::destroy(s);
                erStack->string = 0;
                stringlist *tmp = erStack;
                erStack = erStack->next;
                delete tmp;
            }
            delete [] lastMsg;
            lastMsg = 0;
        }

    void init_error()
        {
            if (erMsgs) {
                delete [] lastMsg;
                lastMsg = getstr();
                stringlist::destroy(erMsgs);
                erMsgs = 0;
            }
        }

    void push_error()
        { erStack = new stringlist((char*)erMsgs, erStack); erMsgs = 0; }

    void pop_error()
        {
            stringlist::destroy(erMsgs);
            erMsgs = 0;
            if (erStack) {
                erMsgs = (stringlist*)erStack->string;
                erStack->string = 0;
                stringlist *tmp = erStack;
                erStack = erStack->next;
                delete tmp;
            }
        }

    bool has_error() { return (erMsgs); }
    const char *get_lasterr() { return (lastMsg); }

    void add_error(const char *fmt, ...);
    void sys_error(const char*);
    void sys_herror(const char*);
    const char *get_error();

    // Maintain a list of warning messages.
    void arm_warnings(bool b) { clear_warnings(); warnings_flag = b; }
    void add_warning(const char *fmt, ...);
    void clear_warnings() { stringlist::destroy(warnings); warnings = 0; }
    const stringlist *get_warnings() { return (warnings); }

private:
    char *getstr();

    stringlist *erMsgs;
    stringlist *erStack;
    char *lastMsg;

    stringlist *warnings;
    bool warnings_flag;

    static ErrRec *instancePtr;
};

#endif

