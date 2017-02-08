Dyslexic Reader
===============

This is a quick reimplementation of espeak-gui.
But this time in C and with GTK3, and instead of espeak directly it uses
speechd to abstract from exact speech synthesis implementation.

It is my Dyslexic aid.

Feel free to use it and patches are most welcome.

At the moment this is purely for GNU/Linux. If there is a healthy cross
platform alternative to speechd I might switch.


Install
=======

At the moment it's just from source.

In Debian/Ubuntu/Mint to install required packages do:

    sudo apt-get install gcc make imagemagick libgtk-3-dev libspeechd-dev libgtksourceview-3.0-dev speech-dispatcher-espeak

Then to install do:

    sudo make install


Authors
=======

Joe Burmeister : joe.burmeister@excelsiorelectronics.com


License
=======

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details (file
COPYING in the root directory).

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 675 Mass
Ave, Cambridge, MA 02139, USA.
