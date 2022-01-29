/* vim: set noet: */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SIGPREFIX		("fsignal:")
#define IMPPREFIX		("important:")
#define SYNCTIME		(10)
#define DEFTIMEOUT		(100)
#define FCOMMANDHELP	("\t%s\n")
#define MIN(a,b)		(((a)<(b))?(a):(b))
#define MAX(a,b)		(((a)>(b))?(a):(b))


typedef struct {
	char *symbol, *name, *id;
} Layout;

static const char *icommands[] = {
	"borrow",
	"cyclelayout",
	"focusdir",
	"focusmon",
	"focusstack",
	"gesture",
	"incnmaster",
	"mousemove",
	"movedir",
	"movex",
	"movey",
	"resetfacts",
	"resizex",
	"resizey",
	"shiftview",
	"shiftviewclients",
	"steal",
	"tagmon",
	"tagmonkt",
	"toggletag",
	"toggleview",
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
	"restore",
	"rioresize",
	"tagall",
	"togglebar",
	"togglefloating",
	"togglefullscr",
	"togglegaps",
	"viewall",
	"winview",
	"xrdb",
	"zoom",
	NULL,
};

static const char *lcommands[] = {
	"--help",
	"-h",
	"activate",
	"active",
	"borderwidth",
	"clienttags",
	"currenttags",
	"help",
	"important",
	"printlayouts",
	"rootwid",
	"setlayout",
	"status",
	"togglelayout",
	"windows",
	"wmname",
	NULL,
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

static int screen;
static char *exename;
static Display *dpy = NULL;
static Window root;


static int activate(Window wid, int timeout);
static void bye();
static void closex();
static void die(int exitcode, char *message);
static int getproperty(Window wid, Atom atom, unsigned char **prop);
static int getpropertydetailed (Window wid, Atom atom, Atom *actual_type, int *actual_format,
		unsigned long *nitems, unsigned long *bytes_after, unsigned char **data);
static void handlelocal(char *command, int argc, char *argv[]);
static void important(char *str);
static void loadx();
static void printhelp();
static void printcmdarr(const char *arr[]);
static void printlayouts();
static void printwindow(Window wid);
static void setlayout(char *arg);
static void signal(char *commmand, char *type, char *arg);
static void setstatus(char *str);
static Window towid(char *str);
static void wmname(char *name);


int
activate(Window wid, int timeout)
{
	XEvent xev = {0};
	XWindowAttributes wattr;
	Atom atom;
	unsigned char *result = NULL;
	int ret, i, status, data;

	if (wid == root) {
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
	ret = XSendEvent(dpy, wattr.screen->root, False,
			SubstructureNotifyMask | SubstructureRedirectMask,
			&xev);

	if (ret && timeout) {
		ret = 0;
		for (i = 0; i < timeout/SYNCTIME; i++) {
			status = getproperty(root, atom, &result);
			if (status == BadWindow) {
				fprintf(stderr, "window id # 0x%lx does not exists!\n", wid);
				break;
				ret = 0;
			} else if (status != Success) {
				fprintf(stderr, "XGetWindowProperty failed!\n");
				ret = 13;
			} else {
				data = *(int *)result;
				if (wid == data) {
					XFree(result);
					ret = 1;
					break;
				}
			}

			nanosleep(&((struct timespec) { .tv_nsec = (time_t)(1000000L * (SYNCTIME)) }), NULL);
		}
	}

	return ret;
}

void
bye()
{
	closex();
	exit(EXIT_SUCCESS);
}

void
closex()
{
	if (dpy)
		XCloseDisplay(dpy);
	dpy = NULL;
}

void
die(int exitcode, char *message)
{
	fprintf(stderr, "%s\n", message);
	closex();
	exit(exitcode);
}

int
getproperty(Window wid, Atom atom, unsigned char **data) {
	Atom actual_type;
	int actual_format;
	unsigned long _nitems;
	unsigned long bytes_after; /* unused */
	return getpropertydetailed(wid, atom, &actual_type, &actual_format, &_nitems, &bytes_after, data);
}

int
getpropertydetailed (Window wid, Atom atom, Atom *actual_type, int *actual_format,
		unsigned long *nitems, unsigned long *bytes_after, unsigned char **data) {
	int status;
	status = XGetWindowProperty(dpy, wid, atom, 0, (~0L),
			False, AnyPropertyType, actual_type,
			actual_format, nitems, bytes_after,
			data);
	return status;
}

void
handlelocal(char *command, int argc, char *argv[])
{
	int wid, timeout, tags, status, bw, i;
	unsigned char *data = NULL;
	if (strcmp(command, "activate") == 0) {
		if (argc == 0)
			die(2, "Not enough arguments");
		wid = towid(argv[0]);
		timeout = argc > 1 ? strtol(argv[1], (char **)NULL, 0) : 0;
		status = activate(wid, timeout);
		if (status)
			bye();
		else
			die(5, "Unable to activate");
	} else if (strcmp(command, "active") == 0) {
		status = getproperty(root, XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False), &data);
		if (status == Success && data) {
			wid = *(int *)data;
			XFree(data);
			printwindow(wid);
			bye();
		}
		die(3, "Unable to get active window");
	} else if (strcmp(command, "borderwidth") == 0) {
		status = getproperty(root, XInternAtom(dpy, "_MWM_BORDER_WIDTH", False), &data);
		if (status == Success && data) {
			bw = *(unsigned int *)data;
			printf("%d\n", bw);
			XFree(data);
			bye();
		}
		die(3, "Unable to get border width");
	} else if (strcmp(command, "clienttags") == 0) {
		if (argc == 0)
			die(2, "Not enough arguments");
		wid = towid(argv[0]);
		status = getproperty(wid, XInternAtom(dpy, "_MWM_CLIENT_TAGS", False), &data);
		if (status == Success && data) {
			tags = *(int *)data;
			printf("%d\n", tags);
			XFree(data);
			bye();
		}
		die(3, "Unable to get client tags");
	} else if (strcmp(command, "currenttags") == 0) {
		status = getproperty(root, XInternAtom(dpy, "_MWM_CURRENT_TAGS", False), &data);
		if (status == Success && data) {
			tags = *(int *)data;
			printf("%d\n", tags);
			XFree(data);
			bye();
		}
		die(3, "Unable to get current tags");
	} else if (strcmp(command, "help") == 0
			|| strcmp(command, "--help") == 0
			|| strcmp(command, "-h") == 0) {
		printhelp();
		bye();
	} else if (strcmp(command, "important") == 0) {
		if (argc == 0)
			die(2, "Not enough arguments");
		important(argv[0]);
		bye();
	} else if (strcmp(command, "printlayouts") == 0) {
		printlayouts();
		bye();
	} else if (strcmp(command, "rootwid") == 0) {
		printf("%ld\n", DefaultRootWindow(dpy));
		bye();
	} else if (strcmp(command, "setlayout") == 0) {
		if (argc == 0)
			die(2, "Not enough arguments");
		setlayout(argv[0]);
		bye();
	} else if (strcmp(command, "status") == 0) {
		if (argc == 0)
			die(2, "Not enough arguments");
		setstatus(argv[0]);
		bye();
	} else if (strcmp(command, "windows") == 0) {
		Atom actual_type;
		int actual_format;
		unsigned long nitems, bytes_after; /* unused */
		status = getpropertydetailed(root, XInternAtom(dpy, "_NET_CLIENT_LIST", False),
				&actual_type, &actual_format, &nitems, &bytes_after, &data);
		if (status == Success && data && actual_format == 32) {
			Window *windows = (Window *) data;
			for (i = 0; i < nitems; i++) {
				wid = windows[i];
				printwindow(wid);
			}
			XFree(data);
			bye();
		}
		die(3, "Unable to list windows");
	} else if (strcmp(command, "wmname") == 0) {
		wmname(argc == 0 ? NULL : argv[0]);
		bye();
	} else {
		die(1, "Please use a valid command (see -h)");
	}

}

void
important(char *str)
{
	char buf[strlen(IMPPREFIX) + strlen(str) + 1];
	strcpy(buf, IMPPREFIX);
	strcpy(&buf[strlen(IMPPREFIX)], str);
	printf("%s\n", buf);
	setstatus(buf);
}

void
loadx()
{
	if (dpy)
		return;
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "%s:  unable to open display '%s'\n",
				exename, XDisplayName(NULL));
		exit(EXIT_FAILURE);
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
}

void
printhelp()
{
	printf("\n%s - Interface to control MoonWM\n\n", exename);

	printf("\nThese commands take an INTEGER as their argument:\n");
	printcmdarr(icommands);

	printf("\nThese commands take an UNSIGNED INTEGER as their argument:\n");
	printcmdarr(uicommands);

	printf("\nThese commands take a FLOATING POINT NUMBER as their argument:\n");
	printcmdarr(fcommands);

	printf("\nThese are the commands with NO ARGUMENT:\n");
	printcmdarr(ncommands);

	printf("\nThese are the commands are handled locally:\n");
	printcmdarr(lcommands);
	printf("\n\tactivate takes an X window id as first argument and a timeout (ms) as second one.\n");
	printf("\tIf no timeout is passed there is no check whether the window got focused.\n");
	printf("\tclienttags also takes an X window id first argument.\n");
	printf("\timportant, setlayout, status and wmname take strings.\n\n");
}

void
printcmdarr(const char *arr[])
{
	printf("\t");
	for (int i = 0; arr[i]; i++) {
		if (!(i % 5) && i)
			printf("\n\t");
		printf("%s, ", arr[i]);
	}
	printf("\n");
}

void
printlayouts()
{
	for (int i = 0; layouts[i].symbol; i++)
		printf("%s %s\t\t%s\n", layouts[i].symbol, layouts[i].name, layouts[i].id);
}

void
printwindow(Window wid)
{
	char *title = NULL;
	int changed = 1;
	XClassHint classhints = {0};
	XFetchName(dpy, wid, (char **) &title);
	XGetClassHint(dpy, wid, &classhints);

	if (title == NULL) {
		title = "";
		changed |= (1 << 1);
	}
	if (classhints.res_name == NULL) {
		classhints.res_name = "unknown";
		changed |= (1 << 2);
	}
	if (classhints.res_class == NULL) {
		classhints.res_class = "unknown";
		changed |= (1 << 3);
	}

	int offset = 25 - (strlen(classhints.res_class) + strlen(classhints.res_name));
	offset = MAX(offset, 0);
	printf("%-15ld%s (%s) %-*s %s\n", wid, classhints.res_class, classhints.res_name,
			offset, "", title);
	if (!(changed & (1 << 1)))
		XFree(title);
	if (!(changed & (1 << 2)))
		XFree(classhints.res_name);
	if (!(changed & (1 << 3)))
		XFree(classhints.res_class);
}

void
setlayout(char *arg)
{
	char buf[12];
	for (int i = 0; layouts[i].symbol; i++)
		if (strcmp(arg, layouts[i].symbol) == 0
				|| strcmp(arg, layouts[i].id) == 0) {
			sprintf(buf, "%d", i);
			signal("setlayout", "i", buf);
			return;
		}
	signal("setlayout", "i", arg);
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
	setstatus(buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, root, str);
}

Window
towid(char *str)
{
	int status;
	unsigned char *data;
	if (strcmp(str, "active") == 0) {
		status = getproperty(root, XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False), &data);
		if (status != Success) {
			die(3, "Unable to get active window");
		} else
			return *(Window *) data;
	}
	return strtol(str, (char **)NULL, 0);
}

void
wmname(char *name) {
	int status;
	unsigned char *data = NULL;
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
}

int
main(int argc, char *argv[])
{

	int i;
	exename = argv[0];
	if (argc == 1) {
		fprintf(stderr, "Please specify a command.\n");
		exit(EXIT_FAILURE);
	}

	if (strcmp(argv[1], "help") != 0
			&& strcmp(argv[1], "--help") != 0
			&& strcmp(argv[1], "-h") != 0)
		loadx();

	/* check int commands */
	for (i = 0; icommands[i]; i++)
		if (strcmp(argv[1], icommands[i]) == 0) {
			if (argc < 3) {
				fprintf(stderr, "The command '%s' requires an argument of type int.\n", argv[1]);
				die(2, "Not enough arguments");
			}
			signal(argv[1], "i", argv[2]);
			bye();
		}

	/* check unsigned int commands */
	for (i = 0; uicommands[i]; i++)
		if (strcmp(argv[1], uicommands[i]) == 0) {
			if (argc < 3) {
				fprintf(stderr, "The command '%s' requires an argument of type unsigned int.\n", argv[1]);
				die(2, "Not enough arguments");
			}
			signal(argv[1], "ui", argv[2]);
			bye();
		}

	/* check float commands */
	for (i = 0; fcommands[i]; i++)
		if (strcmp(argv[1], fcommands[i]) == 0) {
			if (argc < 3) {
				fprintf(stderr, "The command '%s' requires an argument of type float.\n", argv[1]);
				die(2, "Not enough arguments");
			}
			signal(argv[1], "f", argv[2]);
			bye();
		}

	/* check commands without argument */
	for (i = 0; ncommands[i]; i++)
		if (strcmp(argv[1], ncommands[i]) == 0) {
			signal(argv[1], NULL, NULL);
			bye();
		}

	/* check local commands*/
	for (i = 0; lcommands[i]; i++)
		if (strcmp(argv[1], lcommands[i]) == 0) {
			handlelocal(argv[1], argc - 2, &argv[2]);
			bye();
		}

	die(1, "Please use a valid command (see -h)");
}
