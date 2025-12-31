#pragma once
#include "Engine/Core/Rgba8.h"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Texture.hpp"
#include <string>
// -----------------------------------------------------------------------------
struct SpawnInfo
{
	SpawnInfo(XmlElement const& spawnInfoElement);
	SpawnInfo();

	std::string m_actorName;
	Vec3		m_position = Vec3::ZERO;
	EulerAngles m_orientation = EulerAngles::ZERO;
	Vec3		m_velocity = Vec3::ZERO;
};
// -----------------------------------------------------------------------------
struct MapDefinition
{
	MapDefinition(XmlElement const& mapDefElement);
	static std::vector<MapDefinition*> s_mapDefinitions;
// -----------------------------------------------------------------------------
	static void InitializeMapDefs();
	static void ClearDefinitions();
	static MapDefinition* GetByName(std::string const& name);
// -----------------------------------------------------------------------------
	std::string m_name;
	Image*		m_image = nullptr;
	Shader*		m_shader = nullptr;
	Texture*	m_spriteSheetTexture = nullptr;
	IntVec2		m_spriteSheetCellCount = IntVec2::ZERO;
	std::vector<SpawnInfo> m_spawningInfo;
};
// -----------------------------------------------------------------------------