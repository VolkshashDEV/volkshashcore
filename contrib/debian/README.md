
Debian
====================
This directory contains files used to package ukkeyd/ukkey-qt
for Debian-based Linux systems. If you compile ukkeyd/ukkey-qt yourself, there are some useful files here.

## ukkey: URI support ##


ukkey-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install ukkey-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your ukkey-qt binary to `/usr/bin`
and the `../../share/pixmaps/ukkey128.png` to `/usr/share/pixmaps`

ukkey-qt.protocol (KDE)

