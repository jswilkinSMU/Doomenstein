#pragma once
#include "Game/GameCommon.h"
#include "Game/Map.hpp"
#include "Engine/Renderer/Camera.h"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Vertex_PCU.h"
// -----------------------------------------------------------------------------
class Player;
class BitmapFont;
// -----------------------------------------------------------------------------
enum class GameState
{
	NONE,
	ATTRACT,
	LOBBY,
	PLAYING,
	COUNT
};
// -----------------------------------------------------------------------------
class Game
{
public:
	App* m_app;
	Game(App* owner);
	~Game();
	void StartUp();

	void Update();


	void UpdateCameras();

	void Render() const;

	void RenderGameOverScreen() const;
	void RenderVictoryScreen() const;

	void RenderAttractMode() const;
	void RenderLobbyMode() const;

	GameState GetCurrentGameState() const;
	void EnterState(GameState state);
	void ExitState(GameState state);

	void Shutdown();
	void DestroyPlayer();
	void DestroyMap();

	void InitializePlayer();
	void InitializeMap();

	void KeyInputPresses();
	void HandleAttractInput();
	void HandleLobbyInput();
	void HandleLobbySinglePlayerInput();
	void HandleLobbyMultiplayerInput();
	void HandlePlayingInput();

	void AdjustForPauseAndTimeDistortion(float deltaSeconds);
	void GameOver(float deltaseconds);
	void VictoryCondition(float deltaSeconds);

	Map* GetMap() const;
	Map*		m_defaultMap = nullptr;

	GameState	m_currentState = GameState::ATTRACT;
	Clock*      m_gameClock = nullptr;

	bool m_hasTwoPlayers = false;
	bool m_playerUsingController = false;
	Camera		m_screenCamera;
	Camera      m_gameWorldCamera;
	std::vector<Player*> m_players;
	BitmapFont* m_font = nullptr;
// -----------------------------------------------------------------------------
private:

	std::vector<Vertex_PCU> m_fontVerts;
	Texture* m_gameOverTexture = nullptr;
	Texture* m_victoryTexture = nullptr;
	GameState	m_nextState = GameState::ATTRACT;
	float m_gameOverSeconds = 6.f;
	float m_victorySeconds = 3.f;
	bool m_hasPlayerWon = false;
	bool m_hasGameOverPlayed = false;

	// Music
	std::string m_mainMenuMusicPath;
	std::string m_gameMusicPath;
	std::string m_clickSoundPath;
	SoundID     m_mainMenuMusic;
	SoundID     m_gameMusic;
	SoundID     m_clickSound;
	SoundID     m_gameOverSound;
	SoundID     m_victorySound;
	SoundPlaybackID m_mainMenuPlayback;
	SoundPlaybackID m_gameMusicPlayback;
	float m_musicVolume = 0.0f;
};