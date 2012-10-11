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
		unsigned GetCount() const { return m_num; }
		void Clear();
		T *Get(unsigned index) { wxASSERT(m_num && m_num > index); return m_slabs[index >> m_slabShift][index & m_slabMask]; }
		T const *Get(unsigned index) const{ wxASSERT(m_num && m_num > index); return m_slabs[index >> m_slabShift][index & m_slabMask]; }
		T const *Last() const { return Get(m_num - 1); }
		T *Last() { return Get(m_num - 1); }
		T *operator[](unsigned index) { return Get(index); }
		T const *operator[](unsigned index) const { return Get(index); }

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

