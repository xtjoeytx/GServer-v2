#include <memory>

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

EventHandleBase::EventHandleBase(EventDispatcherBase* dispatcher, size_t id)
	: m_dispatcher(dispatcher), m_eventId(id)
{
};

EventDispatcherBase::EventDispatcherBase()
{
}

EventDispatcherBase::~EventDispatcherBase()
{
	unsubscribeAll();
}

bool EventDispatcherBase::unsubscribe(EventHandleBase* handle)
{
	if (!handle)
		return false;

	// Check if the event handle event exists
	auto itr = m_eventHandlers.find(handle->m_eventId);
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

bool EventDispatcherBase::unsubscribe(std::shared_ptr<EventHandleBase> handle)
{
	if (handle)
		return unsubscribe(handle.get());
	else
		return false;
};

void EventDispatcherBase::unsubscribeAll()
{
	for (auto& handler : m_eventHandlers)
	{
		auto ptr = handler.second.lock();
		if (ptr)
			ptr->m_dispatcher = nullptr;
	}

	m_eventHandlers.clear();
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
