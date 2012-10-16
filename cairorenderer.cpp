// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "cairorenderer.h"
#include "utils.h"
#include "wxcairo.h"


TileCacheEntry::TileCacheEntry(unsigned id, wxBitmap const &src, wxRect const &rect, ExpressionMD5 const &md5, TileCacheEntry *next)
	: IdObject(id)
{
	m_bitmap = src.GetSubBitmap(rect);
	wxMemoryDC dc(m_bitmap);

	dc.SetPen(*wxRED_PEN);
	dc.SetBrush(*wxRED_BRUSH);
	dc.DrawCircle(rect.GetWidth()/2,rect.GetHeight()/2, rect.GetWidth()/3);


	m_md5 = md5;
	m_next = next;
	if (m_next)
	{
		m_next->m_prev = this;
	}
	m_prev = NULL;
}

TileCacheEntry::~TileCacheEntry()
{
	assert(!m_prev);
	assert(!m_next);
}


void TileCacheEntry::ToHead(TileCacheEntry **head, TileCacheEntry **tail)
{
	if (!m_prev)
	{
		assert(*head == this);
		return;
	}

	assert(*head != this);
	m_prev->m_next = m_next;

	if (m_next)
	{
		m_next->m_prev = m_prev;
	}
	else
	{
		assert(*tail == this);
		*tail = m_prev;
	}
	m_prev = NULL;
	m_next = *head;
	m_next->m_prev = this;
	*head = this;
}



TileCache::TileCache(int size)
	: m_tiles(size)
{
	m_size = size;
	m_ageListHead = m_ageListTail = NULL;
	m_num = 0;
}

TileCache::~TileCache()
{
	for (TileCacheEntryMap::iterator it = m_tiles.begin(); it != m_tiles.end(); it++)
	{
		if (it->second)
		{
			it->second->m_next = it->second->m_prev = NULL;
			delete it->second;
		}
	}
}


void TileCache::Delete(TileCacheEntry *t)
{
	m_num--;
	m_tiles.erase(t->m_id);
	if (!t->m_prev)
	{
		assert(t = m_ageListHead);
		m_ageListHead = t->m_next;
		if (m_ageListHead)
		{
			m_ageListHead->m_prev = NULL;
		}
		t->m_next = t->m_prev = NULL;
		if (t == m_ageListTail)
		{
			m_ageListTail = NULL;
		}
		delete t;
		return;
	}

	assert(t != m_ageListHead);

	t->m_prev->m_next = t->m_next;

	if(t->m_next)
	{
		t->m_next->m_prev = t->m_prev;
	}
	else
	{
		assert(t == m_ageListTail);
		m_ageListTail = t->m_prev;
		m_ageListTail->m_next = NULL;
	}

	t->m_next = t->m_prev = NULL;
	delete t;
}

void TileCache::Add(OsmTile *tile,ExpressionMD5 &md5, wxBitmap &bmp, wxRect const &rect)
{
	TileCacheEntry *e = m_tiles[tile->m_id];

	if (e)
	{
		if (!(e->MD5().Difference(md5)))
		{
			e->ToHead(&m_ageListHead, &m_ageListTail);
			return;
		}

		if(e->m_next)
		{
			e->m_next->m_prev = e->m_prev;
		}
		Delete(e);
	}
	e = new TileCacheEntry(tile->m_id, bmp, rect, md5, m_ageListHead);
	m_tiles[tile->m_id] = e;
	m_ageListHead = e;
	if (!m_ageListTail)
	{
		m_ageListTail = e;
	}
	m_num++;
	if (m_num > m_size)
	{
		Delete(m_ageListTail);
	}

}

TileCacheEntry *TileCache::Get(OsmTile *tile, ExpressionMD5 const &md5)
{
	TileCacheEntry *ret = m_tiles[tile->m_id];

	if (ret && !(ret->MD5().Difference(md5)))
	{
		return ret;
	}

	return NULL;
}


void CairoRenderer::Begin(Renderer::TYPE type, int layer)
{
	m_type = type;
	m_curLayer = layer;
	cairo_set_fill_rule (layers[layer], CAIRO_FILL_RULE_EVEN_ODD);
	if(type != R_INNER && type != R_OUTER)
	{
		cairo_new_path(layers[layer]);
	}
	else
	{
		cairo_close_path(layers[layer]);
		cairo_new_sub_path(layers[layer]);
	}
}

void CairoRenderer::End()
{
	switch(m_type)
	{
		case R_MULTIPOLYGON:
			//fallthrough
		case R_POLYGON:
			cairo_set_source_rgba(layers[m_curLayer], m_fillR, m_fillG, m_fillB, m_fillA);
			cairo_fill_preserve(layers[m_curLayer]);
			// fall through;
		case R_LINE:
			cairo_set_line_width(layers[m_curLayer], m_lineWidth);
			cairo_set_source_rgba(layers[m_curLayer], m_lineR, m_lineG, m_lineB, m_lineA);
			cairo_stroke(layers[m_curLayer]);
			break;
		case R_INNER:
			m_type = R_MULTIPOLYGON;
			break;
		case R_OUTER:
			m_type = R_MULTIPOLYGON;
			break;
	}
}

void CairoRenderer::ClearOutput()
{
	if (m_outputBitmap)
	{
		ClearToWhite(m_outputBitmap);
	}
}


void CairoRenderer::Commit()
{

//	ClearToWhite(m_outputBitmap);

	for (int i = 0; i < m_numLayers; i++)
	{
		cairo_surface_flush(layerBuffers[i]);
		OverlayImageSurface(layerBuffers[i], m_outputBitmap);
	}

//	wxBitmap tmpBitmap(tmp);

//	wxMemoryDC to;
//	to.SelectObject(*m_outputBitmap);

//	to.DrawBitmap(tmpBitmap, 0, 0);

}

void CairoPdfRenderer::Begin(Renderer::TYPE type, int layer)
{
	cairo_set_fill_rule (m_context, CAIRO_FILL_RULE_EVEN_ODD);
	if(type != R_INNER && type != R_OUTER)
	{
		cairo_new_path(m_context);
	}
	else
	{
		cairo_close_path(m_context);
		cairo_new_sub_path(m_context);
	}
}


void CairoPdfRenderer::AddPoint(double x, double y, double xshift, double yshift)
{
	cairo_line_to(m_context, (x - m_offX) * m_scaleX + xshift, m_outputHeight - (y - m_offY) * m_scaleY + yshift);
}

void CairoPdfRenderer::End()
{
	switch(m_type)
	{
		case R_MULTIPOLYGON:
			//fallthrough
		case R_POLYGON:
			cairo_set_source_rgba(m_context, m_fillR, m_fillG, m_fillB, m_fillA);
			cairo_fill_preserve(m_context);
			// fall through;
		case R_LINE:
			cairo_set_line_width(m_context, m_lineWidth);
			cairo_set_source_rgba(m_context, m_lineR, m_lineG, m_lineB, m_lineA);
			cairo_stroke(m_context);
		case R_INNER:
			break;
		case R_OUTER:
			break;
	}
}

void CairoPdfRenderer::DrawCenteredText(char const *text, double x, double y, double angle, int r, int g, int b, int a, int layer){ }





