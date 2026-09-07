//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: F-Stop camera viewfinder HUD - shown while weapon_camera or
//			weapon_placement is the active weapon. Draws a simple camera
//			framing overlay, and a thumbnail of the most recently taken
//			photograph (rendered into one of the _rt_LargePhoto0/1/2 client
//			render targets by CViewRender::RenderPhotoSnapshot() - see
//			viewrender.cpp - in response to the server's TakePhoto
//			usermessage, sent from weapon_camera.cpp's SendPhotoSnapshot()).
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
#include "viewrender.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define NUM_LARGE_PHOTO_SLOTS 3
#define PHOTO_THUMBNAIL_FADE_TIME 4.0f

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
	void DrawPhotoThumbnail( void );

	int		m_nPhotoTextureID[ NUM_LARGE_PHOTO_SLOTS ];
	int		m_nLastPhotoSlot;
	float	m_flLastPhotoTime;
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

	m_nLastPhotoSlot = -1;
	m_flLastPhotoTime = -1.0f;

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
//			target slot it's going into - queue the actual render (done next
//			frame, alongside the main view - see CViewRender::
//			CheckPendingPhotoSnapshot()) and remember it for our thumbnail.
//-----------------------------------------------------------------------------
bool CHudViewfinder::MsgFunc_TakePhoto( const CUsrMsg_TakePhoto &msg )
{
	int nSlot = msg.slot();
	if ( nSlot < 0 || nSlot >= NUM_LARGE_PHOTO_SLOTS )
		return true;

	CViewRender::QueuePhotoSnapshot( nSlot );

	m_nLastPhotoSlot = nSlot;
	m_flLastPhotoTime = gpGlobals->curtime;

	return true;
}

void CHudViewfinder::Paint( void )
{
	DrawCameraFrame();
	DrawPhotoThumbnail();
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
// Purpose: A fading thumbnail of the last photo taken, bottom-right corner -
//			standing in for the missing HUD/hud_icon_picture art.
//-----------------------------------------------------------------------------
void CHudViewfinder::DrawPhotoThumbnail( void )
{
	if ( m_nLastPhotoSlot < 0 )
		return;

	float flAge = gpGlobals->curtime - m_flLastPhotoTime;
	if ( flAge < 0.0f || flAge > PHOTO_THUMBNAIL_FADE_TIME )
		return;

	int alpha = 255;
	if ( flAge > PHOTO_THUMBNAIL_FADE_TIME - 1.0f )
	{
		alpha = (int)( 255.0f * ( PHOTO_THUMBNAIL_FADE_TIME - flAge ) );
		alpha = clamp( alpha, 0, 255 );
	}

	int wide, tall;
	GetSize( wide, tall );

	int nThumbSize = 128;
	int x0 = wide - nThumbSize - 24;
	int y0 = tall - nThumbSize - 24;

	surface()->DrawSetColor( 255, 255, 255, alpha );
	surface()->DrawSetTexture( m_nPhotoTextureID[ m_nLastPhotoSlot ] );
	surface()->DrawTexturedRect( x0, y0, x0 + nThumbSize, y0 + nThumbSize );

	surface()->DrawSetColor( 255, 255, 255, alpha );
	surface()->DrawOutlinedRect( x0, y0, x0 + nThumbSize, y0 + nThumbSize );
}
