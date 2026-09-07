//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A capturable prop that fires a named target once it's actually
//			placed, classified by whether it ended up lying flat as a floor,
//			flat as a ceiling, or upright. The decompiled binary confirms
//			this classification and its 4 named targets, but not the actual
//			condition that decides "success" vs. "fail" (a deeper photo-
//			placement-query system this repo doesn't have) - so for now,
//			reaching a real placement (weapon_placement's generic hull-fit
//			check already passed) always counts as success and fires
//			successtarget; the 3 fail targets are wired but never fire yet,
//			reserved for whatever validation eventually decides a placement
//			should be rejected.
//
//=============================================================================//
#ifndef PROP_PORTAL_TUNNEL_H
#define PROP_PORTAL_TUNNEL_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

class CPropPortalTunnel : public CPhysicsProp
{
	DECLARE_CLASS( CPropPortalTunnel, CPhysicsProp );
	DECLARE_DATADESC();
public:
	enum PlacementCase_t
	{
		CASE_HORIZONTAL = 0,
		CASE_FLOOR,
		CASE_CEILING,
	};

	virtual void Spawn( void );
	virtual void OnCameraPlaced( void );

private:
	void EvaluatePlacementResult( bool bSuccess, PlacementCase_t nCase );
	PlacementCase_t ClassifyOrientation( void );

	string_t	m_strHorizontalFailTarget;	// FGD: failtarget
	string_t	m_strFloorFailTarget;		// FGD: failtarget_floor
	string_t	m_strCeilingFailTarget;		// FGD: failtarget_ceiling
	string_t	m_strSuccessTarget;			// FGD: successtarget
};

#endif // PROP_PORTAL_TUNNEL_H
