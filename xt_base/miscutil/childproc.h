
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef CHILDPROC_H
#define CHILDPROC_H


// This centralizes the SIGCHLD handling.  Since there are many places
// in the application where child processes are created and monitored,
// we have to be careful that they don't clobber one another.  The
// application should not set handlers for SIGCHLD, but instead use
// the registration mechanism here.

struct sProc *Proc();

typedef void(*SigcHandlerFunc)(int, int, void*);

struct sProc
{
    friend sProc *Proc() { return (p_ptr); }

    // List of active children.
    //
    struct sSigC
    {
        sSigC(int p, SigcHandlerFunc h, void *d, sSigC *n)
            {
                pid = p;
                handler = h;
                data = d;
                next = n;
            }

        int pid;
        void *data;
        SigcHandlerFunc handler;
        sSigC *next;
    };

    sProc();

    void RegisterChildHandler(int, void(*)(int, int, void*), void*);
    void RemoveChildHandler(int, void(*)(int, int, void*));
    void SigchldHandler();

private:

    sSigC *p_siglist;

    static sProc *p_ptr;
};

#endif

