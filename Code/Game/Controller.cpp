#include "Game/Controller.hpp"
#include "Game/Actor.hpp"
#include "Engine/Core/EngineCommon.h"

Controller::Controller(Map* currentMap)
	:m_theMap(currentMap)
{
}

Controller::~Controller()
{
}

void Controller::Update(float deltaseconds)
{
	UNUSED(deltaseconds);
}

void Controller::Render() const
{
}

void Controller::Possess(ActorHandle& actorHandle)
{
	Actor* possessedActor = m_theMap->GetActorByHandle(m_currentHandle);

	// Unpossess any currently possessed actor
	if (possessedActor != nullptr && possessedActor->m_actorHandle.IsValid())
	{
		possessedActor->OnUnPossessed();
	}

	Actor* newlyPossessedActor = m_theMap->GetActorByHandle(actorHandle);

	// Possess the new actor
	if (newlyPossessedActor != nullptr && newlyPossessedActor->m_actorHandle.IsValid())
	{
		newlyPossessedActor->OnPossessed(this);
	}

	// Set the new actorhandle to be the current actor handle
	m_currentHandle = actorHandle;
}


Actor* Controller::GetActor() const
{
	return m_theMap->GetActorByHandle(m_currentHandle);
}
