//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: npc_android_missile / CAndroidMissile - see
//			npc_android_missile.h.
//
//=============================================================================//
#include "cbase.h"
#include "npc_android_missile.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_basenpc.h"
#include "explode.h"
#include "soundent.h"
#include "vstdlib/random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//
// Custom activities (in addition to npc_android's).
//
static int ACT_ANDROID_RANGED_ATTACK1;

//
// Custom animation events (in addition to npc_android's).
//
static int AE_ANDROID_STARTSHOOT;
static int AE_ANDROID_SHOOT;

ConVar android_missile_speed( "android_missile_speed", "500", FCVAR_REPLICATED );
ConVar android_missile_damage( "android_missile_damage", "25", FCVAR_REPLICATED );
ConVar android_missile_radius( "android_missile_radius", "100", FCVAR_REPLICATED );
ConVar android_missile_lifetime( "android_missile_lifetime", "8", FCVAR_REPLICATED, "How long an android missile flies before self-destructing if it never hits anything." );

#define ANDROID_MISSILE_MODEL "models/Weapons/w_missile.mdl"

//=============================================================================
// CNPC_Android_Missile
//=============================================================================
LINK_ENTITY_TO_CLASS( npc_android_missile, CNPC_Android_Missile );

void CNPC_Android_Missile::Spawn( void )
{
	BaseClass::Spawn();

	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 );
}

void CNPC_Android_Missile::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( ANDROID_MISSILE_MODEL );

	PrecacheScriptSound( "NPC_Android.MissileNearmiss" );
	PrecacheScriptSound( "NPC_Android.MissileHitBody" );
	PrecacheScriptSound( "NPC_Android.MissileHitWorld" );
	PrecacheScriptSound( "NPC_Android.MissilePreExplode" );
	PrecacheScriptSound( "NPC_Android.MissileExplode" );
}

int CNPC_Android_Missile::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > GetRangeAttackRange() )
		return COND_TOO_FAR_TO_ATTACK;

	if ( flDot < 0.5f )
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}

int CNPC_Android_Missile::SelectSchedule( void )
{
	if ( GetEnemy() && HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		return SCHED_ANDROID_RANGE_ATTACK1;

	return BaseClass::SelectSchedule();
}

void CNPC_Android_Missile::HandleAnimEvent( animevent_t *pEvent )
{
	int nEvent = pEvent->Event();

	if ( nEvent == AE_ANDROID_STARTSHOOT )
	{
		EmitSound( "NPC_Android.Servo" );
		return;
	}

	if ( nEvent == AE_ANDROID_SHOOT )
	{
		FireAndroidMissile();
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

void CNPC_Android_Missile::FireAndroidMissile( void )
{
	Vector vecLaunch = WorldSpaceCenter();
	QAngle angLaunch = GetAbsAngles();

	if ( GetEnemy() )
	{
		Vector vecToEnemy = GetEnemy()->WorldSpaceCenter() - vecLaunch;
		VectorAngles( vecToEnemy, angLaunch );
	}

	CreateAndroidMissile( vecLaunch, angLaunch, this, GetEnemy() );
}

//=============================================================================
// Schedules
//=============================================================================
AI_BEGIN_CUSTOM_NPC( npc_android_missile, CNPC_Android_Missile )

	DECLARE_ACTIVITY( ACT_ANDROID_RANGED_ATTACK1 )

	DECLARE_ANIMEVENT( AE_ANDROID_STARTSHOOT )
	DECLARE_ANIMEVENT( AE_ANDROID_SHOOT )

	//=========================================================
	// bot_male's 'shoot' sequence is tagged with the custom activity
	// ACT_ANDROID_RANGED_ATTACK1 (confirmed by binary analysis), not the
	// generic ACT_RANGE_ATTACK1 TASK_RANGE_ATTACK1 would look for - play
	// it directly by name so the AE_ANDROID_SHOOT event embedded in it
	// still fires FireAndroidMissile() via HandleAnimEvent().
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_ANDROID_RANGED_ATTACK1"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_OCCLUDED"
		"		COND_HEAR_DANGER"
	)

AI_END_CUSTOM_NPC()

//=============================================================================
// CAndroidMissile
//=============================================================================
LINK_ENTITY_TO_CLASS( android_missile, CAndroidMissile );

CAndroidMissile *CreateAndroidMissile( const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pOwner, CBaseEntity *pTarget )
{
	CAndroidMissile *pMissile = static_cast<CAndroidMissile*>( CBaseEntity::Create( "android_missile", vecOrigin, angAngles, pOwner ) );
	if ( !pMissile )
		return NULL;

	pMissile->SetOwnerEntity( pOwner );
	pMissile->SetTargetEntity( pTarget );

	Vector vecForward;
	AngleVectors( angAngles, &vecForward );
	pMissile->SetAbsVelocity( vecForward * android_missile_speed.GetFloat() );

	return pMissile;
}

void CAndroidMissile::Spawn( void )
{
	Precache();

	SetModel( ANDROID_MISSILE_MODEL );

	SetMoveType( MOVETYPE_FLY );
	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_NOT_STANDABLE );
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	UTIL_SetSize( this, -Vector( 4, 4, 4 ), Vector( 4, 4, 4 ) );

	SetGravity( 0.0f );

	m_takedamage = DAMAGE_EVENTS_ONLY;

	SetTouch( &CAndroidMissile::MissileTouch );

	m_flDieTime = gpGlobals->curtime + android_missile_lifetime.GetFloat();
	SetThink( &CAndroidMissile::AndroidMissileSeekThink );
	SetNextThink( gpGlobals->curtime + 0.05f );

	EmitSound( "NPC_Android.MissilePreExplode" );
}

void CAndroidMissile::Precache( void )
{
	PrecacheModel( ANDROID_MISSILE_MODEL );
}

//-----------------------------------------------------------------------------
// Purpose: Steer toward our target's current position at a constant speed.
//-----------------------------------------------------------------------------
void CAndroidMissile::AndroidMissileSeekThink( void )
{
	if ( gpGlobals->curtime > m_flDieTime )
	{
		UTIL_Remove( this );
		return;
	}

	CBaseEntity *pTarget = m_hTarget.Get();
	if ( pTarget )
	{
		Vector vecToTarget = pTarget->WorldSpaceCenter() - WorldSpaceCenter();
		VectorNormalize( vecToTarget );

		Vector vecVelocity = vecToTarget * android_missile_speed.GetFloat();
		SetAbsVelocity( vecVelocity );

		QAngle angFacing;
		VectorAngles( vecVelocity, angFacing );
		SetAbsAngles( angFacing );
	}

	SetNextThink( gpGlobals->curtime + 0.05f );
}

void CAndroidMissile::MissileTouch( CBaseEntity *pOther )
{
	if ( !pOther || pOther == GetOwnerEntity() )
		return;

	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
		return;

	Explode( pOther );
}

void CAndroidMissile::Explode( CBaseEntity *pOther )
{
	if ( pOther && pOther->IsPlayer() )
	{
		EmitSound( "NPC_Android.MissileHitBody" );
	}
	else
	{
		EmitSound( "NPC_Android.MissileHitWorld" );
	}

	EmitSound( "NPC_Android.MissileExplode" );

	m_takedamage = DAMAGE_NO;
	SetSolid( SOLID_NONE );

	ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), GetOwnerEntity(), (int)android_missile_damage.GetFloat(),
		(int)android_missile_radius.GetFloat(), SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS, 0.0f, this );

	UTIL_Remove( this );
}
