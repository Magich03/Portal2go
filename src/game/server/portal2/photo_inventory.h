//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds whatever single object weapon_camera has captured until
//			weapon_placement puts it back into the world. Split out from the
//			weapons themselves so the two can hand a capture off to each
//			other, matching how the F-Stop prototype separated capture from
//			placement into two distinct tools.
//
//=============================================================================//
#ifndef PHOTO_INVENTORY_H
#define PHOTO_INVENTORY_H
#ifdef _WIN32
#pragma once
#endif

class CBaseAnimating;

class CPhotoInventory
{
public:
	CPhotoInventory();

	bool			HasPhoto( void ) const { return m_hCapturedEntity.Get() != NULL; }
	CBaseAnimating*	GetCapturedEntity( void ) const { return m_hCapturedEntity.Get(); }

	// Pulls pEntity out of normal play (disables its physics/collision, makes
	// it translucent) and stores it. Fails if we're already holding something.
	bool			CapturePhoto( CBaseAnimating *pEntity );

	// Restores the held object at the given transform, applying the current
	// scale level, and fires its OnCameraRelease output.
	void			PlacePhoto( const Vector &vecOrigin, const QAngle &angOrigin );

	// Safety unwind: restores the held object roughly where it is right now,
	// without treating it as a real placement (no OnCameraRelease). Used when
	// something interrupts the normal capture/place flow (weapon removed, etc).
	void			ReleaseWithoutPlacing( void );

	int				GetScaleLevel( void ) const { return m_nScaleLevel; }
	void			ScaleUp( void );
	void			ScaleDown( void );
	float			GetScaleMultiplier( void ) const;

private:
	CHandle<CBaseAnimating>	m_hCapturedEntity;
	int							m_nScaleLevel;
};

#endif // PHOTO_INVENTORY_H
