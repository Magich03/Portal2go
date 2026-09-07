//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: item_photograph - see item_photograph.h.
//
//=============================================================================//
#include "cbase.h"
#include "item_photograph.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// No F-Stop photograph art ships in this repo (see weapon_camera.txt/
// weapon_placement.txt for the same placeholder situation with viewmodels) -
// reuse a normal HL2 physics prop with known-good vphysics collision so the
// photograph can be picked up, scaled and placed like any other object.
#define PHOTOGRAPH_MODEL "models/props_junk/cardboard_box004a.mdl"

IMPLEMENT_SERVERCLASS_ST( CItem_Photograph, DT_Photograph )
	SendPropString( SENDINFO( m_szTextureName ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( item_photograph, CItem_Photograph );

void CItem_Photograph::Spawn( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		PrecacheModel( PHOTOGRAPH_MODEL );
		SetModel( PHOTOGRAPH_MODEL );
	}

	BaseClass::Spawn();

	// Photographs are always capturable - that's their entire purpose.
	SetCanBeCaptured( true );
}

void CItem_Photograph::SetPhotoTexture( const char *pszTextureName )
{
	Q_strncpy( m_szTextureName, pszTextureName, sizeof( m_szTextureName ) );
}

CItem_Photograph *CreatePhotograph( const Vector &vecOrigin, const QAngle &angOrigin, const char *pszTextureName )
{
	CItem_Photograph *pPhoto = static_cast<CItem_Photograph*>(
		CBaseEntity::Create( "item_photograph", vecOrigin, angOrigin, NULL ) );

	if ( !pPhoto )
		return NULL;

	pPhoto->SetPhotoTexture( pszTextureName );

	return pPhoto;
}
