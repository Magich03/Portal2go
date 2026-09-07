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

// Binary analysis of the real compiled F-Stop assets (chicken/chicken.mdl,
// chicken/fastchicken.mdl) shows chicken.mdl alone carries every sequence
// either scale state needs: idle01/walk01/run01 (ACT_IDLE/ACT_WALK/ACT_RUN),
// peck_attack (ACT_MELEE_ATTACK1), flap/flap_falling/bounce (ACT_JUMP/
// ACT_GLIDE/ACT_LAND) and roost/roost_idle (ACT_CROUCH/ACT_CROUCHIDLE).
// fastchicken.mdl only has ref/fastchicken_run/flap/flap_falling/bounce -
// no ACT_IDLE and no ACT_MELEE_ATTACK1 sequence at all - so swapping to it
// for the "big" (attacking) state would silently break both its idle and
// its peck attack. The visual size change already comes from the F-Stop
// scale system (GetModelScale()/SetObjectScaleLevel()), so no model swap
// is needed or correct here; chicken.mdl is used for both states.
#define CHICKEN_MODEL "models/chicken/chicken.mdl"

//
// Custom animation events.
//
static int AE_CHICKEN_PECK;
static int AE_CHICKEN_FOOTSTEP_RIGHT;
static int AE_CHICKEN_FOOTSTEP_LEFT;

ConVar sk_chicken_health( "sk_chicken_health", "5", FCVAR_REPLICATED );
ConVar sk_chicken_dmg_peck( "sk_chicken_dmg_peck", "15", FCVAR_REPLICATED, "Peck damage dealt by a big (F-Stop scaled up) npc_chicken." );
ConVar chicken_enemy_too_close_dist( "chicken_enemy_too_close_dist", "300", FCVAR_REPLICATED );
ConVar chicken_enemy_way_too_close_dist( "chicken_enemy_way_too_close_dist", "150", FCVAR_REPLICATED );
ConVar chicken_flee_speed( "chicken_flee_speed", "300", FCVAR_REPLICATED, "Horizontal speed of a small npc_chicken's flee hop/glide." );
ConVar chicken_flee_upward_speed( "chicken_flee_upward_speed", "250", FCVAR_REPLICATED, "Upward speed of a small npc_chicken's flee hop/glide." );

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

	// Small (the default) fears npc_android and flees; OnCameraPlaced()
	// switches this to hate (and adds melee capability) if placed back at
	// a big enough F-Stop scale.
	AddClassRelationship( CLASS_COMBINE, D_FR, 0 );

	SetCanBeCaptured( true );

	m_bWasOffGround = false;
	m_bCaptured = false;
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

//-----------------------------------------------------------------------------
// Purpose: Freeze while held (polaroid or ghost preview) - matches every
//			other capturable prop, but a thinking NPC also needs its
//			schedule forced to something inert (see SelectSchedule()) since
//			just going non-solid/EF_NODRAW doesn't stop it trying to act.
//-----------------------------------------------------------------------------
void CNPC_Chicken::OnCameraCaptured( void )
{
	m_bCaptured = true;
	SetEnemy( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Placed back down - big enough (per IsBig()) and it turns
//			predator instead of prey. The model stays chicken.mdl in both
//			cases (see the CHICKEN_MODEL comment) - only the relationship,
//			melee capability and F-Stop render scale change.
//-----------------------------------------------------------------------------
void CNPC_Chicken::OnCameraPlaced( void )
{
	m_bCaptured = false;

	if ( IsBig() )
	{
		AddClassRelationship( CLASS_COMBINE, D_HT, 0 );
		CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1 );
	}
	else
	{
		AddClassRelationship( CLASS_COMBINE, D_FR, 0 );
		CapabilitiesRemove( bits_CAP_INNATE_MELEE_ATTACK1 );
	}

	SetHullSizeNormal();
}

int CNPC_Chicken::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( !IsBig() )
		return COND_TOO_FAR_TO_ATTACK;

	if ( flDist > GetPeckAttackRange() )
		return COND_TOO_FAR_TO_ATTACK;

	if ( flDot < 0.7f )
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: A big chicken's peck - simple hull trace in front of us.
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_Chicken::PeckAttack( float flDist, int iDamage )
{
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	return CheckTraceHullAttack( flDist, vecMins, vecMaxs, iDamage, DMG_CLUB );
}

void CNPC_Chicken::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->Event() )
	{
	case AE_CHICKEN_PECK:
		EmitSound( "NPC_Chicken.Clucks" );
		if ( IsBig() )
		{
			PeckAttack( GetPeckAttackRange(), sk_chicken_dmg_peck.GetFloat() );
		}
		return;
	case AE_CHICKEN_FOOTSTEP_RIGHT:
	case AE_CHICKEN_FOOTSTEP_LEFT:
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Matches the decompiled priority order: react to falling/landing
//			first, then (big) hunt or (small) flee, then occasionally roost,
//			otherwise idle or wander. Forced inert while captured/ghosted.
//-----------------------------------------------------------------------------
int CNPC_Chicken::SelectSchedule( void )
{
	if ( m_bCaptured )
		return SCHED_CHICKEN_IDLE_STAND;

	if ( HasCondition( COND_CHICKEN_OFF_GROUND ) )
		return SCHED_CHICKEN_FALL;

	if ( HasCondition( COND_CHICKEN_HIT_GROUND ) )
		return SCHED_CHICKEN_HIT_GROUND;

	if ( IsBig() )
	{
		if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
			return SCHED_CHICKEN_MELEE_ATTACK1;

		if ( GetEnemy() )
			return SCHED_CHICKEN_CHASE_ENEMY;
	}
	else
	{
		if ( HasCondition( COND_CHICKEN_ENEMY_WAY_TOO_CLOSE ) )
		{
			ClearCondition( COND_CHICKEN_ENEMY_WAY_TOO_CLOSE );
			return SCHED_CHICKEN_FLY_AWAY;
		}

		if ( HasCondition( COND_CHICKEN_ENEMY_TOO_CLOSE ) )
		{
			ClearCondition( COND_CHICKEN_ENEMY_TOO_CLOSE );
			return SCHED_CHICKEN_WALK_AWAY;
		}
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

	case TASK_CHICKEN_FLY_AWAY:
	{
		if ( GetEnemy() == NULL )
		{
			TaskFail( "No enemy" );
			break;
		}

		Vector vecAway = GetAbsOrigin() - GetEnemy()->GetAbsOrigin();
		vecAway.z = 0;
		VectorNormalize( vecAway );

		Vector vecVelocity = vecAway * chicken_flee_speed.GetFloat();
		vecVelocity.z = chicken_flee_upward_speed.GetFloat();

		SetGroundEntity( NULL );
		SetAbsVelocity( vecVelocity );

		TaskComplete();
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
	DECLARE_TASK( TASK_CHICKEN_FLY_AWAY )

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
		SCHED_CHICKEN_FLY_AWAY,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_CHICKEN_FLY_AWAY		0"
		""
		"	Interrupts"
		"		COND_CHICKEN_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CHICKEN_CHASE_ENEMY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		TASK_GET_CHASE_PATH_TO_ENEMY	600"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_FACE_ENEMY					0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CHICKEN_OFF_GROUND"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CHICKEN_MELEE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_MELEE_ATTACK1		0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
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
