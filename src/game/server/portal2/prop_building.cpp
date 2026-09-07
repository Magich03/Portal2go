//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: prop_building - see prop_building.h.
//
//=============================================================================//
#include "cbase.h"
#include "prop_building.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// No F-Stop building art ships in this repo.
#define BUILDING_MODEL "models/props_fstop/dollhouse04.mdl"

LINK_ENTITY_TO_CLASS( prop_building, CPropBuilding );

BEGIN_DATADESC( CPropBuilding )
	DEFINE_KEYFIELD( m_strTargetPortal, FIELD_STRING, "target_portal" ),
	DEFINE_FIELD( m_hTargetPortal, FIELD_EHANDLE ),
END_DATADESC()

void CPropBuilding::Spawn( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		PrecacheModel( BUILDING_MODEL );
		SetModel( BUILDING_MODEL );
	}

	BaseClass::Spawn();
}

void CPropBuilding::Activate( void )
{
	BaseClass::Activate();

	ResolveTargetPortal();
}

void CPropBuilding::ResolveTargetPortal( void )
{
	if ( m_strTargetPortal != NULL_STRING )
	{
		m_hTargetPortal = gEntList.FindEntityByName( NULL, m_strTargetPortal );
	}
}
