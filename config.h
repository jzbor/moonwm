/* See LICENSE file for copyright and license details. */

# define Button8 8
# define Button9 9

/* appearance */
static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const unsigned int snap      = 16;       /* snap pixel */
static const int swallowfloating    = 0;        /* 1 means swallow floating windows by default */
static const unsigned int gappih    = 5;       /* horiz inner gap between windows */
static const unsigned int gappiv    = 5;       /* vert inner gap between windows */
static const unsigned int gappoh    = 5;       /* horiz outer gap between windows and screen edge */
static const unsigned int gappov    = 5;       /* vert outer gap between windows and screen edge */
static const int smartgaps          = 1;        /* 1 means no outer gap when there is only one window */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const int showsystray        = 1;     /* 0 means no systray */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[]          = { "monospace:size=10" };
static const char dmenufont[]       = "monospace:size=10";
static char normbgcolor[]           = "#222222";
static char normbordercolor[]       = "#444444";
static char normfgcolor[]           = "#bbbbbb";
static char selfgcolor[]            = "#eeeeee";
static char selbordercolor[]        = "#005577";
static char selbgcolor[]            = "#005577";
static char *colors[][3] = {
       /*               fg           bg           border   */
       [SchemeNorm] = { normfgcolor, normbgcolor, normbordercolor },
       [SchemeSel]  = { selfgcolor,  selbgcolor,  selbordercolor  },
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class     instance  title           tags mask  isfloating  isterminal  noswallow  monitor */
	{ "Gimp",    NULL,     NULL,           0,         1,          0,           0,        -1 },
	{ "firefox", NULL,     NULL,           0,		  0,          0,	   1,        -1 },
	{ "Spotify", NULL,     NULL,           1 << 7,    0,          0,          -1,        -1 },
	{ "St",      NULL,     NULL,           0,         0,          1,           0,        -1 },
	{ "polybar", NULL,     NULL,           0,         0,          0,           1,        -1 },
	{ "XClock",  NULL,     NULL,           0,         1,          0,           1,        -1 },
	{ "Alacritty", NULL,   NULL,           0,         0,          1,           1,        -1 },
	{ "UXterm",	 NULL,	   NULL,           0,         0,          1,           1,        -1 },
	{ NULL,      NULL,     "Event Tester", 0,         1,          0,           1,        -1 }, /* xev */
	{ "Thunderbird", NULL, NULL,           1 << 8,    0,          0,          -1,        -1 },
	{ NULL,      NULL,     "[debug]",	   0,         1,          0,           1,        -1 }, /* personal debugging */
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 0;    /* 1 means respect size hints in tiled resizals */
#include "vanitygaps.h"

static const Layout layouts[] = {
	/* symbol     arrange function */
 	{ "[]=",	tile },			/* Default: Master on left, slaves on right */
	{ "[]D",	deck },			/* Master on left, slaves in monocle-like mode on right */

 	{ "[M]",	monocle },		/* All windows on top of eachother */
	{ "TTT",	bstack },		/* Master on top, slaves on bottom */

	{ "[@]",	spiral },		/* Fibonacci spiral */
	{ "[\\]",	dwindle },		/* Decreasing in size right and leftward */

	{ "|M|",	centeredmaster },		/* Master in middle, slaves on sides */
	{ ">M>",	centeredfloatingmaster },	/* Same but master floats */

	{ "><>",	NULL },			/* no layout function means floating behavior */
	{ NULL,		NULL },
};

/* static const Layout cyclablelayouts[] = { */
/* 	layouts[0], layouts[8], layouts[4], layouts[5], */
/* 	layouts[9], /1* needs to be the last layouts[] element *1/ */
/* }; */

/* signals */
#include "signal.h"

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },
#define STACKKEYS(MOD,ACTION) \
	{ MOD, XK_j,     ACTION##stack, {.i = INC(+1) } }, \
	{ MOD, XK_k,     ACTION##stack, {.i = INC(-1) } }, \
	{ MOD, XK_h,     ACTION##stack, {.i = 0 } }, \
	{ MOD, XK_l,     ACTION##stack, {.i = PREVSEL } },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = {  "/bin/sh", "-c", "rofi -combi-modi drun,window,ssh -show combi", NULL };
static const char *termcmd[]  = { "/bin/sh", "-c", "$TERMINAL", NULL };

/* commands spawned when clicking statusbar, the mouse button pressed is exported as BUTTON */
static char *statuscmds[] = { "tray-options.sh $BUTTON", "dwmmusic.sh $BUTTON", "dwmvolume.sh $BUTTON", "dwmnetwork.sh", "dwmdate.sh $BUTTON" };
static char *statuscmd[] = { "/bin/sh", "-c", NULL, NULL };

static Key keys[] = {
	/* modifier             key			function	    argument */
	STACKKEYS(MODKEY,						focus)
	STACKKEYS(MODKEY|ShiftMask,				push)
	/* { MODKEY,               XK_d,	    spawn,          {.v = dmenucmd } }, */
	/* { MODKEY,				XK_Return,  spawn,          {.v = termcmd } }, */
	{ MODKEY|ControlMask,   XK_h,	    setmfact,       {.f = -0.05} },
	{ MODKEY|ControlMask,   XK_l,	    setmfact,       {.f = +0.05} },
	{ MODKEY,               XK_Tab,	    view,           {0} },
	{ MODKEY,				XK_q,	    killclient,     {0} },
	{ MODKEY,				XK_f,	    togglefullscr,  {0} },
	{ MODKEY,               XK_F1,	    togglebar,      {0} },
	{ MODKEY,               XK_t,	    setlayout,      {.v = &layouts[0]} },
	{ MODKEY,               XK_c,	    setlayout,      {.v = &layouts[1]} },
	{ MODKEY,               XK_m,	    setlayout,      {.v = &layouts[2]} },
	{ MODKEY|ShiftMask,     XK_t,	    setlayout,      {.v = &layouts[3]} },
	{ MODKEY,				XK_z,	    setlayout,      {.v = &layouts[4]} },
	{ MODKEY|ShiftMask,     XK_z,	    setlayout,      {.v = &layouts[5]} },
	{ MODKEY,               XK_a,	    setlayout,      {.v = &layouts[6]} },
	{ MODKEY|ShiftMask,     XK_a,	    setlayout,      {.v = &layouts[7]} },
	{ MODKEY|ShiftMask,     XK_f,	    setlayout,      {.v = &layouts[8]} },
	{ MODKEY,               XK_space,   cyclelayout,    {.i = +1 } },
	{ MODKEY|ControlMask,   XK_space,   cyclelayout,    {.i = -1 } },
	{ MODKEY|ShiftMask,     XK_space,   togglefloating, {0} },
	{ MODKEY,               XK_comma,   focusmon,       {.i = -1 } },
	{ MODKEY,               XK_period,  focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,     XK_comma,   tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,     XK_period,  tagmon,         {.i = +1 } },
	TAGKEYS(                XK_1,						0)
	TAGKEYS(                XK_2,			    		1)
	TAGKEYS(                XK_3,			    		2)
	TAGKEYS(                XK_4,			    		3)
	TAGKEYS(                XK_5,			    		4)
	TAGKEYS(                XK_6,			    		5)
	TAGKEYS(                XK_7,			    		6)
	TAGKEYS(                XK_8,			    		7)
	TAGKEYS(                XK_9,			    		8)
	{ ControlMask|Mod1Mask,	XK_BackSpace,	quit,       {0} },
	{ MODKEY,               XK_F5,	    xrdb,           {.v = NULL } },
	{ MODKEY|ControlMask,   XK_c,	    center,			{0} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	/* tagtoleft, tagtoright */
	{ ClkTagBar,            0,              Button4,        shiftview,		{.i = -1} },
	{ ClkTagBar,            0,              Button5,        shiftview,		{.i = +1} },
	/* { ClkTagBar,            0,              Button8,        view,		{.i = -1} }, */
	/* { ClkTagBar,            0,              Button9,        view,		{.i = +1} }, */
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
	{ ClkLtSymbol,          0,              Button1,		setlayout,		{.v = &layouts[0]} },
	{ ClkLtSymbol,			0,				Button2,        setlayout,		{.v = &layouts[8]} },
	{ ClkLtSymbol,          0,              Button3,		setlayout,		{.v = &layouts[1]} },
	{ ClkLtSymbol,          0,              Button4,		cyclelayout,    {.i = +1 } },
	{ ClkLtSymbol,          0,              Button5,		cyclelayout,    {.i = -1 } },
	{ ClkWinTitle,          0,              Button1,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          MODKEY,         Button1,        spawn,          {.v = dmenucmd } },
	{ ClkWinTitle,          0,              Button2,        togglefloating,	{0} },
	{ ClkWinTitle,          MODKEY,         Button2,        killclient,     {0} },
	{ ClkWinTitle,          0,              Button3,        spawn,			SHCMD("xmenu.sh") },
	{ ClkWinTitle,          MODKEY,         Button3,        spawn,          SHCMD("menu.sh system") },
	{ ClkWinTitle,          0,              Button4,        focusstack,		{.i = INC(-1) } },
	{ ClkWinTitle,          ShiftMask,      Button4,        pushstack,		{.i = INC(-1) } },
	{ ClkWinTitle,          0,              Button5,        focusstack,		{.i = INC(+1) } },
	{ ClkWinTitle,          ShiftMask,      Button5,        pushstack,		{.i = INC(+1) } },
	{ ClkWinTitle,          0,              Button8,        spawn,			SHCMD("rofi-windows.sh") },
	{ ClkWinTitle,          MODKEY,         Button8,        spawn,          SHCMD("$TERMINAL") },
	{ ClkWinTitle,          0,              Button9,        spawn,			{.v = dmenucmd } },
	{ ClkWinTitle,          MODKEY,         Button9,        spawn,          SHCMD("firefox") },
	{ ClkStatusText,        0,              Button1,        spawn,          {.v = statuscmd } },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = statuscmd } },
	{ ClkStatusText,        0,              Button3,        spawn,          {.v = statuscmd } },
	{ ClkStatusText,        0,              Button4,        spawn,          {.v = statuscmd } },
	{ ClkStatusText,        0,              Button5,        spawn,          {.v = statuscmd } },
	{ ClkStatusText,        0,              Button8,        spawn,          SHCMD("mpv") },
	{ ClkStatusText,        0,              Button9,        spawn,          SHCMD("spotify") },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkClientWin,         Mod1Mask,       Button3,        spawn,			SHCMD("xmenu.sh") },
	{ ClkClientWin,         MODKEY,			Button4,		pushstack,		{.i = INC(-1) } },
	{ ClkClientWin,         MODKEY, 		Button5,      	pushstack,		{.i = INC(+1) } },
	{ ClkClientWin,         MODKEY, 		Button8,      	spawn,			SHCMD("rofi-windows.sh") },
	{ ClkClientWin,         MODKEY, 		Button9,      	spawn,			{.v = dmenucmd} },
	{ ClkRootWin,			0,				Button2,        setlayout,		{.v = &layouts[8]} },
	{ ClkRootWin,			0,				Button3,        spawn,			SHCMD("xmenu.sh") },
	{ ClkRootWin,			0,				Button8,        spawn,          SHCMD("$TERMINAL") },
	{ ClkRootWin,			MODKEY, 		Button8,      	spawn,			SHCMD("rofi-windows.sh") },
	{ ClkRootWin,          	0,				Button9,        spawn,          SHCMD("firefox") },
	{ ClkRootWin,			MODKEY, 		Button9,      	spawn,			{.v = dmenucmd} },
};

