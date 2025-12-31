#pragma once
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Math/MathUtils.h"
#include <map>
#include <string>
// -----------------------------------------------------------------------------
class Shader;
class SpriteAnimationGroup;
// -----------------------------------------------------------------------------
struct Sounds
{
	std::string   m_soundName;
	std::string   m_soundFilePath;
};
struct ActorDefinition
{
	ActorDefinition();
	ActorDefinition(XmlElement const& actorDefElement);
	static std::vector<ActorDefinition*> s_actorDefinitions;
// -----------------------------------------------------------------------------
	void ParseCollision(XmlElement const& actorDefElement);
	void ParsePhysics(XmlElement const& actorDefElement);
	void ParseCamera(XmlElement const& actorDefElement);
	void ParseAI(XmlElement const& actorDefElement);
	void ParseVisuals(XmlElement const& actorDefElement);
	void ParseSounds(XmlElement const& actorDefElement);
	void ParseSpawning(XmlElement const& actorDefElement);
	void ParseInventory(XmlElement const& actorDefElement);
// -----------------------------------------------------------------------------
	static void InitializeActorDefs();
	static void InitializeProjectileActorDefs();
	static ActorDefinition* GetByActorName(std::string const& name);
	SpriteAnimationGroup* GetAnimationByName(std::string const& animationName);
	std::string GetSoundByName(std::string const& soundName);
// -----------------------------------------------------------------------------
	std::string m_actorName;
	bool		m_isVisible = false;
	int			m_health = 1;
	float		m_corpseLifetime = 0.0f;
	std::string m_faction = "NEUTRAL";
	bool		m_canBePossessed = false;
	float		m_physicsRadius = 0.0f;
	float		m_physicsHeight = 0.0f;
	FloatRange  m_legHeight = FloatRange::ZERO;
	FloatRange  m_bodyHeight = FloatRange::ZERO;
	FloatRange  m_headHeight = FloatRange::ZERO;
	bool		m_collidesWithWorld = false;
	bool		m_collidesWithActors = false;
	bool		m_dieOnCollide = false;
	FloatRange  m_damageOnCollide = FloatRange::ZERO;
	float		m_impulseOnCollide = 0.0f;
	bool		m_isSimulated = false;
	bool		m_isFlying = false;
	float		m_walkSpeed = 0.0f;
	float		m_runSpeed = 0.0f;
	float		m_drag = 0.0f;
	float		m_turnSpeed = 0.0f;
	float		m_eyeHeight = 0.0f;
	float		m_cameraFOVDeg = 60.0f;
	bool		m_isAIEnabled = false;
	float		m_sightRadius = 0.0f;
	float		m_sightAngle = 0.0f;
	std::vector<std::string> m_weaponNames;
// -----------------------------------------------------------------------------
	bool          m_dieOnSpawn = false;
	Vec2		  m_spriteSize = Vec2::ONE;
	Vec2          m_spritePivot = Vec2::ONEHALF;
	BillboardType m_billboardType = BillboardType::NONE;
	bool          m_renderLit = false;
	bool		  m_renderRounded = false;
	Shader*		  m_shader = nullptr;
	SpriteSheet*  m_spriteSheet = nullptr;
	IntVec2       m_cellCount = IntVec2::ONE;
	Vec3		  m_direction = Vec3::XAXE;
	int			  m_startFrame = 0;
	int			  m_endFrame = 0;
	std::vector<SpriteAnimationGroup*> m_animationGroups;
	std::vector<Sounds> m_sounds;
	std::string   m_enemyType = "Imp";
	float         m_spawnInterval = 0.0f;
// -----------------------------------------------------------------------------
};