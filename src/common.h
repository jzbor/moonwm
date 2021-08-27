/* vim: set noet: */

#ifndef WMCOMMONW_COMMON_H
#define WMCOMMONW_COMMON_H

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


/* structs */
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
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

typedef struct {
	const char *class;
	const char *role;
	const char *instance;
	const char *wintype;
	const char *title;
	unsigned int tags;
	int gameid;
	int props;
	int monitor;
} Rule;

typedef struct {
	const char * sig;
	void (*func)(const Arg *);
} Signal;

typedef struct {
	Window win;
	Client *icons;
} Systray;


#endif
