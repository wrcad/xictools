
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef IF_COMMON_H
#define IF_COMMON_H

//
// Some definitions used for the interface fuctions to CD, GEO, etc.
//

// Message types for PrintCvLog().
//
enum OUTmsgType
{
    IFLOG_INFO,         // info message to log
    IFLOG_INFO_SHOW,    // info message to log and screen
    IFLOG_WARN,         // warning to log
    IFLOG_FATAL         // fatal error to log and screen
};

// First argument to InfoMessage().
//
enum INFOmsgType
{
    IFMSG_INFO,         // message to prompt line
    IFMSG_RD_PGRS,      // message to prompt line, read progress
    IFMSG_WR_PGRS,      // message to prompt line, write progress
    IFMSG_CNAME,        // message to prompt line, cell name
    IFMSG_POP_ERR,      // message to pop-up error window
    IFMSG_POP_WARN,     // message to pop-up warning window
    IFMSG_POP_INFO,     // message to pop-up info window
    IFMSG_LOG_ERR,      // message to log, as error
    IFMSG_LOG_WARN      // message to log, as warning
};

#endif

