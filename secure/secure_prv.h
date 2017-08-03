
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
 * License server and authentication, and related utilities               *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

// Private include file for the authorization routines.
// All checksums are updated with the checksum of the string found in
// key.h

#ifndef SECURE_PRV_H
#define SECURE_PRV_H

#include <string.h>

// If this is defined, mail will be sent if potential security concerns
// are detected.
//
#define MAIL_ADDR "stevew@wrcad.com"

// Environment variable containing server host name
#define SERVERENV "XTLSERVER"

// File containing server host name
#define SERVERFILE "license.host"

// Host name alias for server
#define SERVERHOST "xtlserver"

// Log file
#define LOG_FILE "xtdaemon.log"

// Name of authentification file created by validate
#define AUTH_FILE "LICENSE"
#define AUTH_FILE_BAK "LICENSE.bak"

// "host" name for class c site access (compiled into xtlserv)
//
#define ANYHOST "any_host_access"

// "host" name for class b site access (compiled into xtlserv)
//
#define ANYHOSTB "any_host_access_b"

#define STRIPHOST(s) {char *t = strchr((s),'.'); if(t) *t='\0';}

// Number of user data blocks in authentication file.  One additional
// block contains the checksum for the first NUMBLKS blocks.
//
#define NUMBLKS 63

// error codes
#define ERR_OK       0
#define ERR_NOLIC    1
#define ERR_NOMEM    2
#define ERR_RDLIC    3
#define ERR_CKSUM    4
#define ERR_NOTLIC   5
#define ERR_TIMEXP   6
#define ERR_USRLIM   7
#define ERR_UNKNO    8
#define ERR_SRVREQ   9
#define ERR_BADREQ   10

namespace secure {

    enum {XTV_NONE, XTV_OPEN, XTV_CHECK, XTV_CLOSE, XTV_DUMP, XTV_KILL};

    struct sJobReq
    {
        sJobReq() { memset(this, 0, sizeof(sJobReq)); }

        char host[64];          // host request is from
        char user[64];          // user for requesting process
        unsigned int date;      // date/time
        unsigned char addr[4];  // IP address of requester
        int reqtype;            // OPEN, CKECK, CLOSE
        unsigned int pid;       // requesting process id
        unsigned code;          // application program code
    };

    // NOTES
    // 1. Code can be 0 - NUMBLKS except for 3 (reserved for server).
    // 2. Reqtype field used for user limit flag when archived by server.
    // 3. Date field updated with server time when archived and after
    //    every CHECK.

    struct sTR
    {
        sTR() { memset(this, 0, sizeof(sTR)); }

        unsigned int  death_date;     // no use after this date (seconds)
        unsigned char valid_addr[4];  // host internet address
        char *valid_host;             // host name
        unsigned char limits[4];      // 0: code, 1: user limit, 2,3: unused
    };

    // used for checksum
#define HOSTNAMLEN 56
    struct block
    {
        block() { memset(this, 0, sizeof(block)); }

        unsigned char addr[4];        // internet address
        unsigned char code[4];        // program code, etc
        char hostname[HOSTNAMLEN];    // host name
    };

    // data block in authentication file
    struct dblk
    {
        dblk() { memset(this, 0, sizeof(dblk)); }

        unsigned int timelim;         // no use after this date (seconds)
        unsigned char limits[4];      // 0: code
                                      // 1: user limit flag
                                      // 2: some user limit
                                      // 3: user limit saved block
        // Just for fun, the user limit is not saved in the same block,
        // rather it is obtained from base[blk->limits[3]]->limits[2].
        unsigned char sum[16];        // checksum of corresponding block
    };
    //
    // limits[0] is the program code.  The max user count for a code,
    // if any, is stored in the code'th block, limits[2] field.

#ifdef __APPLE__
    char *getMacSerialNumber();
#endif
}

using namespace secure;

#endif

