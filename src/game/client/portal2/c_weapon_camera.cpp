//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: weapon_camera has no client-only state or prediction of its own,
//			so it's linked to a trivial stub client class that inherits the
//			shared portal weapon behavior.
//
//=============================================================================//
#include "cbase.h"
#include "c_weapon__stubs.h"
#include "weapon_portalbasecombatweapon.h"

STUB_WEAPON_CLASS( weapon_camera, WeaponCamera, CBasePortalCombatWeapon );
