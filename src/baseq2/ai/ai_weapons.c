/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "..\g_local.h"
#include "ai_local.h"

ai_weapon_t		AIWeapons[WEAP_TOTAL];


//WEAP_NONE,
//WEAP_BLASTER
//WEAP_SHOTGUN
//WEAP_SUPERSHOTGUN
//WEAP_MACHINEGUN
//WEAP_CHAINGUN
//WEAP_GRENADES
//WEAP_GRENADELAUNCHER
//WEAP_ROCKETLAUNCHER
//WEAP_HYPERBLASTER
//WEAP_RAILGUN
//WEAP_BFG
//WEAP_GRAPPLE


//==========================================
// AI_InitAIWeapons
// 
// AIWeapons are the way the AI uses to analize
// weapon types, for choosing and fire them
//==========================================
void AI_InitAIWeapons (void)
{
	//clear all
	memset( &AIWeapons, 0, sizeof(ai_weapon_t)*WEAP_TOTAL);

	//BLASTER
	AIWeapons[WEAP_BLASTER].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_LONG_RANGE] = 0.05f; //blaster must always have some value
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.05f;
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.1f;
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.2f;
	AIWeapons[WEAP_BLASTER].weaponItem = GetItemByIndex(ITI_BLASTER);
	AIWeapons[WEAP_BLASTER].ammoItem = NULL;		//doesn't use ammo

	//SHOTGUN
	AIWeapons[WEAP_SHOTGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.1f;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.1f;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2f;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3f;
	AIWeapons[WEAP_SHOTGUN].weaponItem = GetItemByIndex(ITI_SHOTGUN);
	AIWeapons[WEAP_SHOTGUN].ammoItem = GetItemByIndex(ITI_SHELLS);
	
	//SUPERSHOTGUN
	AIWeapons[WEAP_SUPERSHOTGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.2f;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.2f;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.6f;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.7f;
	AIWeapons[WEAP_SUPERSHOTGUN].weaponItem = GetItemByIndex(ITI_SUPER_SHOTGUN);
	AIWeapons[WEAP_SUPERSHOTGUN].ammoItem = GetItemByIndex(ITI_SHELLS);

	//MACHINEGUN
	AIWeapons[WEAP_MACHINEGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.3f;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.3f;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.3f;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4f;
	AIWeapons[WEAP_MACHINEGUN].weaponItem = GetItemByIndex(ITI_MACHINEGUN);
	AIWeapons[WEAP_MACHINEGUN].ammoItem = GetItemByIndex(ITI_BULLETS);

	//CHAINGUN
	AIWeapons[WEAP_CHAINGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.4f;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6f;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.7f;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.7f;
	AIWeapons[WEAP_CHAINGUN].weaponItem = GetItemByIndex(ITI_CHAINGUN);
	AIWeapons[WEAP_CHAINGUN].ammoItem = GetItemByIndex(ITI_BULLETS);

	//GRENADES
	AIWeapons[WEAP_GRENADES].aimType = AI_AIMSTYLE_DROP;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_LONG_RANGE] = 0.0f;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.0f;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2f;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_MELEE_RANGE] = 0.2f;
	AIWeapons[WEAP_GRENADES].weaponItem = GetItemByIndex(ITI_GRENADES);
	AIWeapons[WEAP_GRENADES].ammoItem = GetItemByIndex(ITI_GRENADES);

	//GRENADELAUNCHER
	AIWeapons[WEAP_GRENADELAUNCHER].aimType = AI_AIMSTYLE_DROP;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_LONG_RANGE] = 0.0f;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.0f;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.3f;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.5f;
	AIWeapons[WEAP_GRENADELAUNCHER].weaponItem = GetItemByIndex(ITI_GRENADE_LAUNCHER);
	AIWeapons[WEAP_GRENADELAUNCHER].ammoItem = GetItemByIndex(ITI_GRENADES);

	//ROCKETLAUNCHER
	AIWeapons[WEAP_ROCKETLAUNCHER].aimType = AI_AIMSTYLE_PREDICTION_EXPLOSIVE;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_LONG_RANGE] = 0.2f;	//machinegun is better
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.7f;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.9f;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.6f;
	AIWeapons[WEAP_ROCKETLAUNCHER].weaponItem = GetItemByIndex(ITI_ROCKET_LAUNCHER);
	AIWeapons[WEAP_ROCKETLAUNCHER].ammoItem = GetItemByIndex(ITI_ROCKETS);

	//WEAP_HYPERBLASTER
	AIWeapons[WEAP_HYPERBLASTER].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_LONG_RANGE] = 0.1f;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.1f;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2f;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3f;
	AIWeapons[WEAP_HYPERBLASTER].weaponItem = GetItemByIndex(ITI_HYPERBLASTER);
	AIWeapons[WEAP_HYPERBLASTER].ammoItem = GetItemByIndex(ITI_CELLS);

	//WEAP_RAILGUN
	AIWeapons[WEAP_RAILGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.9f;
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6f;
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.4f;
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3f;
	AIWeapons[WEAP_RAILGUN].weaponItem = GetItemByIndex(ITI_RAILGUN);
	AIWeapons[WEAP_RAILGUN].ammoItem = GetItemByIndex(ITI_SLUGS);

	//WEAP_BFG
	AIWeapons[WEAP_BFG].aimType = AI_AIMSTYLE_PREDICTION_EXPLOSIVE;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_LONG_RANGE] = 0.3f;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6f;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_SHORT_RANGE] = 0.7f;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_MELEE_RANGE] = 0.0f;
	AIWeapons[WEAP_BFG].weaponItem = GetItemByIndex(ITI_BFG10K);
	AIWeapons[WEAP_BFG].ammoItem = GetItemByIndex(ITI_CELLS);

#if CTF
	//WEAP_GRAPPLE
	AIWeapons[WEAP_GRAPPLE].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_LONG_RANGE] = 0.0f; //grapple is not used for attacks
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.0f;
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_SHORT_RANGE] = 0.0f;
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_MELEE_RANGE] = 0.0f;
	AIWeapons[WEAP_GRAPPLE].weaponItem = FindItemByClassname("weapon_grapplinghook");
	AIWeapons[WEAP_GRAPPLE].ammoItem = NULL;		//doesn't use ammo
#endif

}









