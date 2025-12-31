#pragma once
#include "Engine/Math/AABB3.hpp"
// -----------------------------------------------------------------------------
struct TileDefinition;
// -----------------------------------------------------------------------------
struct Tile
{
	Tile() = default;
	Tile(TileDefinition* tileIndex);
	Tile(TileDefinition* tileIndex, AABB3 tileBounds);
	~Tile();
	TileDefinition const* GetTileDef() const;
// -----------------------------------------------------------------------------
	AABB3 m_bounds;
	TileDefinition* m_tileDef;
};
