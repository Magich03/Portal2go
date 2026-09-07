//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds up to 3 captures at once for weapon_camera/weapon_placement
//			(matching the original F-Stop/Exposure decompile's 3-render-target
//			photo pool - see portal_render_targets.h's NUM_LARGE_PHOTO_SLOTS).
//			Split out from the weapons themselves so the two can hand
//			captures off to each other, matching how the F-Stop prototype
//			separated capture from placement into two distinct tools.
//
//			Two different capture flows exist:
//
//			- Direct capture (CaptureDirect): for props that override
//			  CBaseAnimating::UsesDirectCapture() (prop_air_vent and similar
//			  gameplay props). The real object goes translucent/non-solid and
//			  starts tracking the player's aim immediately. Entirely separate
//			  from the photo stack below - only one direct capture can be
//			  held at a time, independent of how many photos are stacked.
//
//			- Polaroid capture (CapturePolaroid): for every other capturable
//			  prop. The real object is stashed completely out of the world
//			  (EF_NODRAW, non-solid) and pushed onto a stack of up to
//			  MAX_PHOTOS (photo_inventory.cpp doesn't network anything
//			  itself - see CPortal_Player::m_PortalLocal.m_nPhotoStackCount,
//			  kept in sync every PreThink(), for the HUD's 3-slot display).
//			  weapon_placement always works on the most recently captured
//			  (top-of-stack) photo: its first click (SpawnGhost) brings that
//			  one object back as a translucent, scalable ghost; placing it
//			  pops the stack, and the next-most-recent photo (if any) becomes
//			  current.
//
//=============================================================================//
#ifndef PHOTO_INVENTORY_H
#define PHOTO_INVENTORY_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

class CBaseAnimating;
class CItem_Photograph;

enum PhotoCaptureMode_t
{
	PHOTOMODE_NONE = 0,
	PHOTOMODE_DIRECT,	// carrying+previewing the real object directly
	PHOTOMODE_POLAROID,	// real object stashed; player holds a 2D polaroid, no ghost yet
	PHOTOMODE_GHOST,	// real object visible again as a translucent, scalable ghost
};

#define MAX_HELD_PHOTOS 3

class CPhotoInventory
{
public:
	CPhotoInventory();

	// True if there's a direct capture out, or at least one photo stacked.
	bool			HasPhoto( void ) const;
	bool			IsPolaroidPending( void ) const;	// top-of-stack photo hasn't had its ghost spawned yet
	bool			IsGhostActive( void ) const;		// something (direct or top-of-stack) is out as a translucent ghost
	bool			IsStackFull( void ) const { return m_Stack.Count() >= MAX_HELD_PHOTOS; }
	int				GetStackCount( void ) const { return m_Stack.Count(); }

	// The object ComputePlacementTransform()/UpdateHeldPreview()/scaling all
	// act on right now: the direct capture if there is one, else the
	// top-of-stack photo.
	CBaseAnimating*	GetCapturedEntity( void ) const;

	// UsesDirectCapture() props: pulls pEntity out of normal play (disables
	// its physics/collision, makes it translucent) and starts previewing it
	// immediately. Fails if we're already directly carrying something.
	bool			CaptureDirect( CBaseAnimating *pEntity );

	// Everything else: stashes pEntity completely (EF_NODRAW, non-solid) and
	// pushes it onto the stack along with pPolaroid (the 2D item the player
	// is now "holding" for it). Fails if the stack is already full.
	bool			CapturePolaroid( CBaseAnimating *pEntity, CItem_Photograph *pPolaroid );

	// First weapon_placement click for the top-of-stack photo: un-stashes it
	// at the given transform as a translucent, scalable ghost (the same
	// state a direct capture starts in) and removes the now-unneeded
	// polaroid. No-op if the top of the stack isn't pending.
	void			SpawnGhost( const Vector &vecOrigin, const QAngle &angOrigin );

	// Restores whatever's currently active (direct capture or top-of-stack
	// ghost) at the given transform, applying its scale level, and fires its
	// OnCameraRelease output. On success, if this was a stacked photo, pops
	// it so the next one down becomes current.
	void			PlacePhoto( const Vector &vecOrigin, const QAngle &angOrigin );

	// Safety unwind: restores the direct capture (if any) and every stacked
	// photo (placed or still pending) roughly where they are right now,
	// without treating any of it as a real placement (no OnCameraRelease).
	// Used when something interrupts the normal flow (weapon removed, etc).
	void			ReleaseWithoutPlacing( void );

	// Scale level of whichever object is currently active (direct or
	// top-of-stack) - each stacked photo remembers its own level.
	int				GetScaleLevel( void ) const;
	void			ScaleUp( void );
	void			ScaleDown( void );
	float			GetScaleMultiplier( void ) const;

private:
	struct StackedPhoto_t
	{
		CHandle<CBaseAnimating>		hEntity;
		CHandle<CItem_Photograph>	hPolaroid;		// valid only while mode == PHOTOMODE_POLAROID
		PhotoCaptureMode_t			mode;			// PHOTOMODE_POLAROID or PHOTOMODE_GHOST
		int							nScaleLevel;
	};

	void			RestoreEntity( CBaseAnimating *pEntity, bool bFullyRestoreVisuals );

	CUtlVector<StackedPhoto_t>	m_Stack;	// back() = most recently captured = current

	CHandle<CBaseAnimating>		m_hDirectEntity;
	int							m_nDirectScaleLevel;
};

#endif // PHOTO_INVENTORY_H
