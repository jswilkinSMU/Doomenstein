#include "Game/Weapon.hpp"
#include "Game/Game.h"
#include "Game/GameCommon.h"
#include "Game/MapDefinition.hpp"
#include "Game/Actor.hpp"
#include "Game/Player.hpp"
#include "Game/SpriteAnimationGroup.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Math/MathUtils.h"

Weapon::Weapon(Actor* weaponHolder, WeaponDefinition* weaponDefinition)
	:m_owner(weaponHolder), m_weaponDef(weaponDefinition)
{
	m_refireTimer = new Timer(static_cast<double>(m_weaponDef->m_refireTime), g_theGame->m_gameClock);
	m_refireTimer->Start();
	m_animationClock = new Clock(*g_theGame->m_gameClock);
	m_currentAnimation = m_weaponDef->GetAnimationByName("Idle");

	// Reset the animation clock
	m_animationClock->Reset();
}

void Weapon::Fire()
{
	if (!m_refireTimer->HasPeriodElapsed() || m_owner == nullptr)
	{
		return;
	}

	m_refireTimer->Start();

	// Cache definition values
	int rayCount = m_weaponDef->m_rayCount;
	int projectileCount = m_weaponDef->m_projectileCount;
	int meleeCount = m_weaponDef->m_meleeCount;

	//-------------------------------------------------------------------------
	// Animation + sound
	//-------------------------------------------------------------------------
	m_currentAnimation = m_weaponDef->GetAnimationByName("Attack");
	m_animationClock->Reset();

	m_owner->m_animGroup = m_owner->m_actorDef->GetAnimationByName("Attack");

	if (m_owner->m_animGroup->m_scaleBySpeed)
	{
		float speedScale = m_owner->m_velocity.GetLength() / m_owner->m_actorDef->m_runSpeed;
		m_owner->m_animationClock->SetTimeScale(speedScale);
	}
	else
	{
		m_owner->m_animationClock->SetTimeScale(1.0f);
	}

	m_owner->m_animationClock->Reset();

	SoundID fireSound = g_theAudio->CreateOrGetSound(m_weaponDef->m_soundFilePath);
	g_theAudio->StartSoundAt(fireSound, m_owner->m_position, false, 0.5f);

	Vec3 forward, left, up;

	//-------------------------------------------------------------------------
	// Hitscan (ray weapons)
	//-------------------------------------------------------------------------
	while (rayCount-- > 0)
	{
		EulerAngles fireOrientation = m_owner->m_orientation;
		fireOrientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);

		ActorHandle targetHandle;
		RaycastResult3D raycastResult = m_owner->m_theMap->RaycastAll(
			m_owner,
			targetHandle,
			m_owner->GetEyePosition(),
			forward,
			10.f
		);

		if (raycastResult.m_didImpact)
		{
			SpawnInfo bulletHit;
			bulletHit.m_actorName = "BulletHit";
			bulletHit.m_position = raycastResult.m_impactPos;
			bulletHit.m_position.x = GetClamped(bulletHit.m_position.x, 0.f, 31.f);
			bulletHit.m_position.y = GetClamped(bulletHit.m_position.y, 0.f, 31.f);
			// g_theGame->m_defaultMap->SpawnActor(bulletHit);
		}

		Actor* target = m_owner->m_theMap->GetActorByHandle(targetHandle);
		if (!target || target == m_owner)
		{
			continue;
		}

		float impactZ = raycastResult.m_impactPos.z;
		float localHitZ = impactZ - target->m_position.z;

		FloatRange const& legs = target->m_actorDef->m_legHeight;
		FloatRange const& body = target->m_actorDef->m_bodyHeight;
		FloatRange const& head = target->m_actorDef->m_headHeight;

		float damage = static_cast<float>(m_weaponDef->m_rayDamage.m_min);

		if (head.isOnRange(localHitZ))
		{
			damage *= 2.0f;
			DebugAddMessage("Headshot!!!", 1.f);
		}
		else if (body.isOnRange(localHitZ))
		{
			DebugAddMessage("Bodyshot", 1.f);
		}
		else if (legs.isOnRange(localHitZ))
		{
			damage *= 0.5f;
			target->m_isSlowed = true;
			target->m_slowAmount = 0.5f;
			target->m_slowTimer = 3.0f;
			DebugAddMessage("Legshot", 1.f);
		}

		target->Damage(damage, m_owner->m_actorHandle);
		target->AddImpulse(m_weaponDef->m_rayImpulse * forward);
	}

	//-------------------------------------------------------------------------
	// Projectile weapons
	//-------------------------------------------------------------------------
	while (projectileCount-- > 0)
	{
		EulerAngles randomDir = GetRandomDirectionInCone(m_owner->m_orientation, m_weaponDef->m_projectileCone);
		randomDir.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);

		SpawnInfo spawnInfo;
		spawnInfo.m_actorName = m_weaponDef->m_projectileActor;
		spawnInfo.m_position = m_owner->GetEyePosition() + m_owner->GetForwardNormal();
		spawnInfo.m_orientation = randomDir;
		spawnInfo.m_velocity = forward * m_weaponDef->m_projectileSpeed;

		Actor* projectile = m_owner->m_theMap->SpawnActor(spawnInfo);
		projectile->m_actorFiringProjectile = m_owner;
	}

	//-------------------------------------------------------------------------
	// Melee
	//-------------------------------------------------------------------------
	while (meleeCount-- > 0)
	{
		m_owner->m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);

		Vec2 ownerPosXY = m_owner->m_position.GetXY();
		Vec2 forwardXY = forward.GetXY();

		float meleeArc = m_weaponDef->m_meleeArc * 0.5f;
		float meleeRangeSq = m_weaponDef->m_meleeRange * m_weaponDef->m_meleeRange;

		Actor* closestTarget = nullptr;
		float closestDistSq = FLT_MAX;

		for (Actor* actor : m_owner->m_theMap->m_allActors)
		{
			if (!actor || actor == m_owner)
			{
				continue;
			}

			if (actor->m_actorDef->m_faction == m_owner->m_actorDef->m_faction)
			{
				continue;
			}

			if (actor->m_actorDef->m_faction == "NEUTRAL" ||
				m_owner->m_actorDef->m_faction == "NEUTRAL")
			{
				continue;
			}

			Vec2 targetPosXY = actor->m_position.GetXY();
			float distSq = GetDistanceSquared2D(ownerPosXY, targetPosXY);
			if (distSq > meleeRangeSq)
			{
				continue;
			}

			Vec2 toTarget = (targetPosXY - ownerPosXY).GetNormalized();
			float angle = GetAngleDegreesBetweenVectors2D(forwardXY, toTarget);
			if (angle > meleeArc)
			{
				continue;
			}

			if (distSq < closestDistSq)
			{
				closestDistSq = distSq;
				closestTarget = actor;
			}
		}

		if (!closestTarget)
		{
			continue;
		}

		float damage = 0.f;

		if (m_owner->m_actorDef->m_actorName == "Demon")
		{
			damage = g_rng->RollRandomFloatInRange(
				m_weaponDef->m_meleeDamage.m_min,
				m_weaponDef->m_meleeDamage.m_max
			);
		}
		else if (m_owner->m_actorDef->m_actorName == "Imp")
		{
			damage = g_rng->RollRandomFloatInRange(m_weaponDef->m_impMeleeDamage.m_min, m_weaponDef->m_impMeleeDamage.m_max);
		}

		closestTarget->Damage(damage, m_owner->m_actorHandle);
		closestTarget->AddImpulse(m_weaponDef->m_meleeImpulse * forward);
	}
}

void Weapon::Render() const
{
	g_theRenderer->BindShader(m_weaponDef->m_hudShader);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
}

EulerAngles Weapon::GetRandomDirectionInCone(EulerAngles direction, float randomAmount) const
{
	float randomYaw = g_rng->RollRandomFloatInRange(-randomAmount, randomAmount);
	float randomPitch = g_rng->RollRandomFloatInRange(-randomAmount, randomAmount);
	float randomRoll = g_rng->RollRandomFloatInRange(-randomAmount, randomAmount);
	EulerAngles randomDirection = EulerAngles(direction.m_yawDegrees + randomYaw,
		direction.m_pitchDegrees + randomPitch,
		direction.m_rollDegrees + randomRoll);
	return randomDirection;
}