#ifndef __POLYGONASSEMBLER_H__
#define __POLYGONASSEMBLER_H__

#include "osm.h"
#include "renderer.h"

class PolygonAssembler
{
	public:
		PolygonAssembler();
		~PolygonAssembler();

		void AddWay(OsmWay *w, bool inner);

		void Render(Renderer *r, int layer);

	private:
		void AssembleAndRender(Renderer *r, WayPointerArray *a, bool inner, int layer);
		WayPointerArray m_outerWays;
		WayPointerArray m_innerWays;

};

#endif //__POLYGONASSEMBLER_H__


