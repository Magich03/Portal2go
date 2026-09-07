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

IMPLEMENT_SERVERCLASS_ST( CWeaponCamera, DT_WeaponCamera )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_camera, CWeaponCamera );

PRECACHE_WEAPON_REGISTER( weapon_camera );

BEGIN_DATADESC( CWeaponCamera )
	DEFINE_FIELD( m_nMaxCaptureSlots, FIELD_INTEGER ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetNumCaptureSlots", InputSetNumCaptureSlots ),
END_DATADESC()

CWeaponCamera::CWeaponCamera()
	: m_nMaxCaptureSlots( MAX_HELD_PHOTOS )
{
}

//-----------------------------------------------------------------------------
// Purpose: Mapper-facing cap on how many photos this camera will let a
//			player stack up, clamped to [0, MAX_HELD_PHOTOS] - the hard
//			technical limit set by the number of _rt_LargePhotoN render
//			targets (see portal_render_targets.h).
//-----------------------------------------------------------------------------
void CWeaponCamera::InputSetNumCaptureSlots( inputdata_t &inputdata )
{
	int nSlots = inputdata.value.Int();

	if ( nSlots < 0 || nSlots > MAX_HELD_PHOTOS )
	{
		Warning( "weapon_camera %s received 'SetNumCaptureSlots' input with invalid max slot number (must be between 0 and %d, given %i).\n",
			GetDebugName(), MAX_HELD_PHOTOS, nSlots );
		return;
	}

	m_nMaxCaptureSlots = nSlots;
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

	// Note: no VPhysicsGetObject() requirement here - CanBeCaptured() being
	// explicitly opted into is the only gate now, since AI-driven NPCs
	// (e.g. npc_chicken) don't have a physics object during normal
	// locomotion but are still meant to be capturable. CPhotoInventory's
	// hide/restore calls already guard every VPhysicsGetObject() use.
	return pAnim;
}

//-----------------------------------------------------------------------------
// Purpose: Tell the owner's client to render its current view into the
//			given _rt_LargePhotoN slot (see CViewRender::
//			CheckPendingPhotoSnapshot() in viewrender.cpp and
//			CHudViewfinder::MsgFunc_TakePhoto() in hud_viewfinder.cpp), for
//			that slot's thumbnail in the HUD's 3-photo display.
//-----------------------------------------------------------------------------
void CWeaponCamera::SendPhotoSnapshot( CPortal_Player *pOwner, int nSlot )
{
	CSingleUserRecipientFilter user( pOwner );
	user.MakeReliable();
	CUsrMsg_TakePhoto msg;
	msg.set_slot( nSlot );
	SendUserMessage( user, UM_TakePhoto, msg );
}

//-----------------------------------------------------------------------------
// Purpose: Capture whatever we're looking at. prop_swap is a special case -
//			it swaps places with the player on the spot instead of being
//			captured at all. Everything else (prop_air_vent included)
//			becomes a polaroid: the real object is stashed out of the world
//			and pushed onto a stack of up to 3, each shown as a flat 2D
//			photo, until weapon_placement's first click on the most recent
//			one brings it back as a scalable ghost.
//-----------------------------------------------------------------------------
void CWeaponCamera::PrimaryAttack( void )
{
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.3f;

	CPortal_Player *pOwner = ToPortalPlayer( GetOwner() );
	if ( !pOwner )
		return;

	CPhotoInventory *pInventory = pOwner->GetPhotoInventory();

	if ( pInventory->IsGhostActive() )
		return;	// something's actively out being placed - finish that first

	CBaseAnimating *pTarget = FindCapturableEntity();
	if ( !pTarget )
		return;

	CPropSwap *pSwapProp = dynamic_cast<CPropSwap*>( pTarget );
	if ( pSwapProp )
	{
		pSwapProp->SwapWithPlayer( pOwner );
		return;
	}

	if ( pInventory->GetStackCount() >= m_nMaxCaptureSlots )
		return;	// already holding as many photos as this camera currently allows

	// Slot = stack position, so the client's 3 render targets always line up
	// 1:1 with the HUD's 3 slot positions regardless of capture order.
	int nSlot = pInventory->GetStackCount();
	SendPhotoSnapshot( pOwner, nSlot );

	char szTextureName[32];
	Q_snprintf( szTextureName, sizeof( szTextureName ), "_rt_LargePhoto%d", nSlot );

	CItem_Photograph *pPolaroid = CreatePhotograph( pOwner->EyePosition(), pOwner->EyeAngles(), szTextureName );
	bool bCaptured = pPolaroid && pInventory->CapturePolaroid( pTarget, pPolaroid );

	if ( !bCaptured )
	{
		if ( pPolaroid )
		{
			UTIL_Remove( pPolaroid );
		}
		return;
	}

	pOwner->SetPlacingPhoto( true );
	EmitSound( "Weapon_Camera.Capture" );

	CBaseCombatWeapon *pPlacementWeapon = pOwner->Weapon_OwnsThisType( "weapon_placement" );
	if ( pPlacementWeapon )
	{
		pOwner->Weapon_Switch( pPlacementWeapon );
	}
}
