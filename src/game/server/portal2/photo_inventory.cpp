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
// Purpose: Undoes a stashed/ghosted entity's hidden state (disabled
//			physics/collision, EF_NODRAW) - safe to call even if some of
//			those weren't set. Render mode/scale are handled by the caller
//			since PlacePhoto() and ReleaseWithoutPlacing() want different
//			final visuals.
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

bool CPhotoInventory::IsPolaroidPending( void ) const
{
	return m_Stack.Count() > 0 && m_Stack.Tail().mode == PHOTOMODE_POLAROID;
}

bool CPhotoInventory::IsGhostActive( void ) const
{
	return m_Stack.Count() > 0 && m_Stack.Tail().mode == PHOTOMODE_GHOST;
}

CBaseAnimating *CPhotoInventory::GetCapturedEntity( void ) const
{
	if ( m_Stack.Count() > 0 )
		return m_Stack.Tail().hEntity.Get();

	return NULL;
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
	// Model scale (visual only - see weapon_camera.cpp header) is already
	// applied live by weapon_placement while previewing; just record the
	// final level.
	pEntity->SetObjectScaleLevel( stacked.nScaleLevel );

	pEntity->m_OnCameraRelease.FireOutput( pEntity, pEntity );
	pEntity->OnCameraPlaced();
}

void CPhotoInventory::ReleaseWithoutPlacing( void )
{
	ReturnPhotosToWorld();
}

int CPhotoInventory::ReturnPhotosToWorld( void )
{
	int nRestored = 0;

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
			++nRestored;
		}

		if ( stacked.hPolaroid.Get() )
		{
			UTIL_Remove( stacked.hPolaroid.Get() );
		}
	}

	m_Stack.RemoveAll();

	return nRestored;
}

int CPhotoInventory::GetScaleLevel( void ) const
{
	if ( m_Stack.Count() > 0 )
		return m_Stack.Tail().nScaleLevel;

	return 0;
}

void CPhotoInventory::ScaleUp( void )
{
	if ( m_Stack.Count() == 0 )
		return;

	int &nLevel = m_Stack.Tail().nScaleLevel;
	if ( nLevel < camera_max_scale_level.GetInt() )
	{
		++nLevel;
	}
}

void CPhotoInventory::ScaleDown( void )
{
	if ( m_Stack.Count() == 0 )
		return;

	int &nLevel = m_Stack.Tail().nScaleLevel;
	if ( nLevel > -camera_max_scale_level.GetInt() )
	{
		--nLevel;
	}
}

float CPhotoInventory::GetScaleMultiplier( void ) const
{
	return powf( camera_scale_step.GetFloat(), (float)GetScaleLevel() );
}
