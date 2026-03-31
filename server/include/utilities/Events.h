#ifndef EVENTS_H
#define EVENTS_H

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

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

	/// @brief Unsubscribes a handler from the event dispatcher.
	/// @param handle The handle to the event handler to unsubscribe. The handle should have been returned by a previous call to subscribe().
	/// @return True if the handler was successfully unsubscribed, false if the handler was not found or if the handle was invalid.
	bool unsubscribe(EventHandleBase* handle);

	/// @brief Unsubscribes a handler from the event dispatcher.
	/// @param handle The handle to the event handler to unsubscribe. The handle should have been returned by a previous call to subscribe().
	/// @return True if the handler was successfully unsubscribed, false if the handler was not found or if the handle was invalid.
	bool unsubscribe(std::shared_ptr<EventHandleBase> handle);

	/// @brief Unsubscribes all handlers from the event dispatcher.
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

template<typename... A>
class EventHandleImpl : public EventHandleBase
{
public:
	EventHandleImpl(EventDispatcherBase* dispatcher, size_t id, std::function<void(A...)> callback)
		: EventHandleBase(dispatcher, id), m_callback(callback) {};

	~EventHandleImpl() {};

	/// @brief Dispatches the event to the subscribed handler.
	/// @param ...args The arguments to pass to the event handler.
	void dispatch(A... args)
	{
		if (m_callback)
			m_callback(args...);
	};

private:
	std::function<void(A...)> m_callback;
};

//----------------------------

/// @brief An event handle that can be used to unsubscribe from an event.
typedef std::shared_ptr<EventHandleBase> EventHandle;

//----------------------------

/// @brief Dispatches events to subscribed handlers.
/// @tparam ...A The types of the arguments that will be passed to the event handlers when an event is posted.
template<typename... A>
class EventDispatcher : public EventDispatcherBase
{
public:
	EventDispatcher() {};
	virtual ~EventDispatcher() {};

	/// @brief Posts an event to all subscribed handlers.
	/// @param ...args The arguments to pass to the event handlers.
	void post(A... args)
	{
		m_isPosting = true;

		for (auto itr = m_eventHandlers.begin(); itr != m_eventHandlers.end();)
		{
			auto current = itr++;
			auto basePtr = current->second.lock();
			if (!basePtr)
			{
				m_eventHandlers.erase(current);
				continue;
			}

			if (auto ptr = std::static_pointer_cast<EventHandleImpl<A...>>(basePtr); ptr)
				ptr->dispatch(args...);
		}

		m_isPosting = false;
		flushPendingSubscriptions();
	};

	/// @brief Subscribes a handler to the event dispatcher.
	/// @param callback The callback function to be called when an event is posted. The callback should take the same arguments as the event dispatcher.
	/// @return A handle to the subscribed event. The handle can be used to unsubscribe from the event.
	EventHandle subscribe(std::function<void(A...)> callback)
	{
		// Static handle counter will give us unique ID's.
		static size_t handleCounter = 0;

		// Get handle ID by incrementing the handle counter.
		auto handleId = handleCounter++;

		// Create an event handle and store it with the handle ID as the key
		std::shared_ptr<EventHandleImpl<A...>> handle = std::make_shared<EventHandleImpl<A...>>(this, handleId, callback);

		// Store a weak pointer to the event handler so we don't increment ref. count
		auto weakHandle = std::weak_ptr<EventHandleBase>(std::static_pointer_cast<EventHandleBase>(handle));
		if (m_isPosting)
			m_pendingEventHandlers.emplace_back(handleId, weakHandle);
		else
			m_eventHandlers[handleId] = weakHandle;

		return handle;
	};

private:
	/// @brief Flushes pending subscriptions that were added while an event was being posted.
	void flushPendingSubscriptions()
	{
		for (auto& pendingHandler : m_pendingEventHandlers)
		{
			if (auto ptr = pendingHandler.second.lock(); ptr)
				m_eventHandlers[pendingHandler.first] = pendingHandler.second;
		}

		m_pendingEventHandlers.clear();
	}

	bool m_isPosting = false;
	std::vector<std::pair<size_t, std::weak_ptr<EventHandleBase>>> m_pendingEventHandlers;
};

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // EVENTS_H
