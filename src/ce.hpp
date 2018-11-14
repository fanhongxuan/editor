#ifndef __WX_CE_HPP__
#define __WX_CE_HPP__

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/aui/framemanager.h>
// -- application --
class MyFrame;
class MyApp : public wxApp
{
private:
    MyFrame *mpFrame;
public:
    MyApp():mpFrame(NULL){}
    MyFrame &frame(){return *mpFrame;}
    bool OnInit() wxOVERRIDE;
    int FilterEvent(wxEvent& evt);
};

wxDECLARE_APP(MyApp);

class MySearchHandler;
class wxSearchResult;
class Edit;
class wxStyledTextEvent;
class wxSearchFile;
class wxSearchDir;
class wxBufferSelect;
class wxExplorer;
class wxAuiDockArt;
class wxAuiNotebookEvent;
class wxAuiManagerEvent;
class wxAuiNotebook;
class MyFrame : public wxFrame
{
    enum
    {
        ID_CreatePerspective = wxID_HIGHEST+1,
        // add begin by fanhongxuan@gmail.com
        ID_ShowSearch,
        ID_ShowFindFiles,
        ID_KillCurrentBuffer,
        ID_ShowOneWindow,
        ID_ShowBufferSelect,
        ID_SaveCurrentBuffer,
        ID_TriggerComment,
        ID_ShowExplorer,
        // add end by fanhongxuan@gmail.com
        ID_FirstPerspective = ID_CreatePerspective+1000
    };

public:
    MyFrame(wxWindow* parent,
            wxWindowID id,
            const wxString& title,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_FRAME_STYLE | wxSUNKEN_BORDER);

    ~MyFrame();

    wxAuiDockArt* GetDockArt();
    void DoUpdate();
    void PrepareResults(MySearchHandler &handler, const wxString &input, std::vector<wxSearchResult*> &results);
    void ChangeToBuffer(Edit *pEdit, int pos);
    void OpenFile(const wxString &name, const wxString &path, bool bActive);
    void QueueFocusEvent();
    
    void OnShowSearch(wxCommandEvent &evt);
    void OnShowFindFiles(wxCommandEvent &evt);
    void OnKillCurrentBuffer(wxCommandEvent &evt);
    void OnShowOneWindow(wxCommandEvent &evt);
    void OnShowBufferSelect(wxCommandEvent &evt);
    void OnShowExplorer(wxCommandEvent &evt);
    void OnSaveCurrentBuffer(wxCommandEvent &evt);
    void OnFileClose(wxAuiNotebookEvent &evt);
    void OnFileClosed(wxAuiNotebookEvent &evt);
    void OnPaneClose(wxAuiManagerEvent &evt);
    void OnClose(wxCloseEvent &evt);
    void OnFileSaved(wxStyledTextEvent &evt);
    void OnFileModified(wxStyledTextEvent &evt);
    void OnFocus(wxFocusEvent &evt);
private:
    void CreateAcceTable();
    void SwitchFocus();
    void SaveInfo();
    void LoadInfo();
private:
    bool mbLoadFinish;
    wxAuiNotebook *mpBufferList;
    wxSearchFile *mpSearch;
    wxSearchDir *mpSearchDir;
    wxBufferSelect *mpBufferSelect;
    wxExplorer *mpExplorer;
    wxAuiManager m_mgr;
    wxArrayString m_perspectives;
    wxMenu* m_perspectives_menu;
    wxTextCtrl *mpCmd;
    wxDECLARE_EVENT_TABLE();
};

#endif
