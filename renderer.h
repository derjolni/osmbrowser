// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "osm.h"
#include "s_expr.h"
#include <wx/dcmemory.h>

class OsmTile;

class Renderer
{
	public:
		Renderer(int numLayers)
		{
			m_numLayers = numLayers;
		}
		virtual ~Renderer() { }

		double GetWidth() { return m_outputWidth; }
		double GetHeight() { return m_outputHeight; }
		
		enum TYPE
		{
			R_POLYGON,
			R_MULTIPOLYGON,
			R_OUTER,
			R_INNER,
			R_LINE
		};

		virtual bool StartTile(OsmTile *t, ExpressionMD5 const &md5) // the renderer *can * use this to implement cacheing. the MD5 is an md5 of the draw/color rules
		{
			return false;
		}

		virtual void EndTile(OsmTile *t, ExpressionMD5 const &md5)
		{
		}

		virtual void ClearOutput() = 0;
		virtual void Begin(Renderer::TYPE type, int layer) = 0;
		virtual void AddPoint(double x, double y, double xshift = 0, double yshift = 0) = 0;
		virtual void End() = 0;
		virtual void DrawCenteredText(char const *text, double x, double y, double angle, int r, int g, int b, int a, int layer) = 0;

		virtual bool SupportsLayers() = 0;

		void Rect(DRect const &re, double border, int r, int g, int b, int a, bool filled, int layer)
		{
			Rect(re.m_x, re.m_y, re.m_w, re.m_h, border, r, g, b, a, filled, layer);
		}

		void Rect(double x, double y, double w, double h, double border, int r, int g, int b, int a, bool filled, int layer)
		{
			SetLineColor(r,g,b,a);
			SetFillColor(r,g,b,a);
			Begin(filled ? R_POLYGON  : R_LINE, layer);
			AddPoint(x, y, -border, -border);
			AddPoint(x + w, y, border, -border);
			AddPoint(x + w, y + h, border, border);
			AddPoint(x, y + h, -border, border);
			AddPoint(x, y, -border, -border);
			End();
		}

		void SetLineColorf(float r, float g, float b, float a)
		{
			SetLineColor((int)(r * 255), (int)(g * 255),(int)(b * 255),(int)(a * 255));
		}

		void SetFillColorf(float r, float g, float b, float a)
		{
			SetFillColor((int)(r * 255), (int)(g * 255),(int)(b * 255),(int)(a * 255));
		}

		virtual void SetLineColor(int r, int g, int b, int a = 0) = 0;
		virtual void SetFillColor(int r, int g, int b, int a = 0) = 0;
		virtual void SetLineWidth(int width) = 0;

		virtual void ClearLayer(int layer = -1) = 0;

		// merge all layers and output to screen
		virtual void Commit() = 0;

		virtual void SetupViewport(DRect const &viewport)
		{
			  m_offX = viewport.m_x;
			  m_offY = viewport.m_y;
			  m_scaleX = m_outputWidth / viewport.m_w;
			  m_scaleY = m_outputHeight/ viewport.m_h;
			  
		}

		DRect GetViewport()
		{
			DRect ret(m_offX, m_offY, m_outputWidth/ m_scaleX, m_outputHeight / m_scaleY);
			return ret;
		}

		enum POINTADDMODE
		{
			NORMAL,
			REPEATFIRST,
			SKIPFIRST,
			ONLYFIRST
		};

		void AddWayPoints(OsmWay *w, bool reverse, POINTADDMODE mode); // skipFirst will skip first *drawn* . when reverse==true this is the last in the Way

	protected:
		double m_offX, m_offY, m_scaleX, m_scaleY;
		double m_outputWidth, m_outputHeight;
		int m_numLayers;
};

class RendererSimple
	: public Renderer
{
	public:
		RendererSimple(int numLayers)
			: Renderer(numLayers)
		{
			m_maxPoints = 1024;
			m_points = new RendererSimple::Point[m_maxPoints];
			m_shifts = new RendererSimple::Point[m_maxPoints];
			m_numPoints = 0;
			m_multiPolygonMode = false;
		}

		~RendererSimple()
		{
			delete [] m_points;
		}

		void Begin(Renderer::TYPE type, int layer)
		{
			m_numPoints = 0;
			m_type = type;
			m_curLayer = layer;
			if (type == Renderer::R_MULTIPOLYGON)
			{
				StartMultiPolygon();
			}
		}

		void AddPoint(double x, double y, double xs = 0, double ys = 0)
		{
			if (m_numPoints >= m_maxPoints)
			{
				Grow();
			}

			m_points[m_numPoints].x = x;
			m_points[m_numPoints].y = y;
			m_shifts[m_numPoints].x = xs;
			m_shifts[m_numPoints].y = ys;
			m_numPoints++;
		}

		void End()
		{
			if (!m_numPoints)
			{
				return;
			}
			
			switch(m_type)
			{
				case Renderer::R_LINE:
				DrawLine();
				break;
				case Renderer::R_POLYGON:
				DrawPolygon();
				break;
				case Renderer::R_OUTER:
				assert(m_multiPolygonMode);
				DrawOuterPolygon();
				m_type = Renderer::R_MULTIPOLYGON;
				break;
				case Renderer::R_INNER:
				DrawInnerPolygon();
				m_type = Renderer::R_MULTIPOLYGON;
				break;
				case Renderer::R_MULTIPOLYGON:
				 m_multiPolygonMode = false;
				 EndMultiPolygon();;
				break;
			};
		}

	protected:
		virtual void DrawPolygon() = 0;
		virtual void DrawLine() = 0;
		virtual void DrawOuterPolygon() = 0;
		virtual void DrawInnerPolygon() = 0;
		virtual void StartMultiPolygon() = 0;
		virtual void EndMultiPolygon() = 0;
	private:
		bool m_multiPolygonMode;
		struct Point
		{
			double x, y;
		};

		void Grow()
		{
			m_maxPoints *=2;
			RendererSimple::Point *n = new  RendererSimple::Point[m_maxPoints];
			RendererSimple::Point *ns = new  RendererSimple::Point[m_maxPoints];
			for (unsigned i = 0; i < m_numPoints; i++)
			{
				n[i] = m_points[i];
				ns[i] = m_shifts[i];
			}

			delete [] m_points;
			delete [] m_shifts;
			m_points = n;
			m_shifts = ns;
		}
	protected:
		Renderer::TYPE m_type;
		RendererSimple::Point *m_points;
		RendererSimple::Point *m_shifts;
		unsigned m_maxPoints;
		unsigned m_numPoints;
		int m_curLayer;
};

class RendererWxBitmap
	: public RendererSimple
{
	public:
		RendererWxBitmap(wxBitmap *output, int numLayers)
		:	RendererSimple(numLayers)
		{
			m_wxPoints = NULL;
			m_numWxPoints  = 0;
			m_maskColor = wxColour(255,0,255);
			m_brush.SetColour(0,0,0);
			m_pen.SetColour(0,0,0);
			m_dc = new wxMemoryDC[m_numLayers];
			m_layer = new wxBitmap[m_numLayers];
			Setup(output);
		}

		~RendererWxBitmap()
		{
			delete [] m_wxPoints;
			//m_dc.SelectObject(NULL);
		}

		bool SupportsLayers() { return true; }

		void DrawCenteredText(char const *s, double x, double y, double angle, int r, int g, int b, int a, int layer);

		void SetLineColor(int r, int g, int b, int a = 0)
		{
			m_pen.SetColour(wxColour(r,g,b));
		}

		void SetFillColor(int r, int g, int b, int a = 0)
		{
			m_brush.SetColour(wxColour(r,g,b));
		}

		void SetLineWidth(int width)
		{
			m_pen.SetWidth(width);
		}

		void Clear(int layer = -1)
		{
			if (layer >=0)
			{
				m_dc[layer].Clear();
			}
			else
			{
				for (int i = 0; i < m_numLayers; i++)
				{
					m_dc[i].Clear();
				}
			}
		}

		void Commit();
	private:
		void Setup(wxBitmap *outputBitmap);
		wxBitmap *m_layer;
		wxBitmap *m_output;
		wxMemoryDC *m_dc;
		wxPoint *m_wxPoints;
		wxPen m_pen;
		wxBrush m_brush;
		wxColour m_maskColor;
		unsigned m_numWxPoints;
	protected:
		void ScalePoints()
		{
			if (m_numWxPoints < m_maxPoints)
			{
				delete [] m_wxPoints;
				m_wxPoints = new wxPoint[m_maxPoints];
				m_numWxPoints = m_maxPoints;
			}
		
			for (unsigned i = 0; i < m_numPoints; i++)
			{
				m_wxPoints[i].x = static_cast<int>((m_points[i].x - m_offX) * m_scaleX + m_shifts[i].x);
				m_wxPoints[i].y = m_layer[0].GetHeight() - static_cast<int>((m_points[i].y - m_offY) * m_scaleY + m_shifts[i].y);
			}
		}
		void BlitWithTransparency(wxBitmap *from, wxImage  *to);

		void DrawPolygon();

		void DrawLine();
};

#endif
