/* vim: set noet: */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */

#include "xwrappers.h"
#include "common.h"
#include "util.h"

/* variables */
static Atom atoms[LastAtom];
static int atoms_intialised = 0;
static int (*xerrorxlib)(Display *, XErrorEvent *);


void
checkotherwm(Display *dpy)
{
	set_xerror_xlib(XSetErrorHandler(xerror_start));
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

Atom *
get_atoms(Display *dpy)
{
	if (!atoms_intialised) {
		atoms[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
		atoms[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
		atoms[WMState] = XInternAtom(dpy, "WM_STATE", False);
		atoms[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
		atoms[WMChangeState] = XInternAtom(dpy, "WM_CHANGE_STATE", False);
		atoms[WMWindowRole] = XInternAtom(dpy, "WM_WINDOW_ROLE", False);

		atoms[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
		atoms[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
		atoms[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
		atoms[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
		atoms[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
		atoms[NetSystemTrayOrientationHorz] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
		atoms[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
		atoms[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
		atoms[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
		atoms[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
		atoms[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
		atoms[NetWMWindowTypeDock] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
		atoms[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
		atoms[NetWMWindowTypeDesktop] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
		atoms[NetWMMaximizedVert] = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
		atoms[NetWMMaximizedHorz] = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
		atoms[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
		atoms[NetClientListStacking] = XInternAtom(dpy, "_NET_CLIENT_LIST_STACKING", False);
		atoms[NetCurrentDesktop] = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
		atoms[NetDesktopNames] = XInternAtom(dpy, "_NET_DESKTOP_NAMES", False);
		atoms[NetDesktopViewport] = XInternAtom(dpy, "_NET_DESKTOP_VIEWPORT", False);
		atoms[NetNumberOfDesktops] = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
		atoms[NetWMActionClose] = XInternAtom(dpy, "_NET_WM_ACTION_CLOSE", False);
		atoms[NetWMDemandsAttention] = XInternAtom(dpy, "_NET_WM_DEMANDS_ATTENTION", False);
		atoms[NetWMDesktop] = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
		atoms[NetWMMoveResize] = XInternAtom(dpy, "_NET_WM_MOVE_RESIZE", False);

		atoms[Manager] = XInternAtom(dpy, "MANAGER", False);
		atoms[Xembed] = XInternAtom(dpy, "_XEMBED", False);
		atoms[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);

		atoms[MWMClientTags] = XInternAtom(dpy, "_MWM_CLIENT_TAGS", False);
		atoms[MWMCurrentTags] = XInternAtom(dpy, "_MWM_CURRENT_TAGS", False);
		atoms[MWMClientMonitor] = XInternAtom(dpy, "_MWM_CLIENT_MONITOR", False);
		atoms[MWMBorderWidth] = XInternAtom(dpy, "_MWM_BORDER_WIDTH", False);
		atoms[SteamGame] = XInternAtom(dpy, "STEAM_GAME", False);

		atoms[Utf8] = XInternAtom(dpy, "UTF8_STRING", False);
		atoms[Motif] = XInternAtom(dpy, "_MOTIF_NetLast, WM_HINTS", False);

		atoms_intialised = 1;
	}
	return atoms;
}

int
get_pointer_pos(Display *dpy, Window win, int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, win, &dummy, &dummy, x, y, &di, &di, &dui);
}

#ifdef XINERAMA
int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

int
send_event(Display *dpy, Window w, Atom proto, int mask,
		long d0, long d1, long d2, long d3, long d4)
{
	int n;
	Atom *protocols, mt;
	int exists = 0;
	XEvent ev;
	get_atoms(dpy);

	if (proto == atoms[WMTakeFocus] || proto == atoms[WMDelete]) {
		mt = atoms[WMProtocols];
		if (XGetWMProtocols(dpy, w, &protocols, &n)) {
			while (!exists && n--)
				exists = protocols[n] == proto;
			XFree(protocols);
		}
	}
	else {
		exists = True;
		mt = proto;
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = w;
		ev.xclient.message_type = mt;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = d0;
		ev.xclient.data.l[1] = d1;
		ev.xclient.data.l[2] = d2;
		ev.xclient.data.l[3] = d3;
		ev.xclient.data.l[4] = d4;
		XSendEvent(dpy, w, False, mask, &ev);
	}
	return exists;
}

void
set_xerror_xlib(int (*xexlib)(Display *, XErrorEvent *))
{
	xerrorxlib = xexlib;
}

Atom
window_get_atomprop(Display *dpy, Window win, Atom prop, Atom req)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;
	get_atoms(dpy);

	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	if (XGetWindowProperty(dpy, win, prop, 0L, sizeof atom, False, req,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		if (da == atoms[XembedInfo] && dl == 2)
			atom = ((Atom *)p)[1];
		XFree(p);
	}
	return atom;
}

int
window_get_intprop(Display *dpy, Window win, Atom prop)
{
	int di, ret = 0;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da;

	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	if (XGetWindowProperty(dpy, win, prop, 0L, 1, False, XA_CARDINAL,
		&da, &di, &dl, &dl, &p) == Success && p) {
		ret = *(int *)p;
		XFree(p);
	}
	return ret;
}

long
window_get_state(Display *dpy, Window win)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;
	get_atoms(dpy);

	if (XGetWindowProperty(dpy, win, atoms[WMState], 0L, 2L, False, atoms[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

int
window_get_textprop(Display *dpy, Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING)
		strncpy(text, (char *)name.value, size - 1);
	else {
		if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

void
window_map(Display *dpy, Window win, int deiconify)
{
	if (!win)
		return;

	XMapWindow(dpy, win);
	if (deiconify)
		window_set_state(dpy, win, NormalState);
	XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
}

void
window_set_state(Display *dpy, Window win, long state)
{
	long data[] = { state, None };
	get_atoms(dpy);

	XChangeProperty(dpy, win, atoms[WMState], atoms[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

void
window_unmap(Display *dpy, Window win, Window root, int iconify)
{
	static XWindowAttributes ra, ca;

	if (!win)
		return;

	XGrabServer(dpy);
	XGetWindowAttributes(dpy, root, &ra);
	XGetWindowAttributes(dpy, win, &ca);
	// prevent UnmapNotify events
	XSelectInput(dpy, root, ra.your_event_mask & ~SubstructureNotifyMask);
	XSelectInput(dpy, win, ca.your_event_mask & ~StructureNotifyMask);
	XUnmapWindow(dpy, win);
	if (iconify)
		window_set_state(dpy, win, IconicState);
	XSelectInput(dpy, root, ra.your_event_mask);
	XSelectInput(dpy, win, ca.your_event_mask);
	XUngrabServer(dpy);
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "moonwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerror_dummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerror_start(Display *dpy, XErrorEvent *ee)
{
	die("moonwm: another window manager is already running");
	return -1;
}

int
xrdb_get(XrmDatabase db, char *name, char **retval, int *retint, unsigned int *retuint)
{
	char *tempval, *type, *dummy;
	int tempint;
	XrmValue xval;
	if (XrmGetResource(db, name, "*", &type, &xval) == False)
		return 0;
	tempval = xval.addr;

	if (retval)
		(*retval) = tempval;
	if (retint || retuint) {
		errno = 0;
		tempint = strtol(tempval, &dummy, 0);
		if (!tempint && errno)
			return 0;
	}
	if (retint)
		(*retint) = tempint;
	if (retuint)
		(*retuint) = tempint;
	return 1;
}

int
xrdb_get_color(XrmDatabase db, char *name, char *dest)
{
	char *str = NULL;
	if (!xrdb_get(db, name, &str, NULL, NULL))
		return 0;
	if (strlen(dest) != 7 || strlen(str) != 7)
		return 0;
	strncpy(dest, str, 7);
	return 1;
}
