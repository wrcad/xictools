
/*========================================================================*
 *                                                                        *
 *  Copyright (c) 2016 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
 *  Author:  Stephen R. Whiteley (stevew@wrcad.com)
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Back end for forms processing.
 *                                                                        *
 *========================================================================*
 $Id: backend.h,v 1.3 2016/01/18 18:52:12 stevew Exp $
 *========================================================================*/

#ifndef BACKEND_H
#define BACKEND_H

#include "sitedefs.h"

// Supported operating systems.
enum OS_type { OS_NONE, OS_MSW, OS_LINUX, OS_OSX };

// Class used to service forms handling, with interface to database,
// email, etc.
//
struct sBackEnd
{
    struct keyval
    {
        char *key;
        char *val;
    };

    sBackEnd(OS_type t)
        {
            be_hostname = 0;
            be_keytext = 0;
            be_email = 0;
            be_info = 0;
            be_key = 0;
            be_errmsg = 0;
            be_ostype = t;
            be_demousers = -1;
            be_xivusers = -1;
            be_xiciiusers = -1;
            be_xicusers = -1;
            be_wrsusers = -1;
            be_xtusers = -1;
            be_months = 0;
            be_time = 0;
        }

    ~sBackEnd()
        {
            delete [] be_hostname;
            delete [] be_keytext;
            delete [] be_email;
            delete [] be_info;
            delete [] be_key;
        }

    static bool get_keyvals(keyval**, int*, const char*);
    static void errordump(const char*);

    bool set_hostname(const char*);
    bool set_keytext(const char*);
    bool set_email(const char*);
    bool set_info(const char*);

    const char *get_key();
    bool set_key(const char*);
    bool save_key_data();
    bool recall_hostname();
    bool recall_mtype();
    bool recall_keytext();
    bool recall_email();
    bool recall_info();
    char *key_datafile();

    static bool key_exists(const char*);

    bool set_demousers(const char*);
    bool set_xivusers(const char*);
    bool set_xiciiusers(const char*);
    bool set_xicusers(const char*);
    bool set_wrsusers(const char*);
    bool set_xtusers(const char*);
    bool set_months(const char*);
    bool check_order();
    bool record_order(int);
    bool send_email_order_wr();
    bool send_email_order_user();
    bool record_demo();
    bool send_email_demo_wr();
    bool send_email_demo_user();
    bool record_acc_order(int);
    bool send_email_acc_order_wr();
    bool send_email_acc_order_user();

    const char *osname()
        {
            switch (be_ostype) {
            case OS_NONE:
                return ("");
            case OS_MSW:
                return ("msw");
            case OS_LINUX:
                return ("linux");
            case OS_OSX:
                return ("osx");
            }
        }

    const char *hostname()      const { return (be_hostname); }
    const char *keytext()       const { return (be_keytext); }
    const char *email()         const { return (be_email); }
    const char *info()          const { return (be_info); }
    const char *key()           const { return (be_key); }
    const char *error_msg()     const { return (be_errmsg); }

    int demousers()             const { return (be_demousers); }
    int xivusers()              const { return (be_xivusers); }
    int xiciiusers()            const { return (be_xiciiusers); }
    int xicusers()              const { return (be_xicusers); }
    int wrsusers()              const { return (be_wrsusers); }
    int xtusers()               const { return (be_xtusers); }
    int months()                const { return (be_months); }

private:
    char *be_hostname;          // licensed host name
    char *be_keytext;           // raw keying text
    char *be_email;             // account email address
    char *be_info;              // account contact info
    char *be_key;               // license key
    const char *be_errmsg;      // message, if error
    OS_type be_ostype;          // license operating system
    int be_demousers;           // demo flag, 0 for host-locked. 1 floating
    int be_xivusers;            // number of Xiv users, 0 for host-locked.
    int be_xiciiusers;          // ditto for XicII
    int be_xicusers;            // ditto for Xic
    int be_wrsusers;            // ditto for WRspice
    int be_xtusers;             // ditto for Xic/WRspice bundled
    int be_months;              // duration of license
    time_t be_time;             // reference time of order
};

#endif

