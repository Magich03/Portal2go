//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: prop_geyser - see prop_geyser.h.
//
//			Reconstructed from decompiled F-Stop/Exposure binaries, which
//			confirm the idle/pre-eruption/erupting state cycle, the 3 ConVar
//			durations, and 3 tiers (small/medium/large) of spout sounds, but
//			not the exact push force or size-tier thresholds - those are
//			this file's own reasonable defaults, scaled by the geyser's
//			current F-Stop model scale like prop_air_vent's push trigger.
//
//=============================================================================//
#include "cbase.h"
#include "prop_geyser.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GEYSER_MODEL "models/props_gameplay/geyser.mdl"

ConVar geyser_idle_time( "geyser_idle_time", "4", FCVAR_REPLICATED, "How long a prop_geyser sits idle between eruptions." );
ConVar geyser_pre_eruption_time( "geyser_pre_eruption_time", "1", FCVAR_REPLICATED, "How long a prop_geyser telegraphs before erupting." );
ConVar geyser_eruption_time( "geyser_eruption_time", "1.5", FCVAR_REPLICATED, "How long a prop_geyser's eruption phase (and its push) lasts." );
ConVar geyser_base_push_speed( "geyser_base_push_speed", "450", FCVAR_REPLICATED, "How hard a prop_geyser launches objects above it at scale 1.0. Scales with the geyser's current F-Stop scale." );
ConVar geyser_column_radius( "geyser_column_radius", "48", FCVAR_REPLICATED, "Horizontal radius of the column a prop_geyser pushes objects up through, at scale 1.0." );
ConVar geyser_column_height( "geyser_column_height", "512", FCVAR_REPLICATED, "How far above itself a prop_geyser reaches to find objects to push, at scale 1.0." );

LINK_ENTITY_TO_CLASS( prop_geyser, CPropGeyser );

void CPropGeyser::Spawn( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		PrecacheModel( GEYSER_MODEL );
		SetModel( GEYSER_MODEL );
	}

	BaseClass::Spawn();

	m_nState = GEYSER_IDLE;
	SetContextThink( &CPropGeyser::GeyserThink, gpGlobals->curtime + geyser_idle_time.GetFloat(), "GeyserThink" );
}

void CPropGeyser::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "geyser_spout_pre_small" );
	PrecacheScriptSound( "geyser_spout_pre_medium" );
	PrecacheScriptSound( "geyser_spout_pre_large" );
	PrecacheScriptSound( "geyser_spout_small" );
	PrecacheScriptSound( "geyser_spout_medium" );
	PrecacheScriptSound( "geyser_spout_large" );
}

//-----------------------------------------------------------------------------
// Purpose: Cycle idle -> pre-eruption -> erupting -> idle. The telegraph
//			sound plays going into pre-eruption; the actual push happens once,
//			at the moment eruption starts.
//-----------------------------------------------------------------------------
void CPropGeyser::GeyserThink( void )
{
	switch ( m_nState )
	{
	case GEYSER_IDLE:
		m_nState = GEYSER_PRE_ERUPTION;
		PlayPreEruptionSound();
		SetContextThink( &CPropGeyser::GeyserThink, gpGlobals->curtime + geyser_pre_eruption_time.GetFloat(), "GeyserThink" );
		break;

	case GEYSER_PRE_ERUPTION:
		m_nState = GEYSER_ERUPTING;
		PushObjectsAboveGeyser();
		SetContextThink( &CPropGeyser::GeyserThink, gpGlobals->curtime + geyser_eruption_time.GetFloat(), "GeyserThink" );
		break;

	case GEYSER_ERUPTING:
	default:
		m_nState = GEYSER_IDLE;
		SetContextThink( &CPropGeyser::GeyserThink, gpGlobals->curtime + geyser_idle_time.GetFloat(), "GeyserThink" );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Which of the 3 size tiers our current F-Stop scale falls into -
//			shared by the pre-eruption telegraph and the eruption itself so
//			they always agree.
//-----------------------------------------------------------------------------
static const char *GetGeyserSizeTier( float flScale )
{
	if ( flScale < 1.5f )
		return "small";
	if ( flScale < 2.5f )
		return "medium";
	return "large";
}

void CPropGeyser::PlayPreEruptionSound( void )
{
	char szSound[64];
	Q_snprintf( szSound, sizeof( szSound ), "geyser_spout_pre_%s", GetGeyserSizeTier( GetModelScale() ) );
	EmitSound( szSound );
}

//-----------------------------------------------------------------------------
// Purpose: Launch anything sitting in the column above us. Tiered spout
//			sound/size picked from our current F-Stop scale, matching how
//			bigger == harder everywhere else in this weapon's mechanics.
//-----------------------------------------------------------------------------
void CPropGeyser::PushObjectsAboveGeyser( void )
{
	float flScale = GetModelScale();

	char szSound[64];
	Q_snprintf( szSound, sizeof( szSound ), "geyser_spout_%s", GetGeyserSizeTier( flScale ) );
	EmitSound( szSound );

	float flRadius = geyser_column_radius.GetFloat() * flScale;
	float flHeight = geyser_column_height.GetFloat() * flScale;
	float flPushSpeed = geyser_base_push_speed.GetFloat() * flScale;

	Vector vecOrigin = GetAbsOrigin();
	Vector vecMins( vecOrigin.x - flRadius, vecOrigin.y - flRadius, vecOrigin.z );
	Vector vecMaxs( vecOrigin.x + flRadius, vecOrigin.y + flRadius, vecOrigin.z + flHeight );

	CBaseEntity *pList[ 64 ];
	int nCount = UTIL_EntitiesInBox( pList, ARRAYSIZE( pList ), vecMins, vecMaxs, 0 );

	for ( int i = 0; i < nCount; ++i )
	{
		CBaseEntity *pEntity = pList[ i ];
		if ( !pEntity || pEntity == this )
			continue;

		IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
		if ( pPhys )
		{
			Vector vecPush( 0, 0, flPushSpeed );
			pPhys->ApplyForceCenter( vecPush * pPhys->GetMass() );
			continue;
		}

		CBasePlayer *pPlayer = ToBasePlayer( pEntity );
		if ( pPlayer )
		{
			Vector vecVelocity = pPlayer->GetAbsVelocity();
			vecVelocity.z = flPushSpeed;
			pPlayer->SetAbsVelocity( vecVelocity );
		}
	}
}
