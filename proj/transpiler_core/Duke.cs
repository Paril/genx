using SixLabors.ImageSharp.Advanced;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using ImageType = SixLabors.ImageSharp.Image<SixLabors.ImageSharp.PixelFormats.Bgra32>;

namespace asset_transpiler
{
	class VOCFile
	{
		public int SampleRate;
		public int Channels;
		public int BPS;
		public byte[] Samples;

		public VOCFile(GRP grp, IArchiveFile file)
		{
			grp.Reader.BaseStream.Position = file.Offset;

			if (Encoding.ASCII.GetString(grp.Reader.ReadBytes(19)) != "Creative Voice File")
				throw new Exception();

			if (grp.Reader.ReadByte() != 0x1A)
				throw new Exception();

			ushort header_size = grp.Reader.ReadUInt16();
			ushort version = grp.Reader.ReadUInt16();
			ushort checksum = grp.Reader.ReadUInt16();

			List<byte> samples = new List<byte>();

			while (grp.Reader.BaseStream.Position != file.Offset + file.Length)
			{
				byte type = grp.Reader.ReadByte();
				uint size = 0;

				if (type != 0)
					size = (uint)(grp.Reader.ReadUInt16() + (grp.Reader.ReadByte() << 16));
				else
					break;

				switch (type)
				{
					case 1:
						{
							byte freq = grp.Reader.ReadByte();
							byte codec = grp.Reader.ReadByte();
							var rate = 1000000 / (256 - freq);

							if (Channels == 0)
								Channels = 1;
							else if (Channels != 1)
								throw new Exception();
							if (BPS == 0)
								BPS = 8;
							else if (BPS != 8)
								throw new Exception();
							if (SampleRate == 0)
								SampleRate = rate;
							else if (SampleRate != rate)
								throw new Exception();

							samples.AddRange(grp.Reader.ReadBytes((int)(size - 2)));

							if (codec != 0)
								throw new Exception();
						}
						break;
					case 2:
						samples.AddRange(grp.Reader.ReadBytes((int)size));
						break;
					case 5:
						{
							string text = Encoding.ASCII.GetString(grp.Reader.ReadBytes((int)size));
						}
						break;
					case 9:
						{
							uint rate = grp.Reader.ReadUInt32();
							byte bits = grp.Reader.ReadByte();
							byte channels = grp.Reader.ReadByte();
							grp.Reader.ReadUInt16();
							grp.Reader.ReadUInt32();
							
							if (Channels == 0)
								Channels = channels;
							else if (Channels != channels)
								throw new Exception();
							if (BPS == 0)
							{
								BPS = bits;

								if (bits == 0)
									return;
							}
							else if (BPS != bits)
								throw new Exception();
							if (SampleRate == 0)
								SampleRate = (int)rate;
							else if (SampleRate != rate)
								throw new Exception();

							samples.AddRange(grp.Reader.ReadBytes((int)(size - 12)));
						}
						break;
					default:
						throw new Exception();
				}
			}

			Samples = samples.ToArray();
		}
	}

	[StructLayout(LayoutKind.Explicit)]
	struct ArtPieces
	{
		[FieldOffset(0)]
		public int Val;

		[FieldOffset(0)]
		public byte NumFrames;
		[FieldOffset(0)]
		public byte AnimType;

		[FieldOffset(1)]
		public sbyte XOffset;

		[FieldOffset(2)]
		public sbyte YOffset;

		[FieldOffset(3)]
		public byte Speed;
	}

	class ArtTile
	{
		public short Width;
		public short Height;
		public uint GRPOffset;
		public uint Index;
		public ArtPieces Pieces;
	}

	class Art
	{
		public Dictionary<uint, ArtTile> Tiles = new Dictionary<uint, ArtTile>();

		public void Read(GRP grp, IArchiveFile file, Action<ArtTile> callback)
		{
			var version = grp.Reader.ReadInt32();
			var numtiles = grp.Reader.ReadInt32();
			var localtilestart = grp.Reader.ReadInt32();
			var localtileend = grp.Reader.ReadInt32();
			var realnumtiles = localtileend - localtilestart + 1;

			short[] x_dims = new short[realnumtiles];
			short[] y_dims = new short[realnumtiles];

			for (var i = 0; i < realnumtiles; ++i)
				x_dims[i] = grp.Reader.ReadInt16();
			for (var i = 0; i < realnumtiles; ++i)
				y_dims[i] = grp.Reader.ReadInt16();

			uint offset = (uint)(grp.Reader.BaseStream.Position + (4 * realnumtiles));

			for (var i = 0; i < realnumtiles; ++i)
			{
				var i32 = grp.Reader.ReadInt32();

				if (x_dims[i] == 0 || y_dims[i] == 0)
					continue;

				var tile = new ArtTile();
				tile.Width = x_dims[i];
				tile.Height = y_dims[i];
				tile.GRPOffset = offset;
				tile.Pieces.Val = i32;
				tile.Index = (uint)(localtilestart + i);

				callback(tile);

				Tiles.Add(tile.Index, tile);

				offset += (uint)(x_dims[i] * y_dims[i]);
			}
		}
	}

	class Duke : GameParser
	{
		static int[] MakeRange(int start, int end)
		{
			var range = new int[end - start + 1];

			for (int i = start, x = 0; i <= end; ++i, ++x)
				range[x] = i;

			return range;
		}

		static Regex _defines = new Regex(@"#?define\s+([a-z0-9_-]+)\s+([0-9]+)\s*$", RegexOptions.IgnoreCase | RegexOptions.Multiline);
		Dictionary<uint, string> _names = new Dictionary<uint, string>();

		void ParseHeader(string file)
		{
			var str = File.ReadAllText(file);

			foreach (Match match in _defines.Matches(str))
			{
				if (match.Groups[1].Value.ToLower().StartsWith("reserved"))
					break;

				_names.TryAdd(uint.Parse(match.Groups[2].Value), match.Groups[1].Value);
			}
		}
		
		static readonly IReadOnlyDictionary<uint, string> pics = new Dictionary<uint, string>()
		{
			{ 2472, "num_0" },
			{ 2473, "num_1" },
			{ 2474, "num_2" },
			{ 2475, "num_3" },
			{ 2476, "num_4" },
			{ 2477, "num_5" },
			{ 2478, "num_6" },
			{ 2479, "num_7" },
			{ 2480, "num_8" },
			{ 2481, "num_9" },

			{ 2462, "hud" },
			{ 2458, "kills" },

			{ 3010, "35_0" },
			{ 3011, "35_1" },
			{ 3012, "35_2" },
			{ 3013, "35_3" },
			{ 3014, "35_4" },
			{ 3015, "35_5" },
			{ 3016, "35_6" },
			{ 3017, "35_7" },
			{ 3018, "35_8" },
			{ 3019, "35_9" },
			{ 3020, "35_colon" },
			{ 3021, "35_slash" },
		};

		static readonly IReadOnlyDictionary<uint, string> simpleSprites = new Dictionary<uint, string>()
		{
			{ 21, "g_pistol" },
			{ 28, "g_shotgun" },
			{ 22, "g_cannon" },
			{ 23, "g_rpg" },
			{ 52, "i_healthsm" },
			{ 51, "i_healthmed" },
			{ 40, "a_clip" },
			{ 49, "a_shells" },
			{ 44, "a_rpg" },
			{ 41, "a_cannon" },
			{ 54, "i_armor" },
			{ 29, "g_devastate" },
			{ 42, "a_devastate" },
			{ 47, "a_pipebombs" },
			{ 24, "g_freezer" },
			{ 27, "g_tripwire" }
		};

		static readonly IReadOnlyDictionary<string, int[]> weaponSprites = new Dictionary<string, int[]>()
		{
			// particles
			{ "shotspark", MakeRange(2595, 2598) },
			{ "shotsmoke", MakeRange(2329, 2332) },
			{ "shotblood", MakeRange(2286, 2293) },
			{ "explosion", MakeRange(1890, 1910) },
			{ "explosion2", MakeRange(2219, 2239) },
			{ "teleport", MakeRange(1262, 1266) },
			{ "freeze", MakeRange(1641, 1643) },
			{ "respawn", MakeRange(1630, 1635) },
			{ "glass", MakeRange(1031, 1033) },

			// gibs
			{ "gib_duketorso", MakeRange(1520, 1527) },
			{ "gib_dukegun", MakeRange(1528, 1535) },
			{ "gib_dukeleg", MakeRange(1536, 1543) },
			{ "gib_jibs1", MakeRange(2245, 2249) },
			{ "gib_jibs2", MakeRange(2250, 2254) },
			{ "gib_jibs3", MakeRange(2255, 2259) },
			{ "gib_jibs4", MakeRange(2260, 2264) },
			{ "gib_jibs5", MakeRange(2265, 2269) },

			// pickups
			{ "i_atomic", MakeRange(100, 115) },
			{ "e_pipebomb", new[] { 26, -26 } },
			{ "a_freezer", MakeRange(37, 39) },

			// weapons
			{ "v_foot", new[] { 2521, 2522 } },
			{ "v_pistol", new[] { 2524, 2525, 2526, 2528, 2529, 2530, 2531, 2532, 2324, 2325, 2326, 2327 } },
			{ "v_shotgun", new[] { 2613, 2614, 2615, 2616, 2617, 2618, 2619 } },
			{ "v_cannon", new[] { 2536, 2537, 2538, 2539, 2540, 2541, 2542, 2543 } },
			{ "v_rpg", new[] { 2544, 2545, 2546 } },
			{ "v_devastate", new[] { 2510, 2511 } },
			{ "v_pipebomb", MakeRange(2570, 2575) },
			{ "v_freezer", new[] { 2548, 2550, 2551, 2552, 2553 } },
			{ "v_tripwire", new[] { 2563, 2566, 2564, 2565 } }
		};

		static readonly IReadOnlyDictionary<string, int[,]> directionalSprites = new Dictionary<string, int[,]>()
		{
			{ "duke", new[,] {
				// standing
				{ 1405, 1406, 1407, 1408, 1409, -1408, -1407, -1406 },

				// water/jetpacking
				{ 1420, 1421, 1422, 1423, 1424, -1423, -1422, -1421 },

				// run frame 1
				{ 1425, 1426, 1427, 1428, 1429, -1428, -1427, -1426 },

				// run frame 2
				{ 1430, 1431, 1432, 1433, 1434, -1433, -1432, -1431 },

				// run frame 3
				{ 1435, 1436, 1437, 1438, 1439, -1438, -1437, -1436 },

				// run frame 4
				{ 1440, 1441, 1442, 1443, 1444, -1443, -1442, -1441 },

				// kick hold
				{ 1445, 1446, 1447, 1448, 1449, -1448, -1447, -1446 },

				// kick release
				{ 1450, 1451, 1452, 1453, 1454, -1453, -1452, -1451 },

				// jump up
				{ 1455, 1456, 1457, 1458, 1459, -1458, -1457, -1456 },

				// jump arc/crouching
				{ 1460, 1461, 1462, 1463, 1464, -1463, -1462, -1461 },

				// jump downwards
				{ 1465, 1466, 1467, 1469, 1469, -1468, -1467, -1466 },

				// crouch-move frame 1
				{ 1491, 1492, 1493, 1494, 1495, -1494, -1493, -1492 },

				// crouch-move frame 2
				{ 1496, 1497, 1498, 1499, 1500, -1499, -1498, -1497 },
						
				// crouch-move frame 3
				{ 1501, 1502, 1503, 1504, 1505, -1504, -1503, -1502 },

				// dying
				{ 1511, 1511, 1511, 1511, 1511, 1511, 1511, 1511 },
				{ 1512, 1512, 1512, 1512, 1512, 1512, 1512, 1512 },
				{ 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513 },
				{ 1514, 1514, 1514, 1514, 1514, 1514, 1514, 1514 },
				{ 1515, 1515, 1515, 1515, 1515, 1515, 1515, 1515 },
				{ 1518, 1518, 1518, 1518, 1518, 1518, 1518, 1518 },

				// swimming frame 1
				{ 1780, 1781, 1782, 1783, 1784, -1783, -1782, -1781 },
				{ 1785, 1786, 1787, 1788, 1789, -1788, -1787, -1786 },
				{ 1790, 1791, 1792, 1793, 1794, -1793, -1792, -1791 },
				{ 1795, 1796, 1797, 1798, 1799, -1798, -1797, -1796 }
			} },
			{ "shotgunshell", new[,] {
					{ 2535 },
					{ -2535 }
				} },
			{ "shell", new[,] {
				{ 2533 },
				{ 2534 },
				{ -2533 },
				{ -2534 }
				} },
			{ "rocket", new[,] {
				{ 2605, 2606, 2607, 2608, 2609, 2610, 2611, -2610, -2609, -2608, -2607, -2606 }
				} },
		};

		void TaskWritePalette(string outname)
		{
			using (var wr = new BinaryWriter(OpenFileStream(outname)))
				foreach (var color in Globals._palettes[(int)PaletteID.Duke])
				{
					wr.Write(color.R);
					wr.Write(color.G);
					wr.Write(color.B);
				}
		}

		void TaskVoc((string grpfile, IArchiveFile file) info)
		{
			using (var grp = new GRP(OpenArchive(info.grpfile)))
			{
				var vf = new VOCFile(grp, info.file);

				if (vf.BPS == 0)
					return;

				WriteWav("sound/duke/" + Path.GetFileNameWithoutExtension(info.file.Name) + ".wav", vf.SampleRate, vf.Samples.Length, vf.Samples, vf.Channels, vf.BPS);
			}
		}

		Art artFiles = new Art();
		HashSet<uint> skipTiles = new HashSet<uint>();
		
		void TaskTile((string grpfile, ArtTile tile) info)
		{
			using (var grp = new BinaryReader(OpenArchive(info.grpfile)))
			{
				if (!_names.TryGetValue(info.tile.Index, out string name))
					name = info.tile.Index.ToString();

				var outpng = "textures/duke/" + name + "." + ImageExtensions.WrittenImageFormat;
				grp.BaseStream.Position = info.tile.GRPOffset;

				var bmp = new ImageType(info.tile.Width, info.tile.Height);
				var bmpSpan = bmp.GetPixelSpan();

				var pixels = new byte[info.tile.Width * info.tile.Height];

				for (var x = 0; x < info.tile.Width; ++x)
					for (var y = 0; y < info.tile.Height; ++y)
					{
						var i = (y * info.tile.Width) + x;
						bmpSpan[i] = Globals.GetMappedColor(PaletteID.Duke, pixels[i] = grp.ReadByte());
					}

				SaveWalTga(bmp, outpng, "textures/duke/" + name + ".wal", "duke/" + name, "", 0, 0, pixels, PaletteID.Duke, info.tile.Width, info.tile.Height);
			}
		}

		void TaskPic((string grpfile, uint index, string name) info)
		{
			var art = artFiles.Tiles[info.index];
			var file = "pics/duke/" + info.name  + "." + ImageExtensions.WrittenImageFormat;

			using (var grp = new BinaryReader(OpenArchive(info.grpfile)))
			{
				grp.BaseStream.Position = art.GRPOffset;

				var bytes = new byte[art.Width * art.Height];

				for (var x = 0; x < art.Width; x++)
					for (var y = 0; y < art.Height; y++)
						bytes[y * art.Width + x] = grp.ReadByte();
				
				ArchiveBase.WriteImageTo(bytes, PaletteID.Duke, file, art.Width, art.Height);
			}
		}

		void TaskSimpleSprite((string grpfile, uint index, string name) info)
		{
			var fileBase = "sprites/duke/" + info.name;
			var dnsFile = fileBase + ".dns";
			var art = artFiles.Tiles[info.index];

			using (var grp = new BinaryReader(OpenArchive(info.grpfile)))
			{
				grp.BaseStream.Position = art.GRPOffset;

				var bytes = new byte[art.Width * art.Height];

				for (var x = 0; x < art.Width; x++)
					for (var y = 0; y < art.Height; y++)
						bytes[y * art.Width + x] = grp.ReadByte();
				
				ArchiveBase.WriteImageTo(bytes, PaletteID.Duke, fileBase + "_0." + ImageExtensions.WrittenImageFormat, art.Width, art.Height);
			}

			using (var bw = new BinaryWriter(OpenFileStream(dnsFile)))
			{
				bw.Write(Encoding.ASCII.GetBytes("DNSP"));
				bw.Write((byte)1);

				bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
				bw.Write((short)((art.Height / 2) + art.Pieces.YOffset));

				bw.Write((byte)1);
				bw.Write((byte)1);

				bw.Write((byte)0);
				bw.Write((byte)0);
			}
		}

		void TaskWeaponSprite((string grpfile, string name, int[] frames) info)
		{
			var fileBase = "sprites/duke/" + info.name;
			var dnsFile = fileBase + ".dns";
			var artIndex = 0;

			using (var grp = new BinaryReader(OpenArchive(info.grpfile)))
			{
				foreach (var index in info.frames)
				{
					var art = artFiles.Tiles[(uint)Math.Abs(index)];
					grp.BaseStream.Position = art.GRPOffset;

					var bytes = new byte[art.Width * art.Height];

					for (var x = 0; x < art.Width; x++)
						for (var y = 0; y < art.Height; y++)
							bytes[y * art.Width + x] = grp.ReadByte();

					// flip X axis
					if (index < 0)
					{
						for (var y = 0; y < art.Height; y++)
							for (var x = 0; x < art.Width / 2; x++)
							{
								var yb = y * art.Width;
								var ix = art.Width - x - 1;
								var copy = bytes[yb + x];
								bytes[yb + x] = bytes[yb + ix];
								bytes[yb + ix] = copy;
							}
					}
					
					ArchiveBase.WriteImageTo(bytes, PaletteID.Duke, fileBase + "_" + artIndex + "." + ImageExtensions.WrittenImageFormat, art.Width, art.Height);

					artIndex++;
				}
			}

			using (var bw = new BinaryWriter(OpenFileStream(dnsFile)))
			{
				bw.Write(Encoding.ASCII.GetBytes("DNSP"));
				bw.Write((byte)info.frames.Length);

				foreach (var index in info.frames)
				{
					var art = artFiles.Tiles[(uint)Math.Abs(index)];

					bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
					bw.Write((short)((art.Height / 2) + art.Pieces.YOffset));
				}

				bw.Write((byte)info.frames.Length);
				bw.Write((byte)1);

				for (var i = 0; i < info.frames.Length; ++i)
				{
					bw.Write((byte)i);
					bw.Write((byte)0);
				}
			}
		}

		void TaskComplex((string grpfile, string name, int[,] frames) info)
		{
			var fileBase = "sprites/duke/" + info.name;
			var dnsFile = fileBase + ".dns";

			var pics_hash = new List<uint>();

			foreach (var frame in info.frames)
			{
				var abs = (uint)Math.Abs(frame);

				if (!pics_hash.Contains(abs))
					pics_hash.Add(abs);
			}

			using (var grp = new BinaryReader(OpenArchive(info.grpfile)))
			{
				var artIndex = 0;

				foreach (var pic in pics_hash)
				{
					var art = artFiles.Tiles[pic];
					grp.BaseStream.Position = art.GRPOffset;

					var bytes = new byte[art.Width * art.Height];

					for (var x = 0; x < art.Width; x++)
						for (var y = 0; y < art.Height; y++)
							bytes[y * art.Width + x] = grp.ReadByte();
					
					ArchiveBase.WriteImageTo(bytes, PaletteID.Duke, fileBase + "_" + artIndex + "." + ImageExtensions.WrittenImageFormat, art.Width, art.Height);

					artIndex++;
				}
			}

			using (var bw = new BinaryWriter(OpenFileStream(dnsFile)))
			{
				bw.Write(Encoding.ASCII.GetBytes("DNSP"));
				bw.Write((byte)pics_hash.Count);

				foreach (var pic in pics_hash)
				{
					var art = artFiles.Tiles[pic];

					if ((pic >= 1460 && pic <= 1464) || (pic >= 1491 && pic <= 1505))
					{
						bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
						bw.Write((short)((art.Height / 2) + art.Pieces.YOffset - 14));
					}
					else if (pic >= 1511 && pic < 1518)
					{
						bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
						bw.Write((short)((art.Height / 2) + art.Pieces.YOffset - 22));
					}
					else if (pic == 1518)
					{
						bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
						bw.Write((short)-16);
					}
					else
					{
						bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
						bw.Write((short)((art.Height / 2) + art.Pieces.YOffset));
					}
				}

				int num_frames = info.frames.GetUpperBound(0) + 1;

				bw.Write((byte)num_frames);

				int num_dirs = info.frames.GetUpperBound(1) + 1;

				bw.Write((byte)num_dirs);

				for (var i = 0; i < num_frames; ++i)
					for (var x = 0; x < num_dirs; ++x)
					{
						int pic = info.frames[i, x];
						bw.Write((byte)pics_hash.IndexOf((uint)Math.Abs(pic)));
						bw.Write((byte)(pic < 0 ? 1 : 0));
					}
			}
		}

		void TaskDukeConchars(string grpfile)
		{
			var start = 2823;
			var end = 2914;
			var charOffset = 34;
			var compiler = new FontCompiler();

			using (var grp = new BinaryReader(OpenArchive(grpfile)))
			{
				for (var i = start; i <= end; i++)
				{
					var ascii = charOffset + (i - start);
					var art = artFiles.Tiles[(uint)i];
					var image = new ImageType(art.Width, art.Height);
					var span = image.GetPixelSpan();
	
					grp.BaseStream.Position = art.GRPOffset;

					for (var x = 0; x < art.Width; x++)
						for (var y = 0; y < art.Height; y++)
							span[y * art.Width + x] = Globals.GetMappedColor(PaletteID.Duke, grp.ReadByte()).Grayscale();

					compiler.Charmap.Add(ascii, image);
				}

				compiler.CompileTo("pics/duke/conchars");
			}
		}

		void TaskGrp(string grpfile)
		{
			// names need to be synchronized
			var namesFile = "names.h";
			var consFile = "DEFS.CON";
			var dir = Path.GetDirectoryName(grpfile);

			ParseHeader(dir + "\\" + consFile);
			ParseHeader(dir + "\\" + namesFile);

			// compile list of tiles we need to delete later
			foreach (var key in pics.Keys)
				skipTiles.Add(key);
			foreach (var key in simpleSprites.Keys)
				skipTiles.Add(key);
			foreach (var keys in weaponSprites.Values)
				foreach (var key in keys)
					skipTiles.Add((uint)Math.Abs(key));
			foreach (var keys in directionalSprites.Values)
				foreach (var key in keys)
					skipTiles.Add((uint)Math.Abs(key));
			for (var i = 2823; i <= 2914; i++)
				skipTiles.Add((uint)i);
		
			// palette
			CreateTask(TaskWritePalette, "pics/duke/palette.dat");
			
			// grp files
			using (var grp = new GRP(OpenArchive(grpfile)))
			{
				foreach (var file in grp.Files)
				{
					grp.Reader.BaseStream.Position = file.Offset;

					if (file.Name.EndsWith(".VOC"))
						CreateTask(TaskVoc, (grpfile, file));
					else if (file.Name.EndsWith(".ART"))
						// this has to be on main thread unfortunately, but luckily the TGA writing is the big eater of time
						artFiles.Read(grp, file, art =>
						{
							if (!skipTiles.Contains(art.Index))
								CreateTask(TaskTile, (grpfile, art));
						});
				}

				CreateTask(TaskDukeConchars, grpfile);

				// pic files
				foreach (var pic in pics)
					CreateTask(TaskPic, (grpfile, pic.Key, pic.Value));

				// simple sprites
				foreach (var simple in simpleSprites)
					CreateTask(TaskSimpleSprite, (grpfile, simple.Key, simple.Value));

				// simple animated sprites
				// (frontal only)
				foreach (var simple in weaponSprites)
					CreateTask(TaskWeaponSprite, (grpfile, simple.Key, simple.Value));

				// directional sprites
				foreach (var complex in directionalSprites)
					CreateTask(TaskComplex, (grpfile, complex.Key, complex.Value));
			}
		}

		public override void Run()
		{
			CreateTask(TaskGrp, Globals.GetGameInfo(PaletteID.Duke).Path + "\\duke3d.grp");
		}

		public override void Cleanup()
		{
		}
	}
}
