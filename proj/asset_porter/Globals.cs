using SixLabors.ImageSharp;
using SixLabors.ImageSharp.PixelFormats;
using SixLabors.ImageSharp.Processing;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Text;

namespace asset_transpiler
{
	public static class Rgba32Extensions
	{
		public static Rgba32[] MakePalette(string hex)
		{
			var colors = new Rgba32[hex.Length / 6];

			for (int x = 0, i = 0; x < colors.Length; i += 6, ++x)
			{
				colors[x].R = byte.Parse(hex.Substring(i, 2), NumberStyles.HexNumber);
				colors[x].G = byte.Parse(hex.Substring(i + 2, 2), NumberStyles.HexNumber);
				colors[x].B = byte.Parse(hex.Substring(i + 4, 2), NumberStyles.HexNumber);
				colors[x].A = (byte)(x == 255 ? 0 : 255);
			}

			return colors;
		}

		public static int Distance(this Rgba32 a, Rgba32 b)
		{
			if (a.A == 0 && b.A == 0)
				return 0;

			var dr = Math.Abs(a.R - b.R);
			var dg = Math.Abs(a.G - b.G);
			var db = Math.Abs(a.B - b.B);

			return dr * dr + dg * dg + db * db;
		}

		public static int ClosestColor(this Rgba32 to, Rgba32[] palette)
		{
			var currentClosest = -1;
			var currentDist = int.MaxValue;

			for (var i = 0; i < palette.Length; ++i)
			{
				var pc = palette[i];
				var dist = pc.Distance(to);

				if (dist == 0)
					return i;
				else if (dist >= currentDist)
					continue;

				currentDist = dist;
				currentClosest = i;
			}

			return currentClosest;
		}
	}

	public enum PaletteID
	{
		Q2,
		Q1,
		Doom,
		Duke,

		Total
	}

	public class Globals
	{
		public static ZipArchive _writeToZip = null;
		public static HashSet<string> _zipFiles = null;

		public static void MakeZip(string zip_location)
		{
			_writeToZip = new ZipArchive(File.Create(zip_location), ZipArchiveMode.Create, false);
			_zipFiles = new HashSet<string>();
		}

		public static void WriteFile(string local_file, byte[] data)
		{
			if (_writeToZip != null)
			{
				_zipFiles.Add(local_file);

				using (var writer = new BinaryWriter(_writeToZip.CreateEntry(local_file, CompressionLevel.Optimal).Open()))
					writer.Write(data);
			}
			else
				File.WriteAllBytes(local_file, data);
		}

		public static void CreateDirectory(string dir)
		{
			if (_writeToZip == null)
				Directory.CreateDirectory(dir);
		}

		public static bool FileExists(string local_file)
		{
			if (_writeToZip != null)
				return _zipFiles.Contains(local_file);

			return File.Exists(local_file);
		}

		public static Stream OpenFileStream(string local_file)
		{
			if (_writeToZip != null)
			{
				_zipFiles.Add(local_file);
				return _writeToZip.CreateEntry(local_file, CompressionLevel.Optimal).Open();
			}

			return File.Create(local_file);
		}

		public static Rgba32[][] _palettes = new Rgba32[(int)PaletteID.Total][]
		{
			Rgba32Extensions.MakePalette("0000000f0f0f1f1f1f2f2f2f3f3f3f4b4b4b5b5b5b6b6b6b7b7b7b8b8b8b9b9b9babababbbbbbbcbcbcbdbdbdbebebeb634b235b431f533f1f4f3b1b47371b3f2f173b2b173327132f23132b1f13271b0f23170f1b130b170f0b130f070f0b075f5f6f5b5b675b535f574f5b534b534f474b473f433f3b3b3b3737332f2f2f2b2b2727272323231b1b1b1717171313138f77537b6343735b3b674f2fcf974ba77b3b8b672f6f5327eb9f27cb8b23af771f93631b774f175b3b0f3f270b231707a73b2b9f2f23972b1b8b27137f1f0f73170b6717075713004b0f00430f003b0f00330b002b0b00230b001b07001307007b5f4b7357436b533f674f3b5f4737574333533f2f4b372b4333273f2f2337271b2f2317271b131f170f170f0b0f0b076f3b175f3717532f17432b17372313271b0f1b130b0f0b07b35b4fbf7b6fcb9b93d7bbb7cbd7dfb3c7d39fb7c387a7b77397a75b879b47778b2f677f17536f134b670f435b0b3f5307374b072f3f072733001f2b00171f000f1300070b0000008b5757834f4f7b47477343436b3b3b6333335b2f2f572b2b4b23233f1f1f331b1b2b13131f0f0f130b0b0b0707000000979f7b8f9773878b6b7f8363777b5f7373576b6b4f6363475b5b434f4f3b43433337372b2f2f2323231b1717130f0f0b9f4b3f9343378b3b2f7f3727772f236b2b1b632317571f134f1b0f43170b37130b2b0f071f0b071707000b0000000000777bcf6f73c3676bb76363a75b5b9b53578f4b4f7f4747733f3f673737572f2f4b27273f231f2f1b1723130f170b07079bab7b8f9f6f8797637b8b5773834b6777435f6f3b5767334b5b273f4f1b3743132f3b0b232f071b23001317000b0f0000ff0023e70f3fd31b53bb275fa72f5f8f335f7b33ffffffffffd3ffffa7ffff7fffff53ffff27ffeb1fffd717ffbf0fffab07ff9300ef7f00e36b00d35700c74700b73b00ab2b009b1f008f17007f0f007307005f00004700002f00001b0000ef00003737ffff00000000ff2b2b231b1b1713130feb977fc373539f57337b3f1bebd3c7c7ab9ba78b77876b579f5b53"),
			Rgba32Extensions.MakePalette("0000000f0f0f1f1f1f2f2f2f3f3f3f4b4b4b5b5b5b6b6b6b7b7b7b8b8b8b9b9b9babababbbbbbbcbcbcbdbdbdbebebeb0f0b07170f0b1f170b271b0f2f2313372b173f2f174b371b533b1b5b431f634b1f6b531f73571f7b5f238367238f6f230b0b0f13131b1b1b272727332f2f3f37374b3f3f574747674f4f735b5b7f63638b6b6b977373a37b7baf8383bb8b8bcb0000000707000b0b001313001b1b002323002b2b072f2f073737073f3f074747074b4b0b53530b5b5b0b63630b6b6b0f0700000f00001700001f00002700002f00003700003f00004700004f00005700005f00006700006f00007700007f00001313001b1b002323002f2b00372f004337004b3b075743075f47076b4b0b77530f8357138b5b13975f1ba3631faf67232313072f170b3b1f0f4b2313572b17632f1f7337237f3b2b8f43339f4f33af632fbf772fcf8f2bdfab27efcb1ffff31b0b07001b13002b230f372b1347331b533723633f2b6f47337f533f8b5f479b6b53a77b5fb7876bc3937bd3a38be3b397ab8ba39f7f979373878b677b7f5b6f7753636b4b575f3f4b5737434b2f3743272f371f232b171b231313170b0b0f0707bb739faf6b8fa35f839757778b4f6b7f4b5f7343536b3b4b5f333f532b3747232b3b1f232f171b231313170b0b0f0707dbc3bbcbb3a7bfa39baf978ba3877b977b6f876f5f7b63536b57475f4b3b533f33433327372b1f271f171b130f0f0b076f837b677b6f5f7367576b5f4f6357475b4f3f5347374b3f2f43372b3b2f2333271f2b1f1723170f1b130b130b070b07fff31befdf17dbcb13cbb70fbba70fab970b9b83078b73077b63076b53005b47004b37003b2b002b1f001b0f000b07000000ff0b0bef1313df1b1bcf2323bf2b2baf2f2f9f2f2f8f2f2f7f2f2f6f2f2f5f2b2b4f23233f1b1b2f13131f0b0b0f2b00003b00004b07005f07006f0f007f1707931f07a3270bb7330fc34b1bcf632bdb7f3be3974fe7ab5fefbf77f7d38ba77b3bb79b37c7c337e7e3577fbfffabe7ffd7ffff6700008b0000b30000d70000ff0000fff393fff7c7ffffff9f5b53"),
			Rgba32Extensions.MakePalette("0000001F170B170F074B4B4BFFFFFF1B1B1B1313130B0B0B0707072F371F232B0F171F070F17004F3B2B4733233F2B1BFFB7B7F7ABABF3A3A3EB9797E78F8FDF8787DB7B7BD37373CB6B6BC76363BF5B5BBB5757B34F4FAF4747A73F3FA33B3B9B3333972F2F8F2B2B8B2323831F1F7F1B1B7717177313136B0F0F670B0B5F07075B07075307074F0000470000430000FFEBDFFFE3D3FFDBC7FFD3BBFFCFB3FFC7A7FFBF9BFFBB93FFB383F7AB7BEFA373E79B6BDF9363D78B5BCF8353CB7F4FBF7B4BB37347AB6F43A36B3F9B633B8F5F378757337F532F774F2B6B47275F4323533F1F4B371B3F2F17332B132B230FEFEFEFE7E7E7DFDFDFDBDBDBD3D3D3CBCBCBC7C7C7BFBFBFB7B7B7B3B3B3ABABABA7A7A79F9F9F9797979393938B8B8B8383837F7F7F7777776F6F6F6B6B6B6363635B5B5B5757574F4F4F4747474343433B3B3B3737372F2F2F27272723232377FF6F6FEF6767DF5F5FCF575BBF4F53AF474B9F3F4393373F832F37732B2F632327531B1F431717330F13230B0B1707BFA78FB79F87AF977FA78F779F876F9B7F6B937B638B735B836B577B634F775F4B6F574367533F5F4B37574333533F2F9F83638F7753836B4B775F3F6753335B472B4F3B2343331B7B7F636F7357676B4F5B634753573B474F333F472B373F27FFFF73EBDB57D7BB43C39B2FAF7B1F9B5B13874307732B00FFFFFFFFDBDBFFBBBBFF9B9BFF7B7BFF5F5FFF3F3FFF1F1FFF0000EF0000E30000D70000CB0000BF0000B30000A700009B00008B00007F00007300006700005B00004F0000430000E7E7FFC7C7FFABABFF8F8FFF7373FF5353FF3737FF1B1BFF0000FF0000E30000CB0000B300009B00008300006B000053FFFFFFFFEBDBFFD7BBFFC79BFFB37BFFA35BFF8F3BFF7F1BF37317EB6F0FDF670FD75F0BCB5707C34F00B74700AF4300FFFFFFFFFFD7FFFFB3FFFF8FFFFF6BFFFF47FFFF23FFFF00A73F009F3700932F008723004F3B27432F1B3723132F1B0B00005300004700003B00002F00002300001700000B000000FF9F43FFE74BFF7BFFFF00FFCF00CF9F009B6F006BA76B6B"),
			Rgba32Extensions.MakePalette("000000040404100C101818182420202C282C383434403C3C4C484858505060585C6C6064746C70807478887C8094888C988C90A09498A89CA0ACA4A8B0ACACB8B0B4C0B8B8C8BCC0CCC4C8D4CCD0D8D4D8E0D8DCE4E0E4ECE8ECF4F0F4FCFCFC0000000404000C0404180C0420100828140C301C103C20144424184C2C1854301C603420683C2470402878442C844C308C5438905C3C986444A06C48A87450AC7C58B08460B89068BC9870C4A078CCAC80D4B08CD8B894DCC49CE4CCA8ECD8B05060945468985C70A06078A86880AC7088B47890BC7C98C484A4CC8CACD094B0D89CB8DCA4C4E4ACCCECB4D4F4BCDCFC0000000404040408100C0C1810142014182C182034202840242C482834502C385C34406438446C3C4C7840508048588C0000000404040C0C0818140C20201028281830301C3C38244440284C4C2C545434605C38686440706C447874488480508C8854949454989C589CA45CA0AC60A4B060A8B864ACC068ACC868ACD06CB0D86CB0DC70B0E470B0EC74B0F474B0FC780000000C0000180400280400340804440C04541004601404701808801C088C200C9C240CAC280CB82C10C83010D834146444186850047458048064048C6C08947808A08008AC8C08B09410BCA010C8AC10D4B414D8C014E4C814F0D814FCE018000000040400080804100C081410101C1414201C1828201C302420342C283C302C403430483C344C40385444405C4C4464544C6C5C50786458806C608C7468947C709C8478A88C7CAC9484B49C8CC0A494C8AC9CD4B0A4D8B8ACE0C4B0ECCCB8341C004428005830006C3800804000904800A45000B45404C85C04CC6818D47430D88444DC9058E4A070ECAC88F4C0A0D84018D84C1CDC5C28E06830E47838E88440EC9048F0A050F0AC5CF0B864F4C474F4D07CF8D88CF8E094F8E8A4FCF0AC08103408084014084C1C085830085C4010645414686818707C187888207498206CA41860B0284CBC2C38CC3018D83414444400A4A400FCFC00440044A400A4FC00FC580000AC0000FC000000440000A40000FC000000440000A40000FCFC00FC")
		};

		public static void WriteWav(string outwav, int sampleRate, int numSamples, byte[] samples, int channels = 1, int bps = 8)
		{
			using (var ms = new MemoryStream())
			using (var wr = new BinaryWriter(ms))
			{
				wr.Write(Encoding.ASCII.GetBytes("RIFF"));
				wr.Write(0);
				wr.Write(Encoding.ASCII.GetBytes("WAVE"));

				wr.Write(Encoding.ASCII.GetBytes("fmt "));
				wr.Write(16);
				wr.Write((short)0x0001);
				wr.Write((short)1);
				wr.Write(sampleRate);
				wr.Write(sampleRate * channels * (bps / 8));
				wr.Write((short)(channels * (bps / 8)));
				wr.Write((short)bps);

				wr.Write(Encoding.ASCII.GetBytes("data"));
				wr.Write(numSamples);
				wr.Write(samples);
				if ((numSamples & 1) == 1)
					wr.Write((byte)0);

				int len = (int)wr.BaseStream.Length;
				wr.BaseStream.Position = 4;
				wr.Write(len - 8);

				using (var os = OpenFileStream(outwav))
					os.Write(ms.ToArray());
			}
		}

		public static byte[][,] GenerateColorMappings()
		{
			var mappings = new byte[(int)PaletteID.Total][,];

			for (PaletteID palette = 0; palette < PaletteID.Total; ++palette)
			{
				var from = _palettes[(int)palette];
				mappings[(int)palette] = new byte[256, (int)PaletteID.Total];

				for (PaletteID palette2 = 0; palette2 < PaletteID.Total; ++palette2)
				{
					var to = _palettes[(int)palette2];

					for (int i = 0; i < 256; ++i)
						mappings[(int)palette][i, (int)palette2] = (byte)from[i].ClosestColor(to);
				}
			}

			return mappings;
		}

		public static byte[][,] _colorMappings = GenerateColorMappings();

		public static byte GetClosestMappedColor(PaletteID from, PaletteID to, int index)
		{
			return _colorMappings[(int)from][index, (int)to];
		}

		public static Rgba32 GetMappedColor(PaletteID from, int index)
		{
			return _palettes[(int)from][index];
		}

		static int _statusTop;
		static int _headerWidth, _statusWidth = 0;

		public static void OpenStatus(string whatwedoing)
		{
			Console.WriteLine(whatwedoing);
			_statusTop = Console.CursorTop;
			_headerWidth = whatwedoing.Length;
			_statusWidth = 0;
		}

		public static void Status(string status)
		{
			Console.CursorTop = _statusTop;
			Console.CursorLeft = 0;

			if (_statusWidth != 0)
			{
				Console.Write(new string('\0', _statusWidth));
				Console.CursorLeft = 0;
			}

			Console.WriteLine(status);

			_statusWidth = status.Length;
		}

		public static void ChangeStatusHeader(string newheader)
		{
			var oldTop = Console.CursorTop;
			Console.CursorTop = _statusTop - 1;
			Console.CursorLeft = 0;

			Console.Write(new string('\0', _headerWidth));
			Console.CursorLeft = 0;

			Console.WriteLine(newheader);
			Console.CursorTop = oldTop;

			_headerWidth = newheader.Length;
		}

		public static void CloseStatus()
		{
			ChangeStatusHeader("Done");
			Status("");
			Console.WriteLine();
		}
		
		static int NearestPow2(int x)
		{
			x--;
			x |= x >> 1;  // handle  2 bit numbers
			x |= x >> 2;  // handle  4 bit numbers
			x |= x >> 4;  // handle  8 bit numbers
			x |= x >> 8;  // handle 16 bit numbers
			x |= x >> 16; // handle 32 bit numbers
			x++;

			return x;
		}

		public static void SaveWal(string outwal, string name, string next_name, int surf, int contents, byte[] data, PaletteID palette, int width, int height)
		{
			/*if (image.Width != NearestPow2(image.Width) ||
				image.Height != NearestPow2(image.Height))
			{
				using (var newImage = image.Clone(c => c.Resize(NearestPow2(image.Width), NearestPow2(image.Height), KnownResamplers.NearestNeighbor)))
				{
					SaveWal(outwal, name, next_name, surf, contents, newImage);
					return;
				}
			}*/

			int sizedWidth = NearestPow2(width);
			int sizedHeight = NearestPow2(height);
			double xFrac = (double)width / sizedWidth;
			double yFrac = (double)height / sizedHeight;
			Func<int, int, byte> FetchPixel = (x, y) =>
			{
				var rx = (int)Math.Floor(xFrac * x);
				var ry = (int)Math.Floor(yFrac * y);
				return data[(ry * width) + rx];
			};

			int headerwidth = 32 + 4 + 4 + 16 + 32 + 4 + 4 + 4;

			BinaryWriter bw = new BinaryWriter(Globals.OpenFileStream(outwal));

			for (var k = 0; k < 32; ++k)
				bw.Write((byte)(k < name.Length ? name[k] : 0));

			bw.Write(sizedWidth);
			bw.Write(sizedHeight);
			
			var ofs = headerwidth;
			bw.Write(ofs);
			bw.Write(ofs += sizedWidth * sizedHeight);
			bw.Write(ofs += (int)Math.Ceiling(sizedWidth / 2.0) * (int)Math.Ceiling(sizedHeight / 2.0));
			bw.Write(ofs += (int)Math.Ceiling(sizedWidth / 4.0) * (int)Math.Ceiling(sizedHeight / 4.0));

			for (var k = 0; k < 32; ++k)
				bw.Write((byte)(k < next_name.Length ? next_name[k] : 0));

			bw.Write(surf);
			bw.Write(contents);
			bw.Write(0);
			
			byte[,] quantized = new byte[sizedWidth, sizedHeight];

			// quantize
			for (var y = 0; y < sizedHeight; y++)
				for (var x = 0; x < sizedWidth; x++)
				{
					var pixel = FetchPixel(x, y);

					if (_palettes[(int)palette][pixel].A == 255)
						quantized[x, y] = GetClosestMappedColor(palette, PaletteID.Q2, pixel);
					else
						quantized[x, y] = 255;
				}

			// mips
			for (var inc = 1; inc <= 8; inc *= 2)
				for (var y = 0; y < sizedHeight; y += inc)
					for (var x = 0; x < sizedWidth; x += inc)
						bw.Write(quantized[x, y]);

			bw.Close();
		}
	}
}
