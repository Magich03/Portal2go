//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: npc_chicken - see npc_chicken.h.
//
//=============================================================================//
#include "cbase.h"
#include "npc_chicken.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_hint.h"
#include "ai_motor.h"
#include "ai_navigator.h"
#include "ai_route.h"
#include "soundent.h"
#include "vstdlib/random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CHICKEN_MODEL "models/chicken/chicken.mdl"

//
// Custom animation events.
//
static int AE_CHICKEN_PECK;
static int AE_CHICKEN_FOOTSTEP_RIGHT;
static int AE_CHICKEN_FOOTSTEP_LEFT;

ConVar sk_chicken_health( "sk_chicken_health", "5", FCVAR_REPLICATED );
ConVar chicken_enemy_too_close_dist( "chicken_enemy_too_close_dist", "300", FCVAR_REPLICATED );
ConVar chicken_enemy_way_too_close_dist( "chicken_enemy_way_too_close_dist", "150", FCVAR_REPLICATED );

LINK_ENTITY_TO_CLASS( npc_chicken, CNPC_Chicken );

void CNPC_Chicken::Spawn( void )
{
	Precache();

	SetModel( CHICKEN_MODEL );

	SetHullType( HULL_TINY );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );

	m_iHealth = sk_chicken_health.GetFloat();
	m_flFieldOfView = VIEW_FIELD_FULL;
	SetViewOffset( Vector( 4, 0, 6 ) );

	SetBloodColor( BLOOD_COLOR_RED );
	m_NPCState = NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_TURN_HEAD );

	m_bWasOffGround = false;
	m_flGroundIdleMoveTime = gpGlobals->curtime + random->RandomFloat( 0.0f, 5.0f );
	m_flNextRoostAttempt = gpGlobals->curtime + random->RandomFloat( 20.0f, 40.0f );

	NPCInit();
}

void CNPC_Chicken::Precache( void )
{
	PrecacheModel( CHICKEN_MODEL );

	PrecacheScriptSound( "NPC_Chicken.Clucks" );
	PrecacheScriptSound( "NPC_Chicken.Squawk" );
	PrecacheScriptSound( "NPC_Chicken.Startle" );

	BaseClass::Precache();
}

Class_T CNPC_Chicken::Classify( void )
{
	return CLASS_EARTH_FAUNA;
}

//-----------------------------------------------------------------------------
// Purpose: Same too-close/way-too-close distance banding npc_crow uses.
//-----------------------------------------------------------------------------
void CNPC_Chicken::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	float flEnemyDist = ( GetLocalOrigin() - pEnemy->GetLocalOrigin() ).Length();

	if ( flEnemyDist < chicken_enemy_way_too_close_dist.GetFloat() )
	{
		SetCondition( COND_CHICKEN_ENEMY_WAY_TOO_CLOSE );
	}

	if ( flEnemyDist < chicken_enemy_too_close_dist.GetFloat() )
	{
		SetCondition( COND_CHICKEN_ENEMY_TOO_CLOSE );
	}

	BaseClass::GatherEnemyConditions( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: Track ground contact so we know when we're falling vs. when
//			we've just landed.
//-----------------------------------------------------------------------------
void CNPC_Chicken::GatherConditions( void )
{
	BaseClass::GatherConditions();

	bool bOnGround = ( GetFlags() & FL_ONGROUND ) != 0;

	if ( !bOnGround )
	{
		SetCondition( COND_CHICKEN_OFF_GROUND );
		m_bWasOffGround = true;
	}
	else if ( m_bWasOffGround )
	{
		SetCondition( COND_CHICKEN_HIT_GROUND );
		m_bWasOffGround = false;
	}
}

bool CNPC_Chicken::ShouldRoost( void )
{
	return gpGlobals->curtime > m_flNextRoostAttempt;
}

void CNPC_Chicken::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->Event() )
	{
	case AE_CHICKEN_PECK:
		EmitSound( "NPC_Chicken.Clucks" );
		return;
	case AE_CHICKEN_FOOTSTEP_RIGHT:
	case AE_CHICKEN_FOOTSTEP_LEFT:
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Matches the decompiled priority order exactly: react to falling/
//			landing first, then fleeing, then occasionally roost, otherwise
//			idle or wander.
//-----------------------------------------------------------------------------
int CNPC_Chicken::SelectSchedule( void )
{
	if ( HasCondition( COND_CHICKEN_OFF_GROUND ) )
		return SCHED_CHICKEN_FALL;

	if ( HasCondition( COND_CHICKEN_HIT_GROUND ) )
		return SCHED_CHICKEN_HIT_GROUND;

	if ( HasCondition( COND_CHICKEN_ENEMY_WAY_TOO_CLOSE ) )
	{
		ClearCondition( COND_CHICKEN_ENEMY_WAY_TOO_CLOSE );
		return SCHED_CHICKEN_RUN_AWAY;
	}

	if ( HasCondition( COND_CHICKEN_ENEMY_TOO_CLOSE ) )
	{
		ClearCondition( COND_CHICKEN_ENEMY_TOO_CLOSE );
		return SCHED_CHICKEN_WALK_AWAY;
	}

	if ( ShouldRoost() )
	{
		m_flNextRoostAttempt = gpGlobals->curtime + random->RandomFloat( 45.0f, 90.0f );
		return SCHED_CHICKEN_ROOST;
	}

	if ( gpGlobals->curtime > m_flGroundIdleMoveTime )
	{
		m_flGroundIdleMoveTime = gpGlobals->curtime + random->RandomFloat( 8.0f, 16.0f );
		return random->RandomInt( 0, 1 ) ? SCHED_CHICKEN_IDLE_STAND : SCHED_CHICKEN_IDLE_WALK;
	}

	return SCHED_CHICKEN_IDLE_STAND;
}

void CNPC_Chicken::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CHICKEN_PICK_RANDOM_GOAL:
	{
		m_vSavePosition = GetLocalOrigin() + Vector( random->RandomFloat( -128.0f, 128.0f ), random->RandomFloat( -128.0f, 128.0f ), 0 );
		TaskComplete();
		break;
	}

	case TASK_CHICKEN_PICK_EVADE_GOAL:
	{
		if ( GetEnemy() != NULL )
		{
			Vector vecEnemyOrigin = GetEnemy()->GetAbsOrigin();
			vecEnemyOrigin.z = GetAbsOrigin().z;

			m_vSavePosition = GetAbsOrigin() - vecEnemyOrigin;
			VectorNormalize( m_vSavePosition );
			m_vSavePosition = GetAbsOrigin() + m_vSavePosition * ( 128 + random->RandomInt( 0, 128 ) );

			GetMotor()->SetIdealYawToTarget( m_vSavePosition );
			TaskComplete();
		}
		else
		{
			TaskFail( "No enemy" );
		}
		break;
	}

	case TASK_CHICKEN_FIND_PATH_TO_NEST:
	{
		CAI_Hint *pHint = CAI_HintManager::FindHint( this, HINT_PORTAL2_NEST, bits_HINT_NODE_NEAREST, 2048.0f );
		if ( !pHint )
		{
			Warning( "Nest hint node missing!\n" );
			TaskFail( FAIL_NO_HINT_NODE );
			break;
		}

		Vector vecHintPos;
		pHint->GetPosition( this, &vecHintPos );

		if ( GetNavigator()->SetGoal( vecHintPos ) )
		{
			TaskComplete();
		}
		else
		{
			pHint->DisableForSeconds( 5.0f );
			TaskFail( FAIL_NO_ROUTE );
		}
		break;
	}

	default:
		BaseClass::StartTask( pTask );
	}
}

void CNPC_Chicken::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CHICKEN_FIND_PATH_TO_NEST:
		// Completed synchronously in StartTask() - nothing to poll.
		break;

	default:
		BaseClass::RunTask( pTask );
	}
}

//=============================================================================
// Schedules
//=============================================================================
AI_BEGIN_CUSTOM_NPC( npc_chicken, CNPC_Chicken )

	DECLARE_TASK( TASK_CHICKEN_PICK_RANDOM_GOAL )
	DECLARE_TASK( TASK_CHICKEN_PICK_EVADE_GOAL )
	DECLARE_TASK( TASK_CHICKEN_FIND_PATH_TO_NEST )

	DECLARE_CONDITION( COND_CHICKEN_ENEMY_TOO_CLOSE )
	DECLARE_CONDITION( COND_CHICKEN_ENEMY_WAY_TOO_CLOSE )
	DECLARE_CONDITION( COND_CHICKEN_RELEASED )
	DECLARE_CONDITION( COND_CHICKEN_OFF_GROUND )
	DECLARE_CONDITION( COND_CHICKEN_HIT_GROUND )

	DECLARE_ANIMEVENT( AE_CHICKEN_PECK )
	DECLARE_ANIMEVENT( AE_CHICKEN_FOOTSTEP_RIGHT )
	DECLARE_ANIMEVENT( AE_CHICKEN_FOOTSTEP_LEFT )

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CHICKEN_IDLE_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING			1"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_WAIT					5"
		"		TASK_WAIT_PVS				0"
		""
		"	Interrupts"
		"		COND_CHICKEN_ENEMY_TOO_CLOSE"
		"		COND_CHICKEN_ENEMY_WAY_TOO_CLOSE"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_IDLE_INTERRUPT"
		"		COND_CHICKEN_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CHICKEN_IDLE_WALK,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHICKEN_IDLE_STAND"
		"		TASK_CHICKEN_PICK_RANDOM_GOAL	0"
		"		TASK_GET_PATH_TO_SAVEPOSITION	0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_WAIT_PVS					0"
		""
		"	Interrupts"
		"		COND_CHICKEN_ENEMY_TOO_CLOSE"
		"		COND_CHICKEN_ENEMY_WAY_TOO_CLOSE"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_NEW_ENEMY"
		"		COND_CHICKEN_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CHICKEN_WALK_AWAY,

		"	Tasks"
		"		TASK_CHICKEN_PICK_EVADE_GOAL	0"
		"		TASK_GET_PATH_TO_SAVEPOSITION	0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		""
		"	Interrupts"
		"		COND_CHICKEN_ENEMY_WAY_TOO_CLOSE"
		"		COND_HEAVY_DAMAGE"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_CHICKEN_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CHICKEN_RUN_AWAY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_RUN_RANDOM"
		"		TASK_CHICKEN_PICK_EVADE_GOAL	0"
		"		TASK_GET_PATH_TO_SAVEPOSITION	0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_CHICKEN_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CHICKEN_SQUAWK,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_JUMP"
		""
		"	Interrupts"
		"		COND_CHICKEN_ENEMY_WAY_TOO_CLOSE"
		"		COND_CHICKEN_ENEMY_TOO_CLOSE"
		"		COND_CHICKEN_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CHICKEN_FALL,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_GLIDE"
		""
		"	Interrupts"
		"		COND_CHICKEN_HIT_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CHICKEN_HIT_GROUND,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_LAND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CHICKEN_ROOST,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_CHICKEN_FIND_PATH_TO_NEST	0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_CROUCH"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_CROUCHIDLE"
		"		TASK_WAIT_INDEFINITE			0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_CHICKEN_ENEMY_TOO_CLOSE"
		"		COND_CHICKEN_ENEMY_WAY_TOO_CLOSE"
	)

AI_END_CUSTOM_NPC()
