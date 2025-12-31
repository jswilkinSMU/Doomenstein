#pragma once
#include "Game/Tile.hpp"
#include "Game/MapDefinition.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Math/IntVec2.h"
#include "Engine/Math/RaycastUtils.hpp"
#include <vector>
// -----------------------------------------------------------------------------
struct AABB2;
struct AABB3;
struct ActorHandle;
class Game;
class Actor;
class Player;
class VertexBuffer;
class IndexBuffer;
class SpriteSheet;
// -----------------------------------------------------------------------------
//------------------------------------------------------------------------------
typedef std::vector<Actor*> ActorList;
// -----------------------------------------------------------------------------
class Map
{
public:
	Map(Game* owner, MapDefinition* definition);
	~Map();

	void CreateTiles();
	void CreateGeometry();
	void AddGeometryForWall(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const;
	void AddGeometryForFloor(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const;
	void AddGeometryForCeiling(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, AABB3 const& bounds, AABB2 const& UVs) const;
	void CreateBuffers();
	void SpawnInitialActors();

	IntVec2 GetTileCoordsForWorldPos(Vec3 const& worldPos) const;
	AABB2 GetTileBounds(int tileX, int tileY) const;
	AABB2 GetTileBounds(IntVec2 const& tileCoords) const;
	bool IsPositionInBounds(Vec3 const& position, const float tolerance = 0.0f) const;
	bool AreCoordsInBounds(int tileX, int tileY) const;
	bool AreCoordsInBounds(IntVec2 const& tileCoords) const;
	bool IsPointInsideTile(Tile const* tile, Vec3 const& start) const;
	const Tile* GetTile(int tileX, int tileY) const;
	const Tile* GetTile(IntVec2 const& tileCoords) const;
	bool IsTileSolid(int tileX, int tileY) const;
	bool IsTileSolid(IntVec2 const& tileCoords) const;

	int GetNextDirection(float direction) const;
	float GetNextCrossingDistance(int tileCoord, float direction, float startPos) const;

	// Actor helpers
	bool IsInvalidActor(Actor const* actor, Actor* otherActor);
	bool IsWithinSightRange(Actor* actor, float distanceSquared);
	bool IsActorInFOV(Actor* scout, Actor const* actor);
	bool HasLineOfSight(Actor* scout , Actor const* actor);
	bool AreAllEnemiesDead() const;

	void Update(float deltaSeconds);
	void UpdateLighting();
	void UpdateActors(float deltaSeconds);
	void CollideActors();
	void CollideActors(Actor* actorA, Actor* actorB);
	void CollideActorsWithMap();
	void CollideActorsWithMap(Actor* actor);
	void DeleteDestroyedActors();

	void Render(Player const* facingPlayer) const;
	void RenderSkyBox() const;
	void RenderMap() const;
	void RenderActors(Player const* facingPlayer) const;

	Actor* SpawnPlayer(Player* playerActor);
	Actor* SpawnActor(SpawnInfo const& spawnInfo);
	Actor* GetActorByHandle(ActorHandle handle) const;
	Actor const* GetClosestVisibleEnemy(Actor* actor);
	void   DebugPossessNext();

	RaycastResult3D RaycastAll(Vec3 const& start, Vec3 const& direction, float distance) const;
	RaycastResult3D RaycastAll(Actor const* ownerOfRaycast, ActorHandle& actorHandle, Vec3 const& start, Vec3 const& direction, float distance) const;
	RaycastResult3D RaycastWorldXY(Vec3 const& start, Vec3 const& direction, float distance) const;
	RaycastResult3D RaycastWorldZ(Vec3 const& start, Vec3 const& direction, float distance) const;
	RaycastResult3D RaycastWorldActors(Vec3 const& start, Vec3 const& direction, float distance) const;
	RaycastResult3D RaycastWorldActors(Actor const* ownerOfRaycast, ActorHandle& actorHandle, Vec3 const& start, Vec3 const& direction, float distance) const;

	Game* m_game = nullptr;

public:

	// Map
	MapDefinition* m_definition;
	std::vector<Tile> m_tiles;
	IntVec2 m_dimensions;

	// Rendering
	std::vector<Vertex_PCUTBN> m_vertexes;
	std::vector<unsigned int> m_indexes;
	Texture* m_texture = nullptr;
	SpriteSheet* m_spriteSheet = nullptr;
	Shader* m_shader = nullptr;	
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;
	Vec3 m_sunDirection = Vec3(2.f, 1.f, -1.f);
	float m_sunIntensity = 0.35f;
	float m_ambientIntensity = 0.25f;

	// Actors
	ActorList m_allActors;
	ActorList m_spawnPoints;
	unsigned int m_nextActorUID = 0;

	// Skybox
	Texture* m_skyBoxFrontTexture = nullptr;
	Texture* m_skyBoxBackTexture = nullptr;
	Texture* m_skyBoxLeftTexture = nullptr;
	Texture* m_skyBoxRightTexture = nullptr;
	Texture* m_skyBoxTopTexture = nullptr;
	Texture* m_skyBoxBottomTexture = nullptr;
};