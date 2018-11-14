using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection.Emit;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace wsc_designer
{
	public partial class Form1 : Form
	{
		void SkipWhitespace(StreamReader reader)
		{
			while (!reader.EndOfStream)
			{
				if (char.IsWhiteSpace((char)reader.Peek()))
					reader.Read();
				else
					break;
			}
		}

		enum SpriteAnchor
		{
			TopLeft,
			TopCenter,
			TopRight,
			CenterRight,
			BottomRight,
			BottomCenter,
			BottomLeft,
			CenterLeft,
			Center
		}

		class WeaponScript
		{
			public class Frame
			{
				public class DrawOp
				{
					[DefaultValue(0)]
					public int SpriteIndex { get; set; }

					[DefaultValue(0)]
					public int FrameIndex { get; set; }

					[DefaultValue(typeof(Point), "0, 0")]
					public Point Offset { get; set; } = new Point(0, 0);

					[DefaultValue(typeof(SpriteAnchor), "BottomCenter")]
					public SpriteAnchor MyAnchor { get; set; } = SpriteAnchor.BottomCenter;

					[DefaultValue(typeof(SpriteAnchor), "BottomCenter")]
					public SpriteAnchor ScreenAnchor { get; set; } = SpriteAnchor.BottomCenter;

					[DefaultValue(false)]
					public bool FullScreen { get; set; } = false;
				}

				[DefaultValue(false)]
				public bool Lerp { get; set; } = false;

				public List<DrawOp> Draws { get; set; } = new List<DrawOp>();
			}

			public class Sprite
			{
				public class Frame
				{
					public Image Image { get; set; }
				}

				public List<Frame> Frames = new List<Frame>();
			}

			public List<Frame> Frames = new List<Frame>();
			public List<Sprite> Sprites = new List<Sprite>();

			[DefaultValue(typeof(Size), "320, 240")]
			[ReadOnly(true)]
			public Size VirtualSize { get; set; } = new Size(320, 240);

			[DefaultValue(typeof(Size), "1280, 720")]
			public Size ScreenSize { get; set; } = new Size(1280, 720);
		}

		string[] SplitTokens(string line)
		{
			var tokens = new List<string>();
			var token = "";
			var tokenizing = false;

			for (var i = 0; i < line.Length; i++)
			{
				char c = line[i];

				if (c == '"')
				{
					if (tokenizing)
						tokens.Add(token);

					tokenizing = !tokenizing;
					token = "";
					continue;
				}
				else if (c == '\\')
				{
					i++;
					c = line[i];
				}

				token += c;
			}

			if (tokenizing)
				throw new Exception("bad line");

			return tokens.ToArray();
		}

		static SpriteAnchor AnchorFromString(string s)
		{
			switch (s)
			{
				case "top left":
					return SpriteAnchor.TopLeft;
				case "top center":
					return SpriteAnchor.TopCenter;
				case "top right":
					return SpriteAnchor.TopRight;
				case "center right":
					return SpriteAnchor.CenterRight;
				case "bottom right":
					return SpriteAnchor.BottomRight;
				case "bottom center":
					return SpriteAnchor.BottomCenter;
				case "bottom left":
					return SpriteAnchor.BottomLeft;
				case "center left":
					return SpriteAnchor.CenterLeft;
				case "center":
					return SpriteAnchor.Center;
			}

			throw new Exception();
		}

		void ParseFrame(WeaponScript.Frame frame, StreamReader reader)
		{
			reader.Read();

			while (true)
			{
				SkipWhitespace(reader);

				if (reader.EndOfStream)
					break;

				if (reader.Peek() == '}')
				{
					reader.Read();
					break;
				}

				var tokens = SplitTokens(reader.ReadLine());

				switch (tokens[0])
				{
					case "lerp":
						frame.Lerp = tokens[1] == "true";
						break;
					case "draw_sprite":
						{
							var op = new WeaponScript.Frame.DrawOp();
							op.SpriteIndex = int.Parse(tokens[1]);
							op.FrameIndex = int.Parse(tokens[2]);
							op.ScreenAnchor = AnchorFromString(tokens[3]);
							op.MyAnchor = AnchorFromString(tokens[4]);
							var ofs = tokens[5].Split(' ');
							op.Offset = new Point(int.Parse(ofs[0]), int.Parse(ofs[1]));
							frame.Draws.Add(op);
						}
						break;
					default:
						throw new Exception("bad type/func");
				}
			}
		}

		void LoadSprite(WeaponScript.Sprite sprite, string filename)
		{
			using (var br = new BinaryReader(GetFile(filename), Encoding.ASCII, false))
			{
				br.ReadBytes(4);

				var basePath = filename.Substring(0, filename.Length - 4); 
				var numPics = br.ReadByte();

				for (var i = 0; i < numPics; i++)
				{
					var frame = new WeaponScript.Sprite.Frame();
					frame.Image = LoadTGA(GetFile(basePath + "_" + i + ".tga"));
					br.ReadBytes(4);
					sprite.Frames.Add(frame);
				}

				// rest of file is ignored
			}
		}

		WeaponScript ParseScript(string filename)
		{
			var script = new WeaponScript();

			using (var reader = new StreamReader(filename, Encoding.ASCII))
			{
				SkipWhitespace(reader);

				if (reader.Read() != '{')
					throw new Exception("invalid format");

				while (true)
				{
					SkipWhitespace(reader);

					if (reader.EndOfStream)
						break;

					if (reader.Peek() == '{')
					{
						var frame = new WeaponScript.Frame();
						ParseFrame(frame, reader);
						script.Frames.Add(frame);
						continue;
					}
					else if (reader.Peek() == '}')
						break;
					else if (reader.Peek() == '/')
					{
						reader.Read();

						// line comment
						if (reader.Peek() == '/')
						{
							reader.ReadLine();
							continue;
						}
						// block comment
						else if (reader.Peek() == '*')
						{
							reader.Read();

							while (true)
							{
								if (reader.EndOfStream)
									break;

								char c = (char) reader.Read();

								if (c == '*' && reader.Peek() == '/')
								{
									reader.Read();
									break;
								}
							}

							continue;
						}
					}

					var tokens = SplitTokens(reader.ReadLine());

					switch (tokens[0])
					{
						// skip these
						case "num_frames":
						case "num_sprites":
							break;
						// virtual size
						case "view":
							{
								var view = tokens[1].Split(' ');
								script.VirtualSize = new Size(int.Parse(view[0]), int.Parse(view[1]));
							}
							break;
						// sprite
						case "load_sprite":
							{
								var sprite = new WeaponScript.Sprite();
								LoadSprite(sprite, tokens[1]);
								script.Sprites.Add(sprite);
							}
							break;
						default:
							throw new Exception("bad type/func");
					}
				}
			}

			return script;
		}

		WeaponScript _root = new WeaponScript();

		static unsafe class Memory
		{
			public delegate void MemCpyFunction(void* des, void* src, uint bytes);
			public static readonly MemCpyFunction Cpy;

			static Memory()
			{
				var dynamicMethod = new DynamicMethod(
					"Cpy",
					typeof(void),
					new[] { typeof(void*), typeof(void*), typeof(uint) },
					typeof(Memory)
				);

				var ilGenerator = dynamicMethod.GetILGenerator();

				ilGenerator.Emit(OpCodes.Ldarg_0);
				ilGenerator.Emit(OpCodes.Ldarg_1);
				ilGenerator.Emit(OpCodes.Ldarg_2);

				ilGenerator.Emit(OpCodes.Cpblk);
				ilGenerator.Emit(OpCodes.Ret);

				Cpy = (MemCpyFunction)dynamicMethod.CreateDelegate(typeof(MemCpyFunction));
			}
		}

		static Image LoadTGA(Stream stream)
		{
			using (var br = new BinaryReader(stream, Encoding.ASCII, false))
			{
				br.ReadBytes(12);

				var width = br.ReadUInt16();
				var height = br.ReadUInt16();

				br.ReadBytes(2);

				unsafe
				{
					byte[] bytes = br.ReadBytes(width * height * 4);

					fixed (byte* pixels = bytes)
					{
						int* intPixels = (int*)pixels;
						
						var image = new Bitmap(width, height);
						var data = image.LockBits(new Rectangle(0, 0, width, height), System.Drawing.Imaging.ImageLockMode.WriteOnly, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
						for (var y = 0; y < height; y++)
						{
							void* from = intPixels + (y * width);
							void* to = ((int*)data.Scan0) + (height - y - 1) * width;
							Memory.Cpy(to, from, (uint) data.Stride);
						}
						image.UnlockBits(data);
						return image;
					}
				}
			}
		}

		List<DirectoryInfo> _paths = new List<DirectoryInfo>();
		List<ZipArchive> _zips = new List<ZipArchive>();

		Stream GetFile(string path)
		{
			var winPath = path.Replace('/', Path.DirectorySeparatorChar);

			foreach (var p in _paths)
			{
				var file = new FileInfo(p.FullName + Path.DirectorySeparatorChar + winPath);

				if (file.Exists)
					return file.OpenRead();
			}

			foreach (var z in _zips)
			{
				var entry = z.GetEntry(path);

				if (entry != null)
					return entry.Open();
			}

			throw new Exception("not found");
		}

		void LoadPaths()
		{
			_paths.Add(new DirectoryInfo("."));

			foreach (var p in _paths)
				foreach (var zip in p.EnumerateFiles("*.pkz"))
					_zips.Add(new ZipArchive(zip.OpenRead()));
		}

		public Form1()
		{
			InitializeComponent();
			LoadPaths();
		}

		private Point PointFromSizeAnchor(Point pos, Size size, SpriteAnchor anchor)
		{
			var p = new Point();

			if (anchor == SpriteAnchor.BottomLeft ||
				anchor == SpriteAnchor.CenterLeft ||
				anchor == SpriteAnchor.TopLeft)
				p.X = pos.X;
			else if (anchor == SpriteAnchor.BottomRight ||
				anchor == SpriteAnchor.CenterRight ||
				anchor == SpriteAnchor.TopRight)
				p.X = pos.X + size.Width;
			else
				p.X = pos.X + size.Width / 2;

			if (anchor == SpriteAnchor.TopRight ||
				anchor == SpriteAnchor.TopCenter ||
				anchor == SpriteAnchor.TopLeft)
				p.Y = pos.Y;
			else if (anchor == SpriteAnchor.BottomRight ||
				anchor == SpriteAnchor.BottomCenter ||
				anchor == SpriteAnchor.BottomLeft)
				p.Y = pos.Y + size.Height;
			else
				p.Y = pos.Y + size.Height / 2;

			return p;
		}

		private void pictureBox1_Paint(object sender, PaintEventArgs e)
		{
			e.Graphics.SmoothingMode = SmoothingMode.None;
			e.Graphics.PixelOffsetMode = PixelOffsetMode.None;
			e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
			e.Graphics.Clear(pictureBox1.BackColor);

			// fit screen size inside of box
			float outerAspectH = (float)pictureBox1.Height / _root.ScreenSize.Height;
			float outerAspect = outerAspectH;

			float screenWidth = _root.ScreenSize.Width * outerAspect;
			float screenHeight = _root.ScreenSize.Height * outerAspect;

			// this isn't necessary in Q2 code
			e.Graphics.TranslateTransform((pictureBox1.Width / 2.0f) - (screenWidth / 2.0f), 0);

			var realScreenPosition = new Point(0, 0);
			var realScreenSize = new Size((int)screenWidth, (int)screenHeight);

			e.Graphics.DrawRectangle(Pens.Red, 0, 0, screenWidth - 1, screenHeight - 1);

			// fit 800x600 virtual screen inside real screen
			float innerAspectW = screenWidth / _root.VirtualSize.Width;
			float innerAspectH = screenHeight / _root.VirtualSize.Height;
			float innerAspect = Math.Min(innerAspectW, innerAspectH);

			float innerWidth = _root.VirtualSize.Width * innerAspect;
			float innerHeight = _root.VirtualSize.Height * innerAspect;
			
			var virtualPosition = new Point((int)((screenWidth / 2.0f) - (innerWidth / 2.0f)), (int)((screenHeight / 2.0f) - (innerHeight / 2.0f)));

			e.Graphics.DrawRectangle(Pens.Blue, virtualPosition.X, virtualPosition.Y, innerWidth - 1, innerHeight - 1);

			float objectScale = innerWidth / _root.VirtualSize.Width;

			var virtualSize = new Size((int)(_root.VirtualSize.Width * objectScale), (int)(_root.VirtualSize.Height * objectScale));

			var halfW = (_root.VirtualSize.Width / 2) * objectScale;
			var halfH = (_root.VirtualSize.Height / 2) * objectScale;

			e.Graphics.DrawLine(Pens.Cyan, virtualPosition.X + halfW, virtualPosition.Y, virtualPosition.X + halfW, virtualPosition.Y + virtualSize.Height - 2);
			e.Graphics.DrawLine(Pens.Cyan, virtualPosition.X, virtualPosition.Y + halfH, virtualPosition.X + virtualSize.Width - 2, virtualPosition.Y + halfH);

			if (_root.Frames.Count != 0)
			{
				var frame = _root.Frames[0];

				for (var i = frame.Draws.Count - 1; i >= 0; i--)
				{
					var draw = frame.Draws[i];
					var sprite = _root.Sprites[draw.SpriteIndex];
					var spriteFrame = sprite.Frames[draw.FrameIndex];

					var objectSize = new Size((int)(spriteFrame.Image.Height * objectScale), (int)(spriteFrame.Image.Height * objectScale));

					var scr = PointFromSizeAnchor(draw.FullScreen ? realScreenPosition : virtualPosition, draw.FullScreen ? realScreenSize : virtualSize, draw.ScreenAnchor);
					var pos = PointFromSizeAnchor(new Point(0, 0), objectSize, draw.MyAnchor);

					e.Graphics.DrawImage(spriteFrame.Image, scr.X - pos.X + (int)(draw.Offset.X * objectScale), scr.Y - pos.Y + (int)(draw.Offset.Y * objectScale), objectSize.Width, objectSize.Height);

					//e.Graphics.DrawImageUnscaled(spriteFrame.Image, (_root.Translate.X + frame.Translate.X + draw.Offset.X) + -spriteFrame.Offset.X, (_root.Translate.Y + frame.Translate.Y + draw.Offset.Y) + -spriteFrame.Offset.Y);
				}
			}
		}

		private void propertyGrid1_PropertyValueChanged(object s, PropertyValueChangedEventArgs e)
		{
			pictureBox1.Invalidate();
		}

		private void Form1_SizeChanged(object sender, EventArgs e)
		{
			pictureBox1.Invalidate();
		}

		string _lastFile = null;

		private void openToolStripMenuItem_Click(object sender, EventArgs e)
		{
			using (var ofd = new OpenFileDialog())
			{
				ofd.Filter = "Weapon Script|*.wsc2";

				if (ofd.ShowDialog() == DialogResult.OK)
				{
					reloadToolStripMenuItem.Enabled = true;
					_root = ParseScript(_lastFile = ofd.FileName);
					pictureBox1.Invalidate();
				}
			}
		}

		private void reloadToolStripMenuItem_Click(object sender, EventArgs e)
		{
			_root = ParseScript(_lastFile);
			pictureBox1.Invalidate();
		}
	}
}
