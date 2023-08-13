


enum RVTtype { RVTauto, RVTnone, RVTrhel6, RVTrhel7, RVTmac, RVTmsw };
namespace {
    RVTtype RevertMode; 
}

// We don't want the toolbar or plot windows to take focus from the
// console when it pops up, but these windows do have
// keyboard sensitivity.  The gtk_window_set_focus_on_map function
// seems like just the ticket, unfortunately it is only a hint to the
// window manager, and is ignored in some cases.  This is a huge
// headache since there a basically no commonality between window
// managers, and the behavior hopefully set here can be extremely
// annoying if not working right.  Basically, we want new windows to
// not take focus when popped up, leaving the focus where it is. 
// However, the windows are capable of focus and can be selected
// explicitly by the user to accept focus.  A new window should appear
// on top, but appear beneath the console if the console is clicked
// in.
//
// There are several different modes here depending on different
// window systems.  By default, one is selected as a best guess, but
// the user can set a variable to override.

namespace {
    int set_accept_focus(void *arg)
    {
        if (RevertMode == RVTmac) {
            // If launched from a Terminal window, the Console will be 0
            // since the terminal is Cocoa, not X (regular xterms work
            // fine).  The following AppleScript will revert focus to the
            // Cocoa terminal.
#ifdef __APPLE__
#ifdef WITH_X11
            if (QTdev::self()->ConsoleXid() == 0)
#endif
                system(
            "osascript -e \"tell application \\\"Terminal\\\" to activate\"");
#endif
            gtk_window_set_keep_above(GTK_WINDOW(arg), false);
        }
        else if (RevertMode == RVTrhel7)
            gtk_window_set_keep_above(GTK_WINDOW(arg), false);
        else if (RevertMode == RVTmsw) {
            gtk_window_set_keep_above(GTK_WINDOW(arg), false);
            gtk_window_set_accept_focus(GTK_WINDOW(arg), true);
        }
#ifdef WITH_X11
        // This is probably crap.
        if (QTdev::self()->ConsoleXid() &&
                Sp.GetVar("wmfocusfix", VTYP_BOOL, 0)) {
            XSetInputFocus(gdk_x11_get_default_xdisplay(),
                QTdev::self()->ConsoleXid(), RevertToPointerRoot,
                CurrentTime);
        }
#endif
        return (false);
    }

    // Expose handler, remove handler and set idle proc
    //
    int revert_proc(GtkWidget *widget, GdkEvent*, void*)
    {
        g_signal_handlers_disconnect_by_func(G_OBJECT(widget),
            (gpointer)revert_proc, widget);
        // Use a timeout rather than an idle, KDE seems to need the delay.
        g_timeout_add(800, set_accept_focus, widget);
        // g_idle_add(set_accept_focus, widget);
        return (0);
    }
}


// Call this on a pop-up shell to cause the focus to revert to the console
// when the pop-up first appears.
//
void
QTtoolbar::RevertFocus(GtkWidget *widget)
{
    VTvalue vv;
    if (Sp.GetVar(kw_revertmode, VTYP_NUM, &vv))
        RevertMode = (RVTtype)vv.get_int();
    if (RevertMode == RVTauto) {
#ifdef __linux__
        RevertMode = RVTrhel7;
#else
#ifdef ___APPLE__
        RevertMode = RVTmac;
#else
#ifdef WIN32
        RevertMode = RVTmsw;
#endif
#endif
#endif
    }

    if (RevertMode == RVTrhel7) {
        // RHEL 7 or newer, uses KDE.  Without the set_keep_above, new
        // plot windows are created below the console, which stinks.

        if (!Sp.GetVar("nototop", VTYP_BOOL, 0)) {
            gtk_window_set_urgency_hint(GTK_WINDOW(widget), true);
            gtk_window_set_keep_above(GTK_WINDOW(widget), true);
        }
        gtk_window_set_focus_on_map(GTK_WINDOW(widget), false);

        // Start with sensitivity off, and use a timer to turn it back
        // on, presumably well after mapping.
        g_signal_connect(G_OBJECT(widget), "expose_event",
            G_CALLBACK(revert_proc), widget);
    }
    else if (RevertMode == RVTrhel6) {
        // RHEL 6 or older, uses Gnome.  The set_keep_above ends up
        // causing the plot windows to disappear beneath other windows
        // after the short timer delay, which is unacceptable.  Below
        // keeps the focus in the console, but the new window may or
        // may not be on top.  This is Gnome behavior and there seems
        // to be no way to fix it.

        gtk_window_set_urgency_hint(GTK_WINDOW(widget), true);
        gtk_window_set_focus_on_map(GTK_WINDOW(widget), false);
    }
    else if (RevertMode == RVTmac) {
        if (!Sp.GetVar("nototop", VTYP_BOOL, 0)) {
            gtk_window_set_urgency_hint(GTK_WINDOW(widget), true);
            gtk_window_set_keep_above(GTK_WINDOW(widget), true);
        }
        gtk_window_set_focus_on_map(GTK_WINDOW(widget), false);
        g_signal_connect(G_OBJECT(widget), "expose_event",
            G_CALLBACK(revert_proc), widget);
    }
    else if (RevertMode == RVTmsw) {
        if (!Sp.GetVar("nototop", VTYP_BOOL, 0)) {
            gtk_window_set_urgency_hint(GTK_WINDOW(widget), true);
            gtk_window_set_keep_above(GTK_WINDOW(widget), true);
        }
        gtk_window_set_focus_on_map(GTK_WINDOW(widget), false);
        gtk_window_set_accept_focus(GTK_WINDOW(widget), false);
        g_signal_connect(G_OBJECT(widget), "expose_event",
            G_CALLBACK(revert_proc), widget);
    }
}

