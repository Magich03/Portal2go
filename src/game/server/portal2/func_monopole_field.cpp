//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: func_monopole_field - see func_monopole_field.h.
//
//			The decompiled binary confirms the keyvalues/inputs and
//			enabled/positive state, but no Touch()/force logic survived
//			reconstruction. Implemented here as a trigger volume that applies
//			the same style of attract/repel force as prop_monopole to
//			whatever's currently touching it, since that's the only way its
//			Toggle/TurnOn/polarity inputs would have any effect at all.
//
//=============================================================================//
#include "cbase.h"
#include "func_monopole_field.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar monopole_field_force( "monopole_field_force", "600", FCVAR_REPLICATED, "Acceleration func_monopole_field applies to anything with physics touching it." );

LINK_ENTITY_TO_CLASS( func_monopole_field, CMonopoleField );

BEGIN_DATADESC( CMonopoleField )
	DEFINE_KEYFIELD( m_bEnabled, FIELD_BOOLEAN, "StartActive" ),
	DEFINE_KEYFIELD( m_bPositive, FIELD_BOOLEAN, "StartPositive" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TogglePolarity", InputTogglePolarity ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnPositive", InputTurnPositive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnNegative", InputTurnNegative ),

	DEFINE_THINKFUNC( FieldThink ),
END_DATADESC()

void CMonopoleField::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	SetContextThink( &CMonopoleField::FieldThink, gpGlobals->curtime, "FieldThink" );
}

void CMonopoleField::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if ( pOther && pOther->VPhysicsGetObject() )
	{
		m_hTouching.AddToTail( pOther );
	}
}

void CMonopoleField::EndTouch( CBaseEntity *pOther )
{
	BaseClass::EndTouch( pOther );

	FOR_EACH_VEC_BACK( m_hTouching, i )
	{
		if ( m_hTouching[ i ].Get() == pOther )
		{
			m_hTouching.Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pull/push everything currently touching us toward/away from our
//			center.
//-----------------------------------------------------------------------------
void CMonopoleField::FieldThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( !m_bEnabled )
		return;

	Vector vecCenter = WorldSpaceCenter();

	FOR_EACH_VEC_BACK( m_hTouching, i )
	{
		CBaseEntity *pEntity = m_hTouching[ i ].Get();
		if ( !pEntity )
		{
			m_hTouching.Remove( i );
			continue;
		}

		IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
		if ( !pPhys )
			continue;

		Vector vecToTarget = pEntity->WorldSpaceCenter() - vecCenter;
		float flDist = vecToTarget.Length();
		if ( flDist < 1.0f )
			continue;

		Vector vecDir = vecToTarget / flDist;
		if ( m_bPositive )
		{
			vecDir = -vecDir;	// positive polarity attracts
		}

		Vector vecImpulse = vecDir * monopole_field_force.GetFloat() * pPhys->GetMass() * gpGlobals->frametime;
		pPhys->ApplyForceCenter( vecImpulse );
	}
}
