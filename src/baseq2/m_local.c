#include "m_local.h"
#include "format/mdl.h"
#include "format/md2.h"
#include "shared/cJSON.h"




#define WIN32_LEAN_AND_MEAN
#define PSAPI_VERSION 1
#include <Windows.h>
#include <DbgHelp.h>
#include <Psapi.h>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")

static HMODULE GetCurrentModule()
{
	HMODULE hModule = NULL;
	GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCTSTR)GetCurrentModule,
		&hModule);

	return hModule;
}

static void SymbolName(const void *addr, char *buffer, size_t buffer_len)
{
	static bool init = false;

	if (!init)
	{
		init = true;
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

		if (!SymInitialize(GetCurrentProcess(), NULL, TRUE))
			gi.error("SymInitialize returned error : %d\n", GetLastError());
	}

	DWORD64  dwDisplacement = 0;

	static char sym_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)sym_buffer;

	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;

	if (SymFromAddr(GetCurrentProcess(), (DWORD64)addr, &dwDisplacement, pSymbol))
	{
		strncpy_s(buffer, buffer_len, pSymbol->Name, pSymbol->NameLen);
		return;
	}

	gi.error("SymFromAddr returned error : %d\n", GetLastError());
}

static void undecorate_symbol_name(const void *sym, char *name_buf, size_t name_buf_len)
{
	SymbolName(sym, name_buf, name_buf_len);
	
	if (Q_strncasecmp(name_buf, "ILT+", 4) == 0)
	{
		int unused;
		char sym_buf[64];
		sscanf_s(name_buf, "ILT+%d(_%[A-Za-z0-9_]s)", &unused, sym_buf, sizeof(sym_buf));
		Q_snprintf(name_buf, name_buf_len, "%s", sym_buf);
	}
}




struct
{
	char			md2_frame_names[MD2_MAX_FRAMES][17];
	int				md2_num_frames;
} script_frame_temp;

static void load_md2_frames(const char *model_filename)
{
	if (model_filename)
	{
		qhandle_t f;

		gi.FS_FOpenFile(model_filename, &f, FS_MODE_READ);

		if (!f)
			gi.error("Can't find model %s", model_filename);

		dmd2header_t header;

		gi.FS_Read(&header, sizeof(header), f);

		script_frame_temp.md2_num_frames = LittleLong(header.num_frames);
		size_t ofs_frames = LittleLong(header.ofs_frames);
		size_t framesize = LittleLong(header.framesize);

		for (size_t i = 0; i < script_frame_temp.md2_num_frames; i++)
		{
			gi.FS_Seek(f, ofs_frames + (framesize * i) + q_offsetof(dmd2frame_t, name));
			gi.FS_Read(script_frame_temp.md2_frame_names[i], sizeof(script_frame_temp.md2_frame_names[i]) - 1, f);
		}

		gi.FS_FCloseFile(f);
	}
	else
		script_frame_temp.md2_num_frames = MD2_MAX_FRAMES;
}

static bool M_ParseMonsterFrame(cJSON *frame, int *out_frame)
{
	if (!cJSON_IsNumber(frame) && !cJSON_IsString(frame))
		return false;

	if (cJSON_IsNumber(frame))
	{
		*out_frame = frame->valueint;
		return true;
	}

	const char *name = cJSON_GetStringValue(frame);

	for (int i = script_frame_temp.md2_num_frames - 1; i >= 0; i--)
	{
		const char *fn = script_frame_temp.md2_frame_names[i];

		if (!fn[0])
			gi.error("No such frame %s", name);
		else if (Q_strncasecmp(name, fn, 16) == 0)
		{
			*out_frame = i;
			return true;
		}
	}

	return false;
}

typedef void (*mthink_t) (edict_t *);

static bool M_ParseMonsterFunc(cJSON *func, const mevent_t *events, mthink_t *out_func)
{
	*out_func = NULL;

	// nothing
	if (!func)
		return true;

	const char *name = cJSON_GetStringValue(func);

	if (Q_stricmp(name, "none") == 0)
		return true;

	for (const mevent_t *event = events; event->name; event++)
	{
		if (Q_stricmp(event->name, name) == 0)
		{
			*out_func = (mthink_t)event->func;
			return true;
		}
	}

	return false;
}

static bool M_ParseMonsterAI(cJSON *ai, const mevent_t *events, mai_func_t *out_func)
{
#define AIFUNC(f) { #f, ai_ ## f }
	static const struct
	{
		const char *name;
		const mai_func_t func;
	} funcs[] = {
		{ "none", NULL },
		AIFUNC(stand),
		AIFUNC(move),
		AIFUNC(walk),
		AIFUNC(turn),
		AIFUNC(run),
		AIFUNC(charge)
	};

	static const size_t funcs_num = q_countof(funcs);

	const char *string_value = cJSON_GetStringValue(ai);

	if (!string_value)
		return false;

	// check built-in
	for (size_t i = 0; i < funcs_num; i++)
	{
		if (Q_stricmp(funcs[i].name, string_value) == 0)
		{
			*out_func = funcs[i].func;
			return true;
		}
	}

	// check events
	for (const mevent_t *event = events; event->name; event++)
	{
		if (Q_stricmp(event->name, string_value) == 0 ||
			Q_stricmp(event->name, va("ai_%s", string_value)) == 0)
		{
			*out_func = (mai_func_t)event->func;
			return true;
		}
	}

	return false;
}

void M_ParseMonsterScript(const char *script_filename, const char *model_filename, const mevent_t *events, mscript_t *script)
{
	const char *err_str = NULL;
	char *file_data;

	gi.FS_LoadFile(va("%s.json", script_filename), &file_data);

	if (!file_data)
		return;

	List_Init(&script->moves);

	for (size_t i = 0; i < MONSTERSCRIPT_HASH_SIZE; i++)
		List_Init(&script->moveHash[i]);

	load_md2_frames(model_filename);

	cJSON *json = cJSON_Parse(file_data);

	gi.FS_FreeFile(file_data);

#define THROW_ERROR(str, ...) \
	{ err_str = va(str, __VA_ARGS__); goto err; }
	
	if (!json)
		THROW_ERROR(cJSON_GetErrorPtr());

	if (!cJSON_HasObjectItem(json, "moves"))
		THROW_ERROR("Monster script missing moves section");

	cJSON *moves = cJSON_GetObjectItem(json, "moves"), *move;

	cJSON_ArrayForEach(move, moves)
	{
		mmove_t *move_out = gi.TagMalloc(sizeof(mmove_t), TAG_GAME);
		const char *name = move->string;

		if (!*name)
			THROW_ERROR("Monster script missing move name");
		if (!cJSON_HasObjectItem(move, "start_frame") || !cJSON_HasObjectItem(move, "end_frame") || !cJSON_HasObjectItem(move, "frames"))
			THROW_ERROR("Monster script missing data for move %s", move_out->name);
		
		Q_strlcpy(move_out->name, name, sizeof(move_out->name));

		if (!M_ParseMonsterFrame(cJSON_GetObjectItem(move, "start_frame"), &move_out->firstframe))
			THROW_ERROR("Monster script missing start frame for move %s", move_out->name);
		if (!M_ParseMonsterFrame(cJSON_GetObjectItem(move, "end_frame"), &move_out->lastframe))
			THROW_ERROR("Monster script missing last frame for move %s", move_out->name);
		if (cJSON_HasObjectItem(move, "post_think") && !M_ParseMonsterFunc(cJSON_GetObjectItem(move, "post_think"), events, &move_out->endfunc))
			THROW_ERROR("Monster script has bad post think for move %s", move_out->name);

		cJSON *frames = cJSON_GetObjectItem(move, "frames"), *frame;

		if (!cJSON_IsArray(frames))
			THROW_ERROR("Monster script has bad frames for move %s", move_out->name);

		const size_t frames_count = cJSON_GetArraySize(frames);

		if (frames_count != abs(move_out->lastframe - move_out->firstframe) + 1)
			THROW_ERROR("Monster script has invalid frames for move %s", move_out->name);

		mframe_t *cur_frame = move_out->frame = gi.TagMalloc(sizeof(mframe_t) * frames_count, TAG_GAME);

		cJSON_ArrayForEach(frame, frames)
		{
			if (!cJSON_IsObject(frame))
				THROW_ERROR("Monster script move frame isn't frame for move %s", move_out->name);

			const size_t frame_id = cur_frame - move_out->frame;

			if (cJSON_HasObjectItem(frame, "inherit"))
			{
				const cJSON *inherit = cJSON_GetObjectItem(frame, "inherit");

				if (!cJSON_IsNumber(inherit) || inherit->valueint < 0 || inherit->valueint >= frame_id)
					THROW_ERROR("Monster script move frame %u has bad inherit for move %s", frame_id, move_out->name);

				memcpy(cur_frame, &move_out->frame[inherit->valueint], sizeof(mframe_t));
			}

			if (cJSON_HasObjectItem(frame, "ai"))
			{
				cJSON *ai = cJSON_GetObjectItem(frame, "ai");

				if (!M_ParseMonsterAI(ai, events, &cur_frame->aifunc))
					THROW_ERROR("Monster script move frame %u has bad ai for move %s", frame_id, move_out->name);
			}

			if (cJSON_HasObjectItem(frame, "dist"))
			{
				const cJSON *dist = cJSON_GetObjectItem(frame, "dist");

				if (!cJSON_IsNumber(dist))
					THROW_ERROR("Monster script move frame %u has bad dist for move %s", frame_id, move_out->name);

				cur_frame->dist = dist->valuedouble;
			}

			if (cJSON_HasObjectItem(frame, "event") && !M_ParseMonsterFunc(cJSON_GetObjectItem(frame, "event"), events, &cur_frame->thinkfunc))
				THROW_ERROR("Monster script move frame %u has bad event think for move %s", frame_id, move_out->name);

			if (cJSON_HasObjectItem(frame, "model_frame"))
			{
				const cJSON *model_frame = cJSON_GetObjectItem(frame, "model_frame");

				if (!cJSON_IsNumber(model_frame))
					THROW_ERROR("Monster script move frame %u has bad model_frame for move %s", frame_id, move_out->name);

				cur_frame->real_frame = model_frame->valueint;
			}

			cur_frame++;
		}

		List_Append(&script->moves, &move_out->listEntry);
		List_Append(&script->moveHash[Com_HashString(name, MONSTERSCRIPT_HASH_SIZE)], &move_out->hashEntry);
	}

	cJSON_Delete(json);
	script->initialized = true;
	return;

err:
	if (json)
		cJSON_Delete(json);

	gi.error(err_str);
}

const mmove_t *M_GetMonsterMove(const mscript_t *script, const char *name)
{
	mmove_t *move;
	size_t hash = Com_HashString(name, MONSTERSCRIPT_HASH_SIZE);

	FOR_EACH_MOVE_HASH(script, move, hash)
	{
		if (!strcmp(name, move->name))
			return move;
	}

	gi.error("No such animation %s", name);
	return NULL;
}



static size_t frame_score(const mframe_t *src, const mframe_t *other)
{
	size_t score = 1;

	if (fabs(other->dist - src->dist) < 0.05)
		score++;
	if (other->thinkfunc == src->thinkfunc)
		score++;

	return score;
}

const mframe_t *M_FindDittoFrame(const mmove_t *move, const mframe_t *frame, size_t i)
{
	if (i <= 0)
		return NULL;

	const mframe_t *best_frame = NULL;
	size_t last_score;

	// search from index 0 and on to find a matching frame
	for (size_t z = 0; z < i; z++)
	{
		const mframe_t *check_frame = &move->frame[z];

		if (best_frame == check_frame ||
			check_frame->aifunc != frame->aifunc)
			continue;

		size_t score = frame_score(frame, check_frame);

		if (!best_frame || last_score < score)
		{
			best_frame = check_frame;
			last_score = score;
		}
	}

	return best_frame;
}

#define JSON_SPACE "\t"

static void M_AddToEventsList(void **events, size_t *num_events, void *event)
{
	for (const void **evt = events; ; evt++)
	{
		if (*evt == event)
			break;
		else if (*evt && *evt != event)
			continue;

		*evt = event;
		*num_events++;
		break;
	}
}

void M_ConvertFramesToMonsterScript(const char *script_filename, const char *model_filename, const mmove_t **moves)
{
	char buff[MAX_QPATH];

	load_md2_frames(model_filename);

	Q_snprintf(buff, sizeof(buff), "%s.json", script_filename);

	qhandle_t f;

	gi.FS_FOpenFile(buff, &f, FS_MODE_WRITE);

	gi.FS_FPrintf(f, "{\n" JSON_SPACE "\"moves\": {\n");

#define MAX_EVENTS 64
	void *events[MAX_EVENTS];
	size_t num_events = 0;

	memset(events, 0, sizeof(events));

	for (const mmove_t **move_ptr = moves; *move_ptr; move_ptr++)
	{
		const mmove_t *move = *move_ptr;

		char name_buf[64];
		SymbolName(move, name_buf, sizeof(name_buf));
		const char *name_skip = strchr(name_buf, '_') + 1;

		if (Q_strncasecmp(name_skip, "move_", 5) == 0)
			name_skip += 5;

		gi.FS_FPrintf(f, JSON_SPACE JSON_SPACE "\"%s\": {\n", name_skip);

		if (*script_frame_temp.md2_frame_names[move->firstframe] && *script_frame_temp.md2_frame_names[move->lastframe])
		{
			gi.FS_FPrintf(f, JSON_SPACE JSON_SPACE JSON_SPACE "\"start_frame\": \"%s\",\n", script_frame_temp.md2_frame_names[move->firstframe]);
			gi.FS_FPrintf(f, JSON_SPACE JSON_SPACE JSON_SPACE "\"end_frame\": \"%s\",\n", script_frame_temp.md2_frame_names[move->lastframe]);
		}
		else
		{
			gi.FS_FPrintf(f, JSON_SPACE JSON_SPACE JSON_SPACE "\"start_frame\": %d,\n", move->firstframe);
			gi.FS_FPrintf(f, JSON_SPACE JSON_SPACE JSON_SPACE "\"end_frame\": %d,\n", move->lastframe);
		}

		gi.FS_FPrintf(f, JSON_SPACE JSON_SPACE JSON_SPACE "\"frames\": [\n");

		const size_t total_frames = abs(move->lastframe - move->firstframe) + 1;
		char func_buffer[64];

		for (size_t i = 0; i < total_frames; i++)
		{
			cJSON *frameJson = cJSON_CreateObject();
			const mframe_t *frame = &move->frame[i];
			const mframe_t *ditto = M_FindDittoFrame(move, frame, i);
			bool needsComma = false;

			gi.FS_FPrintf(f, JSON_SPACE JSON_SPACE JSON_SPACE JSON_SPACE "{ ");

			if (ditto)
			{
				gi.FS_FPrintf(f, "\"inherit\": %d", ditto - move->frame);
				needsComma = true;
			}

			if ((!ditto && frame->aifunc) || (ditto && frame->aifunc != ditto->aifunc))
			{
				if (needsComma)
					gi.FS_FPrintf(f, ", ");

				if (frame->aifunc)
				{
					undecorate_symbol_name(frame->aifunc, func_buffer, sizeof(func_buffer));
					cJSON_AddStringToObject(frameJson, "ai", func_buffer + 3);
					gi.FS_FPrintf(f, "\"ai\": \"%s\"", func_buffer + 3);
				}
				else
					gi.FS_FPrintf(f, "\"ai\": \"none\"");

				needsComma = true;
			}

			if ((!ditto && frame->dist) || (ditto && frame->dist != ditto->dist))
			{
				if (needsComma)
					gi.FS_FPrintf(f, ", ");

				gi.FS_FPrintf(f, "\"dist\": %g", frame->dist);
				needsComma = true;
			}

			if ((!ditto && frame->thinkfunc) || (ditto && frame->thinkfunc != ditto->thinkfunc))
			{
				if (needsComma)
					gi.FS_FPrintf(f, ", ");

				if (frame->thinkfunc)
				{
					undecorate_symbol_name(frame->thinkfunc, func_buffer, sizeof(func_buffer));
					cJSON_AddStringToObject(frameJson, "event", func_buffer);
					gi.FS_FPrintf(f, "\"event\": \"%s\"", func_buffer);

					M_AddToEventsList(events, &num_events, frame->thinkfunc);
				}
				else
					gi.FS_FPrintf(f, "\"event\": \"none\"");

				needsComma = true;
			}

			gi.FS_FPrintf(f, " }");

			if (i != total_frames - 1)
				gi.FS_FPrintf(f, ",");

			gi.FS_FPrintf(f, "\n");
		}

		gi.FS_FPrintf(f, JSON_SPACE JSON_SPACE JSON_SPACE "]");

		if (move->endfunc)
		{
			undecorate_symbol_name(move->endfunc, func_buffer, sizeof(func_buffer));
			gi.FS_FPrintf(f, ",\n" JSON_SPACE JSON_SPACE JSON_SPACE "\"post_think\": \"%s\"", func_buffer);

			M_AddToEventsList(events, &num_events, move->endfunc);
		}

		gi.FS_FPrintf(f, "\n" JSON_SPACE JSON_SPACE "}");

		if (*(move_ptr + 1) != NULL)
			gi.FS_FPrintf(f, ", ");

		gi.FS_FPrintf(f, "\n");
	}

	gi.FS_FPrintf(f, JSON_SPACE "}\n}");

	gi.FS_FCloseFile(f);

	if (num_events)
	{
		char file_buf[64];
		Q_snprintf(file_buf, sizeof(file_buf), "%s.evt", script_filename);

		gi.FS_FOpenFile(file_buf, &f, FS_MODE_WRITE);

		gi.FS_FPrintf(f, "static const mevent_t events[] =\n{\n");

		for (const void **evt = events; *evt; evt++)
		{
			char sym_buf[64];
			undecorate_symbol_name(*evt, sym_buf, sizeof(sym_buf));
			gi.FS_FPrintf(f, "\tEVENT_FUNC(%s),\n", sym_buf);
		}

		gi.FS_FPrintf(f, "\tNULL\n};");

		gi.FS_FCloseFile(f);
	}
}
