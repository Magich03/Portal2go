//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: prop_monopole - see prop_monopole.h.
//
//			The decompiled binary confirms the keyvalues/inputs and that
//			MonopoleThink() iterates candidates in some field, applying a
//			magnetic force/constraint up to m_iMaxObjectsAttached of them
//			("ApplyMagneticForceOrConstraint" - functional intent only, exact
//			force math not recoverable). This applies a straightforward
//			radius-based attract/repel force scaled by massScale and clamped
//			by forcelimit, rather than a constraint.
//
//=============================================================================//
#include "cbase.h"
#include "prop_monopole.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// No F-Stop monopole art ships in this repo - "models/flag/briefcase.mdl" is
// a real HL2 model (a briefcase, matching the FGD exactly).
#define MONOPOLE_MODEL "models/flag/briefcase.mdl"

ConVar monopole_field_radius( "monopole_field_radius", "256", FCVAR_REPLICATED, "How far a prop_monopole reaches to find objects to attract/repel." );
ConVar monopole_think_interval( "monopole_think_interval", "0.1", FCVAR_REPLICATED, "How often a prop_monopole re-scans for candidates." );

LINK_ENTITY_TO_CLASS( prop_monopole, CMonopole );

BEGIN_DATADESC( CMonopole )
	DEFINE_KEYFIELD( m_flMassScale, FIELD_FLOAT, "massScale" ),
	DEFINE_KEYFIELD( m_iszOverrideScript, FIELD_STRING, "overridescript" ),
	DEFINE_KEYFIELD( m_iMaxObjectsAttached, FIELD_INTEGER, "maxobjects" ),
	DEFINE_KEYFIELD( m_flForceLimit, FIELD_FLOAT, "forcelimit" ),
	DEFINE_KEYFIELD( m_flTorqueLimit, FIELD_FLOAT, "torquelimit" ),
	DEFINE_KEYFIELD( m_bEnabled, FIELD_BOOLEAN, "StartActive" ),
	DEFINE_KEYFIELD( m_bPositive, FIELD_BOOLEAN, "StartPositive" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TogglePolarity", InputTogglePolarity ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnPositive", InputTurnPositive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnNegative", InputTurnNegative ),

	DEFINE_THINKFUNC( MonopoleThink ),
END_DATADESC()

void CMonopole::Spawn( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		PrecacheModel( MONOPOLE_MODEL );
		SetModel( MONOPOLE_MODEL );
	}

	if ( m_iMaxObjectsAttached <= 0 )
	{
		m_iMaxObjectsAttached = 1;
	}

	if ( m_flMassScale <= 0.0f )
	{
		m_flMassScale = 1.0f;
	}

	BaseClass::Spawn();

	VPhysicsInitNormal( SOLID_VPHYSICS, GetSolidFlags(), false );

	SetContextThink( &CMonopole::MonopoleThink, gpGlobals->curtime + monopole_think_interval.GetFloat(), "MonopoleThink" );
}

//-----------------------------------------------------------------------------
// Purpose: Pull (positive) or push (negative) nearby physics objects,
//			strongest closest to us, clamped by forcelimit and scaled by
//			massScale and our own current F-Stop scale.
//-----------------------------------------------------------------------------
void CMonopole::MonopoleThink( void )
{
	SetNextThink( gpGlobals->curtime + monopole_think_interval.GetFloat() );

	if ( !m_bEnabled )
		return;

	float flRadius = monopole_field_radius.GetFloat() * GetModelScale();

	CBaseEntity *pList[ 32 ];
	int nCount = UTIL_EntitiesInSphere( pList, ARRAYSIZE( pList ), GetAbsOrigin(), flRadius, 0 );

	int nAttached = 0;

	for ( int i = 0; i < nCount && nAttached < m_iMaxObjectsAttached; ++i )
	{
		CBaseEntity *pEntity = pList[ i ];
		if ( !pEntity || pEntity == this )
			continue;

		IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
		if ( !pPhys )
			continue;

		Vector vecToTarget = pEntity->WorldSpaceCenter() - WorldSpaceCenter();
		float flDist = vecToTarget.Length();
		if ( flDist < 1.0f )
			continue;

		Vector vecDir = vecToTarget / flDist;
		if ( m_bPositive )
		{
			vecDir = -vecDir;	// positive polarity attracts
		}

		float flFalloff = 1.0f - ( flDist / flRadius );
		flFalloff = clamp( flFalloff, 0.0f, 1.0f );

		float flForce = m_flForceLimit * m_flMassScale * flFalloff * GetModelScale();

		Vector vecImpulse = vecDir * flForce * gpGlobals->frametime;
		pPhys->ApplyForceCenter( vecImpulse );

		++nAttached;
	}
}
