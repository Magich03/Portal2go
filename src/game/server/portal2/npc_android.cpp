//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: npc_android - see npc_android.h.
//
//			Simplifications from the decompiled schedule text: the evidenced
//			TASK_ANDROID_DELAY_SWAT is folded into a plain TASK_WAIT (same
//			effect - a pause before swatting - without needing a dedicated
//			task), and swatting/melee use a single CheckTraceHullAttack/
//			impulse rather than npc_BaseZombie's fuller per-direction swat
//			activity selection, matching the decompile's own simpler
//			SwatPhysicsObject()/ClawAttack() pseudocode rather than zombie's
//			more elaborate real implementation.
//
//=============================================================================//
#include "cbase.h"
#include "npc_android.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "ai_navigator.h"
#include "ai_route.h"
#include "ai_moveprobe.h"
#include "soundent.h"
#include "vstdlib/random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//
// Custom activities.
//
static int ACT_ANDROID_RISE_FROM_GROUND;

//
// Custom animation events.
//
static int AE_ANDROID_ATTACK_RIGHT;
static int AE_ANDROID_ATTACK_LEFT;
static int AE_ANDROID_ATTACK_BOTH;
static int AE_ANDROID_SWATITEM;
static int AE_ANDROID_STARTSWAT;
static int AE_ANDROID_WIFF;
static int AE_ANDROID_STEP_LEFT;
static int AE_ANDROID_STEP_RIGHT;
static int AE_ANDROID_MOVESTART_LEFT;
static int AE_ANDROID_MOVESTART_RIGHT;
static int AE_ANDROID_SCUFF_LEFT;
static int AE_ANDROID_SCUFF_RIGHT;
static int AE_ANDROID_ATTACK_SCREAM;
static int AE_ANDROID_GET_UP;
static int AE_ANDROID_POUND;
static int AE_ANDROID_ALERTSOUND;

ConVar sk_android_health( "sk_android_health", "40", FCVAR_REPLICATED );
ConVar sk_android_dmg_melee( "sk_android_dmg_melee", "10", FCVAR_REPLICATED );
ConVar android_max_swat_mass( "android_max_swat_mass", "35", FCVAR_REPLICATED );
ConVar android_swat_dist( "android_swat_dist", "512", FCVAR_REPLICATED, "How far away an npc_android will notice a swattable physics object." );
ConVar android_swat_reach( "android_swat_reach", "48", FCVAR_REPLICATED, "How close an npc_android needs to be to actually swat a physics object." );

LINK_ENTITY_TO_CLASS( npc_android, CNPC_Android );

void CNPC_Android::Spawn( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		SetModelName( AllocPooledString( GetAndroidModelName() ) );
	}

	Precache();

	SetModel( STRING( GetModelName() ) );

	SetHullType( HULL_HUMAN );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );

	m_iHealth = sk_android_health.GetFloat();
	m_flFieldOfView = 0.5f;
	SetViewOffset( Vector( 0, 0, 64 ) );

	SetBloodColor( DONT_BLEED );
	m_NPCState = NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_TURN_HEAD | bits_CAP_INNATE_MELEE_ATTACK1 );

	m_bWasOffGround = false;
	m_flNextSwatScan = 0.0f;
	m_flNextSwat = 0.0f;
	m_hPhysicsEnt = NULL;

	NPCInit();
}

void CNPC_Android::Precache( void )
{
	PrecacheModel( GetAndroidModelName() );

	PrecacheScriptSound( "NPC_Android.Squash" );
	PrecacheScriptSound( "NPC_Android.Servo" );
	PrecacheScriptSound( "NPC_Android.Pain" );

	BaseClass::Precache();
}

Class_T CNPC_Android::Classify( void )
{
	return CLASS_COMBINE;
}

//-----------------------------------------------------------------------------
// Purpose: Track ground contact (fall/land) and periodically look for
//			something swattable while chasing an enemy.
//-----------------------------------------------------------------------------
void CNPC_Android::GatherConditions( void )
{
	ClearCondition( COND_ANDROID_LOCAL_MELEE_OBSTRUCTION );

	BaseClass::GatherConditions();

	bool bOnGround = ( GetFlags() & FL_ONGROUND ) != 0;
	if ( !bOnGround )
	{
		SetCondition( COND_ANDROID_OFF_GROUND );
		m_bWasOffGround = true;
	}
	else if ( m_bWasOffGround )
	{
		SetCondition( COND_ANDROID_HIT_GROUND );
		m_bWasOffGround = false;
	}

	if ( m_NPCState == NPC_STATE_COMBAT && GetEnemy() )
	{
		if ( gpGlobals->curtime >= m_flNextSwatScan && m_hPhysicsEnt == NULL )
		{
			FindNearestPhysicsObject( android_max_swat_mass.GetFloat() );
			m_flNextSwatScan = gpGlobals->curtime + 2.0f;
		}

		if ( m_hPhysicsEnt != NULL && gpGlobals->curtime >= m_flNextSwat && HasCondition( COND_SEE_ENEMY ) )
		{
			SetCondition( COND_ANDROID_CAN_SWAT_ATTACK );
		}
		else
		{
			ClearCondition( COND_ANDROID_CAN_SWAT_ATTACK );
		}
	}
	else
	{
		ClearCondition( COND_ANDROID_CAN_SWAT_ATTACK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Nearest small, sleeping, moveable physics object within
//			android_swat_dist that isn't us.
//-----------------------------------------------------------------------------
bool CNPC_Android::FindNearestPhysicsObject( float flMaxMass )
{
	CBaseEntity *pList[ 32 ];
	int nCount = UTIL_EntitiesInSphere( pList, ARRAYSIZE( pList ), GetAbsOrigin(), android_swat_dist.GetFloat(), 0 );

	CBaseEntity *pNearest = NULL;
	float flNearestDist = FLT_MAX;

	for ( int i = 0; i < nCount; ++i )
	{
		CBaseEntity *pEntity = pList[ i ];
		if ( !pEntity || pEntity == this )
			continue;

		IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
		if ( !pPhys || pPhys->GetMass() > flMaxMass || !pPhys->IsMoveable() )
			continue;

		float flDist = ( pEntity->WorldSpaceCenter() - WorldSpaceCenter() ).Length();
		if ( flDist < flNearestDist )
		{
			flNearestDist = flDist;
			pNearest = pEntity;
		}
	}

	m_hPhysicsEnt = pNearest;
	return pNearest != NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Knock our swat target away from us, toward wherever our enemy is.
//-----------------------------------------------------------------------------
void CNPC_Android::SwatPhysicsObject( void )
{
	CBaseEntity *pPhysicsEntity = m_hPhysicsEnt.Get();
	if ( !pPhysicsEntity )
		return;

	IPhysicsObject *pPhysObj = pPhysicsEntity->VPhysicsGetObject();
	if ( pPhysObj )
	{
		Vector vecDir = pPhysicsEntity->WorldSpaceCenter() - WorldSpaceCenter();
		VectorNormalize( vecDir );

		Vector vecImpulse = vecDir * 400.0f * pPhysObj->GetMass();
		pPhysObj->ApplyForceCenter( vecImpulse );
	}

	m_flNextSwat = gpGlobals->curtime + 3.0f;
	m_hPhysicsEnt = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Simple bare-handed melee - a straight hull trace in front of us.
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_Android::ClawAttack( float flDist, int iDamage )
{
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	return CheckTraceHullAttack( flDist, vecMins, vecMaxs, iDamage, DMG_CLUB );
}

int CNPC_Android::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > GetClawAttackRange() )
		return COND_TOO_FAR_TO_ATTACK;

	if ( flDot < 0.7f )
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_MELEE_ATTACK1;
}

void CNPC_Android::HandleAnimEvent( animevent_t *pEvent )
{
	int nEvent = pEvent->Event();

	if ( nEvent == AE_ANDROID_ATTACK_RIGHT || nEvent == AE_ANDROID_ATTACK_LEFT || nEvent == AE_ANDROID_ATTACK_BOTH )
	{
		ClawAttack( GetClawAttackRange(), sk_android_dmg_melee.GetFloat() );
		return;
	}

	if ( nEvent == AE_ANDROID_STARTSWAT || nEvent == AE_ANDROID_ATTACK_SCREAM || nEvent == AE_ANDROID_ALERTSOUND )
	{
		EmitSound( "NPC_Android.Servo" );
		return;
	}

	if ( nEvent == AE_ANDROID_SWATITEM )
	{
		SwatPhysicsObject();
		return;
	}

	if ( nEvent == AE_ANDROID_WIFF || nEvent == AE_ANDROID_STEP_LEFT || nEvent == AE_ANDROID_STEP_RIGHT ||
		 nEvent == AE_ANDROID_MOVESTART_LEFT || nEvent == AE_ANDROID_MOVESTART_RIGHT ||
		 nEvent == AE_ANDROID_SCUFF_LEFT || nEvent == AE_ANDROID_SCUFF_RIGHT || nEvent == AE_ANDROID_GET_UP ||
		 nEvent == AE_ANDROID_POUND )
	{
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Matches the decompiled priority order exactly.
//-----------------------------------------------------------------------------
int CNPC_Android::SelectSchedule( void )
{
	if ( HasCondition( COND_ANDROID_OFF_GROUND ) )
		return SCHED_ANDROID_FALL;

	if ( HasCondition( COND_ANDROID_HIT_GROUND ) )
		return SCHED_ANDROID_HIT_GROUND;

	if ( HasCondition( COND_ANDROID_CAN_SWAT_ATTACK ) )
		return SCHED_ANDROID_MOVE_SWATITEM;

	if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		return SCHED_ANDROID_MELEE_ATTACK1;

	if ( GetEnemy() )
		return SCHED_ANDROID_CHASE_ENEMY;

	return BaseClass::SelectSchedule();
}

void CNPC_Android::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANDROID_SWAT_ITEM:
		SwatPhysicsObject();
		TaskComplete();
		break;

	case TASK_ANDROID_GET_PATH_TO_PHYSOBJ:
	{
		CBaseEntity *pPhysicsEnt = m_hPhysicsEnt.Get();
		if ( !pPhysicsEnt )
		{
			TaskFail( "Physics ent NULL" );
			break;
		}

		AI_NavGoal_t goal( pPhysicsEnt->WorldSpaceCenter() );
		goal.pTarget = pPhysicsEnt;
		if ( GetNavigator()->SetGoal( goal ) )
		{
			TaskComplete();
		}
		else
		{
			TaskFail( FAIL_NO_ROUTE );
		}
		break;
	}

	case TASK_ANDROID_WAIT_POST_MELEE:
		SetWait( 1.0f );
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}

void CNPC_Android::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANDROID_WAIT_POST_MELEE:
		if ( IsWaitFinished() )
		{
			TaskComplete();
		}
		break;

	default:
		BaseClass::RunTask( pTask );
	}
}

//=============================================================================
// Schedules
//=============================================================================
AI_BEGIN_CUSTOM_NPC( npc_android, CNPC_Android )

	DECLARE_TASK( TASK_ANDROID_SWAT_ITEM )
	DECLARE_TASK( TASK_ANDROID_GET_PATH_TO_PHYSOBJ )
	DECLARE_TASK( TASK_ANDROID_WAIT_POST_MELEE )

	DECLARE_ACTIVITY( ACT_ANDROID_RISE_FROM_GROUND )

	DECLARE_CONDITION( COND_ANDROID_RELEASED )
	DECLARE_CONDITION( COND_ANDROID_OFF_GROUND )
	DECLARE_CONDITION( COND_ANDROID_HIT_GROUND )
	DECLARE_CONDITION( COND_ANDROID_RISE_FROM_GROUND )
	DECLARE_CONDITION( COND_ANDROID_CAN_SWAT_ATTACK )
	DECLARE_CONDITION( COND_ANDROID_LOCAL_MELEE_OBSTRUCTION )

	DECLARE_ANIMEVENT( AE_ANDROID_ATTACK_RIGHT )
	DECLARE_ANIMEVENT( AE_ANDROID_ATTACK_LEFT )
	DECLARE_ANIMEVENT( AE_ANDROID_ATTACK_BOTH )
	DECLARE_ANIMEVENT( AE_ANDROID_SWATITEM )
	DECLARE_ANIMEVENT( AE_ANDROID_STARTSWAT )
	DECLARE_ANIMEVENT( AE_ANDROID_WIFF )
	DECLARE_ANIMEVENT( AE_ANDROID_STEP_LEFT )
	DECLARE_ANIMEVENT( AE_ANDROID_STEP_RIGHT )
	DECLARE_ANIMEVENT( AE_ANDROID_MOVESTART_LEFT )
	DECLARE_ANIMEVENT( AE_ANDROID_MOVESTART_RIGHT )
	DECLARE_ANIMEVENT( AE_ANDROID_SCUFF_LEFT )
	DECLARE_ANIMEVENT( AE_ANDROID_SCUFF_RIGHT )
	DECLARE_ANIMEVENT( AE_ANDROID_ATTACK_SCREAM )
	DECLARE_ANIMEVENT( AE_ANDROID_GET_UP )
	DECLARE_ANIMEVENT( AE_ANDROID_POUND )
	DECLARE_ANIMEVENT( AE_ANDROID_ALERTSOUND )

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_MOVE_SWATITEM,

		"	Tasks"
		"		TASK_WAIT						3"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ANDROID_GET_PATH_TO_PHYSOBJ	0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_FACE_ENEMY					0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_MELEE_ATTACK1"
		"		TASK_ANDROID_SWAT_ITEM			0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_ANDROID_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_SWATITEM,

		"	Tasks"
		"		TASK_WAIT						3"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_FACE_ENEMY					0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_MELEE_ATTACK1"
		"		TASK_ANDROID_SWAT_ITEM			0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_ANDROID_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_ATTACKITEM,

		"	Tasks"
		"		TASK_FACE_ENEMY					0"
		"		TASK_MELEE_ATTACK1				0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_ANDROID_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_CHASE_ENEMY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		TASK_SET_TOLERANCE_DISTANCE		24"
		"		TASK_GET_CHASE_PATH_TO_ENEMY	600"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_FACE_ENEMY					0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_TASK_FAILED"
		"		COND_ANDROID_CAN_SWAT_ATTACK"
		"		COND_ANDROID_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_WANDER_MEDIUM,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_WANDER						480384"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT_PVS					0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ANDROID_WANDER_MEDIUM"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ANDROID_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_WANDER_STANDOFF,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_WANDER						480384"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT_PVS					0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_ANDROID_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_WANDER_FAIL,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_WAIT				1"
		"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ANDROID_WANDER_MEDIUM"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_ANDROID_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_MELEE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"
		"		TASK_MELEE_ATTACK1		0"
		"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ANDROID_POST_MELEE_WAIT"
		""
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ANDROID_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_POST_MELEE_WAIT,

		"	Tasks"
		"		TASK_ANDROID_WAIT_POST_MELEE		0"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_MOVE_TO_AMBUSH,

		"	Tasks"
		"		TASK_WAIT						0.0"
		"		TASK_FIND_COVER_FROM_ENEMY		0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_TURN_LEFT					180"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ANDROID_WAIT_AMBUSH"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_NEW_ENEMY"
		"		COND_ANDROID_OFF_GROUND"
		"		COND_ANDROID_HIT_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_WAIT_AMBUSH,

		"	Tasks"
		"		TASK_WAIT_FACE_ENEMY	99999"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_ANDROID_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_RISE,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_ANDROID_RISE_FROM_GROUND"
		"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ANDROID_WANDER_MEDIUM"
		""
		"	Interrupts"
		"		COND_ANDROID_OFF_GROUND"
		"		COND_ANDROID_HIT_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_FALL,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_GLIDE"
		"		TASK_WAIT_INDEFINITE			0"
		""
		"	Interrupts"
		"		COND_ANDROID_HIT_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_HIT_GROUND,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_LAND"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_STAND"
		""
		"	Interrupts"
		"		COND_ANDROID_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANDROID_STUMBLE,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_SIGNAL1"
		""
		"	Interrupts"
		"		COND_ANDROID_OFF_GROUND"
	)

AI_END_CUSTOM_NPC()
