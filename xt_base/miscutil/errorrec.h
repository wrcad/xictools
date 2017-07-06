
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id: errorrec.h,v 1.6 2010/10/09 03:46:32 stevew Exp $
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

