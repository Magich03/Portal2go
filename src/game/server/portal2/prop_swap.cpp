//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: prop_swap - see prop_swap.h.
//
//			Reconstructed decompiles of the F-Stop/Exposure binaries turned
//			up a CPropSwap with a "swap the model" reading (m_swapModel,
//			SwapModel), but that's not how this prop is meant to work here:
//			capturing one with weapon_camera swaps the player's position
//			with the prop's, instead of picking it up like a normal object.
//
//=============================================================================//
#include "cbase.h"
#include "prop_swap.h"
#include "gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( prop_swap, CPropSwap );

BEGIN_DATADESC( CPropSwap )
	DEFINE_INPUTFUNC( FIELD_VOID, "Swap", InputSwap ),
	DEFINE_OUTPUT( m_OnSwap, "OnSwap" ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Is there room for this prop (its own collision bounds) at vecOrigin?
//-----------------------------------------------------------------------------
bool CPropSwap::IsClearForProp( const Vector &vecOrigin )
{
	ICollideable *pCollide = CollisionProp();
	Vector vecMins = pCollide ? pCollide->OBBMins() : -Vector( 8, 8, 8 );
	Vector vecMaxs = pCollide ? pCollide->OBBMaxs() :  Vector( 8, 8, 8 );

	trace_t tr;
	Ray_t ray;
	ray.Init( vecOrigin, vecOrigin, vecMins, vecMaxs );
	CTraceFilterSimple filter( this, COLLISION_GROUP_NONE );
	UTIL_TraceRay( ray, MASK_SOLID, &filter, &tr );

	return !tr.startsolid && !tr.allsolid;
}

//-----------------------------------------------------------------------------
// Purpose: Swap this prop's position with the player's, if both new spots
//			are clear.
//-----------------------------------------------------------------------------
bool CPropSwap::SwapWithPlayer( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return false;

	Vector vecPropOrigin = GetAbsOrigin();
	Vector vecPlayerOrigin = pPlayer->GetAbsOrigin();

	// The player needs room to stand at the prop's spot, and the prop needs
	// room to sit at the player's spot, or we don't swap either of them.
	if ( !g_pGameRules->IsSpawnPointValid( this, pPlayer ) )
		return false;

	if ( !IsClearForProp( vecPlayerOrigin ) )
		return false;

	// Only positions swap - the player keeps their view angles and the prop
	// keeps whatever orientation it already had.
	pPlayer->Teleport( &vecPropOrigin, NULL, &vec3_origin );
	Teleport( &vecPlayerOrigin, NULL, &vec3_origin );

	m_OnSwap.FireOutput( pPlayer, this );

	return true;
}

void CPropSwap::InputSwap( inputdata_t &inputdata )
{
	CBasePlayer *pPlayer = ToBasePlayer( inputdata.pActivator );
	if ( !pPlayer )
	{
		pPlayer = UTIL_GetLocalPlayer();
	}

	SwapWithPlayer( pPlayer );
}
