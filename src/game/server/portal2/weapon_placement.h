//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: F-Stop placement - previews the player's current photo (from
//			weapon_camera) at their aim point, lets them scale it, and
//			commits the placement.
//
//=============================================================================//
#ifndef WEAPON_PLACEMENT_H
#define WEAPON_PLACEMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_portalbasecombatweapon.h"

//-----------------------------------------------------------------------------
// Placement / F-Stop placement weapon
//-----------------------------------------------------------------------------
class CWeaponPlacement : public CBasePortalCombatWeapon
{
	DECLARE_CLASS( CWeaponPlacement, CBasePortalCombatWeapon );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponPlacement();

	virtual void	Precache();
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	virtual void	ItemPostFrame();
	virtual void	UpdateOnRemove();

private:
	bool	ComputePlacementTransform( Vector *pOutOrigin, QAngle *pOutAngles );
	void	UpdateHeldPreview( void );

	bool	m_bLastPreviewValid;
};

#endif // WEAPON_PLACEMENT_H
