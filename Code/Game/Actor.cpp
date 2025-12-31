#include "Game/Actor.hpp"
#include "Game/GameCommon.h"
#include "Game/Game.h"
#include "Game/Controller.hpp"
#include "Game/Ai.h"
#include "Game/Player.hpp"
#include "Game/SpriteAnimationGroup.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/VertexUtils.h"
#include "Engine/Core/DebugRender.hpp"

Actor::Actor(Map* owner, SpawnInfo spawnInfo, ActorHandle actorHandle)
	:m_theMap(owner),
	 m_actorDef(ActorDefinition::GetByActorName(spawnInfo.m_actorName)),
	 m_position(spawnInfo.m_position),
	 m_orientation(spawnInfo.m_orientation),
	 m_velocity(spawnInfo.m_velocity),
	 m_actorHandle(actorHandle),
	 m_animationClock(new Clock(*g_theGame->m_gameClock))
{
	m_health = m_actorDef->m_health;
	m_physicsHeight = m_actorDef->m_physicsHeight;
	m_physicsRadius = m_actorDef->m_physicsRadius;
	m_isMovable = m_actorDef->m_isSimulated;
	m_enemySpawnInterval = m_actorDef->m_spawnInterval;

	// Load weapons
	for (int weaponIndex = 0; weaponIndex < static_cast<int>(m_actorDef->m_weaponNames.size()); ++weaponIndex)
	{
		std::string const& items = m_actorDef->m_weaponNames[weaponIndex];
		WeaponDefinition* weaponName = WeaponDefinition::GetByWeaponName(items);
		if (weaponName != nullptr)
		{
			m_weapons.push_back(new Weapon(this, weaponName));
		}
	}
	if (!m_weapons.empty() && m_weapons[0] != nullptr)
	{
		m_equippedWeapon = m_weapons[0];
	}

	InitializeActorColor();

	// Create geometry if we are visible
	if (m_actorDef->m_isVisible && m_actorDef->m_actorName == "EnemySpawner")
	{
		AddVertsForCylinderZ3D(m_actorVerts, Vec3::ZERO, m_physicsRadius, m_physicsHeight, m_color);
	}

	if (m_actorDef->m_isVisible && !m_actorDef->m_animationGroups.empty())
	{
		m_animGroup = m_actorDef->m_animationGroups[0];
	}

	m_hurtSound = g_theAudio->CreateOrGetSound(m_actorDef->GetSoundByName("Hurt"));
	m_deathSound = g_theAudio->CreateOrGetSound(m_actorDef->GetSoundByName("Death"));
}

void Actor::InitializeActorColor()
{
	if (m_actorDef->m_actorName == "Marine")
	{
		m_color = Rgba8::GREEN;
	}
	if (m_actorDef->m_actorName == "Demon")
	{
		m_color = Rgba8::DARKRED;
	}
	if (m_actorDef->m_actorName == "PlasmaProjectile")
	{
		m_color = Rgba8::SAPPHIRE;
	}
	if (m_actorDef->m_actorName == "EnemySpawner")
	{
		m_color = Rgba8::BLACK;
	}
}

void Actor::Update(float deltaSeconds)
{
	if (m_isDead == true || m_actorDef->m_dieOnSpawn == true)
	{
		m_lifetime += deltaSeconds;

		// Play death animation
		PlayAnimation("Death");
	}


	if (m_lifetime > m_actorDef->m_corpseLifetime)
	{
		m_isDestroyed = true;
	}

	SpawnEnemy(deltaSeconds);
	EnemySpawnerPulse(deltaSeconds);

	if (m_animGroup == nullptr)
	{
		return;
	}

	float animDuration = m_animGroup->m_anims[0].GetDuration();
	if (m_animationClock->GetTotalSeconds() > animDuration && m_animGroup->m_playbackMode == SpriteAnimPlaybackType::ONCE)
	{
		if (m_animGroup != m_actorDef->m_animationGroups[0])
		{
			m_animGroup = m_actorDef->m_animationGroups[0];
			m_animationClock->Reset();
		}
	}
	if (m_animGroup->m_scaleBySpeed)
	{
		m_animationClock->SetTimeScale(m_velocity.GetLength() / m_actorDef->m_runSpeed);
	}
	else
	{
		m_animationClock->SetTimeScale(1.f);
	}


	if (m_actorDef->m_isSimulated)
	{
		UpdatePhysics(deltaSeconds);
		if (m_aiController)
		{
			m_aiController->Update(deltaSeconds);
		}
	}
}

void Actor::EnemySpawnerPulse(float deltaSeconds)
{
	if (m_actorDef->m_actorName == "EnemySpawner")
	{
		m_colorPulseTime += deltaSeconds;
		float pulsePeriod = 2.f;
		float fraction = 0.5f * (1.f + sinf((2.f * 3.14f / pulsePeriod) * m_colorPulseTime));
		m_color = m_color.Rgba8Interpolate(Rgba8::BLACK, Rgba8::RED, fraction);

		m_actorVerts.clear();
		AddVertsForCylinderZ3D(m_actorVerts, Vec3::ZERO, m_physicsRadius, m_physicsHeight, m_color);
	}
}

void Actor::SpawnEnemy(float deltaSeconds)
{
	if (m_actorDef->m_actorName == "EnemySpawner")
	{
		m_timeSinceSpawn += deltaSeconds;
		if (m_timeSinceSpawn >= m_enemySpawnInterval)
		{
			m_timeSinceSpawn -= m_enemySpawnInterval;
			SpawnInfo spawningInfo;
			spawningInfo.m_actorName = m_actorDef->m_enemyType;
			spawningInfo.m_position = m_position;
			m_theMap->SpawnActor(spawningInfo);
		}
	}
}

void Actor::UpdatePhysics(float deltaSeconds)
{
	if (!m_actorDef->m_isSimulated || m_isDestroyed || m_isDead)
	{
		return;
	}

	// Set non-flying actors to 0 z component
	if (m_actorDef->m_isFlying && m_actorDef->m_actorName == "LostSoul")
	{
		m_position.z = 0.35f;
	}
	else if (m_actorDef->m_isFlying == false)
	{
		m_position.z = 0.0f;
	}

	// Add a drag force equal to our drag times our negative current velocity
	Vec3 dragForce = -m_actorDef->m_drag * m_velocity;

	// Integrate acceleration, velocity, and position
	m_acceleration += dragForce;
	m_velocity += m_acceleration * deltaSeconds;
	m_position += m_velocity * deltaSeconds;

	// Clear out acceleration for next frame
	m_acceleration = Vec3::ZERO;

	// Apply slow effect to velocity if active
	if (m_isSlowed)
	{
		m_velocity *= m_slowAmount;
		m_slowTimer -= deltaSeconds;
		if (m_slowTimer <= 0.f)
		{
			m_isSlowed = false;
			m_slowAmount = 1.f;
		}
	}
}

void Actor::Render(Player const* facingPlayer) const
{
	// Draw untextured first
	g_theRenderer->SetModelConstants(GetModelToWorldTransform());
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(m_actorVerts);

	if (m_actorDef->m_actorName == "SpawnPoint" || m_actorDef->m_actorName == "EnemySpawner")
	{
		return;
	}

	if (!m_actorDef->m_isVisible)
	{
		return;
	}

	// Setting billboard types
	Vec3  eyeHeight = Vec3(0.f, 0.f, m_actorDef->m_eyeHeight);
	Mat44 localToWorldTransform;
	if (m_actorDef->m_billboardType == BillboardType::NONE)
	{
		localToWorldTransform = GetModelToWorldTransform();
	}
	else
	{
		if (m_actorDef->m_billboardType == BillboardType::WORLD_UP_FACING)
		{
			localToWorldTransform.Append(GetBillboardMatrix(BillboardType::WORLD_UP_FACING, facingPlayer->m_playerCamera.GetCameraToWorldTransform(), m_position));
		}
		else if (m_actorDef->m_billboardType == BillboardType::FULL_OPPOSING)
		{
			localToWorldTransform.Append(GetBillboardMatrix(BillboardType::FULL_OPPOSING, facingPlayer->m_playerCamera.GetCameraToWorldTransform(), m_position));
		}
		else if (m_actorDef->m_billboardType == BillboardType::WORLD_UP_OPPOSING)
		{
			localToWorldTransform.Append(GetBillboardMatrix(BillboardType::WORLD_UP_OPPOSING, facingPlayer->m_playerCamera.GetCameraToWorldTransform(), m_position + eyeHeight));
		}
		else
		{
			localToWorldTransform = GetModelToWorldTransform();
		}
	}

	Vec2 playerToActorDirectionXY = (m_position - facingPlayer->m_position).GetXY();
	Vec3 playerToActorDirection = playerToActorDirectionXY.GetNormalized().GetAsVec3();
	Vec3 viewingDirection = GetModelToWorldTransform().GetOrthonormalInverse().TransformVectorQuantity3D(playerToActorDirection);

	SpriteAnimDefinition anim = m_animGroup->GetAnimDirection(viewingDirection);
	SpriteDefinition spriteDef = anim.GetSpriteDefAtTime(static_cast<float>(m_animationClock->GetTotalSeconds()));
	AABB2 spriteUVs = spriteDef.GetUVs();
	
	Vec3 spriteOffsetSize = -Vec3(0.f, m_actorDef->m_spriteSize.x, m_actorDef->m_spriteSize.y);
	Vec3 spriteOffsetPivot = Vec3(0.f, m_actorDef->m_spritePivot.x, m_actorDef->m_spritePivot.y);
	Vec3 spriteOffset = spriteOffsetSize * spriteOffsetPivot;

	Vec3 bL = Vec3::ZERO;
	Vec3 bR = (Vec3::YAXE * m_actorDef->m_spriteSize.x);
	Vec3 tR = (Vec3::YAXE * m_actorDef->m_spriteSize.x) + (Vec3::ZAXE * m_actorDef->m_spriteSize.y);
	Vec3 tL = (Vec3::ZAXE * m_actorDef->m_spriteSize.y);

	bool isSpriteLit = m_actorDef->m_renderLit;
	std::vector<Vertex_PCU> unlitVerts;
	std::vector<Vertex_PCUTBN> litVertexes;

	if (isSpriteLit)
	{
		if (m_actorDef->m_renderRounded)
		{
			litVertexes.reserve(10000);
			AddVertsForRoundedQuad3D(litVertexes, bL, bR, tR, tL, Rgba8::WHITE, spriteUVs);
			TransformVertexArrayTBN3D(litVertexes, Mat44::MakeTranslation3D(spriteOffset));
		}
		else
		{
			litVertexes.reserve(10000);
			AddVertsForQuad3D(litVertexes, bL, bR, tR, tL, Rgba8::WHITE, spriteUVs);
			TransformVertexArrayTBN3D(litVertexes, Mat44::MakeTranslation3D(spriteOffset));
		}
		g_theRenderer->SetLightingConstants(m_theMap->m_sunDirection, m_theMap->m_sunIntensity, m_theMap->m_ambientIntensity);
		g_theRenderer->SetModelConstants(localToWorldTransform);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->BindShader(m_actorDef->m_shader);
		g_theRenderer->BindTexture(&spriteDef.GetTexture());
		g_theRenderer->DrawVertexArray(litVertexes);
		return;
	}
	if (!isSpriteLit)
	{
		unlitVerts.reserve(10000);
		AddVertsForQuad3D(unlitVerts, bL, bR, tR, tL, Rgba8::WHITE, spriteUVs);
		TransformVertexArray3D(unlitVerts, Mat44::MakeTranslation3D(spriteOffset));

		g_theRenderer->SetModelConstants(localToWorldTransform);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->BindShader(m_actorDef->m_shader);
		g_theRenderer->BindTexture(&spriteDef.GetTexture());
		g_theRenderer->DrawVertexArray(unlitVerts);
		return;
	}
}

Mat44 Actor::GetModelToWorldTransform() const
{
	Mat44 modelToWorldMatrix;
	modelToWorldMatrix.SetTranslation3D(m_position);
	EulerAngles orientation;
	orientation.m_yawDegrees = m_orientation.m_yawDegrees;
	modelToWorldMatrix.Append(orientation.GetAsMatrix_IFwd_JLeft_KUp());
	return modelToWorldMatrix;
}

void Actor::AddForce(Vec3 appliedForce)
{
	m_acceleration += appliedForce;
}

void Actor::AddImpulse(Vec3 appliedImpulse)
{
	m_velocity += appliedImpulse;
}

void Actor::OnCollide(Actor* actor)
{
	if (m_actorFiringProjectile && actor->m_actorFiringProjectile)
	{
		return;
	}

	if (actor == nullptr || m_isDead)
	{
		return;
	}

	Vec2 positionXY = m_position.GetXY();
	Vec2 actorPosXY = actor->m_position.GetXY();
	bool isOverlapping = DoDiscsOverlap(positionXY, m_physicsRadius, actorPosXY, actor->m_physicsRadius);

	// Lost soul damage
	if (actor != nullptr && m_actorDef->m_damageOnCollide.m_max > 0.f && isOverlapping && actor->m_actorDef->m_faction != m_actorDef->m_faction)
	{
		float damage = g_rng->RollRandomFloatInRange(m_actorDef->m_damageOnCollide.m_min, m_actorDef->m_damageOnCollide.m_max);
		actor->Damage(damage, m_actorHandle);
	}

	// Lost Soul death
	if (actor != nullptr && m_actorDef->m_dieOnCollide && isOverlapping)
	{
		if (actor->m_actorDef->m_actorName == "Marine")
		{
			m_isDead = true;

			// Play death sound
			g_theAudio->StartSoundAt(m_deathSound, m_position, false, 0.5f);
		}
	}

	// Projectile damage
	if (DoDiscsOverlap(positionXY, m_physicsRadius, actorPosXY, actor->m_physicsRadius))
	{
		if (m_actorFiringProjectile != nullptr)
		{
			float randomDamage = g_rng->RollRandomFloatInRange(m_actorDef->m_damageOnCollide.m_min, m_actorDef->m_damageOnCollide.m_max);
			actor->Damage(randomDamage, m_actorFiringProjectile->m_actorHandle);
		}
	}

	// Impulse
	if (actor != nullptr && m_actorDef->m_impulseOnCollide > 0.f)
	{
		actor->AddImpulse(m_actorDef->m_impulseOnCollide * GetForwardNormal());
	}

	// Projectile Death
	if (actor != nullptr && m_actorDef->m_dieOnCollide && m_actorDef->m_actorName == "PlasmaProjectile")
	{
		m_isDead = true;
	}
}

void Actor::OnPossessed(Controller* controller)
{
	m_controller = controller;
}

void Actor::OnUnPossessed()
{
	if (m_aiController)
	{
		m_controller = m_aiController;
	}
	m_controller = nullptr;
}

void Actor::Damage(float damage, Actor* attackingActor)
{
	m_health -= static_cast<int>(damage);

	// Check if damaged
	if (m_aiController)
	{
		(dynamic_cast<AI*>(m_aiController))->DamagedBy(attackingActor->m_actorHandle);
	}

	// Check if dead
	if (m_health <= 0)
	{
		m_isDead = true;
	}
}

void Actor::Damage(float damage, ActorHandle& attackingActor)
{
	if (m_isDead)
	{
		return;
	}

	m_health -= static_cast<int>(roundf(damage));

	if (m_health > 0)
	{
		g_theAudio->StartSoundAt(m_hurtSound, m_position, false, 0.2f);

		// Play hurt animation
		PlayAnimation("Hurt");
	}

	// Check if damaged
	if (m_aiController)
	{
		(dynamic_cast<AI*>(m_aiController))->DamagedBy(attackingActor);
	}

	// Check if dead
	if (m_health <= 0)
	{
		m_isDead = true;

		// Play death sound
		g_theAudio->StartSoundAt(m_deathSound, m_position, false, 0.5f);

		if (m_theMap->m_game->m_players[0]->m_numPlayerLives > 0 && m_actorDef->m_actorName == "Marine")
		{
			m_theMap->m_game->m_players[0]->m_numPlayerLives -= 1;
		}
	}
}

void Actor::MoveInDirection(Vec3 direction, float speed)
{
	Vec3 movement = direction;
	movement.Normalize();
	float accel = speed * m_actorDef->m_drag;
	AddForce(accel * movement);
}

void Actor::TurnInDirection(Vec2 const& targetPosition, float maxTurnDegrees)
{
	Vec2 actorPosXY = m_position.GetXY();
	Vec2 actorToTargetDisp = targetPosition - actorPosXY;
	float orientationToTarget = actorToTargetDisp.GetOrientationDegrees();
	m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, orientationToTarget, maxTurnDegrees);
}

void Actor::TurnInDirection(Vec3 targetDirection, float maxDegrees)
{
	Vec2 currentDir = Vec2::MakeFromPolarDegrees(m_orientation.m_yawDegrees);
	float goalAngleDegrees = targetDirection.GetAngleAboutZDegrees();
	float turnDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, goalAngleDegrees, maxDegrees);
	m_orientation.m_yawDegrees = turnDegrees;
}

void Actor::Attack()
{
	m_equippedWeapon->Fire();
}

void Actor::EquipWeapon(int weapon)
{
	m_equippedWeapon = m_weapons[weapon];
}


Vec3 Actor::GetEyePosition() const
{
	return m_position + Vec3(0.f, 0.f, m_actorDef->m_eyeHeight);
}

Vec3 Actor::GetForwardNormal() const
{
	return Vec3::MakeFromPolarDegrees(m_orientation.m_pitchDegrees, m_orientation.m_yawDegrees);
}

Vec3 Actor::GetPosition() const
{
	return m_position;
}

float Actor::GetPhysicsRadius() const
{
	return m_physicsRadius;
}

float Actor::GetPhysicsHeight() const
{
	return m_physicsHeight;
}

Rgba8 Actor::GetColor() const
{
	return m_color;
}

bool Actor::IsMovable() const
{
	return m_isMovable;
}

bool Actor::IsDead() const
{
	return m_isDead;
}

bool Actor::IsDestroyed() const
{
	return m_isDestroyed;
}

bool Actor::IsEnemy() const
{
	return m_actorDef->m_faction == "Demon";
}

void Actor::PlayAnimation(std::string const& animName)
{
	for (int animIndex = 0; animIndex < static_cast<int>(m_actorDef->m_animationGroups.size()); ++animIndex)
	{
		if (m_actorDef->m_animationGroups[animIndex]->m_animationGroupName.compare(animName) == 0)
		{
			if (m_animGroup != m_actorDef->m_animationGroups[animIndex])
			{
				m_animGroup = m_actorDef->m_animationGroups[animIndex];
				m_animationClock->Reset();
			}
			break;
		}
	}
}