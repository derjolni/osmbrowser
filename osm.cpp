// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "osm.h"
#include <assert.h> // for lazy memory allocation checking
#include <stdlib.h>
#include <string.h>


//compare fuinction for sorted array
int CompareIdObjectPointers(IdObject *o1, IdObject *o2)
{
	if (o1->m_id > o2->m_id)
	{
		return 1;
	}
	else if (o1->m_id < o2->m_id)
	{
		return -1;
	}

	return 0;
}


TagStore::TagStore()
{
	m_keys = NULL;
	m_numKeys = 0;
	m_maxNumKeys = 0;

	m_maxNumValues = m_numValues = NULL;
	m_values = NULL;
	m_valueMappers	= NULL;

	GrowKeys(1024);
	
}

unsigned TagStore::GetNumKeys()
{
	return m_numKeys;
}

char const *TagStore::GetKey(unsigned keyIndex)
{
	assert(keyIndex >=0);
	assert(keyIndex < m_numKeys);
	return m_keys[keyIndex];
}

unsigned TagStore::GetNumValues(unsigned keyIndex)
{
	assert(keyIndex >=0);
	assert(keyIndex  < m_numKeys);
	
	return m_numValues[keyIndex];
}

char const *TagStore::GetValue(unsigned keyIndex, unsigned valueIndex)
{
	assert(keyIndex >=0);
	assert(keyIndex  < m_numKeys);
	assert(valueIndex >=0);
	assert(valueIndex < m_numValues[keyIndex]);

	return m_values[keyIndex][valueIndex];
}

void TagStore::GrowKeys(unsigned amount)
{
	char **newKeys = new char *[m_maxNumKeys + amount];
	char ***newValues = new char **[m_maxNumKeys + amount];
	unsigned *newNumValues =  new unsigned[m_maxNumKeys + amount];
	unsigned *newMaxNumValues = new unsigned[m_maxNumKeys + amount];
	StringToIndexMapper **newMappers = new StringToIndexMapper *[m_maxNumKeys + amount];

	for (unsigned i = 0; i < m_maxNumKeys; i++)
	{
		newKeys[i] = m_keys[i];
		newValues[i] = m_values[i];
		newNumValues[i] = m_numValues[i];
		newMaxNumValues[i] = m_maxNumValues[i];
		newMappers[i] = m_valueMappers[i];
	}

	for (unsigned i = 0; i < amount; i++)
	{
		newKeys[i + m_maxNumKeys] = NULL;
		newValues[i + m_maxNumKeys] = NULL;
		newMaxNumValues[i + m_maxNumKeys] = 0;
		newNumValues[i + m_maxNumKeys] = 0;
		newMappers[i + m_maxNumKeys] = NULL;
	}


	delete [] m_keys;
	m_keys = newKeys;

	delete [] m_values;
	m_values = newValues;

	delete [] m_numValues;
	m_numValues = newNumValues;
	
	delete [] m_maxNumValues;
	m_maxNumValues = newMaxNumValues;

	delete [] m_valueMappers;
	m_valueMappers = newMappers;

	m_maxNumKeys += amount;
}


void TagStore::GrowValues(unsigned  key)
{
	unsigned  amount = m_maxNumValues[key];

	if (!amount)
		amount = 1;


	char **newValues = new char *[m_maxNumValues[key] + amount];

	for (unsigned i = 0; i < m_maxNumValues[key]; i++)
		newValues[i] = m_values[key][i];

	for (unsigned i = 0; i < amount; i++)
		newValues[i + m_maxNumValues[key]] = NULL;
		
	delete [] m_values[key];

	m_values[key] = newValues;
	m_maxNumValues[key] += amount;
	

}

TagStore::~TagStore()
{
	for (unsigned i = 0; i < m_numKeys; i++)
	{
		free(m_keys[i]);
		delete m_valueMappers[i];
		for (unsigned j = 0; j < m_numValues[i]; j++)
		{
			free(m_values[i][j]);
		}
		delete [] m_values[i];
	}
	
	delete [] m_keys;
	delete [] m_values;
	delete [] m_maxNumValues;
	delete [] m_numValues;
	delete [] m_valueMappers;
}

TagIndex TagStore::Find(char const *key, char const *value)
{
	unsigned k = 0;


	if (!FindKey(key, &k))
		return TagIndex::CreateInvalid();

	if (!value)
	{
		return TagIndex::Create(k);
	}

	unsigned v = 0;

	if (!FindValue(k, value, &v))
		return TagIndex::CreateInvalid();

	return TagIndex::Create(k, v + 1);
}

char const *TagStore::GetKey(TagIndex index)
{
	assert(index.m_keyIndex >=0 && index.m_keyIndex < m_numKeys);
	
	return m_keys[index.m_keyIndex];
}

char const *TagStore::GetValue(TagIndex index)
{
	assert(index.m_keyIndex >=0 && index.m_keyIndex < m_numKeys);
	assert(index.m_valueIndex >= 1 && index.m_valueIndex - 1 < m_numValues[index.m_keyIndex]);

	return m_values[index.m_keyIndex][index.m_valueIndex - 1];
}

bool TagStore::FindKey(char const *key, unsigned *k)
{
	StringToIndexMapper::iterator f = m_keyMapper.find(wxString(key, wxConvUTF8));

	if (f == m_keyMapper.end())
		return false;

	*k = f->second;

	return true;
}


bool TagStore::FindValue(unsigned key, char const *value, unsigned *v)
{

	if (m_valueMappers[key])
	{
		StringToIndexMapper::iterator f = m_valueMappers[key]->find(wxString(value, wxConvUTF8));

		if (f == m_valueMappers[key]->end())
			return false;
	
		*v = f->second;
	
		return true;
	
	}

	for (unsigned i = 0; i < m_numValues[key]; i++)
	{
		if (!strcmp(value, m_values[key][i]))
		{
			*v = i;
			return true;
		}
	}

	return false;
}

unsigned TagStore::AddKey(char const *k)
{
	assert(m_numKeys < TagIndex::MaxNumKeys());

	if (m_numKeys >= m_maxNumKeys)
		GrowKeys(m_maxNumKeys ? m_maxNumKeys : 1024);

	m_keys[m_numKeys] = strdup(k);
	assert(m_keys[m_numKeys]);

	wxString  s = wxString(m_keys[m_numKeys], wxConvUTF8);

	m_keyMapper.insert(StringToIndexMapper::value_type(s, m_numKeys));

	m_numKeys++;

	return m_numKeys -1;
}

unsigned TagStore::AddValue(unsigned key, char const *value)
{
//        if (m_numValues[key] >= TagIndex::MaxNumValues())
//        {
//          printf("key = %d (%s) numv %d (%s)!\n", key, m_keys[key], m_numValues[key], value);
//        }

	assert(m_numValues[key] < TagIndex::MaxNumValues());

	if (m_numValues[key] >= m_maxNumValues[key])
		GrowValues(key);

	m_values[key][m_numValues[key]] = strdup(value);
	assert(m_values[key][m_numValues[key]]);


	if (m_numValues[key] > 5)
	{
		if (!m_valueMappers[key])
		{
			m_valueMappers[key] = new StringToIndexMapper;
			for (unsigned i = 0; i < m_numValues[key]; i++)
			{
				wxString s = wxString(m_values[key][i], wxConvUTF8);
				m_valueMappers[key]->insert(StringToIndexMapper::value_type(s, i));
			}
		}

		wxString s = wxString(m_values[key][m_numValues[key]], wxConvUTF8);
		m_valueMappers[key]->insert(StringToIndexMapper::value_type(s, m_numValues[key]));

	}


	m_numValues[key]++;
	
	return m_numValues[key] - 1;
}

TagStore *OsmTag::m_tagStore = 0;

TagIndex TagStore::FindOrAdd(char const *key, char const *value)
{
	TagIndex t = Find(key, value);

	if (t.Valid())
		return t;


	unsigned k = 0;

	if (!FindKey(key, &k))
	{
		k = AddKey(key);
	}

	if (!value)
	{
		return TagIndex::Create(k);
	}

	unsigned v = 0;


	if (!FindValue(k, value, &v));
	{
		v = AddValue(k, value);
	}
	
	return TagIndex::Create(k, v + 1);
}


OsmTag::OsmTag(char const *k, char const *v, OsmTag *next)
	: ListObject(next)
{
	if (!m_tagStore)
	{
		m_tagStore= new TagStore;
	}

	m_index = m_tagStore->FindOrAdd(k, v);
}

OsmTag::OsmTag(bool noCreate, char const *k, char const *v, OsmTag *next)
	: ListObject(next)
{
	if (!m_tagStore)
	{
		m_tagStore= new TagStore;
	}

	if (noCreate)
	{
		m_index = m_tagStore->Find(k, v);
	}
	else
	{
		m_index = m_tagStore->FindOrAdd(k, v);
	}
}

OsmTag::OsmTag(OsmTag const &other)
	: ListObject(other.m_next)
{
	m_index = other.m_index;
}


OsmTag::~OsmTag()
{
}

bool OsmTag::KeyExists(char const *key)
{
	if (!m_tagStore)
	{
		m_tagStore= new TagStore;
	}

	TagIndex t = m_tagStore->Find(key, NULL);

	return t.Valid();
}


OsmNode *OsmWay::GetClosestNode(double lon, double lat, double *foundDistSquared)
{
	double found = -1;
	unsigned foundIndex = 0;
	double distsq;
	
	for (unsigned i = 0; i < m_numResolvedNodes; i++)
	{
		if (m_resolvedNodes[i])
		{
			distsq = DISTSQUARED(m_resolvedNodes[i]->Lon(), m_resolvedNodes[i]->Lat(), lon, lat);

//			printf("%p:  %f %f  %f %f  %f\n", m_resolvedNodes[i], m_resolvedNodes[i]->m_lon, m_resolvedNodes[i]->m_lat, lon, lat, distsq);
			if (found < 0 || distsq < found)
			{
				foundIndex = i;
				found = distsq;
			}
		}
	}

	if (foundDistSquared)
	{
		*foundDistSquared = found;
	}

	return m_resolvedNodes[foundIndex];
}


void OsmWay::Resolve(IdObjectStore *store)
{

	unsigned size = m_nodeRefs.GetCount();

	if (!size)
	{
		return;
	}

	assert((!m_numResolvedNodes) || (m_numResolvedNodes == size));

	m_numResolvedNodes = size;

	if (!m_resolvedNodes)
	{
		m_resolvedNodes = new OsmNode *[size];
	}

	bool resolvedAll = true;
	for (unsigned i = 0; i < size; i++)
	{
		unsigned id = m_nodeRefs[i];
		m_resolvedNodes[i] = (OsmNode *)store->GetObject(id);

		if (!m_resolvedNodes[i])
			resolvedAll = false;
	}

	if (resolvedAll)
	{
		m_nodeRefs.Clear();
	}

}

bool OsmWay::Intersects(DRect const &rect) const
{
	assert(m_numResolvedNodes);
	OsmNode *prev = NULL;
	double px = 0, py = 0;
	for (unsigned i = 0; i < m_numResolvedNodes; i++)
	{
		OsmNode *n = m_resolvedNodes[i];
		if (n)
		{
			double x = n->Lon();
			double y = n->Lat();
			if (rect.Contains(x, y))
			{
				return true;
			}
			if (prev)
			{
				if (rect.Intersects(px,py,x,y))
				{
					return true;
				}
			}
			prev = n;
			px = x;
			py = y;
		}
	}

	return false;
}



void OsmRelation::Resolve(IdObjectStore *nodeStore, IdObjectStore *wayStore)
{
	OsmWay::Resolve(nodeStore);

	unsigned size = m_wayRefs.GetCount();

	if (!size)
	{
		return;
	}

	assert((!m_numResolvedWays) || (m_numResolvedWays == size));

	m_numResolvedWays = size;

	if (!m_resolvedWays)
	{
		m_resolvedWays = new OsmWay *[size];
	}

	bool resolvedAll = true;
	for (unsigned i = 0; i < size; i++)
	{
		unsigned id = m_wayRefs[i];
		m_resolvedWays[i] = (OsmWay *)wayStore->GetObject(id);

		if (!m_resolvedWays[i])
		{
			resolvedAll = false;
		}
		else
		{
			// add ourselves to this way's relations
			m_resolvedWays[i]->m_relations = new OsmRelationList(this, m_resolvedWays[i]->m_relations);
		}
	}

	if (resolvedAll)
	{
		m_wayRefs.Clear();
	}

	if (!HasTags() || HasTag("type", "multipolygon"))
	{
		unsigned outerWay = 0;
		unsigned numOuter = 0;
		for (unsigned i = 0; i < m_numResolvedWays; i++)
		{
			if (m_roles[i] == IdObjectWithRole::OUTER)
			{
				outerWay = i;
				numOuter++;
			}
		}

		if (numOuter == 1 && m_resolvedWays[outerWay] && m_resolvedWays[outerWay]->HasTags())
		{
			// steal tags
			StealTags(m_resolvedWays[outerWay]);
		}
	}

}

IdObjectStore::IdObjectStore(unsigned bitmaskSize)
{
	m_size = 1 << bitmaskSize;
	m_mask = 0;

	for (unsigned i = 0; i < bitmaskSize; i++)
		m_mask |= 1 << i;

	m_locator = new ObjectList *[m_size];

	memset(m_locator, 0, sizeof(ObjectList *) * m_size);

	m_listSizes.Add(0, m_size);
}


IdObjectStore::~IdObjectStore()
{
	for (int i = 0; i < m_size; i++)
	{
		if (m_locator[i])
			delete m_locator[i];
	}

	delete [] m_locator;

	m_objects.Clear();
}

void IdObjectStore::AddObject(IdObject *o)
{
	if (!o)
		return;

//    o->m_size = m_content ? m_content->m_size + 1 : 1;
	m_objects.Add(o);

	unsigned key = o->m_id & m_mask;

	m_locator[key] = new ObjectList(m_objects.GetCount() - 1, m_locator[key]);
	m_listSizes[key]++;
}

IdObject *IdObjectStore::GetObject(unsigned id)
{
	unsigned key = id & m_mask;

	ObjectList *o = m_locator[key];

	while (o)
	{
		if (m_objects[o->m_index]->m_id == id)
		{
			return m_objects[o->m_index];
		}
		o = o->m_next;
	}

	return NULL;
}


OsmData::OsmData()
	: m_nodes(24), m_ways(16), m_relations(16)
{
	m_minlat = m_maxlat = m_minlon = m_maxlon = 0;
	m_parsingState = PARSE_TOPLEVEL;
	m_elementCount = 0;
	m_skipAttribs = false;
}

void OsmData::StartNode(unsigned id, double lat, double lon)
{
	assert(m_parsingState == PARSE_TOPLEVEL);

	m_parsingState = PARSE_NODE;

	OsmNode *node = new OsmNode(id, lat, lon);


	if (!m_nodes.m_objects.GetCount())
	{
		m_minlat = m_maxlat = lat;
		m_minlon = m_maxlon = lon;
	}
	else
	{
		if (lat < m_minlat)
			m_minlat = lat;
		else if (lat > m_maxlat)
			m_maxlat = lat;

		if (lon < m_minlon)
			m_minlon = lon;
		else if (lon > m_maxlon)
			m_maxlon = lon;
	}

	m_nodes.AddObject(node);
	m_elementCount++;
}

void OsmData::EndNode()
{
	assert(m_parsingState == PARSE_NODE);

	m_parsingState = PARSE_TOPLEVEL;
}

void OsmData::StartWay(unsigned id)
{
	assert(m_parsingState == PARSE_TOPLEVEL);

	m_parsingState = PARSE_WAY;

	OsmWay *way = new OsmWay(id);

	m_ways.AddObject(way);
	m_elementCount++;
}

void OsmData::EndWay()
{
	static_cast<OsmWay *>(m_ways.m_objects.Last())->Resolve(&m_nodes);

	assert(m_parsingState == PARSE_WAY);

	m_parsingState = PARSE_TOPLEVEL;
}

void OsmData::StartRelation(unsigned id)
{
	assert(m_parsingState == PARSE_TOPLEVEL);

	m_parsingState = PARSE_RELATION;

	OsmRelation *rel = new OsmRelation(id);

	m_relations.AddObject(rel);
	m_elementCount++;
}

void OsmData::EndRelation()
{
	static_cast<OsmRelation *>(m_relations.m_objects.Last())->Resolve(&m_nodes, &m_ways);
	assert(m_parsingState == PARSE_RELATION);

	m_parsingState = PARSE_TOPLEVEL;
}


void OsmData::AddNodeRef(unsigned id)
{
	switch (m_parsingState)
	{
		default:
			abort();
			break;
		case PARSE_WAY:
			((OsmWay *)m_ways.m_objects.Last())->AddNodeRef(id);
			break;
		case PARSE_RELATION:
			((OsmRelation *)m_relations.m_objects.Last())->AddNodeRef(id);
			break;
	}
}

void OsmData::AddWayRef(unsigned id, IdObjectWithRole::ROLE role)
{
	assert(m_parsingState == PARSE_RELATION);

	((OsmRelation *)(m_relations.m_objects.Last()))->AddWayRef(id, role);
}

void OsmData::AddTag(char const *key, char const *value)
{
	switch(m_parsingState)
	{
		default:
			abort();
			break;
		case PARSE_NODE:
			static_cast<IdObjectWithTags *>(m_nodes.m_objects.Last())->AddTag(key, value);
			break;
		case PARSE_WAY:
			static_cast<IdObjectWithTags *>(m_ways.m_objects.Last())->AddTag(key, value);
			break;
		case PARSE_RELATION:
			static_cast<IdObjectWithTags *>(m_relations.m_objects.Last())->AddTag(key, value);
			break;
	}
}

void OsmData::AddAttribute(char const *key, char const *value)
{
	if (m_skipAttribs)
	{
		return;
	}

	char newkey[1024];

	newkey[0] = '@';
	strncpy(newkey+1, key, 1022);
	newkey[1023] = 0;
	
	switch(m_parsingState)
	{
		default:
			abort();
			break;
		case PARSE_NODE:
			static_cast<IdObjectWithTags *>(m_nodes.m_objects.Last())->AddTag(newkey, value);
			break;
		case PARSE_WAY:
			static_cast<IdObjectWithTags *>(m_ways.m_objects.Last())->AddTag(newkey, value);
			break;
		case PARSE_RELATION:
			static_cast<IdObjectWithTags *>(m_relations.m_objects.Last())->AddTag(newkey, value);
			break;
	}
}


void OsmData::Resolve()
{
	for (unsigned w = 0; w < m_ways.m_objects.GetCount(); w++)
	{
		OsmWay *way = dynamic_cast<OsmWay *>(m_ways.m_objects[w]);
		wxASSERT(way);
		way->Resolve(&m_nodes);
	}
	
	for (unsigned r = 0; r < m_relations.m_objects.GetCount(); r++)
	{
		OsmRelation *rel = dynamic_cast<OsmRelation *>(m_relations.m_objects[r]);
		wxASSERT(rel);
		rel->Resolve(&m_nodes, &m_ways);
	}
}
