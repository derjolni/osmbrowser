// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __OSMDATA_H__
#define __OSMDATA_H__

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
//#include <stdio.h>
#include <wx/hashmap.h>
#include <wx/hashset.h>
#include <wx/arrstr.h>
#include <wx/dynarray.h>
#include "Eigen/Geometry"
#include "slabarray.h"
#define DISTSQUARED(x1, y1, x2, y2)  (((x1) - (x2)) * ((x1) - (x2)) + ((y1) - (y2)) * ((y1) - (y2)))

class DRect
{
	public:
		DRect()
		{
			m_x = m_y = m_h = 0;
			m_w = -1;
		}
		
		DRect(double minX, double minY, double w = -1, double h = 0)
		{
			m_x = minX;
			m_y = minY;
			m_w = w;
			m_h = h;
		}

		bool IsEmpty()
		{
			return m_w < 0;
		}

		void MakeEmpty()
		{
			m_x = m_y = m_h = 0;
			m_w = -1;
		}

		void Include(double x, double y)
		{
			if (m_w < 0)
			{
				m_x = x;
				m_y = y;
				m_w = m_h = 0;
			}
			else
			{
				if (x < m_x)
				{
					m_w += m_x - x;
					m_x = x;
				}
				else if (x > m_x + m_w)
				{
					m_w = x - m_x;
				}

				if (y < m_y)
				{
					m_h += m_y - y;
					m_y = y;
				}
				else if (y > m_y + m_h)
				{
					m_h = y - m_y;
				}
			}
		}

		double Right() const
		{
			return m_x + m_w;
		}

		void SetRight(double x)
		{
			m_w = x - m_x;
		}

		double Top() const
		{
			return m_y + m_h;
		}

		void SetTop(double y)
		{
			m_h = y - m_y;
		}

		void SetSize(double w, double h)
		{
			m_w = w;
			m_h = h;
		}

		double m_x, m_y;
		double m_w, m_h;

		DRect Add(DRect const &other)
		{
			DRect ret = *this;
			if (other.m_w < 0)
			{
				return ret;
			}
			
			ret.Include(other.m_x, other.m_y);
			ret.Include(other.Right(), other.Top());
			return ret;
		}
		
		DRect InterSect(DRect const &other)
		{
			DRect ret = *this;
			if (m_w < 0 || other.m_w < 0)
			{
				ret.MakeEmpty();
				return ret;;
			}
		
			double r = Right();
			double ro = other.Right();
			double t = Top();
			double to = other.Top();

			if (ro < ret.m_x || to < ret.m_y || other.m_x > r || other.m_y > t)
			{
				ret.MakeEmpty();
			}
			else
			{
				ret.m_x = ret.m_x > other.m_x ? ret.m_x : other.m_x;
				ret.m_y = ret.m_y > other.m_y ? ret.m_y : other.m_y;

				ret.SetRight(r < ro ? r : ro);
				ret.SetTop(t < to ? t : to);
			}
			return ret;
		}

		bool Intersects(double x1, double y1, double x2, double y2) const
		{
			if (m_w < 0)
			{
				return false;
			}

			Eigen::ParametrizedLine<double, 2> line = Eigen::ParametrizedLine<double, 2>::Through(Eigen::Vector2d(x1,y1),Eigen::Vector2d(x2,y2));

			double lensq = (x1-x2) * (x1-x2) + (y1-y2) *(y1-y2);

			Eigen::Hyperplane<double, 2>side = Eigen::Hyperplane<double, 2>::Through(Eigen::Vector2d(m_x,m_y), Eigen::Vector2d(m_x+m_w,m_y));

			double intersect = line.intersection(side);

			if (intersect >=0 && intersect * intersect < lensq)
			{
				double x = (line.origin() + intersect * line.direction())[0];

				if (x > m_x && x < m_x + m_w)
				{
					return true;
				}
			}


			side = Eigen::Hyperplane<double, 2>::Through(Eigen::Vector2d(m_x,m_y + m_h), Eigen::Vector2d(m_x+m_w,m_y + m_h));

			intersect = line.intersection(side);

			if (intersect >=0 && intersect * intersect < lensq)
			{
				double x = (line.origin() + intersect * line.direction())[0];

				if (x > m_x && x < m_x + m_w)
				{
					return true;
				}
			}


			side = Eigen::Hyperplane<double, 2>::Through(Eigen::Vector2d(m_x,m_y), Eigen::Vector2d(m_x,m_y + m_h));

			intersect = line.intersection(side);

			if (intersect >=0 && intersect * intersect < lensq)
			{
				double y = (line.origin() + intersect * line.direction())[1];

				if (y > m_y && y < m_y + m_h)
				{
					return true;
				}
			}

			side = Eigen::Hyperplane<double, 2>::Through(Eigen::Vector2d(m_x+m_w,m_y), Eigen::Vector2d(m_x+m_w,m_y + m_h));

			intersect = line.intersection(side);

			if (intersect >=0 && intersect * intersect < lensq)
			{
				double y = (line.origin() + intersect * line.direction())[1];

				if (y > m_y && y < m_y + m_h)
				{
					return true;
				}
			}

			return false;
		}

		bool Contains(double x, double y) const
		{
			if (m_w < 0)
			{
				return false;
			}
			return (x >= m_x && x < m_x + m_w && y >= m_y && y < m_y + m_h);
		}

		bool OverLaps(DRect const &other)
		{
			
			return !(InterSect(other).IsEmpty());
		}
		
};


class ListObject
{
	public:
	  ListObject(ListObject *next = NULL)
	  {
		m_next = next;

//        m_size = m_next ? m_next->m_size + 1 : 1;
	  }

	  ListObject *AppendList(ListObject *other)
	  {
		ListObject *l = this;
		while (l->m_next)
		{
			l = l->m_next;
		}
		l->m_next = other;

		return this;
	  }

	  static ListObject *Concat(ListObject *first, ListObject *second)
	  {
		if (!first)
		{
			return second;
		}

		if (!second)
		{
			return first;
		}

		return first->AppendList(second);
	  }
	  
	  virtual ~ListObject()
	  {
	  }
	  
	  void DestroyList()
	  {
		if (m_next)
		{
			m_next->DestroyList();
		}
		delete this;
	  }

	  unsigned GetSize()
	  {
		if (!m_next)
			return 1;

		return m_next->GetSize() + 1;
		//return m_size;
	  }

	  ListObject *m_next;
//      unsigned m_size;

};


class TagIndex
{
	public:
		static TagIndex Create(unsigned k, unsigned v = 0)
		{
			TagIndex ret;
			assert(k >=0);
			assert(k < 0xFFFFFFFF - 1);
			assert(v >=0);
			assert(v < 0xFFFFFFFF - 1);
			ret.m_keyIndex =  k;
			ret.m_valueIndex = v;

			return ret;
		}
		
		unsigned m_keyIndex;
		unsigned m_valueIndex;

		static unsigned MaxNumKeys() {return 0xFFFFFFFF -1;}
		static unsigned MaxNumValues() {return 0xFFFFFFFF - 1; }

		static TagIndex CreateInvalid()
		{
			TagIndex t;
			t.m_keyIndex = t.m_valueIndex = 0xFFFFFFFF;

			return t;

		}

		bool Valid()
		{

			return !(m_keyIndex == 0xFFFFFFFF && m_valueIndex == 0xFFFFFFFF);
		}

		// a tagIndex whose value-part = 0 will match only the key part
//		bool operator==(const TagIndex &other)
//		{
//			return (m_keyIndex == other.m_keyIndex) && (!m_valueIndex || !(other.m_valueIndex) || (m_valueIndex == other.m_valueIndex));
//		}

		bool Matches(const TagIndex &other) const
		{
			return (m_keyIndex == other.m_keyIndex) && (!m_valueIndex || !(other.m_valueIndex) || (m_valueIndex == other.m_valueIndex));
		}

		bool Equal(const TagIndex &other) const
		{
			return (m_keyIndex == other.m_keyIndex) && (m_valueIndex == other.m_valueIndex);
		}

};

WX_DECLARE_STRING_HASH_MAP( unsigned , StringToIndexMapper );

class StringStore
{
	public:

		unsigned GetInvalid()
		{
			return 0xFFFFFFFF;
		}

		unsigned Find(char const *s, bool add = true)
		{
			wxString st(s, wxConvUTF8);
			return Find(st, add);
		}

		unsigned Find(wxString const &s, bool add = true)
		{
			unsigned id = FindNoAdd(s);

			if (!add)
			{
				return id;
			}

			if (id == 0xFFFFFFFF)
			{
				return Add(s);
			}

			return id;
		}

		wxString const &Get(unsigned id)
		{
			if (id < m_strings.GetCount())
				return m_strings[id];

			return empty;
		}


	private:
		unsigned FindNoAdd(wxString const &s)
		{
			StringToIndexMapper::iterator f = m_mapper.find(s);

			if (f == m_mapper.end())
				return 0xFFFFFFFF;

			return f->second;

		}

		unsigned Add(wxString const &s)
		{
			unsigned id = m_strings.Add(s);
			m_mapper.insert(StringToIndexMapper::value_type(m_strings[id] , id));

			return id;
		}

		StringToIndexMapper m_mapper;
		wxArrayString m_strings;

		wxString empty;
};



class TagStore
{
	public:
	TagStore();
	~TagStore();
	unsigned GetNumKeys();
	char const *GetKey(unsigned keyIndex);
	unsigned GetNumValues(unsigned keyIndex);
	char const *GetValue(unsigned keyIndex, unsigned valueIndex);
	void GrowKeys(unsigned amount);

	void GrowValues(unsigned  key);

	TagIndex FindOrAdd(char const *key, char const *value);
	TagIndex Find(char const *key, char const *value);

	char const *GetKey(TagIndex index);
	char const *GetValue(TagIndex index);

	private:
	bool FindKey(char const *key, unsigned *k);
	bool FindValue(unsigned key, char const *value, unsigned *v);

	unsigned AddKey(char const *k);
	unsigned AddValue(unsigned key, char const *value);


	char **m_keys;
	unsigned m_numKeys;
	unsigned m_maxNumKeys;

	char ***m_values;
	unsigned *m_numValues;
	unsigned *m_maxNumValues;

	StringToIndexMapper **m_valueMappers;
	StringToIndexMapper m_keyMapper;
};

class OsmTag
	: public ListObject
{
	public:
	OsmTag(char const *key, char const *value = NULL, OsmTag *next = NULL);
	OsmTag(bool noCreate, char const *key, char const *value = NULL, OsmTag *next = NULL);
	OsmTag(OsmTag const &other);
	~OsmTag();


	static bool KeyExists(char const *key);

	bool Valid()
	{
		return m_index.Valid();
	}

	TagIndex Index() { return m_index; }

	char const *GetKey() const
	{
		return m_tagStore->GetKey(m_index);
	}

	char const *GetValue() const
	{
		return m_tagStore->GetValue(m_index);
	}

	bool HasTag(OsmTag const &other)
	{
		if (m_index.Matches(other.m_index))
		{
			return true;
		}
		if (!m_next)
			return false;

		return static_cast<OsmTag *>(m_next)->HasTag(other);

	}

	//exact match of this tag (and only this one, doesn't look at m_next)
	bool Equal(OsmTag *other) const
	{
		return m_index.Equal(other->m_index);
	}

	static TagStore *m_tagStore;
	TagIndex m_index;
	
};

class IdObject
{
	public:
		IdObject(unsigned id = 0)
		{
			m_id = id;
		}

		virtual ~IdObject()
		{
		}

		unsigned m_id;
};

int CompareIdObjectPointers(IdObject *o1, IdObject *o2);

WX_DEFINE_ARRAY(IdObject *, IdObjectArraySmall);
WX_DEFINE_SORTED_ARRAY(IdObject *, SortedIdObjectArray);

typedef SlabArray<IdObject *, 16>  IdObjectArrayLarge;

class IdObjectWithRole
	: public IdObject
{
	public:
		enum ROLE : char
		{
			OUTER,
			INNER
		};

		IdObjectWithRole(unsigned id, ROLE role)
			: IdObject(id)
		{
			m_role = role;
		}

		ROLE m_role;

};

WX_DEFINE_ARRAY(IdObjectWithRole *, IdObjectWithRoleArray);
WX_DEFINE_ARRAY_CHAR(IdObjectWithRole::ROLE, RolesArray);
WX_DEFINE_ARRAY_INT(unsigned, IdArray);

WX_DECLARE_HASH_SET(unsigned, wxIntegerHash, wxIntegerEqual, WXIdSet);

class IdSet
{
	public:
		void Add(unsigned id)
		{
			m_set.insert(id);
		}

		bool Has(unsigned id)
		{
			return m_set.find(id) != m_set.end();
		}

	private:
		WXIdSet m_set;
};

class IdObjectStore
{
	private:
		class ObjectList
		{
			public:
			ObjectList(unsigned index, ObjectList *tail)
			{
				m_next = tail;
				m_index = index;
			}

			~ObjectList()
			{
				if (m_next)
					delete m_next;
			}
			ObjectList *m_next;
			unsigned m_index;
		};
	public:
		IdObjectStore(unsigned bitmaskSize);
		~IdObjectStore();

		void GetStatistics(double *avgLen, double *standardDeviation, int *maxlen)
		{
			unsigned long long s1 = 0, s2 = 0;
			*maxlen = 0;
			int c;
			for (int i = 0; i < m_size; i++)
			{
				c = m_listSizes[i];
				s1 += c;
				s2 += c * c;
				if (c > *maxlen)
				{
					*maxlen = c;
				}
			}

			*avgLen = static_cast<double>(s1) / m_size;
			*standardDeviation = sqrt(static_cast<double>(s2 * m_size - s1 * s1))/ m_size;
		}

		ObjectList **m_locator;
		IdObjectArrayLarge m_objects;
		int m_size;
		unsigned m_mask;
		wxArrayInt m_listSizes;

		void AddObject(IdObject *object);
		IdObject *GetObject(unsigned id);
};


class IdObjectWithTags
	: public IdObject
{
	public:
		IdObjectWithTags(unsigned id = 0)
			: IdObject(id)
		{
			m_tags = NULL;
		}
		
		~IdObjectWithTags()
		{
			if (m_tags)
			{
				m_tags->DestroyList();
			}
		}

		void AddTag(char const *key, char const *value)
		{
			m_tags = new OsmTag(key, value, m_tags);
		}

		bool HasTag(OsmTag const &tag) const
		{
			return m_tags ? m_tags->HasTag(tag) : false;
		}

		bool HasTag(char const *key, char const *value = NULL)
		{
			OsmTag t(key, value);
			return m_tags ? m_tags->HasTag(t) : false;
		}

		void StealTags(IdObjectWithTags *other)
		{
			while (other->m_tags)
			{
				OsmTag *next = (OsmTag *)(other->m_tags->m_next);
				other->m_tags->m_next = m_tags;
				m_tags = other->m_tags;
				other->m_tags = next;
			}
		}

		bool HasTags() { return m_tags != NULL; }

		OsmTag *m_tags;
};

#define LONLATRESOLUTION 0x7FFFFFFF

class OsmRelationList;

class OsmNode
	: public IdObjectWithTags
{
	public:

	OsmNode(unsigned id, double lat, double lon)
		: IdObjectWithTags(id)
	{
		if (lon > 180.0)
			lon -= 360.0;
		if (lon < -180.0)
			lon += 360.0;

		m_ilat = (wxInt32)((lat/90.0) * LONLATRESOLUTION);
		m_ilon = (wxInt32)((lon/180.0) * LONLATRESOLUTION);
	}


	double Lon()
	{
		double r = (double)m_ilon/LONLATRESOLUTION;
		 r *= 180.0;
		 return r;
	}

	double Lat()
	{
		double r = (double)m_ilat/LONLATRESOLUTION;
		 r *= 90.0;
		 return r;
	}


	wxInt32 m_ilat;
	wxInt32 m_ilon;

};


class OsmWay
	: public IdObjectWithTags
{
	public:

	OsmWay(unsigned id)
		: IdObjectWithTags(id)
	{
		m_resolvedNodes = NULL;
		m_numResolvedNodes = 0;
		m_relations = NULL;
	}

	~OsmWay()
	{
		m_nodeRefs.Clear();

		if (m_resolvedNodes)
		{
			delete [] m_resolvedNodes;
		}
	}

	DRect GetBB()
	{
		DRect m_bb;
		if (m_bb.m_w < 0)
		{
			for (unsigned i = 0; i < m_numResolvedNodes; i++)
			{
				if (m_resolvedNodes[i])
				{
					m_bb.Include(m_resolvedNodes[i]->Lon(), m_resolvedNodes[i]->Lat());
				}
			}
		}
		return m_bb;
	}

	bool Intersects(DRect const &rect) const;

	OsmNode *GetClosestNode(double lon, double lat, double *foundDistSquared);

	unsigned FirstNodeId()
	{
		if (m_nodeRefs.GetCount())
		{
			return m_nodeRefs[0];
		}

		if (m_numResolvedNodes)
		{
			for (unsigned i = 0; i < m_numResolvedNodes; i++)
			{
				if (m_resolvedNodes[i])
				{
					return m_resolvedNodes[i]->m_id;
				}
			}
		}

		return 0;
	}
	unsigned LastNodeId()
	{
		if (m_nodeRefs.GetCount())
		{
			return m_nodeRefs.Last();
		}

		if (m_numResolvedNodes)
		{
			for (int i = m_numResolvedNodes-1; i >=0 ; i--)
			{
				if (m_resolvedNodes[i])
				{
					return m_resolvedNodes[i]->m_id;
				}
			}
		}

		return 0;

	}

	bool ContainsNode(OsmNode const *node) const
	{
		for (unsigned i = 0; i < m_numResolvedNodes; i++)
		{
			if (m_resolvedNodes[i] && (m_resolvedNodes[i]->m_id == node->m_id))
				return true;
		}

		return false;
	}

	void AddNodeRef(unsigned id)
	{
		m_nodeRefs.Add(id);
	}

	IdArray m_nodeRefs;

	void Resolve(IdObjectStore *store);
	// these are only valid after calling resolve
	OsmNode **m_resolvedNodes;
	unsigned m_numResolvedNodes;
//	DRect m_bb;

	// gets filled by OsmRelation::Resolve, so will be empty until the relations are resolved
	OsmRelationList *m_relations;
};


class OsmRelation
	: public OsmWay
{
	public:
	OsmRelation(unsigned id)
		: OsmWay(id)
	{
		m_resolvedWays = NULL;
		m_numResolvedWays = 0;
	}


	DRect GetBB()
	{
		DRect ret = OsmWay::GetBB();
		for (unsigned i = 0; i < m_numResolvedWays; i++)
		{
			if (m_resolvedWays[i])
			{
				ret.Add(m_resolvedWays[i]->GetBB());
			}
		}
		return ret;
	}
	
	~OsmRelation()
	{
		m_wayRefs.Clear();

		if (m_resolvedWays)
		{
			delete [] m_resolvedWays;
		}
	}
	
	IdArray m_wayRefs;

	void AddWayRef(unsigned id, IdObjectWithRole::ROLE role)
	{
		m_wayRefs.Add(id);
		m_roles.Add(role);
	}
	
	void Resolve(IdObjectStore *nodeStore, IdObjectStore *wayStore);

	OsmWay **m_resolvedWays;
	RolesArray m_roles;
	unsigned m_numResolvedWays;
	
};


class OsmRelationList
	: public ListObject
{
	public:
	OsmRelationList(OsmRelation *r, OsmRelationList *next)
		: ListObject(next)
	{
		m_relation = r;
	}

	OsmRelation *m_relation;
};

class OsmData
{
	public:
	OsmData();

	IdObjectStore m_nodes;
	IdObjectStore m_ways;
	IdObjectStore m_relations;
	

	// bounding box;
	double m_minlat, m_maxlat, m_minlon, m_maxlon;

	// parsing stuff
	void StartNode(unsigned id, double lat, double lon);
	void EndNode();
	void StartWay(unsigned id);
	void EndWay();
	void StartRelation(unsigned id);
	void EndRelation();

	void AddNodeRef(unsigned id);
	void AddWayRef(unsigned id, IdObjectWithRole::ROLE role);

	void AddTag(char const *k, char const *v);
	void AddAttribute(char const *k, char const *v);

	typedef enum
	{
		PARSE_TOPLEVEL,
		PARSE_NODE,
		PARSE_WAY,
		PARSE_RELATION
	} PARSINGSTATE;

	PARSINGSTATE m_parsingState;

	void Resolve();
	unsigned m_elementCount;

	bool m_skipAttribs;

};

WX_DEFINE_ARRAY_PTR(OsmWay *, WayPointerArray);
WX_DEFINE_ARRAY_PTR(OsmNode *, NodePointerArray);
WX_DEFINE_ARRAY_PTR(OsmRelation *, RelationPointerArray);


#endif
