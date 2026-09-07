//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A capturable balloon that continuously lifts whatever entity its
//			AttachedEntity keyvalue points at. Being scaled up by
//			weapon_placement (like any other captured prop) makes it lift
//			harder, matching the observed F-Stop/Exposure puzzle pattern of
//			scaling a balloon up to make it carry heavier cargo.
//
//=============================================================================//
#ifndef PROP_LEVITATOR_H
#define PROP_LEVITATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

class CPropLevitator : public CPhysicsProp
{
	DECLARE_CLASS( CPropLevitator, CPhysicsProp );
	DECLARE_DATADESC();
public:

	virtual void Spawn( void );
	virtual void Activate( void );

private:
	void LevitationThink( void );
	void ResolveAttachedEntity( void );

	string_t				m_strAttachedEntity;	// FGD: AttachedEntity
	EHANDLE					m_hAttachedEntity;
};

#endif // PROP_LEVITATOR_H
