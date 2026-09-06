//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: trigger_airvent_push - see trigger_airvent_push.h.
//
//			Reconstructed from decompiled F-Stop/Exposure binaries, which
//			confirm the classname and the "push in local forward direction"
//			shape, but not the exact original push implementation. This
//			borrows the real, proven continuous-push logic already used by
//			this codebase's own trigger_push (CTriggerPush::Touch() in
//			triggers.cpp) rather than guessing at a new one, since that class
//			isn't declared in a header and can't be subclassed directly.
//
//=============================================================================//
#include "cbase.h"
#include "trigger_airvent_push.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( trigger_airvent_push, CTriggerAirVentPush );

BEGIN_DATADESC( CTriggerAirVentPush )
	DEFINE_KEYFIELD( m_flPushSpeed, FIELD_FLOAT, "pushspeed" ),
END_DATADESC()

void CTriggerAirVentPush::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	if ( m_flPushSpeed == 0 )
	{
		m_flPushSpeed = 100.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Push pOther along our forward direction. Mirrors the continuous
//			(non-once) branches of CTriggerPush::Touch().
//-----------------------------------------------------------------------------
void CTriggerAirVentPush::Touch( CBaseEntity *pOther )
{
	if ( !pOther || !pOther->IsSolid() )
		return;

	if ( pOther->GetMoveType() == MOVETYPE_PUSH || pOther->GetMoveType() == MOVETYPE_NONE )
		return;

	if ( !PassesTriggerFilters( pOther ) )
		return;

	if ( pOther->GetMoveParent() )
		return;

	Vector vecForward;
	AngleVectors( GetAbsAngles(), &vecForward );
	Vector vecPush = m_flPushSpeed * vecForward;

	if ( pOther->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		const float DEFAULT_MASS = 100.0f;
		IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
		if ( pPhys )
		{
			pPhys->ApplyForceCenter( vecPush * DEFAULT_MASS * gpGlobals->frametime );
		}
		return;
	}

	if ( pOther->GetFlags() & FL_BASEVELOCITY )
	{
		vecPush += pOther->GetBaseVelocity();
	}

	if ( vecPush.z > 0 && ( pOther->GetFlags() & FL_ONGROUND ) )
	{
		pOther->SetGroundEntity( NULL );
		Vector vecOrigin = pOther->GetAbsOrigin();
		vecOrigin.z += 1.0f;
		pOther->SetAbsOrigin( vecOrigin );
	}

	pOther->SetBaseVelocity( vecPush );
	pOther->AddFlag( FL_BASEVELOCITY );
}
