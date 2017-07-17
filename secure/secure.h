
/*========================================================================*
 *                                                                        *
 *  XicTools Integrated Circuit Design System                             *
 *  Copyright (c) 2008 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *========================================================================*
 *                                                                        *
 * Interface Functions                                                    *
 *                                                                        *
 *========================================================================*
 $Id: secure.h,v 2.19 2015/11/19 04:48:47 stevew Exp $
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

    void set_validation_host(const char*);
    int validate(int, const char*, const char* = 0);
    void closeValidation();
    bool serverCmd(const char*, int, const char* = 0, int = 0);

    static void decode(const unsigned char*, char*);
    static void print_error(int);

    // secure_int.cc
    int validate_int(int, const char*);

    // This should be called periodically by the application, with the
    // elapsed time in milliseconds as the argument.  Every
    // CHECK_TIME_MSEC we see if the server is still alive.  If not,
    // return a warning string.
    //
    // Warning:  assume that this is called from a SIGALRM handler.
    //
    char *periodicTest(unsigned long msec)
        {
            if (!ac_local && msec - ac_lastchk > AC_CHECK_TIME_MSEC) {
                ac_lastchk = msec;
                if (!ac_armed)
                    return (periodicTestCore());
            }
            return (0);
        }

private:
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

