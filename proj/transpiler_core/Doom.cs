using SixLabors.ImageSharp.Advanced;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using ImageType = SixLabors.ImageSharp.Image<SixLabors.ImageSharp.PixelFormats.Bgra32>;

namespace asset_transpiler
{
	class FontCompiler
	{
		public IDictionary<int, ImageType> Charmap { get; } = new SortedDictionary<int, ImageType>();

		public FontCompiler()
		{
		}

		public void CompileTo(string output)
		{
			// make highbit chars
			foreach (var ch in Charmap.AsEnumerable().ToArray())
			{
				if (ch.Key > 128)
					continue;

				var img = ch.Value.Clone();
				var span = img.GetPixelSpan();

				for (var i = 0; i < span.Length; i++)
				{
					if (span[i].A == 0)
						continue;
					span[i].R = (byte)(span[i].R * 0.1);
					span[i].B = (byte)(span[i].B * 0.1);
				}

				Charmap.Add(ch.Key | 128, img);
			}

			// copy low chars to high chars
			for (var i = 'a'; i <= 'z'; i++)
			{
				var high = i - ('a' - 'A');

				if (!Charmap.ContainsKey(i))
					Charmap.Add(i, Charmap[high]);

				if (!Charmap.ContainsKey(i | 128))
					Charmap.Add(i | 128, Charmap[high | 128]);
			}

			// fill in empty slots from 0 to 255
			using (var pak = new PAK(GameParser.OpenArchive(Globals.GetGameInfo(PaletteID.Q2).Path + Path.DirectorySeparatorChar + "pak0.pak")))
			{
				pak.Reader.BaseStream.Position = pak.Get("pics/conchars.pcx").Offset;

				using (var q2Conchars = ImageExtensions.LoadFromPCX(pak.Reader))
					for (var i = 0; i < 255; i++)
						if (!Charmap.ContainsKey(i))
							Charmap.Add(i, q2Conchars.MakeCropped((i % 16) * 8, (i / 16) * 8, 8, 8));
			}

			var charmapWidth = Charmap.Aggregate(0, (a, b) => a + b.Value.Width);
			var largestHeight = Charmap.Max(a => a.Value.Height);
			using (var pixels = new ImageType(charmapWidth, largestHeight))
			{
				var chX = 0;

				using (var font = new BinaryWriter(GameParser.OpenFileStream(output + ".font", 256)))
					foreach (var ch in Charmap)
					{
						var dx = chX;
						var dy = (ch.Key == '_' || ch.Key == ',' || ch.Key == '.') ? (largestHeight - ch.Value.Height) : 0;

						for (var ly = 0; ly < ch.Value.Height; ly++)
							for (var lx = 0; lx < ch.Value.Width; lx++)
								pixels[dx + lx, dy + ly] = ch.Value[lx, ly];

						chX += ch.Value.Width;
						font.Write((byte)ch.Value.Width);
					}

				foreach (var ch in Charmap)
					ch.Value.Dispose();

				ArchiveBase.WriteImageTo(pixels, output + "." + ImageExtensions.WrittenImageFormat, charmapWidth, largestHeight);
			}
		}
	}

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

		public DoomPicture(BinaryReader reader, WADFile file)
		{
			reader.BaseStream.Position = file.Offset;
			Width = reader.ReadInt16();
			Height = reader.ReadInt16();

			X = reader.ReadInt16();
			Y = reader.ReadInt16();

			var columns = new int[Width];

			for (var i = 0; i < Width; ++i)
				columns[i] = reader.ReadInt32();

			Pixels = new byte[Width * Height];

			for (var i = 0; i < Pixels.Length; ++i)
				Pixels[i] = 255;

			for (var i = 0; i < Width; ++i)
			{
				reader.BaseStream.Position = file.Offset + columns[i];
				var rowstart = 0;

				while (rowstart != 255)
				{
					rowstart = reader.ReadByte();

					if (rowstart == 255)
						break;

					var pixel_count = reader.ReadByte();
					reader.ReadByte();

					for (int j = 0; j < pixel_count; ++j)
						Pixels[((j + rowstart) * Width) + i] = reader.ReadByte();

					reader.ReadByte();
				}
			}
		}

		public override string ToString()
		{
			return Width + " x " + Height;
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

		static void DoomLumpMarkers(WAD wad, ref int lumpPos, string prefix, Action<WADFile> callback)
		{
			LumpExpect(wad, ref lumpPos, prefix + "START");
			string lumpName;

			while ((lumpName = LumpPeek(wad, ref lumpPos)) != prefix + "END")
				callback(LumpExpect(wad, ref lumpPos, lumpName));

			LumpExpect(wad, ref lumpPos, prefix + "END");
		}

		void TaskDoomSimpleTexture((string wadfile, WADFile lump, string output) info)
		{
			using (var wad = new BinaryReader(OpenArchive(info.wadfile)))
			{
				var picture = new DoomPicture(wad, info.lump);
				ArchiveBase.WriteImageTo(picture.Pixels, PaletteID.Doom, info.output, picture.Width, picture.Height);
			}
		}

		void WriteSimpleDoomTexture(string wadfile, WADFile file, string output)
		{
			CreateTask(TaskDoomSimpleTexture, (wadfile, file, output));
		}

		static void LumpLoadTexture(WAD wad, ref int lumpPos, List<DoomMapTexture> mapTextures, string name)
		{
			var file = LumpExpect(wad, ref lumpPos, name);

			wad.Reader.BaseStream.Position = file.Offset;

			var numTextures = wad.Reader.ReadInt32();
			var offsets = new int[numTextures];

			for (var i = 0; i < numTextures; ++i)
				offsets[i] = wad.Reader.ReadInt32();

			for (var i = 0; i < numTextures; ++i)
			{
				wad.Reader.BaseStream.Position = file.Offset + offsets[i];
				mapTextures.Add(new DoomMapTexture(wad));
			}
		}

		static void LumpLoadPatchNames(WAD wad, ref int lumpPos, List<string> mapPatchNames, string name)
		{
			var file = LumpExpect(wad, ref lumpPos, name);

			wad.Reader.BaseStream.Position = file.Offset;

			var numPatches = wad.Reader.ReadInt32();

			for (var i = 0; i < numPatches; ++i)
			{
				string pname = Encoding.ASCII.GetString(wad.Reader.ReadBytes(8));

				if (pname.IndexOf('\0') != -1)
					pname = pname.Substring(0, pname.IndexOf('\0'));

				mapPatchNames.Add(pname.ToLower());
			}
		}

		void TaskDoomPalette((string wadfile, WADFile lump) info)
		{
			using (var wad = new BinaryReader(OpenArchive(info.wadfile)))
			{
				wad.BaseStream.Position = info.lump.Offset;

				using (var wr = new BinaryWriter(OpenFileStream("pics/doom/palette.dat")))
					wr.Write(wad.ReadBytes(256 * 3));
			}
		}

		void TaskDoomSprite((string wadfile, string name, List<WADFile> files) info)
		{
			using (var wad = new BinaryReader(OpenArchive(info.wadfile)))
			{
				var pics = new List<DoomPicture>();
				var num_frames = 0;
				var frameMappings = new Dictionary<int, Tuple<int, bool>[]>();
				
				for (int i = 0; i < info.files.Count; ++i)
				{
					var pic = info.files[i];
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

				var fileBase = "sprites/doom/" + info.name.Substring(0, 4);
				var spriteFile = fileBase + ".d2s";

				int picIndex = 0;
				foreach (var pic in pics)
					ArchiveBase.WriteImageTo(pic.Pixels, PaletteID.Doom, fileBase + "_" + picIndex++ + "." + ImageExtensions.WrittenImageFormat, pic.Width, pic.Height);

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
						foreach (var dir in frame)
						{
							bw.Write((byte)dir.Item1);
							bw.Write((byte)(dir.Item2 == true ? 1 : 0));
						}
				}
			}
		}

		Dictionary<string, WADFile> mapPatches = new Dictionary<string, WADFile>();
		List<string> mapPatchNames = new List<string>();

		static readonly IReadOnlyDictionary<string, string> _animatedPatches = new Dictionary<string, string>
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

		static readonly IReadOnlyDictionary<string, string> _animatedFlats = new Dictionary<string, string>
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

		void TaskUnpackTexture((string wadfile, DoomMapTexture texture) info)
		{
			var pixels = new byte[info.texture.Width * info.texture.Height];

			for (var i = 0; i < pixels.Length; ++i)
				pixels[i] = 255;

			using (var wad = new BinaryReader(OpenArchive(info.wadfile)))
				for (var i = 0; i < info.texture.Patches.Length; ++i)
				{
					var patchData = info.texture.Patches[i];
					var patchFile = mapPatches[mapPatchNames[patchData.Patch]];
					var patch = new DoomPicture(wad, patchFile);

					ByteBuffer.BoxCopy(patch.Pixels, 0, 0, patch.Width, patch.Height, pixels, patchData.OriginX, patchData.OriginY, info.texture.Width, info.texture.Height, patch.Width, patch.Height, true);
				}

			var name = info.texture.Name;

			// Write WAL
			if (!_animatedPatches.TryGetValue(info.texture.Name, out var next_name))
				next_name = "";

			var bmp = new ImageType(info.texture.Width, info.texture.Height);
			var span = bmp.GetPixelSpan();

			for (var y = 0; y < info.texture.Height; ++y)
				for (var x = 0; x < info.texture.Width; ++x)
				{
					var c = (y * info.texture.Width) + x;
					span[c] = Globals.GetMappedColor(PaletteID.Doom, pixels[c]);
				}

			SaveWalTga(bmp, "textures/doom/" + name + "." + ImageExtensions.WrittenImageFormat, "textures/doom/" + name + ".wal", name, next_name, 0, 0, pixels, PaletteID.Doom, info.texture.Width, info.texture.Height);
		}
		
		void TaskBacktile(byte[] flatBuffer)
		{
			ArchiveBase.WriteImageTo(flatBuffer, PaletteID.Doom, "pics/doom/backtile." + ImageExtensions.WrittenImageFormat, 64, 64);
		}

		void TaskDoomFlat((string wadfile, WADFile file) info)
		{
			var flatBuffer = new byte[64 * 64];

			var name = info.file.Name;

			using (var wad = new BinaryReader(OpenArchive(info.wadfile)))
			{
				wad.BaseStream.Position = info.file.Offset;
				wad.Read(flatBuffer, 0, flatBuffer.Length);
			}

			// Write WALs
			if (!_animatedFlats.TryGetValue(info.file.Name, out var next_name))
				next_name = "";

			if (name == "FLOOR7_2")
				CreateTask(TaskBacktile, flatBuffer);

			var bmp = new ImageType(64, 64);
			var span = bmp.GetPixelSpan();

			for (var y = 0; y < 64; ++y)
				for (var x = 0; x < 64; ++x)
				{
					var c = (y * 64) + x;
					span[c] = Globals.GetMappedColor(PaletteID.Doom, flatBuffer[c]);
				}
				
			SaveWalTga(bmp, "textures/doom/" + name + "." + ImageExtensions.WrittenImageFormat, "textures/doom/" + name + ".wal", name, next_name, 0, 0, flatBuffer, PaletteID.Doom, 64, 64);
		}

		void TaskDoomSound((string wadfile, string lumpName, WADFile lump) info)
		{
			using (var wad = new BinaryReader(OpenArchive(info.wadfile)))
			{
				wad.BaseStream.Position = info.lump.Offset;

				wad.ReadUInt16();
				var sampleRate = wad.ReadUInt16();
				var numSamples = wad.ReadUInt16();
				wad.ReadUInt16();
				var samples = wad.ReadBytes(numSamples);

				var outwav = "sound/doom/" + info.lumpName.Substring(2) + ".wav";

				WriteWav(outwav, sampleRate, numSamples, samples);
			}
		}

		void TaskDoomConchars((string wadfile, List<WADFile> chars) info)
		{
			var compiler = new FontCompiler();

			using (var wad = new BinaryReader(OpenArchive(info.wadfile)))
			{
				foreach (var ch in info.chars)
				{
					var i = int.Parse(ch.Name.Substring(5));
					var pic = new DoomPicture(wad, ch);
					var image = new ImageType(pic.Width, pic.Height);
					var span = image.GetPixelSpan();

					for (var p = 0; p < pic.Width * pic.Height; p++)
						span[p] = Globals.GetMappedColor(PaletteID.Doom, pic.Pixels[p]).Grayscale();

					compiler.Charmap.Add(i, image);
				}

				if (compiler.Charmap.ContainsKey((int)'y') && !(compiler.Charmap.ContainsKey((int)'x') && compiler.Charmap.ContainsKey((int)'z')))
				{
					compiler.Charmap.Add(124, compiler.Charmap[(int)'y']);
					compiler.Charmap.Remove((int)'y');
				}


				compiler.CompileTo("pics/doom/conchars");
			}
		}

		void TaskWad(string wadfile)
		{
			using (var wad = new WAD(OpenArchive(wadfile)))
			{
				int lumpPos = 0;
				string lumpName;

				// read PLAYPAL lump
				var lump = LumpExpect(wad, ref lumpPos, "PLAYPAL");

				CreateTask(TaskDoomPalette, (wadfile, lump));

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

				// Load TEXTURE1
				LumpLoadTexture(wad, ref lumpPos, mapTextures, "TEXTURE1");

				// Get rid of AASHITTY
				mapTextures.RemoveAt(0);

				LumpLoadPatchNames(wad, ref lumpPos, mapPatchNames, "PNAMES");

				// Skip music stuff
				LumpExpect(wad, ref lumpPos, "GENMIDI");
				LumpExpect(wad, ref lumpPos, "DMXGUSC");

				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("D"))
				{
					lump = LumpExpect(wad, ref lumpPos, lumpName);

					// Sounds
					if (lumpName.StartsWith("DS"))
						CreateTask(TaskDoomSound, (wadfile, lumpName, lump));
				}

				// Picture stuff, yay.
				// Skip full-screen garbage.
				var skipFullNames = new HashSet<string>() { "HELP", "HELP1", "HELP2", "BOSSBACK", "TITLEPIC", "CREDIT", "VICTORY2", "PFUB1", "PFUB2" };

				while (skipFullNames.Contains(lumpName = LumpPeek(wad, ref lumpPos)))
					LumpExpect(wad, ref lumpPos, lumpName);

				// Skip ENDx names
				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("END"))
					LumpExpect(wad, ref lumpPos, lumpName);

				// We actually want AMMNUMx
				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("AMMNUM"))
				{
					lump = LumpExpect(wad, ref lumpPos, lumpName);

					WriteSimpleDoomTexture(wadfile, lump, "pics/doom/" + lumpName + "." + ImageExtensions.WrittenImageFormat);
				}

				// Status bar stuff
				List<WADFile> _conchars = new List<WADFile>();

				while ((lumpName = LumpPeek(wad, ref lumpPos)).StartsWith("ST"))
				{
					lump = LumpExpect(wad, ref lumpPos, lumpName);

					if (lumpName.StartsWith("STCFN"))
						_conchars.Add(lump);
					else
						WriteSimpleDoomTexture(wadfile, lump, "pics/doom/" + lumpName + "." + ImageExtensions.WrittenImageFormat);
				}

				CreateTask(TaskDoomConchars, (wadfile, _conchars));

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

				var spritelist = new Dictionary<string, List<WADFile>>();

				// Now we're at sprites, yay
				DoomLumpMarkers(wad, ref lumpPos, "S_", (lmp) =>
				{
					var spriteName = lmp.Name.Substring(0, 4);

					if (!spritelist.TryGetValue(spriteName, out var wadList))
						spritelist.Add(spriteName, wadList = new List<WADFile>());

					wadList.Add(lmp);
				});

				foreach (var sprite in spritelist)
					CreateTask(TaskDoomSprite, (wadfile, sprite.Key, sprite.Value));

				// Patches
				DoomLumpMarkers(wad, ref lumpPos, "P_", lmp =>
				{
					if (lmp.Length == 0)
						return;

					var lower = lmp.Name.ToLower();

					if (!mapPatches.TryAdd(lower, lmp))
						mapPatches[lower] = lmp;
				});

				// now that we have patches, we can write textures!
				foreach (var texture in mapTextures)
					CreateTask(TaskUnpackTexture, (wadfile, texture));

				// Flats
				DoomLumpMarkers(wad, ref lumpPos, "F_", lmp =>
				{
					if (lmp.Length != 0)
						CreateTask(TaskDoomFlat, (wadfile, lmp));
				});
			}
		}

		public override void Run()
		{
			CreateTask(TaskWad, Globals.GetGameInfo(PaletteID.Doom).Path + "\\doom2.wad");
		}
	}
}
