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

#include "xwrappers.h"
#include "wmdef.h"
#include "util.h"

int
send_event(Display *dpy, Window w, Atom proto, int mask,
		long d0, long d1, long d2, long d3, long d4)
{
	int n;
	Atom *protocols, mt;
	int exists = 0;
	XEvent ev;

	if (proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
		mt = wmatom[WMProtocols];
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

	XChangeProperty(dpy, win, wmatom[WMState], wmatom[WMState], 32,
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
