#include "osmcanvas.h"
#include "parse.h"

#include <math.h>

DECLARE_EVENT_TYPE(wxEVT_THREAD_DONE, -1)

DEFINE_EVENT_TYPE(wxEVT_THREAD_DONE)


BEGIN_EVENT_TABLE(OsmCanvas, Canvas)
        EVT_MOUSEWHEEL(OsmCanvas::OnMouseWheel)
        EVT_LEFT_DOWN(OsmCanvas::OnLeftDown)
//        EVT_MIDDLE_DOWN(OsmCanvas::OnMiddleDown)
//        EVT_RIGHT_DOWN(OsmCanvas::OnRightDown)
        EVT_LEFT_UP(OsmCanvas::OnLeftUp)
//        EVT_MIDDLE_UP(OsmCanvas::OnMiddleUp)
//        EVT_RIGHT_UP(OsmCanvas::OnRightUp)
        EVT_MOTION(OsmCanvas::OnMouseMove)
        EVT_COMMAND(-1, wxEVT_THREAD_DONE, OsmCanvas::OnRenderThreadDone)
END_EVENT_TABLE()


class RenderInfo
{
    public:
    RenderInfo(wxBitmap *onto,OsmData *data, double xOffset, double yOffset, double scale, OsmCanvas *owner)
    {
        m_bitmap = onto;
        m_data = data;
        m_xOffset = xOffset;
        m_yOffset = yOffset;
        m_scale = scale;
        m_owner = owner;
    }

    virtual void Render();

    wxBitmap *m_bitmap;
    OsmCanvas *m_owner;
    OsmData *m_data;

    double m_xOffset, m_yOffset, m_scale;
    
};

class RenderThread
    : public wxThread
{
    public:
    RenderThread(RenderInfo *info)
    {
        m_info = info;
    }

    ~RenderThread()
    {
        delete m_info;
    }

    ExitCode Entry()
    {

        m_info->Render();
        wxCommandEvent event( wxEVT_THREAD_DONE, GetId() );

        m_info->m_owner->AddPendingEvent(event);

        return 0;
    }



    RenderInfo *m_info;
    
};


OsmCanvas::OsmCanvas(wxWindow *parent, wxString const &fileName)
    : Canvas(parent)
{

    m_dragging = false;
    wxString binFile = fileName;
    binFile.Append(wxT(".cache"));

    FILE *infile = fopen(binFile.mb_str(wxConvUTF8), "r");

    if (infile)
    {
        printf("found preprocessed file %s, opening that instead.\n", (char const *)(binFile.mb_str(wxConvUTF8)) );
        m_data = parse_binary(infile);
    }
    else
    {
        infile = fopen(fileName.mb_str(wxConvUTF8), "r");
    
        if (!infile)
        {
            puts("could not open file:");
            puts(fileName.mb_str(wxConvUTF8));
            abort();
        }
    
        if (fileName.EndsWith(wxT(".cache")))
        {
            m_data = parse_binary(infile);
        }
        else
        {
            m_data = parse_osm(infile);
    
            FILE *outFile = fopen(binFile.mb_str(wxConvUTF8) , "wb");
    
            if (outFile)
            {
            
                printf("writing cache\n");
                write_binary(m_data, outFile);
                fclose(outFile);
            }
        }
    }
    fclose(infile);

    double xscale = 800.0 / (m_data->m_maxlon - m_data->m_minlon);
    double yscale = 800.0 / (m_data->m_maxlon - m_data->m_minlon);
    m_scale = xscale < yscale ? xscale : yscale;
    m_xOffset = m_data->m_minlon;
    m_yOffset = m_data->m_minlat;

    m_lastX = m_lastY = 0;


    m_threadRuns = false;
    m_threadMustRestart = false;
    m_threadMustStop = false;
    
}

void OsmCanvas::OnRenderThreadDone(wxCommandEvent &evt)
{
    m_threadRuns = false;
    m_threadMustStop = false;
    Draw(NULL);

    printf("thread donw\n");


    if (m_threadMustRestart)
    {
        m_threadMustRestart= false;
        Render();
    }
}


void OsmCanvas::Render()
{
    if (m_threadRuns)
    {
        m_threadMustStop = true;
        m_threadMustRestart = true;
        return;
    }

    printf("start thread\n");

    m_threadRuns = true;

    RenderThread *t = new RenderThread(new RenderInfo(&m_backBuffer, m_data, m_xOffset, m_yOffset, m_scale, this));

    t->Create();
    t->Run();

}


void RenderInfo::Render()
{
    int w = m_bitmap->GetWidth();
    int h = m_bitmap->GetHeight();

    wxMemoryDC dc;
    dc.SelectObject(*(m_bitmap));
    dc.SetPen(*wxBLACK_PEN);
    dc.Clear();

    for (OsmWay *w = static_cast<OsmWay *>(m_data->m_ways.m_content); w ; w = static_cast<OsmWay *>(w->m_next))
    {
        for (unsigned j = 0; j < w->m_numResolvedNodes - 1; j++)
        {
            OsmNode *node1 = w->m_resolvedNodes[j];
            OsmNode *node2 = w->m_resolvedNodes[j+1];
            if (node1 && node2)
            {
                int x1 = (node1->m_lon - m_xOffset) * m_scale;
                int y1 = (node1->m_lat - m_yOffset) * m_scale;
                int x2 = (node2->m_lon - m_xOffset) * m_scale;
                int y2 = (node2->m_lat - m_yOffset) * m_scale;
                y1 = h - y1;
                y2 = h - y2;

//                printf("drawing %g %g %g %g s%g %d %d\n",node1->m_lon, node1->m_lat, m_xOffset, m_yOffset, m_scale, x1,y1);
                dc.DrawLine(x1,y1, x2,y2);
            }
            
        }
    }
    
}

OsmCanvas::~OsmCanvas()
{
    delete m_data;
}

void OsmCanvas::OnMouseWheel(wxMouseEvent &evt)
{
    double w = evt.GetWheelRotation() / 1200.0;
    int h = m_backBuffer.GetHeight();

    double xm = evt.m_x / m_scale;
    double ym = (h - evt.m_y) / m_scale;

    m_xOffset += xm;
    m_yOffset += ym;

    m_scale = m_scale * (1.0 + w);

    xm = evt.m_x / m_scale;
    ym = (h - evt.m_y) / m_scale;
    m_xOffset -= xm;
    m_yOffset -= ym;


    Render();
    Draw();
}

void OsmCanvas::OnMouseMove(wxMouseEvent &evt)
{
    if (m_dragging)
    {
        int idx = evt.m_x - m_lastX;
        int idy = evt.m_y - m_lastY;


        m_lastX = evt.m_x;
        m_lastY = evt.m_y;
        double dx = idx / m_scale;
        double dy = idy / m_scale;

        m_xOffset -= dx;
        m_yOffset += dy;


        Render();
        
    }
}


void OsmCanvas::OnLeftDown(wxMouseEvent &evt)
{
    if (!HasCapture())
        CaptureMouse();

    m_lastX = evt.m_x;
    m_lastY = evt.m_y;

    m_dragging = true;
}


void OsmCanvas::OnLeftUp(wxMouseEvent &evt)
{
    if (HasCapture())
        ReleaseMouse();
    m_dragging = false;

}

