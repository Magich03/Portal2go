//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A capturable geyser that cycles idle -> pre-eruption -> erupting
//			-> idle, launching anything above it upward during the eruption
//			phase. Being a normal CPhysicsProp, it's picked up/placed/scaled
//			by weapon_camera and weapon_placement like any other prop - a
//			bigger geyser (placed at a higher F-Stop scale) erupts harder.
//
//=============================================================================//
#ifndef PROP_GEYSER_H
#define PROP_GEYSER_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

class CPropGeyser : public CPhysicsProp
{
	DECLARE_CLASS( CPropGeyser, CPhysicsProp );
public:
	enum GeyserState_t
	{
		GEYSER_IDLE = 0,
		GEYSER_PRE_ERUPTION,
		GEYSER_ERUPTING,
	};

	virtual void Spawn( void );
	virtual void Precache( void );

	void GeyserThink( void );

private:
	void PlayPreEruptionSound( void );
	void PushObjectsAboveGeyser( void );

	GeyserState_t m_nState;
};

#endif // PROP_GEYSER_H
