#include "polygonassembler.h"

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

