#include "Game/Player.hpp"
#include "Game/Actor.hpp"
#include "Game/App.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Core/DebugRender.hpp"

Player::Player(Map* map, int playerID)
	:Controller(map), m_playerID(playerID)
{
	Mat44 cameraToRender(Vec3::ZAXE, -Vec3::XAXE, Vec3::YAXE, Vec3::ZERO);
	m_playerCamera.SetCameraToRenderTransform(cameraToRender);
	m_playerViewCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

Player::Player(Map* map, Vec3 const& position, EulerAngles const& orientation)
	:m_position(position), m_orientation(orientation), Controller(map)
{
	Mat44 cameraToRender(Vec3::ZAXE, -Vec3::XAXE, Vec3::YAXE, Vec3::ZERO);
	m_playerCamera.SetCameraToRenderTransform(cameraToRender);
	m_playerViewCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

Player::~Player()
{
}

void Player::Update(float deltaSeconds)
{
	if (m_currentCameraMode == CameraMode::ACTOR_CAMERA && g_theGame->m_currentState == GameState::PLAYING)
	{
		Actor* possessedActor = GetActor();
		if (possessedActor && !possessedActor->m_isDead)
		{
			m_playerCamera.SetPerspectiveView(2.0f, possessedActor->m_actorDef->m_cameraFOVDeg, 0.1f, 1000.f);
			m_position = Vec3(possessedActor->m_position.x, possessedActor->m_position.y, possessedActor->m_actorDef->m_eyeHeight);

			m_orientation.m_yawDegrees = possessedActor->m_orientation.m_yawDegrees;
			m_orientation.m_pitchDegrees = possessedActor->m_orientation.m_pitchDegrees;
			possessedActor->m_orientation.m_pitchDegrees = GetClamped(possessedActor->m_orientation.m_pitchDegrees, -85.f, 85.f);
			if (m_playerID == 0)
			{
				HandleControllerPlayerMovement(deltaSeconds);
			}
			else if (m_playerID == 1)
			{
				possessedActor->m_orientation.m_yawDegrees += g_theInput->GetCursorClientDelta().x * 0.075f;
				possessedActor->m_orientation.m_pitchDegrees -= g_theInput->GetCursorClientDelta().y * 0.075f;
				HandleKeyboardPlayerMovement(deltaSeconds);
			}
		}
		else if (possessedActor && possessedActor->m_isDead)
		{
			float fallSpeed = 2.0f;
			m_position.x = possessedActor->m_position.x;
			m_position.y = possessedActor->m_position.y;
			m_position.z = GetClamped(m_position.z - fallSpeed * deltaSeconds, 0.f, possessedActor->m_actorDef->m_eyeHeight);
		}
	}
	else
	{
		m_playerCamera.SetPerspectiveView(2.f, 60.f, 0.1f, 1000.f);
		m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);
		CameraKeyPresses(deltaSeconds);
		CameraControllerPresses(deltaSeconds);
	}
	m_playerCamera.SetPositionAndOrientation(m_position, m_orientation);
}

void Player::Render() const
{
	g_theRenderer->BeginCamera(m_playerViewCamera);
	if (g_theGame->m_currentState != GameState::PLAYING || m_currentCameraMode == CameraMode::FREEFLY_CAMERA)
	{
		return;
	}

	std::vector<Vertex_PCU> hudVerts;
	AABB2 screenBox = AABB2(GetNormalizedScreen().m_mins * Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y), GetNormalizedScreen().m_maxs * Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	Actor* possessedActor = GetActor();
	if (possessedActor == nullptr)
	{
		return;
	}

	Weapon* weapon = possessedActor->m_equippedWeapon;
	float weaponScale = (GetNormalizedScreen().GetDimensions().x * GetNormalizedScreen().GetDimensions().y);
	Vec2 retSize = Vec2(static_cast<float>(weapon->m_weaponDef->m_reticleSize.x), static_cast<float>(weapon->m_weaponDef->m_reticleSize.y));
	Vec2 bM = screenBox.GetPointAtUV(Vec2(0.5f, 0.128f / GetNormalizedScreen().GetDimensions().y));
	Vec2 bL = bM - Vec2((float)weapon->m_weaponDef->m_spriteSize.x * 0.5f * weaponScale, 0.f);
	Vec2 tR = bM + Vec2((float)weapon->m_weaponDef->m_spriteSize.x * 0.5f * weaponScale, (float)weapon->m_weaponDef->m_spriteSize.y * weaponScale);
	AABB2 weaponSpriteBox = AABB2(bL, tR);

	// If actor is dead draw transparent gray quad
	if (possessedActor->m_isDead)
	{
		std::vector<Vertex_PCU> deadVerts;
		AddVertsForAABB2D(deadVerts, AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), Rgba8(0, 0, 0, 80));
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(deadVerts);
	}

	// Draw HUD base texture
	AddVertsForAABB2D(hudVerts, screenBox.GetBoxAtUVs(Vec2::ZERO, Vec2(1.f, 0.128f / GetNormalizedScreen().GetDimensions().y)), Rgba8::WHITE);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindShader(possessedActor->m_equippedWeapon->m_weaponDef->m_hudShader);
	g_theRenderer->BindTexture(possessedActor->m_equippedWeapon->m_weaponDef->m_baseTexture);
	g_theRenderer->DrawVertexArray(hudVerts);


	// Draw the reticle
	std::vector<Vertex_PCU> reticleVerts;
	Vec2 screenCenter = screenBox.GetCenter();
	AddVertsForAABB2D(reticleVerts, AABB2(screenCenter - retSize * 0.5f, screenCenter + retSize * 0.5f), Rgba8::WHITE);
	g_theRenderer->BindTexture(weapon->m_weaponDef->m_reticleTexture);
	g_theRenderer->DrawVertexArray(reticleVerts);

	// Draw HUD text
	std::vector<Vertex_PCU> textVerts;
	AABB2 healthScreenTextBox = screenBox.GetBoxAtUVs(Vec2(0.25f, 0.f), Vec2(0.36f, 0.128f / GetNormalizedScreen().GetDimensions().y));
	AABB2 killScreenTextBox = screenBox.GetBoxAtUVs(Vec2(0.f, 0.f), Vec2(0.1f, 0.128f / GetNormalizedScreen().GetDimensions().y));
	AABB2 deathScreenBox = screenBox.GetBoxAtUVs(Vec2(0.85f, 0.f), Vec2(1.f, 0.128f / GetNormalizedScreen().GetDimensions().y));
	AABB2 livesScreenBox = screenBox.GetBoxAtUVs(Vec2(0.47f, 0.f), Vec2(0.8f, 0.128f / GetNormalizedScreen().GetDimensions().y));
	g_theGame->m_font->AddVertsForTextInBox2D(textVerts, Stringf("%d", static_cast<int>(possessedActor->m_health)), healthScreenTextBox, 40.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
	g_theGame->m_font->AddVertsForTextInBox2D(textVerts, Stringf("%d", m_numPlayerLives), livesScreenBox, 40.f, Rgba8::WHITE, 1.f, Vec2(0.4f, 0.4f));
	g_theGame->m_font->AddVertsForTextInBox2D(textVerts, Stringf("%d", m_deaths), deathScreenBox, 40.f, Rgba8::WHITE, 1.f, Vec2(0.6f, 0.4f));
	g_theGame->m_font->AddVertsForTextInBox2D(textVerts, Stringf("%d", m_kills), killScreenTextBox, 40.f, Rgba8::WHITE, 1.f, Vec2(0.6f, 0.4f));
	g_theRenderer->BindTexture(&g_theGame->m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);

	// Draw weapon
	std::vector<Vertex_PCU> weaponSpriteVerts;
	SpriteAnimDefinition* anim = weapon->m_currentAnimation;
	if (anim == nullptr)
	{
		anim = weapon->m_weaponDef->GetAnimationByName("Idle");
	}
	if (anim->GetDuration() < weapon->m_animationClock->GetTotalSeconds())
	{
		anim = weapon->m_weaponDef->GetAnimationByName("Idle");
	}

	SpriteDefinition const spriteAtTime = anim->GetSpriteDefAtTime(static_cast<float>(weapon->m_animationClock->GetTotalSeconds()));
	AddVertsForAABB2D(weaponSpriteVerts, weaponSpriteBox, Rgba8::WHITE, spriteAtTime.GetUVs().m_mins, spriteAtTime.GetUVs().m_maxs);
	g_theRenderer->BindShader(weapon->m_weaponDef->m_animationShader);
	g_theRenderer->BindTexture(&spriteAtTime.GetTexture());
	g_theRenderer->DrawVertexArray(weaponSpriteVerts);

	if (possessedActor->m_actorDef->m_actorName == "Marine")
	{
		if (possessedActor->m_equippedWeapon)
		{
			possessedActor->m_equippedWeapon->Render();
		}
	}
	g_theRenderer->EndCamera(m_playerViewCamera);
}

Vec3 Player::GetForwardNormal() const
{
	return Vec3::MakeFromPolarDegrees(m_orientation.m_pitchDegrees, m_orientation.m_yawDegrees);
}

Camera Player::GetPlayerCamera() const
{
	return m_playerCamera;
}

Mat44 Player::GetModelToWorldTransform() const
{
	Mat44 modelToWorldMatrix;
	modelToWorldMatrix.SetTranslation3D(m_position);
	modelToWorldMatrix.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());
	return modelToWorldMatrix;
}

AABB2 Player::GetNormalizedScreen() const
{
	if (g_theGame->m_players.size() == 2)
	{
		if (m_playerID == 0)
		{
			// Top Viewport
			return AABB2(Vec2(0.f, 0.5f), Vec2(1.f, 1.f));
		}
		else
		{
			// Bottom Viewport
			return AABB2(Vec2(0.f, 0.f), Vec2(1.f, 0.5f));
		}
	}
	return AABB2(Vec2(0.f, 0.f), Vec2(1.f, 1.f));
}

void Player::Possess(ActorHandle& actorHandle)
{
	Controller::Possess(actorHandle);
}

void Player::ToggleCameraMode(CameraMode cameraMode)
{
	m_currentCameraMode = cameraMode;

	if (m_currentCameraMode == CameraMode::FREEFLY_CAMERA)
	{
		cameraMode = CameraMode::ACTOR_CAMERA;
	}
	else
	{
		cameraMode = CameraMode::FREEFLY_CAMERA;
	}
}

void Player::CameraKeyPresses(float deltaSeconds)
{
	if (g_theGame->m_hasTwoPlayers)
	{
		return;
	}

	// Yaw and Pitch with mouse
	m_orientation.m_yawDegrees += 0.08f * g_theInput->GetCursorClientDelta().x;
	m_orientation.m_pitchDegrees -= 0.08f * g_theInput->GetCursorClientDelta().y;

	float movementSpeed = 1.f;

	// Toggle first person actor camera
	if (g_theInput->WasKeyJustPressed('F'))
	{
		ToggleCameraMode(CameraMode::ACTOR_CAMERA);
	}

	// Increase speed by a factor of 15
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		movementSpeed *= 15.f;
	}

	// Move left or right
	if (g_theInput->IsKeyDown('A'))
	{
		m_position += movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('D'))
	{
		m_position += -movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds;
	}

	// Move Forward and Backward
	if (g_theInput->IsKeyDown('W'))
	{
		m_position += movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('S'))
	{
		m_position += -movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds;
	}

	// Move Up and Down
	if (g_theInput->IsKeyDown('Z'))
	{
		m_position += -movementSpeed * Vec3::ZAXE * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('C'))
	{
		m_position += movementSpeed * Vec3::ZAXE * deltaSeconds;
	}

}

void Player::HandleControllerPlayerMovement(float deltaSeconds)
{
	if (g_theGame->m_currentState != GameState::PLAYING)
	{
		return;
	}

	Actor* possessedActor = GetActor();
	if (possessedActor == nullptr)
	{
		return;
	}

	XboxController controller = g_theInput->GetController(0);
	Vec3 forward = possessedActor->GetModelToWorldTransform().GetIBasis3D();
	Vec3 left = possessedActor->GetModelToWorldTransform().GetJBasis3D();

	float movementSpeed = possessedActor->m_actorDef->m_walkSpeed;
	if (controller.IsButtonDown(XBOX_BUTTON_A))
	{
		movementSpeed = possessedActor->m_actorDef->m_runSpeed;
	}

	// Movement
	float orientation = controller.GetLeftStick().GetOrientationDegrees();
	float magnitude = controller.GetLeftStick().GetMagnitude();
	Vec2 movement = Vec2::MakeFromPolarDegrees(orientation, magnitude);
	movement.RotateMinus90Degrees();

	possessedActor->m_position += forward * movement.x * movementSpeed * deltaSeconds;
	possessedActor->m_position += left * movement.y * movementSpeed * deltaSeconds;

	// Look/aim
	Vec2 rightStick = controller.GetRightStick().GetPosition();
	float rightMagnitude = controller.GetRightStick().GetMagnitude();

	if (rightMagnitude > 0.f)
	{
		float turnRate = possessedActor->m_actorDef->m_turnSpeed;
		Vec2 deltaAim = rightStick * movementSpeed * rightMagnitude * turnRate * deltaSeconds;
		possessedActor->m_orientation.m_yawDegrees += -deltaAim.x;
		possessedActor->m_orientation.m_pitchDegrees += -deltaAim.y;
	}

	// Attack
	if (controller.GetRightTrigger() > 0.0f)
	{
		possessedActor->Attack();
	}

	// Weapons
	if (controller.WasButtonJustPressed(XBOX_BUTTON_X))
	{
		possessedActor->EquipWeapon(0);
	}
	if (controller.WasButtonJustPressed(XBOX_BUTTON_Y))
	{
		possessedActor->EquipWeapon(1);
	}
	if (controller.WasButtonJustPressed(XBOX_BUTTON_DPAD_DOWN))
	{
		CycleWeapon(possessedActor, -1);
	}
	if (controller.WasButtonJustPressed(XBOX_BUTTON_DPAD_UP))
	{
		CycleWeapon(possessedActor, 1);
	}
}

void Player::HandleKeyboardPlayerMovement(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_theGame->m_currentState != GameState::PLAYING)
	{
		return;
	}

	if (g_theInput->WasKeyJustPressed('F') && !g_theGame->m_hasTwoPlayers)
	{
		ToggleCameraMode(CameraMode::FREEFLY_CAMERA);
	}

	//if (g_theInput->WasKeyJustPressed('N') && !g_theGame->m_hasTwoPlayers)
	//{
	//	m_theMap->DebugPossessNext();
	//}

	Actor* possessedActor = GetActor();
	if (possessedActor == nullptr)
	{
		return;
	}

	Vec3 forward = possessedActor->GetModelToWorldTransform().GetIBasis3D();
	Vec3 left = possessedActor->GetModelToWorldTransform().GetJBasis3D();

	float movementSpeed = possessedActor->m_actorDef->m_walkSpeed;
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		movementSpeed = possessedActor->m_actorDef->m_runSpeed;
	}

	// Movement
	if (g_theInput->IsKeyDown('W'))
	{
		possessedActor->MoveInDirection(forward, movementSpeed);
		possessedActor->PlayAnimation("Walk");
	}
	if (g_theInput->IsKeyDown('A'))
	{
		possessedActor->MoveInDirection(left, movementSpeed);
		possessedActor->PlayAnimation("Walk");
	}
	if (g_theInput->IsKeyDown('S'))
	{
		possessedActor->MoveInDirection(-forward, movementSpeed);
		possessedActor->PlayAnimation("Walk");
	}
	if (g_theInput->IsKeyDown('D'))
	{
		possessedActor->MoveInDirection(-left, movementSpeed);
		possessedActor->PlayAnimation("Walk");
	}

	// Attack
	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		possessedActor->Attack();
	}

	// Weapons
	if (g_theInput->WasKeyJustPressed('1'))
	{
		possessedActor->EquipWeapon(0);
	}
	if (g_theInput->WasKeyJustPressed('2'))
	{
		possessedActor->EquipWeapon(1);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTARROW))
	{
		CycleWeapon(possessedActor, -1);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHTARROW))
	{
		CycleWeapon(possessedActor, 1);
	}
}

void Player::CycleWeapon(Actor* actor, int direction)
{
	if (actor == nullptr || actor->m_weapons.empty()) return;

	int index = -1;
	int weaponCount = static_cast<int>(actor->m_weapons.size());

	for (int i = 0; i < weaponCount; ++i)
	{
		if (actor->m_weapons[i] == actor->m_equippedWeapon)
		{
			index = i;
			break;
		}
	}

	if (index != -1)
	{
		index += direction;
		if (index < 0)
		{
			index = weaponCount - 1;
		}
		else if (index >= weaponCount)
		{
			index = 0;
		}
		actor->EquipWeapon(index);
	}
}

void Player::CameraControllerPresses(float deltaSeconds)
{
	XboxController const& controller = g_theInput->GetController(0);
	float movementSpeed = 1.f;

	// Increase speed by a factor of 10
	if (controller.IsButtonDown(XBOX_BUTTON_A))
	{
		movementSpeed *= 15.f;
	}

	// Rolling
	if (controller.GetLeftTrigger())
	{
		m_orientation.m_rollDegrees = -90.f * deltaSeconds;
	}
	if (controller.GetRightTrigger())
	{
		m_orientation.m_rollDegrees = 90.f * deltaSeconds;
	}

	Vec2  leftStickPosition = controller.GetLeftStick().GetPosition();
	Vec2  rightStickPosition = controller.GetRightStick().GetPosition();
	float leftStickMagnitude = controller.GetLeftStick().GetMagnitude();
	float rightStickMagnitude = controller.GetRightStick().GetMagnitude();

	// Move left, right, forward, and backward
	if (leftStickMagnitude > 0.f)
	{
		m_position += (-movementSpeed * leftStickPosition.x * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds);
		m_position += (movementSpeed * leftStickPosition.y * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds);
	}

	if (rightStickMagnitude > 0.f)
	{
		float turnRate = 80.f;
		m_orientation.m_yawDegrees += -(rightStickPosition * movementSpeed * rightStickMagnitude * turnRate * deltaSeconds).x;
		m_orientation.m_pitchDegrees += -(rightStickPosition * movementSpeed * rightStickMagnitude * turnRate * deltaSeconds).y;
	}

	//// Move Up and Down
	if (controller.IsButtonDown(XBOX_BUTTON_LSHOULDER))
	{
		m_position += -movementSpeed * Vec3::ZAXE * deltaSeconds;
	}
	if (controller.IsButtonDown(XBOX_BUTTON_RSHOULDER))
	{
		m_position += movementSpeed * Vec3::ZAXE * deltaSeconds;
	}

	//// Reset position and orientation to zero
	if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
	{
		m_position = Vec3::ZERO;
		m_orientation = EulerAngles(0.f, 0.f, 0.f);
	}
}
