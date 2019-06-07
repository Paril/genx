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
extern byte     is_silenced;

void Weapon_Q1(edict_t *ent, int num_frames, void(*fire)(edict_t *ent, gunindex_e gun), gunindex_e gun)
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
					if ((game_iteminfos[ent->s.game].ammo_usages[ITEM_INDEX(ent->client->pers.weapon)] <= 0) ||
						(ent->client->pers.ammo >= GetWeaponUsageCount(ent, ent->client->pers.weapon)))
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
		multi_ent[i].hit->s.clip_contents = multi_ent[i].old_clipmask;
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
			gi.error("Too many multihits");

		hit->hit_index = ++multi_hits;
		multi_ent[hit->hit_index - 1].hit = hit;
		multi_ent[hit->hit_index - 1].old_clipmask = hit->s.clip_contents;
	}

	multi_ent[hit->hit_index - 1].damage += damage;
	multi_ent[hit->hit_index - 1].kick += kick;
	VectorCopy(point, multi_ent[hit->hit_index - 1].point);
	VectorCopy(normal, multi_ent[hit->hit_index - 1].normal);

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

void weapon_q1_gl_fire(edict_t *ent, gunindex_e gun)
{
	if (ent->client->ps.guns[gun].frame != 0)
	{
		if (ent->client->ps.guns[gun].frame >= 6 && (ent->client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->client->pers.weapon) && !ent->client->gunstates[gun].newweapon)
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
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

	Weapon_Q1_Anim_Rock(ent);
	ent->client->gunstates[gun].kick_angles[0] = -1;
	ent->client->gunstates[gun].kick_time = level.time + 100;
}

void q1_rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
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
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

	ent->attack_finished_time = ent->client->player_time + 700;
	Weapon_Q1_Anim_Rock(ent);
	ent->client->gunstates[gun].kick_angles[0] = -1;
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
		LightningHit(tr.ent, from, tr.endpos, damage);

	e2 = tr.ent;
	VectorSubtract(p1, f, p1f);
	VectorSubtract(p2, f, p2f);
	tr = gi.trace(p1f, vec3_origin, vec3_origin, p2f, ent, MASK_BULLET);

	if (tr.ent != e1 && tr.ent != e2 && tr.ent->takedamage)
		LightningHit(tr.ent, from, tr.endpos, damage);
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

		float cells = PLAYER_SHOTS_FOR_WEAPON(ent, ent->client->pers.weapon);
		ent->client->pers.ammo = 0;
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
	if (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->gunstates[gun].newweapon || !HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
	{
		ent->client->ps.guns[gun].frame = 5;
		return;
	}

	vec3_t      forward;
	int         damage = 30, blowup_damage = 35;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);

	if (is_quad)
	{
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

	if (!((int)dmflags->value & DF_INFINITE_AMMO) && HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

	if (ent->client->ps.guns[gun].frame == 3)
		ent->client->ps.guns[gun].frame = 1;
	else
		ent->client->ps.guns[gun].frame++;

	Weapon_Q1_Anim_Light(ent);
	ent->client->gunstates[gun].kick_angles[0] = -1 * random();
	ent->client->gunstates[gun].kick_time = level.time + 100;
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
	Q1_SpawnBlood(ent->s.origin, vel, damage);
};

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

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		spawn_touchblood(self, vec3_origin, self->dmg);
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 0, DAMAGE_Q1, self->meansOfDeath);
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
	if (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->gunstates[gun].newweapon || !HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
	{
		ent->client->ps.guns[gun].frame = 9;
		return;
	}

	vec3_t      forward;
	int         damage = 9;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);

	if (is_quad)
		damage *= 4;

	W_FireSpikes(ent, damage, (ent->client->ps.guns[gun].frame % 2) == 0 ? 4 : -4);
	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_MACHINEGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

	if (ent->client->ps.guns[gun].frame == 8)
		ent->client->ps.guns[gun].frame = 1;
	else
		ent->client->ps.guns[gun].frame++;

	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);
	Weapon_Q1_Anim_Nail(ent);
	ent->client->gunstates[gun].kick_angles[0] = -1 * random();
	ent->client->gunstates[gun].kick_time = level.time + 100;
}

void weapon_q1_snailgun_fire(edict_t *ent, gunindex_e gun)
{
	if (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->gunstates[gun].newweapon || !HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
	{
		ent->client->ps.guns[gun].frame = 9;
		return;
	}

	vec3_t      forward;
	int         damage = 18;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);

	if (is_quad)
		damage *= 4;

	//if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] > 2)
	W_FireSuperSpikes(ent, damage);
	//else
	//	W_FireSpikes(ent, damage, (ent->client->ps.guns[gun].frame % 2) == 0 ? 4 : -4);
	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_CHAINGUN1 | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

	if (ent->client->ps.guns[gun].frame == 8)
		ent->client->ps.guns[gun].frame = 1;
	else
		ent->client->ps.guns[gun].frame++;

	Weapon_Q1_Anim_Nail(ent);
	ent->client->gunstates[gun].kick_angles[0] = -1 * random();
	ent->client->gunstates[gun].kick_time = level.time + 100;
}

void weapon_q1_shotgun_fire(edict_t *ent, gunindex_e gun)
{
	if (ent->client->ps.guns[gun].frame != 0)
	{
		if (ent->client->ps.guns[gun].frame >= 5 && (ent->client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->client->pers.weapon) && !ent->client->gunstates[gun].newweapon)
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

	if (is_quad)
		damage *= 4;

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
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

	Weapon_Q1_Anim_Shot(ent);
	ent->client->gunstates[gun].kick_time = level.time + 100;
	ent->client->gunstates[gun].kick_angles[0] = -1;
	ent->attack_finished_time = level.time + 600;
}

void weapon_q1_sshotgun_fire(edict_t *ent, gunindex_e gun)
{
	if (ent->client->ps.guns[gun].frame != 0)
	{
		ent->client->ps.guns[gun].frame++;
		return;
	}

	/*if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] == 1)
	{
		weapon_q1_shotgun_fire(ent, gun);
		return;
	}*/
	vec3_t      forward, right, up;
	int         damage = 4;
	AngleVectors(ent->client->v_angle, forward, right, up);

	if (is_quad)
		damage *= 4;

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
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

	Weapon_Q1_Anim_Shot(ent);
	ent->client->gunstates[gun].kick_angles[0] = -3;
	ent->client->gunstates[gun].kick_time = level.time + 100;
}

void laser_q1_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
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
		spawn_touchblood(self, vec3_origin, self->dmg);
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 0, DAMAGE_Q1, self->meansOfDeath);
	}
	else
	{
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

	if (tr.fraction < 1.0f)
	{
		VectorMA(bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}
}

static void Weapon_Q1_Axe(edict_t *ent, gunindex_e gun)
{
	Weapon_Q1(ent, 9, weapon_q1_axe_fire, gun);
}

static void Weapon_Q1_Shotgun(edict_t *ent, gunindex_e gun)
{
	Weapon_Q1(ent, 7, weapon_q1_shotgun_fire, gun);
}

static void Weapon_Q1_SuperShotgun(edict_t *ent, gunindex_e gun)
{
	Weapon_Q1(ent, 7, weapon_q1_sshotgun_fire, gun);
}

static void Weapon_Q1_Nailgun(edict_t *ent, gunindex_e gun)
{
	Weapon_Q1(ent, 9, weapon_q1_nailgun_fire, gun);
}

static void Weapon_Q1_SuperNailgun(edict_t *ent, gunindex_e gun)
{
	Weapon_Q1(ent, 8, weapon_q1_snailgun_fire, gun);
}

static void Weapon_Q1_GrenadeLauncher(edict_t *ent, gunindex_e gun)
{
	Weapon_Q1(ent, 7, weapon_q1_gl_fire, gun);
}

static void Weapon_Q1_RocketLauncher(edict_t *ent, gunindex_e gun)
{
	Weapon_Q1(ent, 7, weapon_q1_rl_fire, gun);
}

static void Weapon_Q1_Thunderbolt(edict_t *ent, gunindex_e gun)
{
	Weapon_Q1(ent, 4, weapon_q1_lightning_fire, gun);
}

void Weapon_Q1_Run(edict_t *ent, gunindex_e gun)
{
	switch (ITEM_INDEX(ent->client->pers.weapon))
	{
		case ITI_Q1_AXE:
			Weapon_Q1_Axe(ent, gun);
			break;

		case ITI_Q1_SHOTGUN:
			Weapon_Q1_Shotgun(ent, gun);
			break;

		case ITI_Q1_SUPER_SHOTGUN:
			Weapon_Q1_SuperShotgun(ent, gun);
			break;

		case ITI_Q1_NAILGUN:
			Weapon_Q1_Nailgun(ent, gun);
			break;

		case ITI_Q1_SUPER_NAILGUN:
			Weapon_Q1_SuperNailgun(ent, gun);
			break;

		case ITI_Q1_GRENADE_LAUNCHER:
			Weapon_Q1_GrenadeLauncher(ent, gun);
			break;

		case ITI_Q1_ROCKET_LAUNCHER:
			Weapon_Q1_RocketLauncher(ent, gun);
			break;

		case ITI_Q1_THUNDERBOLT:
			Weapon_Q1_Thunderbolt(ent, gun);
			break;
	}
}