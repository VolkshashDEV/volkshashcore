
Debian
====================
This directory contains files used to package volkshashd/volkshash-qt
for Debian-based Linux systems. If you compile volkshashd/volkshash-qt yourself, there are some useful files here.

## volkshash: URI support ##


volkshash-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install volkshash-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your volkshash-qt binary to `/usr/bin`
and the `../../share/pixmaps/volkshash128.png` to `/usr/share/pixmaps`

volkshash-qt.protocol (KDE)

