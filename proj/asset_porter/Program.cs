using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;

namespace asset_transpiler
{
	class Program
	{
		static void CheckPath(ref string pathIn, string who, string what)
		{
			if (string.IsNullOrEmpty(pathIn) || !Directory.Exists(pathIn))
			{
				Console.WriteLine("Paste path to " + who + " " + what + " (or containing folder):");
				pathIn = Console.ReadLine();

				if (pathIn.EndsWith("/" + what))
					pathIn = Path.GetDirectoryName(pathIn);
			}
		}

		static void LoadPaths()
		{
			if (File.Exists("asset_paths.cfg"))
			{
				var paths = File.ReadAllLines("asset_paths.cfg");

				if (paths.Length >= 3)
				{
					Globals.Quake1Path = paths[0];
					Globals.DoomPath = paths[1];
					Globals.DukePath = paths[2];
				}
			}

restart:
			CheckPath(ref Globals.Quake1Path, "Quake", "pak0.pak");
			CheckPath(ref Globals.DoomPath, "Doom II", "doom2.wad");
			CheckPath(ref Globals.DukePath, "Duke Nukem 3D", "duke.grp");

			Console.WriteLine("Paths loaded:");
			Console.WriteLine("Quake 1: " + Globals.Quake1Path);
			Console.WriteLine("Doom II: " + Globals.DoomPath);
			Console.WriteLine("Duke 3D: " + Globals.DukePath);
			Console.WriteLine("Are these still all good and valid? y/n");

			var doit = Console.ReadLine();

			if (doit == "y")
			{
				File.WriteAllLines("asset_paths.cfg", new string[] { Globals.Quake1Path, Globals.DoomPath, Globals.DukePath });
				return;
			}

			Globals.Quake1Path = Globals.DoomPath = Globals.DukePath = "";
			goto restart;
		}

		static bool do_zips;

		static void RunParser(string pkzFile, GameParser parser)
		{
			if (do_zips)
				parser.MakeZip(pkzFile);

			parser.Run();

			if (do_zips)
				parser._writeToZip.Dispose();
		}

		[STAThread]
		static void Main(string[] args)
		{
			Console.WriteLine("Generations : Asset Transpiler");
			Console.WriteLine("Moves and renames assets from official games");
			Console.WriteLine("for use in this abomination.");
			Console.WriteLine("------------------------------");
			Console.WriteLine("Clear all existing stuff? y/n");
			var doit = Console.ReadLine();

			if (doit == "y")
			{
				Console.WriteLine("Deleting old stuff...");
				
				if (File.Exists("quake1.pkz"))
					File.Delete("quake1.pkz");
				if (File.Exists("doom2.pkz"))
					File.Delete("doom2.pkz");
				if (File.Exists("duke.pkz"))
					File.Delete("duke.pkz");
				
				if (Directory.Exists("models"))
					Directory.Delete("models", true);
				if (Directory.Exists("pics"))
					Directory.Delete("pics", true);
				if (Directory.Exists("sound"))
					Directory.Delete("sound", true);
				if (Directory.Exists("sprites"))
					Directory.Delete("sprites", true);
				if (Directory.Exists("textures"))
					Directory.Delete("textures", true);
			}

			Console.WriteLine("Write zips? y/n");
			do_zips = Console.ReadLine() == "y";

			Console.WriteLine("Add development files (.wals, etc)? y/n");
			Globals.SaveWals = Console.ReadLine() == "y";

			LoadPaths();
	
			Console.WriteLine("Load Quake I data? y/n");
			var runQ1 = Console.ReadLine() == "y";

			Console.WriteLine("Load Doom II data? y/n");
			var runD2 = Console.ReadLine() == "y";

			Console.WriteLine("Load Duke Nukem 3D data? y/n");
			var runDuke = Console.ReadLine() == "y";

			Console.Clear();

			List<Task> tasks = new List<Task>();

			var quake = new Quake();
			var doom = new Doom();
			var duke = new Duke();

			if (runQ1)
				tasks.Add(Task.Run(() => RunParser("quake1.pkz", quake)));

			if (runD2)
				tasks.Add(Task.Run(() => RunParser("doom2.pkz", doom)));

			if (runDuke)
				tasks.Add(Task.Run(() => RunParser("duke.pkz", duke)));

			Task.WaitAll(tasks.ToArray());
		}
	}
}
