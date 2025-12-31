#include "Game/Tile.hpp"
#include "Game/TileDefinition.hpp"

Tile::Tile(TileDefinition* tileIndex)
	:m_tileDef(tileIndex)
{
}

Tile::Tile(TileDefinition* tileIndex, AABB3 tileBounds)
	:m_tileDef(tileIndex),
	 m_bounds(tileBounds)
{
}

Tile::~Tile()
{
}

TileDefinition const* Tile::GetTileDef() const
{
	return m_tileDef;
}
