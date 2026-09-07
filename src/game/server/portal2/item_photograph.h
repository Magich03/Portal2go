//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: item_photograph - a real photograph object taken by weapon_camera
//			when there's nothing directly capturable in view. It networks the
//			name of the client render target (_rt_LargePhoto0/1/2) holding the
//			snapshot that was rendered at the moment of capture, which the
//			client's CPhotoMaterialProxy (c_item_photo.cpp) binds as its
//			$basetexture. Once spawned it plugs into the exact same
//			CPhotoInventory pickup/scale/placement pipeline as any other
//			CanBeCaptured prop (see weapon_camera.cpp/photo_inventory.cpp).
//
//=============================================================================//
#ifndef ITEM_PHOTOGRAPH_H
#define ITEM_PHOTOGRAPH_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

class CItem_Photograph : public CPhysicsProp
{
	DECLARE_CLASS( CItem_Photograph, CPhysicsProp );
	DECLARE_SERVERCLASS();

public:
	virtual void Spawn( void );

	void SetPhotoTexture( const char *pszTextureName );

private:
	char m_szTextureName[ 260 ];
};

// Spawns an item_photograph at the given transform, already carrying the
// named snapshot texture. Convenience wrapper for weapon_camera.cpp.
CItem_Photograph *CreatePhotograph( const Vector &vecOrigin, const QAngle &angOrigin, const char *pszTextureName );

#endif // ITEM_PHOTOGRAPH_H
