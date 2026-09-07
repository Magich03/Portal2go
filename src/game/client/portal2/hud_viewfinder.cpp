//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: F-Stop camera viewfinder HUD - shown while weapon_camera or
//			weapon_placement is the active weapon. Draws a simple camera
//			framing overlay, and a 3-slot photo inventory row (matching
//			CPhotoInventory's MAX_HELD_PHOTOS - see photo_inventory.h) bound
//			to the 3 _rt_LargePhoto0/1/2 client render targets. Each slot is
//			rendered into by CViewRender::RenderPhotoSnapshot() - see
//			viewrender.cpp - in response to the server's TakePhoto
//			usermessage (weapon_camera.cpp's SendPhotoSnapshot()); whether a
//			slot is currently occupied comes straight from the local
//			player's networked photo stack count, so it never depends on
//			messages arriving in order.
//
//			Reconstructed from the F-Stop decompile's CHudViewfinder, whose
//			surviving shell only described DrawCameraFrame()/
//			DrawPhotoInventoryStatus() by name - the actual drawing here is a
//			from-scratch reimplementation using stock vgui primitives, since
//			no F-Stop HUD art (HUD/hud_icon_camera, HUD/hud_icon_picture)
//			ships in this repo.
//
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "c_baseplayer.h"
#include "c_basecombatweapon.h"
#include "c_portal_player.h"
#include "viewrender.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define NUM_LARGE_PHOTO_SLOTS 3

class CHudViewfinder : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudViewfinder, vgui::Panel );

public:
	explicit CHudViewfinder( const char *pElementName );

	virtual void	Init( void );
	virtual void	VidInit( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool	ShouldDraw( void );
	virtual void	Paint( void );

	bool MsgFunc_TakePhoto( const CUsrMsg_TakePhoto &msg );

	CUserMessageBinder m_UMCMsgTakePhoto;

private:
	void DrawCameraFrame( void );
	void DrawPhotoInventoryStatus( void );

	int		m_nPhotoTextureID[ NUM_LARGE_PHOTO_SLOTS ];
};

DECLARE_HUDELEMENT( CHudViewfinder );
DECLARE_HUD_MESSAGE( CHudViewfinder, TakePhoto );

CHudViewfinder::CHudViewfinder( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudViewfinder" )
{
	vgui::Panel *pParent = GetClientMode()->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i = 0; i < NUM_LARGE_PHOTO_SLOTS; ++i )
	{
		m_nPhotoTextureID[ i ] = -1;
	}

	SetPaintBackgroundEnabled( false );
}

void CHudViewfinder::Init( void )
{
	HOOK_HUD_MESSAGE( CHudViewfinder, TakePhoto );
}

void CHudViewfinder::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetPaintBackgroundEnabled( false );
	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
}

void CHudViewfinder::VidInit( void )
{
	char szMaterialName[64];

	for ( int i = 0; i < NUM_LARGE_PHOTO_SLOTS; ++i )
	{
		if ( m_nPhotoTextureID[ i ] == -1 )
		{
			m_nPhotoTextureID[ i ] = vgui::surface()->CreateNewTextureID();
		}

		Q_snprintf( szMaterialName, sizeof( szMaterialName ), "vgui/hud/photo_slot%d", i );
		vgui::surface()->DrawSetTextureFile( m_nPhotoTextureID[ i ], szMaterialName, true, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Only while actively taking or placing a photo, matching the
//			F-Stop reconstruction's ShouldDraw().
//-----------------------------------------------------------------------------
bool CHudViewfinder::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if ( !pWeapon )
		return false;

	return ( Q_stricmp( pWeapon->GetClassname(), "weapon_camera" ) == 0 ) ||
		   ( Q_stricmp( pWeapon->GetClassname(), "weapon_placement" ) == 0 );
}

//-----------------------------------------------------------------------------
// Purpose: The server tells us a photo was just taken and which render
//			target slot it's going into - queue the actual render, done next
//			frame alongside the main view (see CViewRender::
//			CheckPendingPhotoSnapshot()). Whether that slot then shows as
//			occupied in the HUD comes from the networked stack count in
//			Paint(), not from this message, so a missed/reordered message
//			can't leave the HUD out of sync with the server.
//-----------------------------------------------------------------------------
bool CHudViewfinder::MsgFunc_TakePhoto( const CUsrMsg_TakePhoto &msg )
{
	int nSlot = msg.slot();
	if ( nSlot < 0 || nSlot >= NUM_LARGE_PHOTO_SLOTS )
		return true;

	CViewRender::QueuePhotoSnapshot( nSlot );

	return true;
}

void CHudViewfinder::Paint( void )
{
	DrawCameraFrame();
	DrawPhotoInventoryStatus();
}

//-----------------------------------------------------------------------------
// Purpose: Simple corner-bracket camera reticle over the middle of the
//			screen, standing in for the missing HUD/hud_icon_camera art.
//-----------------------------------------------------------------------------
void CHudViewfinder::DrawCameraFrame( void )
{
	int wide, tall;
	GetSize( wide, tall );

	int nFrameWide = wide / 3;
	int nFrameTall = tall / 3;
	int x0 = ( wide - nFrameWide ) / 2;
	int y0 = ( tall - nFrameTall ) / 2;
	int x1 = x0 + nFrameWide;
	int y1 = y0 + nFrameTall;
	int nBracket = 16;

	surface()->DrawSetColor( 255, 255, 255, 200 );

	// Top-left
	surface()->DrawFilledRect( x0, y0, x0 + nBracket, y0 + 2 );
	surface()->DrawFilledRect( x0, y0, x0 + 2, y0 + nBracket );
	// Top-right
	surface()->DrawFilledRect( x1 - nBracket, y0, x1, y0 + 2 );
	surface()->DrawFilledRect( x1 - 2, y0, x1, y0 + nBracket );
	// Bottom-left
	surface()->DrawFilledRect( x0, y1 - 2, x0 + nBracket, y1 );
	surface()->DrawFilledRect( x0, y1 - nBracket, x0 + 2, y1 );
	// Bottom-right
	surface()->DrawFilledRect( x1 - nBracket, y1 - 2, x1, y1 );
	surface()->DrawFilledRect( x1 - 2, y1 - nBracket, x1, y1 );
}

//-----------------------------------------------------------------------------
// Purpose: A row of 3 slots, bottom-right corner, one per _rt_LargePhotoN -
//			standing in for the missing HUD/hud_icon_picture art. A slot is
//			occupied (shows its rendered thumbnail) if its index is below the
//			local player's current photo stack count; otherwise it's drawn as
//			an empty outline. Slot 0 is the oldest held photo (placed last),
//			matching CPhotoInventory's stack order.
//-----------------------------------------------------------------------------
void CHudViewfinder::DrawPhotoInventoryStatus( void )
{
	C_Portal_Player *pPlayer = ToPortalPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer )
		return;

	int nStackCount = pPlayer->GetPhotoStackCount();

	int wide, tall;
	GetSize( wide, tall );

	int nThumbSize = 96;
	int nGap = 12;
	int nTotalWide = NUM_LARGE_PHOTO_SLOTS * nThumbSize + ( NUM_LARGE_PHOTO_SLOTS - 1 ) * nGap;
	int x0 = wide - nTotalWide - 24;
	int y0 = tall - nThumbSize - 24;

	for ( int i = 0; i < NUM_LARGE_PHOTO_SLOTS; ++i )
	{
		int x = x0 + i * ( nThumbSize + nGap );

		if ( i < nStackCount )
		{
			surface()->DrawSetColor( 255, 255, 255, 255 );
			surface()->DrawSetTexture( m_nPhotoTextureID[ i ] );
			surface()->DrawTexturedRect( x, y0, x + nThumbSize, y0 + nThumbSize );
			surface()->DrawSetColor( 255, 255, 255, 255 );
		}
		else
		{
			surface()->DrawSetColor( 255, 255, 255, 60 );
		}

		surface()->DrawOutlinedRect( x, y0, x + nThumbSize, y0 + nThumbSize );
	}
}
