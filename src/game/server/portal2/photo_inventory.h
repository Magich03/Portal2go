//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds up to 3 captures at once for weapon_camera/weapon_placement
//			(matching the original F-Stop/Exposure decompile's 3-render-target
//			photo pool - see portal_render_targets.h's NUM_LARGE_PHOTO_SLOTS).
//			Split out from the weapons themselves so the two can hand
//			captures off to each other, matching how the F-Stop prototype
//			separated capture from placement into two distinct tools.
//
//			Every capturable prop (aside from prop_swap, which is handled
//			entirely separately in weapon_camera.cpp) goes through the same
//			polaroid capture flow: the real object is stashed completely out
//			of the world (EF_NODRAW, non-solid) and pushed onto a stack of up
//			to MAX_HELD_PHOTOS. photo_inventory.cpp doesn't network anything
//			itself - see CPortal_Player::m_PortalLocal.m_nPhotoStackCount,
//			kept in sync every PreThink(), for the HUD's 3-slot display.
//			weapon_placement always works on the most recently captured
//			(top-of-stack) photo: its first click (SpawnGhost) brings that
//			one object back as a translucent, scalable ghost; placing it
//			pops the stack, and the next-most-recent photo (if any) becomes
//			current. A prop's OnCameraCaptured()/OnCameraPlaced() hooks (see
//			baseanimating.h - prop_air_vent uses these to disable/resize its
//			push trigger) fire the same way regardless of how long it sits in
//			the stack first.
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
	PHOTOMODE_POLAROID = 0,	// real object stashed; player holds a 2D polaroid, no ghost yet
	PHOTOMODE_GHOST,		// real object visible again as a translucent, scalable ghost
};

#define MAX_HELD_PHOTOS 3

class CPhotoInventory
{
public:
	CPhotoInventory() {}

	bool			HasPhoto( void ) const { return m_Stack.Count() > 0; }
	bool			IsPolaroidPending( void ) const;	// top-of-stack photo hasn't had its ghost spawned yet
	bool			IsGhostActive( void ) const;		// top-of-stack photo is out as a translucent ghost
	bool			IsStackFull( void ) const { return m_Stack.Count() >= MAX_HELD_PHOTOS; }
	int				GetStackCount( void ) const { return m_Stack.Count(); }

	// The object ComputePlacementTransform()/UpdateHeldPreview()/scaling all
	// act on right now: the top-of-stack photo, if any.
	CBaseAnimating*	GetCapturedEntity( void ) const;

	// Stashes pEntity completely (EF_NODRAW, non-solid) and pushes it onto
	// the stack along with pPolaroid (the 2D item the player is now
	// "holding" for it). Fails if the stack is already full.
	bool			CapturePolaroid( CBaseAnimating *pEntity, CItem_Photograph *pPolaroid );

	// First weapon_placement click for the top-of-stack photo: un-stashes it
	// at the given transform as a translucent, scalable ghost and removes
	// the now-unneeded polaroid. No-op if the top of the stack isn't pending.
	void			SpawnGhost( const Vector &vecOrigin, const QAngle &angOrigin );

	// Restores the top-of-stack ghost at the given transform, applying its
	// scale level, fires its OnCameraRelease output, and pops it so the next
	// one down (if any) becomes current.
	void			PlacePhoto( const Vector &vecOrigin, const QAngle &angOrigin );

	// Safety unwind: restores every stacked photo (placed or still pending)
	// roughly where it is right now, without treating any of it as a real
	// placement (no OnCameraRelease). Used when something interrupts the
	// normal flow (weapon removed, etc).
	void			ReleaseWithoutPlacing( void );

	// Scale level of the top-of-stack photo - each stacked photo remembers
	// its own level.
	int				GetScaleLevel( void ) const;
	void			ScaleUp( void );
	void			ScaleDown( void );
	float			GetScaleMultiplier( void ) const;

private:
	struct StackedPhoto_t
	{
		CHandle<CBaseAnimating>		hEntity;
		CHandle<CItem_Photograph>	hPolaroid;		// valid only while mode == PHOTOMODE_POLAROID
		PhotoCaptureMode_t			mode;
		int							nScaleLevel;
	};

	CUtlVector<StackedPhoto_t>	m_Stack;	// back() = most recently captured = current
};

#endif // PHOTO_INVENTORY_H
