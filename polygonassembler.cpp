#include "polygonassembler.h"


void SimplePolygon::AddWayPoints(OsmWay *w, bool reverse, Renderer::POINTADDMODE mode)
{
	int start = mode == Renderer::SKIPFIRST ? 1 : 0;
	unsigned maxCount = mode == Renderer::ONLYFIRST ? 1 : w->m_numResolvedNodes;
	OsmNode *first = NULL;
	for (unsigned j = start, count = 0; j < w->m_numResolvedNodes && count < maxCount; j++)
	{
		int index = reverse ? w->m_numResolvedNodes - 1 - j : j;
		OsmNode *node = w->m_resolvedNodes[index];

		if (!first)
		{
			first = node;
		}

		if (node)
		{
			AddPoint(node->Lon(), node->Lat());
			count++;
		}
		//! maybe warn if we encounter any unresolved nodes here? for now we just accept any drawing errors
	}

	if (mode == Renderer::REPEATFIRST && first)
	{
		AddPoint(first->Lon(), first->Lat());
	}

}

void SimplePolygon::SendToRenderer(Renderer *r) const
{
	for (unsigned i = 0; i < m_x.GetCount(); i++)
	{
		r->AddPoint(m_x[i], m_y[i]);
	}
}

void SimplePolygon::Append(SimplePolygon const &other, bool reverse)
{
	if (!reverse)
	{
		WX_APPEND_ARRAY(m_x, other.m_x);
		WX_APPEND_ARRAY(m_y, other.m_y);
	}
	else
	{
		for (unsigned i = other.m_x.GetCount() - 1; i >=0; i--)
		{
			m_x.Add(other.m_x[i]);
			m_y.Add(other.m_y[i]);
		}
	}
}

void SimplePolygon::Prepend(SimplePolygon const &other, bool reverse)
{
	if (!reverse)
	{
		WX_PREPEND_ARRAY(m_x, other.m_x);
		WX_PREPEND_ARRAY(m_y, other.m_y);
	}
	else
	{
		unsigned oc = other.GetCount();
		unsigned tc = GetCount();
		m_x.Add(0, oc);
		m_y.Add(0, oc);
		for (unsigned i = tc - 1; i >=0; i--)
		{
			m_x[oc+i] = m_x[i];
			m_y[oc+i] = m_y[i];
		}
		for (unsigned i = 0; i < oc; i++)
		{
			m_x[i] = other.m_x[oc - i - 1];
			m_y[i] = other.m_y[oc - i - 1];
		}
	}
}


PolygonAssembler::PolygonAssembler()
{
}

PolygonAssembler::~PolygonAssembler()
{
}

void PolygonAssembler::AddWay(OsmWay *w, bool inner)
{
	if (inner)
	{
		m_innerWays.Add(w);
	}
	else
	{
		m_outerWays.Add(w);
	}
}


void PolygonAssembler::AssembleAndRender(Renderer *r, WayPointerArray *a, bool inner, int layer)
{

	while (a->GetCount())
	{
		OsmWay *start = (*a)[0];
		OsmWay *cur = start;
		a->RemoveAt(0);

		r->Begin(inner ? Renderer::R_INNER : Renderer::R_OUTER, layer);
		bool reverse = false;
		if (cur->FirstNodeId() == cur->LastNodeId() )
		{
			r->AddWayPoints(cur, reverse, Renderer::REPEATFIRST);
		}
		else
		{
			// end is not same as start. see if we can find another way in the list which connects
			bool notFirst = false;
			while (cur)
			{
				r->AddWayPoints(cur, false, notFirst ? Renderer::SKIPFIRST : Renderer::NORMAL);
				notFirst = true;
				OsmWay *next = cur = NULL;
				unsigned end = reverse ? cur->LastNodeId() : cur->FirstNodeId();
				for (unsigned i = 0; i < a->GetCount(); i++)
				{
					next = (*a)[i];
					if (next->FirstNodeId() == end)
					{
						reverse = false;
						a->RemoveAt(i);
						cur = next;
						break;
					}
					else if (next->LastNodeId() == end)
					{
						a->RemoveAt(i);
						reverse = true;
						cur = next;
						break;
					}
				}
			}
			// add first point of startway to close the loop
			r->AddWayPoints(start, false, Renderer::ONLYFIRST);
		}
		r->End();

	}



}

void PolygonAssembler::Render(Renderer *r, int layer)
{
	r->Begin(Renderer::R_MULTIPOLYGON, layer);

	AssembleAndRender(r, &m_outerWays, false, layer);
	AssembleAndRender(r, &m_innerWays, true, layer);

	r->End();

}

