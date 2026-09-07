//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: npc_android_missile - a ranged variant of npc_android that fires
//			a homing projectile (CAndroidMissile) instead of relying solely
//			on melee. Reconstructed from decompiled F-Stop/Exposure binaries.
//
//=============================================================================//
#ifndef NPC_ANDROID_MISSILE_H
#define NPC_ANDROID_MISSILE_H
#ifdef _WIN32
#pragma once
#endif

#include "npc_android.h"

class CNPC_Android_Missile : public CNPC_Android
{
	DECLARE_CLASS( CNPC_Android_Missile, CNPC_Android );
public:

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual const char *GetAndroidModelName( void ) { return "models/Zombie/Classic.mdl"; }

	virtual void HandleAnimEvent( animevent_t *pEvent );
	virtual int SelectSchedule( void );

	virtual int RangeAttack1Conditions( float flDot, float flDist );
	float GetRangeAttackRange( void ) const { return 1536.0f; }

	DEFINE_CUSTOM_AI;

private:
	void FireAndroidMissile( void );
};

//-----------------------------------------------------------------------------
// A simple homing projectile fired by npc_android_missile.
//-----------------------------------------------------------------------------
class CAndroidMissile : public CBaseAnimating
{
	DECLARE_CLASS( CAndroidMissile, CBaseAnimating );
public:

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void MissileTouch( CBaseEntity *pOther );

	void SetTargetEntity( CBaseEntity *pTarget ) { m_hTarget = pTarget; }

	void AndroidMissileSeekThink( void );

private:
	void Explode( CBaseEntity *pOther );

	CHandle<CBaseEntity>	m_hTarget;
	float					m_flDieTime;
};

CAndroidMissile *CreateAndroidMissile( const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pOwner, CBaseEntity *pTarget );

#endif // NPC_ANDROID_MISSILE_H
