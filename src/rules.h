static const Rule rules[] = {
    /* xprop(1):
     *    WM_CLASS(STRING) = instance, class
     *    WM_NAME(STRING) = title
     */
    /* class     instance  title           tags mask  isfloating  isterminal  noswallow  monitor */
    { .class = "firefox", .monitor = -1,  .noswallow = 1 },
    { .class = "polybar", .monitor = -1,  .noswallow = 1 },
    /* Terminals */
    { .class = "Alacritty", .monitor = -1,  .isterminal = 1, .noswallow = 1 },
    { .class = "Uxterm",    .monitor = -1,  .isterminal = 1, .noswallow = 1 },
    /* Apps on specific tags */
    { .class = "Spotify",       .monitor = -1,  .tags = 1 << 7 },
    { .class = "TeamSpeak 3",   .monitor = -1,  .tags = 1 << 6 },
    { .class = "Thunderbird",   .monitor = -1,  .tags = 1 << 8 },
    { .class = "discord",       .monitor = -1,  .tags = 1 << 6 },
    { .class = "jetbrains-idea", .monitor = -1, .tags = 1 << 4 },
    { .title = "win0",          .monitor = - 1, .tags = 1 << 4, .isfloating = 1 },
    /* Floating */
    { .class = "XClock",                .monitor = -1, .isfloating = 1, .noswallow = 1 },
    { .title = "Event Tester",          .monitor = -1, .isfloating = 1, .noswallow = 1 }, /* xev */
    { .title = "[debug]",               .monitor = -1, .isfloating = 1, .noswallow = 1 }, /* personal debugging */
    /* Non-Floating */
    { .class = "Gimp",  .monitor = -1,  .isfloating = 0 },
    /* Games */
    { .class = "Steam",         .monitor = -1,  .tags = 1 << 5 },
    { .class = "steam_app_",    .monitor = 0,   .tags = 1 << 5 },
    { .gameid = -1,             .monitor = 0,   .tags = 1 << 5 },
    /* window types */
    { .wintype = "_NET_WM_WINDOW_TYPE_DIALOG",    .isfloating = 1},
    { .wintype = "_NET_WM_WINDOW_TYPE_UTILITY",   .isfloating = 1},
    { .wintype = "_NET_WM_WINDOW_TYPE_TOOLBAR",   .isfloating = 1},
    { .wintype = "_NET_WM_WINDOW_TYPE_SPLASH",    .isfloating = 1},
};
