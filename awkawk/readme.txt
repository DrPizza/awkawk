<(@) awkawk (@)>

Contents
--------

1) Version history
2) Bugs/issues/etc.
3) Credits
4) License

1) Version history

0.3.	Volume control, improve full-screen behaviour, begin implementing some kind of playlist and URL support, although it's not usable yet
0.0.2	Fix some multimonitor issues, begin implementing full-screen mode
0.0.1	Initial release

2) Known bugs/issues/etc.

*	It's extremely rudimentary
*	The current level of testing on ATI cards is low
*	It probably screws up multihead systems (not having one it's hard to tell)
*	Requires a video card supporting non-power of two non-square textures (life's too short to support obsolete hardware)
*	No playlists.  Implementing them actually completely sucks, due to the fact that for some files I don't know they're a playlist until I actually feed them to DirectShow.
*	I'm a complete nub at D3D and DS, and as such the code is probably rather rough
*	The icon is pissweak
*	Overlay support may be useful for "theater mode" in NVidia and ATI video cards
*	Support DVDs (use the special API)
*	Support playing from URLs (needs a little dialogue box and full playlist support)

3) Credits

With special thanks to Uijong Choi.

4) License

Copyright (C) 2006 Peter Bright

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

Peter Bright <drpizza@quiscalusmexicanus.org>
