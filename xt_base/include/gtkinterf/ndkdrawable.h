

#ifndef NDKDRAWABLE_H
#define NDKDRAWABLE_H

struct ndkDrawable
{
    ndkDrawable();
    ~ndkDrawable();

private:
    GdkWindow *d_window;
    GdkScreen *d_screen;

#ifdef WITH_X11
    Window d_xid;
#endif
};

#endif

/*
Do we really need a ndkDrawable?

Avoid it for now.  

*/
