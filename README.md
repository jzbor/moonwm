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
geoclue
i3lock
kdeconnect
network-manager-applet
nextcloud
otf-nerd-fonts-fira-code
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
