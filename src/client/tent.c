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
// cl_tent.c -- client side temporary entities

#include "client.h"

soundhandle_t   cl_sfx_ric1;
soundhandle_t   cl_sfx_ric2;
soundhandle_t   cl_sfx_ric3;
soundhandle_t   cl_sfx_lashit;
soundhandle_t   cl_sfx_spark5;
soundhandle_t   cl_sfx_spark6;
soundhandle_t   cl_sfx_spark7;
soundhandle_t   cl_sfx_railg;
soundhandle_t   cl_sfx_rockexp;
soundhandle_t   cl_sfx_grenexp;
soundhandle_t   cl_sfx_watrexp;
soundhandle_t   cl_sfx_footsteps[4];

soundhandle_t   cl_sfx_lightning;
soundhandle_t   cl_sfx_disrexp;

modelhandle_t   cl_mod_explode;
modelhandle_t   cl_mod_smoke;
modelhandle_t   cl_mod_flash;
modelhandle_t   cl_mod_parasite_segment;
modelhandle_t   cl_mod_grapple_cable;
modelhandle_t   cl_mod_explo4;
modelhandle_t   cl_mod_bfg_explo;
modelhandle_t   cl_mod_powerscreen;
modelhandle_t   cl_mod_laser;
modelhandle_t   cl_mod_dmspot;

modelhandle_t   cl_mod_lightning;
modelhandle_t   cl_mod_heatbeam;
modelhandle_t   cl_mod_explo4_big;

// Generations
soundhandle_t   cl_sfx_tink1;
soundhandle_t	cl_sfx_r_exp3;
soundhandle_t	cl_sfx_wizhit;
soundhandle_t	cl_sfx_knighthit;

modelhandle_t	cl_mod_q1_lightning1;
modelhandle_t	cl_mod_q1_lightning2;
modelhandle_t	cl_mod_q1_lightning3;
modelhandle_t	cl_mod_q1_explode;
modelhandle_t	cl_mod_player;

soundhandle_t	cl_sfx_doom_boom;
soundhandle_t	cl_sfx_doom_plas;
soundhandle_t	cl_sfx_doom_imp_xplo;

modelhandle_t	cl_mod_doom_tfog;
modelhandle_t	cl_mod_doom_ifog;
modelhandle_t	cl_mod_doom_puff;
modelhandle_t	cl_mod_doom_misl;
modelhandle_t	cl_mod_doom_plse;
modelhandle_t	cl_mod_doom_bal1;
modelhandle_t	cl_mod_doom_bal2;
modelhandle_t	cl_mod_doom_bal7;
modelhandle_t	cl_mod_doom_apbx;
modelhandle_t	cl_mod_doom_blud;
modelhandle_t	cl_mod_doom_fbxp;

modelhandle_t	cl_mod_duke_shotspark;
modelhandle_t	cl_mod_duke_shotsmoke;
modelhandle_t	cl_mod_duke_shotblood;
modelhandle_t	cl_mod_duke_shotgunshell;
modelhandle_t	cl_mod_duke_shell;
modelhandle_t	cl_mod_duke_explosion;
modelhandle_t	cl_mod_duke_explosion2;
modelhandle_t	cl_mod_duke_teleport;
modelhandle_t	cl_mod_duke_freezeblast;
modelhandle_t	cl_mod_duke_glass;

soundhandle_t	cl_sfx_duke_rpg_boom;
soundhandle_t	cl_sfx_duke_pipe_boom;
soundhandle_t	cl_sfx_duke_ricochet;
soundhandle_t	cl_sfx_duke_glass;

/*
=================
CL_RegisterTEntSounds
=================
*/
void CL_RegisterTEntSounds(void)
{
    int     i;
    char    name[MAX_QPATH];

    cl_sfx_ric1 = S_RegisterSound("world/ric1.wav");
    cl_sfx_ric2 = S_RegisterSound("world/ric2.wav");
    cl_sfx_ric3 = S_RegisterSound("world/ric3.wav");
    cl_sfx_lashit = S_RegisterSound("weapons/lashit.wav");
    cl_sfx_spark5 = S_RegisterSound("world/spark5.wav");
    cl_sfx_spark6 = S_RegisterSound("world/spark6.wav");
    cl_sfx_spark7 = S_RegisterSound("world/spark7.wav");
    cl_sfx_railg = S_RegisterSound("weapons/railgf1a.wav");
    cl_sfx_rockexp = S_RegisterSound("weapons/rocklx1a.wav");
    cl_sfx_grenexp = S_RegisterSound("weapons/grenlx1a.wav");
    cl_sfx_watrexp = S_RegisterSound("weapons/xpld_wat.wav");

    S_RegisterSound("player/land1.wav");
    S_RegisterSound("player/fall2.wav");
    S_RegisterSound("player/fall1.wav");

    for (i = 0; i < 4; i++) {
        Q_snprintf(name, sizeof(name), "player/step%i.wav", i + 1);
        cl_sfx_footsteps[i] = S_RegisterSound(name);
    }

    cl_sfx_lightning = S_RegisterSound("weapons/tesla.wav");
    cl_sfx_disrexp = S_RegisterSound("weapons/disrupthit.wav");

	// Generations
	cl_sfx_tink1 = S_RegisterSound("q1/weapons/tink1.wav");
	cl_sfx_r_exp3 = S_RegisterSound("q1/weapons/r_exp3.wav");
	cl_sfx_wizhit = S_RegisterSound("q1/wizard/hit.wav");
	cl_sfx_knighthit = S_RegisterSound("q1/hknight/hit.wav");

	cl_sfx_doom_boom = S_RegisterSound("doom/BAREXP.wav");
	cl_sfx_doom_plas = S_RegisterSound("doom/FIRXPL.wav");

	cl_sfx_duke_rpg_boom = S_RegisterSound("%s_duke_rpg_explode");
	cl_sfx_duke_pipe_boom = S_RegisterSound("%s_duke_pipe_explode");
	cl_sfx_duke_ricochet = S_RegisterSound("duke/RICOCHET.wav");
	cl_sfx_duke_glass = S_RegisterSound("duke/GLASS.wav");
}

/*
=================
CL_RegisterTEntModels
=================
*/
void CL_RegisterTEntModels(void)
{
    cl_mod_explode = R_RegisterModel("models/objects/explode/tris.md2");
    cl_mod_smoke = R_RegisterModel("models/objects/smoke/tris.md2");
    cl_mod_flash = R_RegisterModel("models/objects/flash/tris.md2");
    cl_mod_parasite_segment = R_RegisterModel("models/monsters/parasite/segment/tris.md2");
    cl_mod_grapple_cable = R_RegisterModel("models/ctf/segment/tris.md2");
    cl_mod_explo4 = R_RegisterModel("models/objects/r_explode/tris.md2");
    cl_mod_bfg_explo = R_RegisterModel("sprites/s_bfg2.sp2");
    cl_mod_powerscreen = R_RegisterModel("models/items/armor/effect/tris.md2");
    cl_mod_laser = R_RegisterModel("models/objects/laser/tris.md2");
    cl_mod_dmspot = R_RegisterModel("models/objects/dmspot/tris.md2");

    cl_mod_lightning = R_RegisterModel("models/proj/lightning/tris.md2");
    cl_mod_heatbeam = R_RegisterModel("models/proj/beam/tris.md2");
    cl_mod_explo4_big = R_RegisterModel("models/objects/r_explode2/tris.md2");

	// Generations
	cl_mod_q1_lightning1 = R_RegisterModel("models/q1/bolt.mdl");
	cl_mod_q1_lightning2 = R_RegisterModel("models/q1/bolt2.mdl");
	cl_mod_q1_lightning3 = R_RegisterModel("models/q1/bolt3.mdl");
	cl_mod_q1_explode = R_RegisterModel("sprites/q1/s_explod.spr");
	cl_mod_player = R_RegisterModel("%p_player");

	cl_mod_doom_tfog = R_RegisterModel("sprites/doom/TFOG.d2s");
	cl_mod_doom_ifog = R_RegisterModel("sprites/doom/IFOG.d2s");
	cl_mod_doom_puff = R_RegisterModel("sprites/doom/PUFF.d2s");
	cl_mod_doom_misl = R_RegisterModel("sprites/doom/MISL.d2s");
	cl_mod_doom_plse = R_RegisterModel("sprites/doom/PLSE.d2s");
	cl_mod_doom_bal1 = R_RegisterModel("sprites/doom/BAL1.d2s");
	cl_mod_doom_bal7 = R_RegisterModel("sprites/doom/BAL7.d2s");
	cl_mod_doom_bal2 = R_RegisterModel("sprites/doom/BAL2.d2s");
	cl_mod_doom_apbx = R_RegisterModel("sprites/doom/APBX.d2s");
	cl_mod_doom_blud = R_RegisterModel("sprites/doom/BLUD.d2s");
	cl_mod_doom_fbxp = R_RegisterModel("sprites/doom/FBXP.d2s");

	cl_mod_duke_shotspark = R_RegisterModel("sprites/duke/shotspark.dns");
	cl_mod_duke_shotsmoke = R_RegisterModel("sprites/duke/shotsmoke.dns");
	cl_mod_duke_shotblood = R_RegisterModel("sprites/duke/shotblood.dns");
	cl_mod_duke_shotgunshell = R_RegisterModel("sprites/duke/shotgunshell.dns");
	cl_mod_duke_shell = R_RegisterModel("sprites/duke/shell.dns");
	cl_mod_duke_explosion = R_RegisterModel("sprites/duke/explosion.dns");
	cl_mod_duke_explosion2 = R_RegisterModel("sprites/duke/explosion2.dns");
	cl_mod_duke_teleport = R_RegisterModel("sprites/duke/teleport.dns");
	cl_mod_duke_freezeblast = R_RegisterModel("sprites/duke/respawn.dns");
	cl_mod_duke_glass = R_RegisterModel("sprites/duke/glass.dns");
}

/*
==============================================================

EXPLOSION MANAGEMENT

==============================================================
*/

#define MAX_EXPLOSIONS  32

typedef struct {
    enum {
		ex_free,
		ex_explosion,
		ex_misc,
		ex_flash,
		ex_mflash,
		ex_poly,
		ex_poly2,
		ex_light

		// Generations
		, ex_q1_explosion,
	
		ex_doom_puff,
		ex_doom_boom,
		ex_doom_plas,
		ex_doom_blood,
		ex_doom_imp,
		ex_duke_teleport,
		ex_duke_fast,
		ex_duke_glass,
	
		ex_duke_blood,
	
		ex_duke_shell
	} type;

    entity_t    ent;
    int         frames;
    float       light;
    vec3_t      lightcolor;
    float       start;
    int         baseframe;

	// Generations
	int			frameofs;
	vec3_t		velocity;
	vec3_t		accel;
	bool		hitground;
} explosion_t;

static explosion_t  cl_explosions[MAX_EXPLOSIONS];

static void CL_ClearExplosions(void)
{
    memset(cl_explosions, 0, sizeof(cl_explosions));
}

static explosion_t *CL_AllocExplosion(void)
{
    explosion_t *e, *oldest;
    int     i;
    int     time;

    for (i = 0, e = cl_explosions; i < MAX_EXPLOSIONS; i++, e++) {
        if (e->type == ex_free) {
            memset(e, 0, sizeof(*e));
			// Generations
			e->ent.scale = 1;
            return e;
        }
    }
// find the oldest explosion
    time = cl.time;
    oldest = cl_explosions;

    for (i = 0, e = cl_explosions; i < MAX_EXPLOSIONS; i++, e++) {
        if (e->start < time) {
            time = e->start;
            oldest = e;
        }
    }
    memset(oldest, 0, sizeof(*oldest));
	// Generations
	oldest->ent.scale = 1;
    return oldest;
}

static explosion_t *CL_PlainExplosion(void)
{
    explosion_t *ex;

    ex = CL_AllocExplosion();
    VectorCopy(te.pos1, ex->ent.origin);
    ex->type = ex_poly;
    ex->ent.flags = RF_FULLBRIGHT;
    ex->start = cl.servertime - CL_FRAMETIME;
    ex->light = 350;
    VectorSet(ex->lightcolor, 1.0f, 0.5f, 0.5f);
    ex->ent.angles[1] = Q_rand() % 360;
    ex->ent.model = cl_mod_explo4;
    if (frand() < 0.5f)
        ex->baseframe = 15;
    ex->frames = 15;

    return ex;
}

/*
=================
CL_SmokeAndFlash
=================
*/
void CL_SmokeAndFlash(vec3_t origin)
{
    explosion_t *ex;

    ex = CL_AllocExplosion();
    VectorCopy(origin, ex->ent.origin);
    ex->type = ex_misc;
    ex->frames = 4;
    ex->ent.flags = RF_TRANSLUCENT | RF_NOSHADOW;
    ex->start = cl.servertime - CL_FRAMETIME;
    ex->ent.model = cl_mod_smoke;

    ex = CL_AllocExplosion();
    VectorCopy(origin, ex->ent.origin);
    ex->type = ex_flash;
    ex->ent.flags = RF_FULLBRIGHT;
    ex->frames = 2;
    ex->start = cl.servertime - CL_FRAMETIME;
    ex->ent.model = cl_mod_flash;
}

static void CL_AddExplosions(void)
{
    entity_t    *ent;
    int         i;
    explosion_t *ex;
    float       frac;
    int         f;
	trace_t		tr; // Generations

    memset(&ent, 0, sizeof(ent));

    for (i = 0, ex = cl_explosions; i < MAX_EXPLOSIONS; i++, ex++) {
        if (ex->type == ex_free)
            continue;
        frac = (cl.time - ex->start) * BASE_1_FRAMETIME;
        f = floor(frac);

        ent = &ex->ent;

		// Generations
		VectorMA(ex->velocity, cls.frametime, ex->accel, ex->velocity);

		vec3_t moved;

		VectorCopy(ent->origin, moved);
		VectorMA(ent->origin, cls.frametime, ex->velocity, moved);

        switch (ex->type) {
        case ex_mflash:
            if (f >= ex->frames - 1)
                ex->type = ex_free;
            break;
        case ex_misc:
        case ex_light:
            if (f >= ex->frames - 1) {
                ex->type = ex_free;
                break;
            }
            ent->alpha = 1.0f - frac / (ex->frames - 1);
            break;
        case ex_flash:
            if (f >= 1) {
                ex->type = ex_free;
                break;
            }
            ent->alpha = 1.0f;
            break;
        case ex_poly:
            if (f >= ex->frames - 1) {
                ex->type = ex_free;
                break;
            }

            ent->alpha = (16.0f - (float)f) / 16.0f;

            if (f < 10) {
                ent->skinnum = (f >> 1);
                if (ent->skinnum < 0)
                    ent->skinnum = 0;
            } else {
                ent->flags |= RF_TRANSLUCENT;
                if (f < 13)
                    ent->skinnum = 5;
                else
                    ent->skinnum = 6;
            }
            break;
        case ex_poly2:
            if (f >= ex->frames - 1) {
                ex->type = ex_free;
                break;
            }

            ent->alpha = (5.0f - (float)f) / 5.0f;
            ent->skinnum = 0;
            ent->flags |= RF_TRANSLUCENT;
            break;
		// Generations
		case ex_q1_explosion:
			if (f >= ex->frames)
				ex->type = ex_free;
			ent->flags |= RF_FULLBRIGHT;
			ent->alpha = 1.0f;
			break;
		case ex_doom_puff:
			f = floor(frac / 1.5f) + 1;
			
			if (f >= ex->frames)
				ex->type = ex_free;

			ent->alpha = 1.0f;
			ent->flags |= RF_FULLBRIGHT;
			VectorCopy(moved, ent->origin);
			break;
		case ex_doom_boom:
			f = floor(frac / 2.0f);
			if (f >= ex->frames)
				ex->type = ex_free;
			ent->alpha = 1.0f;
			ent->flags |= RF_FULLBRIGHT;
			break;
		case ex_duke_teleport:
			f = floor(frac / 1.5f);
			if (f >= ex->frames)
				ex->type = ex_free;
			ent->alpha = 1.0f;
			ent->flags |= RF_FULLBRIGHT;
			break;
		case ex_doom_imp:
			f = floor(frac / 2.0f);
			if (f >= ex->frames)
				ex->type = ex_free;
			f = f + 2;
			ent->alpha = 1.0f;
			ent->flags |= RF_FULLBRIGHT;
			break;
		case ex_doom_plas:
			f = floor(frac / 1.0f);
			if (f >= ex->frames)
				ex->type = ex_free;
			ent->alpha = 1.0f;
			ent->flags |= RF_FULLBRIGHT;
			break;
		case ex_duke_fast:
			f = floor(frac / 0.5f);
			if (f >= ex->frames)
				ex->type = ex_free;
			ent->flags |= RF_FULLBRIGHT;
			break;
		case ex_doom_blood:
			f = ex->frames;
			ent->alpha = 1.0f;

			if (!ex->hitground)
			{
				CM_BoxTrace(&tr, ent->origin, moved, vec3_origin, vec3_origin, cl.bsp->nodes, MASK_SOLID);

				if (tr.fraction != 1.0)
				{
					VectorClear(ex->velocity);
					VectorClear(ex->accel);

					VectorMA(tr.endpos, 2, tr.plane.normal, ent->origin);
					ex->hitground = true;
					ex->start = cl.servertime - CL_FRAMETIME;
				}
				else
					VectorCopy(moved, ent->origin);
			}
			else
			{
				f = ex->frames - floor(frac / 1.5f);

				if (f < 0)
					ex->type = ex_free;
			}
			
			break;
		case ex_duke_blood:
			f = ex->frames;
			ent->alpha = 1.0f;

			if (!ex->hitground)
			{
				CM_BoxTrace(&tr, ent->origin, moved, vec3_origin, vec3_origin, cl.bsp->nodes, MASK_SOLID);

				if (tr.fraction != 1.0f)
				{
					VectorClear(ex->velocity);
					VectorClear(ex->accel);

					VectorMA(tr.endpos, 3, tr.plane.normal, ent->origin);
					ex->hitground = true;
					ex->start = cl.servertime - CL_FRAMETIME;
				}
				else
					VectorCopy(moved, ent->origin);

				f = ((int)floor(frac / 1.5f)) % 2;
			}
			else
			{
				f = 2 + floor(frac);

				if (f >= ex->frames)
					ex->type = ex_free;
			}

			break;

		case ex_duke_shell:
			ent->alpha = 1.0f;

			CM_BoxTrace(&tr, ent->origin, moved, vec3_origin, vec3_origin, cl.bsp->nodes, MASK_SOLID);

			if (tr.fraction != 1.0f)
				ex->type = ex_free;
			else
				VectorCopy(moved, ent->origin);

			f = (int)(ex->frameofs + floor(frac / 1.5f)) % ex->frames;
			break;

		case ex_duke_glass:
			ent->alpha = 1.0f;

			CM_BoxTrace(&tr, ent->origin, moved, vec3_origin, vec3_origin, cl.bsp->nodes, MASK_SOLID);

			if (ex->hitground)
				ent->scale = min(0.5f, (1 - frac / 8.0f));

			if (ent->scale <= 0)
				ex->type = ex_free;
			else if (tr.fraction != 1.0f)
			{
				VectorMA(tr.endpos, 2, tr.plane.normal, ent->origin);

				for (int x = 0; x < 3; ++x)
					ex->velocity[x] -= (2 * 0.8f) * tr.plane.normal[x] * (tr.plane.normal[x] * ex->velocity[x]);

				if (!ex->hitground)
				{
					ex->hitground = true;
					ex->start = cl.servertime - CL_FRAMETIME;
				}
			}
			else
				VectorCopy(moved, ent->origin);

			f = (int)(ex->frameofs + floor(frac / 1.5f)) % ex->frames;

			break;

        default:
            break;
        }

        if (ex->type == ex_free)
            continue;

        if (ex->light)
            V_AddLight(ent->origin, ex->light * ent->alpha,
                       ex->lightcolor[0], ex->lightcolor[1], ex->lightcolor[2]);

        if (ex->type != ex_light) {
            VectorCopy(ent->origin, ent->oldorigin);

            if (f < 0)
                f = 0;

			// Generations
			if (ex->type <= ex_poly2) {
				ent->frame = ex->baseframe + f + 1;
				ent->oldframe = ex->baseframe + f;
			} else {
				ent->frame = ex->baseframe + f;
				ent->oldframe = ex->baseframe + f - 1;
			}
			ent->backlerp = 1.0f - (frac - f);

            V_AddEntity(ent);
        }
    }
}

/*
==============================================================

LASER MANAGEMENT

==============================================================
*/

#define MAX_LASERS  1024

typedef struct {
    vec3_t      start;
    vec3_t      end;
    int         color;
    color_t     rgba;
    int         width;
    int         lifetime, starttime;
} laser_t;

static laser_t  cl_lasers[MAX_LASERS];

static void CL_ClearLasers(void)
{
    memset(cl_lasers, 0, sizeof(cl_lasers));
}

static laser_t *CL_AllocLaser(void)
{
    laser_t *l;
    int i;

    for (i = 0, l = cl_lasers; i < MAX_LASERS; i++, l++) {
        if (cl.time - l->starttime >= l->lifetime) {
            memset(l, 0, sizeof(*l));
            l->starttime = cl.time;
            return l;
        }
    }

    return NULL;
}

static void CL_AddLasers(void)
{
    laser_t     *l;
    entity_t    ent;
    int         i;
    int         time;

    memset(&ent, 0, sizeof(ent));
	// Generations
	ent.scale = 1;

    for (i = 0, l = cl_lasers; i < MAX_LASERS; i++, l++) {
        time = l->lifetime - (cl.time - l->starttime);
        if (time < 0) {
            continue;
        }

        if (l->color == -1) {
            float f = (float)time / (float)l->lifetime;

            ent.rgba.u8[0] = l->rgba.u8[0];
            ent.rgba.u8[1] = l->rgba.u8[1];
            ent.rgba.u8[2] = l->rgba.u8[2];
            ent.rgba.u8[3] = l->rgba.u8[3] * f;
            ent.alpha = f;
        } else {
            ent.alpha = 0.30f;
        }

        ent.skinnum = l->color;
        ent.flags = RF_TRANSLUCENT | RF_BEAM;
        VectorCopy(l->start, ent.origin);
        VectorCopy(l->end, ent.oldorigin);
        ent.frame = l->width;

        V_AddEntity(&ent);
    }
}

static void CL_ParseLaser(int colors)
{
    laser_t *l;

    l = CL_AllocLaser();
    if (!l)
        return;

    VectorCopy(te.pos1, l->start);
    VectorCopy(te.pos2, l->end);
    l->lifetime = 100;
    l->color = (colors >> ((Q_rand() % 4) * 8)) & 0xff;
    l->width = 4;
}

/*
==============================================================

BEAM MANAGEMENT

==============================================================
*/

#define MAX_BEAMS   32

typedef struct {
    int         	entity;
    int         	dest_entity;
	modelhandle_t   model;
    int         	endtime;
    vec3_t      	offset;
    vec3_t      	start, end;
} beam_t;

static beam_t   cl_beams[MAX_BEAMS];
static beam_t   cl_playerbeams[MAX_BEAMS];

static void CL_ClearBeams(void)
{
    memset(cl_beams, 0, sizeof(cl_beams));
    memset(cl_playerbeams, 0, sizeof(cl_playerbeams));
}

static void CL_ParseBeam(modelhandle_t model)
{
    beam_t  *b;
    int     i;

// override any beam with the same source AND destination entities
    for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++)
        if (b->entity == te.entity1 && b->dest_entity == te.entity2)
            goto override;

// find a free beam
    for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++) {
        if (!b->model.handle || b->endtime < cl.time) {
override:
            b->entity = te.entity1;
            b->dest_entity = te.entity2;
            b->model = model;
            b->endtime = cl.time + 200;
            VectorCopy(te.pos1, b->start);
            VectorCopy(te.pos2, b->end);
            VectorCopy(te.offset, b->offset);
            return;
        }
    }
}

static void CL_ParsePlayerBeam(modelhandle_t model)
{
    beam_t  *b;
    int     i;

// override any beam with the same entity
    for (i = 0, b = cl_playerbeams; i < MAX_BEAMS; i++, b++) {
        if (b->entity == te.entity1) {
            b->entity = te.entity1;
            b->model = model;
            b->endtime = cl.time + 200;
            VectorCopy(te.pos1, b->start);
            VectorCopy(te.pos2, b->end);
            VectorCopy(te.offset, b->offset);
            return;
        }
    }

// find a free beam
    for (i = 0, b = cl_playerbeams; i < MAX_BEAMS; i++, b++) {
        if (!b->model.handle || b->endtime < cl.time) {
            b->entity = te.entity1;
            b->model = model;
            b->endtime = cl.time + 100;     // PMM - this needs to be 100 to prevent multiple heatbeams
            VectorCopy(te.pos1, b->start);
            VectorCopy(te.pos2, b->end);
            VectorCopy(te.offset, b->offset);
            return;
        }
    }

}

/*
=================
CL_AddBeams
=================
*/
static void CL_AddBeams(void)
{
    int         i, j;
    beam_t      *b;
    vec3_t      dist, org;
    float       d;
    entity_t    ent;
    vec3_t      angles;
    float       len, steps;
    float       model_length;

// update beams
    for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++) {
        if (!b->model.handle || b->endtime < cl.time)
            continue;

        // if coming from the player, update the start position
        if (b->entity == cl.frame.clientNum + 1)
            VectorAdd(cl.playerEntityOrigin, b->offset, org);
        else
            VectorAdd(b->start, b->offset, org);

        // calculate pitch and yaw
        VectorSubtract(b->end, org, dist);
        vectoangles2(dist, angles);

        // add new entities for the beams
        d = VectorNormalize(dist);
        if (b->model.handle == cl_mod_lightning.handle) {
            model_length = 35.0f;
            d -= 20.0f; // correction so it doesn't end in middle of tesla
        } else {
            model_length = 30.0f;
        }
        steps = ceil(d / model_length);
        len = (d - model_length) / (steps - 1);

        memset(&ent, 0, sizeof(ent));
        ent.model = b->model;

		// Generations
		ent.scale = 1;

        // PMM - special case for lightning model .. if the real length is shorter than the model,
        // flip it around & draw it from the end to the start.  This prevents the model from going
        // through the tesla mine (instead it goes through the target)
        if ((b->model.handle == cl_mod_lightning.handle) && (d <= model_length)) {
            VectorCopy(b->end, ent.origin);
            ent.flags = RF_FULLBRIGHT;
            ent.angles[0] = angles[0];
            ent.angles[1] = angles[1];
            ent.angles[2] = Q_rand() % 360;
            V_AddEntity(&ent);
            return;
        }

        while (d > 0) {
            VectorCopy(org, ent.origin);
            if (b->model.handle == cl_mod_lightning.handle) {
                ent.flags = RF_FULLBRIGHT;
                ent.angles[0] = -angles[0];
                ent.angles[1] = angles[1] + 180.0f;
                ent.angles[2] = Q_rand() % 360;
            } else {
                ent.angles[0] = angles[0];
                ent.angles[1] = angles[1];
                ent.angles[2] = Q_rand() % 360;
            }

            V_AddEntity(&ent);

            for (j = 0; j < 3; j++)
                org[j] += dist[j] * len;
            d -= model_length;
        }
    }
}

/*
=================
CL_AddPlayerBeams

Draw player locked beams. Currently only used by the plasma beam.
=================
*/
static void CL_AddPlayerBeams(void)
{
    int         i, j;
    beam_t      *b;
    vec3_t      dist, org;
    float       d;
    entity_t    ent;
    vec3_t      angles;
    float       len, steps;
    int         framenum;
    float       model_length;
    float       hand_multiplier;
    player_state_t  *ps, *ops;

    if (info_hand->integer == 2)
        hand_multiplier = 0;
    else if (info_hand->integer == 1)
        hand_multiplier = -1;
    else
        hand_multiplier = 1;

// update beams
    for (i = 0, b = cl_playerbeams; i < MAX_BEAMS; i++, b++) {
        if (!b->model.handle || b->endtime < cl.time)
            continue;

        // if coming from the player, update the start position
        if (b->entity == cl.frame.clientNum + 1) {
            // set up gun position
            ps = CL_KEYPS;
            ops = CL_OLDKEYPS;

			// Generations
            for (j = 0; j < 3; j++)
                b->start[j] = cl.refdef.vieworg[j] + ops->guns[GUN_MAIN].offset[j] +
                    CL_KEYLERPFRAC * (ps->guns[GUN_MAIN].offset[j] - ops->guns[GUN_MAIN].offset[j]);

            VectorMA(b->start, (hand_multiplier * b->offset[0]), cl.v_right, org);
            VectorMA(org, b->offset[1], cl.v_forward, org);
            VectorMA(org, b->offset[2], cl.v_up, org);
            if (info_hand->integer == 2)
                VectorMA(org, -1, cl.v_up, org);

            // calculate pitch and yaw
            VectorSubtract(b->end, org, dist);

            // FIXME: don't add offset twice?
            d = VectorLength(dist);
            VectorScale(cl.v_forward, d, dist);
            VectorMA(dist, (hand_multiplier * b->offset[0]), cl.v_right, dist);
            VectorMA(dist, b->offset[1], cl.v_forward, dist);
            VectorMA(dist, b->offset[2], cl.v_up, dist);
            if (info_hand->integer == 2)
                VectorMA(org, -1, cl.v_up, org);

            // FIXME: use cl.refdef.viewangles?
            vectoangles2(dist, angles);

            // if it's the heatbeam, draw the particle effect
            CL_Heatbeam(org, dist);

            framenum = 1;
        } else {
            VectorCopy(b->start, org);

            // calculate pitch and yaw
            VectorSubtract(b->end, org, dist);
            vectoangles2(dist, angles);

            // if it's a non-origin offset, it's a player, so use the hardcoded player offset
            if (!VectorEmpty(b->offset)) {
                vec3_t  tmp, f, r, u;

                tmp[0] = angles[0];
                tmp[1] = angles[1] + 180.0f;
                tmp[2] = 0;
                AngleVectors(tmp, f, r, u);

                VectorMA(org, -b->offset[0] + 1, r, org);
                VectorMA(org, -b->offset[1], f, org);
                VectorMA(org, -b->offset[2] - 10, u, org);
            } else {
                // if it's a monster, do the particle effect
                CL_MonsterPlasma_Shell(b->start);
            }

            framenum = 2;
        }

        // add new entities for the beams
        d = VectorNormalize(dist);
        model_length = 32.0f;
        steps = ceil(d / model_length);
        len = (d - model_length) / (steps - 1);

        memset(&ent, 0, sizeof(ent));
        ent.model = b->model;
        ent.frame = framenum;
        ent.flags = RF_FULLBRIGHT;
        ent.angles[0] = -angles[0];
        ent.angles[1] = angles[1] + 180.0f;
        ent.angles[2] = cl.time % 360;

		// Generations
		ent.scale = 1;

        while (d > 0) {
            VectorCopy(org, ent.origin);

            V_AddEntity(&ent);

            for (j = 0; j < 3; j++)
                org[j] += dist[j] * len;
            d -= model_length;
        }
    }
}


/*
==============================================================

SUSTAIN MANAGEMENT

==============================================================
*/

#define MAX_SUSTAINS    32

static cl_sustain_t     cl_sustains[MAX_SUSTAINS];

static void CL_ClearSustains(void)
{
    memset(cl_sustains, 0, sizeof(cl_sustains));
}

static cl_sustain_t *CL_AllocSustain(void)
{
    cl_sustain_t    *s;
    int             i;

    for (i = 0, s = cl_sustains; i < MAX_SUSTAINS; i++, s++) {
        if (s->id == 0)
            return s;
    }

    return NULL;
}

static void CL_ProcessSustain(void)
{
    cl_sustain_t    *s;
    int             i;

    for (i = 0, s = cl_sustains; i < MAX_SUSTAINS; i++, s++) {
        if (s->id) {
            if ((s->endtime >= cl.time) && (cl.time >= s->nextthink))
                s->think(s);
            else if (s->endtime < cl.time)
                s->id = 0;
        }
    }
}

static void CL_ParseSteam(void)
{
    cl_sustain_t    *s;

    if (te.entity1 == -1) {
        CL_ParticleSteamEffect(te.pos1, te.dir, te.color & 0xff, te.count, te.entity2);
        return;
    }

    s = CL_AllocSustain();
    if (!s)
        return;

    s->id = te.entity1;
    s->count = te.count;
    VectorCopy(te.pos1, s->org);
    VectorCopy(te.dir, s->dir);
    s->color = te.color & 0xff;
    s->magnitude = te.entity2;
    s->endtime = cl.time + te.time;
    s->think = CL_ParticleSteamEffect2;
    s->thinkinterval = 100;
    s->nextthink = cl.time;
}

static void CL_ParseWidow(void)
{
    cl_sustain_t    *s;

    s = CL_AllocSustain();
    if (!s)
        return;

    s->id = te.entity1;
    VectorCopy(te.pos1, s->org);
    s->endtime = cl.time + 2100;
    s->think = CL_Widowbeamout;
    s->thinkinterval = 1;
    s->nextthink = cl.time;
}

static void CL_ParseNuke(void)
{
    cl_sustain_t    *s;

    s = CL_AllocSustain();
    if (!s)
        return;

    s->id = 21000;
    VectorCopy(te.pos1, s->org);
    s->endtime = cl.time + 1000;
    s->think = CL_Nukeblast;
    s->thinkinterval = 1;
    s->nextthink = cl.time;
}

//==============================================================

static color_t  railcore_color;
static color_t  railspiral_color;

static cvar_t *cl_railtrail_type;
static cvar_t *cl_railtrail_time;
static cvar_t *cl_railcore_color;
static cvar_t *cl_railcore_width;
static cvar_t *cl_railspiral_color;
static cvar_t *cl_railspiral_radius;

static void cl_railcore_color_changed(cvar_t *self)
{
    if (!SCR_ParseColor(self->string, &railcore_color)) {
        Com_WPrintf("Invalid value '%s' for '%s'\n", self->string, self->name);
        Cvar_Reset(self);
        railcore_color.u32 = U32_RED;
    }
}

static void cl_railspiral_color_changed(cvar_t *self)
{
    if (!SCR_ParseColor(self->string, &railspiral_color)) {
        Com_WPrintf("Invalid value '%s' for '%s'\n", self->string, self->name);
        Cvar_Reset(self);
        railspiral_color.u32 = U32_BLUE;
    }
}

static void CL_RailCore(void)
{
    laser_t *l;

    l = CL_AllocLaser();
    if (!l)
        return;

    VectorCopy(te.pos1, l->start);
    VectorCopy(te.pos2, l->end);
    l->color = -1;
    l->lifetime = cl_railtrail_time->integer;
    l->width = cl_railcore_width->integer;
    l->rgba.u32 = railcore_color.u32;
}

void CL_AddNavLaser(vec3_t first, vec3_t second, int width, color_t color)
{
	laser_t *l;

	l = CL_AllocLaser();
	if (!l)
		return;

	VectorCopy(first, l->start);
	VectorCopy(second, l->end);
	l->color = -1;
	l->lifetime = 1;
	l->width = width;
	l->rgba.u32 = color.u32;
}

static void CL_RailSpiral(void)
{
    vec3_t      move;
    vec3_t      vec;
    float       len;
    int         j;
    cparticle_t *p;
    vec3_t      right, up;
    int         i;
    float       d, c, s;
    vec3_t      dir;

    VectorCopy(te.pos1, move);
    VectorSubtract(te.pos2, te.pos1, vec);
    len = VectorNormalize(vec);

    MakeNormalVectors(vec, right, up);

    for (i = 0; i < len; i++) {
        p = CL_AllocParticle();
        if (!p)
            return;

        p->time = cl.time;
        VectorClear(p->accel);

        d = i * 0.1f;
        c = cos(d);
        s = sin(d);

        VectorScale(right, c, dir);
        VectorMA(dir, s, up, dir);

        p->alpha = 1.0f;
        p->alphavel = -1.0f / (cl_railtrail_time->value + frand() * 0.2f);
        p->color = -1;
        p->rgba.u32 = railspiral_color.u32;
        for (j = 0; j < 3; j++) {
            p->org[j] = move[j] + dir[j] * cl_railspiral_radius->value;
            p->vel[j] = dir[j] * 6;
        }

        VectorAdd(move, vec, move);
    }
}

static void CL_RailTrail(void)
{
    if (!cl_railtrail_type->integer) {
        CL_OldRailTrail();
    } else {
        if (cl_railcore_width->integer > 0) {
        	CL_RailCore();
        }
        if (cl_railtrail_type->integer > 1) {
            CL_RailSpiral();
        }
    }
}

static void dirtoangles(vec3_t angles)
{
    angles[0] = RAD2DEG(acos(te.dir[2]));
    if (te.dir[0])
        angles[1] = RAD2DEG(atan2(te.dir[1], te.dir[0]));
    else if (te.dir[1] > 0)
        angles[1] = 90;
    else if (te.dir[1] < 0)
        angles[1] = 270;
    else
        angles[1] = 0;
}


/*
===============
R_RunParticleEffect
Generations
===============
*/

int		ramp1[8] = { 0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61 };
int		ramp2[8] = { 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66 };
int		ramp3[8] = { 0x6d, 0x6b, 6, 5, 4, 3 };

void R_RunParticleEffect(vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	cparticle_t	*p;
	int			scale;

	if (count > 130)
		scale = 3;
	else if (count > 20)
		scale = 2;
	else
		scale = 1;

	for (i = 0; i<count; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		p->die = cl.time + 100*(Q_rand() % 5);
		p->color = -1;
		p->rgba.u32 = d_palettes[GAME_Q1][(color&~7) + (Q_rand() & 7)];
		p->type = PARTICLE_Q1_SLOWGRAV;
		VectorClear(p->accel);
		p->alpha = 1;
		p->alphavel = 0;
		p->accel[2] = -20;

		for (j = 0; j<3; j++)
		{
			p->org[j] = org[j] + scale*((rand() & 15) - 8);
			p->vel[j] = dir[j] * 15;// + (rand()%300)-150;
		}
	}
}

void R_ParticleExplosion(vec3_t org)
{
	int			i, j;
	cparticle_t	*p;

	for (i = 0; i<1024; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		p->die = cl.time + 5 * 1000;
		p->color = -1;
		p->rgba.u32 = d_palettes[GAME_Q1][ramp1[0]];
		p->ramp = rand() & 3;
		if (i & 1)
		{
			p->type = PARTICLE_Q1_EXPLODE;
			for (j = 0; j<3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
		else
		{
			p->type = PARTICLE_Q1_EXPLODE2;
			for (j = 0; j<3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
		VectorClear(p->accel);
		p->accel[2] = -PARTICLE_GRAVITY / 2;
		p->alpha = 1;
		p->alphavel = 0;
	}
}



/*
===============
R_LavaSplash

===============
*/
void R_LavaSplash(vec3_t org)
{
	int			i, j, k;
	cparticle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i = -16; i<16; i++)
		for (j = -16; j<16; j++)
			for (k = 0; k<1; k++)
			{
				p = CL_AllocParticle();

				if (!p)
					return;

				p->time = cl.time;
				p->color = -1;

				p->die = cl.time + (2 + (rand() & 31) * 0.02) * 1000;
				p->rgba.u32 = d_palettes[GAME_Q1][224 + (rand() & 7)];
				p->type = PARTICLE_Q1_SLOWGRAV;

				dir[0] = j * 8 + (rand() & 7);
				dir[1] = i * 8 + (rand() & 7);
				dir[2] = 256;

				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand() & 63);

				VectorNormalize(dir);
				vel = 50 + (rand() & 63);
				VectorScale(dir, vel, p->vel);

				VectorClear(p->accel);
				p->alpha = 1;
				p->alphavel = 0;
				p->accel[2] = -20;
			}
}


/*
===============
R_BlobExplosion

===============
*/
void R_BlobExplosion(vec3_t org)
{
	int			i, j;
	cparticle_t	*p;

	for (i = 0; i<1024; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		p->color = -1;

		p->die = cl.time + (1 + (rand() & 8)*0.05) * 1000;

		if (i & 1)
		{
			p->type = PARTICLE_Q1_BLOB1;
			p->rgba.u32 = d_palettes[GAME_Q1][66 + rand() % 6];
			for (j = 0; j<3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
		else
		{
			p->type = PARTICLE_Q1_BLOB2;
			p->rgba.u32 = d_palettes[GAME_Q1][150 + rand() % 6];
			for (j = 0; j<3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}

		VectorClear(p->accel);
		p->accel[2] = -PARTICLE_GRAVITY / 2;
		p->alpha = 1;
		p->alphavel = 0;
	}
}

void R_Doom_Teleport_Fog(int entity, vec3_t pos)
{
	S_StartSound(NULL, entity, CHAN_AUTO, S_RegisterSound("doom/TELEPT.wav"), 1, ATTN_IDLE, 0);

	// sprite
	explosion_t *ex = CL_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	ex->ent.origin[2] -= 20;
	ex->type = ex_doom_plas;
	ex->start = cl.servertime - CL_FRAMETIME;
	ex->light = 0;
	ex->ent.model = cl_mod_doom_tfog;
	ex->frames = 10;
}

void R_Doom_Respawn_Fog(vec3_t pos)
{
	// sprite
	explosion_t *ex = CL_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	ex->ent.origin[2] -= 20;
	ex->type = ex_doom_plas;
	ex->start = cl.servertime - CL_FRAMETIME;
	ex->light = 0;
	ex->ent.model = cl_mod_doom_ifog;
	ex->frames = 5;
}

void Duke_Teleport(int entity, vec3_t pos)
{
	S_StartSound(NULL, entity, CHAN_AUTO, S_RegisterSound("duke/TELEPORT.wav"), 1, ATTN_IDLE, 0);

	// sprite
	explosion_t *ex = CL_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	ex->type = ex_duke_teleport;
	ex->start = cl.servertime - CL_FRAMETIME;
	ex->light = 0;
	ex->ent.model = cl_mod_duke_teleport;
	ex->frames = 5;
}

void R_Q1_TeleportSplash(int, vec3_t);

void CL_MakeDukeGunshot()
{
	// sprite
	explosion_t *ex = CL_AllocExplosion();
	VectorCopy(te.pos1, ex->ent.origin);
	ex->type = ex_doom_plas;
	ex->start = cl.servertime - CL_FRAMETIME;
	ex->light = 0;
	ex->ent.model = cl_mod_duke_shotspark;
	ex->ent.scale = 0.1f;
	ex->frames = 4;

	ex = CL_AllocExplosion();
	VectorCopy(te.pos1, ex->ent.origin);
	ex->type = ex_doom_puff;
	ex->start = cl.servertime - CL_FRAMETIME;
	ex->light = 0;
	ex->ent.model = cl_mod_duke_shotsmoke;
	ex->ent.scale = 0.3f;
	ex->frames = 4;
	ex->velocity[2] = 40;
}

void CL_MakeDukeShell(vec3_t position, vec3_t velocity, bool shotgun)
{
	explosion_t *ex = CL_AllocExplosion();
	VectorCopy(position, ex->ent.origin);
	ex->type = ex_duke_shell;
	ex->start = cl.servertime - CL_FRAMETIME;
	ex->light = 0;
	ex->ent.model = shotgun ? cl_mod_duke_shotgunshell : cl_mod_duke_shell;
	ex->ent.scale = 0.05f;
	ex->frames = shotgun ? 2 : 4;
	VectorCopy(velocity, ex->velocity);
	ex->accel[2] = -400;
	ex->frameofs = (Q_rand() % ex->frames);
}

/*
=================
CL_ParseTEnt
=================
*/
static const byte splash_color[] = {0x00, 0xe0, 0xb0, 0x50, 0xd0, 0xe0, 0xe8};

void CL_ParseTEnt(void)
{
    explosion_t *ex;
    int r;

    switch (te.type) {
    case TE_BLOOD:          // bullet hitting flesh
        if (!(cl_disable_particles->integer & NOPART_BLOOD))
            CL_ParticleEffect(te.pos1, te.dir, 0xe8, 60);
        break;

    case TE_GUNSHOT:            // bullet hitting wall
    case TE_SPARKS:
    case TE_BULLET_SPARKS:
        if (te.type == TE_GUNSHOT)
            CL_ParticleEffect(te.pos1, te.dir, 0, 40);
        else
            CL_ParticleEffect(te.pos1, te.dir, 0xe0, 6);

        if (te.type != TE_SPARKS) {
            CL_SmokeAndFlash(te.pos1);

            // impact sound
            r = Q_rand() & 15;
            if (r == 1)
                S_StartSound(te.pos1, 0, 0, cl_sfx_ric1, 1, ATTN_NORM, 0);
            else if (r == 2)
                S_StartSound(te.pos1, 0, 0, cl_sfx_ric2, 1, ATTN_NORM, 0);
            else if (r == 3)
                S_StartSound(te.pos1, 0, 0, cl_sfx_ric3, 1, ATTN_NORM, 0);
        }
        break;

    case TE_SCREEN_SPARKS:
    case TE_SHIELD_SPARKS:
        if (te.type == TE_SCREEN_SPARKS)
            CL_ParticleEffect(te.pos1, te.dir, 0xd0, 40);
        else
            CL_ParticleEffect(te.pos1, te.dir, 0xb0, 40);
        //FIXME : replace or remove this sound
        S_StartSound(te.pos1, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
        break;

    case TE_SHOTGUN:            // bullet hitting wall
        CL_ParticleEffect(te.pos1, te.dir, 0, 20);
        CL_SmokeAndFlash(te.pos1);
        break;

    case TE_SPLASH:         // bullet hitting water
        if (te.color < 0 || te.color > 6)
            r = 0x00;
        else
            r = splash_color[te.color];
        CL_ParticleEffect(te.pos1, te.dir, r, te.count);

        if (te.color == SPLASH_SPARKS) {
            r = Q_rand() & 3;
            if (r == 0)
                S_StartSound(te.pos1, 0, 0, cl_sfx_spark5, 1, ATTN_STATIC, 0);
            else if (r == 1)
                S_StartSound(te.pos1, 0, 0, cl_sfx_spark6, 1, ATTN_STATIC, 0);
            else
                S_StartSound(te.pos1, 0, 0, cl_sfx_spark7, 1, ATTN_STATIC, 0);
        }
        break;

    case TE_LASER_SPARKS:
        CL_ParticleEffect2(te.pos1, te.dir, te.color, te.count);
        break;

    case TE_BLUEHYPERBLASTER:
        CL_BlasterParticles(te.pos1, te.dir);
        break;

    case TE_BLASTER:            // blaster hitting wall
    case TE_BLASTER2:           // green blaster hitting wall
    case TE_FLECHETTE:          // flechette
        ex = CL_AllocExplosion();
        VectorCopy(te.pos1, ex->ent.origin);
        dirtoangles(ex->ent.angles);
        ex->type = ex_misc;
        ex->ent.flags = RF_FULLBRIGHT | RF_TRANSLUCENT;
        switch (te.type) {
        case TE_BLASTER:
            CL_BlasterParticles(te.pos1, te.dir);
            ex->lightcolor[0] = 1;
            ex->lightcolor[1] = 1;
            break;
        case TE_BLASTER2:
            CL_BlasterParticles2(te.pos1, te.dir, 0xd0);
            ex->ent.skinnum = 1;
            ex->lightcolor[1] = 1;
            break;
        case TE_FLECHETTE:
            CL_BlasterParticles2(te.pos1, te.dir, 0x6f);  // 75
            ex->ent.skinnum = 2;
            ex->lightcolor[0] = 0.19f;
            ex->lightcolor[1] = 0.41f;
            ex->lightcolor[2] = 0.75f;
            break;
        }
        ex->start = cl.servertime - CL_FRAMETIME;
        ex->light = 150;
        ex->ent.model = cl_mod_explode;
        ex->frames = 4;
        S_StartSound(te.pos1,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
        break;

    case TE_RAILTRAIL:          // railgun effect
        CL_RailTrail();
        S_StartSound(te.pos2, 0, 0, cl_sfx_railg, 1, ATTN_NORM, 0);
        break;

    case TE_GRENADE_EXPLOSION:
    case TE_GRENADE_EXPLOSION_WATER:
        ex = CL_PlainExplosion();
        ex->frames = 19;
        ex->baseframe = 30;
        if (cl_disable_explosions->integer & NOEXP_GRENADE)
            ex->type = ex_light;

        if (!(cl_disable_particles->integer & NOPART_GRENADE_EXPLOSION))
            CL_ExplosionParticles(te.pos1);

        if (te.type == TE_GRENADE_EXPLOSION_WATER)
            S_StartSound(te.pos1, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
        else
            S_StartSound(te.pos1, 0, 0, cl_sfx_grenexp, 1, ATTN_NORM, 0);
        break;

    case TE_EXPLOSION2:
        ex = CL_PlainExplosion();
        ex->frames = 19;
        ex->baseframe = 30;
        CL_ExplosionParticles(te.pos1);
        S_StartSound(te.pos1, 0, 0, cl_sfx_grenexp, 1, ATTN_NORM, 0);
        break;

    case TE_PLASMA_EXPLOSION:
        CL_PlainExplosion();
        CL_ExplosionParticles(te.pos1);
        S_StartSound(te.pos1, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
        break;

    case TE_ROCKET_EXPLOSION:
    case TE_ROCKET_EXPLOSION_WATER:
        ex = CL_PlainExplosion();
        if (cl_disable_explosions->integer & NOEXP_ROCKET)
            ex->type = ex_light;

        if (!(cl_disable_particles->integer & NOPART_ROCKET_EXPLOSION))
            CL_ExplosionParticles(te.pos1);

        if (te.type == TE_ROCKET_EXPLOSION_WATER)
            S_StartSound(te.pos1, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
        else
            S_StartSound(te.pos1, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
        break;

    case TE_EXPLOSION1:
        CL_PlainExplosion();
        CL_ExplosionParticles(te.pos1);
        S_StartSound(te.pos1, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
        break;

    case TE_EXPLOSION1_NP:
        CL_PlainExplosion();
        S_StartSound(te.pos1, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
        break;

    case TE_EXPLOSION1_BIG:
        ex = CL_PlainExplosion();
        ex->ent.model = cl_mod_explo4_big;
        S_StartSound(te.pos1, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
        break;

    case TE_BFG_EXPLOSION:
        ex = CL_AllocExplosion();
        VectorCopy(te.pos1, ex->ent.origin);
        ex->type = ex_poly;
        ex->ent.flags = RF_FULLBRIGHT;
        ex->start = cl.servertime - CL_FRAMETIME;
        ex->light = 350;
        ex->lightcolor[0] = 0.0f;
        ex->lightcolor[1] = 1.0f;
        ex->lightcolor[2] = 0.0f;
        ex->ent.model = cl_mod_bfg_explo;
        ex->ent.flags |= RF_TRANSLUCENT;
        ex->ent.alpha = 0.30f;
        ex->frames = 4;
        break;

    case TE_BFG_BIGEXPLOSION:
        CL_BFGExplosionParticles(te.pos1);
        break;

    case TE_BFG_LASER:
        CL_ParseLaser(0xd0d1d2d3);
        break;

    case TE_BUBBLETRAIL:
        CL_BubbleTrail(te.pos1, te.pos2);
        break;

    case TE_PARASITE_ATTACK:
    case TE_MEDIC_CABLE_ATTACK:
        VectorClear(te.offset);
        te.entity2 = 0;
        CL_ParseBeam(cl_mod_parasite_segment);
        break;

	// Generations
	// Q1
	case TE_Q1_GUNSHOT:
		R_RunParticleEffect(te.pos1, vec3_origin, 0, 20 * te.count);
		break;
	case TE_Q1_BLOOD:
		R_RunParticleEffect(te.pos1, te.dir, 73, te.count);
		break;
	case TE_Q1_WIZSPIKE:
		R_RunParticleEffect(te.pos1, vec3_origin, 20, 30);
		S_StartSound(te.pos1, 0, 0, cl_sfx_wizhit, 1, ATTN_NORM, 0);
		break;
	case TE_Q1_KNIGHTSPIKE:
		R_RunParticleEffect(te.pos1, vec3_origin, 226, 20);
		S_StartSound(te.pos1, 0, 0, cl_sfx_knighthit, 1, ATTN_NORM, 0);
		break;
	case TE_Q1_TAREXPLOSION:
		R_BlobExplosion(te.pos1);

		// sound
		S_StartSound(te.pos1, 0, 0, cl_sfx_r_exp3, 1, ATTN_NORM, 0);

		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_q1_explosion;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 350;
		ex->lightcolor[0] = 0.2;
		ex->lightcolor[1] = 0.1;
		ex->lightcolor[2] = 0.05;
		ex->ent.model = cl_mod_q1_explode;
		ex->frames = 6;
		break;
	case TE_Q1_EXPLODE:
		R_ParticleExplosion(te.pos1);

		// sound
		S_StartSound(te.pos1, 0, 0, cl_sfx_r_exp3, 1, ATTN_NORM, 0);

		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_q1_explosion;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 350;
		ex->lightcolor[0] = 0.2;
		ex->lightcolor[1] = 0.1;
		ex->lightcolor[2] = 0.05;
		ex->ent.model = cl_mod_q1_explode;
		ex->frames = 6;
		break;

	case TE_DOOM_GUNSHOT:
		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_doom_puff;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_doom_puff;
		ex->velocity[2] = 40;
		ex->frames = 4;
		break;

	case TE_DUKE_GUNSHOT:
		CL_MakeDukeGunshot();

		if ((Q_rand()&255) < 4)
			S_StartSound(te.pos1, 0, 0, cl_sfx_duke_ricochet, 1, ATTN_NORM, 0);
		break;

	case TE_DUKE_BLOOD:
		CL_MakeDukeGunshot();

	case TE_DUKE_BLOOD_SPLAT:
		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_duke_blood;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_duke_shotblood;
		ex->ent.scale = 0.3f;
		ex->accel[2] = -400;
		ex->frames = 8;

		if (te.type == TE_DUKE_BLOOD_SPLAT)
			ex->hitground = true;
		break;

	case TE_DUKE_FREEZE_HIT:
		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_duke_fast;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_duke_freezeblast;
		ex->ent.scale = 0.4f;
		ex->frames = 10;
		ex->ent.flags |= RF_FROZEN | RF_TRANSLUCENT | RF_ANIM_BOUNCE;
		ex->ent.alpha = 0.6f;
		break;

	case TE_DUKE_GLASS:
		S_StartSound(te.pos1, 0, 0, cl_sfx_duke_glass, 1, ATTN_NORM, 0);

		for (int i = 0; i < 20; ++i)
		{
			ex = CL_AllocExplosion();
			VectorCopy(te.pos1, ex->ent.origin);

			for (int x = 0; x < 3; ++x)
				ex->ent.origin[x] += crand() * 12;

			ex->type = ex_duke_glass;
			ex->start = cl.servertime - CL_FRAMETIME;
			ex->light = 0;
			ex->ent.model = cl_mod_duke_glass;
			ex->ent.scale = 0.5f;
			ex->frames = 4;
			ex->ent.flags |= RF_FROZEN;
			ex->baseframe = rand() % ex->frames;
			ex->velocity[0] = crand() * 250;
			ex->velocity[1] = crand() * 250;
			ex->velocity[2] = 450 + crand() * 200;
			ex->accel[2] = -1400;
		}
		break;

	case TE_DUKE_EXPLODE:
	case TE_DUKE_EXPLODE_PIPE:
	case TE_DUKE_EXPLODE_SMALL:
		// sound

		if (te.type == TE_DUKE_EXPLODE_PIPE)
		{
			te.pos1[2] += 22;
			S_StartSound(te.pos1, 0, 0, cl_sfx_duke_pipe_boom, 1, ATTN_NORM, 0);
		}
		else
			S_StartSound(te.pos1, 0, 0, cl_sfx_duke_rpg_boom, 1, ATTN_NORM, 0);

		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_doom_plas;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_duke_explosion;

		if (te.type == TE_DUKE_EXPLODE_SMALL)
			ex->ent.scale = 0.2f;

		ex->frames = 20;

		if (te.type == TE_DUKE_EXPLODE_PIPE)
		{
			// sprite
			ex = CL_AllocExplosion();
			VectorCopy(te.pos1, ex->ent.origin);
			ex->type = ex_doom_plas;
			ex->start = cl.servertime - CL_FRAMETIME;
			ex->light = 0;
			ex->ent.model = cl_mod_duke_explosion2;

			ex->frames = 20;
		}
		break;

	case TE_DOOM_EXPLODE:
		// sound
		S_StartSound(te.pos1, 0, 0, cl_sfx_doom_boom, 1, ATTN_NORM, 0);

		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_doom_boom;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_doom_misl;
		ex->frames = 3;
		break;

	case TE_DOOM_PLASMA:
		// sound
		S_StartSound(te.pos1, 0, 0, cl_sfx_doom_plas, 1, ATTN_NORM, 0);

		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_doom_plas;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_doom_plse;
		ex->frames = 5;
		break;

	case TE_DOOM_IMP_BOOM:
		// sound
		S_StartSound(te.pos1, 0, 0, cl_sfx_doom_plas, 1, ATTN_NORM, 0);

		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_doom_imp;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_doom_bal1;
		ex->frames = 3;
		break;

	case TE_DOOM_BOSS_BOOM:
		// sound
		S_StartSound(te.pos1, 0, 0, cl_sfx_doom_plas, 1, ATTN_NORM, 0);

		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_doom_imp;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_doom_bal7;
		ex->frames = 3;
		break;

	case TE_DOOM_CACO_BOOM:
		// sound
		S_StartSound(te.pos1, 0, 0, cl_sfx_doom_plas, 1, ATTN_NORM, 0);

		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_doom_imp;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_doom_bal2;
		ex->frames = 3;
		break;

	case TE_DOOM_BSPI_BOOM:
		// sound
		S_StartSound(te.pos1, 0, 0, cl_sfx_doom_plas, 1, ATTN_NORM, 0);

		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_doom_plas;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_doom_apbx;
		ex->frames = 5;
		break;

	case TE_DOOM_BLOOD:
		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_doom_blood;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_doom_blud;
		ex->frames = 1;
		ex->accel[2] = -400;
		ex->hitground = false;
		break;

	case TE_DOOM_FBXP_BOOM:
		// sound
		S_StartSound(te.pos1, 0, 0, cl_sfx_doom_boom, 1, ATTN_NORM, 0);

		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_doom_plas;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_doom_fbxp;
		ex->frames = 3;
		break;

	case TE_DOOM_PUFF:
		// sprite
		ex = CL_AllocExplosion();
		VectorCopy(te.pos1, ex->ent.origin);
		ex->type = ex_doom_plas;
		ex->start = cl.servertime - CL_FRAMETIME;
		ex->light = 0;
		ex->ent.model = cl_mod_doom_puff;
		ex->frames = 4;
		break;

	case TE_Q1_SPIKE:
		R_RunParticleEffect(te.pos1, vec3_origin, 0, 10);

		if (rand() % 5)
			S_StartSound(te.pos1, 0, 0, cl_sfx_tink1, 1, ATTN_NORM, 0);
		else
		{
			int rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound(te.pos1, 0, 0, cl_sfx_ric1, 1, ATTN_NORM, 0);
			else if (rnd == 2)
				S_StartSound(te.pos1, 0, 0, cl_sfx_ric2, 1, ATTN_NORM, 0);
			else
				S_StartSound(te.pos1, 0, 0, cl_sfx_ric3, 1, ATTN_NORM, 0);
		}
		break;

	case TE_Q1_SUPERSPIKE:
		R_RunParticleEffect(te.pos1, vec3_origin, 0, 20);

		if (rand() % 5)
			S_StartSound(te.pos1, 0, 0, cl_sfx_tink1, 1, ATTN_NORM, 0);
		else
		{
			int rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound(te.pos1, 0, 0, cl_sfx_ric1, 1, ATTN_NORM, 0);
			else if (rnd == 2)
				S_StartSound(te.pos1, 0, 0, cl_sfx_ric2, 1, ATTN_NORM, 0);
			else
				S_StartSound(te.pos1, 0, 0, cl_sfx_ric3, 1, ATTN_NORM, 0);
		}
		break;

	case TE_Q1_LIGHTNINGBLOOD:
		{
			vec3_t lightning_ofs = { 0, 0, 6.25 };
			R_RunParticleEffect(te.pos1, lightning_ofs, 225, 50);
		}
		break;


	case TE_Q1_LIGHTNING1:
		VectorClear(te.offset);
		te.entity2 = 0;
		CL_ParseBeam(cl_mod_q1_lightning1);
		break;

	case TE_Q1_LIGHTNING2:
		VectorClear(te.offset);
		te.entity2 = 0;
		CL_ParseBeam(cl_mod_q1_lightning2);
		break;

	case TE_Q1_LIGHTNING3:
		VectorClear(te.offset);
		te.entity2 = 0;
		CL_ParseBeam(cl_mod_q1_lightning3);
		break;

	case TE_Q1_LAVASPLASH:
		R_LavaSplash(te.pos1);
		break;

    case TE_BOSSTPORT:          // boss teleporting to station
        CL_BigTeleportParticles(te.pos1);
        S_StartSound(te.pos1, 0, 0, S_RegisterSound("misc/bigtele.wav"), 1, ATTN_NONE, 0);
        break;

    case TE_GRAPPLE_CABLE:
        te.entity2 = 0;
        CL_ParseBeam(cl_mod_grapple_cable);
        break;

    case TE_WELDING_SPARKS:
        CL_ParticleEffect2(te.pos1, te.dir, te.color, te.count);

        ex = CL_AllocExplosion();
        VectorCopy(te.pos1, ex->ent.origin);
        ex->type = ex_flash;
        // note to self
        // we need a better no draw flag
        ex->ent.flags = RF_BEAM;
        ex->start = cl.servertime - CL_FRAMETIME;
        ex->light = 100 + (Q_rand() % 75);
        ex->lightcolor[0] = 1.0f;
        ex->lightcolor[1] = 1.0f;
        ex->lightcolor[2] = 0.3f;
        ex->ent.model = cl_mod_flash;
        ex->frames = 2;
        break;

    case TE_GREENBLOOD:
        CL_ParticleEffect2(te.pos1, te.dir, 0xdf, 30);
        break;

    case TE_TUNNEL_SPARKS:
        CL_ParticleEffect3(te.pos1, te.dir, te.color, te.count);
        break;

    case TE_LIGHTNING:
        S_StartSound(NULL, te.entity1, CHAN_WEAPON, cl_sfx_lightning, 1, ATTN_NORM, 0);
        VectorClear(te.offset);
        CL_ParseBeam(cl_mod_lightning);
        break;

    case TE_DEBUGTRAIL:
        CL_DebugTrail(te.pos1, te.pos2);
        break;

    case TE_PLAIN_EXPLOSION:
        CL_PlainExplosion();
        break;

    case TE_FLASHLIGHT:
#if USE_DLIGHTS
        CL_Flashlight(te.entity1, te.pos1);
#endif
        break;

    case TE_FORCEWALL:
        CL_ForceWall(te.pos1, te.pos2, te.color);
        break;

    case TE_HEATBEAM:
        VectorSet(te.offset, 2, 7, -3);
        CL_ParsePlayerBeam(cl_mod_heatbeam);
        break;

    case TE_MONSTER_HEATBEAM:
        VectorClear(te.offset);
        CL_ParsePlayerBeam(cl_mod_heatbeam);
        break;

    case TE_HEATBEAM_SPARKS:
        CL_ParticleSteamEffect(te.pos1, te.dir, 0x8, 50, 60);
        S_StartSound(te.pos1,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
        break;

    case TE_HEATBEAM_STEAM:
        CL_ParticleSteamEffect(te.pos1, te.dir, 0xE0, 20, 60);
        S_StartSound(te.pos1,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
        break;

    case TE_STEAM:
        CL_ParseSteam();
        break;

    case TE_BUBBLETRAIL2:
        CL_BubbleTrail2(te.pos1, te.pos2, 8);
        S_StartSound(te.pos1,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
        break;

    case TE_MOREBLOOD:
        CL_ParticleEffect(te.pos1, te.dir, 0xe8, 250);
        break;

    case TE_CHAINFIST_SMOKE:
        VectorSet(te.dir, 0, 0, 1);
        CL_ParticleSmokeEffect(te.pos1, te.dir, 0, 20, 20);
        break;

    case TE_ELECTRIC_SPARKS:
        CL_ParticleEffect(te.pos1, te.dir, 0x75, 40);
        //FIXME : replace or remove this sound
        S_StartSound(te.pos1, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
        break;

    case TE_TRACKER_EXPLOSION:
#if USE_DLIGHTS
        CL_ColorFlash(te.pos1, 0, 150, -1, -1, -1);
#endif
        CL_ColorExplosionParticles(te.pos1, 0, 1);
        S_StartSound(te.pos1, 0, 0, cl_sfx_disrexp, 1, ATTN_NORM, 0);
        break;

    case TE_TELEPORT_EFFECT:
    case TE_DBALL_GOAL:
        CL_TeleportParticles(te.pos1);
        break;

    case TE_WIDOWBEAMOUT:
        CL_ParseWidow();
        break;

    case TE_NUKEBLAST:
        CL_ParseNuke();
        break;

    case TE_WIDOWSPLASH:
        CL_WidowSplash();
        break;

    default:
        Com_Error(ERR_DROP, "%s: bad type", __func__);
    }
}

/*
=================
CL_AddTEnts
=================
*/
void CL_AddTEnts(void)
{
    CL_AddBeams();
    CL_AddPlayerBeams();
    CL_AddExplosions();
    CL_ProcessSustain();
    CL_AddLasers();
}

/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts(void)
{
    CL_ClearBeams();
    CL_ClearExplosions();
    CL_ClearLasers();
    CL_ClearSustains();
}

void CL_InitTEnts(void)
{
    cl_railtrail_type = Cvar_Get("cl_railtrail_type", "0", 0);
    cl_railtrail_time = Cvar_Get("cl_railtrail_time", "1.0", 0);
    cl_railtrail_time->changed = cl_timeout_changed;
    cl_railtrail_time->changed(cl_railtrail_time);
    cl_railcore_color = Cvar_Get("cl_railcore_color", "red", 0);
    cl_railcore_color->changed = cl_railcore_color_changed;
    cl_railcore_color->generator = Com_Color_g;
    cl_railcore_color_changed(cl_railcore_color);
    cl_railcore_width = Cvar_Get("cl_railcore_width", "2", 0);
    cl_railspiral_color = Cvar_Get("cl_railspiral_color", "blue", 0);
    cl_railspiral_color->changed = cl_railspiral_color_changed;
    cl_railspiral_color->generator = Com_Color_g;
    cl_railspiral_color_changed(cl_railspiral_color);
    cl_railspiral_radius = Cvar_Get("cl_railspiral_radius", "3", 0);
}
