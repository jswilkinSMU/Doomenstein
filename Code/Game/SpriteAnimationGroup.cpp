#include "Game/SpriteAnimationGroup.hpp"
#include "Engine/Math/MathUtils.h"
#include "Engine/Core/ErrorWarningAssert.hpp"

SpriteAnimationGroup::SpriteAnimationGroup(XmlElement const& element, SpriteSheet* spritesheet)
	:m_spriteSheet(spritesheet)
{
	m_animationGroupName = ParseXmlAttribute(element, "name", m_animationGroupName);
 	float secondsPerFrame = ParseXmlAttribute(element, "secondsPerFrame", 0.f);

	std::string playbackMode = ParseXmlAttribute(element, "playbackMode", playbackMode);
	if (playbackMode == "Loop")
	{
		m_playbackMode = SpriteAnimPlaybackType::LOOP;
	}
	else if (playbackMode == "Once")
	{
		m_playbackMode = SpriteAnimPlaybackType::ONCE;
	}

	m_scaleBySpeed = ParseXmlAttribute(element, "scaleBySpeed", m_scaleBySpeed);

	XmlElement const* directionElement = element.FirstChildElement("Direction");
	while (directionElement != nullptr)
	{
		Vec3 direction = ParseXmlAttribute(*directionElement, "vector", direction);
		direction.Normalize();

		XmlElement const* animElement = directionElement->FirstChildElement("Animation");
		if (animElement == nullptr)
		{
			ERROR_AND_DIE(Stringf("Could not find animation element for \"%s\"", directionElement->Name()));
		}

		m_directions.push_back(direction);
		SpriteAnimDefinition animDef(*spritesheet, -1, -1, secondsPerFrame, m_playbackMode);
		animDef.LoadFromXmlElement(animElement);
		m_anims.push_back(animDef);
		directionElement = directionElement->NextSiblingElement("Direction");
	}
}

SpriteAnimDefinition SpriteAnimationGroup::GetAnimDirection(Vec3 const& direction)
{
	int animResultIndex = 0;
	float maxDot = -1000.f;

	for (int animIndex = 0; animIndex < static_cast<int>(m_anims.size()); ++animIndex)
	{
		float dot = DotProduct3D(direction, m_directions[animIndex]);
		if (dot > maxDot)
		{
			maxDot = dot;
			animResultIndex = animIndex;
		}
	}
	return m_anims[animResultIndex];
}

float SpriteAnimationGroup::GetAnimationDuration()
{
	if (m_anims.size() != m_directions.size())
	{
		ERROR_AND_DIE("Animation and direction vectors are incorrectly sized.");
	}

	for (int animIndex = 0; animIndex < static_cast<int>(m_anims.size()); ++animIndex)
	{
		float totalLength = m_anims[animIndex].GetDuration();
		if (totalLength > 0.f)
		{
			return totalLength;
		}
	}
	return -1.f;
}
