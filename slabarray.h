#ifndef __SLABARRAY_H__
#define __SLABARRAY_H__

#include <stdlib.h> // for size_t
#include <wx/log.h>

template<typename T, size_t SlabSize>
class SlabArray
{
	public:
		SlabArray();
		~SlabArray();
		void Add(T const &item);
		unsigned GetCount() const { return m_num; }
		void Clear();
		T const &Get(unsigned index) const { wxASSERT(m_num && m_num > index); return m_slabs[index >> m_slabShift][index & m_slabMask]; }
		T const &Last() const { return Get(m_num - 1); }
		T const &operator[](unsigned index) const { return Get(index); }

	private:
		unsigned m_slabSize;
		unsigned m_slabShift;
		unsigned m_slabMask;
		T **m_slabs;

		unsigned m_num;
		unsigned m_maxNumSlabs;
		void GrowMaxNumSlabs();

		void SetZero();

};


template<typename T, size_t SlabShift>
SlabArray<T, SlabShift>::SlabArray()
{
	m_slabShift = SlabShift;
	m_slabMask = 0;
	m_slabSize = 1 << m_slabShift;
	for (unsigned i = 0; i < m_slabShift; i++)
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
void SlabArray<T, SlabSize>::Add(T const &item)
{
	unsigned slab = m_num >> m_slabShift;
	if (slab >= m_maxNumSlabs)
	{
		GrowMaxNumSlabs();
	}

	if (!m_slabs[slab])
	{
		m_slabs[slab] = new T[m_slabSize];
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
	delete [] m_slabs;
	SetZero();
}



template<typename T, size_t SlabSize>
void SlabArray<T, SlabSize>::GrowMaxNumSlabs()
{
	unsigned newNumSlabs = m_maxNumSlabs ? 2 * m_maxNumSlabs : 1;
	T **newSlabs =  new T*[newNumSlabs];

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
	m_maxNumSlabs++;
}

#endif //__SLABARRAY_H__

