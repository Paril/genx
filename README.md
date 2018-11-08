GenerationsX
=====

While I wait for Generations:Arena 1.0 to get released in 2028, I wanted to make something to tide me over in the mean time. I started work on this project many years ago, and hearing that Q2PRO got some new commits got me interested in updating that old project to the new commits and mess around with it some more.

It's a fairly modified Q2PRO engine, and game DLL with the following games available as classes:
* Quake 2
* Quake 1
* Doom
* Duke3D

It's also all fully legal:tm:, using an asset compiler to pull over and convert the content from other games (ie, you must own the original games for the mod to work).

All of the monsters in Q1 are fully implemented and working (in fact, you can play the full Quake 1 campaign recompiled as Q2 BSP inside this mod!), and many from Doom are implemented as well.

The concept was originally just a Deathmatch-style mod like the other Generations things (even has bots for testing this), but eventually I decided on an invasion-style game where you fend off hordes of monsters. Might end up just supporting both, not sure entirely yet.

I also want to expand the game selection to a few more, like Heretic, Shadow Warrior, and Blood. The project files are not currently uploaded (+ the compilers required to play the other games), but I'll have them here soonish.

Q2PRO
=====

Q2PRO is an enhanced, multiplayer oriented Quake 2 client and server.

Features include:

* rewritten OpenGL renderer optimized for stable FPS
* enhanced client console with persistent history
* ZIP packfiles (.pkz), JPEG and PNG textures, MD3 models
* fast HTTP downloads
* multichannel sound using OpenAL
* recording from demos, forward and backward seeking
* server side multiview demos and GTV capabilities

For building Q2PRO, consult the INSTALL file.

For information on using and configuring Q2PRO, refer to client and server
manuals available in doc/ subdirectory.
