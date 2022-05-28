
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
 * License server and authentication, and related utilities               *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef SECURE_H
#define SECURE_H

#include <stdio.h>

//  Exports and defines for security system.

// Program codes
#define SERVER_CODE         3
#define OA_CODE             5
#define XIV_CODE            7
#define XICII_CODE          9
#define XIC_CODE            11
#define XIC_DAEMON_CODE     12
#define WRSPICE_CODE        14

// Interval between XTV_CHECK requests from application.
#define AC_CHECK_TIME_MSEC  300000

// Time to live after XTV_CHECK failure.
#define AC_LIFETIME_MINUTES 10

namespace secure {
    struct sJobReq;
}
struct sockaddr_in;
struct protoent;

// This should be instantiated within the application.  If local is true,
// validation is internal, i.e., no license server.
//
struct sAuthChk
{
    // secure.cc
    sAuthChk(bool);
    ~sAuthChk()
        {
            delete [] ac_license_host;
            delete [] ac_validation_host;
            delete [] ac_working_ip;
            delete [] ac_working_alt;
        }

    void closeValidation()  { validate(0, 0); }

    void set_validation_host(const char*);
    int validate(int, const char*, const char* = 0);
    char *periodicTest(unsigned long);
    bool serverCmd(const char*, int, const char* = 0, int = 0);

    static void decode(const unsigned char*, char*);
    static void print_error(int);

private:

    // secure_int.cc
    int validate_int(int, const char*);

    char *periodicTestCore();
    bool validation(int, int, const char*, int*);
    int open_srv(sockaddr_in&, protoent*);
    bool write(secure::sJobReq&, int, const char*, const char*, const char*,
        int, int);
    bool check(secure::sJobReq&, int, char*, const char*, const char*, int,
        int*);
    char *get_license_host(const char*);
    bool read_block(int, void*, size_t);
    bool read_dump(int);
    bool write_block(int, void*, size_t);
    bool fill_req(secure::sJobReq*, const char*, const char*, const char*);
    int unwind(secure::sJobReq*, secure::sJobReq*);

    char *ac_license_host;      // license server host
    char *ac_validation_host;   // user-supplied license server host
    const char *ac_job_host;    // override host name
    char *ac_working_ip;        // validation IP adress
    char *ac_working_alt;       // alternate key
    int ac_validation_port;     // user-supplied port number for server
    int ac_open_pid;            // opening pid, may change if fork so save
    bool ac_local;              // local auth if set
    bool ac_connected;          // connected to the license server once
    bool ac_armed;              // periodic test flag
    unsigned long ac_lastchk;   // periodic test last time
};

#endif

