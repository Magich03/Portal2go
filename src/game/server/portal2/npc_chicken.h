//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: npc_chicken - simple ambient wildlife. Reconstructed from
//			decompiled F-Stop/Exposure binaries, whose schedule/task/
//			condition names and full schedule text (Tasks/Interrupts) match
//			this repo's own npc_crow.cpp closely enough that the chicken's
//			AI was almost certainly built the same way - a grounded cousin
//			of it. Wanders/idles, occasionally roosts at a HINT_PORTAL2_NEST
//			hint node (a real, pre-existing but previously unused Portal 2
//			hint type - see ai_hint.h), and reacts to being off the ground
//			(falling) vs. just having landed.
//
//			Capturable and scale-dependent, per direct user confirmation:
//			small (the default), it's afraid of npc_android and flees by
//			hopping/gliding away when threatened; scaled up big enough via
//			weapon_camera/weapon_placement, it instead hates npc_android and
//			hunts it down to peck-attack it. See IsBig()/OnCameraCaptured()/
//			OnCameraPlaced().
//
//=============================================================================//
#ifndef NPC_CHICKEN_H
#define NPC_CHICKEN_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"

//-----------------------------------------------------------------------------
// Custom schedules.
//-----------------------------------------------------------------------------
enum
{
	SCHED_CHICKEN_IDLE_STAND = LAST_SHARED_SCHEDULE,
	SCHED_CHICKEN_IDLE_WALK,
	SCHED_CHICKEN_WALK_AWAY,
	SCHED_CHICKEN_RUN_AWAY,
	SCHED_CHICKEN_FLY_AWAY,
	SCHED_CHICKEN_SQUAWK,
	SCHED_CHICKEN_FALL,
	SCHED_CHICKEN_HIT_GROUND,
	SCHED_CHICKEN_ROOST,
	SCHED_CHICKEN_CHASE_ENEMY,
	SCHED_CHICKEN_MELEE_ATTACK1,
};

//-----------------------------------------------------------------------------
// Custom tasks.
//-----------------------------------------------------------------------------
enum
{
	TASK_CHICKEN_PICK_RANDOM_GOAL = LAST_SHARED_TASK,
	TASK_CHICKEN_PICK_EVADE_GOAL,
	TASK_CHICKEN_FIND_PATH_TO_NEST,
	TASK_CHICKEN_FLY_AWAY,
};

//-----------------------------------------------------------------------------
// Custom conditions.
//-----------------------------------------------------------------------------
enum
{
	COND_CHICKEN_ENEMY_TOO_CLOSE = LAST_SHARED_CONDITION,
	COND_CHICKEN_ENEMY_WAY_TOO_CLOSE,
	COND_CHICKEN_RELEASED,
	COND_CHICKEN_OFF_GROUND,
	COND_CHICKEN_HIT_GROUND,
};

class CNPC_Chicken : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Chicken, CAI_BaseNPC );
public:

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual Class_T Classify( void );
	virtual void GatherEnemyConditions( CBaseEntity *pEnemy );
	virtual void GatherConditions( void );

	virtual void HandleAnimEvent( animevent_t *pEvent );

	virtual int MeleeAttack1Conditions( float flDot, float flDist );
	float GetPeckAttackRange( void ) const { return 48.0f; }

	virtual int SelectSchedule( void );
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );

	// CBaseAnimating capture hooks (see baseanimating.h) - freezes the
	// chicken's AI while it's held (polaroid or ghost), and on final
	// placement decides whether it's now big enough to be aggressive.
	virtual void OnCameraCaptured( void );
	virtual void OnCameraPlaced( void );

	bool IsBig( void ) const { return GetModelScale() >= 1.5f; }

	DEFINE_CUSTOM_AI;

private:
	bool ShouldRoost( void );
	CBaseEntity *PeckAttack( float flDist, int iDamage );

	bool	m_bWasOffGround;
	bool	m_bCaptured;
	float	m_flGroundIdleMoveTime;
	float	m_flNextRoostAttempt;
};

#endif // NPC_CHICKEN_H
