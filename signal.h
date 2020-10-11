void
setlayoutex(const Arg *arg) {
	setlayout(&((Arg) { .v = &layouts[arg->i] }));
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
	{ "focusmon",       focusmon },
	{ "focusstack",     focusstack },
	{ "incnmaster",     incnmaster },
	{ "killclient",     killclient },
	{ "quit",           quit },
	{ "setlayout",      setlayout },
	{ "setlayoutex",    setlayoutex },
	{ "setmfact",       setmfact },
	{ "shiftview",      shiftview },
	{ "tag",            tag },
	{ "tagall",         tagall },
	{ "tagex",          tagex },
	{ "tagmon",         tagmon },
	{ "togglebar",      togglebar },
	{ "togglefloating", togglefloating },
	{ "togglefullscr",  togglefullscr },
	{ "toggletag",      tag },
	{ "toggletagex",    toggletagex },
	{ "toggleview",     view },
	{ "toggleviewex",   toggleviewex },
	{ "view",           view },
	{ "viewall",        viewall },
	{ "viewex",         viewex },
	{ "zoom",           zoom },
};
