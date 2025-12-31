#include "Game/Map.hpp"
#include "Game/GameCommon.h"
#include "Game/Actor.hpp"
#include "Game/Player.hpp"
#include "Game/Ai.h"
#include "Game/ActorHandle.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/SpriteAnimationGroup.hpp"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/EngineCommon.h"

Map::Map(Game* owner, MapDefinition* definition)
	:m_game(owner),
	 m_definition(definition)
{
	// Get texture and shader
	m_dimensions = m_definition->m_image->GetDimensions();
	m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Terrain_8x8.png");
	m_shader = g_theRenderer->CreateOrGetShader("Data/Shaders/Diffuse", VertexType::VERTEX_PCUTBN);
	m_spriteSheet = new SpriteSheet(*m_texture, IntVec2(8, 8));

	// Get skybox textures
	m_skyBoxFrontTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_ft.png");
	m_skyBoxBackTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_bk.png");
	m_skyBoxLeftTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_lf.png");
	m_skyBoxRightTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_rt.png");
	m_skyBoxTopTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_up.png");
	m_skyBoxBottomTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_dn.png");

	// Initialize Tiles
	CreateTiles();

	// Initialize Geometry
	CreateGeometry();

	// Spawn Actors
	SpawnInitialActors();
}

Map::~Map()
{
	// Delete the vertexbuffer
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;

	// Delete the indexbuffer
	delete m_indexBuffer;
	m_indexBuffer = nullptr;

	delete m_spriteSheet;
	m_spriteSheet = nullptr;
}

void Map::CreateTiles()
{
	m_tiles.reserve(m_dimensions.x * m_dimensions.y);

	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			IntVec2 tileCoords = IntVec2(tileX, tileY);
			Vec3 tileMins = Vec3(static_cast<float>(tileX), static_cast<float>(tileY), 0.f);
			Vec3 tileMaxs = Vec3(static_cast<float>(tileX) + 1.f, static_cast<float>(tileY) + 1.f, 1.f);

			Rgba8 texColor = m_definition->m_image->GetTexelColor(tileCoords);
			TileDefinition* tileDefColor = TileDefinition::GetByMapColor(texColor);
			Tile tile = Tile(tileDefColor);
			tile.m_bounds = AABB3(tileMins, tileMaxs);
			tile.m_tileDef = tileDefColor;

			m_tiles.push_back(tile);
		}
	}
}

void Map::CreateGeometry()
{
	for (int tileIndex = 0; tileIndex < static_cast<int>(m_tiles.size()); ++tileIndex)
	{
		TileDefinition const* tileDef = m_tiles[tileIndex].m_tileDef;

		// Floor
		if (!tileDef->m_isSolid)
		{
			IntVec2 floorTileCoords = tileDef->m_floorCoords;
			AABB2 floorUVs = m_spriteSheet->GetSpriteUVCoords(floorTileCoords);
			AddGeometryForFloor(m_vertexes, m_indexes, m_tiles[tileIndex].m_bounds, floorUVs);
		}

		// Wall
		if (tileDef->m_isSolid)
		{
			IntVec2 wallTileCoords = tileDef->m_wallCoords;
			AABB2 wallUVs = m_spriteSheet->GetSpriteUVCoords(wallTileCoords);
			AddGeometryForWall(m_vertexes, m_indexes, m_tiles[tileIndex].m_bounds, wallUVs);
		}

		// Ceiling
		//if (!tileDef->m_isSolid)
		//{
		//	IntVec2 ceilingTileCoords = tileDef->m_ceilingCoords;
		//	AABB2 ceilingUVs = m_spriteSheet->GetSpriteUVCoords(ceilingTileCoords);
		//	AddGeometryForCeiling(m_vertexes, m_indexes, m_tiles[tileIndex].m_bounds, ceilingUVs);
		//}
	}

	// Initialize Buffers
	CreateBuffers();
}

void Map::AddGeometryForWall(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const
{
	float wallZDown = 0.f;
	float wallZUp = 1.f;

	Vec3 wallMinsDown     = Vec3(bounds.m_mins.x, bounds.m_mins.y, wallZDown);
	Vec3 wallMinsUp       = Vec3(bounds.m_mins.x, bounds.m_mins.y, wallZUp);
	Vec3 wallMaxXMinYDown = Vec3(bounds.m_maxs.x, bounds.m_mins.y, wallZDown);
	Vec3 wallMaxXMinYUp   = Vec3(bounds.m_maxs.x, bounds.m_mins.y, wallZUp);
	Vec3 wallMaxsDown     = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, wallZDown);
	Vec3 wallMaxsUp       = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, wallZUp);
	Vec3 wallMinXMaxYDown = Vec3(bounds.m_mins.x, bounds.m_maxs.y, wallZDown);
	Vec3 wallMinXMaxYUp   = Vec3(bounds.m_mins.x, bounds.m_maxs.y, wallZUp);

	// Mins to MaxXMinY wall
	AddVertsForQuad3D(vertexes, indexes, wallMinsDown, wallMaxXMinYDown, wallMaxXMinYUp, wallMinsUp, Rgba8::WHITE, UVs);

	// MaxXMinY to Maxs wall
	AddVertsForQuad3D(vertexes, indexes, wallMaxXMinYDown, wallMaxsDown, wallMaxsUp, wallMaxXMinYUp, Rgba8::WHITE, UVs);

	// Maxs to MinXMaxY wall
	AddVertsForQuad3D(vertexes, indexes, wallMaxsDown, wallMinXMaxYDown, wallMinXMaxYUp, wallMaxsUp, Rgba8::WHITE, UVs);

	// MinXMaxY to Mins wall
	AddVertsForQuad3D(vertexes, indexes, wallMinXMaxYDown, wallMinsDown, wallMinsUp, wallMinXMaxYUp, Rgba8::WHITE, UVs);
}

void Map::AddGeometryForFloor(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const
{
	float zFloor = 0.f;

	Vec3 floorMins	   = Vec3(bounds.m_mins.x, bounds.m_mins.y, zFloor);
	Vec3 floorMaxXMinY = Vec3(bounds.m_maxs.x, bounds.m_mins.y, zFloor);
	Vec3 floorMaxs	   = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, zFloor);
	Vec3 floorMinXMaxY = Vec3(bounds.m_mins.x, bounds.m_maxs.y, zFloor);

	AddVertsForQuad3D(vertexes, indexes, floorMins, floorMaxXMinY, floorMaxs, floorMinXMaxY, Rgba8::WHITE, UVs);
}

void Map::AddGeometryForCeiling(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const
{
	float zCeiling = 1.f;

	Vec3 ceilingMinXMaxY = Vec3(bounds.m_mins.x, bounds.m_maxs.y, zCeiling);
	Vec3 ceilingMaxs     = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, zCeiling);
	Vec3 ceilingMaxXMinY = Vec3(bounds.m_maxs.x, bounds.m_mins.y, zCeiling);
	Vec3 ceilingMins     = Vec3(bounds.m_mins.x, bounds.m_mins.y, zCeiling);

	AddVertsForQuad3D(vertexes, indexes, ceilingMinXMaxY, ceilingMaxs, ceilingMaxXMinY, ceilingMins, Rgba8::WHITE, UVs);
}

void Map::CreateBuffers()
{
	m_vertexBuffer = g_theRenderer->CreateVertexBuffer(static_cast<unsigned int>(m_vertexes.size()) * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	m_indexBuffer = g_theRenderer->CreateIndexBuffer(static_cast<unsigned int>(m_indexes.size()) * sizeof(unsigned int), sizeof(unsigned int));

	g_theRenderer->CopyCPUToGPU(m_vertexes.data(), m_vertexBuffer->GetSize(), m_vertexBuffer);
	g_theRenderer->CopyCPUToGPU(m_indexes.data(), m_indexBuffer->GetSize(), m_indexBuffer);
}

void Map::SpawnInitialActors()
{
	for (int spawnInfoIndex = 0; spawnInfoIndex < static_cast<int>(m_definition->m_spawningInfo.size()); ++spawnInfoIndex)
	{
		SpawnInfo spawnInfo = m_definition->m_spawningInfo[spawnInfoIndex];
		SpawnActor(spawnInfo);
	}
	for (Player* player : m_game->m_players)
	{
		Actor* playerActor = SpawnPlayer(player);
		player->Possess(playerActor->m_actorHandle);
	}
}

IntVec2 Map::GetTileCoordsForWorldPos(Vec3 const& worldPos) const
{
	int tileX = RoundDownToInt(worldPos.x);
	int tileY = RoundDownToInt(worldPos.y);
	return IntVec2(tileX, tileY);
}

AABB2 Map::GetTileBounds(int tileX, int tileY) const
{
	return AABB2(GetTile(tileX, tileY)->m_bounds.m_mins.x, GetTile(tileX, tileY)->m_bounds.m_mins.y, 
				 GetTile(tileX, tileY)->m_bounds.m_maxs.x, GetTile(tileX, tileY)->m_bounds.m_maxs.y);
}

AABB2 Map::GetTileBounds(IntVec2 const& tileCoords) const
{
	Vec2 mins(static_cast<float>(tileCoords.x), static_cast<float>(tileCoords.y));
	Vec2 maxs = mins + Vec2::ONE;
	return AABB2(mins, maxs);
}

bool Map::IsPositionInBounds(Vec3 const& position, const float tolerance) const
{
	Vec3 firstTile = m_tiles[0].m_bounds.m_mins;
	Tile const* finalTile = GetTile(m_dimensions.x - 1, m_dimensions.y - 1);
	Vec3 finalTileMaxs = finalTile->m_bounds.m_maxs;

	// X Check
	if (position.x - tolerance < firstTile.x || position.x + tolerance > finalTileMaxs.x)
	{
		return false;
	}

	// Y Check
	if (position.y - tolerance < firstTile.y || position.y + tolerance > finalTileMaxs.y)
	{
		return false;
	}

	// Z Check
	if (position.z - tolerance < 0 || position.z + tolerance > finalTileMaxs.z)
	{
		return false;
	}

	return true;
}

bool Map::AreCoordsInBounds(int tileX, int tileY) const
{
	if (tileX < 0 || tileY < 0)
	{
		return false;
	}
	if (tileX > m_dimensions.x - 1 || tileY > m_dimensions.y - 1)
	{
		return false;
	}
	return true;
}

bool Map::AreCoordsInBounds(IntVec2 const& tileCoords) const
{
	return AreCoordsInBounds(tileCoords.x, tileCoords.y);
}

bool Map::IsPointInsideTile(Tile const* tile, Vec3 const& start) const
{
	bool isSolid = tile->m_tileDef->m_isSolid;
	return (isSolid && start.z > 0.f && start.z < 1.f);
}

const Tile* Map::GetTile(int tileX, int tileY) const
{
	int tileIndex = (tileY * m_dimensions.x) + tileX;
	return &m_tiles[tileIndex];
}

const Tile* Map::GetTile(IntVec2 const& tileCoords) const
{
	int tileIndex = (tileCoords.y * m_dimensions.x) + tileCoords.x;
	return &m_tiles[tileIndex];
}

bool Map::IsTileSolid(int tileX, int tileY) const
{
	if (!AreCoordsInBounds(tileX, tileY))
	{
		return false;
	}

	return GetTile(tileX, tileY)->m_tileDef->m_isSolid;
}

bool Map::IsTileSolid(IntVec2 const& tileCoords) const
{
	return IsTileSolid(tileCoords.x, tileCoords.y);
}

int Map::GetNextDirection(float direction) const
{
	if (direction < 0)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

float Map::GetNextCrossingDistance(int tileCoord, float direction, float startPos) const
{
	int tileNext = GetNextDirection(direction);
	return tileCoord + (tileNext + 1) / 2.f - startPos;
}

bool Map::IsInvalidActor(Actor const* actor, Actor* otherActor)
{
	if (actor == nullptr)
	{
		return true;
	}
	if (actor == otherActor)
	{
		return true;
	}
	if (actor->m_actorDef->m_faction == otherActor->m_actorDef->m_faction)
	{
		return true;
	}
	if (actor->m_actorDef->m_faction == "NEUTRAL" || otherActor->m_actorDef->m_faction == "NEUTRAL")
	{
		return true;
	}
	return false;
}

bool Map::IsWithinSightRange(Actor* actor, float distanceSquared)
{
	float sightRadiusSquared = actor->m_actorDef->m_sightRadius * actor->m_actorDef->m_sightRadius;
	return distanceSquared <= sightRadiusSquared;
}

bool Map::IsActorInFOV(Actor* scout, Actor const* actor)
{
	Vec3 forward, left, up;
	scout->m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);

	Vec2 scoutPosXY = scout->m_position.GetXY();
	Vec2 forwardXY = forward.GetXY();
	Vec2 actorPosXY = actor->m_position.GetXY();
	Vec2 dirToActor = (actorPosXY - scoutPosXY).GetNormalized();

	return IsPointInsideDirectedSector2D(actorPosXY, scoutPosXY, forwardXY, scout->m_actorDef->m_sightAngle, scout->m_actorDef->m_sightRadius);
}

bool Map::HasLineOfSight(Actor* scout, Actor const* actor)
{
	Vec3 direction3D = actor->GetEyePosition() - scout->GetEyePosition();
	direction3D.Normalize();

	Vec2 actorPosXY = actor->m_position.GetXY();
	Vec2 scoutPosXY = scout->m_position.GetXY();

	float distanceSquared = GetDistanceSquared2D(actorPosXY, Vec2(scout->m_position.x, scout->m_position.y));
	ActorHandle resultActor;

	RaycastResult3D result = RaycastAll(scout, resultActor, scout->GetEyePosition(), direction3D, distanceSquared);
	return result.m_didImpact && IsPointInsideDisc2D(Vec2(result.m_impactPos.x, result.m_impactPos.y), actorPosXY, actor->m_physicsRadius + 0.1f);
}

bool Map::AreAllEnemiesDead() const
{
	for (int actorIndex = 0; actorIndex < static_cast<int>(m_allActors.size()); ++actorIndex)
	{
		Actor* actor = m_allActors[actorIndex];
		if (actor && !actor->IsDead() && actor->IsEnemy()) 
		{
			return false;
		}
	}
	return true;
}

void Map::Update(float deltaSeconds)
{
	UpdateLighting();
	UpdateActors(deltaSeconds);
	CollideActors();
	CollideActorsWithMap();
	DeleteDestroyedActors();

	// Respawning player
	for (Player* player : m_game->m_players)
	{
		if (player->GetActor() == nullptr)
		{
			Actor* playerActor = SpawnPlayer(player);
			if (playerActor != nullptr)
			{
				player->Possess(playerActor->m_actorHandle);
			}
		}
	}
}

void Map::UpdateLighting()
{
	m_sunDirection.Normalize();

	m_sunIntensity = GetClamped(m_sunIntensity, 0.f, 1.f);
	m_ambientIntensity = GetClamped(m_ambientIntensity, 0.f, 1.f);

	// Move sun direction x component
	if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
	{
		std::string xMessage = Stringf("Sun's current X value: %0.1f", m_sunDirection.x);

		m_sunDirection -= Vec3::XAXE;
		DebugAddMessage(xMessage, 4.f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F3))
	{
		std::string xMessage = Stringf("Sun's current X value: %0.1f", m_sunDirection.x);

		m_sunDirection += Vec3::XAXE;
		DebugAddMessage(xMessage, 4.f);
	}

	// Move sun direction y component
	if (g_theInput->WasKeyJustPressed(KEYCODE_F4))
	{
		std::string yMessage = Stringf("Sun's current Y value: %0.1f", m_sunDirection.y);

		m_sunDirection -= Vec3::YAXE;
		DebugAddMessage(yMessage, 4.f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F5))
	{
		std::string yMessage = Stringf("Sun's current Y value: %0.1f", m_sunDirection.y);

		m_sunDirection += Vec3::YAXE;
		DebugAddMessage(yMessage, 4.f);
	}

	// Decrease and Increase sun intensity
	if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
	{
		std::string sunIntensityMessage = Stringf("Sun's current intensity: %0.2f", m_sunIntensity);

		m_sunIntensity -= 0.05f;
		DebugAddMessage(sunIntensityMessage, 4.f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
	{
		std::string sunIntensityMessage = Stringf("Sun's current intensity: %0.2f", m_sunIntensity);

		m_sunIntensity += 0.05f;
		DebugAddMessage(sunIntensityMessage, 4.f);
	}

	// Decrease and Increase ambient intensity
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		std::string ambientIntensityMessage = Stringf("Current Ambient intensity: %0.2f", m_ambientIntensity);

		m_ambientIntensity -= 0.05f;
		DebugAddMessage(ambientIntensityMessage, 4.f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F9))
	{
		std::string ambientIntensityMessage = Stringf("Current Ambient intensity: %0.2f", m_ambientIntensity);

		m_ambientIntensity += 0.05f;
		DebugAddMessage(ambientIntensityMessage, 4.f);
	}
}

void Map::UpdateActors(float deltaSeconds)
{
	for (int actorIndex = 0; actorIndex < static_cast<int>(m_allActors.size()); ++actorIndex)
	{
		if (m_allActors[actorIndex] != nullptr)
		{
			m_allActors[actorIndex]->Update(deltaSeconds);
		}
	}
}

void Map::CollideActors()
{
	for (int actorAIndex = 0; actorAIndex < static_cast<int>(m_allActors.size()); ++actorAIndex)
	{
		for (int actorBIndex = actorAIndex; actorBIndex < static_cast<int>(m_allActors.size()); ++actorBIndex)
		{
			CollideActors(m_allActors[actorAIndex], m_allActors[actorBIndex]);
		}
	}
}

void Map::CollideActors(Actor* actorA, Actor* actorB)
{
	if (actorA == nullptr || actorB == nullptr)
	{
		return;
	}

	if (actorA->m_actorDef->m_actorName == "SpawnPoint" || actorB->m_actorDef->m_actorName == "SpawnPoint")
	{
		return;
	}

	Vec2 actorAPosXY = actorA->m_position.GetXY();
	Vec2 actorBPosXY = actorB->m_position.GetXY();
	float actorAStart = actorA->m_position.z;
	float actorAEnd = actorA->m_position.z + actorA->GetPhysicsHeight();
	float actorBStart = actorB->m_position.z;
	float actorBEnd = actorB->m_position.z + actorB->GetPhysicsHeight();

	if (!DoDiscsOverlap(actorAPosXY, actorA->GetPhysicsRadius(), actorBPosXY, actorB->GetPhysicsRadius()))
	{
		return;
	}

	bool overlappingOnZ = (actorAStart <= actorBEnd && actorAEnd >= actorBStart);

	if (overlappingOnZ)
	{
		if (actorA->IsMovable() && !actorB->IsMovable())
		{
			PushDiscOutOfDisc2D(actorAPosXY, actorA->GetPhysicsRadius(), actorBPosXY, actorB->GetPhysicsRadius());
			actorA->m_position.x = actorAPosXY.x;
			actorA->m_position.y = actorAPosXY.y;
			actorA->OnCollide(actorB);
		}
		else if (!actorA->IsMovable() && actorB->IsMovable())
		{
			PushDiscOutOfDisc2D(actorBPosXY, actorB->GetPhysicsRadius(), actorAPosXY, actorA->GetPhysicsRadius());
			actorB->m_position.x = actorBPosXY.x;
			actorB->m_position.y = actorBPosXY.y;
			actorB->OnCollide(actorA);
		}
		else if (actorA->IsMovable() && actorB->IsMovable())
		{
			PushDiscsOutOfEachOther2D(actorAPosXY, actorA->GetPhysicsRadius(), actorBPosXY, actorB->GetPhysicsRadius());
			actorA->m_position.x = actorAPosXY.x;
			actorA->m_position.y = actorAPosXY.y;
			actorB->m_position.x = actorBPosXY.x;
			actorB->m_position.y = actorBPosXY.y;
			actorA->OnCollide(actorB);
			actorB->OnCollide(actorA);

		}
	}
}

void Map::CollideActorsWithMap()
{
	for (int actorIndex = 0; actorIndex < static_cast<int>(m_allActors.size()); ++actorIndex)
	{
		CollideActorsWithMap(m_allActors[actorIndex]);
	}
}

void Map::CollideActorsWithMap(Actor* actor)
{
	if (actor == nullptr)
	{
		return;
	}
	Vec3& actorPos = actor->m_position;
	Vec2 actorPosXY = actor->m_position.GetXY();
	float& actorRadius = actor->m_physicsRadius;
	float& actorHeight = actor->m_physicsHeight;

	IntVec2 tileCoords = GetTileCoordsForWorldPos(actorPos);
	IntVec2 north = tileCoords + IntVec2(0, 1);
	IntVec2 south = tileCoords + IntVec2(0, -1);
	IntVec2 east = tileCoords + IntVec2(1, 0);
	IntVec2 west = tileCoords + IntVec2(-1, 0);
	IntVec2 northeast = tileCoords + IntVec2(1, 1);
	IntVec2 northwest = tileCoords + IntVec2(-1, 1);
	IntVec2 southeast = tileCoords + IntVec2(1, -1);
	IntVec2 southwest = tileCoords + IntVec2(-1, -1);

	// Floor check
	float floorZ = 0.f;
	if (actorPos.z < floorZ)
	{
		actorPos.z = floorZ;
		if (actor->m_actorDef->m_dieOnCollide && actor->m_actorDef->m_actorName == "PlasmaProjectile")
		{
			actor->m_isDead = true;
		}
	}

	// Ceiling check
	float ceilingZ = 1.f;
	if (actorPos.z + actorHeight > ceilingZ)
	{
		actorPos.z = ceilingZ - actorHeight;
		if (actor->m_actorDef->m_dieOnCollide && actor->m_actorDef->m_actorName == "PlasmaProjectile")
		{
			actor->m_isDead = true;
		}
	}

	// North check
	if (IsTileSolid(north))
	{
		if (PushDiscOutOfAABB2D(actorPosXY, actorRadius, GetTileBounds(north)))
		{
			actorPos.x = actorPosXY.x;
			actorPos.y = actorPosXY.y;
			if (actor->m_actorDef->m_dieOnCollide && actor->m_actorDef->m_actorName == "PlasmaProjectile")
			{
				actor->m_isDead = true;
			}
		}
	}

	// South check
	if (IsTileSolid(south))
	{
		if (PushDiscOutOfAABB2D(actorPosXY, actorRadius, GetTileBounds(south)))
		{
			actorPos.x = actorPosXY.x;
			actorPos.y = actorPosXY.y;
			if (actor->m_actorDef->m_dieOnCollide && actor->m_actorDef->m_actorName == "PlasmaProjectile")
			{
				actor->m_isDead = true;
			}
		}
	}

	// East check
	if (IsTileSolid(east))
	{
		if (PushDiscOutOfAABB2D(actorPosXY, actorRadius, GetTileBounds(east)))
		{
			actorPos.x = actorPosXY.x;
			actorPos.y = actorPosXY.y;
			if (actor->m_actorDef->m_dieOnCollide && actor->m_actorDef->m_actorName == "PlasmaProjectile")
			{
				actor->m_isDead = true;
			}
		}
	}

	// West Check
	if (IsTileSolid(west))
	{
		if (PushDiscOutOfAABB2D(actorPosXY, actorRadius, GetTileBounds(west)))
		{
			actorPos.x = actorPosXY.x;
			actorPos.y = actorPosXY.y;
			if (actor->m_actorDef->m_dieOnCollide && actor->m_actorDef->m_actorName == "PlasmaProjectile")
			{
				actor->m_isDead = true;
			}
		}
	}

	// Northeast check
	if (IsTileSolid(northeast))
	{
		if (PushDiscOutOfAABB2D(actorPosXY, actorRadius, GetTileBounds(northeast)))
		{
			actorPos.x = actorPosXY.x;
			actorPos.y = actorPosXY.y;
		}
	}

	// Northwest check
	if (IsTileSolid(northwest))
	{
		if (PushDiscOutOfAABB2D(actorPosXY, actorRadius, GetTileBounds(northwest)))
		{
			actorPos.x = actorPosXY.x;
			actorPos.y = actorPosXY.y;
		}
	}

	// Southeast check
	if (IsTileSolid(southeast))
	{
		if (PushDiscOutOfAABB2D(actorPosXY, actorRadius, GetTileBounds(southeast)))
		{
			actorPos.x = actorPosXY.x;
			actorPos.y = actorPosXY.y;
		}
	}

	// Southwest check
	if (IsTileSolid(southwest))
	{
		if (PushDiscOutOfAABB2D(actorPosXY, actorRadius, GetTileBounds(southwest)))
		{
			actorPos.x = actorPosXY.x;
			actorPos.y = actorPosXY.y;
		}
	}
}

void Map::DeleteDestroyedActors()
{
	for (int actorIndex = 0; actorIndex < static_cast<int>(m_allActors.size()); ++actorIndex)
	{
		Actor*& actor = m_allActors[actorIndex];
		if (actor != nullptr && actor->IsDestroyed())
		{
			delete actor;
			actor = nullptr;
		}
	}
}

void Map::Render(Player const* facingPlayer) const
{
	RenderSkyBox();
	RenderMap();
	RenderActors(facingPlayer);
}

void Map::RenderSkyBox() const
{
	Vec2 dimensionSize = Vec2(static_cast<float>(m_dimensions.x), static_cast<float>(m_dimensions.y));
	Vec3 dimensions3D = dimensionSize.GetAsVec3(1.f);
	//Vec3 dimensions3D = m_game->m_players[0]->m_position;
	AABB3 skyBoxBounds = AABB3(Vec3(-5.f, -5.f, -30.f) * dimensions3D, Vec3(5.f, 5.f, 30.f) * dimensions3D);
	Vec3 mins = skyBoxBounds.m_mins;
	Vec3 maxs = skyBoxBounds.m_maxs;

	Vec3 bottomLeftFwd = Vec3(mins.x, maxs.y, mins.z);
	Vec3 bottomRightFwd = Vec3(mins.x, mins.y, mins.z);
	Vec3 topRightFwd = Vec3(mins.x, mins.y, maxs.z);
	Vec3 topLeftFwd = Vec3(mins.x, maxs.y, maxs.z);
	Vec3 bottomLeftBack = Vec3(maxs.x, maxs.y, mins.z);
	Vec3 bottomRightBack = Vec3(maxs.x, mins.y, mins.z);
	Vec3 topRightBack = Vec3(maxs.x, mins.y, maxs.z);
	Vec3 topLeftBack = Vec3(maxs.x, maxs.y, maxs.z);

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindShader(nullptr);

	// FRONT (+X)
	std::vector<Vertex_PCU> frontVerts;
	AddVertsForQuad3D(frontVerts, bottomLeftFwd, bottomRightFwd, topRightFwd, topLeftFwd);
	g_theRenderer->BindTexture(m_skyBoxRightTexture);
	g_theRenderer->DrawVertexArray(frontVerts);

	// BACK (-X)
	std::vector<Vertex_PCU> backVerts;
	AddVertsForQuad3D(backVerts, bottomRightBack, bottomLeftBack, topLeftBack, topRightBack);
	g_theRenderer->BindTexture(m_skyBoxLeftTexture);
	g_theRenderer->DrawVertexArray(backVerts);

	// LEFT (-Y)
	std::vector<Vertex_PCU> leftVerts;
	AddVertsForQuad3D(leftVerts, bottomLeftBack, bottomLeftFwd, topLeftFwd, topLeftBack);
	g_theRenderer->BindTexture(m_skyBoxBackTexture);
	g_theRenderer->DrawVertexArray(leftVerts);

	// RIGHT (+Y)
	std::vector<Vertex_PCU> rightVerts;
	AddVertsForQuad3D(rightVerts, bottomRightFwd, bottomRightBack, topRightBack, topRightFwd);
	g_theRenderer->BindTexture(m_skyBoxFrontTexture);
	g_theRenderer->DrawVertexArray(rightVerts);

	// TOP (+Z)
	std::vector<Vertex_PCU> topVerts;
	AddVertsForQuad3D(topVerts, topLeftFwd, topRightFwd, topRightBack, topLeftBack);
	g_theRenderer->BindTexture(m_skyBoxTopTexture);
	g_theRenderer->DrawVertexArray(topVerts);

	// BOTTOM (-Z)
	std::vector<Vertex_PCU> bottomVerts;
	AddVertsForQuad3D(bottomVerts, bottomLeftBack, bottomRightBack, bottomRightFwd, bottomLeftFwd);
	g_theRenderer->BindTexture(m_skyBoxBottomTexture);
	g_theRenderer->DrawVertexArray(bottomVerts);
}

void Map::RenderMap() const
{
	LightingConstants lightingConstants = { };
	lightingConstants.m_sunDirection = m_sunDirection;
	lightingConstants.m_ambientIntensity = m_ambientIntensity;
	lightingConstants.m_sunIntensity = m_sunIntensity;
	lightingConstants.NumPointLights = 0;

	ActorDefinition const* plasmaDef = ActorDefinition::GetByActorName("PlasmaProjectile");
	for (int actorIndex = 0; actorIndex < static_cast<int>(m_allActors.size()); ++actorIndex)
	{
		if (m_allActors[actorIndex] == nullptr)
		{
			continue;
		}
		if (m_allActors[actorIndex]->m_actorDef != plasmaDef)
		{
			continue;
		}
		Vec4 actorPosAsVec4 = Vec4(m_allActors[actorIndex]->m_position.x, m_allActors[actorIndex]->m_position.y, m_allActors[actorIndex]->m_position.z, 1.f);
		lightingConstants.PointLights[lightingConstants.NumPointLights].Position = actorPosAsVec4;
		Rgba8 lightColor = Rgba8::SAPPHIRE;
		lightColor.GetAsFloats(lightingConstants.PointLights[lightingConstants.NumPointLights].Color);
		lightingConstants.NumPointLights += 1;
		if (lightingConstants.NumPointLights >= MAX_POINTLIGHTS)
		{
			break;
		}
	}

	g_theRenderer->SetLightingConstants(lightingConstants);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->BindShader(m_shader);
	g_theRenderer->DrawIndexedVertexBuffer(m_vertexBuffer, m_indexBuffer, static_cast<unsigned int>(m_indexes.size()));
}

void Map::RenderActors(Player const* facingPlayer) const
{
	for (int actorIndex = 0; actorIndex < static_cast<int>(m_allActors.size()); ++actorIndex)
	{
		if (m_allActors[actorIndex] != nullptr)
		{
			m_allActors[actorIndex]->Render(facingPlayer);
		}
	}
}

Actor* Map::SpawnPlayer(Player* playerActor)
{
	// Get a random spawn point
	Actor* spawnPoint = nullptr;
	for (int actorIndex = 0; actorIndex < static_cast<int>(m_allActors.size()); ++actorIndex)
	{
		Actor*& actor = m_allActors[actorIndex];
		if (actor != nullptr && actor->m_actorDef->m_actorName == "SpawnPoint")
		{
			spawnPoint = actor;
			break;
		}
	}

	// If a spawn point exists, spawn the player actor
	if (spawnPoint)
	{
		SpawnInfo spawnInfo;
		spawnInfo.m_actorName = "Marine";
		spawnInfo.m_position = spawnPoint->m_position;
		spawnInfo.m_orientation = spawnPoint->m_orientation;

		Actor* player = SpawnActor(spawnInfo);
		playerActor->m_theMap = this;
		return player;
	}

	return nullptr;
}

Actor* Map::SpawnActor(SpawnInfo const& spawnInfo)
{
	ActorHandle handle(m_nextActorUID++, static_cast<unsigned int>(m_allActors.size()));
	Actor* actor = new Actor(this, spawnInfo, handle);
	m_allActors.push_back(actor);

	if (actor->m_actorDef->m_isAIEnabled)
	{
		actor->m_aiController = new AI(this);
		actor->m_controller = actor->m_aiController;
		actor->m_controller->Possess(actor->m_actorHandle);
	}
	
	return actor;
}

Actor* Map::GetActorByHandle(ActorHandle handle) const
{
	if (handle.IsValid() == false)
	{
		return nullptr;
	}

	unsigned int actorIndex = handle.GetIndex();

	if (actorIndex >= m_allActors.size())
	{
		return nullptr;
	}

	Actor* actor = m_allActors[actorIndex];

	if (actor == nullptr)
	{
		return nullptr;
	}

	if (actor->m_actorHandle == handle)
	{
		return actor;
	}
	else
	{
		return nullptr;
	}
}

Actor const* Map::GetClosestVisibleEnemy(Actor* chasingActor)
{
	float closestDist = 999999.f;
	Actor const* chasedActor = nullptr;

	for (int actorIndex = 0; actorIndex < static_cast<int>(m_allActors.size()); ++actorIndex)
	{
		Actor*& actor = m_allActors[actorIndex];
		if (IsInvalidActor(actor, chasingActor))
		{
			continue;
		}

		Vec2 actorPosXY = actor->m_position.GetXY();
		Vec2 chasingActorPosXY = chasingActor->m_position.GetXY();
		float distSquared = GetDistanceSquared2D(actorPosXY, chasingActorPosXY);

		// Check if enemy is within range to move
		if (!IsWithinSightRange(chasingActor, distSquared))
		{
			continue;
		}

		// Check if enemy is within our visible arc
		if (!IsActorInFOV(chasingActor, actor))
		{
			continue;
		}

		// Check if enemy is in our line of sight
		if (!HasLineOfSight(chasingActor, actor))
		{
			continue;
		}

		if (distSquared < closestDist)
		{
			closestDist = distSquared;
			chasedActor = actor;
		}
	}
	return chasedActor;
}

void Map::DebugPossessNext()
{
	Player* player = m_game->m_players[0];
	int currentPossActorIndex = player->m_currentHandle.GetIndex();
	int nextPossIndex = currentPossActorIndex + 1;

	// Check if handle is larger than the actors
	if (nextPossIndex >= static_cast<int>(m_allActors.size()))
	{
		nextPossIndex = 0;
	}
	
	// Possess the next actor in the list that can be possessed
	for (int actorIndex = 0; actorIndex < static_cast<int>(m_allActors.size()); ++actorIndex)
	{
		int actorIndexCycle = (nextPossIndex + actorIndex) % m_allActors.size();
		Actor*& actor = m_allActors[actorIndexCycle];
		if (actor != nullptr)
		{
			if (actor->m_actorDef->m_canBePossessed)
			{
				player->Possess(actor->m_actorHandle);
				player->m_currentHandle = actor->m_actorHandle; 
				return;
			}
		}
	}
}

RaycastResult3D Map::RaycastAll(Vec3 const& start, Vec3 const& direction, float distance) const
{
	float closestResult = 999999.f;
	RaycastResult3D raycastResult;

	RaycastResult3D raycastAgainstActors = RaycastWorldActors(start, direction, distance);

	if (raycastAgainstActors.m_didImpact)
	{
		if (raycastAgainstActors.m_impactDist < closestResult)
		{
			closestResult = raycastAgainstActors.m_impactDist;
			raycastResult = raycastAgainstActors;
		}
	}

	RaycastResult3D raycastAgainstXY = RaycastWorldXY(start, direction, distance);

	if (raycastAgainstXY.m_didImpact)
	{
		if (raycastAgainstXY.m_impactDist < closestResult)
		{
			closestResult = raycastAgainstXY.m_impactDist;
			raycastResult = raycastAgainstXY;
		}
	}

	RaycastResult3D raycastAgainstZ = RaycastWorldZ(start, direction, distance);

	if (raycastAgainstZ.m_didImpact)
	{
		if (raycastAgainstZ.m_impactDist < closestResult)
		{
			closestResult = raycastAgainstZ.m_impactDist;
			raycastResult = raycastAgainstZ;
		}
	}

	return raycastResult;
}

RaycastResult3D Map::RaycastAll(Actor const* ownerOfRaycast, ActorHandle& actorHandle, Vec3 const& start, Vec3 const& direction, float distance) const
{
	float closestResult = 999999.f;
	RaycastResult3D raycastResult;

	RaycastResult3D raycastAgainstActors = RaycastWorldActors(ownerOfRaycast, actorHandle, start, direction, distance);

	if (raycastAgainstActors.m_didImpact)
	{
		if (raycastAgainstActors.m_impactDist < closestResult)
		{
			closestResult = raycastAgainstActors.m_impactDist;
			raycastResult = raycastAgainstActors;
		}
	}

	RaycastResult3D raycastAgainstXY = RaycastWorldXY(start, direction, distance);

	if (raycastAgainstXY.m_didImpact)
	{
		if (raycastAgainstXY.m_impactDist < closestResult)
		{
			closestResult = raycastAgainstXY.m_impactDist;
			raycastResult = raycastAgainstXY;
		}
	}

	RaycastResult3D raycastAgainstZ = RaycastWorldZ(start, direction, distance);

	if (raycastAgainstZ.m_didImpact)
	{
		if (raycastAgainstZ.m_impactDist < closestResult)
		{
			closestResult = raycastAgainstZ.m_impactDist;
			raycastResult = raycastAgainstZ;
		}
	}

	return raycastResult;
}

RaycastResult3D Map::RaycastWorldXY(Vec3 const& start, Vec3 const& direction, float distance) const
{
	RaycastResult3D rayCastResult;

	Vec2 startXY = start.GetXY();
	int tileX = static_cast<int>(startXY.x);
	int tileY = static_cast<int>(startXY.y);
	IntVec2 tileCoords = IntVec2(tileX, tileY);
	Tile const* tile = GetTile(tileX, tileY);

	// Is Point Inside check for tiles
	if (IsPointInsideTile(tile, start))
	{
		rayCastResult.m_didImpact = true;
		rayCastResult.m_impactDist = 0.f;
		rayCastResult.m_impactPos = start;
		rayCastResult.m_impactNormal = -direction;
		return rayCastResult;
	}

	// Early check and exit if we aren't in bounds
	if (!AreCoordsInBounds(tileX, tileY))
	{
		return rayCastResult;
	}

	int tileNextX = GetNextDirection(direction.x);
	int tileNextY = GetNextDirection(direction.y);
	float fwdDirX = 1.f / fabsf(direction.x);
	float fwdDirY = 1.f / fabsf(direction.y);

	float xDistance = GetNextCrossingDistance(tileX, direction.x, start.x);
	float forwardXDistance = fabsf(xDistance) * fwdDirX;
	float yDistance = GetNextCrossingDistance(tileY, direction.y, start.y);
	float forwardYDistance = fabsf(yDistance) * fwdDirY;

	while (true)
	{
		if (forwardXDistance < forwardYDistance)
		{
			// Check X tiles
			if (forwardXDistance > distance)
			{
				return rayCastResult;
			}

			tileX += tileNextX;

			// If next tile is solid, we have an impact
			if (IsTileSolid(tileX, tileY))
			{
				rayCastResult.m_didImpact = true;
				rayCastResult.m_impactDist = forwardXDistance;
				rayCastResult.m_impactPos = start + (direction * forwardXDistance);
				rayCastResult.m_impactNormal = (tileNextX > 0.f) ? -Vec3::XAXE : Vec3::XAXE;
				if (rayCastResult.m_impactPos.z > 0.f && rayCastResult.m_impactPos.z < 1.f)
				{
					return rayCastResult;
				}
				else
				{
					forwardXDistance += fwdDirX;
				}
			}
			else
			{
				forwardXDistance += fwdDirX;
			}
		}
		else
		{
			// Check Y tiles
			if (forwardYDistance > distance)
			{
				return rayCastResult;
			}

			tileY += tileNextY;

			// If next tile is solid, we have an impact
			if (IsTileSolid(tileX, tileY))
			{
				rayCastResult.m_didImpact = true;
				rayCastResult.m_impactDist = forwardYDistance;
				rayCastResult.m_impactPos = start + (direction * forwardYDistance);
				rayCastResult.m_impactNormal = (tileNextY > 0.f) ? -Vec3::YAXE : Vec3::YAXE;
				if (rayCastResult.m_impactPos.z > 0.f && rayCastResult.m_impactPos.z < 1.f)
				{
					return rayCastResult;
				}
				else
				{
					forwardYDistance += fwdDirY;
				}
			}
			else
			{
				forwardYDistance += fwdDirY;
			}
		}
	}
	return rayCastResult;
}

RaycastResult3D Map::RaycastWorldZ(Vec3 const& start, Vec3 const& direction, float distance) const
{
	RaycastResult3D raycastResult;

	if (direction.z == 0.0f)
	{
		return raycastResult;
	}

	float time = (direction.z > 0.f) ? (1.f - start.z) / direction.z : (0.f - start.z) / direction.z;
	Vec3 impactPos = start + direction * time;

	if (time >= 0.f && time <= distance)
	{
		if (IsPositionInBounds(impactPos))
		{
			raycastResult.m_didImpact = true;
			raycastResult.m_impactPos = impactPos;
			raycastResult.m_impactDist = time;
			if (direction.z > 0.f)
			{
				raycastResult.m_impactNormal = -Vec3::ZAXE;
			}
			else
			{
				raycastResult.m_impactNormal = Vec3::ZAXE;
			}
			return raycastResult;
		}
	}
	return raycastResult;
}

RaycastResult3D Map::RaycastWorldActors(Vec3 const& start, Vec3 const& direction, float distance) const
{
	float closestResult = 99999.f;
	RaycastResult3D raycastResult;
	for (int actorIndex = 0; actorIndex < static_cast<int>(m_allActors.size()); ++actorIndex)
	{
		Vec3& actorStart = m_allActors[actorIndex]->m_position;
		float actorRadius = m_allActors[actorIndex]->m_physicsRadius;
		float actorHeight = m_allActors[actorIndex]->m_physicsHeight;
		RaycastResult3D raycastAgainstActors = RaycastVsCylinder3D(start, direction, distance, actorStart, actorRadius, actorHeight);

		if (raycastAgainstActors.m_didImpact)
		{
			if (raycastAgainstActors.m_impactDist < closestResult)
			{
				closestResult = raycastAgainstActors.m_impactDist;
				raycastResult = raycastAgainstActors;
			}
		}
	}
	return raycastResult;
}

RaycastResult3D Map::RaycastWorldActors(Actor const* ownerOfRaycast, ActorHandle& actorHandle, Vec3 const& start, Vec3 const& direction, float distance) const
{
	actorHandle = ActorHandle::INVALID;
	float closestResult = 99999.f;
	RaycastResult3D raycastResult;
	for (int actorIndex = 0; actorIndex < static_cast<int>(m_allActors.size()); ++actorIndex)
	{
		if (m_allActors[actorIndex] == nullptr)
		{
			continue;
		}
		if (ownerOfRaycast == m_allActors[actorIndex])
		{
			continue;
		}

		Vec3 actorStart = m_allActors[actorIndex]->m_position;
		float actorRadius = m_allActors[actorIndex]->GetPhysicsRadius();
		float actorHeight = m_allActors[actorIndex]->GetPhysicsHeight();
		RaycastResult3D raycastAgainstActors = RaycastVsCylinder3D(start, direction, distance, actorStart, actorRadius, actorHeight);

		if (raycastAgainstActors.m_didImpact)
		{
			if (raycastAgainstActors.m_impactDist < closestResult)
			{
				if (m_allActors[actorIndex] == ownerOfRaycast)
				{
					//return raycastResult;
				}
				else if (!m_allActors[actorIndex]->m_isDead)
				{
					closestResult = raycastAgainstActors.m_impactDist;
					raycastResult = raycastAgainstActors;
					actorHandle   = m_allActors[actorIndex]->m_actorHandle;
				}
			}
		}
	}
	return raycastResult;
}
