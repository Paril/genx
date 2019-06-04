#pragma once

#include "g_local.h"

/*
A frames list consists of any number of single animation frames, followed by a \n or \0

A single animation frame consists a full frame, a ditto or an index lookup.

A full frame starts with a string deducing the type, followed by any number of arguments prefixed with a hyphen.
Animation type may be one of stand, move, walk, turn, run, charge
	<type> [-key value...]

A ditto uses the last frame defined, consists of a ^, and is followed by any number of changed arguments prefixed with a hyphen.

An index lookup uses a specific frame already defined, consists of a @ followed by the zero-index in the frames list which must be one that has already been seen, followed by any number of changed arguments prefixed with a hyphen.

Allowed arguments:
	Inherited arguments:
		speed (float) = movement speed this frame
		event ("none" or index into event list) = event to play this frame
	Per-frame arguments:
		repeat (int > 0) = how many frames this frame entry will fill
*/

typedef void (*mevent_func_t) (edict_t *self);

typedef struct
{
	const char			*name;
	const mevent_func_t	func;
} mevent_t;

#define EVENT_FUNC(func) \
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