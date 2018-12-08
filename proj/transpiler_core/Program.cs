using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Threading;
using System.Linq;
using System.Threading.Tasks;
using System.Text.RegularExpressions;

namespace asset_transpiler
{
	class Program
	{
		static void CheckPath(GameInfo gameInfo)
		{
			if (!string.IsNullOrEmpty(gameInfo.Path) && Directory.Exists(gameInfo.Path))
				return;

			Console.WriteLine("Enter or paste path to " + gameInfo.Name + " " + gameInfo.Archive + " (or containing folder):");
			gameInfo.Path = Console.ReadLine();

			if (gameInfo.Path.EndsWith("/" + gameInfo.Archive))
				gameInfo.Path = Path.GetDirectoryName(gameInfo.Path);
		}

		static bool WaitForYN()
		{
			ConsoleKeyInfo key;

			do
			{
				key = Console.ReadKey(true);
			} while (key.Key != ConsoleKey.Y && key.Key != ConsoleKey.N);

			return key.Key == ConsoleKey.Y;
		}

		static void LoadPaths()
		{
			if (File.Exists("asset_paths.cfg"))
			{
				var paths = File.ReadAllLines("asset_paths.cfg");

				for (var i = 0; i < Math.Min((int)(PaletteID.Total - 1), paths.Length); i++)
					Globals.GetGameInfo(PaletteID.Q1 + i).Path = paths[i];
			}

			restart:
			for (var i = PaletteID.Q1; i < PaletteID.Total; i++)
				CheckPath(Globals.GetGameInfo(i));

			Console.WriteLine("Paths loaded:");
			for (var i = PaletteID.Q1; i < PaletteID.Total; i++)
				Console.WriteLine(" " + Globals.GetGameInfo(i).Name + ": " + Globals.GetGameInfo(i).Path);
			Console.WriteLine("Are these still all good and valid? y/n");
			
			if (WaitForYN())
			{
				File.WriteAllLines("asset_paths.cfg", Globals.GameInfos.Skip(1).Select(a => a.Path));
				return;
			}

			for (var i = PaletteID.Q1; i < PaletteID.Total; i++)
				Globals.GetGameInfo(i).Path = "";
			goto restart;
		}

		[STAThread]
		static void Main(string[] args)
		{
			Console.WriteLine("Generations : Asset Transpiler");
			Console.WriteLine("Moves and renames assets from official games");
			Console.WriteLine("for use in this abomination.");
			Console.WriteLine("------------------------------");
			Console.WriteLine("Clear all existing stuff? This will PERMANENTLY DELETE these loose directories:\n" + string.Join(", ", Globals.PackedDirectories) + ", temp\ny/n");

			if (WaitForYN())
			{
				Console.WriteLine("Deleting old stuff...");

				for (var i = PaletteID.Q1; i < PaletteID.Total; i++)
				{
					if (File.Exists(Globals.GetGameInfo(i).ID + ".pkz"))
						File.Delete(Globals.GetGameInfo(i).ID + ".pkz");

					foreach (var dir in Globals.PackedDirectories)
						if (File.Exists(Globals.GetGameInfo(i).ID + "_" + dir + ".pkz"))
							File.Delete(Globals.GetGameInfo(i).ID + "_" + dir + ".pkz");
				}

				foreach (var dir in Globals.PackedDirectories)
					if (Directory.Exists(dir))
						Directory.Delete(dir, true);

				if (Directory.Exists("temp"))
				{
					Parallel.ForEach(Directory.EnumerateFiles(Globals.TempDir, "*", SearchOption.AllDirectories), File.Delete);
					Directory.Delete("temp", true);
				}
			}

			Console.WriteLine("Compress final output? y/n");
			Globals.DoZips = WaitForYN();

			if (Globals.DoZips)
			{
				Console.WriteLine("Use in-memory files? Uses more memory but packs quicker. y/n");
				Globals.DoMemoryFiles = WaitForYN();

				if (!Globals.DoMemoryFiles && !Directory.Exists("temp"))
					Directory.CreateDirectory("temp");

				//Console.WriteLine("Compress files optimally? Takes more time, but uses less disk space. y/n");
				//Globals.CompressZip = WaitForYN();
				Globals.CompressZip = true;

				Console.WriteLine("Merge types into game-split pkz's? (game.pkz vs game_sounds.pkz, game_textures.pkz, etc) y/n");
				Globals.SplitZips = !WaitForYN();
			}

			Console.WriteLine("Add development files (.wals, etc)? y/n");
			Globals.SaveWals = WaitForYN();

			LoadPaths();

			if (Globals.DoZips && !Globals.DoMemoryFiles)
				Directory.SetCurrentDirectory(Directory.GetCurrentDirectory() + Path.DirectorySeparatorChar + "temp");

			Console.WriteLine("Load Quake I data? y/n");
			var runQ1 = WaitForYN();

			Console.WriteLine("Load Doom II data? y/n");
			var runD2 = WaitForYN();

			Console.WriteLine("Load Duke Nukem 3D data? y/n");
			var runDuke = WaitForYN();

			Console.Clear();

			var parsers = new List<GameParser>();
			var sw = new Stopwatch();
			sw.Start();

			if (runQ1)
				parsers.Add(new Quake());
			if (runD2)
				parsers.Add(new Doom());
			if (runDuke)
				parsers.Add(new Duke());

			if (parsers.Count != 0)
			{
				Console.CursorVisible = false;
				GameParser.ClearTasks("Running asset transpiling tasks...");

				foreach (var parser in parsers)
					parser.Run();

				while (true)
				{
					GameParser.WriteProgressBar();

					if (GameParser.AllTasksFinished)
						break;

					Thread.Sleep(50);
				}

				GameParser.ClearTasks("Running cleanup tasks...");

				foreach (var parser in parsers)
					parser.Cleanup();

				while (true)
				{
					GameParser.WriteProgressBar();

					if (GameParser.AllTasksFinished)
						break;

					Thread.Sleep(50);
				}

				Console.CursorVisible = true;
			}

			if (Globals.DoZips)
			{
				Console.CursorVisible = false;

				if (!Globals.DoMemoryFiles)
					Directory.SetCurrentDirectory(Directory.GetParent(Directory.GetCurrentDirectory()).FullName);

				GameParser.ClearTasks("Creating asset pkz files...");

				var compressionLevel = Globals.CompressZip ? CompressionLevel.Fastest : CompressionLevel.NoCompression;

				void ZipFiles(string filename, Regex regex)
				{
					Interlocked.Increment(ref GameParser.NumTasksQueued);

					Task.Factory.StartNew(() =>
					{
						if (!Globals.DoMemoryFiles)
						{
							var files = Globals.PackedDirectories.Select(dir => Directory.EnumerateFiles(Globals.TempDir + dir, "*.*", SearchOption.AllDirectories).Where(x => regex.IsMatch(x.Substring(Globals.TempDir.Length)))).SelectMany(x => x).ToArray();

							if (files.Length != 0)
							{
								Interlocked.Add(ref GameParser.NumTasksQueued, files.Length);

								using (var archive = new ZipArchive(new FileStream(filename, FileMode.Create, FileAccess.Write), ZipArchiveMode.Create))
									foreach (var file in files)
										using (Stream fileStream = GameParser.OpenFileStream(file), fileStreamInZip = archive.CreateEntry(file.Substring(Globals.TempDir.Length), compressionLevel).Open())
										{
											fileStream.CopyTo(fileStreamInZip);
											Interlocked.Increment(ref GameParser.NumTasksDone);
										}
							}
						}
						else
						{

							var files = GameParser.MemoryFiles.Where(kv => regex.IsMatch(kv.Key)).ToArray();

							if (files.Length != 0)
							{
								Interlocked.Add(ref GameParser.NumTasksQueued, files.Length);

								using (var archive = new ZipArchive(new FileStream(filename, FileMode.Create, FileAccess.Write), ZipArchiveMode.Create))
									foreach (var file in files)
										using (var fileStreamInZip = archive.CreateEntry(file.Key, compressionLevel).Open())
										{
											file.Value.WriteTo(fileStreamInZip);
											Interlocked.Increment(ref GameParser.NumTasksDone);
										}
							}
						}

						Interlocked.Increment(ref GameParser.NumTasksDone);
					});
				}

				for (var i = PaletteID.Q1; i < PaletteID.Total; i++)
				{
					var gameInfo = Globals.GetGameInfo(i);

					if (!Globals.SplitZips)
						ZipFiles(gameInfo.ID + ".pkz", gameInfo.GetFileMatcher(Globals.PackedDirectories));
					else
					{
						foreach (var dir in Globals.PackedDirectories)
							ZipFiles(gameInfo.ID + "_" + dir + ".pkz", gameInfo.GetFileMatcher(dir));
					}
				}

				while (true)
				{
					GameParser.WriteProgressBar();

					if (GameParser.AllTasksFinished)
						break;

					Thread.Sleep(50);
				}

				if (!Globals.DoMemoryFiles)
				{
					Console.WriteLine("Deleting loose files...");

					Parallel.ForEach(Directory.EnumerateFiles(Globals.TempDir, "*", SearchOption.AllDirectories), File.Delete);
					Directory.Delete(Globals.TempDir, true);
				}

				Console.CursorVisible = true;
			}

			sw.Stop();
			Console.WriteLine("Finished in " + sw.Elapsed.TotalSeconds + " seconds");
			Console.WriteLine("Press any key to close.");
			Console.ReadKey(true);
		}
	}
}
