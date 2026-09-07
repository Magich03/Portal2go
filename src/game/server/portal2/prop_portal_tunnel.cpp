//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: prop_portal_tunnel - see prop_portal_tunnel.h.
//
//=============================================================================//
#include "cbase.h"
#include "prop_portal_tunnel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// No F-Stop portal tunnel art ships in this repo - "models/props_gameplay/
// aperture_door_frame.mdl" per the FGD.
#define PORTAL_TUNNEL_MODEL "models/props_gameplay/aperture_door_frame.mdl"

LINK_ENTITY_TO_CLASS( prop_portal_tunnel, CPropPortalTunnel );

BEGIN_DATADESC( CPropPortalTunnel )
	DEFINE_KEYFIELD( m_strHorizontalFailTarget, FIELD_STRING, "failtarget" ),
	DEFINE_KEYFIELD( m_strFloorFailTarget, FIELD_STRING, "failtarget_floor" ),
	DEFINE_KEYFIELD( m_strCeilingFailTarget, FIELD_STRING, "failtarget_ceiling" ),
	DEFINE_KEYFIELD( m_strSuccessTarget, FIELD_STRING, "successtarget" ),
END_DATADESC()

void CPropPortalTunnel::Spawn( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		PrecacheModel( PORTAL_TUNNEL_MODEL );
		SetModel( PORTAL_TUNNEL_MODEL );
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Floor/ceiling if we ended up lying flat (facing mostly up or
//			down), horizontal otherwise.
//-----------------------------------------------------------------------------
CPropPortalTunnel::PlacementCase_t CPropPortalTunnel::ClassifyOrientation( void )
{
	Vector vecUp;
	AngleVectors( GetAbsAngles(), NULL, NULL, &vecUp );

	const float FLAT_THRESHOLD = 0.7f;

	if ( vecUp.z > FLAT_THRESHOLD )
		return CASE_FLOOR;

	if ( vecUp.z < -FLAT_THRESHOLD )
		return CASE_CEILING;

	return CASE_HORIZONTAL;
}

void CPropPortalTunnel::OnCameraPlaced( void )
{
	// Reaching a real placement already means weapon_placement's generic
	// hull-fit check passed - we don't have the deeper photo-placement-query
	// system that would let this actually reject a spot, so this always
	// counts as success. See the header for why the fail targets below are
	// wired but currently unreachable.
	EvaluatePlacementResult( true, ClassifyOrientation() );
}

void CPropPortalTunnel::EvaluatePlacementResult( bool bSuccess, PlacementCase_t nCase )
{
	if ( bSuccess )
	{
		FireTargets( STRING( m_strSuccessTarget ), this, this, USE_TOGGLE, 0 );
		return;
	}

	switch ( nCase )
	{
	case CASE_FLOOR:
		FireTargets( STRING( m_strFloorFailTarget ), this, this, USE_TOGGLE, 0 );
		break;
	case CASE_CEILING:
		FireTargets( STRING( m_strCeilingFailTarget ), this, this, USE_TOGGLE, 0 );
		break;
	default:
		FireTargets( STRING( m_strHorizontalFailTarget ), this, this, USE_TOGGLE, 0 );
		break;
	}
}
