#include <memory>
#include <ranges>

#include <utilities/Events.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

EventHandleBase::~EventHandleBase()
{
	if (m_dispatcher)
		m_dispatcher->unsubscribe(this);
}

EventHandleBase::EventHandleBase(EventDispatcherBase* dispatcher, const size_t id)
	: m_dispatcher(dispatcher), m_eventId(id)
{
};

EventDispatcherBase::~EventDispatcherBase()
{
	unsubscribeAll();
}

bool EventDispatcherBase::unsubscribe(EventHandleBase* handle)
{
	if (!handle)
		return false;

	// Check if the event handle event exists
	const auto itr = m_eventHandlers.find(handle->m_eventId);
	if (itr == m_eventHandlers.end())
		return false;

	// Unregister the event handler.
	// Remove the parent dispatcher just in case
	// it was manually unsubscribed instead of deleted.
	// Otherwise you might get a double delete.
	handle->m_dispatcher = nullptr;
	m_eventHandlers.erase(itr);

	return true;
};

bool EventDispatcherBase::unsubscribe(const std::shared_ptr<EventHandleBase>& handle)
{
	if (handle)
		return unsubscribe(handle.get());
	return false;
};

void EventDispatcherBase::unsubscribeAll()
{
	for (auto& val : m_eventHandlers | std::views::values)
	{
		auto ptr = val.lock();
		if (ptr)
			ptr->m_dispatcher = nullptr;
	}

	m_eventHandlers.clear();
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
