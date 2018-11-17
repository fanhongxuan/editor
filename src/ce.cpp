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
#include "ce.hpp"
#include "res/sample.xpm"

#include <wx/config.h>
#include <wx/wxcrtvararg.h> // for wxPrintf
#include "wxSearch.hpp"
#include "wxEdit.hpp"
#include "wxBufferSelect.hpp"
#include "wxExplorer.hpp"
#include "wxWorkSpace.hpp"
#include "wxDockArt.hpp"

wxIMPLEMENT_APP(MyApp);
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

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
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
EVT_MENU(ID_ShowExplorer, MyFrame::OnShowExplorer)
EVT_MENU(ID_ShowWorkSpace, MyFrame::OnShowWorkSpace)
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
    mbLoadFinish = false;
    mpSearchDir = NULL;
    mpSearch = NULL;
    mpBufferList = NULL;
    mpBufferSelect = NULL;
    mpSearchHandler = NULL;
    mpCmd = NULL;
    // tell wxAuiManager to manage this frame
    m_mgr.SetArtProvider(new wxMyDockArt);
    m_mgr.SetManagedWindow(this);

    // set frame icon
    SetIcon(wxIcon(sample_xpm));
    CreateAcceTable();
    
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
    LoadInfo();
    mbLoadFinish = true;
}

MyFrame::~MyFrame()
{
    m_mgr.UnInit();
}

void MyFrame::CreateAcceTable()
{
#define ACCE_COUNT  9  
    wxAcceleratorEntry e[ACCE_COUNT];
    e[0].Set(wxACCEL_CTRL, (int)'F', ID_ShowSearch); // CTRL+F (Find in current file)
    e[1].Set(wxACCEL_CTRL, (int)'O', ID_ShowFindFiles); // CTRL+O (find and Open of file)
    e[2].Set(wxACCEL_CTRL, (int)'K', ID_KillCurrentBuffer); // CTRL+K (Hide Current Pane, If in buffer list, mean kill that buffer)
    e[3].Set(wxACCEL_CTRL, (int)'1', ID_ShowOneWindow); // CTRL+1 (Hide all the pane except BufferList)
    e[4].Set(wxACCEL_CTRL, (int)'B', ID_ShowBufferSelect); // CTRL+B (SwitchBuffer)
    e[5].Set(wxACCEL_CTRL, (int)'S', ID_SaveCurrentBuffer); // CTRL+S (Save Current Buffer)
    e[6].Set(wxACCEL_ALT,  (int)';', ID_TriggerComment);// ALT+; to comments/uncomments a block or current line
    e[7].Set(wxACCEL_CTRL, (int)'E', ID_ShowExplorer); // CTRL+E show the explorer
    e[8].Set(wxACCEL_CTRL, (int)'W', ID_ShowWorkSpace); // CTRL+W show WorkSpace
    // todo:fanhongxuan@gmail.com
    // add CTRL+X C to close CE.
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
            // mpEdit->SetSelection(pRet->GetPos(), pRet->GetPos());
            // mpEdit->GotoPos(pRet->GetPos());
            if (bActive){
                mpEdit->SetFocus();
            }
        }
    }
    
    void OnPrepareResult(const wxString &input, std::vector<wxSearchResult*> &results){
        if (NULL != mpFrame){
            mpFrame->PrepareResults(*this, input, results);
        }
    }
};

void MyFrame::OpenFile(const wxString &filename, const wxString &path, bool bActive)
{
    if (NULL == mpBufferList){
        return;
    }
    int i = 0;
    Edit *pEdit = NULL;
    wxString name, ext;
    wxFileName::SplitPath(filename, NULL, NULL, &name, &ext);
    if (!ext.empty()){
        name += ".";
        name += ext;
    }
    for (i = 0; i < mpBufferList->GetPageCount(); i++){
        pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(i));
        if (NULL != pEdit && pEdit->GetFilename() == path){
            mpBufferList->SetSelection(i);
            break;
        }
    }
    
    if (i == mpBufferList->GetPageCount()){
        pEdit = new Edit(mpBufferList);
        if (!path.empty()){
            pEdit->LoadFile(path);
        }
        else{
            pEdit->NewFile(name);
        }
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

void MyFrame::SaveInfo()
{
    // note: fanhongxuan@gmail.com
    // store current perspective, when open, will restore it.
    wxString perspective = m_mgr.SavePerspective();
    wxConfig config("CE");
    config.Write("/Config/LastPerspective", perspective);
    // store all the open file, when open, open it again.
    if (config.HasGroup("/Config/LastOpenFile")){
        config.DeleteGroup("/Config/LastOpenFile");
    }
    if (NULL != mpBufferList && mpBufferList->GetPageCount() != 0){
        int i = 0;
        for (i = 0; i < mpBufferList->GetPageCount(); i++){
            Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(i));
            if (NULL != pEdit){
                config.Write(wxString::Format("/Config/LastOpenFile/%d", i), pEdit->GetFilename());
            }
        }
        int selection = mpBufferList->GetSelection();
        config.Write("/Config/LastOpenFile/Selection", selection);
        Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(selection));
        if (NULL != pEdit){
           config.Write("/Config/LastOpenFile/FirstVisibleLine", pEdit->GetFirstVisibleLine());
           config.Write("/Config/LastOpenFile/InsertionPoint", pEdit->GetInsertionPoint());
        }
    }

    // save the window pos and size
    wxPoint pos = GetPosition();
    wxSize size = GetSize();
    config.Write("/Config/LastPos.X", pos.x);
    config.Write("/Config/LastPos.Y", pos.y);
    config.Write("/Config/LastSize.width", size.GetWidth());
    config.Write("/Config/LastSize.height", size.GetHeight());
}

void MyFrame::LoadInfo()
{
    wxConfig config("CE");
    int i = 0;
    for (i = 0; i < 100; i++){
        wxString entry = wxString::Format("/Config/LastOpenFile/%d", i);
        if (!config.HasEntry(entry)){
            int selection = config.ReadLong("/Config/LastOpenFile/Selection", 0);
            if (selection < mpBufferList->GetPageCount()){
                Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(selection));
                if (NULL != pEdit){
                    long line = config.ReadLong("/Config/LastOpenFile/FirstVisibleLine", 0);
                    long insertionPoint = config.ReadLong("/Config/LastOpenFile/InsertionPoint", 0);
                    pEdit->SetFirstVisibleLine(line);
                    pEdit->SetSelection(insertionPoint, insertionPoint);
                    pEdit->SetInsertionPoint(insertionPoint);
                }
                mpBufferList->SetSelection(selection);
            }
            
            break;
        }
        else{
            wxString filename;
            config.Read(entry, &filename);
            if (!filename.empty()){
                OpenFile(filename, filename, false);
            }
        }
    }
    wxString perspective;
    config.Read("/Config/LastPerspective", &perspective);
    if (!perspective.empty()){
            wxCommandEvent evt;
            OnShowSearch(evt);
            OnShowExplorer(evt);
            OnShowWorkSpace(evt);
            OnShowFindFiles(evt);
            OnShowBufferSelect(evt);
        m_mgr.LoadPerspective(perspective);
    }
    else{
        m_mgr.Update();
    }
    int x = 0, y = 0, width = 0, height = 0;
    x = config.ReadLong("/Config/LastPos.X", 0);
    y = config.ReadLong("/Config/LastPos.Y", 0);
    width = config.ReadLong("/Config/LastSize.width", 0);
    height = config.ReadLong("/Config/LastSize.height", 0);
    if ((x+y+width+height) != 0){
        SetSize(x, y, width, height);
    }
    SwitchFocus();
}

void MyFrame::SetActiveEdit(Edit *pEdit)
{
    if (NULL != mpSearch && NULL != pEdit){
        mpSearch->SetFileName(pEdit->GetFilename());
        mpSearch->SetEdit(pEdit);
    }
}

void MyFrame::PrepareResults(MySearchHandler &handler, const wxString &input, std::vector<wxSearchResult*> &results)
{
    // note:fanhongxuan@gmail.com
    // prepare the current file to wxSearchFile.
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
    // mpSearch->SetBuffer(pEdit->GetText());
    mpSearch->SetEdit(pEdit);
}

void MyFrame::OnClose(wxCloseEvent &evt)
{
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

    SaveInfo();
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
#define VALID_CHAR_WHEN_SEARCH_FILE "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"    
    wxString value;
    int line = -1;
    int select = mpBufferList->GetSelection();
    if (wxNOT_FOUND != select && mbLoadFinish){
        Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(select));
        if (NULL != pEdit){
            value = pEdit->GetCurrentWord(VALID_CHAR_WHEN_SEARCH_FILE);
            line = pEdit->GetCurrentLine();
        }
    }
    wxAuiPaneInfo &pane = m_mgr.GetPane(wxT("Find"));
    if (pane.IsOk()){
        pane.Show();
    }
    else{
        // add file search
        mpSearch = new wxSearchFile(this);
        if (NULL == mpSearchHandler){
            mpSearchHandler = new MySearchHandler(this);
        }
        mpSearch->AddHandler(mpSearchHandler);
        mpSearch->SetMinStartLen(3);
        m_mgr.AddPane(mpSearch, wxAuiPaneInfo().Name(wxT("Find")).Caption(wxT("Find..."))
                      .Bottom().CloseButton(false).Row(1).BestSize(wxSize(300,200)).PaneBorder(false).MinSize(wxSize(300,100)));
    }
    if (!value.empty()){
        if (line >= 0){
            mpSearch->SetCurrentLine(line);
        }
        // note:fanhongxuan@gmail.com
        // select the candidate by the current position
        mpSearch->SetInput(value);
    }
    if (NULL != mpSearch){
        mpSearch->SetFocus();
    }
    m_mgr.Update();
}

void MyFrame::OnShowFindFiles(wxCommandEvent &evt)
{
#define VALID_CHAR_WHEN_SEARCH_DIR "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890/."     
    wxString value;
    int select = mpBufferList->GetSelection();
    if (wxNOT_FOUND != select && mbLoadFinish){
        Edit *pEdit = dynamic_cast<Edit*>(mpBufferList->GetPage(select));
        if (NULL != pEdit){
            value = pEdit->GetCurrentWord(VALID_CHAR_WHEN_SEARCH_DIR);
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
                      .Bottom().CloseButton(false).Row(1).BestSize(wxSize(300,200)).PaneBorder(false).MinSize(wxSize(300, 100)));

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
            wxPrintf("content:%s\n", ret.Content());
            if (ret.Content() == wxString::Format(wxT("New %s"), ret.Target())){
                if (bActive){
                    wxCommandEvent evt;
                    mpFrame->OnKillCurrentBuffer(evt);
                    mpFrame->OpenFile(ret.Target(), "", bActive);
                }
            }
            else{
                if (bActive){
                    wxCommandEvent evt;
                    mpFrame->OnKillCurrentBuffer(evt);
                }
                mpFrame->OpenFile(ret.Content(), ret.Target(), bActive);
            }
        }
    }
    
    void OnUpdateSearchResultAfter(const wxString &input, std::vector<wxSearchResult*> &results, int match){
        if (!input.empty() && match <= 10){
            results.push_back(new wxSearchResult(wxString::Format(wxT("New %s"), input), input));
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
        // if (!pEdit->Getfilename().empty()){
        // }
        pEdit->SaveFile(false);
        wxString filename = pEdit->GetFilename();
        if (!filename.empty()){
            wxString name, ext;
            wxFileName::SplitPath(filename, NULL, NULL, &name, &ext);
            if (!ext.empty()){
                name += ".";
                name += ext;
            }
            mpBufferList->SetPageText(select, name);
        }
    }
}

void MyFrame::OnShowWorkSpace(wxCommandEvent &evt)
{
    wxAuiPaneInfo &explorer = m_mgr.GetPane(wxT("Explorer"));
    if (explorer.IsOk()){
        explorer.Hide();
    }
    wxAuiPaneInfo &pane = m_mgr.GetPane(wxT("WorkSpace"));
    if (pane.IsOk()){
        pane.Show();
    }
    else{
        mpWorkSpace = new wxWorkSpace(this);
        m_mgr.AddPane(mpWorkSpace, wxAuiPaneInfo().Name(wxT("WorkSpace")).Caption(wxT("WorkSpace")).
                      Left().CloseButton(false).BestSize(wxSize(200, 500)).PaneBorder(false).MinSize(wxSize(200,200)));
    }
    if (NULL != mpWorkSpace){
        mpWorkSpace->SetFocus();
    }
    m_mgr.Update();
}

void MyFrame::OnShowExplorer(wxCommandEvent &evt)
{
    // note:fanhongxuan@gmail.com
    // when show explorer, will hide WorkSpace
    wxAuiPaneInfo &workspace = m_mgr.GetPane(wxT("WorkSpace"));
    if (workspace.IsOk()){
        workspace.Hide();
    }
    wxAuiPaneInfo &pane = m_mgr.GetPane(wxT("Explorer"));
    if (pane.IsOk()){
        pane.Show();
    }
    else{
        mpExplorer = new wxExplorer(this);
        m_mgr.AddPane(mpExplorer, wxAuiPaneInfo().Name(wxT("Explorer")).Caption(wxT("Explorer")).
                      Left().CloseButton(false).BestSize(wxSize(200, 500)).PaneBorder(false).MinSize(wxSize(200,200)));
    }
    if (NULL != mpExplorer){
        mpExplorer->SetFocus();
    }
    m_mgr.Update();
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
                      .Bottom().CloseButton(false).Row(1).BestSize(wxSize(300,200)).PaneBorder(false).MinSize(wxSize(300,100)));
    }
    if (NULL != mpBufferSelect){
        mpBufferSelect->SetFocus();
    }
    m_mgr.Update();
}

void MyFrame::OnShowOneWindow(wxCommandEvent &evt)
{
    bool bUpdate = false;
    if (NULL != mpSearch){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpSearch);
        if (info.IsOk()){
            bUpdate = true;
            info.Hide();
        }
    }
    if (NULL != mpBufferSelect){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpBufferSelect);
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
    
    if (NULL != mpExplorer){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpExplorer);
        if (info.IsOk()){
            bUpdate = true;
            info.Hide();
        }
    }
    if (NULL != mpWorkSpace){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpWorkSpace);
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
        int select = mpBufferList->GetSelection();
        if (wxNOT_FOUND != select){
            Edit *pEdit = dynamic_cast<Edit *>(mpBufferList->GetPage(select));
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
