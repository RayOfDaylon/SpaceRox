# Stellar Mayhem
Extended remake of Atari Asteroids 1979 arcade game using Unreal Engine

"SpaceRox" is the internal project name.

Example Videos

Version 1.2: https://www.youtube.com/watch?v=mXKK6ThleVQ  
Version 1.4 (powerups): https://www.youtube.com/watch?v=i2CBQ7l-PXI  
Version 1.6.4 (nicer intro): https://www.youtube.com/watch?v=xYPj2w0ImMY  
Version 1.8.1 (minibosses): https://youtu.be/W9DrlSYwxMM

The game departs from Asteroids as development continues, e.g.:
- Powerups such as shields and double-guns  
- Scavenger enemies that harvest unpicked powerups
- Minibosses that are shielded

The DaylonGraphicsLibrary plugin provides generic types and functions
that can be used in other similar games, along with some widgets.
It's included directly instead of using a git submodule, so 
that if you clone or pull this repo, you'll have it without any fuss
ready to go.

Unreal Engine 5.1 or higher required.

Keyboard and gamepad input supported at present. There's also an input controller asset
that supports fightstick arcade controllers (see the docs for more info), and if you 
have an arcade cabinet or your own arcade controller with USB output, it shouldn't be hard to support that.

A 1920 x 1080 (HD) viewport is used. Monitors of different sizes and/or display scales will scale. Textures are deliberately oversized to avoid blurring when upscaled.

Game sounds may be loud. Until a volume setting is added, it's recommended to manually lower the volume in your OS speaker/headphone settings.

All graphics are done in UMG/Slate. This project was partly done to see how performant a pure UMG game could be. It was fine, but Slate is now used for all gameplay objects to be even better.

If you prefer to just download a Windows binary to play the game, one is available at https://www.daylongraphics.com/download/spacerox/SpaceRox.zip

Version: 1.8.4  
Size: 141,420,373 bytes (134 MiB)  
CRC32: D1ED1D93  
CRC64: 0012C8752E4DB68F  
SHA256: CF9996D87A3A4D1C0E90078A181F093662DFE71F2704686EF107DBD6AA06387A  
SHA1: E837A49CD23A491F48A066BFDE64BE4EC8CECA94
