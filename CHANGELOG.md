# MoonWM - CHANGELOG

<!-- START doctoc.sh generated TOC please keep comment here to allow auto update -->
<!-- DO NOT EDIT THIS SECTION, INSTEAD RE-RUN doctoc.sh TO UPDATE -->
**Table of Contents**

- [Upcoming](#upcoming)
- [7.0.3](#703)
- [7.0.2](#702)
- [7.0.1](#701)
- [7.0.0](#700)

<!-- END doctoc.sh generated TOC please keep comment here to allow auto update -->

## Upcoming
* Help messages for menu
* Improved EWMH support
* Keybinding to focus floating/tiled windows
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
