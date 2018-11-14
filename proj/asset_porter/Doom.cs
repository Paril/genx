using SixLabors.ImageSharp;
using SixLabors.ImageSharp.PixelFormats;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace asset_transpiler
{
	class DoomMapTexture
	{
		public struct DoomPatch
		{
			public short OriginX;
			public short OriginY;
			public short Patch;
			public short StepDir;
			public short Colormap;
		}

		public string Name;
		public int Masked;
		public short Width;
		public short Height;
		public int ColumnDirectory;
		public DoomPatch[] Patches;

		public DoomMapTexture(WAD wad)
		{
			Name = Encoding.ASCII.GetString(wad.Reader.ReadBytes(8));

			if (Name.IndexOf('\0') != -1)
				Name = Name.Substring(0, Name.IndexOf('\0'));

			Masked = wad.Reader.ReadInt32();

			Width = wad.Reader.ReadInt16();
			Height = wad.Reader.ReadInt16();

			ColumnDirectory = wad.Reader.ReadInt32();

			short patchCount = wad.Reader.ReadInt16();

			Patches = new DoomPatch[patchCount];

			for (var i = 0; i < patchCount; ++i)
			{
				Patches[i].OriginX = wad.Reader.ReadInt16();
				Patches[i].OriginY = wad.Reader.ReadInt16();
				Patches[i].Patch = wad.Reader.ReadInt16();
				Patches[i].StepDir = wad.Reader.ReadInt16();
				Patches[i].Colormap = wad.Reader.ReadInt16();
			}
		}
	}

	class DoomPicture
	{
		public short Width, Height;
		public short X, Y;
		public byte[] Pixels;

		public DoomPicture(WAD wad, WADFile file)
		{
			wad.Reader.BaseStream.Position = file.Offset;
			Width = wad.Reader.ReadInt16();
			Height = wad.Reader.ReadInt16();

			X = wad.Reader.ReadInt16();
			Y = wad.Reader.ReadInt16();

			int[] columns = new int[Width];

			for (int i = 0; i < Width; ++i)
				columns[i] = wad.Reader.ReadInt32();

			Pixels = new byte[Width * Height];

			for (int i = 0; i < Pixels.Length; ++i)
				Pixels[i] = 255;

			for (int i = 0; i < Width; ++i)
			{
				wad.Reader.BaseStream.Position = file.Offset + columns[i];
				int rowstart = 0;

				while (rowstart != 255)
				{
					rowstart = wad.Reader.ReadByte();

					if (rowstart == 255)
						break;

					byte pixel_count = wad.Reader.ReadByte();
					wad.Reader.ReadByte();

					for (int j = 0; j < pixel_count; ++j)
					{
						byte pixel = wad.Reader.ReadByte();
						Pixels[((j + rowstart) * Width) + i] = pixel;
					}

					wad.Reader.ReadByte();
				}
			}
		}
	}

	class Doom : GameParser
	{
		static WADFile LumpExpect(WAD wad, ref int lumpPos, string name)
		{
			var file = (WADFile) wad.Files[lumpPos];

			if (file.Name != name)
				throw new Exception("wrong wad?");

			lumpPos++;
			return file;
		}

		static string LumpPeek(WAD wad, ref int lumpPos)
		{
			return wad.Files[lumpPos].Name;
		}

		void UnpackDoomTexture(DoomMapTexture texture, Dictionary<string, string> animated, Dictionary<string, DoomPicture> mapPatches, List<string> mapPatchNames)
		{
			byte[] pixels = new byte[texture.Width * texture.Height];

			for (var i = 0; i < pixels.Length; ++i)
				pixels[i] = 255;

			for (var i = 0; i < texture.Patches.Length; ++i)
			{
				var patchData = texture.Patches[i];
				var patch = mapPatches[mapPatchNames[patchData.Patch]];

				ByteBuffer.BoxCopy(patch.Pixels, 0, 0, patch.Width, patch.Height, pixels, patchData.OriginX, patchData.OriginY, texture.Width, texture.Height, patch.Width, patch.Height, true);
			}

			var name = texture.Name;
			string outwal = "textures/doom/" + name + ".wal";
			string outpng = "textures/doom/" + name + ".tga";

			CreateDirectory(Path.GetDirectoryName(outwal));

			if (FileExists(outpng))
				return;

			Status("Writing patched texture " + name + "...", 2);

			using (var bmp = new Image<Rgba32>(texture.Width, texture.Height))
			{
				for (var y = 0; y < texture.Height; ++y)
					for (var x = 0; x < texture.Width; ++x)
					{
						var c = pixels[(y * texture.Width) + x];
						bmp[x, y] = Globals.GetMappedColor(PaletteID.Doom, c);
					}

				using (var stream = OpenFileStream(outpng))
					bmp.SaveAsTga(stream);

				// Write WAL
				string walname = name, next_name;

				if (!animated.TryGetValue(texture.Name, out next_name))
					next_name = "";

				outwal = "textures/doom/" + walname + ".wal";

				SaveWal(outwal, walname, next_name, 0, 0, pixels, PaletteID.Doom, texture.Width, texture.Height);
			}
		}

		static void DoomLumpMarkers(WAD wad, ref int lumpPos, string prefix, Action<WADFile> callback)
		{
			LumpExpect(wad, ref lumpPos, prefix + "START");
			string lumpName;

			while ((lumpName = LumpPeek(wad, ref lumpPos)) != prefix + "END")
				callback(LumpExpect(wad, ref lumpPos, lumpName));

			LumpExpect(wad, ref lumpPos, prefix + "END");
		}

		void WriteSimpleDoomTexture(WAD wad, WADFile file, string output)
		{
			CreateDirectory(Path.GetDirectoryName(output));

			DoomPicture picture = new DoomPicture(wad, file);
			wad.WriteImageTo(picture.Pixels, PaletteID.Doom, output, picture.Width, picture.Height);
		}

		static void LumpLoadTexture(WAD wad, ref int lumpPos, List<DoomMapTexture> mapTextures, string name)
		{
			var file = LumpExpect(wad, ref lumpPos, name);

			wad.Reader.BaseStream.Position = file.Offset;

			int numTextures = wad.Reader.ReadInt32();
			int[] offsets = new int[numTextures];

			for (int i = 0; i < numTextures; ++i)
				offsets[i] = wad.Reader.ReadInt32();

			for (int i = 0; i < numTextures; ++i)
			{
				wad.Reader.BaseStream.Position = file.Offset + offsets[i];
				mapTextures.Add(new DoomMapTexture(wad));
			}
		}

		static void LumpLoadPatchNames(WAD wad, ref int lumpPos, List<string> mapPatchNames, string name)
		{
			var file = LumpExpect(wad, ref lumpPos, name);

			wad.Reader.BaseStream.Position = file.Offset;

			int numPatches = wad.Reader.ReadInt32();

			for (int i = 0; i < numPatches; ++i)
			{
				string pname = Encoding.ASCII.GetString(wad.Reader.ReadBytes(8));

				if (pname.IndexOf('\0') != -1)
					pname = pname.Substring(0, pname.IndexOf('\0'));

				mapPatchNames.Add(pname.ToLower());
			}
		}

		void D2CopyStuff(string wadfile)
		{
			Status("Parsing " + wadfile + "...", 0);

			using (var wad = new WAD(this, File.Open(wadfile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite)))
			{
				int lumpPos = 0;
				string lumpName;

				Status("Parsing palette...", 1);
				
				// read PLAYPAL lump
				var lump = LumpExpect(wad, ref lumpPos, "PLAYPAL");

				wad.Reader.BaseStream.Position = lump.Offset;

				byte[] palette = wad.Reader.ReadBytes(256 * 3);
				var outpal = "pics/doom/palette.dat";

				CreateDirectory(Path.GetDirectoryName(outpal));

				using (var wr = new BinaryWriter(OpenFileStream(outpal)))
					wr.Write(palette);

				Status("Skipping nonsense...", 1);

				// skip COLORMAP, ENDOOM
				LumpExpect(wad, ref lumpPos, "COLORMAP");
				LumpExpect(wad, ref lumpPos, "ENDOOM");

				// skip DEMOs
				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("DEMO"))
					LumpExpect(wad, ref lumpPos, lumpName);

				// skip MAPs
				var mapLumpNames = new HashSet<string>() { "THINGS", "LINEDEFS", "SIDEDEFS", "VERTEXES", "SEGS", "SSECTORS", "NODES", "SECTORS", "REJECT", "BLOCKMAP", "BEHAVIOR" };

				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("MAP"))
				{
					LumpExpect(wad, ref lumpPos, lumpName);

					while (mapLumpNames.Contains(lumpName = LumpPeek(wad, ref lumpPos)))
						LumpExpect(wad, ref lumpPos, lumpName);
				}

				var mapTextures = new List<DoomMapTexture>();
				var mapPatchNames = new List<string>();

				Status("Loading TEXTURE1...", 1);

				// Load TEXTURE1
				LumpLoadTexture(wad, ref lumpPos, mapTextures, "TEXTURE1");

				// Get rid of AASHITTY
				mapTextures.RemoveAt(0);

				Status("Loading PNAMES...", 1);
				LumpLoadPatchNames(wad, ref lumpPos, mapPatchNames, "PNAMES");

				Status("Skipping nonsense...", 1);

				// Skip music stuff
				LumpExpect(wad, ref lumpPos, "GENMIDI");
				LumpExpect(wad, ref lumpPos, "DMXGUSC");

				Status("Parsing sound lumps...", 1);
				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("D"))
				{
					lump = LumpExpect(wad, ref lumpPos, lumpName);

					// Sounds
					if (lumpName.StartsWith("DS"))
					{
						wad.Reader.BaseStream.Position = lump.Offset;

						wad.Reader.ReadUInt16();
						var sampleRate = wad.Reader.ReadUInt16();
						var numSamples = wad.Reader.ReadUInt16();
						wad.Reader.ReadUInt16();
						var samples = wad.Reader.ReadBytes(numSamples);

						var outwav = "sound/doom/" + lumpName.Substring(2) + ".wav";

						Status("Writing sound " + lumpName.Substring(2) + "...", 2);
						CreateDirectory(Path.GetDirectoryName(outwav));

						WriteWav(outwav, sampleRate, numSamples, samples);
					}
				}

				Status("Skipping nonsense...", 1);
				// Picture stuff, yay.
				// Skip full-screen garbage.
				var skipFullNames = new HashSet<string>() { "HELP", "HELP1", "HELP2", "BOSSBACK", "TITLEPIC", "CREDIT", "VICTORY2", "PFUB1", "PFUB2" };

				while (skipFullNames.Contains(lumpName = LumpPeek(wad, ref lumpPos)))
					LumpExpect(wad, ref lumpPos, lumpName);

				// Skip ENDx names
				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("END"))
					LumpExpect(wad, ref lumpPos, lumpName);

				Status("Parsing status bar images...", 1);
				// We actually want AMMNUMx
				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("AMMNUM"))
				{
					lump = LumpExpect(wad, ref lumpPos, lumpName);

					WriteSimpleDoomTexture(wad, lump, "pics/doom/" + lumpName + ".tga");
				}

				// Status bar stuff
				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("ST"))
				{
					lump = LumpExpect(wad, ref lumpPos, lumpName);

					WriteSimpleDoomTexture(wad, lump, "pics/doom/" + lumpName + ".tga");
				}

				Status("Skipping nonsense...", 1);
				// Skip M_x names
				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("M_"))
					LumpExpect(wad, ref lumpPos, lumpName);

				// Skip BRDRx names
				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("BRDR"))
					LumpExpect(wad, ref lumpPos, lumpName);

				// Skip WIx names
				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("WI"))
					LumpExpect(wad, ref lumpPos, lumpName);

				// Skip CWILVx names
				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("CWILV"))
					LumpExpect(wad, ref lumpPos, lumpName);

				// Skip INTERPIC
				LumpExpect(wad, ref lumpPos, "INTERPIC");

				Status("Parsing sprites...", 1);

				var sprites_to_save = new HashSet<string>
				{
					// first person weap
					"PUNG",
					"MISG",
					"MISF",
					"SHTG",
					"SHTF",
					"SHT2",
					"PLSG",
					"PLSF",
					"BFGG",
					"BFGF",
					"SAWG",
					"PISG",
					"PISF",
					"CHGG",
					"CHGF",

					// ground items
					"SHOT",
					"MGUN",
					"LAUN",
					"CSAW",

					"CLIP",
					"SHEL",
					"ROCK",
					"STIM",
					"MEDI",
					"ARM1",
					"ARM2",
					"BPAK",
					"BROK",
					"AMMO",
					"SBOX",
					"SUIT",
					"PINS",
					"BON1",
					"SOUL",
					"BON2",
					"SGN2",
					"PLAS",
					"BFUG",
					"CELL",
					"PSTR",
					"CELP",
					"PINV",
					"MEGA",

					// projectiles and stuff
					"MISL",
					"PUFF",
					"BLUD",
					"TFOG",
					"PLSS",
					"PLSE",
					"BFS1",
					"BFE1",
					"BFE2",
					
					// big stuff
					"PLAY"
				};

				var spritelist = new Dictionary<string, List<WADFile>>();

				// Now we're at sprites, yay
				DoomLumpMarkers(wad, ref lumpPos, "S_", (lmp) =>
				{
					var spriteName = lmp.Name.Substring(0, 4);

					if (true)//sprites_to_save.Contains(spriteName))
					{
						List<WADFile> wadList;

						if (!spritelist.TryGetValue(spriteName, out wadList))
							spritelist.Add(spriteName, wadList = new List<WADFile>());

						wadList.Add(lmp);
					}
				});

				foreach (var sprite in spritelist)
				{
					List<DoomPicture> pics = new List<DoomPicture>();
					int num_frames = 0;
					var frameMappings = new Dictionary<int, Tuple<int, bool>[]>();

					Status("Parsing sprite " + sprite.Key + "...", 2);

					for (int i = 0; i < sprite.Value.Count; ++i)
					{
						var pic = sprite.Value[i];
						pics.Add(new DoomPicture(wad, pic));

						var framedir = pic.Name.Substring(4);

						int frame = framedir[0] - 'A';
						int dir = int.Parse(framedir[1].ToString());

						num_frames = Math.Max(num_frames, frame + 1);

						Tuple<int, bool>[] mapping;

						if (!frameMappings.TryGetValue(frame, out mapping))
						{
							mapping = new Tuple<int, bool>[8];

							for (int d = 0; d < 8; ++d)
								mapping[d] = Tuple.Create(-1, false);

							frameMappings.Add(frame, mapping);
						}

						if (dir == 0)
						{
							for (int d = 0; d < 8; ++d)
								mapping[d] = Tuple.Create(i, false);
						}
						else
						{
							mapping[dir - 1] = Tuple.Create(i, false);

							if (framedir.Length == 4)
							{
								frame = framedir[2] - 'A';
								dir = int.Parse(framedir[3].ToString());

								num_frames = Math.Max(num_frames, frame + 1);

								if (!frameMappings.TryGetValue(frame, out mapping))
								{
									mapping = new Tuple<int, bool>[8];

									for (int d = 0; d < 8; ++d)
										mapping[d] = Tuple.Create(-1, false);

									frameMappings.Add(frame, mapping);
								}

								mapping[dir - 1] = Tuple.Create(i, true);
							}
						}
					}

					var actual_max = frameMappings.Keys.Max() + 1;

					if (actual_max != num_frames ||
						frameMappings.Count != num_frames)
						throw new Exception();

					for (var i = 0; i < num_frames; ++i)
					{
						if (!frameMappings.ContainsKey(i))
							throw new Exception();

						foreach (var mapping in frameMappings)
							if (mapping.Key == -1)
								throw new Exception();
					}

					var fileBase = "sprites/doom/" + sprite.Key.Substring(0, 4);
					var spriteFile = fileBase + ".d2s";
					CreateDirectory(Path.GetDirectoryName(spriteFile));

					int picIndex = 0;
					foreach (var pic in pics)
						wad.WriteImageTo(pic.Pixels, PaletteID.Doom, fileBase + "_" + picIndex++ + ".tga", pic.Width, pic.Height);

					using (var bw = new BinaryWriter(OpenFileStream(spriteFile)))
					{
						bw.Write(Encoding.ASCII.GetBytes("D2SP"));
						bw.Write((byte)pics.Count);

						foreach (var pic in pics)
						{
							bw.Write(pic.X);
							bw.Write(pic.Y);
						}

						bw.Write((byte)actual_max);
						bw.Write((byte)8);

						foreach (var frame in frameMappings.Values)
						{
							foreach (var dir in frame)
							{
								bw.Write((byte)dir.Item1);
								bw.Write((byte)(dir.Item2 == true ? 1 : 0));
							}
						}
					}
				}

				var mapPatches = new Dictionary<string, DoomPicture>();

				Status("Parsing patches...", 1);
				// Patches
				DoomLumpMarkers(wad, ref lumpPos, "P_", (lmp) =>
				{
					if (lmp.Length == 0)
						return;

					var patch = new DoomPicture(wad, lmp);

					if (mapPatches.ContainsKey(lmp.Name.ToLower()))
						mapPatches[lmp.Name.ToLower()] = patch;
					else
						mapPatches.Add(lmp.Name.ToLower(), patch);
				});

				var animated_textures = new Dictionary<string, string>
				{
					{ "BLODGR1", "BLODGR2" },
					{ "BLODGR2", "BLODGR3" },
					{ "BLODGR3", "BLODGR4" },
					{ "BLODGR4", "BLODGR1" },

					{ "BLODRIP1", "BLODRIP2" },
					{ "BLODRIP2", "BLODRIP3" },
					{ "BLODRIP3", "BLODRIP4" },
					{ "BLODRIP4", "BLODRIP1" },

					{ "FIREBLU1", "FIREBLU2" },
					{ "FIREBLU2", "FIREBLU1" },

					{ "FIRLAV3", "FIRELAVA" },
					{ "FIRELAVA", "FIRLAV3" },

					{ "FIREMAG1", "FIREMAG2" },
					{ "FIREMAG2", "FIREMAG3" },
					{ "FIREMAG3", "FIREMAG1" },

					{ "FIREWALA", "FIREWALB" },
					{ "FIREWALB", "FIREWALL" },
					{ "FIREWALL", "FIREWALA" },

					{ "GSTFONT1", "GSTFONT2" },
					{ "GSTFONT2", "GSTFONT3" },
					{ "GSTFONT3", "GSTFONT1" },

					{ "ROCKRED1", "ROCKRED2" },
					{ "ROCKRED2", "ROCKRED3" },
					{ "ROCKRED3", "ROCKRED1" },

					{ "SLADRIP1", "SLADRIP2" },
					{ "SLADRIP2", "SLADRIP3" },
					{ "SLADRIP3", "SLADRIP1" },

					{ "BFALL1", "BFALL2" },
					{ "BFALL2", "BFALL3" },
					{ "BFALL3", "BFALL4" },
					{ "BFALL4", "BFALL1" },

					{ "SFALL1", "SFALL2" },
					{ "SFALL2", "SFALL3" },
					{ "SFALL3", "SFALL4" },
					{ "SFALL4", "SFALL1" },

					{ "WFALL1", "WFALL2" },
					{ "WFALL2", "WFALL3" },
					{ "WFALL3", "WFALL4" },
					{ "WFALL4", "WFALL1" },

					{ "DBRAIN1", "DBRAIN2" },
					{ "DBRAIN2", "DBRAIN3" },
					{ "DBRAIN3", "DBRAIN4" },
					{ "DBRAIN4", "DBRAIN1" }
				};
				
				// now that we have patches, we can write textures!
				foreach (var texture in mapTextures)
				{
					Status("Unpacking texture " + texture.Name + "...", 2);
					UnpackDoomTexture(texture, animated_textures, mapPatches, mapPatchNames);
				}

				// Flats
				var flats = new Dictionary<WADFile, byte[]>();
				var animated = new Dictionary<string, string>
				{
					{ "NUKAGE1", "NUKAGE2" },
					{ "NUKAGE2", "NUKAGE3" },
					{ "NUKAGE3", "NUKAGE1" },

					{ "FWATER1", "FWATER2" },
					{ "FWATER2", "FWATER3" },
					{ "FWATER3", "FWATER4" },
					{ "FWATER4", "FWATER1" },

					{ "LAVA1", "LAVA2" },
					{ "LAVA2", "LAVA3" },
					{ "LAVA3", "LAVA4" },
					{ "LAVA4", "LAVA1" },

					{ "BLOOD1", "BLOOD2" },
					{ "BLOOD2", "BLOOD3" },
					{ "BLOOD3", "BLOOD1" },

					{ "RROCK05", "RROCK06" },
					{ "RROCK06", "RROCK07" },
					{ "RROCK07", "RROCK08" },
					{ "RROCK08", "RROCK05" },

					{ "SLIME01", "SLIME02" },
					{ "SLIME02", "SLIME03" },
					{ "SLIME03", "SLIME04" },
					{ "SLIME04", "SLIME01" },

					{ "SLIME05", "SLIME06" },
					{ "SLIME06", "SLIME07" },
					{ "SLIME07", "SLIME08" },
					{ "SLIME08", "SLIME05" },

					{ "SLIME09", "SLIME10" },
					{ "SLIME10", "SLIME11" },
					{ "SLIME11", "SLIME12" },
					{ "SLIME12", "SLIME09" }
				};

				Status("Parsing flats...", 1);
				DoomLumpMarkers(wad, ref lumpPos, "F_", (lmp) =>
				{
					if (lmp.Length == 0)
						return;

					wad.Reader.BaseStream.Position = lmp.Offset;
					flats.Add(lmp, wad.Reader.ReadBytes(64 * 64));
				});

				// Write flats
				foreach (var flat in flats)
				{
					var name = flat.Key.Name;
					string outwal = "textures/doom/" + name + ".wal";

					Status("Writing flat " + name + "...", 2);

					if (name == "FLOOR7_2")
						wad.WriteImageTo(flat.Value, PaletteID.Doom, "pics/doom/backtile.tga", 64, 64);

					if (FileExists(outwal))
						continue;

					// Test PNGs
					string outpng = "textures/doom/" + name + ".tga";

					using (var bmp = new Image<Rgba32>(64, 64))
					{
						for (var y = 0; y < 64; ++y)
							for (var x = 0; x < 64; ++x)
							{
								var c = flat.Value[(y * 64) + x];
								bmp[x, y] = Globals.GetMappedColor(PaletteID.Doom, c);
							}

						using (var stream = OpenFileStream(outpng))
							bmp.SaveAsTga(stream);
					}

					// Write WALs
					string walname = name, next_name;

					if (!animated.TryGetValue(flat.Key.Name, out next_name))
						next_name = "";

					outwal = "textures/doom/" + walname + ".wal";
					SaveWal(outwal, walname, next_name, 0, 0, flat.Value, PaletteID.Doom, 64, 64);
				}
			}

			ClearStatus();
			Status("Done!", 0);
		}

		public override void Run()
		{
			D2CopyStuff(Globals.DoomPath + "\\doom2.wad");
		}
	}
}
