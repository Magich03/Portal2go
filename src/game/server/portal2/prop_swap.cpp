//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: prop_swap - see prop_swap.h. Reconstructed from decompiled
//			F-Stop/Exposure binaries (confirmed classname/field/input names:
//			CPropSwap, m_swapModel, Swap, SwapModel); this codebase's own
//			VPhysicsDestroyObject()/SetModel()/CreateVPhysics() are used to
//			actually rebuild physics for the new model, since the exact
//			original rebuild call didn't survive the decompile.
//
//=============================================================================//
#include "cbase.h"
#include "prop_swap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( prop_swap, CPropSwap );

BEGIN_DATADESC( CPropSwap )
	DEFINE_KEYFIELD( m_iszSwapModel, FIELD_STRING, "swapmodel" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Swap", InputSwap ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SwapModel", InputSwapModel ),
END_DATADESC()

void CPropSwap::Precache( void )
{
	BaseClass::Precache();

	if ( m_iszSwapModel != NULL_STRING )
	{
		PrecacheModel( STRING( m_iszSwapModel ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Swap to whatever model is currently stored in m_iszSwapModel,
//			remembering the model we just swapped away from so the next
//			Swap toggles back to it.
//-----------------------------------------------------------------------------
void CPropSwap::Swap( void )
{
	if ( m_iszSwapModel == NULL_STRING )
		return;

	string_t iszOldModel = GetModelName();

	VPhysicsDestroyObject();
	SetModel( STRING( m_iszSwapModel ) );
	CreateVPhysics();

	m_iszSwapModel = iszOldModel;
}

void CPropSwap::InputSwap( inputdata_t &inputdata )
{
	Swap();
}

void CPropSwap::InputSwapModel( inputdata_t &inputdata )
{
	m_iszSwapModel = AllocPooledString( inputdata.value.String() );
	Swap();
}
