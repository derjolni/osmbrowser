#include "slabarray.h"

template<typename T, size_t SlabSize>
SlabArray<T, SlabSize>::SlabArray()
{
	m_slabShift = SlabSize;
	m_slabMask = 0;
	m_slabSize = 1 << m_slabSize;
	for (int i = 0; i < m_slabShift; i++)
	{
		m_slabMask |= (1 << i);
	}
	SetZero();
}


template<typename T, size_t SlabSize>
void SlabArray<T, SlabSize>::SetZero()
{
	m_num = m_maxNumSlabs = 0;

	m_slabs = NULL;
}

template<typename T, size_t SlabSize>
SlabArray<T, SlabSize>::~SlabArray()
{
	Clear();
}

template<typename T, size_t SlabSize>
void SlabArray<T, SlabSize>::Add(T *item)
{
	unsigned slab = m_num >> m_slabShift;
	if (slab >= m_maxNumSlabs)
	{
		GrowMaxNumSlabs();
	}

	if (!m_slabs[slab])
	{
		m_slabs[slab] = new T*[m_slabSize];
	}

	m_slabs[slab][m_num & m_slabMask] = item;

	m_num++;
}

template<typename T, size_t SlabSize>
void SlabArray<T, SlabSize>::Clear()
{
	for (unsigned i = 0; i < m_maxNumSlabs; i++)
	{
		delete [] m_slabs[i];
	}

	SetZero();
}



template<typename T, size_t SlabSize>
void SlabArray<T, SlabSize>::GrowMaxNumSlabs()
{
	unsigned newNumSlabs = m_maxNumSlabs ? 2 * m_maxNumSlabs : 1;
	T **newSlabs =  new T**[newNumSlabs];

	for (unsigned i = 0; i < m_maxNumSlabs; i++)
	{
		newSlabs[i] = m_slabs[i];
	}
	for (unsigned i = m_maxNumSlabs; i < newNumSlabs; i++)
	{
		newSlabs[i] = NULL;
	}

	delete [] m_slabs;
	m_slabs = newSlabs;
}
