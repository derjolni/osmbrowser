// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __CAIRORENDERER__H__
#define __CAIRORENDERER__H__

#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-ps.h>
#include "renderer.h"
#include "tiledrawer.h"
#include "frame.h"
#include "s_expr.h"

class TileCacheEntry
	: public IdObject
{
	public:
		TileCacheEntry(unsigned id, wxBitmap const &src, wxRect const &rect, ExpressionMD5 const &md5, TileCacheEntry *m_tail);
		~TileCacheEntry();

		void ToHead(TileCacheEntry **head, TileCacheEntry **tail);

		ExpressionMD5 const &MD5() { return m_md5; }

//		friend class TileCache;
		wxBitmap m_bitmap;
		ExpressionMD5 m_md5;
		unsigned m_age;
		TileCacheEntry *m_next;
		TileCacheEntry *m_prev;

		void DumpList()
		{
			printf("list:");
			for (TileCacheEntry *t = this; t; t = t->m_next)
			{
				printf(" %u ", t->m_id);
			}
			printf("\n");
		}
		private:
};

WX_DECLARE_HASH_MAP(int, TileCacheEntry *, wxIntegerHash, wxIntegerEqual, TileCacheEntryMap);

class TileCache
{
	public:
		TileCache(int size);
		~TileCache();

		void Add(OsmTile*t, ExpressionMD5 &md5, wxBitmap &bmp, wxRect const &rect);
		TileCacheEntry *Get(OsmTile *tile, ExpressionMD5 const &md5);

	private:
			void Delete(TileCacheEntry *t);
		unsigned m_size, m_num;
		TileCacheEntryMap m_tiles;
		TileCacheEntry *m_ageListHead, *m_ageListTail;
};

class CairoRendererBase
	: public Renderer
{
	public:
	CairoRendererBase(int numLayers)
		: Renderer(numLayers)
	{
		m_lineWidth = 1;
		m_fillR = m_fillG = m_fillB = 0;
		m_lineR = m_lineG = m_lineB = 0;
		m_lineA = m_fillA = .5;
	}


	void SetLineColor(int r, int g, int b, int a = 0)
	{
		m_lineA = (255 - a) / 255.0;
		m_lineR = (r / 255.0);
		m_lineG = (g / 255.0);
		m_lineB = (b / 255.0);
	}

	void SetFillColor(int r, int g, int b, int a = 0)
	{
		m_fillA = (255 - a) / 255.0;
		m_fillR = (r / 255.0);
		m_fillG = (g / 255.0);
		m_fillB = (b / 255.0);
	}

	void SetLineWidth(int width)
	{
		m_lineWidth = width;
	}

	protected:
		double m_fillR, m_fillG, m_fillB, m_fillA;
		double m_lineR, m_lineG, m_lineB, m_lineA;
		double m_lineWidth;

} ;

class CairoRenderer
	: public CairoRendererBase
{
	public:
		CairoRenderer(wxBitmap *output, int numLayers)
			: CairoRendererBase(numLayers), m_tileCache(100)
		{
			layerBuffers = new cairo_surface_t *[m_numLayers];
			layers = new cairo_t *[m_numLayers];

			Setup(output);
		}

		~CairoRenderer()
		{
			for (int i = 0; i < m_numLayers; i++)
			{
				cairo_surface_destroy(layerBuffers[i]);
				cairo_destroy(layers[i]);
			}

			delete [] layerBuffers;
			delete [] layers;
		}

		void Begin(Renderer::TYPE type, int layer);

		void AddPoint(double x, double y, double xshift = 0, double yshift = 0)
		{
			cairo_line_to(layers[m_curLayer], (x - m_offX) * m_scaleX + xshift, m_outputHeight - (y - m_offY) * m_scaleY + yshift);
		}

		void End();

		bool SupportsLayers() { return true; }

		void ClearLayer(int layer = -1)
		{
			for (int i = 0; i < m_numLayers; i++)
			{
				if (layer < 0 || layer == i)
				{
					cairo_set_operator(layers[i], CAIRO_OPERATOR_SOURCE);
					cairo_set_source_rgba(layers[i], 0,0,0,0);
					cairo_paint(layers[i]);
					cairo_set_operator(layers[i], CAIRO_OPERATOR_OVER);
				}
			}
		}

		void Commit();
		void ClearOutput();

		virtual void DrawCenteredText(char const *text, double x, double y, double angle, int r, int g, int b, int a, int layer)
		{
			// not implemented
		}

		bool StartTile(OsmTile *t, ExpressionMD5 const &md5)
		{
			ExpressionMD5 smd5;
			smd5.Init();
			smd5.Add(&m_scaleX, sizeof(m_scaleX));
			smd5.Add(&m_scaleY, sizeof(m_scaleY));
			smd5.Add(md5);
			smd5.Finish();

			TileCacheEntry *e = m_tileCache.Get(t, smd5);

			if (e && m_outputBitmap)
			{
				wxMemoryDC dc(*m_outputBitmap);
				dc.DrawBitmap(e->m_bitmap, (int)(t->m_x - m_offX) * m_scaleX, (int)(t->m_y - m_offY) * m_scaleY);
				return true;
			}

			return false;
		}

		void EndTile(OsmTile *t, ExpressionMD5 const &md5)
		{
			if (!m_outputBitmap)
			{
				return;
			}
			int x = (int)((t->m_x - m_offX) * m_scaleX);
			int y = (int)((t->m_y - m_offY) * m_scaleY);
			int w = (int)(t->m_w * m_scaleX);
			int h = (int)(t->m_h * m_scaleY);

			if ( x >=0 && x + w <= m_outputBitmap->GetWidth() && y >=0 && (y + h) <= m_outputBitmap->GetHeight())
			{
				ExpressionMD5 smd5;
				smd5.Init();
				smd5.Add(&m_scaleX, sizeof(m_scaleX));
				smd5.Add(&m_scaleY, sizeof(m_scaleY));
				smd5.Add(md5);
				smd5.Finish();
				m_tileCache.Add(t, smd5, *m_outputBitmap, wxRect(x,y,w,h));
			}
		}


	private:
		TileCache m_tileCache;
		void Setup(wxBitmap *output)
		{
			m_outputBitmap = output;
			
			for (int i = 0; i < m_numLayers; i++)
			{
				layerBuffers[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, output->GetWidth(), output->GetHeight());
				layers[i] = cairo_create(layerBuffers[i]);

			}

			m_outputWidth = output->GetWidth();
			m_outputHeight = output->GetHeight();

		}

		Renderer::TYPE  m_type;
		int m_curLayer;
		cairo_t **layers;
		cairo_surface_t **layerBuffers;

		wxBitmap *m_outputBitmap;
};


class CairoPdfRenderer
	: public CairoRendererBase
{
	public:
		CairoPdfRenderer(wxString const &fileName, int w, int h)
			: CairoRendererBase(1)
		{
			m_surface = cairo_pdf_surface_create(fileName.mb_str(wxConvUTF8), w, h);
			m_context = cairo_create(m_surface);
			m_outputWidth = w;
			m_outputHeight = h;
		}

		~CairoPdfRenderer()
		{
			cairo_show_page(m_context);
			cairo_destroy(m_context);
			cairo_surface_flush(m_surface);
			cairo_surface_destroy(m_surface);
		}

		void ClearOutput() { }

		void Begin(Renderer::TYPE type, int layer);
		void AddPoint(double x, double y, double xshift = 0, double yshift = 0);
		void End();
		virtual void DrawCenteredText(char const *text, double x, double y, double angle, int r, int g, int b, int a, int layer);

		bool SupportsLayers() { return false; }

		void ClearLayer(int layer = -1) { /* pdf doesn't support clearing */ }

		void Commit() { /* nop, we don't support layers */ }


		
	private:
		Renderer::TYPE  m_type;
		cairo_surface_t *m_surface;
		cairo_t *m_context;
};

class PdfJob
	: public RenderJob
{
	public:
		PdfJob(MainFrame *mainFrame, Renderer *r)
			: RenderJob(r)
		{
			m_mainFrame = mainFrame;
		}

		bool MustCancel(double progress)
		{
			m_mainFrame->SetProgress(progress);
			return false;
		}

	private:
		MainFrame *m_mainFrame;
};


#endif
