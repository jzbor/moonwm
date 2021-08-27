/* vim: set noet: */

#ifndef WMCOMMONS_XWRAPPERS_H
#define WMCOMMONS_XWRAPPERS_H

/* enums */
enum { NetSupported, NetWMDemandsAttention, NetWMName, NetWMState, NetWMCheck,
		NetWMActionClose, NetWMActionMinimize, NetWMAction, NetWMMoveResize,
	   	NetWMMaximizedVert, NetWMMaximizedHorz,
	   	NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation, NetSystemTrayOrientationHorz,
	   	NetWMFullscreen, NetActiveWindow, NetWMWindowType, NetWMWindowTypeDock, NetWMDesktop,
	   	NetWMWindowTypeDesktop, NetWMWindowTypeDialog, NetClientList, NetClientListStacking,
	   	NetDesktopNames, NetDesktopViewport, NetNumberOfDesktops,
	   	NetCurrentDesktop, NetLast, /* EWMH atoms */

		Manager, Xembed, XembedInfo,  /* Xembed atoms */

		SteamGame, MWMClientTags, MWMCurrentTags, MWMClientMonitor, MWMLast, /* MoonWM atoms */

		WMProtocols, WMDelete, WMState, WMTakeFocus, WMChangeState,
		WMWindowRole, /* default atoms */

		Utf8, Motif, LastAtom };

/* function declarations */
void checkotherwm(Display *dpy);
int get_pointer_pos(Display *dpy, Window win, int *x, int *y);
Atom *get_atoms(Display *dpy);
#ifdef XINERAMA
int isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info);
#endif /* XINERAMA */
int send_event(Display *dpy, Window w, Atom proto, int m,
		long d0, long d1, long d2, long d3, long d4);
void set_xerror_xlib(int (*xexlib)(Display *, XErrorEvent *));
Atom window_get_atomprop(Display *dpy, Window win, Atom prop, Atom req);
int window_get_intprop(Display *dpy, Window w, Atom prop);
long window_get_state(Display *dpy, Window win);
int window_get_textprop(Display *dpy, Window w, Atom atom, char *text, unsigned int size);
void window_map(Display *dpy, Window win, int deiconify);
void window_set_state(Display *dpy, Window win, long state);
void window_unmap(Display *dpy, Window win, Window root, int iconify);
int xerror(Display *dpy, XErrorEvent *ee);
int xerror_dummy(Display *dpy, XErrorEvent *ee);
int xerror_start(Display *dpy, XErrorEvent *ee);
int xrdb_get(XrmDatabase db, char *name, char **retval, int *retint, unsigned int *retuint);
int xrdb_get_color(XrmDatabase db, char *name, char *dest);

#endif

