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
// p_doom_weapons.c

#include "g_local.h"

extern bool     is_quad;

void Weapon_Q1(edict_t *ent, int num_frames, G_WeaponRunFunc fire, player_gun_t *gun, gun_state_t *gun_state)
{
	if (ent->deadflag || !ent->server.solid || ent->freeze_time > level.time) // VWep animations screw up corpses
		return;

	/*if (ent->server.client->next_weapon_update > ent->server.client->player_time)
		return;

	ent->server.client->next_weapon_update = ent->server.client->player_time + 100;*/

	switch (gun_state->weaponstate)
	{
		case WEAPON_ACTIVATING:
		case WEAPON_READY:
			gun_state->weaponstate = WEAPON_READY;

			if ((gun_state->newweapon) && (gun_state->weaponstate != WEAPON_FIRING))
			{
				ChangeWeapon(ent, gun, gun_state);
				return;
			}

			if ((ent->server.client->latched_buttons | ent->server.client->buttons) & BUTTON_ATTACK)
			{
				ent->server.client->latched_buttons &= ~BUTTON_ATTACK;

				if ((level.time - ent->server.client->respawn_time) > 500 &&
					ent->attack_finished_time < ent->server.client->player_time)
				{
					if ((game_iteminfos[ent->server.state.game].ammo_usages[ITEM_INDEX(ent->server.client->pers.weapon)] <= 0) ||
						(ent->server.client->pers.ammo >= GetWeaponUsageCount(ent, ent->server.client->pers.weapon)))
					{
						fire(ent, gun, gun_state);
						gun_state->weaponstate = WEAPON_FIRING;
					}
					else
						NoAmmoWeaponChange(ent, gun_state);
				}
			}

			break;

		case WEAPON_FIRING:
			fire(ent, gun, gun_state);

			if (gun->frame >= num_frames)
			{
				gun->frame = 0;
				gun_state->weaponstate = WEAPON_READY;
			}

			break;
	}
}

void Weapon_Q1_Anim_Shot(edict_t *ent)
{
	if (ent->health <= 0)
		return;

	ent->server.client->anim_priority = ANIM_ATTACK;
	ent->server.state.frame = 113 - 1;
	ent->server.client->anim_end = 118;
	Weapon_QuadDamage(ent);
}

void Weapon_Q1_Anim_Rock(edict_t *ent)
{
	if (ent->health <= 0)
		return;

	ent->server.client->anim_priority = ANIM_ATTACK;
	ent->server.state.frame = 107 - 1;
	ent->server.client->anim_end = 112;
	Weapon_QuadDamage(ent);
}

void Weapon_Q1_Anim_Nail(edict_t *ent)
{
	if (ent->health <= 0)
		return;

	ent->server.client->anim_priority = ANIM_ATTACK;
	ent->server.state.frame = (ent->server.state.frame == 103 ? 104 : 103) - 1;
	ent->server.client->anim_end = 104;
	Weapon_QuadDamage(ent);
}

void Weapon_Q1_Anim_Light(edict_t *ent)
{
	if (ent->health <= 0)
		return;

	ent->server.client->anim_priority = ANIM_ATTACK;
	ent->server.state.frame = (ent->server.state.frame == 105 ? 106 : 105) - 1;
	ent->server.client->anim_end = 106;
	Weapon_QuadDamage(ent);
}


void Q1_SpawnBlood(vec3_t org, vec3_t vel, int damage);

#define MAX_MULTIHIT 32

static struct
{
	edict_t		*hit;
	int			damage;
	int			kick;
	int			old_clipmask;
	vec3_t		point, normal;
} multi_ent[MAX_MULTIHIT];
static int multi_hits = 0;

static void ClearMultiDamage()
{
	for (int i = 0; i < multi_hits; ++i)
	{
		multi_ent[i].hit->hit_index = 0;
		multi_ent[i].hit->server.state.clip_contents = multi_ent[i].old_clipmask;
	}

	memset(multi_ent, 0, sizeof(multi_ent));
	multi_hits = 0;
}

void ApplyMultiDamage(edict_t *self, vec3_t aimdir, int dflags, meansOfDeath_t multi_mod)
{
	for (int i = 0; i < multi_hits; ++i)
		T_Damage(multi_ent[i].hit, self, self, aimdir, multi_ent[i].point, multi_ent[i].normal, multi_ent[i].damage, multi_ent[i].kick, dflags, multi_mod);

	ClearMultiDamage();
}

void AddMultiDamage(edict_t *hit, int damage, int kick, meansOfDeath_t multi_mod, int dflags, bool absorb_all, vec3_t point, vec3_t normal)
{
	if (!hit)
		return;

	if (hit->hit_index == 0)
	{
		if (multi_hits == MAX_MULTIHIT)
			Com_Error(ERR_FATAL, "Too many multihits");

		hit->hit_index = ++multi_hits;
		multi_ent[hit->hit_index - 1].hit = hit;
		multi_ent[hit->hit_index - 1].old_clipmask = hit->server.state.clip_contents;
	}

	multi_ent[hit->hit_index - 1].damage += damage;
	multi_ent[hit->hit_index - 1].kick += kick;
	VectorCopy(point, multi_ent[hit->hit_index - 1].point);
	VectorCopy(normal, multi_ent[hit->hit_index - 1].normal);

	if (!absorb_all)
	{
		int total_to_absorb = hit->health;

		if (hit->gib_health && hit->server.state.game != GAME_DOOM)
			total_to_absorb += -hit->gib_health;

		// traces will skip this for future shots
		if (multi_ent[hit->hit_index - 1].damage >= total_to_absorb)
			hit->server.state.clip_contents = 0;
	}
}

static void TraceAttack(edict_t *ent, trace_t *tr, int damage, vec3_t dir, vec3_t v_up, vec3_t v_right, meansOfDeath_t mod)
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
		Q1_SpawnBlood(org, vel, damage);
		AddMultiDamage(tr->ent, damage, 0, mod, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, true, tr->endpos, tr->plane.normal);
	}
	else
	{
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_Q1_GUNSHOT);
		MSG_WriteByte(1);
		MSG_WritePos(org);
		gi.multicast(org, MULTICAST_PVS);
	}
}

void FireBullets(edict_t *ent, int shotcount, int damage, vec3_t dir, vec3_t spread, vec3_t v_up, vec3_t v_right, meansOfDeath_t mod)
{
	vec3_t direction, src;
	vec3_t forward, right, up;

	if (ent->server.client)
		AngleVectors(ent->server.client->v_angle, forward, right, up);
	else
		AngleVectors(ent->server.state.angles, forward, right, up);

	VectorMA(ent->server.state.origin, 10, forward, src);
	src[2] += ent->viewheight - 8;
	vec3_t end;
	VectorMA(src, 2048, dir, end);
	trace_t tr = gi.trace(src, vec3_origin, vec3_origin, end, ent, MASK_BULLET);

	while (shotcount > 0)
	{
		for (int i = 0; i < 3; ++i)
			direction[i] = dir[i] + crandom() * spread[0] * right[i] + crandom() * spread[1] * up[i];

		VectorMA(src, 2048, direction, end);
		tr = gi.trace(src, vec3_origin, vec3_origin, end, ent, MASK_BULLET);

		if (tr.fraction != 1.0f)
			TraceAttack(ent, &tr, damage, direction, v_up, v_right, mod);

		shotcount = shotcount - 1;
	}

	ApplyMultiDamage(ent, dir, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, mod);
}

void Q1Grenade_Explode(edict_t *ent)
{
	vec3_t      origin;
	
#ifdef ENABLE_COOP
	PlayerNoise(ent->server.owner, ent->server.state.origin, PNOISE_IMPACT);
#endif

	T_RadiusDamage(ent, ent->server.owner, ent->dmg, ent, DAMAGE_Q1, ent->dmg, ent->meansOfDeath);
	VectorMA(ent->server.state.origin, -0.02, ent->velocity, origin);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_Q1_EXPLODE);
	MSG_WritePos(origin);
	gi.multicast(ent->server.state.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void Q1Grenade_Touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->server.owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
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
	VectorCopy(start, grenade->server.state.origin);
	VectorScale(aimdir, 600, grenade->velocity);
	VectorMA(grenade->velocity, 200 + crandom() * 10.0f, up, grenade->velocity);
	VectorMA(grenade->velocity, crandom() * 10.0f, right, grenade->velocity);
	VectorSet(grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->server.clipmask = MASK_SHOT;
	grenade->server.solid = SOLID_BBOX;
	grenade->server.state.effects |= EF_GRENADE;
	grenade->server.state.game = GAME_Q1;
	VectorClear(grenade->server.mins);
	VectorClear(grenade->server.maxs);
	grenade->server.state.modelindex = gi.modelindex("models/q1/grenade.mdl");
	grenade->server.owner = self;
	grenade->touch = Q1Grenade_Touch;
	grenade->nextthink = level.time + (timer * 1000);
	grenade->think = Q1Grenade_Explode;
	grenade->dmg = damage;
	grenade->entitytype = ET_GRENADE;

	if (self->server.client)
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), grenade, DT_DIRECT);
	else
		grenade->meansOfDeath = MakeAttackerMeansOfDeath(self, grenade, MD_NONE, DT_DIRECT);

	gi.linkentity(grenade);
}

void weapon_q1_gl_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (gun->frame != 0)
	{
		if (gun->frame >= 6 && (ent->server.client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) && !gun_state->newweapon)
			gun->frame = 0;
		else
		{
			gun->frame++;
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
	AngleVectors(ent->server.client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
	fire_q1_grenade(ent, start, forward, damage, 2.5);

	G_SendMuzzleFlash(ent, MZ_GRENADE);

	gun->frame++;

	RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

	Weapon_Q1_Anim_Rock(ent);
	gun_state->kick_angles[0] = -1;
	gun_state->kick_time = level.time + 100;
}

void q1_rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->server.owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}
	
#ifdef ENABLE_COOP
	PlayerNoise(ent->server.owner, ent->server.state.origin, PNOISE_IMPACT);
#endif

	// calculate position for the explosion entity
	int damg = ent->dmg + random() * (20 * (120 / ent->dmg));

	if (other->takedamage)
	{
#ifdef ENABLE_COOP
		if (other->entitytype == ET_Q1_MONSTER_SHAMBLER)
			damg = damg * 0.5;	// mostly immune
#endif

		T_Damage(other, ent, ent->server.owner, ent->velocity, ent->server.state.origin, plane->normal, damg, 0, DAMAGE_Q1, ent->meansOfDeath);
	}

	T_RadiusDamage(ent, ent->server.owner, ent->dmg, other, DAMAGE_Q1, ent->dmg_radius, ent->meansOfDeath);
	VectorNormalize(ent->velocity);
	VectorMA(ent->server.state.origin, -8, ent->velocity, origin);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_Q1_EXPLODE);
	MSG_WritePos(origin);
	gi.multicast(ent->server.state.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void fire_q1_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int damage_radius, int speed)
{
	edict_t *rocket;
	rocket = G_Spawn();
	VectorCopy(start, rocket->server.state.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->server.state.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->server.clipmask = MASK_SHOT;
	rocket->server.solid = SOLID_BBOX;
	rocket->server.state.effects |= EF_ROCKET;
	rocket->server.state.game = GAME_Q1;
	VectorClear(rocket->server.mins);
	VectorClear(rocket->server.maxs);
	rocket->server.state.modelindex = gi.modelindex("%e_rocket");
	rocket->server.owner = self;
	rocket->touch = q1_rocket_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->dmg_radius = damage_radius;
	rocket->entitytype = ET_ROCKET;

#if ENABLE_COOP
	check_dodge(self, rocket->server.state.origin, dir, speed);
#endif

	if (self->server.client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	gi.linkentity(rocket);
}

void weapon_q1_rl_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (gun->frame != 0)
	{
		gun->frame++;
		return;
	}

	vec3_t  offset;
	vec3_t  forward, right;
	vec3_t  start;
	int     damage = 120;

	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 0, ent->viewheight - 8);
	AngleVectors(ent->server.client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
	fire_q1_rocket(ent, start, forward, damage, 120, 1000);

	G_SendMuzzleFlash(ent, MZ_ROCKET);

	gun->frame++;

	RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

	ent->attack_finished_time = ent->server.client->player_time + 700;
	Weapon_Q1_Anim_Rock(ent);
	gun_state->kick_angles[0] = -1;
}

void weapon_q1_axe_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (gun->frame == 0)
	{
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("q1/weapons/ax1.wav"), 1, ATTN_NORM, 0);

		if (random() < 0.5f)
			gun->frame = 0;
		else
			gun->frame = 4;

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

		ent->server.client->anim_priority = ANIM_ATTACK;
		ent->server.state.frame = start_anim - 1;
		ent->server.client->anim_end = start_anim + 4;
		Weapon_QuadDamage(ent);
	}
	else if (gun->frame == 4 || gun->frame == 8)
	{
		gun->frame = 9;
		return;
	}

	if (gun->frame == 2 || gun->frame == 6)
	{
		vec3_t source, org;
		vec3_t forward;
		AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
		VectorCopy(ent->server.state.origin, source);
		source[2] += ent->viewheight - 8;
		VectorMA(source, 64, forward, org);
		trace_t tr = gi.trace(source, vec3_origin, vec3_origin, org, ent, MASK_SHOT);

		if (tr.fraction != 1.0f)
		{
			VectorMA(tr.endpos, -4, forward, org);

			if (tr.ent->takedamage)
				T_Damage(tr.ent, ent, ent, vec3_origin, vec3_origin, vec3_origin, 20, 0, DAMAGE_Q1, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));
			else
			{
				// hit wall
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("q1/player/axhit2.wav"), 1, ATTN_NORM, 0);
				MSG_WriteByte(svc_temp_entity);
				MSG_WriteByte(TE_Q1_GUNSHOT);
				MSG_WriteByte(3);
				MSG_WritePos(org);
				gi.multicast(org, MULTICAST_PVS);
			}
		}
	}

	gun->frame++;
}

void LightningHit(edict_t *ent, edict_t *from, vec3_t pos, int damage)
{
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_Q1_LIGHTNINGBLOOD);
	MSG_WritePos(pos);
	gi.multicast(pos, MULTICAST_PVS);
	meansOfDeath_t mod;

	if (from->server.client)
		mod = MakeWeaponMeansOfDeath(from, GetIndexByItem(from->server.client->pers.weapon), from, DT_DIRECT);
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
		/*if (ent->server.client && .classname == "player")
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
		LightningHit(tr.ent, from, tr.endpos, damage);

	e2 = tr.ent;
	VectorSubtract(p1, f, p1f);
	VectorSubtract(p2, f, p2f);
	tr = gi.trace(p1f, vec3_origin, vec3_origin, p2f, ent, MASK_BULLET);

	if (tr.ent != e1 && tr.ent != e2 && tr.ent->takedamage)
		LightningHit(tr.ent, from, tr.endpos, damage);
}

void W_FireLightning(edict_t *ent, int damage, int blowup_damage)
{
	// explode if under water
	if (ent->waterlevel > 1)
	{
		meansOfDeath_t mod;

		if (ent->server.client)
			mod = MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_INDIRECT);
		else
			mod = MakeGenericMeansOfDeath(ent, MD_NONE, DT_INDIRECT);

		float cells = PLAYER_SHOTS_FOR_WEAPON(ent, ent->server.client->pers.weapon);
		ent->server.client->pers.ammo = 0;
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
	VectorCopy(ent->server.state.origin, org);
	org[2] += ent->viewheight - 8;
	vec3_t forward;
	AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
	vec3_t end;
	VectorMA(org, 600, forward, end);
	trace_t tr = gi.trace(org, vec3_origin, vec3_origin, end, ent, MASK_SOLID);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_Q1_LIGHTNING2);
	MSG_WriteShort(ent - g_edicts);
	MSG_WritePos(org);
	MSG_WritePos(tr.endpos);
	gi.multicast(org, MULTICAST_PVS);
	VectorMA(tr.endpos, 4, forward, end);
	LightningDamage(ent, org, end, ent, damage);
}

void weapon_q1_lightning_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (!(ent->server.client->buttons & BUTTON_ATTACK) || gun_state->newweapon || !HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon))
	{
		gun->frame = 5;
		return;
	}

	vec3_t      forward;
	int         damage = 30, blowup_damage = 35;
	AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);

	if (is_quad)
	{
		damage *= 4;
		blowup_damage *= 4;
	}

	// send muzzle flash
	if (gun_state->weaponstate != WEAPON_FIRING)
	{
		G_SendMuzzleFlash(ent, MZ_HYPERBLASTER);
		ent->decel = 0;
	}

	W_FireLightning(ent, damage, blowup_damage);

	if (HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon))
		RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

	if (gun->frame == 3)
		gun->frame = 1;
	else
		gun->frame++;

	Weapon_Q1_Anim_Light(ent);
	gun_state->kick_angles[0] = -1 * random();
	gun_state->kick_time = level.time + 100;
}

/*
================
spawn_touchblood
================
*/
void spawn_touchblood(edict_t *ent, vec3_t plane_normal, float damage)
{
	vec3_t vel, v_right, v_up, dir;

	VectorNormalize2(ent->velocity, dir);

	AngleVectors(dir, NULL, v_right, v_up);

	VectorMA(dir, crandom(), v_up, vel);
	VectorMA(vel, crandom(), v_right, vel);
	VectorNormalize(vel);
	VectorMA(vel, 2, plane_normal, vel);
	VectorScale(vel, 200 * 0.02f, vel);
	Q1_SpawnBlood(ent->server.state.origin, vel, damage);
};

/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the blaster and hyper blaster.
=================
*/
void spike_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->server.owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}
	
#ifdef ENABLE_COOP
	PlayerNoise(self->server.owner, self->server.state.origin, PNOISE_IMPACT);
#endif

	if (other->takedamage)
	{
		spawn_touchblood(self, vec3_origin, self->dmg);
		T_Damage(other, self, self->server.owner, self->velocity, self->server.state.origin, plane->normal, self->dmg, 0, DAMAGE_Q1, self->meansOfDeath);
	}
	else
	{
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(self->count);
		MSG_WritePos(self->server.state.origin);
		gi.multicast(self->server.state.origin, MULTICAST_PVS);
	}

	G_FreeEdict(self);
}

edict_t *fire_spike(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, bool super)
{
	edict_t *bolt;
	VectorNormalize(dir);
	bolt = G_Spawn();
	bolt->server.flags.deadmonster = true;
	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster/hyperblaster shots), the player won't be solid clipped against
	// the object.  Right now trying to run into a firing hyperblaster
	// is very jerky since you are predicted 'against' the shots.
	VectorCopy(start, bolt->server.state.origin);
	VectorCopy(start, bolt->server.state.old_origin);
	vectoangles(dir, bolt->server.state.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->server.state.game = GAME_Q1;
	bolt->server.clipmask = MASK_SHOT;
	bolt->server.solid = SOLID_BBOX;
	VectorClear(bolt->server.mins);
	VectorClear(bolt->server.maxs);
	bolt->count = (super) ? TE_Q1_SUPERSPIKE : TE_Q1_SPIKE;
	bolt->server.state.renderfx |= RF_PROJECTILE;

	if (super)
		bolt->server.state.modelindex = gi.modelindex("models/q1/s_spike.mdl");
	else
		bolt->server.state.modelindex = gi.modelindex("models/q1/spike.mdl");

	bolt->server.owner = self;
	bolt->touch = spike_touch;
	bolt->nextthink = level.time + 6000;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->entitytype = ET_Q1_SPIKE;
	gi.linkentity(bolt);

	if (self->server.client)
		bolt->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), bolt, DT_DIRECT);
	else
		bolt->meansOfDeath = MakeAttackerMeansOfDeath(self, bolt, MD_NONE, DT_DIRECT);
	
#if ENABLE_COOP
	check_dodge(self, bolt->server.state.origin, dir, speed);
#endif

	/*tr = gi.trace(self->server.state.origin, NULL, NULL, bolt->server.state.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0) {
		VectorMA(bolt->server.state.origin, -10, dir, bolt->server.state.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}*/

	return bolt;
}

void W_FireSuperSpikes(edict_t *ent, int damage)
{
	vec3_t dir, right;
	AngleVectors(ent->server.client->v_angle, dir, right, NULL);
	vec3_t start;
	VectorSet(start, ent->server.state.origin[0], ent->server.state.origin[1], ent->server.state.origin[2] + 16);
	fire_spike(ent, start, dir, damage, 1000, true);
}

void W_FireSpikes(edict_t *ent, int damage, float ox)
{
	vec3_t dir, right;
	AngleVectors(ent->server.client->v_angle, dir, right, NULL);
	vec3_t start;
	VectorScale(right, ox, right);
	VectorSet(start, ent->server.state.origin[0], ent->server.state.origin[1], ent->server.state.origin[2] + ent->viewheight - 8);
	VectorAdd(start, right, start);
	fire_spike(ent, start, dir, damage, 1000, false);
}

void weapon_q1_nailgun_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (!(ent->server.client->buttons & BUTTON_ATTACK) || gun_state->newweapon || !HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon))
	{
		gun->frame = 9;
		return;
	}

	vec3_t      forward;
	int         damage = 9;
	AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);

	if (is_quad)
		damage *= 4;

	W_FireSpikes(ent, damage, (gun->frame % 2) == 0 ? 4 : -4);

	G_SendMuzzleFlash(ent, MZ_MACHINEGUN);

	RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

	if (gun->frame == 8)
		gun->frame = 1;
	else
		gun->frame++;

	Weapon_Q1_Anim_Nail(ent);
	gun_state->kick_angles[0] = -1 * random();
	gun_state->kick_time = level.time + 100;
}

void weapon_q1_snailgun_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (!(ent->server.client->buttons & BUTTON_ATTACK) || gun_state->newweapon || !HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon))
	{
		gun->frame = 9;
		return;
	}

	vec3_t      forward;
	int         damage = 18;
	AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);

	if (is_quad)
		damage *= 4;

	//if (ent->server.client->pers.inventory[gun_state->ammo_index] > 2)
	W_FireSuperSpikes(ent, damage);
	//else
	//	W_FireSpikes(ent, damage, (gun->frame % 2) == 0 ? 4 : -4);

	G_SendMuzzleFlash(ent, MZ_CHAINGUN1);

	RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

	if (gun->frame == 8)
		gun->frame = 1;
	else
		gun->frame++;

	Weapon_Q1_Anim_Nail(ent);
	gun_state->kick_angles[0] = -1 * random();
	gun_state->kick_time = level.time + 100;
}

void weapon_q1_shotgun_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (gun->frame != 0)
	{
		if (gun->frame >= 5 && (ent->server.client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) && !gun_state->newweapon)
			gun->frame = 0;
		else
		{
			gun->frame++;
			return;
		}
	}

	vec3_t      forward, right, up;
	int         damage = 4;
	AngleVectors(ent->server.client->v_angle, forward, right, up);

	if (is_quad)
		damage *= 4;

	vec3_t spread = { 0.04, 0.04, 0 };
	FireBullets(ent, 6, damage, forward, spread, up, right, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));

	G_SendMuzzleFlash(ent, MZ_SHOTGUN);
	
	gun->frame++;

	RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

	Weapon_Q1_Anim_Shot(ent);
	gun_state->kick_time = level.time + 100;
	gun_state->kick_angles[0] = -1;
	ent->attack_finished_time = level.time + 600;
}

void weapon_q1_sshotgun_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (gun->frame != 0)
	{
		gun->frame++;
		return;
	}

	/*if (ent->server.client->pers.inventory[gun_state->ammo_index] == 1)
	{
		weapon_q1_shotgun_fire(ent, gun);
		return;
	}*/
	vec3_t      forward, right, up;
	int         damage = 4;
	AngleVectors(ent->server.client->v_angle, forward, right, up);

	if (is_quad)
		damage *= 4;

	vec3_t spread = { 0.14, 0.08, 0 };
	FireBullets(ent, 14, damage, forward, spread, up, right, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));

	G_SendMuzzleFlash(ent, MZ_SSHOTGUN);

	gun->frame++;

	RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

	Weapon_Q1_Anim_Shot(ent);
	gun_state->kick_angles[0] = -3;
	gun_state->kick_time = level.time + 100;
}

void laser_q1_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->server.owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}
	
#ifdef ENABLE_COOP
	PlayerNoise(self->server.owner, self->server.state.origin, PNOISE_IMPACT);
#endif

	vec3_t org, dir;
	VectorNormalize2(self->velocity, dir);
	VectorMA(self->server.state.origin, -8, dir, org);
	gi.sound(self, CHAN_WEAPON, gi.soundindex("q1/enforcer/enfstop.wav"), 1, ATTN_STATIC, 0);

	if (other->takedamage)
	{
		spawn_touchblood(self, vec3_origin, self->dmg);
		T_Damage(other, self, self->server.owner, self->velocity, self->server.state.origin, plane->normal, self->dmg, 0, DAMAGE_Q1, self->meansOfDeath);
	}
	else
	{
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_Q1_GUNSHOT);
		MSG_WriteByte(1);
		MSG_WritePos(org);
		gi.multicast(org, MULTICAST_PVS);
	}

	G_FreeEdict(self);
}

void fire_q1_laser(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t *bolt;
	VectorNormalize(dir);
	bolt = G_Spawn();
	bolt->server.flags.deadmonster = true;
	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster/hyperblaster shots), the player won't be solid clipped against
	// the object.  Right now trying to run into a firing hyperblaster
	// is very jerky since you are predicted 'against' the shots.
	VectorCopy(start, bolt->server.state.origin);
	VectorCopy(start, bolt->server.state.old_origin);
	vectoangles(dir, bolt->server.state.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->server.clipmask = MASK_SHOT;
	bolt->server.solid = SOLID_BBOX;
	VectorClear(bolt->server.mins);
	VectorClear(bolt->server.maxs);
	bolt->server.state.modelindex = gi.modelindex("models/q1/laser.mdl");
	bolt->server.owner = self;
	bolt->touch = laser_q1_touch;
	bolt->nextthink = level.time + 5000;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	gi.linkentity(bolt);
	bolt->meansOfDeath = MakeAttackerMeansOfDeath(self, bolt, MD_NONE, DT_DIRECT);
	
#if ENABLE_COOP
	check_dodge(self, bolt->server.state.origin, dir, speed);
#endif

	trace_t tr = gi.trace(self->server.state.origin, NULL, NULL, bolt->server.state.origin, bolt, MASK_SHOT);

	if (tr.fraction < 1.0f)
	{
		VectorMA(bolt->server.state.origin, -10, dir, bolt->server.state.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}
}

static void Weapon_Q1_Axe(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Q1(ent, 9, weapon_q1_axe_fire, gun, gun_state);
}

static void Weapon_Q1_Shotgun(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Q1(ent, 7, weapon_q1_shotgun_fire, gun, gun_state);
}

static void Weapon_Q1_SuperShotgun(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Q1(ent, 7, weapon_q1_sshotgun_fire, gun, gun_state);
}

static void Weapon_Q1_Nailgun(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Q1(ent, 9, weapon_q1_nailgun_fire, gun, gun_state);
}

static void Weapon_Q1_SuperNailgun(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Q1(ent, 8, weapon_q1_snailgun_fire, gun, gun_state);
}

static void Weapon_Q1_GrenadeLauncher(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Q1(ent, 7, weapon_q1_gl_fire, gun, gun_state);
}

static void Weapon_Q1_RocketLauncher(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Q1(ent, 7, weapon_q1_rl_fire, gun, gun_state);
}

static void Weapon_Q1_Thunderbolt(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Q1(ent, 4, weapon_q1_lightning_fire, gun, gun_state);
}

static G_WeaponRunFunc q1_weapon_run_funcs[] =
{
	[ITI_Q1_AXE - ITI_WEAPONS_START] = Weapon_Q1_Axe,
	[ITI_Q1_SHOTGUN - ITI_WEAPONS_START] = Weapon_Q1_Shotgun,
	[ITI_Q1_SUPER_SHOTGUN - ITI_WEAPONS_START] = Weapon_Q1_SuperShotgun,
	[ITI_Q1_NAILGUN - ITI_WEAPONS_START] = Weapon_Q1_Nailgun,
	[ITI_Q1_SUPER_NAILGUN - ITI_WEAPONS_START] = Weapon_Q1_SuperNailgun,
	[ITI_Q1_GRENADE_LAUNCHER - ITI_WEAPONS_START] = Weapon_Q1_GrenadeLauncher,
	[ITI_Q1_ROCKET_LAUNCHER - ITI_WEAPONS_START] = Weapon_Q1_RocketLauncher,
	[ITI_Q1_THUNDERBOLT - ITI_WEAPONS_START] = Weapon_Q1_Thunderbolt
};

void Weapon_Q1_Run(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	q1_weapon_run_funcs[ITEM_INDEX(ent->server.client->pers.weapon) - ITI_WEAPONS_START](ent, gun, gun_state);
}