//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A capturable miniature building prop that resolves a
//			target_portal keyvalue to an entity handle. The decompiled
//			binary confirms the keyvalue and that it's resolved on Activate,
//			but nothing about what it's then used for survived
//			reconstruction, so this deliberately stops there rather than
//			inventing gameplay behavior the evidence doesn't support -
//			m_hTargetPortal is exposed for a future mechanic to consume.
//
//=============================================================================//
#ifndef PROP_BUILDING_H
#define PROP_BUILDING_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

class CPropBuilding : public CPhysicsProp
{
	DECLARE_CLASS( CPropBuilding, CPhysicsProp );
	DECLARE_DATADESC();
public:

	virtual void Spawn( void );
	virtual void Activate( void );

	CBaseEntity *GetTargetPortal( void ) { return m_hTargetPortal.Get(); }

private:
	void ResolveTargetPortal( void );

	string_t	m_strTargetPortal;	// FGD: target_portal
	EHANDLE		m_hTargetPortal;
};

#endif // PROP_BUILDING_H
