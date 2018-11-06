using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace asset_compiler
{
	class GRPFile
	{
		public string Name;
		public uint Offset;
		public uint Length;

		public override string ToString()
		{
			return Name + "(" + Offset + " : " + Length + ")";
		}
	}

	class GRP : IDisposable
	{
		public List<GRPFile> Files = new List<GRPFile>();
		public BinaryReader Reader;

		public GRP(Stream stream)
		{
			Reader = new BinaryReader(stream);

			var magic = Encoding.ASCII.GetString(Reader.ReadBytes(12));

			if (magic != "KenSilverman")
				throw new Exception();

			var num_entries = Reader.ReadUInt32();
			uint offset = (uint)(Reader.BaseStream.Position + ((12 + 4) * num_entries));

			for (uint i = 0; i < num_entries; ++i)
			{
				GRPFile file = new GRPFile();

				file.Name = Encoding.ASCII.GetString(Reader.ReadBytes(12));
				file.Length = Reader.ReadUInt32();
				file.Offset = offset;

				offset += file.Length;

				if (file.Name.IndexOf('\0') != -1)
					file.Name = file.Name.Substring(0, file.Name.IndexOf('\0'));

				Files.Add(file);
			}
		}

		public GRPFile Get(string name)
		{
			foreach (var file in Files)
				if (file.Name == name)
					return file;

			return null;
		}

		public byte[] GetBytes(GRPFile file)
		{
			Reader.BaseStream.Position = file.Offset;
			return Reader.ReadBytes((int)file.Length);
		}

		public void WriteFileTo(GRPFile file, string output)
		{
			Globals.CreateDirectory(Path.GetDirectoryName(output));
			Globals.WriteFile(output, GetBytes(file));
		}

		public void WriteFileTo(GRPFile file, string output, Action<BinaryWriter> writeHeader)
		{
			Globals.CreateDirectory(Path.GetDirectoryName(output));

			using (var bw = new BinaryWriter(Globals.OpenFileStream(output)))
			{
				writeHeader(bw);
				bw.Write(GetBytes(file));
			}
		}
		public void WriteFileTo(GRPFile file, string output, Action<BinaryWriter, byte[]> writeData)
		{
			Globals.CreateDirectory(Path.GetDirectoryName(output));

			using (var bw = new BinaryWriter(Globals.OpenFileStream(output)))
				writeData(bw, GetBytes(file));
		}

		public void Dispose()
		{
			Reader.Dispose();
		}
	}

	class VOCFile
	{
		public int SampleRate;
		public int Channels;
		public int BPS;
		public byte[] Samples;

		public VOCFile(GRP grp, GRPFile file)
		{
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
							var rate = (1000000 / (256 - freq));

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
						samples.AddRange(grp.Reader.ReadBytes((int)(size)));
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
		public ArtPieces Pieces = new ArtPieces();
	}

	class Art
	{
		public Dictionary<uint, ArtTile> Tiles = new Dictionary<uint, ArtTile>();

		public void Read(GRP grp, GRPFile file)
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

				ArtTile tile = new ArtTile();
				tile.Width = x_dims[i];
				tile.Height = y_dims[i];
				tile.GRPOffset = offset;

				tile.Pieces.Val = i32;
				
				Tiles.Add((uint)(localtilestart + i), tile);

				offset += (uint)(x_dims[i] * y_dims[i]);
			}
		}
	}

	public static class Duke
	{
		static int[] MakeRange(int start, int end)
		{
			var range = new int[end - start + 1];

			for (int i = start, x = 0; i <= end; ++i, ++x)
				range[x] = i;

			return range;
		}

		public static void CopyStuff(string grpfile)
		{
			Globals.OpenStatus("Making palette...");

			var outpal = "pics/duke/palette.dat";

			Globals.CreateDirectory(Path.GetDirectoryName(outpal));

			using (var wr = new BinaryWriter(Globals.OpenFileStream(outpal)))
			{
				foreach (var color in Globals._palettes[(int)PaletteID.Duke])
				{
					wr.Write((byte)color.R);
					wr.Write((byte)color.G);
					wr.Write((byte)color.B);
				}
			}

			Globals.OpenStatus("Parsing GRP file...");

			using (var grp = new GRP(File.Open(grpfile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite)))
			{
				Art artFiles = new Art();

				foreach (var file in grp.Files)
				{
					grp.Reader.BaseStream.Position = file.Offset;

					if (file.Name.EndsWith(".VOC"))
					{
						VOCFile vf = new VOCFile(grp, file);

						if (vf.BPS == 0)
							continue;

						var outwav = "sound/duke/" + Path.GetFileNameWithoutExtension(file.Name) + ".wav";

						Globals.Status("Writing sound " + Path.GetFileNameWithoutExtension(file.Name) + "...");
						Globals.CreateDirectory(Path.GetDirectoryName(outwav));

						Globals.WriteWav(outwav, vf.SampleRate, vf.Samples.Length, vf.Samples, vf.Channels, vf.BPS);
					}
					else if (file.Name.EndsWith(".ART"))
						artFiles.Read(grp, file);
				}

				// test PNGs, woo

				foreach (var art in artFiles.Tiles)
				{
					var outpng = "duke_tmp/" + art.Key + ".png";
					grp.Reader.BaseStream.Position = art.Value.GRPOffset;

					if (!Directory.Exists(Path.GetDirectoryName(outpng)))
						Directory.CreateDirectory(Path.GetDirectoryName(outpng));

					if (File.Exists(outpng))
						continue;

					Bitmap bmp = new Bitmap(art.Value.Width, art.Value.Height);

					for (var x = 0; x < art.Value.Width; ++x)
						for (var y = 0; y < art.Value.Height; ++y)
						{
							var pixel = grp.Reader.ReadByte();
							var color = Globals._palettes[(int)PaletteID.Duke][pixel];

							bmp.SetPixel(x, y, color);
						}

					bmp.Save(outpng);
				}

				Dictionary<uint, string> pics = new Dictionary<uint, string>()
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

				foreach (var pic in pics)
				{
					var art = artFiles.Tiles[pic.Key];

					var file = "pics/duke/" + pic.Value + ".dnp";
					grp.Reader.BaseStream.Position = art.GRPOffset;

					Globals.CreateDirectory(Path.GetDirectoryName(file));

					using (var bw = new BinaryWriter(Globals.OpenFileStream(file)))
					{
						bw.Write((uint)art.Width);
						bw.Write((uint)art.Height);

						var bytes = new byte[art.Width * art.Height];

						for (var x = 0; x < art.Width; ++x)
							for (var y = 0; y < art.Height; ++y)
								bytes[(y * art.Width) + x] = grp.Reader.ReadByte();

						bw.Write(bytes);
					}
				}


				// simple sprites
				Dictionary<uint, string> simpleSprites = new Dictionary<uint, string>()
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
					{ 24, "g_freezer" }
				};

				foreach (var simple in simpleSprites)
				{
					var art = artFiles.Tiles[simple.Key];

					var file = "sprites/duke/" + simple.Value + ".dns";
					grp.Reader.BaseStream.Position = art.GRPOffset;
					
					Globals.CreateDirectory(Path.GetDirectoryName(file));

					using (var bw = new BinaryWriter(Globals.OpenFileStream(file)))
					{
						bw.Write(Encoding.ASCII.GetBytes("DNSP"));
						bw.Write((byte)1);

						bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
						bw.Write((short)((art.Height / 2) + art.Pieces.YOffset));
						bw.Write(art.Width);
						bw.Write(art.Height);

						var bytes = new byte[art.Width * art.Height];

						for (var x = 0; x < art.Width; ++x)
							for (var y = 0; y < art.Height; ++y)
								bytes[(y * art.Width) + x] = grp.Reader.ReadByte();

						bw.Write(bytes);

						bw.Write((byte)1);
						bw.Write((byte)1);
						
						bw.Write((byte)0);
						bw.Write((byte)0);
					}
				}

				// simple animated sprites
				// (frontal only)
				Dictionary<string, int[]> weaponSprites = new Dictionary<string, int[]>()
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
					{ "e_pipebomb", new int[] { 26, -26 } },
					{ "a_freezer", MakeRange(37, 39) },

					// weapons
					{ "v_foot", new int[] { 2521, 2522 } },
					{ "v_off_foot", new int[] { -2521, -2522 } },
					{ "v_pistol", new int[] { 2524, 2525, 2526, 2528, 2529, 2530, 2531, 2532, 2324, 2325, 2326, 2327 } },
					{ "v_shotgun", new int[] { 2613, 2614, 2615, 2616, 2617, 2618, 2619 } },
					{ "v_cannon", new int[] { 2536, 2537, 2538, 2539, 2540, 2541, 2542, 2543 } },
					{ "v_rpg", new int[] { 2544, 2545, 2546 } },
					{ "v_devastate", new int[] { 2510, 2511, -2510, -2511 } },
					{ "v_pipebomb", MakeRange(2570, 2575) },
					{ "v_freezer", new int[] { 2548, 2550, 2551, 2552, 2553 } }
				};

				foreach (var simple in weaponSprites)
				{
					var file = "sprites/duke/" + simple.Key + ".dns";

					Globals.CreateDirectory(Path.GetDirectoryName(file));

					using (var bw = new BinaryWriter(Globals.OpenFileStream(file)))
					{
						bw.Write(Encoding.ASCII.GetBytes("DNSP"));
						bw.Write((byte)simple.Value.Length);

						for (var i = 0; i < simple.Value.Length; ++i)
						{
							var art = artFiles.Tiles[(uint)Math.Abs(simple.Value[i])];

							bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
							bw.Write((short)((art.Height / 2) + art.Pieces.YOffset));
							bw.Write(art.Width);
							bw.Write(art.Height);

							var bytes = new byte[art.Width * art.Height];

							grp.Reader.BaseStream.Position = art.GRPOffset;

							if (simple.Value[i] < 0)
							{
								for (var x = 0; x < art.Width; ++x)
									for (var y = 0; y < art.Height; ++y)
										bytes[(y * art.Width) + (art.Width - x - 1)] = grp.Reader.ReadByte();
							}
							else
							{
								for (var x = 0; x < art.Width; ++x)
									for (var y = 0; y < art.Height; ++y)
										bytes[(y * art.Width) + x] = grp.Reader.ReadByte();
							}

							bw.Write(bytes);
						}

						bw.Write((byte)simple.Value.Length);
						bw.Write((byte)1);

						for (var i = 0; i < simple.Value.Length; ++i)
						{
							bw.Write((byte)i);
							bw.Write((byte)0);
						}
					}
				}

				// directional sprites
				Dictionary<string, int[,]> directionalSprites = new Dictionary<string, int[,]>()
				{
					{ "duke", new int[,] {
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
					{ "shotgunshell", new int[,] {
							{ 2535 },
							{ -2535 }
						} },
					{ "shell", new int[,] {
						{ 2533 },
						{ 2534 },
						{ -2533 },
						{ -2534 }
						} },
					{ "rocket", new int[,] {
						{ 2605, 2606, 2607, 2608, 2609, 2610, 2611, -2610, -2609, -2608, -2607, -2606 }
						} },
				};

				foreach (var complex in directionalSprites)
				{
					var file = "sprites/duke/" + complex.Key + ".dns";

					Globals.CreateDirectory(Path.GetDirectoryName(file));

					var pics_hash = new HashSet<uint>();

					foreach (var frame in complex.Value)
						pics_hash.Add((uint)Math.Abs(frame));

					var piclist = pics_hash.ToList();

					using (var bw = new BinaryWriter(Globals.OpenFileStream(file)))
					{
						bw.Write(Encoding.ASCII.GetBytes("DNSP"));
						bw.Write((byte)piclist.Count);

						foreach (var pic in piclist)
						{
							var art = artFiles.Tiles[pic];

							if ((pic >= 1460 && pic <= 1464) || (pic >= 1491 && pic <= 1505))
							{
								bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
								bw.Write((short)(((art.Height / 2) + art.Pieces.YOffset) - 14));
							}
							else if (pic >= 1511 && pic < 1518)
							{
								bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
								bw.Write((short)(((art.Height / 2) + art.Pieces.YOffset) - 22));
							}
							else if (pic == 1518)
							{
								bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
								bw.Write((short)(-16));
							}
							else
							{
								bw.Write((short)((art.Width / 2) + art.Pieces.XOffset));
								bw.Write((short)((art.Height / 2) + art.Pieces.YOffset));
							}
							bw.Write(art.Width);
							bw.Write(art.Height);

							var bytes = new byte[art.Width * art.Height];

							grp.Reader.BaseStream.Position = art.GRPOffset;
							for (var x = 0; x < art.Width; ++x)
								for (var y = 0; y < art.Height; ++y)
									bytes[(y * art.Width) + x] = grp.Reader.ReadByte();

							bw.Write(bytes);
						}

						int num_frames = complex.Value.GetUpperBound(0) + 1;

						bw.Write((byte)num_frames);

						int num_dirs = complex.Value.GetUpperBound(1) + 1;

						bw.Write((byte)num_dirs);

						for (var i = 0; i < num_frames; ++i)
						{
							for (var x = 0; x < num_dirs; ++x)
							{
								int pic = complex.Value[i, x];
								bw.Write((byte)piclist.IndexOf((uint)Math.Abs(pic)));
								bw.Write((byte)(pic < 0 ? 1 : 0));
							}
						}
					}
				}
			}
		}
	}
}
