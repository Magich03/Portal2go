//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: F-Stop camera - captures a nearby capturable prop into the
//			player's photo inventory (see photo_inventory.h) and hands off to
//			weapon_placement to preview/scale/place it. Two separate tools,
//			matching how the F-Stop prototype split capture from placement.
//
//			This is a from-scratch reimplementation of the "F-Stop"/Exposure
//			camera+placement loop, built around real (if previously unused)
//			hooks already present in this codebase: CBaseAnimating's
//			m_bCanBeCaptured/CanBeCaptured(), m_OnCameraCapture/m_OnCameraRelease,
//			GetObjectScaleLevel()/SetObjectScaleLevel(), and SetModelScale().
//			It intentionally does NOT attempt to rescale physics collision -
//			that's the part the original Exposure binary itself documents as
//			broken, so placed objects keep their original-size collision even
//			when visually scaled.
//
//=============================================================================//
#include "cbase.h"
#include "weapon_camera.h"
#include "portal_player.h"
#include "photo_inventory.h"
#include "prop_swap.h"
#include "item_photograph.h"
#include "usermessages.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar camera_capture_distance( "camera_capture_distance", "256", FCVAR_REPLICATED, "How far away weapon_camera can capture an object from." );

// Must match NUM_LARGE_PHOTO_SLOTS in the client's portal_render_targets.h -
// the server only needs the render target name string, not the client-only
// render target singleton, so the count is duplicated here rather than shared.
#define NUM_LARGE_PHOTO_SLOTS 3

IMPLEMENT_SERVERCLASS_ST( CWeaponCamera, DT_WeaponCamera )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_camera, CWeaponCamera );

PRECACHE_WEAPON_REGISTER( weapon_camera );

CWeaponCamera::CWeaponCamera()
{
}

void CWeaponCamera::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_Camera.Capture" );
}

//-----------------------------------------------------------------------------
// Purpose: Trace out from the player's eyes and see if we're looking at
//			something that allows capture.
//-----------------------------------------------------------------------------
CBaseAnimating *CWeaponCamera::FindCapturableEntity( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return NULL;

	Vector vecEye = pOwner->EyePosition();
	Vector vecForward;
	AngleVectors( pOwner->EyeAngles(), &vecForward );

	trace_t tr;
	Ray_t ray;
	ray.Init( vecEye, vecEye + vecForward * camera_capture_distance.GetFloat() );
	CTraceFilterSimple traceFilter( pOwner, COLLISION_GROUP_NONE );
	UTIL_TraceRay( ray, MASK_SOLID, &traceFilter, &tr );

	if ( !tr.m_pEnt )
		return NULL;

	CBaseAnimating *pAnim = tr.m_pEnt->GetBaseAnimating();
	if ( !pAnim )
		return NULL;

	if ( !pAnim->CanBeCaptured() )
		return NULL;

	if ( !pAnim->VPhysicsGetObject() )
		return NULL;

	return pAnim;
}

//-----------------------------------------------------------------------------
// Purpose: Cycle through the 3 _rt_LargePhotoN client render targets and tell
//			the owner's client to render its current view into the one we
//			picked (see CViewRender::CheckPendingPhotoSnapshot() in
//			viewrender.cpp and CHudViewfinder::MsgFunc_TakePhoto() in
//			hud_viewfinder.cpp), for the polaroid thumbnail in the HUD.
//-----------------------------------------------------------------------------
void CWeaponCamera::SendPhotoSnapshot( CPortal_Player *pOwner, char *pszTextureNameOut, int nTextureNameSize )
{
	static int s_nNextPhotoSlot = 0;
	int nSlot = s_nNextPhotoSlot;
	s_nNextPhotoSlot = ( s_nNextPhotoSlot + 1 ) % NUM_LARGE_PHOTO_SLOTS;

	Q_snprintf( pszTextureNameOut, nTextureNameSize, "_rt_LargePhoto%d", nSlot );

	CSingleUserRecipientFilter user( pOwner );
	user.MakeReliable();
	CUsrMsg_TakePhoto msg;
	msg.set_slot( nSlot );
	SendUserMessage( user, UM_TakePhoto, msg );
}

//-----------------------------------------------------------------------------
// Purpose: Capture whatever we're looking at. prop_swap is a special case -
//			it swaps places with the player on the spot instead of being
//			captured at all. Props that override UsesDirectCapture() (like
//			prop_air_vent) get carried+previewed directly, like before the
//			polaroid step existed. Everything else becomes a polaroid: the
//			real object is stashed out of the world and the player holds a
//			flat 2D photo (item_photograph) until weapon_placement's first
//			click brings it back as a scalable ghost.
//-----------------------------------------------------------------------------
void CWeaponCamera::PrimaryAttack( void )
{
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.3f;

	CPortal_Player *pOwner = ToPortalPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( pOwner->GetPhotoInventory()->HasPhoto() )
		return;	// already holding a capture - place it with weapon_placement first

	CBaseAnimating *pTarget = FindCapturableEntity();
	if ( !pTarget )
		return;

	CPropSwap *pSwapProp = dynamic_cast<CPropSwap*>( pTarget );
	if ( pSwapProp )
	{
		pSwapProp->SwapWithPlayer( pOwner );
		return;
	}

	bool bCaptured;

	if ( pTarget->UsesDirectCapture() )
	{
		bCaptured = pOwner->GetPhotoInventory()->CaptureDirect( pTarget );
	}
	else
	{
		char szTextureName[32];
		SendPhotoSnapshot( pOwner, szTextureName, sizeof( szTextureName ) );

		CItem_Photograph *pPolaroid = CreatePhotograph( pOwner->EyePosition(), pOwner->EyeAngles(), szTextureName );
		bCaptured = pPolaroid && pOwner->GetPhotoInventory()->CapturePolaroid( pTarget, pPolaroid );

		if ( !bCaptured && pPolaroid )
		{
			UTIL_Remove( pPolaroid );
		}
	}

	if ( !bCaptured )
		return;

	pOwner->SetPlacingPhoto( true );
	EmitSound( "Weapon_Camera.Capture" );

	CBaseCombatWeapon *pPlacementWeapon = pOwner->Weapon_OwnsThisType( "weapon_placement" );
	if ( pPlacementWeapon )
	{
		pOwner->Weapon_Switch( pPlacementWeapon );
	}
}
