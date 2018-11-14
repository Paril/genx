using SixLabors.ImageSharp;
using SixLabors.ImageSharp.PixelFormats;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace asset_transpiler
{
	interface IArchiveFile
	{
		string Name { get; }
		uint Offset { get; }
		uint Length { get; }
	}

	class ArchiveBase : IDisposable
	{
		public IList<IArchiveFile> Files { get; private set; }
		public BinaryReader Reader { get; private set; }
		GameParser Parser;

		public ArchiveBase(GameParser parser, Stream stream)
		{
			Parser = parser;
			Reader = new BinaryReader(stream);
			Files = new List<IArchiveFile>();
		}

		public IArchiveFile Get(string name)
		{
			foreach (var file in Files)
				if (file.Name == name)
					return file;

			return null;
		}

		public T Get<T>(string name) where T : IArchiveFile
		{
			return (T)Get(name);
		}

		public byte[] GetBytes(IArchiveFile file)
		{
			Reader.BaseStream.Position = file.Offset;
			return Reader.ReadBytes((int)file.Length);
		}
		
		public void WriteFileTo(IArchiveFile file, string output, Action<BinaryWriter> writeHeader = null, Action<BinaryWriter, byte[]> writeData = null)
		{
			Parser.CreateDirectory(Path.GetDirectoryName(output));

			using (var bw = new BinaryWriter(Parser.OpenFileStream(output)))
			{
				writeHeader?.Invoke(bw);

				if (writeData != null)
					writeData(bw, GetBytes(file));
				else
					bw.Write(GetBytes(file));
			}
		}

		public void WriteImageTo(byte[] colors, PaletteID pal, string output, int width, int height, Func<byte, byte> changeColor = null)
		{
			if (changeColor == null)
				changeColor = b => b;

			Parser.CreateDirectory(Path.GetDirectoryName(output));

			using (var img = new Image<Rgba32>(width, height))
			{
				for (var y = 0; y < height; y++)
					for (var x = 0; x < width; x++)
						img[x, y] = Globals.GetMappedColor(pal, changeColor(colors[y * width + x]));

				using (var strm = Parser.OpenFileStream(output))
					img.SaveAsTga(strm);
			}
		}

		public void WriteImageTo(IArchiveFile file, PaletteID pal, string output)
		{
			var oldpos = Reader.BaseStream.Position;
			Reader.BaseStream.Position = file.Offset;

			int width = Reader.ReadInt32();
			int height = Reader.ReadInt32();
			byte[] bytes = Reader.ReadBytes(width * height);

			WriteImageTo(bytes, pal, output, width, height);
			Reader.BaseStream.Position = oldpos;
		}

		void IDisposable.Dispose()
		{
			Reader.Dispose();
		}
	}

	class GRPFile : IArchiveFile
	{
		public string Name { get; set; }
		public uint Offset { get; set; }
		public uint Length { get; set; }
	}

	class GRP : ArchiveBase
	{
		public GRP(GameParser parser, Stream stream) :
			base(parser, stream)
		{
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
	}

	enum WADFileType : sbyte
	{
		Pal = 0x40,
		Pic = 0x42,
		Mip = 0x44,
		Con = 0x45
	}

	class WADFile : IArchiveFile
	{
		public string Name { get; set; }
		public uint Offset { get; set; }
		public uint Length { get; set; }

		public WADFileType Type;
	}

	class WAD : ArchiveBase
	{
		public WAD(GameParser parser, Stream stream) :
			base(parser, stream)
		{
			var magic = Encoding.ASCII.GetString(Reader.ReadBytes(4));

			switch (magic)
			{
				case "WAD2":
				case "IWAD":
				case "PWAD":
					break;
				default:
					throw new Exception();
			}

			var num_entries = Reader.ReadUInt32();
			var dir_offset = Reader.ReadUInt32();

			Reader.BaseStream.Position = dir_offset;

			for (uint i = 0; i < num_entries; ++i)
			{
				WADFile file = new WADFile();

				switch (magic)
				{
					case "WAD2":
						file.Offset = Reader.ReadUInt32();
						file.Length = Reader.ReadUInt32();
						Reader.ReadUInt32();
						file.Type = (WADFileType)Reader.ReadSByte();

						if (Reader.ReadSByte() != 0)
							throw new Exception();

						Reader.ReadUInt16();
						file.Name = Encoding.ASCII.GetString(Reader.ReadBytes(16));
						break;
					case "IWAD":
					case "PWAD":
						file.Offset = Reader.ReadUInt32();
						file.Length = Reader.ReadUInt32();

						file.Name = Encoding.ASCII.GetString(Reader.ReadBytes(8));
						break;
				}

				if (file.Name.IndexOf('\0') != -1)
					file.Name = file.Name.Substring(0, file.Name.IndexOf('\0'));

				Files.Add(file);
			}
		}
	}

	class PAKFile : IArchiveFile
	{
		public string Name { get; set; }
		public uint Offset { get; set; }
		public uint Length { get; set; }
	}

	class PAK : ArchiveBase
	{
		public PAK(GameParser parser, Stream stream) :
			base(parser, stream)
		{
			if (Encoding.ASCII.GetString(Reader.ReadBytes(4)) != "PACK")
				throw new Exception();

			uint offset = Reader.ReadUInt32();
			uint length = Reader.ReadUInt32();

			uint num_files = length / (56 + 4 + 4);

			Reader.BaseStream.Position = offset;

			for (uint i = 0; i < num_files; ++i)
			{
				PAKFile file = new PAKFile();
				file.Name = Encoding.ASCII.GetString(Reader.ReadBytes(56));

				if (file.Name.IndexOf('\0') != -1)
					file.Name = file.Name.Substring(0, file.Name.IndexOf('\0'));

				file.Offset = Reader.ReadUInt32();
				file.Length = Reader.ReadUInt32();
				Files.Add(file);
			}
		}
	}
}
