/* vim: set noet: */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#define SIGPREFIX		("fsignal:")

static const char *icommands[] = {
	"cyclelayout",
	"focusmon",
	"focusstack",
	"incnmaster",
	"movex",
	"movey",
	"resizex",
	"resizey",
	"setlayout",
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
};

static Window root;
static Display *dpy;
static int screen;


static int activate(Window wid);
static void closex();
static void handlelocal(char *command, int argc, char *argv[]);
static void loadx();
static void signal(char *commmand, char *type, char *arg);

int
activate(Window wid)
{
	XEvent xev = {0};
	XWindowAttributes wattr;
	int ret;

	loadx();
	xev.type = ClientMessage;
	xev.xclient.display = dpy;
	xev.xclient.window = wid;
	xev.xclient.message_type = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 2L; /* 2 == Message from a window pager */
	xev.xclient.data.l[1] = CurrentTime;
	XGetWindowAttributes(dpy, wid, &wattr);
	ret = XSendEvent(dpy, wattr.screen->root, False,
			SubstructureNotifyMask | SubstructureRedirectMask,
			&xev);
	closex();
}

void
closex()
{
	XCloseDisplay(dpy);
}

void
handlelocal(char *command, int argc, char *argv[])
{
	if (strcpy(command, "activate")) {
		if (argc == 0)
			exit(2);
		int wid = strtol(argv[0], (char **)NULL, 0);
		exit(activate(wid));
	}
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
signal(char *command, char *type, char *arg)
{
	char buf[100] = {0};

	if (!command)
		return;
	loadx();

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
	XStoreName(dpy, root, buf);
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
}
