//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basecombatweapon.h"
#include "npcevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "weapon_ar2.h"
#include "grenade_ar2.h"
#include "gamerules.h"
#include "game.h"
#include "in_buttons.h"
#include "ai_memory.h"
#include "soundent.h"
#include "hl2_player.h"
#include "EntityFlame.h"
#include "weapon_flaregun.h"
#include "te_effect_dispatch.h"
#include "prop_combine_ball.h"
#include "beam_shared.h"
#ifdef HL2_EPISODIC 
	#include "npc_combine_episodic.h"
#else
	#include "npc_combine.h"
#endif//HL2_EPISODIC
#include "rumble_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_weapon_ar2_alt_fire_radius( "sk_weapon_ar2_alt_fire_radius", "10" );
ConVar sk_weapon_ar2_alt_fire_duration( "sk_weapon_ar2_alt_fire_duration", "2" );
ConVar sk_weapon_ar2_alt_fire_mass( "sk_weapon_ar2_alt_fire_mass", "150" );

//=========================================================
//=========================================================

BEGIN_DATADESC( CWeaponAR2 )

	DEFINE_FIELD( m_flDelayedFire,	FIELD_TIME ),
	DEFINE_FIELD( m_bShotDelayed,	FIELD_BOOLEAN ),
	//DEFINE_FIELD( m_nVentPose, FIELD_INTEGER ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CWeaponAR2, DT_WeaponAR2)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_ar2, CWeaponAR2 );
PRECACHE_WEAPON_REGISTER(weapon_ar2);

acttable_t	CWeaponAR2::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },		// FIXME: hook to AR2 unique

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },		// FIXME: hook to AR2 unique
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },		// FIXME: hook to AR2 unique
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },
};

IMPLEMENT_ACTTABLE(CWeaponAR2);

CWeaponAR2::CWeaponAR2( )
{
	m_fMinRange1	= 65;
	m_fMaxRange1	= 2048;

	m_fMinRange2	= 256;
	m_fMaxRange2	= 1024;

	m_nShotsFired	= 0;
	m_nVentPose		= -1;

	m_bAltFiresUnderwater = false;
}

void CWeaponAR2::Precache( void )
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "prop_combine_ball" );
	UTIL_PrecacheOther( "env_entity_dissolver" );
}

//-----------------------------------------------------------------------------
// Purpose: Handle grenade detonate in-air (even when no ammo is left)
//-----------------------------------------------------------------------------
void CWeaponAR2::ItemPostFrame( void )
{
	// See if we need to fire off our secondary round
	if ( m_bShotDelayed && gpGlobals->curtime > m_flDelayedFire )
	{
		DelayedAttack();
	}

	// Update our pose parameter for the vents
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner )
	{
		CBaseViewModel *pVM = pOwner->GetViewModel();

		if ( pVM )
		{
			if ( m_nVentPose == -1 )
			{
				m_nVentPose = pVM->LookupPoseParameter( "VentPoses" );
			}
			
			float flVentPose = RemapValClamped( m_nShotsFired, 0, 5, 0.0f, 1.0f );
			pVM->SetPoseParameter( m_nVentPose, flVentPose );
		}
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponAR2::GetPrimaryAttackActivity( void )
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_RECOIL1;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - 
//			nDamageType - 
//-----------------------------------------------------------------------------
void CWeaponAR2::DoImpactEffect( trace_t &tr, int nDamageType )
{
	CEffectData data;

	data.m_vOrigin = tr.endpos + ( tr.plane.normal * 1.0f );
	data.m_vNormal = tr.plane.normal;

	DispatchEffect( "AR2Impact", data );

	BaseClass::DoImpactEffect( tr, nDamageType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR2::DelayedAttack( void )
{
	m_bShotDelayed = false;
	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	// Deplete the clip completely
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	m_flNextSecondaryAttack = pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();

	// Register a muzzleflash for the AI
	pOwner->DoMuzzleFlash();
	pOwner->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
	
	WeaponSound( WPN_DOUBLE );

	pOwner->RumbleEffect(RUMBLE_SHOTGUN_DOUBLE, 0, RUMBLE_FLAG_RESTART );

	// Fire the bullets
	Vector vecSrc	 = pOwner->Weapon_ShootPosition( );
	Vector vecAiming = pOwner->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
	Vector impactPoint = vecSrc + ( vecAiming * MAX_TRACE_LENGTH );

	// Fire the bullets
	Vector vecVelocity = vecAiming * 1000.0f;

	// Fire the combine ball
	CreateCombineBall(	vecSrc, 
						vecVelocity, 
						sk_weapon_ar2_alt_fire_radius.GetFloat(), 
						sk_weapon_ar2_alt_fire_mass.GetFloat(),
						sk_weapon_ar2_alt_fire_duration.GetFloat(),
						pOwner );

	// View effects
	color32 white = {255, 255, 255, 64};
	UTIL_ScreenFade( pOwner, white, 0.1, 0, FFADE_IN  );
	
	//Disorient the player
	QAngle angles = pOwner->GetLocalAngles();

	angles.x += random->RandomInt( -4, 4 );
	angles.y += random->RandomInt( -4, 4 );
	angles.z = 0;

	pOwner->SnapEyeAngles( angles );
	
	pOwner->ViewPunch( QAngle( random->RandomInt( -8, -12 ), random->RandomInt( 1, 2 ), 0 ) );

	// Decrease ammo
	pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );

	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR2::SecondaryAttack( void )
{
	if ( m_bShotDelayed )
		return;

	// Cannot fire underwater
	if ( GetOwner() && GetOwner()->GetWaterLevel() == 3 )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		BaseClass::WeaponSound( EMPTY );
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		return;
	}

	m_bShotDelayed = true;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flDelayedFire = gpGlobals->curtime + 0.5f;

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if( pPlayer )
	{
		pPlayer->RumbleEffect(RUMBLE_AR2_ALT_FIRE, 0, RUMBLE_FLAG_RESTART );
	}

	SendWeaponAnim( ACT_VM_FIDGET );
	WeaponSound( SPECIAL1 );
}

//-----------------------------------------------------------------------------
// Purpose: Override if we're waiting to release a shot
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponAR2::CanHolster( void )
{
	if ( m_bShotDelayed )
		return false;

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: Override if we're waiting to release a shot
//-----------------------------------------------------------------------------
bool CWeaponAR2::Reload( void )
{
	if ( m_bShotDelayed )
		return false;

	return BaseClass::Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOperator - 
//-----------------------------------------------------------------------------
void CWeaponAR2::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	Vector vecShootOrigin, vecShootDir;

	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT( npc != NULL );

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
	}

	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), static_cast<int>(SOUNDENT_VOLUME_MACHINEGUN), 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );

	// NOTENOTE: This is overriden on the client-side
	// pOperator->DoMuzzleFlash();

	m_iClip1 = m_iClip1 - 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR2::FireNPCSecondaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	WeaponSound( WPN_DOUBLE );

	CNPC_Combine *pSoldier;

	pSoldier = dynamic_cast<CNPC_Combine*>(GetOwner());

	if( !pSoldier )
		return;

	// Fire!
	Vector vecSrc;
	Vector vecAiming;

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecSrc, angShootDir );
		AngleVectors( angShootDir, &vecAiming );
	}
	else 
	{
		vecSrc = pSoldier->Weapon_ShootPosition( );
		vecAiming = pSoldier->GetAltFireTarget() - vecSrc;
		VectorNormalize( vecAiming );
	}

	Vector impactPoint = vecSrc + ( vecAiming * MAX_TRACE_LENGTH );

	float flAmmoRatio = 1.0f;
	float flDuration = RemapValClamped( flAmmoRatio, 0.0f, 1.0f, 0.5f, sk_weapon_ar2_alt_fire_duration.GetFloat() );
	float flRadius = RemapValClamped( flAmmoRatio, 0.0f, 1.0f, 4.0f, sk_weapon_ar2_alt_fire_radius.GetFloat() );

	// Fire the bullets
	Vector vecVelocity = vecAiming * 1000.0f;

	// Fire the combine ball
	CreateCombineBall(	vecSrc, 
		vecVelocity, 
		flRadius, 
		sk_weapon_ar2_alt_fire_mass.GetFloat(),
		flDuration,
		pSoldier );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR2::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	if ( bSecondary )
	{
		FireNPCSecondaryAttack( pOperator, true );
	}
	else
	{
		// Ensure we have enough rounds in the clip
		m_iClip1++;

		FireNPCPrimaryAttack( pOperator, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponAR2::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{ 
		case EVENT_WEAPON_AR2:
			{
				FireNPCPrimaryAttack( pOperator, false );
			}
			break;

		case EVENT_WEAPON_AR2_ALTFIRE:
			{
				FireNPCSecondaryAttack( pOperator, false );
			}
			break;

		default:
			CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAR2::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	8.0f	//Degrees
	#define	SLIDE_LIMIT			5.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponAR2::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 3.0,		0.85	},
		{ 5.0/3.0,	0.75	},
		{ 1.00,		1.0		},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}
