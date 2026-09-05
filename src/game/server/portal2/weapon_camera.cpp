//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: F-Stop camera - captures a nearby capturable prop, carries it as a
//			translucent preview that follows the player's aim, and places it
//			back into the world (optionally rescaled) on a second trigger pull.
//
//			This is a from-scratch reimplementation of the "F-Stop"/Exposure
//			camera+placement loop, built around real (if previously unused)
//			hooks already present in this codebase: CBaseAnimating's
//			m_bCanBeCaptured/CanBeCaptured(), m_OnCameraCapture/m_OnCameraRelease,
//			GetObjectScaleLevel()/SetObjectScaleLevel(), and SetModelScale().
//			It intentionally does NOT attempt to rescale physics collision -
//			that's the part the original Exposure binary itself documents as
//			broken, so placed objects keep their original-size collision even
//			when visually scaled.
//
//=============================================================================//
#include "cbase.h"
#include "weapon_camera.h"
#include "portal_player.h"
#include "in_buttons.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar camera_capture_distance( "camera_capture_distance", "256", FCVAR_REPLICATED, "How far away weapon_camera can capture an object from." );
ConVar camera_placement_distance( "camera_placement_distance", "150", FCVAR_REPLICATED, "How far in front of the player a captured object previews for placement." );
ConVar camera_scale_step( "camera_scale_step", "1.25", FCVAR_REPLICATED, "Multiplier applied per scale level when resizing a captured object." );
ConVar camera_max_scale_level( "camera_max_scale_level", "4", FCVAR_REPLICATED, "Highest/lowest scale level (in either direction) weapon_camera allows." );

IMPLEMENT_SERVERCLASS_ST( CWeaponCamera, DT_WeaponCamera )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_camera, CWeaponCamera );

PRECACHE_WEAPON_REGISTER( weapon_camera );

BEGIN_DATADESC( CWeaponCamera )
	DEFINE_FIELD( m_hCapturedEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_nScaleLevel, FIELD_INTEGER ),
	DEFINE_FIELD( m_bLastPreviewValid, FIELD_BOOLEAN ),
END_DATADESC()

CWeaponCamera::CWeaponCamera()
	: m_nScaleLevel( 0 ),
	  m_bLastPreviewValid( false )
{
}

void CWeaponCamera::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_Camera.Capture" );
	PrecacheScriptSound( "Weapon_Camera.Release" );
	PrecacheScriptSound( "Weapon_Camera.StretchUp" );
	PrecacheScriptSound( "Weapon_Camera.StretchDown" );
}

//-----------------------------------------------------------------------------
// Purpose: Trace out from the player's eyes and see if we're looking at
//			something that allows capture.
//-----------------------------------------------------------------------------
CBaseAnimating *CWeaponCamera::FindCapturableEntity( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return NULL;

	Vector vecEye = pOwner->EyePosition();
	Vector vecForward;
	AngleVectors( pOwner->EyeAngles(), &vecForward );

	trace_t tr;
	Ray_t ray;
	ray.Init( vecEye, vecEye + vecForward * camera_capture_distance.GetFloat() );
	CTraceFilterSimple traceFilter( pOwner, COLLISION_GROUP_NONE );
	UTIL_TraceRay( ray, MASK_SOLID, &traceFilter, &tr );

	if ( !tr.m_pEnt )
		return NULL;

	CBaseAnimating *pAnim = tr.m_pEnt->GetBaseAnimating();
	if ( !pAnim )
		return NULL;

	if ( !pAnim->CanBeCaptured() )
		return NULL;

	if ( !pAnim->VPhysicsGetObject() )
		return NULL;

	return pAnim;
}

//-----------------------------------------------------------------------------
// Purpose: Pull an entity out of normal play - disable its physics/collision,
//			make it translucent, and start previewing it at the player's aim.
//-----------------------------------------------------------------------------
void CWeaponCamera::CaptureEntity( CBaseAnimating *pEntity )
{
	m_hCapturedEntity = pEntity;
	m_nScaleLevel = 0;
	m_bLastPreviewValid = false;

	IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
	if ( pPhys )
	{
		pPhys->EnableMotion( false );
		pPhys->EnableCollisions( false );
	}

	pEntity->AddSolidFlags( FSOLID_NOT_SOLID );
	pEntity->SetRenderMode( kRenderTransColor );
	pEntity->SetRenderColorA( 180 );
	pEntity->SetObjectScaleLevel( 0 );

	pEntity->m_OnCameraCapture.FireOutput( pEntity, pEntity );

	CPortal_Player *pPortalPlayer = ToPortalPlayer( GetOwner() );
	if ( pPortalPlayer )
	{
		pPortalPlayer->SetPlacingPhoto( true );
	}

	EmitSound( "Weapon_Camera.Capture" );
}

//-----------------------------------------------------------------------------
// Purpose: Where would the held object land if we placed it right now?
//-----------------------------------------------------------------------------
bool CWeaponCamera::ComputePlacementTransform( Vector *pOutOrigin, QAngle *pOutAngles )
{
	CBaseAnimating *pEntity = m_hCapturedEntity.Get();
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pEntity || !pOwner )
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

	// Make sure the object's (unscaled - see file header) bounding box fits at that spot.
	ICollideable *pCollide = pEntity->CollisionProp();
	Vector vecMins = pCollide ? pCollide->OBBMins() : -Vector(8,8,8);
	Vector vecMaxs = pCollide ? pCollide->OBBMaxs() :  Vector(8,8,8);

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
void CWeaponCamera::UpdateHeldPreview( void )
{
	CBaseAnimating *pEntity = m_hCapturedEntity.Get();
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

	float flScale = GetScaleMultiplier();
	if ( pEntity->GetModelScale() != flScale )
	{
		pEntity->SetModelScale( flScale );
	}
}

float CWeaponCamera::GetScaleMultiplier( void ) const
{
	return powf( camera_scale_step.GetFloat(), (float)m_nScaleLevel );
}

//-----------------------------------------------------------------------------
// Purpose: Commit the held object at its current preview transform.
//-----------------------------------------------------------------------------
void CWeaponCamera::CommitPlacement( void )
{
	CBaseAnimating *pEntity = m_hCapturedEntity.Get();
	if ( !pEntity )
		return;

	IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
	if ( pPhys )
	{
		pPhys->EnableCollisions( true );
		pPhys->EnableMotion( true );
		pPhys->Wake();
	}

	pEntity->RemoveSolidFlags( FSOLID_NOT_SOLID );
	pEntity->SetRenderMode( kRenderNormal );
	pEntity->SetRenderColorA( 255 );
	pEntity->SetObjectScaleLevel( m_nScaleLevel );

	pEntity->m_OnCameraRelease.FireOutput( pEntity, pEntity );

	CPortal_Player *pPortalPlayer = ToPortalPlayer( GetOwner() );
	if ( pPortalPlayer )
	{
		pPortalPlayer->SetPlacingPhoto( false );
	}

	EmitSound( "Weapon_Camera.Release" );

	m_hCapturedEntity = NULL;
	m_nScaleLevel = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Something interrupted us (weapon dropped/removed) - put the held
//			object back the way it was rather than leaving it permanently
//			non-solid and translucent.
//-----------------------------------------------------------------------------
void CWeaponCamera::ReleaseWithoutPlacing( void )
{
	CBaseAnimating *pEntity = m_hCapturedEntity.Get();
	if ( !pEntity )
		return;

	IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
	if ( pPhys )
	{
		pPhys->EnableCollisions( true );
		pPhys->EnableMotion( true );
		pPhys->Wake();
	}

	pEntity->RemoveSolidFlags( FSOLID_NOT_SOLID );
	pEntity->SetRenderMode( kRenderNormal );
	pEntity->SetRenderColorA( 255 );
	pEntity->SetModelScale( 1.0f );
	pEntity->SetObjectScaleLevel( 0 );

	CPortal_Player *pPortalPlayer = ToPortalPlayer( GetOwner() );
	if ( pPortalPlayer )
	{
		pPortalPlayer->SetPlacingPhoto( false );
	}

	m_hCapturedEntity = NULL;
	m_nScaleLevel = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Not holding anything -> try to capture. Holding something ->
//			try to place it.
//-----------------------------------------------------------------------------
void CWeaponCamera::PrimaryAttack( void )
{
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.3f;

	if ( !IsHoldingObject() )
	{
		CBaseAnimating *pTarget = FindCapturableEntity();
		if ( pTarget )
		{
			CaptureEntity( pTarget );
		}
		return;
	}

	if ( m_bLastPreviewValid )
	{
		CommitPlacement();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Scale the held object up a step.
//-----------------------------------------------------------------------------
void CWeaponCamera::SecondaryAttack( void )
{
	if ( !IsHoldingObject() )
		return;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.2f;

	if ( m_nScaleLevel >= camera_max_scale_level.GetInt() )
		return;

	++m_nScaleLevel;
	EmitSound( "Weapon_Camera.StretchUp" );
}

//-----------------------------------------------------------------------------
// Purpose: Scale the held object down a step. Bound to Reload since the
//			camera only has the two attack inputs otherwise.
//-----------------------------------------------------------------------------
void CWeaponCamera::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( IsHoldingObject() )
	{
		if ( pOwner->m_afButtonPressed & IN_RELOAD )
		{
			if ( m_nScaleLevel > -camera_max_scale_level.GetInt() )
			{
				--m_nScaleLevel;
				EmitSound( "Weapon_Camera.StretchDown" );
			}
		}

		UpdateHeldPreview();
	}
}

bool CWeaponCamera::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( IsHoldingObject() )
	{
		ReleaseWithoutPlacing();
	}

	return BaseClass::Holster( pSwitchingTo );
}

void CWeaponCamera::UpdateOnRemove( void )
{
	if ( IsHoldingObject() )
	{
		ReleaseWithoutPlacing();
	}

	BaseClass::UpdateOnRemove();
}
