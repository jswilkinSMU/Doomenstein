#include "Game/ActorDefinition.hpp"
#include "Game/GameCommon.h"
#include "Game/SpriteAnimationGroup.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Core/ErrorWarningAssert.hpp"

std::vector<ActorDefinition*> ActorDefinition::s_actorDefinitions;

ActorDefinition::ActorDefinition()
{
}

ActorDefinition::ActorDefinition(XmlElement const& actorDefElement)
{
	// Base
	m_actorName		 = ParseXmlAttribute(actorDefElement, "name", m_actorName);
	m_isVisible      = ParseXmlAttribute(actorDefElement, "visible", m_isVisible);
	m_health         = ParseXmlAttribute(actorDefElement, "health", m_health);
	m_corpseLifetime = ParseXmlAttribute(actorDefElement, "corpseLifetime", m_corpseLifetime);
	m_faction        = ParseXmlAttribute(actorDefElement, "faction", m_faction);
	m_canBePossessed = ParseXmlAttribute(actorDefElement, "canBePossessed", m_canBePossessed);
	m_dieOnSpawn	 = ParseXmlAttribute(actorDefElement, "dieOnSpawn", m_dieOnSpawn);

	// Collision
	ParseCollision(actorDefElement);

	// Physics
	ParsePhysics(actorDefElement);

	// Camera
	ParseCamera(actorDefElement);

	// AI
	ParseAI(actorDefElement);

	// Visuals
	ParseVisuals(actorDefElement);

	// Sounds
	ParseSounds(actorDefElement);

	// Spawning
	ParseSpawning(actorDefElement);

	// Weapon name
	ParseInventory(actorDefElement);
}

void ActorDefinition::ParseCollision(XmlElement const& actorDefElement)
{
	XmlElement const* collisionElement = actorDefElement.FirstChildElement("Collision");
	if (!collisionElement)
	{
		return;
	}

	m_physicsRadius      = ParseXmlAttribute(*collisionElement, "radius", m_physicsRadius);
	m_physicsHeight      = ParseXmlAttribute(*collisionElement, "height", m_physicsHeight);
	m_legHeight			 = ParseXmlAttribute(*collisionElement, "legHeight", m_legHeight);
	m_bodyHeight		 = ParseXmlAttribute(*collisionElement, "bodyHeight", m_bodyHeight);
	m_headHeight		 = ParseXmlAttribute(*collisionElement, "headHeight", m_headHeight);
	m_collidesWithWorld  = ParseXmlAttribute(*collisionElement, "collidesWithWorld", m_collidesWithWorld);
	m_collidesWithActors = ParseXmlAttribute(*collisionElement, "collidesWithActors", m_collidesWithActors);
	m_dieOnCollide       = ParseXmlAttribute(*collisionElement, "dieOnCollide", m_dieOnCollide);
	m_damageOnCollide    = ParseXmlAttribute(*collisionElement, "damageOnCollide", m_damageOnCollide);
	m_impulseOnCollide   = ParseXmlAttribute(*collisionElement, "impulseOnCollide", m_impulseOnCollide);
}

void ActorDefinition::ParsePhysics(XmlElement const& actorDefElement)
{
	XmlElement const* physicsElement = actorDefElement.FirstChildElement("Physics");
	if (!physicsElement)
	{
		return;
	}

	m_isSimulated = ParseXmlAttribute(*physicsElement, "simulated", m_isSimulated);
	m_isFlying	  = ParseXmlAttribute(*physicsElement, "flying", m_isFlying);
	m_walkSpeed   = ParseXmlAttribute(*physicsElement, "walkSpeed", m_walkSpeed);
	m_runSpeed    = ParseXmlAttribute(*physicsElement, "runSpeed", m_runSpeed);
	m_drag        = ParseXmlAttribute(*physicsElement, "drag", m_drag);
	m_turnSpeed   = ParseXmlAttribute(*physicsElement, "turnSpeed", m_turnSpeed);
}

void ActorDefinition::ParseCamera(XmlElement const& actorDefElement)
{
	XmlElement const* cameraElement = actorDefElement.FirstChildElement("Camera");
	if (!cameraElement)
	{
		return;
	}

	m_eyeHeight		= ParseXmlAttribute(*cameraElement, "eyeHeight", m_eyeHeight);
	m_cameraFOVDeg  = ParseXmlAttribute(*cameraElement, "cameraFOV", m_cameraFOVDeg);
}

void ActorDefinition::ParseAI(XmlElement const& actorDefElement)
{
	XmlElement const* aiElement = actorDefElement.FirstChildElement("AI");
	if (!aiElement)
	{
		return;
	}

	m_isAIEnabled = ParseXmlAttribute(*aiElement, "aiEnabled", m_isAIEnabled);
	m_sightRadius = ParseXmlAttribute(*aiElement, "sightRadius", m_sightRadius);
	m_sightAngle  = ParseXmlAttribute(*aiElement, "sightAngle", m_sightAngle);
}

void ActorDefinition::ParseVisuals(XmlElement const& actorDefElement)
{
	XmlElement const* visualElement = actorDefElement.FirstChildElement("Visuals");
	if (!visualElement)
	{
		return;
	}

	m_spriteSize = ParseXmlAttribute(*visualElement, "size", m_spriteSize);
	m_spritePivot = ParseXmlAttribute(*visualElement, "pivot", m_spritePivot);

	// Billboard parsing
	std::string billboardType = ParseXmlAttribute(*visualElement, "billboardType", billboardType);
	if (billboardType == "WorldUpFacing")
	{
 		m_billboardType = BillboardType::WORLD_UP_FACING;
	}
	else if (billboardType == "WorldUpOpposing")
	{
		m_billboardType = BillboardType::WORLD_UP_OPPOSING;
	}
	else if (billboardType == "FullOpposing")
	{
		m_billboardType = BillboardType::FULL_OPPOSING;
	}

	m_renderLit = ParseXmlAttribute(*visualElement, "renderLit", m_renderLit);
	m_renderRounded = ParseXmlAttribute(*visualElement, "renderRounded", m_renderRounded);

	// Shader parsing
	std::string shader = ParseXmlAttribute(*visualElement, "shader", shader);
	if (shader == "Default")
	{
		m_shader = nullptr;
	}
	else
	{
		m_shader = g_theRenderer->CreateShader(shader.c_str(), VertexType::VERTEX_PCUTBN);
	}

	// SpriteSheet parsing
	std::string spritesheet = ParseXmlAttribute(*visualElement, "spriteSheet", spritesheet);
	m_cellCount = ParseXmlAttribute(*visualElement, "cellCount", m_cellCount);
	Texture* spriteSheetTextureImg = g_theRenderer->CreateOrGetTextureFromFile(spritesheet.c_str());
	m_spriteSheet = new SpriteSheet(*spriteSheetTextureImg, m_cellCount);

	// AnimationGroup parsing
	for (XmlElement const* animGroupElement = visualElement->FirstChildElement("AnimationGroup"); animGroupElement != nullptr; animGroupElement = animGroupElement->NextSiblingElement("AnimationGroup"))
	{
		SpriteAnimationGroup* animGroup = new SpriteAnimationGroup(*animGroupElement, m_spriteSheet);
		m_animationGroups.push_back(animGroup);
	}
}

void ActorDefinition::ParseSounds(XmlElement const& actorDefElement)
{
	XmlElement const* soundsElement = actorDefElement.FirstChildElement("Sounds");
	if (!soundsElement)
	{
		return;
	}

	XmlElement const* soundElement = soundsElement->FirstChildElement("Sound");
	while (soundElement)
	{
		Sounds sounds;
		sounds.m_soundName = ParseXmlAttribute(*soundElement, "sound", sounds.m_soundName);
		sounds.m_soundFilePath = ParseXmlAttribute(*soundElement, "name", sounds.m_soundFilePath);
		m_sounds.push_back(sounds);
		g_theAudio->CreateOrGetSound(sounds.m_soundFilePath);

		soundElement = soundElement->NextSiblingElement("Sound");
	}
}

void ActorDefinition::ParseSpawning(XmlElement const& actorDefElement)
{
	XmlElement const* spawningElement = actorDefElement.FirstChildElement("Spawning");
	if (!spawningElement)
	{
		return;
	}

	m_enemyType = ParseXmlAttribute(*spawningElement, "enemyType", m_enemyType);
	m_spawnInterval = ParseXmlAttribute(*spawningElement, "spawnInterval", m_spawnInterval);
}

void ActorDefinition::ParseInventory(XmlElement const& actorDefElement)
{
	XmlElement const* inventoryElement = actorDefElement.FirstChildElement("Inventory");
	if (!inventoryElement)
	{
		return;
	}

	XmlElement const* weaponElement = inventoryElement->FirstChildElement("Weapon");
	while (weaponElement)
	{
		std::string weaponName = ParseXmlAttribute(*weaponElement, "name", "");
		if (!weaponName.empty())
		{
			m_weaponNames.push_back(weaponName);
		}
		weaponElement = weaponElement->NextSiblingElement("Weapon");
	}
}

void ActorDefinition::InitializeActorDefs()
{
	XmlDocument actorDefsXML;
	char const* filePath = "Data/Definitions/ActorDefinitions.xml";
	XmlError result = actorDefsXML.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open required actor definitions file \"%s\"", filePath));

	XmlElement* rootElement = actorDefsXML.RootElement();
	GUARANTEE_OR_DIE(rootElement, "RootElement not found!");

	XmlElement* actorDefElement = rootElement->FirstChildElement();
	while (actorDefElement)
	{
		std::string elementName = actorDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "ActorDefinition", Stringf("Root child element in %s was <%s>, must be <ActorDefinitions>!", filePath, elementName.c_str()));
		ActorDefinition* newActorDef = new ActorDefinition(*actorDefElement);
		s_actorDefinitions.push_back(newActorDef);
		actorDefElement = actorDefElement->NextSiblingElement();
	}
}

void ActorDefinition::InitializeProjectileActorDefs()
{
	XmlDocument actorDefsXML;
	char const* filePath = "Data/Definitions/ProjectileActorDefinitions.xml";
	XmlError result = actorDefsXML.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open required projectile actor definitions file \"%s\"", filePath));

	XmlElement* rootElement = actorDefsXML.RootElement();
	GUARANTEE_OR_DIE(rootElement, "RootElement not found!");

	XmlElement* actorDefElement = rootElement->FirstChildElement();
	while (actorDefElement)
	{
		std::string elementName = actorDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "ActorDefinition", Stringf("Root child element in %s was <%s>, must be <ActorDefinitions>!", filePath, elementName.c_str()));
		ActorDefinition* newActorDef = new ActorDefinition(*actorDefElement);
		s_actorDefinitions.push_back(newActorDef);
		actorDefElement = actorDefElement->NextSiblingElement();
	}
}

ActorDefinition* ActorDefinition::GetByActorName(std::string const& name)
{
	for (int actorDefIndex = 0; actorDefIndex < static_cast<int>(s_actorDefinitions.size()); ++actorDefIndex)
	{
		if (s_actorDefinitions[actorDefIndex]->m_actorName == name)
		{
			return s_actorDefinitions[actorDefIndex];
		}
	}
	return nullptr;
}

SpriteAnimationGroup* ActorDefinition::GetAnimationByName(std::string const& animationName)
{
	for (int animDefIndex = 0; animDefIndex < static_cast<int>(m_animationGroups.size()); ++animDefIndex)
	{
		if (m_animationGroups[animDefIndex]->m_animationGroupName == animationName)
		{
			return m_animationGroups[animDefIndex];
		}
	}
	return nullptr;
}

std::string ActorDefinition::GetSoundByName(std::string const& soundName)
{
	for (int soundIndex = 0; soundIndex < static_cast<int>(m_sounds.size()); ++soundIndex)
	{
		if (m_sounds[soundIndex].m_soundName == soundName)
		{
			return m_sounds[soundIndex].m_soundFilePath;
		}
	}
	return "default";
}
