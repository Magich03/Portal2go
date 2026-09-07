//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Portal mod render targets are specified by and accessable through this singleton
//
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#ifndef PORTALRENDERTARGETS_H_
#define PORTALRENDERTARGETS_H_
#ifdef _WIN32
#pragma once
#endif

#include "baseclientrendertargets.h" // Base class, with interfaces called by engine and inherited members to init common render targets

#ifndef PORTAL 
#pragma message ( "This file should only be built with portal builds" )
#endif

// externs
class IMaterialSystem;
class IMaterialSystemHardwareConfig;

// F-Stop photograph render targets - the "camera" renders a snapshot of the
// player's view into one of these (round-robin) whenever a photo is taken.
// Reconstructed from the F-Stop decompile as _rt_LargePhoto0/1/2, 256x256.
#define NUM_LARGE_PHOTO_SLOTS 3

class CPortalRenderTargets : public CBaseClientRenderTargets
{
	// no networked vars
	DECLARE_CLASS_GAMEROOT( CPortalRenderTargets, CBaseClientRenderTargets );
public:
	virtual void InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig );
	virtual void ShutdownClientRenderTargets();

	ITexture* GetPortal1Texture( void );
	ITexture* GetPortal2Texture( void );
	ITexture* GetDepthDoublerTexture( void );
	ITexture* GetLargePhotoTexture( int nSlot );

	//recursive views require different water textures

private:
	CTextureReference m_Portal1Texture;
	CTextureReference m_Portal2Texture;
	CTextureReference m_DepthDoublerTexture;
	CTextureReference m_LargePhotoTexture[ NUM_LARGE_PHOTO_SLOTS ];


	ITexture* InitPortal1Texture ( IMaterialSystem* pMaterialSystem );
	ITexture* InitPortal2Texture ( IMaterialSystem* pMaterialSystem );
	ITexture* InitDepthDoublerTexture ( IMaterialSystem* pMaterialSystem );
	ITexture* InitLargePhotoTexture ( IMaterialSystem* pMaterialSystem, int nSlot );

	void InitPortalWaterTextures ( IMaterialSystem* pMaterialSystem );

};

extern CPortalRenderTargets* portalrendertargets;

const char *GetSubTargetNameForPortalRecursionLevel( int iRecursionLevel );

#endif //PORTALRENDERTARGETS_H_