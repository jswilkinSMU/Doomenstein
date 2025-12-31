#pragma once
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
// -----------------------------------------------------------------------------
class SpriteSheet;
// -----------------------------------------------------------------------------
class SpriteAnimationGroup
{
public:
	SpriteAnimationGroup(XmlElement const& element, SpriteSheet* spritesheet);
	SpriteAnimDefinition GetAnimDirection(Vec3 const& direction);
	float                GetAnimationDuration();
// -----------------------------------------------------------------------------
	SpriteSheet* m_spriteSheet = nullptr;
	std::string m_animationGroupName;
	float m_secondsPerFrame = 0.f;
	SpriteAnimPlaybackType m_playbackMode = SpriteAnimPlaybackType::ONCE;
	bool m_scaleBySpeed = false;

	std::vector<SpriteAnimDefinition> m_anims;
	std::vector<Vec3> m_directions;
};