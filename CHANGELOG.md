# MoonWM - CHANGELOG

<!-- START doctoc.sh generated TOC please keep comment here to allow auto update -->
<!-- DO NOT EDIT THIS SECTION, INSTEAD RE-RUN doctoc.sh TO UPDATE -->
**Table of Contents**

- [7.3.3](#733)
- [7.3.2](#732)
- [7.3.1](#731)
- [7.3.0](#730)
- [7.2.1](#721)
- [7.2.0](#720)
- [7.1.1](#711)
- [7.1.0](#710)
- [7.0.4](#704)
- [7.0.3](#703)
- [7.0.2](#702)
- [7.0.1](#701)
- [7.0.0](#700)

<!-- END doctoc.sh generated TOC please keep comment here to allow auto update -->

## 8.0.0
* Merging `wmcommons` back into the project
* Dropping the following functions of `moonwm-utils`:
    * `brightness`, `launch`, `monitors` `screenlayouts`, `screenstup`, `setup-keyboard`, `setwallpaper`, `start`, `volume`
* Integrating `pademelon-tools` for volume, backlight and wallpaper
* Launching status only when `MOONWM_LAUNCH_STATUS` is specified (temporary workaround)

## 7.3.3
* Fixing cursor warping
* Fixing window activating across monitors
* Fixing window placing by mouse for multiple monitors

## 7.3.2
* Respecting `_KDE_NET_WM_WINDOW_TYPE_OVERRIDE`
* Quickfix for cursor jumping

## 7.3.1
* Hotfix for wmcommons

## 7.3.0
* Initial compatibility with pademelon
* Improved resource management with lockscreen image generating
* Borderwidth now always respects smartgaps
* Pipewire for a more efficient implementation of playing sounds
* Fixed behavior for `_NET_WM_DESKTOP`

## 7.2.1
* Fixing built-in status
* Fix smartgaps for monocle layout
* Adding command to list all windows: `windows`
* Adding commands `borrow`, `steal` and `restore` to quickly get windows onto your current tags:
    * `borrow` borrows a window from its tags until it looses focus
    * `steal` permanently takes a window from its tags
    * `restore` reverts any of the two actions listed above

## 7.2.0
* Move windows with your mouse even when they are fullscreen
* Spawn floating windows where the mouse is
* Pulling windows to first monitor if theirs got disconnected
* Configuration is now done in the xres format
* Making a lot of stuff optional via options:
    * rules and tagrules
    * enable/disable NerdFont icons via env variable (`MOONWM_NERDFONT`)
* Navigation and organisation by direction:
    * added movedir
    * improved focusdir algorithm
* Support for feh as wallpaper-setter
* Improved drun (with `!` and `?` modifier)
* Improving the 'expose' feature
* Compiler optimizations enabled by default
* Splitting stuff of to [wmcommons](https://github.com/jzbor/wmcommons)

## 7.1.1
* Better implementation for the hiding of windows
* Better window centering
* Config option for a default layout
* Better multi-monitor support (`moonwm-util monitors`/`MOD+Shift+M`)
* `MOD + space` now cycles through the open windows

## 7.1.0
* Replacing moonie with moonctl (also replaces some dependencies)
* Adding an `expose` feature to replace `skippy-xd`
* Implementing cfacts
* Drag mfact/cfacts with mouse
* Additional Xorg atoms: `_MWM_CURRENT_TAGS`, `_MWM_CLIENT_TAGS` and `_MWM_CLIENT_MONITOR`
* Using _MWM_* atoms to restore state after reload

## 7.0.4
* Help messages for menu
* Improved EWMH and WindowRole/WindowType support
* Keybinding to focus floating/tiled windows
* Adding center functionality to menu
* Wallpaper can now be set with `moonwm-util setwallpaper <file>` or via your file manager
* More config options

## 7.0.3
* Grid layout
* More config options exposed via env/config variables
* Highly improved version of xdg-xmenu
* Improved layout handling in scripts
* Loading moonwm xres values as defaults for xmenu

## 7.0.2
* Changed config option names
* Config file in `~/.config/moonwm/config.env`
* Moving modmap file to `~/.config/moonwm/modmap`
* Reloading config when restarting in place
* Use picom's experimental backends (`MOONWM_PICOMEXP`)
* Multiple fixed bugs and memory leaks

## 7.0.1
* Modkey configurable via `MOONWM_MODKEY`
* Improved support for Steam (especially Proton)
* Kill menu (Mod + Shift + right click)

## 7.0.0
* Wrapper script to ensure out-of-the-box usability
* Barmenu
* Clickable statusbar
* Gaps between windows
* Rio-like resizing
* Rio-like context menu
* Terminal swallowing
* Multiple additional layouts
* Simple IPC
* Xrdb support
* Improved compatibility
* Other improvements

See also [README.md#patches-implemented](./README.md#patches-implemented).
