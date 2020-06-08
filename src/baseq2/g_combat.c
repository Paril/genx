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
// g_combat.c

#include "g_local.h"

meansOfDeath_t blankMeansOfDeath = { DT_DIRECT, NULL, ET_NULL, GAME_Q2, 0, ITI_NULL, NULL, MD_NONE, 0 };

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
bool CanDamage(edict_t *targ, edict_t *inflictor)
{
	vec3_t  dest;
	trace_t trace;

	// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
	{
		VectorAdd(targ->server.absmin, targ->server.absmax, dest);
		VectorScale(dest, 0.5f, dest);
		trace = gi.trace(inflictor->server.state.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);

		if (trace.fraction == 1.0f)
			return true;

		if (trace.ent == targ)
			return true;

		return false;
	}

	trace = gi.trace(inflictor->server.state.origin, vec3_origin, vec3_origin, targ->server.state.origin, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0f)
		return true;

	VectorCopy(targ->server.state.origin, dest);
	dest[0] += 15.0f;
	dest[1] += 15.0f;
	trace = gi.trace(inflictor->server.state.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0f)
		return true;

	VectorCopy(targ->server.state.origin, dest);
	dest[0] += 15.0f;
	dest[1] -= 15.0f;
	trace = gi.trace(inflictor->server.state.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0f)
		return true;

	VectorCopy(targ->server.state.origin, dest);
	dest[0] -= 15.0f;
	dest[1] += 15.0f;
	trace = gi.trace(inflictor->server.state.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0f)
		return true;

	VectorCopy(targ->server.state.origin, dest);
	dest[0] -= 15.0f;
	dest[1] -= 15.0f;
	trace = gi.trace(inflictor->server.state.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0f)
		return true;

	return false;
}


/*
============
Killed
============
*/
void ClientObituary(edict_t *self);
static void Killed(edict_t *targ, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (targ->health < -999)
		targ->health = -999;

	targ->enemy = attacker;

	if (targ->movetype == MOVETYPE_PUSH || targ->movetype == MOVETYPE_STOP || targ->movetype == MOVETYPE_NONE)
	{
		// doors, triggers, etc
		targ->die(targ, inflictor, attacker, damage, point);
		return;
	}

	if (targ->deadflag == DEAD_NO)
	{
#ifdef ENABLE_COOP
		if (targ->server.flags.monster)
		{
			if (!(targ->monsterinfo.aiflags & AI_GOOD_GUY))
			{
				level.killed_monsters++;

				// medics won't heal monsters that they kill themselves
				if (attacker->entitytype == ET_MONSTER_MEDIC)
					targ->server.owner = attacker;
			}

			targ->touch = NULL;
			monster_death_use(targ);
		}
#endif

		ClientObituary(targ);

		// scoring.
		// gain 1 point if we kill a monster
		if (attacker->server.client && targ->server.flags.monster)
			attacker->server.client->resp.score++;
		// lose 1 point if killed by monster
		else if (attacker->server.flags.monster && targ->server.client)
			targ->server.client->resp.score--;
		else if (targ->server.client && attacker->server.client)
		{
			// lose 1 point if we friendly fire kill
			if (targ->meansOfDeath.damage_means & MD_FRIENDLY_FIRE)
				attacker->server.client->resp.score--;
			// gain 1 point if we kill somebody we're
			// not on the same team with
			else
				attacker->server.client->resp.score++;
		}
	}

	if (targ->server.flags.monster && targ->freeze_time > level.time)
	{
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_DUKE_GLASS);
		MSG_WritePos(targ->server.state.origin);
		gi.multicast(targ->server.state.origin, MULTICAST_PVS);
		G_FreeEdict(targ);
	}
	else
		targ->die(targ, inflictor, attacker, damage, point);
}


/*
================
SpawnDamage
================
*/
static void SpawnDamage(int type, vec3_t origin, vec3_t normal, int damage)
{
	if (damage > 255)
		damage = 255;

	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(type);
	//  MSG_WriteByte (damage);
	MSG_WritePos(origin);
	MSG_WriteDir(normal);
	gi.multicast(origin, MULTICAST_PVS);
}


/*
============
T_Damage

targ        entity that is being damaged
inflictor   entity that is causing the damage
attacker    entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir         direction of the attack
point       point at which the damage is being inflicted
normal      normal vector from that point
damage      amount of damage being inflicted
knockback   force to be applied against targ as a result of the damage

dflags      these flags are used to control how T_Damage works
	DAMAGE_RADIUS           damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR         armor does not protect from this damage
	DAMAGE_ENERGY           damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK     do not affect velocity, just view angles
	DAMAGE_BULLET           damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION    kills godmode, armor, everything
============
*/
static int CheckPowerArmor(edict_t *ent, vec3_t point, vec3_t normal, int damage, int dflags)
{
	gclient_t   *client;
	float		save;
	power_armor_type_e         power_armor_type;
	float       damagePerCell;
	int         pa_te_type;
	float       power = 0;
	float       power_used;

	if (!damage)
		return 0;

	client = ent->server.client;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	if (client)
	{
		power_armor_type = PowerArmorType(ent);

		if (power_armor_type != POWER_ARMOR_NONE)
			power = floorf(AMMO_PER_POWER_ARMOR_ABSORB * client->pers.ammo);
	}
#ifdef ENABLE_COOP
	else if (ent->server.flags.monster)
	{
		power_armor_type = ent->monsterinfo.power_armor_type;
		power = ent->monsterinfo.power_armor_power;
	}
#endif
	else
		return 0;

	if (power_armor_type == POWER_ARMOR_NONE)
		return 0;

	if (!power)
		return 0;

	if (power_armor_type == POWER_ARMOR_SCREEN)
	{
		vec3_t      vec;
		float       dot;
		vec3_t      forward;
		// only works if damage point is in front
		AngleVectors(ent->server.state.angles, forward, NULL, NULL);
		VectorSubtract(point, ent->server.state.origin, vec);
		VectorNormalize(vec);
		dot = DotProduct(vec, forward);

		if (dot <= 0.3f)
			return 0;

		damagePerCell = 1;
		pa_te_type = TE_SCREEN_SPARKS;
		damage = damage / 3;
	}
	else
	{
		damagePerCell = 2;
		pa_te_type = TE_SHIELD_SPARKS;
		damage = (2 * damage) / 3;
	}

	save = power * damagePerCell;

	if (!save)
		return 0;

	if (save > damage)
		save = damage;

	SpawnDamage(pa_te_type, point, normal, save);
	ent->powerarmor_time = level.time + 200;
	power_used = save / damagePerCell;

	if (client)
		client->pers.ammo = max(0, client->pers.ammo - (power_used * AMMO_PER_POWER_ARMOR_ABSORB));
#ifdef ENABLE_COOP
	else
		ent->monsterinfo.power_armor_power -= power_used;
#endif

	return save;
}

static int CheckArmor(edict_t *ent, vec3_t point, vec3_t normal, int damage, int te_sparks, int dflags)
{
	gclient_t   *client;
	int         save;
	gitem_t     *armor;

	if (!damage)
		return 0;

	client = ent->server.client;

	if (!client)
		return 0;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	itemid_e index = ArmorIndex(ent);

	if (!index)
		return 0;

	armor = GetItemByIndex(index);
	gitem_armor_t *armorinfo = &game_iteminfos[ent->server.state.game].armors[index - ITI_JACKET_ARMOR];

	if (dflags & DAMAGE_ENERGY)
		save = ceilf(armorinfo->energy_protection * damage);
	else
		save = ceilf(armorinfo->normal_protection * damage);

	if (save >= client->pers.inventory[index])
		save = client->pers.inventory[index];

	if (!save)
		return 0;

	client->pers.inventory[index] -= save;
	SpawnDamage(te_sparks, point, normal, save);
	return save;
}

#ifdef ENABLE_COOP
static void M_ReactToDamage(edict_t *targ, edict_t *attacker)
{
	if (!(attacker->server.client) && !attacker->server.flags.monster)
		return;

	if (attacker == targ || attacker == targ->enemy)
		return;

	// if we are a good guy monster and our attacker is a player
	// or another good guy, do not get mad at them
	if (targ->monsterinfo.aiflags & AI_GOOD_GUY)
	{
		if (attacker->server.client || (attacker->monsterinfo.aiflags & AI_GOOD_GUY))
			return;
	}

	// we now know that we are not both good guys

	// if attacker is a client, get mad at them because he's good and we're not
	if (attacker->server.client)
	{
		targ->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

		// this can only happen in coop (both new and old enemies are clients)
		// only switch if can't see the current enemy
		if (targ->enemy && targ->enemy->server.client)
		{
			if (visible(targ, targ->enemy))
			{
				targ->oldenemy = attacker;
				return;
			}

			targ->oldenemy = targ->enemy;
		}

		targ->enemy = attacker;

		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget(targ);

		return;
	}

	// it's the same base (walk/swim/fly) type and a different classname and it's not a tank
	// (they spray too much), get mad at them
	if (((targ->flags & (FL_FLY | FL_SWIM)) == (attacker->flags & (FL_FLY | FL_SWIM))) &&
		targ->entitytype != attacker->entitytype &&
		attacker->entitytype != ET_MONSTER_TANK &&
		attacker->entitytype != ET_MONSTER_TANK_COMMANDER &&
		attacker->entitytype != ET_MONSTER_SUPERTANK &&
		attacker->entitytype != ET_MONSTER_MAKRON &&
		attacker->entitytype != ET_MONSTER_JORG)
	{
		if (targ->enemy && targ->enemy->server.client)
			targ->oldenemy = targ->enemy;

		targ->enemy = attacker;

		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget(targ);
	}
	// if they *meant* to shoot us, then shoot back
	else if (attacker->enemy == targ)
	{
		if (targ->enemy && targ->enemy->server.client)
			targ->oldenemy = targ->enemy;

		targ->enemy = attacker;

		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget(targ);
	}
	// otherwise get mad at whoever they are mad at (help our buddy) unless it is us!
	else if (attacker->enemy && attacker->enemy != targ)
	{
		if (targ->enemy && targ->enemy->server.client)
			targ->oldenemy = targ->enemy;

		targ->enemy = attacker->enemy;

		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget(targ);
	}
}
#endif

void Q1_SpawnBlood(vec3_t org, vec3_t vel, int damage)
{
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_Q1_BLOOD);
	MSG_WritePos(org);

	if (vel)
		MSG_WritePos(vel);
	else
		MSG_WritePos(vec3_origin);

	MSG_WriteByte((byte)min((byte)255, (byte)damage));
	gi.multicast(org, MULTICAST_PVS);
}

static void ApplyMeansOfDeath(edict_t *target, meansOfDeath_t *mod)
{
	// if it's not a monster or client, means of death will do nothing
	if (!target->server.client && !target->server.flags.monster)
		return;

	// if the attacker is a client or monster, copy us over right away
	if (mod->attacker->server.client || mod->attacker->server.flags.monster)
	{
		target->meansOfDeath.attacker = mod->attacker;
		target->meansOfDeath.attacker_type = mod->attacker->entitytype;
		target->meansOfDeath.attacker_game = mod->attacker->server.state.game;
		target->meansOfDeath.attacker_time = level.time + 10000;
	}

	target->meansOfDeath.damage_means = mod->damage_means;
	target->meansOfDeath.damage_type = mod->damage_type;
	target->meansOfDeath.inflictor = mod->inflictor;

	if ((mod->damage_means & MD_MEANS_MASK) == MD_WEAPON)
		target->meansOfDeath.weapon_id = mod->weapon_id;

	target->meansOfDeath.time = mod->time;
}

void T_Damage(edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir_in, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, meansOfDeath_t mod)
{
	gclient_t   *client;
	int         take;
	int         save;
	int         asave;
	int         psave;
	int         te_sparks;

	if (!targ->takedamage)
		return;

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if (targ != attacker && OnSameTeam(targ, attacker))
	{
		if (!dmflags.friendly_fire)
			damage = 0;
		else
			mod.damage_means |= MD_FRIENDLY_FIRE;
	}

#ifdef ENABLE_COOP
	// easy mode takes half damage
	if (skill->integer == 0 && deathmatch->value == 0 && targ->server.client)
	{
		damage *= 0.5f;

		if (!damage)
			damage = 1;
	}
#endif

	client = targ->server.client;

	if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

	vec3_t dir;
	VectorNormalize2(dir_in, dir);

	// bonus damage for suprising a monster
	if (!(dflags & DAMAGE_RADIUS) && targ->server.flags.monster && (attacker->server.client) && (!targ->enemy) && (targ->health > 0) && (targ->server.state.game == GAME_Q2))
		damage *= 2;

	if (targ->flags & FL_NO_KNOCKBACK)
		knockback = 0;

	// figure momentum add
	if (!(dflags & DAMAGE_NO_KNOCKBACK))
	{
		if (targ->server.flags.monster && targ->server.state.game == GAME_DOOM)
		{
			// Some close combat weapons should not
			// inflict thrust and push the victim out of reach,
			// thus kick away unless using the chainsaw.
			if (inflictor && (!attacker
					|| !attacker->server.client
					|| attacker->server.client->pers.weapon != GetItemByIndex(ITI_DOOM_CHAINSAW)))
			{
				vec3_t ang;
				VectorAdd(inflictor->server.absmin, inflictor->server.absmax, ang);
				VectorScale(ang, 0.5f, ang);
				VectorSubtract(targ->server.state.origin, ang, ang);
				VectorNormalize(ang);
				float thrust = damage/* *(FRACUNIT >> 3) */ * 100 / targ->mass;

				// make fall forwards sometimes
				if (damage < 40
					&& damage > targ->health
					&& targ->server.state.origin[2] - inflictor->server.state.origin[2] > 64
					&& (Q_rand() & 1))
				{
					vectoangles(ang, ang);
					ang[1] += 180;
					AngleVectors(ang, ang, NULL, NULL);
					thrust *= 4;
				}

				VectorMA(targ->velocity, thrust, ang, targ->velocity);
			}
		}
		else if ((knockback) && (targ->movetype != MOVETYPE_NONE) && (targ->movetype != MOVETYPE_BOUNCE) && (targ->movetype != MOVETYPE_PUSH) && (targ->movetype != MOVETYPE_STOP))
		{
			if (dflags & DAMAGE_Q1)
			{
				vec3_t td;
				VectorAdd(inflictor->server.absmin, inflictor->server.absmax, td);
				VectorScale(td, 0.5f, td);
				VectorSubtract(targ->server.state.origin, td, td);
				VectorNormalize(td);
				VectorMA(targ->velocity, damage * 8, dir, targ->velocity);
			}
			else
			{
				vec3_t  kvel;
				float   mass;

				if (targ->mass < 50)
					mass = 50;
				else
					mass = targ->mass;

				if (targ->server.client && attacker == targ/* && targ->server.state.game == GAME_Q2*/)
					VectorScale(dir, 1600.0f * (float)knockback / mass, kvel);   // the rocket jump hack...
				else
					VectorScale(dir, 500.0f * (float)knockback / mass, kvel);

				VectorAdd(targ->velocity, kvel, targ->velocity);
			}
		}
	}

	take = damage;
	save = 0;

	// check for godmode
	if ((targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		take = 0;
		save = damage;

		if (!(dflags & DAMAGE_NO_PARTICLES))
			SpawnDamage(te_sparks, point, normal, save);
	}

	// check for invincibility
	if ((client && client->invincible_time > level.time) && !(dflags & (DAMAGE_NO_PROTECTION | DAMAGE_PARTICLES_ONLY)))
	{
		if (targ->pain_debounce_time < level.time)
		{
			gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			targ->pain_debounce_time = level.time + 2000;
		}

		take = 0;
		save = damage;
	}

	psave = CheckPowerArmor(targ, point, normal, take, dflags);
	take -= psave;
	asave = CheckArmor(targ, point, normal, take, te_sparks, dflags);
	take -= asave;
	//treat cheat/powerup savings the same as armor
	asave += save;

	// team damage avoidance
	//if (!(dflags & DAMAGE_NO_PROTECTION) && CheckTeamDamage(targ, attacker))
	//	return;

	// do the damage
	if (take)
	{
		if (!(dflags & DAMAGE_NO_PARTICLES))
		{
			if (dflags & DAMAGE_Q1)
				Q1_SpawnBlood(point, normal, take);
			else
			{
				if (targ->server.flags.monster || (client))
					SpawnDamage(TE_BLOOD, point, normal, take);
				else
					SpawnDamage(te_sparks, point, normal, take);
			}
		}

		if (!(dflags & DAMAGE_PARTICLES_ONLY))
		{
			ApplyMeansOfDeath(targ, &mod);
			targ->health = targ->health - take;

			// Doom uses this for faces
			if (targ->server.client)
			{
				vec3_t diff = { targ->server.state.origin[0] - attacker->server.state.origin[0], targ->server.state.origin[1] - attacker->server.state.origin[1], 0 };
				VectorNormalize(diff);
				vectoangles2(diff, diff);
				targ->server.client->damage_dir = diff[1];
			}

			if (targ->health <= 0)
			{
				Killed(targ, inflictor, attacker, take, point);
				return;
			}
		}
	}

	if (dflags & DAMAGE_PARTICLES_ONLY)
		return;

#ifdef ENABLE_COOP
	if (targ->server.flags.monster)
	{
		if (take)
		{
			M_ReactToDamage(targ, attacker);

			if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			{
				if (targ->pain)
				{
					targ->pain(targ, attacker, knockback, take);

					if (targ->entitytype != ET_DOOM_MONSTER_SKUL)
						targ->monsterinfo.aiflags |= AI_JUST_HIT;
				}

				// nightmare mode monsters don't go into pain frames often
				if (skill->value == 3)
					targ->pain_debounce_time = level.time + 5000;
			}
		}
	}
	else
#endif
	if (client)
	{
		if (!(targ->flags & FL_GODMODE) && (take))
			targ->pain(targ, attacker, knockback, take);
	}
	else if (take)
	{
		if (targ->pain)
			targ->pain(targ, attacker, knockback, take);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client)
	{
		client->damage_parmor += psave;
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		VectorCopy(point, client->damage_from);
	}
}


/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage(edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, int dflags, float radius, meansOfDeath_t mod)
{
	float   points;
	edict_t *ent = NULL;
	vec3_t  v;
	vec3_t  dir;

	if (dflags & DAMAGE_Q1)
		radius += 40;

	mod.damage_type = DT_INDIRECT;

	while ((ent = findradius(ent, inflictor->server.state.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;

		if (!ent->takedamage)
			continue;

		VectorAdd(ent->server.mins, ent->server.maxs, v);
		VectorMA(ent->server.state.origin, 0.5f, v, v);
		VectorSubtract(inflictor->server.state.origin, v, v);
		points = damage - 0.5f * VectorLength(v);

		if (ent == attacker)
			points = points * 0.5f;

		if (points > 0)
		{
			if (CanDamage(ent, inflictor))
			{
				VectorSubtract(ent->server.state.origin, inflictor->server.state.origin, dir);

				// shambler takes half damage
#ifdef ENABLE_COOP
				points *= 0.5f;
#endif

				T_Damage(ent, inflictor, attacker, dir, inflictor->server.state.origin, vec3_origin, (int)points, (int)points, dflags | DAMAGE_RADIUS, mod);
			}
		}
	}
}
