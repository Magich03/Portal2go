//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: prop_levitator - see prop_levitator.h.
//
//			The decompiled binary confirms the AttachedEntity keyvalue and
//			that Activate()/UpdateLevitation() resolve and act on it, but not
//			the exact force calculation ("ApplyLevitationForceOrConstraint" -
//			functional intent only). This applies a straightforward
//			continuous upward force to the attached entity's physics object,
//			scaled by this balloon's own F-Stop model scale, rather than a
//			physics constraint - simplest interpretation that matches the
//			observed "bigger balloon lifts more" puzzle behavior.
//
//=============================================================================//
#include "cbase.h"
#include "prop_levitator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define LEVITATOR_MODEL "models/props_gameplay/balloon.mdl"

ConVar levitator_base_lift_force( "levitator_base_lift_force", "400", FCVAR_REPLICATED, "Continuous upward force (on top of gravity) a prop_levitator applies to its AttachedEntity at scale 1.0. Scales with the levitator's current F-Stop scale." );

LINK_ENTITY_TO_CLASS( prop_levitator, CPropLevitator );

BEGIN_DATADESC( CPropLevitator )
	DEFINE_KEYFIELD( m_strAttachedEntity, FIELD_STRING, "AttachedEntity" ),
	DEFINE_FIELD( m_hAttachedEntity, FIELD_EHANDLE ),
	DEFINE_THINKFUNC( LevitationThink ),
END_DATADESC()

void CPropLevitator::Spawn( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		PrecacheModel( LEVITATOR_MODEL );
		SetModel( LEVITATOR_MODEL );
	}

	BaseClass::Spawn();

	ResolveAttachedEntity();

	SetContextThink( &CPropLevitator::LevitationThink, gpGlobals->curtime, "LevitationThink" );
}

void CPropLevitator::Activate( void )
{
	BaseClass::Activate();

	ResolveAttachedEntity();
}

void CPropLevitator::ResolveAttachedEntity( void )
{
	if ( m_strAttachedEntity != NULL_STRING )
	{
		m_hAttachedEntity = gEntList.FindEntityByName( NULL, m_strAttachedEntity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Continuously lift whatever we're attached to, harder the bigger
//			we've been scaled.
//-----------------------------------------------------------------------------
void CPropLevitator::LevitationThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.05f );

	CBaseEntity *pAttached = m_hAttachedEntity.Get();
	if ( pAttached )
	{
		IPhysicsObject *pPhys = pAttached->VPhysicsGetObject();
		if ( pPhys )
		{
			// Scaled by the target's own mass, like an upward acceleration
			// (in/s^2) rather than a fixed impulse, so it scales up against
			// gravity the same way regardless of what's attached - a big
			// enough balloon (high enough scale) can lift anything.
			float flAccel = levitator_base_lift_force.GetFloat() * GetModelScale();
			Vector vecLift( 0, 0, flAccel * pPhys->GetMass() );
			pPhys->ApplyForceCenter( vecLift * gpGlobals->frametime );
		}
	}
}
