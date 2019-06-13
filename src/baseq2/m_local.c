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
	// only set at start
	mmove_t			*move;
	mframe_t		*current_frame;
	const mevent_t	*events;
	char			*script_ptr;
	char			md2_frame_names[MD2_MAX_FRAMES][17];
	int				md2_num_frames;

	// inherited, per-frame
	mai_func_t		move_type;
	mevent_func_t	event;
	float			speed;
} script_frame_temp;

static const mevent_func_t parse_get_event_func(const char *event_name)
{
	if (Q_stricmp(event_name, "none") == 0)
		return NULL;

	for (const mevent_t *event = script_frame_temp.events; event->name; event++)
	{
		if (Q_stricmp(event->name, event_name) == 0)
			return event->func;
	}

	gi.error("Missing event func %s", event_name);
	return NULL;
}

// skip the specified chars
static void parse_script_skip(const char *chars)
{
	while (*Q_strchrnul(chars, *script_frame_temp.script_ptr))
		script_frame_temp.script_ptr++;
}

typedef bool (*script_char_matcher_func) (char c);

static const char *parse_script_token(script_char_matcher_func matcher, bool strict, size_t *len)
{
	// skip whitespace
	parse_script_skip(" \t\r");

	if (strict && !matcher(*script_frame_temp.script_ptr))
		return NULL;

	const char *start = script_frame_temp.script_ptr;

	while (*script_frame_temp.script_ptr && matcher(*script_frame_temp.script_ptr))
		script_frame_temp.script_ptr++;

	if (len)
		*len = script_frame_temp.script_ptr - start;

	return start;
}

static bool parse_is_arg(char c)
{
	return c && c != '+' && c != '\r' && c != '\n';
}

static void parse_frame_script(void)
{
	size_t repeat = 1;

	// figure out what we're parsing
	switch (*script_frame_temp.script_ptr)
	{
		// parse frame type
		default:
		{
			size_t type_len;
			const char *type_start = parse_script_token(Q_isalpha, true, &type_len);

			if (!type_start)
				gi.error("Animation script invalid type");
	
#define TYPE_IS(x)		Q_stricmpn(type_start, x, type_len) == 0

			if (TYPE_IS("stand"))
				script_frame_temp.move_type = ai_stand;
			else if (TYPE_IS("move"))
				script_frame_temp.move_type = ai_move;
			else if (TYPE_IS("walk"))
				script_frame_temp.move_type = ai_walk;
			else if (TYPE_IS("turn"))
				script_frame_temp.move_type = ai_turn;
			else if (TYPE_IS("run"))
				script_frame_temp.move_type = ai_run;
			else if (TYPE_IS("charge"))
				script_frame_temp.move_type = ai_charge;
			else if (TYPE_IS("none"))
				script_frame_temp.move_type = NULL;
			else
				gi.error("Animation script bad type");

			script_frame_temp.speed = 0;
			script_frame_temp.event = NULL;
		}
			break;
		case '^':
			script_frame_temp.script_ptr++;

			if (script_frame_temp.current_frame == script_frame_temp.move->frame)
				gi.error("Animation script ^ without prior frame");

			break;
		case '@':
		{
			script_frame_temp.script_ptr++;

			size_t digit_len;
			const char *digit_start = parse_script_token(Q_isdigit, true, &digit_len);

			if (!digit_start)
				gi.error("Animation script @ without index number");

			char digit_buffer[10] = { 0 };
			strncpy_s(digit_buffer, sizeof(digit_buffer) - 1, digit_start, digit_len);
			uint32_t index = atoi(digit_buffer);

			if (index >= (script_frame_temp.move->frame - script_frame_temp.current_frame))
				gi.error("Animation script @ for frame not parsed yet");

			script_frame_temp.move_type = script_frame_temp.move->frame[index].aifunc;
			script_frame_temp.speed = script_frame_temp.move->frame[index].dist;
			script_frame_temp.event = script_frame_temp.move->frame[index].thinkfunc;
		}
			break;
	}
	
	// skip whitespace
	parse_script_skip(" \t\r");

	// parse arguments
	while (*script_frame_temp.script_ptr != '\n' && *script_frame_temp.script_ptr != '\0' && *script_frame_temp.script_ptr != '}')
	{
		if (*script_frame_temp.script_ptr != '+')
			gi.error("Animation script expected +");

		script_frame_temp.script_ptr++;

		size_t arg_len;
		const char *arg_start = parse_script_token(Q_isalpha, true, &arg_len);

		// next char has to be alpha
		if (!arg_start)
			gi.error("Animation script frame argument without proper name");

		size_t val_len;
		const char *val_start = parse_script_token(parse_is_arg, true, &val_len);

		// trim end spaces
		while (val_len && Q_isspace(val_start[val_len - 1]))
			val_len--;

		// next char has to be valid
		if (!val_len || !val_start)
			gi.error("Animation script frame argument without proper value");

#define ARG_IS(x)		Q_stricmpn(arg_start, x, arg_len) == 0

		if (ARG_IS("speed"))
		{
			char float_buffer[32] = { 0 };
			strncpy_s(float_buffer, sizeof(float_buffer) - 1, val_start, val_len);
			script_frame_temp.speed = atof(float_buffer);
		}
		else if (ARG_IS("event"))
		{
			char str_buffer[64] = { 0 };
			strncpy_s(str_buffer, sizeof(str_buffer) - 1, val_start, val_len);
			script_frame_temp.event = parse_get_event_func(str_buffer);
		}
		else if (ARG_IS("repeat"))
		{
			char digit_buffer[10] = { 0 };
			strncpy_s(digit_buffer, sizeof(digit_buffer) - 1, val_start, val_len);
			repeat = atoi(digit_buffer);
		}
		else
			gi.error("Animation script uses invalid argument");

		// skip whitespace
		parse_script_skip(" \t\r");
	}

	// once we reach a \n \0 or }, setup frame and return
	for (int i = 0; i < repeat; i++)
	{
		if (script_frame_temp.current_frame > script_frame_temp.move->frame + (abs(script_frame_temp.move->lastframe - script_frame_temp.move->firstframe)))
			gi.error("Animation script defines too many frames");

		script_frame_temp.current_frame->aifunc = script_frame_temp.move_type;
		script_frame_temp.current_frame->dist = script_frame_temp.speed;
		script_frame_temp.current_frame->thinkfunc = script_frame_temp.event;
		script_frame_temp.current_frame++;
	}
	
	// skip whitespace
	parse_script_skip(" \t\r\n");
}

static void parse_anim_script(void)
{
	// skip whitespaces
	parse_script_skip(" \t\r\n");

	while (*script_frame_temp.script_ptr != '\0' && *script_frame_temp.script_ptr != '}')
		parse_frame_script();

	if (script_frame_temp.current_frame != script_frame_temp.move->frame + (abs(script_frame_temp.move->lastframe - script_frame_temp.move->firstframe) + 1))
		gi.error("Animation script missing frames");

	// skip whitespace
	parse_script_skip(" \t\r\n");

	if (*script_frame_temp.script_ptr != '}')
		gi.error("Expected } after end of frames");
}

static mmove_t *M_ParseAnimationScript(const char *script, const mevent_t *events, uint32_t frame_start, uint32_t frame_end, mevent_func_t finished_func)
{
	memset(&script_frame_temp, 0, sizeof(script_frame_temp));

	mmove_t *move = gi.TagMalloc(sizeof(mmove_t), TAG_GAME);

	move->frame = gi.TagMalloc(sizeof(mframe_t) * (frame_end - frame_start + 1), TAG_GAME);
	move->firstframe = frame_start;
	move->lastframe = frame_end;
	move->endfunc = finished_func;

	script_frame_temp.move = move;
	script_frame_temp.current_frame = move->frame;

	parse_anim_script();

	return move;
}

static bool parse_is_frame(char c)
{
	return Q_isalnum(c) || c == '_' || c == '$';
}

static size_t parse_frame_id(void)
{
	char frame_buffer[20] = { 0 };
	size_t frame_len;
	const char *frame_start = parse_script_token(parse_is_frame, true, &frame_len);
	strncpy_s(frame_buffer, sizeof(frame_buffer) - 1, frame_start, frame_len);

	int frame;

	if (frame_buffer[0] == '$')
	{
		for (size_t i = 0; i < MD2_MAX_FRAMES; i++)
		{
			if (!script_frame_temp.md2_frame_names[i][0])
				gi.error("No such frame %s", frame_buffer);
			else if (Q_strncasecmp(frame_buffer + 1, script_frame_temp.md2_frame_names[i], 16) == 0)
				return i;
		}

		gi.error("No such frame %s", frame_buffer);
	}

	frame = atoi(frame_buffer);

	if (frame > script_frame_temp.md2_num_frames)
		gi.error("No such frame %i", frame);

	return frame;
}

static inline bool parse_is_name(char c)
{
	return Q_isalnum(c) || c == '_';
}

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

static void M_ConvertScriptToJson(const char *script_filename, mscript_t *script)
{
	char buff[MAX_QPATH];

	Q_snprintf(buff, sizeof(buff), "%s.json", script_filename);

	qhandle_t f;

	gi.FS_FOpenFile(buff, &f, FS_MODE_WRITE);

	gi.FS_FPrintf(f, "{\n" JSON_SPACE "\"moves\": {\n");

	mmove_t *move;

	LIST_FOR_EACH(mmove_t, move, &script->moves, listEntry)
	{
		gi.FS_FPrintf(f, JSON_SPACE JSON_SPACE "\"%s\": {\n", move->name);

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
		}
		
		gi.FS_FPrintf(f, "\n" JSON_SPACE JSON_SPACE "}");

		if (!LIST_TERM(LIST_NEXT(mmove_t, move, listEntry), &script->moves, listEntry))
			gi.FS_FPrintf(f, ", ");

		gi.FS_FPrintf(f, "\n");
	}

	gi.FS_FPrintf(f, JSON_SPACE "}\n}");

	gi.FS_FCloseFile(f);
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

	for (size_t i = 0; i < MD2_MAX_FRAMES; i++)
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

	for (const mevent_t *event = script_frame_temp.events; event->name; event++)
	{
		if (Q_stricmp(event->name, name) == 0)
		{
			*out_func = event->func;
			return true;
		}
	}

	return false;
}

static bool M_ParseMonsterAI(cJSON *ai, mai_func_t *out_func)
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

	for (size_t i = 0; i < funcs_num; i++)
	{
		if (Q_stricmp(funcs[i].name, string_value) == 0)
		{
			*out_func = funcs[i].func;
			return true;
		}
	}

	return false;
}

static void M_ParseMonsterScriptJSON(const char *script_filename, const char *model_filename, const mevent_t *events, mscript_t *script)
{
	const char *err_str = NULL;
	char *file_data;

	gi.FS_LoadFile(script_filename, &file_data);

	if (!file_data)
		return;

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

				if (!M_ParseMonsterAI(ai, &cur_frame->aifunc))
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

void M_ParseMonsterScript(const char *script_filename, const char *model_filename, const mevent_t *events, mscript_t *script)
{
	memset(&script_frame_temp, 0, sizeof(script_frame_temp));

	script_frame_temp.events = events;

	load_md2_frames(model_filename);

	List_Init(&script->moves);

	for (size_t i = 0; i < MONSTERSCRIPT_HASH_SIZE; i++)
		List_Init(&script->moveHash[i]);

	char json_test[MAX_QPATH];
	Q_snprintf(json_test, sizeof(json_test), "%s.json", script_filename);

	if (gi.FS_LoadFile(json_test, NULL) > 0)
	{
		M_ParseMonsterScriptJSON(json_test, model_filename, events, script);
		return;
	}

	char *file_data;

	gi.FS_LoadFile(script_filename, &file_data);

	script_frame_temp.script_ptr = file_data;

	while (*script_frame_temp.script_ptr != '\0')
	{
		// skip whitespace
		parse_script_skip(" \t\r\n");

		// parse anim name
		size_t name_len;
		const char *name_start = parse_script_token(parse_is_name, true, &name_len);
		char name_buffer[32] = { 0 };
		strncpy_s(name_buffer, sizeof(name_buffer) - 1, name_start, name_len);

		// parse frames
		size_t	start_frame = parse_frame_id(),
				end_frame = parse_frame_id();

		// parse end func
		size_t end_len;
		const char *end_start = parse_script_token(parse_is_name, false, &end_len);

		mevent_func_t end_func = NULL;

		if (end_len)
		{
			char end_func_buffer[32] = { 0 };
			strncpy_s(end_func_buffer, sizeof(end_func_buffer) - 1, end_start, end_len);
			end_func = parse_get_event_func(end_func_buffer);
		}

		// skip whitespace
		parse_script_skip(" \t\r\n");

		if (*script_frame_temp.script_ptr != '{')
			gi.error("Expected {");

		// skip the {
		script_frame_temp.script_ptr++;

		mmove_t *move = script_frame_temp.move = gi.TagMalloc(sizeof(mmove_t), TAG_GAME);
		move->firstframe = start_frame;
		move->lastframe = end_frame;
		script_frame_temp.current_frame = move->frame = gi.TagMalloc(sizeof(mframe_t) * (abs(end_frame - start_frame) + 1), TAG_GAME);
		move->endfunc = end_func;
		memcpy(move->name, name_buffer, sizeof(move->name));

		List_Append(&script->moves, &move->listEntry);
		uint32_t hash = Com_HashString(name_buffer, MONSTERSCRIPT_HASH_SIZE);
		List_Append(&script->moveHash[hash], &move->hashEntry);

		// skip whitespaces
		parse_script_skip(" \t\r\n");

		while (*script_frame_temp.script_ptr != '}')
			parse_anim_script();

		// skip the }
		script_frame_temp.script_ptr++;

		// skip whitespace
		parse_script_skip(" \t\r\n");
	}

	gi.FS_FreeFile(file_data);
	script_frame_temp.script_ptr = NULL;

	script->initialized = true;

	M_ConvertScriptToJson(script_filename, script);
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

void M_ConvertFramesToMonsterScript(const char *script_filename, const char *model_filename, const mmove_t **moves)
{
	qhandle_t f;

	memset(&script_frame_temp, 0, sizeof(script_frame_temp));
	load_md2_frames(model_filename);

	gi.FS_FOpenFile(script_filename, &f, FS_MODE_WRITE);

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

		gi.FS_FPrintf(f, "%s ", name_skip);

		if (script_frame_temp.md2_frame_names[move->firstframe][0])
			gi.FS_FPrintf(f, "$%s ", script_frame_temp.md2_frame_names[move->firstframe]);
		else
			gi.FS_FPrintf(f, "%d ", move->firstframe);

		if (script_frame_temp.md2_frame_names[move->lastframe][0])
			gi.FS_FPrintf(f, "$%s", script_frame_temp.md2_frame_names[move->lastframe]);
		else
			gi.FS_FPrintf(f, "%d", move->lastframe);

		if (move->endfunc)
		{
			undecorate_symbol_name(move->endfunc, name_buf, sizeof(name_buf));
			gi.FS_FPrintf(f, " %s", name_buf);

			for (const void **evt = events; ; evt++)
			{
				if (*evt == move->endfunc)
					break;
				else if (*evt  && *evt != move->endfunc)
					continue;
				
				*evt = move->endfunc;
				num_events++;
				break;
			}
		}

		gi.FS_FPrintf(f, "\n{\n");

		size_t total_frames = abs(move->lastframe - move->firstframe) + 1;

		for (size_t i = 0; i < total_frames; )
		{
			const mframe_t *ditto_frame = NULL;
			const mframe_t *frame = &move->frame[i];
			size_t num_repeats = 1;

			if (i > 0)
				ditto_frame = M_FindDittoFrame(move, frame, i);

			for (size_t z = i + 1; z < total_frames; z++)
			{
				mframe_t *next_frame = &move->frame[z];

				if (frame->aifunc != next_frame->aifunc || frame->dist != next_frame->dist ||
					frame->thinkfunc != next_frame->thinkfunc)
					break;

				num_repeats++;
			}

			i += num_repeats;

			if (!ditto_frame)
			{
				if (frame->aifunc)
				{
					undecorate_symbol_name(frame->aifunc, name_buf, sizeof(name_buf));
					gi.FS_FPrintf(f, "\t%s", name_buf + 3);
				}
				else
					gi.FS_FPrintf(f, "\tnone", name_buf + 3);
			}
			else
			{
				if (ditto_frame == frame - 1)
					gi.FS_FPrintf(f, "\t^");
				else
					gi.FS_FPrintf(f, "\t@%u", (size_t)(ditto_frame - move->frame));
			}

			if ((!ditto_frame && frame->dist) ||
				(ditto_frame && frame->dist != ditto_frame->dist))
				gi.FS_FPrintf(f, " +speed %g", frame->dist);

			if ((!ditto_frame && frame->thinkfunc) ||
				(ditto_frame && frame->thinkfunc != ditto_frame->thinkfunc))
			{
				if (frame->thinkfunc)
				{
					undecorate_symbol_name(frame->thinkfunc, name_buf, sizeof(name_buf));
					gi.FS_FPrintf(f, " +event %s", name_buf);

					for (const void **evt = events; ; evt++)
					{
						if (*evt == frame->thinkfunc)
							break;
						else if (*evt && *evt != frame->thinkfunc)
							continue;

						*evt = frame->thinkfunc;
						num_events++;
						break;
					}
				}
				else
					gi.FS_FPrintf(f, " +event none");
			}

			if (num_repeats != 1)
				gi.FS_FPrintf(f, " +repeat %i", num_repeats);

			gi.FS_FPrintf(f, "\n");
		}

		gi.FS_FPrintf(f, "}\n\n");
	}

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