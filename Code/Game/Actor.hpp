#pragma once
#include "Game/Map.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/Weapon.hpp"
#include "Engine/Core/Vertex_PCU.h"
#include "Engine/Audio/AudioSystem.hpp"
// -----------------------------------------------------------------------------
struct ActorHandle;
class Controller;
class Mat44;
class Clock;
// -----------------------------------------------------------------------------
class Actor
{
public:
	Actor(Map* owner, SpawnInfo spawnInfo, ActorHandle actorHandle);

	void InitializeActorColor();

	void Update(float deltaSeconds);

	void EnemySpawnerPulse(float deltaSeconds);

	void SpawnEnemy(float deltaSeconds);

	void UpdatePhysics(float deltaSeconds);
	void Render(Player const* facingPlayer) const;
	Mat44 GetModelToWorldTransform() const;

	void AddForce(Vec3 appliedForce);
	void AddImpulse(Vec3 appliedImpulse);
	void OnCollide(Actor* actor);
	void OnPossessed(Controller* controller);
	void OnUnPossessed();

	void Damage(float damage, Actor* attackingActor);
	void Damage(float damage, ActorHandle& attackingActor);
	void MoveInDirection(Vec3 direction, float speed);
	void TurnInDirection(Vec2 const& targetPosition, float maxTurnDegrees);
	void TurnInDirection(Vec3 dir, float maxDegrees);
	void Attack();
	void EquipWeapon(int weapon);

public:
	Vec3  GetEyePosition() const;
	Vec3  GetForwardNormal() const;
	Vec3  GetPosition() const;
	float GetPhysicsRadius() const;
	float GetPhysicsHeight() const;
	Rgba8 GetColor() const;
	bool  IsMovable() const;
	bool  IsDead() const;
	bool  IsDestroyed() const;
	bool  IsEnemy() const;
	void  PlayAnimation(std::string const& name);

	Clock* m_animationClock = nullptr;
	SpriteAnimationGroup* m_animGroup = nullptr;
// -----------------------------------------------------------------------------
public:
	Vec3 m_position = Vec3::ZERO;
	EulerAngles m_orientation = EulerAngles::ZERO;
	Vec3 m_velocity = Vec3::ZERO;
	Vec3 m_acceleration = Vec3::ZERO;
	Rgba8 m_color = Rgba8::WHITE;
	float m_physicsRadius = 0.f;
	float m_physicsHeight = 0.f;
	float m_legHeight = 0.f;
	float m_bodyHeight = 0.f;
	bool  m_isMovable = false;
	bool  m_isDead = false;
	bool  m_isDestroyed = false;
	int	  m_health = 1;
	float m_lifetime = 0.f;
	float m_enemySpawnInterval = 0.f;
	float m_timeSinceSpawn = 0.f;
	float m_colorPulseTime = 0.f;
	bool   m_isSlowed = false;
	float  m_slowTimer = 0.f;
	float  m_slowAmount = 1.f;

	Map* m_theMap = nullptr;
	Actor* m_actorFiringProjectile = nullptr;
	ActorHandle m_actorHandle = ActorHandle::INVALID;

	Controller* m_controller = nullptr;
	Controller* m_aiController = nullptr;

	SpawnInfo m_spawnInfo;
	ActorDefinition* m_actorDef;
	std::vector<Vertex_PCU> m_actorVerts;
	std::vector<Vertex_PCU> m_overlayVerts;
	std::vector<Weapon*> m_weapons;
	Weapon* m_equippedWeapon = nullptr;

	SoundID m_hurtSound;
	SoundID m_deathSound;
};