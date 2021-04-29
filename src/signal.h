void
setlayoutex(const Arg *arg) {
    for(int i = 0; layouts[i].symbol && i <= arg->i; i++)
        if (i == arg->i) {
            setlayout(&((Arg) { .v = &layouts[arg->i] }));
            break;
        }
}

void
viewex(const Arg *arg) {
	view(&((Arg) { .ui = 1 << arg->ui }));
}

void
viewall(const Arg *arg) {
	view(&((Arg){.ui = ~0}));
}

void
toggleviewex(const Arg *arg) {
	toggleview(&((Arg) { .ui = 1 << arg->ui }));
}

void
tagex(const Arg *arg) {
	tag(&((Arg) { .ui = 1 << arg->ui }));
}

void
toggletagex(const Arg *arg) {
	toggletag(&((Arg) { .ui = 1 << arg->ui }));
}

void
tagall(const Arg *arg) {
	tag(&((Arg){.ui = ~0}));
}

/* signal definitions */
/* signum must be greater than 0 */
/* trigger signals using `xsetroot -name "fsignal:<signame> [<type> <value>]"` */
static Signal signals[] = {
	/* signum           function */
	{ "center",         center },
	{ "cyclelayout",    cyclelayout },
	{ "focusmon",       focusmon },
	{ "focusstack",     focusstack },
	{ "incnmaster",     incnmaster },
	{ "killclient",     killclient },
	{ "movex",          movex },
	{ "movey",          movey },
	{ "quit",           quit },
	{ "resizex",        resizex },
	{ "resizey",        resizey },
	{ "rioresize",      rioresize },
	{ "restart",        restart },
	{ "restartlaunched", restartlaunched },
	{ "setlayout",      setlayoutex },
	{ "setmfact",       setmfact },
	{ "shiftview",      shiftview },
	{ "shiftviewclients", shiftviewclients },
	{ "tag",            tagex },
	{ "tagall",         tagall },
	{ "tagmon",         tagmon },
	{ "tagmonkt",       tagmonkt },
	{ "togglebar",      togglebar },
	{ "togglefloating", togglefloating },
	{ "togglefullscr",  togglefullscr },
	{ "togglegaps",     togglegaps },
	{ "toggletag",      toggletagex },
	{ "toggleview",     toggleviewex },
	{ "viewall",        viewall },
	{ "view",           viewex },
	{ "xrdb",           xrdb },
	{ "zoom",           zoom },
};
