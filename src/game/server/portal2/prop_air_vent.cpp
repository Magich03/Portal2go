//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: prop_air_vent - see prop_air_vent.h.
//
//=============================================================================//
#include "cbase.h"
#include "prop_air_vent.h"
#include "trigger_airvent_push.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define AIR_VENT_MODEL "models/props_lab/prop_airvent.mdl"

// The push column extends this far out from the vent face, and is this wide,
// at scale 1.0 - scaled up/down along with the vent's model scale.
static const Vector AIRVENT_TRIGGER_MINS( 0.0f, -16.0f, -16.0f );
static const Vector AIRVENT_TRIGGER_MAXS( 128.0f, 16.0f, 16.0f );

ConVar airvent_base_push_speed( "airvent_base_push_speed", "150", FCVAR_REPLICATED, "How hard a prop_air_vent pushes at scale 1.0. Scales with the vent's current F-Stop scale." );

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

void CPropAirVent::Activate( void )
{
	BaseClass::Activate();

	if ( !m_hPushTrigger.Get() )
	{
		CreatePushTrigger();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spawn our own push trigger, parented to us so it tracks our
//			position/angles automatically.
//-----------------------------------------------------------------------------
void CPropAirVent::CreatePushTrigger( void )
{
	CTriggerAirVentPush *pTrigger = static_cast<CTriggerAirVentPush*>(
		CBaseEntity::Create( "trigger_airvent_push", GetAbsOrigin(), GetAbsAngles(), this ) );

	if ( !pTrigger )
		return;

	pTrigger->SetParent( this );
	pTrigger->SetLocalOrigin( vec3_origin );
	pTrigger->SetLocalAngles( vec3_angle );
	pTrigger->SetOwnerEntity( this );

	m_hPushTrigger = pTrigger;

	UpdatePushTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: Resize the push volume and rescale its push speed to match our
//			current F-Stop model scale - a bigger vent blows a bigger, harder
//			column of air.
//-----------------------------------------------------------------------------
void CPropAirVent::UpdatePushTrigger( void )
{
	CTriggerAirVentPush *pTrigger = m_hPushTrigger.Get();
	if ( !pTrigger )
		return;

	float flScale = GetModelScale();

	// We have no brush model, so InitTrigger()'s SetSolid(SOLID_BSP) guess (it
	// only picks SOLID_VPHYSICS/BBOX-friendly solids when it sees a parent at
	// spawn time, which we don't have yet that early) doesn't apply to us -
	// force a plain bounding-box trigger and size it ourselves.
	pTrigger->SetSolid( SOLID_BBOX );
	UTIL_SetSize( pTrigger, AIRVENT_TRIGGER_MINS * flScale, AIRVENT_TRIGGER_MAXS * flScale );
	pTrigger->SetPushSpeed( airvent_base_push_speed.GetFloat() * flScale );
}

//-----------------------------------------------------------------------------
// Purpose: Don't blow air while we're being carried around as a preview.
//-----------------------------------------------------------------------------
void CPropAirVent::OnCameraCaptured( void )
{
	if ( m_hPushTrigger.Get() )
	{
		m_hPushTrigger->Disable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Placed back down (possibly at a new scale) - resize/rescale the
//			push volume to match and turn it back on.
//-----------------------------------------------------------------------------
void CPropAirVent::OnCameraPlaced( void )
{
	UpdatePushTrigger();

	if ( m_hPushTrigger.Get() )
	{
		m_hPushTrigger->Enable();
	}
}
