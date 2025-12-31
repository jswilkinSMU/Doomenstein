#include "Game/WeaponDefinition.hpp"
#include "Game/GameCommon.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Audio/AudioSystem.hpp"

std::vector<WeaponDefinition*> WeaponDefinition::s_weaponDefinitions;

WeaponDefinition::WeaponDefinition(XmlElement const& weaponDefElement)
{
	// General
	m_weaponName = ParseXmlAttribute(weaponDefElement, "name", m_weaponName);
	m_refireTime = ParseXmlAttribute(weaponDefElement, "refireTime", m_refireTime);

	// Pistol
	m_rayCount = ParseXmlAttribute(weaponDefElement, "rayCount", m_rayCount);
	m_rayCone = ParseXmlAttribute(weaponDefElement, "rayCone", m_rayCone);
	m_rayRange = ParseXmlAttribute(weaponDefElement, "rayRange", m_rayRange);
	m_rayDamage = ParseXmlAttribute(weaponDefElement, "rayDamage", m_rayDamage);
	m_rayImpulse = ParseXmlAttribute(weaponDefElement, "rayImpulse", m_rayImpulse);

	// Plasma Rifle
	m_projectileCount = ParseXmlAttribute(weaponDefElement, "projectileCount", m_projectileCount);
	m_projectileActor = ParseXmlAttribute(weaponDefElement, "projectileActor", m_projectileActor);
	m_projectileCone = ParseXmlAttribute(weaponDefElement, "projectileCone", m_projectileCone);
	m_projectileSpeed = ParseXmlAttribute(weaponDefElement, "projectileSpeed", m_projectileSpeed);
	m_maxRange = ParseXmlAttribute(weaponDefElement, "maxRange", m_maxRange);

	// Melee
	m_meleeCount = ParseXmlAttribute(weaponDefElement, "meleeCount", m_meleeCount);
	m_meleeArc = ParseXmlAttribute(weaponDefElement, "meleeArc", m_meleeArc);
	m_meleeRange = ParseXmlAttribute(weaponDefElement, "meleeRange", m_meleeRange);
	m_meleeDamage = ParseXmlAttribute(weaponDefElement, "meleeDamage", m_meleeDamage);
	m_impMeleeDamage = ParseXmlAttribute(weaponDefElement, "impmeleeDamage", m_impMeleeDamage);
	m_meleeImpulse = ParseXmlAttribute(weaponDefElement, "meleeImpulse", m_meleeImpulse);

	// HUD
	ParseHUD(weaponDefElement);

	// Sound
	ParseSound(weaponDefElement);
}

void WeaponDefinition::InitializeWeaponsDefs()
{
	XmlDocument weaponDefsXml;
	char const* filePath = "Data/Definitions/WeaponDefinitions.xml";
	XmlError result = weaponDefsXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open required weapon definitions file \"%s\"", filePath));

	XmlElement* rootElement = weaponDefsXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "RootElement not found!");

	XmlElement* weaponDefElement = rootElement->FirstChildElement();
	while (weaponDefElement)
	{
		std::string elementName = weaponDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "WeaponDefinition", Stringf("Root child element in %s was <%s>, must be <WeaponDefinition>!", filePath, elementName.c_str()));
		WeaponDefinition* newWeaponDef = new WeaponDefinition(*weaponDefElement);
		s_weaponDefinitions.push_back(newWeaponDef);
		weaponDefElement = weaponDefElement->NextSiblingElement();
	}
}

void WeaponDefinition::ParseHUD(XmlElement const& hudDefElement)
{
	XmlElement const* hudElement = hudDefElement.FirstChildElement("HUD");
	if (!hudElement)
	{
		return;
	}

	// Parse the default shader
	std::string shader = ParseXmlAttribute(*hudElement, "shader", shader);
	if (shader != "ERRORCELL")
	{
		m_hudShader = g_theRenderer->CreateShader(shader.c_str());
	}

	// Parse the HUD textures
 	std::string baseTexture = ParseXmlAttribute(*hudElement, "baseTexture", baseTexture);
	m_baseTexture = g_theRenderer->CreateOrGetTextureFromFile(baseTexture.c_str());
	std::string reticleTexture = ParseXmlAttribute(*hudElement, "reticleTexture", reticleTexture);
	m_reticleTexture = g_theRenderer->CreateOrGetTextureFromFile(reticleTexture.c_str());

	// Parse sizes and pivot
	m_reticleSize = ParseXmlAttribute(*hudElement, "reticleSize", m_reticleSize);
	m_spriteSize = ParseXmlAttribute(*hudElement, "spriteSize", m_spriteSize);
	m_spritePivot = ParseXmlAttribute(*hudElement, "spritePivot", m_spritePivot);

	// Parse Animations
	ParseAnimation(hudElement);
}

void WeaponDefinition::ParseAnimation(XmlElement const* hudElement)
{
	for (XmlElement const* animElement = hudElement->FirstChildElement("Animation"); animElement != nullptr; animElement = animElement->NextSiblingElement("Animation"))
	{
		std::string animName = ParseXmlAttribute(*animElement, "name", "default");
		if (!animName.empty())
		{
			m_animationNames.push_back(animName);
		}

		std::string animShader = ParseXmlAttribute(*animElement, "shader", "");
		if (!animShader.empty())
		{
			m_animationShader = g_theRenderer->CreateShader(animShader.c_str());
		}

		// Parse spritesheet and animation settings
		std::string spriteSheetPath = ParseXmlAttribute(*animElement, "spriteSheet", "");
		IntVec2 cellCount = ParseXmlAttribute(*animElement, "cellCount", IntVec2(1, 1));
		int startFrame = ParseXmlAttribute(*animElement, "startFrame", 0);
		int endFrame = ParseXmlAttribute(*animElement, "endFrame", 0);
		float secondsPerFrame = ParseXmlAttribute(*animElement, "secondsPerFrame", 1.0f);

		Texture* texture = g_theRenderer->CreateOrGetTextureFromFile(spriteSheetPath.c_str());
		SpriteSheet* sheet = new SpriteSheet(*texture, cellCount);
		SpriteAnimDefinition* animDef = new SpriteAnimDefinition(*sheet, startFrame, endFrame, secondsPerFrame, SpriteAnimPlaybackType::ONCE);

		m_animationDefs.push_back(animDef);

		if (!m_spriteSheet)
		{
			m_spriteSheet = sheet;
		}
	}
}

void WeaponDefinition::ParseSound(XmlElement const& soundDefElement)
{
	XmlElement const* soundsElement = soundDefElement.FirstChildElement("Sounds");
	if (!soundsElement)
	{
		return;
	}

	XmlElement const* soundElement = soundsElement->FirstChildElement("Sound");
	if (!soundElement)
	{
		return;
	}

	m_soundName = ParseXmlAttribute(*soundElement, "sound", m_soundName);
	m_soundFilePath = ParseXmlAttribute(*soundElement, "name", m_soundFilePath);
	g_theAudio->CreateOrGetSound(m_soundFilePath);
}

WeaponDefinition* WeaponDefinition::GetByWeaponName(std::string const& name)
{
	for (int weaponDefIndex = 0; weaponDefIndex < static_cast<int>(s_weaponDefinitions.size()); ++weaponDefIndex)
	{
		if (s_weaponDefinitions[weaponDefIndex]->m_weaponName == name)
		{
			return s_weaponDefinitions[weaponDefIndex];
		}
	}
	return nullptr;
}

SpriteAnimDefinition* WeaponDefinition::GetAnimationByName(std::string const& name)
{
	for (int animIndex = 0; animIndex < static_cast<int>(m_animationNames.size()); ++animIndex)
	{
		if (m_animationNames[animIndex] == name)
		{
			return m_animationDefs[animIndex];
		}
	}
	return nullptr;
}
