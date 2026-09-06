//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A decorative air vent prop. Its gameplay effect (pushing the
//			player/objects) comes from a separate trigger_airvent_push volume
//			placed alongside it - this entity is just the visual model, same
//			as the F-Stop/Exposure decompile shows it (a bare CBaseAnimating
//			that defaults its model, nothing more).
//
//=============================================================================//
#ifndef PROP_AIR_VENT_H
#define PROP_AIR_VENT_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

class CPropAirVent : public CDynamicProp
{
	DECLARE_CLASS( CPropAirVent, CDynamicProp );
public:

	virtual void Spawn( void );
};

#endif // PROP_AIR_VENT_H
