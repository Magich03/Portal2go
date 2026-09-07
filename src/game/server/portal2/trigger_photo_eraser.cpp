//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: trigger_photo_eraser - see trigger_photo_eraser.h.
//
//=============================================================================//
#include "cbase.h"
#include "trigger_photo_eraser.h"
#include "portal_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( trigger_photo_eraser, CTriggerPhotoEraser );

BEGIN_DATADESC( CTriggerPhotoEraser )
	DEFINE_OUTPUT( m_OnObjectsFizzled, "OnObjectsFizzled" ),
END_DATADESC()

void CTriggerPhotoEraser::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: A player walking through erases every photo they're currently
//			holding, dumping the real objects back into the world.
//-----------------------------------------------------------------------------
void CTriggerPhotoEraser::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	CPortal_Player *pPlayer = ToPortalPlayer( pOther );
	if ( !pPlayer )
		return;

	int nRestored = pPlayer->GetPhotoInventory()->ReturnPhotosToWorld();
	if ( nRestored > 0 )
	{
		pPlayer->SetPlacingPhoto( false );
		m_OnObjectsFizzled.FireOutput( pPlayer, this );
	}
}
