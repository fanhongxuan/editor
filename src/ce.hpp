#ifndef __WX_CE_HPP__
#define __WX_CE_HPP__

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/aui/framemanager.h>
#include <set>
// -- application --
class ceSymbol;
class MyFrame;
class MyServer;
class wxSingleInstanceChecker;
// #define USE_NOTEBOOK
#ifdef USE_NOTEBOOK
class Notebook;
#define ceNootBook Notebook
#else
#define ceNootBook wxAuiNotebook
#endif

class MyApp : public wxApp
{
private:
    MyFrame *mpFrame;
    MyServer *mpServer;
    wxSingleInstanceChecker *mpChecker;
public:
    MyApp():mpFrame(NULL){}
    MyFrame *frame(){return mpFrame;}
    virtual bool OnInit() wxOVERRIDE;
    virtual int OnExit() wxOVERRIDE;
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser) wxOVERRIDE;
    virtual void OnInitCmdLine(wxCmdLineParser &parser) wxOVERRIDE;
    virtual bool OnCmdLineError(wxCmdLineParser &parser) wxOVERRIDE;
};

wxDECLARE_APP(MyApp);

class MySearchHandler;
class MyAgSearchHandler;
class MySymbolSearchHandler;
class MyRefSearchHandler;
class wxSearchResult;
class wxSearchFileResult;
class ceEdit;
class wxStyledTextEvent;
class wxSearchFile;
class wxSearchDir;
class wxAgSearch;
class wxSymbolSearch;
class ceRefSearch;
class wxBufferSelect;
class wxExplorer;
class wxWorkSpace;
class ceCompile;
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
        ID_ShowExplorerOfCurrentFile,
        ID_ShowWorkSpace,
        ID_ShowAgSearch,
        ID_ShowSymbolList,
        ID_ShowReference,
        ID_GotoDefine,
        ID_ShowGrepText,
        ID_GoBack,
        ID_GoForward,
        ID_NextFile,
        ID_PrevFile,
        ID_Compile,
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
    bool FindDef(const wxString &symbol, std::vector<wxSearchFileResult *> &outputs);
    bool FindDef(std::set<ceSymbol*> &symbols, 
                 const wxString &name, 
                 const wxString &className = "",
                 const wxString &type = "",
                 const wxString &language = "",
                 const wxString &filename = "");
    bool GetSymbols(std::set<ceSymbol*> &symbols, 
                    const wxString &scope, 
                    const wxString &type = "",
                    const wxString &language = "", 
                    const wxString &filename = "");
    wxString GetDbRecordByKey(const wxString &key);
    
    void ShowStatus(const wxString &status, int index = 0);
    void SetActiveEdit(ceEdit *pEdit);
    void DoUpdate();
    void PrepareResults(MySearchHandler &handler, const wxString &input, std::vector<wxSearchResult*> &results);
    void ChangeToBuffer(ceEdit *pEdit, int pos);
    void OpenFile(const wxString &name, const wxString &path, bool bActive, int line = -1, bool updateGotoHistory = true);
    void AddDirToWorkSpace(const wxString &path);
    bool ShowMiniBuffer(const wxString &name, bool bHide = false);
    
    void OnGoBack(wxCommandEvent &evt);
    void OnGoForward(wxCommandEvent &evt);
    void OnGotoDefine(wxCommandEvent &evt);
    void OnShowReference(wxCommandEvent &evt);
    void OnShowGrepText(wxCommandEvent &evt);
    void OnShowSymbolList(wxCommandEvent &evt);
    void OnShowSearch(wxCommandEvent &evt);
    void OnShowFindFiles(wxCommandEvent &evt);
    void OnKillCurrentBuffer(wxCommandEvent &evt);
    void OnShowOneWindow(wxCommandEvent &evt);
    void OnShowBufferSelect(wxCommandEvent &evt);
    void OnShowExplorer(wxCommandEvent &evt);
    void OnShowExplorerOfCurrentFile(wxCommandEvent &evt);
    void OnShowWorkSpace(wxCommandEvent &evt);
    void OnShowAgSearch(wxCommandEvent &evt);
    void OnSaveCurrentBuffer(wxCommandEvent &evt);
    void OnNextFile(wxCommandEvent &evt);
    void OnPrevFile(wxCommandEvent &evt);
    void OnCompile(wxCommandEvent &evt);
    void OnFileClose(wxAuiNotebookEvent &evt);
    void OnFileClosed(wxAuiNotebookEvent &evt);
    void OnPaneClose(wxAuiManagerEvent &evt);
    void OnClose(wxCloseEvent &evt);
    void OnFileSaved(wxStyledTextEvent &evt);
    void OnFileModified(wxStyledTextEvent &evt);
    void OnFileUnModified(wxStyledTextEvent &evt);

private:
    void ShowReference(int type);
    void CreateAcceTable();
    void SwitchFocus();
    void SaveInfo();
    void LoadInfo();
    void UpdateWorkDirs(ceEdit *pActiveEdit = NULL, bool showWorkSpace = false, bool showExplorer = false);
private:
    std::vector<std::pair<wxString, int> > mGotoHistory;
    int mGotoIndex;
    bool mbLoadFinish;
    ceNootBook *mpBufferList;
    wxSearchFile *mpSearch;
    MySearchHandler *mpSearchHandler;
    MyAgSearchHandler *mpAgSearchHandler;
    MySymbolSearchHandler *mpSymbolSearchHandler;
    MyRefSearchHandler *mpRefSearchHandler;
    ceRefSearch *mpRefSearch;
    wxSearchDir *mpSearchDir;
    wxBufferSelect *mpBufferSelect;
    wxAgSearch *mpAgSearch;
    wxSymbolSearch *mpSymbolSearch;
    wxExplorer *mpExplorer;
    wxWorkSpace *mpWorkSpace;
    ceCompile *mpCompile;
    ceEdit *mpActiveEdit;
    wxAuiManager m_mgr;
    wxArrayString m_perspectives;
    wxMenu* m_perspectives_menu;
    wxTextCtrl *mpCmd;
    std::vector<wxString> mStatus;
    wxDECLARE_EVENT_TABLE();
};

#endif
