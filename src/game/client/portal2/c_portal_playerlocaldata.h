#ifndef PORTAL_PLAYER_LOCAL_DATA_H
#define PORTAL_PLAYER_LOCAL_DATA_H

#include "cbase.h"
#include "portal_player_shared.h"
#include "hl2/hl_movedata.h"

#define CPortalPlayerLocalData C_PortalPlayerLocalData

class C_Trigger_TractorBeam;

class C_PortalPlayerLocalData
{
public:
	
	DECLARE_CLASS_NOBASE( C_PortalPlayerLocalData );
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_SIMPLE_DATADESC();
	DECLARE_PREDICTABLE();

	C_PortalPlayerLocalData();

#if USE_SLOWTIME
	bool m_bSlowingTime;
	float m_flSlowTimeRemaining;
	float m_flSlowTimeMaximum;
#endif // USE_SLOWTIME

	StickCameraState m_nStickCameraState;
	InAirState m_InAirState;
	PaintPowerType m_PaintedPowerType;

	int m_nLocatorEntityIndices;

	bool m_bShowingViewFinder;
	bool m_bPlacingPhoto;
	int m_nPhotoStackCount;
	bool m_bDoneStickInterp;
	bool m_bDoneCorrectPitch;
	bool m_bAttemptHullResize;
	bool m_bJumpedThisFrame;
	bool m_bBouncedThisFrame;
	bool m_bDuckedInAir;
	bool m_bZoomedIn;
	bool m_bPreventedCrouchJumpThisFrame;

	float m_flAirControlSupressionTime;
	float m_flAirInputScale;
	float m_flCurrentStickTime;
	float m_fBouncedTime;

	CountdownTimer m_PaintedPowerTimer;
	CachedPaintPowerChoiceResult m_CachedPaintPowerChoiceResults[PAINT_POWER_TYPE_COUNT];

	Vector m_StickNormal;
	Vector m_OldStickNormal;
	Vector m_vPreUpdateVelocity;
	Vector m_Up;
	Vector m_vStickRotationAxis;
	Vector m_StandHullMin;
	Vector m_StandHullMax;
	Vector m_DuckHullMin;
	Vector m_DuckHullMax;
	Vector m_CachedStandHullMinAttempt;
	Vector m_CachedStandHullMaxAttempt;
	Vector m_CachedDuckHullMinAttempt;
	Vector m_CachedDuckHullMaxAttempt;
	Vector m_vLocalUp;
	Vector m_vEyeOffset;

	QAngle m_qQuaternionPunch;

#ifdef NO_TRACTOR_BEAM
	EHANDLE m_hTractorBeam; // Was probably CHandle<> instead
#else
	CHandle<C_Trigger_TractorBeam> m_hTractorBeam;
#endif
	int m_nTractorBeamCount;

	//EHANDLE m_hLadder;
	//EHANDLE m_hLadder;
	//LadderMove_t	m_LadderMove;

	// Client only

	Vector m_PaintPowerHudInfoPosition[PAINT_POWER_TYPE_COUNT];
	Vector m_PaintPowerHudInfoNormal[PAINT_POWER_TYPE_COUNT];

	// Hard to tell what type this was.
	bool m_PaintPowerHudInfoValidity[PAINT_POWER_TYPE_COUNT];

	


};

EXTERN_RECV_TABLE(DT_PortalLocal);

#endif //PORTAL_PLAYER_LOCAL_DATA_H