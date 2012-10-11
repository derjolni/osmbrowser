// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "parse.h"
#include "osm.h"
#include <expat.h>
#include <string.h>
#include <assert.h>

// op windows heeft expat dit nodig. als het niet gedefinieerd is definieer het als niks
// stel dat we ooit op windows moeten werken dan is het er vast bij getypt
#ifndef XMLCALL
	#define XMLCALL
#endif


#define FILEFORMAT_VERSION "OsmBrowserCachev1.3\004"

static XML_Char const *get_attribute(const XML_Char *name, const XML_Char **attrs)
{
	int count = 0;

	while (attrs[count *2])
	{
		if (!strcmp(name, attrs[count*2]))
		{
			return attrs[count * 2+1];
		}
		count++;
	}

	return NULL;
}

static void ReadAttribs(OsmData *o, XML_Char const **attrs)
{
	XML_Char const *keys[] =
	{
		"user",
		"uid",
		"visible",
		"version",
		"changeset",
		"timestamp"
	};

	int size = sizeof(keys) / sizeof( XML_Char *);

	for (int i = 0; i < size; i++)
	{
		char const *v = get_attribute(keys[i], attrs);

		if (v)
			o->AddAttribute(keys[i], v);
	}
}

void XMLCALL start_element_handler(void *user_data, const XML_Char *name, const XML_Char **attrs)
{
	OsmData *o = (OsmData *)user_data;

	if (!(o->m_elementCount % 1000000))
	{
		printf("parsed %uM elements\n", o->m_elementCount/1000000);

		double a,s;
		int m;
		o->m_nodes.GetStatistics(&a, &s, &m);
		printf(" statistics: a %g s %g max %d | ", a, s, m);
		o->m_ways.GetStatistics(&a, &s, &m);
		printf("a %g s %g max %d | ", a, s, m);
		o->m_relations.GetStatistics(&a, &s, &m);
		printf("a %g s %g max %d \n", a, s, m);
		
	}

	if (!strcmp(name, "node"))
	{
		XML_Char const *latS = get_attribute("lat", attrs);
		XML_Char const *lonS = get_attribute("lon", attrs);
		XML_Char const *idS = get_attribute("id", attrs);
		double lat = strtod(latS, NULL);
		double lon = strtod(lonS, NULL);
		unsigned id = strtoul(idS, NULL, 0);

		assert(latS && lonS && idS); // in case it didn't crash on the strto* functions

		o->StartNode(id, lat, lon);

		ReadAttribs(o, attrs);
	}
	else if (!strcmp(name, "way"))
	{
		XML_Char const *idS = get_attribute("id", attrs);
		unsigned id = strtoul(idS, NULL, 0);
		assert(idS);
		o->StartWay(id);
		ReadAttribs(o, attrs);
	}
	else if (!strcmp(name, "relation"))
	{
		XML_Char const *idS = get_attribute("id", attrs);
		unsigned id = strtoul(idS, NULL, 0);

		assert(idS);
		o->StartRelation(id);
		ReadAttribs(o, attrs);
	}
	else if (!strcmp(name, "tag"))
	{
		XML_Char const *key = get_attribute("k", attrs);
		XML_Char const *value = get_attribute("v", attrs);
		assert(key && value);

		o->AddTag(key, value);
	}
	else if (!strcmp(name, "nd"))
	{
		XML_Char const *idS = get_attribute("ref", attrs);
		unsigned id = strtoul(idS, NULL, 0);
		assert(idS);

		o->AddNodeRef(id);
	}
	else if (!strcmp(name, "member"))
	{
		XML_Char const *type = get_attribute("type", attrs);
		XML_Char const *idS = get_attribute("ref", attrs);
		XML_Char const *roleS = get_attribute("role", attrs);

		IdObjectWithRole::ROLE role = IdObjectWithRole::OUTER;
		if (roleS && !strcmp(roleS, "inner"))
		{
			role = IdObjectWithRole::INNER;
		}
		unsigned id = strtoul(idS, NULL, 0);
		assert(idS && type);

		if (!strcmp(type, "node"))
		{
			o->AddNodeRef(id);
		}
		else if (!strcmp(type, "way"))
		{
			o->AddWayRef(id, role);
		}
	}
}

void XMLCALL end_element_handler(void *user_data, const XML_Char *name)
{
	OsmData *o = (OsmData *)user_data;

	if (!strcmp(name, "node"))
	{
		o->EndNode();
	}
	else if (!strcmp(name, "way"))
	{
		o->EndWay();
	}
	else if (!strcmp(name, "relation"))
	{
		o->EndRelation();
	}
}



OsmData *parse_osm(FILE *file, bool skipAttribs)
{
	char buffer[1024];
	int len;

	// cannot handle 16bit character sets
	// so if expat is configured wrong bail out
	assert(sizeof(XML_Char) == sizeof(char));

	OsmData *ret = new OsmData;

	ret->m_skipAttribs = skipAttribs;

	XML_Parser xml = XML_ParserCreate(NULL);

	XML_SetStartElementHandler(xml, start_element_handler);
	XML_SetEndElementHandler(xml, end_element_handler);

	XML_SetUserData(xml, ret);
	

	unsigned count = 0;
	while (0 != (len = fread(buffer, 1, 1024, file)))
	{
		XML_Parse(xml, buffer, len, feof(file));
		count++;
		if (!(count % 10240))
			printf("parsed %uMB\n", count / 1024);
	}

	XML_ParserFree(xml);

	ret->Resolve();

	return ret;
}

#define MAXTAGSIZE 10240

static void ReadTags(OsmData *d, unsigned count, FILE *f, bool readAttribs = false)
{
	char keybuf[MAXTAGSIZE+1];
	char valbuf[MAXTAGSIZE+1];

	keybuf[MAXTAGSIZE] = valbuf[MAXTAGSIZE] = 0;

	for (unsigned i = 0; i < count; i++)
	{
		int count = 0;
		int c;
		while((c = getc(f)))
		{
			if (count < MAXTAGSIZE)
			{
				keybuf[count] = c;
			}
			count++;
		}

		if (count < MAXTAGSIZE)
		{
			keybuf[count] = 0;
		}
		// read val
		count = 0;
		while((c = getc(f)))
		{
			if (count < MAXTAGSIZE)
			{
				valbuf[count] = c;
			}
			count++;
		}

		if (count < MAXTAGSIZE)
		{
			valbuf[count] = 0;
		}

		if (readAttribs)
		{
			d->AddAttribute(keybuf, valbuf);
		}
		else
		{
			d->AddTag(keybuf,valbuf);
		}

	}


	
}

static void ReadNode(OsmData *d, FILE *f)
{
	double lat, lon;
	unsigned id, tagCount;
	int ret;
	ret = fread(&id, sizeof(id), 1, f);
	assert(ret == 1);
	ret = fread(&lat, sizeof(lat), 1, f);
	assert(ret == 1);
	ret = fread(&lon, sizeof(lon), 1, f);
	assert(ret == 1);
	ret =fread(&tagCount, sizeof(tagCount), 1, f);
	assert(ret == 1);

	d->StartNode(id, lat, lon);
	ReadTags(d, tagCount, f);
	d->EndNode();

}

static void ReadWay(OsmData *d, FILE *f)
{
	unsigned id, tagCount, nodeRefCount;
	int ret;
	ret = fread(&id, sizeof(id), 1, f);
	assert(ret == 1);
	d->StartWay(id);
	ret = fread(&nodeRefCount, sizeof(nodeRefCount), 1, f);
	assert(ret == 1);
	for (unsigned i = 0; i < nodeRefCount; i++)
	{
		ret = fread(&id, sizeof(id), 1, f);
		assert(ret == 1);
		d->AddNodeRef(id);
	}
	ret = fread(&tagCount, sizeof(tagCount), 1, f);
	assert(ret == 1);
	ReadTags(d, tagCount, f);
	d->EndWay();
}

static void ReadRelation(OsmData *d, FILE *f)
{
	unsigned id, tagCount, nodeRefCount, wayRefCount;
	IdObjectWithRole::ROLE role;
	int ret;
	ret = fread(&id, sizeof(id), 1, f);
	assert(ret == 1);
	d->StartRelation(id);
	ret = fread(&nodeRefCount, sizeof(nodeRefCount), 1, f);
	assert(ret == 1);
	for (unsigned i = 0; i < nodeRefCount; i++)
	{
		ret = fread(&id, sizeof(id), 1, f);
		assert(ret == 1);
		d->AddNodeRef(id);
	}

	ret = fread(&wayRefCount, sizeof(wayRefCount), 1, f);
	assert(ret == 1);
	for (unsigned i = 0; i < wayRefCount; i++)
	{
		ret = fread(&id, sizeof(id), 1, f);
		assert(ret == 1);
		ret = fread(&role, sizeof(role), 1, f);
		assert(ret == 1);
		d->AddWayRef(id, role);
	}
	ret = fread(&tagCount, sizeof(tagCount), 1, f);
	assert(ret == 1);
	ReadTags(d, tagCount, f);
	d->EndRelation();
}


OsmData *parse_binary(FILE *f, bool skipAttribs)
{
	OsmData *ret = new OsmData();

	char buffer[1024];


	int rret = fread(buffer, strlen(FILEFORMAT_VERSION), 1, f);

	if (rret !=1)
	{
		printf("reading failed!");
		return NULL;
	}

	if (strcmp(FILEFORMAT_VERSION, buffer))
	{
		printf("not a valid OsmBrowser cache file!");
		return NULL; // invalid cache data
	}

	ret->m_skipAttribs = skipAttribs;
	unsigned count = 0;
	while (!feof(f))
	{
		if (!(count % (1024*1024)))
		{
			printf("Read %dM elements \n", count / (1024*1024));
		}

		count++;
		
		int c = fgetc(f);

		if (feof(f))
		{
			break;
		}

		switch(c)
		{
			case 'N':
				ReadNode(ret, f);
				break;
			case 'W':
				ReadWay(ret, f);
				break;
			case 'R':
				ReadRelation(ret, f);
				break;
			default:
				printf("illegal element at position %u\n", count);
				abort();
				break;
		}
	}


	ret->Resolve();

	return ret;
}

static void WriteTags(OsmTag *tags, FILE *f)
{
	unsigned zero = 0;

	if (!tags)
	{
		fwrite(&zero, sizeof(zero), 1, f);
		return;
	}

	unsigned size = tags->GetSize();

	fwrite(&(size), sizeof(size), 1, f);

	for (OsmTag *t = tags; t; t = static_cast<OsmTag *>(t->m_next))
	{
		fwrite(t->GetKey(), sizeof(char), strlen(t->GetKey()) + 1, f);
		fwrite(t->GetValue(), sizeof(char), strlen(t->GetValue()) + 1, f);
	}
	
}

void write_binary(OsmData *d, FILE *f)
{
	fputs(FILEFORMAT_VERSION, f);
	unsigned zero = 0;
	printf("writing nodes...\n" );
	for (unsigned n = 0; n < d->m_nodes.m_objects.GetCount(); n++)
	{
		OsmNode *node = dynamic_cast<OsmNode *>(d->m_nodes.m_objects[n]);
		wxASSERT(node);
		fputc('N', f);
		double lat = node->Lat();
		double lon = node->Lon();
		fwrite(&(node->m_id), sizeof(node->m_id), 1, f);
		fwrite(&(lat), sizeof(double), 1, f);
		fwrite(&(lon), sizeof(double), 1, f);

		WriteTags(node->m_tags, f);
	}

	printf("writing ways...\n" );
	for (unsigned w = 0; w < d->m_ways.m_objects.GetCount(); w++)
	{
		OsmWay *way = dynamic_cast<OsmWay *>(d->m_ways.m_objects[w]);
		wxASSERT(way);
		fputc('W', f);
		fwrite(&(way->m_id), sizeof(way->m_id), 1, f);

		if (way->m_nodeRefs.GetCount()) // if the noderefs still exists, this means the way is not fully resolved, so use the refs
		{
			unsigned size = way->m_nodeRefs.GetCount();
			fwrite(&(size), sizeof(size), 1, f);

			for (unsigned i = 0; i < way->m_nodeRefs.GetCount(); i++)
			{
				unsigned id = way->m_nodeRefs[i];
				fwrite(&(id), sizeof(id), 1, f);
			}
			
		}
		else if (way->m_resolvedNodes) // the refs don't exists, so the way must be fully resolved
		{
			fwrite(&(way->m_numResolvedNodes), sizeof(way->m_numResolvedNodes), 1, f);

			for (unsigned  i = 0; i < way->m_numResolvedNodes; i++)
			{
				unsigned id = way->m_resolvedNodes[i]->m_id;
				fwrite(&(id), sizeof(id), 1, f);
			}
		}
		else
		{
			fwrite(&zero, sizeof(zero), 1, f);
		}

		WriteTags(way->m_tags, f);
	}


	printf("writing relations...\n" );
	for (unsigned r = 0; r < d->m_relations.m_objects.GetCount(); r++)
	{
		OsmRelation *rel = dynamic_cast<OsmRelation *>(d->m_relations.m_objects[r]);
		wxASSERT(rel);
		fputc('R', f);
		fwrite(&(rel->m_id), sizeof(rel->m_id), 1, f);

		if (rel->m_nodeRefs.GetCount())
		{
			unsigned size = rel->m_nodeRefs.GetCount();
			fwrite(&(size), sizeof(size), 1, f);

			for (unsigned i = 0; i < rel->m_nodeRefs.GetCount(); i++)
			{
				unsigned id = rel->m_nodeRefs[i];

				fwrite(&(id), sizeof(id), 1, f);
			}
			
		}
		else if (rel->m_resolvedNodes) // the refs don't exists, so the way must be fully resolved
		{
			fwrite(&(rel->m_numResolvedNodes), sizeof(rel->m_numResolvedNodes), 1, f);

			for (unsigned  i = 0; i < rel->m_numResolvedNodes; i++)
			{
				unsigned id = rel->m_resolvedNodes[i]->m_id;
				fwrite(&(id), sizeof(id), 1, f);
			}
		}
		else
		{
			fwrite(&zero, sizeof(zero), 1, f);
		}

		if (rel->m_wayRefs.GetCount())
		{
			unsigned size = rel->m_wayRefs.GetCount();
			fwrite(&(size), sizeof(size), 1, f);

			for (unsigned i = 0; i < rel->m_wayRefs.GetCount(); i++)
			{
				unsigned id = rel->m_wayRefs[i];
				IdObjectWithRole::ROLE role = rel->m_roles[i];
				fwrite(&(id), sizeof(id), 1, f);
				fwrite(&(role), sizeof(role), 1, f);
			}
			
		}
		else if (rel->m_resolvedWays) // the refs don't exists, so the way must be fully resolved
		{
			fwrite(&(rel->m_numResolvedWays), sizeof(rel->m_numResolvedWays), 1, f);

			for (unsigned  i = 0; i < rel->m_numResolvedWays; i++)
			{
				unsigned id = rel->m_resolvedWays[i]->m_id;
				IdObjectWithRole::ROLE role = rel->m_roles[i];
				fwrite(&(id), sizeof(id), 1, f);
				fwrite(&(role), sizeof(id), 1, f);
			}
		}
		else
		{
			fwrite(&zero, sizeof(zero), 1, f);
		}

		WriteTags(rel->m_tags, f);
	}
	printf("done writing\n");
}
