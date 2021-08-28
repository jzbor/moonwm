![](moonwm.png)
# MoonWM

<!-- START doctoc.sh generated TOC please keep comment here to allow auto update -->
<!-- DO NOT EDIT THIS SECTION, INSTEAD RE-RUN doctoc.sh TO UPDATE -->
**Table of Contents**

- [Usage](#usage)
  - [Most important shortcuts](#most-important-shortcuts)
  - [Quick help](#quick-help)
  - [Setting up multiple monitors](#setting-up-multiple-monitors)
- [Customizing](#customizing)
  - [Environment Variables](#environment-variables)
  - [Configuration File (X Resources)](#configuration-file-x-resources)
    - [Window Manager Settings](#window-manager-settings)
    - [Colors](#colors)
  - [Autostart](#autostart)
  - [Creating your own status script](#creating-your-own-status-script)
  - [Add favorites menu](#add-favorites-menu)
- [Details](#details)
  - [Default tags](#default-tags)
  - [Available layouts](#available-layouts)
  - [Dependencies](#dependencies)
  - [Patches implemented](#patches-implemented)
- [Forking](#forking)
- [Credits](#credits)

<!-- END doctoc.sh generated TOC please keep comment here to allow auto update -->


**Screenshot:** ![Screenshot](screenshot.png)

**Packages:** `aur/moonwm`, `aur/moonwm-git`

**Installation from source:**
```sh
sudo make install install-scripts clean
```

## Usage

### Most important shortcuts
* `MOD + d` **Open Application**
* `MOD + 1-9` **Switch Tag**
* `MOD + Shift + q` **Close Application**
* `MOD + F11` **Help Menu (see below)**

Use **hjkl** or **arrow keys** for `<direction>`:
* `MOD + <direction>` **Focus Client**
* `MOD + Shift + <direction>` **Move Client**
* `MOD + Control + <direction>` **Resize Client**

### Quick help
**For a quick interactive help page press MOD(Win)+F11.
You will get a interactive list with shortcuts and their corresponding actions.
You can also directly select most of the entries to execute their action.**

### Setting up multiple monitors
Keep in mind that MoonWM is not able to automatically add newly connected monitors.
There are two ways to setup your monitors:

1. Manual:
    * Use `arandr` to configure your screens and save them.
    * They will be saved in `~/.screenlayouts`
    * Any layout called `default.sh` will be automatically loaded on startup.
    * You can use the layout menu `Mod+Control+m` to select a layout.
2. Automatically:
    * There are four different modes: `internal`, `external`, `extend` and `mirror`
    * On startup `external` will be used if `default.sh` is not defined.
    * You can use `Mod+Shift+m` to cycle through the four different modes.
    * You can also use `moonwm-util monitors {internal,external,extend,mirror}` to set a mode.

**NOTE**: `moonwm-util screensetup` and the usage of `~/.screenlayouts/autoload.sh` are both deprecated.
Use `moonwm-util monitors` and `~/.screenlayouts/default.sh` instead.

## Customizing
MoonWM brings defaults for everything.
It automatically searches for stuff like the default browser or terminal.
Please let me know if your favorite browser or terminal does not get recognised yet.
Of course if you dislike the defaults you can always adjust the looks and the behavior as well as default applications.
While styling and window manager settings are managed via the config file some things like default applications have to be set via environment variables.

### Environment Variables
These are the settings that can be adjusted via environment variables.
Note that as stated above none of these is strictly necessary for MoonWM to run.
The values below are not necessarily the defaults but rather examples.
If you wonder where they go take a look at `~/.profile`.

```sh
# default applications
BROWSER="firefox"
FILEMANAGER="pcmanfm"
TERMINAL="alacritty"
DMENUCMD="rofi -show drun"
# other basic settings
MOONWM_KEYMAP="us"
MOONWM_WALLPAPER="~/path/to/wallpaper.jpg"
MOONWM_THEMEDDMENU=1            # automatic dmenu theming
MOONWM_THEMEDXMENU=1            # automatic xmenu theming
TOUCHEGG_THRESHOLD="750 750"    # if you are using touchegg
```

To even further customize your keyboard you can put a file with `xmodmap` expressions in `~/.config/moonwm/modmap`.
It will be evaluated automatically.
Or you can add `setxkbmap` options to your configuration like so:
```sh
MOONWM_KEYMAP="us,de -option -option grp:lalt_switch"
```

You can disable or customize certain autostarts of the `moonwm-util` **autostart** routine.
This is useful if you for example have your own wrapper scripts or other replacements:
```sh
MOONWM_PICOMEXP=0       # start picom with --experimental-backends flag
MOONWM_NOTIFYD=1        # enables/disables the notification daemon
MOONWM_PICOM=1          # enables/disables picom
```

### Configuration File (X Resources)
The following options should be set in the config file in `~/.config/moonwm/config.xres`.
Their format is the same as the one used by the `.Xresources` file.

#### Window Manager Settings
With these settings you can turn features on or off (listed with their default values):
```yaml
moonwm.keys:        1   # enable/disable internal moonwm key handling
moonwm.workspaces:  0   # use workspaces like i3 instead of tags (experimental)
moonwm.swallow:     1   # enable/disable swallowing
# bar
moonwm.showbar:     1   # show a bar
moonwm.systray:     1   # show system tray icons
moonwm.topbar:      1   # place bar at the top or bottom
# clients
moonwm.smartgaps:   1   # disable gaps when only one client is visible
moonwm.resizehints: 0   # let clients choose their size when tiled
moonwm.centeronrh:  0   # if resizehints applies, center the window
moonwm.decorhints:  1   # decoration hints (MOTIF)
```

You can also customize these settings (also listed with their defaults), which all take unsigned integer arguments:
```yaml
moonwm.layout:      0   # initial default layout
moonwm.borderwidth: 5   # width of the window borders
moonwm.framerate:   60  # frame rate when dragging windows; should be >= monitor refresh rate
moonwm.gaps:        5   # gaps; 0 to disable gaps
moonwm.mfact:       55  # master size ratio; must be between 5 and 95
```

#### Colors
These are the color values you can customize:
```yaml
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
```yaml
*background:            #1d2021
*foreground:            #ebdbb2

xmenu.border:           #1d2021
xmenu.selbackground:    #ebdbb2
xmenu.selforeground:    #1d2021
```

_Note that configuration via the `.Xresources` file or similar is also possible, although the config file mentioned above is preffered.
To enable styling of other applications `~/.Xresources` is loaded on startup._


### Autostart

On login or reload `moonwm-util` starts a bunch of useful programs by default.
You can find a list of "essential" utilities that are started automatically [below](#Dependencies).
In addition these programs get started, provided you have installed them:
```
nextcloud       # cloud for your data
redshift        # make screen less blue at night time
touchegg        # touch gestures
kdeconnect      # interaction with your mobile phone
```

If you wish to run any of your own scripts: `~/.local/share/moonwm/autostart.sh` and `~/.local/share/moonwm/autostart_blocking.sh` are run on each startup if available and executable.

### Creating your own status script
The built-in `moonwm-status` script should be a good foundation for making your own statuscmd.
It is easily extensible and you can simply add blocks with `add_block` in the `get_status` function.
Make sure to escape '%' though, as it is interpreted by printf as escape sequence.

To setup your own status command you should also set the according env variable in your `~/.profile`:
```sh
export MOONWM_STATUSCMD="/path/to/my/statuscmd"
```
This command gets asynchonously on MoonWMs startup with `loop` as the first parameter.
It should then repeatedly set the status to the WM_NAME (for example with `moonctl status`).
Make sure to add in a `sleep` so it doesn't unnecessarily wastes resources.

You can also define clickable blocks actions delimited by ascii chars that are smaller than space.
For example:
```
date: 11:05 |\x01 volume: 55% |\x02 cpu-usage: 21% ||
```
Once you press one of the blocks the statuscmd script will be called with `action` as its first parameter.
The according mouse button will be set as `$BUTTON` and the block number as `$STATUSCMDN`.

The standard MoonWM status interface also includes the `update` parameter, which tells the bar to immediately refresh.
`status` as first parameter prints out the current statusline to stdout.

### Add favorites menu
You can put a file in the `xmenu(1)` format in `~/.config/moonwm/favorites`.
To open this menu middle click the menu button on the bar.

Example:
```
# vim: set noet:
  Terminal			$TERMINAL
  Web				$BROWSER
  Media			mpv
  Files			$FILEMANAGER
ﭮ  Discord			discord
ﭮ  Discord [c]		discord-canary
  Spotify			if pidof spotify; then moonctl toggleview $((1<<7)); else spotify; fi
  Mail				thunderbird
```


## Details

### Default tags
Some applications have default  tags they open on:
```
5:      Jetbrains IDEA
7:      Discord, Teamspeak
8:      Spotify
9:      Thunderbird
```
If you want alternative replacements added to the rules please tell me.

### Available layouts
* bstack
* centeredfloatingmaster (disabled)
* centeredmaster (disabled)
* deck
* dwindle (disabled)
* fibonacci (disabled)
* spiral (disabled)
* tile
* tileleft

### Dependencies
All packages are listed with their names in the Arch or Arch User Repositories.

**These are the ones required by the MoonWM build itself:**
```
libx11
libxcb
libxinerama
slop
xmenu
```
**These are the ones the `moonwm-util` script uses, starts or other programs I deem essential for a working desktop interface:**
```
arandr          # gui to setup screens
dmenu           # (application) menu
ffmpeg          # playing sounds
geoclue         # location services
i3lock          # screen locking
imagemagick     # lock screen generation
libnotify       # desktop notifications
light           # change background light of screen
network-manager-applet      # network management
otf-nerd-fonts-fira-code    # default font
pamixer         # volume manipulation
picom           # compositor
polkit-gnome    # security policy agent
sound-theme-freedesktop     # sounds
xfce4-power-manager         # power management
xorg-setxkbmap  # set keyboard layout
xorg-xrandr     # setup screens
xorg-xrdb       # interaction with xres database
xwallpaper      # set wallpaper
```
You may want to use `rofi-dmenu` as a provider for `dmenu` if you use rofi.

The `Fira Code Nerd Font` is available [here](https://github.com/ryanoasis/nerd-fonts/blob/v2.1.0/patched-fonts/FiraCode/Regular/complete/Fira%20Code%20Regular%20Nerd%20Font%20Complete.ttf)

### Patches implemented
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


## Forking
You are encouraged to fork this project.
At the moment `config.def.h` might be a little outdated, so you are probably better off just editing `config.h`.
However please consider the project is not "suckless" neither is that its goal.
MoonWM should mostly be compatible wtih dwm patches although the huge amount of patches already applied make it a little hard and due to the code being relocated to `src/` you will have to patch them manually.


## Credits
* to **Guzman Barquin** for the moon in the title image
