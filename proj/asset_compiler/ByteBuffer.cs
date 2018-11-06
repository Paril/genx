using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace asset_compiler
{

	static class ByteBuffer
	{
		public static void BoxCopy(byte[] src, int src_x, int src_y, int src_w, int src_h, byte[] dst, int dst_x, int dst_y, int dst_w, int dst_h, int write_w, int write_h, bool skip_trans = false)
		{
			for (var y = 0; y < write_h; ++y)
			{
				var sy = src_y + y;
				var dy = dst_y + y;

				for (var x = 0; x < write_w; ++x)
				{
					var sx = src_x + x;
					var dx = dst_x + x;

					var si = (sy * src_w) + sx;
					var di = (dy * dst_w) + dx;

					if (dx < dst_w && dx >= 0 &&
						dy < dst_h && dy >= 0 &&
						(!skip_trans || src[si] != 255))
						dst[di] = src[si];
				}
			}
		}
	}

}
