/* vim: set noet: */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SIGPREFIX		("fsignal:")
#define IMPPREFIX		("important:")
#define SYNCTIME		(10)


typedef struct {
	char *symbol, *name, *id;
} Layout;

static const char *icommands[] = {
	"cyclelayout",
	"focusmon",
	"focusstack",
	"incnmaster",
	"movex",
	"movey",
	"resizex",
	"resizey",
	"shiftview",
	"shiftviewclients",
	"tagmon",
	"tagmonkt",
	"toggletag",
	"toggleview",
	"resetfacts",
	NULL,
};

static const char *uicommands[] = {
	"tag",
	"view",
	NULL,
};

static const char *fcommands[] = {
	"setcfact",
	"setmfact",
	NULL,
};

static const char *ncommands[] = {
	"center",
	"killclient",
	"quit",
	"restart",
	"restartlaunched",
	"rioresize",
	"tagall",
	"togglebar",
	"togglefloating",
	"togglefullscr",
	"togglegaps",
	"viewall",
	"xrdb",
	"zoom",
	NULL,
};

static const char *lcommands[] = {
	"activate",
	"important",
	"printlayouts",
	"setlayout",
	"status",
	"wmname",
};

static const Layout layouts[] = {
	{ "[]=", "Tiled Layout",			"tile" },
	{ "[]D", "Deck Layout",				"deck" },
	{ "[M]", "Monocle Layout",			"monocle" },
	{ "><>", "Floating Layout",			"floating" },
	{ "=[]", "Left Tiled Layout",		"tileleft" },
	{ "TTT", "Bottom Stack Layout",		"bstack" },
	{ "HHH", "Grid Layout",				"grid" },
	{  NULL,  NULL,						NULL, },
};

static Window root;
static Display *dpy;
static int screen;


static int activate(Window wid, int timeout);
static void closex();
static int getproperty(Window wid, Atom atom, unsigned char **prop);
static void handlelocal(char *command, int argc, char *argv[]);
static void important(char *str);
static void loadx();
static void printlayouts();
static int setlayout(char *arg);
static void signal(char *commmand, char *type, char *arg);
static void status(char *str);
static void wmname(char *name);


int
activate(Window wid, int timeout)
{
	XEvent xev = {0};
	XWindowAttributes wattr;
	Atom atom;
	unsigned char *result = NULL;
	int ret, i, status, data;
	unsigned long sleeptime;

	loadx();
	if (wid == root) {
		closex();
		return 10;
	}

	atom = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);

	xev.type = ClientMessage;
	xev.xclient.display = dpy;
	xev.xclient.window = wid;
	xev.xclient.message_type = atom;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 2L; /* 2 == Message from a window pager */
	xev.xclient.data.l[1] = CurrentTime;
	XGetWindowAttributes(dpy, wid, &wattr);
	ret = !XSendEvent(dpy, wattr.screen->root, False,
			SubstructureNotifyMask | SubstructureRedirectMask,
			&xev);

	if (!ret && timeout) {
		ret = 11;
		for (i = 0; i < timeout/SYNCTIME; i++) {
			status = getproperty(root, atom, &result);
			if (status == BadWindow) {
				fprintf(stderr, "window id # 0x%lx does not exists!\n", wid);
				break;
				ret = 12;
			} else if (status != Success) {
				fprintf(stderr, "XGetWindowProperty failed!\n");
				ret = 13;
			} else {
				data = *(int *)result;
				if (wid == data) {
					XFree(result);
					ret = 0;
					break;
				}
			}

			nanosleep(&((struct timespec) { .tv_nsec = (time_t)(1000000L * (SYNCTIME)) }), NULL);
		}
	}

	closex();
	return ret;
}

void
closex()
{
	XCloseDisplay(dpy);
}

int
getproperty(Window wid, Atom atom, unsigned char **data) {
	Atom actual_type;
	int actual_format;
	unsigned long _nitems;
	unsigned long bytes_after; /* unused */
	int status;
	status = XGetWindowProperty(dpy, wid, atom, 0, (~0L),
			False, AnyPropertyType, &actual_type,
			&actual_format, &_nitems, &bytes_after,
			data);
	return status;
}

void
handlelocal(char *command, int argc, char *argv[])
{
	if (strcmp(command, "activate") == 0) {
		if (argc == 0)
			exit(2);
		int wid = strtol(argv[0], (char **)NULL, 0);
		int timeout = argc > 1 ? strtol(argv[1], (char **)NULL, 0) : 0;
		exit(activate(wid, timeout));
	} else if (strcmp(command, "important") == 0) {
		if (argc == 0)
			exit(2);
		important(argv[0]);
		exit(EXIT_SUCCESS);
	} else if (strcmp(command, "printlayouts") == 0) {
		printlayouts();
		exit(EXIT_SUCCESS);
	} else if (strcmp(command, "setlayout") == 0) {
		if (argc == 0)
			exit(2);
		exit(setlayout(argv[0]));
	} else if (strcmp(command, "status") == 0) {
		if (argc == 0)
			exit(2);
		status(argv[0]);
		exit(EXIT_SUCCESS);
	} else if (strcmp(command, "wmname") == 0) {
		wmname(argc == 0 ? NULL : argv[0]);
		exit(EXIT_SUCCESS);
	} else {
		exit(1);
	}

}

void
important(char *str)
{
	char buf[strlen(IMPPREFIX) + strlen(str) + 1];
	strcpy(buf, IMPPREFIX);
	strcpy(&buf[strlen(IMPPREFIX)], str);
	printf("%s\n", buf);
	status(buf);
}

void
loadx()
{
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "%s:  unable to open display '%s'\n",
				"moonctl", XDisplayName(NULL));
		exit(3);
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
}

void
printlayouts()
{
	for (int i = 0; layouts[i].symbol; i++)
		printf("%s %s\t\t%s\n", layouts[i].symbol, layouts[i].name, layouts[i].id);
}

int
setlayout(char *arg)
{
	char buf[12];
	for (int i = 0; layouts[i].symbol; i++)
		if (strcmp(arg, layouts[i].symbol) == 0
				|| strcmp(arg, layouts[i].id) == 0) {
			sprintf(buf, "%d", i);
			signal("setlayout", "i", buf);
			return 0;
		}
	signal("setlayout", "i", arg);
	return 0;
}

void
signal(char *command, char *type, char *arg)
{
	char buf[100] = {0};

	if (!command)
		return;

	strcpy(buf, SIGPREFIX);
	strcat(buf, command);
	strcat(buf, " ");
	if (type) {
		if (!arg)
			return;
		strcat(buf, type);
		strcat(buf, " ");
		strcat(buf, arg);
	}
	status(buf);
}

void
status(char *str) {
	loadx();
	XStoreName(dpy, root, str);
	closex();
}

void
wmname(char *name) {
	int status;
	unsigned char *data = NULL;
	loadx();
	if (name) {
		XChangeProperty(dpy, root, XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False),
				XA_WINDOW, 32, PropModeReplace, (unsigned char *)&root, 1);
		XChangeProperty(dpy, root, XInternAtom(dpy, "_NET_WM_NAME", False),
				XInternAtom(dpy, "UTF8_STRING", False), 8, PropModeReplace, (unsigned char *)name, strlen(name));
	} else {
		status = getproperty(root, XInternAtom(dpy, "_NET_WM_NAME", False), &data);
		if(data && status == Success)
			fprintf(stdout, "%s\n", data);
		XFree(data);
	}
	closex();
}

int
main(int argc, char *argv[])
{

	int i;
	if (argc == 1) {
		fprintf(stderr, "Please specify a command.\n");
		exit(EXIT_FAILURE);
	}

	/* check int commands */
	for (i = 0; icommands[i]; i++)
		if (strcmp(argv[1], icommands[i]) == 0) {
			if (argc < 3) {
				fprintf(stderr, "The command '%s' requires an argument of type int.\n", argv[1]);
				exit(2);
			}
			signal(argv[1], "i", argv[2]);
			exit(EXIT_SUCCESS);
		}

	/* check unsigned int commands */
	for (i = 0; uicommands[i]; i++)
		if (strcmp(argv[1], uicommands[i]) == 0) {
			if (argc < 3) {
				fprintf(stderr, "The command '%s' requires an argument of type unsigned int.\n", argv[1]);
				exit(2);
			}
			signal(argv[1], "ui", argv[2]);
			exit(EXIT_SUCCESS);
		}

	/* check float commands */
	for (i = 0; fcommands[i]; i++)
		if (strcmp(argv[1], fcommands[i]) == 0) {
			if (argc < 3) {
				fprintf(stderr, "The command '%s' requires an argument of type float.\n", argv[1]);
				exit(2);
			}
			signal(argv[1], "f", argv[2]);
			exit(EXIT_SUCCESS);
		}

	/* check commands without argument */
	for (i = 0; ncommands[i]; i++)
		if (strcmp(argv[1], ncommands[i]) == 0) {
			signal(argv[1], NULL, NULL);
			exit(EXIT_SUCCESS);
		}

	/* check local commands*/
	for (i = 0; lcommands[i]; i++)
		if (strcmp(argv[1], lcommands[i]) == 0) {
			handlelocal(argv[1], argc - 2, &argv[2]);
			exit(EXIT_SUCCESS);
		}


	fprintf(stderr, "'%s' is not a valid command.\n", argv[1]);
	exit(EXIT_FAILURE);
}
