//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A capturable trap that, on its Snap input, launches whatever's
//			currently sitting in front of it up and away.
//
//=============================================================================//
#ifndef PROP_MOUSETRAP_H
#define PROP_MOUSETRAP_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

class CPropMousetrap : public CPhysicsProp
{
	DECLARE_CLASS( CPropMousetrap, CPhysicsProp );
	DECLARE_DATADESC();
public:

	virtual void Spawn( void );
	virtual void Precache( void );

	void InputSnap( inputdata_t &inputdata );

private:
	void Snap( void );
	CBaseEntity *FindObjectInTrapArea( void );
};

#endif // PROP_MOUSETRAP_H
