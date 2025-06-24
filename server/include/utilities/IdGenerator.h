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
		std::optional<T> endId;
		std::set<T> freeIds;
		std::set<T> manuallyUsedIds;
	};

	std::map<T, Segment> m_segments;

public:
	IdGenerator() { createSegment(T(0)); }
	IdGenerator(T startId) { createSegment(startId); }

	// Create a new segment starting at the specified ID.
	bool createSegment(T startId)
	{
		Segment* previous = nullptr;
		Segment* next = nullptr;
		for (auto& [segmentStartId, segment] : m_segments)
		{
			// Find the segment previous to the startId.
			if (segmentStartId < startId && (previous == nullptr || segmentStartId > previous->startId))
				previous = &segment;
			// Find the segment next to the startId.
			if (segmentStartId > startId && (next == nullptr || segmentStartId < next->startId))
				next = &segment;
		}

		Segment newSegment{ .startId = startId, .nextId = startId };
		if (next != nullptr)
			newSegment.endId = next->startId - 1;

		// If we found a segment, move any overlapping IDs to the new segment.
		if (previous != nullptr)
		{
			previous->endId = startId - 1;

			// Check if the nextId of the previous segment is beyond our startId.
			if (previous->nextId > startId)
			{
				newSegment.nextId = previous->nextId;
				previous->nextId = startId;
			}
			// Move over the manually used ids.
			for (auto id : previous->manuallyUsedIds)
			{
				if (id >= startId)
				{
					newSegment.manuallyUsedIds.insert(id);
					previous->manuallyUsedIds.erase(id);
				}
			}
			// Clear out any free ids.
			for (auto id : previous->freeIds)
			{
				if (id >= startId)
					previous->freeIds.erase(id);
			}
			condense(*previous);
		}

		m_segments.emplace(std::make_pair(startId, std::move(newSegment)));
		return true;
	}

	// Generate a new ID
	T getAvailableId(std::optional<T> startId = {})
	{
		for (auto& [segmentStartId, segment] : m_segments)
		{
			// Segment too early?  Continue.
			if (segmentStartId < startId.value_or(T{}))
				continue;

			// If there is a free id, just use it.
			if (!segment.freeIds.empty())
			{
				T id = *segment.freeIds.begin();
				segment.freeIds.erase(segment.freeIds.begin());
				return id;
			}

			// Find a free ID.
			auto searchId = segment.nextId + 1;
			while (true)
			{
				if (!segment.manuallyUsedIds.contains(searchId))
				{
					segment.nextId = searchId;
					return searchId;
				}
				++searchId;

				if (segment.endId.has_value() && searchId > segment.endId.value())
					break;
			}
		}

		// Uh oh.
		throw std::runtime_error("No available ID found in IdGenerator.");
	}

	// Marks the ID as used
	bool markAsUsed(T id)
	{
		if (auto segment = getSegmentForId(id); segment != nullptr)
		{
			auto p = segment->manuallyUsedIds.insert(id);
			if (id == segment->nextId)
				++segment->nextId;
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

	// Free an ID
	void freeId(T id)
	{
		if (auto segment = getSegmentForId(id); segment != nullptr)
		{
			// If the ID was manually used, and it was beyond our next ID, then don't add it to the free list.
			if (segment->manuallyUsedIds.erase(id) != 0 && id >= segment->nextId)
				return;

			segment->freeIds.insert(id);
			condense(*segment);
		}
	}

	// Reset the free IDs and set the next ID
	void reset()
	{
		for (auto& [segmentStartId, segment] : m_segments)
		{
			segment.freeIds.clear();
			segment.manuallyUsedIds.clear();
			segment.nextId = segmentStartId;
		}
	}

protected:
	void condense(Segment& segment)
	{
		// See if we can condense the free IDs.
		if (!segment.freeIds.empty())
		{
			auto searchId = segment.nextId - 1;
			auto it = segment.freeIds.rbegin();
			while (it != segment.freeIds.rend())
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
			if (it != segment.freeIds.rend())
			{
				segment.freeIds.erase(*(++it).base());
			}
		}
	}

	Segment* getSegmentForId(T id)
	{
		for (auto& [segmentStartId, segment] : m_segments)
		{
			if (id < segmentStartId || (segment.endId.has_value() && segment.endId.value() < id))
				continue;

			// If the segment has free ids, pick it.
			if (!segment.freeIds.empty())
				return &segment;

			// If there is space left, pick it.
			if (!segment.endId.has_value() || segment.nextId < segment.endId.value())
				return &segment;
		}
		return nullptr;
	}

	const Segment* getSegmentForId(T id) const
	{
		for (auto& [segmentStartId, segment] : m_segments)
		{
			if (id < segmentStartId || (segment.endId.has_value() && segment.endId.value() < id))
				continue;

			// If the segment has free ids, pick it.
			if (!segment.freeIds.empty())
				return &segment;

			// If there is space left, pick it.
			if (!segment.endId.has_value() || segment.nextId < segment.endId.value())
				return &segment;
		}
		return nullptr;
	}
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // IDGENERATOR_H
