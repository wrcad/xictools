
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
 $Id: if_common.h,v 5.4 2009/10/08 06:05:44 stevew Exp $
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

