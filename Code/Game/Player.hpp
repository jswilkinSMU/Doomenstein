#pragma once
#include "Game/Game.h"
#include "Game/Controller.hpp"
#include "Engine/Renderer/Camera.h"
// -----------------------------------------------------------------------------
enum class CameraMode
{
	FREEFLY_CAMERA,
	ACTOR_CAMERA
};
// -----------------------------------------------------------------------------
class Player : public Controller
{
public:
	Player(Map* map, int playerID);
	Player(Map* map, Vec3 const& position, EulerAngles const& orientation);
	~Player();

	void Update(float deltaSeconds);
	void Render() const;
	Vec3 GetForwardNormal() const;

	Camera GetPlayerCamera() const;
	Mat44 GetModelToWorldTransform() const;
	AABB2 GetNormalizedScreen() const;

	void Possess(ActorHandle& actorHandle) override;

	void ToggleCameraMode(CameraMode cameraMode);
	CameraMode m_currentCameraMode = CameraMode::ACTOR_CAMERA;
	Camera m_playerCamera;
	int    m_playerID = 0;
	int    m_kills = 0;
	int    m_deaths = 0;
	int    m_numPlayerLives = 3;

public:
	void CameraKeyPresses(float deltaSeconds);
	void CameraControllerPresses(float deltaSeconds);
	void HandleControllerPlayerMovement(float deltaSeconds);
	void HandleKeyboardPlayerMovement(float deltaSeconds);
	void CycleWeapon(Actor* actor, int direction);
	Camera m_playerViewCamera;
	Rgba8 m_color = Rgba8::WHITE;

	Vec3 m_position;
	EulerAngles m_orientation = EulerAngles::ZERO;

	// Temp variables for map assignment
	Vec3 m_raycastStart = Vec3::ZERO;
	Vec3 m_raycastEnd = Vec3::ZERO;
};