using System;
using System.IO;
using System.IO.Compression;

namespace asset_transpiler
{
	class Program
	{
		static string Quake1Path = "";
		static string DoomPath = "";
		static string DukePath = "";

		static void LoadPaths()
		{
			if (File.Exists("asset_paths.cfg"))
			{
				var paths = File.ReadAllLines("asset_paths.cfg");

				if (paths.Length >= 3)
				{
					Quake1Path = paths[0];
					DoomPath = paths[1];
					DukePath = paths[2];
				}
			}

restart:

			if (string.IsNullOrEmpty(Quake1Path) || !Directory.Exists(Quake1Path))
			{
				Console.WriteLine("Paste path to Quake 1 pak0.pak (or containing folder):");
				Quake1Path = Console.ReadLine();

				if (Quake1Path.EndsWith("/pak0.pak"))
					Quake1Path = Path.GetDirectoryName(Quake1Path);
			}

			if (string.IsNullOrEmpty(DoomPath) || !Directory.Exists(DoomPath))
			{
				Console.WriteLine("Paste path to Doom II doom2.wad (or containing folder):");
				DoomPath = Console.ReadLine();

				if (DoomPath.EndsWith("/doom2.wad"))
					DoomPath = Path.GetDirectoryName(DoomPath);
			}

			if (string.IsNullOrEmpty(DukePath) || !Directory.Exists(DukePath))
			{
				Console.WriteLine("Paste path to Duke 3D duke.grp (or containing folder):");
				DukePath = Console.ReadLine();

				if (DukePath.EndsWith("/duke.grp"))
					DukePath = Path.GetDirectoryName(DukePath);
			}

			Console.WriteLine("Paths loaded:");
			Console.WriteLine("Quake 1: " + Quake1Path);
			Console.WriteLine("Doom II: " + DoomPath);
			Console.WriteLine("Duke 3D: " + DukePath);
			Console.WriteLine("Are these still all good and valid? y/n");

			var doit = Console.ReadLine();

			if (doit == "y")
			{
				File.WriteAllLines("asset_paths.cfg", new string[] { Quake1Path, DoomPath, DukePath });
				return;
			}

			Quake1Path = DoomPath = DukePath = "";
			goto restart;
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
				Globals.OpenStatus("Deleting old stuff...");

				Globals.Status("ZIPs");

				if (File.Exists("quake1.pkz"))
					File.Delete("quake1.pkz");
				if (File.Exists("doom2.pkz"))
					File.Delete("doom2.pkz");
				if (File.Exists("duke.pkz"))
					File.Delete("duke.pkz");

				Globals.Status("Models");
				if (Directory.Exists("models"))
					Directory.Delete("models", true);
				Globals.Status("Pics");
				if (Directory.Exists("pics"))
					Directory.Delete("pics", true);
				Globals.Status("Sound");
				if (Directory.Exists("sound"))
					Directory.Delete("sound", true);
				Globals.Status("Sprites");
				if (Directory.Exists("sprites"))
					Directory.Delete("sprites", true);
				Globals.Status("Textures");
				if (Directory.Exists("textures"))
					Directory.Delete("textures", true);

				Globals.CloseStatus();
			}

			Console.WriteLine("Write zips? y/n");
			var do_zips = Console.ReadLine() == "y";

			LoadPaths();
	
			Console.WriteLine("Load Quake I data? y/n");
			var path = Console.ReadLine();

			if (path != "n")
			{
				var q1 = new DirectoryInfo(Quake1Path);

				// Copy Q1 files
				if (do_zips)
					Globals.MakeZip("quake1.pkz");

				Console.WriteLine("Rifling through pak0...");
				Quake.Q1CopyStuff(q1.FullName + "\\pak0.pak");
				Console.WriteLine("Rifling through pak1...");
				Quake.Q1CopyStuff(q1.FullName + "\\pak1.pak");

				if (do_zips)
				{
					Console.WriteLine("Saving quake1.pkz...");
					Globals._writeToZip.Dispose();
				}
			}

			Console.WriteLine("Load Doom II data? y/n");
			path = Console.ReadLine();

			if (path != "n")
			{
				var d2 = new DirectoryInfo(DoomPath);

				if (do_zips)
					Globals.MakeZip("doom2.pkz");

				Console.WriteLine("Rifling through doom2.wad...");
				Doom.D2CopyStuff(d2.FullName + "\\doom2.wad");

				if (do_zips)
				{
					Console.WriteLine("Saving doom2.pkz...");
					Globals._writeToZip.Dispose();
				}
			}

			Console.WriteLine("Load Duke Nukem 3D data? y/n");
			path = Console.ReadLine();

			if (path != "n")
			{
				var duke = new DirectoryInfo(DukePath);

				if (do_zips)
					Globals.MakeZip("duke.pkz");

				Console.WriteLine("Rifling through duke3d.grp...");
				Duke.CopyStuff(duke.FullName + "\\duke3d.grp");

				if (do_zips)
				{
					Console.WriteLine("Saving duke.pkz...");
					Globals._writeToZip.Dispose();
				}
			}
		}
	}
}
