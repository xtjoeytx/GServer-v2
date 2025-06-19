#ifndef IDGENERATOR_H
#define IDGENERATOR_H

#include <concepts>
#include <map>
#include <optional>
#include <set>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

template <std::integral T>
class IdGenerator
{
protected:
	struct Segment
	{
		T startId = static_cast<T>(0);
		T nextId = static_cast<T>(0);
		std::set<T> freeIds;
		std::set<T> manuallyUsedIds;
	};

public:
	IdGenerator() { createSegment(T(0)); }
	IdGenerator(T startId) { createSegment(startId); }

	// Create a new segment starting at the specified ID.
	bool createSegment(T startId)
	{
		for (auto& [segmentStartId, segment] : m_segments)
		{
			if (segmentStartId + segment.nextId > startId)
				return false;
		}
		m_segments.emplace(std::make_pair(startId, Segment{ .startId = startId, .nextId = startId }));
		return true;
	}

	// Generate a new ID
	T getAvailableId(std::optional<T> startId = {})
	{
		Segment& segment = m_segments.begin()->second;
		if (startId.has_value())
		{
			if (auto location = getSegmentForId(startId.value()); location != nullptr)
				segment = *location;
		}

		if (!segment.freeIds.empty())
		{
			T id = *segment.freeIds.begin();
			segment.freeIds.erase(segment.freeIds.begin());
			return id;
		}
		return segment.nextId++;
	}

	// Marks the ID as used
	bool markAsUsed(T id)
	{
		if (auto segment = getSegmentForId(id); segment != nullptr)
		{
			auto p = segment->manuallyUsedIds.insert(id);
			return p.second;
		}
		return false;
	}

	// Checks if the ID is being used
	bool isIdUsed(T id) const
	{
		if (auto segment = getSegmentForId(id); segment != nullptr)
			return (segment->manuallyUsedIds.find(id) != segment->manuallyUsedIds.end()) || (id < segment->nextId && segment->freeIds.find(id) == segment->freeIds.end());
		return false;
	}

	// Peeks the next ID
	T peekNextId(std::optional<T> startId = {}) const
	{
		const Segment* segment = &m_segments.begin()->second;
		if (startId.has_value())
		{
			if (const auto location = getSegmentForId(startId.value()); location != nullptr)
				segment = location;
		}

		return segment->nextId;
	}

	// Set the next ID
	/*
	void setNextId(T id)
	{
		m_nextId = id;
	}
	*/

	// Free an ID
	void freeId(T id)
	{
		if (auto segment = getSegmentForId(id); segment != nullptr)
		{
			// If the ID was manually used, and it was beyond our next ID, then don't add it to the free list.
			if (segment->manuallyUsedIds.erase(id) != 0 && id >= segment->nextId)
				return;

			segment->freeIds.insert(id);

			// See if we can condense the free IDs.
			if (!segment->freeIds.empty())
			{
				auto searchId = segment->nextId - 1;
				auto it = segment->freeIds.rbegin();
				while (it != segment->freeIds.rend())
				{
					if (*it == searchId)
					{
						--searchId;
						++it;
						continue;
					}
					break;
				}

				// Erase the IDs.
				if (it != segment->freeIds.rend())
				{
					segment->freeIds.erase(*(++it).base());
				}
			}
		}
	}

	// Reset the free IDs and set the next ID
	/*
	void resetAndSetNext(T nextId = 0)
	{
		m_freeIds.clear();
		m_nextId = nextId;
	}
	*/

protected:
	Segment* getSegmentForId(T id)
	{
		Segment* result = nullptr;
		for (auto& [segmentStartId, segment] : m_segments)
		{
			if (result == nullptr)
			{
				result = &segment;
				continue;
			}

			if (id >= segmentStartId && segmentStartId > result->startId)
				result = &segment;
		}
		return result;
	}

	const Segment* getSegmentForId(T id) const
	{
		const Segment* result = nullptr;
		for (const auto& [segmentStartId, segment] : m_segments)
		{
			if (result == nullptr)
			{
				result = &segment;
				continue;
			}

			if (id >= segmentStartId && segmentStartId > result->startId)
				result = &segment;
		}
		return result;
	}

protected:
	std::map<T, Segment> m_segments;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // IDGENERATOR_H
