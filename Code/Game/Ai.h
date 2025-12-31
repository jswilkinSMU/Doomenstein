#pragma once
#include "Game/Controller.hpp"
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class AI : public Controller
{
public:
	AI(Map* currentMap);
	void Update(float deltaseconds) override;
	void DamagedBy(ActorHandle& actorUID);
	void Possess(ActorHandle& actorHandle) override;
// -----------------------------------------------------------------------------
	ActorHandle m_targetActorHandle ;
};