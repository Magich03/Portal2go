//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A capturable prop that, instead of being picked up into the
//			camera like a normal object, swaps places with the player: the
//			player teleports to where the prop was, and the prop teleports
//			to where the player was standing.
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

	// Swaps this prop's position/angles with pPlayer's, if both ends of the
	// swap are clear. Returns false (and does nothing) if either isn't.
	bool SwapWithPlayer( CBasePlayer *pPlayer );

	void InputSwap( inputdata_t &inputdata );

	COutputEvent m_OnSwap;

private:
	bool IsClearForProp( const Vector &vecOrigin );
};

#endif // PROP_SWAP_H
