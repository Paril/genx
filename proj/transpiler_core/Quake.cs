using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Linq;
using System.Threading;
using DotNet.Globbing;
using ImageType = SixLabors.ImageSharp.Image<SixLabors.ImageSharp.PixelFormats.Bgra32>;
using SixLabors.ImageSharp.Advanced;

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
	
	class Q1BSP : IDisposable
	{
		public List<uint> TextureOffsets = new List<uint>();
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

			for (uint i = 0; i < numtex; ++i)
			{
				var offset = Reader.ReadInt32();

				if (offset == -1)
					continue;

				TextureOffsets.Add((uint)(miptex.Offset + offset));
			}
		}

		public void Dispose()
		{
			Reader.Dispose();
		}

		public Q1BSPTexture GetTexture(string side_texture)
		{
			Q1BSPTexture texture = new Q1BSPTexture();

			foreach (var offset in TextureOffsets)
			{
				Reader.BaseStream.Position = offset;

				var name = Encoding.ASCII.GetString(Reader.ReadBytes(16));

				if (name.IndexOf('\0') != -1)
					name = name.Substring(0, name.IndexOf('\0'));

				if (!name.EndsWith(side_texture))
					continue;

				var frame = new Q1BSPTextureAnim();

				if (name[0] == '+')
					frame.Frame = name[1];

				texture.Width = Reader.ReadUInt32();
				texture.Height = Reader.ReadUInt32();
				var dataOffset = Reader.ReadInt32();

				Reader.BaseStream.Position = offset + dataOffset;

				frame.Data = Reader.ReadBytes((int)(texture.Width * texture.Height));
				texture.Frames.Add(frame);
			}

			if (texture.Frames.Count == 0)
				throw new Exception("no frames!");

			texture.Frames.Sort((a, b) => a.Frame - b.Frame);

			return texture;
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

			"maps/*.bsp",

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

		void CompileCubeModel(Q1BSPTexture[] textures, uint[] boxSize, uint[] textureSize, ModelFace[] faces, string name, int yofs = 16)
		{
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

		static Mutex _highestAnimationsLock = new Mutex();
		static Mutex _textureHashesLock = new Mutex();
		static Dictionary<string, byte> _animationCounts = new Dictionary<string, byte>();
		static Dictionary<string, IList<byte[]>> _textureHashes = new Dictionary<string, IList<byte[]>>();

		void TaskBspTexture((string pakname, IArchiveFile pakfile, uint textureOffset) info)
		{
			using (var br = new BinaryReader(OpenArchive(info.pakname)))
			{
				br.BaseStream.Position = info.pakfile.Offset + info.textureOffset;

				var name = Encoding.ASCII.GetString(br.ReadBytes(16));

				if (name.IndexOf('\0') != -1)
					name = name.Substring(0, name.IndexOf('\0'));

				var width = br.ReadUInt32();
				var height = br.ReadUInt32();
				var dataOffset = br.ReadInt32();
				var anim = '0';
				var isAnimated = name[0] == '+';
				var isSky = name[0] == '*';

				if (isAnimated)
				{
					anim = name[1];
					name = name.Substring(2);
				}
				else if (isSky)
					name = name.Substring(1);

				var fullName = Path.GetFileNameWithoutExtension(info.pakfile.Name) + "/" + name;

				br.BaseStream.Position = info.pakfile.Offset + info.textureOffset + dataOffset;

				var data = br.ReadBytes((int)(width * height));

				using (var sha256 = SHA256.Create())
				{
					var hash = sha256.ComputeHash(data);

					_textureHashesLock.WaitOne();
					if (_textureHashes.TryGetValue(name, out var otherHashes))
					{
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
						{
							_textureHashesLock.ReleaseMutex();
							return;
						}

						otherHashes.Add(hash);
					}
					else
						_textureHashes.Add(name, new List<byte[]>(new byte[][] { hash }));

					_textureHashesLock.ReleaseMutex();
				}

				if (isAnimated)
				{
					_highestAnimationsLock.WaitOne();
					if (!_animationCounts.TryGetValue(fullName, out var current))
						_animationCounts[fullName] = 1;
					else
						_animationCounts[fullName]++;
					_highestAnimationsLock.ReleaseMutex();
				}

				// Write WAL
				string walname = "q1/" + fullName, next_name = "";

				if (isAnimated)
				{
					walname = "q1/" + fullName + "_f" + anim;
					next_name = "q1/" + fullName + "_f" + (char)(anim + 1);
				}

				string outwal = "textures/" + walname + ".wal";
				string outpng = "textures/" + walname + "." + ImageExtensions.WrittenImageFormat;

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
				else if (isSky)
				{
					surf = Quake2.SURF_WARP | Quake2.SURF_TRANS66;
					contents = Quake2.CONTENTS_WATER;
				}
				else if (walname.StartsWith("q1/sky"))
					surf = Quake2.SURF_SKY | Quake2.SURF_NODRAW;

				var bmp = new ImageType((int)width, (int)height);
				var span = bmp.GetPixelSpan();

				for (var y = 0; y < height; ++y)
					for (var x = 0; x < width; ++x)
					{
						var c = (y * (int)width) + x;
						span[c] = Globals.GetMappedColor(PaletteID.Q1, data[c]);
					}

				SaveWalTga(bmp, outpng, outwal, walname, next_name, surf, contents, data, PaletteID.Q1, (int)width, (int)height);
			}
		}

		void TaskBsp((string pakname, IArchiveFile pakfile) info)
		{
			using (var pak = new PAK(OpenArchive(info.pakname)))
			using (var bsp = new Q1BSP(new MemoryStream(pak.GetBytes(info.pakfile))))
			{
				foreach (var textureOffset in bsp.TextureOffsets)
					CreateTask(TaskBspTexture, (info.pakname, info.pakfile, textureOffset));
			}
		}

		void TaskCopy((string pakname, IArchiveFile pakfile, string output) info)
		{
			using (var pak = new PAK(OpenArchive(info.pakname)))
				pak.WriteFileTo(info.pakfile, info.output);
		}

		void TaskImageCopy((string pakname, IArchiveFile pakfile, PaletteID palette, string output) info)
		{
			using (var pak = new PAK(OpenArchive(info.pakname)))
				pak.WriteImageTo(info.pakfile, PaletteID.Q1, info.output);
		}

		void TaskConchars((string pakname, string wadfile, uint offset, uint length) info)
		{
			using (var sr = new BinaryReader(OpenArchive(info.pakname)))
			{
				sr.BaseStream.Position = info.offset;
				
				ArchiveBase.WriteImageTo(sr.ReadBytes((int)info.length), PaletteID.Q1, "pics/q1/" + info.wadfile.ToLower() + "." + ImageExtensions.WrittenImageFormat, 128, 128, color => (byte)(color == 0 ? 255 : color));
			}
		}

		void TaskWadPicMip((string pakname, string wadfile, uint offset, uint length) info)
		{
			using (var sr = new BinaryReader(OpenArchive(info.pakname)))
			{
				sr.BaseStream.Position = info.offset;

				using (var file = OpenFileStream("pics/q1/" + info.wadfile.ToLower() + "." + ImageExtensions.WrittenImageFormat))
					ArchiveBase.WriteImageTo(sr, PaletteID.Q1, file);
			}
		}

		void TaskWad((string pakname, IArchiveFile pakfile) info)
		{
			using (var pak = new PAK(OpenArchive(info.pakname)))
			using (var wad = new WAD(new MemoryStream(pak.GetBytes(info.pakfile))))
				foreach (WADFile wadfile in wad.Files)
				{
					if (wadfile.Type != WADFileType.Pic && wadfile.Type != WADFileType.Mip)
						continue;

					var outInfo = (info.pakname, wadfile.Name, info.pakfile.Offset + wadfile.Offset, wadfile.Length);

					if (wadfile.Name.ToLower() == "conchars")
						CreateTask(TaskConchars, outInfo);
					else
						CreateTask(TaskWadPicMip, outInfo);
				}
		}

		void TaskBspModel((string pakname, IArchiveFile pakfile) info)
		{
			using (var pak = new PAK(OpenArchive(info.pakname)))
			using (var bsp = new Q1BSP(new MemoryStream(pak.GetBytes(info.pakfile))))
				switch (Path.GetFileNameWithoutExtension(info.pakfile.Name))
				{
					case "b_shell0":
						{
							var side = bsp.GetTexture("shot0sid");
							var top = bsp.GetTexture("shot0top");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 24, 24, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;

					case "b_shell1":
						{
							var side = bsp.GetTexture("shot1sid");
							var top = bsp.GetTexture("shot1top");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;

					case "b_nail0":
						{
							var side = bsp.GetTexture("nail0sid");
							var top = bsp.GetTexture("nail0top");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 24, 24, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;


					case "b_nail1":
						{
							var side = bsp.GetTexture("nail1sid");
							var top = bsp.GetTexture("nail1top");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;

					case "b_batt0":
						{
							var side = bsp.GetTexture("batt1sid");
							var top = bsp.GetTexture("batt0top");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 24, 24, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;

					case "b_batt1":
						{
							var side = bsp.GetTexture("batt0sid");
							var top = bsp.GetTexture("batt1top");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 24 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;

					case "b_rock0":
						{
							var side = bsp.GetTexture("rock0sid");
							var top = bsp.GetTexture("rockettop");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 16, 16, 32 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;

					case "b_bh10":
						{
							var side = bsp.GetTexture("med3_1");
							var top = bsp.GetTexture("med3_0");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 16 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;

					case "b_bh25":
						{
							var side = bsp.GetTexture("_med25s");
							var top = bsp.GetTexture("_med25");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 16 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;

					case "b_bh100":
						{
							var side = bsp.GetTexture("_med100");
							var top = bsp.GetTexture("med100");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 32 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;

					case "b_rock1":
						{
							var side = bsp.GetTexture("rock1sid");
							var top = bsp.GetTexture("rockettop");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 16, 32 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;

					case "b_explob":
						{
							var side = bsp.GetTexture("_box_side");
							var top = bsp.GetTexture("_box_top");

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

							CompileCubeModel(new Q1BSPTexture[] { side, top }, new uint[] { 32, 32, 64 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name), 32);
						}
						break;

					case "b_exbox2":
						{
							var top = bsp.GetTexture("_box_top");

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

							CompileCubeModel(new Q1BSPTexture[] { top }, new uint[] { 32, 32, 32 }, td, new ModelFace[] { topSide, pxSide, nxSide, pySide, nySide }, Path.GetFileNameWithoutExtension(info.pakfile.Name));
						}
						break;
				}
		}

		void RunThroughPak(string pakname)
		{
			using (var pak = new PAK(File.Open(pakname, FileMode.Open, FileAccess.Read, FileShare.ReadWrite)))
			{
				foreach (var pakfile in pak.Files)
				{
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

					if (pakfile.Name.EndsWith(".bsp") && !Path.GetFileName(pakfile.Name).StartsWith("b_"))
						CreateTask(TaskBsp, (pakname, pakfile));
					else if (pakfile.Name.EndsWith(".mdl"))
						CreateTask(TaskCopy, (pakname, pakfile, "models/q1/" + pakfile.Name.Substring("progs/".Length)));
					else if (pakfile.Name.EndsWith(".wav"))
						CreateTask(TaskCopy, (pakname, pakfile, "sound/q1/" + pakfile.Name.Substring("sound/".Length)));
					else if (pakfile.Name.EndsWith(".spr"))
						CreateTask(TaskCopy, (pakname, pakfile, "sprites/q1/" + pakfile.Name.Substring("progs/".Length)));
					else if (pakfile.Name.EndsWith(".lmp"))
					{
						if (pakfile.Name.EndsWith("palette.lmp"))
							CreateTask(TaskCopy, (pakname, pakfile, "pics/q1/palette.dat"));
						else if (!pakfile.Name.EndsWith("colormap.lmp") && !pakfile.Name.EndsWith("pop.lmp"))
							CreateTask(TaskImageCopy, (pakname, pakfile, PaletteID.Q1, "pics/q1/" + pakfile.Name.Substring("gfx/".Length, pakfile.Name.Length - 4 - "gfx/".Length) + "." + ImageExtensions.WrittenImageFormat));
					}
					else if (pakfile.Name.EndsWith(".wad"))
						CreateTask(TaskWad, (pakname, pakfile));
					else if (Path.GetFileNameWithoutExtension(pakfile.Name).StartsWith("b_") && pakfile.Name.EndsWith(".bsp"))
						CreateTask(TaskBspModel, (pakname, pakfile));
				}
			}
		}

		public override void Run()
		{
			CreateTask(RunThroughPak, Globals.GetGameInfo(PaletteID.Q1).Path + "\\pak0.pak");
			CreateTask(RunThroughPak, Globals.GetGameInfo(PaletteID.Q1).Path + "\\pak1.pak");
		}
		
		void TaskCleanFrames((string key, string[] files) info)
		{
			var underscorePosition = info.key.Length + 3;

			for (var i = 0; i < info.files.Length; i++)
			{
				var file = info.files[i].Substring(0, info.files[i].Length - 4);

				if (Globals.SaveWals)
				{
					using (var fs = OpenFileStream(file + ".wal"))
					{
						fs.Position = underscorePosition;

						if (i == 0)
							fs.WriteByte(0);
						else
						{
							fs.Position++;
							fs.WriteByte((byte)'f');
							fs.WriteByte((byte)('0' + i));
							fs.WriteByte(0);
						}

						if (i > 0)
						{
							fs.Position = 56 + underscorePosition;

							var nextFrame = (i + 1) % info.files.Length;

							if (nextFrame == 0)
								fs.WriteByte(0);
							else
							{
								fs.Position++;
								fs.WriteByte((byte)'f');
								fs.WriteByte((byte)('0' + nextFrame));
								fs.WriteByte(0);
							}
						}
					}

					if (i == 0)
						MoveFile(file + ".wal", file.Substring(0, file.LastIndexOf('_')) + ".wal");
					else
						MoveFile(file + ".wal", file.Substring(0, file.LastIndexOf('_')) + "_f" + (char)('0' + i) + ".wal");
				}

				if (i == 0)
					MoveFile(file + "." + ImageExtensions.WrittenImageFormat, file.Substring(0, file.LastIndexOf('_')) + "." + ImageExtensions.WrittenImageFormat);
				else
					MoveFile(file + "." + ImageExtensions.WrittenImageFormat, file.Substring(0, file.LastIndexOf('_')) + "_f" + (char)('0' + i) + "." + ImageExtensions.WrittenImageFormat);
			}
		}

		void TaskCleanAnimation(KeyValuePair<string, byte> animation)
		{
			string[] files;
			var match = animation.Key + "_f*." + ImageExtensions.WrittenImageFormat;

			if (Globals.DoMemoryFiles)
			{
				var glob = Glob.Parse("textures/q1/" + match);
				files = GetMemoryFiles().Where(glob.IsMatch).ToArray();
			}
			else
				files = Directory.GetFiles("textures/q1/", match);

			Array.Sort(files);
			CreateTask(TaskCleanFrames, (animation.Key, files));
		}

		public override void Cleanup()
		{
			foreach (var animation in _animationCounts)
				CreateTask(TaskCleanAnimation, animation);
		}
	}
}
