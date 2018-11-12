#ifndef __WX_DOCK_ART_H__
#define __WX_DOCK_ART_H__

#include <wx/window.h>
#include <wx/aui/framemanager.h>
#include <wx/aui/dockart.h>
#include <wx/aui/auibook.h>

class wxMyDockArt: public wxAuiDefaultDockArt
{
public:
    wxMyDockArt();
    virtual void  DrawCaption (wxDC &dc, wxWindow *window, const wxString &text, const wxRect &rect, wxAuiPaneInfo &pane);
};

// copy from the default wxAuiSimpleTabArt
class wxMyTabArt : public wxAuiTabArt
{

public:

    wxMyTabArt();
    virtual ~wxMyTabArt();

    wxAuiTabArt* Clone() wxOVERRIDE;
    void SetFlags(unsigned int flags) wxOVERRIDE;

    void SetSizingInfo(const wxSize& tabCtrlSize,
                       size_t tabCount) wxOVERRIDE;

    void SetNormalFont(const wxFont& font) wxOVERRIDE;
    void SetSelectedFont(const wxFont& font) wxOVERRIDE;
    void SetMeasuringFont(const wxFont& font) wxOVERRIDE;
    void SetColour(const wxColour& colour) wxOVERRIDE;
    void SetActiveColour(const wxColour& colour) wxOVERRIDE;

    void DrawBorder(
                 wxDC& dc,
                 wxWindow* wnd,
                 const wxRect& rect) wxOVERRIDE;

    void DrawBackground(
                 wxDC& dc,
                 wxWindow* wnd,
                 const wxRect& rect) wxOVERRIDE;

    virtual void DrawTab(wxDC& dc,
                 wxWindow* wnd,
                 const wxAuiNotebookPage& pane,
                 const wxRect& inRect,
                 int closeButtonState,
                 wxRect* outTabRect,
                 wxRect* outButtonRect,
                 int* xExtent);

    void DrawButton(
                 wxDC& dc,
                 wxWindow* wnd,
                 const wxRect& inRect,
                 int bitmapId,
                 int buttonState,
                 int orientation,
                 wxRect* outRect) wxOVERRIDE;

    int GetIndentSize() wxOVERRIDE;

    int GetBorderWidth(
                 wxWindow* wnd) wxOVERRIDE;

    int GetAdditionalBorderSpace(
                 wxWindow* wnd) wxOVERRIDE;

    wxSize GetTabSize(
                 wxDC& dc,
                 wxWindow* wnd,
                 const wxString& caption,
                 const wxBitmap& bitmap,
                 bool active,
                 int closeButtonState,
                 int* xExtent) wxOVERRIDE;

    int ShowDropDown(
                 wxWindow* wnd,
                 const wxAuiNotebookPageArray& items,
                 int activeIdx) wxOVERRIDE;

    int GetBestTabCtrlSize(wxWindow* wnd,
                 const wxAuiNotebookPageArray& pages,
                 const wxSize& requiredBmpSize) wxOVERRIDE;

protected:

    wxFont m_normalFont;
    wxFont m_selectedFont;
    wxFont m_measuringFont;
    wxPen m_normalBkPen;
    wxPen m_selectedBkPen;
    wxBrush m_normalBkBrush;
    wxBrush m_selectedBkBrush;
    wxBrush m_bkBrush;
    wxBitmap m_activeCloseBmp;
    wxBitmap m_disabledCloseBmp;
    wxBitmap m_activeLeftBmp;
    wxBitmap m_disabledLeftBmp;
    wxBitmap m_activeRightBmp;
    wxBitmap m_disabledRightBmp;
    wxBitmap m_activeWindowListBmp;
    wxBitmap m_disabledWindowListBmp;

    int m_fixedTabWidth;
    unsigned int m_flags;
};
#endif

