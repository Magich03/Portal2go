//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A brush-shaped magnetic zone - while enabled, continuously pulls
//			(positive) or pushes (negative) anything with physics currently
//			touching it, toward or away from the volume's center. A simpler,
//			mapper-drawn-shape cousin of prop_monopole's point-source field.
//
//=============================================================================//
#ifndef FUNC_MONOPOLE_FIELD_H
#define FUNC_MONOPOLE_FIELD_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "utlvector.h"

class CMonopoleField : public CBaseTrigger
{
	DECLARE_CLASS( CMonopoleField, CBaseTrigger );
	DECLARE_DATADESC();
public:

	virtual void Spawn( void );
	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );

	void InputToggle( inputdata_t &inputdata )         { m_bEnabled = !m_bEnabled; }
	void InputTurnOn( inputdata_t &inputdata )         { m_bEnabled = true; }
	void InputTurnOff( inputdata_t &inputdata )        { m_bEnabled = false; }
	void InputTogglePolarity( inputdata_t &inputdata ) { m_bPositive = !m_bPositive; }
	void InputTurnPositive( inputdata_t &inputdata )   { m_bPositive = true; }
	void InputTurnNegative( inputdata_t &inputdata )   { m_bPositive = false; }

	bool IsEnabled( void ) const  { return m_bEnabled; }
	bool IsPositive( void ) const { return m_bPositive; }

	void FieldThink( void );

private:
	bool					m_bEnabled;		// FGD: StartActive
	bool					m_bPositive;	// FGD: StartPositive
	CUtlVector<EHANDLE>		m_hTouching;
};

#endif // FUNC_MONOPOLE_FIELD_H
