/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
// snd_mem.c: sound caching

#include "sound.h"
#include <SDL2/SDL.h>

wavinfo_t s_info;

/*
================
ResampleSfx
================
*/
bool S_ResampleSfx(sfx_t *sfx, int wanted_rate, byte **data, size_t *num_samples)
{
	float stepscale = (float)s_info.rate / wanted_rate;      // this is usually 0.5, 1, or 2
	int outcount = s_info.samples / stepscale;

	if (!outcount)
	{
		Com_DPrintf("%s resampled to zero length\n", s_info.name);
		sfx->error = Q_ERR_TOO_FEW;
		return false;
	}

	*num_samples = outcount;
	sfx->loopstart = s_info.loopstart == -1 ? -1 : s_info.loopstart / stepscale;
	sfx->width = s_info.width;

	if (stepscale == 1)
	{
		*data = s_info.data;
		return true;
	}

	// general case
	const float frac = 256;
	const float frac_inv = 1.0f / frac;
	int samplefrac = 0;
	int fracstep = stepscale * frac;
	int i;
	byte *out_data = *data = S_Malloc(outcount * s_info.width);

	for (i = 0; i < outcount; i++)
	{
		int srcsample = (int)(samplefrac * frac_inv) * s_info.width;
		samplefrac += fracstep;
		memcpy(out_data, s_info.data + srcsample, s_info.width);
		out_data += s_info.width;
	}

	return true;
}

/*
===============================================================================

WAV loading

===============================================================================
*/

static byte     *data_p;
static byte     *iff_end;
static byte     *iff_data;
static uint32_t iff_chunk_len;

static int GetLittleShort(void)
{
	int val;

	if (data_p + 2 > iff_end)
		return -1;

	val = LittleShortMem(data_p);
	data_p += 2;
	return val;
}

static int GetLittleLong(void)
{
	int val;

	if (data_p + 4 > iff_end)
		return -1;

	val = LittleLongMem(data_p);
	data_p += 4;
	return val;
}

static void FindNextChunk(uint32_t search)
{
	uint32_t chunk, len;
	size_t remaining;

	while (data_p + 8 < iff_end)
	{
		chunk = RawLongMem(data_p);
		data_p += 4;
		len = LittleLongMem(data_p);
		data_p += 4;
		remaining = (size_t)(iff_end - data_p);

		if (len > remaining)
			len = remaining;

		if (chunk == search)
		{
			iff_chunk_len = len;
			return;
		}

		data_p += ALIGN(len, 2);
	}

	// didn't find the chunk
	data_p = NULL;
}

static void FindChunk(uint32_t search)
{
	data_p = iff_data;
	FindNextChunk(search);
}

#define TAG_RIFF    MakeRawLong('R', 'I', 'F', 'F')
#define TAG_WAVE    MakeRawLong('W', 'A', 'V', 'E')
#define TAG_fmt     MakeRawLong('f', 'm', 't', ' ')
#define TAG_cue     MakeRawLong('c', 'u', 'e', ' ')
#define TAG_LIST    MakeRawLong('L', 'I', 'S', 'T')
#define TAG_MARK    MakeRawLong('M', 'A', 'R', 'K')
#define TAG_data    MakeRawLong('d', 'a', 't', 'a')

static bool GetWavinfo(void)
{
	int format;
	int samples, width;
	uint32_t chunk;
	// find "RIFF" chunk
	FindChunk(TAG_RIFF);

	if (!data_p)
	{
		Com_DPrintf("%s has missing/invalid RIFF chunk\n", s_info.name);
		return false;
	}

	chunk = GetLittleLong();

	if (chunk != TAG_WAVE)
	{
		Com_DPrintf("%s has missing/invalid WAVE chunk\n", s_info.name);
		return false;
	}

	iff_data = data_p;
	// get "fmt " chunk
	FindChunk(TAG_fmt);

	if (!data_p)
	{
		Com_DPrintf("%s has missing/invalid fmt chunk\n", s_info.name);
		return false;
	}

	format = GetLittleShort();

	if (format != 1)
	{
		Com_DPrintf("%s has non-Microsoft PCM format\n", s_info.name);
		return false;
	}

	format = GetLittleShort();

	if (format != 1)
	{
		Com_DPrintf("%s has bad number of channels\n", s_info.name);
		return false;
	}

	s_info.rate = GetLittleLong();
	/* if (s_info.rate < 8000 || s_info.rate > 48000) {
	     Com_DPrintf("%s has bad rate\n", s_info.name);
	     return false;
	 }*/
	data_p += 4 + 2;
	width = GetLittleShort();

	switch (width)
	{
		case 8:
			s_info.width = 1;
			break;

		case 16:
			s_info.width = 2;
			break;

		default:
			Com_DPrintf("%s has bad width\n", s_info.name);
			return false;
	}

	// get cue chunk
	FindChunk(TAG_cue);

	if (data_p)
	{
		data_p += 24;
		s_info.loopstart = GetLittleLong();

		if (s_info.loopstart < 0 || s_info.loopstart > INT_MAX)
		{
			Com_DPrintf("%s has bad loop start\n", s_info.name);
			return false;
		}

		FindNextChunk(TAG_LIST);

		if (data_p)
		{
			data_p += 20;
			chunk = GetLittleLong();

			if (chunk == TAG_MARK)
			{
				// this is not a proper parse, but it works with cooledit...
				data_p += 16;
				samples = GetLittleLong();    // samples in loop

				if (samples < 0 || samples > INT_MAX - s_info.loopstart)
				{
					Com_DPrintf("%s has bad loop length\n", s_info.name);
					return false;
				}

				s_info.samples = s_info.loopstart + samples;
			}
		}
	}
	else
		s_info.loopstart = -1;

	// find data chunk
	FindChunk(TAG_data);

	if (!data_p)
	{
		Com_DPrintf("%s has missing/invalid data chunk\n", s_info.name);
		return false;
	}

	samples = iff_chunk_len / s_info.width;

	if (!samples)
	{
		Com_DPrintf("%s has zero length\n", s_info.name);
		return false;
	}

	if (s_info.samples)
	{
		if (samples < s_info.samples)
		{
			Com_DPrintf("%s has bad loop length\n", s_info.name);
			return false;
		}
	}
	else
		s_info.samples = samples;

	s_info.data = data_p;
	return true;
}

/*
==============
S_LoadSound
==============
*/
bool S_LoadSound(sfx_t *s)
{
	byte        *data;
	int         len;
	char        *name;

	if (s->name[0] == '*')
		return false;

	if (s->bufnum)
		return s;

	// don't retry after error
	if (s->error)
		return false;

	// load it in
	if (s->truename)
		name = s->truename;
	else
		name = s->name;

	len = FS_LoadFile(name, (void **)&data);

	if (!data)
	{
		s->error = len;
		return false;
	}

	memset(&s_info, 0, sizeof(s_info));
	s_info.name = name;
	iff_data = data;
	iff_end = data + len;

	/*SDL_RWops *myRWops = SDL_RWFromMem(data, len);
	SDL_AudioSpec wav_spec;
	Uint32 wav_length;
	Uint8 *wav_buffer;
	SDL_LoadWAV_RW(myRWops, 1, &wav_spec, &wav_buffer, &wav_length);
	SDL_RWclose(myRWops);*/

	if (!GetWavinfo())
	{
		s->error = Q_ERR_INVALID_FORMAT;
		goto fail;
	}

	if (s_started == SS_OAL)
	{
		byte *in_data = s_info.data;
		byte *out_data;

		if (!AL_UploadSfx(s, &out_data))
			goto fail;

		if (out_data != in_data)
			Z_Free(out_data);
	}

fail:
	FS_FreeFile(data);
	return true;
}

