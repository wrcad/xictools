
/*========================================================================*
 *                                                                        *
 *  Copyright (c) 2016 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
 *  Author:  Stephen R. Whiteley (stevew@wrcad.com)
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * License order form handling
 *                                                                        *
 *========================================================================*
 $Id: licform.cc,v 1.1 2016/01/15 19:45:21 stevew Exp $
 *========================================================================*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "backend.h"

//
// This handles the license order form.
//

int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;

    sBackEnd::keyval *kval;
    int nkv;
    if (!sBackEnd::get_keyvals(&kval, &nkv, argv[1])) {
        sBackEnd::errordump("Parse failed!");
        return (1);
    }

    // The keys:
    // key
    // demousers
    // xivusers
    // xiciiusers
    // xicusers
    // wrsusers
    // xtusers
    // months
    // total

    sBackEnd be(OS_NONE);

    int ototal = 0;
    for (int i = 0; i < nkv; i++) {
        if (!strcmp(kval[i].key, "key")) {
            if (!be.set_key(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
        else if (!strcmp(kval[i].key, "demousers")) {
            if (!be.set_demousers(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
        else if (!strcmp(kval[i].key, "xivusers")) {
            if (!be.set_xivusers(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
        else if (!strcmp(kval[i].key, "xiciiusers")) {
            if (!be.set_xiciiusers(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
        else if (!strcmp(kval[i].key, "xicusers")) {
            if (!be.set_xicusers(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
        else if (!strcmp(kval[i].key, "wrsusers")) {
            if (!be.set_wrsusers(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
        else if (!strcmp(kval[i].key, "xtusers")) {
            if (!be.set_xtusers(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
        else if (!strcmp(kval[i].key, "months")) {
            if (!be.set_months(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
        else if (!strcmp(kval[i].key, "total")) {
            ototal = atoi(kval[i].val);
        }
    }

    if (!be.check_order()) {
        sBackEnd::errordump(be.error_msg());
        return (1);
    }

    int total = 0;
    if (be.demousers() < 0) {
        int u = be.xivusers() == 0 ? 1 : be.xivusers();
        if (u > 0)
            total += u*XIVPM;
        u = be.xiciiusers() == 0 ? 1 : be.xiciiusers();
        if (u > 0)
            total += u*XICIIPM;
        u = be.xicusers() == 0 ? 1 : be.xicusers();
        if (u > 0)
            total += u*XICPM;
        u = be.wrsusers() == 0 ? 1 : be.wrsusers();;
        if (u > 0)
            total += u*WRSPM;
        u = be.xtusers() == 0 ? 1 : be.xtusers();
        if (u > 0)
            total += u*XTPM;
        total *= be.months();
        if (total <= 0) {
            sBackEnd::errordump("No purchase indicated");
            return (1);
        }

        if (ototal && ototal != total) {
            sBackEnd::errordump("Uh-oh, price consistency failure");
            return (1);
        }
    }

    // Compose summary page.

    printf("<body background=/images/tmbg.gif text=#000000 link=#9c009e" 
        " vlink=#551a8b alink=#ff0000>\n");
    printf("<br><br><br><br><br><br>\n");
    printf("<center><table border=1 cellpadding=12><tr><td bgcolor=white>\n");

    printf("<table border=0 cellpadding=4>\n");

    if (be.demousers() >= 0) {
        // User requested a demo.

        printf("<tr><th>Product</th> <th>Duration<br>(months)</th> "
            "<th>Floating seats<br>or fixed</th></tr>\n");
        if (be.demousers() == 0) {
            printf("<tr><th>DEMO</th> <td align=center>1</td> "
                "<td align=center>fixed<br>unlimited users</td></tr>\n");
        }
        else {
            printf("<tr><th>DEMO</th> <td align=center>1</td> "
                "<td align=center>floating<br>1 user</td></tr>\n");
        }
        printf("</table>\n");

        if (!ototal) {
            // Initial call, from form submit button.  Display a
            // confirmation page.  If the user confirms, this program
            // will be called again, with the "total" field nonzero.

            printf("<p>If not correct, use your browser's <b>Back</b> button to "
                "return to previous page.\n");
            printf("<p><a href=/cgi-bin/orderform.cgi?%s&total=%d><b>Click "
                "Here</b></a> to send demo request to Whiteley Research.\n",
                argv[1], 1);
            printf("</td></tr></table></center>\n");
            return (0);
        }

        // Get here on the second call, from the confirmation page. 
        // Record the demo record (only one demo per key) and send
        // email to user and Whiteley Research.

        if (!be.record_demo()) {
            printf("<p><font color=red><b>Uh-oh, something didn't work, %s.<br>"
                "Order not recorded, please contact Whiteley Research."
                "</b></font>\n", be.error_msg());
            printf("</td></tr></table></center>\n");
            return (0);
        }
        if (!be.send_email_demo_wr()) {
            printf("<p><font color=red><b>Uh-oh, something didn't work, %s.<br>"
                "Order not sent, please contact Whiteley Research."
                "</b></font>\n", be.error_msg());
            printf("</td></tr></table></center>\n");
            return (0);
        }
        if (!be.send_email_demo_user()) {
            printf("<p><font color=red><b>Uh-oh, something didn't work, %s.<br>"
                "Order not confirmed, please contact Whiteley Research %s."
                "</b></font>\n", be.error_msg(), be.email());
                //XXX
            printf("</td></tr></table></center>\n");
            return (0);
        }
        printf("<p><font color=blue><b>Request has been sent to Whiteley "
            "Research, and a<br>confirmation email has been sent to "
            "<tt>%s</tt>.</b></font>\n", be.email());
        printf("</td></tr></table></center>\n");
        return (0);
    }

    // Get here when not requesting a demo.

    printf("<tr><th>Product</th> <th>Base</th> <th>Duration<br>(months)</th> "
        "<th>Floating seats<br>or fixed</th> <th>subtotal</th></tr>\n");
    if (be.xivusers() == 0) {
        printf("<tr><th>XIV</th> <td>$%d</t> <td align=center>%d</td> "
            "<td align=center>fixed<br>unlimited users</td> <td>$%d</td></tr>\n",
            XIVPM, be.months(), XIVPM*be.months());
    }
    else if (be.xivusers() > 0) {
        printf("<tr><th>XIV</th> <td>$%d</t> <td align=center>%d</td> "
            "<td align=center>floating<br>%d users</td> <td>$%d</td></tr>\n",
            XIVPM, be.months(), be.xivusers(), XIVPM*be.xivusers()*be.months());
    }

    if (be.xiciiusers() == 0) {
        printf("<tr><th>XICII</th> <td>$%d</t> <td align=center>%d</td> "
            "<td align=center>fixed<br>unlimited users</td> <td>$%d</td></tr>\n",
            XICIIPM, be.months(), XICIIPM*be.months());
    }
    else if (be.xiciiusers() > 0) {
        printf("<tr><th>XICII</th> <td>$%d</t> <td align=center>%d</td> "
            "<td align=center>floating<br>%d users</td> <td>$%d</td></tr>\n",
            XICIIPM, be.months(), be.xiciiusers(),
            XICIIPM*be.xiciiusers()*be.months());
    }

    if (be.xicusers() == 0) {
        printf("<tr><th>XIC</th> <td>$%d</t> <td align=center>%d</td> "
            "<td align=center>fixed<br>unlimited users</td> <td>$%d</td></tr>\n",
            XICPM, be.months(), XICPM*be.months());
    }
    else if (be.xicusers() > 0) {
        printf("<tr><th>XIC</th> <td>$%d</t> <td align=center>%d</td> "
            "<td align=center>floating<br>%d users</td> <td>$%d</td></tr>\n",
            XICPM, be.months(), be.xicusers(), XICPM*be.xicusers()*be.months());
    }

    if (be.wrsusers() == 0) {
        printf("<tr><th>WRS</th> <td>$%d</t> <td align=center>%d</td> "
            "<td align=center>fixed<br>unlimited users</td> <td>$%d</td></tr>\n",
            WRSPM, be.months(), WRSPM*be.months());
    }
    else if (be.wrsusers() > 0) {
        printf("<tr><th>WRS</th> <td>$%d</t> <td align=center>%d</td> "
            "<td align=center>floating<br>%d users</td> <td>$%d</td></tr>\n",
            WRSPM, be.months(), be.wrsusers(), WRSPM*be.wrsusers()*be.months());
    }

    if (be.xtusers() == 0) {
        printf("<tr><th>XT</th> <td>$%d</t> <td align=center>%d</td> "
            "<td align=center>fixed<br>unlimited users</td> <td>$%d</td></tr>\n",
            XTPM, be.months(), XTPM*be.months());
    }
    else if (be.xtusers() > 0) {
        printf("<tr><th>XT</th> <td>$%d</t> <td align=center>%d</td> "
            "<td align=center>floating<br>%d users</td> <td>$%d</td></tr>\n",
            XTPM, be.months(), be.xtusers(), XTPM*be.xtusers()*be.months());
    }
    printf("<tr><td colspan=4><b>TOTAL</b></td> <td><b>$%d</b></td></tr>\n",
        total);
    printf("</table>\n");

    if (!ototal) {
        // Initial call, from form submit button.  Display a
        // confirmation page.  If the user confirms, this program will
        // be called again, with the "total" field nonzero.

        printf("<p>If not correct, use your browser's <b>Back</b> button to "
            "return to previous page.\n");
        printf("<p><a href=/cgi-bin/orderform.cgi?%s&total=%d><b>Click "
            "Here</b></a> to send order to Whiteley Research.\n",
            argv[1], total);
        printf("</td></tr></table></center>\n");
        return (0);
    }

    // Get here on the second call, from the confirmation page.

    if (!be.record_order(total)) {
        printf("<p><font color=red><b>Uh-oh, something didn't work, %s.<br>"
            "Order not recorded, please contact Whiteley Research."
            "</b></font>\n", be.error_msg());
        printf("</td></tr></table></center>\n");
        return (0);
    }
    if (!be.send_email_order_wr()) {
        printf("<p><font color=red><b>Uh-oh, something didn't work, %s.<br>"
            "Order not sent, please contact Whiteley Research."
            "</b></font>\n", be.error_msg());
        printf("</td></tr></table></center>\n");
        return (0);
    }
    if (!be.send_email_order_user()) {
        printf("<p><font color=red><b>Uh-oh, something didn't work, %s.<br>"
            "Order not confirmed, please contact Whiteley Research %s."
            "</b></font>\n", be.error_msg(), be.email());
        printf("</td></tr></table></center>\n");
        return (0);
    }
    printf("<p><font color=blue><b>Order has been sent to Whiteley "
        "Research, and a<br>confirmation email has been sent to "
        "<tt>%s</tt>.</b></font>\n", be.email());
    printf("</td></tr><tr><td bgcolor=#ffeeee><center>\n");
    printf("<font size=5 face=utopia>Payment by Credit Card</font><br>\n");
    printf("<img src=/images/ccards00.gif><br>\n");
    printf("<font color=#1c00cf size=4 face=utopia>Whiteley Research Inc.</font><br>\n");
    printf("<font face=utopia>456 Flora Vista Avenue</font><br>\n");
    printf("<font face=utopia>Sunnyvale, CA 94086 USA</font><br>\n");
    printf("<b>(408) 735-8973, (408) 245-4033 (Fax)</b><br>\n");
    printf("</center><p><ul>\n");
    printf("<li>Enter your <b>billing</b> address in the form requesting "
        "your shipping address.\n");
    printf("<p><li>You will receive notification via email when the "
        "transaction is complete.\n");
    printf("<p><li>Your credit card will be billed to <a "
        "href=http://www.2checkout.com>2Checkout</a> and <i>not</i> Whiteley"
        " Research Inc.\n");
    printf("</ul><p><center>\n");

    printf("<form method=post action=https://www.2checkout.com/2co/buyer/purchase>\n");
    printf("<input type=hidden name=sid value=36902>\n");
    printf("<input type=hidden name=cart_order_id value=1000>\n");
    printf("<input type=hidden name=total value=%d> \n", total);
    // printf("<input type=hidden name=demo value=Y>\n");
    printf("<input type=submit value=\"Pay Now\">\n");
    printf("</form></center>\n");

    printf("</td></tr></table></center>\n");
    return (0);
}

