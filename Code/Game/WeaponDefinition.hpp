#pragma once
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
// -----------------------------------------------------------------------------
class Shader;
class Texture;
// -----------------------------------------------------------------------------
struct WeaponDefinition
{
	WeaponDefinition(XmlElement const& weaponDefElement);
	static std::vector<WeaponDefinition*> s_weaponDefinitions;
	static void InitializeWeaponsDefs();
	void ParseHUD(XmlElement const& hudDefElement);
	void ParseAnimation(XmlElement const* hudElement);
	void ParseSound(XmlElement const& soundDefElement);
	static WeaponDefinition* GetByWeaponName(std::string const& name);
	SpriteAnimDefinition* GetAnimationByName(std::string const& name);
// -----------------------------------------------------------------------------
	std::string m_weaponName;
	float		m_refireTime = 0.0f;
// -----------------------------------------------------------------------------
	int			m_rayCount = 0;
	float		m_rayCone = 0.0f;
	float		m_rayRange = 0.0f;
	FloatRange  m_rayDamage = FloatRange::ZERO;
	float		m_rayImpulse = 0.0f;
// -----------------------------------------------------------------------------
	int			m_projectileCount = 0;
	std::string m_projectileActor;
	float		m_projectileCone = 0.0f;
	float		m_projectileSpeed = 0.0f;
	float       m_maxRange = 0.0f;
// -----------------------------------------------------------------------------
	int			m_meleeCount = 0;
	float		m_meleeArc = 0.0f;
	float		m_meleeRange = 0.0f;
	FloatRange  m_meleeDamage = FloatRange::ZERO;
	FloatRange  m_impMeleeDamage = FloatRange::ZERO;
	float		m_meleeImpulse = 0.0f;
// -----------------------------------------------------------------------------
	Shader*     m_hudShader = nullptr;
	Texture*    m_baseTexture = nullptr;
	Texture*    m_reticleTexture = nullptr;
	IntVec2     m_reticleSize = IntVec2::ZERO;
	Vec2        m_spriteSize = Vec2::ZERO;
	Vec2        m_spritePivot = Vec2::ZERO;
// -----------------------------------------------------------------------------
	std::vector<std::string>           m_animationNames;
	Shader*                            m_animationShader = nullptr;
	SpriteSheet*                       m_spriteSheet = nullptr;
	std::vector<SpriteAnimDefinition*> m_animationDefs;
// -----------------------------------------------------------------------------
	std::string m_soundName;
	std::string m_soundFilePath;
// -----------------------------------------------------------------------------
};