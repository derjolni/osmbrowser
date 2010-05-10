#include "tiledrawer.h"
#include "rulecontrol.h"

TileWay::TileWay(OsmWay *way, TileSpans *allTiles, TileWay *next)
	: ListObject(next)
{
	m_way = way;
	m_tiles = allTiles;
	if (m_tiles)
		m_tiles->Ref();
}

TileWay::~TileWay()
{
	m_tiles->UnRef();
}

TileDrawer::TileDrawer(Renderer *renderer, double minLon,double minLat, double maxLon, double maxLat, double dLon, double dLat)
{
	m_renderer = renderer;
	m_tiles = NULL;

	m_selection = NULL;

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
	m_visibleTiles = NULL;
	unsigned id = 0;
	// build a list of empty tiles;
	m_tileArray = new OsmTile **[m_xNum];
	for (unsigned x = 0; x < m_xNum; x++)
	{
		m_tileArray[x] = new OsmTile *[m_yNum];
		for (unsigned y = 0; y < m_yNum; y++)
		{
			m_tiles = new OsmTile(id++, m_minLon + x * dLon , m_minLat + y * dLat, m_minLon + (x + 1) * dLon, m_minLat + (y+1) * dLat, m_tiles);
			m_tileArray[x][y] = m_tiles;
		}
	}
 }


bool TileDrawer::RenderTiles(wxApp *app, OsmCanvas *canvas, double lon, double lat, double w, double h, bool restart)
{
	bool mustCancel = false;

	DRect bb(lon, lat, w, h);

//	canvas->Rect(wxEmptyString, bb, 0, 255,0,0);

	if (restart || !m_visibleTiles)
	{
		if (m_visibleTiles)
			m_visibleTiles->UnRef();


		m_renderedTiles.MakeEmpty();
		m_visibleTiles = GetTiles(bb);

		m_curTile = m_visibleTiles;
		m_curLayer = 0;
	}

	if (!m_visibleTiles)
	{
		return false;
	}

	while (!mustCancel && m_curLayer < NUMLAYERS)
	{
		while (m_curTile && !mustCancel)
		{
			OsmTile *t = m_curTile->m_tile;
//			canvas->Rect(wxEmptyString, *t, -1, 0,255,0);
			if (t->OverLaps(bb))
			{
				for (TileWay *w = t->m_ways; w && !mustCancel; w = static_cast<TileWay *>(w->m_next))
				{
					if (!w->m_tiles->InterSect(&m_renderedTiles))
					{
						RenderWay(w->m_way, m_curLayer);
					}
				}	// for way
			}  // if overlaps
			mustCancel = app->Pending();

			m_renderedTiles.Add(t);
			m_curTile = static_cast<TileList *>(m_curTile->m_next);
		}	 // while curTile
		
		if (!m_curTile)
		{
			m_curLayer++;
			m_curTile = m_visibleTiles;
			m_renderedTiles.MakeEmpty();
		}
	}  // while curLayer

	return !mustCancel;
}

void TileDrawer::Rect(wxString const &text, double lon1, double lat1, double lon2, double lat2, double border, int r, int g, int b)
{
	m_renderer->SetLineColor(r,g,b);
	m_renderer->Rect(lon1, lat1, lon2 - lon1, lat2 - lat1, border , r, g, b);
	m_renderer->DrawCenteredText(text.mb_str(wxConvUTF8), (lon1 + lon2)/2, (lat1 + lat2)/2, 0, r, g, b);
}

// render using default colours. should plug in rule engine here
void TileDrawer::RenderWay(OsmWay *w, int curLayer)
{

	if ((!m_drawRule) || m_drawRule->Evaluate(w))
	{
		wxColour c = wxColour(150,150,150);
		bool poly = false;
		int layer = 1;
		if (m_colorRules)
		{
			for (int i = 0; i < m_colorRules->m_num; i++)
			{
				if (m_colorRules->m_rules[i]->Evaluate(w))
				{
					c = m_colorRules->m_pickers[i]->GetColour();
					poly = m_colorRules->m_checkBoxes[i]->IsChecked();
					layer = m_colorRules->m_layers[i]->GetSelection();
					break; // stop after first match
				}
			}
		}
		if (curLayer == layer)
		{
			RenderWay(w, c, poly, c);
		}
	}
}


void TileDrawer::RenderWay(OsmWay *w, wxColour lineColour, bool poly, wxColour fillColour)
{


	m_renderer->SetLineColor(lineColour.Red(), lineColour.Green(), lineColour.Blue());
	m_renderer->SetFillColor(fillColour.Red(), fillColour.Green(), fillColour.Blue());

	if (!poly)
	{
		m_renderer->Begin(Renderer::R_LINE);
		for (unsigned j = 0; j < w->m_numResolvedNodes; j++)
		{
			OsmNode *node = w->m_resolvedNodes[j];
		
			if (node)
			{
				m_renderer->AddPoint(node->m_lon, node->m_lat);
			}
			else
			{
				m_renderer->End();
				m_renderer->Begin(Renderer::R_LINE);
			}
		
		}
		m_renderer->End();
	}
	else
	{
		m_renderer->Begin(Renderer::R_POLYGON);
		for (unsigned j = 0; j < w->m_numResolvedNodes; j++)
		{
			OsmNode *node = w->m_resolvedNodes[j];
		
			if (node)
			{
				m_renderer->AddPoint(node->m_lon, node->m_lat);
			}
			
		}
		m_renderer->End();
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

void TileDrawer::LatLonToIndex(double lon, double lat, int *x, int *y)
{
	if (x)
	{
		*x = static_cast<int>((lon - m_minLon) / m_dLon);
		if (*x < 0) *x = 0;
		if (*x > static_cast<int>(m_xNum - 1)) *x = static_cast<int>(m_xNum - 1);
	}

	if (y)
	{
		*y = static_cast<int>((lat - m_minLat) / m_dLat);
	if (*y < 0) *y = 0;
	if (*y > static_cast<int>(m_yNum - 1)) *y = static_cast<int>(m_yNum - 1);
	}
}


TileList *TileDrawer::GetTiles(double minLon, double minLat, double maxLon, double maxLat)
{
//            printf("gettiles (%g %g)-(%g-%g):\n", minLon, minLat, maxLon, maxLat);
	int xMin = 0, xMax = 0, yMin = 0, yMax = 0;

	LatLonToIndex(minLon, minLat, &xMin, &yMin);
	LatLonToIndex(maxLon, maxLat, &xMax, &yMax);


	TileList *ret = NULL;


	for (int x = xMin; x <= xMax; x++)
	{
		for (int y = yMin; y <= yMax; y++)
		{
			ret = new TileList(m_tileArray[x][y], ret);
		}
	}

	
	if (ret)
		ret->Ref();

	return ret;

	
}

OsmNode *TileDrawer::GetClosestNodeInTile(int x, int y, double lon, double lat, double *foundDistSq)
{
	double fDSq;
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


OsmNode *TileDrawer::GetClosestNode(double lat, double lon)
{
	int x =0, y = 0;
	double distSq = -1;

	LatLonToIndex(lat, lon, &x, &y);

	OsmNode *found = GetClosestNodeInTile(x, y, lon, lat, &distSq);

	return found;
}