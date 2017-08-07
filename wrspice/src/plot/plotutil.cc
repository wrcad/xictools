
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

#include "outplot.h"
#include "frontend.h"
#include "cshell.h"
#include "ttyio.h"
#include "miscutil/miscutil.h"

// for threads
#include "circuit.h"


// Misc. graphics functions.

enum eOption
{
    error_option,       // a reply option only
    button_option,      // get a button press
    char_option,        // get a char
    click_option,       // point at a widget
    point_option,       // get a char or button, return coords
    checkup_option      // see if events in queue
};

struct sRequest
{
    sRequest() { option = error_option; fd = socket = -1; };

    eOption option;
    int fd;
    int socket;
};

struct sResponse
{
    sResponse() { option = error_option; x = y = 0; fd = -1; reply.ch = 0; };

    eOption option;
    int x;
    int y;
    int fd;
    union {
      int ch;
      sGraph *graph;
      int button;
    } reply;
};

namespace { void input(sRequest*, sResponse*); }


// Get a characters, returns fd from which it was read.
//
int
SPgraphics::Getchar(int fd, int socket, int *retfd)
{       
    sRequest request;
    sResponse response;
    request.option = char_option;
    request.fd = fd;
    request.socket = socket;
    input(&request, &response);
    if (retfd)
        *retfd = response.fd;
    return (response.reply.ch);
}


bool
SPgraphics::isMainThread()
{
#ifdef WITH_THREADS
    if (!spg_mainThread)
        spg_mainThread = (void*)pthread_self();
    if (!pthread_equal((pthread_t)spg_mainThread, pthread_self()))
        return (false);
#endif
    return (true);
}


// Process the event queue.
//
int
SPgraphics::Checkup()
{
    if (isMainThread()) {
        sRequest request;
        request.option = checkup_option;
        input(&request, 0);
    }
    return (true);
}


// Return the parameters from the next button or key press.
//
void
SPgraphics::ReturnEvent(int *x, int *y, int *key, int *button)
{
    sRequest request;
    sResponse response;
    request.option = point_option;
    input(&request, &response);
    if (x)
        *x = response.x;
    if (y)
        *y = response.y;
    if (response.option == char_option) {
        if (key)
            *key = response.reply.ch;
        if (button)
            *button = 0;
    }
    else {
        if (button)
            *button = response.reply.button;
        if (key)
            *key = 0;
    }
}


// Some error occurred, halt the graphics.
//
void
SPgraphics::HaltFullScreenGraphics()
{
    // halt graphics if not windowed
    if (!GRpkgIf()->CurDev())
        return;
    if (GRpkgIf()->CurDev()->devtype == GRfullScreen && GP.Cur() &&
            (GP.Cur()->apptype() == GR_PLOT ||
            GP.Cur()->apptype() == GR_MPLT)) {
        GP.Cur()->halt();
        int id = GP.Cur()->id();
        PopGraphContext();
        DestroyGraph(id);
    }
}


// Pop up a terminal emulator window.
//
int
SPgraphics::PopUpXterm(const char *string)
{
    if (GRpkgIf()->CurDev() && GRpkgIf()->CurDev()->devtype == GRmultiWindow) {
        int pid = miscutil::fork_terminal(string);
        return (pid > 0 ? 0 : -1);
    }
    return (CP.System(string));
}


namespace {
    // Private input processing function.
    //
    void input(sRequest *request, sResponse *response)
    {
        if (request->option == char_option) {
            int key = EOF;
            int fd = GRpkgIf()->Input(request->fd, request->socket , &key);
            if (key == 3) // ^C
               Sp.SetFlag(FT_INTERRUPT, true);
            if (response) {
                response->option = request->option;
                response->fd = fd;
                response->reply.ch = key;
            }
        }
        else if (request->option == checkup_option) {
            if (GRpkgIf()->CheckForEvents())
                // ^C typed
               Sp.SetFlag(FT_INTERRUPT, true);
        }
        else if (request->option == point_option) {
            if (GP.Cur() && GP.Cur()->dev()) {
                int key, button, x, y;
                GP.Cur()->dev()->Input(&key, &button, &x, &y);
                if (button) {
                    if (response) {
                        response->reply.button = button;
                        response->x = x;
                        response->y = y;
                        response->option = button_option;
                    }
                }
                else if (key) {
                    if (response) {
                        response->reply.ch = key;
                        response->x = x;
                        response->y = y;
                        response->option = char_option;
                    }
                }
            }
        }
        else {
            GRpkgIf()->ErrPrintf(ET_INTERR,
                "Input: unrecognized input type.\n");
            if (response)
                response->option = error_option;
        }
    }
}

