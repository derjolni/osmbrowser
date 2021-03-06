// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "cairorenderer.h"
#include "utils.h"
#include "wxcairo.h"


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

void CairoRenderer::Commit()
{

	ClearToWhite(m_outputBitmap);

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





