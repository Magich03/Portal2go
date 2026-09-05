//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: See photo_inventory.h
//
//=============================================================================//
#include "cbase.h"
#include "photo_inventory.h"
#include "baseanimating.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar camera_scale_step;
extern ConVar camera_max_scale_level;

CPhotoInventory::CPhotoInventory()
	: m_nScaleLevel( 0 )
{
}

bool CPhotoInventory::CapturePhoto( CBaseAnimating *pEntity )
{
	if ( HasPhoto() || !pEntity )
		return false;

	m_hCapturedEntity = pEntity;
	m_nScaleLevel = 0;

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

	return true;
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

	m_hCapturedEntity = NULL;
	m_nScaleLevel = 0;
}

void CPhotoInventory::ReleaseWithoutPlacing( void )
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

	m_hCapturedEntity = NULL;
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
