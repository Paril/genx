#ifdef ENABLE_COOP

#include "g_local.h"

typedef enum
{
	WAVE_INIT,
	WAVE_SPAWNING,
	WAVE_COOLDOWN
} wave_state_e;

typedef struct
{
	uint32_t			wave_id;
	float				chance;
} wave_monster_entry_chances_t;

typedef struct
{
	entitytype_e					entity;
	const char						*chancesString;

	wave_monster_entry_chances_t	*chances;
	uint32_t						count;
} wave_monster_entry_t;

wave_monster_entry_t wave_monster_entries[] =
{
	{ ET_MONSTER_SOLDIER_LIGHT,	"1	50%		5	25%" },
	{ ET_MONSTER_SOLDIER,		"1	5%		5	50%" },
	{ ET_MONSTER_SOLDIER_SS,	"1	2%		2	20%	5	80%" },
	{ ET_Q1_MONSTER_ARMY,		"1	5%		3	30%	5	40%" },
	{ ET_DOOM_MONSTER_POSS,		"1	2%		3	40%	5	50%" },
	{ ET_DOOM_MONSTER_SPOS,		"1	8%		3	35%	5	50%" },
	{ ET_MONSTER_INFANTRY,		"2	1%		4	12%	7	35%" },
	{ ET_Q1_MONSTER_DOG,		"1	25%		2	35%	5	20%" },
	{ ET_DOOM_MONSTER_TROO,		"4	15%" },
	{ ET_DOOM_MONSTER_SKUL,		"2	12%" },
	{ ET_Q1_MONSTER_WIZARD,		"4	18%" },
	{ ET_Q1_MONSTER_OGRE,		"3	20%" },
	{ ET_Q1_MONSTER_KNIGHT,		"2	35%" },
	{ ET_Q1_MONSTER_ZOMBIE,		"5	35%" },
	{ ET_Q1_MONSTER_DEMON,		"5	15%		7	35%" }
};

const uint32_t num_wave_monster_entries = q_countof(wave_monster_entries);

static void Init_WaveMonsterEntries()
{
	wave_monster_entry_t *entry = &wave_monster_entries[0];

	for (uint32_t i = 0; i < num_wave_monster_entries; ++i, ++entry)
	{
		// count %s
		uint32_t percents = 0;

		for (uint32_t x = 0; x < strlen(entry->chancesString); ++x)
		{
			if (entry->chancesString[x] == '%')
				percents++;
		}

		entry->chances = (wave_monster_entry_chances_t *)Z_TagMalloc(sizeof(*entry->chances) * percents, TAG_GAME);
		entry->count = percents;
		const char *buffer = entry->chancesString;

		for (uint32_t x = 0; x < percents; ++x)
		{
			wave_monster_entry_chances_t *chance = &entry->chances[x];
			char *token = COM_Parse(&buffer);
			chance->wave_id = atoi(token);
			token = COM_Parse(&buffer);
			chance->chance = atof(token);
		}
	}
}

typedef struct
{
	entitytype_e		id;
	float				chance;
	float				probability;
} monster_wave_type_t;

#define MONSTER_START(type) ET_MARKER_##type##_MONSTERS_START
#define MONSTER_END(type) ET_MARKER_##type##_MONSTERS_END
#define MONSTER_SIZE(type) (MONSTER_END(type) - MONSTER_START(type)) + 1

#define total_monsters_in_table MONSTER_SIZE(Q2) + MONSTER_SIZE(Q1) + MONSTER_SIZE(DOOM)

struct
{
	wave_state_e		wave_state;
	gtime_t				next_monster_spawn;

	uint32_t			wave_number;
	uint32_t			num_monsters_to_spawn;
	uint32_t			num_monsters_left;
	list_t				monsters_list;

	uint32_t			num_spawnable_monsters;
	monster_wave_type_t	spawnable_monsters[total_monsters_in_table];
} wave_globals;

bool Wave_PickTheTwoViableWaveChances(wave_monster_entry_chances_t *chances, uint32_t count, uint32_t wave_id, wave_monster_entry_chances_t **closest_lower_wave, wave_monster_entry_chances_t **closest_next_wave)
{
	*closest_lower_wave = *closest_next_wave = NULL;

	for (uint32_t i = 0; i < count; ++i, ++chances)
	{
		if (chances->wave_id > wave_id)
		{
			// nothing available yet, need at least
			// one <= to do this
			if (i == 0)
				return false;

			*closest_next_wave = chances;
			break;
		}
	}

	*closest_lower_wave = chances - 1;
	return true;
}

float Wave_GetChanceValue(uint32_t wave_id, wave_monster_entry_chances_t *closest_lower_wave, wave_monster_entry_chances_t *closest_next_wave)
{
	if (!closest_lower_wave)
		return 0;
	else if (!closest_next_wave || closest_lower_wave->chance == closest_next_wave->chance)
		return closest_lower_wave->chance;

	uint32_t low_wave = closest_lower_wave->wave_id;
	uint32_t next_wave = closest_next_wave->wave_id;
	float distance_to_next = (float)(wave_id - low_wave) / (float)(next_wave - low_wave);
	return LerpFloat(closest_lower_wave->chance, closest_next_wave->chance, distance_to_next);
}

int CompareSpawnable(const void *a, const void *b)
{
	const monster_wave_type_t *aa = (const monster_wave_type_t *)a;
	const monster_wave_type_t *bb = (const monster_wave_type_t *)b;

	if (aa->chance < bb->chance)
		return -1;
	else if (aa->chance > bb->chance)
		return 1;

	return 0;
}

void Wave_CalculateSpawnableMonsters(uint32_t wave_id)
{
	wave_monster_entry_t *wave_entry = wave_monster_entries;
	monster_wave_type_t *spawnable_ptr = wave_globals.spawnable_monsters;
	float total_chance = 0;

	for (uint32_t i = 0; i < num_wave_monster_entries; ++i, ++wave_entry)
	{
		wave_monster_entry_chances_t *lower, *next;

		if (!Wave_PickTheTwoViableWaveChances(wave_entry->chances, wave_entry->count, wave_id, &lower, &next))
			continue;

		spawnable_ptr->id = wave_entry->entity;
		total_chance += (spawnable_ptr->chance = Wave_GetChanceValue(wave_id, lower, next));
		++spawnable_ptr;
	}

	wave_globals.num_spawnable_monsters = spawnable_ptr - wave_globals.spawnable_monsters;
	float probability = 0;
	qsort(wave_globals.spawnable_monsters, wave_globals.num_spawnable_monsters, sizeof(wave_globals.spawnable_monsters[0]), CompareSpawnable);
	static char buffer[1024];
	buffer[0] = 0;
	Q_snprintf(buffer, sizeof(buffer), "Monsters in this wave: ");

	for (uint32_t i = 0; i < wave_globals.num_spawnable_monsters; ++i)
	{
		wave_globals.spawnable_monsters[i].chance /= total_chance;
		wave_globals.spawnable_monsters[i].probability = (probability += wave_globals.spawnable_monsters[i].chance);
		Q_snprintf(buffer, sizeof(buffer), "%s%s%i: %.2f%%", buffer, i == 0 ? "" : ", ", wave_globals.spawnable_monsters[i].id, (float)(wave_globals.spawnable_monsters[i].chance * 100));
	}

	gi.bprintf(PRINT_HIGH, "%s", buffer);
}

entitytype_e Wave_PickSpawnableMonster()
{
	float r = random();

	for (size_t i = 0; i < wave_globals.num_spawnable_monsters; ++i)
	{
		if (r < wave_globals.spawnable_monsters[i].probability)
			return wave_globals.spawnable_monsters[i].id;
	}

	Com_Error(ERR_FATAL, "Couldn't pick monster");
}

bool ED_CallSpawnByType(edict_t *ent, entitytype_e type);

static q_soundhandle wave_noise;

void Wave_Init()
{
	memset(&wave_globals, 0, sizeof(wave_globals));
	List_Init(&wave_globals.monsters_list);
	Init_WaveMonsterEntries();
}

void Wave_Precache()
{
	wave_noise = gi.soundindex("world/x_alarm.wav");
}

edict_t *SelectMonsterSpawnPoint(void)
{
	edict_t *spot;
	int     count = 0;
	int     selection;
	spot = NULL;

	while ((spot = G_FindByType(spot, ET_INFO_PLAYER_DEATHMATCH)) != NULL)
		count++;

	if (!count)
		return NULL;

	selection = rand() % count;
	spot = NULL;

	do
	{
		spot = G_FindByType(spot, ET_INFO_PLAYER_DEATHMATCH);
	}
	while (selection--);

	return spot;
}

void Wave_Begin()
{
	gi.positioned_sound(vec3_origin, world, CHAN_AUTO, wave_noise, 1, ATTN_NONE, 0);
	wave_globals.wave_number++;
	Wave_CalculateSpawnableMonsters(wave_globals.wave_number);

	for (int i = 0; i < game.maxclients; ++i)
	{
		edict_t *player = &g_edicts[i + 1];

		if (player->inuse && player->client->pers.connected)
			gi.centerprintf(player, "Wave %u\n", wave_globals.wave_number);
	}

	wave_globals.next_monster_spawn = level.time + 3000;
	wave_globals.num_monsters_to_spawn = 20;
	wave_globals.wave_state = WAVE_SPAWNING;
	List_Init(&wave_globals.monsters_list);
}

void Wave_MonsterDead(edict_t *self)
{
	wave_globals.num_monsters_left--;
}

void Wave_MonsterFreed(edict_t *self)
{
	List_Remove(&self->monsterinfo.wave_entry);
}

static edict_t *last_enemy = NULL;

void Wave_SpawnMonster()
{
	edict_t *point = SelectMonsterSpawnPoint();
	edict_t *s = G_Spawn();
	VectorCopy(point->s.origin, s->s.origin);
	s->s.origin[2] += 16;
	VectorCopy(point->s.angles, s->s.angles);
	ED_CallSpawnByType(s, Wave_PickSpawnableMonster());
	gi.linkentity(s);

	if (!last_enemy)
		last_enemy = &g_edicts[0];

	edict_t *first_enemy = last_enemy;

	do
	{
		last_enemy++;

		if (last_enemy > g_edicts + maxclients->integer)
			last_enemy = &g_edicts[1];

		if (last_enemy->inuse && last_enemy->health > 0 && !(last_enemy->flags & FL_NOTARGET))
		{
			s->enemy = last_enemy;
			break;
		}
	} while (last_enemy != first_enemy);

	if (s->enemy)
		FoundTarget(s);

	List_Append(&wave_globals.monsters_list, &s->monsterinfo.wave_entry);
	wave_globals.num_monsters_to_spawn--;
	wave_globals.num_monsters_left++;
}

void Wave_Frame()
{
	switch (wave_globals.wave_state)
	{
		case WAVE_INIT:
			if (wave_globals.next_monster_spawn < level.time)
			{
				wave_globals.next_monster_spawn = level.time + 10000;

				for (int i = 0; i < game.maxclients; ++i)
				{
					edict_t *player = &g_edicts[i + 1];

					if (player->inuse && player->client->pers.connected)
						gi.centerprintf(player, "Type \"wave_start\" to start the game.\n");
				}
			}

			break;

		case WAVE_SPAWNING:
			if (wave_globals.next_monster_spawn < level.time)
			{
				if (wave_globals.num_monsters_to_spawn)
				{
					Wave_SpawnMonster();
					wave_globals.next_monster_spawn = level.time + 100;
				}
				else if (!wave_globals.num_monsters_left)
				{
					wave_globals.wave_state = WAVE_COOLDOWN;
					wave_globals.next_monster_spawn = level.time + 10000;

					for (int i = 0; i < game.maxclients; ++i)
					{
						edict_t *player = &g_edicts[i + 1];

						if (player->inuse && player->client->pers.connected)
							gi.centerprintf(player, "Wave complete! 10 second cooldown.\n");
					}

					edict_t *cur, *next;
					LIST_FOR_EACH_SAFE(edict_t, cur, next, &wave_globals.monsters_list, monsterinfo.wave_entry)
					{
						G_FreeEdict(cur);
					}
				}
			}

			break;

		case WAVE_COOLDOWN:
			if (wave_globals.next_monster_spawn < level.time)
				Wave_Begin();

			break;
	}
}

bool Wave_GameReady()
{
	return wave_globals.wave_state > WAVE_INIT;
}

bool Wave_Commands(edict_t *ent)
{
	char    *cmd;

	if (!ent->client)
		return false;     // not fully in game yet

	cmd = Cmd_Argv(0);

	if (!strcmp(cmd, "wave_start"))
	{
		if (wave_globals.wave_state == WAVE_INIT)
		{
			wave_globals.wave_state = WAVE_COOLDOWN;
			wave_globals.next_monster_spawn = level.time + 10000;

			for (int i = 0; i < game.maxclients; ++i)
			{
				edict_t *player = &g_edicts[i + 1];

				if (player->inuse && player->client->pers.connected)
				{
					gi.centerprintf(player, "Waves will start in 10 seconds.\n");
					respawn(player);
				}
			}
		}

		return true;
	}

	return false;
}

#endif