using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Text;
using System.Threading;

namespace asset_transpiler
{
	abstract class GameParser
	{
		public ZipArchive _writeToZip = null;
		public HashSet<string> _zipFiles = null;

		public void MakeZip(string zip_location)
		{
			_writeToZip = new ZipArchive(File.Create(zip_location), ZipArchiveMode.Create, false);
			_zipFiles = new HashSet<string>();
		}

		public void WriteFile(string local_file, byte[] data)
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

		public void CreateDirectory(string dir)
		{
			if (_writeToZip == null)
				Directory.CreateDirectory(dir);
		}

		public bool FileExists(string local_file)
		{
			if (_writeToZip != null)
				return _zipFiles.Contains(local_file);

			return File.Exists(local_file);
		}

		public Stream OpenFileStream(string local_file)
		{
			if (_writeToZip != null)
			{
				_zipFiles.Add(local_file);
				return _writeToZip.CreateEntry(local_file, CompressionLevel.Optimal).Open();
			}

			return File.Create(local_file);
		}

		public void WriteWav(string outwav, int sampleRate, int numSamples, byte[] samples, int channels = 1, int bps = 8)
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

		public void SaveWal(string outwal, string name, string next_name, int surf, int contents, byte[] data, PaletteID palette, int width, int height)
		{
			if (!Globals.SaveWals)
				return;

			using (var bw = new BinaryWriter(OpenFileStream(outwal)))
			{
				for (var k = 0; k < 32; ++k)
					bw.Write((byte)(k < name.Length ? name[k] : 0));

				bw.Write(width);
				bw.Write(height);

				var offset = 100;

				for (var i = 0; i < 4; i++)
				{
					bw.Write(offset);

					var w = Math.Max(1, width >> i);
					var h = Math.Max(1, height >> i);

					offset += w * h;
				}

				for (var k = 0; k < 32; ++k)
					bw.Write((byte)(k < next_name.Length ? next_name[k] : 0));

				bw.Write(surf);
				bw.Write(contents);
				bw.Write(0);

				// convert image to q2 palette
				byte[] q2 = new byte[data.Length];

				for (var i = 0; i < q2.Length; i++)
					q2[i] = (byte) Globals._palettes[(int)palette][data[i]].ClosestColor(PaletteID.Q2);

				bw.Write(q2);

				for (var i = 1; i < 4; i++)
				{
					var w = Math.Max(1, width >> i);
					var h = Math.Max(1, height >> i);
					var xf = (double)width / w;
					var yf = (double)height / h;
					var yh = 0;

					for (double y = 0; yh < h; y += yf, yh++)
					{
						var yw = 0;

						for (double x = 0; yw < w; x += xf, yw++)
							bw.Write(q2[(int)(Math.Truncate(y) * width + Math.Truncate(x))]);
					}
				}
			}
		}

		// status stoof
		int[] _statusMarks = new int[4];
		static Mutex _consoleMutex = new Mutex();

		static void ReserveStatusMarks(string title, int[] vals)
		{
			_consoleMutex.WaitOne();

			Console.WriteLine(title);

			for (var i = 0; i < vals.Length; i++)
			{
				vals[i] = Console.CursorTop;
				Console.WriteLine();
			}

			_consoleMutex.ReleaseMutex();
		}

		public void ClearStatus()
		{
			_consoleMutex.WaitOne();

			foreach (var mark in _statusMarks)
			{
				Console.CursorTop = mark;
				Console.CursorLeft = 0;
				Console.Write(new string('\0', Console.BufferWidth));
			}

			_consoleMutex.ReleaseMutex();
		}

		public void Status(string status, int mark)
		{
			_consoleMutex.WaitOne();

			Console.CursorTop = _statusMarks[mark];
			Console.CursorLeft = 0;
			
			Console.Write(new string('\0', Console.BufferWidth));
			Console.CursorTop = _statusMarks[mark];
			Console.CursorLeft = 0;

			Console.WriteLine(status);

			for (var i = mark + 1; i < _statusMarks.Length; i++)
			{
				Console.CursorTop = _statusMarks[i];
				Console.CursorLeft = 0;
				Console.Write(new string('\0', Console.BufferWidth));
			}

			_consoleMutex.ReleaseMutex();
		}

		protected GameParser()
		{
			ReserveStatusMarks("========== " + GetType().Name + " ==========", _statusMarks);
		}

		public abstract void Run();
	}
}
