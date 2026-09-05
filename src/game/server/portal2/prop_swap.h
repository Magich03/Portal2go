//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A physics prop that can swap its model (and rebuild physics for
//			the new shape) on command, remembering the model it swapped away
//			from so a second Swap toggles back.
//
//=============================================================================//
#ifndef PROP_SWAP_H
#define PROP_SWAP_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

class CPropSwap : public CPhysicsProp
{
	DECLARE_CLASS( CPropSwap, CPhysicsProp );
	DECLARE_DATADESC();
public:

	virtual void Precache( void );

	void InputSwap( inputdata_t &inputdata );
	void InputSwapModel( inputdata_t &inputdata );

private:
	void Swap( void );

	string_t m_iszSwapModel;
};

#endif // PROP_SWAP_H
