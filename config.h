/* See LICENSE file for copyright and license details. */

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
	{ "Firefox", NULL,     NULL,           1 << 8,    0,          0,          -1,        -1 },
	{ "Spotify", NULL,     NULL,           1 << 7,    0,          0,          -1,        -1 },
	{ "St",      NULL,     NULL,           0,         0,          1,           0,        -1 },
	{ "polybar", NULL,     NULL,           0,         0,          1,           1,        -1 },
	{ NULL,      NULL,     "Event Tester", 0,         0,          0,           1,        -1 }, /* xev */
};

/* layout(s) */
static float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static int nmaster     = 1;    /* number of clients in master area */
static int resizehints = 1;    /* 1 means respect size hints in tiled resizals */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
};

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
	{ MOD, XK_l,     ACTION##stack, {.i = 1 } },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = {  "/bin/sh", "-c", "rofi -config ~/.config/rofi/apps.rasi -combi-modi drun,window,ssh -show combi", NULL };
static const char *termcmd[]  = { "/bin/sh", "-c", "alacritty", NULL };

/*
 * Xresources preferences to load at startup
 */
ResourcePref resources[] = {
		{ "normbgcolor",        STRING,  &normbgcolor },
		{ "normbordercolor",    STRING,  &normbordercolor },
		{ "normfgcolor",        STRING,  &normfgcolor },
		{ "selbgcolor",         STRING,  &selbgcolor },
		{ "selbordercolor",     STRING,  &selbordercolor },
		{ "selfgcolor",         STRING,  &selfgcolor },
		{ "borderpx",          	INTEGER, &borderpx },
		{ "snap",          		INTEGER, &snap },
		{ "showbar",          	INTEGER, &showbar },
		{ "topbar",          	INTEGER, &topbar },
		{ "nmaster",          	INTEGER, &nmaster },
		{ "resizehints",       	INTEGER, &resizehints },
		{ "mfact",      	 	FLOAT,   &mfact },
};

/* commands spawned when clicking statusbar, the mouse button pressed is exported as BUTTON */
static char *statuscmds[] = { "dwmmusic.sh $BUTTON", "dwmvolume.sh $BUTTON", "dwmnetwork.sh", "dwmdate.sh" };
static char *statuscmd[] = { "/bin/sh", "-c", NULL, NULL };

static Key keys[] = {
	/* modifier                     key	    function	    argument */
	STACKKEYS(MODKEY,			    focus)
	STACKKEYS(MODKEY|ShiftMask,		    push)
	{ MODKEY,                       XK_d,	    spawn,          {.v = dmenucmd } },
	{ MODKEY,			XK_Return,  spawn,          {.v = termcmd } },
	{ MODKEY|ControlMask,           XK_h,	    setmfact,       {.f = -0.05} },
	{ MODKEY|ControlMask,           XK_l,	    setmfact,       {.f = +0.05} },
	{ MODKEY,                       XK_Tab,	    view,           {0} },
	{ MODKEY,			XK_q,	    killclient,     {0} },
	{ MODKEY,			XK_f,	    togglefullscr,  {0} },
	{ MODKEY,                       XK_t,	    setlayout,      {.v = &layouts[0]} },
	{ MODKEY|ShiftMask,             XK_f,	    setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,	    setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_space,   setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,   togglefloating, {0} },
	{ MODKEY,                       XK_comma,   focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period,  focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,   tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period,  tagmon,         {.i = +1 } },
	TAGKEYS(                        XK_1,			    0)
	TAGKEYS(                        XK_2,			    1)
	TAGKEYS(                        XK_3,			    2)
	TAGKEYS(                        XK_4,			    3)
	TAGKEYS(                        XK_5,			    4)
	TAGKEYS(                        XK_6,			    5)
	TAGKEYS(                        XK_7,			    6)
	TAGKEYS(                        XK_8,			    7)
	TAGKEYS(                        XK_9,			    8)
	{ MODKEY|ControlMask|ShiftMask, XK_Escape,  quit,           {0} },
	{ MODKEY|ShiftMask,		XK_d,	    spawn,	    SHCMD("rofi -show run") },
	{ MODKEY,			XK_Home,    spawn,	    SHCMD("menu.sh") },
	{ MODKEY,			XK_Menu,    spawn,	    SHCMD("menu.sh") },
	{ ControlMask|Mod1Mask,		XK_Delete,  spawn,	    SHCMD("menu.sh system") },
	{ MODKEY,			XK_x,	    spawn,	    SHCMD("lock.sh") },
	{ MODKEY|ShiftMask,		XK_x,	    spawn,	    SHCMD("music.sh pause && lock.sh") },
	{ 0,		XF86XK_AudioRaiseVolume,    spawn,	    SHCMD("volume.sh -i") },
	{ 0,		XF86XK_AudioLowerVolume,    spawn,	    SHCMD("volume.sh -d") },
	{ ShiftMask,	XF86XK_AudioRaiseVolume,    spawn,	    SHCMD("volume.sh -i 1") },
	{ ShiftMask,	XF86XK_AudioLowerVolume,    spawn,	    SHCMD("volume.sh -d 1") },
	{ 0,		XF86XK_AudioMute,	    spawn,	    SHCMD("volume.sh -t") },
	{ MODKEY,	XK_plus,		    spawn,	    SHCMD("volume.sh -i") },
	{ MODKEY,	XK_minus,		    spawn,	    SHCMD("volume.sh -d") },
	{ MODKEY|ShiftMask,	XK_plus,	    spawn,	    SHCMD("volume.sh -i 1") },
	{ MODKEY|ShiftMask,	XK_minus,	    spawn,	    SHCMD("volume.sh -d 1") },
	{ MODKEY,		XK_numbersign,	    spawn,	    SHCMD("volume.sh -t") },
	{ MODKEY|ShiftMask, XK_numbersign,	    spawn,	    SHCMD("volume.sh -t mic") },
	{ MODKEY|ControlMask, XK_numbersign,	    spawn,	    SHCMD("volume.sh -t mic-unmute") },
	{ 0,		XF86XK_AudioMicMute,	    spawn,	    SHCMD("volume.sh -t mic") },
	{ 0,		XF86XK_AudioPlay,		    spawn,	    SHCMD("music.sh play") },
	{ 0,		XF86XK_AudioPause,		    spawn,	    SHCMD("music.sh pause") },
	{ 0,		XF86XK_AudioPrev,		    spawn,	    SHCMD("music.sh previous") },
	{ 0,		XF86XK_AudioNext,		    spawn,	    SHCMD("music.sh next") },
	{ MODKEY,		XK_u,		    spawn,	    SHCMD("music.sh previous") },
	{ MODKEY,		XK_i,		    spawn,	    SHCMD("music.sh play-pause") },
	{ MODKEY,		XK_o,		    spawn,	    SHCMD("music.sh next") },
	{ MODKEY|ShiftMask,	XK_u,		    spawn,	    SHCMD("playerctl -p firefox previous") },
	{ MODKEY|ShiftMask,	XK_i,		    spawn,	    SHCMD("playerctl -p firefox play-pause") },
	{ MODKEY|ShiftMask,	XK_o,		    spawn,	    SHCMD("playerctl -p firefox next") },
	{ 0,		XF86XK_MonBrightnessUp,	    spawn,	    SHCMD("backlight.sh -inc 10") },
	{ 0,		XF86XK_MonBrightnessDown,    spawn,	    SHCMD("backlight.sh -dec 10") },
	{ MODKEY|ControlMask,	XK_plus,	    spawn,	    SHCMD("backlight.sh -inc 10") },
	{ MODKEY|ControlMask,	XK_minus,	    spawn,	    SHCMD("backlight.sh -dec 10") },
	{ MODKEY|ShiftMask,	XK_w,		    spawn,	    SHCMD("wifi.sh toggle") },
	{ MODKEY|ShiftMask,	XK_b,		    spawn,	    SHCMD("bluetooth.sh toggle") },
	{ MODKEY|ShiftMask,	XK_e,		    spawn,	    SHCMD("ethernet.sh toggle") },
	{ MODKEY,		XK_s,		    spawn,	    SHCMD("setup_displays.sh") },
	{ MODKEY,		XK_Print,	    spawn,	    SHCMD("sleep 0.2; scrot.sh") },
	{ MODKEY|ShiftMask,	XK_Print,	    spawn,	    SHCMD("sleep 0.2; scrot.sh focused") },
	{ MODKEY|ControlMask,	XK_Print,	    spawn,	    SHCMD("sleep 0.2; scrot.sh screen") },
	// Programs
	{ MODKEY,		XK_w,		    spawn,	    SHCMD("firefox") },
	{ MODKEY,		XK_b,		    spawn,	    SHCMD("pcmanfm") },
	{ MODKEY,		XK_BackSpace,	    spawn,	    SHCMD("terminal.sh nvim") },
	// Rofi scripts
	{ MODKEY|ShiftMask,	XK_m,		    spawn,	    SHCMD("rofi-mount.sh mnt") },
	{ MODKEY|ControlMask,	XK_m,		    spawn,	    SHCMD("rofi-mount.sh umnt") },
	{ MODKEY|ShiftMask,	XK_s,		    spawn,	    SHCMD("rofi-unicode.sh insert") },
	{ MODKEY|ControlMask,	XK_s,		    spawn,	    SHCMD("rofi-unicode.sh") },
	{ MODKEY|ShiftMask,	XK_c,		    spawn,	    SHCMD("rofi-projects.sh") },
	{ MODKEY,		XK_Delete,	    spawn,	    SHCMD("rofi-kill.sh") },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button1,        spawn,          {.v = statuscmd } },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = statuscmd } },
	{ ClkStatusText,        0,              Button3,        spawn,          {.v = statuscmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

