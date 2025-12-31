#pragma once
#include "Game/WeaponDefinition.hpp"
#include "Engine/Math/AABB2.h"
// -----------------------------------------------------------------------------
class Actor;
class Timer;
class Clock;
// -----------------------------------------------------------------------------
class Weapon
{
public:
	Weapon(Actor* weaponHolder, WeaponDefinition* weaponDefinition);
	void Fire();
	void Render() const;
	EulerAngles GetRandomDirectionInCone(EulerAngles direction, float randomAmount) const;
// -----------------------------------------------------------------------------
public:
	Actor* m_owner = nullptr;
	Timer* m_refireTimer = nullptr;
	Clock* m_animationClock = nullptr;
	WeaponDefinition* m_weaponDef = nullptr;
	SpriteAnimDefinition* m_currentAnimation = nullptr;
};