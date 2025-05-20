#ifndef IDGENERATOR_H
#define IDGENERATOR_H

#include <concepts>
#include <set>
#include <cstdint>

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

using PlayerID = uint16_t;
using NPCID = uint32_t;

template <std::integral T>
class IdGenerator
{
public:
	IdGenerator() = default;
	IdGenerator(T startId) : m_nextId(startId) {}

	// Generate a new ID
	T getAvailableId()
	{
		if (!m_freeIds.empty())
		{
			T id = *m_freeIds.begin();
			m_freeIds.erase(m_freeIds.begin());
			return id;
		}
		return m_nextId++;
	}

	// Marks the ID as used
	bool markAsUsed(T id)
	{
		auto p = m_manuallyUsedIds.insert(id);
		return p.second;
	}

	// Checks if the ID is being used
	bool isIdUsed(T id) const
	{
		return (m_manuallyUsedIds.find(id) != m_manuallyUsedIds.end()) || (id < m_nextId && m_freeIds.find(id) == m_freeIds.end());
	}

	// Peeks the next ID
	T peekNextId() const
	{
		return m_nextId;
	}

	// Set the next ID
	void setNextId(T id)
	{
		m_nextId = id;
	}

	// Free an ID
	void freeId(T id)
	{
		// If the ID was manually used, and it was beyond our next ID, then don't add it to the free list.
		if (m_manuallyUsedIds.erase(id) != 0 && id >= m_nextId)
			return;

		m_freeIds.insert(id);

		// See if we can condense the free IDs.
		if (!m_freeIds.empty())
		{
			auto searchId = m_nextId - 1;
			auto it = m_freeIds.rbegin();
			while (it != m_freeIds.rend())
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
			if (it != m_freeIds.rbegin())
			{
				m_freeIds.erase(*(++it).base());
			}
		}
	}

	// Reset the free IDs and set the next ID
	void resetAndSetNext(T nextId = 0)
	{
		m_freeIds.clear();
		m_nextId = nextId;
	}

protected:
	T m_nextId = static_cast<T>(0);
	std::set<T> m_freeIds;
	std::set<T> m_manuallyUsedIds;
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // IDGENERATOR_H
