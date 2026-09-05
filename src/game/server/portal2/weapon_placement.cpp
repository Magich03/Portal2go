//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: F-Stop placement - previews the player's current photo (from
//			weapon_camera) at their aim point, lets them scale it, and
//			commits the placement. See weapon_camera.cpp for the capture half
//			of the loop, and photo_inventory.h for the shared capture state.
//
//=============================================================================//
#include "cbase.h"
#include "weapon_placement.h"
#include "portal_player.h"
#include "photo_inventory.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar camera_placement_distance( "camera_placement_distance", "150", FCVAR_REPLICATED, "How far in front of the player a captured object previews for placement." );
ConVar camera_scale_step( "camera_scale_step", "1.25", FCVAR_REPLICATED, "Multiplier applied per scale level when resizing a captured object." );
ConVar camera_max_scale_level( "camera_max_scale_level", "4", FCVAR_REPLICATED, "Highest/lowest scale level (in either direction) weapon_placement allows." );

IMPLEMENT_SERVERCLASS_ST( CWeaponPlacement, DT_WeaponPlacement )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_placement, CWeaponPlacement );

PRECACHE_WEAPON_REGISTER( weapon_placement );

CWeaponPlacement::CWeaponPlacement()
	: m_bLastPreviewValid( false )
{
}

void CWeaponPlacement::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_Camera.Release" );
	PrecacheScriptSound( "Weapon_Camera.StretchUp" );
	PrecacheScriptSound( "Weapon_Camera.StretchDown" );
}

//-----------------------------------------------------------------------------
// Purpose: Where would the held photo land if we placed it right now?
//-----------------------------------------------------------------------------
bool CWeaponPlacement::ComputePlacementTransform( Vector *pOutOrigin, QAngle *pOutAngles )
{
	CPortal_Player *pOwner = ToPortalPlayer( GetOwner() );
	if ( !pOwner )
		return false;

	CBaseAnimating *pEntity = pOwner->GetPhotoInventory()->GetCapturedEntity();
	if ( !pEntity )
		return false;

	Vector vecEye = pOwner->EyePosition();
	Vector vecForward;
	AngleVectors( pOwner->EyeAngles(), &vecForward );

	trace_t tr;
	Ray_t ray;
	ray.Init( vecEye, vecEye + vecForward * camera_placement_distance.GetFloat() );
	CTraceFilterSimple traceFilter( pOwner, COLLISION_GROUP_NONE );
	UTIL_TraceRay( ray, MASK_SOLID, &traceFilter, &tr );

	*pOutOrigin = tr.endpos;
	pOutAngles->Init( 0, pOwner->EyeAngles().y, 0 );

	// Make sure the object's (unscaled - see weapon_camera.cpp header) bounding
	// box fits at that spot.
	ICollideable *pCollide = pEntity->CollisionProp();
	Vector vecMins = pCollide ? pCollide->OBBMins() : -Vector( 8, 8, 8 );
	Vector vecMaxs = pCollide ? pCollide->OBBMaxs() :  Vector( 8, 8, 8 );

	trace_t hullTrace;
	Ray_t hullRay;
	hullRay.Init( *pOutOrigin, *pOutOrigin, vecMins, vecMaxs );
	CTraceFilterSimple hullFilter( pEntity, COLLISION_GROUP_NONE );
	UTIL_TraceRay( hullRay, MASK_SOLID, &hullFilter, &hullTrace );

	return !hullTrace.startsolid && !hullTrace.allsolid;
}

//-----------------------------------------------------------------------------
// Purpose: Move the held preview to track the player's aim every frame,
//			tinting it based on whether the current spot is placeable.
//-----------------------------------------------------------------------------
void CWeaponPlacement::UpdateHeldPreview( void )
{
	CPortal_Player *pOwner = ToPortalPlayer( GetOwner() );
	if ( !pOwner )
		return;

	CBaseAnimating *pEntity = pOwner->GetPhotoInventory()->GetCapturedEntity();
	if ( !pEntity )
		return;

	Vector vecOrigin;
	QAngle angOrigin;
	bool bValid = ComputePlacementTransform( &vecOrigin, &angOrigin );

	pEntity->Teleport( &vecOrigin, &angOrigin, &vec3_origin );

	if ( bValid != m_bLastPreviewValid )
	{
		m_bLastPreviewValid = bValid;
		pEntity->SetRenderColor( 255, bValid ? 255 : 60, bValid ? 255 : 60 );
	}

	float flScale = pOwner->GetPhotoInventory()->GetScaleMultiplier();
	if ( pEntity->GetModelScale() != flScale )
	{
		pEntity->SetModelScale( flScale );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Commit the held photo at its current preview transform.
//-----------------------------------------------------------------------------
void CWeaponPlacement::PrimaryAttack( void )
{
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.3f;

	CPortal_Player *pOwner = ToPortalPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( !pOwner->GetPhotoInventory()->HasPhoto() || !m_bLastPreviewValid )
		return;

	Vector vecOrigin;
	QAngle angOrigin;
	if ( !ComputePlacementTransform( &vecOrigin, &angOrigin ) )
		return;

	pOwner->GetPhotoInventory()->PlacePhoto( vecOrigin, angOrigin );
	m_bLastPreviewValid = false;

	EmitSound( "Weapon_Camera.Release" );
}

//-----------------------------------------------------------------------------
// Purpose: Scale the held photo up a step.
//-----------------------------------------------------------------------------
void CWeaponPlacement::SecondaryAttack( void )
{
	CPortal_Player *pOwner = ToPortalPlayer( GetOwner() );
	if ( !pOwner || !pOwner->GetPhotoInventory()->HasPhoto() )
		return;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.2f;

	pOwner->GetPhotoInventory()->ScaleUp();
	EmitSound( "Weapon_Camera.StretchUp" );
}

//-----------------------------------------------------------------------------
// Purpose: Scale the held photo down a step (Reload), and keep the preview
//			tracking the player's aim every frame.
//-----------------------------------------------------------------------------
void CWeaponPlacement::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	CPortal_Player *pOwner = ToPortalPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( !pOwner->GetPhotoInventory()->HasPhoto() )
		return;

	if ( pOwner->m_afButtonPressed & IN_RELOAD )
	{
		pOwner->GetPhotoInventory()->ScaleDown();
		EmitSound( "Weapon_Camera.StretchDown" );
	}

	UpdateHeldPreview();
}

void CWeaponPlacement::UpdateOnRemove( void )
{
	CPortal_Player *pOwner = ToPortalPlayer( GetOwner() );
	if ( pOwner && pOwner->GetPhotoInventory()->HasPhoto() )
	{
		pOwner->GetPhotoInventory()->ReleaseWithoutPlacing();
	}

	BaseClass::UpdateOnRemove();
}
