using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace asset_compiler
{
	class Quake2
	{
		// lower bits are stronger, and will eat weaker brushes completely
		public const int CONTENTS_SOLID = 1;       // an eye is never valid in a solid
		public const int CONTENTS_WINDOW = 2;       // translucent, but not watery
		public const int CONTENTS_AUX = 4;
		public const int CONTENTS_LAVA = 8;
		public const int CONTENTS_SLIME = 16;
		public const int CONTENTS_WATER = 32;
		public const int CONTENTS_MIST = 64;
		public const int LAST_VISIBLE_CONTENTS = 64;

		// remaining contents are non-visible, and don't eat brushes

		public const int CONTENTS_AREAPORTAL = 0x8000;

		public const int CONTENTS_PLAYERCLIP = 0x10000;
		public const int CONTENTS_MONSTERCLIP = 0x20000;

		// currents can be added to any other contents, and may be mixed
		public const int CONTENTS_CURRENT_0 = 0x40000;
		public const int CONTENTS_CURRENT_90 = 0x80000;
		public const int CONTENTS_CURRENT_180 = 0x100000;
		public const int CONTENTS_CURRENT_270 = 0x200000;
		public const int CONTENTS_CURRENT_UP = 0x400000;
		public const int CONTENTS_CURRENT_DOWN = 0x800000;

		public const int CONTENTS_ORIGIN = 0x1000000;   // removed before bsping an entity

		public const int CONTENTS_MONSTER = 0x2000000;   // should never be on a brush, only in game
		public const int CONTENTS_DEADMONSTER = 0x4000000;
		public const int CONTENTS_DETAIL = 0x8000000;   // brushes to be added after vis leafs
		public const int CONTENTS_TRANSLUCENT = 0x10000000;  // auto set if any surface has trans
		public const int CONTENTS_LADDER = 0x20000000;



		public const int SURF_LIGHT = 0x1;     // value will hold the light strength

		public const int SURF_SLICK = 0x2;     // effects game physics

		public const int SURF_SKY = 0x4;    // don't draw, but add to skybox
		public const int SURF_WARP = 0x8;    // turbulent water warp
		public const int SURF_TRANS33 = 0x10;
		public const int SURF_TRANS66 = 0x20;
		public const int SURF_FLOWING = 0x40;  // scroll towards angle
		public const int SURF_NODRAW = 0x80; // don't bother referencing the texture

		public const int SURF_ALPHATEST = 0x02000000; // used by kmquake2
		public const int SURF_Q1 = 0x04000000;
		public const int SURF_DOOM = 0x08000000;

	}
}
