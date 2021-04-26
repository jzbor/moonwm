static const Rule rules[] = {
    /* xprop(1):
     *    WM_CLASS(STRING) = instance, class
     *    WM_NAME(STRING) = title
     */
    /* class     instance  title           tags mask  isfloating  isterminal  noswallow  monitor */
    { "firefox", NULL,     NULL,           0,         0,          0,           1,        -1 },
    { "polybar", NULL,     NULL,           0,         0,          0,           1,        -1 },
    /* Terminals */
    { "Alacritty", NULL,   NULL,           0,         0,          1,           1,        -1 },
    { "UXterm",  NULL,     NULL,           0,         0,          1,           1,        -1 },
    /* Apps on specific tags */
    { "Spotify", NULL,     NULL,           1 << 7,    0,          0,          -1,        -1 },
    { "TeamSpeak 3", NULL, NULL,           1 << 6,    0,          0,           0,        -1 },
    { "Thunderbird", NULL, NULL,           1 << 8,    0,          0,          -1,        -1 },
    { "discord", NULL,     NULL,           1 << 6,    0,          0,           0,        -1 },
    { "jetbrains-idea", NULL, NULL,        1 << 4,    0,          0,           0,        -1 },
    { NULL,      NULL,     "win0",         1 << 4,    1,          0,           0,        -1 }, /* intellij startup */
    /* Floating */
    { "Blueman-manager", NULL, NULL,       0,         1,          0,           0,        -1 },
    { "Nm-connection-editor", NULL, NULL,  0,         1,          0,           0,        -1 },
    { "Pavucontrol", NULL, NULL,           0,         1,          0,           0,        -1 },
    { "XClock",  NULL,     NULL,           0,         1,          0,           1,        -1 },
    { NULL,      NULL,     "Event Tester", 0,         1,          0,           1,        -1 }, /* xev */
    { NULL,      NULL,     "[debug]",      0,         1,          0,           1,        -1 }, /* personal debugging */
    /* Non-Floating */
    { "Gimp",    NULL,     NULL,           0,         0,          0,           0,        -1 },
    /* Games */
    { "Steam",   NULL,     NULL,           1 << 5,    0,          0,           0,        -1 },
    { "steam_app_", NULL,  NULL,           1 << 5,    0,          0,           0,        0 },
    { "csgo_linux64", NULL, NULL,          1 << 5,    0,          0,           0,        0 },
};
