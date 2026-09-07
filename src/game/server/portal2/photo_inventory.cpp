//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: See photo_inventory.h
//
//=============================================================================//
#include "cbase.h"
#include "photo_inventory.h"
#include "baseanimating.h"
#include "item_photograph.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar camera_scale_step;
extern ConVar camera_max_scale_level;

CPhotoInventory::CPhotoInventory()
	: m_nMode( PHOTOMODE_NONE )
	, m_nScaleLevel( 0 )
{
}

bool CPhotoInventory::CaptureDirect( CBaseAnimating *pEntity )
{
	if ( HasPhoto() || !pEntity )
		return false;

	m_hCapturedEntity = pEntity;
	m_nScaleLevel = 0;
	m_nMode = PHOTOMODE_DIRECT;

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
	pEntity->OnCameraCaptured();

	return true;
}

bool CPhotoInventory::CapturePolaroid( CBaseAnimating *pEntity, CItem_Photograph *pPolaroid )
{
	if ( HasPhoto() || !pEntity )
		return false;

	m_hCapturedEntity = pEntity;
	m_hPolaroid = pPolaroid;
	m_nScaleLevel = 0;
	m_nMode = PHOTOMODE_POLAROID;

	// Stash the real object completely out of the world - nothing 3D exists
	// while the player is just holding the polaroid. SpawnGhost() brings it
	// back once they click to place it.
	IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
	if ( pPhys )
	{
		pPhys->EnableMotion( false );
		pPhys->EnableCollisions( false );
	}

	pEntity->AddSolidFlags( FSOLID_NOT_SOLID );
	pEntity->AddEffects( EF_NODRAW );

	pEntity->m_OnCameraCapture.FireOutput( pEntity, pEntity );
	pEntity->OnCameraCaptured();

	return true;
}

void CPhotoInventory::SpawnGhost( const Vector &vecOrigin, const QAngle &angOrigin )
{
	if ( m_nMode != PHOTOMODE_POLAROID )
		return;

	CBaseAnimating *pEntity = m_hCapturedEntity.Get();
	if ( !pEntity )
		return;

	pEntity->Teleport( &vecOrigin, &angOrigin, &vec3_origin );
	pEntity->RemoveEffects( EF_NODRAW );
	pEntity->SetRenderMode( kRenderTransColor );
	pEntity->SetRenderColorA( 180 );
	pEntity->SetObjectScaleLevel( 0 );

	if ( m_hPolaroid.Get() )
	{
		UTIL_Remove( m_hPolaroid.Get() );
		m_hPolaroid = NULL;
	}

	m_nMode = PHOTOMODE_GHOST;
}

void CPhotoInventory::PlacePhoto( const Vector &vecOrigin, const QAngle &angOrigin )
{
	CBaseAnimating *pEntity = m_hCapturedEntity.Get();
	if ( !pEntity )
		return;

	pEntity->Teleport( &vecOrigin, &angOrigin, &vec3_origin );

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
	// Model scale (visual only - see weapon_camera.cpp header) is already applied
	// live by weapon_placement while previewing; just record the final level.
	pEntity->SetObjectScaleLevel( m_nScaleLevel );

	pEntity->m_OnCameraRelease.FireOutput( pEntity, pEntity );
	pEntity->OnCameraPlaced();

	m_hCapturedEntity = NULL;
	m_nMode = PHOTOMODE_NONE;
	m_nScaleLevel = 0;
}

void CPhotoInventory::ReleaseWithoutPlacing( void )
{
	CBaseAnimating *pEntity = m_hCapturedEntity.Get();
	if ( pEntity )
	{
		IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
		if ( pPhys )
		{
			pPhys->EnableCollisions( true );
			pPhys->EnableMotion( true );
			pPhys->Wake();
		}

		pEntity->RemoveSolidFlags( FSOLID_NOT_SOLID );
		pEntity->RemoveEffects( EF_NODRAW );
		pEntity->SetRenderMode( kRenderNormal );
		pEntity->SetRenderColorA( 255 );
		pEntity->SetModelScale( 1.0f );
		pEntity->SetObjectScaleLevel( 0 );
		pEntity->OnCameraPlaced();
	}

	if ( m_hPolaroid.Get() )
	{
		UTIL_Remove( m_hPolaroid.Get() );
	}

	m_hCapturedEntity = NULL;
	m_hPolaroid = NULL;
	m_nMode = PHOTOMODE_NONE;
	m_nScaleLevel = 0;
}

void CPhotoInventory::ScaleUp( void )
{
	if ( m_nScaleLevel < camera_max_scale_level.GetInt() )
	{
		++m_nScaleLevel;
	}
}

void CPhotoInventory::ScaleDown( void )
{
	if ( m_nScaleLevel > -camera_max_scale_level.GetInt() )
	{
		--m_nScaleLevel;
	}
}

float CPhotoInventory::GetScaleMultiplier( void ) const
{
	return powf( camera_scale_step.GetFloat(), (float)m_nScaleLevel );
}
