using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Linq;
using System.Collections.Concurrent;
using ImageType = SixLabors.ImageSharp.Image<SixLabors.ImageSharp.PixelFormats.Bgra32>;

namespace asset_transpiler
{
	abstract class GameParser
	{
		static IDictionary<string, MemoryStream> _memoryFiles = new ConcurrentDictionary<string, MemoryStream>();

		public static IDictionary<string, MemoryStream> MemoryFiles { get => _memoryFiles; }

		public static string[] GetMemoryFiles() => _memoryFiles.Keys.ToArray();

		public class StreamWrapper : Stream
		{
			Stream _stream;

			public StreamWrapper(Stream stream) :
				base()
			{
				_stream = stream;
			}

			public override bool CanRead => _stream.CanRead;

			public override bool CanSeek => _stream.CanSeek;

			public override bool CanWrite => _stream.CanWrite;

			public override long Length => _stream.Length;

			public override long Position { get => _stream.Position; set => _stream.Position = value; }

			public override void Flush() => _stream.Flush();

			public override int Read(byte[] buffer, int offset, int count) => _stream.Read(buffer, offset, count);

			public override long Seek(long offset, SeekOrigin origin) => _stream.Seek(offset, origin);

			public override void SetLength(long value) => _stream.SetLength(value);

			public override void Write(byte[] buffer, int offset, int count) => _stream.Write(buffer, offset, count);
		}

		public static Stream OpenFileStream(string local_file, int _initialSize = 0)
		{
			if (Globals.DoMemoryFiles)
			{
				MemoryStream stream;

				if (!_memoryFiles.TryGetValue(local_file, out stream))
				{
					stream = new MemoryStream(_initialSize);
					_memoryFiles.Add(local_file, stream);
				}
				else
					stream.Position = 0;

				return new StreamWrapper(stream);
			}

			Directory.CreateDirectory(Path.GetDirectoryName(local_file));
			
			return File.Open(local_file, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.ReadWrite);
		}

		public static void MoveFile(string old_name, string new_name)
		{
			if (Globals.DoMemoryFiles)
			{
				_memoryFiles.TryGetValue(old_name, out var value);
				_memoryFiles.Remove(old_name);
				_memoryFiles.Add(new_name, value);
				return;
			}

			File.Move(old_name, new_name);
		}

		public static Stream OpenArchive(string archive_file)
		{
			return File.Open(archive_file, FileMode.Open, FileAccess.Read, FileShare.Read);
		}

		public static void WriteWav(string outwav, int sampleRate, int numSamples, byte[] samples, int channels = 1, int bps = 8)
		{
			using (var ms = new MemoryStream())
			using (var wr = new BinaryWriter(ms, Encoding.ASCII, true))
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

		private static void SaveWal(string outwal, string name, string next_name, int surf, int contents, byte[] data, PaletteID palette, int width, int height)
		{
			if (!Globals.SaveWals)
				return;

			using (var bw = new BinaryWriter(OpenFileStream(outwal, 100 + (width >> 0 * height >> 0) + (width >> 1 * height >> 1) + (width >> 2 * height >> 2) + (width >> 3 * height >> 3) + (width >> 4 * height >> 4))))
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
				unsafe
				{
					var q2 = stackalloc byte[data.Length];

					for (var i = 0; i < data.Length; i++)
						q2[i] = Globals.GetClosestMappedQ2Color(palette, data[i]);

					bw.Write(new ReadOnlySpan<byte>(q2, data.Length));

					for (var i = 1; i < 4; i++)
					{
						var w = Math.Max(1, width >> i);
						var h = Math.Max(1, height >> i);
						var xf = (float)width / w;
						var yf = (float)height / h;
						var yh = 0;
						int p = 0;
						var newPixels = stackalloc byte[w * h];

						for (float y = 0; yh < h; y += yf, yh++)
						{
							var yw = 0;

							for (float x = 0; yw < w; x += xf, yw++, p++)
								newPixels[p] = q2[(int)(Math.Floor(y) * width + Math.Floor(x))];
						}

						bw.Write(new ReadOnlySpan<byte>(newPixels, w * h));
					}
				}
			}
		}

		static void TaskSaveTga((ImageType bmp, string outtga) info)
		{
			using (var strm = OpenFileStream(info.outtga, info.bmp.TgaSize()))
				info.bmp.SaveAsImage(strm);
		}

		static void TaskSaveWal((string outwal, string walname, string next_name, int surf, int contents, byte[] data, PaletteID palette, int width, int height) info)
		{
			SaveWal(info.outwal, info.walname, info.next_name, info.surf, info.contents, info.data, info.palette, info.width, info.height);
		}

		static void TaskFreeImage(ImageType bmp)
		{
			bmp.Dispose();
		}

		public static void SaveWalTga(ImageType bmp, string outtga, string outwal, string walname, string next_name, int surf, int contents, byte[] data, PaletteID palette, int width, int height)
		{
			CreateTaskWhenAll(TaskFreeImage, bmp,
								CreateTask(TaskSaveTga, (bmp, outtga)),
								CreateTask(TaskSaveWal, (outwal, walname, next_name, surf, contents, data, palette, width, height)));
		}

		public static int NumTasksDone;
		public static int NumTasksQueued;
		
		public static bool AllTasksFinished => NumTasksQueued == NumTasksDone;

		static char _finishedChar = '█', _25 = '░', _50 = '▓', _75 = '▓', _unfinishedChar = '-';
		static int _lastWidth = 0;

		public static void WriteProgressBar()
		{
			if (Console.BufferWidth != _lastWidth)
			{
				_lastWidth = Console.BufferWidth;
				Console.Clear();
			}

			Console.SetCursorPosition(0, 1);

			var pips = Console.BufferWidth - 2;
			var done = (float)NumTasksDone / NumTasksQueued;
			
			ThreadPool.GetAvailableThreads(out int workerThreads, out int completionPortThreads);
			ThreadPool.GetMaxThreads(out int maxWorkerThreads, out int maxCompletionPortThreads);

			Console.WriteLine(((int)(done * 100) + "% (" + NumTasksDone + " processed / " + NumTasksQueued + " total) | " + (maxWorkerThreads - workerThreads) + " workers inuse, " + (maxCompletionPortThreads - completionPortThreads) + " I/O threads inuse").PadRight(Console.BufferWidth, '\0'));

			Console.SetCursorPosition(0, 2);

			done *= pips;
			var doneTotal = (int)Math.Truncate(done);
			var frac = done - doneTotal;

			Console.Write('[');

			for (var i = 0; i < doneTotal; i++)
			{
				if (i == doneTotal - 1)
				{
					if (frac < 0.25)
						Console.Write(_25);
					else if (frac < 0.50)
						Console.Write(_50);
					else if (frac < 0.75)
						Console.Write(_75);
					else
						Console.Write(_finishedChar);
				}
				else
					Console.Write(_finishedChar);
			}
			for (var i = 0; i < pips - done; i++)
				Console.Write(_unfinishedChar);

			Console.Write(']');
		}

		public static void ClearTasks(string status, int queued = 0, int done = 0)
		{
			Console.Clear();
			Console.WriteLine(status);
			Interlocked.Exchange(ref NumTasksQueued, queued);
			Interlocked.Exchange(ref NumTasksDone, done);
		}

		protected static Task CreateTask<T>(Action<T> func, T arg)
		{
			Interlocked.Increment(ref NumTasksQueued);
			return Task.Factory.StartNew(() =>
			{
				func(arg);
				Interlocked.Increment(ref NumTasksDone);
			});
		}
	
		protected static Task CreateTask(Action func)
		{
			Interlocked.Increment(ref NumTasksQueued);
			return Task.Factory.StartNew(() =>
			{
				func();
				Interlocked.Increment(ref NumTasksDone);
			});
		}

		protected static Task CreateTaskWhenAll<T>(Action<T> func, T arg, params Task[] tasks)
		{
			Interlocked.Increment(ref NumTasksQueued);
			return Task.Factory.ContinueWhenAll(tasks, (t) =>
			{
				func(arg);
				Interlocked.Increment(ref NumTasksDone);
			});
		}

		public abstract void Run();
		public virtual void Cleanup() { }
	}
}
