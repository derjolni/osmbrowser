#ifndef __SLABARRAY_H__
#define __SLABARRAY_H__

#include <stdlib.h> // for size_t

template<typename T, size_t SlabSize>
class SlabArray
{
	public:
		SlabArray();
		~SlabArray();
		void Add(T *item);
		T *Get(unsigned index) { return m_slabs[index >> m_slabShift][index & m_slabMask]; }
		unsigned GetCount() { return m_num; }
		void Clear();

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


#endif //__SLABARRAY_H__

