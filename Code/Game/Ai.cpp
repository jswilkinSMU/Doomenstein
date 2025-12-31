#include "Game/Ai.h"
#include "Game/Map.hpp"
#include "Game/Game.h"
#include "Game/Player.hpp"
#include "Game/SpriteAnimationGroup.hpp"
#include "Game/Actor.hpp"
#include "Engine/Core/EngineCommon.h"

AI::AI(Map* currentMap)
	:Controller(currentMap)
{
}

void AI::Update(float deltaseconds)
{
	Actor* self = m_theMap->GetActorByHandle(m_currentHandle);
	if (!self || self->IsDead())
	{
		return;
	}

	//-------------------------------------------------------------------------
	// Target acquisition
	//-------------------------------------------------------------------------
	Actor const* visibleEnemy = m_theMap->GetClosestVisibleEnemy(self);
	if (visibleEnemy && !visibleEnemy->IsDead() && m_targetActorHandle != visibleEnemy->m_actorHandle)
	{
		m_targetActorHandle = visibleEnemy->m_actorHandle;
	}

	Actor* target = m_theMap->GetActorByHandle(m_targetActorHandle);
	if (!target || target->IsDead())
	{
		m_targetActorHandle = ActorHandle::INVALID;
		return;
	}

	//-------------------------------------------------------------------------
	// Facing/movement
	//-------------------------------------------------------------------------
	Vec3 toTarget = target->m_position - self->m_position;

	float maxTurnDegrees = self->m_actorDef->m_turnSpeed * deltaseconds;
	self->TurnInDirection(toTarget, maxTurnDegrees);

	float distance = toTarget.GetLength();
	float combinedRadius = self->GetPhysicsRadius() + target->GetPhysicsRadius();

	if (self->m_actorDef->m_actorName != "Cacodemon" && distance > combinedRadius)
	{
		self->MoveInDirection(toTarget, self->m_actorDef->m_runSpeed);
	}

	//-------------------------------------------------------------------------
	// Weapon usage
	//-------------------------------------------------------------------------
	Weapon* weapon = self->m_equippedWeapon;
	if (!weapon)
	{
		return;
	}

	WeaponDefinition const* weaponDef = weapon->m_weaponDef;

	if (weaponDef->m_meleeCount > 0 &&
		distance < weaponDef->m_meleeRange + target->m_physicsRadius)
	{
		weapon->Fire();
	}

	if (weaponDef->m_projectileCount > 0 &&
		distance <= weaponDef->m_maxRange)
	{
		weapon->Fire();
	}
}

void AI::DamagedBy(ActorHandle& actorHandle)
{
	m_targetActorHandle = actorHandle;
}

void AI::Possess(ActorHandle& actorHandle)
{
	Controller::Possess(actorHandle);
}
