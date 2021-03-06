// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __OSMCANVAS_H__
#define __OSMCANVAS_H__

#include <wx/timer.h>
#include <wx/app.h>
#include "wxcanvas.h"
#include "osm.h"
#include "renderer.h"
#include "cairorenderer.h"
#include "tiledrawer.h"

class RuleControl;
class ColorRules;
class InfoTreeCtrl;
class MainFrame;

class CanvasJob
	: public RenderJob
{
	public:
		CanvasJob(wxApp *app, MainFrame *mainFrame, Renderer *r);

		bool MustCancel(double progress);

	private:
		wxApp *m_app;
		MainFrame *m_mainFrame;
};

class OsmCanvas
	: public Canvas
{
	public:
		OsmCanvas(wxApp *app, MainFrame *mainFrame, wxWindow *parent, wxString const &fileName, int numLayers);
		void Render(bool force = false);

		~OsmCanvas();

		void Lock()
		{
			m_locked = true;
		}

		void Unlock(bool refresh = true)
		{
			m_locked = false;
			if (refresh)
			{
				Redraw();
			}
		}

		void Redraw()
		{
			m_restart = true;
		}

		void SetRuleControls(RuleControl *rules, ColorRules *colors);

		void SetInfoDisplay(InfoTreeCtrl *info);

		void SaveView(wxString const &fileName, MainFrame *mainFrame);

		void SelectWay(OsmWay *way);
		void SelectRelation(OsmRelation *rel);
	private:
		CanvasJob *m_renderJob;
		void SetupRenderer();
		OsmData *m_data;
		InfoTreeCtrl *m_info;
		DECLARE_EVENT_TABLE();

		void OnMouseWheel(wxMouseEvent &evt);
		void OnLeftDown(wxMouseEvent &evt);
		void OnLeftUp(wxMouseEvent &evt);
		void OnMouseMove(wxMouseEvent &evt);
		void OnIdle(wxIdleEvent &evt)
		{
			if (m_busy)
			{
				return;
			}
			if (m_restart || !m_done)
			{
				Render();
			}

			if (m_done)
			{
				Draw(NULL);
			}
			else
			{
				wxWakeUpIdle();
			}
		}

		double m_scale;
		double m_xOffset, m_yOffset;

		int m_lastX, m_lastY;
		bool m_dragging;

		TileDrawer *m_tileDrawer;

		wxApp *m_app;
		MainFrame *m_mainFrame;
		bool m_done;
		bool m_restart;
		bool m_locked;
		bool m_busy;

		Renderer *m_renderer;

		bool m_cursorLocked;
		bool m_firstDragStep;
};


#endif
