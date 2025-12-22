#ifndef EVENTS_H
#define EVENTS_H

#include <functional>
#include <memory>
#include <unordered_map>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

class EventHandleBase;

class EventDispatcherBase
{
public:
	EventDispatcherBase();
	virtual ~EventDispatcherBase();

	bool unsubscribe(EventHandleBase* handle);
	bool unsubscribe(std::shared_ptr<EventHandleBase> handle);
	void unsubscribeAll();

protected:
	std::unordered_map<size_t, std::weak_ptr<EventHandleBase>> m_eventHandlers;
};

//----------------------------

class EventHandleBase
{
	friend class EventDispatcherBase;

public:
	virtual ~EventHandleBase();

protected:
	EventHandleBase(EventDispatcherBase* dispatcher, size_t id);

	EventDispatcherBase* m_dispatcher;
	size_t m_eventId;
};

template <typename ...A>
class EventHandleImpl : public EventHandleBase
{
public:
	EventHandleImpl(EventDispatcherBase* dispatcher, size_t id, std::function<void(A...)> callback)
		: EventHandleBase(dispatcher, id), m_callback(callback)
	{};

	~EventHandleImpl()
	{};

	void dispatch(A ...args)
	{
		if (m_callback)
			m_callback(args...);
	};

private:
	std::function<void(A...)> m_callback;
};

typedef std::shared_ptr<EventHandleBase> EventHandle;

//----------------------------

template <typename ...A>
class EventDispatcher : public EventDispatcherBase
{
public:
	EventDispatcher() {};
	virtual ~EventDispatcher() {};

	void post(A ...args)
	{
		// TODO: Optimize this by not copying the entire map each time (hold a vector of newly added handlers while we are processing the list).
		std::unordered_map<size_t, std::weak_ptr<EventHandleBase>> handlersCopy = m_eventHandlers;
		for (auto& handler : handlersCopy)
		{
			auto ptr = std::static_pointer_cast<EventHandleImpl<A...>>(handler.second.lock());
			if (ptr)
				ptr->dispatch(args...);
		}
	};

	EventHandle subscribe(std::function<void(A...)> callback)
	{
		// Static handle counter will give us unique ID's.
		static size_t handleCounter = 0;

		// Get handle ID by incrementing the handle counter.
		auto handleId = handleCounter++;

		// Create an event handle and store it with the handle ID as the key
		std::shared_ptr<EventHandleImpl<A...>> handle = std::make_shared<EventHandleImpl<A...>>(this, handleId, callback);

		// Store a weak pointer to the event handler so we don't increment ref. count
		m_eventHandlers[handleId] = std::weak_ptr<EventHandleBase>(std::static_pointer_cast<EventHandleBase>(handle));

		return handle;
	};
};

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // EVENTS_H
