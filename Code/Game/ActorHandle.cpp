#include "Game/ActorHandle.hpp"
#include "Engine/Core/EngineCommon.h"
// -----------------------------------------------------------------------------
const ActorHandle ActorHandle::INVALID = ActorHandle(0x0000ffffu, 0x0000ffffu);
// -----------------------------------------------------------------------------
ActorHandle::ActorHandle()
{
	m_data = 0xffffffffu;
}

ActorHandle::ActorHandle(unsigned int uid, unsigned int index)
{
	m_data = (uid << 16) | index & 0xFFFF;
}

bool ActorHandle::IsValid() const
{
	if (m_data != INVALID.m_data)
	{
		return true;
	}
	return false;
}

unsigned int ActorHandle::GetIndex() const
{
	return m_data & 0xFFFF;
}

bool ActorHandle::operator==(ActorHandle const& other) const
{
	return m_data == other.m_data;
}

bool ActorHandle::operator!=(ActorHandle const& other) const
{
	return m_data != other.m_data;
}
// -----------------------------------------------------------------------------
