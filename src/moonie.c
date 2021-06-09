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

static Window root;
static Display *dpy;
static int screen;


static void closex();
static void loadx();
static void signal(char *commmand, char *type, char *arg);

void
closex()
{
	XCloseDisplay(dpy);
}

void
loadx()
{
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "%s:  unable to open display '%s'\n",
				"moonie", XDisplayName(NULL));
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

	fprintf(stderr, "'%s' is not a valid command.\n", argv[1]);
}
