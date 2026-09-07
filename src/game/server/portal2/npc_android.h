//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: npc_android - a bare-handed melee combat NPC that swats nearby
//			physics objects out of its way while chasing an enemy.
//			Reconstructed from decompiled F-Stop/Exposure binaries. Its
//			anim event names (AE_ANDROID_ATTACK_RIGHT/LEFT/BOTH,
//			AE_ANDROID_SWATITEM/STARTSWAT, AE_ANDROID_STEP_*/SCUFF_*, etc.)
//			match this repo's own npc_BaseZombie.cpp closely enough that the
//			android's AI was almost certainly built the same way (matching
//			npc_chicken's relationship to npc_crow) - a simpler, single-enemy
//			melee NPC without the zombie's headcrab/torso/burning mechanics.
//			npc_android_missile (npc_android_missile.h/.cpp) is a ranged
//			subclass.
//
//=============================================================================//
#ifndef NPC_ANDROID_H
#define NPC_ANDROID_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"

//-----------------------------------------------------------------------------
// Custom schedules.
//-----------------------------------------------------------------------------
enum
{
	SCHED_ANDROID_MOVE_SWATITEM = LAST_SHARED_SCHEDULE,
	SCHED_ANDROID_SWATITEM,
	SCHED_ANDROID_ATTACKITEM,
	SCHED_ANDROID_CHASE_ENEMY,
	SCHED_ANDROID_WANDER_MEDIUM,
	SCHED_ANDROID_WANDER_STANDOFF,
	SCHED_ANDROID_WANDER_FAIL,
	SCHED_ANDROID_MELEE_ATTACK1,
	SCHED_ANDROID_POST_MELEE_WAIT,
	SCHED_ANDROID_MOVE_TO_AMBUSH,
	SCHED_ANDROID_WAIT_AMBUSH,
	SCHED_ANDROID_RISE,
	SCHED_ANDROID_FALL,
	SCHED_ANDROID_HIT_GROUND,
	SCHED_ANDROID_STUMBLE,
	SCHED_ANDROID_RANGE_ATTACK1,	// npc_android_missile only
};

//-----------------------------------------------------------------------------
// Custom tasks.
//-----------------------------------------------------------------------------
enum
{
	TASK_ANDROID_SWAT_ITEM = LAST_SHARED_TASK,
	TASK_ANDROID_GET_PATH_TO_PHYSOBJ,
	TASK_ANDROID_WAIT_POST_MELEE,
};

//-----------------------------------------------------------------------------
// Custom conditions.
//-----------------------------------------------------------------------------
enum
{
	COND_ANDROID_RELEASED = LAST_SHARED_CONDITION,
	COND_ANDROID_OFF_GROUND,
	COND_ANDROID_HIT_GROUND,
	COND_ANDROID_RISE_FROM_GROUND,
	COND_ANDROID_CAN_SWAT_ATTACK,
	COND_ANDROID_LOCAL_MELEE_OBSTRUCTION,
};

class CNPC_Android : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Android, CAI_BaseNPC );
public:

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual Class_T Classify( void );
	virtual void GatherConditions( void );

	virtual void HandleAnimEvent( animevent_t *pEvent );

	virtual int MeleeAttack1Conditions( float flDot, float flDist );
	virtual float GetClawAttackRange( void ) const { return 64.0f; }

	virtual int SelectSchedule( void );
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );

	// Overridden by npc_android_missile for its different model.
	virtual const char *GetAndroidModelName( void ) { return "models/bot_fem/bot_fem.mdl"; }

	DEFINE_CUSTOM_AI;

protected:
	CBaseEntity *ClawAttack( float flDist, int iDamage );
	bool FindNearestPhysicsObject( float flMaxMass );
	void SwatPhysicsObject( void );

	CHandle<CBaseEntity>	m_hPhysicsEnt;
	float					m_flNextSwatScan;
	float					m_flNextSwat;

private:
	bool	m_bWasOffGround;
};

#endif // NPC_ANDROID_H
