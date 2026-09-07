//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds whatever single object weapon_camera has captured until
//			weapon_placement puts it back into the world. Split out from the
//			weapons themselves so the two can hand a capture off to each
//			other, matching how the F-Stop prototype separated capture from
//			placement into two distinct tools.
//
//			Two different capture flows share this one piece of state:
//
//			- Direct capture (CaptureDirect): for props that override
//			  CBaseAnimating::UsesDirectCapture() (prop_air_vent and similar
//			  gameplay props). The real object goes translucent/non-solid and
//			  starts tracking the player's aim immediately, exactly like
//			  before the polaroid step below existed.
//
//			- Polaroid capture (CapturePolaroid): for every other capturable
//			  prop. The real object is stashed completely out of the world
//			  (EF_NODRAW, non-solid) and the player is left holding nothing
//			  but a 2D polaroid (see item_photograph.h/weapon_camera.cpp) -
//			  no 3D object exists anywhere yet. The first weapon_placement
//			  click (SpawnGhost) brings the real object back as a translucent,
//			  scalable ghost and discards the polaroid; from that point on a
//			  polaroid capture behaves exactly like a direct one.
//
//=============================================================================//
#ifndef PHOTO_INVENTORY_H
#define PHOTO_INVENTORY_H
#ifdef _WIN32
#pragma once
#endif

class CBaseAnimating;
class CItem_Photograph;

enum PhotoCaptureMode_t
{
	PHOTOMODE_NONE = 0,
	PHOTOMODE_DIRECT,	// carrying+previewing the real object directly
	PHOTOMODE_POLAROID,	// real object stashed; player holds a 2D polaroid, no ghost yet
	PHOTOMODE_GHOST,	// real object visible again as a translucent, scalable ghost
};

class CPhotoInventory
{
public:
	CPhotoInventory();

	bool			HasPhoto( void ) const { return m_nMode != PHOTOMODE_NONE; }
	bool			IsPolaroidPending( void ) const { return m_nMode == PHOTOMODE_POLAROID; }
	bool			IsGhostActive( void ) const { return m_nMode == PHOTOMODE_DIRECT || m_nMode == PHOTOMODE_GHOST; }
	CBaseAnimating*	GetCapturedEntity( void ) const { return m_hCapturedEntity.Get(); }

	// UsesDirectCapture() props: pulls pEntity out of normal play (disables
	// its physics/collision, makes it translucent) and starts previewing it
	// immediately. Fails if we're already holding something.
	bool			CaptureDirect( CBaseAnimating *pEntity );

	// Everything else: stashes pEntity completely (EF_NODRAW, non-solid) and
	// remembers pPolaroid (the 2D item the player is now "holding") so
	// SpawnGhost()/ReleaseWithoutPlacing() can clean it up later.
	bool			CapturePolaroid( CBaseAnimating *pEntity, CItem_Photograph *pPolaroid );

	// First weapon_placement click while IsPolaroidPending(): un-stashes the
	// captured object at the given transform as a translucent, scalable
	// ghost (exactly the same state a direct capture starts in) and removes
	// the now-unneeded polaroid. No-op if we're not in polaroid mode.
	void			SpawnGhost( const Vector &vecOrigin, const QAngle &angOrigin );

	// Restores the held object at the given transform, applying the current
	// scale level, and fires its OnCameraRelease output.
	void			PlacePhoto( const Vector &vecOrigin, const QAngle &angOrigin );

	// Safety unwind: restores the held object roughly where it is right now
	// (or where it was stashed, if still in polaroid mode), without treating
	// it as a real placement (no OnCameraRelease). Also cleans up an
	// abandoned polaroid. Used when something interrupts the normal
	// capture/place flow (weapon removed, etc).
	void			ReleaseWithoutPlacing( void );

	int				GetScaleLevel( void ) const { return m_nScaleLevel; }
	void			ScaleUp( void );
	void			ScaleDown( void );
	float			GetScaleMultiplier( void ) const;

private:
	CHandle<CBaseAnimating>		m_hCapturedEntity;
	CHandle<CItem_Photograph>	m_hPolaroid;
	PhotoCaptureMode_t			m_nMode;
	int							m_nScaleLevel;
};

#endif // PHOTO_INVENTORY_H
