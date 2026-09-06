//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A volume that continuously pushes anything touching it along its
//			own forward direction (its angles), meant to be placed over a
//			prop_air_vent. Functionally a simpler, always-continuous cousin
//			of trigger_push with a fixed local-forward push direction instead
//			of a separate pushdir keyvalue.
//
//=============================================================================//
#ifndef TRIGGER_AIRVENT_PUSH_H
#define TRIGGER_AIRVENT_PUSH_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

class CTriggerAirVentPush : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerAirVentPush, CBaseTrigger );
	DECLARE_DATADESC();
public:

	virtual void Spawn( void );
	virtual void Touch( CBaseEntity *pOther );

	void SetPushSpeed( float flPushSpeed ) { m_flPushSpeed = flPushSpeed; }
	float GetPushSpeed( void ) const { return m_flPushSpeed; }

private:
	float m_flPushSpeed;
};

#endif // TRIGGER_AIRVENT_PUSH_H
