#pragma once

#include "g_local.h"

typedef struct
{
	const char			*name;
	const void			*func;
} mevent_t;

#define EVENT_FUNC(func) \
	{ #func, func }

#define AI_FUNC(func) \
	{ #func, func }

#define	MONSTERSCRIPT_HASH_SIZE		16

typedef struct
{
	bool	initialized;
	list_t	moves;
	list_t	moveHash[MONSTERSCRIPT_HASH_SIZE];
} mscript_t;

#define FOR_EACH_MOVE_HASH(script, move, hash) \
	LIST_FOR_EACH(mmove_t, move, &script->moveHash[hash], hashEntry)
#define FOR_EACH_MOVE(script, move) \
	LIST_FOR_EACH(mmove_t, move, &script->moves, listEntry)

void M_ParseMonsterScript(const char *script_filename, const char *model_filename, const mevent_t *events, mscript_t *script);

const mmove_t *M_GetMonsterMove(const mscript_t *script, const char *name);

void M_ConvertFramesToMonsterScript(const char *script_filename, const char *model_filename, const mmove_t **moves);