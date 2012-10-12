#ifndef __POLYGONASSEMBLER_H__
#define __POLYGONASSEMBLER_H__

#include "osm.h"
#include "renderer.h"

class SimplePolygon
{
	public:
		SimplePolygon(int size)
		{
			m_x.Alloc(size);
			m_y.Alloc(size);
		}
		void AddWayPoints(OsmWay *w, bool reverse, Renderer::POINTADDMODE mode); // skipFirst will skip first *drawn* . when reverse==true this is the last in the Way
		void SendToRenderer(Renderer *r) const;
		void Append(SimplePolygon const &other, bool reverse);
		void Prepend(SimplePolygon const &other, bool reverse);
		void AddPoint(double x, double y)
		{
			m_x.Add(x);
			m_y.Add(y);
		}

		unsigned GetCount() const { return m_x.GetCount(); }
	private:
		wxArrayDouble m_x;
		wxArrayDouble m_y;
};

WX_DEFINE_ARRAY_PTR(SimplePolygon *, SimplePolygonArray);

class PolygonAssembler
{
	public:
		PolygonAssembler();
		~PolygonAssembler();

		void AddWay(OsmWay *w, bool inner);

		void Render(Renderer *r, int layer);

	private:
		void Dump(WayPointerArray const &ways);
		void AssembleAndRender(Renderer *r, WayPointerArray *a, bool inner, int layer);
		WayPointerArray m_outerWays;
		WayPointerArray m_innerWays;

};

#endif //__POLYGONASSEMBLER_H__


