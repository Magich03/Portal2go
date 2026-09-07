//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A capturable air vent. Owns its push volume automatically (no
//			separate mapper-placed trigger needed) and, being a normal
//			CBaseAnimating, gets picked up/placed/scaled by weapon_camera and
//			weapon_placement like any other prop - except placing it back
//			down at a different scale resizes its push volume and makes it
//			blow harder or softer to match.
//
//=============================================================================//
#ifndef PROP_AIR_VENT_H
#define PROP_AIR_VENT_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

class CTriggerAirVentPush;

class CPropAirVent : public CDynamicProp
{
	DECLARE_CLASS( CPropAirVent, CDynamicProp );
public:

	virtual void Spawn( void );
	virtual void Activate( void );

	// CBaseAnimating capture hooks (see baseanimating.h). A vent needs to
	// keep reacting (disabling its push trigger, then resizing it) the whole
	// time it's held, so it skips weapon_camera's polaroid/ghost step and
	// stays carried+previewed directly like before that step existed.
	virtual bool UsesDirectCapture( void ) { return true; }
	virtual void OnCameraCaptured( void );
	virtual void OnCameraPlaced( void );

private:
	void CreatePushTrigger( void );
	void UpdatePushTrigger( void );

	CHandle<CTriggerAirVentPush> m_hPushTrigger;
};

#endif // PROP_AIR_VENT_H
