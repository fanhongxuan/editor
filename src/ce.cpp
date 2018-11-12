///////////////////////////////////////////////////////////////////////////////
// Name:        auidemo.cpp
// Purpose:     wxaui: wx advanced user interface - sample/test program
// Author:      Benjamin I. Williams
// Modified by:
// Created:     2005-10-03
// Copyright:   (C) Copyright 2005, Kirix Corporation, All Rights Reserved.
// Licence:     wxWindows Library Licence, Version 3.1
///////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/app.h"
#include "wx/grid.h"
#include "wx/treectrl.h"
#include "wx/spinctrl.h"
#include "wx/artprov.h"
#include "wx/clipbrd.h"
#include "wx/image.h"
#include "wx/colordlg.h"
#include "wx/wxhtml.h"
#include "wx/imaglist.h"
#include "wx/dataobj.h"
#include "wx/dcclient.h"
#include "wx/bmpbuttn.h"
#include "wx/menu.h"
#include "wx/toolbar.h"
#include "wx/statusbr.h"
#include "wx/msgdlg.h"
#include "wx/textdlg.h"

#include <wx/textctrl.h>

#include "wx/aui/aui.h"
#include "res/sample.xpm"

#include <wx/wxcrtvararg.h> // for wxPrintf
#include "wxSearch.hpp"
#include "wxEdit.hpp"
#include "wxBufferSelect.hpp"
#include "wxDockArt.hpp"
// -- application --
class MyFrame;
class MyApp : public wxApp
{
private:
    MyFrame *mpFrame;
public:
    MyApp():mpFrame(NULL){}
    bool OnInit() wxOVERRIDE;
    int FilterEvent(wxEvent& evt);
};

wxDECLARE_APP(MyApp);
wxIMPLEMENT_APP(MyApp);

class MySearchHandler;
// -- frame --

class MyFrame : public wxFrame
{
    enum
    {
        ID_CreateTree = wxID_HIGHEST+1,
        // add begin by fanhongxuan@gmail.com
        ID_ShowSearch,
        ID_ShowFindFiles,
        ID_KillCurrentBuffer,
        ID_ShowOneWindow,
        ID_ShowBufferSelect,
        ID_SaveCurrentBuffer,
        // add end by fanhongxuan@gmail.com
        ID_CreateGrid,
        ID_CreateText,
        ID_CreateHTML,
        ID_CreateNotebook,
        ID_CreateSizeReport,
        ID_GridContent,
        ID_TextContent,
        ID_TreeContent,
        ID_HTMLContent,
        ID_NotebookContent,
        ID_SizeReportContent,
        ID_CreatePerspective,
        ID_CopyPerspectiveCode,
        ID_AllowFloating,
        ID_AllowActivePane,
        ID_TransparentHint,
        ID_VenetianBlindsHint,
        ID_RectangleHint,
        ID_NoHint,
        ID_HintFade,
        ID_NoVenetianFade,
        ID_TransparentDrag,
        ID_NoGradient,
        ID_VerticalGradient,
        ID_HorizontalGradient,
        ID_LiveUpdate,
        ID_AllowToolbarResizing,
        ID_Settings,
        ID_CustomizeToolbar,
        ID_DropDownToolbarItem,
        ID_NotebookNoCloseButton,
        ID_NotebookCloseButton,
        ID_NotebookCloseButtonAll,
        ID_NotebookCloseButtonActive,
        ID_NotebookAllowTabMove,
        ID_NotebookAllowTabExternalMove,
        ID_NotebookAllowTabSplit,
        ID_NotebookWindowList,
        ID_NotebookScrollButtons,
        ID_NotebookTabFixedWidth,
        ID_NotebookArtGloss,
        ID_NotebookArtSimple,
        ID_NotebookAlignTop,
        ID_NotebookAlignBottom,

        ID_SampleItem,

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
    
private:
    wxAuiNotebook *mpBufferList;
    wxSearchFile *mpSearch;
    wxSearchDir *mpSearchDir;
    wxBufferSelect *mpBufferSelect;
    wxAuiManager m_mgr;
    wxArrayString m_perspectives;
    wxMenu* m_perspectives_menu;
    wxTextCtrl *mpCmd;
    wxDECLARE_EVENT_TABLE();
};

bool MyApp::OnInit()
{
    mpFrame = NULL;
    if ( !wxApp::OnInit() )
        return false;
    
    mpFrame = new MyFrame(NULL,
                          wxID_ANY,
                          wxT("CodeBrowser"),
                          wxDefaultPosition,
                          wxSize(800, 600));
    mpFrame->Show();
    return true;
}

void MyFrame::QueueFocusEvent()
{
    QueueEvent(new wxFocusEvent(wxEVT_KILL_FOCUS));
}

int MyApp::FilterEvent(wxEvent& evt){
    // if (evt.GetEventType() == wxEVT_SET_FOCUS && NULL != mpFrame && mpFrame != dynamic_cast<MyFrame*>(evt.GetEventObject())){
    if (NULL != mpFrame && evt.GetEventType() == wxEVT_SET_FOCUS){
        mpFrame->QueueFocusEvent();
    }
    return -1;
}

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_SET_FOCUS(MyFrame::OnFocus)
EVT_KILL_FOCUS(MyFrame::OnFocus)
EVT_STC_SAVEPOINTREACHED(wxID_ANY, MyFrame::OnFileSaved)
EVT_STC_SAVEPOINTLEFT(wxID_ANY, MyFrame::OnFileModified)
EVT_CLOSE(MyFrame::OnClose)
EVT_AUI_PANE_CLOSE(MyFrame::OnPaneClose)
EVT_MENU(ID_ShowSearch, MyFrame::OnShowSearch)
EVT_MENU(ID_ShowFindFiles, MyFrame::OnShowFindFiles)
EVT_MENU(ID_KillCurrentBuffer, MyFrame::OnKillCurrentBuffer)
EVT_MENU(ID_ShowOneWindow, MyFrame::OnShowOneWindow)
EVT_MENU(ID_ShowBufferSelect, MyFrame::OnShowBufferSelect)
EVT_MENU(ID_SaveCurrentBuffer, MyFrame::OnSaveCurrentBuffer)
EVT_AUINOTEBOOK_PAGE_CLOSE(wxID_ANY, MyFrame::OnFileClose)
EVT_AUINOTEBOOK_PAGE_CLOSED(wxID_ANY, MyFrame::OnFileClosed)
wxEND_EVENT_TABLE()

class MyDirSearchHandler: public wxSearchHandler
{
private:
    MyFrame *mpMain;
public:
    MyDirSearchHandler(MyFrame *pMain){
        mpMain = pMain;
    }
    void OnSelect(wxSearchResult &ret, bool bActive){
        if (NULL != mpMain){
            mpMain->OpenFile(ret.Content(), ret.Target(), bActive);
        }
    }
};

MyFrame::MyFrame(wxWindow* parent,
                 wxWindowID id,
                 const wxString& title,
                 const wxPoint& pos,
                 const wxSize& size,
                 long style)
        : wxFrame(parent, id, title, pos, size, style)
{
    mpSearchDir = NULL;
    mpSearch = NULL;
    mpBufferList = NULL;
    mpBufferSelect = NULL;
    mpCmd = NULL;
    // tell wxAuiManager to manage this frame
    m_mgr.SetArtProvider(new wxMyDockArt);
    m_mgr.SetManagedWindow(this);

    // set frame icon
    SetIcon(wxIcon(sample_xpm));

    CreateAcceTable();
    
    // // create menu
    // wxMenuBar* mb = new wxMenuBar;

    // wxMenu* file_menu = new wxMenu;
    // file_menu->Append(wxID_EXIT);
    
    // m_perspectives_menu = new wxMenu;
    // m_perspectives_menu->Append(ID_CreatePerspective, _("Create Perspective"));
    // m_perspectives_menu->Append(ID_CopyPerspectiveCode, _("Copy Perspective Data To Clipboard"));
    // m_perspectives_menu->AppendSeparator();
    // m_perspectives_menu->Append(ID_FirstPerspective+0, _("Default Startup"));
    // m_perspectives_menu->Append(ID_FirstPerspective+1, _("All Panes"));

    
    // min size for the frame itself isn't completely done.
    // see the end up wxAuiManager::Update() for the test
    // code. For now, just hard code a frame minimum size
    SetMinSize(wxSize(400,300));

    // add by fanhongxuan@gmail.com
	mpBufferList = new wxAuiNotebook(this, wxID_ANY,
                                     wxDefaultPosition,
                                     wxDefaultSize,
                                     wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_WINDOWLIST_BUTTON);
    // mpBufferList->SetArtProvider(new wxAuiSimpleTabArt);
    mpBufferList->SetArtProvider(new wxMyTabArt);
    m_mgr.AddPane(mpBufferList, wxAuiPaneInfo().Name(wxT("Main")).Caption(wxT("Main")).CenterPane());

    mpCmd = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(9999, 25));
    m_mgr.AddPane(mpCmd, wxAuiPaneInfo().Name(wxT("Cmd")).Caption(wxT("Command"))
                  .Bottom().CloseButton(false).Resizable(false).Fixed().Floatable(false)
                  .CaptionVisible(false).Row(0).Layer(0).Position(0));
    // "commit" all changes made to wxAuiManager
    m_mgr.Update();
}

MyFrame::~MyFrame()
{
    m_mgr.UnInit();
}

void MyFrame::CreateAcceTable()
{
#define ACCE_COUNT  6  
    wxAcceleratorEntry e[ACCE_COUNT];
    e[0].Set(wxACCEL_CTRL, (int)'F', ID_ShowSearch); // CTRL+F (Find in current file)
    e[1].Set(wxACCEL_CTRL, (int)'O', ID_ShowFindFiles); // CTRL+O (find and Open of file)
    e[2].Set(wxACCEL_CTRL, (int)'K', ID_KillCurrentBuffer); // CTRL+K (Hide Current Pane, If in buffer list, mean kill that buffer)
    e[3].Set(wxACCEL_CTRL, (int)'1', ID_ShowOneWindow); // CTRL+1 (Hide all the pane except BufferList)
    e[4].Set(wxACCEL_CTRL, (int)'B', ID_ShowBufferSelect); // CTRL+B (SwitchBuffer)
    e[5].Set(wxACCEL_CTRL, (int)'S', ID_SaveCurrentBuffer); // CTRL+S (Save Current Buffer)
    wxAcceleratorTable acce(ACCE_COUNT, e);
    SetAcceleratorTable(acce);
}

wxAuiDockArt* MyFrame::GetDockArt()
{
    return m_mgr.GetArtProvider();
}

void MyFrame::DoUpdate()
{
    m_mgr.Update();
}

class MySearchHandler: public wxSearchHandler
{
private:
    Edit *mpEdit;
    MyFrame *mpFrame;
public:
    MySearchHandler(MyFrame *frame){mpFrame = frame;}
    void SetEdit(Edit *pEdit){mpEdit = pEdit;}
    void OnSelect(wxSearchResult &ret, bool bActive){
        wxSearchFileResult *pRet = dynamic_cast<wxSearchFileResult*>(&ret);
        if (NULL != pRet){
            int line = pRet->GetLine();
            mpFrame->ChangeToBuffer(mpEdit, line);
            // mpEdit->ScrollToLine(line);
            mpEdit->GotoLine(line);
            int lineStart = mpEdit->PositionFromLine (line);
            int lineEnd = mpEdit->PositionFromLine (line + 1);
            if (bActive){
                mpEdit->SetSelection(pRet->GetPos(), pRet->GetPos());
                mpEdit->SetInsertionPoint(pRet->GetPos());
                mpEdit->SetFocus();
            }
            else{
                // todo:fanhongxuan@gmail.com
                // show the searched char as red
                mpEdit->SetSelection(lineStart, lineEnd);
            }
        }
    }
    
    void OnPrepareResult(const wxString &input, std::vector<wxSearchResult*> &results){
        if (NULL != mpFrame){
            mpFrame->PrepareResults(*this, input, results);
        }
    }
};

void MyFrame::OpenFile(const wxString &name, const wxString &path, bool bActive)
{
    if (NULL == mpBufferList){
        return;
    }
    int i = 0;
    Edit *pEdit = NULL;
    for (i = 0; i < mpBufferList->GetPageCount(); i++){
        pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(i));
        if (NULL != pEdit && pEdit->GetFilename() == path){
            mpBufferList->SetSelection(i);
            break;
        }
    }
    if (i == mpBufferList->GetPageCount()){
        pEdit = new Edit(mpBufferList);
        pEdit->LoadFile(path);
        pEdit->SelectNone();
        mpBufferList->AddPage(pEdit, name, true);
        if (NULL != mpBufferSelect){
            mpBufferSelect->AddBuffer(name, path);
        }
    }
    if (bActive && NULL != pEdit){
        pEdit->SetFocus();
    }
}

void MyFrame::ChangeToBuffer(Edit *pEdit, int pos)
{
    if (NULL == pEdit){
        return;
    }
    int i = 0;
    for (i = 0; i < mpBufferList->GetPageCount(); i++){
        if (pEdit == mpBufferList->GetPage(i)){
            mpBufferList->SetSelection(i);
            break;
        }
    }
    //pEdit->SetSelection(pos, pos);
    //pEdit->SetInsertionPoint(pos);
}

void MyFrame::PrepareResults(MySearchHandler &handler, const wxString &input, std::vector<wxSearchResult*> &results)
{
    // get the current selection
    if (NULL == mpBufferList || NULL == mpSearch){
        return;
    }
    int select = mpBufferList->GetSelection();
    if (wxNOT_FOUND == select){
        return;
    }
    Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(select));
    if (NULL == pEdit){
        return;
    }
    handler.SetEdit(pEdit);
    mpSearch->SetFileName(pEdit->GetFilename());
    mpSearch->SetBuffer(pEdit->GetText());
}

void MyFrame::OnClose(wxCloseEvent &evt)
{
    // todo:fanhongxuan@gmail.com
    // store all the information and restore later.

    if (NULL != mpBufferList){
        int i = 0;
        for (i = 0; i < mpBufferList->GetPageCount(); i++){
            Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(i));
            if (NULL != pEdit){
                if (pEdit->Modified()){
                    int answer = wxMessageBox(wxString::Format("%s has been modified, save it?", mpBufferList->GetPageText(i)),
                                              wxT("Confirm Save"), wxYES_NO | wxCANCEL, this);
                    if (answer == wxYES){
                        pEdit->SaveFile(true);
                    }
                    else if (answer == wxCANCEL){
                        if (evt.CanVeto()){
                            evt.Veto();
                            return;
                        }
                    }
                }
            }
        }
    }
    evt.Skip();
}

void MyFrame::OnPaneClose(wxAuiManagerEvent &evt)
{
    // todo:fanhongxuan@gmail.com
    // when a pane is closed by close button, need to update the focus.
    // m_mgr.Update();
    // SwitchFocus();
}

void MyFrame::OnFileClosed(wxAuiNotebookEvent &evt)
{
    SwitchFocus();
}

void MyFrame::OnFocus(wxFocusEvent &evt)
{
    // wxPrintf("OnFocus\n");
    m_mgr.Update();
}

void MyFrame::OnFileSaved(wxStyledTextEvent &evt)
{
    if (NULL == mpBufferList){
        return;
    }
    int id = mpBufferList->GetPageIndex(dynamic_cast<wxWindow*>(evt.GetEventObject()));
    if (id == wxNOT_FOUND){
        return;
    }
    wxString title = mpBufferList->GetPageText(id);
    title = title.substr(1);
    mpBufferList->SetPageText(id, title);
}

void MyFrame::OnFileModified(wxStyledTextEvent &evt)
{
    if (NULL == mpBufferList){
        return;
    }
    int id = mpBufferList->GetPageIndex(dynamic_cast<wxWindow*>(evt.GetEventObject()));
    if (id == wxNOT_FOUND){
        return;
    }
    wxString title = mpBufferList->GetPageText(id);
    title = "*" + title;
    mpBufferList->SetPageText(id, title);
}

void MyFrame::OnFileClose(wxAuiNotebookEvent &evt)
{
    int select = evt.GetSelection();
    if (select == wxNOT_FOUND || NULL == mpBufferList){
        return;
    }
    wxString name = mpBufferList->GetPageText(select);
    Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(select));
    if (NULL != pEdit){
        if (pEdit->Modified()){
            int answer = wxMessageBox(wxString::Format("%s has been modified, save it?", name),
                                      wxT("Confirm Save"), wxYES_NO | wxCANCEL, this);
            if (answer == wxYES){
                pEdit->SaveFile(true);
            }
            else if (answer == wxCANCEL){
                if (evt.IsAllowed()){
                    evt.Veto();
                    return;
                }
            }
        }
        if (NULL != mpBufferSelect){
            mpBufferSelect->DelBuffer(name, pEdit->GetFilename());
        }
    }
}

void MyFrame::OnShowSearch(wxCommandEvent &evt)
{
    wxString value;
    int select = mpBufferList->GetSelection();
    if (wxNOT_FOUND != select){
        Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(select));
        if (NULL != pEdit){
            value = pEdit->GetCurrentWord();
        }
    }
    wxAuiPaneInfo &pane = m_mgr.GetPane(wxT("Find"));
    if (pane.IsOk()){
        pane.Show();
    }
    else{
        // add file search
        mpSearch = new wxSearchFile(this);
        MySearchHandler *pSearchHandler = new MySearchHandler(this);
        mpSearch->AddHandler(pSearchHandler);
        mpSearch->SetMinStartLen(3);
        m_mgr.AddPane(mpSearch, wxAuiPaneInfo().Name(wxT("Find")).Caption(wxT("Find..."))
                      .Bottom().Row(1).BestSize(wxSize(300,200)).PaneBorder(false).MinSize(wxSize(300,100)));
    }
    if (!value.empty()){
        mpSearch->SetInput(value);
    }
    if (NULL != mpSearch){
        mpSearch->SetFocus();
    }
    m_mgr.Update();
}

void MyFrame::OnShowFindFiles(wxCommandEvent &evt)
{
    wxString value;
    int select = mpBufferList->GetSelection();
    if (wxNOT_FOUND != select){
        Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(select));
        if (NULL != pEdit){
            value = pEdit->GetCurrentWord();
        }
    }
    
    wxAuiPaneInfo &pane = m_mgr.GetPane(wxT("FindFiles"));
    if (pane.IsOk()){
        pane.Show();
    }
    else{
        mpSearchDir = new wxSearchDir(this);
        MyDirSearchHandler *pSearchHandler = new MyDirSearchHandler(this);
        mpSearchDir->AddHandler(pSearchHandler);
        mpSearchDir->SetMinStartLen(3);
        m_mgr.AddPane(mpSearchDir, wxAuiPaneInfo().Name(wxT("FindFiles")).Caption(wxT("Find files..."))
                      .Bottom().Row(1).BestSize(wxSize(300,200)).PaneBorder(false).MinSize(wxSize(300, 100)));

    }
    
    if (!value.empty()){
        mpSearchDir->SetInput(value);
    }
    if (NULL != mpSearchDir){
        mpSearchDir->SetFocus();
    }
    m_mgr.Update();
}

class MyBufferSelectHandler: public wxSearchHandler
{
private:
    MyFrame *mpFrame;
public:
    MyBufferSelectHandler(MyFrame *frame){mpFrame = frame;}

    void OnSelect(wxSearchResult &ret, bool bActive){
        // if name is *FindFile* show mpSearchDir
        // if name is *Find* show mpSearch
        if (NULL != mpFrame){
            if (bActive){
                wxCommandEvent evt;
                mpFrame->OnKillCurrentBuffer(evt);
            }
            mpFrame->OpenFile(ret.Content(), ret.Target(), bActive);
        }
    }
};

void MyFrame::OnSaveCurrentBuffer(wxCommandEvent &evt)
{
    if (NULL == mpBufferList){
        return;
    }
    int select = mpBufferList->GetSelection();
    if (wxNOT_FOUND == select){
        return;
    }
    Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(select));
    if (NULL != pEdit && pEdit->HasFocus()){
        pEdit->SaveFile(false);
    }
}

void MyFrame::OnShowBufferSelect(wxCommandEvent &evt)
{
    wxAuiPaneInfo &pane = m_mgr.GetPane(wxT("BufferSelect"));
    if (pane.IsOk()){
        pane.Show();
    }
    else{
        mpBufferSelect = new wxBufferSelect(this);
        mpBufferSelect->AddHandler(new MyBufferSelectHandler(this));
        if (NULL != mpBufferList){
            int i = 0;
            for (i = 0; i < mpBufferList->GetPageCount(); i++){
                Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(i));
                if (NULL != pEdit){
                    mpBufferSelect->AddBuffer(mpBufferList->GetPageText(i), pEdit->GetFilename());
                }
            }
        }
        m_mgr.AddPane(mpBufferSelect, wxAuiPaneInfo().Name(wxT("BufferSelect")).Caption(wxT("Select buffers..."))
                      .Bottom().Row(1).BestSize(wxSize(300,200)).PaneBorder(false).MinSize(wxSize(300,100)));
    }
    if (NULL != mpBufferSelect){
        mpBufferSelect->SetFocus();
    }
    m_mgr.Update();
}

void MyFrame::OnShowOneWindow(wxCommandEvent &evt)
{
    wxPrintf("OnShowOneWindow\n");
    bool bUpdate = false;
    if (NULL != mpSearch){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpSearch);
        if (info.IsOk()){
            bUpdate = true;
            info.Hide();
        }
    }
    if (NULL != mpSearchDir){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpSearchDir);
        if (info.IsOk()){
            bUpdate = true;
            info.Hide();
        }
    }
    if (bUpdate){
        m_mgr.Update();
        SwitchFocus();
    }
}

void MyFrame::OnKillCurrentBuffer(wxCommandEvent &evt)
{
    bool bUpdate = false;
    wxWindow *window = FindFocus();
    if (NULL != mpSearch && mpSearch->IsDescendant(window)){
        window = mpSearch;
    }
    else if (NULL != mpSearchDir && mpSearchDir->IsDescendant(window)){
        window = mpSearchDir;
    }
    else if (NULL != mpBufferSelect && mpBufferSelect->IsDescendant(window)){
        window = mpBufferSelect;
    }
    else if (NULL != mpBufferList && mpBufferList->IsDescendant(window)){
        int id = mpBufferList->GetSelection();
        if (id != wxNOT_FOUND){    
            wxAuiNotebookEvent evt;
            evt.SetSelection(id);
            OnFileClose(evt);
            if (evt.IsAllowed()){
                mpBufferList->DeletePage(id);
                OnFileClosed(evt);
            }
        }
    }
    wxAuiPaneInfo &info = m_mgr.GetPane(window);
    if (info.IsOk()){
        bUpdate = true;
        info.Hide();
    }
    if (bUpdate){
        m_mgr.Update();
        SwitchFocus();
    }
}

void MyFrame::SwitchFocus()
{
    if (NULL != mpBufferList){
        int selection = mpBufferList->GetSelection();
        if (wxNOT_FOUND != selection){
            Edit *pEdit = dynamic_cast<Edit *>(mpBufferList->GetPage(selection));
            if (NULL != pEdit){
                wxPrintf("Change focus to %s\n", pEdit->GetFilename());
                pEdit->SetFocus();
                return;
            }
        }
    }
    if (NULL != mpSearch){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpSearch);
        if (info.IsOk() && info.IsShown()){
            wxPrintf("Change focus to Search\n");
            mpSearch->SetFocus();
            return;
        }
    }
    if (NULL != mpSearchDir){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpSearchDir);
        if (info.IsOk() && info.IsShown()){
            wxPrintf("Change focus to search dir\n");
            mpSearchDir->SetFocus();
            return;
        }
    }
    if (NULL != mpCmd){
        wxPrintf("Change focus to cmd\n");
        mpCmd->SetFocus();
    }
}
