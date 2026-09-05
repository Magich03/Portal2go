//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: F-Stop camera - captures a nearby capturable prop into the
//			player's photo inventory. See weapon_placement.cpp for the other
//			half of the loop (preview/scale/place).
//
//=============================================================================//
#ifndef WEAPON_CAMERA_H
#define WEAPON_CAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_portalbasecombatweapon.h"

class CBaseAnimating;

//-----------------------------------------------------------------------------
// Camera / F-Stop capture weapon
//-----------------------------------------------------------------------------
class CWeaponCamera : public CBasePortalCombatWeapon
{
	DECLARE_CLASS( CWeaponCamera, CBasePortalCombatWeapon );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponCamera();

	virtual void	Precache();
	virtual void	PrimaryAttack();

private:
	CBaseAnimating*	FindCapturableEntity( void );
};

#endif // WEAPON_CAMERA_H
