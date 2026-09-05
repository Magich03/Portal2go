//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: F-Stop camera - captures a nearby capturable prop, carries it as a
//			translucent preview that follows the player's aim, and places it
//			back into the world (optionally rescaled) on a second trigger pull.
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
	DECLARE_DATADESC();
	DECLARE_CLASS( CWeaponCamera, CBasePortalCombatWeapon );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponCamera();

	virtual void	Precache();
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	virtual void	ItemPostFrame();
	virtual void	UpdateOnRemove();
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	bool			IsHoldingObject( void ) const { return m_hCapturedEntity.Get() != NULL; }

private:
	CBaseAnimating*	FindCapturableEntity( void );
	void			CaptureEntity( CBaseAnimating *pEntity );
	void			UpdateHeldPreview( void );
	bool			ComputePlacementTransform( Vector *pOutOrigin, QAngle *pOutAngles );
	void			CommitPlacement( void );
	void			ReleaseWithoutPlacing( void );
	float			GetScaleMultiplier( void ) const;

	CHandle<CBaseAnimating>	m_hCapturedEntity;
	int							m_nScaleLevel;
	bool						m_bLastPreviewValid;
};

#endif // WEAPON_CAMERA_H
