//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A capturable point-source magnet: while enabled, continuously
//			pulls (positive) or pushes (negative) nearby physics objects
//			toward or away from itself.
//
//=============================================================================//
#ifndef PROP_MONOPOLE_H
#define PROP_MONOPOLE_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

class CMonopole : public CBaseAnimating
{
	DECLARE_CLASS( CMonopole, CBaseAnimating );
	DECLARE_DATADESC();
public:

	virtual void Spawn( void );

	void InputToggle( inputdata_t &inputdata )         { SetEnabled( !m_bEnabled ); }
	void InputTurnOn( inputdata_t &inputdata )         { SetEnabled( true ); }
	void InputTurnOff( inputdata_t &inputdata )        { SetEnabled( false ); }
	void InputTogglePolarity( inputdata_t &inputdata ) { SetPositive( !m_bPositive ); }
	void InputTurnPositive( inputdata_t &inputdata )   { SetPositive( true ); }
	void InputTurnNegative( inputdata_t &inputdata )   { SetPositive( false ); }

	void SetEnabled( bool bEnabled )   { m_bEnabled = bEnabled; }
	void SetPositive( bool bPositive ) { m_bPositive = bPositive; }

	void MonopoleThink( void );

private:
	float		m_flMassScale;			// FGD: massScale
	string_t	m_iszOverrideScript;	// FGD: overridescript (unused - no known consumer)
	int			m_iMaxObjectsAttached;	// FGD: maxobjects
	float		m_flForceLimit;			// FGD: forcelimit
	float		m_flTorqueLimit;		// FGD: torquelimit (unused - we don't apply torque)
	bool		m_bEnabled;				// FGD: StartActive
	bool		m_bPositive;			// FGD: StartPositive
};

#endif // PROP_MONOPOLE_H
