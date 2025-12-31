#pragma once
#include "Game/ActorHandle.hpp"
// -----------------------------------------------------------------------------
class Actor;
class Map;
// -----------------------------------------------------------------------------
class Controller
{
public:
	Controller(Map* currentMap);
	virtual ~Controller();
	virtual void Update(float deltaseconds);
	virtual void Render() const;
	virtual void Possess(ActorHandle& actorHandle);
	Actor* GetActor() const;
	Map* m_theMap = nullptr;
	ActorHandle m_currentHandle;
// -----------------------------------------------------------------------------
protected:
	Actor* m_currentPossessedActor = nullptr;
};