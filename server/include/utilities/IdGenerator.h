#ifndef UTILITIES_ID_GENERATOR_H
#define UTILITIES_ID_GENERATOR_H

#include <set>

template <typename T>
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
		m_freeIds.insert(id);
	}

	// Reset the free IDs and set the next ID
	void resetAndSetNext(T nextId = 0)
	{
		m_freeIds.clear();
		m_nextId = nextId;
	}

protected:
	T m_nextId = 0;
	std::set<T> m_freeIds;
};

#endif // UTILITIES_ID_GENERATOR_H
