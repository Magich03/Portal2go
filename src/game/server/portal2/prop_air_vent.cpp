//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: prop_air_vent - see prop_air_vent.h.
//
//=============================================================================//
#include "cbase.h"
#include "prop_air_vent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define AIR_VENT_MODEL "models/props_lab/prop_airvent.mdl"

LINK_ENTITY_TO_CLASS( prop_air_vent, CPropAirVent );

void CPropAirVent::Spawn( void )
{
	// Let a mapper-specified "model" keyvalue win; otherwise default to the vent grate.
	if ( GetModelName() == NULL_STRING )
	{
		PrecacheModel( AIR_VENT_MODEL );
		SetModel( AIR_VENT_MODEL );
	}

	BaseClass::Spawn();
}
