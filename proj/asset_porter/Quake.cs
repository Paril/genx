using SixLabors.ImageSharp;
using SixLabors.ImageSharp.PixelFormats;
using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Linq;

namespace asset_transpiler
{
	class Q1BSPTextureAnim : IComparable<Q1BSPTextureAnim>
	{
		public char Frame;
		public byte[] Data;

		public int CompareTo(Q1BSPTextureAnim other)
		{
			return Frame.CompareTo(other.Frame);
		}

		public Q1BSPTextureAnim Crop(Q1BSPTexture texture, uint x, uint y, uint w, uint h)
		{
			Q1BSPTextureAnim anim = new Q1BSPTextureAnim();

			anim.Frame = Frame;
			anim.Data = new byte[w * h];

			ByteBuffer.BoxCopy(Data, (int)x, (int)y, (int)texture.Width, (int)texture.Height, anim.Data, 0, 0, (int)w, (int)h, (int)w, (int)h);

			return anim;
		}

		public Q1BSPTextureAnim Extend(Q1BSPTexture texture, int scaleX, int scaleY)
		{
			Q1BSPTextureAnim anim = new Q1BSPTextureAnim();

			anim.Frame = Frame;
			anim.Data = new byte[texture.Width * scaleX * texture.Height * scaleY];

			for (int y = 0; y < scaleY; ++y)
				for (var x = 0; x < scaleX; ++x)
				{
					ByteBuffer.BoxCopy(Data, 0, 0, (int)texture.Width, (int)texture.Height, anim.Data, (int)(x * texture.Width), (int)(y * texture.Height), (int)(scaleX * texture.Width), (int)(scaleY * texture.Height), (int)texture.Width, (int)texture.Height);
				}

			return anim;
		}
	}

	class Q1BSPTexture
	{
		public List<Q1BSPTextureAnim> Frames = new List<Q1BSPTextureAnim>();
		public uint Width;
		public uint Height;

		public Q1BSPTexture Crop(uint x, uint y, uint w, uint h)
		{
			Q1BSPTexture texture = new Q1BSPTexture();

			texture.Width = w;
			texture.Height = h;

			foreach (var frame in Frames)
				texture.Frames.Add(frame.Crop(this, x, y, w, h));

			return texture;
		}

		public Q1BSPTexture Extend(int scaleX, int scaleY)
		{
			Q1BSPTexture texture = new Q1BSPTexture();

			texture.Width = (uint)(Width * scaleX);
			texture.Height = (uint)(Height * scaleY);

			foreach (var frame in Frames)
				texture.Frames.Add(frame.Extend(this, scaleX, scaleY));

			return texture;
		}
	}

	class Q1BSPTextureMips
	{
		public string Name;
		public uint Width;
		public uint Height;

		public Q1BSPTexture[] Mips = new Q1BSPTexture[4];
	}

	class Q1BSP : IDisposable
	{
		public List<Q1BSPTextureMips> Textures = new List<Q1BSPTextureMips>();
		public BinaryReader Reader;

		struct EntryDef
		{
			public int Offset;
			public uint Size;
		}

		EntryDef ReadEntryDef()
		{
			return new EntryDef() { Offset = Reader.ReadInt32(), Size = Reader.ReadUInt32() };
		}

		public Q1BSP(Stream stream)
		{
			Reader = new BinaryReader(stream);

			var version = Reader.ReadUInt32();

			var entities = ReadEntryDef(); // entities
			var planes = ReadEntryDef(); // planes
			var miptex = ReadEntryDef(); // miptex
			var vertices = ReadEntryDef(); // vertices
			var visilist = ReadEntryDef(); // visilist
			var nodes = ReadEntryDef(); // nodes
			var texinfo = ReadEntryDef(); // texinfo
			var faces = ReadEntryDef(); // faces
			var lightmap = ReadEntryDef(); // lightmap
			var clipnode = ReadEntryDef(); // clipnode
			var leaves = ReadEntryDef(); // leaves
			var lface = ReadEntryDef(); // lface
			var edges = ReadEntryDef(); // edges
			var ledges = ReadEntryDef(); // ledges
			var models = ReadEntryDef(); // models

			Reader.BaseStream.Position = miptex.Offset;

			uint numtex = Reader.ReadUInt32();
			int[] offsets = new int[numtex];

			for (uint i = 0; i < numtex; ++i)
				offsets[i] = Reader.ReadInt32();

			for (uint i = 0; i < numtex; ++i)
			{
				if (offsets[i] == -1)
					continue;

				Reader.BaseStream.Position = miptex.Offset + offsets[i];

				var name = Encoding.ASCII.GetString(Reader.ReadBytes(16));

				if (name.IndexOf('\0') != -1)
					name = name.Substring(0, name.IndexOf('\0'));

				var width = Reader.ReadUInt32();
				var height = Reader.ReadUInt32();
				var mipoffsets = new int[] { Reader.ReadInt32(), Reader.ReadInt32(), Reader.ReadInt32(), Reader.ReadInt32() };
				char anim = ' ';

				if (name.StartsWith("+"))
				{
					anim = name[1];
					name = name.Substring(2);
				}

				Q1BSPTextureMips texture = null;

				foreach (var tex in Textures)
				{
					if (tex.Name == name)
					{
						texture = tex;
						break;
					}
				}

				if (texture == null)
				{
					texture = new Q1BSPTextureMips();
					texture.Name = name;
					texture.Width = width;
					texture.Height = height;
					Textures.Add(texture);

					for (int m = 0; m < mipoffsets.Length; ++m)
					{
						texture.Mips[m] = new Q1BSPTexture();
						texture.Mips[m].Width = width >> m;
						texture.Mips[m].Height = height >> m;
					}
				}

				for (int m = 0; m < mipoffsets.Length; ++m)
				{
					uint mw = width >> m;
					uint mh = height >> m;
					int offset = mipoffsets[m];

					Q1BSPTextureAnim animation = new Q1BSPTextureAnim();
					animation.Frame = anim;
					Reader.BaseStream.Position = miptex.Offset + offsets[i] + offset;
					animation.Data = Reader.ReadBytes((int)(mw * mh));
					texture.Mips[m].Frames.Add(animation);
				}
			}

			foreach (var tex in Textures)
				foreach (var mip in tex.Mips)
					mip.Frames.Sort();
		}

		public void Dispose()
		{
			Reader.Dispose();
		}

		public Q1BSPTextureMips GetTexture(string side_texture)
		{
			foreach (var texture in Textures)
				if (texture.Name == side_texture)
					return texture;

			throw new IndexOutOfRangeException();
		}
	}

	class ModelFace
	{
		public float[,] Vertices;
		public float[,] TexCoords;

		public byte[] VertIndexes = new byte[4];
	}

	class Quake : GameParser
	{
		static uint[] GetTextureDimensions(params Q1BSPTexture[] textures)
		{
			uint[] p = new uint[2] { 0, 0 };

			foreach (var texture in textures)
			{
				p[0] += texture.Width;
				p[1] = Math.Max(p[1], texture.Height);
			}

			return p;
		}

		static HashSet<string> q1FilesWeNeed = new HashSet<string>()
		{
			"gfx.wad",
			"gfx/palette.lmp",
			"gfx/conback.lmp",

			"maps/b_*.bsp",

			"progs/*.mdl",
			"progs/*.spr",

			"sound/*.wav"
		};

		static string WildcardToRegex(string pattern)
		{
			return "^" + Regex.Escape(pattern).
			Replace("\\*", ".*").
			Replace("\\?", ".") + "$";
		}
		
		Dictionary<string, IList<byte[]>> _textureHashes = new Dictionary<string, IList<byte[]>>();

		void Q1CopyStuff(string file)
		{
			Status("Parsing " + file, 0);

			using (var pak = new PAK(this, File.Open(file, FileMode.Open, FileAccess.Read, FileShare.ReadWrite)))
			{
				foreach (var pakfile in pak.Files)
				{
					Status("Checking " + pakfile.Name + "...", 1);

					if (pakfile.Name.EndsWith(".bsp") && !Path.GetFileName(pakfile.Name).StartsWith("b_"))
					{
						Status("BSP: " + pakfile.Name, 1);
						Status("Parsing BSP...", 2);

						using (var bsp = new Q1BSP(new MemoryStream(pak.GetBytes(pakfile))))
						{
							CreateDirectory("textures/q1/");

							Status("BSP: " + pakfile.Name + " - compiling textures...", 2);

							foreach (var texture in bsp.Textures)
							{
								var name = texture.Name.Trim('*');
			
								using (var sha256 = SHA256.Create())
								{
									var hash = sha256.ComputeHash(texture.Mips[0].Frames[0].Data);

									if (_textureHashes.ContainsKey(name))
									{
										var otherHashes = _textureHashes[name];
										var doesMatch = false;

										foreach (var otherHash in otherHashes)
										{
											if (hash.SequenceEqual(otherHash))
											{
												doesMatch = true;
												break;
											}
										}

										if (doesMatch)
											continue;

										otherHashes.Add(hash);
									}
									else
										_textureHashes.Add(name, new List<byte[]>(new byte[][] { hash }));
								}

								name = Path.GetFileNameWithoutExtension(pakfile.Name) + "/" + name;

								string outwal = "textures/q1/" + name + ".wal";
								string outpng = "textures/q1/" + name + ".tga";

								Status(name, 3);

								if (FileExists(outpng))
									continue;

								// Write WALs
								for (var f = 0; f < texture.Mips[0].Frames.Count; ++f)
								{
									string walname, next_name;

									if (f == 0)
									{
										walname = "q1/" + name;

										if (texture.Mips[0].Frames.Count == 1)
											next_name = "";
										else
											next_name = "q1/" + name + "_f" + (f + 1);
									}
									else
									{
										walname = "q1/" + name + "_f" + f;

										if ((f + 1) % texture.Mips[0].Frames.Count == 0)
											next_name = "q1/" + name;
										else
											next_name = "q1/" + name + "_f" + (f + 1) % texture.Mips[0].Frames.Count;
									}

									outwal = "textures/" + walname + ".wal";
									outpng = "textures/" + walname + ".tga";

									int surf = 0, contents = 0;

									if (walname == "q1/clip")
									{
										surf = Quake2.SURF_NODRAW;
										contents = Quake2.CONTENTS_PLAYERCLIP | Quake2.CONTENTS_MONSTERCLIP;
									}
									else if (walname == "q1/trigger")
										surf = Quake2.SURF_NODRAW;
									else if (walname.StartsWith("q1/slime"))
									{
										surf = Quake2.SURF_WARP | Quake2.SURF_TRANS66;
										contents = Quake2.CONTENTS_SLIME;
									}
									else if (walname.StartsWith("q1/lava"))
									{
										surf = Quake2.SURF_WARP | Quake2.SURF_TRANS66;
										contents = Quake2.CONTENTS_LAVA;
									}
									else if (texture.Name[0] == '*')
									{
										surf = Quake2.SURF_WARP | Quake2.SURF_TRANS66;
										contents = Quake2.CONTENTS_WATER;
									}
									else if (walname.StartsWith("q1/sky"))
										surf = Quake2.SURF_SKY | Quake2.SURF_NODRAW;

									using (var bmp = new Image<Rgba32>((int)texture.Width, (int)texture.Height))
									{
										for (var y = 0; y < texture.Height; ++y)
											for (var x = 0; x < texture.Width; ++x)
											{
												var c = texture.Mips[0].Frames[f].Data[(y * (int)texture.Width) + x];
												bmp[x, y] = Globals.GetMappedColor(PaletteID.Q1, c);
											}

										using (var strm = OpenFileStream(outpng))
											bmp.SaveAsTga(strm);

										SaveWal(outwal, walname, next_name, surf, contents, texture.Mips[0].Frames[f].Data, PaletteID.Q1, (int)texture.Width, (int)texture.Height);
									}
								}
							}
						}
					}

					bool matches = false;

					foreach (var match in q1FilesWeNeed)
					{
						if (Regex.IsMatch(pakfile.Name, WildcardToRegex(match)))
						{
							matches = true;
							break;
						}
					}

					if (!matches)
						continue;
					
					Status("Handling " + pakfile.Name + "...", 1);

					if (pakfile.Name.EndsWith(".mdl"))
						pak.WriteFileTo(pakfile, "models/q1/" + pakfile.Name.Substring("progs/".Length));
					else if (pakfile.Name.EndsWith(".wav"))
						pak.WriteFileTo(pakfile, "sound/q1/" + pakfile.Name.Substring("sound/".Length));
					else if (pakfile.Name.EndsWith(".spr"))
						pak.WriteFileTo(pakfile, "sprites/q1/" + pakfile.Name.Substring("progs/".Length));
					else if (pakfile.Name.EndsWith(".lmp"))
					{
						if (pakfile.Name.EndsWith("palette.lmp"))
							pak.WriteFileTo(pakfile, "pics/q1/palette.dat");
						else if (!pakfile.Name.EndsWith("colormap.lmp") && !pakfile.Name.EndsWith("pop.lmp"))
							pak.WriteImageTo(pakfile, PaletteID.Q1, "pics/q1/" + pakfile.Name.Substring("gfx/".Length, pakfile.Name.Length - 4 - "gfx/".Length) + ".tga");
					}
					else if (pakfile.Name.EndsWith(".wad"))
					{
						Status("Opening subwad...", 2);

						using (var wad = new WAD(this, new MemoryStream(pak.GetBytes(pakfile))))
						{
							foreach (WADFile wadfile in wad.Files)
							{
								Status("Handling wadfile " + wadfile.Name + "...", 3);

								switch (wadfile.Type)
								{
									case WADFileType.Pic:
									case WADFileType.Mip:
										switch (wadfile.Name.ToLower())
										{
											case "conchars":
												wad.WriteImageTo(wad.GetBytes(wadfile), PaletteID.Q1, "pics/q1/" + wadfile.Name.ToLower() + ".tga", 128, 128, color => (byte)(color == 0 ? 255 : color));
												break;
											default:
												wad.WriteImageTo(wadfile, PaletteID.Q1, "pics/q1/" + wadfile.Name.ToLower() + ".tga");
												break;
										}
										break;
								}
							}
						}
					}
					else if (Path.GetFileNameWithoutExtension(pakfile.Name).StartsWith("b_") && pakfile.Name.EndsWith(".bsp"))
					{
						Status("Parsing BSP model...", 2);

						using (var bsp = new Q1BSP(new MemoryStream(pak.GetBytes(pakfile))))
						{
							switch (Path.GetFileNameWithoutExtension(pakfile.Name))
							{
								case "b_shell0":
									{
										var side = bsp.GetTexture("shot0sid").Mips[0];
										var top = bsp.GetTexture("shot0top").Mips[0];

										side = side.Crop(0, 8, 24, 24);
										top = top.Crop(0, 8, 24, 24);

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 24, 0 },
											{ 24 + 24, 0 },
											{ 24 + 24, 24 },
											{ 24, 24 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 24, 24 },
											{ 24, 0 },
											{ 0, 0 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 24, 0 },
											{ 24, 24 },
											{ 0, 24 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 24, 0 },
											{ 24, 24 },
											{ 0, 24 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 24, 24 },
											{ 24, 0 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 24, 24, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;

								case "b_shell1":
									{
										var side = bsp.GetTexture("shot1sid").Mips[0];
										var top = bsp.GetTexture("shot1top").Mips[0];

										side = side.Crop(0, 8, 32, 24);

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 0 },
											{ 32 + 32, 0 },
											{ 32 + 32, 32 },
											{ 32, 32 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 32, 24 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 24 },
											{ 0, 24 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 24 },
											{ 0, 24 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 32, 24 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;

								case "b_nail0":
									{
										var side = bsp.GetTexture("nail0sid").Mips[0];
										var top = bsp.GetTexture("nail0top").Mips[0];

										side = side.Crop(0, 8, 24, 24);
										top = top.Crop(0, 8, 24, 24);

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 24, 0 },
											{ 24 + 24, 0 },
											{ 24 + 24, 24 },
											{ 24, 24 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 24, 24 },
											{ 24, 0 },
											{ 0, 0 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 24, 0 },
											{ 24, 24 },
											{ 0, 24 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 24, 0 },
											{ 24, 24 },
											{ 0, 24 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 24, 24 },
											{ 24, 0 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 24, 24, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;


								case "b_nail1":
									{
										var side = bsp.GetTexture("nail1sid").Mips[0];
										var top = bsp.GetTexture("nail1top").Mips[0];

										side = side.Crop(0, 8, 32, 24);

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 0 },
											{ 32 + 32, 0 },
											{ 32 + 32, 32 },
											{ 32, 32 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 32, 24 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 24 },
											{ 0, 24 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 24 },
											{ 0, 24 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 32, 24 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;

								case "b_batt0":
									{
										var side = bsp.GetTexture("batt1sid").Mips[0];
										var top = bsp.GetTexture("batt0top").Mips[0];

										side = side.Crop(0, 8, 24, 24);
										top = top.Crop(0, 8, 24, 24);

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 24, 0 },
											{ 24 + 24, 0 },
											{ 24 + 24, 24 },
											{ 24, 24 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 24, 24 },
											{ 24, 0 },
											{ 0, 0 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 24, 0 },
											{ 24, 24 },
											{ 0, 24 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 24, 0 },
											{ 24, 24 },
											{ 0, 24 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 24, 24 },
											{ 24, 0 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 24, 24, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;

								case "b_batt1":
									{
										var side = bsp.GetTexture("batt0sid").Mips[0];
										var top = bsp.GetTexture("batt1top").Mips[0];

										side = side.Crop(0, 0, 32, 24);

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 0 },
											{ 32 + 32, 0 },
											{ 32 + 32, 32 },
											{ 32, 32 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 32, 24 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 24 },
											{ 0, 24 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 24 },
											{ 0, 24 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 24 },
											{ 32, 24 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;

								case "b_rock0":
									{
										var side = bsp.GetTexture("rock0sid").Mips[0];
										var top = bsp.GetTexture("rockettop").Mips[0];

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 0 },
											{ 32 + 16, 0 },
											{ 32 + 16, 16 },
											{ 32, 16 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 16 },
											{ 32, 0 },
											{ 0, 0 },
											{ 0, 16 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 0, 16 },
											{ 32, 16 },
											{ 32, 0 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 16 },
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 16 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 0 },
											{ 32, 16 },
											{ 0, 16 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 16, 16, 32 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;

								case "b_bh10":
									{
										var side = bsp.GetTexture("med3_1").Mips[0];
										var top = bsp.GetTexture("med3_0").Mips[0];

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 0 },
											{ 32 + 32, 0 },
											{ 32 + 32, 32 },
											{ 32, 32 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 16 },
											{ 32, 16 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 16 },
											{ 0, 16 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 16 },
											{ 0, 16 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 16 },
											{ 32, 16 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 16 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;

								case "b_bh25":
									{
										var side = bsp.GetTexture("_med25s").Mips[0];
										var top = bsp.GetTexture("_med25").Mips[0];

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 0 },
											{ 32 + 32, 0 },
											{ 32 + 32, 32 },
											{ 32, 32 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 16 },
											{ 32, 16 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 16 },
											{ 0, 16 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 16 },
											{ 0, 16 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 16 },
											{ 32, 16 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 16 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;

								case "b_bh100":
									{
										var side = bsp.GetTexture("_med100").Mips[0];
										var top = bsp.GetTexture("med100").Mips[0];

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 0 },
											{ 32 + 32, 0 },
											{ 32 + 32, 32 },
											{ 32, 32 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 32 },
											{ 32, 32 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 32 },
											{ 0, 32 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 32 },
											{ 0, 32 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 32 },
											{ 32, 32 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 32 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;

								case "b_rock1":
									{
										var side = bsp.GetTexture("rock1sid").Mips[0];
										var top = bsp.GetTexture("rockettop").Mips[0];

										side = side.Extend(1, 2);
										top = top.Extend(1, 2);

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 32 },
											{ 32, 0 },
											{ 32 + 16, 0 },
											{ 32 + 16, 32 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 16 },
											{ 32, 0 },
											{ 0, 0 },
											{ 0, 16 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 16 },
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 16 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 32 },
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 32 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 32 },
											{ 32, 0 },
											{ 0, 0 },
											{ 0, 32 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 16, 32 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;

								case "b_explob":
									{
										var side = bsp.GetTexture("_box_side").Mips[0];
										var top = bsp.GetTexture("_box_top").Mips[0];

										var td = GetTextureDimensions(side, top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 32, 0 },
											{ 32 + 32, 0 },
											{ 32 + 32, 32 },
											{ 32, 32 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 64 },
											{ 32, 64 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 64 },
											{ 0, 64 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 64 },
											{ 0, 64 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 64 },
											{ 32, 64 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 64 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name), 32);
									}
									break;

								case "b_exbox2":
									{
										var top = bsp.GetTexture("_box_top").Mips[0];

										var td = GetTextureDimensions(top);

										var topSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 32 },
											{ 0, 32 },
											}
										};

										var pxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 32 },
											{ 32, 32 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										var nxSide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 1.0f },
											{ -0.5f, 0.5f, 1.0f },
											{ -0.5f, 0.5f, 0.0f },
											{ -0.5f, -0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 32 },
											{ 0, 32 },
											}
										};

										var pySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 1.0f },
											{ 0.5f, 0.5f, 0.0f },
											{ -0.5f, 0.5f, 0.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 0 },
											{ 32, 0 },
											{ 32, 32 },
											{ 0, 32 },
											}
										};

										var nySide = new ModelFace
										{
											Vertices = new float[4, 3]
											{
											{ -0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 0.0f },
											{ 0.5f, -0.5f, 1.0f },
											{ -0.5f, -0.5f, 1.0f },
											},
											TexCoords = new float[4, 2]
											{
											{ 0, 32 },
											{ 32, 32 },
											{ 32, 0 },
											{ 0, 0 },
											}
										};

										CompileCubeModel(new Q1BSPTexture[] { top }, new uint[] { 32, 32, 32 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(pakfile.Name));
									}
									break;
							}
						}
					}
				}
			}

			ClearStatus();
			Status("Done!", 0);
		}

		void CompileCubeModel(Q1BSPTexture[] textures, uint[] boxSize, uint[] textureSize, ModelFace[] faces, string name, int yofs = 16)
		{
			CreateDirectory("models/q1");

			using (var bw = new BinaryWriter(OpenFileStream("models/q1/" + name + ".q1m")))
			{
				var _verts = new List<Tuple<sbyte, sbyte, sbyte, byte, byte>>();

				foreach (var face in faces)
				{
					for (var i = 0; i < 4; ++i)
					{
						var vert = Tuple.Create((sbyte)(boxSize[0] * face.Vertices[i, 0]), (sbyte)(boxSize[1] * face.Vertices[i, 1]), (sbyte)(boxSize[2] * face.Vertices[i, 2]), (byte)face.TexCoords[i, 0], (byte)face.TexCoords[i, 1]);
						int vi = _verts.IndexOf(vert);

						if (vi == -1)
						{
							face.VertIndexes[i] = (byte)_verts.Count;
							_verts.Add(vert);
						}
						else
							face.VertIndexes[i] = (byte)vi;
					}
				}

				bw.Write(Encoding.ASCII.GetBytes("Q1BM"));

				// Write textures first
				bw.Write((byte)textureSize[0]);
				bw.Write((byte)textureSize[1]);

				int num_frames = 0;

				foreach (var texture in textures)
					num_frames = Math.Max(num_frames, texture.Frames.Count);

				bw.Write((byte)num_frames);

				// Compile texture data
				byte[] textureBuffer = new byte[textureSize[0] * textureSize[1]];

				for (var i = 0; i < num_frames; ++i)
				{
					uint x = 0;

					foreach (var texture in textures)
					{
						var frame = texture.Frames[i % texture.Frames.Count];

						ByteBuffer.BoxCopy(frame.Data, 0, 0, (int)texture.Width, (int)texture.Height, textureBuffer, (int)x, 0, (int)textureSize[0], (int)textureSize[1], (int)texture.Width, (int)texture.Height);
						x += texture.Width;
					}

					bw.Write(textureBuffer);
				}

				// Write geometry data
				// Vertices
				bw.Write((byte)_verts.Count);

				foreach (var vert in _verts)
				{
					bw.Write(vert.Item1);
					bw.Write(vert.Item2);
					bw.Write((sbyte)(vert.Item3 - yofs));

					bw.Write(vert.Item4);
					bw.Write(vert.Item5);
				}

				// Faces
				bw.Write((byte)faces.Length);

				foreach (var face in faces)
				{
					bw.Write(face.VertIndexes[2]);
					bw.Write(face.VertIndexes[1]);
					bw.Write(face.VertIndexes[0]);
					bw.Write(face.VertIndexes[3]);
					bw.Write(face.VertIndexes[2]);
					bw.Write(face.VertIndexes[0]);
				}
			}
		}

		public override void Run()
		{
			Q1CopyStuff(Globals.Quake1Path + "\\pak0.pak");
			Q1CopyStuff(Globals.Quake1Path + "\\pak1.pak");
		}
	}
}
