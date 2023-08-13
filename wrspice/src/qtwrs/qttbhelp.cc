
//========================================================================
// Pop up to display keyword help lists.  Clicking on the list entries
// calls the main help system.  This is called from the dialogs which
// contain lists of 'set' variables to modify.
//

struct sTBhelp
{
    friend void QTtoolbar::PopDownTBhelp(TBH_type);

    sTBhelp(GRobject, GRobject);
    // No destructor, this struct should not be destroyed explicitly.

    QWidget *show(TBH_type);

private:
    void select(QWidget*, int, int);

    static void th_cancel_proc(QWidget*, void*);
    static int th_btn_hdlr(QWidget*, QEvent*, void*);
    static void th_destroy(void*);

    QWidget *th_popup;
    QWidget *th_text;

    int th_lx;
    int th_ly;
};


void
QTtoolbar::PopUpTBhelp(GRobject parent, GRobject call_btn, TBH_type type)
{
    if (tb_kw_help[type])
        return;
    sTBhelp *th = new sTBhelp(parent, call_btn);
    tb_kw_help[type] = th->show(type);;
//    g_object_set_data(G_OBJECT(tb_kw_help[type]), "tbtype",
//        (void*)(long)type);
}


void
QTtoolbar::PopDownTBhelp(TBH_type type)
{
    if (!tb_kw_help[type])
        return;
/*
    g_signal_handlers_disconnect_by_func(G_OBJECT(tb_kw_help[type]),
        (gpointer)sTBhelp::th_cancel_proc, (gpointer)tb_kw_help[type]);
    gdk_window_get_root_origin(gtk_widget_get_window(tb_kw_help[type]),
        &tb_kw_help_pos[type].x, &tb_kw_help_pos[type].y);
    gtk_widget_destroy(GTK_WIDGET(tb_kw_help[type]));
*/
    tb_kw_help[type] = 0;
}


sTBhelp::sTBhelp(GRobject parent, GRobject call_btn)
{
    /*
    th_popup = gtk_NewPopup(0, "Keyword Help", th_cancel_proc, 0);
    th_text = 0;
    th_lx = 0;
    th_ly = 0;

    g_object_set_data_full(G_OBJECT(th_popup), "tbhelp", this, th_destroy);
    g_object_set_data(G_OBJECT(th_popup), "caller", call_btn);

    GtkWidget *form = gtk_table_new(1, 4, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(th_popup), form);

    //
    // label in frame
    //
    GtkWidget *label = gtk_label_new("Click on entries for more help: ");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sLstr lstr;
    int i;
    if (parent == (GRobject)TB()->sh_shell) {
        for (i = 0; KW.shell(i)->word; i++)
            KW.shell(i)->print(&lstr);
    }
    else if (parent == (GRobject)TB()->sd_shell) {
        for (i = 0; KW.sim(i)->word; i++)
            KW.sim(i)->print(&lstr);
    }
    else if (parent == (GRobject)TB()->cm_shell) {
        for (i = 0; KW.cmds(i)->word; i++)
            KW.cmds(i)->print(&lstr);
    }
    else if (parent == (GRobject)TB()->pd_shell) {
        for (i = 0; KW.plot(i)->word; i++)
            KW.plot(i)->print(&lstr);
    }
    else if (parent == (GRobject)TB()->db_shell) {
        for (i = 0; KW.debug(i)->word; i++)
            KW.debug(i)->print(&lstr);
    }
    else
        lstr.add("Internal error.");

    //
    // text area
    //
    GtkWidget *hbox;
    text_scrollable_new(&hbox, &th_text, FNT_FIXED);

    text_set_chars(th_text, lstr.string());

    gtk_widget_add_events(th_text,
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

    g_signal_connect(G_OBJECT(th_text), "button_press_event",
        G_CALLBACK(th_btn_hdlr), this);
    g_signal_connect(G_OBJECT(th_text), "button_release_event",
        G_CALLBACK(th_btn_hdlr), this);

    // This will provide an arrow cursor.
    g_signal_connect_after(G_OBJECT(th_text), "realize",
        G_CALLBACK(text_realize_proc), 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);

    int wid = 80*QTfont::stringWidth(th_text, 0);
    int hei = 12*QTfont::stringHeight(th_text, 0);
    gtk_window_set_default_size(GTK_WINDOW(th_popup), wid + 8, hei + 20);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // buttons
    //
    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(th_cancel_proc), th_popup);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    */
}


QWidget *
sTBhelp::show(TBH_type type)
{

    /*
    gtk_window_set_transient_for(GTK_WINDOW(th_popup),
        GTK_WINDOW(TB()->context->Shell()));
    if (TB()->tb_kw_help_pos[type].x != 0 && TB()->tb_kw_help_pos[type].y != 0)
        gtk_window_move(GTK_WINDOW(th_popup), TB()->tb_kw_help_pos[type].x,
            TB()->tb_kw_help_pos[type].y);
    gtk_widget_show(th_popup);
    */
    return (th_popup);
}


void
sTBhelp::select(GtkWidget *caller, int x, int y)
{
    /*
    char *string = text_get_chars(caller, 0, -1);
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    char *line_start = string + gtk_text_iter_get_offset(&iline);

    // find first word
    while (isspace(*line_start) && *line_start != '\n')
        line_start++;
    if (*line_start == 0 || *line_start == '\n') {
        text_select_range(caller, 0, 0);
        delete [] string;
        return;
    }

    int start = line_start - string;
    char buf[64];
    char *s = buf;
    while (!isspace(*line_start))
        *s++ = *line_start++;
    *s = '\0';
    int end = line_start - string;

    text_select_range(caller, start, end);
#ifdef HAVE_MOZY
    HLP()->word(buf);
#endif
    delete [] string;
    */
}


// Static function.
//
void
sTBhelp::th_cancel_proc(GtkWidget*, void *client_data)
{
    /*
    GtkWidget *popup = (GtkWidget*)client_data;
    GtkWidget *caller = (GtkWidget*)g_object_get_data(G_OBJECT(popup),
        "caller");
    if (caller)
        QTdev::Deselect(caller);
    int type = (intptr_t)g_object_get_data(G_OBJECT(popup), "tbtype");
    TB()->PopDownTBhelp((TBH_type)type);
    */
}


// Static function.
//
int
sTBhelp::th_btn_hdlr(GtkWidget *caller, GdkEvent *event, void *arg)
{
    /*
    sTBhelp *th = (sTBhelp*)arg;
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button.button == 1) {
            th->th_lx = (int)event->button.x;
            th->th_ly = (int)event->button.y;
            return (false);
        }
        return (true);
    }
    if (event->type == GDK_BUTTON_RELEASE) {
        if (event->button.button == 1) {
            int x = (int)event->button.x;
            int y = (int)event->button.y;
            if (abs(x - th->th_lx) <= 4 && abs(y - th->th_ly) <= 4)
                th->select(caller, th->th_lx, th->th_ly);
            return (false);
        }
    }
    */
    return (true);
}


// Static function.
//
void
sTBhelp::th_destroy(void *arg)
{
    delete (sTBhelp*)arg;
}

