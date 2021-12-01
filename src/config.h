/* See LICENSE file for copyright and license details. */
/* vim: set noet: */

# define Button8 8
# define Button9 9
# define DynamicModifier    (1<<16)

/* appearance */
static unsigned int borderpx        = 2;        /* border pixel of windows */
static const unsigned int snap      = 16;       /* snap pixel */
static unsigned int framerate       = 60;       /* fps to render while moving windows */
static int swallowdefault			= 1;        /* 1 means swallow windows by default */
static const int swallowfloating    = 0;        /* 1 means swallow floating windows by default */
static unsigned int gappih          = 5;        /* horiz inner gap between windows */
static unsigned int gappiv          = 5;        /* vert inner gap between windows */
static unsigned int gappoh          = 5;        /* horiz outer gap between windows and screen edge */
static unsigned int gappov          = 5;        /* vert outer gap between windows and screen edge */
static int smartgaps                = 1;        /* 1 means no outer gap when there is only one window */
static unsigned int defaultlayout	= 0;        /* index of the default layout */
static const unsigned int systraypinning = 1;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static int showsystray      = 1;	/* 0 means no systray */
static int showbar          = 1;    /* 0 means no bar */
static int topbar           = 1;    /* 0 means bottom bar */
static int centerspawned	= 0;	/* center newly spawned windows */
static int usefocusdir		= 1;	/* enable/disable focusdir */
static int usemovedir		= 1;	/* enable/disable movedir */
static const int spawnedoffset		= 100;		/* offset for new windows from cursor */
static const char floatingindicator[] = "[ %s ]";           /* mark floating windows in title bar */
static const char istatusprefix[]   = "important:";         /* prefix for important status messages */
static const char istatusclose[]    = "important:close";    /* prefix to close important status messages */
static const int istatustimeout     = 10;       /* max timeout before displaying regular status after istatus */
static const int riodraw_borders    = 0;        /* 0 or 1, indicates whether the area drawn using slop includes the window borders */
static const int riodraw_matchpid   = 0;        /* 0 or 1, indicates whether to match the PID of the client that was spawned with riospawn */
static const int riodraw_spawnasync = 1;        /* 0 means that the application is only spawned after a successful selection while
												 * 1 means that the application is being initialised in the background while the selection is made */
static const int center_relbar		= 1;		/* 1 means centering applications relative to the bar when using center() or spawning new windows */
static int wraparound				= 0;		/* wrap around screenedges in focusdir, movedir */
static int workspaces               = 0;
static int userules					= 1;		/* 1 means don't apply any rules */
static int usetagrules				= 0;		/* 1 means don't apply any tag-specific rules */
static int managekeys = 1;
static const int placemousemode     = 1;
	/* placemouse options, choose which feels more natural:
	 *    0 - tiled position is relative to mouse cursor
	 *    1 - tiled position is relative to window center
	 *    2 - mouse pointer warps to window center
	 */
static const char *fonts[]          = { "FiraCode Nerd Font:size=10", "monospace:size=10" };
static const char dmenufont[]       = "monospace:size=10";
static char menulabel[]            = "::::::";
static char normtagfg[]             = "#7c6f64";
static char normtagbg[]             = "#1d2021";
static char normtitlefg[]           = "#7c6f64";
static char normtitlebg[]           = "#1d2021";
static char statusfg[]              = "#ebdbb2";
static char statusbg[]              = "#1d2021";
static char normborderfg[]          = "#1d2021";
static char menufg[]                = "#1d2021";
static char menubg[]                = "#fb4934";

static char hightagfg[]             = "#ebdbb2";
static char hightagbg[]             = "#1d2021";
static char hightitlefg[]           = "#ebdbb2";
static char hightitlebg[]           = "#1d2021";
static char highborderfg[]          = "#ebdbb2";
static char winselection[]          = "#fb4934";
static char areaselection[]         = "#ebdbb2";

static char *colors[][9] = {
       /*               fg           bg           border   */
       [SchemeNorm] = { normtagfg, normtagbg, normtitlefg, normtitlebg, statusfg,
           statusbg, normborderfg, menufg, menubg},
       [SchemeHigh] = { hightagfg, hightagbg, hightitlefg, hightitlebg, statusfg,
           statusbg, highborderfg, winselection, areaselection},
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
/* static const char *tags[] = { "", "", "", "", "", "", "‭ﭮ", "", "" }; */

/* layout(s) */
#define FORCE_VSPLIT 1  /* nrowgrid layout: force two clients to always split vertically */
static float mfact           = 0.55; /* factor of master area size [0.05..0.95] */
static const float dmfnuance = 0.75; /* nuance for dragmfact */
static const float dcfnuance = 4.0;	 /* nuance for dragcfact */
static const int nmaster     = 1;    /* number of clients in master area */
static int resizehints       = 0;    /* 1 means respect size hints in tiled resizals */
static int centeronrh        = 0;    /* 1 means if sizehints are respected center the window */
static int decorhints        = 1;    /* 1 means respect decoration hints */
#include "layouts.h"

static const Layout layouts[] = {
    /* symbol     arrange function */
    { "[]=",    tile },            /* Default: Master on left, stack on right */
    { "[]D",    deck },            /* Master on left,laves in monocle-like mode on right */

    { "[M]",    monocle },         /* All windows on top of eachother */
    { "><>",    NULL },            /* no layout function means floating behavior */

    { "=[]",    tileleft },        /* Tile, but with switched sides */
    { "TTT",    bstack },          /* Master on top, stack on bottom */

    { "HHH",    gaplessgrid },     /* All clients in a (gapless) grid */
    /* { "===",    horizgrid },       /1* same as gaplessgrid, but horizontal *1/ */

    /* { "[@]",    spiral },          /1* Fibonacci spiral *1/ */
    /* { "[\\]",    dwindle },        /1* Decreasing in size right and leftward *1/ */

    /* { "|M|",    centeredmaster },            /1* Master in middle, stack on sides *1/ */
    /* { ">M>",    centeredfloatingmaster },    /1* Same but master floats *1/ */

    { NULL,        NULL },
};

#define TILEPOS     0
#define DECKPOS     1
#define MONOCLEPOS  2
#define FLOATPOS    3
#define TILELEFTPOS 4
#define BSTACKPOS   5
#define GRIDPOS     6

/* static const Layout cyclablelayouts[] = { */
/*     layouts[0], layouts[8], layouts[4], layouts[5], */
/*     layouts[9], /1* needs to be the last layouts[] element *1/ */
/* }; */

/* signals */
#include "signal.h"

/* key definitions */
/* #define MODKEY Mod4Mask */
#define MODKEY DynamicModifier
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
#define DIRECTIONKEY(KEY, AXIS, ARG) \
    { MODKEY|ShiftMask,      KEY,    move##AXIS,          ARG }, \
    { MODKEY|ControlMask,    KEY,    resize##AXIS,        ARG },
#define FOCUSKEYS(LEFT, DOWN, UP, RIGHT) \
    { MODKEY,    LEFT,      focusdir,        {.i = 0} }, \
    { MODKEY,    RIGHT,     focusdir,        {.i = 1} }, \
    { MODKEY,    UP,        focusdir,        {.i = 2} }, \
    { MODKEY,    DOWN,      focusdir,        {.i = 3} },


/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static const char *termcmd[]  = { "/bin/sh", "-c", "$TERMINAL", NULL };
static const char *layoutmenu_cmd = "moonwm-menu layouts-classic";

/* this script or program gets called for bar button presses
 * the button is exported as BUTTON the according module as STATUSCMDN (a number)*/
static char *statushandler[] = { "/bin/sh", "-c", "$STATUSCMD action", NULL };
// this command gets executed on startup and should set you status to WM_NAME
static char *statuscmd[] = { "/bin/sh", "-c", "$STATUSCMD", NULL };

/* most keys are specified in <X11/keysymdef.h> */
static Key keys[] = {
    /* modifier             key            function        argument */
    /* TAG AND MONITOR MANAGEMENT */
    TAGKEYS(                XK_1,                        0)
    TAGKEYS(                XK_2,                        1)
    TAGKEYS(                XK_3,                        2)
    TAGKEYS(                XK_4,                        3)
    TAGKEYS(                XK_5,                        4)
    TAGKEYS(                XK_6,                        5)
    TAGKEYS(                XK_7,                        6)
    TAGKEYS(                XK_8,                        7)
    TAGKEYS(                XK_9,                        8)
	{ MODKEY,               XK_0,       view,            {.ui = ~0 } },
	{ MODKEY|ShiftMask,     XK_0,       tag,             {.ui = ~0 } },
    { MODKEY,               XK_Tab,     view,            {0} },
    { MODKEY|ShiftMask,     XK_Tab,     shiftviewclients, {.i = +1 } },
    { MODKEY|ControlMask,   XK_Tab,     shiftviewclients, {.i = -1 } },
    { MODKEY,               XK_comma,   shiftview,       {.i = -1 } },
    { MODKEY,               XK_period,  shiftview,       {.i = +1 } },
    { MODKEY,               XK_Prior,   focusmon,        {.i = -1 } },
    { MODKEY|ShiftMask,     XK_Prior,   tagmon,          {.i = -1 } },
    { MODKEY|ControlMask,   XK_Prior,   tagmonkt,        {.i = -1 } },
    { MODKEY,               XK_Next,    focusmon,        {.i = +1 } },
    { MODKEY|ShiftMask,     XK_Next,    tagmon,          {.i = +1 } },
    { MODKEY|ControlMask,   XK_Next,    tagmonkt,        {.i = +1 } },
    /* WINDOW MANAGEMENT */
    FOCUSKEYS(XK_h, XK_j, XK_k, XK_l)
    FOCUSKEYS(XK_Left, XK_Down, XK_Up, XK_Right)
    DIRECTIONKEY(XK_h, x, {.i = -20})
    DIRECTIONKEY(XK_j, y, {.i = -20})
    DIRECTIONKEY(XK_k, y, {.i = 20})
    DIRECTIONKEY(XK_l, x, {.i = 20})
    DIRECTIONKEY(XK_Left,  x, {.i = -20})
    DIRECTIONKEY(XK_Down,  y, {.i = -20})
    DIRECTIONKEY(XK_Up,    y, {.i = 20})
    DIRECTIONKEY(XK_Right, x, {.i = 20})
    { MODKEY,               XK_r,       rioresize,       {0} },
    /* { MODKEY|ShiftMask,     XK_r,       resetfacts,      { .i = 1 } }, */
    { MODKEY|ShiftMask,     XK_r,       resetfacts,      {0} },
    /* { MODKEY,               XK_space,   spawn,           SHCMD("pidof skippy-xd || skippy-xd > /dev/null 2>&1") }, */
    { MODKEY,               XK_space,   focusstack,      {.i = INC(+1) } },
    { MODKEY|ShiftMask,     XK_q,       killclient,      {0} },
    /* LAYOUTS */
    { MODKEY,               XK_t,       togglelayout,    {.v = &layouts[TILEPOS]} },
    { MODKEY,               XK_c,       togglelayout,    {.v = &layouts[DECKPOS]} },
    { MODKEY,               XK_m,       togglelayout,    {.v = &layouts[MONOCLEPOS]} },
    { MODKEY|ShiftMask,     XK_f,       togglelayout,    {.v = &layouts[FLOATPOS]} },
    { MODKEY|ShiftMask,     XK_t,       togglelayout,    {.v = &layouts[TILELEFTPOS]} },
    { MODKEY|ControlMask,   XK_t,       togglelayout,    {.v = &layouts[BSTACKPOS]} },
    { MODKEY,               XK_g,       togglelayout,    {.v = &layouts[GRIDPOS]} },
    { MODKEY|ShiftMask,     XK_space,   togglefloating,  {0} },
    { MODKEY,               XK_f,       togglefullscr,   {0} },
    { MODKEY,               XK_z,       center,          {0} },
    /* APPS */
    { MODKEY,               XK_Return,  spawn,           {.v = termcmd } },
    { MODKEY,               XK_w,       spawn,           SHCMD("$BROWSER") },
    { 0,			XF86XK_HomePage,	spawn,           SHCMD("$BROWSER") },
    { MODKEY,               XK_b,       spawn,           SHCMD("$FILEMANAGER") },
    { 0,			XF86XK_MyComputer,	spawn,           SHCMD("$FILEMANAGER") },
    /* MENUS AND NOTIFICATIONS */
    { MODKEY,               XK_d,       dmenu,           {0} },
    { ControlMask|Mod1Mask, XK_Delete,  spawn,           SHCMD("moonwm-menu 3") },
    { ControlMask|Mod4Mask, XK_Delete,  spawn,           SHCMD("moonwm-menu 3") },
    /* MEDIA AND BRIGHTNESS CONTROL */
    { 0,    XF86XK_AudioLowerVolume,    spawn,           SHCMD("wmc-utils volume -5") },
    { 0,    XF86XK_AudioRaiseVolume,    spawn,           SHCMD("wmc-utils volume +5") },
    { 0,    XF86XK_AudioMute,           spawn,           SHCMD("wmc-utils volume mute") },
    { 0,    XF86XK_AudioPrev,           spawn,           SHCMD("playerctl previous") },
    { 0,    XF86XK_AudioPlay,           spawn,           SHCMD("playerctl play-pause") },
    { 0,    XF86XK_AudioPause,          spawn,           SHCMD("playerctl play-pause") },
    { 0,    XF86XK_AudioPrev,           spawn,           SHCMD("playerctl next") },
    { 0,    XF86XK_MonBrightnessDown,   spawn,           SHCMD("wmc-utils brightness -5") },
    { 0,    XF86XK_MonBrightnessUp,     spawn,           SHCMD("wmc-utils brightness +5") },
    /* SCREENSHOTS */
    { MODKEY,               XK_Print,   spawn,           SHCMD("wmc-utils screenshot") },
    { MODKEY|ShiftMask,     XK_Print,   spawn,           SHCMD("wmc-utils screenshot screen") },
    { MODKEY|ControlMask,   XK_Print,   spawn,           SHCMD("wmc-utils screenshot focused") },
    /* OTHER */
    { MODKEY,               XK_x,       spawn,           SHCMD("wmc-utils lock") },
    { ControlMask|MODKEY,   XK_BackSpace, quit,          {0} },
    { 0,			XF86XK_Display,		spawn,           SHCMD("wmc-utils monitors cycle") },
    { MODKEY|ShiftMask,     XK_m,		spawn,           SHCMD("wmc-utils monitors cycle") },
    { MODKEY|ControlMask,   XK_m,		spawn,           SHCMD("wmc-utils screenlayouts") },
    /* FUNCTION KEYS */
    { MODKEY,               XK_F1,      togglebar,       {0} },
    { MODKEY,               XK_F2,      togglegaps,      {0} },
    { MODKEY,               XK_F3,      spawn,           SHCMD("wmc-utils setup-keyboard") },
    { MODKEY,               XK_F5,      spawn,           SHCMD("wmc-utils xrdb") },
    { MODKEY,               XK_F11,     spawn,           SHCMD("moonwm-helper") },
};

/* button definitions */
#define WINBUTTON(MOD, MASK, BUTTON, FUNCTION, ARGUMENT) \
    { ClkWinTitle,      MASK,       BUTTON,     FUNCTION,        ARGUMENT}, \
    { ClkWinTitle,      MOD|MASK,   BUTTON,     FUNCTION,        ARGUMENT}, \
    { ClkClientWin,     MOD|MASK,   BUTTON,     FUNCTION,        ARGUMENT},
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
    /* click                event mask      button          function        argument */
    WINBUTTON(MODKEY,       0,              Button2,        togglefloating, {0})
    WINBUTTON(MODKEY,       0,              Button4,        focusstack,     {.i = INC(-1) })
    WINBUTTON(MODKEY,       0,              Button5,        focusstack,     {.i = INC(+1) })
    WINBUTTON(MODKEY,       ShiftMask,      Button4,        pushstack,      {.i = INC(-1) })
    WINBUTTON(MODKEY,       ShiftMask,      Button5,        pushstack,      {.i = INC(+1) })
    WINBUTTON(MODKEY,       ControlMask,    Button4,        scrollresize,   {.i = -20 })
    WINBUTTON(MODKEY,       ControlMask,    Button5,        scrollresize,   {.i = 20 })
    WINBUTTON(MODKEY,       0,              Button8,        pushstack,      {.i = INC(+1) })
    WINBUTTON(MODKEY,       0,              Button9,        pushstack,      {.i = INC(-1) })
    { ClkMenu,              0,              Button1,        spawn,          SHCMD("moonwm-menu 1") },
    { ClkMenu,              0,              Button2,        spawn,          SHCMD("moonwm-menu 2") },
    { ClkMenu,              0,              Button3,        spawn,          SHCMD("moonwm-menu 3") },
    { ClkMenu,              0,              Button4,        shiftview,      {.i = -1} },
    { ClkMenu,              0,              Button5,        shiftview,      {.i = +1} },
    { ClkMenu,              0,              Button8,        spawn,          SHCMD("$TERMINAL") },
    { ClkMenu,              0,              Button9,        spawn,          SHCMD("$BROWSER") },
    { ClkTagBar,            0,              Button1,        view,           {0} },
    { ClkTagBar,            0,              Button2,        shiftviewclients, {.i = +1} },
    { ClkTagBar,            0,              Button3,        toggleview,     {0} },
    { ClkTagBar,            0,              Button4,        shiftview,      {.i = -1} },
    { ClkTagBar,            0,              Button5,        shiftview,      {.i = +1} },
    { ClkTagBar,            0,              Button8,        toggletag,      {0} },
    { ClkTagBar,            0,              Button9,        tag,            {0} },
    { ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
    { ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
    { ClkLtSymbol,          0,              Button1,        togglelayout,   {.v = &layouts[0]} },
    { ClkLtSymbol,          0,              Button2,        togglelayout,   {.v = &layouts[FLOATPOS]} },
    { ClkLtSymbol,          0,              Button3,        layoutmenu,     {0} },
    { ClkLtSymbol,          0,              Button4,        cyclelayout,    {.i = +1 } },
    { ClkLtSymbol,          0,              Button5,        cyclelayout,    {.i = -1 } },
    { ClkWinTitle,          0,              Button1,        rioresize,      {0} },
    { ClkWinTitle,          0,              Button2,        center,			{0} },
    { ClkWinTitle,          0,              Button3,        spawn,          SHCMD("moonwm-menu select") },
    { ClkStatusText,        0,              Button1,        spawn,          {.v = statushandler } },
    { ClkStatusText,        0,              Button2,        spawn,          {.v = statushandler } },
    { ClkStatusText,        0,              Button3,        spawn,          {.v = statushandler } },
    { ClkStatusText,        0,              Button4,        spawn,          {.v = statushandler } },
    { ClkStatusText,        0,              Button5,        spawn,          {.v = statushandler } },
    { ClkStatusText,        0,              Button8,        spawn,          {.v = statushandler } },
    { ClkStatusText,        0,              Button9,        spawn,          {.v = statushandler } },
    { ClkClientWin,         MODKEY,         Button1,        moveorplace,    {.i = 0} },
    { ClkClientWin,         MODKEY|ControlMask, Button1,    moveorplace,    {.i = 1} },
    { ClkClientWin,         MODKEY|ShiftMask, Button1,      rioresize,      {0} },
    { ClkClientWin,         MODKEY,         Button3,        resizeorxfact,  {0} },
    { ClkClientWin,         MODKEY|ControlMask, Button3,    resizeorxfact,	{.i = 1} },
    { ClkClientWin,         MODKEY|ShiftMask, Button3,      spawn,          SHCMD("moonwm-menu context") },
    { ClkRootWin,			0,              Button2,        spawn,			SHCMD("moonwm-desktop") },
    { ClkRootWin,           0,              Button3,        spawn,          SHCMD("moonwm-menu select") },
    { ClkRootWin,           0,              Button8,        riospawn,       SHCMD("$TERMINAL") },
    { ClkRootWin,           0,              Button9,        riospawn,       SHCMD("$BROWSER") },
    { ClkRootWin,           MODKEY,         Button9,        dmenu,          {0} },
};

