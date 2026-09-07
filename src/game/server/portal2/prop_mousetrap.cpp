//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: prop_mousetrap - see prop_mousetrap.h.
//
//=============================================================================//
#include "cbase.h"
#include "prop_mousetrap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// No F-Stop mousetrap art ships in this repo - like prop_geyser, the
// original FGD itself marks this model as a placeholder.
#define MOUSETRAP_MODEL "models/props/metal_box.mdl"

// Local box (relative to our forward direction) we search for a victim in.
static const Vector MOUSETRAP_TRAP_MINS( 0.0f, -16.0f, -4.0f );
static const Vector MOUSETRAP_TRAP_MAXS( 32.0f, 16.0f, 16.0f );

ConVar mousetrap_forward_velocity( "mousetrap_forward_velocity", "250", FCVAR_REPLICATED, "How hard a prop_mousetrap launches its victim forward when it snaps." );
ConVar mousetrap_upward_velocity( "mousetrap_upward_velocity", "150", FCVAR_REPLICATED, "How hard a prop_mousetrap launches its victim upward when it snaps." );

LINK_ENTITY_TO_CLASS( prop_mousetrap, CPropMousetrap );

BEGIN_DATADESC( CPropMousetrap )
	DEFINE_INPUTFUNC( FIELD_VOID, "Snap", InputSnap ),
END_DATADESC()

void CPropMousetrap::Spawn( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		PrecacheModel( MOUSETRAP_MODEL );
		SetModel( MOUSETRAP_MODEL );
	}

	BaseClass::Spawn();
}

void CPropMousetrap::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "Mousetrap.Snap" );
}

void CPropMousetrap::InputSnap( inputdata_t &inputdata )
{
	Snap();
}

//-----------------------------------------------------------------------------
// Purpose: Find whatever's sitting in the trap area, in local space rotated
//			to our facing, biased along our forward direction.
//-----------------------------------------------------------------------------
CBaseEntity *CPropMousetrap::FindObjectInTrapArea( void )
{
	Vector vecForward, vecRight, vecUp;
	AngleVectors( GetAbsAngles(), &vecForward, &vecRight, &vecUp );

	Vector vecCenter = GetAbsOrigin() +
		vecForward * ( ( MOUSETRAP_TRAP_MINS.x + MOUSETRAP_TRAP_MAXS.x ) * 0.5f );

	float flHalfWide = ( MOUSETRAP_TRAP_MAXS.x - MOUSETRAP_TRAP_MINS.x ) * 0.5f;

	Vector vecMins( vecCenter.x - flHalfWide, vecCenter.y - flHalfWide, vecCenter.z + MOUSETRAP_TRAP_MINS.z );
	Vector vecMaxs( vecCenter.x + flHalfWide, vecCenter.y + flHalfWide, vecCenter.z + MOUSETRAP_TRAP_MAXS.z );

	CBaseEntity *pList[ 16 ];
	int nCount = UTIL_EntitiesInBox( pList, ARRAYSIZE( pList ), vecMins, vecMaxs, 0 );

	for ( int i = 0; i < nCount; ++i )
	{
		CBaseEntity *pEntity = pList[ i ];
		if ( !pEntity || pEntity == this )
			continue;

		if ( pEntity->VPhysicsGetObject() )
			return pEntity;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Launch whatever's in the trap forward and up.
//-----------------------------------------------------------------------------
void CPropMousetrap::Snap( void )
{
	EmitSound( "Mousetrap.Snap" );

	CBaseEntity *pVictim = FindObjectInTrapArea();
	if ( !pVictim )
		return;

	Vector vecForward;
	AngleVectors( GetAbsAngles(), &vecForward );

	Vector vecVelocity = vecForward * mousetrap_forward_velocity.GetFloat();
	vecVelocity.z += mousetrap_upward_velocity.GetFloat();

	IPhysicsObject *pPhys = pVictim->VPhysicsGetObject();
	if ( pPhys )
	{
		pPhys->AddVelocity( &vecVelocity, NULL );
	}
}
