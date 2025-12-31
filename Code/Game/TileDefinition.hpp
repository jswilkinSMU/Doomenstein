#pragma once
#include "Engine/Core/Rgba8.h"
#include "Engine/Math/IntVec2.h"
#include "Engine/Core/XmlUtils.hpp"
#include <string>
// -----------------------------------------------------------------------------
struct TileDefinition
{
	TileDefinition(XmlElement const& tileDefElement);
	static std::vector<TileDefinition*> s_definitions;
	static void InitializeTileDefs();

	static TileDefinition* GetByMapColor(Rgba8 color);
	static bool  AreTexelColorsEqual(Rgba8 colorA, Rgba8 colorB);
// -----------------------------------------------------------------------------
	std::string m_name;
	bool		m_isSolid;
	Rgba8		m_mapImageColor;
	IntVec2     m_floorCoords   = IntVec2::ZERO;
	IntVec2		m_wallCoords    = IntVec2::ZERO;
	IntVec2		m_ceilingCoords = IntVec2::ZERO;
};