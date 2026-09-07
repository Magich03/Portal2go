//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: item_photograph - the "polaroid" a player is holding between
//			weapon_camera's capture and weapon_placement's first click (see
//			photo_inventory.h's PHOTOMODE_POLAROID). It's never drawn in the
//			world (EF_NODRAW) - its only job is to network the name of the
//			client render target (_rt_LargePhoto0/1/2) holding the snapshot
//			taken at the moment of capture, for the HUD viewfinder's polaroid
//			thumbnail (and, via CPhotoMaterialProxy in c_item_photo.cpp, for
//			any future in-world display). It's discarded the moment
//			CPhotoInventory::SpawnGhost() brings the real captured object
//			back out - see weapon_camera.cpp/weapon_placement.cpp.
//
//=============================================================================//
#ifndef ITEM_PHOTOGRAPH_H
#define ITEM_PHOTOGRAPH_H
#ifdef _WIN32
#pragma once
#endif

#include "baseanimating.h"

class CItem_Photograph : public CBaseAnimating
{
	DECLARE_CLASS( CItem_Photograph, CBaseAnimating );
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
