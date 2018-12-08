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
		static float Lerp(float a, float b, float frac)
		{
			return a + (b - a) * frac;
		}

		static void SkipWhitespace(StreamReader reader)
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

		enum SpriteFlip
		{
			X = 1,
			Y = 2
		}

		enum SpriteLerp
		{
			Draws = 1,
			Translate = 2
		}

		class WeaponScript
		{
			public class Frame
			{
				public class DrawOp
				{
					public int SpriteIndex { get; set; }
					public int FrameIndex { get; set; }
					public Point Offset { get; set; } = Point.Empty;
					public SpriteAnchor MyAnchor { get; set; } = SpriteAnchor.BottomCenter;
					public SpriteAnchor ScreenAnchor { get; set; } = SpriteAnchor.BottomCenter;
					public bool FullScreen { get; set; } = false;
					public SpriteFlip Flip { get; set; } = 0;
				}

				public Point Translate { get; set; } = Point.Empty;
				public SpriteLerp Lerp { get; set; } = 0;
				public Dictionary<int, DrawOp> Draws { get; set; } = new Dictionary<int, DrawOp>();
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
			public Size VirtualSize { get; set; } = new Size(320, 200);
			public Size ScreenSize { get; set; } = new Size(1408, 720);
			public int SelectedFrame { get; set; }
			public int LastFrame { get; set; }
			public int[] Animations { get; set; } = new int[0];
		}

		static string ParseToken(StreamReader reader)
		{
			int c;
			string s = "";

			if (reader.EndOfStream)
				return null;

			// skip whitespace
			skipwhite:
			while ((c = reader.Peek()) <= ' ')
			{
				if (c == -1)
					return s;

				reader.Read();
			}

			if (c == '/')
			{
				reader.Read();

				// skip // comments
				if (reader.Peek() == '/')
				{
					reader.Read();

					while (reader.Peek() != -1 && reader.Peek() != '\n')
						reader.Read();

					goto skipwhite;
				}
				// skip /* */ comments
				else if (reader.Peek() == '*')
				{
					reader.Read();

					while (reader.Peek() != -1)
					{
						if (reader.Peek() == '*')
						{
							reader.Read();

							if (reader.Peek() == '/')
							{
								reader.Read();
								break;
							}
						}
						else
							reader.Read();
					}
					goto skipwhite;
				}
				else
					reader.BaseStream.Position--;
			}

			// handle quoted strings specially
			if (c == '\"')
			{
				reader.Read();
				while (true)
				{
					c = reader.Read();
					if (c == '\"' || c <= 0)
						goto finish;
					
					s += (char)c;
				}
			}

			// parse a regular word
			do
			{
				s += (char) c;

				reader.Read();
				c = reader.Peek();
			} while (c > 32);

			finish:
			return s;
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

		static void ParseFrame(WeaponScript.Frame frame, StreamReader reader)
		{
			while (true)
			{
				var token = ParseToken(reader);

				if (token == null || token == "}")
					break;

				switch (token)
				{
					case "translate":
						{
							var ofs = ParseToken(reader).Split(' ');
							frame.Translate = new Point(int.Parse(ofs[0]), int.Parse(ofs[1]));
						}
						break;
					case "lerp":
						{
							var ofs = ParseToken(reader).Split(' ');

							if (ofs.Contains("draws"))
								frame.Lerp |= SpriteLerp.Draws;
							if (ofs.Contains("translates"))
								frame.Lerp |= SpriteLerp.Translate;
						}
						break;
					case "draw_sprite":
					case "draw_sprite_flip_x":
					case "draw_sprite_flip_y":
					case "draw_sprite_flip_xy":
						{
							var op = new WeaponScript.Frame.DrawOp();
							op.SpriteIndex = int.Parse(ParseToken(reader));
							op.FrameIndex = int.Parse(ParseToken(reader));
							op.ScreenAnchor = AnchorFromString(ParseToken(reader));
							op.MyAnchor = AnchorFromString(ParseToken(reader));
							var ofs = ParseToken(reader).Split(' ');
							op.Offset = new Point(int.Parse(ofs[0]), int.Parse(ofs[1]));
							op.FullScreen = ParseToken(reader) == "full";

							if (token.StartsWith("draw_sprite_flip_"))
							{
								var flips = token.Substring("draw_sprite_flip_".Length);

								if (flips.Length == 2)
									op.Flip = SpriteFlip.X | SpriteFlip.Y;
								else if (flips[0] == 'y')
									op.Flip = SpriteFlip.Y;
								else
									op.Flip = SpriteFlip.X;
							}

							frame.Draws.Add(op.SpriteIndex, op);
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
					Stream stream;

					if ((stream = GetFile(basePath + "_" + i + ".tga")) != null)
						frame.Image = LoadTGA(stream);
					else if ((stream = GetFile(basePath + "_" + i + ".png")) != null)
						frame.Image = Image.FromStream(stream);

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
				if (ParseToken(reader) != "{")
					throw new Exception("invalid format");

				while (true)
				{
					var token = ParseToken(reader);

					if (token == "{")
					{
						var frame = new WeaponScript.Frame();
						ParseFrame(frame, reader);
						script.Frames.Add(frame);
						continue;
					}
					else if (token == "}")
						break;
					
					switch (token)
					{
						// skip these
						case "num_sprites":
							ParseToken(reader);
							break;
						// virtual size
						case "view":
							{
								var view = ParseToken(reader).Split(' ');
								script.VirtualSize = new Size(int.Parse(view[0]), int.Parse(view[1]));
							}
							break;
						// sprite
						case "load_sprite":
							{
								var sprite = new WeaponScript.Sprite();
								LoadSprite(sprite, ParseToken(reader));
								script.Sprites.Add(sprite);
							}
							break;
						// animations
						case "animations":
							script.Animations = ParseToken(reader).Split(new[] { ' ', '\t', '\r', '\n', }, StringSplitOptions.RemoveEmptyEntries).Select(a => int.Parse(a)).ToArray();
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

			public delegate void MemSetFunction(void* des, byte value, uint bytes);
			public static readonly MemSetFunction Set;

			static T GenerateMethod<T>(params OpCode[] opCodes) where T : Delegate
			{
				var method = typeof(T).GetMethod("Invoke");
				var dynamicMethod = new DynamicMethod(
					"Cpy",
					method.ReturnType,
					method.GetParameters().Select(t => t.ParameterType).ToArray(),
					typeof(Memory)
				);

				var ilGenerator = dynamicMethod.GetILGenerator();

				foreach (var code in opCodes)
					ilGenerator.Emit(code);

				return (T)dynamicMethod.CreateDelegate(typeof(T));
			}

			static Memory()
			{
				Cpy = GenerateMethod<MemCpyFunction>(OpCodes.Ldarg_0, OpCodes.Ldarg_1, OpCodes.Ldarg_2, OpCodes.Cpblk, OpCodes.Ret);
				Set = GenerateMethod<MemSetFunction>(OpCodes.Ldarg_0, OpCodes.Ldarg_1, OpCodes.Ldarg_2, OpCodes.Initblk, OpCodes.Ret);
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

				if (entry == null)
					entry = z.GetEntry(winPath);

				if (entry != null)
					return entry.Open();
			}

			return null;
		}

		void LoadPaths()
		{
			_paths.Add(new DirectoryInfo("."));

			foreach (var p in _paths)
				foreach (var zip in p.EnumerateFiles("*.pkz"))
					_zips.Add(new ZipArchive(zip.OpenRead()));
		}

		Timer _timer;
		long _nowFrame, _nextFrame;
		float _frameFrac;
		FileSystemWatcher _watcher;

		public Form1()
		{
			InitializeComponent();
			LoadPaths();

			_timer = new Timer();
			_timer.Interval = 13;
			_timer.Tick += _timer_Tick;

			_watcher = new FileSystemWatcher();
			_watcher.SynchronizingObject = this;
			_watcher.Changed += _watcher_Changed;
			_watcher.NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.Size;
		}

		private void _watcher_Changed(object sender, FileSystemEventArgs e)
		{
			reloadToolStripMenuItem_Click(sender, EventArgs.Empty);
		}

		private void _timer_Tick(object sender, EventArgs e)
		{
			var now = DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond;

			if (now > _nextFrame)
			{
				_nowFrame = now;
				_nextFrame = _nowFrame + 100;
				_root.LastFrame = _root.SelectedFrame;

				_programmaticallyChanged = true;
				button2_Click(sender, e);
				_programmaticallyChanged = false;
			}

			_frameFrac = (float)(now - _nowFrame) / (_nextFrame - _nowFrame);

			pictureBox1.Invalidate();
		}

		private PointF PointFromSizeAnchor(PointF pos, SizeF size, SpriteAnchor anchor)
		{
			var p = new PointF();

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

		private void DrawFlippedImage(Graphics g, Image image, float x, float y, float w, float h)
		{
			if (w < 0)
				x -= w;

			if (h < 0)
				y -= h;

			g.DrawImage(image, x, y, w, h);
		}

		// draw the frame within the virtual screen.
		// ortho should be set up as such
		private void paintFrame(Graphics g, int oldframe, int frame, PointF realScreenPosition, SizeF realScreenSize, float sidePadding)
		{
			var weaponFrame = _root.Frames[_root.Animations[frame]];
			var virtualPosition = new PointF(0, 0);
			var virtualSize = new SizeF(_root.VirtualSize.Width, _root.VirtualSize.Height);

			for (var i = _root.Sprites.Count - 1; i >= 0; i--)
			{
				if (!weaponFrame.Draws.TryGetValue(i, out var draw))
					continue;

				var sprite = _root.Sprites[draw.SpriteIndex];
				var spriteFrame = sprite.Frames[draw.FrameIndex];
				var objectSize = spriteFrame.Image.Size;
				var scr = PointFromSizeAnchor(draw.FullScreen ? realScreenPosition : virtualPosition, draw.FullScreen ? realScreenSize : virtualSize, draw.ScreenAnchor);
				var pos = PointFromSizeAnchor(PointF.Empty, objectSize, draw.MyAnchor);
				var flippedSize = new SizeF((draw.Flip & SpriteFlip.X) != 0 ? -objectSize.Width : objectSize.Width, (draw.Flip & SpriteFlip.Y) != 0 ? -objectSize.Height : objectSize.Height);

				var curX = sidePadding + (scr.X - pos.X + draw.Offset.X);
				var curY = scr.Y - pos.Y + draw.Offset.Y;
				PointF translate = weaponFrame.Translate;

				if (weaponFrame.Lerp != 0)
				{
					var prevWeaponFrame = _root.Frames[_root.Animations[oldframe]];
					var prevTranslate = prevWeaponFrame.Translate;

					if ((weaponFrame.Lerp & SpriteLerp.Draws) != 0 && prevWeaponFrame.Draws.TryGetValue(i, out var prevDraw))
					{
						var prevSprite = _root.Sprites[prevDraw.SpriteIndex];
						var prevSpriteFrame = prevSprite.Frames[prevDraw.FrameIndex];
						var prevObjectSize = prevSpriteFrame.Image.Size;
						var prevScr = PointFromSizeAnchor(prevDraw.FullScreen ? realScreenPosition : virtualPosition, prevDraw.FullScreen ? realScreenSize : virtualSize, prevDraw.ScreenAnchor);
						var prevPos = PointFromSizeAnchor(PointF.Empty, prevObjectSize, prevDraw.MyAnchor);

						var prevX = sidePadding + (prevScr.X - prevPos.X + prevDraw.Offset.X);
						var prevY = prevScr.Y - prevPos.Y + prevDraw.Offset.Y;

						curX = Lerp(prevX, curX, _frameFrac);
						curY = Lerp(prevY, curY, _frameFrac);
					}

					if ((weaponFrame.Lerp & SpriteLerp.Translate) != 0)
						translate = new PointF(Lerp(prevTranslate.X, translate.X, _frameFrac), Lerp(prevTranslate.Y, translate.Y, _frameFrac));
				}

				DrawFlippedImage(g, spriteFrame.Image, curX + translate.X, curY + translate.Y, flippedSize.Width, flippedSize.Height);
			}
		}

		private void pictureBox1_Paint(object sender, PaintEventArgs e)
		{
			e.Graphics.SmoothingMode = SmoothingMode.None;
			e.Graphics.PixelOffsetMode = PixelOffsetMode.None;
			e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
			e.Graphics.Clear(pictureBox1.BackColor);

			_root.ScreenSize = new Size(pictureBox1.Width, pictureBox1.Height);

			//e.Graphics.ScaleTransform(2, 2);

			var heightAspect = (float)_root.VirtualSize.Height / _root.ScreenSize.Height;
			var realWidth = _root.ScreenSize.Width * heightAspect;
			var sidePadding = (realWidth - _root.VirtualSize.Width) / 2.0f;

			var realScreenSize = new SizeF(realWidth, _root.VirtualSize.Height);
			var realScreenPosition = new PointF(-sidePadding, 0);

			e.Graphics.DrawRectangle(Pens.Red, 0, 0, realWidth - 1, _root.VirtualSize.Height - 1);

			e.Graphics.DrawRectangle(Pens.Blue, sidePadding, 0, _root.VirtualSize.Width - 1, _root.VirtualSize.Height - 1);

			e.Graphics.DrawLine(Pens.Cyan, sidePadding + (_root.VirtualSize.Width / 2), 0, sidePadding + (_root.VirtualSize.Width / 2), _root.VirtualSize.Height - 1);

			if (_root.Animations.Length != 0)
				paintFrame(e.Graphics, _root.LastFrame, _root.SelectedFrame, realScreenPosition, realScreenSize, sidePadding);
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
				ofd.Filter = "Weapon Script|*.wsc";

				if (ofd.ShowDialog() == DialogResult.OK)
				{
					reloadToolStripMenuItem.Enabled = true;
					_lastFile = ofd.FileName;

					reloadToolStripMenuItem_Click(sender, e);
				}
			}
		}

		private void reloadToolStripMenuItem_Click(object sender, EventArgs e)
		{
			var oldFrame = _root.SelectedFrame;
			_root = ParseScript(_lastFile);

			trackBar1.Minimum = 0;
			trackBar1.Maximum = _root.Animations.Length - 1;
			_root.SelectedFrame = _root.LastFrame = trackBar1.Value = Math.Min(trackBar1.Maximum, oldFrame);
			pictureBox1.Invalidate();

			if (_watcher.Path != Path.GetDirectoryName(_lastFile) ||
				_watcher.Filter != Path.GetFileName(_lastFile))
			{
				_watcher.Path = Path.GetDirectoryName(_lastFile);
				_watcher.Filter = Path.GetFileName(_lastFile);
				_watcher.EnableRaisingEvents = true;
			}
		}

		private void button1_Click(object sender, EventArgs e)
		{
			if (trackBar1.Value > 0)
				trackBar1.Value--;
			else
				trackBar1.Value = trackBar1.Maximum;
		}

		private void button2_Click(object sender, EventArgs e)
		{
			if (trackBar1.Value < trackBar1.Maximum)
				trackBar1.Value++;
			else
				trackBar1.Value = 0;
		}

		private void checkBox1_CheckedChanged(object sender, EventArgs e)
		{
			if (checkBox1.Checked)
				_timer.Start();
			else
				_timer.Stop();

			_nowFrame = DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond;
			_nextFrame = _nowFrame + 100;
		}

		bool _programmaticallyChanged = false;

		private void trackBar1_Scroll(object sender, EventArgs e)
		{
			if (!_programmaticallyChanged)
				_root.LastFrame = trackBar1.Value;
			_root.SelectedFrame = trackBar1.Value;
			pictureBox1.Invalidate();
		}
	}
}
