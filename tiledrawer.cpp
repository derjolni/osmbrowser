// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "tiledrawer.h"
#include "rulecontrol.h"
#include "polygonassembler.h"

TileWay::TileWay(OsmWay *way, TileWay *next)
	: ListObject(next)
{
	m_way = way;
}

TileWay::~TileWay()
{
}


TileWay *OsmTile::GetWaysContainingNode(OsmNode *node)
{
	TileWay *ret = NULL;

	for (TileWay *t = m_ways; t; t = static_cast<TileWay *>(t->m_next))
	{
		if (t->m_way->ContainsNode(node))
		{
			ret = new TileWay(t->m_way, ret);
		}
	}

	return ret;
}


TileDrawer::TileDrawer(double minLon,double minLat, double maxLon, double maxLat, double dLon, double dLat)
{
	m_selection = NULL;
	m_selectionColor = wxColour(255,0,0);
	m_selectedWay = NULL;
	m_selectedRelation = NULL;
	m_selectedTile = NULL;

	m_drawRule = NULL;
	m_colorRules = NULL;

	m_xNum = static_cast<int>((maxLon - minLon) / dLon) + 1;
	m_yNum = static_cast<int>((maxLat - minLat) / dLat) + 1;
	m_minLon = minLon;
	m_w = m_xNum * dLon;
	m_minLat = minLat;
	m_h = m_yNum *dLat;
	m_dLon = dLon;
	m_dLat = dLat;
	unsigned id = 0;
	// build a list of empty tiles;
	m_tileArray = new OsmTile **[m_xNum];
	for (unsigned x = 0; x < m_xNum; x++)
	{
		m_tileArray[x] = new OsmTile *[m_yNum];
		for (unsigned y = 0; y < m_yNum; y++)
		{
			m_tiles.Add(new OsmTile(id++, m_minLon + x * dLon , m_minLat + y * dLat, m_minLon + (x + 1) * dLon, m_minLat + (y+1) * dLat));
			m_tileArray[x][y] = m_tiles.Last();
		}
	}
 }


bool TileDrawer::RenderTiles(RenderJob *job, int maxNumToRender)
{
	bool mustCancel = false;


	if (!job->m_visibleTiles)
	{
		job->m_visibleTiles = GetTiles(job->m_bb);

		job->m_curTile = job->m_visibleTiles;

		job->m_numTilesToRender = job->m_visibleTiles->GetSize();
		job->m_numTilesRendered = 0;
	}

	if (!job->m_visibleTiles || job->m_finished)
	{
		job->m_finished = true;
		return true;
	}

	int count = 0;
	while (job->m_curTile && !mustCancel && (count++ < maxNumToRender))
	{
		OsmTile *t = job->m_curTile->m_tile;
		if (t->m_ways)
		{
			if (job->m_curLayer < 0) // curlayer < 0 means the renderer supports layers
			{
				Rect(job->m_renderer, wxEmptyString, *t, -1, 0,155,55, 20, NUMLAYERS);
			}
			
			if (t->OverLaps(job->m_bb))
			{
				for (TileWay *w = t->m_ways; w && !mustCancel; w = static_cast<TileWay *>(w->m_next))
				{
					for (OsmRelationList *rl = w->m_way->m_relations; rl; rl = static_cast<OsmRelationList *>(rl->m_next))
					{
						if (!(job->m_renderedRelationIds.Has(rl->m_relation->m_id)))
						{
							RenderRelation(job, rl->m_relation);
						}
					}
					if (!(job->m_renderedWayIds.Has(w->m_way->m_id)))
					{
						RenderWay(job, w->m_way);
					}
				}	// for way
			}  // if overlaps
		}
		else
		{
			count--;
		}
		//not needed anymore for cairo renderer. move to mustcancel callback?

		job->m_curTile = static_cast<TileList *>(job->m_curTile->m_next);
		job->m_numTilesRendered++;

		double progress = static_cast<double>(job->m_numTilesRendered)/ (job->m_numTilesToRender);
		if (job->m_curLayer >= 0)
		{
			progress /= NUMLAYERS;
		}

		mustCancel = job->MustCancel(progress);
	}	 // while curTile

	if (!job->m_curTile)
	{
		if (job->m_curLayer >= 0)
		{
			job->m_curLayer++;
			if (job->m_curLayer < NUMLAYERS)
			{
				job->m_curTile = job->m_visibleTiles;
			}
		}
		DrawOverlay(job->m_renderer, true);
	}
	else
	{
		DrawOverlay(job->m_renderer, false);
	}

	if (!job->m_curTile)
	{
		job->m_finished = true;
	}

	return job->m_finished;
}

void TileDrawer::Rect(Renderer *renderer, wxString const &text, double lon1, double lat1, double lon2, double lat2, double border, int r, int g, int b, int a, int layer)
{
	renderer->Rect(lon1, lat1, lon2 - lon1, lat2 - lat1, border, r, g, b, a, false, layer);
	renderer->DrawCenteredText(text.mb_str(wxConvUTF8), (lon1 + lon2)/2, (lat1 + lat2)/2, 0, r, g, b, a,  layer);
}


void TileDrawer::RenderRelation(Renderer *rnd, OsmRelation *r, wxColour lineColour, bool poly, wxColour fillColour, int width, int layer)
{
	if (!poly)
	{
		for (unsigned i = 0; i < r->m_numResolvedWays; i++)
		{
			if (r->m_resolvedWays[i])
			{
				RenderWay(rnd, r->m_resolvedWays[i], lineColour, poly, fillColour, width, layer);
//					job->m_renderedWayIds.Add(r->m_resolvedWays[i]->m_id); // if it has been drawn in the relation, don't draw it again on it's own
			}
		}
	}
	else
	{
		PolygonAssembler a;
		for (unsigned i = 0; i < r->m_numResolvedWays; i++)
		{
			OsmWay *w = r->m_resolvedWays[i];
			if (w)
			{
				a.AddWay(w, r->m_roles[i] == IdObjectWithRole::INNER);
//					job->m_renderedWayIds.Add(w->m_id); // if it has been drawn in the relation, don't draw it again on it's own
			}
		}
		rnd->SetLineColor(lineColour.Red(), lineColour.Green(), lineColour.Blue());
		rnd->SetFillColor(fillColour.Red(), fillColour.Green(), fillColour.Blue());
		a.Render(rnd, layer);
	}
}

void TileDrawer::RenderRelation(RenderJob *job, OsmRelation *r)
{

	if (m_drawRule && (m_drawRule->Evaluate(r) == LogicalExpression::S_FALSE))
	{
		return;
	}

	wxColour c = wxColour(150,150,150);
	bool poly = false;
	int layer = 1;
	if (m_colorRules)
	{
		for (int i = 0; i < m_colorRules->m_num; i++)
		{
			if (m_colorRules->m_rules[i]->Evaluate(r) == LogicalExpression::S_TRUE)
			{
				c = m_colorRules->m_pickers[i]->GetColour();
				poly = m_colorRules->m_checkBoxes[i]->IsChecked();
				layer = m_colorRules->m_layers[i]->GetSelection();
				break; // stop after first match
			}
		}
	}

	if (job->m_curLayer < 0 || job->m_curLayer == layer)
	{
		RenderRelation(job->m_renderer, r, c, poly, c, 1, job->m_curLayer <0 ? layer : 0);
	}
	job->m_renderedRelationIds.Add(r->m_id);

}

// render using default colours. should plug in rule engine here
void TileDrawer::RenderWay(RenderJob *job, OsmWay *w)
{
	bool draw = true;

	if (m_drawRule && (m_drawRule->Evaluate(w) == LogicalExpression::S_FALSE))
	{
		draw = false;
	}

	if (draw)
	{
		wxColour c = wxColour(150,150,150);
		bool poly = false;
		int layer = 1;
		if (m_colorRules)
		{
			for (int i = 0; i < m_colorRules->m_num; i++)
			{
				if (m_colorRules->m_rules[i]->Evaluate(w) == LogicalExpression::S_TRUE)
				{
					c = m_colorRules->m_pickers[i]->GetColour();
					poly = m_colorRules->m_checkBoxes[i]->IsChecked();
					layer = m_colorRules->m_layers[i]->GetSelection();
					break; // stop after first match
				}
			}
		}

		if (job->m_curLayer < 0 || job->m_curLayer == layer)
		{
			RenderWay(job->m_renderer, w, c, poly, c, 1, job->m_curLayer <0 ? layer : 0);
			job->m_renderedWayIds.Add(w->m_id);
		}
	}
}


void TileDrawer::RenderWay(Renderer *r, OsmWay *w, wxColour lineColour, bool poly, wxColour fillColour, int width, int layer)
{


	r->SetLineWidth(width);
	r->SetLineColor(lineColour.Red(), lineColour.Green(), lineColour.Blue());
	r->SetFillColor(fillColour.Red(), fillColour.Green(), fillColour.Blue());

	if (!poly)
	{
		r->Begin(Renderer::R_LINE, layer);
		for (unsigned j = 0; j < w->m_numResolvedNodes; j++)
		{
			OsmNode *node = w->m_resolvedNodes[j];
		
			if (node)
			{
				r->AddPoint(node->Lon(), node->Lat());
			}
			else
			{
				r->End();
				r->Begin(Renderer::R_LINE, layer);
			}
		
		}
		r->End();
	}
	else
	{
		r->Begin(Renderer::R_POLYGON, layer);
		for (unsigned j = 0; j < w->m_numResolvedNodes; j++)
		{
			OsmNode *node = w->m_resolvedNodes[j];
		
			if (node)
			{
				r->AddPoint(node->Lon(), node->Lat());
			}
			
		}
		r->End();
	}
}

TileSpans *TileDrawer::GetTileSpans(TileList *all)
{
	TileSpans *ret = new TileSpans;

	for (TileList *t = all; t; t = static_cast<TileList *>(t->m_next))
	{
		ret->Add(t->m_tile);
	}
	
	ret->Ref();
	return ret;
	
}

void TileDrawer::LonLatToIndex(double lon, double lat, int *x, int *y)
{
	if (x)
	{
		*x = static_cast<int>(floor((lon - m_minLon) / m_dLon));
		if (*x < 0) *x = 0;
		if (*x > static_cast<int>(m_xNum - 1)) *x = static_cast<int>(m_xNum - 1);
	}

	if (y)
	{
		*y = static_cast<int>(floor((lat - m_minLat) / m_dLat));
	if (*y < 0) *y = 0;
	if (*y > static_cast<int>(m_yNum - 1)) *y = static_cast<int>(m_yNum - 1);
	}
}


TileList *TileDrawer::GetTiles(double minLon, double minLat, double maxLon, double maxLat)
{
//            printf("gettiles (%g %g)-(%g-%g):\n", minLon, minLat, maxLon, maxLat);
	int xMin = 0, xMax = 0, yMin = 0, yMax = 0;

	LonLatToIndex(minLon, minLat, &xMin, &yMin);
	LonLatToIndex(maxLon, maxLat, &xMax, &yMax);


	TileList *ret = NULL;


	for (int x = xMin; x <= xMax; x++)
	{
		for (int y = yMin; y <= yMax; y++)
		{
			ret = new TileList(m_tileArray[x][y], ret);
		}
	}

	
	if (ret)
	{
		ret->Ref();
	}
	
	return ret;

	
}

OsmNode *TileDrawer::GetClosestNodeInTile(int x, int y, double lon, double lat, double *foundDistSq)
{
	double fDSq = 0;
	double shortest = -1;
	OsmNode *found = NULL;
	OsmNode *n;

	for (TileWay *t = m_tileArray[x][y]->m_ways; t; t = static_cast<TileWay *>(t->m_next))
	{
		OsmWay * w = t->m_way;
		if (!m_drawRule || m_drawRule->Evaluate(w))
		{
			n = w->GetClosestNode(lon,lat, &fDSq);

			if (shortest < 0.0 || fDSq < shortest)
			{
//				printf(" found %p distsq %f\n", n, fDSq);
				shortest = fDSq;
				found = n;
			}
		}
	}

	if (foundDistSq)
	{
		*foundDistSq = shortest;
	}

	return found;
}


OsmNode *TileDrawer::GetClosestNode(double lon, double lat)
{
	int x =0, y = 0;
	double distSq = -1;

	LonLatToIndex(lon, lat, &x, &y);

	OsmNode *found = GetClosestNodeInTile(x, y, lon, lat, &distSq);

	return found;
}


bool TileDrawer::SetSelection(double lon, double lat)
{
	OsmNode *s = GetClosestNode(lon, lat);

//	printf("setsel %f %f : %p (%f %f)\n", lon, lat, s, s->m_lon, s->m_lat);

	if (s != m_selection)
	{
		m_selection = s;
		return true;
	}

	return false;
}

bool TileDrawer::SetSelectedWay(OsmWay *way)
{
	if (way != m_selectedWay)
	{
		m_selectedWay = way;
		return true;
	}
	return false;
}

bool TileDrawer::SetSelectedRelation(OsmRelation *rel)
{
	if (rel != m_selectedRelation)
	{
		m_selectedRelation = rel;
		return true;
	}
	return false;
}


void TileDrawer::DrawOverlay(Renderer *r, bool clear)
{
	if (!r->SupportsLayers())
	{
		return; //! warn maybe?
	}

	if (clear)
		r->Clear(NUMLAYERS);
		
	if (m_selection)
	{
		double lon = m_selection->Lon();
		double lat = m_selection->Lat();
		r->Rect(lon, lat, 0, 0, 4, m_selectionColor.Red(), m_selectionColor.Green(), m_selectionColor.Blue(), 100, true, NUMLAYERS);
	}

	if (m_selectedWay)
	{
		RenderWay(r, m_selectedWay, m_selectionColor, false, wxColour(0,0,0), 3, NUMLAYERS);
	}

	if (m_selectedRelation)
	{
		RenderRelation(r, m_selectedRelation, m_selectionColor, false, wxColour(0,0,0), 3, NUMLAYERS);
	}

	if (m_selectedTile)
	{
		for (TileWay *w = m_selectedTile->m_ways; w; w = (TileWay *)(w->m_next))
		{
			RenderWay(r, w->m_way, m_selectionColor, false, wxColour(0,0,0), 3, NUMLAYERS);

		}
		Rect(r, wxEmptyString, *m_selectedTile, -1, 255,55,55, 10, NUMLAYERS);
	}
}

//destroy the list when done. the TileSpans member will not be set
TileWay *TileDrawer::GetWaysContainingNode(OsmNode *node)
{

	int x = 0, y = 0;
	LonLatToIndex(node->Lon(), node->Lat(), &x, &y);
	

	return m_tileArray[x][y]->GetWaysContainingNode(node);
}

bool TileDrawer::SetSelectionColor(int r, int g, int b)
{
	wxColour newColor(r, g, b);

	bool changed = newColor != m_selectionColor;

	m_selectionColor = newColor;

	if (changed)
	{
		return true;
	}

	return false;
}
