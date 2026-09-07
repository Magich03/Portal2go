//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: weapon_placement's only client-only behavior is turning the mouse
//			wheel into scale up/down input for the ghost it's currently
//			holding (see weapon_placement.cpp's ItemPostFrame) - the same
//			IN_WEAPON1/IN_WEAPON2 technique the HL2 gravity gun uses for its
//			pull-distance control (c_weapon_gravitygun.cpp/physgun.cpp).
//
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "in_buttons.h"
#include "weapon_portalbasecombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_WeaponPlacement : public CBasePortalCombatWeapon
{
	DECLARE_CLASS( C_WeaponPlacement, CBasePortalCombatWeapon );
public:
	DECLARE_PREDICTABLE();
	DECLARE_CLIENTCLASS();

	C_WeaponPlacement() {}

	int KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
	{
		switch ( keynum )
		{
		case MOUSE_WHEEL_UP:
			GetHud().m_iKeyBits |= IN_WEAPON1;
			return 0;

		case MOUSE_WHEEL_DOWN:
			GetHud().m_iKeyBits |= IN_WEAPON2;
			return 0;
		}

		// Allow engine to process
		return BaseClass::KeyInput( down, keynum, pszCurrentBinding );
	}

private:
	C_WeaponPlacement( const C_WeaponPlacement & );
};

IMPLEMENT_CLIENTCLASS_DT( C_WeaponPlacement, DT_WeaponPlacement, CWeaponPlacement )
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_WeaponPlacement )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_placement, C_WeaponPlacement );
