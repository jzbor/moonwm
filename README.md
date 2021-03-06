<!-- START doctoc.sh generated TOC please keep comment here to allow auto update -->
<!-- DO NOT EDIT THIS SECTION, INSTEAD RE-RUN doctoc.sh TO UPDATE -->
**Table of Contents**

- [Setup](#setup)
- [Setting up different screen layouts](#setting-up-different-screen-layouts)
- [Dependencies](#dependencies)
- [Customizing](#customizing)
- [Setting up your own status](#setting-up-your-own-status)
- [Default tags](#default-tags)
- [Patches implemented](#patches-implemented)
- [Available layouts](#available-layouts)

<!-- END doctoc.sh generated TOC please keep comment here to allow auto update -->
# MoonWM

## Setup
You should set up the following environment variables in your `.profile`.
Customize them to your personal needs (fill in you terminal etc.).
```sh
export BROWSER="firefox"
export FILEBROWSER="pcmanfm"
export TERMINAL="alacritty"
export MOONWM_KEYMAP="us"
export MOONWM_WALLPAPER="~/path/to/wallpaper.jpg"
```
You should also be able to add `setxbkmap` options to your keyboard layout like this:
```sh
export MOONWM_KEYMAP="us,de -option -option grp:lalt_switch"
```
You can disable certain autostarts of the `moonwm-util` **autostart** routine.
This is useful if you for example have your own wrapper scripts or other replacements:
```sh
export MOONWM_NODUNST=1        # disables dunst
export MOONWM_NOPICOM=1        # disables picom
export MOONWM_NOSTATUS=1       # disables status
```
To set your **application launcher** (like dmenu or rofi) simply put a script called `dmenucmd` in your $PATH (e.g. `~/.local/bin`).
For example:
```
#!/bin/sh
rofi -combi-modi drun,window,ssh -show combi
```
If you wish to run any of your own scripts: `~/.local/share/moonwm/autostart.sh` and `~/.local/share/moonwm/autostart_blocking.sh` are run on each startup if available and executable.

**Note:** To make xmenu fit you should add in the xresources for xmenu as shown below in "Customizing" (`foreground`, `background`, `xmenu.*`).


## Setting up different screen layouts
`moonwm-util` arranges monitors next to each other with their native resolution by default.
However this may not be suitable for all situations.
To create custom layouts use the graphical tool `arandr`.
It lets you save a layout in `~/.screenlayouts` from where you can load your layouts either with `arandr` or with dmenu/rofi by pressing `Mod+Shift+r`.
To trigger the default `moonwm-util` layout (for example when you connect a new display) you can use the keyboard shortcut `Mod+r`.


## Dependencies
All packages are listed with their names in the Arch or Arch User Repositories.

**These are the ones required by the MoonWM build itself:**
```
slop
xmenu
xorg-xsetroot
```
**These are the ones the `moonwm-util` script uses, starts or other programs I deem essential for a working desktop interface:**
```
cal
dmenu
ffmpeg
geoclue
i3lock
imagemagick
kdeconnect
libnotify
light
network-manager-applet
otf-nerd-fonts-fira-code
pamixer
picom
polkit-gnome
redshift
wmname
xfce4-power-manager
xorg-setxkbmap
xorg-xrandr
xorg-xrdb
xwallpaper
```

`touchegg` also gets started if available, but it only really makes sense if you set it up properly.
Same goes for `nextcloud`. If you have it installed it gets started.
You may want to use `rofi-dmenu` as a provider for `dmenu` if you use rofi.


## Customizing
Custom values for colors and some other properties can be set via `xrdb(1)`.
To edit the design simply add/change these values in your `~/.Xresources`:
```xrdb
moonwm.focusedBorder:      #ebdbb2
moonwm.focusedTitleBg:     #1d2021
moonwm.focusedTitleFg:     #ebdbb2
moonwm.menuBg:             #fb4934
moonwm.menuFg:             #1d2021
moonwm.occupiedTagBg:      #1d2021
moonwm.occupiedTagFg:      #ebdbb2
moonwm.statusBg:           #1d2021
moonwm.statusFg:           #ebdbb2
moonwm.unfocusedBorder:    #1d2021
moonwm.unfocusedTitleBg:   #1d2021
moonwm.unfocusedTitleFg:   #7c6f64
moonwm.vacantTagBg:        #1d2021
moonwm.vacantTagFg:        #7c6f64
```

You should also edit the according xmenu and general entries to get everything to fit together:
```xrdb
*background:            #1d2021
*foreground:            #ebdbb2

xmenu.border:           #1d2021
xmenu.selbackground:    #ebdbb2
xmenu.selforeground:    #1d2021
```


## Setting up your own status
To setup your own status command you should first set the according env variables in your `~/.profile`:
```sh
export MOONWM_NOSTATUS=1
export MOONWM_STATUSHANDLER="/path/to/my/statushandler"
```
You can then add your status script to autostart (`~/.local/share/moonwm/autostart.sh`).
I should set the string you want as name of the X11 root window like this:
```sh
xsetroot -name "$(get_status)"
```
In this example `get_status` is a function that prints the current status to stdout.
You can also define clickable blocks actions delimited by ascii chars that are smaller than space.
For example:
```
date: 11:05 |\x01 volume: 55% |\x02 cpu-usage: 21% ||
```
It is totally up to you how you generate this string, but you can take a look at the reference implementation in `moonwm-util`.
Once you press one of the blocks the status handler script will be called with the according mouse button set as `$BUTTON` and the block number set as `$STATUSCMDN`.
It is also up to you how you wish to handle them, but a reference exists in `moonwm-util`


## Default tags
Some applications have default  tags they open on:
```
5:      Jetbrains IDEA
7:      Discord, Teamspeak
8:      Spotify
9:      Thunderbird
```
If you want alternative replacements added to the rules please tell me.


## Patches implemented
* actualfullscreen
* alpha (fixborders)
* attachaside
* autostart
* centeredwindowname
* cyclelayouts
* deck
* deck-tilegap
* decorhints
* dwmc
* ewmhtags
* fixborders
* focusdir
* focusonnetactive
* layoutmenu
* pertag
* placemouse
* riodraw
* restart
* savefloats
* shiftview/shiftviewclients
* stacker
* statusallmons
* statuscmd
* swallow
* systray
* vanitygaps
* warp
* xrdb


## Available layouts
* bstack
* centeredfloatingmaster
* centeredmaster
* deck
* dwindle
* fibonacci
* spiral
* tile
* tileleft

