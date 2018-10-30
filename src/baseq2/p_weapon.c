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
// g_weapon.c

#include "g_local.h"
#include "m_player.h"


static bool     is_quad;
static byte     is_silenced;

static int pipe_detonator_index, pipe_bomb_index, foot_index;

void NoAmmoWeaponChange(edict_t *ent, gunindex_e gun);

void check_dodge(edict_t *, vec3_t, vec3_t, int);

const gtime_t DOOM_ANIM_TIME = 400;
const gtime_t DOOM_ANIM_FRAC = DOOM_ANIM_TIME;

void P_ProjectSource(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t  _distance;

	VectorCopy(distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource(point, _distance, forward, right, result);
}

void Weapon_QuadDamage(edict_t *ent)
{
	if (ent->client->quad_time > level.time)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
}

void Weapon_Doom(edict_t *ent, int num_frames, void(*fire) (edict_t *ent, gunindex_e gun, bool first), gunindex_e gun)
{
	if (ent->deadflag || !ent->solid || ent->freeze_time > level.time) // VWep animations screw up corpses
		return;

	/*if (ent->client->next_weapon_update > ent->client->player_time)
	return;

	ent->client->next_weapon_update = ent->client->player_time + 1;*/

	VectorClear(ent->client->ps.guns[gun].offset);

	switch (ent->client->gunstates[gun].weaponstate)
	{
	case WEAPON_ACTIVATING:
		ent->client->gunstates[gun].weapon_time = min(DOOM_ANIM_TIME, ent->client->gunstates[gun].weapon_time + game.frametime);

		if (ent->client->gunstates[gun].weapon_time == DOOM_ANIM_TIME)
			ent->client->ps.guns[gun].offset[2] = 0.0f;
		else
		{
			ent->client->ps.guns[gun].offset[2] = (float)(DOOM_ANIM_TIME - ent->client->gunstates[gun].weapon_time) / DOOM_ANIM_FRAC;
			break;
		}

	case WEAPON_READY:
		ent->client->gunstates[gun].weaponstate = WEAPON_READY;

		if ((ent->client->gunstates[gun].newweapon) && (ent->client->gunstates[gun].weaponstate != WEAPON_FIRING))
		{
			if (ent->s.game == GAME_DUKE)
			{
				const char *sound = NULL;

				switch (GetIndexByItem(ent->client->gunstates[gun].newweapon))
				{
				case ITI_DUKE_FOOT:
					//case ITI_DUKE_TRIPBOMBS:
				case ITI_DUKE_PIPEBOMBS:
					break;
				case ITI_DUKE_PISTOL:
					sound = "duke/CLIPIN.wav";
					break;
				case ITI_DUKE_CANNON:
				default:
					sound = "duke/WPNSEL21.wav";
					break;
				case ITI_DUKE_SHOTGUN:
					sound = "duke/SHOTGNCK.wav";
					break;
				}

				if (sound)
					gi.sound(ent, CHAN_ITEM | CHAN_NO_PHS_ADD | CHAN_NO_EAVESDROP, gi.soundindex(sound), 1, ATTN_NONE, 0);
			}

			ent->client->gunstates[gun].weaponstate = WEAPON_DROPPING;
			ent->client->gunstates[gun].weapon_time = DOOM_ANIM_TIME;
			goto dropping;
		}

		if ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;

			if ((level.time - ent->client->respawn_time) > 500 &&
				ent->attack_finished_time < level.time)
			{
				if ((!ent->client->gunstates[gun].ammo_index) || (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] >= GetWeaponUsageCount(ent, ent->client->pers.weapon)))
				{
					fire(ent, gun, true);

					if (ent->client->ps.guns[gun].frame >= num_frames)
					{
						ent->client->ps.guns[gun].frame = 0;
						ent->client->gunstates[gun].weaponstate = WEAPON_READY;
					}
					else
						ent->client->gunstates[gun].weaponstate = WEAPON_FIRING;
				}
				else
					NoAmmoWeaponChange(ent, gun);
			}

			if (ent->s.game == GAME_DUKE)
				ent->client->ps.guns[gun].offset[1] = 1.0f;
		}
		else
			ent->client->ps.guns[gun].offset[1] = 1.0f;

		break;
	case WEAPON_FIRING:
		if (ent->attack_finished_time < level.time)
			fire(ent, gun, false);

		if (ent->client->ps.guns[gun].frame >= num_frames)
		{
			ent->client->ps.guns[gun].frame = 0;
			ent->client->gunstates[gun].weaponstate = WEAPON_READY;
		}

		if (ent->client->gunstates[gun].weaponstate == WEAPON_FIRING && ent->s.game == GAME_DUKE)
			ent->client->ps.guns[gun].offset[1] = 1.0f;

		break;
	case WEAPON_DROPPING:
		ent->client->gunstates[gun].weapon_time = (gtime_t)max(0, (int32_t)ent->client->gunstates[gun].weapon_time - (int32_t)game.frametime);

	dropping:

		if (ent->client->gunstates[gun].weapon_time == 0)
		{
			ent->client->ps.guns[gun].offset[2] = DOOM_ANIM_FRAC;
			ChangeWeapon(ent, gun);
			return;
		}
		else
			ent->client->ps.guns[gun].offset[2] = (float)(DOOM_ANIM_TIME - ent->client->gunstates[gun].weapon_time) / DOOM_ANIM_FRAC;

		break;
	}
}

void Weapon_Q1(edict_t *ent, int num_frames, void(*fire) (edict_t *ent, gunindex_e gun), gunindex_e gun)
{
	if (ent->deadflag || !ent->solid || ent->freeze_time > level.time) // VWep animations screw up corpses
		return;

	/*if (ent->client->next_weapon_update > ent->client->player_time)
		return;

	ent->client->next_weapon_update = ent->client->player_time + 100;*/

	switch (ent->client->gunstates[gun].weaponstate)
	{
	case WEAPON_ACTIVATING:
	case WEAPON_READY:
		ent->client->gunstates[gun].weaponstate = WEAPON_READY;

		if ((ent->client->gunstates[gun].newweapon) && (ent->client->gunstates[gun].weaponstate != WEAPON_FIRING))
		{
			ChangeWeapon(ent, gun);
			return;
		}

		if ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;

			if ((level.time - ent->client->respawn_time) > 500 &&
				ent->attack_finished_time < ent->client->player_time)
			{
				if ((!ent->client->gunstates[gun].ammo_index) || (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] >= GetWeaponUsageCount(ent, ent->client->pers.weapon)))
				{
					fire(ent, gun);

					ent->client->gunstates[gun].weaponstate = WEAPON_FIRING;
				}
				else
					NoAmmoWeaponChange(ent, gun);
			}
		}
		break;
	case WEAPON_FIRING:
		fire(ent, gun);

		if (ent->client->ps.guns[gun].frame >= num_frames)
		{
			ent->client->ps.guns[gun].frame = 0;
			ent->client->gunstates[gun].weaponstate = WEAPON_READY;
		}

		break;
	}
}

void Weapon_Q1_Anim_Shot(edict_t *ent)
{
	if (ent->health <= 0)
		return;

	ent->client->anim_priority = ANIM_ATTACK;
	ent->s.frame = 113 - 1;
	ent->client->anim_end = 118;
	Weapon_QuadDamage(ent);
}

void Weapon_Q1_Anim_Rock(edict_t *ent)
{
	if (ent->health <= 0)
		return;

	ent->client->anim_priority = ANIM_ATTACK;
	ent->s.frame = 107 - 1;
	ent->client->anim_end = 112;
	Weapon_QuadDamage(ent);
}

void Weapon_Q1_Anim_Nail(edict_t *ent)
{
	if (ent->health <= 0)
		return;

	ent->client->anim_priority = ANIM_ATTACK;
	ent->s.frame = (ent->s.frame == 103 ? 104 : 103) - 1;
	ent->client->anim_end = 104;
	Weapon_QuadDamage(ent);
}

void Weapon_Q1_Anim_Light(edict_t *ent)
{
	if (ent->health <= 0)
		return;

	ent->client->anim_priority = ANIM_ATTACK;
	ent->s.frame = (ent->s.frame == 105 ? 106 : 105) - 1;
	ent->client->anim_end = 106;
	Weapon_QuadDamage(ent);
}

void Weapon_Doom_Anim_Regular(edict_t *ent)
{
	if (ent->health <= 0)
		return;

	ent->client->anim_priority = ANIM_ATTACK | ANIM_REVERSE;
	ent->s.frame = 5 + 1;
	ent->client->anim_end = 4;
}

void Weapon_Doom_Anim_Fast(edict_t *ent)
{
	if (Q_rand() % 2 == 0)
		Weapon_Doom_Anim_Regular(ent);
	else
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = 4 - 1;
		ent->client->anim_end = 5;
	}
}

void Q1_SpawnBlood(vec3_t org, vec3_t vel, int damage);

#define MAX_MULTIHIT 32

static struct {
	edict_t		*hit;
	int			damage;
	int			kick;
	int			old_clipmask;
} multi_ent[MAX_MULTIHIT];
static int multi_hits = 0;

static void ClearMultiDamage()
{
	for (int i = 0; i < multi_hits; ++i)
	{
		multi_ent[i].hit->hit_index = 0;
		multi_ent[i].hit->s.clip_contents = multi_ent[i].old_clipmask;
	}

	memset(multi_ent, 0, sizeof(multi_ent));
	multi_hits = 0;
}

void ApplyMultiDamage(edict_t *self, int dflags, meansOfDeath_t multi_mod)
{
	for (int i = 0; i < multi_hits; ++i)
		T_Damage (multi_ent[i].hit, self, self, vec3_origin, vec3_origin, vec3_origin, multi_ent[i].damage, multi_ent[i].kick, dflags, multi_mod);

	ClearMultiDamage();
}

void AddMultiDamage(edict_t *hit, int damage, int kick, meansOfDeath_t multi_mod, int dflags, bool absorb_all)
{
	if (!hit)
		return;

	if (hit->hit_index == 0)
	{
		if (multi_hits == MAX_MULTIHIT)
		{
			gi.dprintf("Too many multihits");

			ApplyMultiDamage(hit, dflags, multi_mod);
		}

		hit->hit_index = ++multi_hits;
		multi_ent[hit->hit_index - 1].hit = hit;
		multi_ent[hit->hit_index - 1].old_clipmask = hit->s.clip_contents;
	}

	multi_ent[hit->hit_index - 1].damage += damage;
	multi_ent[hit->hit_index - 1].kick += kick;

	if (!absorb_all)
	{
		int total_to_absorb = hit->health;

		if (hit->gib_health && hit->s.game != GAME_DOOM)
			total_to_absorb += -hit->gib_health;

		// traces will skip this for future shots
		if (multi_ent[hit->hit_index - 1].damage >= total_to_absorb)
			hit->s.clip_contents = 0;
	}
}

void TraceAttack(edict_t *ent, trace_t *tr, int damage, vec3_t dir, vec3_t v_up, vec3_t v_right, meansOfDeath_t mod)
{
	vec3_t org;

	VectorMA(tr->endpos, -4, dir, org);

	if (tr->ent->takedamage)
	{
		vec3_t vel;
		VectorMA(dir, crandom(), v_up, vel);
		VectorMA(vel, crandom(), v_right, vel);
		VectorNormalize(vel);
		VectorMA(vel, 2, tr->plane.normal, vel);
		VectorScale(vel, 200 * 0.02f, vel);

		/*blood_count = blood_count + 1;
		VectorCopy(org, blood_org);
		AddMultiDamage(ent, tr->ent, damage);
		SpawnBlood(blood_org, vel, damage);*/
		Q1_SpawnBlood(org, vel, damage);
		AddMultiDamage(tr->ent, damage, 0, mod, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, true);
		//T_Damage(tr->ent, ent, ent, vec3_origin, org, vel, damage, 0, DAMAGE_Q1 | DAMAGE_BULLET, mod);
	}
	else
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_Q1_GUNSHOT);
		gi.WriteByte(1);
		gi.WritePosition(org);
		gi.multicast(org, MULTICAST_PVS);
	}
}

void FireBullets(edict_t *ent, int shotcount, int damage, vec3_t dir, vec3_t spread, vec3_t v_up, vec3_t v_right, meansOfDeath_t mod)
{
	vec3_t direction, src;
	vec3_t forward, right, up;

	if (ent->client)
		AngleVectors(ent->client->v_angle, forward, right, up);
	else
		AngleVectors(ent->s.angles, forward, right, up);

	VectorMA(ent->s.origin, 10, forward, src);
	src[2] += ent->viewheight - 8;

	vec3_t end;

	VectorMA(src, 2048, dir, end);

	trace_t tr = gi.trace(src, vec3_origin, vec3_origin, end, ent, MASK_BULLET);

	while (shotcount > 0)
	{
		for (int i = 0; i < 3; ++i)
			direction[i] = dir[i] + crandom()*spread[0] * right[i] + crandom()*spread[1] * up[i];

		VectorMA(src, 2048, direction, end);

		tr = gi.trace(src, vec3_origin, vec3_origin, end, ent, MASK_BULLET);

		if (tr.fraction != 1.0f)
		{
			TraceAttack(ent, &tr, damage, direction, v_up, v_right, mod);
		}

		shotcount = shotcount - 1;
	}

	ApplyMultiDamage(ent, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, mod);
}

/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void PlayerNoise(edict_t *who, vec3_t where, int type)
{
	edict_t     *noise;

	if (type == PNOISE_WEAPON) {
		if (who->client->silencer_shots) {
			who->client->silencer_shots--;
			return;
		}
	}

	if (deathmatch->value)
		return;

	//if (who->flags & FL_NOTARGET)
	//	return;


	if (!who->mynoise) {
		noise = G_Spawn();
		noise->entitytype = ET_PLAYER_NOISE;
		VectorSet(noise->mins, -8, -8, -8);
		VectorSet(noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise = noise;

		noise = G_Spawn();
		noise->entitytype = ET_PLAYER_NOISE;
		VectorSet(noise->mins, -8, -8, -8);
		VectorSet(noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise2 = noise;
	}

	if (type == PNOISE_SELF || type == PNOISE_WEAPON) {
		noise = who->mynoise;
		level.sound_entity = noise;
		level.sound_entity_time = level.time + 100;
	}
	else { // type == PNOISE_IMPACT
		noise = who->mynoise2;
		level.sound2_entity = noise;
		level.sound2_entity_time = level.time + 100;
	}

	VectorCopy(where, noise->s.origin);
	VectorSubtract(where, noise->maxs, noise->absmin);
	VectorAdd(where, noise->maxs, noise->absmax);
	noise->teleport_time = level.time;
	gi.linkentity(noise);
}

extern bool do_propagate;

bool ShouldSwapToPotentiallyBetterWeapon(edict_t *ent, gitem_t *new_item, bool is_new)
{
	bool weapon_uses_ammo = !!game_iteminfos[ent->s.game].dynamic.weapon_uses_this_ammo[GetIndexByItem(ent->client->pers.weapon)];
	bool swap_back_to_pipebombs;

	switch (ent->s.game)
	{
	case GAME_Q2:
		if (deathmatch->value && weapon_uses_ammo)
			return false;
		// switch to Grenades if we have none yet
		// or we have a Blaster out
		else if ((new_item && (new_item->flags & IT_AMMO) && is_new) || !weapon_uses_ammo)
			return true;
		break;
	case GAME_Q1:
		if (deathmatch->value && weapon_uses_ammo && GetIndexByItem(ent->client->pers.weapon) != ITI_Q1_SHOTGUN)
			return false;
		break;
	case GAME_DOOM:
		if (deathmatch->value && weapon_uses_ammo && GetIndexByItem(ent->client->pers.weapon) != ITI_DOOM_PISTOL)
			return false;
		break;
	case GAME_DUKE:
		swap_back_to_pipebombs = GetIndexByItem(ent->client->pers.weapon) == ITI_DUKE_PIPEBOMBS &&
			ent->client->pers.alt_weapon &&
			GetIndexByItem(new_item) == ITI_DUKE_PIPEBOMBS;

		if (swap_back_to_pipebombs)
			return true;
		else if (deathmatch->value && weapon_uses_ammo && GetIndexByItem(ent->client->pers.weapon) != ITI_DUKE_PISTOL)
			return false;
		else if (new_item != NULL && (new_item->flags & IT_AMMO) && is_new)
			return false;
		break;
	}

	return !new_item || ent->client->pers.weapon != new_item;
}

bool Pickup_Weapon(edict_t *ent, edict_t *other)
{
	int         index;
	gitem_t     *ammo;
	bool		is_new;

	index = ITEM_INDEX(ent->item);

	if (!invasion->value && (((int)(dmflags->value) & DF_WEAPONS_STAY) || coop->value) && other->client->pers.inventory[index])
	{
		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
			return false;
	}

	is_new = !other->client->pers.inventory[index];

	if (!other->client->pers.inventory[index])
		other->client->pers.inventory[index]++;

	if (!(ent->spawnflags & DROPPED_ITEM) || ent->count)
	{
		// give them some ammo with it
		itemid_e ammo_index = game_iteminfos[other->s.game].dynamic.weapon_uses_this_ammo[index];

		if (ammo_index != ITI_NULL)
		{
			ammo = GetItemByIndex(ammo_index);

			int real_num;

			if (ent->count)
				real_num = ent->count;
			else
				// the count we get for ammo is from the real item, not the item we're getting.
				// this is so "Bullets (Large)" gives the right amount for Q1, for instance,
				// while not affecting Q2
				// ...
				real_num = game_iteminfos[other->s.game].dynamic.ammo_pickup_amounts[GetIndexByItem(ent->real_item)];

			ammo = ResolveItemRedirect(other, ammo);
			ammo_index = GetIndexByItem(ammo);

			if ((int)dmflags->value & DF_INFINITE_AMMO)
				Add_Ammo(other, ammo_index, 1000, false);
			else
			{
				// ...unless it's -1
				if (real_num != -1)
					Add_Ammo(other, ammo_index, real_num, false);
				else
					Add_Ammo(other, ammo_index, game_iteminfos[other->s.game].dynamic.ammo_pickup_amounts[ammo_index], false);
			}
		}

		if (!(ent->spawnflags & (DROPPED_ITEM|DROPPED_PLAYER_ITEM)))
		{
			if (deathmatch->value)
			{
				if ((int)(dmflags->value) & DF_WEAPONS_STAY)
					ent->flags |= FL_RESPAWN;
				else
					SetRespawn(ent, 30);
			}
			else if (coop->value)
				ent->flags |= FL_RESPAWN;
		}
	}

	if (ShouldSwapToPotentiallyBetterWeapon(other, ent->item, is_new))
	{
		gitem_t *best = GetBestWeapon(other, ent->item, is_new);

		if (best && best == ent->item)
			other->client->gunstates[GUN_MAIN].newweapon = best;
	}

	// in invasion, propagate item to other clients
	if (invasion->value && do_propagate)
	{
		do_propagate = false;
		for (int i = 0; i < game.maxclients; ++i)
		{
			edict_t *cl = &g_edicts[i + 1];

			if (!cl->inuse || !cl->client->pers.connected || !cl->s.game || cl == other)
				continue;

			Pickup_Weapon(ent, cl);
		}
		do_propagate = true;
	}

	// only the pickup duke player will have a quip
	if (other->s.game == GAME_DUKE && do_propagate)
	{
		int r = Q_rand() % 256;
		const char *sound;

		if (r < 64)
			sound = "duke/cool01.wav";
		else if (r < 96)
			sound = "duke/getsom1a.wav";
		else if (r < 128)
			sound = "duke/groovy02.wav";
		else if (r < 140)
			sound = "duke/wansom4a.wav";
		else
			sound = "duke/hail01.wav";

		gi.sound(other, CHAN_VOICE, gi.soundindex(sound), 1, ATTN_NORM, 0);
	}

	return true;
}

#define GRENADE_TIMER       3000.0f
#define GRENADE_MINSPEED    400.0f
#define GRENADE_MAXSPEED    800.0f

void weapon_grenade_fire(edict_t *ent, gunindex_e gun, bool held)
{
	vec3_t  offset;
	vec3_t  forward, right;
	vec3_t  start;
	int     damage = 125;
	int     speed;
	float   radius;

	radius = damage + 40;

	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	gtime_t timer;

	if (ent->client->grenade_time > level.time)
		timer = ent->client->grenade_time - level.time;
	else
		timer = 0;

	speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER);
	fire_grenade2(ent, start, forward, damage, speed, timer, radius, held);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;

	ent->client->grenade_time = level.time + 1000;

	if (ent->deadflag || !ent->solid) { // VWep animations screw up corpses
		return;
	}

	if (ent->health <= 0)
		return;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = FRAME_crattak1 - 1;
		ent->client->anim_end = FRAME_crattak3;
	}
	else {
		ent->client->anim_priority = ANIM_WAVE | ANIM_REVERSE;
		ent->s.frame = FRAME_wave08;
		ent->client->anim_end = FRAME_wave01;
	}
}


/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon(edict_t *ent, gunindex_e gun)
{
	int i;

	if (ent->client->grenade_time) {
		ent->client->grenade_time = level.time;
		ent->client->weapon_sound = 0;
		weapon_grenade_fire(ent, gun, false);
		ent->client->grenade_time = 0;
	}

	bool first = !ent->client->pers.lastweapon;

	if (ent->client->pers.weapon != ent->client->pers.lastweapon)
		ent->client->pers.lastweapon = ent->client->pers.weapon;

	gitem_t *old_weapon = ent->client->pers.weapon;
	
	ent->client->pers.weapon = ent->client->gunstates[gun].newweapon;
	ent->client->gunstates[gun].newweapon = NULL;
	ent->client->gunstates[gun].machinegun_shots = 0;

	// set visible model
	if (ent->s.modelindex == 255)
	{
		if (ent->client->pers.weapon)
			i = ((ent->client->pers.weapon->weapmodel & 0xff) << 8);
		else
			i = 0;
		ent->s.skinnum = (ent - g_edicts - 1) | i;
	}

	if (ent->client->pers.weapon && game_iteminfos[ent->s.game].dynamic.weapon_uses_this_ammo[GetIndexByItem(ent->client->pers.weapon)])
		ent->client->gunstates[gun].ammo_index = game_iteminfos[ent->s.game].dynamic.weapon_uses_this_ammo[GetIndexByItem(ent->client->pers.weapon)];
	else
		ent->client->gunstates[gun].ammo_index = 0;

	if (!ent->client->pers.weapon) {
		// dead
		memset(ent->client->ps.guns, 0, sizeof(ent->client->ps.guns));
		return;
	}

	ent->client->gunstates[gun].weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.guns[gun].frame = 0;
	ent->client->ps.guns[gun].index = gi.modelindex(ent->client->pers.weapon->view_model);

	switch (ent->s.game)
	{
	case GAME_Q2:
		ent->client->anim_priority = ANIM_PAIN;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
			ent->s.frame = FRAME_crpain1;
			ent->client->anim_end = FRAME_crpain4;
		}
		else {
			ent->s.frame = FRAME_pain301;
			ent->client->anim_end = FRAME_pain304;
		}
		break;
	case GAME_Q1:
		ent->client->force_anim = true;
		break;
	case GAME_DOOM:
	case GAME_DUKE:
		if (first)
		{
			//ent->client->next_weapon_update = ent->client->player_time;

			if (GetIndexByItem(ent->client->pers.weapon) == ITI_DUKE_PISTOL &&
				!ent->client->pers.pistol_clip &&
				ent->client->pers.inventory[ITI_DUKE_CLIP])
			{
				ent->client->gunstates[gun].weaponstate = WEAPON_FIRING;
				ent->client->ps.guns[gun].frame = 14;
			}
		}

		// special case for pipebomb
		if (GetIndexByItem(ent->client->pers.weapon) == ITI_DUKE_PIPEBOMBS)
		{
			if (!ent->client->pers.inventory[GetIndexByItem(ent->client->pers.weapon)])
				ent->client->pers.alt_weapon = true;
			else if (GetIndexByItem(old_weapon) == ITI_DUKE_PIPEBOMBS)
				ent->client->pers.alt_weapon = !ent->client->pers.alt_weapon;
			else
			{
				if (ent->client->pers.inventory[GetIndexByItem(ent->client->pers.weapon)])
					ent->client->pers.alt_weapon = false;
				else
					ent->client->pers.alt_weapon = true;
			}

			if (ent->client->pers.alt_weapon)
				ent->client->ps.guns[gun].index = pipe_detonator_index;
		}
		else
			ent->client->pers.alt_weapon = false;
		break;
	}
}

static bool HasAmmoCount(edict_t *ent, itemid_e ammo_index, itemid_e new_item_index, int count)
{
	if (ent->client->pers.inventory[ammo_index] >= count)
		return true;

	if (new_item_index == ammo_index)
	{
		int pickup_count = game_iteminfos[ent->s.game].dynamic.ammo_pickup_amounts[new_item_index];
		return pickup_count >= count;
	}

	return false;
}

#define HasAmmo(ent, ammo_index, new_item_index) HasAmmoCount(ent, ammo_index, new_item_index, 1)

static bool HasWeapon(edict_t *ent, itemid_e weapon_index, itemid_e new_item_index)
{
	return ent->client->pers.inventory[weapon_index] || new_item_index == weapon_index;
}

#define HasAmmoCountQuick(type, count) HasAmmoCount(ent, type, new_item_index, count)
#define HasAmmoQuick(type) HasAmmo(ent, type, new_item_index)
#define HasWeaponQuick(type) HasWeapon(ent, type, new_item_index)

static itemid_e dukeWeaponChoice[] = {
	ITI_DUKE_SHOTGUN,
	ITI_DUKE_CANNON,
	ITI_DUKE_RPG,
	//ITI_DUKE_SHRINKER,
	ITI_DUKE_DEVASTATOR,
	ITI_DUKE_PIPEBOMBS,
	ITI_DUKE_FREEZER,
	ITI_DUKE_PISTOL,
	//ITI_DUKE_TRIPBOMBS,
	ITI_DUKE_FOOT
};

gitem_t *GetBestWeapon(edict_t *ent, gitem_t *new_item, bool is_new)
{
	itemid_e new_item_index = GetIndexByItem(new_item);
	int i;
	itemid_e weap;

	switch (ent->s.game)
	{
	case GAME_Q2:
		if (HasAmmoQuick(ITI_SLUGS) && HasWeaponQuick(ITI_RAILGUN))
			return GetItemByIndex(ITI_RAILGUN);
		else if (HasAmmoQuick(ITI_CELLS) && HasWeaponQuick(ITI_HYPERBLASTER))
			return GetItemByIndex(ITI_HYPERBLASTER);
		else if ((HasAmmoCountQuick(ITI_BULLETS, 25) ||
				(HasAmmoQuick(ITI_BULLETS) && !HasWeaponQuick(ITI_MACHINEGUN))) && HasWeaponQuick(ITI_CHAINGUN))
			return GetItemByIndex(ITI_CHAINGUN);
		else if (HasAmmoQuick(ITI_BULLETS) && HasWeaponQuick(ITI_MACHINEGUN))
			return GetItemByIndex(ITI_MACHINEGUN);
		else if (HasAmmoCountQuick(ITI_SHELLS, 2) && HasWeaponQuick(ITI_SUPER_SHOTGUN))
			return GetItemByIndex(ITI_SUPER_SHOTGUN);
		else if (HasAmmoQuick(ITI_SHELLS) && HasWeaponQuick(ITI_SHOTGUN))
			return GetItemByIndex(ITI_SHOTGUN);
		break;

	case GAME_Q1:
		if (ent->waterlevel <= 1 && HasAmmoQuick(ITI_CELLS) && HasWeaponQuick(ITI_Q1_THUNDERBOLT))
			return GetItemByIndex(ITI_Q1_THUNDERBOLT);
		else if (HasAmmoCountQuick(ITI_BULLETS, 2) && HasWeaponQuick(ITI_Q1_SUPER_NAILGUN))
			return GetItemByIndex(ITI_Q1_SUPER_NAILGUN);
		else if (HasAmmoCountQuick(ITI_SHELLS, 2) && HasWeaponQuick(ITI_Q1_SUPER_SHOTGUN))
			return GetItemByIndex(ITI_Q1_SUPER_SHOTGUN);
		else if (HasAmmoQuick(ITI_BULLETS) && HasWeaponQuick(ITI_Q1_NAILGUN))
			return GetItemByIndex(ITI_Q1_NAILGUN);
		else if (HasAmmoQuick(ITI_SHELLS) && HasWeaponQuick(ITI_Q1_SHOTGUN))
			return GetItemByIndex(ITI_Q1_SHOTGUN);
		break;

	case GAME_DOOM:
		if (HasWeaponQuick(ITI_DOOM_PLASMA_GUN) && HasAmmoQuick(ITI_CELLS))
			return GetItemByIndex(ITI_DOOM_PLASMA_GUN);
		else if (HasWeaponQuick(ITI_DOOM_SUPER_SHOTGUN) && HasAmmoCountQuick(ITI_SHELLS, 2))
			return GetItemByIndex(ITI_DOOM_SUPER_SHOTGUN);
		else if (HasWeaponQuick(ITI_DOOM_CHAINGUN) && HasAmmoQuick(ITI_BULLETS))
			return GetItemByIndex(ITI_DOOM_CHAINGUN);
		else if (HasWeaponQuick(ITI_DOOM_SHOTGUN) && HasAmmoQuick(ITI_SHELLS))
			return GetItemByIndex(ITI_DOOM_SHOTGUN);
		else if (HasWeaponQuick(ITI_DOOM_PISTOL) && HasAmmoQuick(ITI_BULLETS))
			return GetItemByIndex(ITI_DOOM_PISTOL);
		else if (HasWeaponQuick(ITI_DOOM_CHAINSAW))
			return GetItemByIndex(ITI_DOOM_CHAINSAW);
		else if (HasWeaponQuick(ITI_DOOM_ROCKET_LAUNCHER) && HasAmmoQuick(ITI_ROCKETS))
			return GetItemByIndex(ITI_DOOM_ROCKET_LAUNCHER);
		else if (HasWeaponQuick(ITI_DOOM_BFG) && HasAmmoCountQuick(ITI_CELLS, 40))
			return GetItemByIndex(ITI_DOOM_BFG);

		break;

	case GAME_DUKE:
		weap = ITI_DUKE_FOOT;

		for (i = 0; i < 10; ++i)
		{
			weap = dukeWeaponChoice[i];

			itemid_e ammo = game_iteminfos[GAME_DUKE].dynamic.weapon_uses_this_ammo[weap];

			if (HasWeaponQuick(weap) && (ammo == ITI_NULL || HasAmmoQuick(ammo)))
				break;
		}

		return GetItemByIndex(weap);
	}

	return GetItemByIndex(ITI_BLASTER);
}

void AttemptBetterWeaponSwap(edict_t *ent)
{
	if (ShouldSwapToPotentiallyBetterWeapon(ent, NULL, false))
		NoAmmoWeaponChange(ent, GUN_MAIN);
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange(edict_t *ent, gunindex_e gun)
{
	gitem_t *best = GetBestWeapon(ent, NULL, false);

	if (!best || best == ent->client->pers.weapon)
		return;

	ent->client->gunstates[gun].newweapon = best;
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Weapon_Duke_Offhand_Foot(edict_t *ent);

void Think_Weapon(edict_t *ent)
{
	// if just died, put the weapon away
	if (ent->health < 1)
	{
		for (int i = 0; i < MAX_PLAYER_GUNS; ++i)
		{
			ent->client->gunstates[i].newweapon = NULL;
			ChangeWeapon(ent, i);
		}
	}

	// call active weapon think routine
	if (ent->client->pers.weapon && ent->client->pers.weapon->weaponthink)
	{
		is_quad = (ent->client->quad_time > level.time);

		if (ent->client->silencer_shots)
			is_silenced = MZ_SILENCED;
		else
			is_silenced = 0;

		ent->client->pers.weapon->weaponthink(ent, GUN_MAIN);

		if (ent->s.game == GAME_DUKE)
			Weapon_Duke_Offhand_Foot(ent);
	}
}


/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon(edict_t *ent, gitem_t *item)
{
	itemid_e    ammo_index;
	gitem_t     *ammo_item;

	// see if we're already using it
	// and we're not pipebombs, which Duke can always switch to
	if (item == ent->client->pers.weapon && !(ent->s.game == GAME_DUKE && GetIndexByItem(item) == ITI_DUKE_PIPEBOMBS))
		return;

	ammo_index = game_iteminfos[ent->s.game].dynamic.weapon_uses_this_ammo[GetIndexByItem(item)];

	if (ammo_index != ITI_NULL && !g_select_empty->value && !(item->flags & IT_AMMO) && game_iteminfos[ent->s.game].dynamic.weapon_usage_counts[ammo_index] != 0)
	{
		ammo_item = ResolveItemRedirect(ent, GetItemByIndex(ammo_index));
		ammo_index = ITEM_INDEX(ammo_item);

		if (!ent->client->pers.inventory[ammo_index]) {
			gi.cprintf(ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}

		if (ent->client->pers.inventory[ammo_index] < GetWeaponUsageCount(ent, item)) {
			gi.cprintf(ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}
	}

	// change to this weapon when down
	ent->client->gunstates[GUN_MAIN].newweapon = item;
}

/*
================
Drop_Weapon
================
*/
void Drop_Weapon(edict_t *ent, gitem_t *item)
{
	int     index;

	if ((int)(dmflags->value) & DF_WEAPONS_STAY)
		return;

	index = ITEM_INDEX(item);
	// see if we're already using it
	if (((item == ent->client->pers.weapon) || (item == ent->client->gunstates[GUN_MAIN].newweapon)) && (ent->client->pers.inventory[index] == 1)) {
		gi.cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	Drop_Item(ent, item);
	ent->client->pers.inventory[index]--;
}


/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
#define FRAME_FIRE_FIRST        (FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST        (FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST  (FRAME_IDLE_LAST + 1)

void Weapon_Generic(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int *pause_frames, int *fire_frames, void(*fire)(edict_t *ent, gunindex_e gun), gunindex_e gun)
{
	int     n;

	if (ent->deadflag || !ent->solid || ent->freeze_time > level.time) { // VWep animations screw up corpses
		return;
	}

/*	if (ent->client->next_weapon_update > ent->client->player_time)
		return;

	ent->client->next_weapon_update = ent->client->player_time + 100;*/

	if (ent->client->gunstates[gun].weaponstate == WEAPON_DROPPING) {
		if (ent->client->ps.guns[gun].frame == FRAME_DEACTIVATE_LAST) {
			ChangeWeapon(ent, gun);
			return;
		}
		else if ((FRAME_DEACTIVATE_LAST - ent->client->ps.guns[gun].frame) == 4) {
			ent->client->anim_priority = ANIM_WAVE | ANIM_REVERSE;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
				ent->s.frame = FRAME_crpain4 + 1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else {
				ent->s.frame = FRAME_pain304 + 1;
				ent->client->anim_end = FRAME_pain301;

			}
		}

		ent->client->ps.guns[gun].frame++;
		return;
	}

	if (ent->client->gunstates[gun].weaponstate == WEAPON_ACTIVATING) {
		if (ent->client->ps.guns[gun].frame == FRAME_ACTIVATE_LAST) {
			ent->client->gunstates[gun].weaponstate = WEAPON_READY;
			ent->client->ps.guns[gun].frame = FRAME_IDLE_FIRST;
			return;
		}

		ent->client->ps.guns[gun].frame++;
		return;
	}

	if ((ent->client->gunstates[gun].newweapon) && (ent->client->gunstates[gun].weaponstate != WEAPON_FIRING)) {
		ent->client->gunstates[gun].weaponstate = WEAPON_DROPPING;
		ent->client->ps.guns[gun].frame = FRAME_DEACTIVATE_FIRST;

		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4) {
			ent->client->anim_priority = ANIM_WAVE | ANIM_REVERSE;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
				ent->s.frame = FRAME_crpain4 + 1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else {
				ent->s.frame = FRAME_pain304 + 1;
				ent->client->anim_end = FRAME_pain301;

			}
		}
		return;
	}

	if (ent->client->gunstates[gun].weaponstate == WEAPON_READY) {
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)) {
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if ((!ent->client->gunstates[gun].ammo_index) ||
				(ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] >= GetWeaponUsageCount(ent, ent->client->pers.weapon))) {
				ent->client->ps.guns[gun].frame = FRAME_FIRE_FIRST;
				ent->client->gunstates[gun].weaponstate = WEAPON_FIRING;

				// start the animation
				ent->client->anim_priority = ANIM_ATTACK;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
					ent->s.frame = FRAME_crattak1 - 1;
					ent->client->anim_end = FRAME_crattak9;
				}
				else {
					ent->s.frame = FRAME_attack1 - 1;
					ent->client->anim_end = FRAME_attack8;
				}
			}
			else {
				if (level.time >= ent->pain_debounce_time) {
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1000;
				}
				NoAmmoWeaponChange(ent, gun);
			}
		}
		else {
			if (ent->client->ps.guns[gun].frame == FRAME_IDLE_LAST) {
				ent->client->ps.guns[gun].frame = FRAME_IDLE_FIRST;
				return;
			}

			if (pause_frames) {
				for (n = 0; pause_frames[n]; n++) {
					if (ent->client->ps.guns[gun].frame == pause_frames[n]) {
						if (Q_rand() & 15)
							return;
					}
				}
			}

			ent->client->ps.guns[gun].frame++;
			return;
		}
	}

	if (ent->client->gunstates[gun].weaponstate == WEAPON_FIRING) {
		for (n = 0; fire_frames[n]; n++) {
			if (ent->client->ps.guns[gun].frame == fire_frames[n]) {
				Weapon_QuadDamage(ent);

				fire(ent, gun);
				break;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.guns[gun].frame++;

		if (ent->client->ps.guns[gun].frame == FRAME_IDLE_FIRST + 1)
			ent->client->gunstates[gun].weaponstate = WEAPON_READY;
	}
}


/*
======================================================================

GRENADE

======================================================================
*/

/*
=================
fire_grenade
=================
*/
void Pipe_Explode(edict_t *ent)
{
	ent->takedamage = DAMAGE_NO;

	vec3_t      origin;

	if (ent->activator->client)
		PlayerNoise(ent->activator, ent->s.origin, PNOISE_IMPACT);

	ent->owner = ent->activator;
	T_RadiusDamage(ent, ent->activator, ent->dmg, ent, DAMAGE_NO_PARTICLES | DAMAGE_DUKE, ent->dmg_radius, ent->meansOfDeath);

	VectorMA(ent->s.origin, -0.02, ent->velocity, origin);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DUKE_EXPLODE_PIPE);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

	G_FreeEdict(ent->chain);
	G_FreeEdict(ent);
}

void Pipe_Touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent->chain);
		G_FreeEdict(ent);
		return;
	}

	if (VectorLengthSquared(ent->velocity))
	{
		if (other != ent->activator)
			gi.sound(ent, CHAN_VOICE, gi.soundindex("duke/PBOMBBNC.wav"), 1, ATTN_NORM, 0);
	}
}

void PipeTrigger_Touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent)
		return;

	if (other->client && level.time > ent->chain->last_move_time)
	{
		edict_t *pipe = ent->chain;

		do_propagate = false;
		Touch_Item(pipe, other, plane, surf);
		do_propagate = true;

		if (!pipe->inuse)
			G_FreeEdict(ent);
	}
}

void Pipe_Think(edict_t *self)
{
	VectorCopy(self->s.origin, self->chain->s.origin);
	gi.linkentity(self->chain);

	self->nextthink = level.time + game.frametime;

	if (VectorLengthSquared(self->velocity))
		self->s.frame = (self->s.frame + 1) % 2;

	if (!self->activator || !self->activator->client || !self->activator->inuse || !self->activator->client->pers.connected)
		return;

	if (GetIndexByItem(self->activator->client->pers.weapon) == ITI_DUKE_PIPEBOMBS &&
		self->activator->client->pers.alt_weapon == true &&
		self->activator->client->ps.guns[GUN_MAIN].frame == 1)
		Pipe_Explode(self);
}

void pipe_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (attacker->client || (attacker->svflags & SVF_MONSTER))
		self->activator = attacker;

	Pipe_Explode(self);
}

void fire_pipe(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float damage_radius)
{
	edict_t *grenade;
	vec3_t  dir;
	vec3_t  forward, right, up;

	vectoangles(aimdir, dir);
	AngleVectors(dir, forward, right, up);

	grenade = G_Spawn();
	VectorCopy(start, grenade->s.origin);
	VectorScale(aimdir, speed, grenade->velocity);
	VectorMA(grenade->velocity, 250, up, grenade->velocity);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = CONTENTS_SOLID | CONTENTS_BULLETS;
	grenade->s.clip_contents = CONTENTS_BULLETS;
	grenade->solid = SOLID_BBOX;
	grenade->health = 1;
	grenade->takedamage = DAMAGE_YES;
	grenade->die = pipe_die;
	grenade->s.game = GAME_DUKE;
	VectorSet(grenade->mins, -8, -8, -4);
	VectorSet(grenade->maxs, 8, 8, 0);
	grenade->spawnflags |= DROPPED_ITEM | DROPPED_PLAYER_ITEM;
	grenade->item = grenade->real_item = GetItemByIndex(ITI_DUKE_PIPEBOMBS);
	grenade->count = 1;
	grenade->s.modelindex = gi.modelindex("%e_pipebomb");
	grenade->activator = self;
	grenade->touch = Pipe_Touch;
	grenade->nextthink = level.time + game.frametime;
	grenade->think = Pipe_Think;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->entitytype = ET_GRENADE;
	grenade->last_move_time = level.time + 500;

	if (self->client)
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), grenade, DT_DIRECT);
	else
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, ITI_GRENADE_LAUNCHER, grenade, DT_DIRECT);

	gi.linkentity(grenade);

	edict_t *trigger;

	trigger = G_Spawn();
	trigger->clipmask = 0;
	trigger->s.clip_contents = CONTENTS_PLAYERCLIP;
	trigger->solid = SOLID_TRIGGER;
	VectorCopy(grenade->mins, trigger->mins);
	VectorCopy(grenade->maxs, trigger->maxs);
	trigger->chain = grenade;
	grenade->chain = trigger;
	trigger->svflags = SVF_NOCLIENT;
	trigger->touch = PipeTrigger_Touch;
	VectorCopy(grenade->s.origin, trigger->s.origin);

	gi.linkentity(trigger);
}

void weapon_duke_pipebomb_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame == 3)
	{
		if (ent->client->buttons & BUTTON_ATTACK)
		{
			if (ent->client->pers.pipebomb_hold < 10)
				ent->client->pers.pipebomb_hold++;

			return;
		}

		if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
		{
			if (!((int)dmflags->value & DF_INFINITE_AMMO))
				ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

			vec3_t start, forward, right, offset;

			AngleVectors(ent->client->v_angle, forward, NULL, NULL);
			VectorSet(offset, 0, 0, ent->viewheight - 8);
			P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

			fire_pipe(ent, start, forward, 150, 250 + (ent->client->pers.pipebomb_hold * 25), 450);
		}
	}

	ent->client->pers.pipebomb_hold = 0;
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame >= 9)
	{
		ent->client->pers.alt_weapon = true;

		ent->client->ps.guns[gun].offset[2] = DOOM_ANIM_FRAC;
		ent->client->ps.guns[gun].index = pipe_detonator_index;
		ent->client->ps.guns[gun].frame = 0;
		ent->client->gunstates[gun].weaponstate = WEAPON_ACTIVATING;
		ent->last_move_time = 0;
	}
}

void weapon_duke_detonator_fire(edict_t *ent, gunindex_e gun, bool first)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame >= 3)
	{
		if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
			ent->client->gunstates[gun].newweapon = ent->client->pers.weapon;
		else
			NoAmmoWeaponChange(ent, gun);
	}
}

void Weapon_Grenade(edict_t *ent, gunindex_e gun)
{
	if (ent->s.game == GAME_DUKE)
	{
		if (ent->client->pers.alt_weapon)
			Weapon_Doom(ent, 3, weapon_duke_detonator_fire, gun);
		else
			Weapon_Doom(ent, 9, weapon_duke_pipebomb_fire, gun);

		return;
	}

	if ((ent->client->gunstates[gun].newweapon) && (ent->client->gunstates[gun].weaponstate == WEAPON_READY))
	{
		ChangeWeapon(ent, gun);
		return;
	}

	if (ent->client->next_weapon_update > ent->client->player_time)
	{
		if (!(ent->client->gunstates[gun].weaponstate == WEAPON_FIRING &&
			ent->client->ps.guns[gun].frame == 11 &&
			!(level.time >= ent->client->grenade_time)))
			return;
	}

	ent->client->next_weapon_update = ent->client->player_time + 1;

	if (ent->client->gunstates[gun].weaponstate == WEAPON_ACTIVATING) {
		ent->client->gunstates[gun].weaponstate = WEAPON_READY;
		ent->client->ps.guns[gun].frame = 16;
		return;
	}

	if (ent->client->gunstates[gun].weaponstate == WEAPON_READY) {
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)) {
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]) {
				ent->client->ps.guns[gun].frame = 1;
				ent->client->gunstates[gun].weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0;
			}
			else {
				if (level.time >= ent->pain_debounce_time) {
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1000;
				}
				NoAmmoWeaponChange(ent, gun);
			}
			return;
		}

		if ((ent->client->ps.guns[gun].frame == 29) || (ent->client->ps.guns[gun].frame == 34) || (ent->client->ps.guns[gun].frame == 39) || (ent->client->ps.guns[gun].frame == 48)) {
			if (Q_rand() & 15)
				return;
		}

		if (++ent->client->ps.guns[gun].frame > 48)
			ent->client->ps.guns[gun].frame = 16;
		return;
	}

	if (ent->client->gunstates[gun].weaponstate == WEAPON_FIRING) {
		if (ent->client->ps.guns[gun].frame == 5)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);

		if (ent->client->ps.guns[gun].frame == 11) {
			if (!ent->client->grenade_time) {
				ent->client->grenade_time = level.time + GRENADE_TIMER + 200;
				ent->client->weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
			}

			// they waited too long, detonate it in their hand
			if (!ent->client->grenade_blew_up && level.time >= ent->client->grenade_time) {
				ent->client->weapon_sound = 0;
				weapon_grenade_fire(ent, gun, true);
				ent->client->grenade_blew_up = true;
			}

			if (ent->client->buttons & BUTTON_ATTACK)
				return;

			if (ent->client->grenade_blew_up) {
				if (level.time >= ent->client->grenade_time) {
					ent->client->ps.guns[gun].frame = 15;
					ent->client->grenade_blew_up = false;
				}
				else {
					return;
				}
			}
		}

		if (ent->client->ps.guns[gun].frame == 12) {
			ent->client->weapon_sound = 0;
			weapon_grenade_fire(ent, gun, false);
		}

		if ((ent->client->ps.guns[gun].frame == 15) && (level.time < ent->client->grenade_time))
			return;

		ent->client->ps.guns[gun].frame++;

		if (ent->client->ps.guns[gun].frame == 16) {
			ent->client->grenade_time = 0;
			ent->client->gunstates[gun].weaponstate = WEAPON_READY;
		}
	}
}




/*
=================
fire_grenade
=================
*/
void Q1Grenade_Explode(edict_t *ent)
{
	vec3_t      origin;

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	T_RadiusDamage(ent, ent->owner, ent->dmg, ent, DAMAGE_Q1, ent->dmg, ent->meansOfDeath);

	VectorMA(ent->s.origin, -0.02, ent->velocity, origin);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_Q1_EXPLODE);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

	G_FreeEdict(ent);
}

void Q1Grenade_Touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY)) {
		G_FreeEdict(ent);
		return;
	}

	if (!other->takedamage)
	{
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("q1/weapons/bounce.wav"), 1, ATTN_NORM, 0);
		return;
	}

	ent->enemy = other;
	Q1Grenade_Explode(ent);
}

void fire_q1_grenade(edict_t *self, vec3_t start, vec3_t aimdir, int damage, float timer)
{
	edict_t *grenade;
	vec3_t  dir;
	vec3_t  forward, right, up;

	vectoangles(aimdir, dir);
	AngleVectors(dir, forward, right, up);

	grenade = G_Spawn();
	VectorCopy(start, grenade->s.origin);
	VectorScale(aimdir, 600, grenade->velocity);
	VectorMA(grenade->velocity, 200 + crandom() * 10.0f, up, grenade->velocity);
	VectorMA(grenade->velocity, crandom() * 10.0f, right, grenade->velocity);
	VectorSet(grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT;
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	grenade->s.game = GAME_Q1;
	VectorClear(grenade->mins);
	VectorClear(grenade->maxs);
	grenade->s.modelindex = gi.modelindex("models/q1/grenade.mdl");
	grenade->owner = self;
	grenade->touch = Q1Grenade_Touch;
	grenade->nextthink = level.time + (timer * 1000);
	grenade->think = Q1Grenade_Explode;
	grenade->dmg = damage;
	grenade->entitytype = ET_GRENADE;

	if (self->client)
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), grenade, DT_DIRECT);
	else
		grenade->meansOfDeath = MakeAttackerMeansOfDeath(self, grenade, MD_NONE, DT_DIRECT);

	gi.linkentity(grenade);
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire(edict_t *ent, gunindex_e gun)
{
	vec3_t  offset;
	vec3_t  forward, right;
	vec3_t  start;
	int     damage = 120;
	float   radius;

	radius = damage + 40;
	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -1;
	ent->client->gunstates[gun].kick_time = level.time + 100;

	fire_grenade(ent, start, forward, damage, 600, 2.5, radius);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_GRENADE | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.guns[gun].frame++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;
}

void weapon_q1_gl_fire(edict_t *ent, gunindex_e gun)
{
	if (ent->client->ps.guns[gun].frame != 0)
	{
		if (ent->client->ps.guns[gun].frame >= 6 && (ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] && !ent->client->gunstates[gun].newweapon)
			ent->client->ps.guns[gun].frame = 0;
		else
		{
			ent->client->ps.guns[gun].frame++;
			return;
		}
	}

	vec3_t  offset;
	vec3_t  forward, right;
	vec3_t  start;
	int     damage = 120;

	if (is_quad)
		damage *= 4;

	VectorSet(offset, 0, 0, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_q1_grenade(ent, start, forward, damage, 2.5);
	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_GRENADE | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.guns[gun].frame++;

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;

	Weapon_Q1_Anim_Rock(ent);
	ent->client->gunstates[gun].kick_angles[0] = -1;
	ent->client->gunstates[gun].kick_time = level.time + 100;
}

void duke_rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY)) {
		G_FreeEdict(ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

	// calculate position for the explosion entity
	T_RadiusDamage(ent, ent->owner, ent->radius_dmg, ent, DAMAGE_NO_PARTICLES, ent->dmg_radius, ent->meansOfDeath);

	VectorNormalize(ent->velocity);
	VectorMA(ent->s.origin, -8, ent->velocity, origin);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(ent->spawnflags == 1 ? TE_DUKE_EXPLODE_SMALL : TE_DUKE_EXPLODE);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

	G_FreeEdict(ent);
}

void fire_duke_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int radius_damage, int speed, bool tiny)
{
	edict_t *rocket;

	rocket = G_Spawn();
	VectorCopy(start, rocket->s.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->s.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.game = GAME_DUKE;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = tiny ? gi.modelindex("%e_smallrocket") : gi.modelindex("%e_rocket");
	rocket->owner = self;
	rocket->touch = duke_rocket_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->radius_dmg = radius_damage;
	rocket->s.frame = 3;
	rocket->entitytype = ET_ROCKET;

	if (tiny)
	{
		rocket->spawnflags = 1;
		rocket->dmg_radius = radius_damage + 120;
	}
	else
		rocket->dmg_radius = radius_damage + 400;

	if (self->client)
		check_dodge(self, rocket->s.origin, dir, speed);

	if (self->client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	gi.linkentity(rocket);
}

void weapon_duke_devastate_fire(edict_t *ent, gunindex_e gun, bool first)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 3 && (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] < 2 || ent->client->gunstates[gun].newweapon))
	{
		ent->client->ps.guns[gun].frame = 6;
		return;
	}
	else if (ent->client->ps.guns[gun].frame >= 6 && (ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] >= 2 && !ent->client->gunstates[gun].newweapon)
		ent->client->ps.guns[gun].frame = 1;

	if (ent->client->ps.guns[gun].frame == 1 || ent->client->ps.guns[gun].frame == 3)
	{
		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_GRENADE | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		vec3_t start, forward, right, offset;
		vec3_t angles;

		for (int i = 0; i < 2; ++i)
		{
			if (!ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
				break;

			VectorCopy(ent->client->v_angle, angles);
			angles[0] += crandom() * 2;
			angles[1] += crandom() * 2;

			AngleVectors(angles, forward, right, NULL);
			VectorSet(offset, 0, ent->client->ps.guns[gun].frame == 1 ? 8 : -8, ent->viewheight - 8);
			P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

			fire_duke_rocket(ent, start, forward, 38, 50, 850 - i * 50, true);

			if (!((int)dmflags->value & DF_INFINITE_AMMO))
				ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;
		}
	}
}

void Weapon_GrenadeLauncher(edict_t *ent, gunindex_e gun)
{
	switch (ent->s.game)
	{
	case GAME_Q2:
	{
		static int  pause_frames[] = { 34, 51, 59, 0 };
		static int  fire_frames[] = { 6, 0 };

		Weapon_Generic(ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_grenadelauncher_fire, gun);
	}
	break;
	case GAME_Q1:
		Weapon_Q1(ent, 7, weapon_q1_gl_fire, gun);
		break;
	case GAME_DUKE:
		Weapon_Doom(ent, 5, weapon_duke_devastate_fire, gun);
		break;
	}
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire(edict_t *ent, gunindex_e gun)
{
	vec3_t  offset, start;
	vec3_t  forward, right;
	int     damage;
	float   damage_radius;
	int     radius_damage;

	damage = 100 + (int)(random() * 20.0f);
	radius_damage = 120;
	damage_radius = 120;
	if (is_quad) {
		damage *= 4;
		radius_damage *= 4;
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -1;
	ent->client->gunstates[gun].kick_time = level.time + 100;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	fire_rocket(ent, start, forward, damage, 650, damage_radius, radius_damage);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_ROCKET | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.guns[gun].frame++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;
}

/*
=================
fire_rocket
=================
*/
void q1_rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY)) {
		G_FreeEdict(ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	// calculate position for the explosion entity
	int damg = ent->dmg + random() * (20 * (120 / ent->dmg));

	if (other->takedamage)
	{
		if (other->entitytype == ET_Q1_MONSTER_SHAMBLER)
			damg = damg * 0.5;	// mostly immune

		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, damg, 0, DAMAGE_Q1, ent->meansOfDeath);
	}

	T_RadiusDamage(ent, ent->owner, ent->dmg, other, DAMAGE_Q1, ent->dmg_radius, ent->meansOfDeath);

	VectorNormalize(ent->velocity);
	VectorMA(ent->s.origin, -8, ent->velocity, origin);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_Q1_EXPLODE);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

	G_FreeEdict(ent);
}

void fire_q1_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int damage_radius, int speed)
{
	edict_t *rocket;

	rocket = G_Spawn();
	VectorCopy(start, rocket->s.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->s.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= EF_ROCKET;
	rocket->s.game = GAME_Q1;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = gi.modelindex("%e_rocket");
	rocket->owner = self;
	rocket->touch = q1_rocket_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->dmg_radius = damage_radius;
	rocket->entitytype = ET_ROCKET;

	if (self->client)
		check_dodge(self, rocket->s.origin, dir, speed);

	if (self->client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	gi.linkentity(rocket);
}

void weapon_q1_rl_fire(edict_t *ent, gunindex_e gun)
{
	if (ent->client->ps.guns[gun].frame != 0)
	{
		ent->client->ps.guns[gun].frame++;
		return;
	}

	vec3_t  offset;
	vec3_t  forward, right;
	vec3_t  start;
	int     damage = 120;

	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 0, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_q1_rocket(ent, start, forward, damage, 120, 1000);
	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_ROCKET | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.guns[gun].frame++;

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;

	ent->attack_finished_time = ent->client->player_time + 700;
	Weapon_Q1_Anim_Rock(ent);
	ent->client->gunstates[gun].kick_angles[0] = -1;
}


/*
=================
fire_rocket
=================
*/
void doom_rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY)) {
		G_FreeEdict(ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

	// calculate position for the explosion entity
	T_RadiusDamage(ent, ent->owner, ent->radius_dmg, ent, DAMAGE_NO_PARTICLES, ent->dmg_radius, ent->meansOfDeath);

	VectorNormalize(ent->velocity);
	VectorMA(ent->s.origin, -8, ent->velocity, origin);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DOOM_EXPLODE);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

	G_FreeEdict(ent);
}

void fire_doom_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int radius_damage, int speed)
{
	edict_t *rocket;

	rocket = G_Spawn();
	VectorCopy(start, rocket->s.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->s.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.game = GAME_DOOM;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = gi.modelindex("%e_rocket");
	rocket->owner = self;
	rocket->touch = doom_rocket_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->radius_dmg = radius_damage / 2.0f;
	rocket->dmg_radius = radius_damage + sqrtf(400);
	rocket->s.frame = 3;
	rocket->entitytype = ET_ROCKET;

	if (self->client)
		check_dodge(self, rocket->s.origin, dir, speed);

	if (self->client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	gi.linkentity(rocket);
}

int Doom_MissileDamageRandomizer(int dmg)
{
	return ((Q_rand() % 8) + 1) * dmg;
}

void weapon_doom_rocket_fire(edict_t *ent, gunindex_e gun, bool first)
{
	bool fire = false;

	if (ent->client->ps.guns[gun].frame >= 7 && (ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] && !ent->client->gunstates[gun].newweapon)
	{
		ent->client->ps.guns[gun].frame = 1;
		fire = true;
	}

	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 2 || fire)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_ROCKET | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

		vec3_t start, forward, right, offset;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		fire_doom_rocket(ent, start, forward, Doom_MissileDamageRandomizer(20), 128, 650);
		Weapon_Doom_Anim_Regular(ent);
	}
}

void weapon_duke_rocket_fire(edict_t *ent, gunindex_e gun, bool first)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 1)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_ROCKET | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

		vec3_t start, forward, right, offset;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		fire_duke_rocket(ent, start, forward, 140, sqrtf(1780) * 2, 850, false);
		Weapon_Doom_Anim_Regular(ent);
	}
}

void Weapon_RocketLauncher(edict_t *ent, gunindex_e gun)
{
	switch (ent->s.game)
	{
	case GAME_Q2:
	{
		static int  pause_frames[] = { 25, 33, 42, 50, 0 };
		static int  fire_frames[] = { 5, 0 };

		Weapon_Generic(ent, 4, 12, 50, 54, pause_frames, fire_frames, Weapon_RocketLauncher_Fire, gun);
	}
	break;
	case GAME_Q1:
		Weapon_Q1(ent, 7, weapon_q1_rl_fire, gun);
		break;
	case GAME_DOOM:
		Weapon_Doom(ent, 8, weapon_doom_rocket_fire, gun);
		break;
	case GAME_DUKE:
		Weapon_Doom(ent, 8, weapon_duke_rocket_fire, gun);
		break;
	}
}


/*
======================================================================

BLASTER / HYPERBLASTER

======================================================================
*/

void Blaster_Fire(edict_t *ent, gunindex_e gun, vec3_t g_offset, int damage, bool hyper, int effect)
{
	vec3_t  forward, right;
	vec3_t  start;
	vec3_t  offset;

	if (is_quad)
		damage *= 4;
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight - 8);
	VectorAdd(offset, g_offset, offset);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -1;
	ent->client->gunstates[gun].kick_time = level.time + 100;

	fire_blaster(ent, start, forward, damage, 1000, effect, hyper);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	if (hyper)
		gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte(MZ_BLASTER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}


void Weapon_Blaster_Fire(edict_t *ent, gunindex_e gun)
{
	int     damage;

	if (deathmatch->value)
		damage = 15;
	else
		damage = 10;
	Blaster_Fire(ent, gun, vec3_origin, damage, false, EF_BLASTER);
	ent->client->ps.guns[gun].frame++;
}

void weapon_q1_axe_fire(edict_t *ent, gunindex_e gun)
{
	if (ent->client->ps.guns[gun].frame == 0)
	{
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("q1/weapons/ax1.wav"), 1, ATTN_NORM, 0);

		if (random() < 0.5f)
			ent->client->ps.guns[gun].frame = 0;
		else
			ent->client->ps.guns[gun].frame = 4;

		int start_anim;

		switch (Q_rand() % 4)
		{
		case 0:
		default:
			start_anim = 119;
			break;
		case 1:
			start_anim = 125;
			break;
		case 2:
			start_anim = 131;
			break;
		case 3:
			start_anim = 137;
			break;
		}

		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = start_anim - 1;
		ent->client->anim_end = start_anim + 4;
		Weapon_QuadDamage(ent);
	}
	else if (ent->client->ps.guns[gun].frame == 4 || ent->client->ps.guns[gun].frame == 8)
	{
		ent->client->ps.guns[gun].frame = 9;
		return;
	}

	if (ent->client->ps.guns[gun].frame == 2 || ent->client->ps.guns[gun].frame == 6)
	{
		vec3_t source, org;
		vec3_t forward;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorCopy(ent->s.origin, source);
		source[2] += ent->viewheight - 8;

		VectorMA(source, 64, forward, org);

		trace_t tr = gi.trace(source, vec3_origin, vec3_origin, org, ent, MASK_SHOT);

		if (tr.fraction != 1.0f)
		{
			VectorMA(tr.endpos, -4, forward, org);

			if (tr.ent->takedamage)
				T_Damage(tr.ent, ent, ent, vec3_origin, vec3_origin, vec3_origin, 20, 0, DAMAGE_Q1, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
			else
			{
				// hit wall
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("q1/player/axhit2.wav"), 1, ATTN_NORM, 0);

				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_Q1_GUNSHOT);
				gi.WriteByte(3);
				gi.WritePosition(org);
				gi.multicast(org, MULTICAST_PVS);
			}
		}
	}

	ent->client->ps.guns[gun].frame++;
}

void weapon_doom_punch_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame == 5 && (ent->client->buttons & BUTTON_ATTACK) && !ent->client->gunstates[gun].newweapon)
		ent->client->ps.guns[gun].frame = 1;
	else
		ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 3)
	{
		vec3_t source, org;
		vec3_t forward;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorCopy(ent->s.origin, source);
		source[2] += ent->viewheight - 8;

		VectorMA(source, 64, forward, org);

		trace_t tr = gi.trace(source, vec3_origin, vec3_origin, org, ent, MASK_SHOT);

		if (tr.fraction != 1.0f)
		{
			VectorMA(tr.endpos, -4, forward, org);

			if (tr.ent->takedamage)
			{
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_DOOM_BLOOD);
				gi.WritePosition(org);
				gi.multicast(org, MULTICAST_PVS);

				int damage = (Q_rand() % 10 + 1) << 1;

				if (ent->client->quad_time > level.time)
					damage *= 10;

				vec3_t dir;

				VectorSubtract(tr.ent->s.origin, ent->s.origin, dir);
				vectoangles(dir, dir);
				dir[0] = 0;
				dir[2] = 0;
				AngleVectors(dir, dir, NULL, NULL);

				T_Damage(tr.ent, ent, ent, dir, vec3_origin, vec3_origin, damage, (ent->client->quad_time > level.time) ? damage : 0, DAMAGE_NO_PARTICLES, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/PUNCH.wav"), 1, ATTN_NORM, 0);
			}
			else
			{
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_DOOM_GUNSHOT);
				gi.WritePosition(org);
				gi.multicast(org, MULTICAST_PVS);
			}
		}

		Weapon_Doom_Anim_Regular(ent);
	}
}

void weapon_duke_foot_fire_generic(edict_t *ent, gunindex_e gun)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame >= 3 && ent->client->ps.guns[gun].frame <= 5)
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = 6;
		ent->client->anim_end = 7;
	}
	else
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = 5;
		ent->client->anim_end = 6;
	}

	if (ent->client->ps.guns[gun].frame == 3)
	{
		vec3_t source, org;
		vec3_t forward;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorCopy(ent->s.origin, source);
		source[2] += ent->viewheight - 8;

		VectorMA(source, 64, forward, org);

		trace_t tr = gi.trace(source, vec3_origin, vec3_origin, org, ent, MASK_SHOT);

		if (tr.fraction != 1.0f)
		{
			VectorMA(tr.endpos, -4, forward, org);

			if (tr.ent->takedamage)
			{
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_DUKE_BLOOD);
				gi.WritePosition(org);
				gi.multicast(org, MULTICAST_PVS);

				int damage = (ent->client->quad_time > level.time) ? 42 : 12;

				vec3_t dir;

				VectorSubtract(tr.ent->s.origin, ent->s.origin, dir);
				vectoangles(dir, dir);
				dir[0] = 0;
				dir[2] = 0;
				AngleVectors(dir, dir, NULL, NULL);

				T_Damage(tr.ent, ent, ent, dir, vec3_origin, vec3_origin, damage, damage, DAMAGE_NO_PARTICLES, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
			}

			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_DUKE_GUNSHOT);
			gi.WritePosition(org);
			gi.multicast(org, MULTICAST_PVS);

			gi.sound(ent, CHAN_WEAPON, gi.soundindex("duke/KICKHIT.wav"), 1, ATTN_NORM, 0);
		}
	}
}

void weapon_duke_foot_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame == 5 && (ent->client->buttons & BUTTON_ATTACK) && !ent->client->gunstates[gun].newweapon)
		ent->client->ps.guns[gun].frame = 0;

	weapon_duke_foot_fire_generic(ent, gun);
}

void weapon_duke_offfoot_fire(edict_t *ent, gunindex_e gun, bool first)
{
	weapon_duke_foot_fire_generic(ent, gun);

	if (ent->client->ps.guns[gun].frame == 5)
	{
		ent->client->ps.guns[gun].index = 0;
		ent->client->ps.guns[gun].frame = 0;
	}
}

void Weapon_Duke_Offhand_Foot(edict_t *ent)
{
	if (ent->deadflag || !ent->solid || ent->freeze_time > level.time)
		return;

	if (ent->client->ps.guns[GUN_OFFHAND].index != foot_index)
	{
		vec3_t source, org;
		vec3_t forward;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorCopy(ent->s.origin, source);
		source[2] += ent->viewheight - 8;

		VectorMA(source, 64, forward, org);

		trace_t tr = gi.trace(source, vec3_origin, vec3_origin, org, ent, MASK_SHOT);

		if (tr.fraction != 1.0f)
		{
			VectorMA(tr.endpos, -4, forward, org);

			if (tr.ent->takedamage && tr.ent->freeze_time > level.time)
			{
				ent->client->ps.guns[GUN_OFFHAND].index = foot_index;
				ent->client->ps.guns[GUN_OFFHAND].frame = 0;
				ent->client->ps.guns[GUN_OFFHAND].offset[1] = 0;
				ent->client->gunstates[GUN_OFFHAND].weaponstate = WEAPON_FIRING;
			}
		}
	}

	if (ent->client->ps.guns[GUN_OFFHAND].index == foot_index)
	{
		Weapon_Doom(ent, 7, weapon_duke_offfoot_fire, GUN_OFFHAND);
		ent->client->ps.guns[GUN_OFFHAND].offset[1] = 0;
	}
}

void Weapon_Blaster(edict_t *ent, gunindex_e gun)
{
	switch (ent->s.game)
	{
	case GAME_Q2:
	{
		static int  pause_frames[] = { 19, 32, 0 };
		static int  fire_frames[] = { 5, 0 };

		Weapon_Generic(ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Blaster_Fire, gun);
		break;
	}
	case GAME_Q1:
		Weapon_Q1(ent, 9, weapon_q1_axe_fire, gun);
		break;
	case GAME_DOOM:
		Weapon_Doom(ent, 6, weapon_doom_punch_fire, gun);
		break;
	case GAME_DUKE:
		Weapon_Doom(ent, 7, weapon_duke_foot_fire, gun);
		break;
	}
}

void Weapon_HyperBlaster_Fire(edict_t *ent, gunindex_e gun)
{
	float   rotation;
	vec3_t  offset;
	int     effect;
	int     damage;

	ent->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");

	if (!(ent->client->buttons & BUTTON_ATTACK)) {
		ent->client->ps.guns[gun].frame++;
	}
	else {
		if (!ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]) {
			if (level.time >= ent->pain_debounce_time) {
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent->pain_debounce_time = level.time + 1000;
			}
			NoAmmoWeaponChange(ent, gun);
		}
		else {
			rotation = (ent->client->ps.guns[gun].frame - 5) * 2 * M_PI / 6;
			offset[0] = -4 * sinf(rotation);
			offset[1] = 0;
			offset[2] = 4 * cosf(rotation);

			if ((ent->client->ps.guns[gun].frame == 6) || (ent->client->ps.guns[gun].frame == 9))
				effect = EF_HYPERBLASTER;
			else
				effect = 0;

			if (deathmatch->value)
				damage = 15;
			else
				damage = 20;
			
			Blaster_Fire(ent, gun, offset, damage, true, effect);
			
			if (!((int)dmflags->value & DF_INFINITE_AMMO))
				ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;

			ent->client->anim_priority = ANIM_ATTACK;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
				ent->s.frame = FRAME_crattak1 - 1;
				ent->client->anim_end = FRAME_crattak9;
			}
			else {
				ent->s.frame = FRAME_attack1 - 1;
				ent->client->anim_end = FRAME_attack8;
			}
		}

		ent->client->ps.guns[gun].frame++;
		if (ent->client->ps.guns[gun].frame == 12 && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
			ent->client->ps.guns[gun].frame = 6;
	}

	if (ent->client->ps.guns[gun].frame == 12) {
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
		ent->client->weapon_sound = 0;
	}
}


void LightningHit(edict_t *ent, edict_t *from, vec3_t pos, int damage)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_Q1_LIGHTNINGBLOOD);
	gi.WritePosition(pos);
	gi.multicast(pos, MULTICAST_PVS);

	meansOfDeath_t mod;

	if (from->client)
		mod = MakeWeaponMeansOfDeath(from, GetIndexByItem(from->client->pers.weapon), from, DT_DIRECT);
	else
		mod = MakeGenericMeansOfDeath(from, MD_NONE, DT_DIRECT);

	T_Damage(ent, from, from, vec3_origin, vec3_origin, vec3_origin, damage, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, mod);
}

/*
=================
LightningDamage
=================
*/
void LightningDamage(edict_t *ent, vec3_t p1, vec3_t p2, edict_t *from, int damage)
{
	edict_t *e1, *e2;
	vec3_t f;

	VectorSubtract(p2, p1, f);
	VectorNormalize(f);
	f[0] = 0 - f[1];
	f[1] = f[0];
	f[2] = 0;
	VectorScale(f, 16, f);

	e1 = e2 = world;

	trace_t tr = gi.trace(p1, vec3_origin, vec3_origin, p2, ent, MASK_BULLET);

	if (tr.ent->takedamage)
	{
		LightningHit(tr.ent, from, tr.endpos, damage);

		/*if (ent->client && .classname == "player")
		{
			if (other.classname == "player")*/
			//tr.ent->velocity[2] += 400;
	//}
	}

	e1 = tr.ent;

	vec3_t p1f, p2f;

	VectorAdd(p1, f, p1f);
	VectorAdd(p2, f, p2f);

	tr = gi.trace(p1f, vec3_origin, vec3_origin, p2f, ent, MASK_BULLET);
	if (tr.ent != e1 && tr.ent->takedamage)
	{
		LightningHit(tr.ent, from, tr.endpos, damage);
	}
	e2 = tr.ent;

	VectorSubtract(p1, f, p1f);
	VectorSubtract(p2, f, p2f);

	tr = gi.trace(p1f, vec3_origin, vec3_origin, p2f, ent, MASK_BULLET);
	if (tr.ent != e1 && tr.ent != e2 && tr.ent->takedamage)
	{
		LightningHit(tr.ent, from, tr.endpos, damage);
	}
}

void W_FireLightning(edict_t *ent, gunindex_e gun, int damage, int blowup_damage)
{
	// explode if under water
	if (ent->waterlevel > 1)
	{
		meansOfDeath_t mod;

		if (ent->client)
			mod = MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_INDIRECT);
		else
			mod = MakeGenericMeansOfDeath(ent, MD_NONE, DT_INDIRECT);

		int cells = ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index];
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] = 0;
		T_RadiusDamage(ent, ent, blowup_damage * cells, world, DAMAGE_Q1, blowup_damage * cells, mod);
		return;
	}

	if (ent->decel <= 0)
	{
		gi.sound(ent, CHAN_BODY, gi.soundindex("q1/weapons/lhit.wav"), 1, ATTN_NORM, 0);
		ent->decel = 0.5;
	}
	else
		ent->decel -= 0.1f;

	vec3_t org;
	VectorCopy(ent->s.origin, org);
	org[2] += ent->viewheight - 8;

	vec3_t forward;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);

	vec3_t end;
	VectorMA(org, 600, forward, end);

	trace_t tr = gi.trace(org, vec3_origin, vec3_origin, end, ent, MASK_SOLID);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_Q1_LIGHTNING2);
	gi.WriteShort(ent - g_edicts);
	gi.WritePosition(org);
	gi.WritePosition(tr.endpos);
	gi.multicast(org, MULTICAST_PVS);

	VectorMA(tr.endpos, 4, forward, end);

	LightningDamage(ent, org, end, ent, damage);
}

void weapon_q1_lightning_fire(edict_t *ent, gunindex_e gun)
{
	if (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->gunstates[gun].newweapon || !ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
	{
		ent->client->ps.guns[gun].frame = 5;
		return;
	}

	vec3_t      forward;
	int         damage = 30, blowup_damage = 35;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);

	if (is_quad) {
		damage *= 4;
		blowup_damage *= 4;
	}

	// send muzzle flash
	if (ent->client->gunstates[gun].weaponstate != WEAPON_FIRING)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
		ent->decel = 0;
	}

	W_FireLightning(ent, gun, damage, blowup_damage);
	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO) && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;

	if (ent->client->ps.guns[gun].frame == 3)
		ent->client->ps.guns[gun].frame = 1;
	else
		ent->client->ps.guns[gun].frame++;

	Weapon_Q1_Anim_Light(ent);
	ent->client->gunstates[gun].kick_angles[0] = -1 * random();
	ent->client->gunstates[gun].kick_time = level.time + 100;
}

void doom_plasma_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY)) {
		G_FreeEdict(ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

	VectorNormalize(ent->velocity);
	VectorMA(ent->s.origin, -8, ent->velocity, origin);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DOOM_PLASMA);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

	G_FreeEdict(ent);
}

void fire_doom_plasma(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t *rocket;

	rocket = G_Spawn();
	VectorCopy(start, rocket->s.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->s.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= EF_ANIM_ALLFAST;
	rocket->s.renderfx |= RF_FULLBRIGHT;
	rocket->s.game = GAME_DOOM;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = gi.modelindex("sprites/doom/PLSS.d2s");
	rocket->owner = self;
	rocket->touch = doom_plasma_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->entitytype = ET_DOOM_PLASMA;

	if (self->client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	if (self->client)
		check_dodge(self, rocket->s.origin, dir, speed);

	gi.linkentity(rocket);
}

void weapon_doom_hyperblaster_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame >= 3)
	{
		++ent->client->ps.guns[gun].frame;
		return;
	}

	if (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->gunstates[gun].newweapon || !ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
	{
		ent->attack_finished_time = level.time + 571;
		ent->client->ps.guns[gun].frame = 3;
		return;
	}

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	vec3_t start, forward, right, offset;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_doom_plasma(ent, start, forward, Doom_MissileDamageRandomizer(5), 850);
	Weapon_Doom_Anim_Fast(ent);

	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	ent->client->ps.guns[gun].frame = 1 + (Q_rand() % 2);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;
}

void duke_freezer_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	vec3_t vel;
	VectorNormalize2(ent->velocity, vel);
	VectorMA(ent->s.origin, -8, vel, origin);

	if (other->takedamage)
	{
		bool freeze_fx = false;

		ent->owner = ent->activator;

		if (other->freeze_time <= level.time)
		{
			if (other->health > ent->dmg)
			{
				T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);
				gi.sound(ent, CHAN_VOICE, gi.soundindex("duke/BULITHIT.wav"), 1, ATTN_NORM, 0);
			}
			else
			{
				if (other->client || (other->svflags & SVF_MONSTER))
				{
					other->freeze_time = level.time + 5000;
					other->health = 1;
				}
				else
					T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

				gi.sound(ent, CHAN_VOICE, gi.soundindex("duke/FREEZE.wav"), 1, ATTN_NORM, 0);
				freeze_fx = true;
			}
		}
		else
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("duke/BULITHIT.wav"), 1, ATTN_NORM, 0);
			freeze_fx = true;
		}

		if (freeze_fx)
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_DUKE_FREEZE_HIT);
			gi.WritePosition(origin);
			gi.multicast(ent->s.origin, MULTICAST_PHS);
		}

		G_FreeEdict(ent);
		return;
	}

	ent->owner = NULL;

	float dot = DotProduct(vel, plane->normal);

	if (!ent->count || (ent->count <= 2 && dot > -0.4f))
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_DUKE_FREEZE_HIT);
		gi.WritePosition(origin);
		gi.multicast(ent->s.origin, MULTICAST_PHS);

		G_FreeEdict(ent);
	}
	else
		--ent->count;
}

void fire_duke_freezer(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t *rocket;

	rocket = G_Spawn();
	VectorCopy(start, rocket->s.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->s.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYBOUNCE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= EF_ANIM_ALLFAST;
	rocket->s.renderfx |= RF_FULLBRIGHT | RF_ANIM_BOUNCE;
	rocket->s.game = GAME_DOOM;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = gi.modelindex("%e_freezer");
	rocket->owner = rocket->activator = self;
	rocket->touch = duke_freezer_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = (damage + (Q_rand() & 7)) >> 2;
	rocket->count = 3;
	rocket->entitytype = ET_DOOM_PLASMA;

	if (self->client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	if (self->client)
		check_dodge(self, rocket->s.origin, dir, speed);

	gi.linkentity(rocket);
}

void weapon_duke_freezer_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (!ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] || ent->client->gunstates[gun].newweapon || !((ent->client->buttons | ent->client->latched_buttons) & BUTTON_ATTACK))
	{
		ent->client->ps.guns[gun].frame = 5;
		return;
	}

	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame >= 4)
		ent->client->ps.guns[gun].frame = 1;

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_BFG | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	vec3_t start, forward, right, offset;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_duke_freezer(ent, start, forward, 20, 900);

	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;
}

void Weapon_HyperBlaster(edict_t *ent, gunindex_e gun)
{
	switch (ent->s.game)
	{
	case GAME_Q2:
	{
		static int  pause_frames[] = { 0 };
		static int  fire_frames[] = { 6, 7, 8, 9, 10, 11, 0 };

		Weapon_Generic(ent, 5, 20, 49, 53, pause_frames, fire_frames, Weapon_HyperBlaster_Fire, gun);
		break;
	}
	case GAME_Q1:
		Weapon_Q1(ent, 4, weapon_q1_lightning_fire, gun);
		break;
	case GAME_DOOM:
		Weapon_Doom(ent, 4, weapon_doom_hyperblaster_fire, gun);
		break;
	case GAME_DUKE:
		Weapon_Doom(ent, 4, weapon_duke_freezer_fire, gun);
		break;
	}
}

/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/

void Machinegun_Fire(edict_t *ent, gunindex_e gun)
{
	int i;
	vec3_t      start;
	vec3_t      forward, right;
	vec3_t      angles;
	int         damage = 8;
	int         kick = 2;
	vec3_t      offset;

	if (!(ent->client->buttons & BUTTON_ATTACK)) {
		ent->client->gunstates[gun].machinegun_shots = 0;
		ent->client->ps.guns[gun].frame++;
		return;
	}

	if (ent->client->ps.guns[gun].frame == 5)
		ent->client->ps.guns[gun].frame = 4;
	else
		ent->client->ps.guns[gun].frame = 5;

	if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] < 1) {
		ent->client->ps.guns[gun].frame = 6;
		if (level.time >= ent->pain_debounce_time) {
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1000;
		}
		NoAmmoWeaponChange(ent, gun);
		return;
	}

	if (is_quad) {
		damage *= 4;
		kick *= 4;
	}

	for (i = 1; i < 3; i++) {
		ent->client->gunstates[gun].kick_origin[i] = crandom() * 0.35f;
		ent->client->gunstates[gun].kick_angles[i] = crandom() * 0.7f;
	}
	ent->client->gunstates[gun].kick_origin[0] = crandom() * 0.35f;
	ent->client->gunstates[gun].kick_angles[0] = ent->client->gunstates[gun].machinegun_shots * -1.5f;
	ent->client->gunstates[gun].kick_time = level.time + 100;

	// raise the gun as it is firing
	if (!deathmatch->value) {
		ent->client->gunstates[gun].machinegun_shots++;
		if (ent->client->gunstates[gun].machinegun_shots > 9)
			ent->client->gunstates[gun].machinegun_shots = 9;
	}

	// get start / end positions
	VectorAdd(ent->client->v_angle, ent->client->gunstates[gun].kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	fire_bullet(ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_MACHINEGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crattak1 - (int)(random() + 0.25f);
		ent->client->anim_end = FRAME_crattak9;
	}
	else {
		ent->s.frame = FRAME_attack1 - (int)(random() + 0.25f);
		ent->client->anim_end = FRAME_attack8;
	}
}

void wall_velocity(edict_t *self, vec3_t vel, vec3_t right, vec3_t up, vec3_t normal)
{
	VectorNormalize2(self->velocity, vel);

	vec3_t vup, vright;

	VectorScale(up, (random() - 0.5f), vup);
	VectorScale(right, (random() - 0.5f), vright);

	VectorAdd(vel, up, vel);
	VectorAdd(vel, right, vel);

	VectorNormalize(vel);

	VectorScale(normal, 2, vup);
	VectorAdd(vel, vup, vel);

	VectorScale(vel, 200, vel);
}

/*
================
spawn_touchblood
================
*/
/*void spawn_touchblood(edict_t *ent, float damage)
{
	vec3_t vel;
	vec3_t right, up;

	AngleVectors(ent->velocity, NULL, right, up);

	wall_velocity(ent, vel, right, up, vec3_origin);
	VectorScale(vel, 0.2, vel);

	vec3_t out;
	VectorMA(ent->s.origin, 0.01, vel, out);

	SpawnBlood(out, damage);
};*/

/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the blaster and hyper blaster.
=================
*/
void spike_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY)) {
		G_FreeEdict(self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 0, DAMAGE_Q1, self->meansOfDeath);
		//spawn_touchblood(self, self->dmg);
	}
	else
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(self->count);
		gi.WritePosition(self->s.origin);
		gi.multicast(self->s.origin, MULTICAST_PVS);
	}

	G_FreeEdict(self);
}

edict_t *fire_spike(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, bool super)
{
	edict_t *bolt;

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->svflags = SVF_DEADMONSTER;
	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster/hyperblaster shots), the player won't be solid clipped against
	// the object.  Right now trying to run into a firing hyperblaster
	// is very jerky since you are predicted 'against' the shots.
	VectorCopy(start, bolt->s.origin);
	VectorCopy(start, bolt->s.old_origin);
	vectoangles(dir, bolt->s.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->s.game = GAME_Q1;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	VectorClear(bolt->mins);
	VectorClear(bolt->maxs);
	bolt->count = (super) ? TE_Q1_SUPERSPIKE : TE_Q1_SPIKE;
	if (super)
		bolt->s.modelindex = gi.modelindex("models/q1/s_spike.mdl");
	else
		bolt->s.modelindex = gi.modelindex("models/q1/spike.mdl");
	bolt->owner = self;
	bolt->touch = spike_touch;
	bolt->nextthink = level.time + 6000;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->entitytype = ET_Q1_SPIKE;
	gi.linkentity(bolt);

	if (self->client)
		bolt->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), bolt, DT_DIRECT);
	else
		bolt->meansOfDeath = MakeAttackerMeansOfDeath(self, bolt, MD_NONE, DT_DIRECT);

	if (self->client)
		check_dodge(self, bolt->s.origin, dir, speed);

	/*tr = gi.trace(self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0) {
		VectorMA(bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}*/

	return bolt;
}

void W_FireSuperSpikes(edict_t *ent, int damage)
{
	vec3_t dir, right;

	AngleVectors(ent->client->v_angle, dir, right, NULL);

	vec3_t start;
	VectorSet(start, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] + 16);

	fire_spike(ent, start, dir, damage, 1000, true);
}

void W_FireSpikes(edict_t *ent, int damage, float ox)
{
	vec3_t dir, right;

	AngleVectors(ent->client->v_angle, dir, right, NULL);

	vec3_t start;
	VectorScale(right, ox, right);
	VectorSet(start, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] + ent->viewheight - 8);
	VectorAdd(start, right, start);

	fire_spike(ent, start, dir, damage, 1000, false);
}

void weapon_q1_nailgun_fire(edict_t *ent, gunindex_e gun)
{
	if (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->gunstates[gun].newweapon || !ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
	{
		ent->client->ps.guns[gun].frame = 9;
		return;
	}

	vec3_t      forward;
	int         damage = 9;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);

	if (is_quad) {
		damage *= 4;
	}

	W_FireSpikes(ent, damage, (ent->client->ps.guns[gun].frame % 2) == 0 ? 4 : -4);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_MACHINEGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;

	if (ent->client->ps.guns[gun].frame == 8)
		ent->client->ps.guns[gun].frame = 1;
	else
		ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	Weapon_Q1_Anim_Nail(ent);
	ent->client->gunstates[gun].kick_angles[0] = -1 * random();
	ent->client->gunstates[gun].kick_time = level.time + 100;
}

static void fire_doom_lead(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, meansOfDeath_t mod)
{
	trace_t     tr;
	vec3_t      dir;
	vec3_t      forward, right, up;
	vec3_t      end;
	float       r;
	float       u;

	tr = gi.trace(self->s.origin, NULL, NULL, start, self, MASK_BULLET);

	if (!(tr.fraction < 1.0f))
	{
		vectoangles(aimdir, dir);
		AngleVectors(dir, forward, right, up);

		r = crandom() * hspread;
		u = crandom() * vspread;
		VectorMA(start, 8192, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		tr = gi.trace(start, NULL, NULL, end, self, MASK_BULLET);
	}

	// send gun puff / flash
	if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0f)
		{
			vec3_t puff;

			VectorMA(tr.endpos, -4, forward, puff);

			if (tr.ent->takedamage)
			{
				AddMultiDamage(tr.ent, damage, kick, mod, DAMAGE_BULLET | DAMAGE_NO_PARTICLES, false);
				//T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_BULLET | DAMAGE_NO_PARTICLES, mod);

				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(te_impact == TE_DUKE_GUNSHOT ? TE_DUKE_BLOOD : TE_DOOM_BLOOD);
				gi.WritePosition(puff);
				gi.multicast(puff, MULTICAST_PVS);
			}
			else if (strncmp(tr.surface->name, "sky", 3) != 0)
			{
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(te_impact);
				gi.WritePosition(puff);
				gi.multicast(puff, MULTICAST_PVS);

				if (self->client)
					PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
			}
		}
	}
}

void fire_doom_bullet(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, meansOfDeath_t mod)
{
	fire_doom_lead(self, start, aimdir, damage, kick, te_impact, hspread, vspread, mod);
	ApplyMultiDamage(self, DAMAGE_NO_PARTICLES, mod);
}

void fire_doom_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int count, int hspread, int vspread, meansOfDeath_t mod)
{
	for (int i = 0; i < count; ++i)
		fire_doom_lead(self, start, aimdir, damage, kick, te_impact, hspread, vspread, mod);

	ApplyMultiDamage(self, DAMAGE_NO_PARTICLES, mod);
}

void weapon_doom_pistol_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame >= 3 && (ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] && !ent->client->gunstates[gun].newweapon)
		ent->client->ps.guns[gun].frame = 0;
	else
		ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 1)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_BLASTER | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

		vec3_t start, forward, right, offset;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		int damage = 5 * (Q_rand() % 3 + 1);

		fire_doom_bullet(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, first ? 0 : 400, 0, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
}

void weapon_duke_pistol_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame == 2)
	{
		if ((ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] && !ent->client->gunstates[gun].newweapon && ent->client->pers.pistol_clip)
			ent->client->ps.guns[gun].frame = 1;
		else
			ent->client->ps.guns[gun].frame = 21;
	}
	else
		ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 4)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/CLIPOUT.wav"), 1, ATTN_IDLE, 0);
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/CLIPIN.wav"), 1, ATTN_IDLE, 0);
	}
	else if (ent->client->ps.guns[gun].frame == 7)
		ent->client->pers.pistol_clip = 12;
	else if (ent->client->ps.guns[gun].frame == 13)
		ent->client->ps.guns[gun].frame = 21;
	else if (ent->client->ps.guns[gun].frame == 15)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/KNUCKLE.wav"), 1, ATTN_IDLE, 0);
	else if (ent->client->ps.guns[gun].frame == 1)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_BLASTER | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

		vec3_t start, forward, right, offset;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		--ent->client->pers.pistol_clip;

		fire_doom_bullet(ent, start, forward, 6, 0, TE_DUKE_GUNSHOT, 250, 250, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
	}
}

void Weapon_Machinegun(edict_t *ent, gunindex_e gun)
{
	switch (ent->s.game)
	{
	case GAME_Q2:
	{
		static int  pause_frames[] = { 23, 45, 0 };
		static int  fire_frames[] = { 4, 5, 0 };

		Weapon_Generic(ent, 3, 5, 45, 49, pause_frames, fire_frames, Machinegun_Fire, gun);
		break;
	}
	case GAME_Q1:
		Weapon_Q1(ent, 9, weapon_q1_nailgun_fire, gun);
		break;
	case GAME_DOOM:
		Weapon_Doom(ent, 6, weapon_doom_pistol_fire, gun);
		break;
	case GAME_DUKE:
		if (ent->client->gunstates[gun].weaponstate == WEAPON_READY && !ent->client->pers.pistol_clip &&
			ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] && ent->client->ps.guns[gun].frame < 3)
		{
			ent->client->gunstates[gun].weaponstate = WEAPON_FIRING;
			ent->client->ps.guns[gun].frame = 3;
		}

		Weapon_Doom(ent, 21, weapon_duke_pistol_fire, gun);

		if (ent->client->ps.guns[gun].frame > 3)
			ent->client->ps.guns[gun].offset[1] = 0;

		break;
	}
}

void Chaingun_Fire(edict_t *ent, gunindex_e gun)
{
	int         i;
	int         shots;
	vec3_t      start;
	vec3_t      forward, right, up;
	float       r, u;
	vec3_t      offset;
	int         damage;
	int         kick = 2;

	if (deathmatch->value)
		damage = 6;
	else
		damage = 8;

	if (ent->client->ps.guns[gun].frame == 5)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);

	if ((ent->client->ps.guns[gun].frame == 14) && !(ent->client->buttons & BUTTON_ATTACK)) {
		ent->client->ps.guns[gun].frame = 32;
		ent->client->weapon_sound = 0;
		return;
	}
	else if ((ent->client->ps.guns[gun].frame == 21) && (ent->client->buttons & BUTTON_ATTACK)
		&& ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]) {
		ent->client->ps.guns[gun].frame = 15;
	}
	else {
		ent->client->ps.guns[gun].frame++;
	}

	if (ent->client->ps.guns[gun].frame == 22) {
		ent->client->weapon_sound = 0;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
	}
	else {
		ent->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");
	}

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crattak1 - (ent->client->ps.guns[gun].frame & 1);
		ent->client->anim_end = FRAME_crattak9;
	}
	else {
		ent->s.frame = FRAME_attack1 - (ent->client->ps.guns[gun].frame & 1);
		ent->client->anim_end = FRAME_attack8;
	}

	if (ent->client->ps.guns[gun].frame <= 9)
		shots = 1;
	else if (ent->client->ps.guns[gun].frame <= 14) {
		if (ent->client->buttons & BUTTON_ATTACK)
			shots = 2;
		else
			shots = 1;
	}
	else
		shots = 3;

	if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] < shots)
		shots = ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index];

	if (!shots) {
		if (level.time >= ent->pain_debounce_time) {
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1000;
		}
		NoAmmoWeaponChange(ent, gun);
		return;
	}

	if (is_quad) {
		damage *= 4;
		kick *= 4;
	}

	for (i = 0; i < 3; i++) {
		ent->client->gunstates[gun].kick_origin[i] = crandom() * 0.35f;
		ent->client->gunstates[gun].kick_angles[i] = crandom() * 0.7f;
	}
	ent->client->gunstates[gun].kick_time = level.time + 100;

	for (i = 0; i < shots; i++) {
		// get start / end positions
		AngleVectors(ent->client->v_angle, forward, right, up);
		r = 7 + crandom() * 4;
		u = crandom() * 4;
		VectorSet(offset, 0, r, u + ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		fire_bullet(ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
	}

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte((MZ_CHAINGUN1 + shots - 1) | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= shots;
}

void weapon_q1_snailgun_fire(edict_t *ent, gunindex_e gun)
{
	if (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->gunstates[gun].newweapon || !ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
	{
		ent->client->ps.guns[gun].frame = 9;
		return;
	}

	vec3_t      forward;
	int         damage = 18;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);

	if (is_quad) {
		damage *= 4;
	}

	if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] > 2)
		W_FireSuperSpikes(ent, damage);
	else
		W_FireSpikes(ent, damage, (ent->client->ps.guns[gun].frame % 2) == 0 ? 4 : -4);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_CHAINGUN1 | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] = max(0, ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] - 2);

	if (ent->client->ps.guns[gun].frame == 8)
		ent->client->ps.guns[gun].frame = 1;
	else
		ent->client->ps.guns[gun].frame++;

	Weapon_Q1_Anim_Nail(ent);
	ent->client->gunstates[gun].kick_angles[0] = -1 * random();
	ent->client->gunstates[gun].kick_time = level.time + 100;
}

#define DOOM_TIC_TO_TIME(t) (gtime_t)(floorf(((1.0f / 35.0f) * 1000) * (t - 1)))

void weapon_doom_chaingun_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if ((ent->client->ps.guns[gun].frame == 2 && !(ent->client->buttons & BUTTON_ATTACK)) || !ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] || ent->client->gunstates[gun].newweapon)
	{
		ent->client->ps.guns[gun].frame = 3;
		return;
	}

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_BLASTER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

	vec3_t start, forward, right, offset;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 2);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	int damage = 5 * (Q_rand() % 3 + 1);

	fire_doom_bullet(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, 400, 0, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));

	ent->client->ps.guns[gun].frame = (ent->client->ps.guns[gun].frame == 1) ? 2 : 1;
	ent->attack_finished_time = level.time + DOOM_TIC_TO_TIME(4);
	Weapon_Doom_Anim_Fast(ent);
}

void weapon_duke_chaingun_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (!(ent->client->buttons & BUTTON_ATTACK) || !ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] || ent->client->gunstates[gun].newweapon)
	{
		ent->client->ps.guns[gun].frame = 5;
		return;
	}

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_CHAINGUN1 | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

	vec3_t start, forward, right, offset;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 2);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_doom_bullet(ent, start, forward, 9, 0, TE_DUKE_GUNSHOT, 300, 300, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));

	if (++ent->client->ps.guns[gun].frame == 5)
		ent->client->ps.guns[gun].frame = 2;
	//ent->attack_finished_time = level.time + DOOM_TIC_TO_TIME(4);
	//Weapon_Doom_Anim_Fast(ent);
}

void Weapon_Chaingun(edict_t *ent, gunindex_e gun)
{
	switch (ent->s.game)
	{
	case GAME_Q2:
	{
		static int  pause_frames[] = { 38, 43, 51, 61, 0 };
		static int  fire_frames[] = { 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0 };

		Weapon_Generic(ent, 4, 31, 61, 64, pause_frames, fire_frames, Chaingun_Fire, gun);
		break;
	}
	case GAME_Q1:
		Weapon_Q1(ent, 8, weapon_q1_snailgun_fire, gun);
		break;
	case GAME_DOOM:
		Weapon_Doom(ent, 3, weapon_doom_chaingun_fire, gun);
		break;
	case GAME_DUKE:
		Weapon_Doom(ent, 5, weapon_duke_chaingun_fire, gun);
		break;
	}
}


/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

void weapon_shotgun_fire(edict_t *ent, gunindex_e gun)
{
	vec3_t      start;
	vec3_t      forward, right;
	vec3_t      offset;
	int         damage = 4;
	int         kick = 8;

	if (ent->client->ps.guns[gun].frame == 9) {
		ent->client->ps.guns[gun].frame++;
		return;
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -2;
	ent->client->gunstates[gun].kick_time = level.time + 100;

	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad) {
		damage *= 4;
		kick *= 4;
	}

	if (deathmatch->value)
		fire_shotgun(ent, start, forward, damage, kick, 500, 500, DEFAULT_DEATHMATCH_SHOTGUN_COUNT, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
	else
		fire_shotgun(ent, start, forward, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;
}

void weapon_q1_shotgun_fire(edict_t *ent, gunindex_e gun)
{
	if (ent->client->ps.guns[gun].frame != 0)
	{
		if (ent->client->ps.guns[gun].frame >= 5 && (ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] && !ent->client->gunstates[gun].newweapon)
			ent->client->ps.guns[gun].frame = 0;
		else
		{
			ent->client->ps.guns[gun].frame++;
			return;
		}
	}

	vec3_t      forward, right, up;
	int         damage = 4;

	AngleVectors(ent->client->v_angle, forward, right, up);

	if (is_quad) {
		damage *= 4;
	}

	vec3_t spread = { 0.04, 0.04, 0 };

	FireBullets(ent, 6, damage, forward, spread, up, right, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;

	Weapon_Q1_Anim_Shot(ent);
	ent->client->gunstates[gun].kick_time = level.time + 100;
	ent->client->gunstates[gun].kick_angles[0] = -1;
	ent->attack_finished_time = level.time + 600;
}

void weapon_doom_shotgun_fire(edict_t *ent, gunindex_e gun, bool first)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 1)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_SHOTGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

		vec3_t start, forward, right, offset;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		int damage = 5 * (Q_rand() % 3 + 1);

		fire_doom_shotgun(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, 7, 400, 0, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
}

void weapon_duke_shotgun_fire(edict_t *ent, gunindex_e gun, bool first)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 5)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/SHOTGNCK.wav"), 1, ATTN_IDLE, 0);
	else if (ent->client->ps.guns[gun].frame == 7)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_SSHOTGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
	}

	if (ent->client->ps.guns[gun].frame == 1)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_SHOTGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

		vec3_t start, forward, right, offset;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		fire_doom_shotgun(ent, start, forward, 7, 0, TE_DUKE_GUNSHOT, 7, 250, 250, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
}

void Weapon_Shotgun(edict_t *ent, gunindex_e gun)
{
	switch (ent->s.game)
	{
	case GAME_Q2:
	{
		static int  pause_frames[] = { 22, 28, 34, 0 };
		static int  fire_frames[] = { 8, 9, 0 };

		Weapon_Generic(ent, 7, 18, 36, 39, pause_frames, fire_frames, weapon_shotgun_fire, gun);
		break;
	}
	case GAME_Q1:
		Weapon_Q1(ent, 7, weapon_q1_shotgun_fire, gun);
		break;
	case GAME_DOOM:
		Weapon_Doom(ent, 10, weapon_doom_shotgun_fire, gun);
		break;
	case GAME_DUKE:
		Weapon_Doom(ent, 10, weapon_duke_shotgun_fire, gun);
		break;
	}
}

void weapon_supershotgun_fire(edict_t *ent, gunindex_e gun)
{
	vec3_t      start;
	vec3_t      forward, right;
	vec3_t      offset;
	vec3_t      v;
	int         damage = 6;
	int         kick = 12;

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -2;
	ent->client->gunstates[gun].kick_time = level.time + 100;

	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad) {
		damage *= 4;
		kick *= 4;
	}

	v[PITCH] = ent->client->v_angle[PITCH];
	v[YAW] = ent->client->v_angle[YAW] - 5;
	v[ROLL] = ent->client->v_angle[ROLL];
	AngleVectors(v, forward, NULL, NULL);
	fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
	v[YAW] = ent->client->v_angle[YAW] + 5;
	AngleVectors(v, forward, NULL, NULL);
	fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_SSHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 2;
}

void weapon_q1_sshotgun_fire(edict_t *ent, gunindex_e gun)
{
	if (ent->client->ps.guns[gun].frame != 0)
	{
		ent->client->ps.guns[gun].frame++;
		return;
	}

	if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] == 1)
	{
		weapon_q1_shotgun_fire(ent, gun);
		return;
	}

	vec3_t      forward, right, up;
	int         damage = 4;

	AngleVectors(ent->client->v_angle, forward, right, up);

	if (is_quad) {
		damage *= 4;
	}

	vec3_t spread = { 0.14, 0.08, 0 };

	FireBullets(ent, 14, damage, forward, spread, up, right, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_SSHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 2;

	Weapon_Q1_Anim_Shot(ent);
	ent->client->gunstates[gun].kick_angles[0] = -3;
	ent->client->gunstates[gun].kick_time = level.time + 100;
}

void weapon_doom_sshotgun_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame >= 16 && (ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] >= 2 && !ent->client->gunstates[gun].newweapon)
		ent->client->ps.guns[gun].frame = 0;

	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 1)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_SSHOTGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 2;

		vec3_t start, forward, right, offset;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		int damage = 5 * (Q_rand() % 3 + 1);

		fire_doom_shotgun(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, 20, 1300, 900, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
	else if (ent->client->ps.guns[gun].frame == 7)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("doom/DBOPN.wav"), 1, ATTN_NORM, 0);
	else if (ent->client->ps.guns[gun].frame == 11)
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/DBLOAD.wav"), 1, ATTN_NORM, 0);
	else if (ent->client->ps.guns[gun].frame == 15)
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/DBCLS.wav"), 1, ATTN_NORM, 0);
}

void Weapon_SuperShotgun(edict_t *ent, gunindex_e gun)
{
	switch (ent->s.game)
	{
	case GAME_Q2:
	{
		static int  pause_frames[] = { 29, 42, 57, 0 };
		static int  fire_frames[] = { 7, 0 };

		Weapon_Generic(ent, 6, 17, 57, 61, pause_frames, fire_frames, weapon_supershotgun_fire, gun);
		break;
	}
	case GAME_Q1:
		Weapon_Q1(ent, 7, weapon_q1_sshotgun_fire, gun);
		break;
	case GAME_DOOM:
		Weapon_Doom(ent, 17, weapon_doom_sshotgun_fire, gun);
		break;
	}
}



/*
======================================================================

RAILGUN

======================================================================
*/

void weapon_railgun_fire(edict_t *ent, gunindex_e gun)
{
	vec3_t      start;
	vec3_t      forward, right;
	vec3_t      offset;
	int         damage;
	int         kick;

	if (deathmatch->value) {
		// normal damage is too extreme in dm
		damage = 100;
		kick = 200;
	}
	else {
		damage = 150;
		kick = 250;
	}

	if (is_quad) {
		damage *= 4;
		kick *= 4;
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -3, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -3;
	ent->client->gunstates[gun].kick_time = level.time + 100;

	VectorSet(offset, 0, 7, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	fire_rail(ent, start, forward, damage, kick);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_RAILGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;
}

void weapon_doom_chainsaw_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->gunstates[gun].newweapon)
	{
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWIDL.wav"), 1, ATTN_NORM, 0);
		ent->client->silencer_shots = 2;
		ent->client->ps.guns[gun].frame = 5;
		return;
	}

	if (ent->client->ps.guns[gun].frame == 2)
		ent->client->ps.guns[gun].frame = 3;
	else
		ent->client->ps.guns[gun].frame = 2;

	int damage = 2 * (Q_rand() % 10 + 1);
	vec3_t source, org;
	vec3_t forward;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorCopy(ent->s.origin, source);
	source[2] += ent->viewheight - 8;

	VectorMA(source, 64, forward, org);

	trace_t tr = gi.trace(source, vec3_origin, vec3_origin, org, ent, MASK_SHOT);

	if (tr.fraction != 1.0f)
	{
		VectorMA(tr.endpos, -4, forward, org);

		if (tr.ent->takedamage)
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_DOOM_BLOOD);
			gi.WritePosition(org);
			gi.multicast(org, MULTICAST_PVS);

			T_Damage(tr.ent, ent, ent, vec3_origin, vec3_origin, vec3_origin, damage, 0, DAMAGE_NO_PARTICLES, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWHIT.wav"), 1, ATTN_NORM, 0);
		}
		else
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_DOOM_GUNSHOT);
			gi.WritePosition(org);
			gi.multicast(org, MULTICAST_PVS);

			gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWFUL.wav"), 1, ATTN_NORM, 0);
		}
	}
	else
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWFUL.wav"), 1, ATTN_NORM, 0);

	Weapon_Doom_Anim_Fast(ent);
}

void Weapon_Railgun(edict_t *ent, gunindex_e gun)
{
	switch (ent->s.game)
	{
	case GAME_Q2:
	{
		static int  pause_frames[] = { 56, 0 };
		static int  fire_frames[] = { 4, 0 };

		Weapon_Generic(ent, 3, 18, 56, 61, pause_frames, fire_frames, weapon_railgun_fire, gun);
	}
	break;
	case GAME_DOOM:
		if (ent->client->gunstates[gun].weaponstate == WEAPON_ACTIVATING && ent->client->silencer_shots == 0)
		{
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWUP.wav"), 1, ATTN_NORM, 0);
			ent->client->silencer_shots = 1;
		}
		else if (ent->client->gunstates[gun].weaponstate == WEAPON_READY)
		{
			if (--ent->client->silencer_shots <= 0 && !(ent->client->buttons & BUTTON_ATTACK))
			{
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWIDL.wav"), 1, ATTN_NORM, 0);
				ent->client->silencer_shots = 2;
			}

			ent->client->ps.guns[gun].frame ^= 1;

			if (ent->client->ps.guns[gun].frame >= 2)
				ent->client->ps.guns[gun].frame = 0;
		}
		else if (ent->client->gunstates[gun].weaponstate == WEAPON_DROPPING)
			ent->client->silencer_shots = 0;

		Weapon_Doom(ent, 4, weapon_doom_chainsaw_fire, gun);
		break;
	}
}


/*
======================================================================

BFG10K

======================================================================
*/

void weapon_bfg_fire(edict_t *ent, gunindex_e gun)
{
	vec3_t  offset, start;
	vec3_t  forward, right;
	int     damage;
	float   damage_radius = 1000;

	if (deathmatch->value)
		damage = 200;
	else
		damage = 500;

	if (ent->client->ps.guns[gun].frame == 9) {
		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_BFG | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		ent->client->ps.guns[gun].frame++;

		PlayerNoise(ent, start, PNOISE_WEAPON);
		return;
	}

	// cells can go down during windup (from power armor hits), so
	// check again and abort firing if we don't have enough now
	if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] < 50) {
		ent->client->ps.guns[gun].frame++;
		return;
	}

	if (is_quad)
		damage *= 4;

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_time = level.time + 100;

	// make a big pitch kick with an inverse fall
	ent->client->v_dmg_pitch = -40;
	ent->client->v_dmg_roll = crandom() * 8;
	ent->client->v_dmg_time = level.time + DAMAGE_TIME;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	fire_bfg(ent, start, forward, damage, 400, damage_radius);

	ent->client->ps.guns[gun].frame++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 50;
}



/*
=================
fire_bfg
=================
*/
void bfg_doom_explode(edict_t *self)
{
	self->nextthink = level.time + (game.frametime * 2);
	self->s.frame++;

	if (self->s.frame == 2 && !self->activator)
	{
		self->meansOfDeath.damage_type = DT_INDIRECT;
		self->activator = self;

		int			i;
		int			j;
		int			damage;
		vec3_t		angles = { 0, 0, 0 };

		// offset angles from its attack angle
		for (i = 0; i < 40; i++)
		{
			const float one_angle = 90.0f / (40 - 1);
			float angle_mul = one_angle * i;
			const float move_left = 90.0f / 2.0f;

			angles[1] = self->s.angles[1] - move_left + angle_mul;

			vec3_t forward;
			AngleVectors(angles, forward, NULL, NULL);

			vec3_t start, end;
			VectorCopy(self->owner->s.origin, start);

			VectorMA(start, 1024, forward, end);

			trace_t tr = gi.trace(start, vec3_origin, vec3_origin, end, self->owner, CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_MONSTER);

			if (tr.fraction == 1.0f || !tr.ent)
				continue;

			/*P_SpawnMobj(linetarget->x,
				linetarget->y,
				linetarget->z + (linetarget->height >> 2),
				MT_EXTRABFG);*/

			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_BFG_EXPLOSION);
			gi.WritePosition(tr.ent->s.origin);
			gi.multicast(tr.ent->s.origin, MULTICAST_PHS);

			damage = 0;
			for (j = 0; j<15; j++)
				damage += (Q_rand() & 7) + 1;

			T_Damage(tr.ent, self, self->owner, vec3_origin, vec3_origin, vec3_origin, damage, 0, DAMAGE_NO_PARTICLES, self->meansOfDeath);
		}
	}

	if (self->s.frame == 6)
		G_FreeEdict(self);
}

void bfg_doom_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY)) {
		G_FreeEdict(self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	// core explosion - prevents firing it into the wall/floor
	if (other->takedamage)
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 0, DAMAGE_NONE, self->meansOfDeath);

	gi.sound(self, CHAN_VOICE, gi.soundindex("doom/RXPLOD.wav"), 1, ATTN_NORM, 0);
	self->solid = SOLID_NOT;
	self->touch = NULL;
	VectorMA(self->s.origin, -1 * game.frameseconds, self->velocity, self->s.origin);
	VectorClear(self->velocity);
	self->s.modelindex = gi.modelindex("sprites/doom/BFE1.d2s");
	self->s.frame = 0;
	self->s.sound = 0;
	self->s.effects &= ~EF_ANIM_ALLFAST;
	self->think = bfg_doom_explode;
	self->nextthink = level.time + (game.frametime * 2);
	self->activator = NULL;
	self->enemy = other;
}

void fire_doom_bfg(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t *bfg;

	bfg = G_Spawn();
	VectorCopy(start, bfg->s.origin);
	VectorCopy(dir, bfg->movedir);
	bfg->s.angles[1] = (self->client ? self->client->v_angle[1] : self->s.angles[1]);
	VectorScale(dir, speed, bfg->velocity);
	bfg->movetype = MOVETYPE_FLYMISSILE;
	bfg->clipmask = MASK_SHOT;
	bfg->solid = SOLID_BBOX;
	bfg->s.effects |= EF_ANIM_ALLFAST;
	bfg->s.renderfx |= RF_FULLBRIGHT;
	VectorClear(bfg->mins);
	VectorClear(bfg->maxs);
	bfg->s.modelindex = gi.modelindex("sprites/doom/BFS1.d2s");
	bfg->owner = self;
	bfg->touch = bfg_doom_touch;
	bfg->nextthink = level.time + (8000000.0f / speed);
	bfg->think = G_FreeEdict;
	bfg->dmg = damage;
	bfg->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), bfg, DT_DIRECT);

	if (self->client)
		check_dodge(self, bfg->s.origin, dir, speed);

	gi.linkentity(bfg);
}

void weapon_doom_bfg_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->yaw_speed > 0)
	{
		if (ent->client->ps.guns[gun].frame >= 3 && !ent->client->gunstates[gun].newweapon && (ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] >= 40)
		{
			ent->client->ps.guns[gun].frame = 0;
			ent->yaw_speed = 0;
			goto refire;
		}

		ent->yaw_speed -= 0.1f;
	}
	else
	{
		refire:
		switch (ent->client->ps.guns[gun].frame)
		{
		case 0:
			gi.WriteByte(svc_muzzleflash);
			gi.WriteShort(ent - g_edicts);
			gi.WriteByte(MZ_BFG | is_silenced);
			gi.multicast(ent->s.origin, MULTICAST_PVS);

			PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

			ent->yaw_speed = 0.5f;
			break;
		case 1:
			ent->yaw_speed = 0.2f;
			break;
		case 2:
			ent->yaw_speed = 0.2f;

			{
				vec3_t forward, right, start, offset;
				AngleVectors(ent->client->v_angle, forward, right, NULL);

				VectorSet(offset, 8, 0, ent->viewheight - 8);
				P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
				fire_doom_bfg(ent, start, forward, Doom_MissileDamageRandomizer(100), 850);
				Weapon_Doom_Anim_Regular(ent);

				if (!((int)dmflags->value & DF_INFINITE_AMMO))
					ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 40;
			}
			break;
		case 3:
			ent->yaw_speed = 0.8f;
			break;
		}

		ent->client->ps.guns[gun].frame++;
	}
}

void Weapon_BFG(edict_t *ent, gunindex_e gun)
{
	switch (ent->s.game)
	{
	case GAME_Q2:
	{
		static int  pause_frames[] = { 39, 45, 50, 55, 0 };
		static int  fire_frames[] = { 9, 17, 0 };

		Weapon_Generic(ent, 8, 32, 55, 58, pause_frames, fire_frames, weapon_bfg_fire, gun);
	}
	break;
	case GAME_DOOM:
		if (ent->client->gunstates[gun].weaponstate != WEAPON_FIRING)
			ent->yaw_speed = 0;

		Weapon_Doom(ent, 5, weapon_doom_bfg_fire, gun);
		break;
	}
}


//======================================================================


/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the blaster and hyper blaster.
=================
*/
void laser_q1_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY)) {
		G_FreeEdict(self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	vec3_t org, dir;
	VectorNormalize2(self->velocity, dir);
	VectorMA(self->s.origin, -8, dir, org);

	gi.sound(self, CHAN_WEAPON, gi.soundindex("q1/enforcer/enfstop.wav"), 1, ATTN_STATIC, 0);

	if (other->takedamage)
	{
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 0, DAMAGE_Q1, self->meansOfDeath);
		//spawn_touchblood(self, self->dmg);
	}
	else {
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_Q1_GUNSHOT);
		gi.WriteByte(1);
		gi.WritePosition(org);
		gi.multicast(org, MULTICAST_PVS);
	}

	G_FreeEdict(self);
}

void fire_q1_laser(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t *bolt;

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->svflags = SVF_DEADMONSTER;
	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster/hyperblaster shots), the player won't be solid clipped against
	// the object.  Right now trying to run into a firing hyperblaster
	// is very jerky since you are predicted 'against' the shots.
	VectorCopy(start, bolt->s.origin);
	VectorCopy(start, bolt->s.old_origin);
	vectoangles(dir, bolt->s.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	VectorClear(bolt->mins);
	VectorClear(bolt->maxs);
	bolt->s.modelindex = gi.modelindex("models/q1/laser.mdl");
	bolt->owner = self;
	bolt->touch = laser_q1_touch;
	bolt->nextthink = level.time + 5000;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	gi.linkentity(bolt);

	bolt->meansOfDeath = MakeAttackerMeansOfDeath(self, bolt, MD_NONE, DT_DIRECT);

	if (self->client)
		check_dodge(self, bolt->s.origin, dir, speed);

	trace_t tr = gi.trace(self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0f) {
		VectorMA(bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}
}

void Weapons_Init()
{
	pipe_detonator_index = gi.modelindex("%wv_detonator");
	pipe_bomb_index = gi.modelindex("%wv_grenades");
	foot_index = gi.modelindex("weaponscripts/duke/v_off_foot.wsc");
}
