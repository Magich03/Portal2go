//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: item_photograph - see item_photograph.h.
//
//=============================================================================//
#include "cbase.h"
#include "item_photograph.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST( CItem_Photograph, DT_Photograph )
	SendPropString( SENDINFO( m_szTextureName ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( item_photograph, CItem_Photograph );

void CItem_Photograph::Spawn( void )
{
	// Bookkeeping only - the player is holding a flat 2D polaroid, not a 3D
	// object, so this entity is never actually drawn or collided with.
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	AddEffects( EF_NODRAW );

	BaseClass::Spawn();
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
