# My build of dwm

## Setup
You should set up the following environment variables in your `.profile`.
Customize them to your personal needs (fill in you terminal etc.).
```sh
export BROWSER="firefox"
export FILEBROWSER="pcmanfm"
export TERMINAL="alacritty"
export DWM_KEYMAP="us"
export DWM_WALLPAPER="~/path/to/wallpaper.jpg"
```
You can disable certain autostarts of the `dwm-util` **autostart** routine.
This is useful if you for example have your own wrapper scripts or other replacements:
```sh
export DWM_NODUNST=1        # disables dunst
export DWM_NOPICOM=1        # disables picom
export DWM_NOSTATUS=1       # disables status
```
To set your **application launcher** (like dmenu or rofi) simply put a script called `dmenucmd` in your $PATH.
For example:
```
#!/bin/sh
rofi -combi-modi drun,window,ssh -show combi
```
If you wish to run any of your own scripts: `~/.local/share/dwm/autostart.sh` and `~/.local/share/dwm/autostart_blocking.sh` are run on each startup if available and executable.

## Dependencies
All packages are listed with their names in the Arch or Arch User Repositories

**These are the ones required by the dwm build:**
```
slop
xmenu
xorg-xsetroot
```
**These are the ones the `dwm-util` script uses, starts or other programs I deem essential for a working desktop interface:**
```
cal
ffmpeg
geoclue
i3lock
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

## Customizing
Custom values for colors and some other properties can be set via `xrdb(1)`.
To edit the design simply add/change these values in your `~/.Xresources`:
```xrdb
dwm.focusedBorder:	    #ebdbb2
dwm.focusedTitleBg:	    #1d2021
dwm.focusedTitleFg:	    #ebdbb2
dwm.menuBg:	            #fb4934
dwm.menuFg:	            #1d2021
dwm.occupiedTagBg:	    #1d2021
dwm.occupiedTagFg:	    #ebdbb2
dwm.statusBg:	        #1d2021
dwm.statusFg:	        #ebdbb2
dwm.unfocusedBorder:	#1d2021
dwm.unfocusedTitleBg:	#1d2021
dwm.unfocusedTitleFg:	#7c6f64
dwm.vacantTagBg:	    #1d2021
dwm.vacantTagFg:	    #7c6f64
```

You should also edit the according xmenu and general entries to get everything to fit together:
```xrdb
*background:	        #1d2021
*foreground:	        #ebdbb2

xmenu.border:	        #1d2021
xmenu.selbackground:	#ebdbb2
xmenu.selforeground:	#1d2021
```

## Setting up your own status:
To setup your own status command you should first set the according env variables in your `~/.profile`:
```sh
export DWM_NOSTATUS=1
export DWMSTATUSHANDLER="/path/to/my/statushandler"
```
You can then add your status script to autostart (`~/.local/share/dwm/autostart.sh`).
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
It is totally up to you how you generate this string, but you can take a look at the reference implementation in `dwm-util`.
Once you press one of the blocks the status handler script will be called with the according mouse button set as `$BUTTON` and the block number set as `$STATUSCMDN`.
It is also up to you how you wish to handle them, but a reference exists in `dwm-util`


## Default tags:
Some applications have default  tags they open on:
```
5:      Jetbrains IDEA
7:      Discord, Teamspeak
8:      Spotify
9:      Thunderbird
```
If you want alternative replacements added to the rules please tell me.


## Patches implemented:
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

## Available layouts:
* bstack
* centeredfloatingmaster
* centeredmaster
* deck
* dwindle
* fibonacci
* spiral
* tile
* tileleft

### Fuck nazis. Fuck white supremacists. Fuck [them](https://mobile.twitter.com/kuschku/status/1156488420413362177).
