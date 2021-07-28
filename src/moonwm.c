/* vim: set noet:
 *
 * See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of moonwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <xcb/res.h>

#include "drw.h"
#include "util.h"

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define GETINC(X)               ((X) - 2000)
#define INC(X)                  ((X) + 2000)
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define INTERSECTC(x,y,w,h,z)   (MAX(0, MIN((x)+(w),(z)->x+(z)->w) - MAX((x),(z)->x)) \
                               * MAX(0, MIN((y)+(h),(z)->y+(z)->h) - MAX((y),(z)->y)))
#define ISFLOATING(C)           (!(C)->mon->lt[(C)->mon->sellt]->arrange || CMASKGET((C), M_FLOATING))
#define ISINC(X)                ((X) > 1000 && (X) < 3000)
#define ISVISIBLEONTAG(C, T)    ((C->tags & T))
#define ISVISIBLE(C)            ISVISIBLEONTAG(C, C->mon->tagset[C->mon->seltags])
#define PREVSEL                 3000
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define CMASKGET(C, I)			((C)->props & (I))
#define CMASKSETTO(C, I, V)		((V) ? ((C)->props |= (I)) : ((C)->props &= ~(I)))
#define CMASKSET(C, I)			((C)->props |= (I))
#define CMASKUNSET(C, I)		((C)->props &= ~(I))
#define CMASKTOGGLE(C, I)		((C)->props ^= (I))
#define MOD(N,M)                ((N)%(M) < 0 ? (N)%(M) + (M) : (N)%(M))
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TAGSLENGTH              (LENGTH(tags))
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define TRUNC(X,A,B)            (MAX((A), MIN((X), (B))))

#define SYSTEM_TRAY_REQUEST_DOCK    0

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY      0
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_FOCUS_IN             4
#define XEMBED_MODALITY_ON         10

#define XEMBED_MAPPED              (1 << 0)
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_WINDOW_DEACTIVATE    2

#define VERSION_MAJOR               0
#define VERSION_MINOR               0
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR

#define MWM_HINTS_FLAGS_FIELD       0
#define MWM_HINTS_DECORATIONS_FIELD 2
#define MWM_HINTS_DECORATIONS       (1 << 1)
#define MWM_DECOR_ALL               (1 << 0)
#define MWM_DECOR_BORDER            (1 << 1)
#define MWM_DECOR_TITLE             (1 << 3)

/* mask indices */
#define	M_BEINGMOVED	(1 << 1)
#define	M_NEVERFOCUS	(1 << 2)
#define	M_OLDSTATE		(1 << 3)
#define M_CENTER		(1 << 4)
#define M_EXPOSED		(1 << 5)
#define M_FIXED			(1 << 6)
#define M_FLOATING		(1 << 7)
#define M_FULLSCREEN	(1 << 8)
#define M_NOSWALLOW		(1 << 9)
#define M_STEAM			(1 << 10)
#define M_TERMINAL		(1 << 11)
#define M_URGENT		(1 << 12)

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeHigh }; /* color schemes */
enum { NetSupported, NetWMDemandsAttention, NetWMName, NetWMState, NetWMCheck,
	   NetWMActionClose, NetWMActionMinimize, NetWMAction, NetWMMoveResize,
	   NetWMMaximizedVert, NetWMMaximizedHorz,
	   NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation, NetSystemTrayOrientationHorz,
	   NetWMFullscreen, NetActiveWindow, NetWMWindowType, NetWMWindowTypeDock, NetWMDesktop,
	   NetWMWindowTypeDesktop, NetWMWindowTypeDialog, NetClientList, NetClientListStacking,
	   NetDesktopNames, NetDesktopViewport, NetNumberOfDesktops,
	   NetCurrentDesktop, NetLast, }; /* EWMH atoms */
enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMChangeState,
	   WMWindowRole, WMLast }; /* default atoms */
enum { SteamGame, MWMClientTags, MWMCurrentTags, MWMClientMonitor, MWMLast }; /* MoonWM atoms */
enum { ClkMenu, ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
	   ClkClientWin, ClkRootWin, ClkLast }; /* clicks */

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	float cfact;
	int x, y, w, h;
	int sfx, sfy, sfw, sfh; /* stored float geometry, used on mode revert */
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw;
	unsigned int tags;
	int isfullscreen,
		isterminal, issteam, isexposed, noswallow, beingmoved;
	int props;
	pid_t pid;
	Client *next;
	Client *snext;
	Client *swallowing;
	Monitor *mon;
	Window win;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char * sig;
	void (*func)(const Arg *);
} Signal;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

typedef struct Pertag Pertag;
struct Monitor {
	char ltsymbol[16];
	float mfact;
	int nmaster;
	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	int gappih;           /* horizontal gap between windows */
	int gappiv;           /* vertical gap between windows */
	int gappoh;           /* horizontal outer gaps */
	int gappov;           /* vertical outer gaps */
	unsigned int seltags;
	unsigned int sellt;
	unsigned int tagset[2];
	int showbar;
	int topbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
	const Layout *lt[2];
	Pertag *pertag;
};

typedef struct {
	const char *class;
	const char *role;
	const char *instance;
	const char *wintype;
	const char *title;
	unsigned int tags;
	int gameid;
	int isfloating;
	int isterminal;
	int center;
	int noswallow;
	int monitor;
} Rule;

typedef struct Systray   Systray;
struct Systray {
	Window win;
	Client *icons;
};

/* function declarations */
static void activate(Client *c);
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int *bw, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachaside(Client *c);
static void attachstack(Client *c);
static int fake_signal(void);
static void buttonpress(XEvent *e);
static void center(const Arg *arg);
static void centerclient(Client *c);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static int compareclients(const void *a, const void *b);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void copyvalidchars(char *text, char *rawtext);
static Monitor *createmon(void);
static void cyclelayout(const Arg *arg);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void dragcfact(const Arg *arg);
static void dragmfact(const Arg *arg);
static void drawbar(Monitor *m);
static void drawbars(void);
static void dropfullscr(Monitor *m, int n, Client *keep);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void exposeview(const Arg *arg);
static void focus(Client *c);
static void focusdir(const Arg *arg);
static void focusfloating(const Arg *arg);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop, Atom req);
static int getintprop(Client *c, Atom prop);
static int getintproproot(Atom prop);
static pid_t getparentprocess(pid_t p);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static unsigned int getsystraywidth();
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void incnmaster(const Arg *arg);
static void incheight(const Arg *arg);
static void incwidth(const Arg *arg);
static int isdescprocess(pid_t p, pid_t c);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void layout(const Arg *arg, int togglelayout);
static void layoutmenu(const Arg *arg);
static void loadclientprops(Client *c);
static int loadenv(char *name, char **retval, int *retint, unsigned int *retuint);
static void loadwmprops(void);
static int loadxcolor(XrmDatabase db, char *name, char *dest);
static void loadxrdb(XrmDatabase db);
static int loadxres(XrmDatabase db, char *name, char **retval, int *retint, unsigned int *retuint);
static void losefullscreen(Client *sel, Client *c, Monitor *m);
static void manage(Window w, XWindowAttributes *wa);
static void map(Client *c, int deiconify);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static void moveorplace(const Arg *arg);
static void movex(const Arg *arg);
static void movey(const Arg *arg);
static void movexfloating(const Arg *arg);
static void moveyfloating(const Arg *arg);
static Client *nexttagged(Client *c);
static Client *nexttiled(Client *c);
static void placemouse(const Arg *arg);
static void pop(Client *);
static void propertynotify(XEvent *e);
static void pushstack(const Arg *arg);
static void quit(const Arg *arg);
static Client *recttoclient(int x, int y, int w, int h);
static Monitor *recttomon(int x, int y, int w, int h);
static void removesystrayicon(Client *i);
static void resetfacts(const Arg *arg);
static void resize(Client *c, int x, int y, int w, int h, int bw, int interact);
static void resizebarwin(Monitor *m);
static void resizeclient(Client *c, int x, int y, int w, int h, int bw);
static void resizefloating(Client *c, int nx, int ny, int nw, int nh);
static void resizemouse(const Arg *arg);
static void resizeorxfact(const Arg *arg);
static void resizerequest(XEvent *e);
static void resizex(const Arg *arg);
static void resizey(const Arg *arg);
static void restack(Monitor *m);
static void restart(const Arg *arg);
static void restartlaunched(const Arg *arg);
static int riodraw(Client *c, const char slopstyle[]);
static void rioposition(Client *c, int x, int y, int w, int h);
static void rioresize(const Arg *arg);
static void riospawn(const Arg *arg);
static void riospawnsync(const Arg *arg);
static void run(void);
static void runautostart(void);
static void scan(void);
static void scrollresize(const Arg *arg);
static int sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
static void sendmon(Client *c, Monitor *m, int keeptags);
static void setclientstate(Client *c, long state);
static void setdesktopnames(void);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(const Arg *arg);
static void setcfact(const Arg *arg);
static void setmfact(const Arg *arg);
static void setmodkey();
static void setnumdesktops(void);
static void settings(void);
static void settingsxrdb(XrmDatabase db);
static void settingsenv(void);
static void setup(void);
static void setviewport(void);
static void seturgent(Client *c, int urg);
static void shiftview(const Arg *arg);
static void shiftviewclients(const Arg *arg);
static void showhide(Client *c);
static void sigchld(int unused);
static void spawn(const Arg *arg);
static pid_t spawncmd(const Arg *arg);
static int stackpos(const Arg *arg);
static Client *swallowingclient(Window w);
static Monitor *systraytomon(Monitor *m);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void tagmonkt(const Arg *arg);
static Client *termforwin(const Client *c);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void togglefullscr(const Arg *arg);
static void togglelayout(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmap(Client *c, int iconify);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static void updateclientmonitor(Client *c);
static void updateclienttags(Client *c);
static void updatecurrenttags(void);
static int updategeom(void);
static void updatemotifhints(Client *c);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatesystray(void);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static void warp(const Client *c, int edge);
static pid_t winpid(Window w);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static Client *wintosystrayicon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void xrdb(const Arg *arg);
static void zoom(const Arg *arg);


/* variables */
static const char autostartblocksh[] = "autostart_blocking.sh";
static const char autostartsh[] = "autostart.sh";
static const char providedautostart[] = "moonwm-util start";
static char *launcherargs[] = { "moonwm-util", "launch" };
static Systray *systray =  NULL;
static const char broken[] = "broken";
static const char moonwmdir[] = "moonwm";
static const char localshare[] = ".local/share";
static const char configfile[] = "config.xres";
static unsigned int imfact = 0;
static char stext[256];
static char rawstext[256];
static int statuscmdn;
static char lastbutton[] = "-";
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh, blw = 0;      /* bar geometry */
static int lrpad;            /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static int ignorewarp = 0;
static int istatustimer = 0;
static int riodimensions[4] = { -1, -1, -1, -1 };
static pid_t riopid = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[ResizeRequest] = resizerequest,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast], mwmatom[MWMLast], motifatom;
static int running = 1;
static int restartwm = 0;
static int restartlauncher = 0;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;

static xcb_connection_t *xcon;

/* configuration, allows nested code to access above variables */
#include "rules.h"
#include "config.h"

struct Pertag {
	unsigned int curtag, prevtag; /* current and previous tag */
	int nmasters[LENGTH(tags) + 1]; /* number of windows in master area */
	float mfacts[LENGTH(tags) + 1]; /* mfacts per tag */
	unsigned int sellts[LENGTH(tags) + 1]; /* selected layouts */
	const Layout *ltidxs[LENGTH(tags) + 1][2]; /* matrix of tags and layouts indexes  */
};

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

/* function implementations */
void
activate(Client *c) {
	unsigned int i;
	Client *n;
	if (!c || c == selmon->sel)
		return;
	for (i = 0; i < LENGTH(tags) && !((1 << i) & c->tags); i++);
	if (i < LENGTH(tags)) {
		const Arg a = {.ui = 1 << i};
		selmon = c->mon;
		if (!ISVISIBLE(c))
			view(&a);
		for (n = selmon->stack; n; n = n->snext)
			losefullscreen(n, c, selmon);
		focus(c);
		restack(c->mon);
	}
}

void
applyrules(Client *c)
{
	const char *class, *instance;
	char role[64];
	unsigned int i, steamid = 0;
	const Rule *r;
	Atom wintype;
	Monitor *m;
	XClassHint ch = { NULL, NULL };

	/* rule matching */
	CMASKUNSET(c, M_FLOATING);
	c->tags = 0;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;
	wintype	 = getatomprop(c, netatom[NetWMWindowType], XA_ATOM);
	gettextprop(c->win, wmatom[WMWindowRole], role, sizeof(role));

	if (strstr(class, "Steam") || strstr(class, "steam_app_")
			|| (steamid = getatomprop(c, mwmatom[SteamGame], AnyPropertyType)))
		c->issteam = 1;

	for (i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if ((!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->role || strstr(role, r->role))
		&& (!r->instance || strstr(instance, r->instance))
		&& (!r->wintype || wintype == XInternAtom(dpy, r->wintype, False))
		&& (!r->gameid || steamid == r->gameid || (steamid && r->gameid == -1) ))
		{
			c->isterminal  = r->isterminal;
			c->noswallow   = r->noswallow;
			CMASKSETTO(c, M_FLOATING, r->isfloating);
			CMASKSETTO(c, M_CENTER, r->center);
			c->tags |= r->tags;
			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int *bw, int interact)
{
	int baseismin, targetw, targeth;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	targetw = *w;
	targeth = *h;
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * *bw < 0)
			*x = 0;
		if (*y + *h + 2 * *bw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * *bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * *bw <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || CMASKGET(c, M_FLOATING) || !c->mon->lt[c->mon->sellt]->arrange) {
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
		/* don't let windows extend beyond their tiled space */
		*w = MIN(*w, targetw);
		*h = MIN(*h, targeth);
		if (resizehints && centeronrh) {
			*x = (2 * *x + targetw - *w) / 2;
			*y = (2 * *y + targeth - *h) / 2;
		}
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h || *bw != c->bw;
}

void
arrange(Monitor *m)
{
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m)
{
	Client *c;
	if (sizeof(m->ltsymbol) > strlen(m->lt[m->sellt]->symbol))
		strcpy(m->ltsymbol, m->lt[m->sellt]->symbol);
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
	else
		/* <>< case; rather than providing an arrange function and upsetting other logic that tests for its presence, simply add borders here */
		for (c = selmon->clients; c; c = c->next)
			if (ISVISIBLE(c) && !c->isfullscreen && c->bw == 0)
				resize(c, c->x, c->y, c->w - 2*borderpx, c->h - 2*borderpx, borderpx, 0);
}

void
attach(Client *c)
{
	c->next = c->mon->clients;
	c->mon->clients = c;
}

void
attachaside(Client *c) {
	Client *at = nexttagged(c);
	if(!at) {
		attach(c);
		return;
 	}
	c->next = at->next;
	at->next = c;
}

void
attachstack(Client *c)
{
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void
swallow(Client *p, Client *c)
{

	if (!swallowdefault || c->noswallow || c->isterminal)
		return;
	if (!swallowfloating && CMASKGET(c, M_FLOATING))
		return;

	detach(c);
	detachstack(c);

	setclientstate(c, WithdrawnState);
	XUnmapWindow(dpy, p->win);

	p->swallowing = c;
	c->mon = p->mon;

	Window w = p->win;
	p->win = c->win;
	c->win = w;
	updatetitle(p);
	XMoveResizeWindow(dpy, p->win, p->x, p->y, p->w, p->h);
	arrange(p->mon);
	configure(p);
	updateclientlist();
	updateclientmonitor(c);
}

void
unswallow(Client *c)
{
	c->win = c->swallowing->win;

	free(c->swallowing);
	c->swallowing = NULL;

	/* unfullscreen the client */
	setfullscreen(c, 0);
	updatetitle(c);
	arrange(c->mon);
	XMapWindow(dpy, c->win);
	XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
	setclientstate(c, NormalState);
	focus(NULL);
	arrange(c->mon);
}

void
buttonpress(XEvent *e)
{
	unsigned int i, x, click;
	Arg arg = {0};
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;
	*lastbutton = '0' + ev->button;

	click = ClkRootWin;
	/* focus monitor if necessary */
	if ((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}

	if (ev->window == selmon->barwin) {
		i = 0;
		x = TEXTW(menulabel);
		do
			x += TEXTW(tags[i]);
		while (ev->x >= x && ++i < LENGTH(tags));

		if (ev->x <= TEXTW(menulabel))
			click = ClkMenu;
		else if (i < LENGTH(tags)) {
			click = ClkTagBar;
			arg.ui = 1 << i;
		} else if (ev->x < x + blw)
			click = ClkLtSymbol;
		else if (ev->x > (x = selmon->ww - (int)TEXTW(stext) + lrpad - (systraytomon(selmon) == selmon ? getsystraywidth() : 0))) {
			click = ClkStatusText;

			char *text = rawstext;
			int i = -1;
			char ch;
			statuscmdn = 0;
			while (text[++i]) {
				if ((unsigned char)text[i] < ' ') {
					ch = text[i];
					text[i] = '\0';
					x += TEXTW(text) - lrpad;
					text[i] = ch;
					text += i+1;
					i = -1;
					if (x >= ev->x) break;
					statuscmdn = ch - 1;
				}
			}
		} else
			click = ClkWinTitle;
	} else if ((c = wintoclient(ev->window))) {
		focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
	for (i = 0; i < LENGTH(buttons); i++)
		if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
center(const Arg *arg)
{
	if(!selmon->sel) {
		return;
	}
	if (selmon->lt[selmon->sellt]->arrange && !CMASKGET(selmon->sel, M_FLOATING))
		return;

	centerclient(selmon->sel);

	/* arrange(selmon); */
}

void
centerclient(Client *c)
{
	int barmod = (c->mon->topbar ? 1 : -1) * (c->mon->showbar && center_relbar ? bh : 0);
	int cx = c->mon->mx + (c->mon->mw - WIDTH(c)) / 2;
	int cy = c->mon->my + barmod + (c->mon->mh - barmod - HEIGHT(c)) / 2;

	resize(c, cx, cy, c->w, c->h, borderpx, 0);
}

void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void)
{
	Layout foo = { "", NULL };
	Monitor *m;
	size_t i;

	/* view(&a); */
	selmon->lt[selmon->sellt] = &foo;
	for (m = mons; m; m = m->next)
		while (m->stack) {
			XMapWindow(dpy, m->stack->win);
			setclientstate(m->stack, NormalState);
			unmanage(m->stack, 0);
		}
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	if (showsystray) {
		while (systray->icons)
			removesystrayicon(systray->icons);
		XUnmapWindow(dpy, systray->win);
		XDestroyWindow(dpy, systray->win);
		free(systray);
	}
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	free(scheme);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	free(mon->pertag);
	free(mon);
}

void
clientmessage(XEvent *e)
{
	unsigned int desktop;
	XWindowAttributes wa;
	XSetWindowAttributes swa;
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if (showsystray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
		/* add systray icons */
		if (cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
			if (!(c = (Client *)calloc(1, sizeof(Client))))
				die("fatal: could not malloc() %u bytes\n", sizeof(Client));
			if (!(c->win = cme->data.l[2])) {
				free(c);
				return;
			}
			c->mon = selmon;
			c->next = systray->icons;
			systray->icons = c;
			if (!XGetWindowAttributes(dpy, c->win, &wa)) {
				/* use sane defaults */
				wa.width = bh;
				wa.height = bh;
				wa.border_width = 0;
			}
			c->x = c->oldx = c->y = c->oldy = 0;
			c->w = c->oldw = wa.width;
			c->h = c->oldh = wa.height;
			c->oldbw = wa.border_width;
			c->bw = 0;
			CMASKSET(c, M_FLOATING);
			/* reuse tags field as mapped status */
			c->tags = 1;
			updatesizehints(c);
			updatesystrayicongeom(c, wa.width, wa.height);
			updateclientmonitor(c);
			XAddToSaveSet(dpy, c->win);
			XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
			XClassHint ch ={"moonwm-systray", "moonwm-systray"};
			XSetClassHint(dpy, c->win, &ch);
			XReparentWindow(dpy, c->win, systray->win, 0, 0);
			/* use parents background color */
			swa.background_pixel  = scheme[SchemeNorm][ColStatusBg].pixel;
			XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			/* FIXME not sure if I have to send these events, too */
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_FOCUS_IN, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_MODALITY_ON, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			XSync(dpy, False);
			resizebarwin(selmon);
			updatesystray();
			setclientstate(c, NormalState);
		}
		return;
	}
	if (cme->message_type == netatom[NetCurrentDesktop]) {
		desktop = cme->data.l[0];
		if (!((1 << desktop) & selmon->tagset[selmon->seltags] & TAGMASK))
			view(&((Arg) { .ui = (1 << desktop) }));
		return;
	} else if (cme->message_type == mwmatom[MWMCurrentTags]) {
		view(&((Arg) { .ui = cme->data.l[0] }));
		return;
	}
	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
		else if(cme->data.l[1] == netatom[NetWMDemandsAttention]) {
			CMASKSETTO(c, M_URGENT, (cme->data.l[0] == 1 || (cme->data.l[0] == 2 && !CMASKGET(c, M_URGENT))));
			drawbar(c->mon);
		}

		/* unsigned int maximize_vert = (cme->data.l[1] == netatom[NetWMMaximizedVert] || cme->data.l[2] == netatom[NetWMMaximizedVert]); */
		/* unsigned int maximize_horz = (cme->data.l[1] == netatom[NetWMMaximizedHorz] || cme->data.l[2] == netatom[NetWMMaximizedHorz]); */
		/* if (c == selmon->sel && (maximize_vert || maximize_horz)) { */
		/* 	c->isfloating = 0; */
		/* 	setlayout(&((Arg) {.v = &layouts[MONOCLEPOS]})); */
		/* } */
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		activate(c);
	} else if (cme->message_type == wmatom[WMChangeState]) {
		if (c == selmon->sel)
			togglefloating(NULL);
	} else if (cme->message_type == netatom[NetWMActionClose]) {
		selmon->sel = c;
		killclient(NULL);
	} else if (cme->message_type == netatom[NetWMDesktop]) {
		desktop = cme->data.l[0];
		c->tags |= (1 << desktop) & TAGMASK;
		focus(NULL);
		arrange(c->mon);
		updateclienttags(c);
	} else if (cme->message_type == netatom[NetWMMoveResize]) {
		resizemouse(&((Arg) { .v = c }));
	}
}

int
compareclients(const void *a, const void *b)
{
	const Client *ca, *cb;
	ca = *(const Client**) a;
	cb = *(const Client**) b;

	if (ca->tags < cb->tags)
		return -1;
	else if (ca->tags > cb->tags)
		return 1;
	else
		return 0;
}

void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e)
{
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	/* TODO: updategeom handling sucks, needs to be simplified */
	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			drw_resize(drw, sw, bh);
			updatebars();
			for (m = mons; m; m = m->next) {
				for (c = m->clients; c; c = c->next)
					if (c->isfullscreen)
						resizeclient(c, m->mx, m->my, m->mw, m->mh, 0);
				resizebarwin(m);
			}
			focus(NULL);
			arrange(NULL);
		}
	}
}

void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (CMASKGET(c, M_FLOATING) || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			if (!c->issteam) {
				if (ev->value_mask & CWX) {
					c->oldx = c->x;
					c->x = m->mx + ev->x;
				}
				if (ev->value_mask & CWY) {
					c->oldy = c->y;
					c->y = m->my + ev->y;
				}
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && CMASKGET(c, M_FLOATING))
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && CMASKGET(c, M_FLOATING))
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

void
copyvalidchars(char *text, char *rawtext)
{
	int i = -1, j = 0;

	while(rawtext[++i]) {
		if ((unsigned char)rawtext[i] >= ' ') {
			text[j++] = rawtext[i];
		}
	}
	text[j] = '\0';
}

Monitor *
createmon(void)
{
	Monitor *m;
	int i;

	m = ecalloc(1, sizeof(Monitor));
	m->tagset[0] = m->tagset[1] = 1;
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	m->topbar = topbar;
	m->gappih = gappih;
	m->gappiv = gappiv;
	m->gappoh = gappoh;
	m->gappov = gappov;
	m->lt[0] = &layouts[defaultlayout];
	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	if (sizeof(m->ltsymbol) > strlen(m->lt[m->sellt]->symbol))
		strcpy(m->ltsymbol, m->lt[m->sellt]->symbol);
	if (!(m->pertag = (Pertag *)calloc(1, sizeof(Pertag))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Pertag));
	m->pertag->curtag = m->pertag->prevtag = 1;
	for (i=0; i <= LENGTH(tags); i++) {
		/* init nmaster */
		m->pertag->nmasters[i] = m->nmaster;

		/* init mfacts */
		m->pertag->mfacts[i] = m->mfact;

		/* init layouts */
		m->pertag->ltidxs[i][0] = m->lt[0];
		m->pertag->ltidxs[i][1] = m->lt[1];
		m->pertag->sellts[i] = m->sellt;
	}
	return m;
}

void
cyclelayout(const Arg *arg) {
	Layout *l;
	for(l = (Layout *)layouts; l != selmon->lt[selmon->sellt]; l++);
	if(arg->i > 0) {
		if(l->symbol && (l + 1)->symbol)
			setlayout(&((Arg) { .v = (l + 1) }));
		else
			setlayout(&((Arg) { .v = layouts }));
	} else {
		if(l != layouts && (l - 1)->symbol)
			setlayout(&((Arg) { .v = (l - 1) }));
		else
			setlayout(&((Arg) { .v = &layouts[LENGTH(layouts) - 2] }));
	}
}

void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);
	else if ((c = swallowingclient(ev->window)))
		unmanage(c->swallowing, 1);
	else if ((c = wintosystrayicon(ev->window))) {
		removesystrayicon(c);
		resizebarwin(selmon);
		updatesystray();
	}
}

void
detach(Client *c) {
	Client **tc;

	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
detachstack(Client *c)
{
	Client **tc, *t;

	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	if (dir > 0) {
		if (!(m = selmon->next))
			m = mons;
	} else if (selmon == mons)
		for (m = mons; m->next; m = m->next);
	else
		for (m = mons; m->next != selmon; m = m->next);
	return m;
}

void
dragcfact(const Arg *arg)
{
	int prev_x, prev_y, dist_x, dist_y, center_x, center_y, sign;
	float fact;
	Client *c;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (CMASKGET(c, M_FLOATING)) {
		resizemouse(arg);
		return;
	}
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);

	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;

	prev_x = prev_y = -999999;

	/* XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w/2, c->h); */

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / framerate))
				continue;
			lasttime = ev.xmotion.time;
			if (prev_x == -999999) {
				prev_x = ev.xmotion.x_root;
				prev_y = ev.xmotion.y_root;
			}

			center_x = selmon->mx + c->x + (WIDTH(c) / 2);
			center_y = selmon->my + c->y + (HEIGHT(c) / 2);
			dist_x = ev.xmotion.x - prev_x;
			dist_y = ev.xmotion.y - prev_y;

			if (abs(dist_x) > abs(dist_y)) {
				sign = abs(ev.xmotion.x - center_x) < abs(prev_x - center_x) ? -1 : 1;
				fact = (float) dcfnuance * (sign * abs(dist_x)) / c->mon->ww;
			} else {
				sign = abs(ev.xmotion.y - center_y) < abs(prev_y - center_y) ? -1 : 1;
				fact = (float) dcfnuance * (sign * abs(dist_y)) / c->mon->wh;
			}

			ignorewarp = 1;
			if (fact)
				setcfact(&((Arg) { .f = fact }));

			prev_x = ev.xmotion.x;
			prev_y = ev.xmotion.y;
			break;
		}
	} while (ev.type != ButtonRelease);

	ignorewarp = 0;

	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
dragmfact(const Arg *arg)
{
	int prev_x, prev_y, dist_x, dist_y, center_x, center_y, sign;
	float fact;
	Client *c;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (CMASKGET(c, M_FLOATING)) {
		resizemouse(arg);
		return;
	}
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);

	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;

	prev_x = prev_y = -999999;

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / framerate))
				continue;
			lasttime = ev.xmotion.time;
			if (prev_x == -999999) {
				prev_x = ev.xmotion.x_root;
				prev_y = ev.xmotion.y_root;
			}

			center_x = selmon->mx + c->x + (WIDTH(c) / 2);
			center_y = selmon->my + c->y + (HEIGHT(c) / 2);
			dist_x = ev.xmotion.x - prev_x;
			dist_y = ev.xmotion.y - prev_y;

			if (abs(dist_x) > abs(dist_y)) {
				sign = abs(ev.xmotion.x - center_x) < abs(prev_x - center_x) ? -1 : 1;
				fact = (float) dmfnuance * (sign * abs(dist_x)) / c->mon->ww;
			} else {
				sign = abs(ev.xmotion.y - center_y) < abs(prev_y - center_y) ? -1 : 1;
				fact = (float) dmfnuance * (sign * abs(dist_y)) / c->mon->wh;
			}

			ignorewarp = 1;
			if (fact)
				setmfact(&((Arg) { .f = fact }));

			prev_x = ev.xmotion.x;
			prev_y = ev.xmotion.y;
			break;
		}
	} while (ev.type != ButtonRelease);


	ignorewarp = 0;

	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
drawbar(Monitor *m)
{
	int x, w, tw = 0, stw = 0;
	unsigned int i, occ = 0, urg = 0;
	Client *c;

	if(showsystray && m == systraytomon(m))
		stw = getsystraywidth();

	/* draw status first so it can be overdrawn by tags later */
	drw_setscheme(drw, scheme[SchemeNorm]);
	tw = TEXTW(stext) - lrpad / 2 + 2; /* 2px right padding */
	drw_text(drw, m->ww - tw - stw, 0, tw, bh, lrpad / 2 - 2, stext, 0, ColStatusFg, ColStatusBg);

	resizebarwin(m);
	for (c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if (CMASKGET(c, M_URGENT))
			urg |= c->tags;
	}

	x = 0;
	w = TEXTW(menulabel);
	drw_setscheme(drw, scheme[SchemeHigh]);
	drw_text(drw, x, 0, w, bh, lrpad/2, menulabel, 0, ColMenuFg, ColMenuBg);
	x = w;

	for (i = 0; i < LENGTH(tags); i++) {
		w = TEXTW(tags[i]);
		if (occ & 1 << i)
			drw_setscheme(drw, scheme[SchemeHigh]);
		else
			drw_setscheme(drw, scheme[SchemeNorm]);

		drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], m->tagset[m->seltags] & 1 << i, ColTagFg, ColTagBg);
		if (m == selmon && selmon->sel && selmon->sel->tags & 1 << i && m->tagset[m->seltags] & 1 << i) {
			drw_rect(drw, x + 8, bh - 4, w - 16, 1, 1, ColTagBg);
		} else if (m == selmon && selmon->sel && selmon->sel->tags & 1 << i) {
			drw_rect(drw, x + 8, bh - 4, w - 16, 1, 1, ColTagFg);
		}
		x += w;
	}
	w = blw = TEXTW(m->ltsymbol);
	drw_setscheme(drw, scheme[SchemeHigh]);
	drw_rect(drw, x, 0, w, bh,  1, ColTagBg);
	x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0, ColTagFg, ColTagBg);

	if ((w = m->ww - tw - stw - x) > bh) {
		if (m->sel) {
			/* make sure name will not overlap on tags even when it is very long */
			char title[LENGTH(m->sel->name) + LENGTH(floatingindicator)] = {0};
			if (CMASKGET(m->sel, M_FLOATING))
				sprintf(title, floatingindicator, m->sel->name);
			else
				strcpy(title, m->sel->name);
			int mid = (m->ww - (int)TEXTW(title)) / 2 - x;
			mid = mid >= lrpad / 2 ? mid : lrpad / 2;
			drw_setscheme(drw, scheme[m == selmon ? SchemeHigh : SchemeNorm]);
			drw_text(drw, x, 0, w, bh, mid, title, 0, ColTitleFg, ColTitleBg);
		} else {
			drw_setscheme(drw, scheme[m == selmon ? SchemeHigh : SchemeNorm]);
			drw_rect(drw, x, 0, w, bh, 1, ColTitleBg);
		}
	}
	drw_map(drw, m->barwin, 0, 0, m->ww - stw, bh);
}

void
drawbars(void)
{
	Monitor *m;

	for (m = mons; m; m = m->next)
		drawbar(m);
}

void
dropfullscr(Monitor *m, int n, Client *keep)
{
	int counter = 0;
	for (Client *c = m->stack; c && ISVISIBLE(c); c = c->snext) {
		if (c->isfullscreen && c != keep) {
			if (counter < n)
				counter++;
			else
				setfullscreen(c, 0);
		}
	}
}

void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
	focus(c);
}

void
expose(XEvent *e)
{
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0 && (m = wintomon(ev->window))) {
		drawbar(m);
		if (m == selmon)
			updatesystray();
	}
}

void
exposeview(const Arg *arg)
{
	unsigned int i, n, bw;
	int x, y, cols, rows, ch, cw, cn, rn, rrest, crest; // counters
	int oh, ov, ih, iv;
	Client *c;
	Monitor *m = selmon;
	XWindowChanges wc;
	XConfigureEvent ce;

	view(&((Arg){.ui = ~0}));
	setlayout(&((Arg) { .v = &layouts[GRIDPOS] }));

	getgaps(m, &oh, &ov, &ih, &iv, &n);
	for (n = 0, c = m->clients; c; c = c->next, n++);
	if (n == 0)
		return;

	/* grid dimensions */
	for (cols = 0; cols <= n/2; cols++)
		if (cols*cols >= n)
			break;
	if (n == 5) /* set layout against the general calculation: not 1:2:2, but 2:3 */
		cols = 2;
	rows = n/cols;
	cn = rn = 0; // reset column no, row no, client count

	ch = (m->wh - 2*oh - ih * (rows - 1)) / rows;
	cw = (m->ww - 2*ov - iv * (cols - 1)) / cols;
	rrest = (m->wh - 2*oh - ih * (rows - 1)) - ch * rows;
	crest = (m->ww - 2*ov - iv * (cols - 1)) - cw * cols;
	x = m->wx + ov;
	y = m->wy + oh;
    bw = n == 1 ? 0 : borderpx;

	Client *clients[n];
	for (i = 0, c = m->clients; c && i < n; i++, c = c->next)
		clients[i] = c;
	qsort(clients, n, sizeof(Client *), compareclients);

	/* UNSORTED: for (i = 0, c = m->clients; c; i++, c = c->next) { */
	for (i = 0, c = clients[0]; i < n; i++, c = clients[i]) {
		if (i/rows + 1 > cols - n%cols) {
			rows = n/cols + 1;
			ch = (m->wh - 2*oh - ih * (rows - 1)) / rows;
			rrest = (m->wh - 2*oh - ih * (rows - 1)) - ch * rows;
		}

		wc.x = x;
		wc.y = y + rn*(ch + ih) + MIN(rn, rrest);
		wc.width = cw + (cn < crest ? 1 : 0) - 2*bw;
		wc.height = ch + (rn < rrest ? 1 : 0) - 2*bw;
		wc.border_width = bw;
		XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
		/* configure(c); */
		XSync(dpy, False);

		ce.type = ConfigureNotify;
		ce.display = dpy;
		ce.event = c->win;
		ce.window = c->win;
		ce.x = wc.x;
		ce.y = wc.y;
		ce.width = wc.width;
		ce.height = wc.height;
		ce.border_width = wc.border_width;
		ce.above = None;
		ce.override_redirect = False;
		XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);

		c->isexposed = 1;

		rn++;
		if (rn >= rows) {
			rn = 0;
			x += cw + ih + (cn < crest ? 1 : 0);
			cn++;
		}
	}
}

void
focus(Client *c)
{
	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (CMASKGET(c, M_URGENT))
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		XSetWindowBorder(dpy, c->win, scheme[SchemeHigh][ColBorder].pixel);
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	drawbars();
}

void
focusdir(const Arg *arg)
{
	Client *s = selmon->sel, *f = NULL, *c, *next;

	if (!s)
		return;

	unsigned int score = -1;
	int dist = 3000000;
	unsigned int client_score;
	int client_dist;
	int dirweight = 20;
	int isfloating = ISFLOATING(s);

	next = s->next;
	if (!next)
		next = s->mon->clients;
	for (c = next; c != s; c = next) {

		next = c->next;
		if (!next)
			next = s->mon->clients;

		if (!ISVISIBLE(c) || ISFLOATING(c) != isfloating) // || HIDDEN(c)
			continue;

		switch (arg->i) {
		case 0: // left
			client_dist = s->x - c->x - c->w;
			client_score =
				dirweight * MIN(abs(client_dist), abs(client_dist + s->mon->ww)) +
				abs(s->y - c->y);
			break;
		case 1: // right
			client_dist = c->x - s->x - s->w;
			client_score =
				dirweight * MIN(abs(client_dist), abs(client_dist + s->mon->ww)) +
				abs(c->y - s->y);
			break;
		case 2: // up
			client_dist = s->y - c->y - c->h;
			client_score =
				dirweight * MIN(abs(client_dist), abs(client_dist + s->mon->wh)) +
				abs(s->x - c->x);
			break;
		default:
		case 3: // down
			client_dist = c->y - s->y - s->h;
			client_score =
				dirweight * MIN(abs(client_dist), abs(client_dist + s->mon->wh)) +
				abs(c->x - s->x);
			break;
		}

		if ((((arg->i == 0 || arg->i == 2) && client_score <= score) || client_score < score)
				|| (dist == client_dist && c == s->snext && !(s->x == c->x && s->y == c->y))){
			score = client_score;
			dist = client_dist;
			f = c;
		}
	}

	if (f && f != s) {
		focus(f);
		restack(f->mon);
	}
}

void
focusfloating(const Arg *arg)
{
	Client *c;

	if (!selmon->sel)
		return;

	for (c = selmon->clients; c && (CMASKGET(c, M_FLOATING) == CMASKGET(selmon->sel, M_FLOATING)); c = c->next) ;
	if (c && (CMASKGET(c, M_FLOATING) != CMASKGET(selmon->sel, M_FLOATING))) {
		focus(c);
		warp(c, 0);
	}
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

void
focusmon(const Arg *arg)
{
	Monitor *m;

	if (!mons->next)
		return;
	if ((m = dirtomon(arg->i)) == selmon)
		return;
	unfocus(selmon->sel, 0);
	selmon = m;
	focus(NULL);
	warp(selmon->sel, 0);
}

void
focusstack(const Arg *arg)
{
	int i = stackpos(arg);
	Client *c, *p;

	if(i < 0 || selmon->sel->isfullscreen)
		return;

	for(p = NULL, c = selmon->clients; c && (i || !ISVISIBLE(c));
		i -= ISVISIBLE(c) ? 1 : 0, p = c, c = c->next);
	focus(c ? c : p);
	restack(selmon);
}

Atom
getatomprop(Client *c, Atom prop, Atom req)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		if (da == xatom[XembedInfo] && dl == 2)
			atom = ((Atom *)p)[1];
		XFree(p);
	}
	return atom;
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

unsigned int
getsystraywidth()
{
	unsigned int w = 0;
	Client *i;
	if(showsystray)
		for(i = systray->icons; i; w += i->w + systrayspacing, i = i->next) ;
	return w ? w + systrayspacing : 1;
}

int
getintprop(Client *c, Atom prop)
{
	int di, ret = 0;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da;

	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	if (XGetWindowProperty(dpy, c->win, prop, 0L, 1, False, XA_CARDINAL,
		&da, &di, &dl, &dl, &p) == Success && p) {
		ret = *(int *)p;
		XFree(p);
	}
	return ret;
}

int
getintproproot(Atom prop)
{
	int di, ret = 0;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da;

	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	if (XGetWindowProperty(dpy, root, prop, 0L, 1, False, XA_CARDINAL,
		&da, &di, &dl, &dl, &p) == Success && p) {
		ret = *(int *)p;
		XFree(p);
	}
	return ret;
}
int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
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
grabbuttons(Client *c, int focused)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].click == ClkClientWin)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, buttons[i].button,
						buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

void
grabkeys(void)
{
	if (!managekeys)
		return;
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		KeyCode code;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		for (i = 0; i < LENGTH(keys); i++)
			if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
						True, GrabModeAsync, GrabModeAsync);
	}
}

void
incnmaster(const Arg *arg)
{
	selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag] = MAX(selmon->nmaster + arg->i, 0);
	arrange(selmon);
}

void
incheight(const Arg *arg)
{
	if(!selmon->sel || selmon->sel->isfullscreen) {
		return;
	}
	if (selmon->lt[selmon->sellt]->arrange && !CMASKGET(selmon->sel, M_FLOATING))
		return;

	int inc = arg->i;
	int height = selmon->sel->h + inc;
	int y = selmon->sel->y - inc/2;

	if ((selmon->sel->y == selmon->wy) && (selmon->sel->y + selmon->sel->h + 2*selmon->sel->bw == selmon->wy + selmon->wh)
			&& (inc < 0)) {
		// centered shrinking
	} else if ((selmon->sel->y == selmon->wy) || (y < selmon->wy)) {
		y = selmon->wy;
	} else if ((selmon->sel->y + selmon->sel->h + 2*selmon->sel->bw == selmon->wy + selmon->wh)
			|| (y + height + 2*selmon->sel->bw > selmon->wy + selmon->wh)) {
		y = selmon->wy + selmon->wh - height - 2*selmon->sel->bw;
	}

	resizefloating(selmon->sel, selmon->sel->x, y,
		   selmon->sel->w,
		   height);
	Client *c = selmon->sel;
	applysizehints(c, &c->x, &c->y, &c->w, &c->h, &c->bw, True);
}

void
incwidth(const Arg *arg)
{
	if(!selmon->sel || selmon->sel->isfullscreen)
		return;
	if (selmon->lt[selmon->sellt]->arrange && !CMASKGET(selmon->sel, M_FLOATING))
		return;

	int inc = arg->i;
	int width = selmon->sel->w + inc;
	int x = selmon->sel->x - inc/2;

	if ((selmon->sel->x == selmon->wx) && (selmon->sel->x + selmon->sel->w + 2*selmon->sel->bw == selmon->wx + selmon->ww)
			&& (inc < 0)) {
		// centered shrinking
	} else if ((selmon->sel->x == selmon->wx) || (x < selmon->wx)) {
		x = selmon->wx;
	} else if ((selmon->sel->x + selmon->sel->w + 2*selmon->sel->bw == selmon->wx + selmon->ww)
			|| (x + width + 2*selmon->sel->bw > selmon->wx + selmon->ww)) {
		x = selmon->wx + selmon->ww - width - 2*selmon->sel->bw;
	}

	resizefloating(selmon->sel, x, selmon->sel->y,
		   width,
		   selmon->sel->h);
}

#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

void
keypress(XEvent *e)
{
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	for (i = 0; i < LENGTH(keys); i++)
		if (keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func)
			keys[i].func(&(keys[i].arg));
}

int
fake_signal(void)
{
	char fsignal[256];
	char indicator[9] = "fsignal:";
	char str_sig[50];
	char param[16];
	int i, len_str_sig, n, paramn;
	size_t len_fsignal, len_indicator = strlen(indicator);
	Arg arg;

	// Get root name property
	if (gettextprop(root, XA_WM_NAME, fsignal, sizeof(fsignal))) {
		len_fsignal = strlen(fsignal);

		// Check if this is indeed a fake signal
		if (len_indicator > len_fsignal ? 0 : strncmp(indicator, fsignal, len_indicator) == 0) {
			paramn = sscanf(fsignal+len_indicator, "%s%n%s%n", str_sig, &len_str_sig, param, &n);

			if (paramn == 1) arg = (Arg) {0};
			else if (paramn > 2) return 1;
			else if (strncmp(param, "i", n - len_str_sig) == 0)
				sscanf(fsignal + len_indicator + n, "%i", &(arg.i));
			else if (strncmp(param, "ui", n - len_str_sig) == 0)
				sscanf(fsignal + len_indicator + n, "%u", &(arg.ui));
			else if (strncmp(param, "f", n - len_str_sig) == 0)
				sscanf(fsignal + len_indicator + n, "%f", &(arg.f));
			else return 1;

			// Check if a signal was found, and if so handle it
			for (i = 0; i < LENGTH(signals); i++)
				if (strcmp(str_sig, signals[i].sig) == 0 && signals[i].func)
					signals[i].func(&(arg));

			// A fake signal was sent
			return 1;
		}
	}

	// No fake signal was sent, so proceed with update
	return 0;
}

void
killclient(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0 , 0, 0)) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
layout(const Arg *arg, int togglelayout)
{
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt]
			|| (togglelayout && arg->v == selmon->lt[selmon->sellt])) {
		selmon->pertag->sellts[selmon->pertag->curtag] ^= 1;
		selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
	}
	if (arg && arg->v && arg->v && (!togglelayout || arg->v != selmon->lt[selmon->sellt^1]))
		selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt] = (Layout *)arg->v;
	selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
	if (sizeof(selmon->ltsymbol) > strlen(selmon->lt[selmon->sellt]->symbol))
		strcpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol);
	if (selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
}

void
layoutmenu(const Arg *arg) {
	FILE *p;
	char c[3], *s;
	int i;

	if (!(p = popen(layoutmenu_cmd, "r")))
		 return;
	s = fgets(c, sizeof(c), p);
	pclose(p);

	if (!s || *s == '\0' || c == '\0')
		 return;

	i = atoi(c);
	setlayout(&((Arg) { .v = &layouts[i] }));
}

void
loadclientprops(Client *c)
{
	unsigned int ui;
	Monitor *m;

	if (!c)
		return;
	ui = getintprop(c, mwmatom[MWMClientTags]);
	if (ui & TAGMASK)
		c->tags = ui & TAGMASK;
	ui = getintprop(c, mwmatom[MWMClientMonitor]);
	if (ui) {
		for (m = mons; m && m->num != ui; m = m->next);
		if (m)
			c->mon = m;
	}
}

int
loadxcolor(XrmDatabase db, char *name, char *dest)
{
	char *str = NULL;
	if (!loadxres(db, name, &str, NULL, NULL))
		return 0;
	if (strlen(dest) != 7 || strlen(str) != 7)
		return 0;
	strncpy(dest, str, 7);
	return 1;
}

int
loadxres(XrmDatabase db, char *name, char **retval, int *retint, unsigned int *retuint)
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

/* load variable from environment variable */
int
loadenv(char *name, char **retval, int *retint, unsigned int *retuint)
{
    int tempint;
    char *tempval, *dummy;

    tempval = getenv(name);
    if (!tempval)
        return 0;
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

void
loadwmprops(void) {
	unsigned int ui;
	ui = getintproproot(mwmatom[MWMCurrentTags]);
	if (ui)
		view(&((Arg) { .ui = ui }));
}

void
loadxrdb(XrmDatabase db)
{
	if (!db)
		return;

	loadxcolor(db, "moonwm.vacantTagFg", normtagfg);
	loadxcolor(db, "moonwm.vacantTagFg", normtagfg);
	loadxcolor(db, "moonwm.vacantTagBg", normtagbg);
	loadxcolor(db, "moonwm.unfocusedTitleFg", normtitlefg);
	loadxcolor(db, "moonwm.unfocusedTitleBg", normtitlebg);
	loadxcolor(db, "moonwm.statusFg", statusfg);
	loadxcolor(db, "moonwm.statusBg", statusbg);
	loadxcolor(db, "moonwm.menuFg", menufg);
	loadxcolor(db, "moonwm.menuBg", menubg);
	loadxcolor(db, "moonwm.unfocusedBorder", normborderfg);
	loadxcolor(db, "moonwm.occupiedTagFg", hightagfg);
	loadxcolor(db, "moonwm.occupiedTagBg", hightagbg);
	loadxcolor(db, "moonwm.focusedTitleFg", hightitlefg);
	loadxcolor(db, "moonwm.focusedTitleBg", hightitlebg);
	loadxcolor(db, "moonwm.focusedBorder", highborderfg);
}

void
losefullscreen(Client *sel, Client *c, Monitor *m)
{
	if (!sel || !c || !m)
		return;
	if (sel->isfullscreen && ISVISIBLE(sel) && sel->mon == m && !CMASKGET(c, M_FLOATING))
		setfullscreen(sel, 0);
}

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL, *term = NULL;
	Window trans = None;
	XWindowChanges wc;

	c = ecalloc(1, sizeof(Client));
	c->win = w;
	c->pid = winpid(w);
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;
	c->cfact = 1.0;

	updatetitle(c);
	c->mon = selmon;
	applyrules(c);
	term = termforwin(c);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
	}
	loadclientprops(c);

	if (getatomprop(c, netatom[NetWMWindowType], XA_ATOM) == netatom[NetWMWindowTypeDesktop]) {
		XMapWindow(dpy, c->win);
		XLowerWindow(dpy, c->win);
		free(c);
		return;
	} else if (getatomprop(c, netatom[NetWMWindowType], XA_ATOM) == netatom[NetWMWindowTypeDock]) {
		XMapWindow(dpy, c->win);
		XRaiseWindow(dpy, c->win);
		free(c);
		return;
	}

	c->bw = borderpx;
	if (centerspawned || CMASKGET(c, M_CENTER)) {
		c->x = c->mon->wx + (c->mon->ww - WIDTH(c)) / 2;
		c->y = c->mon->wy + (c->mon->wh - HEIGHT(c)) / 2;
		centerclient(c);
	} else {
		getrootptr(&c->x, &c->y);
		c->x += spawnedoffset;
		c->y += spawnedoffset;
	}
	c->x = MIN(c->x, c->mon->mx + c->mon->mw - WIDTH(c));
	c->y = MIN(c->y, c->mon->my + c->mon->mh - HEIGHT(c));
	c->x = MAX(c->x, c->mon->wx);
	c->y = MAX(c->y, c->mon->wy);

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	updatemotifhints(c);
	updateclienttags(c);
	updateclientmonitor(c);
	c->sfx = c->x;
	c->sfy = c->y;
	c->sfw = c->w;
	c->sfh = c->h;
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	if (!CMASKGET(c, M_FLOATING))
		CMASKSETTO(c, M_FLOATING|M_OLDSTATE, trans != None || CMASKGET(c, M_FIXED));
	if (CMASKGET(c, M_FLOATING))
		XRaiseWindow(dpy, c->win);
	attachaside(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XChangeProperty(dpy, root, netatom[NetClientListStacking], XA_WINDOW, 32, PropModePrepend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (c->mon == selmon) {
		losefullscreen(selmon->sel, c, c->mon);
		unfocus(selmon->sel, 0);
	}
	c->mon->sel = c;

	if (riopid && (!riodraw_matchpid || isdescprocess(riopid, c->pid))
			&& riodimensions[3] != -1)
		rioposition(c, riodimensions[0], riodimensions[1], riodimensions[2], riodimensions[3]);

	arrange(c->mon);
	if (term)
		swallow(term, c);
	focus(NULL);
}

void
map(Client *c, int deiconify)
{
	XMapWindow(dpy, c->win);
	if (deiconify)
		setclientstate(c, NormalState);
	XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;
	Client *i;
	if ((i = wintosystrayicon(ev->window))) {
		sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
		resizebarwin(selmon);
		updatesystray();
	}

	if (!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if (wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if (ev->window != root)
		return;
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / framerate))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (abs(selmon->wx - nx) < snap)
				nx = selmon->wx;
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			if (abs(selmon->wy - ny) < snap)
				ny = selmon->wy;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			if (!CMASKGET(c, M_FLOATING) && selmon->lt[selmon->sellt]->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);
			if (!selmon->lt[selmon->sellt]->arrange || CMASKGET(c, M_FLOATING))
				resize(c, nx, ny, c->w, c->h, c->bw, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m, arg->i ? 1 : 0);
		selmon = m;
		focus(NULL);
	}

	if (arg->i)
		activate(c);
}

void
moveorplace(const Arg *arg) {
	if (selmon->sel && ISFLOATING(selmon->sel)
			&& !(selmon->sel->isfullscreen && !CMASKGET(selmon->sel, M_OLDSTATE)))
		movemouse(arg);
	else
		placemouse(arg);
}

void
movex(const Arg *arg) {
	if (ISFLOATING(selmon->sel)) {
		movexfloating(arg);
	} else {
		zoom(NULL);
	}
}

void
movey(const Arg *arg) {
	if (ISFLOATING(selmon->sel)) {
		arg = &(Arg) { .i = -arg->i };
		moveyfloating(arg);
	} else if (arg->i < 0) {
		pushstack(&((Arg) { .i = INC(+1) }));
	} else {
		pushstack(&((Arg) { .i = INC(-1) }));
	}
}

void
movexfloating(const Arg *arg)
{
	if(!selmon->sel || selmon->sel->isfullscreen) {
		return;
	}
	if (selmon->lt[selmon->sellt]->arrange && !CMASKGET(selmon->sel, M_FLOATING))
		return;

	int dist = arg->i;
	int x = selmon->sel->x + dist;

	if ((selmon->sel->x > selmon->wx) && (x < selmon->wx)) {
		x = selmon->wx;
	} else if ((selmon->sel->x + WIDTH(selmon->sel) < selmon->wx + selmon->ww)
			&& (x + WIDTH(selmon->sel) > selmon->wx + selmon->ww)) {
		x = selmon->wx + selmon->ww - WIDTH(selmon->sel);
	}


	resizefloating(selmon->sel, x, selmon->sel->y,
		   selmon->sel->w,
		   selmon->sel->h);
}

void
moveyfloating(const Arg *arg)
{
	if(!selmon->sel || selmon->sel->isfullscreen) {
		return;
	}
	if (selmon->lt[selmon->sellt]->arrange && !CMASKGET(selmon->sel, M_FLOATING))
		return;

	int dist = arg->i;
	int y = selmon->sel->y + dist;

	if ((selmon->sel->y > selmon->wy) && (y < selmon->wy)) {
		y = selmon->wy;
	} else if ((selmon->sel->y + HEIGHT(selmon->sel) < selmon->wy + selmon->wh)
			&& (y + HEIGHT(selmon->sel) > selmon->wy + selmon->wh)) {
		y = selmon->wy + selmon->wh - HEIGHT(selmon->sel);
	}


	resizefloating(selmon->sel, selmon->sel->x, y,
		   selmon->sel->w,
		   selmon->sel->h);
}

Client *
nexttagged(Client *c) {
	Client *walked = c->mon->clients;
	for(;
		walked && (CMASKGET(walked, M_FLOATING) || !ISVISIBLEONTAG(walked, c->tags));
		walked = walked->next
	);
	return walked;
}

Client *
nexttiled(Client *c)
{
	for (; c && (CMASKGET(c, M_FLOATING)  || !ISVISIBLE(c)); c = c->next);
	return c;
}

void
placemouse(const Arg *arg)
{
	// arg->i = 1 to keep tags
	int x, y, px, py, ocx, ocy, nx = -9999, ny = -9999, freemove = 0;
	Client *c, *r = NULL, *at, *prevr;
	Monitor *m;
	XEvent ev;
	XWindowAttributes wa;
	Time lasttime = 0;
	int attachmode, prevattachmode, wasfullscreen = 0;
	attachmode = prevattachmode = -1;

	if (!(c = selmon->sel) || !c->mon->lt[c->mon->sellt]->arrange) /* no support for placemouse when floating layout is used */
		return;
	if (c->isfullscreen) {
		wasfullscreen = 1;
		/* togglefullscr(NULL); */
		setfullscreen(c, 0);
	}

	restack(selmon);
	prevr = c;
	px = c->x;
	py = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess) {
		if (wasfullscreen)
			setfullscreen(c, 1);
		return;
	}

	CMASKUNSET(c, M_FLOATING);
	c->beingmoved = 1;

	XGetWindowAttributes(dpy, c->win, &wa);
	ocx = wa.x;
	ocy = wa.y;

	if (placemousemode == 2) // warp cursor to client center
		XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, WIDTH(c) / 2, HEIGHT(c) / 2);

	if (!getrootptr(&x, &y)) {
		if (wasfullscreen)
			setfullscreen(c, 1);
		return;
	}

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / framerate))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);

			if (!freemove && (abs(nx - ocx) > snap || abs(ny - ocy) > snap))
				freemove = 1;

			if (freemove)
				XMoveWindow(dpy, c->win, nx, ny);

			if ((m = recttomon(ev.xmotion.x, ev.xmotion.y, 1, 1)) && m != selmon)
				selmon = m;

			if (placemousemode == 1) { // tiled position is relative to the client window center point
				px = nx + wa.width / 2;
				py = ny + wa.height / 2;
			} else { // tiled position is relative to the mouse cursor
				px = ev.xmotion.x;
				py = ev.xmotion.y;
			}

			r = recttoclient(px, py, 1, 1);

			if (!r || r == c)
				break;

			attachmode = 0; // below
			if (((float)(r->y + r->h - py) / r->h) > ((float)(r->x + r->w - px) / r->w)) {
				if (abs(r->y - py) < r->h / 2)
					attachmode = 1; // above
			} else if (abs(r->x - px) < r->w / 2)
					attachmode = 1; // above

			if ((r && r != prevr) || (attachmode != prevattachmode)) {
				detachstack(c);
				detach(c);
				if (c->mon != r->mon) {
					arrangemon(c->mon);
					if (!arg->i) {
						c->tags = r->mon->tagset[r->mon->seltags];
						updateclienttags(c);
					}
					/* if (!arg->i) { */
					/* 	arrangemon(c->mon); */
					/* 	c->tags = r->mon->tagset[r->mon->seltags]; */
					/* } else */
					/* 	arrange(c->mon); */
				}

				c->mon = r->mon;
				r->mon->sel = r;

				if (attachmode) {
					if (r == r->mon->clients)
						attach(c);
					else {
						for (at = r->mon->clients; at->next != r; at = at->next);
						c->next = at->next;
						at->next = c;
					}
				} else {
					c->next = r->next;
					r->next = c;
				}

				updateclientmonitor(c);
				attachstack(c);
				arrangemon(r->mon);
				prevr = r;
				prevattachmode = attachmode;
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);

	if ((m = recttomon(px, py, 1, 1)) && m != c->mon) {
		detach(c);
		detachstack(c);
		arrangemon(c->mon);
		c->mon = m;
		updateclientmonitor(c);
		if (!arg->i) {
			c->tags = m->tagset[m->seltags];
			updateclienttags(c);
		}
		attach(c);
		attachstack(c);
		selmon = m;
	}

	focus(c);
	c->beingmoved = 0;

	if (nx != -9999)
		resize(c, nx, ny, c->w, c->h, c->bw, 0);
	if (!arg->i)
		arrangemon(c->mon);
	else {
		arrange(c->mon);
		activate(c);
	}

	if (wasfullscreen)
		setfullscreen(c, 1);
	dropfullscr(c->mon, 0, c);
}

void
pop(Client *c)
{
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if ((c = wintosystrayicon(ev->window))) {
		if (ev->atom == XA_WM_NORMAL_HINTS) {
			updatesizehints(c);
			updatesystrayicongeom(c, c->w, c->h);
		}
		else
			updatesystrayiconstate(c, ev);
		resizebarwin(selmon);
		updatesystray();
	}
	if ((ev->window == root) && (ev->atom == XA_WM_NAME)) {
		if (!fake_signal())
			updatestatus();
	}
	else if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!CMASKGET(c, M_FLOATING) && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(CMASKSETTO(c, M_FLOATING, (wintoclient(trans)) != NULL)))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			updatesizehints(c);
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
		if (ev->atom == motifatom)
			updatemotifhints(c);
	}
}

void
pushstack(const Arg *arg) {
	int i = stackpos(arg);
	Client *sel = selmon->sel, *c, *p;

	if(i < 0)
		return;
	else if(i == 0) {
		detach(sel);
		attach(sel);
	}
	else {
		for(p = NULL, c = selmon->clients; c; p = c, c = c->next)
			if(!(i -= (ISVISIBLE(c) && c != sel)))
				break;
		c = c ? c : p;
		detach(sel);
		sel->next = c->next;
		c->next = sel;
	}
	arrange(selmon);
}

void
quit(const Arg *arg)
{
	running = 0;
}

Client *
recttoclient(int x, int y, int w, int h)
{
	Client *c, *r = NULL;
	int a, area = 0;

	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next)) {
		if ((a = INTERSECTC(x, y, w, h, c)) > area) {
			area = a;
			r = c;
		}
	}
	return r;
}

Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void
removesystrayicon(Client *i)
{
	Client **ii;

	if (!showsystray || !i)
		return;
	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
	if (ii)
		*ii = i->next;
	free(i);
}

void
resetfacts(const Arg *arg)
{
	int i;
	Client *c;

	if (arg->i) {
		if (!selmon->sel)
			return;
		selmon->sel->cfact = 1.0;
		for (i = 0, c = nexttiled(selmon->clients); c && i < selmon->nmaster; c = nexttiled(c->next), i++)
			if ((c == selmon->sel))
				selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = mfact;
	} else {
		for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next))
			c->cfact = 1.0;
		selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = mfact;
	}
	arrange(selmon);
}

void
resize(Client *c, int x, int y, int w, int h, int bw, int interact)
{
	if (c->isexposed) {
		c->isexposed = 0;
		resizeclient(c, x, y, w, h, bw);
		for (c = selmon->clients; c; c = c->next)
			if (ISVISIBLE(c) && c->isfullscreen) {
				XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
					PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
				resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh, 0);
			}
	} else if (applysizehints(c, &x, &y, &w, &h, &bw, interact))
		resizeclient(c, x, y, w, h, bw);
}

void
resizebarwin(Monitor *m) {
	unsigned int w = m->ww;
	if (showsystray && m == systraytomon(m))
		w -= getsystraywidth();
	XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, w, bh);
}

void
resizeclient(Client *c, int x, int y, int w, int h, int bw)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	c->oldbw = c->bw = wc.border_width = bw;

	if (c->beingmoved)
		return;

	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

void
resizefloating(Client *c, int nx, int ny, int nw, int nh) {
 	resize(c, nx, ny, nw, nh, c->bw, False);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w / 2, c->h / 2);

	/* XEvent ev; */
	/* while(XCheckMaskEvent(dpy, EnterWindowMask, &ev)); */
}

void
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / framerate))
				continue;
			lasttime = ev.xmotion.time;

			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!CMASKGET(c, M_FLOATING) && selmon->lt[selmon->sellt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if (!selmon->lt[selmon->sellt]->arrange || CMASKGET(c, M_FLOATING))
				resize(c, c->x, c->y, nw, nh, c->bw, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m, 0);
		selmon = m;
		focus(NULL);
	}

	if (arg->i) {
		centerclient(c);
		warp(c, 1);
	}
}

static
void resizeorxfact(const Arg *arg) {
	int i, ismaster = 0;
	Client *c;

	if (selmon->sel && ISFLOATING(selmon->sel))
		resizemouse(arg);
	else {
		for (i = 0, c = nexttiled(selmon->clients); c && i < selmon->nmaster; c = nexttiled(c->next), i++)
			if ((c == selmon->sel))
				ismaster = 1;
		if (ismaster)
			dragmfact(arg);
		else
			dragcfact(arg);
	}

}

void
resizerequest(XEvent *e)
{
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		updatesystrayicongeom(i, ev->width, ev->height);
		resizebarwin(selmon);
		updatesystray();
	}
}

void
resizex(const Arg *arg) {
	if (ISFLOATING(selmon->sel)) {
		incwidth(arg);
	} else if (arg->i > 0) {
		setmfact(&((Arg) { .f = +0.05 }));
	} else {
		setmfact(&((Arg) { .f = -0.05 }));
	}
}

void
resizey(const Arg *arg) {
	if (ISFLOATING(selmon->sel)) {
		incheight(arg);
	} else if (arg->i > 0) {
		setcfact(&((Arg) { .f = +0.25 }));
	} else {
		setcfact(&((Arg) { .f = -0.25 }));
	}
}

void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if (!m->sel)
		return;
	if (CMASKGET(m->sel, M_FLOATING) || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!CMASKGET(c, M_FLOATING) && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if (m == selmon && (m->tagset[m->seltags] & m->sel->tags) && selmon->lt[selmon->sellt] != &layouts[2]) {
		if (!ignorewarp)
			warp(m->sel, 0);
		ignorewarp = 0;
	}
}

void
restart(const Arg *arg)
{
	restartwm = 1;
	running = 0;
}

void
restartlaunched(const Arg *arg)
{
	restartlauncher = 1;
	running = 0;
}

// drag out an area using slop and resize the selected window to it.
int
riodraw(Client *c, const char slopstyle[])
{
	int i;
	char str[100] = {0};
	char strout[100] = {0};
	char tmpstring[30] = {0};
	char slopcmd[100] = "slop -f x%xx%yx%wx%hx ";
	int firstchar = 0;
	int counter = 0;

	/* if (c && c->win) */
	/* 	unmap(c, 0); */

	strcat(slopcmd, slopstyle);
	FILE *fp = popen(slopcmd, "r");

	while (fgets(str, 100, fp) != NULL)
		strcat(strout, str);

	pclose(fp);

	if (strlen(strout) < 6)
		return 0;

	for (i = 0; i < strlen(strout); i++){
		if(!firstchar) {
			if (strout[i] == 'x')
				firstchar = 1;
			continue;
		}

		if (strout[i] != 'x')
			tmpstring[strlen(tmpstring)] = strout[i];
		else {
			riodimensions[counter] = atoi(tmpstring);
			counter++;
			memset(tmpstring,0,strlen(tmpstring));
		}
	}

	/* if (c && c->win) */
	/* 	map(c, 0); */

	if (riodimensions[0] <= -40 || riodimensions[1] <= -40 || riodimensions[2] <= 50 || riodimensions[3] <= 50) {
		riodimensions[3] = -1;
		return 0;
	}

	if (c) {
		rioposition(c, riodimensions[0], riodimensions[1], riodimensions[2], riodimensions[3]);
		return 0;
	}

	return 1;
}

void
rioposition(Client *c, int x, int y, int w, int h)
{
	Monitor *m;
	if ((m = recttomon(x, y, w, h)) && m != c->mon) {
		detach(c);
		detachstack(c);
		arrange(c->mon);
		c->mon = m;
		c->tags = m->tagset[m->seltags];
		updateclienttags(c);
		updateclientmonitor(c);
		attach(c);
		attachstack(c);
		selmon = m;
		focus(c);
	}

	CMASKSET(c, M_FLOATING);
	if (riodraw_borders)
		resize(c, x, y, w - (borderpx * 2), h - (borderpx * 2), borderpx, 0);
	else
		resize(c, x - borderpx, y - borderpx, w, h, borderpx, 0);
	arrange(c->mon);

	riodimensions[3] = -1;
	riopid = 0;
}

/* drag out an area using slop and resize the selected window to it */
void
rioresize(const Arg *arg)
{
	Client *c = (arg && arg->v ? (Client*)arg->v : selmon->sel);
	if (c)
		riodraw(c, slopresizestyle);
}

/* spawn a new window and drag out an area using slop to postiion it */
void
riospawn(const Arg *arg)
{
	if (riodraw_spawnasync && arg->v != dmenucmd) {
		riopid = spawncmd(arg);
		riodraw(NULL, slopspawnstyle);
	} else
		riospawnsync(arg);
}

void
riospawnsync(const Arg *arg)
{
	if (riodraw(NULL, slopspawnstyle))
		riopid = spawncmd(arg);
}

void
run(void)
{
	XEvent ev;
	/* main event loop */
	XSync(dpy, False);
	while (running && !XNextEvent(dpy, &ev))
		if (handler[ev.type])
			handler[ev.type](&ev); /* call handler */
}

void
runautostart(void)
{
	char *pathpfx;
	char *path;
	char *xdgdatahome;
	char *home;
	struct stat sb;

	if ((home = getenv("HOME")) == NULL)
		/* this is almost impossible */
		return;

	/* run moonwm-util if available */
	system(providedautostart);

	/* run statuscmd if available */
	spawn(&((Arg) { .v = statuscmd }));

	/* if $XDG_DATA_HOME is set and not empty, use $XDG_DATA_HOME/moonwm,
	 * otherwise use ~/.local/share/moonwm as autostart script directory
	 */
	xdgdatahome = getenv("XDG_DATA_HOME");
	if (xdgdatahome != NULL && *xdgdatahome != '\0') {
		/* space for path segments, separators and nul */
		pathpfx = ecalloc(1, strlen(xdgdatahome) + strlen(moonwmdir) + 2);

		if (sprintf(pathpfx, "%s/%s", xdgdatahome, moonwmdir) <= 0) {
			free(pathpfx);
			return;
		}
	} else {
		/* space for path segments, separators and nul */
		pathpfx = ecalloc(1, strlen(home) + strlen(localshare)
							 + strlen(moonwmdir) + 3);

		if (sprintf(pathpfx, "%s/%s/%s", home, localshare, moonwmdir) < 0) {
			free(pathpfx);
			return;
		}
	}

	/* check if the autostart script directory exists */
	if (! (stat(pathpfx, &sb) == 0 && S_ISDIR(sb.st_mode))) {
		/* the XDG conformant path does not exist or is no directory
		 * so we try ~/.moonwm instead
		 */
		if (realloc(pathpfx, strlen(home) + strlen(moonwmdir) + 3) == NULL) {
			free(pathpfx);
			return;
		}

		if (sprintf(pathpfx, "%s/.%s", home, moonwmdir) <= 0) {
			free(pathpfx);
			return;
		}
	}

	/* try the blocking script first */
	path = ecalloc(1, strlen(pathpfx) + strlen(autostartblocksh) + 2);
	if (sprintf(path, "%s/%s", pathpfx, autostartblocksh) <= 0) {
		free(path);
		free(pathpfx);
	}

	if (access(path, X_OK) == 0)
		system(path);

	/* now the non-blocking script */
	if (sprintf(path, "%s/%s", pathpfx, autostartsh) <= 0) {
		free(path);
		free(pathpfx);
	}

	if (access(path, X_OK) == 0)
		system(strcat(path, " &"));

	free(pathpfx);
	free(path);
}

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

void
scrollresize(const Arg *arg)
{
	int i, sign, ismaster = 0;
	Client *c;
	if (!selmon->sel)
		return;

	if (ISFLOATING(selmon->sel)) {
			incwidth(arg);
			incheight(arg);
	} else {
		for (i = 0, c = nexttiled(selmon->clients); c && i < selmon->nmaster; c = nexttiled(c->next), i++)
			if ((c == selmon->sel))
				ismaster = 1;
		sign = (arg->i) > 0 ? 1 : -1;
		if (ismaster)
			setmfact(&((Arg) { .f = sign * 0.05 }));
		else
			setcfact(&((Arg) { .f = sign * 0.25 }));
	}
}

void
sendmon(Client *c, Monitor *m, int keeptags)
{
	if (c->mon == m)
		return;
	unfocus(c, 1);
	detach(c);
	detachstack(c);
	c->mon = m;
	updateclientmonitor(c);
	if (!keeptags) {
		c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
		updateclienttags(c);
	}
	attachaside(c);
	attachstack(c);
	if (c->isfullscreen) {
		setfullscreen(c, 0);
		setfullscreen(c, 1);
		dropfullscr(c->mon, 0, c);
	}
	focus(NULL);
	arrange(NULL);
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

void setdesktopnames(void){
	XTextProperty text;
	Xutf8TextListToTextProperty(dpy, (char **) tags, TAGSLENGTH, XUTF8StringStyle, &text);
	XSetTextProperty(dpy, root, &text, netatom[NetDesktopNames]);
}

int
sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4)
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
setfocus(Client *c)
{
	if (!CMASKGET(c, M_NEVERFOCUS)) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}
	if (c->issteam)
		setclientstate(c, NormalState);
	sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
}

void
setfullscreen(Client *c, int fullscreen)
{
	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		CMASKSETTO(c, M_OLDSTATE, CMASKGET(c, M_FLOATING));
		CMASKSET(c, M_FLOATING);
		unsigned int bw = c->bw;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh, 0);
		c->oldbw = bw;
		XRaiseWindow(dpy, c->win);
	} else if (!fullscreen && c->isfullscreen){
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		CMASKSETTO(c, M_FLOATING, CMASKGET(c, M_OLDSTATE));
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h, c->bw);
		arrange(c->mon);
	}
}

void
setlayout(const Arg *arg)
{
	layout(arg, 0);
}

void
setcfact(const Arg *arg)
{
	float f;
	Client *c;

	c = selmon->sel;

	if(!arg || !c || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f + c->cfact;
	if(arg->f == 0.0)
		f = 1.0;
	else if (f < 0.25)
		f = 0.25;
	else if (f > 4.0)
		f = 4.0;
	c->cfact = f;
	arrange(selmon);
}


/* arg > 1.0 will set mfact absolutely */
void
setmfact(const Arg *arg)
{
	float f;

	if (!arg || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
	if (f < 0.05 || f > 0.95)
		return;
	selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = f;
	arrange(selmon);
}

void
setnumdesktops(void){
	long data[] = { TAGSLENGTH };
	XChangeProperty(dpy, root, netatom[NetNumberOfDesktops], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}

void
setmodkey()
{
	char* modenv = getenv("MOONWM_MODKEY");
	unsigned int modkey;

    if(!modenv)
		modkey = Mod1Mask;
	else if (strcmp(modenv, "Super") == 0)
		modkey = Mod4Mask;
    else
		modkey = Mod1Mask;

	for (int i = 0; i < LENGTH(keys); i++) {
		if ((keys[i].mod & DynamicModifier) != 0) {
			keys[i].mod &= ~DynamicModifier;
			keys[i].mod |= modkey;
		}
	}

	for (int i = 0; i < LENGTH(buttons); i++) {
		if ((buttons[i].mask & DynamicModifier) != 0) {
			buttons[i].mask &= ~DynamicModifier;
			buttons[i].mask |= modkey;
		}
	}
}

void
settings(void) {
	XrmDatabase dpydb, cfiledb;
	char  *home, *xdgconfighome, *path = NULL;
	imfact = mfact * 100;

	setmodkey();

	char *temp = XResourceManagerString(dpy);
	if (temp != NULL) {
		dpydb = XrmGetStringDatabase(temp);
		if (dpydb) {
			settingsxrdb(dpydb);
			loadxrdb(dpydb);
		}
		XrmDestroyDatabase(dpydb);
	}

	/* dpydb = XrmGetDatabase(dpy); */
	/* if (dpydb) */
	/* 	settingsxrdb(dpydb); */

	if ((xdgconfighome = getenv("XDG_CONFIG_HOME"))) {
		path = ecalloc(sizeof(char), strlen(xdgconfighome) + strlen(moonwmdir) + strlen(configfile) + 3);
		sprintf(path, "%s/%s/%s", xdgconfighome, moonwmdir, configfile);
	} else if ((home = getenv("HOME"))) {
		path = ecalloc(sizeof(char), strlen(home) + strlen(moonwmdir) + strlen(configfile) + 11);
		sprintf(path, "%s/.config/%s/%s", home, moonwmdir, configfile);
	}
	if (path) {
		printf("Config file path: %s\n", path);
		cfiledb = XrmGetFileDatabase(path);
		if (cfiledb) {
			settingsxrdb(cfiledb);
			loadxrdb(cfiledb);
		}
		XrmDestroyDatabase(cfiledb);
		free(path);
	}
	else
		printf("No config file path available\n");

	/* settingsenv(); */

	/* sanity checks */
	if (!framerate)
		framerate = 60;
	if (borderpx > 100)
		borderpx = 2;

	if (imfact >= 5 && imfact <= 95)
		mfact = (float) imfact / 100;
}

void
settingsenv(void) {
	unsigned int imfact = mfact * 100;
	loadenv("MOONWM_CENTERONRH",	NULL,	&centeronrh,		NULL);
	loadenv("MOONWM_DECORHINTS",	NULL,	&decorhints,		NULL);
	loadenv("MOONWM_GAPS",			NULL,	&enablegaps,		NULL);
	loadenv("MOONWM_KEYS",			NULL,	&managekeys,		NULL);
	loadenv("MOONWM_RESIZEHINTS",	NULL,	&resizehints,		NULL);
	loadenv("MOONWM_SHOWBAR",		NULL,	&showbar,			NULL);
	loadenv("MOONWM_SMARTGAPS",		NULL,	&smartgaps,			NULL);
	loadenv("MOONWM_SWALLOW",		NULL,	&swallowdefault,	NULL);
	loadenv("MOONWM_SYSTRAY",		NULL,	&showsystray,		NULL);
	loadenv("MOONWM_TOPBAR",		NULL,	&topbar,			NULL);
	loadenv("MOONWM_WORKSPACES",	NULL,	&workspaces,		NULL);
	loadenv("MOONWM_BORDERWIDTH",	NULL,	NULL,	&borderpx);
	loadenv("MOONWM_FRAMERATE",		NULL,	NULL,	&framerate);
	loadenv("MOONWM_GAPS",			NULL,	NULL,	&gappih);
	loadenv("MOONWM_GAPS",			NULL,	NULL,	&gappiv);
	loadenv("MOONWM_GAPS",			NULL,	NULL,	&gappoh);
	loadenv("MOONWM_GAPS",			NULL,	NULL,	&gappov);
	loadenv("MOONWM_LAYOUT",		NULL,	NULL,	&defaultlayout);
	loadenv("MOONWM_MFACT", NULL, NULL, &imfact);
}

void
settingsxrdb(XrmDatabase db) {
	loadxres(db,	"moonwm.centeronrh",	NULL,	&centeronrh,		NULL);
	loadxres(db,	"moonwm.decorhints",	NULL,	&decorhints,		NULL);
	loadxres(db,	"moonwm.gaps",			NULL,	&enablegaps,		NULL);
	loadxres(db,	"moonwm.keys",			NULL,	&managekeys,		NULL);
	loadxres(db,	"moonwm.resizehints",	NULL,	&resizehints,		NULL);
	loadxres(db,	"moonwm.showbar",		NULL,	&showbar,			NULL);
	loadxres(db,	"moonwm.smartgaps",		NULL,	&smartgaps,			NULL);
	loadxres(db,	"moonwm.swallow",		NULL,	&swallowdefault,	NULL);
	loadxres(db,	"moonwm.systray",		NULL,	&showsystray,		NULL);
	loadxres(db,	"moonwm.topbar",		NULL,	&topbar,			NULL);
	loadxres(db,	"moonwm.workspaces",	NULL,	&workspaces,		NULL);
	loadxres(db,	"moonwm.borderwidth",	NULL,	NULL,	&borderpx);
	loadxres(db,	"moonwm.framerate",		NULL,	NULL,	&framerate);
	loadxres(db,	"moonwm.gaps",			NULL,	NULL,	&gappih);
	loadxres(db,	"moonwm.gaps",			NULL,	NULL,	&gappiv);
	loadxres(db,	"moonwm.gaps",			NULL,	NULL,	&gappoh);
	loadxres(db,	"moonwm.gaps",			NULL,	NULL,	&gappov);
	loadxres(db,	"moonwm.layout",		NULL,	NULL,	&defaultlayout);
	loadxres(db,	"moonwm.mfact",			NULL, NULL, &imfact);
}

void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;

	/* clean up any zombies immediately */
	sigchld(0);

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	drw = drw_create(dpy, screen, root, sw, sh);
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h;
	bh = drw->fonts->h + 2;
	updategeom();
	/* init atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	wmatom[WMChangeState] = XInternAtom(dpy, "WM_CHANGE_STATE", False);
	wmatom[WMWindowRole] = XInternAtom(dpy, "WM_WINDOW_ROLE", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
	netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
	netatom[NetSystemTrayOrientationHorz] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDock] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetWMWindowTypeDesktop] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
	netatom[NetWMMaximizedVert] = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	netatom[NetWMMaximizedHorz] = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	netatom[NetClientListStacking] = XInternAtom(dpy, "_NET_CLIENT_LIST_STACKING", False);
	netatom[NetCurrentDesktop] = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
	netatom[NetDesktopNames] = XInternAtom(dpy, "_NET_DESKTOP_NAMES", False);
	netatom[NetDesktopViewport] = XInternAtom(dpy, "_NET_DESKTOP_VIEWPORT", False);
	netatom[NetNumberOfDesktops] = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
	netatom[NetWMActionClose] = XInternAtom(dpy, "_NET_WM_ACTION_CLOSE", False);
	netatom[NetWMDemandsAttention] = XInternAtom(dpy, "_NET_WM_DEMANDS_ATTENTION", False);
	netatom[NetWMDesktop] = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
	netatom[NetWMMoveResize] = XInternAtom(dpy, "_NET_WM_MOVE_RESIZE", False);
	xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
	xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
	xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
	motifatom = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
	mwmatom[MWMClientTags] = XInternAtom(dpy, "_MWM_CLIENT_TAGS", False);
	mwmatom[MWMCurrentTags] = XInternAtom(dpy, "_MWM_CURRENT_TAGS", False);
	mwmatom[MWMClientMonitor] = XInternAtom(dpy, "_MWM_CLIENT_MONITOR", False);
	mwmatom[SteamGame] = XInternAtom(dpy, "STEAM_GAME", False);
	/* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 9);
	/* init system tray */
	updatesystray();
	/* init bars */
	updatebars();
	updatestatus();
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "moonwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	/* load properties from last session */
	loadwmprops();
	setnumdesktops();
	updatecurrenttags();
	setdesktopnames();
	setviewport();
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	XDeleteProperty(dpy, root, netatom[NetClientListStacking]);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys();
	focus(NULL);
}

void
setviewport(void){
	int nmons = 0;
	for (Monitor *m = mons; m; m = m->next) {
		nmons++;
	}

	long data[nmons * 2];

	Monitor *m = mons;
	for (int i = 0; i < nmons; i++) {
		data[i*2] = (long)m->mx;
		data[i*2+1] = (long)m->my;
		m = m->next;
	}

	XChangeProperty(dpy, root, netatom[NetDesktopViewport], XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)data, nmons * 2);
}

void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	CMASKSETTO(c, M_URGENT, urg);
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
shiftview(const Arg *arg) {
	Arg shifted;

	if(arg->i > 0) // left circular shift
		shifted.ui = (selmon->tagset[selmon->seltags] << arg->i)
		   | (selmon->tagset[selmon->seltags] >> (LENGTH(tags) - arg->i));

	else // right circular shift
		shifted.ui = selmon->tagset[selmon->seltags] >> (- arg->i)
		   | selmon->tagset[selmon->seltags] << (LENGTH(tags) + arg->i);

	view(&shifted);
}

void
shiftviewclients(const Arg *arg)
{
	Arg shifted;
	Client *c;
	unsigned int tagmask = 0;

	for (c = selmon->clients; c; c = c->next)
		tagmask = tagmask | c->tags;

	shifted.ui = selmon->tagset[selmon->seltags];
	if (arg->i > 0) // left circular shift
		do {
			shifted.ui = (shifted.ui << arg->i)
			   | (shifted.ui >> (LENGTH(tags) - arg->i));
		} while (tagmask && !(shifted.ui & tagmask));
	else // right circular shift
		do {
			shifted.ui = (shifted.ui >> (- arg->i)
			   | shifted.ui << (LENGTH(tags) + arg->i));
		} while (tagmask && !(shifted.ui & tagmask));

	view(&shifted);
}

void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* show clients top down */
		map(c, 1);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		unmap(c, 1);
	}
}

void
sigchld(int unused)
{
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		die("can't install SIGCHLD handler:");
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
spawn(const Arg *arg)
{
	spawncmd(arg);
}

pid_t
spawncmd(const Arg *arg)
{
	pid_t pid;
	int now = time(NULL);
	if (arg->v == dmenucmd)
		dmenumon[0] = '0' + selmon->num;
	else if (arg->v == statushandler) {
		if (now != -1 && now - istatustimer <= istatustimeout)
			return ~0;
		char strstatuscmdn[8];
		sprintf(strstatuscmdn, "%i", statuscmdn);
		setenv("BUTTON", lastbutton, 1);
		setenv("STATUSCMDN", strstatuscmdn, 1);
	}
	if ((pid = fork()) == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		fprintf(stderr, "moonwm: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
	return pid;
}

int
stackpos(const Arg *arg) {
	int n, i;
	Client *c, *l;

	if(!selmon->clients)
		return -1;

	if(arg->i == PREVSEL) {
		for(l = selmon->stack; l && (!ISVISIBLE(l) || l == selmon->sel); l = l->snext);
		if(!l)
			return -1;
		for(i = 0, c = selmon->clients; c != l; i += ISVISIBLE(c) ? 1 : 0, c = c->next);
		return i;
	}
	else if(ISINC(arg->i)) {
		if(!selmon->sel)
			return -1;
		for(i = 0, c = selmon->clients; c != selmon->sel; i += ISVISIBLE(c) ? 1 : 0, c = c->next);
		for(n = i; c; n += ISVISIBLE(c) ? 1 : 0, c = c->next);
		return MOD(i + GETINC(arg->i), n);
	}
	else if(arg->i < 0) {
		for(i = 0, c = selmon->clients; c; i += ISVISIBLE(c) ? 1 : 0, c = c->next);
		return MAX(i + arg->i, 0);
	}
	else
		return arg->i;
}

void
tag(const Arg *arg)
{
	if (selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
		updateclienttags(selmon->sel);
		focus(NULL);
		arrange(selmon);
	}
}

void
tagmon(const Arg *arg)
{
	if (!selmon->sel || !mons->next)
		return;
	sendmon(selmon->sel, dirtomon(arg->i), 0);
}

void
tagmonkt(const Arg *arg)
{
	if (!selmon->sel || !mons->next)
		return;
	sendmon(selmon->sel, dirtomon(arg->i), 1);
}

void
togglebar(const Arg *arg)
{
	selmon->showbar = !selmon->showbar;
	updatebarpos(selmon);
	resizebarwin(selmon);
	if (showsystray) {
		XWindowChanges wc;
		if (!selmon->showbar)
			wc.y = -bh;
		else if (selmon->showbar) {
			wc.y = 0;
			if (!selmon->topbar)
				wc.y = selmon->mh - bh;
		}
		XConfigureWindow(dpy, systray->win, CWY, &wc);
	}
	arrange(selmon);
}

void
togglefloating(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (selmon->sel->isfullscreen) /* no support for fullscreen windows */
		return;
	CMASKSETTO(selmon->sel, M_FLOATING, !CMASKGET(selmon->sel, M_FLOATING) || CMASKGET(selmon->sel, M_FIXED));
	if (CMASKGET(selmon->sel, M_FLOATING) && selmon->lt[selmon->sellt]->arrange)
		/* restore last known float dimensions */
		resize(selmon->sel, selmon->sel->sfx, selmon->sel->sfy,
			   selmon->sel->sfw, selmon->sel->sfh, borderpx, 0);
	else {
		/* save last known float dimensions */
		selmon->sel->sfx = selmon->sel->x;
		selmon->sel->sfy = selmon->sel->y;
		selmon->sel->sfw = selmon->sel->w;
		selmon->sel->sfh = selmon->sel->h;
	}
	arrange(selmon);
}

void
togglefullscr(const Arg *arg)
{
  if(selmon->sel)
	setfullscreen(selmon->sel, !selmon->sel->isfullscreen);
}

void
togglelayout(const Arg *arg)
{
	layout(arg, 1);
}

void
toggletag(const Arg *arg)
{
	unsigned int newtags;

	if (!selmon->sel)
		return;
	if (workspaces)
		return;
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	if (newtags) {
		selmon->sel->tags = newtags;
		updateclienttags(selmon->sel);
		focus(NULL);
		arrange(selmon);
	}
	updatecurrenttags();
}

void
toggleview(const Arg *arg)
{
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);
	int i;

	if (workspaces)
		return;

	if (newtagset) {
		if (newtagset == ~0) {
			selmon->pertag->prevtag = selmon->pertag->curtag;
			selmon->pertag->curtag = 0;
		}
		/* test if the user did not select the same tag */
		if (!(newtagset & 1 << (selmon->pertag->curtag - 1))) {
			selmon->pertag->prevtag = selmon->pertag->curtag;
			for (i=0; !(newtagset & 1 << i); i++) ;
			selmon->pertag->curtag = i + 1;
		}
		selmon->tagset[selmon->seltags] = newtagset;

		/* apply settings for this view */
		selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
		selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
		selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
		selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
		selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];
		focus(NULL);
		arrange(selmon);
	}
	dropfullscr(selmon, 1, NULL);
	updatecurrenttags();
}

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

void
unmanage(Client *c, int destroyed)
{
	int di;
	unsigned int dui;
	Monitor *m = c->mon;
	Window dummy, win;
	XWindowChanges wc;

	if (c->swallowing) {
		unswallow(c);
		return;
	}

	Client *s = swallowingclient(c->win);
	if (s) {
		free(s->swallowing);
		s->swallowing = NULL;
		arrange(m);
		focus(NULL);
		return;
	}

	detach(c);
	detachstack(c);
	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy); /* avoid race conditions */
		XSetErrorHandler(xerrordummy);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);

	if (!s) {
		ignorewarp = 1;
		arrange(m);
		/* focus under mouse instead of last focused */
		if (XQueryPointer(dpy, root, &dummy, &win, &di, &di, &di, &di, &dui) && win)
			focus(wintoclient(win));
		else
			focus(NULL);
		updateclientlist();
	}
}

void
unmap(Client *c, int iconify)
{
	static XWindowAttributes ra, ca;

	if (!c)
		return;

	XGrabServer(dpy);
	XGetWindowAttributes(dpy, root, &ra);
	XGetWindowAttributes(dpy, c->win, &ca);
	// prevent UnmapNotify events
	XSelectInput(dpy, root, ra.your_event_mask & ~SubstructureNotifyMask);
	XSelectInput(dpy, c->win, ca.your_event_mask & ~StructureNotifyMask);
	XUnmapWindow(dpy, c->win);
	if (iconify)
		setclientstate(c, IconicState);
	XSelectInput(dpy, root, ra.your_event_mask);
	XSelectInput(dpy, c->win, ca.your_event_mask);
	XUngrabServer(dpy);
}

void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
	}
	else if ((c = wintosystrayicon(ev->window))) {
		/* KLUDGE! sometimes icons occasionally unmap their windows, but do
		 * _not_ destroy them. We map those windows back */
		XMapRaised(dpy, c->win);
		updatesystray();
	}
}

void
updatebars(void)
{
	unsigned int w;
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};
	XClassHint ch = {"moonwm", "moonwm"};
	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		w = m->ww;
		if (showsystray && m == systraytomon(m))
			w -= getsystraywidth();
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, w, bh, 0, DefaultDepth(dpy, screen),
				CopyFromParent, DefaultVisual(dpy, screen),
				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
	   	XChangeProperty(dpy, m->barwin, netatom[NetWMWindowType], XA_ATOM, 32,
				PropModeReplace, (unsigned char *) & netatom[NetWMWindowTypeDock], 1);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
		if (showsystray && m == systraytomon(m))
			XMapRaised(dpy, systray->win);
		XMapRaised(dpy, m->barwin);
		XSetClassHint(dpy, m->barwin, &ch);
	}
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (m->showbar) {
		m->wh -= bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		m->wy = m->topbar ? m->wy + bh : m->wy;
	} else
		m->by = -bh;
}

void
updateclientlist()
{
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
				XA_WINDOW, 32, PropModeAppend,
				(unsigned char *) &(c->win), 1);

	XDeleteProperty(dpy, root, netatom[NetClientListStacking]);
	for (m = mons; m; m = m->next)
		for (c = m->stack; c; c = c->snext)
			XChangeProperty(dpy, root, netatom[NetClientListStacking],
					XA_WINDOW, 32, PropModeAppend,
					(unsigned char *) &(c->win), 1);
}

void
updateclientmonitor(Client *c) {
    unsigned long data[1];
    data[0] = c->mon->num;
    XChangeProperty(dpy, c->win, mwmatom[MWMClientMonitor],
            XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, 1);
}

void
updateclienttags(Client *c) {
    unsigned long data[1];
	int i;
	for(i = 0; !(c->tags & (1 << i)); i++);
    data[0] = i;
    XChangeProperty(dpy, c->win, netatom[NetWMDesktop],
            XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, 1);
	data[0] = c->tags;
    XChangeProperty(dpy, c->win, mwmatom[MWMClientTags],
            XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, 1);
}

void
updatecurrenttags(void){
    unsigned long data[1];
	int i;
	for(i = 0; !(selmon->tagset[selmon->seltags] & (1 << i)); i++);
    data[0] = i;
	XChangeProperty(dpy, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
	data[0] = selmon->tagset[selmon->seltags];
	XChangeProperty(dpy, root, mwmatom[MWMCurrentTags], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}

int
updategeom(void)
{
	int dirty = 0;

#ifdef XINERAMA
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;
		if (n <= nn) { /* new monitors available */
			for (i = 0; i < (nn - n); i++) {
				for (m = mons; m && m->next; m = m->next);
				if (m)
					m->next = createmon();
				else
					mons = createmon();
			}
			for (i = 0, m = mons; i < nn && m; m = m->next, i++)
				if (i >= n
				|| unique[i].x_org != m->mx || unique[i].y_org != m->my
				|| unique[i].width != m->mw || unique[i].height != m->mh)
				{
					dirty = 1;
					m->num = i;
					m->mx = m->wx = unique[i].x_org;
					m->my = m->wy = unique[i].y_org;
					m->mw = m->ww = unique[i].width;
					m->mh = m->wh = unique[i].height;
					updatebarpos(m);
				}
		} else { /* less monitors available nn < n */
			for (i = nn; i < n; i++) {
				for (m = mons; m && m->next; m = m->next);
				while ((c = m->clients)) {
					dirty = 1;
					m->clients = c->next;
					detachstack(c);
					c->mon = mons;
					attachaside(c);
					attachstack(c);
					if (ISFLOATING(c))
						resize(c, mons->wx + (c->x - m->wx), mons->wy + (c->y - m->wy), c->w, c->h, c->bw, 0);
				}
				if (m == selmon)
					selmon = mons;
				cleanupmon(m);
			}
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void
updatemotifhints(Client *c)
{
	Atom real;
	int format;
	unsigned char *p = NULL;
	unsigned long n, extra;
	unsigned long *motif;
	int width, height;

	if (!decorhints)
		return;

	if (XGetWindowProperty(dpy, c->win, motifatom, 0L, 5L, False, motifatom,
						   &real, &format, &n, &extra, &p) == Success && p != NULL) {
		motif = (unsigned long*)p;
		if (motif[MWM_HINTS_FLAGS_FIELD] & MWM_HINTS_DECORATIONS) {
			width = WIDTH(c);
			height = HEIGHT(c);

			if (motif[MWM_HINTS_DECORATIONS_FIELD] & MWM_DECOR_ALL ||
				motif[MWM_HINTS_DECORATIONS_FIELD] & MWM_DECOR_BORDER ||
				motif[MWM_HINTS_DECORATIONS_FIELD] & MWM_DECOR_TITLE)
				c->bw = c->oldbw = borderpx;
			else
				c->bw = c->oldbw = 0;

			resize(c, c->x, c->y, width - (2*c->bw), height - (2*c->bw), c->bw, 0);
		}
		XFree(p);
	}
}

void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	CMASKSETTO(c, M_FIXED, (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh));
}

void
updatestatus(void)
{
	Monitor* m;
	int now;
	if (!gettextprop(root, XA_WM_NAME, rawstext, sizeof(rawstext)))
		strcpy(stext, "moonwm-"VERSION);
	else {
		now = time(NULL);
		if (strncmp(istatusclose, rawstext, strlen(istatusclose)) == 0) {
			istatustimer = 0;
			return;
		} else if (strncmp(istatusprefix, rawstext, strlen(istatusprefix)) == 0) {
			istatustimer = now;
			copyvalidchars(stext, rawstext + sizeof(char) * strlen(istatusprefix) );
		} else if (now == -1 || now - istatustimer > istatustimeout) {
			copyvalidchars(stext, rawstext);
		} else
			return;
	}
	for (m = mons; m; m = m->next) {
		drawbar(m);
	}
	updatesystray();
}

void
updatesystrayicongeom(Client *i, int w, int h)
{
	int bw = 0;
	if (i) {
		i->h = bh;
		if (w == h)
			i->w = bh;
		else if (h == bh)
			i->w = w;
		else
			i->w = (int) ((float)bh * ((float)w / (float)h));
		applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), &bw, False);
		/* force icons into the systray dimensions if they don't want to */
		if (i->h > bh) {
			if (i->w == i->h)
				i->w = bh;
			else
				i->w = (int) ((float)bh * ((float)i->w / (float)i->h));
			i->h = bh;
		}
	}
}

void
updatesystrayiconstate(Client *i, XPropertyEvent *ev)
{
	long flags;
	int code = 0;

	if (!showsystray || !i || ev->atom != xatom[XembedInfo] ||
			!(flags = getatomprop(i, xatom[XembedInfo], xatom[XembedInfo])))
		return;

	if (flags & XEMBED_MAPPED && !i->tags) {
		i->tags = 1;
		code = XEMBED_WINDOW_ACTIVATE;
		XMapRaised(dpy, i->win);
		setclientstate(i, NormalState);
	}
	else if (!(flags & XEMBED_MAPPED) && i->tags) {
		i->tags = 0;
		code = XEMBED_WINDOW_DEACTIVATE;
		XUnmapWindow(dpy, i->win);
		setclientstate(i, WithdrawnState);
	}
	else
		return;
	sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
			systray->win, XEMBED_EMBEDDED_VERSION);
}

void
updatesystray(void)
{
	XSetWindowAttributes wa;
	XWindowChanges wc;
	Client *i;
	Monitor *m = systraytomon(NULL);
	unsigned int x = m->mx + m->mw;
	unsigned int w = 1;

	if (!showsystray)
		return;
	if (!systray) {
		/* init systray */
		if (!(systray = (Systray *)calloc(1, sizeof(Systray))))
			die("fatal: could not malloc() %u bytes\n", sizeof(Systray));
		systray->win = XCreateSimpleWindow(dpy, root, x, m->by, w, bh, 0, 0, scheme[SchemeHigh][ColStatusBg].pixel);
		wa.event_mask        = ButtonPressMask | ExposureMask;
		wa.override_redirect = True;
		wa.background_pixel  = scheme[SchemeNorm][ColStatusBg].pixel;
		XSelectInput(dpy, systray->win, SubstructureNotifyMask);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *)&netatom[NetSystemTrayOrientationHorz], 1);
		XChangeWindowAttributes(dpy, systray->win, CWEventMask|CWOverrideRedirect|CWBackPixel, &wa);
		XMapRaised(dpy, systray->win);
		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
		if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
			sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
			XSync(dpy, False);
		}
		else {
			fprintf(stderr, "moonwm: unable to obtain system tray.\n");
			free(systray);
			systray = NULL;
			return;
		}
	}
	for (w = 0, i = systray->icons; i; i = i->next) {
		/* make sure the background color stays the same */
		wa.background_pixel  = scheme[SchemeNorm][ColStatusBg].pixel;
		XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
		XMapRaised(dpy, i->win);
		w += systrayspacing;
		i->x = w;
		XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
		w += i->w;
		if (i->mon != m) {
			i->mon = m;
			updateclientmonitor(i);
		}
	}
	w = w ? w + systrayspacing : 1;
	x -= w;
	XMoveResizeWindow(dpy, systray->win, x, m->by, w, bh);
	wc.x = x; wc.y = m->by; wc.width = w; wc.height = bh;
	wc.stack_mode = Above; wc.sibling = m->barwin;
	XConfigureWindow(dpy, systray->win, CWX|CWY|CWWidth|CWHeight|CWSibling|CWStackMode, &wc);
	XMapWindow(dpy, systray->win);
	XMapSubwindows(dpy, systray->win);
	/* redraw background */
	XSetForeground(dpy, drw->gc, scheme[SchemeNorm][ColStatusBg].pixel);
	XFillRectangle(dpy, systray->win, drw->gc, 0, 0, w, bh);
	XSync(dpy, False);
}

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState], XA_ATOM);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType], XA_ATOM);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		CMASKSET(c, M_FLOATING);
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			CMASKSETTO(c, M_URGENT, (wmh->flags & XUrgencyHint) ? 1 : 0);
		if (wmh->flags & InputHint)
			CMASKSETTO(c, M_NEVERFOCUS, !wmh->input);
		else
			CMASKUNSET(c, M_NEVERFOCUS);
		XFree(wmh);
	}
}

void
view(const Arg *arg)
{
	int i;
	unsigned int tmptag;

	if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
		return;
	selmon->seltags ^= 1; /* toggle sel tagset */
	if (arg->ui & TAGMASK) {
		selmon->pertag->prevtag = selmon->pertag->curtag;
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
		if (arg->ui == ~0)
			selmon->pertag->curtag = 0;
		else {
			for (i=0; !(arg->ui & 1 << i); i++) ;
			selmon->pertag->curtag = i + 1;
		}
	} else {
		tmptag = selmon->pertag->prevtag;
		selmon->pertag->prevtag = selmon->pertag->curtag;
		selmon->pertag->curtag = tmptag;
	}
	selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
	selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
	selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
	selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
	selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];
	focus(NULL);
	arrange(selmon);
	dropfullscr(selmon, 1, NULL);
	updatecurrenttags();
}

pid_t
winpid(Window w)
{
	pid_t result = 0;

	xcb_res_client_id_spec_t spec = {0};
	spec.client = w;
	spec.mask = XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID;

	xcb_generic_error_t *e = NULL;
	xcb_res_query_client_ids_cookie_t c = xcb_res_query_client_ids(xcon, 1, &spec);
	xcb_res_query_client_ids_reply_t *r = xcb_res_query_client_ids_reply(xcon, c, &e);

	if (!r)
		return (pid_t)0;

	xcb_res_client_id_value_iterator_t i = xcb_res_query_client_ids_ids_iterator(r);
	for (; i.rem; xcb_res_client_id_value_next(&i)) {
		spec = i.data->spec;
		if (spec.mask & XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID) {
			uint32_t *t = xcb_res_client_id_value_value(i.data);
			result = *t;
			break;
		}
	}

	free(r);

	if (result == (pid_t)-1)
		result = 0;
	return result;
}

pid_t
getparentprocess(pid_t p)
{
	unsigned int v = 0;

#ifdef __linux__
	FILE *f;
	char buf[256];
	snprintf(buf, sizeof(buf) - 1, "/proc/%u/stat", (unsigned)p);

	if (!(f = fopen(buf, "r")))
		return 0;

	fscanf(f, "%*u %*s %*c %u", &v);
	fclose(f);
#endif /* __linux__*/

	return (pid_t)v;
}

int
isdescprocess(pid_t p, pid_t c)
{
	while (p != c && c != 0)
		c = getparentprocess(c);

	return (int)c;
}

Client *
termforwin(const Client *w)
{
	Client *c;
	Monitor *m;

	if (!w->pid || w->isterminal)
		return NULL;

	for (m = mons; m; m = m->next) {
		for (c = m->clients; c; c = c->next) {
			if (c->isterminal && !c->swallowing && c->pid && isdescprocess(c->pid, w->pid))
				return c;
		}
	}

	return NULL;
}

Client *
swallowingclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next) {
		for (c = m->clients; c; c = c->next) {
			if (c->swallowing && c->swallowing->win == w)
				return c;
		}
	}

	return NULL;
}

void
warp(const Client *c, int edge)
{
	int x, y;

	if (!c) {
		XWarpPointer(dpy, None, root, 0, 0, 0, 0, selmon->wx + selmon->ww/2, selmon->wy + selmon->wh/2);
		return;
	}

	if (!getrootptr(&x, &y) ||
		(x > c->x - c->bw &&
		 y > c->y - c->bw &&
		 x < c->x + c->w + c->bw*2 &&
		 y < c->y + c->h + c->bw*2) ||
		(y > c->mon->by && y < c->mon->by + bh) ||
		(c->mon->topbar && !y))
		return;

	if (edge)
		XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w, c->h);
	else
		XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w / 2, c->h / 2);
}

Client *
wintoclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->win == w)
				return c;
	return NULL;
}

Client *
wintosystrayicon(Window w) {
	Client *i = NULL;

	if (!showsystray || !w)
		return i;
	for (i = systray->icons; i && i->win != w; i = i->next) ;
	return i;
}

Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		if (w == m->barwin)
			return m;
	if ((c = wintoclient(w)))
		return c->mon;
	return selmon;
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
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	die("moonwm: another window manager is already running");
	return -1;
}

Monitor *
systraytomon(Monitor *m) {
	Monitor *t;
	int i, n;
	if(!systraypinning) {
		if(!m)
			return selmon;
		return m == selmon ? m : NULL;
	}
	for(n = 1, t = mons; t && t->next; n++, t = t->next) ;
	for(i = 1, t = mons; t && t->next && i < systraypinning; i++, t = t->next) ;
	if(systraypinningfailfirst && n < systraypinning)
		return mons;
	return t;
}

void
xrdb(const Arg *arg)
{
	XrmDatabase dpydb;
	char *temp = XResourceManagerString(dpy);
	if (temp != NULL) {
		dpydb = XrmGetStringDatabase(temp);
		if (dpydb)
			loadxrdb(dpydb);
		XrmDestroyDatabase(dpydb);
	}
	/* int i; */
	/* for (i = 0; i < LENGTH(colors); i++) */
	/* 	scheme[i] = drw_scm_create(drw, colors[i], 9); */
	focus(NULL);
	arrange(NULL);
}

void
zoom(const Arg *arg)
{
	Client *c = selmon->sel;

	if (!selmon->lt[selmon->sellt]->arrange
	|| (selmon->sel && CMASKGET(selmon->sel, M_FLOATING)))
		return;
	if (c == nexttiled(selmon->clients))
		if (!c || !(c = nexttiled(c->next)))
			return;
	pop(c);
}

int
main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("moonwm-"VERSION);
	else if (argc != 1)
		die("usage: moonwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("moonwm: cannot open display");
	if (!(xcon = XGetXCBConnection(dpy)))
		die("moonwm: cannot get xcb connection\n");
	checkotherwm();
	XrmInitialize();
	settings();
	setup();
#ifdef __OpenBSD__
	if (pledge("stdio rpath proc exec", NULL) == -1)
		die("pledge");
#endif /* __OpenBSD__ */
	scan();
	runautostart();
	run();
	cleanup();
	XCloseDisplay(dpy);
	if (restartwm) {
		execvp(argv[0], argv);
	} else if (restartlauncher) {
		execvp(launcherargs[0], launcherargs);
	}
	return EXIT_SUCCESS;
}
