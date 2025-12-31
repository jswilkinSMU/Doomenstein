#include "Game/Game.h"
#include "Game/GameCommon.h"
#include "Game/App.h"
#include "Game/Player.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/WeaponDefinition.hpp"
#include "Game/ActorDefinition.hpp"

#include "Engine/Input/InputSystem.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/Rgba8.h"
#include "Engine/Core/Vertex_PCU.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/VertexUtils.h"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/AABB3.hpp"

Game::Game(App* owner)
	: m_app(owner)
{
	// Get sound from game config
	m_mainMenuMusicPath = g_gameConfigBlackboard.GetValue("mainMenuMusic", "default");
	m_gameMusicPath = g_gameConfigBlackboard.GetValue("gameMusic", "default");
	m_clickSoundPath = g_gameConfigBlackboard.GetValue("buttonClickSound", "default");
	m_musicVolume = g_gameConfigBlackboard.GetValue("musicVolume", 0.f);

	m_mainMenuMusic = g_theAudio->CreateOrGetSound(m_mainMenuMusicPath);
	m_gameMusic = g_theAudio->CreateOrGetSound(m_gameMusicPath);
	m_clickSound = g_theAudio->CreateOrGetSound(m_clickSoundPath);
	m_gameOverSound = g_theAudio->CreateOrGetSound("Data/Audio/GameOver.mp3");
	m_victorySound = g_theAudio->CreateOrGetSound("Data/Audio/StarshipVictory.mp3");
	m_mainMenuPlayback = g_theAudio->StartSound(m_mainMenuMusic, true, m_musicVolume);
	m_gameMusicPlayback = m_gameMusic;
}

Game::~Game()
{
}

void Game::StartUp()
{
	// Write control interface into devconsole
	g_theDevConsole->AddLine(Rgba8::CYAN, "Welcome to Doomenstein!");
	g_theDevConsole->AddLine(Rgba8::SEAWEED, "----------------------------------------------------------------------");
	g_theDevConsole->AddLine(Rgba8::CYAN, "CONTROLS:");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "ESC   - Quits the game or exits Playing state");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "SPACE - Start game");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "SHIFT - Increase speed by factor of 15.");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "A/D   - Move left/right");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "W/S   - Move forward/backward");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "Z/C   - Move down/up");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "LMB   - Fire equipped weapon");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "1     - Equip pistol");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "2     - Equip plasma rifle");
	g_theDevConsole->AddLine(Rgba8::SEAWEED, "----------------------------------------------------------------------");
	g_theDevConsole->AddLine(Rgba8::CYAN, "DEBUG CONTROLS:");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "F     - Switch between actor and camera mode.");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "F2/F3 - Moves sun x direction -/+.");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "F4/F5 - Moves sun y direction -/+.");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "F6/F7 - Decrease/Increase sun intensity.");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "F8/F9 - Decrease/Increase ambient intensity.");
	g_theDevConsole->AddLine(Rgba8::SEAWEED, "----------------------------------------------------------------------");

	// Loading XML elements
	TileDefinition::InitializeTileDefs();
	MapDefinition::InitializeMapDefs();
	ActorDefinition::InitializeProjectileActorDefs();
	WeaponDefinition::InitializeWeaponsDefs();
	ActorDefinition::InitializeActorDefs();

	m_gameClock = new Clock(Clock::GetSystemClock());
	m_gameOverTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/gameover.png");
	m_victoryTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/VictoryScreen.png");

	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	m_font->AddVertsForTextInBox2D(m_fontVerts, "Press SPACE to join with mouse and keyboard", AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), 20.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.2f));
	m_font->AddVertsForTextInBox2D(m_fontVerts, "Press START to join with controller", AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), 20.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.15f));
	m_font->AddVertsForTextInBox2D(m_fontVerts, "Press ESC or BACK to exit", AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), 20.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.1f));

	InitializePlayer();
}

void Game::Update()
{
	// Setting clock time variables
	double deltaSeconds = m_gameClock->GetDeltaSeconds();
	AdjustForPauseAndTimeDistortion(static_cast<float>(deltaSeconds));
	KeyInputPresses();

	if (m_currentState == GameState::PLAYING)
	{
		std::string timeText = Stringf("[Game Clock] Time: %0.2f, FPS: %0.2f, TimeScale: %0.2f",
			m_gameClock->GetTotalSeconds(), m_gameClock->GetFrameRate(), m_gameClock->GetTimeScale());
		DebugAddScreenText(timeText, AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), 12.f, Vec2(0.97f, 0.97f), 0.f, Rgba8::WHITE, Rgba8::WHITE);

		for (Player* player : m_players)
		{
			player->Update(static_cast<float>(deltaSeconds));
		}
		m_defaultMap->Update(static_cast<float>(deltaSeconds));
		VictoryCondition(static_cast<float>(deltaSeconds));
		GameOver(static_cast<float>(deltaSeconds));
	}

	UpdateCameras();
}

void Game::VictoryCondition(float deltaSeconds)
{
	if (!m_hasPlayerWon && m_players[0]->m_numPlayerLives > 0 && m_defaultMap->AreAllEnemiesDead())
	{
		g_theAudio->StopSound(m_gameMusicPlayback);
		g_theAudio->StartSound(m_victorySound);
		m_hasPlayerWon = true;
		m_victorySeconds = 3.f;
	}

	if (m_hasPlayerWon && m_victorySeconds > 0.0f)
	{
		m_victorySeconds -= deltaSeconds;
		if (m_victorySeconds <= 0.0f)
		{
			EnterState(GameState::ATTRACT);
		}
	}
}

void Game::Render() const
{
	if (m_currentState == GameState::ATTRACT)
	{
		g_theRenderer->BeginCamera(m_screenCamera);
		RenderAttractMode();
		g_theRenderer->EndCamera(m_screenCamera);
	}
	if (m_currentState == GameState::LOBBY)
	{
		g_theRenderer->BeginCamera(m_screenCamera);
		RenderLobbyMode();
		g_theRenderer->EndCamera(m_screenCamera);
	}
	if (m_currentState == GameState::PLAYING)
	{
		for (int playerIndex = 0; playerIndex < static_cast<int>(m_players.size()); ++playerIndex)
		{
			Player* player = m_players[playerIndex];
			Camera cam = player->m_playerCamera;
			Camera viewCamera = player->m_playerViewCamera;

			if (m_players.size() == 2)
			{
				if (playerIndex == 0)
				{
					cam.SetNormalizedViewport(Vec2(0.f, 0.5f), Vec2(1.f, 1.f));
					viewCamera.SetNormalizedViewport(Vec2(0.f, 0.5f), Vec2(1.f, 1.f));
				}
				else
				{
					cam.SetNormalizedViewport(Vec2::ZERO, Vec2(1.f, 0.5f));
					viewCamera.SetNormalizedViewport(Vec2::ZERO, Vec2(1.f, 0.5f));
				}
			}
			else
			{
				cam.SetNormalizedViewport(Vec2::ZERO, Vec2(1.f, 1.f));
				viewCamera.SetNormalizedViewport(Vec2::ZERO, Vec2(1.f, 1.f));
			}

			g_theRenderer->BeginCamera(cam);
			m_defaultMap->Render(player);
			g_theRenderer->EndCamera(cam);
			player->Render();
			g_theRenderer->BeginCamera(m_screenCamera);
			RenderGameOverScreen();
			RenderVictoryScreen();
			g_theRenderer->EndCamera(m_screenCamera);
		}

		DebugRenderScreen(m_screenCamera);
		if (!m_players.empty())
		{
			DebugRenderWorld(m_players[0]->GetPlayerCamera());
		}
	}
}

void Game::RenderGameOverScreen() const
{
	std::vector<Vertex_PCU> m_endScreenVerts;
	if (m_gameOverSeconds > 0 && m_gameOverSeconds < 6)
	{
		AddVertsForAABB2D(m_endScreenVerts, AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), Rgba8::WHITE);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(m_gameOverTexture);
		g_theRenderer->DrawVertexArray(m_endScreenVerts);
	}
}

void Game::RenderVictoryScreen() const
{
	if (m_victorySeconds > 0.0f && m_victorySeconds < 3.0f)
	{
		std::vector<Vertex_PCU> m_victoryScreenVerts;
		AddVertsForAABB2D(m_victoryScreenVerts, AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), Rgba8::WHITE);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(m_victoryTexture);
		g_theRenderer->DrawVertexArray(m_victoryScreenVerts);
	}
}

void Game::Shutdown()
{
	delete m_gameClock;
	m_gameClock = nullptr;

	DestroyMap();
	DestroyPlayer();
}

void Game::DestroyPlayer()
{
	for (Player* player : m_players)
	{
		delete player;
	}
	m_players.clear();
}

void Game::DestroyMap()
{
	delete m_defaultMap;
	m_defaultMap = nullptr;
}

void Game::InitializePlayer()
{
	//m_player = new Player(m_defaultMap);
	DestroyPlayer();

	// Multiplayer setup
	if (m_hasTwoPlayers)
	{
		m_players.push_back(new Player(m_defaultMap, 0));
		m_players.push_back(new Player(m_defaultMap, 1));
	}
	else
	{
		if (m_playerUsingController)
		{
			m_players.push_back(new Player(m_defaultMap, 0));
		}
		else
		{
			m_players.push_back(new Player(m_defaultMap, 1));
		}
	}
}

void Game::InitializeMap()
{
	std::string mapName = g_gameConfigBlackboard.GetValue("defaultMap", "");
	MapDefinition* currentMap = MapDefinition::GetByName(mapName);
	m_defaultMap = new Map(this, currentMap);
}

void Game::KeyInputPresses()
{
	switch (m_currentState)
	{
		case GameState::ATTRACT:
		{
			HandleAttractInput();
			break;
		}
		case GameState::LOBBY:
		{
			HandleLobbyInput();
			break;
		}
		case GameState::PLAYING:
		{
			HandlePlayingInput();
			break;
		}
	}
}

void Game::HandleAttractInput()
{
	XboxController const& controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(' '))
	{
		m_playerUsingController = false;
		g_theAudio->StartSound(m_clickSound);
		EnterState(GameState::LOBBY);
	}
	else if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
	{
		m_playerUsingController = true;
		g_theAudio->StartSound(m_clickSound);
		EnterState(GameState::LOBBY);
	}
	else if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_theEventSystem->FireEvent("Quit");
	}
}

void Game::HandleLobbyInput()
{
	if (m_hasTwoPlayers)
	{
		HandleLobbyMultiplayerInput();
	}
	else
	{
		HandleLobbySinglePlayerInput();
	}
}

void Game::HandleLobbySinglePlayerInput()
{
	XboxController const& controller = g_theInput->GetController(0);

	if (!m_playerUsingController)
	{
		if (g_theInput->WasKeyJustPressed(' '))
		{
			g_theAudio->StartSound(m_clickSound);
			EnterState(GameState::PLAYING);
		}
		else if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
		{
			g_theAudio->StartSound(m_clickSound);
			EnterState(GameState::ATTRACT);
		}
		else if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
		{
			g_theAudio->StartSound(m_clickSound);
			m_hasTwoPlayers = true;
		}
	}
	else
	{
		if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
		{
			g_theAudio->StartSound(m_clickSound);
			EnterState(GameState::PLAYING);
		}
		else if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
		{
			g_theAudio->StartSound(m_clickSound);
			EnterState(GameState::ATTRACT);
		}
		else if (g_theInput->WasKeyJustPressed(' '))
		{
			g_theAudio->StartSound(m_clickSound);
			m_hasTwoPlayers = true;
		}
	}
}

void Game::HandleLobbyMultiplayerInput()
{
	XboxController const& controller = g_theInput->GetController(0);

	if (!m_playerUsingController)
	{
		if (g_theInput->WasKeyJustPressed(' ') || controller.WasButtonJustPressed(XBOX_BUTTON_START))
		{
			g_theAudio->StartSound(m_clickSound);
			EnterState(GameState::PLAYING);
		}
		else if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
		{
			g_theAudio->StartSound(m_clickSound);
			m_hasTwoPlayers = false;
			m_playerUsingController = true;
		}
		else if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
		{
			g_theAudio->StartSound(m_clickSound);
			m_hasTwoPlayers = false;
		}
	}
	else
	{
		if (controller.WasButtonJustPressed(XBOX_BUTTON_START) || g_theInput->WasKeyJustPressed(' '))
		{
			g_theAudio->StartSound(m_clickSound);
			EnterState(GameState::PLAYING);
		}
		else if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
		{
			g_theAudio->StartSound(m_clickSound);
			m_hasTwoPlayers = false;
			m_playerUsingController = false;
		}
		else if (g_theInput->WasKeyJustPressed(' '))
		{
			g_theAudio->StartSound(m_clickSound);
			m_hasTwoPlayers = true;
		}
		else if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
		{
			g_theAudio->StartSound(m_clickSound);
			m_hasTwoPlayers = false;
		}
	}
}

void Game::HandlePlayingInput()
{
	XboxController const& controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) ||
		controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		EnterState(GameState::ATTRACT);
	}
}

void Game::AdjustForPauseAndTimeDistortion(float deltaSeconds) {

	UNUSED(deltaSeconds);

	if (g_theInput->IsKeyDown('T'))
	{
		m_gameClock->SetTimeScale(0.1);
	}
	else
	{
		m_gameClock->SetTimeScale(1.0);
	}

	if (g_theInput->WasKeyJustPressed('P'))
	{
		m_gameClock->TogglePause();
	}

	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_gameClock->StepSingleFrame();
	}
}

void Game::GameOver(float deltaseconds)
{
	if (m_hasPlayerWon)
	{
		return;
	}

	if (m_players[0]->m_numPlayerLives <= 0)
	{
		if (!m_hasGameOverPlayed)
		{
			g_theAudio->StopSound(m_gameMusicPlayback);
			g_theAudio->StartSound(m_gameOverSound);
			m_hasGameOverPlayed = true;
			m_gameOverSeconds = 6.f;
		}

		if (m_gameOverSeconds > 0.0f)
		{
			m_gameOverSeconds -= deltaseconds;
			if (m_gameOverSeconds <= 0.0f)
			{
				EnterState(GameState::ATTRACT);
				return;
			}
		}
	}
}

Map* Game::GetMap() const
{
	return m_defaultMap;
}

void Game::UpdateCameras()
{
	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	//m_screenCamera.SetNormalizedViewport(Vec2::ZERO, Vec2(1.f, 0.5f));
}

void Game::RenderAttractMode() const
{
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(m_fontVerts);
}

void Game::RenderLobbyMode() const
{
	g_theRenderer->ClearScreen(Rgba8(70, 70, 70, 255));
	std::vector<Vertex_PCU> textVerts;
	AABB2 upperScreenBox = AABB2(Vec2(400.f, 400.f), Vec2(1200.f, 800.f));
	AABB2 screenCenterBox = AABB2(Vec2(400.f, 200.f), Vec2(1200.f, 600.f));
	AABB2 lowerScreenBox = AABB2(Vec2(400.f, 0.f), Vec2(1200.f, 400.f));
	std::string inputText;
	std::string controlsText;
	std::string inputTwoText;
	std::string controlsTwoText;

	std::string playerText = "Player 1\n";
	std::string playerTwoText = "Player 2\n";

	if (!m_hasTwoPlayers)
	{
		m_font->AddVertsForTextInBox2D(textVerts, playerText, screenCenterBox, 50.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
		if (m_playerUsingController)
		{
			inputText = "Controller\n";
			controlsText = "Press START to start game\nPress BACK to leave game\nPress SPACE to join player\n";
		}
		else
		{
			inputText = "Mouse and Keyboard\n";
			controlsText = "Press SPACE to start game\nPress ESCAPE to leave game\nPress START to join player\n";
		}
		m_font->AddVertsForTextInBox2D(textVerts, inputText, screenCenterBox, 30.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.375f));
		m_font->AddVertsForTextInBox2D(textVerts, controlsText, screenCenterBox, 17.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.10f));
	}
	else if (m_hasTwoPlayers)
	{
		m_font->AddVertsForTextInBox2D(textVerts, playerText, upperScreenBox, 50.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
		if (m_playerUsingController)
		{
			inputText = "Controller\n";
			controlsText = "Press START to start game\nPress BACK to leave game\n";
		}
		else
		{
			inputText = "Keyboard & Mouse\n";
			controlsText = "Press SPACE to start game\nPress ESCAPE to leave game\n";
		}
		m_font->AddVertsForTextInBox2D(textVerts, inputText, upperScreenBox, 30.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.375f));
		m_font->AddVertsForTextInBox2D(textVerts, controlsText, upperScreenBox, 17.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.10f));
		m_font->AddVertsForTextInBox2D(textVerts, playerTwoText, lowerScreenBox, 50.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
		if (m_playerUsingController)
		{
			inputTwoText = "Keyboard & Mouse\n";
			controlsTwoText = "Press SPACE to start game\nPress ESCAPE to leave game\n";
		}
		else
		{
			inputTwoText = "Controller\n";
			controlsTwoText = "Press START to start game\nPress BACK to leave game\n";
		}
		m_font->AddVertsForTextInBox2D(textVerts, inputTwoText, lowerScreenBox, 30.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.375f));
		m_font->AddVertsForTextInBox2D(textVerts, controlsTwoText, lowerScreenBox, 17.f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.10f));
	}

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}

GameState Game::GetCurrentGameState() const
{
	return m_currentState;
}

void Game::EnterState(GameState state)
{
	ExitState(m_currentState);
	m_currentState = state;

	switch (state)
	{
	case GameState::NONE:
	{
		break;
	}
		case GameState::ATTRACT:
		{
			if (!g_theAudio->IsPlaying(m_mainMenuPlayback))
			{
				m_mainMenuPlayback = g_theAudio->StartSound(m_mainMenuMusic, true, m_musicVolume);
			}
			break;
		}
		case GameState::LOBBY:
		{
			break;
		}
		case GameState::PLAYING:
		{
			m_hasPlayerWon = false;
			m_hasGameOverPlayed = false;

			InitializePlayer();
			g_theAudio->StopSound(m_mainMenuPlayback);
			m_gameMusicPlayback = g_theAudio->StartSound(m_gameMusic, true, m_musicVolume);

			if (m_hasTwoPlayers)
			{
				g_theAudio->SetNumListeners(2);
			}
			else
			{
				g_theAudio->SetNumListeners(1);
			}

			InitializeMap();

			std::string objText = "Objective: Kill all the enemies and destroy their spawners!";
			DebugAddScreenText(objText, AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), 20.f, Vec2(0.5f, 0.5f), 4.f);
			break;
		}
		default:
		{
			break;
		}
	}
}

void Game::ExitState(GameState state)
{
	switch (state)
	{
		case GameState::NONE:
		{
			break;
		}
		case GameState::ATTRACT:
		{
			break;
		}
		case GameState::LOBBY:
		{
			break;
		}
		case GameState::PLAYING:
		{
			g_theAudio->StopSound(m_gameMusicPlayback);
			DestroyPlayer();
			DestroyMap();
			break;
		}
		default:
		{
			break;
		}
	}
}
