#include "g_local.h"


const int Q1_WEAPON_BIG2 = 1;

void SP_q1_item_ammo(edict_t *self)
{
	int old_spawnflags = self->spawnflags;
	self->spawnflags = 0;
	gitem_t *item;

	if (self->entitytype == ET_Q1_ITEM_SHELLS)
	{
		if (old_spawnflags & Q1_WEAPON_BIG2)
			item = GetItemByIndex(ITI_SHELLS);
		else
			item = GetItemByIndex(ITI_SHELLS_LARGE);
	}
	else if (self->entitytype == ET_Q1_ITEM_SPIKES)
	{
		if (old_spawnflags & Q1_WEAPON_BIG2)
			item = GetItemByIndex(ITI_BULLETS);
		else
			item = GetItemByIndex(ITI_BULLETS_LARGE);
	}
	else if (self->entitytype == ET_Q1_ITEM_ROCKETS)
	{
		if (old_spawnflags & Q1_WEAPON_BIG2)
			item = GetItemByIndex(ITI_ROCKETS);
		else
			item = GetItemByIndex(ITI_GRENADES);
	}
	else if (self->entitytype == ET_Q1_ITEM_CELLS)
	{
		if (old_spawnflags & Q1_WEAPON_BIG2)
			item = GetItemByIndex(ITI_SLUGS);
		else
			item = GetItemByIndex(ITI_CELLS);
	}
	else
		return;

	self->s.origin[0] += 16;
	self->s.origin[1] += 16;
	self->s.origin[2] += 18;

	SpawnItem(self, item);
}


/*QUAKED item_weapon (0 .5 .8) (0 0 0) (32 32 32) shotgun rocket spikes big
DO NOT USE THIS!!!! IT WILL BE REMOVED!
*/

#define WEAPON_SHOTGUN 1
#define WEAPON_ROCKET 2
#define WEAPON_SPIKES 4
#define WEAPON_BIG 8
void SP_q1_item_weapon(edict_t *self)
{
	int sf = self->spawnflags;
	self->spawnflags = (sf & WEAPON_BIG) ? Q1_WEAPON_BIG2 : 0;

	if (sf & WEAPON_SHOTGUN)
		self->entitytype = ET_Q1_ITEM_SHELLS;
	else if (sf & WEAPON_SPIKES)
		self->entitytype = ET_Q1_ITEM_SPIKES;
	else if (sf & WEAPON_ROCKET)
		self->entitytype = ET_Q1_ITEM_ROCKETS;
	else
		gi.error("???");

	SP_q1_item_ammo(self);
}

const int H_ROTTEN = 1;
const int H_MEGA = 2;

void SP_q1_item_health(edict_t *self)
{
	int old_spawnflags = self->spawnflags;
	self->spawnflags = 0;

	self->s.origin[0] += 16;
	self->s.origin[1] += 16;
	self->s.origin[2] += 18;

	if (old_spawnflags & H_ROTTEN)
		SpawnItem(self, GetItemByIndex(ITI_MEDIUM_HEALTH));
	else if (old_spawnflags & H_MEGA)
		SpawnItem(self, GetItemByIndex(ITI_MEGA_HEALTH));
	else
		SpawnItem(self, GetItemByIndex(ITI_LARGE_HEALTH));
}

void FireAmbient(edict_t *self)
{
	self->s.sound = gi.soundindex("q1/ambience/fire1.wav");
}

/*QUAKED light_torch_small_walltorch (0 .5 0) (-10 -10 -20) (10 10 20)
Short wall torch
Default light value is 200
Default style is 0
*/
void Torch_Animate(edict_t *self)
{
	self->s.frame = (self->s.frame + 1) % 6;
	self->nextthink = level.time + game.frametime;
}

void light_torch_small_walltorch(edict_t *self)
{
	gi.setmodel(self, "models/q1/flame.mdl");
	self->s.frame = Q_rand() % 6;
	self->think = Torch_Animate;
	self->nextthink = level.time + game.frametime;
	self->s.renderfx |= RF_FULLBRIGHT;
	FireAmbient(self);
	gi.linkentity(self);
}

void Fire_Large_Animate(edict_t *self)
{
	self->s.frame = 6 + ((self->s.frame - 6 + 1) % 11);
	self->nextthink = level.time + game.frametime;
}


/*QUAKED light_flame_large_yellow (0 1 0) (-10 -10 -12) (12 12 18)
Large yellow flame ball
*/
void light_flame_large_yellow(edict_t *self)
{
	gi.setmodel(self, "models/q1/flame2.mdl");
	self->s.frame = 6 + (Q_rand() % 11);
	self->think = Fire_Large_Animate;
	self->nextthink = level.time + game.frametime;
	self->s.renderfx |= RF_FULLBRIGHT;
	FireAmbient(self);
	gi.linkentity(self);
}

/*QUAKED light_flame_small_yellow (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Small yellow flame ball
*/
void light_flame_small_yellow(edict_t *self)
{
	gi.setmodel(self, "models/q1/flame2.mdl");
	self->s.frame = Q_rand() % 6;
	self->think = Torch_Animate;
	self->nextthink = level.time + game.frametime;
	self->s.renderfx |= RF_FULLBRIGHT;
	FireAmbient(self);
	gi.linkentity(self);
}

/*QUAKED light_flame_small_white (0 1 0) (-10 -10 -40) (10 10 40) START_OFF
Small white flame ball
*/
void light_flame_small_white(edict_t *self)
{
	gi.setmodel(self, "models/q1/flame2.mdl");
	self->s.frame = Q_rand() % 6;
	self->think = Torch_Animate;
	self->nextthink = level.time + game.frametime;
	self->s.renderfx |= RF_FULLBRIGHT;
	FireAmbient(self);
	gi.linkentity(self);
}

void SP_q1_weapon(edict_t *self)
{
	gitem_t *item;

	if (self->entitytype == ET_Q1_WEAPON_SUPERSHOTGUN)
		item = GetItemByIndex(ITI_Q1_SUPER_SHOTGUN);
	else if (self->entitytype == ET_Q1_WEAPON_NAILGUN)
		item = GetItemByIndex(ITI_Q1_NAILGUN);
	else if (self->entitytype == ET_Q1_WEAPON_SUPERNAILGUN)
		item = GetItemByIndex(ITI_Q1_SUPER_NAILGUN);
	else if (self->entitytype == ET_Q1_WEAPON_GRENADELAUNCHER)
		item = GetItemByIndex(ITI_Q1_GRENADE_LAUNCHER);
	else if (self->entitytype == ET_Q1_WEAPON_ROCKETLAUNCHER)
		item = GetItemByIndex(ITI_Q1_ROCKET_LAUNCHER);
	else if (self->entitytype == ET_Q1_WEAPON_LIGHTNING)
		item = GetItemByIndex(ITI_Q1_THUNDERBOLT);
	else
		return;

	self->s.origin[0] += 16;
	self->s.origin[1] += 16;
	self->s.origin[2] += 16;

	SpawnItem(self, item);
}

void SP_q1_item_armor(edict_t *self)
{
	gitem_t *item;

	if (self->entitytype == ET_Q1_ITEM_ARMOR1)
		item = GetItemByIndex(ITI_Q1_GREEN_ARMOR);
	else if (self->entitytype == ET_Q1_ITEM_ARMOR2)
		item = GetItemByIndex(ITI_Q1_YELLOW_ARMOR);
	else if (self->entitytype == ET_Q1_ITEM_ARMORINV)
		item = GetItemByIndex(ITI_Q1_RED_ARMOR);
	else
		return;

	self->s.origin[2] += 16;

	SpawnItem(self, item);
}

void SP_q1_item_artifact(edict_t *self)
{
	gitem_t *item;

	if (self->entitytype == ET_Q1_ITEM_ARTIFACT_ENVIROSUIT)
		item = GetItemByIndex(ITI_ENVIRONMENT_SUIT);
	else if (self->entitytype == ET_Q1_ITEM_ARTIFACT_INVISIBILITY)
		item = GetItemByIndex(ITI_Q1_INVISIBILITY);
	else if (self->entitytype == ET_Q1_ITEM_ARTIFACT_INVULNERABILITY)
		item = GetItemByIndex(ITI_INVULNERABILITY);
	else if (self->entitytype == ET_Q1_ITEM_ARTIFACT_SUPER_DAMAGE)
		item = GetItemByIndex(ITI_QUAD_DAMAGE);
	else
		return;

	self->s.origin[2] += 16;

	SpawnItem(self, item);
}

#define START_OFF   1

void light_use(edict_t *self, edict_t *other, edict_t *activator);

void light_fluoro(edict_t *self)
{
	if (self->style >= 32)
	{
		self->use = light_use;
		if (self->spawnflags & START_OFF)
			gi.configstring(CS_LIGHTS + self->style, "a");
		else
			gi.configstring(CS_LIGHTS + self->style, "m");
	}

	self->s.sound = gi.soundindex("q1/ambience/fl_hum1.wav");
	gi.linkentity(self);
}

void light_fluorospark(edict_t *self)
{
	if (!self->style)
		self->style = 10;

	self->s.sound = gi.soundindex("q1/ambience/buzz1.wav");
	gi.linkentity(self);
}

void ambientsound(edict_t *self, const char *sound)
{
	self->s.sound = gi.soundindex(sound);
	gi.linkentity(self);
}

//============================================================================
/*QUAKED ambient_suck_wind (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_suck_wind(edict_t *self)
{
	ambientsound(self, "q1/ambience/suck1.wav");
}

/*QUAKED ambient_drone (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_drone(edict_t *self)
{
	ambientsound(self, "q1/ambience/drone6.wav");
}

/*QUAKED ambient_flouro_buzz (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_flouro_buzz(edict_t *self)
{
	ambientsound(self, "q1/ambience/buzz1.wav");
}

/*QUAKED ambient_drip (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_drip(edict_t *self)
{
	ambientsound(self, "q1/ambience/drip1.wav");
}

/*QUAKED ambient_comp_hum (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_comp_hum(edict_t *self)
{
	ambientsound(self, "q1/ambience/comp1.wav");
}

/*QUAKED ambient_thunder (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_thunder(edict_t *self)
{
	ambientsound(self, "q1/ambience/thunder1.wav");
}

/*QUAKED ambient_light_buzz (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_light_buzz(edict_t *self)
{
	ambientsound(self, "q1/ambience/fl_hum1.wav");
}

/*QUAKED ambient_swamp1 (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_swamp1(edict_t *self)
{
	ambientsound(self, "q1/ambience/swamp1.wav");
}

/*QUAKED ambient_swamp2 (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void ambient_swamp2(edict_t *self)
{
	ambientsound(self, "q1/ambience/swamp2.wav");
}

#define PLAYER_ONLY 1
#define SILENT 2

void InitTrigger(edict_t *self);

#include <assert.h>

bool G_KillBox(edict_t *ent, vec3_t origin, vec3_t mins, vec3_t maxs);

void teleport_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t *t;

	if (self->targetname)
	{
		if (self->nextthink < level.time)
			return;		// not fired yet
	}

	if (self->spawnflags & PLAYER_ONLY)
	{
		if (!other->client)
			return;
	}

	// only teleport living creatures
	if (other->health <= 0 || other->solid != SOLID_BBOX || (!(other->svflags & SVF_MONSTER) && !other->client))
		return;

	G_UseTargets(self, self->activator);

	// put a tfog where the player was
	self->s.event = EV_PLAYER_TELEPORT;
	other->s.event = EV_PLAYER_TELEPORT;

	t = G_Find(NULL, FOFS(targetname), self->target);

	if (!t)
		gi.error("couldn't find target\n");

	vec3_t forward;
	AngleVectors(t->s.angles, forward, NULL, NULL);

	// move the player and lock him down for a little while
	gi.unlinkentity(other);

	if (other->client)
	{
		other->client->ps.pmove.pm_time = 160 >> 3;     // hold time
		other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

		for (int i = 0; i < 3; i++)
			other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(t->s.angles[i] - other->client->resp.cmd_angles[i]);

		VectorClear(other->client->ps.viewangles);
		VectorClear(other->client->v_angle);
		VectorClear(other->s.angles);
	}

	if (!other->health)
	{
		VectorCopy(t->s.origin, other->s.origin);

		vec3_t velA, velB;
		VectorScale(forward, other->velocity[0], velA);
		VectorScale(forward, other->velocity[1], velB);
		VectorAdd(velA, velB, other->velocity);
	}
	else
	{
		VectorCopy(t->s.origin, other->s.origin);

		if (!other->client)
			VectorCopy(t->s.angles, other->s.angles);
		else
		{
			if (other->groundentity)
				other->groundentity = NULL;

			VectorScale(forward, 300, other->velocity);
		}
	}

	vec3_t killmins = { -1, -1, -1 }, killmaxs = { 1, 1, 1 };
	VectorAdd(other->mins, killmins, other->mins);
	VectorAdd(other->maxs, killmaxs, other->maxs);

	// kill anything at the destination
	G_KillBox(self, other->s.origin, killmins, killmaxs);

	gi.linkentity(other);
}

void teleport_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->nextthink = level.time + 200;
}

void SP_trigger_teleport(edict_t *self)
{
	InitTrigger(self);

	self->touch = teleport_touch;
	// find the destination
	self->use = teleport_use;

	if (!(self->spawnflags & SILENT))
		ambientsound(self, "q1/ambience/hum1.wav");

	gi.setmodel(self, self->model);

	gi.linkentity(self);
}

void info_teleport_destination(edict_t *self)
{
	VectorCopy(self->s.origin, self->absmin);
	VectorCopy(self->s.origin, self->absmax);
	self->s.origin[2] += 27;
}


/*
=============================================================================

SECRET DOORS

=============================================================================
*/

void fd_secret_move1(edict_t *self);
void fd_secret_move2(edict_t *self);
void fd_secret_move3(edict_t *self);
void fd_secret_move4(edict_t *self);
void fd_secret_move5(edict_t *self);
void fd_secret_move6(edict_t *self);
void fd_secret_done(edict_t *self);

#define SECRET_OPEN_ONCE  1			// stays open
#define SECRET_1ST_LEFT   2			// 1st move is left of arrow
#define SECRET_1ST_DOWN   4			// 1st move is down from arrow
#define SECRET_NO_SHOOT   8			// only opened by trigger
#define SECRET_YES_SHOOT  16		// shootable even if targeted

void Move_Calc(edict_t *ent, vec3_t dest, void(*func)(edict_t*));

void fd_secret_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->health = 10000;

	// exit if still moving around...
	if (!VectorCompare(self->s.origin, vec3_origin))
		return;

	self->message = NULL;		// no more message

	G_UseTargets(self, activator);				// fire all targets / killtargets

	if (!(self->spawnflags & SECRET_NO_SHOOT))
	{
		self->pain = NULL;
		self->takedamage = DAMAGE_NO;
	}

	VectorClear(self->velocity);

	// Make a sound, wait a little...

	gi.sound(self, CHAN_VOICE, self->noise1, 1, ATTN_NORM, 0);
	self->nextthink = level.time + 100;

	int temp = 1 - (self->spawnflags & SECRET_1ST_LEFT);	// 1 or -1

	vec3_t v_forward, v_right, v_up;
	AngleVectors(self->move_angles, v_forward, v_right, v_up);

	if (!self->t_width)
	{
		if (self->spawnflags & SECRET_1ST_DOWN)
			self->t_width = fabsf(DotProduct(v_up, self->size)) - 2;
		else
			self->t_width = fabsf(DotProduct(v_right, self->size)) - 2;
	}

	if (!self->t_length)
		self->t_length = fabsf(DotProduct(v_forward, self->size)) - 2;

	if (self->spawnflags & SECRET_1ST_DOWN)
		VectorMA(self->s.origin, -self->t_width, v_up, self->dest1);
	else
		VectorMA(self->s.origin, (self->t_width * temp), v_right, self->dest1);

	VectorMA(self->dest1, self->t_length, v_forward, self->dest2);
	Move_Calc(self, self->dest1, fd_secret_move1);
	gi.sound(self, CHAN_VOICE, self->noise2, 1, ATTN_NORM, 0);
}

// Wait after first movement...
void fd_secret_move1(edict_t *self)
{
	self->nextthink = level.time + 1000;
	self->think = fd_secret_move2;
	gi.sound(self, CHAN_VOICE, self->noise3, 1, ATTN_NORM, 0);
}

// Start moving sideways w/sound...
void fd_secret_move2(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, self->noise2, 1, ATTN_NORM, 0);
	Move_Calc(self, self->dest2, fd_secret_move3);
}

// Wait here until time to go back...
void fd_secret_move3(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, self->noise3, 1, ATTN_NORM, 0);

	if (!(self->spawnflags & SECRET_OPEN_ONCE))
	{
		if (self->moveinfo.wait >= 0)
		{
			self->nextthink = level.time + self->moveinfo.wait * 1000;
			self->think = fd_secret_move4;
		}
	}
}

// Move backward...
void fd_secret_move4(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, self->noise2, 1, ATTN_NORM, 0);
	Move_Calc(self, self->dest1, fd_secret_move5);
}

// Wait 1 second...
void fd_secret_move5(edict_t *self)
{
	self->nextthink = level.time + 1000;
	self->think = fd_secret_move6;
	gi.sound(self, CHAN_VOICE, self->noise3, 1, ATTN_NORM, 0);
}

void fd_secret_move6(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, self->noise2, 1, ATTN_NORM, 0);
	Move_Calc(self, self->moveinfo.start_origin, fd_secret_done);
}

void fd_secret_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	fd_secret_use(self, attacker, attacker);
}

void fd_secret_pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	fd_secret_use(self, attacker, attacker);
}

void fd_secret_done(edict_t *self)
{
	if (!self->targetname || (self->spawnflags & SECRET_YES_SHOOT))
	{
		self->health = 10000;
		self->takedamage = DAMAGE_YES;
		self->pain = fd_secret_pain;
	}
	gi.sound(self, CHAN_VOICE, self->noise3, 1, ATTN_NORM, 0);
}

void secret_blocked(edict_t *self, edict_t *other)
{
	if (level.time < self->attack_finished_time)
		return;
	self->attack_finished_time = level.time + 500;
	T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, self->dmg, 0, DAMAGE_NONE, MakeBlankMeansOfDeath(self));
}

/*
================
secret_touch

Prints messages
================
*/
void secret_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;
	if (self->attack_finished_time > level.time)
		return;

	self->attack_finished_time = level.time + 2000;

	if (self->message)
	{
		gi.centerprintf(other, "%s", self->message);
		gi.sound(other, CHAN_BODY, gi.soundindex("misc/talk.wav"), 1, ATTN_NORM, 0);
	}
}


/*QUAKED func_door_secret (0 .5 .8) ? open_once 1st_left 1st_down no_shoot always_shoot
Basic secret door. Slides back, then to the side. Angle determines direction.
wait  = # of seconds before coming back
1st_left = 1st move is left of arrow
1st_down = 1st move is down from arrow
always_shoot = even if targeted, keep shootable
t_width = override WIDTH to move back (or height if going down)
t_length = override LENGTH to move sideways
"dmg"		damage to inflict when blocked (2 default)

If a secret door has a targetname, it will only be opened by it's botton or trigger, not by damage.
"sounds"
1) medieval
2) metal
3) base
*/

void func_door_secret(edict_t *self)
{
	if (self->sounds == 0)
		self->sounds = 3;
	if (self->sounds == 1)
	{
		self->noise1 = gi.soundindex("q1/doors/latch2.wav");
		self->noise2 = gi.soundindex("q1/doors/winch2.wav");
		self->noise3 = gi.soundindex("q1/doors/drclos4.wav");
	}
	if (self->sounds == 2)
	{
		self->noise2 = gi.soundindex("q1/doors/airdoor1.wav");
		self->noise1 = gi.soundindex("q1/doors/airdoor2.wav");
		self->noise3 = gi.soundindex("q1/doors/airdoor2.wav");
	}
	if (self->sounds == 3)
	{
		self->noise2 = gi.soundindex("q1/doors/basesec1.wav");
		self->noise1 = gi.soundindex("q1/doors/basesec2.wav");
		self->noise3 = gi.soundindex("q1/doors/basesec2.wav");
	}

	if (!self->dmg)
		self->dmg = 2;

	// Magic formula...
	VectorCopy(self->s.angles, self->move_angles);
	VectorClear(self->s.angles);
	self->solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	self->entitytype = ET_FUNC_DOOR;
	gi.setmodel(self, self->model);
	self->s.effects |= EF_ANIM_ALL;

	self->touch = secret_touch;
	self->blocked = secret_blocked;
	self->speed = 50;
	self->use = fd_secret_use;

	if (!self->targetname || self->spawnflags & SECRET_YES_SHOOT)
	{
		self->health = 10000;
		self->takedamage = DAMAGE_YES;
		self->pain = fd_secret_pain;
		self->die = fd_secret_die;
	}
	VectorCopy(self->s.origin, self->moveinfo.start_origin);

	if (!self->wait)
		self->wait = 5;		// 5 seconds before closing

	self->moveinfo.wait = self->wait;
	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->moveinfo.speed;
	self->moveinfo.decel = self->moveinfo.speed;

	gi.linkentity(self);
}

#define DOOR_START_OPEN		1
#define DOOR_DONT_LINK		4
#define DOOR_GOLD_KEY		8
#define DOOR_SILVER_KEY		16
#define DOOR_TOGGLE			32

/*

Doors are similar to buttons, but can spawn a fat trigger field around them
to open without a touch, and they link together to form simultanious
double/quad doors.

Door.owner is the master door.  If there is only one door, it points to itself.
If multiple doors, all will point to a single one.

Door.enemy chains from the master door through all doors linked in the chain.

*/

/*
=============================================================================

THINK FUNCTIONS

=============================================================================
*/

void q1_door_go_down(edict_t *self);
void q1_door_go_up(edict_t *self, edict_t *activator);

void q1_door_blocked(edict_t *self, edict_t *other)
{
	T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, self->dmg, 0, DAMAGE_NONE, MakeBlankMeansOfDeath(self));

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast
	if (self->moveinfo.wait >= 0)
	{
		if (self->moveinfo.state == STATE_DOWN)
			q1_door_go_up(self, self->activator);
		else
			q1_door_go_down(self);
	}
}

void q1_door_hit_top(edict_t *self)
{
	if (self->noise1)
		gi.sound(self, CHAN_VOICE, self->noise1, 1, ATTN_NORM, 0);
	self->moveinfo.state = STATE_TOP;
	if (self->spawnflags & DOOR_TOGGLE || self->moveinfo.wait < 0)
		return;		// don't come down automatically
	self->think = q1_door_go_down;
	self->nextthink = level.time + (self->moveinfo.wait * 1000);
}

void q1_door_hit_bottom(edict_t *self)
{
	if (self->noise1)
		gi.sound(self, CHAN_VOICE, self->noise1, 1, ATTN_NORM, 0);
	self->moveinfo.state = STATE_BOTTOM;
}

void q1_door_go_down(edict_t *self)
{
	if (self->noise2)
		gi.sound(self, CHAN_VOICE, self->noise2, 1, ATTN_NORM, 0);
	if (self->max_health)
	{
		self->takedamage = DAMAGE_YES;
		self->health = self->max_health;
	}

	self->moveinfo.state = STATE_DOWN;
	Move_Calc(self, self->pos1, q1_door_hit_bottom);
}

void q1_door_go_up(edict_t *self, edict_t *activator)
{
	if (self->moveinfo.state == STATE_UP)
		return;		// allready going up

	if (self->moveinfo.state == STATE_TOP)
	{	// reset top wait time
		if (self->moveinfo.wait >= 0)
			self->nextthink = level.time + (self->moveinfo.wait * 1000);
		return;
	}

	if (self->noise2)
		gi.sound(self, CHAN_VOICE, self->noise2, 1, ATTN_NORM, 0);
	self->moveinfo.state = STATE_UP;
	Move_Calc(self, self->pos2, q1_door_hit_top);

	G_UseTargets(self, activator);
}


/*
=============================================================================

ACTIVATION FUNCTIONS

=============================================================================
*/

void q1_door_fire(edict_t *self, edict_t *activator)
{
	edict_t *oself;
	edict_t *starte;

	if (self->owner != self)
		gi.error("door_fire: self.owner != self");

	// play use key sound

	if (self->item)
		//		sound (self, CHAN_VOICE, self.noise4, 1, ATTN_NORM);
		gi.sound(self, CHAN_ITEM, self->noise4, 1, ATTN_NORM, 0);

	self->message = NULL;		// no more message
	oself = self;

	if (self->spawnflags & DOOR_TOGGLE)
	{
		if (self->moveinfo.state == STATE_UP || self->moveinfo.state == STATE_TOP)
		{
			starte = self;
			do
			{
				q1_door_go_down(self);
				self = self->enemy;
			} while ((self != starte) && (self != world && self));
			self = oself;
			return;
		}
	}

	// trigger all paired doors
	starte = self;
	do
	{
		q1_door_go_up(self, activator);
		self = self->enemy;
	} while ((self != starte) && (self != world && self));
	self = oself;
}

void q1_door_use(edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *oself;

	self->message = NULL;			// door message are for touch only
	self->owner->message = NULL;
	self->enemy->message = NULL;
	oself = self;
	self = self->owner;
	q1_door_fire(self, activator);
	self = oself;
}


void q1_door_trigger_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->health <= 0)
		return;

	if (level.time < self->attack_finished_time)
		return;

	self->attack_finished_time = level.time + 1000;

	self->activator = other;

	self = self->owner;
	q1_door_use(self, other, self->activator);
}

void q1_door_killed(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	edict_t *oself;

	oself = self;
	self = self->owner;
	self->health = self->max_health;
	self->takedamage = DAMAGE_NO;	// wil be reset upon return
	q1_door_use(self, attacker, attacker);
	self = oself;
}


/*
================
door_touch

Prints messages and opens key doors
================
*/
void q1_door_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;
	if (self->owner->attack_finished_time > level.time)
		return;

	self->owner->attack_finished_time = level.time + 2000;

	if (self->owner->message && self->owner->message[0])
	{
		gi.centerprintf(other, "%s", self->owner->message);
		gi.sound(other, CHAN_VOICE, gi.soundindex("misc/talk.wav"), 1, ATTN_NORM, 0);
	}

	// key door stuff
	if (!self->item)
		return;

	// FIXME: blink key on player's status bar
	if (!other->client->pers.inventory[ITEM_INDEX(self->item)])
	{
		if (ITEM_INDEX(self->item) == ITI_Q1_SILVER_KEY)
		{
			if (level.world_type == 2)
			{
				gi.centerprintf(other, "You need the silver keycard");
				gi.sound(self, CHAN_VOICE, self->noise3, 1, ATTN_NORM, 0);
			}
			else if (level.world_type == 1)
			{
				gi.centerprintf(other, "You need the silver runekey");
				gi.sound(self, CHAN_VOICE, self->noise3, 1, ATTN_NORM, 0);
			}
			else if (level.world_type == 0)
			{
				gi.centerprintf(other, "You need the silver key");
				gi.sound(self, CHAN_VOICE, self->noise3, 1, ATTN_NORM, 0);
			}
		}
		else
		{
			if (level.world_type == 2)
			{
				gi.centerprintf(other, "You need the gold keycard");
				gi.sound(self, CHAN_VOICE, self->noise3, 1, ATTN_NORM, 0);
			}
			else if (level.world_type == 1)
			{
				gi.centerprintf(other, "You need the gold runekey");
				gi.sound(self, CHAN_VOICE, self->noise3, 1, ATTN_NORM, 0);
			}
			else if (level.world_type == 0)
			{
				gi.centerprintf(other, "You need the gold key");
				gi.sound(self, CHAN_VOICE, self->noise3, 1, ATTN_NORM, 0);
			}
		}
		return;
	}

	other->client->pers.inventory[ITEM_INDEX(self->item)]--;
	self->touch = NULL;
	if (self->enemy)
		self->enemy->touch = NULL;	// get paired door
	q1_door_use(self, other, other);
}

/*
=============================================================================

SPAWNING FUNCTIONS

=============================================================================
*/


edict_t *q1_spawn_field(edict_t *self, vec3_t fmins, vec3_t fmaxs)
{
	edict_t *trigger = G_Spawn();
	trigger->movetype = MOVETYPE_NONE;
	trigger->solid = SOLID_TRIGGER;
	trigger->owner = self;
	trigger->touch = q1_door_trigger_touch;

	vec3_t size = { 60, 60, 8 };

	VectorSubtract(fmins, size, trigger->mins);
	VectorAdd(fmaxs, size, trigger->maxs);
	gi.linkentity(trigger);

	return trigger;
}

bool EntitiesTouching(edict_t *e1, edict_t *e2)
{
	if (e1->mins[0] > e2->maxs[0])
		return false;
	if (e1->mins[1] > e2->maxs[1])
		return false;
	if (e1->mins[2] > e2->maxs[2])
		return false;
	if (e1->maxs[0] < e2->mins[0])
		return false;
	if (e1->maxs[1] < e2->mins[1])
		return false;
	if (e1->maxs[2] < e2->mins[2])
		return false;
	return true;
}


/*
=============
LinkDoors


=============
*/
void LinkDoors(edict_t *self)
{
	edict_t *t, *starte;
	vec3_t cmins, cmaxs;

	if (self->enemy)
		return;		// already linked by another door
	if (self->spawnflags & 4)
	{
		self->owner = self->enemy = self;
		return;		// don't want to link this door
	}

	VectorCopy(self->mins, cmins);
	VectorCopy(self->maxs, cmaxs);

	starte = self;
	t = self;

	do
	{
		self->owner = starte;			// master door

		if (self->health)
			starte->health = self->health;
		if (self->targetname)
			starte->targetname = self->targetname;
		if (self->message && self->message[0])
			starte->message = self->message;

		t = G_FindByType(t, self->entitytype);
		if (!t)
		{
			self->enemy = starte;		// make the chain a loop

										// shootable, fired, or key doors just needed the owner/enemy links,
										// they don't spawn a field

			self = self->owner;

			if (self->health)
				return;
			if (self->targetname)
				return;
			if (self->item)
				return;

			self->owner->trigger_field = q1_spawn_field(self, cmins, cmaxs);
			return;
		}

		if (EntitiesTouching(self, t))
		{
			if (t->enemy)
				gi.error("cross connected doors");

			self->enemy = t;
			self = t;

			if (t->mins[0] < cmins[0])
				cmins[0] = t->mins[0];
			if (t->mins[1] < cmins[1])
				cmins[1] = t->mins[1];
			if (t->mins[2] < cmins[2])
				cmins[2] = t->mins[2];
			if (t->maxs[0] > cmaxs[0])
				cmaxs[0] = t->maxs[0];
			if (t->maxs[1] > cmaxs[1])
				cmaxs[1] = t->maxs[1];
			if (t->maxs[2] > cmaxs[2])
				cmaxs[2] = t->maxs[2];
		}
	} while (1);

}


/*QUAKED func_door (0 .5 .8) ? START_OPEN x DOOR_DONT_LINK GOLD_KEY SILVER_KEY TOGGLE
if two doors touch, they are assumed to be connected and operate as a unit.

TOGGLE causes the door to wait in both the start and end states for a trigger event.

START_OPEN causes the door to move to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not usefull for touch or takedamage doors).

Key doors are allways wait -1.

"message"	is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"	if set, door must be shot open
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"lip"		lip remaining at end of move (8 default)
"dmg"		damage to inflict when blocked (2 default)
"sounds"
0)	no sound
1)	stone
2)	base
3)	stone chain
4)	screechy metal
*/

void func_door(edict_t *self)
{
	if (level.world_type == 0)
	{
		self->noise3 = gi.soundindex("q1/doors/medtry.wav");
		self->noise4 = gi.soundindex("q1/doors/meduse.wav");
	}
	else if (level.world_type == 1)
	{
		self->noise3 = gi.soundindex("q1/doors/runetry.wav");
		self->noise4 = gi.soundindex("q1/doors/runeuse.wav");
	}
	else if (level.world_type == 2)
	{
		self->noise3 = gi.soundindex("q1/doors/basetry.wav");
		self->noise4 = gi.soundindex("q1/doors/baseuse.wav");
	}
	else
	{
		gi.dprintf("no worldtype set!\n");
	}

	if (self->sounds == 0)
	{
	}
	if (self->sounds == 1)
	{
		self->noise1 = gi.soundindex("q1/doors/drclos4.wav");
		self->noise2 = gi.soundindex("q1/doors/doormv1.wav");
	}
	if (self->sounds == 2)
	{
		self->noise2 = gi.soundindex("q1/doors/hydro1.wav");
		self->noise1 = gi.soundindex("q1/doors/hydro2.wav");
	}
	if (self->sounds == 3)
	{
		self->noise2 = gi.soundindex("q1/doors/stndr1.wav");
		self->noise1 = gi.soundindex("q1/doors/stndr2.wav");
	}
	if (self->sounds == 4)
	{
		self->noise1 = gi.soundindex("q1/doors/ddoor2.wav");
		self->noise2 = gi.soundindex("q1/doors/ddoor1.wav");
	}

	G_SetMovedir(self->s.angles, self->movedir);

	self->max_health = self->health;
	self->solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);

	self->blocked = q1_door_blocked;
	self->use = q1_door_use;

	if (self->spawnflags & DOOR_SILVER_KEY)
		self->item = GetItemByIndex(ITI_Q1_SILVER_KEY);
	if (self->spawnflags & DOOR_GOLD_KEY)
		self->item = GetItemByIndex(ITI_Q1_GOLD_KEY);

	if (!self->speed)
		self->speed = 100;
	if (!self->wait)
		self->wait = 3;
	if (!spawnTemp.lip)
		spawnTemp.lip = 8;
	if (!self->dmg)
		self->dmg = 2;

	VectorCopy(self->s.origin, self->pos1);
	float dist = fabsf(DotProduct(self->movedir, self->size)) - spawnTemp.lip;
	VectorMA(self->pos1, dist, self->movedir, self->pos2);
	//self.pos2 = self.pos1 + self.movedir*(fabs(self.movedir*self.size) - self.lip);

	// DOOR_START_OPEN is to allow an entity to be lighted in the closed position
	// but spawn in the open position
	if (self->spawnflags & DOOR_START_OPEN)
	{
		VectorCopy(self->pos2, self->s.origin);
		VectorCopy(self->pos1, self->pos2);
		VectorCopy(self->s.origin, self->pos1);
	}

	self->moveinfo.state = STATE_BOTTOM;

	if (self->health)
	{
		self->takedamage = DAMAGE_YES;
		self->die = q1_door_killed;
	}

	if (self->item)
		self->wait = -1;

	self->touch = q1_door_touch;

	self->moveinfo.wait = self->wait;
	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->moveinfo.speed;
	self->moveinfo.decel = self->moveinfo.speed;

	// LinkDoors can't be done until all of the doors have been spawned, so
	// the sizes can be detected properly.
	self->think = LinkDoors;
	self->nextthink = level.time + 1;
	gi.linkentity(self);
}

void q1_plat_center_touch(edict_t *self, edict_t *other, cplane_t *, csurface_t *);
void q1_plat_go_down(edict_t *self);
void q1_plat_outside_touch(edict_t *self, edict_t *other, cplane_t *, csurface_t *);

#define PLAT_LOW_TRIGGER 1

void q1_plat_spawn_inside_trigger(edict_t *self)
{
	edict_t *trigger;
	vec3_t tmin, tmax;

	//
	// middle trigger
	//	
	trigger = G_Spawn();
	trigger->touch = q1_plat_center_touch;
	trigger->movetype = MOVETYPE_NONE;
	trigger->solid = SOLID_TRIGGER;
	trigger->enemy = self;

	VectorSet(tmin, 25, 25, 0);
	VectorSet(tmax, 25, 25, -8);

	VectorAdd(self->mins, tmin, tmin);
	VectorSubtract(self->maxs, tmax, tmax);

	tmin[2] = tmax[2] - (self->pos1[2] - self->pos2[2] + 8);

	if (self->spawnflags & PLAT_LOW_TRIGGER)
		tmax[2] = tmin[2] + 8;

	if (self->size[0] <= 50)
	{
		tmin[0] = (self->mins[0] + self->maxs[0]) / 2;
		tmax[0] = tmin[0] + 1;
	}
	if (self->size[1] <= 50)
	{
		tmin[1] = (self->mins[1] + self->maxs[1]) / 2;
		tmax[1] = tmin[1] + 1;
	}

	VectorCopy(tmin, trigger->mins);
	VectorCopy(tmax, trigger->maxs);
	gi.linkentity(trigger);
}

void q1_plat_hit_top(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, self->noise1, 1, ATTN_NORM, 0);
	self->moveinfo.state = STATE_TOP;
	self->think = q1_plat_go_down;
	self->nextthink = level.time + 3000;
}

void q1_plat_hit_bottom(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, self->noise1, 1, ATTN_NORM, 0);
	self->moveinfo.state = STATE_BOTTOM;
}

void q1_plat_go_down(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, self->noise, 1, ATTN_NORM, 0);
	self->moveinfo.state = STATE_DOWN;
	Move_Calc(self, self->pos2, q1_plat_hit_bottom);
}

void q1_plat_go_up(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, self->noise, 1, ATTN_NORM, 0);
	self->moveinfo.state = STATE_UP;
	Move_Calc(self, self->pos1, q1_plat_hit_top);
}

void q1_plat_center_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;

	if (other->health <= 0)
		return;

	self = self->enemy;
	if (self->moveinfo.state == STATE_BOTTOM)
		q1_plat_go_up(self);
	else if (self->moveinfo.state == STATE_TOP)
		self->nextthink = level.time + 1000;	// delay going down
}

void q1_plat_outside_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;

	if (other->health <= 0)
		return;

	//dprint ("plat_outside_touch\n");
	self = self->enemy;
	if (self->moveinfo.state == STATE_TOP)
		q1_plat_go_down(self);
}

void q1_plat_trigger_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->think)
		return;		// allready activated
	q1_plat_go_down(self);
}

void q1_plat_crush(edict_t *self, edict_t *other)
{
	//dprint ("plat_crush\n");

	T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, 1, 0, DAMAGE_NONE, MakeBlankMeansOfDeath(self));

	if (self->moveinfo.state == STATE_UP)
		q1_plat_go_down(self);
	else if (self->moveinfo.state == STATE_DOWN)
		q1_plat_go_up(self);
	else
		gi.error("plat_crush: bad self.state\n");
}

void q1_plat_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->use = NULL;
	if (self->moveinfo.state != STATE_UP)
		gi.error("plat_use: not in up state");
	q1_plat_go_down(self);
}


/*QUAKED func_plat (0 .5 .8) ? PLAT_LOW_TRIGGER
speed	default 150

Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in the extended position until it is trigger, when it will lower and become a normal plat.

If the "height" key is set, that will determine the amount the plat moves, instead of being implicitly determined by the model's height.
Set "sounds" to one of the following:
1) base fast
2) chain slow
*/


void func_plat(edict_t *self)
{
	//	edict_t *t;

	if (!self->t_length)
		self->t_length = 80;
	if (!self->t_width)
		self->t_width = 10;

	if (self->sounds == 0)
		self->sounds = 2;
	// FIX THIS TO LOAD A GENERIC PLAT SOUND

	if (self->sounds == 1)
	{
		self->noise = gi.soundindex("q1/plats/plat1.wav");
		self->noise1 = gi.soundindex("q1/plats/plat2.wav");
	}

	if (self->sounds == 2)
	{
		self->noise = gi.soundindex("q1/plats/medplat1.wav");
		self->noise1 = gi.soundindex("q1/plats/medplat2.wav");
	}

	VectorCopy(self->s.angles, self->move_angles);
	VectorClear(self->s.angles);

	self->solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);

	self->blocked = q1_plat_crush;
	if (!self->speed)
		self->speed = 150;

	// pos1 is the top position, pos2 is the bottom
	VectorCopy(self->s.origin, self->pos1);
	VectorCopy(self->s.origin, self->pos2);

	if (spawnTemp.height)
		self->pos2[2] = self->s.origin[2] - spawnTemp.height;
	else
		self->pos2[2] = self->s.origin[2] - self->size[2] + 8;

	self->use = q1_plat_trigger_use;

	q1_plat_spawn_inside_trigger(self);	// the "start moving" trigger	

	if (self->targetname)
	{
		self->moveinfo.state = STATE_UP;
		self->use = q1_plat_use;
	}
	else
	{
		VectorCopy(self->pos2, self->s.origin);
		self->moveinfo.state = STATE_BOTTOM;
	}

	self->moveinfo.wait = self->wait;
	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->moveinfo.speed;
	self->moveinfo.decel = self->moveinfo.speed;
	gi.linkentity(self);
}

void q1_button_done(edict_t *self)
{
	self->moveinfo.state = STATE_BOTTOM;
}

void q1_button_return(edict_t *self)
{
	self->moveinfo.state = STATE_DOWN;
	Move_Calc(self, self->pos1, q1_button_done);
	self->s.effects &= ~EF_ANIM23;
	self->s.effects |= EF_ANIM01;
	if (self->health)
		self->takedamage = DAMAGE_YES;	// can be shot again
}

void q1_button_wait(edict_t *self)
{
	self->moveinfo.state = STATE_TOP;
	if (self->moveinfo.wait >= 0)
		self->nextthink = level.time + self->moveinfo.wait * 1000;
	self->think = q1_button_return;
	G_UseTargets(self, self->enemy);
	self->s.effects &= ~EF_ANIM01;
	self->s.frame = 3;
}

void q1_button_blocked(edict_t *self, edict_t *other)
{	// do nothing, just don't ome all the way back out
}

void q1_button_fire(edict_t *self, edict_t *other)
{
	if (self->moveinfo.state == STATE_UP || self->moveinfo.state == STATE_TOP)
		return;

	gi.sound(self, CHAN_VOICE, self->noise, 1, ATTN_NORM, 0);

	self->moveinfo.state = STATE_UP;
	Move_Calc(self, self->pos2, q1_button_wait);
}


void q1_button_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->enemy = activator;
	q1_button_fire(self, activator);
}

void q1_button_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;
	self->enemy = other;
	q1_button_fire(self, other);
}

void q1_button_killed(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->enemy = attacker;
	self->health = self->max_health;
	self->takedamage = DAMAGE_NO;	// wil be reset upon return
	q1_button_fire(self, attacker);
}


/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"sounds"
0) steam metal
1) wooden clunk
2) metallic click
3) in-out
*/
void func_button(edict_t *self)
{
	//	float gtemp, ftemp;

	if (self->sounds == 0)
	{
		self->noise = gi.soundindex("q1/buttons/airbut1.wav");
	}
	if (self->noise == 1)
	{
		self->noise = gi.soundindex("q1/buttons/switch21.wav");
	}
	if (self->noise == 2)
	{
		self->noise = gi.soundindex("q1/buttons/switch02.wav");
	}
	if (self->noise == 3)
	{
		self->noise = gi.soundindex("q1/buttons/switch04.wav");
	}

	G_SetMovedir(self->s.angles, self->movedir);

	self->movetype = MOVETYPE_PUSH;
	self->solid = SOLID_BSP;
	gi.setmodel(self, self->model);

	self->blocked = q1_button_blocked;
	self->use = q1_button_use;
	self->s.effects |= EF_ANIM01;

	if (self->health)
	{
		self->max_health = self->health;
		self->die = q1_button_killed;
		self->takedamage = DAMAGE_YES;
	}
	else
		self->touch = q1_button_touch;

	if (!self->speed)
		self->speed = 40;
	if (!self->wait)
		self->wait = 1;
	if (!spawnTemp.lip)
		spawnTemp.lip = 4;

	self->moveinfo.state = STATE_BOTTOM;

	VectorCopy(self->s.origin, self->pos1);
	float dist = fabsf(DotProduct(self->movedir, self->size)) - spawnTemp.lip;
	VectorMA(self->pos1, dist, self->movedir, self->pos2);

	self->moveinfo.wait = self->wait;
	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->moveinfo.speed;
	self->moveinfo.decel = self->moveinfo.speed;
	gi.linkentity(self);
}

void SP_q1_item_key1(edict_t *self)
{
	SpawnItem(self, GetItemByIndex(ITI_Q1_SILVER_KEY));
}

void SP_q1_item_key2(edict_t *self)
{
	SpawnItem(self, GetItemByIndex(ITI_Q1_GOLD_KEY));
}



/*
==============================================================================

SIMPLE BMODELS

==============================================================================
*/

void q1_func_wall_use(edict_t *self, edict_t *other, edict_t *activator)
{	// change to alternate textures
	self->s.frame = 1 - self->s.frame;

	if (self->s.effects & EF_ANIM23)
	{
		self->s.effects &= ~EF_ANIM23;
		self->s.effects |= EF_ANIM01;
	}
	else
	{
		self->s.effects &= ~EF_ANIM01;
		self->s.effects |= EF_ANIM23;
	}
}

/*QUAKED func_wall (0 .5 .8) ?
This is just a solid wall if not inhibitted
*/
void func_wall(edict_t *self)
{
	VectorClear(self->s.angles);
	self->movetype = MOVETYPE_PUSH;	// so it doesn't get pushed by anything
	self->solid = SOLID_BSP;
	self->use = q1_func_wall_use;
	gi.setmodel(self, self->model);
	self->s.effects |= EF_ANIM01;
	gi.linkentity(self);
}


/*QUAKED func_illusionary (0 .5 .8) ?
A simple entity that looks solid but lets you walk through it.
*/
void func_illusionary(edict_t *self)
{
	VectorClear(self->s.angles);
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	gi.setmodel(self, self->model);
	self->s.effects |= EF_ANIM01;
	gi.linkentity(self);
}

/*QUAKED func_episodegate (0 .5 .8) ? E1 E2 E3 E4
This bmodel will appear if the episode has allready been completed, so players can't reenter it.
*/
void func_episodegate(edict_t *self)
{
	if (!(game.serverflags & self->spawnflags))
		return;			// can still enter episode

	VectorClear(self->s.angles);
	self->movetype = MOVETYPE_PUSH;	// so it doesn't get pushed by anything
	self->solid = SOLID_BSP;
	self->use = q1_func_wall_use;
	gi.setmodel(self, self->model);
	self->s.effects |= EF_ANIM01;
	gi.linkentity(self);
}

/*QUAKED func_bossgate (0 .5 .8) ?
This bmodel appears unless players have all of the episode sigils.
*/
void func_bossgate(edict_t *self)
{
	if ((game.serverflags & 15) == 15)
		return;		// all episodes completed
	VectorClear(self->s.angles);
	self->movetype = MOVETYPE_PUSH;	// so it doesn't get pushed by anything
	self->solid = SOLID_BSP;
	self->use = q1_func_wall_use;
	gi.setmodel(self, self->model);
	self->s.effects |= EF_ANIM01;

	gi.linkentity(self);
}



void trigger_reactivate(edict_t *self)
{
	self->solid = SOLID_TRIGGER;
}

//=============================================================================

#define	SPAWNFLAG_NOMESSAGE 1
#define	SPAWNFLAG_NOTOUCH 1

// the wait time has passed, so set back up for another activation
void q1_multi_wait(edict_t *self)
{
	if (self->max_health)
	{
		self->health = self->max_health;
		self->takedamage = DAMAGE_YES;
		self->solid = SOLID_BBOX;
	}
}


// the trigger was just touched/killed/used
// self.enemy should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void q1_multi_trigger(edict_t *self)
{
	if (self->nextthink > level.time)
	{
		return;		// allready been triggered
	}

	if (self->entitytype == ET_Q1_TRIGGER_SECRET)
	{
		if (!self->enemy->client)
			return;
		level.found_secrets = level.found_secrets + 1;
		gi.centerprintf(self->enemy, "%s", self->message);
		//WriteByte (MSG_ALL, SVC_FOUNDSECRET);
	}

	if (self->noise)
		gi.sound(self, CHAN_VOICE, self->noise, 1, ATTN_NORM, 0);

	// don't trigger again until reset
	self->takedamage = DAMAGE_NO;

	self->activator = self->enemy;

	G_UseTargets(self, self->activator);

	if (self->wait > 0)
	{
		self->think = q1_multi_wait;
		self->nextthink = level.time + self->wait * 1000;
	}
	else
	{	// we can't just remove (self) here, because this is a touch function
		// called wheil C code is looping through area links...
		self->touch = NULL;
		self->nextthink = level.time + game.frametime;
		self->think = G_FreeEdict;
	}
}

void multi_killed(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->enemy = attacker;
	q1_multi_trigger(self);
}

void multi_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->enemy = activator;
	q1_multi_trigger(self);
}

void multi_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;

	// if the trigger has an angles field, check player's facing direction
	if (!VectorEmpty(self->movedir))
	{
		vec3_t forward;
		AngleVectors(other->s.angles, forward, NULL, NULL);

		if (DotProduct(forward, self->movedir) < 0)
			return;		// not facing the right way
	}

	self->enemy = other;
	q1_multi_trigger(self);
}

/*QUAKED trigger_multiple (.5 .5 .5) ? notouch
Variable sized repeatable trigger.  Must be targeted at one or more entities.  If "health" is set, the trigger must be killed to activate each time.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
If notouch is set, the trigger is only fired by other entities, not by touching.
NOTOUCH has been obsoleted by trigger_relay!
sounds
1)	secret
2)	beep beep
3)	large switch
4)
set "message" to text string
*/
void trigger_multiple(edict_t *self)
{
	if (self->sounds == 1)
	{
		self->noise = gi.soundindex("misc/secret.wav");
	}
	else if (self->sounds == 2)
	{
		self->noise = gi.soundindex("misc/talk.wav");
	}
	else if (self->sounds == 3)
	{
		self->noise = gi.soundindex("q1/misc/trigger1.wav");
	}

	if (!self->wait)
		self->wait = 0.2;
	self->use = multi_use;

	InitTrigger(self);

	if (self->health)
	{
		if (self->spawnflags & SPAWNFLAG_NOTOUCH)
			gi.error("health and notouch don't make sense\n");
		self->max_health = self->health;
		self->die = multi_killed;
		self->takedamage = DAMAGE_YES;
		self->solid = SOLID_BBOX;
	}
	else
	{
		if (!(self->spawnflags & SPAWNFLAG_NOTOUCH))
		{
			self->touch = multi_touch;
		}
	}
	gi.linkentity(self);	// make sure it links into the world
}


/*QUAKED trigger_once (.5 .5 .5) ? notouch
Variable sized trigger. Triggers once, then removes itself.  You must set the key "target" to the name of another object in the level that has a matching
"targetname".  If "health" is set, the trigger must be killed to activate.
If notouch is set, the trigger is only fired by other entities, not by touching.
if "killtarget" is set, any objects that have a matching "target" will be removed when the trigger is fired.
if "angle" is set, the trigger will only fire when someone is facing the direction of the angle.  Use "360" for an angle of 0.
sounds
1)	secret
2)	beep beep
3)	large switch
4)
set "message" to text string
*/
void trigger_once(edict_t *self)
{
	self->wait = -1;
	trigger_multiple(self);
}

//=============================================================================

void q1_trigger_relay_use(edict_t *self, edict_t *other, edict_t *activator)
{
	G_UseTargets(self, activator);
}

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
This fixed size trigger cannot be touched, it can only be fired by other events.  It can contain killtargets, targets, delays, and messages.
*/
void trigger_relay(edict_t *self)
{
	self->use = q1_trigger_relay_use;
	gi.linkentity(self);	// make sure it links into the world
}


//=============================================================================

/*QUAKED trigger_secret (.5 .5 .5) ?
secret counter trigger
sounds
1)	secret
2)	beep beep
3)
4)
set "message" to text string
*/
void trigger_secret(edict_t *self)
{
	level.total_secrets = level.total_secrets + 1;
	self->wait = -1;
	if (!self->message)
		self->message = "You found a secret area!";
	if (!self->sounds)
		self->sounds = 1;

	if (self->sounds == 1)
	{
		self->noise = gi.soundindex("misc/secret.wav");
	}
	else if (self->sounds == 2)
	{
		self->noise = gi.soundindex("misc/talk.wav");
	}

	trigger_multiple(self);
}

//=============================================================================


void counter_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->count = self->count - 1;
	if (self->count < 0)
		return;

	if (self->count != 0)
	{
		if (activator->client
			&& (self->spawnflags & SPAWNFLAG_NOMESSAGE) == 0)
		{
			if (self->count >= 4)
				gi.centerprintf(activator, "There are more to go...");
			else if (self->count == 3)
				gi.centerprintf(activator, "Only 3 more to go...");
			else if (self->count == 2)
				gi.centerprintf(activator, "Only 2 more to go...");
			else
				gi.centerprintf(activator, "Only 1 more to go...");
		}
		return;
	}

	if (activator->client
		&& (self->spawnflags & SPAWNFLAG_NOMESSAGE) == 0)
		gi.centerprintf(activator, "Sequence completed!");
	self->enemy = activator;
	q1_multi_trigger(self);
}

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.

If nomessage is not set, t will print "1 more.. " etc when triggered and "sequence complete" when finished.

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.
*/
void trigger_counter(edict_t *self)
{
	self->wait = -1;
	if (!self->count)
		self->count = 2;

	self->use = counter_use;
	gi.linkentity(self);	// make sure it links into the world
}


/*
==============================================================================

trigger_setskill

==============================================================================
*/

void trigger_skill_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;

	gi.cvar_forceset("skill", self->message);
}

/*QUAKED trigger_setskill (.5 .5 .5) ?
sets skill level to the value of "message".
Only used on start map.
*/
void trigger_setskill(edict_t *self)
{
	InitTrigger(self);
	self->touch = trigger_skill_touch;
	gi.linkentity(self);	// make sure it links into the world
}


/*
==============================================================================

ONLY REGISTERED TRIGGERS

==============================================================================
*/

void trigger_onlyregistered_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;
	if (self->attack_finished_time > level.time)
		return;

	self->attack_finished_time = level.time + 2000;
	//if (cvar("registered"))
	//{
	self->message = "";
	G_UseTargets(self, other);
	G_FreeEdict(self);
	/*}
	else
	{
	if (self.message != "")
	{
	centerprint (other, self.message);
	sound (other, CHAN_BODY, "misc/talk.wav", 1, ATTN_NORM);
	}
	}*/
}

/*QUAKED trigger_onlyregistered (.5 .5 .5) ?
Only fires if playing the registered version, otherwise prints the message
*/
void trigger_onlyregistered(edict_t *self)
{
	InitTrigger(self);
	self->touch = trigger_onlyregistered_touch;
	gi.linkentity(self);	// make sure it links into the world
}

//============================================================================

void hurt_on(edict_t *self)
{
	self->solid = SOLID_TRIGGER;
	self->nextthink = 0;
}

void q1_hurt_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->takedamage)
	{
		self->solid = SOLID_NOT;
		T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, self->dmg, 0, DAMAGE_NONE, MakeBlankMeansOfDeath(self));
		self->think = hurt_on;
		self->nextthink = level.time + 1000;
	}

	return;
}

/*QUAKED trigger_hurt (.5 .5 .5) ?
Any object touching this will be hurt
set dmg to damage amount
defalt dmg = 5
*/
void trigger_hurt(edict_t *self)
{
	InitTrigger(self);
	self->touch = q1_hurt_touch;
	if (!self->dmg)
		self->dmg = 5;
	gi.linkentity(self);	// make sure it links into the world
}

//============================================================================

#define PUSH_ONCE 1

void q1_trigger_push_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->entitytype == ET_GRENADE || other->entitytype == ET_HAND_GRENADE)
		VectorScale(self->movedir, self->speed * 10, other->velocity);
	else if (other->health > 0)
	{
		VectorScale(self->movedir, self->speed * 10, other->velocity);

		if (other->client)
		{
			if (other->fly_sound_debounce_time < level.time)
			{
				other->fly_sound_debounce_time = level.time + 1500;
				gi.sound(other, CHAN_AUTO, gi.soundindex("q1/ambience/windfly.wav"), 1, ATTN_NORM, 0);
			}
		}
	}
	if (self->spawnflags & PUSH_ONCE)
		G_FreeEdict(self);
}


/*QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE
Pushes the player
*/
void trigger_push(edict_t *self)
{
	InitTrigger(self);
	self->touch = q1_trigger_push_touch;
	if (!self->speed)
		self->speed = 1000;
}

//============================================================================

void q1_trigger_monsterjump_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!(other->svflags & SVF_MONSTER))
		return;

	// set XY even if not on ground, so the jump will clear lips
	other->velocity[0] = self->movedir[0] * self->speed;
	other->velocity[1] = self->movedir[1] * self->speed;

	if (!other->groundentity)
		return;

	other->groundentity = NULL;
	other->velocity[2] = self->t_length;
}

/*QUAKED trigger_monsterjump (.5 .5 .5) ?
Walking monsters that touch this will jump in the direction of the trigger's angle
"speed" default to 200, the speed thrown forward
"height" default to 200, the speed thrown upwards
*/
void trigger_monsterjump(edict_t *self)
{
	if (!self->speed)
		self->speed = 200;
	if (!spawnTemp.height)
		spawnTemp.height = 200;
	if (VectorEmpty(self->s.angles))
		VectorSet(self->s.angles, 0, 360, 0);
	InitTrigger(self);
	self->touch = q1_trigger_monsterjump_touch;
	self->t_length = spawnTemp.height;
	gi.linkentity(self);
}




void q1_barrel_explode(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->takedamage = DAMAGE_NO;
	// did say self.owner
	T_RadiusDamage(self, self, 160, NULL, DAMAGE_Q1, 160, MakeBlankMeansOfDeath(self));

	gi.sound(self, CHAN_VOICE, gi.soundindex("q1/weapons/r_exp3.wav"), 1, ATTN_NORM, 0);

	self->s.origin[2] = self->s.origin[2] + 32;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_Q1_EXPLODE);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	G_FreeEdict(self);
}

void G_droptofloor(edict_t *ent)
{
	vec3_t      end;
	trace_t     trace;

	ent->s.origin[2] += 1;
	VectorCopy(ent->s.origin, end);
	end[2] -= 256;

	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1 || trace.allsolid)
		return;

	VectorCopy(trace.endpos, ent->s.origin);

	gi.linkentity(ent);
}

/*QUAKED misc_explobox (0 .5 .8) (0 0 0) (32 32 64)
TESTING THING
*/

void misc_explobox(edict_t *self)
{
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_NONE;
	self->s.modelindex = gi.modelindex("models/q1/b_explob.q1m");
	VectorSet(self->mins, -16, -16, -32);
	VectorSet(self->maxs, 16, 16, 32);
	self->health = 20;
	self->die = q1_barrel_explode;
	self->takedamage = DAMAGE_AIM;

	self->s.origin[0] += 16;
	self->s.origin[1] += 16;

	self->s.origin[2] = self->s.origin[2] + 64;
	G_droptofloor(self);
}

/*QUAKED misc_explobox2 (0 .5 .8) (0 0 0) (32 32 64)
Smaller exploding box, REGISTERED ONLY
*/

void misc_explobox2(edict_t *self)
{
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_NONE;
	self->s.modelindex = gi.modelindex("models/q1/b_exbox2.q1m");
	VectorSet(self->mins, -16, -16, -16);
	VectorSet(self->maxs, 16, 16, 16);
	self->health = 20;
	self->die = q1_barrel_explode;
	self->takedamage = DAMAGE_AIM;

	self->s.origin[0] += 16;
	self->s.origin[1] += 16;

	self->s.origin[2] = self->s.origin[2] + 32;
	G_droptofloor(self);
}



//============================================================================

#define SPAWNFLAG_SUPERSPIKE 1
#define SPAWNFLAG_LASER 2

edict_t *fire_spike(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, bool super);
void fire_q1_laser(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed);

void spikeshooter_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & SPAWNFLAG_LASER)
	{
		gi.positioned_sound(self->s.origin, self, CHAN_VOICE, gi.soundindex("q1/enforcer/enfire.wav"), 1, ATTN_NORM, 0);
		fire_q1_laser(self, self->s.origin, self->movedir, 15, 500);
	}
	else
	{
		gi.positioned_sound(self->s.origin, self, CHAN_VOICE, gi.soundindex("q1/weapons/spike2.wav"), 1, ATTN_NORM, 0);
		fire_spike(self, self->s.origin, self->movedir, self->spawnflags & SPAWNFLAG_SUPERSPIKE ? 18 : 9, 500, self->spawnflags & SPAWNFLAG_SUPERSPIKE);
	}
}

void shooter_think(edict_t *self)
{
	spikeshooter_use(self, self, self);
	self->nextthink = level.time + (self->wait * 1000);
}


/*QUAKED trap_spikeshooter (0 .5 .8) (-8 -8 -8) (8 8 8) superspike laser
When triggered, fires a spike in the direction set in QuakeEd.
Laser is only for REGISTERED.
*/

void trap_spikeshooter(edict_t *self)
{
	G_SetMovedir(self->s.angles, self->movedir);
	self->use = spikeshooter_use;

	if (self->spawnflags & SPAWNFLAG_LASER)
	{
		gi.modelindex("models/q1/laser.mdl");

		gi.soundindex("q1/enforcer/enfire.wav");
		gi.soundindex("q1/enforcer/enfstop.wav");
	}
	else
		gi.soundindex("q1/weapons/spike2.wav");
}


/*QUAKED trap_shooter (0 .5 .8) (-8 -8 -8) (8 8 8) superspike laser
Continuously fires spikes.
"wait" time between spike (1.0 default)
"nextthink" delay before firing first spike, so multiple shooters can be stagered.
*/
void trap_shooter(edict_t *self)
{
	trap_spikeshooter(self);

	if (self->wait == 0)
		self->wait = 1;
	self->nextthink = ((spawnTemp.nextthink + self->wait) * 1000) + level.time;
	self->think = shooter_think;
}

void path_corner_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);

void movetarget_f(edict_t *self)
{
	if (!self->targetname)
		gi.error("monster_movetarget: no targetname");

	self->solid = SOLID_TRIGGER;
	self->touch = path_corner_touch;
	self->svflags |= SVF_NOCLIENT;
	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);
	gi.linkentity(self);
}

/*QUAKED path_corner (0.5 0.3 0) (-8 -8 -8) (8 8 8)
Monsters will continue walking towards the next target corner.
*/
void path_corner(edict_t *self)
{
	movetarget_f(self);
}

void changelevel_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	//	edict_t *pos;

	if (level.intermissiontime)
		return;     // already activated

	if (!deathmatch->value && !coop->value) {
		if (g_edicts[1].health <= 0)
			return;
	}

	if (!other->client)
		return;

	if (deathmatch->value && !((int)dmflags->value & DF_ALLOW_EXIT) && other != world)
	{
		T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, 50000, 0, DAMAGE_NONE, MakeBlankMeansOfDeath(self));
		return;
	}

	if (deathmatch->value || coop->value) {
		if (other && other->client)
			gi.bprintf(PRINT_HIGH, "%s exited the level.\n", other->client->pers.netname);
	}

	G_UseTargets(self, other);

	self->touch = NULL;

	// we can't move people right now, because touch functions are called
	// in the middle of C movement code, so set a think time to do it
	self->think = BeginIntermission;
	self->nextthink = level.time + 1;
}

/*QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
When the player touches this, he gets sent to the map listed in the "map" variable.  Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
*/
void trigger_changelevel(edict_t *self)
{
	if (!self->map || !self->map[0])
		gi.error("changelevel trigger doesn't have map");

	InitTrigger(self);
	self->touch = changelevel_touch;
	gi.linkentity(self);
}



//============================================================================

void q1_train_next(edict_t *self);
void q1_func_train_find(edict_t *self);

void q1_train_blocked(edict_t *self, edict_t *other)
{
	if (level.time < self->attack_finished_time)
		return;
	self->attack_finished_time = level.time + 500;
	T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, self->dmg, 0, DAMAGE_NONE, MakeBlankMeansOfDeath(self));
}

void q1_train_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->think != q1_func_train_find)
		return;		// already activated

	q1_train_next(self);
}

void q1_train_wait(edict_t *self)
{
	if (self->wait)
	{
		if (self->wait > 0)
			self->nextthink = level.time + self->wait * 1000;
		gi.sound(self, CHAN_VOICE, self->noise, 1, ATTN_NORM, 0);
	}
	else
		self->nextthink = level.time + 1;

	self->think = q1_train_next;
}

void q1_train_next(edict_t *self)
{
	edict_t *targ;

	targ = G_Find(NULL, FOFS(targetname), self->target);
	self->target = targ->target;
	if (!self->target)
		gi.error("train_next: no next target");
	if (targ->wait)
		self->wait = targ->wait;
	else
		self->wait = 0;
	gi.sound(self, CHAN_VOICE, self->noise1, 1, ATTN_NORM, 0);
	vec3_t p;
	VectorSubtract(targ->s.origin, self->mins, p);
	p[0] -= 1;
	p[1] -= 1;
	p[2] -= 1;
	Move_Calc(self, p, q1_train_wait);
}

void q1_func_train_find(edict_t *self)
{
	edict_t *targ;

	targ = G_Find(NULL, FOFS(targetname), self->target);
	self->target = targ->target;
	VectorSubtract(targ->s.origin, self->mins, self->s.origin);
	self->s.origin[0] -= 1;
	self->s.origin[1] -= 1;
	self->s.origin[2] -= 1;
	gi.linkentity(self);
	if (!self->targetname)
	{	// not triggered, so start immediately
		self->nextthink = level.time + 1;
		self->think = q1_train_next;
	}
}

/*QUAKED func_train (0 .5 .8) ?
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed	default 100
dmg		default	2
sounds
1) ratchet metal

*/
void func_train(edict_t *self)
{
	if (!self->speed)
		self->speed = 100;
	if (!self->target)
		gi.error("func_train without a target");
	if (!self->dmg)
		self->dmg = 2;

	if (self->sounds == 1)
	{
		self->noise = gi.soundindex("q1/plats/train2.wav");
		self->noise1 = gi.soundindex("q1/plats/train1.wav");
	}

	//self.cnt = 1;
	self->solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	self->blocked = q1_train_blocked;
	self->use = q1_train_use;

	gi.setmodel(self, self->model);

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + 1;
	self->think = q1_func_train_find;

	self->moveinfo.wait = self->wait;
	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->moveinfo.speed;
	self->moveinfo.decel = self->moveinfo.speed;
	gi.linkentity(self);
}

/*QUAKED misc_teleporttrain (0 .5 .8) (-8 -8 -8) (8 8 8)
This is used for the final bos
*/
void misc_teleporttrain(edict_t *self)
{
	if (!self->speed)
		self->speed = 100;
	if (!self->target)
		gi.error("func_train without a target");

	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_PUSH;
	self->blocked = q1_train_blocked;
	self->use = q1_train_use;
	VectorSet(self->avelocity, 100, 200, 300);

	self->s.modelindex = gi.modelindex("models/q1/teleport.mdl");
	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + 100;
	self->think = q1_func_train_find;

	self->moveinfo.wait = self->wait;
	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->moveinfo.speed;
	self->moveinfo.decel = self->moveinfo.speed;
	gi.linkentity(self);
}



//===========================================================================

static edict_t *le1, *le2;
static gtime_t lightning_end;

void lightning_fire(edict_t *self)
{
	vec3_t p1, p2;

	if (level.time >= lightning_end)
	{	// done here, put the terminals back up
		self = le1;
		q1_door_go_down(self);
		self = le2;
		q1_door_go_down(self);
		return;
	}

	VectorAdd(le1->mins, le1->maxs, p1);
	VectorScale(p1, 0.5f, p1);
	p1[2] = le1->absmin[2] - 16;

	VectorAdd(le2->mins, le2->maxs, p2);
	VectorScale(p2, 0.5f, p2);
	p2[2] = le2->absmin[2] - 16;

	// compensate for length of bolt
	vec3_t sub;
	VectorSubtract(p2, p1, sub);
	VectorNormalize(sub);
	VectorScale(sub, 100, sub);
	VectorSubtract(p2, sub, p2);

	self->nextthink = level.time + 100;
	self->think = lightning_fire;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_Q1_LIGHTNING3);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(p1);
	gi.WritePosition(p2);
	gi.multicast(p1, MULTICAST_PVS);
}

void boss_sound_pain(edict_t *self);
void boss_shocka(edict_t *self);
void boss_shockb(edict_t *self);
void boss_shockc(edict_t *self);

void lightning_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (lightning_end >= level.time + 1000)
		return;

	le1 = G_Find(NULL, FOFS(target), "lightning");
	le2 = G_Find(le1, FOFS(target), "lightning");
	if (!le1 || !le2)
	{
		gi.dprintf("missing lightning targets\n");
		return;
	}

	if (
		(le1->moveinfo.state != STATE_TOP && le1->moveinfo.state != STATE_BOTTOM)
		|| (le2->moveinfo.state != STATE_TOP && le2->moveinfo.state != STATE_BOTTOM)
		|| (le1->moveinfo.state != le2->moveinfo.state))
	{
		//		dprint ("not aligned\n");
		return;
	}

	// don't let the electrodes go back up until the bolt is done
	le1->nextthink = GTIME_MAX;
	le2->nextthink = GTIME_MAX;
	lightning_end = level.time + 1000;

	gi.sound(self, CHAN_VOICE, gi.soundindex("q1/misc/power.wav"), 1, ATTN_NORM, 0);
	lightning_fire(self);

	// advance the boss pain if down
	self = G_FindByType(NULL, ET_Q1_MONSTER_BOSS);

	if (!self)
		return;

	self->enemy = activator;
	if (le1->moveinfo.state == STATE_TOP && self->health > 0)
	{
		boss_sound_pain(self);

		self->health = self->health - 1;
		if (self->health >= 2)
			boss_shocka(self);
		else if (self->health == 1)
			boss_shockb(self);
		else if (self->health == 0)
			boss_shockc(self);
	}
}

/*QUAKED event_lightning (0 1 1) (-16 -16 -16) (16 16 16)
Just for boss level.
*/
void event_lightning(edict_t *self)
{
	self->use = lightning_use;
	gi.linkentity(self);
}



/*QUAKED misc_fireball (0 .5 .8) (-8 -8 -8) (8 8 8)
Lava Balls
*/

void fire_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self == other)
		return;

	if (other->takedamage)
		T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, 20, 0, DAMAGE_NONE, MakeBlankMeansOfDeath(self));

	G_FreeEdict(self);
}

void fire_fly(edict_t *self)
{
	edict_t *fireball;

	fireball = G_Spawn();
	fireball->solid = SOLID_TRIGGER;
	fireball->movetype = MOVETYPE_TOSS;
	fireball->s.game = GAME_Q1;
	fireball->s.effects |= EF_ROCKET;
	fireball->velocity[0] = (random() * 100) - 50;
	fireball->velocity[1] = (random() * 100) - 50;
	fireball->velocity[2] = self->speed + (random() * 200);
	fireball->s.modelindex = gi.modelindex("models/q1/lavaball.mdl");
	VectorClear(fireball->mins);
	VectorClear(fireball->maxs);
	VectorCopy(self->s.origin, fireball->s.origin);
	fireball->nextthink = level.time + 5000;
	fireball->think = G_FreeEdict;
	fireball->touch = fire_touch;
	gi.linkentity(fireball);

	self->nextthink = level.time + (random() * 5000) + 3000;
	self->think = fire_fly;
}

void misc_fireball(edict_t *self)
{
	gi.modelindex("q1/progs/lavaball.mdl");
	self->nextthink = level.time + (random() * 5000);
	self->think = fire_fly;

	if (!self->speed)
		self->speed = 1000;

	gi.linkentity(self);
}


void sigil_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;
	if (other->health <= 0)
		return;

	gi.centerprintf(other, "You got the rune!");

	gi.sound(other, CHAN_ITEM, self->noise, 1, ATTN_NORM, 0);
	other->client->bonus_alpha = 0.25;

	self->solid = SOLID_NOT;
	self->s.modelindex = 0;
	game.serverflags |= self->spawnflags & 15;
	self->entitytype = ET_NULL;		// so rune doors won't find it

	G_UseTargets(self, other);				// fire all targets / killtargets
}


/*QUAKED item_sigil (0 .5 .8) (-16 -16 -24) (16 16 32) E1 E2 E3 E4
End of level sigil, pick up to end episode and return to jrstart.
*/
void droptofloor(edict_t *self);

void item_sigil(edict_t *self)
{
	if (!self->spawnflags)
		gi.error("no spawnflags");

	self->noise = gi.soundindex("q1/misc/runekey.wav");

	if (self->spawnflags & 1)
		gi.setmodel(self, "models/q1/end1.mdl");

	if (self->spawnflags & 2)
		gi.setmodel(self, "models/q1/end2.mdl");

	if (self->spawnflags & 4)
		gi.setmodel(self, "models/q1/end3.mdl");

	if (self->spawnflags & 8)
		gi.setmodel(self, "models/q1/end4.mdl");

	self->touch = sigil_touch;
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 32);
	self->nextthink = level.time + game.frametime * 2;    // items start after other solids
	self->think = droptofloor;
	self->s.effects |= EF_ROTATE;

	gi.linkentity(self);
}