#include "Game/MapDefinition.hpp"
#include "Game/GameCommon.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Core/ErrorWarningAssert.hpp"

std::vector<MapDefinition*> MapDefinition::s_mapDefinitions;

MapDefinition::MapDefinition(XmlElement const& mapDefElement)
{
	// Parsing name
	m_name = ParseXmlAttribute(mapDefElement, "name", m_name);

	// Parsing image
	std::string imageName;
	//m_image = ParseXmlAttribute(mapDefElement, "image", imageName).c_str();
	imageName = ParseXmlAttribute(mapDefElement, "image", imageName);
	m_image = new Image(imageName.c_str());

	// Parsing shader
	std::string shaderName;
	shaderName = ParseXmlAttribute(mapDefElement, "shader", imageName).c_str();
	if (shaderName == "Default")
	{
		m_shader = nullptr;
	}
	else
	{
		m_shader = g_theRenderer->CreateShader(shaderName.c_str(), VertexType::VERTEX_PCUTBN);
	}

	// Parsing texture
	Image texture = Image(ParseXmlAttribute(mapDefElement, "spriteSheetTexture", imageName).c_str());
	m_spriteSheetTexture = g_theRenderer->CreateTextureFromImage(texture);

	// Parsing spritesheet cell count
	m_spriteSheetCellCount = ParseXmlAttribute(mapDefElement, "spriteSheetCellCount", m_spriteSheetCellCount);

	// Load SpawnInfo
	XmlElement const* spawnInfosElement = mapDefElement.FirstChildElement("SpawnInfos");
	if (spawnInfosElement)
	{
		for (XmlElement const* spawnInfoElement = spawnInfosElement->FirstChildElement("SpawnInfo");
			spawnInfoElement != nullptr; spawnInfoElement = spawnInfoElement->NextSiblingElement("SpawnInfo"))
		{
			m_spawningInfo.push_back(SpawnInfo(*spawnInfoElement));
		}
	}
}

void MapDefinition::InitializeMapDefs()
{
	XmlDocument mapDefsXml;
	char const* filePath = "Data/Definitions/MapDefinitions.xml";
	XmlError result = mapDefsXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open required map definitions file \"%s\"", filePath));

	XmlElement* rootElement = mapDefsXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "RootElement not found!");

	XmlElement* mapDefElement = rootElement->FirstChildElement();
	while (mapDefElement)
	{
		std::string elementName = mapDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "MapDefinition", Stringf("Root child element in %s was <%s>, must be <MapDefinition>!", filePath, elementName.c_str()));
		MapDefinition* newMapDef = new MapDefinition(*mapDefElement);
		s_mapDefinitions.push_back(newMapDef);
		mapDefElement = mapDefElement->NextSiblingElement();
	}
}

void MapDefinition::ClearDefinitions()
{
	s_mapDefinitions.clear();
}

MapDefinition* MapDefinition::GetByName(std::string const& name)
{
	for (int mapDefIndex = 0; mapDefIndex < static_cast<int>(s_mapDefinitions.size()); ++mapDefIndex)
	{
		if (s_mapDefinitions[mapDefIndex]->m_name == name)
		{
			return s_mapDefinitions[mapDefIndex];
		}
	}
	return nullptr;
}
// -----------------------------------------------------------------------------
SpawnInfo::SpawnInfo(XmlElement const& spawnInfoElement)
{
	m_actorName   = ParseXmlAttribute(spawnInfoElement, "actor", m_actorName);
	m_position    = ParseXmlAttribute(spawnInfoElement, "position", m_position);
	m_orientation = ParseXmlAttribute(spawnInfoElement, "orientation", m_orientation);
}

SpawnInfo::SpawnInfo()
{
}
