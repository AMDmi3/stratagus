//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name missile.c	-	The missiles. */
//
//	(c) Copyright 1998-2003 by Lutz Sammer and Jimmy Salmon
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; version 2 dated June, 1991.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//	$Id$

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "stratagus.h"
#include "video.h"
#include "font.h"
#include "tileset.h"
#include "map.h"
#include "sound_id.h"
#include "unitsound.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "missile.h"
#include "sound.h"
#include "ui.h"

#include "etlib/hash.h"

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

/**
**	Missile class names, used to load/save the missiles.
*/
global const char* MissileClassNames[] = {
    "missile-class-none",
    "missile-class-point-to-point",
    "missile-class-point-to-point-with-delay",
    "missile-class-stay-with-delay",
    "missile-class-point-to-point-3bounces",
    "missile-class-fireball",
    "missile-class-flame-shield",
    "missile-class-blizzard",
    "missile-class-death-decay",
    "missile-class-whirlwind",
    "missile-class-cycle-once",
    "missile-class-point-to-point-with-hit",
    "missile-class-fire",
    "missile-class-custom",
    "missile-class-hit",
    "missile-class-parabolic",
    NULL
};

/**
**	Missile type type definition
*/
global const char MissileTypeType[] = "missile-type";

/**
**	W*rCr*ft number to internal missile-type name.
*/
global char** MissileTypeWcNames;

/**
**	Defined missile-types.
*/
global MissileType* MissileTypes;

global int NumMissileTypes;		/// number of missile-types made.

/*
**	Next missile-types are used hardcoded in the source.
*/
    /// missile-type for the explosion missile
global MissileType* MissileTypeExplosion;

#ifdef DEBUG
global int NoWarningMissileType;		/// quiet ident lookup.
#endif

local Missile* GlobalMissiles[MAX_MISSILES];	/// all global missiles on map
local int NumGlobalMissiles;			/// currently used missiles

local Missile* LocalMissiles[MAX_MISSILES*8];	/// all local missiles on map
local int NumLocalMissiles;			/// currently used missiles

#ifdef DOXYGEN                          // no real code, only for document

    /// lookup table for missile names
local MissileType* MissileTypeHash[61];

#else

    /// lookup table for missile names
local hashtable(MissileType*, 61) MissileTypeHash;

#endif

global BurningBuildingFrame* BurningBuildingFrames;  /// Burning building frames

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Load the graphics for the missiles.
*/
global void LoadMissileSprites(void)
{
    int i;
    const char* file;

    for (i = 0; MissileTypes[i].OType; ++i) {
	if ((file = MissileTypes[i].File)) {
	    char* buf;

	    buf = alloca(strlen(file) + 9 + 1);
	    file = strcat(strcpy(buf, "graphics/"), file);
	    ShowLoadProgress("Missile %s", file);
	    MissileTypes[i].Sprite = LoadSprite(
		    file, MissileTypes[i].Width, MissileTypes[i].Height);

	    // Correct the number of frames in graphic
	    DebugCheck(MissileTypes[i].Sprite->NumFrames < MissileTypes[i].SpriteFrames);
	    MissileTypes[i].Sprite->NumFrames = MissileTypes[i].SpriteFrames;
	    // FIXME: Don't use NumFrames as number of frames.
	}
    }
}

/**
**	Get Missile type by identifier.
**
**	@param ident	Identifier.
**
**	@return		Missile type pointer.
*/
global MissileType* MissileTypeByIdent(const char* ident)
{
    MissileType* const* mtype;

    mtype = (MissileType**)hash_find(MissileTypeHash, (char*)ident);
    if (mtype) {
	return *mtype;
    }

#ifdef DEBUG
    if (!NoWarningMissileType) {
	DebugLevel0Fn("Missile %s not found\n" _C_ ident);
    }
#endif
    return NULL;
}

/**
**	Allocate an empty missile-type slot.
**
**	@todo	Don't allocate an array of missile-types, allocate an array
**		of pointers.
**
**	@param ident	Identifier to identify the slot (malloced by caller!).
**
**	@return		New allocated (zeroed) missile-type pointer.
*/
global MissileType* NewMissileTypeSlot(char* ident)
{
    MissileType* mtype;
    int i;

    //
    //	Allocate new memory. (+2 for start end empty last entry.)
    //
    mtype = calloc(NumMissileTypes + 2, sizeof(MissileType));
    if (!mtype) {
	fprintf(stderr, "Out of memory\n");
	ExitFatal(-1);
    }
    memcpy(mtype, MissileTypes, sizeof(MissileType) * NumMissileTypes);
    if (MissileTypes) {
	free(MissileTypes);
    }
    MissileTypes = mtype;
    mtype = MissileTypes + NumMissileTypes++;
    mtype->OType = MissileTypeType;
    mtype->Ident = ident;
    //
    //	Rehash.
    //
    for (i = 0; i < NumMissileTypes; ++i) {
	*(MissileType**)hash_add(MissileTypeHash,MissileTypes[i].Ident) = &MissileTypes[i];
    }

    mtype->CanHitOwner = 0;		// defaults
    mtype->FriendlyFire = 0;

    return mtype;
}

/**
**	Allocate memory for a new global missile.
*/
local Missile* NewGlobalMissile(void)
{
    Missile* missile;

    //	Check maximum missiles!
    if (NumGlobalMissiles == MAX_MISSILES) {
	fprintf(stderr, "Maximum of global missiles reached\n");
	abort();
	return NULL;
    }

    missile = calloc(1, sizeof(Missile));
    missile->MissileSlot = GlobalMissiles + NumGlobalMissiles;
    GlobalMissiles[NumGlobalMissiles++] = missile;

    return missile;
}

/**
**	Allocate memory for a new local missile.
*/
local Missile* NewLocalMissile(void)
{
    Missile* missile;

    //	Check maximum missiles!
    if (NumLocalMissiles == MAX_MISSILES * 8) {
	fprintf(stderr, "Maximum of local missiles reached\n");
	abort();
	return NULL;
    }

    missile = calloc(1, sizeof(Missile));
    missile->MissileSlot = LocalMissiles + NumLocalMissiles;
    LocalMissiles[NumLocalMissiles++] = missile;
    missile->Local = 1;

    return missile;
}

/**
**	Initialize a new made missle.
**
**	@param missile	Pointer to new uninitialized missile.
**	@param mtype	Type pointer of missile.
**	@param sx	Missile x start point in pixel.
**	@param sy	Missile y start point in pixel.
**	@param dx	Missile x destination point in pixel.
**	@param dy	Missile y destination point in pixel.
**
**	@return		created missile.
*/
local Missile* InitMissile(Missile* missile, MissileType* mtype, int sx,
    int sy, int dx, int dy)
{
    DebugCheck(!mtype);
    DebugCheck(!missile);
    missile->X = sx - mtype->Width / 2;
    missile->Y = sy - mtype->Height / 2;
    missile->DX = dx - mtype->Width / 2;
    missile->DY = dy - mtype->Height / 2;
    missile->SourceX = sx;
    missile->SourceY = sy;
    missile->Type = mtype;
    missile->SpriteFrame = 0;
    missile->State = 0;
    missile->Wait = mtype->Sleep;	// initial wait = sleep
    missile->Delay = mtype->StartDelay;	// initial delay

    missile->SourceUnit = NULL;

    missile->Damage = 0;
    missile->TargetUnit = NULL;
    missile->TTL = -1;
    missile->Controller = NULL;

    return missile;
}

/**
**	Create a new global missile at (x,y).
**
**	@param mtype	Type pointer of missile.
**	@param sx	Missile x start point in pixel.
**	@param sy	Missile y start point in pixel.
**	@param dx	Missile x destination point in pixel.
**	@param dy	Missile y destination point in pixel.
**
**	@return		created missile.
*/
global Missile* MakeMissile(MissileType* mtype,int sx,int sy,int dx,int dy)
{
    Missile* missile;

    DebugCheck(!mtype);
    DebugLevel3Fn("type %d(%s) at %d,%d to %d,%d\n" _C_
	mtype - MissileTypes _C_ mtype->Ident _C_ sx _C_ sy _C_ dx _C_ dy);


    if (!(missile = NewGlobalMissile())) {
	return missile;
    }

    return InitMissile(missile, mtype, sx, sy, dx, dy);
}

/**
**	Create a new local missile at (x,y).
**
**	@param mtype	Type pointer of missile.
**	@param sx	Missile x start point in pixel.
**	@param sy	Missile y start point in pixel.
**	@param dx	Missile x destination point in pixel.
**	@param dy	Missile y destination point in pixel.
**
**	@return		created missile.
*/
global Missile* MakeLocalMissile(MissileType* mtype, int sx, int sy, int dx, int dy)
{
    Missile* missile;

    DebugCheck(!mtype);
    DebugLevel3Fn("type %d(%s) at %d,%d to %d,%d\n" _C_
	mtype - MissileTypes _C_ mtype->Ident _C_ sx _C_ sy _C_ dx _C_ dy);

    if (!(missile = NewLocalMissile())) {
	return missile;
    }

    return InitMissile(missile, mtype, sx, sy, dx, dy);
}

/**
**	Free a missile.
**
**	@param missile	Missile pointer.
*/
local void FreeMissile(Missile* missile)
{
    Missile* temp;
    Unit* unit;

    //
    //	Release all unit references.
    //
    if ((unit = missile->SourceUnit)) {
	RefsDebugCheck(!unit->Refs);
	if (unit->Destroyed) {
	    if (!--unit->Refs) {
		ReleaseUnit(unit);
	    }
	} else {
	    --unit->Refs;
	    RefsDebugCheck(!unit->Refs);
	}
    }
    if((unit = missile->TargetUnit)) {
	RefsDebugCheck(!unit->Refs);
	if (unit->Destroyed) {
	    if (!--unit->Refs) {
		ReleaseUnit(unit);
	    }
	} else {
	    --unit->Refs;
	    RefsDebugCheck(!unit->Refs);
	}
    }

    //
    //	Free the missile memory
    //		Note: removing the last missile works.
    //
    if (missile->Local) {
	DebugCheck(*missile->MissileSlot != missile);
	temp=LocalMissiles[--NumLocalMissiles];
	DebugCheck(*temp->MissileSlot != temp);
	temp->MissileSlot = missile->MissileSlot;
	*missile->MissileSlot = temp;
	LocalMissiles[NumLocalMissiles] = NULL;
    } else {
	DebugCheck(*missile->MissileSlot != missile);
	temp = GlobalMissiles[--NumGlobalMissiles];
	DebugCheck(*temp->MissileSlot != temp);
	temp->MissileSlot = missile->MissileSlot;
	*missile->MissileSlot = temp;
	GlobalMissiles[NumGlobalMissiles] = NULL;
    }

    free(missile);
}

/**
**	Calculate damage.
**
**	Damage calculation:
**		(BasicDamage-Armor)+PiercingDamage
**	damage =----------------------------------
**				    2
**	damage is multiplied by random between 1 and 2.
**
**	@todo NOTE: different targets (big are hit by some missiles better)
**	@todo NOTE: lower damage for hidden targets.
**	@todo NOTE: lower damage for targets on higher ground.
**
**	@param attacker_stats	Attacker attributes.
**	@param goal_stats	Goal attributes.
**	@param bloodlust	If attacker has bloodlust
**	@param xp		Experience of attacker.
**
**	@return			damage inflicted to goal.
*/
local int CalculateDamageStats(const UnitStats* attacker_stats,
    const UnitStats* goal_stats, int bloodlust, int xp)
{
    int damage;
    int basic_damage;
    int piercing_damage;

    basic_damage = attacker_stats->BasicDamage + isqrt(xp / 100) * XpDamage;
    piercing_damage = attacker_stats->PiercingDamage;
    if (bloodlust) {
	basic_damage *= 2;
	piercing_damage *= 2;
	DebugLevel3Fn("bloodlust\n");
    }

#if 0
    damage = basic_damage - goal_stats->Armor;
    if (damage < 0) {
	// Use minimum damage
	if (piercing_damage < 30 && basic_damage < 30) {
	    damage = (piercing_damage + 1) / 2;
	} else {
	    damage = (piercing_damage + basic_damage - 30) / 2;
	}
    } else {
	damage += piercing_damage;
	damage -= SyncRand() % ((damage + 2) / 2);
    }
#else
    damage = max(basic_damage - goal_stats->Armor, 1) + piercing_damage;
    damage -= SyncRand() % ((damage + 2) / 2);
    DebugCheck(damage < 0);
#endif
	
    DebugLevel3Fn("\nDamage done [%d] %d %d ->%d\n" _C_ goal_stats->Armor _C_
	basic_damage _C_ piercing_damage _C_ damage);

    return damage;
}

/**
**	Calculate damage.
**
**	@param attacker_stats	Attacker attributes.
**	@param goal		Goal unit.
**	@param bloodlust	If attacker has bloodlust
**	@param xp		Experience of attack.
**
**	@return			damage produces on goal.
*/
local int CalculateDamage(const UnitStats* attacker_stats,
    const Unit* goal, int bloodlust, int xp)
{
    return CalculateDamageStats(attacker_stats, goal->Stats, bloodlust, xp);
}

/**
**	Fire missile.
**
**	@param unit	Unit that fires the missile.
*/
global void FireMissile(Unit* unit)
{
    int x;
    int y;
    int dx;
    int dy;
    Unit* goal;
    Missile* missile;

    //
    //	Goal dead?
    //
    goal = unit->Orders[0].Goal;
    if (goal) {

	// Better let the caller/action handle this.

	if (goal->Destroyed) {
	    DebugLevel0Fn("destroyed unit\n");
	    return;
	}
	if (goal->Removed) {
	    DebugLevel3Fn("Missile-none hits removed unit!\n");
	    return;
	}
	if (!goal->HP || goal->Orders[0].Action == UnitActionDie) {
	    DebugLevel3Fn("Missile hits dead unit!\n");
	    return;
	}

	// FIXME: Some missile hit the field of the target and all units on it.
	// FIXME: goal is already dead, but missile could hit others?
    }

    //
    //	No missile hits immediately!
    //
    if (((MissileType*)unit->Type->Missile.Missile)->Class == MissileClassNone) {
	// No goal, take target coordinates
	if (!goal) {
	    dx = unit->Orders[0].X;
	    dy = unit->Orders[0].Y;
	    if (WallOnMap(dx, dy)) {
		if (HumanWallOnMap(dx, dy)) {
		    HitWall(dx, dy,
			CalculateDamageStats(unit->Stats,
			    UnitTypeHumanWall->Stats, unit->Bloodlust, unit->XP));
		} else {
		    HitWall(dx, dy,
			CalculateDamageStats(unit->Stats,
			    UnitTypeOrcWall->Stats, unit->Bloodlust, unit->XP));
		}
		return;
	    }

	    DebugLevel1Fn("Missile-none hits no unit, shouldn't happen!\n");
	    return;
	}

	HitUnit(unit, goal,
	    CalculateDamage(unit->Stats, goal, unit->Bloodlust, unit->XP));

	return;
    }

    x = unit->X * TileSizeX + TileSizeX / 2;	// missile starts in tile middle
    y = unit->Y * TileSizeY + TileSizeY / 2;

    if (goal) {
	DebugCheck(!goal->Type);	// Target invalid?
	// Fire to nearest point of the unit!
	NearestOfUnit(goal, unit->X, unit->Y, &dx, &dy);
	DebugLevel3Fn("Fire to unit at %d,%d\n" _C_ dx _C_ dy);

	//
	//	Moved out of attack range?
	//
	if (MapDistanceBetweenUnits(unit, goal) < unit->Type->MinAttackRange) {
	    DebugLevel0Fn("Missile target too near %d,%d\n" _C_
		MapDistanceBetweenUnits(unit,goal) _C_ unit->Type->MinAttackRange);
	    // FIXME: do something other?
	    return;
	}

    } else {
	dx = unit->Orders[0].X;
	dy = unit->Orders[0].Y;
	// FIXME: Can this be too near??
    }

    // Fire to the tile center of the destination.
    dx = dx * TileSizeX + TileSizeX / 2;
    dy = dy * TileSizeY + TileSizeY / 2;
    missile = MakeMissile(unit->Type->Missile.Missile, x, y, dx, dy);
    //
    //	Damage of missile
    //
    if (goal) {
	missile->TargetUnit = goal;
	RefsDebugCheck(!goal->Refs || goal->Destroyed);
	goal->Refs++;
    }
    missile->SourceUnit = unit;
    RefsDebugCheck(!unit->Refs || unit->Destroyed);
    unit->Refs++;
}

/**
**      Get area of tiles covered by missile
**
**      @param missile  Missile to be checked and set.
**	@param sx	OUT: Pointer to X of top left corner in map tiles.
**	@param sy	OUT: Pointer to Y of top left corner in map tiles.
**	@param ex	OUT: Pointer to X of bottom right corner in map tiles.
**	@param ey	OUT: Pointer to Y of bottom right corner in map tiles.
**      @return         sx,sy,ex,ey defining area in Map
*/
local void GetMissileMapArea(const Missile* missile, int* sx, int* sy,
    int* ex, int* ey)
{
    *sx = missile->X / TileSizeX;
    *sy = missile->Y / TileSizeY;
    *ex = (missile->X + missile->Type->Width) / TileSizeX;
    *ey = (missile->Y + missile->Type->Height) / TileSizeY;
}

/**
**      Check missile visibility in a given viewport.
**
**	@param vp	Viewport to be checked.
**      @param missile  Missile pointer to check if visible.
**
**      @return         Returns true if visible, false otherwise.
*/
local int MissileVisibleInViewport(const Viewport* vp, const Missile* missile)
{
    int min_x;
    int max_x;
    int min_y;
    int max_y;

    GetMissileMapArea(missile, &min_x, &min_y, &max_x, &max_y);
    if (!AnyMapAreaVisibleInViewport(vp, min_x, min_y, max_x, max_y)) {
	return 0;
    }
    DebugLevel3Fn("Missile bounding box %d %d %d %d\n" _C_ min_x _C_ max_x _C_
	min_y _C_ max_y);

    if (!IsMapFieldVisible(ThisPlayer, (missile->X - TileSizeX / 2) / TileSizeX,
	    (missile->Y - TileSizeY / 2) / TileSizeY) && !ReplayRevealMap) {
	return 0;
    }

    return 1;
}

/**
**      Check and sets if missile must be drawn on screen-map
**
**      @param missile  Missile to be checked.
**      @return         True if map marked to be drawn, false otherwise.
*/
global int CheckMissileToBeDrawn(const Missile* missile)
{
    int sx;
    int sy;
    int ex;
    int ey;

    GetMissileMapArea(missile, &sx, &sy, &ex, &ey);
    return MarkDrawAreaMap(sx, sy, ex, ey);
}

/**
**	Draw missile.
**
**	@param mtype	Missile type
**	@param frame	Animation frame
**	@param x	Screen pixel X position
**	@param y	Screen pixel Y position
*/
global void DrawMissile(const MissileType* mtype, int frame, int x, int y)
{
    // FIXME: This is a hack for mirrored sprites
    if (frame < 0) {
	VideoDrawClipX(mtype->Sprite, -frame, x, y);
    } else {
	VideoDrawClip(mtype->Sprite, frame, x, y);
    }
}

/**
**	FIXME: docu
*/
local int MissileDrawLevelCompare(const void* v1, const void* v2)
{
    const Missile* c1;
    const Missile* c2;

    c1 = *(Missile**)v1;
    c2 = *(Missile**)v2;

    if (c1->Type->DrawLevel == c2->Type->DrawLevel) {
	return c1->MissileSlot < c2->MissileSlot ? -1 : 1;
    } else {
	return c1->Type->DrawLevel <= c2->Type->DrawLevel ? -1 : 1;
    }
}
/**
**	Draw all missiles on map.
**
**	@param vp	Viewport pointer.
**	@param table
*/
global int FindAndSortMissiles(const Viewport* vp, Missile** table)
{
    Missile* missile;
    Missile* const* missiles;
    Missile* const* missiles_end;
    int flag;
    int nmissiles;

    //
    //	Loop through global missiles, than through locals.
    //
    flag = 1;
    missiles = GlobalMissiles;
    missiles_end = missiles + NumGlobalMissiles;
    nmissiles = 0;
    do {
	for (; missiles < missiles_end; ++missiles) {
	    missile = *missiles;
	    if (missile->Type->Class == MissileClassCustom) {
		continue;	// custom are handled by Controller() only
	    }
	    if (missile->Delay) {
		continue;	// delayed aren't shown
	    }
	    // Draw only visible missiles
	    if (!flag || MissileVisibleInViewport(vp, missile)) {
	        table[nmissiles++] = missile;
	    }
	}
	missiles = LocalMissiles;
	missiles_end = missiles + NumLocalMissiles;
    } while (flag--);
    if (nmissiles) {
	qsort((void *)table, nmissiles, sizeof(Missile*), MissileDrawLevelCompare);
    }

    return nmissiles;
}

/**
**	Change missile heading from x,y.
**
**	@param missile	Missile pointer.
**	@param dx	Delta in x.
**	@param dy	Delta in y.
*/
local void MissileNewHeadingFromXY(Missile* missile, int dx, int dy)
{
    int dir;
    int nextdir;

    if (missile->SpriteFrame < 0) {
	missile->SpriteFrame = -missile->SpriteFrame;
    }
    missile->SpriteFrame /= missile->Type->NumDirections / 2 + 1;
    missile->SpriteFrame *= missile->Type->NumDirections / 2 + 1;

    nextdir = 256 / missile->Type->NumDirections;
    dir = ((DirectionToHeading(dx, dy) + nextdir / 2) & 0xFF) / nextdir;
    if (dir <= LookingS / nextdir ) {	// north->east->south
	missile->SpriteFrame += dir;
    } else {
	missile->SpriteFrame += 256 / nextdir - dir;
	missile->SpriteFrame = -missile->SpriteFrame;
    }
}

/**
**	Handle point to point missile.
**
**	@param missile	Missile pointer.
*/
local int PointToPointMissile(Missile* missile)
{
    int dx;
    int dy;
    int xstep;
    int ystep;
    int i;

    if (!(missile->State & 1)) {
	// initialize
	dy = missile->DY - missile->Y;
	ystep = 1;
	if (dy < 0) {
	    dy = -dy;
	    ystep = -1;
	}
	dx = missile->DX - missile->X;
	xstep = 1;
	if (dx < 0) {
	    dx = -dx;
	    xstep = -1;
	}

	// FIXME: could be better written
	if (missile->Type->Class == MissileClassWhirlwind ||
		missile->Type->Class == MissileClassFlameShield) {
	    // must not call MissileNewHeading nor frame change
	} else if (missile->Type->Class == MissileClassBlizzard) {
	    missile->SpriteFrame = 0;
	} else if (missile->Type->Class == MissileClassPointToPoint3Bounces) {
	    missile->DX -= xstep * TileSizeX / 2;
	    missile->DY -= ystep * TileSizeY / 2;
	} else {
	    MissileNewHeadingFromXY(missile, dx * xstep, dy * ystep);
	}

	if (dy == 0) {		// horizontal line
	    if (dx == 0) {
		return 1;
	    }
	} else if (dx == 0) {	// vertical line
	} else if (dx < dy) {	// step in vertical direction
	    missile->D = dy - 1;
	    dx += dx;
	    dy += dy;
	} else if (dx > dy) {	// step in horizontal direction
	    missile->D = dx - 1;
	    dx += dx;
	    dy += dy;
	}

	missile->Dx = dx;
	missile->Dy = dy;
	missile->Xstep = xstep;
	missile->Ystep = ystep;
	missile->State++;
	DebugLevel3Fn("Init: %d,%d, %d,%d, =%d\n" _C_ dx _C_ dy _C_
	    xstep _C_ ystep _C_ missile->D);
	return 0;
    } else {
	// on the way
	dx = missile->Dx;
	dy = missile->Dy;
	xstep = missile->Xstep;
	ystep = missile->Ystep;
    }

    //
    //	Move missile
    //
    if (dy == 0) {		// horizontal line
	for (i = 0; i < missile->Type->Speed; ++i) {
	    if (missile->X == missile->DX) {
		return 1;
	    }
	    missile->X += xstep;
	}
	return 0;
    }

    if (dx == 0) {		// vertical line
	for (i = 0; i < missile->Type->Speed; ++i) {
	    if (missile->Y == missile->DY) {
		return 1;
	    }
	    missile->Y += ystep;
	}
	return 0;
    }

    if (dx < dy) {		// step in vertical direction
	for (i = 0; i < missile->Type->Speed; ++i) {
	    if (missile->Y == missile->DY) {
		return 1;
	    }
	    missile->Y += ystep;
	    missile->D -= dx;
	    if (missile->D < 0) {
		missile->D += dy;
		missile->X += xstep;
	    }
	}
	return 0;
    }

    if (dx > dy) {		// step in horizontal direction
	for (i = 0; i < missile->Type->Speed; ++i) {
	    if (missile->X == missile->DX) {
		return 1;
	    }
	    missile->X += xstep;
	    missile->D -= dy;
	    if (missile->D < 0) {
		missile->D += dx;
		missile->Y += ystep;
	    }
	}
	return 0;
    }
				// diagonal line
    for (i = 0; i < missile->Type->Speed; ++i) {
	if (missile->Y == missile->DY) {
	    return 1;
	}
	missile->X += xstep;
	missile->Y += ystep;
    }
    return 0;
}

/**
**	Calculate parabolic trajectories.
**
**	@param missile	Missile pointer.
**	@param amplitude    How high can the missile go. This value depends
**			    on the missile direction and game perspective.
*/
local int ParabolicCalc(Missile* missile, int amplitude)
{
    int xmid;
    long sinu;
    int thetha;

    missile->Xl -= missile->Xstep;
    missile->X = missile->Xl / 100;

    xmid = (missile->SourceX + missile->DX) / 2;
    sinu = (missile->X - xmid) * (missile->X - xmid);
    thetha = missile->SourceX - xmid;
    missile->Y = ((missile->Angle * (missile->X - missile->SourceX)) -
	amplitude * isqrt(-sinu + thetha * thetha) + missile->SourceY * 100) / 100;

    return 0;
}

/**
**	Calculate parabolic trajectories.
**
**	@param missile	Missile pointer.
*/
local int ParabolicMissile(Missile* missile)
{
    int i;
    int sx;
    int sy;

    if (!(missile->State & 1)) {
	int dx;
	int dy;
	int xstep;
	int ystep;

	// initialize
	dy = missile->DY - missile->Y;
	ystep = 1;
	if (dy < 0) {
	    dy = -dy;
	    ystep = -1;
	}
	dx = missile->DX - missile->X;
	xstep = 1;
	if (dx < 0) {
	    dx = -dx;
	    xstep = -1;
	}
	if (missile->SourceX - missile->DX != 0) {
	    missile->Angle = (100 * (missile->SourceY - missile->DY)) / 
		(missile->SourceX - missile->DX);
	} else {
	    missile->Angle = 1;
	}
	missile->Xl = missile->X * 100;

	MissileNewHeadingFromXY(missile, dx * xstep, dy * ystep);

	if (dx == 0 && dy == 0) {
	    return 1;
	}

	missile->Dx = dx;
	missile->Dy = dy;
	dx = missile->SourceX - missile->DX;
	dy = missile->SourceY - missile->DY;
	missile->Xstep = (100 * dx) / isqrt(dx * dx + dy * dy);
	missile->Ystep = ystep;
	++missile->State;
	DebugLevel3Fn("Init: %d,%d\n" _C_ dx _C_ dy);
	return 0;
    }

    sx = missile->X;
    sy = missile->Y;

    //
    //	Move missile
    //
    if (missile->Dy == 0) {		// horizontal line
	for (i = 0; i<missile->Type->Speed; ++i) {
	    if (missile->X == missile->DX) {
		return 1;
	    }
	    ParabolicCalc(missile, 50);
	}
	MissileNewHeadingFromXY(missile, missile->X - sx, missile->Y - sy);
	return 0;
    }

    if (missile->Dx == 0) {		// vertical line
	for (i = 0; i < missile->Type->Speed; ++i) {
	    if (missile->Y == missile->DY) {
		return 1;
	    }					
	    missile->Y += missile->Ystep; //no parabolic missile there.
	}
	return 0;
    }

    for (i = 0; i < missile->Type->Speed; ++i) {
	if (abs(missile->X - missile->DX) <= 1 &&
		abs(missile->Y - missile->DY) <= 1) {
	    return 1;
	}
	ParabolicCalc(missile, 100);
	MissileNewHeadingFromXY(missile, missile->X - sx, missile->Y - sy);
    }
    return 0;
}

/**
**	Missile hits the goal.
**
**	@param missile	Missile hitting the goal.
**	@param goal	Goal of the missile.
**	@param splash	Splash damage divisor.
*/
local void MissileHitsGoal(const Missile* missile, Unit* goal, int splash)
{
    if (!missile->Type->CanHitOwner && goal == missile->SourceUnit) {
	return;				// blizzard cannot hit owner unit
    }

    if (goal->HP && goal->Orders[0].Action != UnitActionDie) {
	if (missile->Damage) {		// direct damage, spells mostly
	    HitUnit(missile->SourceUnit, goal, missile->Damage / splash);
	} else {
	    HitUnit(missile->SourceUnit, goal,
		CalculateDamage(missile->SourceUnit->Stats, goal,
		    missile->SourceUnit->Bloodlust, 0) / splash);
	}
    }
}

/**
**	Missile hits wall.
**
**	@param missile	Missile hitting the goal.
**	@param x	Wall X map tile position.
**	@param y	Wall Y map tile position.
**	@param splash	Splash damage divisor.
**
**	@todo	FIXME: Support for more races.
*/
local void MissileHitsWall(const Missile* missile, int x, int y, int splash)
{
    if (WallOnMap(x, y)) {
	DebugLevel3Fn("Missile on wall?\n");
	if (HumanWallOnMap(x, y)) {
	    if (missile->Damage) {	// direct damage, spells mostly
		HitWall(x, y, missile->Damage / splash);
	    } else {
		HitWall(x, y,
		    CalculateDamageStats(missile->SourceUnit->Stats, 
			UnitTypeHumanWall->Stats, 0, 0) / splash);
	    }
	} else {
	    if (missile->Damage) {	// direct damage, spells mostly
		HitWall(x, y, missile->Damage / splash);
	    } else {
		HitWall(x, y,
		    CalculateDamageStats(missile->SourceUnit->Stats,
			UnitTypeOrcWall->Stats, 0, 0) / splash);
	    }
	}
	return;
    }
}

/**
**	Work for missile hit.
**
**	@param missile	Missile reaching end-point.
*/
global void MissileHit(Missile* missile)
{
    Unit* goal;
    int x;
    int y;
    Unit* table[UnitMax];
    int n;
    int i;

    // FIXME: should I move the PlayMissileSound to here?

    if (missile->Type->ImpactSound.Sound) {
	PlayMissileSound(missile, missile->Type->ImpactSound.Sound);
    }

    x = missile->X + missile->Type->Width / 2;
    y = missile->Y + missile->Type->Height / 2;

    //
    //	The impact generates a new missile.
    //
    if (missile->Type->ImpactMissile) {
	Missile* mis;

	mis = MakeMissile(missile->Type->ImpactMissile, x, y, 0, 0);
	// Impact missiles didn't generate any damage now.
#if 0
	mis->Damage = missile->Damage; // direct damage, spells mostly
	mis->SourceUnit = missile->SourceUnit;
	// FIXME: should copy target also?
	if (mis->SourceUnit) {
	    // RefsDebugCheck( mis->SourceUnit->Destroyed );
	    if (mis->SourceUnit->Destroyed) {
		DebugLevel0Fn("Referencing a destroyed unit, I think it is good here\n");
	    }
	    RefsDebugCheck(!mis->SourceUnit->Refs);
	    mis->SourceUnit->Refs++;
	}
#endif
    }

    if (!missile->SourceUnit) {		// no owner - green-cross ...
	DebugLevel3Fn("Missile has no owner!\n");
	return;
    }

    x /= TileSizeX;
    y /= TileSizeY;

    if (x < 0 || y < 0 || x >= TheMap.Width || y >= TheMap.Height ) {
	// FIXME: this should handled by caller?
	DebugLevel0Fn("Missile gone outside of map!\n");
	return;				// outside the map.
    }

    //
    //	Choose correct goal.
    //
    if (!missile->Type->Range) {
	if (missile->TargetUnit) {
	    //
	    //	Missiles without range only hits the goal always.
	    //
	    goal = missile->TargetUnit;
	    if (goal->Destroyed) {			// Destroyed
		RefsDebugCheck(!goal->Refs);
		if (!--goal->Refs) {
		    ReleaseUnit(goal);
		}
		missile->TargetUnit = NoUnitP;
		return;
	    }
	    MissileHitsGoal(missile, goal, 1);
	    return;
	}
	MissileHitsWall(missile, x, y, 1);
	return;
    }

    //
    //	Hits all units in range.
    //
    i = missile->Type->Range;
    n = SelectUnits(x - i + 1, y - i + 1, x + i, y + i, table);
    for (i = 0; i < n; ++i) {
	goal=table[i];
	//
	//	Can the unit attack the this unit-type?
	//	NOTE: perhaps this should be come a property of the missile.
	//
	if (CanTarget(missile->SourceUnit->Type, goal->Type)) {
	    // We are attacking the nearest field of the unit
	    if (x < goal->X || y < goal->Y ||
		    x >= goal->X + goal->Type->TileWidth ||
		    y >= goal->Y + goal->Type->TileHeight ) {
		MissileHitsGoal(missile, goal, 2);
	    } else {
		MissileHitsGoal(missile, goal, 1);
	    }
	}
    }
    //
    //	Missile hits ground.
    //
    // FIXME: no bock writting it correct.
    x -= missile->Type->Range;
    y -= missile->Type->Range;
    for (i = missile->Type->Range * 2; --i;) {
	for (n = missile->Type->Range * 2; --n;) {
	    if (x + i >= 0 && x + i < TheMap.Width && y + n >= 0 && y + n < TheMap.Height) {
		if (i == 0 && n == 0) {
		    MissileHitsWall(missile, x + i, y + n, 1);
		} else {
		    MissileHitsWall(missile, x + i, y + n, 2);
		}
	    }
	}
    }
}

/**
**	Handle action for a missile.
**
**	@param missile	Missile pointer.
*/
local void MissileAction(Missile* missile)
{
    int neg;

    // Mark missile area on screen to be drawn, if missile moves or disappears.
    CheckMissileToBeDrawn(missile);

    switch (missile->Type->Class) {
	//
	//	Missile flies from x,y to x1,y1
	//
	case MissileClassPointToPoint:
	    missile->Wait = missile->Type->Sleep;
	    if (PointToPointMissile(missile)) {
		MissileHit(missile);
		FreeMissile(missile);
		missile = NULL;
	    } else {
		//
		//	Animate missile, cycle through frames
		//
		neg = 0;
		if (missile->SpriteFrame < 0) {
		    neg = 1;
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
		missile->SpriteFrame += 5;		// FIXME: frames per row
		if (missile->SpriteFrame >= VideoGraphicFrames(missile->Type->Sprite)) {
		    missile->SpriteFrame-=
			VideoGraphicFrames(missile->Type->Sprite);
		}
		if (neg) {
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
		DebugLevel3Fn("Frame %d of %d\n" _C_
		    missile->SpriteFrame _C_
		    VideoGraphicFrames(missile->Type->Sprite));
	    }
	    break;

	case MissileClassPointToPointWithDelay:
	    missile->Wait = missile->Type->Sleep;
	    if (PointToPointMissile(missile)) {
		MissileHit(missile);
		FreeMissile(missile);
		missile = NULL;
	    } else {
		int totalx;
		int dx;
		int f;
		int i;
		int j;

		neg = 0;
		if (missile->SpriteFrame < 0) {
		    neg = 1;
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
		totalx = abs(missile->DX - missile->SourceX);
		dx = abs(missile->X - missile->SourceX);
		f = VideoGraphicFrames(missile->Type->Sprite) / 5; // FIXME: frames per row
		f = 2 * f - 1;
		for (i = 1, j = 1; i <= f; ++i) {
		    if (dx * f / i < totalx) {
			if ((i - 1) * 2 < f) {
			    j = i - 1;
			} else {
			    j = f - i;
			}
			missile->SpriteFrame = missile->SpriteFrame % 5 +
			    j * 5; // FIXME: frames per row
			break;
		    }
		}
		if (neg) {
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
	    }
	    break;

	case MissileClassPointToPoint3Bounces:
	    missile->Wait = missile->Type->Sleep;
	    if (PointToPointMissile(missile)) {
		//
		//	3 Bounces.
		//
		switch (missile->State) {
		    case 1:
		    case 3:
		    case 5:
			missile->State += 2;
			missile->DX += missile->Xstep * TileSizeX * 3 / 2;
			missile->DY += missile->Ystep * TileSizeY * 3 / 2;
			MissileHit(missile);
			// FIXME: hits to left and right
			// FIXME: reduce damage effects on later impacts
			break;
		    default:
			FreeMissile(missile);
			missile = NULL;
			break;
		}
	    } else {
		//
		//	Animate missile, cycle through frames
		//
		neg = 0;
		if (missile->SpriteFrame < 0) {
		    neg = 1;
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
		missile->SpriteFrame += 5;		// FIXME: frames per row
		if (missile->SpriteFrame >= VideoGraphicFrames(missile->Type->Sprite)) {
		    missile->SpriteFrame-=
			VideoGraphicFrames(missile->Type->Sprite);
		}
		if (neg) {
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
		DebugLevel3Fn("Frame %d of %d\n" _C_
		    missile->SpriteFrame _C_
		    VideoGraphicFrames(missile->Type->Sprite));
	    }
	    break;

	case MissileClassPointToPointWithHit:
	    missile->Wait = missile->Type->Sleep;
	    if (PointToPointMissile(missile)) {
		//
		//	Animate hit
		//
		neg = 0;
		if (missile->SpriteFrame < 0) {
		    neg = 1;
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
		missile->SpriteFrame += 5;	// FIXME: frames per row
		if (missile->SpriteFrame >= VideoGraphicFrames(missile->Type->Sprite)) {
		    MissileHit(missile);
		    FreeMissile(missile);
		    missile = NULL;
		}
		if (neg && missile) {
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
	    }
	    break;

#if 0	// FIXME: is done by the mythic controller
	case MissileClassFlameShield:
	    neg = 0;
	    if (missile->SpriteFrame < 0) {
		neg = 1;
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    missile->Wait = missile->Type->Sleep;
	    if (++missile->SpriteFrame == VideoGraphicFrames(missile->Type->Sprite)) {
		missile->SpriteFrame = 0;
		if (PointToPointMissile(missile)) {
		    // Must set new goal.
		}
	    }
	    if (neg) {
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    break;
#endif

	case MissileClassBlizzard:
	    missile->Wait = missile->Type->Sleep;
	    if (PointToPointMissile(missile)) {
		//
		//	Animate hit
		//
		neg = 0;
		if (missile->SpriteFrame < 0) {
		    neg = 1;
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
		missile->SpriteFrame += 4;	// FIXME: frames per row
		if (missile->SpriteFrame >= VideoGraphicFrames(missile->Type->Sprite)) {
		    MissileHit(missile);
		    FreeMissile(missile);
		    missile=NULL;
		}
		if (neg && missile) {
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
	    }
	    break;

	case MissileClassDeathDecay:
	    //NOTE: vladi: this is exact copy of MissileClassStayWithDelay
	    // but with check for blizzard-type hit (friendly fire:))
	    neg = 0;
	    if (missile->SpriteFrame < 0) {
		neg = 1;
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    missile->Wait = missile->Type->Sleep;
	    if (++missile->SpriteFrame == VideoGraphicFrames(missile->Type->Sprite)) {
		MissileHit(missile);
		FreeMissile(missile);
		missile=NULL;
	    }
	    if (neg && missile) {
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    break;

	case MissileClassWhirlwind:
	    neg = 0;
	    if (missile->SpriteFrame < 0) {
		neg = 1;
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    missile->Wait = missile->Type->Sleep;
	    if (++missile->SpriteFrame == VideoGraphicFrames(missile->Type->Sprite)) {
		missile->SpriteFrame = 0;
		PointToPointMissile(missile);
	    }
	    if (neg) {
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    break;

	case MissileClassStayWithDelay:
	    neg = 0;
	    if (missile->SpriteFrame < 0) {
		neg = 1;
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    missile->Wait = missile->Type->Sleep;
	    if (++missile->SpriteFrame == VideoGraphicFrames(missile->Type->Sprite)) {
		MissileHit(missile);
		FreeMissile(missile);
		missile = NULL;
		// FIXME: should MissileHitAndFree();
	    }
	    if (neg && missile) {
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    break;

	case MissileClassCycleOnce:
	    neg = 0;
	    if (missile->SpriteFrame < 0) {
		neg = 1;
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    missile->Wait = missile->Type->Sleep;
	    switch (missile->State) {
		case 0:
		case 2:
		    ++missile->State;
		    break;
		case 1:
		    if (++missile->SpriteFrame == VideoGraphicFrames(missile->Type->Sprite)) {
			--missile->SpriteFrame;
			++missile->State;
		    }
		    break;
		case 3:
		    if (!missile->SpriteFrame--) {
			MissileHit(missile);
			FreeMissile(missile);
			missile = NULL;
		    }
		    break;
	    }
	    if (neg && missile) {
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    break;

	case MissileClassFire: {
	    Unit* unit;

	    unit = missile->SourceUnit;
	    if (unit->Destroyed || !unit->HP) {
		FreeMissile(missile);
		missile = NULL;
		break;
	    }
	    neg = 0;
	    if (missile->SpriteFrame < 0) {
		neg = 1;
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    missile->Wait = missile->Type->Sleep;
	    if (++missile->SpriteFrame ==VideoGraphicFrames(missile->Type->Sprite)) {
		int f;
		MissileType* fire;

		missile->SpriteFrame = 0;
		f = (100 * unit->HP) / unit->Stats->HitPoints;
		fire = MissileBurningBuilding(f);
		if (!fire) {
		    FreeMissile(missile);
		    missile = NULL;
		    unit->Burning = 0;
		} else {
		    if (missile->Type != fire) {
			missile->X += missile->Type->Width / 2;
			missile->Y += missile->Type->Height / 2;
			missile->Type = fire;
			missile->X -= missile->Type->Width / 2;
			missile->Y -= missile->Type->Height / 2;
		    }
		}
	    }
	    if (neg && missile) {
		missile->SpriteFrame = -missile->SpriteFrame;
	    }
	    break;

	case MissileClassHit:
	    missile->Wait = missile->Type->Sleep;
	    if (PointToPointMissile(missile)) {
		MissileHit(missile);
		FreeMissile(missile);
		missile = NULL;
	    }
	    break;
	}

	case MissileClassParabolic:
	    missile->Wait = missile->Type->Sleep;
	    if (ParabolicMissile(missile)) {
		MissileHit(missile);
		FreeMissile(missile);
		missile = NULL;
	    } else {
		int totalx;
		int dx;
		int f;
		int i;
		int j;

		neg = 0;
		if (missile->SpriteFrame < 0) {
		    neg = 1;
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
		totalx = abs(missile->DX - missile->SourceX);
		dx = abs(missile->X - missile->SourceX);
		f = VideoGraphicFrames(missile->Type->Sprite) / 5; // FIXME: frames per row
		f = 2 * f - 1;
		for (i = 1, j = 1; i <= f; ++i) {
		    if (dx * f / i < totalx) {
			if ((i - 1) * 2 < f) {
			    j = i - 1;
			} else {
			    j = f - i;
			}
			missile->SpriteFrame = missile->SpriteFrame % 5 +
			    j * 5; // FIXME: frames per row
			break;
		    }
		}
		if (neg) {
		    missile->SpriteFrame = -missile->SpriteFrame;
		}
	    }
	    break;
    }

    if (missile) {			// check after movement
	CheckMissileToBeDrawn(missile);
    }
}

/**
**	Handle all missile actions of global/local missiles.
**
**	@param missiles		Table of missiles.
*/
local void MissilesActionLoop(Missile** missiles)
{
    Missile* missile;

    //
    //	NOTE: missiles[??] could be modified!!!
    //
    while ((missile = *missiles)) {
	if (missile->Delay && missile->Delay--) {
	    ++missiles;
	    continue;		// delay start of missile
	}

	if (missile->TTL > 0) {
	    missile->TTL--;	// overall time to live if specified
	}

	if (missile->Controller) {
	    missile->Controller(missile);
	    if (*missiles != missile) {	// Missile is destroyed
		continue;
	    }
	}

	if (!missile->TTL) {
	    FreeMissile(missile);
	    continue;
	}

	if (--missile->Wait) {	// wait until time is over
	    ++missiles;
	    continue;
	}

	if (missile->Type->Class == MissileClassCustom) {
	    missile->Wait = missile->Type->Sleep;
	    ++missiles;
	    continue;	// custom missiles are handled by Controller() only
	}

	MissileAction(missile);

	if (*missiles == missile) {	// Missile not destoryed
	    ++missiles;
	}
    }
}

/**
**	Handle all missile actions.
*/
global void MissileActions(void)
{
    MissilesActionLoop(GlobalMissiles);
    MissilesActionLoop(LocalMissiles);
}

/**
**	Calculate distance from view-point to missle.
**
**	@param missile	Missile pointer for distance.
*/
global int ViewPointDistanceToMissile(const Missile* missile)
{
    int x;
    int y;

    x = (missile->X+missile->Type->Width / 2) / TileSizeX;
    y = (missile->Y+missile->Type->Height / 2) / TileSizeY;	// pixel -> tile

    DebugLevel3Fn("Missile %p at %d %d\n" _C_ missile _C_ x _C_ y);

    return ViewPointDistance(x, y);
}

/**
**	Get the burning building missile based on hp percent.
**
**	@param percent	HP percent
*/
global MissileType* MissileBurningBuilding(int percent)
{
    BurningBuildingFrame* frame;
    BurningBuildingFrame* tmp;

    frame = tmp = BurningBuildingFrames;
    while (tmp) {
	if (percent < tmp->Percent) {
	    break;
	}
	frame = tmp;
	tmp = tmp->Next;
    }
    return frame->Missile;
}

/**
**	Save the missile-types to file.
**
**	@param file	Output file.
**
**	@todo FIXME: CanHitOwner and FriendlyFire not supported!
*/
global void SaveMissileTypes(CLFile* file)
{
    MissileType* mtype;
    char** sp;
    int i;

    CLprintf(file,"\n;;; -----------------------------------------\n");
    CLprintf(file,";;; MODULE: missile-types $Id$\n\n");

    //
    //	Original number to internal missile-type name.
    //
    i = CLprintf(file, "(define-missiletype-wc-names");
    for (sp = MissileTypeWcNames; *sp; ++sp) {
	if (i + strlen(*sp) > 79) {
	    i = CLprintf(file, "\n ");
	}
	i += CLprintf(file, " '%s", *sp);
    }
    CLprintf(file, ")\n\n");

    //
    //	Missile types
    //
    for (mtype = MissileTypes; mtype < &MissileTypes[NumMissileTypes]; ++mtype) {
	CLprintf(file, "(define-missile-type '%s\n ", mtype->Ident);
	if (mtype->File) {
	    CLprintf(file, " 'file \"%s\"", mtype->File);
	}
	CLprintf(file, " 'size '(%d %d)", mtype->Width, mtype->Height);
	if (mtype->Sprite) {
	    CLprintf(file," 'frames %d", mtype->SpriteFrames);
	}
	CLprintf(file, "\n  'num-directions %d", mtype->NumDirections);
	CLprintf(file, "\n ");
	if (mtype->FiredSound.Name) {
	    CLprintf(file, " 'fired-sound \"%s\"", mtype->FiredSound.Name);
	}
	if (mtype->ImpactSound.Name) {
	    CLprintf(file, " 'impact-sound \"%s\"", mtype->ImpactSound.Name);
	}
	if (mtype->FiredSound.Name || mtype->ImpactSound.Name) {
	    CLprintf(file, "\n ");
	}
	CLprintf(file, " 'class '%s", MissileClassNames[mtype->Class]);
	CLprintf(file, " 'draw-level %d ", mtype->DrawLevel);
	if (mtype->StartDelay) {
	    CLprintf(file, " 'delay %d", mtype->StartDelay);
	}
	CLprintf(file, " 'sleep %d", mtype->Sleep);
	CLprintf(file, " 'speed %d", mtype->Speed);
	CLprintf(file, " 'range %d", mtype->Range);
	if (mtype->ImpactMissile) {
	    CLprintf(file, "\n  'impact-missile '%s", mtype->ImpactMissile->Ident);
	}
	CLprintf(file, "\n ");
	CLprintf(file, " 'can-hit-owner #%c", mtype->CanHitOwner ? 't' : 'f');
	CLprintf(file, " 'friendly-fire #%c", mtype->FriendlyFire ? 't' : 'f');
	CLprintf(file, ")\n");
    }
}

/**
**	Save the state of a missile to file.
*/
local void SaveMissile(const Missile* missile,CLFile* file)
{
    char* s1;

    CLprintf(file, "(missile 'type '%s",missile->Type->Ident);
    CLprintf(file, " 'pos '(%d %d) 'goal '(%d %d)",
	missile->X, missile->Y, missile->DX, missile->DY);
    CLprintf(file," '%s", missile->Local ? "local" : "global");
    CLprintf(file,"\n  'frame %d 'state %d 'wait %d 'delay %d\n ",
	missile->SpriteFrame, missile->State, missile->Wait, missile->Delay);
    if (missile->SourceUnit) {
	CLprintf(file, " 'source '%s", s1 = UnitReference(missile->SourceUnit));
	free(s1);
    }
    if (missile->TargetUnit) {
	CLprintf(file, " 'target '%s", s1 = UnitReference(missile->TargetUnit));
	free(s1);
    }
    CLprintf(file, " 'damage %d", missile->Damage);
    // FIXME: need symbolic names for controller
    CLprintf(file, " 'ttl %d 'controller %ld",
	missile->TTL, (long)missile->Controller);
    CLprintf(file, " 'data '(%d %d %d %d %d)",
	missile->D, missile->Dx, missile->Dy, missile->Xstep, missile->Ystep);
    CLprintf(file, ")\n");
}

/**
**	Save the state missiles to file.
**
**	@param file	Output file.
*/
global void SaveMissiles(CLFile* file)
{
    Missile* const* missiles;

    CLprintf(file,"\n;;; -----------------------------------------\n");
    CLprintf(file,";;; MODULE: missiles $Id$\n\n");

    for (missiles = GlobalMissiles; *missiles; ++missiles) {
	SaveMissile(*missiles, file);
    }
    for (missiles = LocalMissiles; *missiles; ++missiles) {
	SaveMissile(*missiles, file);
    }
}

/**
**	Initialize missile-types.
*/
global void InitMissileTypes(void)
{
    MissileType* mtype;

    for (mtype = MissileTypes; mtype->OType; ++mtype) {
	//
	//	Add missile names to hash table
	//
	*(MissileType**)hash_add(MissileTypeHash, mtype->Ident) = mtype;

	//
	//	Resolve impact missiles and sounds.
	//
	if (mtype->FiredSound.Name) {
	    mtype->FiredSound.Sound = SoundIdForName(mtype->FiredSound.Name);
	}
	if (mtype->ImpactSound.Name) {
	    mtype->ImpactSound.Sound = SoundIdForName(mtype->ImpactSound.Name);
	}
	if (mtype->ImpactName) {
	    mtype->ImpactMissile = MissileTypeByIdent(mtype->ImpactName);
	}
    }

    MissileTypeExplosion = MissileTypeByIdent("missile-explosion");
}

/**
**	Clean up missile-types.
*/
global void CleanMissileTypes(void)
{
    MissileType* mtype;

    for (mtype = MissileTypes; mtype->OType; ++mtype) {
	hash_del(MissileTypeHash, mtype->Ident);

	free(mtype->Ident);
	free(mtype->File);
	free(mtype->FiredSound.Name);
	free(mtype->ImpactSound.Name);
	free(mtype->ImpactName);

	VideoSaveFree(mtype->Sprite);
    }
    free(MissileTypes);
    MissileTypes = NULL;
    NumMissileTypes = 0;
}

/**
**	Initialize missiles.
*/
global void InitMissiles(void)
{
}

/**
**	Clean up missiles.
*/
global void CleanMissiles(void)
{
    Missile** missiles;
    Missile* missile;
    char** sp;

    for (missiles = GlobalMissiles; (missile = *missiles); ++missiles) {
	free(missile);
	*missiles = NULL;
    }
    NumGlobalMissiles = 0;
    for (missiles = LocalMissiles; (missile = *missiles); ++missiles) {
	free(missile);
	*missiles = NULL;
    }
    NumLocalMissiles = 0;

    if (MissileTypeWcNames) {
	for (sp = MissileTypeWcNames; *sp; ++sp) {
	    free(*sp);
	}
	free(MissileTypeWcNames);
	MissileTypeWcNames = NULL;
    }
}

//@}
