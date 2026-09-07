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

//-----------------------------------------------------------------------------
// Purpose: Undoes whichever "hidden" state a stashed/ghosted entity was in
//			(disabled physics/collision, EF_NODRAW) - safe to call even if
//			some of those weren't set. Shared by PlacePhoto() and
//			ReleaseWithoutPlacing(); render mode/scale are handled by the
//			caller since the two want different final visuals.
//-----------------------------------------------------------------------------
static void UnstashEntity( CBaseAnimating *pEntity )
{
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
	pEntity->RemoveEffects( EF_NODRAW );
}

CPhotoInventory::CPhotoInventory()
	: m_nDirectScaleLevel( 0 )
{
}

bool CPhotoInventory::HasPhoto( void ) const
{
	return m_hDirectEntity.Get() != NULL || m_Stack.Count() > 0;
}

bool CPhotoInventory::IsPolaroidPending( void ) const
{
	return m_Stack.Count() > 0 && m_Stack.Tail().mode == PHOTOMODE_POLAROID;
}

bool CPhotoInventory::IsGhostActive( void ) const
{
	if ( m_hDirectEntity.Get() != NULL )
		return true;

	return m_Stack.Count() > 0 && m_Stack.Tail().mode == PHOTOMODE_GHOST;
}

CBaseAnimating *CPhotoInventory::GetCapturedEntity( void ) const
{
	if ( m_hDirectEntity.Get() != NULL )
		return m_hDirectEntity.Get();

	if ( m_Stack.Count() > 0 )
		return m_Stack.Tail().hEntity.Get();

	return NULL;
}

bool CPhotoInventory::CaptureDirect( CBaseAnimating *pEntity )
{
	if ( m_hDirectEntity.Get() != NULL || !pEntity )
		return false;

	m_hDirectEntity = pEntity;
	m_nDirectScaleLevel = 0;

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
	if ( IsStackFull() || !pEntity )
		return false;

	// Stash the real object completely out of the world - nothing 3D exists
	// while the player is just holding the polaroid. SpawnGhost() brings it
	// back once it reaches the top of the stack and they click to place it.
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

	StackedPhoto_t &stacked = m_Stack[ m_Stack.AddToTail() ];
	stacked.hEntity = pEntity;
	stacked.hPolaroid = pPolaroid;
	stacked.mode = PHOTOMODE_POLAROID;
	stacked.nScaleLevel = 0;

	return true;
}

void CPhotoInventory::SpawnGhost( const Vector &vecOrigin, const QAngle &angOrigin )
{
	if ( !IsPolaroidPending() )
		return;

	StackedPhoto_t &stacked = m_Stack.Tail();
	CBaseAnimating *pEntity = stacked.hEntity.Get();
	if ( !pEntity )
		return;

	pEntity->Teleport( &vecOrigin, &angOrigin, &vec3_origin );
	pEntity->RemoveEffects( EF_NODRAW );
	pEntity->SetRenderMode( kRenderTransColor );
	pEntity->SetRenderColorA( 180 );
	pEntity->SetObjectScaleLevel( 0 );

	if ( stacked.hPolaroid.Get() )
	{
		UTIL_Remove( stacked.hPolaroid.Get() );
		stacked.hPolaroid = NULL;
	}

	stacked.mode = PHOTOMODE_GHOST;
}

void CPhotoInventory::PlacePhoto( const Vector &vecOrigin, const QAngle &angOrigin )
{
	if ( m_hDirectEntity.Get() != NULL )
	{
		CBaseAnimating *pEntity = m_hDirectEntity.Get();

		pEntity->Teleport( &vecOrigin, &angOrigin, &vec3_origin );
		UnstashEntity( pEntity );
		pEntity->SetRenderMode( kRenderNormal );
		pEntity->SetRenderColorA( 255 );
		// Model scale (visual only - see weapon_camera.cpp header) is already
		// applied live by weapon_placement while previewing; just record the
		// final level.
		pEntity->SetObjectScaleLevel( m_nDirectScaleLevel );

		pEntity->m_OnCameraRelease.FireOutput( pEntity, pEntity );
		pEntity->OnCameraPlaced();

		m_hDirectEntity = NULL;
		m_nDirectScaleLevel = 0;
		return;
	}

	if ( m_Stack.Count() == 0 || m_Stack.Tail().mode != PHOTOMODE_GHOST )
		return;

	StackedPhoto_t stacked = m_Stack.Tail();
	m_Stack.RemoveMultipleFromTail( 1 );

	CBaseAnimating *pEntity = stacked.hEntity.Get();
	if ( !pEntity )
		return;

	pEntity->Teleport( &vecOrigin, &angOrigin, &vec3_origin );
	UnstashEntity( pEntity );
	pEntity->SetRenderMode( kRenderNormal );
	pEntity->SetRenderColorA( 255 );
	pEntity->SetObjectScaleLevel( stacked.nScaleLevel );

	pEntity->m_OnCameraRelease.FireOutput( pEntity, pEntity );
	pEntity->OnCameraPlaced();
}

void CPhotoInventory::ReleaseWithoutPlacing( void )
{
	CBaseAnimating *pDirect = m_hDirectEntity.Get();
	if ( pDirect )
	{
		UnstashEntity( pDirect );
		pDirect->SetRenderMode( kRenderNormal );
		pDirect->SetRenderColorA( 255 );
		pDirect->SetModelScale( 1.0f );
		pDirect->SetObjectScaleLevel( 0 );
		pDirect->OnCameraPlaced();
	}
	m_hDirectEntity = NULL;
	m_nDirectScaleLevel = 0;

	FOR_EACH_VEC( m_Stack, i )
	{
		StackedPhoto_t &stacked = m_Stack[ i ];

		CBaseAnimating *pEntity = stacked.hEntity.Get();
		if ( pEntity )
		{
			UnstashEntity( pEntity );
			pEntity->SetRenderMode( kRenderNormal );
			pEntity->SetRenderColorA( 255 );
			pEntity->SetModelScale( 1.0f );
			pEntity->SetObjectScaleLevel( 0 );
			pEntity->OnCameraPlaced();
		}

		if ( stacked.hPolaroid.Get() )
		{
			UTIL_Remove( stacked.hPolaroid.Get() );
		}
	}

	m_Stack.RemoveAll();
}

int CPhotoInventory::GetScaleLevel( void ) const
{
	if ( m_hDirectEntity.Get() != NULL )
		return m_nDirectScaleLevel;

	if ( m_Stack.Count() > 0 )
		return m_Stack.Tail().nScaleLevel;

	return 0;
}

void CPhotoInventory::ScaleUp( void )
{
	int *pLevel = NULL;

	if ( m_hDirectEntity.Get() != NULL )
	{
		pLevel = &m_nDirectScaleLevel;
	}
	else if ( m_Stack.Count() > 0 )
	{
		pLevel = &m_Stack.Tail().nScaleLevel;
	}

	if ( pLevel && *pLevel < camera_max_scale_level.GetInt() )
	{
		++( *pLevel );
	}
}

void CPhotoInventory::ScaleDown( void )
{
	int *pLevel = NULL;

	if ( m_hDirectEntity.Get() != NULL )
	{
		pLevel = &m_nDirectScaleLevel;
	}
	else if ( m_Stack.Count() > 0 )
	{
		pLevel = &m_Stack.Tail().nScaleLevel;
	}

	if ( pLevel && *pLevel > -camera_max_scale_level.GetInt() )
	{
		--( *pLevel );
	}
}

float CPhotoInventory::GetScaleMultiplier( void ) const
{
	return powf( camera_scale_step.GetFloat(), (float)GetScaleLevel() );
}
