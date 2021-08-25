/* vim: set noet: */

#ifndef WMCOMMONS_XWRAPPERS_H
#define WMCOMMONS_XWRAPPERS_H

int send_event(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
int window_get_textprop(Window w, Atom atom, char *text, unsigned int size);
void window_map(Display *dpy, Window win, int deiconify);
void window_set_state(Display *dpy, Window win, long state);
void window_unmap(Display *dpy, Window win, Window root, int iconify);
int xerror(Display *dpy, XErrorEvent *ee);
int xerror_dummy(Display *dpy, XErrorEvent *ee);
int xerror_start(Display *dpy, XErrorEvent *ee);

#endif

