//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: A volumetric trigger that erases whatever photos a player is
//			currently holding, dumping every stashed/ghosted object straight
//			back into the world roughly where it already is (see
//			CPhotoInventory::ReturnPhotosToWorld()).
//
//=============================================================================//
#ifndef TRIGGER_PHOTO_ERASER_H
#define TRIGGER_PHOTO_ERASER_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

class CTriggerPhotoEraser : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerPhotoEraser, CBaseTrigger );
	DECLARE_DATADESC();
public:

	virtual void Spawn( void );
	virtual void StartTouch( CBaseEntity *pOther );

	COutputEvent m_OnObjectsFizzled;
};

#endif // TRIGGER_PHOTO_ERASER_H
