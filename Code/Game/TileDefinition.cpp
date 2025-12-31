#include "Game/TileDefinition.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

std::vector<TileDefinition*> TileDefinition::s_definitions;

TileDefinition::TileDefinition(XmlElement const& tileDefElement)
{
	m_name			= ParseXmlAttribute(tileDefElement, "name", m_name);
	m_isSolid		= ParseXmlAttribute(tileDefElement, "isSolid", m_isSolid);
	m_mapImageColor = ParseXmlAttribute(tileDefElement, "mapImagePixelColor", m_mapImageColor);
	m_floorCoords   = ParseXmlAttribute(tileDefElement, "floorSpriteCoords", m_floorCoords);
	m_wallCoords	= ParseXmlAttribute(tileDefElement, "wallSpriteCoords", m_wallCoords);
	m_ceilingCoords = ParseXmlAttribute(tileDefElement, "ceilingSpriteCoords", m_ceilingCoords);
}

void TileDefinition::InitializeTileDefs()
{
	XmlDocument tileDefsXml;
	char const* filePath = "Data/Definitions/TileDefinitions.xml";
	XmlError result = tileDefsXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open required tile definitions file \"%s\"", filePath));

	XmlElement* rootElement = tileDefsXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "RootElement not found!");

	XmlElement* tileDefElement = rootElement->FirstChildElement();
	while (tileDefElement)
	{
		std::string elementName = tileDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "TileDefinition", Stringf("Root child element in %s was <%s>, must be <TileDefinition>!", filePath, elementName.c_str()));
		TileDefinition* newTileDef = new TileDefinition(*tileDefElement);
		s_definitions.push_back(newTileDef);
		tileDefElement = tileDefElement->NextSiblingElement();
	}
}

TileDefinition* TileDefinition::GetByMapColor(Rgba8 color)
{
	for (int tileDefIndex = 0; tileDefIndex < static_cast<int>(s_definitions.size()); ++tileDefIndex)
	{
		if (AreTexelColorsEqual(s_definitions[tileDefIndex]->m_mapImageColor, color))
		{
			return s_definitions[tileDefIndex];
		}
	}
	return nullptr;
}

bool TileDefinition::AreTexelColorsEqual(Rgba8 colorA, Rgba8 colorB)
{
	bool red = (colorA.r == colorB.r);
	bool green = (colorA.g == colorB.g);
	bool blue = (colorA.b == colorB.b);
	bool alpha = (colorA.a == colorB.a);
	return (red && green && blue && alpha);
}

