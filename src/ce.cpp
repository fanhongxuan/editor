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
#include <wx/ipc.h>
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
#include <wx/cmdline.h>
#include <wx/snglinst.h>
#include <wx/filefn.h> 
#include <wx/config.h>
#include <wx/wxcrtvararg.h> // for wxPrintf
#include <wx/aui/aui.h>

#include "ce.hpp"
#include "res/sample.xpm"


#include "wxSearch.hpp"
#include "wxBufferSelect.hpp"
#include "wxExplorer.hpp"
#include "wxWorkSpace.hpp"
#include "wxDockArt.hpp"
#include "wxAgSearch.hpp"
#include "wxSymbolSearch.hpp"
#include "ceRefSearch.hpp"
#include "ceUtils.hpp"
#include "ceEdit.hpp"
#include "ceCompile.hpp"
#ifdef USE_NOTEBOOK
#include "Notebook.h"
#endif

wxIMPLEMENT_APP(MyApp);

void MyApp::OnInitCmdLine( wxCmdLineParser &  parser )
{
    if (argc  != 1){
        parser.AddParam();
    }
}

bool MyApp::OnCmdLineError(wxCmdLineParser &parser)
{
    return true;
}

bool MyApp::OnCmdLineParsed(wxCmdLineParser& parser){
    return true;
}
#define MY_IPC_TOPIC "ce-ipc"
#define MY_IPC_SERVICE "61398"

class MyConnection: public wxConnection
{
public:
    MyConnection(){}
    ~MyConnection(){}
    
    virtual bool OnPoke(const wxString &topic, const wxString &item, const void *data, size_t size, wxIPCFormat format) wxOVERRIDE{
        wxPrintf("OnPoke:<%s><%s>\n", topic, item);
        if (NULL != wxGetApp().frame()){
            wxGetApp().frame()->OpenFile(item, item, true);
            wxGetApp().frame()->Restore();
            wxGetApp().frame()->Raise();
            // wxGetApp().frame()->Show();
            // wxGetApp().frame()->RequestUserAttention();
        }
		return true;
    }
    virtual bool OnDisconnect() wxOVERRIDE{return true;}
};

class MyServer: public wxServer
{
public:
    MyServer(){
        mpConnection = NULL;
        };
    virtual ~MyServer(){
        if (NULL != mpConnection){
            delete mpConnection;
            mpConnection = NULL;
        }
    }
    
    virtual wxConnectionBase *OnAcceptConnection(const wxString &topic) wxOVERRIDE
    {
        if (topic == MY_IPC_TOPIC){
            if (NULL != mpConnection){
                delete mpConnection;
                mpConnection = NULL;
            }
            mpConnection = new MyConnection;
            return mpConnection;
        }
        return NULL;
    }
protected:
    MyConnection *mpConnection;
};

class MyClient: public wxClient
{
public:
    MyClient(){}
    virtual ~MyClient(){}
};

int MyApp::OnExit()
{
    if (NULL != mpChecker){
        delete mpChecker;
        mpChecker = NULL;
    }
    if (NULL != mpServer){
        delete mpServer;
        mpServer = NULL;
    }
    return 0;
}

bool MyApp::OnInit()
{
    mpChecker = new wxSingleInstanceChecker;
    mpFrame = NULL;
    if (!wxApp::OnInit()){
        return false;
    }
    wxString target;
    if (argc > 1){
        if((access(argv[1], 0) != 0)){
            FILE *fp = fopen(argv[1], "w+");
            if (NULL == fp){
                wxPrintf("Failed to create %s\n", argv[1]);
                return false;
            }
            fclose(fp);
        }
        target = argv[1];
    }
    
    if (mpChecker->IsAnotherRunning()){
        if (!target.empty()){
            // convert the target to a absolute file
            wxFileName file(target);
            file.MakeAbsolute();
            target = file.GetFullPath();
            MyClient client;
            wxConnectionBase *pConnection = client.MakeConnection("localhost", MY_IPC_SERVICE, MY_IPC_TOPIC);
            if (NULL != pConnection){
                // wxPrintf("Connected to service\n");
                pConnection->Poke(target, target);
                wxPrintf("Open %s in exist ce\n", target);
                delete pConnection;
            }
        }
        return false;
    }
    
    mpServer = new MyServer;
    if(!mpServer->Create(MY_IPC_SERVICE)){
        wxPrintf("Failed to start IPC Server\n");
    }
    else{
        wxPrintf("Start IPC Server ok\n");
    }
    wxPrintf("Start create MyFrame\n");
    mpFrame = new MyFrame(NULL,
                          wxID_ANY,
                          wxT("CodeBrowser"),
                          wxDefaultPosition,
                          wxSize(800, 600));
    wxPrintf("Create mpFrame ok\n");
    mpFrame->Show();
    
    if (!target.empty()){
        mpFrame->OpenFile(target, target, true);
    }
    return true;
}

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_STC_SAVEPOINTREACHED(wxID_ANY, MyFrame::OnFileUnModified)
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
EVT_MENU(ID_ShowExplorerOfCurrentFile, MyFrame::OnShowExplorerOfCurrentFile)
EVT_MENU(ID_ShowWorkSpace, MyFrame::OnShowWorkSpace)
EVT_MENU(ID_ShowAgSearch, MyFrame::OnShowAgSearch)
EVT_MENU(ID_ShowSymbolList, MyFrame::OnShowSymbolList)
EVT_MENU(ID_ShowReference, MyFrame::OnShowReference)
EVT_MENU(ID_ShowGrepText, MyFrame::OnShowGrepText)
EVT_MENU(ID_GotoDefine, MyFrame::OnGotoDefine)
EVT_MENU(ID_GoBack, MyFrame::OnGoBack)
EVT_MENU(ID_NextFile, MyFrame::OnNextFile)
EVT_MENU(ID_PrevFile, MyFrame::OnPrevFile)
EVT_MENU(ID_GoForward, MyFrame::OnGoForward)
EVT_MENU(ID_Compile, MyFrame::OnCompile)
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
            if (bActive){
                mpMain->ShowMiniBuffer("FindFiles", true);
            }
            mpMain->OpenFile(ret.Content(), ret.Target(), bActive);
        }
    }
};

class MyDnDText: public wxTextDropTarget
{
public:
    MyDnDText(wxWindow *pOwner, MyFrame *pFrame)
    {
        mpOwner = pOwner;
        mpFrame = pFrame;
    }
    virtual bool OnDropText(wxCoord x, wxCoord y, const wxString &text)
    {
        if (text.find("file://") == 0){
            // the target is a file.
            wxString path = text.substr(wxString("file://").length());
            if (path.size() > 1 && (path[path.length()-1] == '\r' || path[path.length()-1] == '\n')){
                path = path.substr(0, path.length()-1);
            }
            if (path.size() > 1 && (path[path.length()-1] == '\r' || path[path.length()-1] == '\n')){
                path = path.substr(0, path.length()-1);
            }
            if (access(static_cast<const char *>(path), 0) == 0){
                if (NULL != mpFrame){
                    mpFrame->OpenFile(path, path, true);
                }
                return true;
            }
            else{
                wxPrintf("Failed to access <%s>\n", path);
            }
        }
        ceEdit *pEdit = dynamic_cast<ceEdit*>(mpOwner);
        if (NULL != pEdit){
            pEdit->InsertText(pEdit->GetCurrentPos(), text);
            return true;
        }
        return false;
    }
private:
    wxWindow *mpOwner;
    MyFrame *mpFrame;
};

// class MyDnDFile: public wxFileDropTarget
// {
// public:
//     MyDnDFile(wxWindow *pOwner)
//     {
//         mpOwner = pOwner;
//     }
//     virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString &files);
// private:
//     wxWindow *mpOwner;
// };

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
    mpSymbolSearch = NULL;
    mpBufferList = NULL;
    mpBufferSelect = NULL;
    mpSearchHandler = NULL;
    mpSymbolSearchHandler = NULL;
    mpAgSearchHandler = NULL;
    mpRefSearchHandler = NULL;
	mpExplorer = NULL;
	mpWorkSpace = NULL;
	mpAgSearch = NULL;
    mpRefSearch = NULL;
    mpActiveEdit = NULL;
    mGotoIndex = -1;
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
#ifdef USE_NOTEBOOK    
    mpBufferList = new ceNootBook(this, wxID_ANY,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  kNotebook_DarkTabs | kNotebook_EnableNavigationEvent | kNotebook_AllowDnD |
                                  kNotebook_ShowFileListButton);
#else
    mpBufferList = new ceNootBook(this, wxID_ANY,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_WINDOWLIST_BUTTON);
    mpBufferList->SetArtProvider(new wxMyTabArt);
#endif

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
#define ACCE_COUNT  19
    wxAcceleratorEntry e[ACCE_COUNT];
    e[ 0].Set(wxACCEL_CTRL, (int)'F', ID_ShowSearch); // CTRL+F (Find in current file)
    e[ 1].Set(wxACCEL_ALT, (int)'O', ID_ShowFindFiles); // CTRL+O (find and Open of file)
    e[ 2].Set(wxACCEL_CTRL, (int)'K', ID_KillCurrentBuffer); // CTRL+K (Hide Current Pane, If in buffer list, mean kill that buffer)
    e[ 3].Set(wxACCEL_CTRL, (int)'1', ID_ShowOneWindow); // CTRL+1 (Hide all the pane except BufferList)
    e[ 4].Set(wxACCEL_CTRL, (int)'B', ID_ShowBufferSelect); // CTRL+B (SwitchBuffer)
    e[ 5].Set(wxACCEL_CTRL, (int)'S', ID_SaveCurrentBuffer); // CTRL+S (Save Current Buffer)
    e[ 6].Set(wxACCEL_CTRL, (int)'E', ID_ShowExplorer); // CTRL+E show the explorer
    e[ 7].Set(wxACCEL_CTRL, (int)'W', ID_ShowWorkSpace); // CTRL+W show WorkSpace
    e[ 8].Set(wxACCEL_ALT,  (int)'F', ID_ShowAgSearch); // ALT+F to find in dir/workspace
    e[ 9].Set(wxACCEL_ALT,  (int)'S', ID_ShowSymbolList); // ALT+S to show the symbol list
    e[10].Set(wxACCEL_ALT,  (int)'R', ID_ShowReference); // ALT+R to show reference of current symbol
    e[11].Set(wxACCEL_ALT,  (int)'.', ID_GotoDefine); // ALT+. to show the define of current symbol
    e[12].Set(wxACCEL_ALT,  (int)'T', ID_ShowGrepText); // ALT+T to grep the currenty symbol
    e[13].Set(wxACCEL_ALT, WXK_RIGHT, ID_GoForward);
    e[14].Set(wxACCEL_ALT, WXK_LEFT, ID_GoBack);
    e[15].Set(wxACCEL_CTRL, WXK_PAGEDOWN, ID_NextFile);
    e[16].Set(wxACCEL_CTRL, WXK_PAGEUP, ID_PrevFile);
    e[17].Set(wxACCEL_NORMAL, WXK_F6, ID_Compile);
    e[18].Set(wxACCEL_ALT,  (int)'E', ID_ShowExplorerOfCurrentFile); // ALT+E show explorer and switch to current file.
    // todo:fanhongxuan@gmail.com
    // add CTRL+X C to close CE.
    wxAcceleratorTable acce(ACCE_COUNT, e);
    SetAcceleratorTable(acce);
}

wxAuiDockArt* MyFrame::GetDockArt()
{
    return m_mgr.GetArtProvider();
}

void MyFrame::UpdateWorkDirs(ceEdit *pActiveEdit, bool showWorkSpace, bool showExplorer)
{
    if (!mbLoadFinish){
        return;
    }
    
    std::set<wxString> dirs;
    std::set<wxString> files;
    wxAuiPaneInfo &workspace = m_mgr.GetPane(wxT("WorkSpace"));
    wxAuiPaneInfo &explorer = m_mgr.GetPane(wxT("Explorer"));
    if (NULL != mpWorkSpace && (showWorkSpace || workspace.IsShown())){
        mpWorkSpace->GetDirs(dirs);
        mpWorkSpace->GetFiles(files);
    }
    else if (NULL != mpExplorer && (showExplorer || explorer.IsShown())){
        dirs.insert(mpExplorer->GetCwd());
    }
    else if (NULL != pActiveEdit){
        wxString filename = pActiveEdit->GetFilename();
        int pos = filename.find_last_of("/\\");
        if (pos != filename.npos){
            dirs.insert(filename.substr(0, pos));
        }
    }
    
    if (NULL != mpSearchDir){
        mpSearchDir->SetDirs(dirs);
    }
    if (NULL != mpAgSearch){
        mpAgSearch->SetSearchDirs(dirs);
        mpAgSearch->SetSearchFiles(files);
    }
}

void MyFrame::ShowStatus(const wxString &status, int index)
{
    // use a stack to show more status.
    // max has 10 status.
    if (index > 10){
        return;
    }
    else if (mStatus.size() <= index){
        mStatus.resize(index+1);
    }
    
    mStatus[index] = status;
    if (NULL != mpCmd){
        wxString value;
        for (int i = 0; i < mStatus.size(); i++){
            if (!mStatus[i].empty()){
                if (!value.empty()){
                    value.Append("\t");
                }
                value.Append(mStatus[i]);
            }
        }
        mpCmd->SetValue(value);
    }
}

void MyFrame::DoUpdate()
{   
    UpdateWorkDirs(mpActiveEdit);
    m_mgr.Update();
}

class MyAgSearchHandler: public wxSearchHandler
{
private:
    MyFrame *mpFrame;
public:
    MyAgSearchHandler(MyFrame *frame){mpFrame = frame;}
    void OnSelect(wxSearchResult &ret, bool bActive){
        wxSearchFileResult *pRet = dynamic_cast<wxSearchFileResult*>(&ret);
        if (NULL != pRet){
            wxPrintf("MyAgSearchHandler::OnSelect:%s\n", pRet->Target());
            int line = pRet->GetLine();
            // if the target is not open, first open it.
            if (!pRet->Target().empty()){
                mpFrame->OpenFile(pRet->Target(), pRet->Target(), true, line);
            }
        }
    }
};

class MyRefSearchHandler: public wxSearchHandler
{
private:
    MyFrame *mpFrame;
public:
    MyRefSearchHandler(MyFrame *frame){mpFrame = frame;}
    void OnSelect(wxSearchResult &ret, bool bActive){
        wxSearchFileResult *pRet = dynamic_cast<wxSearchFileResult *>(&ret);
        if (NULL != pRet && NULL != mpFrame){
            if (bActive){
                mpFrame->ShowMiniBuffer("Reference", true);
            }
            int line = pRet->GetLine();
            if (!pRet->Target().empty()){
                mpFrame->OpenFile(pRet->Target(), pRet->Target(), true, line);
            }
        }
    }
};

class MySymbolSearchHandler: public wxSearchHandler
{
private:
    ceEdit *mpEdit;
    MyFrame *mpFrame;
public:
    MySymbolSearchHandler(MyFrame *frame){mpFrame = frame; mpEdit = NULL;}
    void SetEdit(ceEdit *pEdit){mpEdit = pEdit;}
    void OnSelect(wxSearchResult &ret, bool bActive){
        wxSearchFileResult *pRet = dynamic_cast<wxSearchFileResult*>(&ret);
        if (NULL != pRet && NULL != mpEdit){
            int line = pRet->GetLine();
            mpFrame->ChangeToBuffer(mpEdit, -1/*currently not used*/);
            mpEdit->SetFirstVisibleLine(line-20);
            mpEdit->GotoLine(line);
            wxPrintf("MySymbolSearchHandler::GotoLine:%d\n", line);
            if (bActive){
                mpEdit->SetFocus();
            }
        }
    }    
};

class MySearchHandler: public wxSearchHandler
{
private:
    ceEdit *mpEdit;
    MyFrame *mpFrame;
public:
    MySearchHandler(MyFrame *frame){mpFrame = frame; mpEdit = NULL;}
    void SetEdit(ceEdit *pEdit){mpEdit = pEdit;}
    void OnSelect(wxSearchResult &ret, bool bActive){
        wxSearchFileResult *pRet = dynamic_cast<wxSearchFileResult*>(&ret);
        if (NULL != pRet && NULL != mpEdit){
            if (bActive){
                mpFrame->ShowMiniBuffer("Find", true);
            }
            int line = pRet->GetLine();
            mpFrame->ChangeToBuffer(mpEdit, -1/*currently not used*/);
            mpEdit->SetFirstVisibleLine(line-20);
            mpEdit->GotoLine(line);
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

void MyFrame::OnNextFile(wxCommandEvent &evt){
    if (NULL == mpBufferList || mpBufferList->GetPageCount() <= 1){
        return;
    }
    int selection = mpBufferList->GetSelection();
    selection++;
    if (selection >= mpBufferList->GetPageCount()){
        selection = 0;
    }
    mpBufferList->SetSelection(selection);
}

void MyFrame::OnPrevFile(wxCommandEvent &evt){
    if (NULL == mpBufferList || mpBufferList->GetPageCount() <= 1){
        return;
    }
    int selection = mpBufferList->GetSelection();
    selection--;
    if (selection < 0){
        selection = mpBufferList->GetPageCount() - 1;
    }
    mpBufferList->SetSelection(selection);
}

void MyFrame::OnGoBack(wxCommandEvent &evt){
    // std::vector< std::pair<wxString, int> > mGotoHistory;
    if (mGotoHistory.empty()){
        return;
    }
    mGotoIndex--;
    if (mGotoIndex < 0){
        mGotoIndex = 0;
    }
    // wxPrintf("OnGoBack<%d>(%ld)\n", mGotoIndex, mGotoHistory.size());
    OpenFile(mGotoHistory[mGotoIndex].first, 
             mGotoHistory[mGotoIndex].first, true, mGotoHistory[mGotoIndex].second, false);
}

void MyFrame::OnCompile(wxCommandEvent &evt){
    int selection = mpBufferList->GetSelection();
    if (selection < 0 || selection >= mpBufferList->GetPageCount()){
        return;
    }
    ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(selection));
    if (NULL == pEdit){
        return;
    }
    wxPrintf("Compile <%s>\n", pEdit->GetFilename());
    
    wxAuiPaneInfo &compile = m_mgr.GetPane(wxT("compile"));
    if (compile.IsOk()){
        compile.Show();
    }
    else{
        mpCompile = new ceCompile(this);
        m_mgr.AddPane(mpCompile, wxAuiPaneInfo().Name(wxT("compile")).Caption(wxT("compile")).
                      Bottom().Row(1).CloseButton(false).BestSize(wxSize(200, 500)).PaneBorder(false).MinSize(wxSize(200,200)));
    }
    m_mgr.Update();
    if (NULL != mpCompile && mbLoadFinish){
        UpdateWorkDirs(mpActiveEdit);
        mpCompile->Compile(pEdit->GetFilename());
        mpCompile->SetFocus();
    }
}

void MyFrame::OnGoForward(wxCommandEvent &evt){
    if (mGotoHistory.empty()){
        return;
    }
    mGotoIndex++;
    if (mGotoIndex >= mGotoHistory.size()){
        mGotoIndex = mGotoHistory.size() - 1;
    }
    // wxPrintf("OnGoForward:<%d>(%ld)\n", mGotoIndex, mGotoHistory.size());
    OpenFile(mGotoHistory[mGotoIndex].first, 
             mGotoHistory[mGotoIndex].first, 
             true, 
             mGotoHistory[mGotoIndex].second, false);
}

void MyFrame::OpenFile(const wxString &filename, const wxString &path, bool bActive, int line, bool updateGotoHistory)
{
    if (NULL == mpBufferList){
        return;
    }
    wxString fname = path;
    if (!fname.empty()){
        wxFileName fpath(path);
        fpath.MakeAbsolute();
        fname = fpath.GetFullPath();
    }
    
    int i = 0;
    ceEdit *pEdit = NULL;
    if (updateGotoHistory && bActive){
        mGotoHistory.resize(mGotoIndex+1);
        ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetCurrentPage());
        if (NULL != pEdit && (!pEdit->GetFilename().empty())){
            mGotoHistory.push_back(std::pair<wxString, int>(pEdit->GetFilename(), pEdit->GetCurrentLine()));
        }
        mGotoIndex = mGotoHistory.size();
        // wxPrintf("GotoHistory:<%s><%d>\n", fname, line);
        mGotoHistory.push_back(std::pair<wxString, int>(fname, line));
    }
    
    wxString name, ext;
    wxFileName::SplitPath(filename, NULL, NULL, &name, &ext);
    if (!ext.empty()){
        name += ".";
        name += ext;
    }
    for (i = 0; i < mpBufferList->GetPageCount(); i++){
        pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(i));
        if (NULL != pEdit && pEdit->GetFilename() == fname){
            mpBufferList->SetSelection(i);
            break;
        }
    }
    
    if (i == mpBufferList->GetPageCount()){
        pEdit = new ceEdit(mpBufferList);
        pEdit->SetDropTarget(new MyDnDText(pEdit, this));
        if (!fname.empty()){
            pEdit->LoadFile(fname);
        }
        else{
            pEdit->NewFile(name);
        }
        pEdit->SelectNone();
        mpBufferList->AddPage(pEdit, name, true);
        mpBufferList->SetPageToolTip(i, fname);
        if (NULL != mpBufferSelect){
            mpBufferSelect->AddBuffer(name, fname);
        }
    }
    if (line > 0){
        pEdit->SetFirstVisibleLine(line-20);
        pEdit->GotoLine(line);
    }
    
    if (bActive && NULL != pEdit){
        pEdit->SetFocus();
    }
    // wxPrintf("Open file:<%s><%d>\n", filename, line);
    m_mgr.Update();
}

void MyFrame::ChangeToBuffer(ceEdit *pEdit, int pos)
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
}

wxString MyFrame::GetDbRecordByKey(const wxString &key){
    if (NULL != mpWorkSpace){
        return mpWorkSpace->GetDbRecordByKey(key);
    }
    return "";
}

bool MyFrame::GetSymbols(std::set<ceSymbol *> &symbols, 
                         const wxString &scope, 
                         const wxString &type, 
                         const wxString &language, 
                         const wxString &filename)
{
    if (NULL != mpWorkSpace){
        return mpWorkSpace->GetSymbols(symbols, scope, type, language, filename);
    }
    return false;
}

bool MyFrame::FindDef(std::set<ceSymbol*> &symbols, 
                      const wxString &name,
                      const wxString &className,
                      const wxString &type,
                      const wxString &language,
                      const wxString &filename)
{
    if (NULL != mpWorkSpace){
        return mpWorkSpace->FindDef(symbols, name, className, type, language, filename);
    }
    return false;
}

bool MyFrame::FindDef(const wxString &symbol, std::vector<wxSearchFileResult *> &outputs)
{
    if (NULL != mpRefSearch){
        if (NULL != mpWorkSpace){
            std::set<wxString> dirs;
            mpWorkSpace->GetDirs(dirs);
            mpRefSearch->SetTagDir(dirs);
        }
        return mpRefSearch->FindDef(symbol, outputs);
    }
    return false;
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
            ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(i));
            if (NULL != pEdit){
                config.Write(wxString::Format("/Config/LastOpenFile/File%d.Name", i), pEdit->GetFilename());
                config.Write(wxString::Format("/Config/LastOpenFile/File%d.FirstVisibleLine", i), pEdit->GetFirstVisibleLine());
                config.Write(wxString::Format("/Config/LastOpenFile/File%d.CurrentPos", i), pEdit->GetCurrentPos());
            }
        }
        int selection = mpBufferList->GetSelection();
        config.Write("/Config/LastOpenFile/Selection", selection);
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
        wxString entry = wxString::Format("/Config/LastOpenFile/File%d.Name", i);
        if (!config.HasEntry(entry)){
            int selection = config.ReadLong("/Config/LastOpenFile/Selection", 0);
            if (selection < mpBufferList->GetPageCount()){
                ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(selection));
                if (NULL != pEdit){
                    OpenFile(pEdit->GetFilename(), pEdit->GetFilename(), true);
                }
                wxPrintf("Selection page:%d\n", selection);
                mpBufferList->SetSelection(selection);
                m_mgr.Update();
            }
            break;
        }
        else{
            wxString filename;
            config.Read(entry, &filename);
            if (!filename.empty()){
                OpenFile(filename, filename, false);
                ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(i));
                if (NULL != pEdit){
                    long line = config.ReadLong(wxString::Format("/Config/LastOpenFile/File%d.FirstVisibleLine",i), 0);
                    long insertionPoint = config.ReadLong(wxString::Format("/Config/LastOpenFile/File%d.CurrentPos",i), 0);
                    // wxPrintf("%s goto line:%ld, pos:%ld\n", filename, line, insertionPoint);
                    pEdit->SetFirstVisibleLine(line);
                    pEdit->SetSelection(insertionPoint, insertionPoint);
                    pEdit->GotoPos(insertionPoint);
                }
                if (i == 0){
                    wxPrintf("Select page 0\n");
                    mpBufferList->SetSelection(0);
                    m_mgr.Update();
                }
            }
        }
    }
    wxString perspective;
    config.Read("/Config/LastPerspective", &perspective);
    if (!perspective.empty()){
        wxCommandEvent evt;
        // note:fanhongxuan@gmail.com
        // make sure all the mini-frame is create before load m_perspectives
        OnShowSearch(evt);
        OnShowExplorer(evt);
        OnShowWorkSpace(evt);
        OnShowFindFiles(evt);
        OnShowAgSearch(evt);
        OnShowSymbolList(evt);
        OnShowBufferSelect(evt);
        OnShowReference(evt);
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
    // if no file is open, open the readme.
    if (NULL != mpBufferList && mpBufferList->GetPageCount() == 0){
        wxString readme = ceGetExecPath();
#ifdef WIN32
        readme += "\\README.md";
#else
        readme += "/README.md";
#endif
        OpenFile("README.md", readme, true);
    }
    SwitchFocus();
}

void MyFrame::SetActiveEdit(ceEdit *pEdit)
{
    mpActiveEdit = pEdit;
    if (NULL != mpSearch){
        mpSearch->ChangeSearchTarget(pEdit);
    }
    
    if (NULL != mpSymbolSearch){
        if (NULL != mpSymbolSearchHandler){
            mpSymbolSearchHandler->SetEdit(pEdit);
        }
        mpSymbolSearch->SetEdit(pEdit);
        wxAuiPaneInfo &info = m_mgr.GetPane(wxT("Find Symbol"));
        if (info.IsOk() && info.IsShown() && NULL != pEdit && pEdit->GetFilename() != mpSymbolSearch->GetFileName()){
            mpSymbolSearch->SetLanguage(pEdit->GetLanguage());
            mpSymbolSearch->SetFileName(pEdit->GetFilename());
            mpSymbolSearch->SetInput("");
            mpSymbolSearch->UpdateSearchList("", false);
        }
    }
    UpdateWorkDirs(mpActiveEdit);
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
    ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(select));
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
            ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(i));
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
    ceEdit *pEdit = dynamic_cast<ceEdit*>(evt.GetEventObject());
    if (pEdit == NULL){
        return ;
    }
    int id = mpBufferList->GetPageIndex(pEdit);
    if (id == wxNOT_FOUND){
        return;
    }

    wxString filename = pEdit->GetFilename();
    if (!filename.empty()){
        wxString name, ext;
        wxFileName::SplitPath(filename, NULL, NULL, &name, &ext);
        if (!ext.empty()){
            name += ".";
            name += ext;
        }
        mpBufferList->SetPageText(id, name);
        mpBufferList->SetPageToolTip(id, filename);
        // update the workspace
        if (NULL != mpWorkSpace){
            mpWorkSpace->UpdateTagForFile(filename);
        }
        mpBufferSelect->ClearBuffer();
        for (int i = 0; i < mpBufferList->GetPageCount(); i++){
            name = mpBufferList->GetPageText(i);
            pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(i));
            if (NULL != pEdit){
                mpBufferSelect->AddBuffer(name, pEdit->GetFilename());
            }
        }
    }   
}

void MyFrame::OnFileUnModified(wxStyledTextEvent &evt)
{
    if (NULL == mpBufferList){
        return;
    }
    int id = mpBufferList->GetPageIndex(dynamic_cast<wxWindow*>(evt.GetEventObject()));
    if (id == wxNOT_FOUND){
        return;
    }
    wxString title = mpBufferList->GetPageText(id);
    int pos = title.find("*");
    if (pos == 0){
        title = title.substr(1);
    }
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
    ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(select));
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
        SetActiveEdit(NULL);
    }
}

bool MyFrame::ShowMiniBuffer(const wxString &name, bool bHide)
{
    wxAuiPaneInfo &info = m_mgr.GetPane(name);
    if (!info.IsOk()){
        return false;
    }
    if (info.IsShown() && bHide){
        info.Hide();
        m_mgr.Update();
        return true;
    }
    else if ((!info.IsShown()) && (!bHide)){
        info.Show();
        m_mgr.Update();
        return true;
    }
    return false;
}

void MyFrame::OnShowSymbolList(wxCommandEvent &evt)
{
    int line = -1;
    int select = mpBufferList->GetSelection();
    ceEdit *pEdit = NULL;
    if (wxNOT_FOUND != select || (mbLoadFinish)){
        pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(select));
        if (NULL != pEdit){
            line = pEdit->GetCurrentLine();
        }
    }
    
    wxAuiPaneInfo &pane = m_mgr.GetPane(wxT("Find Symbol"));
    if (pane.IsOk()){
        pane.Show();
    }
    else{
        // add file search
        mpSymbolSearch = new wxSymbolSearch(this);
        if (NULL == mpSymbolSearchHandler){
            mpSymbolSearchHandler = new MySymbolSearchHandler(this);
        }
        mpSymbolSearch->AddHandler(mpSymbolSearchHandler);
        m_mgr.AddPane(mpSymbolSearch, wxAuiPaneInfo().Name(wxT("Find Symbol")).Caption(wxT("Find Symbol..."))
                      .Right().CloseButton(false).Row(1).BestSize(wxSize(300,200)).PaneBorder(false).MinSize(wxSize(300,100)));
    }
    m_mgr.Update();
    if (NULL != mpSymbolSearch && mbLoadFinish){
        mpSymbolSearch->Reset();
        if (NULL != pEdit){
            mpSymbolSearch->SetLanguage(pEdit->GetLanguage());
            mpSymbolSearch->SetFileName(pEdit->GetFilename());
        }
        mpSymbolSearch->SetEdit(pEdit);
        // note:fanhongxuan@gmail.com
        // select the candidate by the current position
        // wxPrintf("SetInput:%s\n", value);
        // mpSymbolSearch->SetInput(value);
        mpSymbolSearch->SetInput("");
		mpSymbolSearch->UpdateSearchList("");
        mpSymbolSearch->SetFocus();
    }
}

void MyFrame::OnShowSearch(wxCommandEvent &evt)
{
#define VALID_CHAR_WHEN_SEARCH_FILE "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"    
    wxString value;
    int line = -1;
    int select = mpBufferList->GetSelection();
    ceEdit *pEdit = NULL;
    if (wxNOT_FOUND != select && mbLoadFinish){
        pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(select));
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
    m_mgr.Update();
	if (NULL != mpSearch && mbLoadFinish){
        if (!value.empty()){
            // note:fanhongxuan@gmail.com
            // select the candidate by the current position
            mpSearch->SetEdit(pEdit);
            mpSearch->SetInput(value);
        }
        mpSearch->SetFocus();
    }
}

void MyFrame::OnShowFindFiles(wxCommandEvent &evt)
{
#define VALID_CHAR_WHEN_SEARCH_DIR "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890/."     
    wxString value;
    int select = mpBufferList->GetSelection();
    if (wxNOT_FOUND != select && mbLoadFinish){
        ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(select));
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
    m_mgr.Update();
    if (NULL != mpSearchDir && mbLoadFinish){
        UpdateWorkDirs(mpActiveEdit);
        mpSearchDir->SetFocus();
    }
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
                mpFrame->ShowMiniBuffer("BufferSelect", true);
            }
            if (ret.Content() == wxString::Format(wxT("New %s"), ret.Target())){
                if (bActive){
                    mpFrame->OpenFile(ret.Target(), "", bActive);
                }
            }
            else{
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
    ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(select));
    if (NULL != pEdit && pEdit->HasFocus()){
        // if (!pEdit->Getfilename().empty()){
        // }
        pEdit->SaveFile(false);
    }
}

void MyFrame::OnGotoDefine(wxCommandEvent &evt)
{
    ShowReference(0);
}

void MyFrame::OnShowReference(wxCommandEvent &evt)
{
    ShowReference(1);
}

void MyFrame::OnShowGrepText(wxCommandEvent &evt)
{
    ShowReference(2);
}

void MyFrame::ShowReference(int type)
{
#define VALID_CHAR_WHEN_SEARCH_REF "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"        
    wxString value;
    int line = -1;
    int select = mpBufferList->GetSelection();
    ceEdit *pEdit = NULL;
    if (wxNOT_FOUND != select && mbLoadFinish){
        pEdit = dynamic_cast<ceEdit *>(mpBufferList->GetPage(select));
        if (NULL != pEdit){
            value = pEdit->GetCurrentWord(VALID_CHAR_WHEN_SEARCH_REF);
            line = pEdit->GetCurrentLine();
        }
    }
    
    // todo:fanhongxuan@gmail.com
    // read the tag file name from the workspace
    // if workspace is empty, notify user to add directy to the workspace.
    wxAuiPaneInfo &ref = m_mgr.GetPane(wxT("Reference"));
    if (ref.IsOk()){
        ref.Show();
    }
    else{
        mpRefSearch = new ceRefSearch(this);
        mpRefSearch->SetMinStartLen(3);
        if (NULL == mpRefSearchHandler){
            mpRefSearchHandler = new MyRefSearchHandler(this);
        }
        mpRefSearch->AddHandler(mpRefSearchHandler);
        m_mgr.AddPane(mpRefSearch, wxAuiPaneInfo().Name(wxT("Reference")).Caption(wxT("Find Ref in WorkSpace")).
                      Bottom().CloseButton(false).Row(1).BestSize(wxSize(200, 500)).PaneBorder(false).MinSize(wxSize(200,200)));
    }
    if (NULL != mpRefSearch && mbLoadFinish){
        if (NULL != mpWorkSpace){
            std::set<wxString> dirs;
            mpWorkSpace->GetDirs(dirs);
            if (type == 0){ // define
                mpRefSearch->SetGrep(false);
                mpRefSearch->SetHasRef(false);
            }
            else if (type == 1){ // ref
                mpRefSearch->SetGrep(false);
                mpRefSearch->SetHasRef(true);
            }
            else{
                mpRefSearch->SetGrep(true);
                mpRefSearch->SetHasRef(false);
            }
            mpRefSearch->SetTagDir(dirs);
        }
        if (!value.empty()){
            mpRefSearch->SetInput(value);
        }
        mpRefSearch->SetFocus();
    }
}

void MyFrame::OnShowAgSearch(wxCommandEvent &evt)
{
    // todo:fanhongxuan@gmail.com
    // how to monitor the status after load persistive
#define VALID_CHAR_WHEN_SEARCH_FILE "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"    
    wxString value;
    int line = -1;
    int select = mpBufferList->GetSelection();
    if (wxNOT_FOUND != select && mbLoadFinish){
        ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(select));
        if (NULL != pEdit){
            value = pEdit->GetCurrentWord(VALID_CHAR_WHEN_SEARCH_FILE);
            line = pEdit->GetCurrentLine();
        }
    }
    
    wxAuiPaneInfo &ag = m_mgr.GetPane(wxT("AGSearch"));
    if (ag.IsOk()){
        ag.Show();
    }
    else{
        mpAgSearch = new wxAgSearch(this);
        mpAgSearch->SetMinStartLen(3);
        if (NULL == mpAgSearchHandler){
            mpAgSearchHandler = new MyAgSearchHandler(this);
        }
        mpAgSearch->AddHandler(mpAgSearchHandler);
        m_mgr.AddPane(mpAgSearch, wxAuiPaneInfo().Name(wxT("AGSearch")).Caption(wxT("Search In Dir")).
                      Bottom().CloseButton(false).Row(1).BestSize(wxSize(200, 500)).PaneBorder(false).MinSize(wxSize(200,200)));
    }
    m_mgr.Update();    
    if (NULL != mpAgSearch && mbLoadFinish){
        if (!value.empty()){
            mpAgSearch->SetInput(value);
        }
        UpdateWorkDirs(mpActiveEdit);
        mpAgSearch->SetFocus();
    }
}

void MyFrame::AddDirToWorkSpace(const wxString &dir){
    if (NULL != mpWorkSpace){
        mpWorkSpace->AddDirToWorkSpace(dir);
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
    m_mgr.Update();
    if (NULL != mpWorkSpace && mbLoadFinish){
        UpdateWorkDirs(mpActiveEdit);
        mpWorkSpace->SetFocus();
    }
}

void MyFrame::OnShowExplorer(wxCommandEvent &evt)
{
    wxPrintf("OnShowExplorer\n");
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
    m_mgr.Update();
    if (NULL != mpExplorer && mbLoadFinish){
        UpdateWorkDirs(mpActiveEdit);
        mpExplorer->SetFocus();
    }
}

void MyFrame::OnShowExplorerOfCurrentFile(wxCommandEvent &evt)
{
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
    m_mgr.Update();
    if (NULL != mpExplorer && mbLoadFinish){
        if (NULL != mpActiveEdit){
            wxString filename = mpActiveEdit->GetFilename();
            int pos = filename.find_last_of("/\\");
            if (pos != filename.npos){
                mpExplorer->SetCwd(filename.substr(0, pos));
            }
        }
        UpdateWorkDirs(mpActiveEdit);
        mpExplorer->SetFocus();
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
                ceEdit *pEdit = dynamic_cast<ceEdit*>(mpBufferList->GetPage(i));
                if (NULL != pEdit){
                    mpBufferSelect->AddBuffer(mpBufferList->GetPageText(i), pEdit->GetFilename());
                }
            }
        }
        m_mgr.AddPane(mpBufferSelect, wxAuiPaneInfo().Name(wxT("BufferSelect")).Caption(wxT("Select buffers..."))
                      .Bottom().CloseButton(false).Row(1).BestSize(wxSize(300,200)).PaneBorder(false).MinSize(wxSize(300,100)));
    }
    m_mgr.Update();    
    if (NULL != mpBufferSelect && mbLoadFinish){
#ifdef WIN32
		// note:fanhongxuan@gmail.com
		// on gtk, when the input first get the focus, will generate a wxevt_text event which will call UpdateSearchList
		// on msw, we need to do it here to make sure will show all the candidate of buffer select.
        mpBufferSelect->UpdateSearchList("", false);
#endif
        mpBufferSelect->SetFocus();
    }
}

void MyFrame::OnShowOneWindow(wxCommandEvent &evt)
{
    wxAuiPaneInfoArray &panes = m_mgr.GetAllPanes();
    int i = 0;
    for (i = 0; i < panes.GetCount(); i++){
        wxAuiPaneInfo &info = panes[i];
        if (info.IsOk()){
            if (info.name == wxT("Main") || info.name == wxT("Cmd")){
                continue;
            }
            info.Hide();
        }
    }
    m_mgr.Update();
    SwitchFocus();
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
    else if (NULL != mpWorkSpace && mpWorkSpace->IsDescendant(window)){
        window = mpWorkSpace;
    }
    else if (NULL != mpExplorer && mpExplorer->IsDescendant(window)){
        window = mpExplorer;
    }
    else if (NULL != mpAgSearch && mpAgSearch->IsDescendant(window)){
        window = mpAgSearch;
    }
    else if (NULL != mpSymbolSearch && mpSymbolSearch->IsDescendant(window)){
        window = mpSymbolSearch;
    }
    else if (NULL != mpRefSearch && mpRefSearch->IsDescendant(window)){
        window = mpRefSearch;
    }
    else if (NULL != mpCompile && mpCompile->IsDescendant(window)){
        window = mpCompile;
    } 
    else if (NULL != mpCmd && mpCmd->IsDescendant(window)){
        // note:fanhongxuan@gmail.com
        // the cmd window can not be killed.
        return;
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
            ceEdit *pEdit = dynamic_cast<ceEdit *>(mpBufferList->GetPage(select));
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
    if (NULL != mpSymbolSearch){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpSymbolSearch);
        if (info.IsOk() && info.IsShown()){
            wxPrintf("Change focus to search symbol\n");
            mpSymbolSearch->SetFocus();
            return;
        }
    }
    if (NULL != mpAgSearch){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpAgSearch);
        if (info.IsOk() && info.IsShown()){
            wxPrintf("Change focus to Ag Search\n");
            mpAgSearch->SetFocus();
            return;
        }
    }
    if (NULL != mpRefSearch){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpRefSearch);
        if (info.IsOk() && info.IsShown()){
            wxPrintf("Change focus to ref Search\n");
            mpRefSearch->SetFocus();
            return;
        }
    }
    if (NULL != mpCompile){
        wxAuiPaneInfo &info = m_mgr.GetPane(mpCompile);
        if (info.IsOk() && info.IsShown()){
            wxPrintf("Change focus to compile\n");
            mpCompile->SetFocus();
            return;
        }
    }
    if (NULL != mpCmd){
        wxPrintf("Change focus to cmd\n");
        mpCmd->SetFocus();
    }
}
