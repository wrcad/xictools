
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
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef HCMSWPDEV_H
#define HCMSWPDEV_H
#ifdef WIN32


// Windows Native hardcopy driver
//

namespace mswinterf {
    extern HCdesc MSPdesc;

    struct MSWdev : public GRdev
    {
    };

    class MSPdev : public MSWdev
    {
    public:
        MSPdev()
            {
                name = "MSP";
                ident = _devMSP_;
                devtype = GRhardcopy;
                printer = 0;
                media = 0;
                data = 0;
            }
        bool Init(int*, char**);
        GRdraw *NewDraw(int);

        friend struct MSPparams;

    private:
        char *printer;
        int media;
        HCdata *data;
    };

    struct MSPparams : public MSWdraw
    {
        MSPparams() { dev = 0; lcx = 0; md_gbag = new sGbagMsw; }
        virtual ~MSPparams() { delete md_gbag; }
        int SwathHeight(int*);
        void ResetViewport(int, int);
        void Halt();

        friend class MSPdev;

    private:
        MSPdev *dev;               // pointer to driver desc
        void *lcx;                 // layer context
    };
}

#endif
#endif

