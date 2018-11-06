//#define CONVERT_TEXTURES

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using Ionic.Zip;

namespace asset_compiler
{
	class Program
	{
		static string ResolveShortcut(string filePath)
		{
			// IWshRuntimeLibrary is in the COM library "Windows Script Host Object Model"
			IWshRuntimeLibrary.WshShell shell = new IWshRuntimeLibrary.WshShell();

			try
			{
				IWshRuntimeLibrary.IWshShortcut shortcut = (IWshRuntimeLibrary.IWshShortcut)shell.CreateShortcut(new FileInfo(filePath).FullName + ".lnk");
				return shortcut.TargetPath;
			}
			catch (COMException)
			{
				// A COMException is thrown if the file is not a valid shortcut (.lnk) file 
				return null;
			}
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

			Console.WriteLine("Enter/paste path to Quake 1 'id1' directory (or 'n' to skip): ");
			var path = Console.ReadLine();

			if (path != "n")
			{
				var q1 = new DirectoryInfo(ResolveShortcut(path));

				// Copy Q1 files
				if (do_zips)
				{
					Globals._writeToZip = new ZipFile("quake1.pkz");
					Globals._writeToZip.CompressionLevel = Ionic.Zlib.CompressionLevel.BestCompression;
				}

				Console.WriteLine("Rifling through pak0...");
				Quake.Q1CopyStuff(q1.FullName + "\\pak0.pak");
				Console.WriteLine("Rifling through pak1...");
				Quake.Q1CopyStuff(q1.FullName + "\\pak1.pak");

				if (do_zips)
				{
					Globals._writeToZip.Save();
					Globals._writeToZip.Dispose();
				}
			}

			Console.WriteLine("Enter/paste path to Doom II (where doom2.wad is, or 'n' to skip') directory: ");
			path = Console.ReadLine();

			if (path != "n")
			{
				var d2 = new DirectoryInfo(ResolveShortcut(path));

				if (do_zips)
				{
					Globals._writeToZip = new ZipFile("doom2.pkz");
					Globals._writeToZip.CompressionLevel = Ionic.Zlib.CompressionLevel.BestCompression;
				}

				Console.WriteLine("Rifling through doom2.wad...");
				Doom.D2CopyStuff(d2.FullName + "\\doom2.wad");

				if (do_zips)
				{
					Globals._writeToZip.Save();
					Globals._writeToZip.Dispose();
				}
			}

			Console.WriteLine("Enter/paste path to Duke Nukem 3D (where duke3d.grp is, or 'n' to skip') directory: ");
			path = Console.ReadLine();

			if (path != "n")
			{
				var duke = new DirectoryInfo(ResolveShortcut(path));

				if (do_zips)
				{
					Globals._writeToZip = new ZipFile("duke.pkz");
					Globals._writeToZip.CompressionLevel = Ionic.Zlib.CompressionLevel.BestCompression;
				}

				Console.WriteLine("Rifling through duke3d.grp...");
				Duke.CopyStuff(duke.FullName + "\\duke3d.grp");

				if (do_zips)
				{
					Globals._writeToZip.Save();
					Globals._writeToZip.Dispose();
				}
			}
		}
	}
}
