#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/srchctrl.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/string.h>
#include <wx/html/htmlcell.h>
#include <wx/html/htmlwin.h>
#include <wx/filesys.h>
#include <wx/filefn.h>
#include <wx/dir.h>
#include <wx/wxcrtvararg.h> // for wxPrintf
#include <wx/stattext.h>
#include "ceEdit.hpp"
#ifndef WIN32
#include <sys/time.h>
#endif
#include <wx/timer.h>
#include "wxSearch.hpp"
#include "ce.hpp"
#include "ceUtils.hpp"
#include <stdio.h>

#include <map>
#include <vector>

//#define DEBUG_FOLD

class wxMyTimeTrace
{
public:
    wxMyTimeTrace(const wxString desc)
        :mDesc(desc)
    {
#ifndef WIN32
        gettimeofday(&mStart, NULL);
#endif
    }
    ~wxMyTimeTrace()
    {
#ifndef WIN32
        struct timeval end;
        gettimeofday(&end, NULL);
        long delta = (end.tv_sec - mStart.tv_sec) * 1000 + (end.tv_usec - mStart.tv_usec)/1000;
        wxPrintf("%s used %ld ms\n", mDesc, delta);
#endif
    }
private:
#ifndef WIN32
    struct timeval mStart;
#endif
    wxString mDesc;
};


class wxSearchListCtrl: public wxStyledTextCtrl
{
private:
    wxSearch *mpParent;
    wxString mKey;
    wxDECLARE_EVENT_TABLE();
public:
    wxSearchListCtrl(wxSearch *parent, wxSize size);
    
    void OnStyleNeeded(wxStyledTextEvent &evt);
    void OnMarginClick(wxStyledTextEvent &evt);
    
    void OnLeftDown(wxMouseEvent &evt);
    void OnKeyDown(wxKeyEvent &evt);
    void SetKey(const wxString &input){mKey = input;}
    void ListUpdated();
    
    bool IsGroupHeader(const wxString &input, long line);
    bool IsGroupEnder(const wxString &input, long line);
    
    int CalcFoldLevel(int type, int line);
    
    virtual bool AcceptsFocus() const {return false;}
    virtual bool AcceptsFocusFromKeyboard() const {return false;}
    virtual bool AcceptsFocusRecursively() const {return false;}
    enum{
        STYLE_NORMAL,
        STYLE_HIGHLIGHT,
        STYLE_GROUP_BEGIN,
        STYLE_GROUP_END,
        STYLE_INDEX,
    };
};

wxBEGIN_EVENT_TABLE(wxSearchListCtrl, wxStyledTextCtrl)
EVT_LEFT_DOWN(wxSearchListCtrl::OnLeftDown)
EVT_STC_STYLENEEDED(wxID_ANY, wxSearchListCtrl::OnStyleNeeded)
EVT_LEFT_DCLICK(wxSearchListCtrl::OnLeftDown)
EVT_KEY_DOWN(wxSearchListCtrl::OnKeyDown)
EVT_STC_MARGINCLICK(wxID_ANY, wxSearchListCtrl::OnMarginClick)
wxEND_EVENT_TABLE()

wxSearchListCtrl::wxSearchListCtrl(wxSearch *parent, wxSize size)
:wxStyledTextCtrl(parent, wxID_ANY, wxDefaultPosition, size)
{
    StyleClearAll();
    
    wxFont font(wxFontInfo(11).Family(wxFONTFAMILY_MODERN));
    StyleSetFont(wxSTC_STYLE_DEFAULT, font);
    
    SetCaretStyle(wxSTC_CARETSTYLE_BLOCK);
    SetCaretForeground(wxColor(wxT("RED")));
    SetCaretLineBackground(wxColour(193, 213, 255));
    SetCaretLineBackAlpha(60);
    SetCaretLineVisible(true);
    SetCaretLineVisibleAlways(true);
    
    // set the default style
    StyleSetForeground(wxSTC_STYLE_DEFAULT, *wxWHITE);
    StyleSetBackground(wxSTC_STYLE_DEFAULT, *wxBLACK);
    
    StyleSetForeground(STYLE_NORMAL, *wxWHITE); // normal
    StyleSetBackground(STYLE_NORMAL, *wxBLACK);
    StyleSetFont(STYLE_NORMAL, font);
    
    StyleSetForeground(STYLE_HIGHLIGHT, *wxRED);  // highlight
    StyleSetBackground(STYLE_HIGHLIGHT, *wxBLACK);
    StyleSetFont(STYLE_HIGHLIGHT, font);
    
    StyleSetForeground(STYLE_GROUP_BEGIN, *wxGREEN); // Fold Header
    StyleSetBackground(STYLE_GROUP_BEGIN, *wxBLACK);
    StyleSetFont(STYLE_GROUP_BEGIN, font);
    
    StyleSetForeground(STYLE_GROUP_END, *wxGREEN); // Fold ender
    StyleSetBackground(STYLE_GROUP_END, *wxBLACK);
    StyleSetFont(STYLE_GROUP_END, font);
    
    StyleSetForeground(STYLE_INDEX, wxColor("orange"));
    StyleSetBackground(STYLE_INDEX, *wxBLACK);
    StyleSetFont(STYLE_INDEX, font);
    
    // 0 is used as line number
    SetMarginType (0, wxSTC_MARGIN_NUMBER);
    StyleSetForeground (wxSTC_STYLE_LINENUMBER, wxColour (wxT("GREEN")));
    StyleSetBackground (wxSTC_STYLE_LINENUMBER, *wxBLACK);
    wxString linenumber = "999";
    SetMarginWidth(0, TextWidth (wxSTC_STYLE_LINENUMBER, linenumber));
    
    // setup about folder
    SetMarginType (1, wxSTC_MARGIN_SYMBOL);
    SetMarginMask (1, wxSTC_MASK_FOLDERS);
    SetMarginWidth (1, 16);
    SetMarginSensitive (1, true);
    SetFoldMarginColour(true, *wxBLACK);
    SetFoldMarginHiColour(true, *wxBLACK);
    MarkerDefine(wxSTC_MARKNUM_FOLDER,        wxSTC_MARK_BOXPLUS, wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN,    wxSTC_MARK_BOXMINUS,  wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDERSUB,     wxSTC_MARK_VLINE,     wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDEREND,     wxSTC_MARK_BOXPLUSCONNECTED, wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED, wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER,     wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL,    wxSTC_MARK_LCORNER,     wxT("BLACK"), wxT("WHITE"));
    
    SetProperty (wxT("fold"), wxT("1"));
    SetProperty (wxT("fold.compact"), wxT("1"));
    
    SetFoldFlags (wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | 
#ifdef DEBUG_FOLD        
        wxSTC_FOLDFLAG_LEVELNUMBERS |
#endif        
        wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);
    
    SetLexer(wxSTC_LEX_CONTAINER);
    // setup about the 
    mpParent = parent;
}

void wxSearchListCtrl::OnLeftDown(wxMouseEvent &evt)
{
    if (NULL == mpParent){
        return;
    }
    long pos = -1;
    HitTest(evt.GetPosition(), &pos);
    if (pos >= 0){
        long x = 0, y = 0;
        PositionToXY(pos, &x, &y);
        mpParent->SelectLine(y, true, wxEVT_LEFT_DOWN == evt.GetEventType());
        //evt.StopPropagation();
    }
    evt.Skip();
}

void wxSearchListCtrl::OnKeyDown(wxKeyEvent &evt)
{
    if (WXK_RETURN == evt.GetKeyCode() || WXK_UP == evt.GetKeyCode() || WXK_DOWN == evt.GetKeyCode()){
        if (NULL != mpParent && mpParent->OnKey(evt)){
        }
    }
    evt.StopPropagation();
}

bool wxSearchListCtrl::IsGroupHeader(const wxString &input, long line){
    if (NULL != mpParent){
        bool isHeader = false;
        bool hasResult = mpParent->IsGroupHeader(input, line, isHeader);
        if (hasResult){
            return isHeader;
        }
    }
    if (input.find("{") == 0){
        return true;
    }
    return false;
}

bool wxSearchListCtrl::IsGroupEnder(const wxString &input, long line){
    if (NULL != mpParent){
        bool isEnd = false;
        bool hasResult = mpParent->IsGroupEnder(input, line, isEnd);
        if (hasResult){
            return isEnd;
        }
    }
    if (input.find("}") == 0){
        return true;
    }
    return false;
}

int wxSearchListCtrl::CalcFoldLevel(int type, int line)
{
    int prevLevel = wxSTC_FOLDLEVELBASE;
    if (line == 0){
        if (type == STYLE_GROUP_BEGIN){
            prevLevel |= wxSTC_FOLDLEVELHEADERFLAG;
        }
        return prevLevel;
    }
    
    prevLevel = GetFoldLevel(line - 1);
    if (prevLevel & wxSTC_FOLDLEVELHEADERFLAG){
        prevLevel++;
    }
    prevLevel = prevLevel & wxSTC_FOLDLEVELNUMBERMASK;
    if (STYLE_GROUP_BEGIN == type){
        prevLevel |= wxSTC_FOLDLEVELHEADERFLAG;
    }
    else if (STYLE_GROUP_END == type){
        if (prevLevel > wxSTC_FOLDLEVELBASE){
            prevLevel--;
        }
    }
    else{
        // normal line, keep the prevLevel
    }
    return prevLevel;
}

void wxSearchListCtrl::OnStyleNeeded(wxStyledTextEvent &evt)
{
    int startPos = GetEndStyled();
    int stopPos = evt.GetPosition();
    int startLine = LineFromPosition(startPos);
    int stopLine = LineFromPosition(stopPos);
    
    int foldLevel = wxSTC_FOLDLEVELBASE;
    
    startPos = XYToPosition(0, startLine);
    stopPos = GetLineEndPosition(stopLine);
    StartStyling(startPos);
    SetStyling(stopPos - startPos, STYLE_NORMAL);
    // wxPrintf("OnStyleNeeded:<%d-%d>\n", startLine, stopLine);
    std::vector<wxString> rets;
    ceSplitString(mKey, rets, ' ');
    
    for (int j = startLine; j <= stopLine; j++ ){
        startPos = XYToPosition(0, j);
        stopPos = GetLineEndPosition(j);
        wxString text = GetLineText(j);
        if (IsGroupHeader(text, j)){
            SetFoldLevel(j,CalcFoldLevel(STYLE_GROUP_BEGIN, j));
            StartStyling(startPos);
            SetStyling(text.length(), STYLE_GROUP_BEGIN);
        }
        else if (IsGroupEnder(text, j)){
            SetFoldLevel(j, CalcFoldLevel(STYLE_GROUP_END, j));
            StartStyling(startPos);
            SetStyling(text.length(), STYLE_GROUP_END);
        }
        else{
            SetFoldLevel(j, CalcFoldLevel(STYLE_NORMAL, j));
        }
        wxString lowText = text.Lower();
        
        int pos = text.find(":");
        if (pos != text.npos){
            StartStyling(startPos);
            SetStyling(pos+1, STYLE_INDEX);
        }
        
        for (int i = 0; i < rets.size(); i++ ){
            int start = 0;
            int length = text.length();
            wxString low = rets[i].Lower();
            bool bCaseSenstive = (low != rets[i]);
            while(start < length){
                if (bCaseSenstive){
                    start = text.find(rets[i], start);
                }
                else{
                    start = lowText.find(low, start);
                }
                if (start == text.npos){
                    break;
                }
                StartStyling(startPos + start);
                SetStyling(rets[i].length(), STYLE_HIGHLIGHT);
                start+= rets[i].length();
            }
        }
    }
    
    // note:fanhongxuan@gmail.com
    // move the style end to the end.
    StartStyling(stopPos);
    SetStyling(0, STYLE_NORMAL);
}

void wxSearchListCtrl::OnMarginClick(wxStyledTextEvent &evt)
{
    if (evt.GetMargin() == 1) {
        int lineClick = LineFromPosition (evt.GetPosition());
        int levelClick = GetFoldLevel (lineClick);
        if ((levelClick & wxSTC_FOLDLEVELHEADERFLAG) > 0) {
            ToggleFold (lineClick);
        }
    }
}

void wxSearchListCtrl::ListUpdated()
{
    Clear(); // Clear selection;
    int lineCount = GetLineCount();
    wxString linenumber = wxT("_");
    while(lineCount != 0){
        linenumber += wxT("9");
        lineCount = lineCount / 10;
    }
#ifdef DEBUG_FOLD    
    linenumber = "99999999999";
#endif
    SetMarginWidth(0, TextWidth (wxSTC_STYLE_LINENUMBER, linenumber));
    GotoLine(0);
}

#define wxSearchInputCtrlBase wxTextCtrl
class wxSearchInputCtrl: public wxSearchInputCtrlBase
{
private:
    wxSearch *mpParent;
    wxSearchListCtrl *mpList;
    wxTimer *mpTimer;
    wxString mPrevValue;
    bool mbEmptyUpdate;
public:
    wxSearchInputCtrl(wxSearch *parent)
    :wxSearchInputCtrlBase(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 25), wxTE_RICH|wxTE_MULTILINE), 
    mpParent(parent), mpList(NULL), mpTimer(NULL), mbEmptyUpdate(false){
        SetBackgroundColour(*wxBLACK);
        SetForegroundColour(*wxWHITE);
        wxTextAttr white(*wxWHITE);
        SetDefaultStyle(white);
        SetScrollbar(wxVERTICAL, 0, 1, 1);
    }
    void SetList(wxSearchListCtrl *pList){mpList = pList;}
    void Reset()
    {
        //mbEmptyUpdate = false;
        mPrevValue = wxEmptyString;
    }
    
    virtual bool ProcessEvent(wxEvent &evt)
    {
        // note:fanhongxuan@gmail.com
        // If we use wxSearchCtrl, we can't receive the wxEVT_KEY_DOWN, wxEVT_KEY_UP evt.
        // So use wxEVT_CHAR_HOOK.
        // If use wxTextCtrl instead of wxSearchCtrl, we can use wxEVT_KEY_DOWN here.
        if (wxEVT_CHAR_HOOK == evt.GetEventType()){
            wxKeyEvent *pEvt = dynamic_cast<wxKeyEvent*>(&evt);
            if (NULL != pEvt){
                if (WXK_UP == pEvt->GetKeyCode() ||
                    WXK_DOWN == pEvt->GetKeyCode()|| 
                    WXK_RETURN == pEvt->GetKeyCode()){
                    // note:fanhongxuan@gmail.com
                    // the default process of wxk_up and wxk_down will switch the keyboard focus,
                    // we don't want it, so directly process it here, and stop propagation to our parent.
                    mpParent->OnKey(*pEvt);
                    evt.StopPropagation();
                    return true;
                }
            }
        }
        else if (wxEVT_TEXT_CUT == evt.GetEventType() || wxEVT_TEXT_PASTE == evt.GetEventType()){
            bool ret = wxSearchInputCtrlBase::ProcessEvent(evt);
            if (NULL != mpParent){
                mpParent->Reset();
                mpParent->UpdateSearchList(GetValue());
            }
            return ret;
        }
        else if (wxEVT_TIMER == evt.GetEventType()){
            if (NULL != mpParent){
                //wxPrintf("Timer out:<%s>\n", GetValue());
                mpParent->UpdateSearchList(GetValue());
            }
        }
        else if (wxEVT_TEXT == evt.GetEventType()){
            // note:fanhongxuan@gmail.com
            // the UpdateSearchList is started after user stop type 100 ms.
            // if the GetValue() && mPrevValue is both empty, let it do one
            if ((GetValue() != mPrevValue) || (GetValue().IsEmpty() && mbEmptyUpdate == false)){
                mPrevValue = GetValue();
                mbEmptyUpdate = mPrevValue.IsEmpty();
                if (NULL == mpTimer){
                    mpTimer = new wxTimer(this, wxID_ANY);
                }
                else{
                    mpTimer->Stop();
                }
                mpTimer->StartOnce(300);
            }
            //mpParent->UpdateSearchList(GetValue());
        }
        else if (wxEVT_MOUSEWHEEL == evt.GetEventType()){
            wxMouseEvent *pEvt = dynamic_cast<wxMouseEvent*>(&evt);
            if (NULL != mpList && NULL != pEvt){
                mpList->ScrollLines(-pEvt->GetWheelDelta()/pEvt->GetWheelRotation());
            }
        }
        else if (wxEVT_SET_FOCUS == evt.GetEventType()){
            if (NULL != wxGetApp().frame()){
                // note:fanhongxuan@gmail.com
                // every setfocus call in wxSearchInputCtrl will call this.
                wxGetApp().frame()->DoUpdate();
            }
        }
        return wxSearchInputCtrlBase::ProcessEvent(evt);
    }
};

wxSearchResult::wxSearchResult(const wxString &content, const wxString &target, void *pCustomData, bool bNeedFilter)
    :mContent(content), mTarget(target), mpCustomData(pCustomData), mbNeedFilter(bNeedFilter)
{
    mbMatch = false;
    mLowContent = mContent.Lower();
}

bool wxSearchResult::ConvertToRichText(wxSearch *pSearch, const wxString &input, wxSearchListCtrl &rich,
                                       const std::vector<wxString> &rets, bool bEnableHighlight)
{
    // this result is not need filter, so directly return false;
    if (!mbNeedFilter){
        mbMatch = false;
        if (NULL != pSearch){
            pSearch->BeforeResultMatch(input, this);
        }
        if (rich.GetLastPosition() != 0){
            rich.WriteText("\n");
        }
        mRange.SetStart(rich.GetLastPosition());
        rich.WriteText(mContent);
        mRange.SetEnd(rich.GetLastPosition());
        return false;
    }
    int i = 0;    
    mbMatch = false;
    for (i = 0; i < rets.size(); i++ ){
        wxString low = rets[i].Lower();
        bool bCaseSenstive = (low != rets[i]);
        int pos = mContent.npos;
        if (bCaseSenstive){
            pos = mContent.find(rets[i]);
        }
        else{
            pos = mLowContent.find(low);
        }
        if (pos == wxString::npos){
            return false;
        }
    }
    mbMatch = true;
    if (NULL != pSearch){
        pSearch->BeforeResultMatch(input, this);
    }
    if (rich.GetLastPosition() != 0){
        rich.WriteText("\n");
    }
    mRange.SetStart(rich.GetLastPosition());
    rich.WriteText(mContent);
    mRange.SetEnd(rich.GetLastPosition());
    return true;
}

wxSearch::wxSearch(wxWindow *pParent)
    :wxPanel(pParent), mCurrentLine(-1), mMinStartLen(1), mMaxCandidate(10000),
     mbStartSearch(false)
{
    wxSizer *pSizer = new wxBoxSizer(wxVERTICAL);
    mpInput = new wxSearchInputCtrl(this);
    wxSizerFlags flags;
    flags.Proportion(0);
    flags.Border(wxALL, 0);
    flags.Expand();
    pSizer->Add(mpInput, flags);

    mpList = new wxSearchListCtrl(this, wxSize(200, 400));
    mpInput->SetList(mpList);
    flags.Proportion(1);
    pSizer->Add(mpList, flags);
    
    mpStatus = new wxStaticText(this, wxID_ANY, wxEmptyString);
    flags.Proportion(0);
    pSizer->Add(mpStatus, flags);

    SetSizer(pSizer);
    
    Reset();
}

wxSearch::~wxSearch()
{
    Reset();
}

bool wxSearch::SetMinStartLen(int minStartLen)
{
    mMinStartLen = minStartLen;
    if (mMinStartLen < 0){
        mMinStartLen = 1;
    }
    return true;
}    

bool wxSearch::SetMaxCandidate(int maxCandidate)
{
    mMaxCandidate = maxCandidate;
    if (maxCandidate <= 0){
        mMaxCandidate = 100;
    }
    return true;
}

bool wxSearch::SetInput(const wxString &input)
{
    Reset();
    if (NULL != mpInput){
        mpInput->SetValue(input);
        mpInput->SetSelection(0, input.size());
    }
    return true;
}

bool wxSearch::SelectLine(int line, bool bActive, bool bRequestFocus)
{
    int number = mpList->GetNumberOfLines();
    if (number <= 0){
        mCurrentLine = -1;
        return false;
    }
    mCurrentLine = line;
    int pos = mpList->GetLineIndentPosition(line);
    mpList->GotoLine(line);
    
    int i = 0;
    if (bActive){
        for (i = 0; i < mResults.size(); i++){
            if (NULL != mResults[i] && mResults[i]->IsMatch()){
                if (mResults[i]->IsInRange(pos)){
                    int j = 0;
                    for (j = 0; j < mHandlers.size(); j++){
                        if (NULL != mHandlers[j]){
                            mHandlers[j]->OnSelect(*mResults[i], !bRequestFocus);
                        }
                    }
                }
            }
        }
        for (i = 0; i < mTempResults.size(); i++){
            if (NULL != mTempResults[i] && mTempResults[i]->IsMatch()){
                if (mTempResults[i]->IsInRange(pos)){
                    int j = 0;
                    for (j = 0; j < mHandlers.size(); j++){
                        if (NULL != mHandlers[j]){
                            mHandlers[j]->OnSelect(*mTempResults[i], !bRequestFocus);
                        }
                    }
                }
            }
        }
    }
    if ((!mpInput->HasFocus()) && bRequestFocus){
        long start, end;
        mpInput->GetSelection(&start, &end);
        mpInput->SetFocus(); // this maybe generate a text-update event on gtk
        mpInput->SetSelection(start, end);
    }
    return true;
}

bool wxSearch::OnKey(wxKeyEvent &evt)
{
    // note:fanhongxuan@gmail.com
    // use UP/DOWN to change the selection but DO NOT active it.
    // when press CTRL+ENTER, if no selection, select the first one and auto active it.
    // else select the next one and active it.
    // when press ALT+ENTER, if no selection, select the first one and auto active it.
    // else select hte previous one and active it.
    // ENTER: if no selection, select the first one,
    //        else active and switch to the selected one.
    int key = evt.GetKeyCode();
    if (key == WXK_UP || (key == WXK_RETURN && evt.ControlDown())){
        mCurrentLine--;
        if (mCurrentLine < 0){
            mCurrentLine = mpList->GetNumberOfLines() - 1;
        }
        SelectLine(mCurrentLine, evt.ControlDown());
        return true;
    }
    else if (key == WXK_DOWN || (key == WXK_RETURN && evt.AltDown())){
        mCurrentLine++;
        if (mCurrentLine >= mpList->GetNumberOfLines()){
            mCurrentLine = 0;
        }
        SelectLine(mCurrentLine, (key == WXK_RETURN && evt.AltDown()) || (key == WXK_DOWN &&evt.ControlDown()));
        return true;
    }
    else if (key == WXK_RETURN && (!evt.ControlDown()) && (!evt.AltDown())){
        if (mCurrentLine >= mpList->GetNumberOfLines() || mCurrentLine < 0){
            mCurrentLine = 0;
        }
        SelectLine(mCurrentLine, true, false /*bRequestFocus == false */);

    }
    return false;
}

wxString wxSearch::GetSummary(const wxString &input, int matchCount)
{
    return wxString::Format("Find '%s', %d match", input, matchCount);
}

wxString wxSearch::GetShortHelp() const
{
    return "UP/DOWN to select, CTRL/ALT+ENTER to select and active, ENTER to active and switch";
}

wxString wxSearch::GetHelp() const
{
    // todo:fanhongxuan@gmail.com
    return GetShortHelp();
}

bool wxSearch::StopSearch()
{
    return true;
}

void wxSearch::ClearTempResult()
{
    int i = 0;
    for (i = 0; i < mTempResults.size(); i++){
        wxSearchResult *pRet = mTempResults[i];
        if (NULL != pRet){
            delete pRet;
        }
    }
    mTempResults.clear();
}

void wxSearch::ResetSearch()
{
    Reset();
    if (NULL != mpInput){
        mpInput->SetValue("");
    }
}

void wxSearch::Reset()
{
    int i = 0;
    for (i = 0; i < mResults.size(); i++){
        wxSearchResult *pRet = mResults[i];
        if (NULL != pRet){
            delete pRet;
        }
    }
    mpInput->Reset();
    mInput = wxEmptyString;
    mCount = 0;
    mpStatus->SetLabel(GetShortHelp());
    mKeys.clear();
    mResults.clear();
    mpList->ClearAll();
    mbStartSearch = false;
}

void wxSearch::AsyncAddSearchResult(wxSearchResult *pResult)
{
    if (NULL == pResult){
        return;
    }
    mResults.push_back(pResult);
    if (mCount >= mMaxCandidate){
        return;
    }
    mpList->SetInsertionPointEnd();
    if (pResult->ConvertToRichText(this, mInput, *mpList, mKeys, mCount <= mMaxCandidate)){
        // wxPrintf("AsyncAddSearchResult:<%s>\n", pResult->Content());
        mCount++;
        AfterResultMatch(mInput, pResult);
    }
    mpList->SetInsertionPoint(0);
    mpStatus->SetLabel(GetSummary(mInput, mCount));

    if (mCurrentLine < 0){
        int prefer = GetPreferedLine(mInput);
        if (prefer >= 0){
            SelectLine(prefer, true, true);
        }
        // no selection, getPreferedSelection
    }
}

void wxSearch::BeginGroup(const wxString &title, bool updateNow)
{
    wxSearchResult *pResult = new wxSearchResult("{" + title, title, NULL, false);
    if (updateNow){
        if (mpList->GetLastPosition() != 0){
            mpList->WriteText("\n");
        }
        mpList->WriteText(pResult->Content());
        delete pResult;
    }
    else{
        AddSearchResult(pResult);
    }
}

void wxSearch::EndGroup(const wxString &title, bool updateNow)
{
    wxSearchResult *pResult = new wxSearchResult("}" + title, title, NULL, false);
    if (updateNow){
        if (mpList->GetLastPosition() != 0){
            mpList->WriteText("\n");
        }
        mpList->WriteText(pResult->Content());
        delete pResult;
    }
    else{
        AddSearchResult(pResult);
    }
}


void wxSearch::AddSearchResult(wxSearchResult *pResult)
{
    if (NULL != pResult){
        mResults.push_back(pResult);
    }
}

void wxSearch::OnDelItem(uint8_t index)
{
    // todo:fanhongxuan@gmail.com
}

void wxSearch::OnPercent(uint8_t percent)
{
}

bool wxSearch::AddHandler(wxSearchHandler *pHandler)
{
    if (NULL == pHandler){
        return false;
    }
    mHandlers.push_back(pHandler);
    return true;
}

bool wxSearch::DelHandler(wxSearchHandler *pHandler)
{
    int i = 0;
    bool ret = false;
    for (i = 0; i < mHandlers.size(); i++){
        if (mHandlers[i] == pHandler){
            mHandlers[i] = NULL;
            ret = true;
        }
    }
    return ret;
}

bool wxSearch::DoStartSearch(const wxString &input)
{
    // todo:fanhongxuan@gmail.com
    // move the file system access in a background thread, and send event to the main thread
    // in case if the current path has too many file which will block the main thread.
    int i = 0;
    for (i = 0; i < mHandlers.size(); i++){
        if (NULL != mHandlers[i]){
            mHandlers[i]->OnPrepareResult(input, mResults);
        }
    }
    mpStatus->SetLabel(wxT("Searching...."));
    // wxPrintf("DoStartSearch %s\n", input);
    // note:fanhongxuan@gmail.com
    // when call StartSearch we only use the first minStartLen char.
    StartSearch(input.substr(0, mMinStartLen), input);
    return true;
}

bool wxSearch::UpdateSearchList(const wxString &input, bool bRequestFocus)
{
    // fixme:fanhongxuan@gmail.com
    // on windows, then the edit is empty, will not call UpdateSearch list
    // so in buffer list, can not show all the candidate.
    int i = 0, count = 0;
    if (NULL == mpList){
        return false;
    }
    
    if (input.length() < mMinStartLen){
        // wxPrintf("input is too short:%d, %d\n", (int)input.length(), mMinStartLen);
        Reset();
        return false;
    }

    // note:fanhongxuan@gmail.com
    // if the mMinStartLen is 0
    // when the input is empty, show all the result
    if (input.find_first_not_of("\r\n\t ") == wxString::npos){
        Reset();
        if (mMinStartLen != 0){
            // wxPrintf("mMinStartLen:%d\n", mMinStartLen);
            return false;
        }
        else{
            // wxPrintf("UpdateSearchList when mMinStartLen is 0\n");
        }
    }
    std::vector<wxString> rets;
    ceSplitString(input, rets, ' ');

    // note:fanhongxuan@gmail.com
    // if all the search key is same as the previous value, directly return, do not need to update it.
    if (mMinStartLen != 0 && rets.size() == mKeys.size()){
        bool bDiff = false;
        for (i = 0; i < rets.size(); i++){
            if (rets[i] != mKeys[i]){
                bDiff = true;
                break;
            }
        }
        if (!bDiff){
            wxPrintf("Same key\n");
            return false;
        }
    }
    
    if (mMinStartLen != 0 && rets.size() > 0 && mKeys.size() > 0){
        if (mKeys[0].substr(0, mMinStartLen) != rets[0].substr(0, mMinStartLen)){
            wxPrintf("Trigger search\n");
            mbStartSearch = false;
        }
    }
    
    if (!mbStartSearch || mResults.empty()){
        Reset(); // make sure the status has been reset.
        mbStartSearch = true;
        DoStartSearch(input);
    }    
    
    mKeys = rets;
    
    // here, we start update the list
    // wxMyTimeTrace trace("UpdateSearchList");
    mpList->ClearAll();
    mpList->SetKey(input);
    mCurrentLine = -1;
    mInput = input;
    
    BeginMatch(input);
    
    for (i = 0; i < mResults.size(); i++){
        // todo:fanhongxuan@gmail.com
        // currently, if the match count is bigger than mMaxCandidate
        // we directly return, later we need to show more result.
        if (count >= mMaxCandidate){
            break;
        }
        if (mResults[i]->ConvertToRichText(this, input, *mpList, rets, count <= mMaxCandidate)){
            count++;
            AfterResultMatch(input, mResults[i]);
        }
    }
    
    // wxPrintf("UpdateSearchList:<%s>\n", input);
    // can add some temp result here.
    // for example:
    // when switch buffer
    // add a candidate like create new file
    ClearTempResult();
    for (i = 0; i < mHandlers.size(); i++){
        if (NULL != mHandlers[i]){
            mHandlers[i]->OnUpdateSearchResultAfter(input, mTempResults, count);
        }
    }
    for (i = 0; i < mTempResults.size(); i++){
        if (mTempResults[i]->ConvertToRichText(this, input, *mpList, rets, true)){
            AfterResultMatch(input, mTempResults[i]);
        }
    }
    
    mCount = count;
    // mpList->SetInsertionPoint(0);
    FinishMatch(input);
    
    mpList->ListUpdated();
    
    mpStatus->SetLabel(GetSummary(mInput, mCount));
    
    if (mCurrentLine < 0){
        int prefer = GetPreferedLine(mInput);
        bool bActive = true;
        if (prefer < 0){
            bActive = false;
            prefer = 0;
        }
        SelectLine(prefer, bActive, bRequestFocus);
    }
    return true;
}

bool wxSearch::ProcessEvent(wxEvent &evt)
{
    return wxPanel::ProcessEvent(evt);
}


wxSearchDir::wxSearchDir(wxWindow *parent)
    :wxSearch(parent)
{
}

wxString wxSearchDir::GetSummary(const wxString &input, int matchCount)
{
    wxString cwd;
    if (!mDirs.empty()){
        cwd = "'" + *mDirs.begin() + "'";
    }
    if (mDirs.size() > 1){
        cwd += "...";
    }
    wxString ret = wxString::Format("Find '%s' in %s, %d%s match", input, cwd, matchCount, 
                                    matchCount >= GetMaxCandidate() ? "+" :"");
    return ret;
}

wxString wxSearchDir::GetShortHelp() const
{
    wxString cwd;
    if (!mDirs.empty()){
        cwd = "'" + *mDirs.begin() + "'";
    }
    if (mDirs.size() > 1){
        cwd += "...";
    }
    if (cwd.empty()){
        cwd = "'" + wxGetCwd() + "'";
    }
    return wxString::Format("Find file in %s", cwd);
}

wxString wxSearchDir::GetHelp() const
{
    wxString ret = wxSearch::GetHelp();
    return ret;
}

bool wxSearch::IsTempFile(const wxString &file)
{
    if (file.empty()){
        return true;
    }
    if (file[0] == '#'){
        return true;
    }
    if (file[file.size() - 1] == '~'){
        return true;
    }
    if (file[file.size() - 1] == '#'){
        return true;
    }
    return false;
}

bool wxSearch::IsBinaryFile(const wxString &file)
{
    wxString ext;
    wxFileName::SplitPath(file, NULL, NULL, NULL, &ext);
    if (ext == "obj" || ext == "o" || ext == "pdb" || ext == "exe" || ext == "d"){
        return true;
    }
    if (ext == "symbol"){
        // ignore our temp file.
        return true;
    }
    return false;
}

void wxSearchDir::SetDirs(const std::set<wxString> &dirs)
{
    if (dirs.size() == mDirs.size()){
        std::set<wxString>::const_iterator it = dirs.begin();
        std::set<wxString>::const_iterator sit = mDirs.begin();
        bool bSame = true;
        while(it != dirs.end() && sit != mDirs.end()){
            if (*it != *sit){
                bSame = false;
                break;
            }
            it++;
            sit++;
        }
        if (bSame){
            return;
        }
    }
    mDirs = dirs;
    ResetSearch();
}

bool wxSearchDir::StartSearch(const wxString &input, const wxString &fullInput)
{
    // todo:fanhongxuan@gmail.com
    // skip all the file and dir list in a file named .ignore
    int i = 0;
    std::vector<wxString> files;
    if (mDirs.empty()){
        mDirs.insert(wxGetCwd());
    }
    std::set<wxString>::iterator it = mDirs.begin();
    while(it != mDirs.end()){
        files.clear();
        wxString path, name, cwd = *it;
        if (cwd.empty()){
            continue;
        }
        ceFindFiles(cwd, files);
        
        if (mDirs.size() > 1){
            int pos = cwd.find_last_of("/\\");
            if (pos != cwd.npos){
                cwd = cwd.substr(0, pos);
            }
        }
#ifdef WIN32
        if (cwd[cwd.size() - 1] != '\\'){
            cwd += "\\";
        }
#else
        if (cwd[cwd.size() - 1] != '/'){
            cwd += "/";
        }
#endif
        
        for (i = 0; i < files.size(); i++){
            path = files[i];
            int pos = path.Find(cwd);
            if (path.npos != pos){
                name = path.substr(pos + cwd.length());
            }
            else{
                name = path;
            }
            AddSearchResult(new wxSearchResult(name, path));
        }
        it++;
    }
    return true;
}

bool wxSearchDir::StopSearch()
{
    // todo:fanhongxuan@gmail.com
    // stop the background working thread.
    return true;
}

wxSearchFile::wxSearchFile(wxWindow *parent)
    :wxSearch(parent), mpEdit(NULL)
{
}

int wxSearchFile::GetPreferedLine(const wxString &input)
{
    int i = 0;
    int dis = 0;
    int line = 0;
    int bestLine = 0;
    int distance = 0;
    if (NULL != mpEdit){
        distance = mpEdit->GetLineCount();
    }
	int CurLine = 0;
	if (NULL != mpEdit){
		CurLine = mpEdit->GetCurrentLine();
	}
    for (i = 0; i < mResults.size(); i++){
        if (mResults[i]->IsMatch() || (mKeys.empty())){
            wxSearchFileResult *pRet = dynamic_cast<wxSearchFileResult*>(mResults[i]);
            if (NULL != pRet){
                // if this is near to the mCurLine, store it
                int curDist = pRet->GetLine() - CurLine;
                if (curDist < 0){ curDist = -curDist;}
                if (curDist < distance){
                    distance = curDist;
                    bestLine = line;
                }
            }
            line++;
        }
    }
    wxPrintf("GetPreferedLine:%d\n", bestLine);
    return bestLine;
}

void wxSearchFile::ChangeSearchTarget(ceEdit *pEdit)
{
    if (NULL != pEdit && mpEdit != pEdit){
        mFileName = pEdit->GetFilename();
        mpEdit = pEdit;
        ResetSearch();
    }
}

void wxSearchFile::SetFileName(const wxString &fileName)
{
    mFileName = fileName;
    // update the summary.
    // mpStatus->SetLabel();
    mpStatus->SetLabel(GetShortHelp());
}

void wxSearchFile::SetBuffer(const wxString &buffer)
{
    mBuffer = buffer;
}

void wxSearchFile::SetEdit(ceEdit *pEdit)
{
    mpEdit = pEdit;
}

wxSearchFileResult::wxSearchFileResult(const wxString &content, const wxString &target, int line, int pos)
    :wxSearchResult(content, target), mLine(line), mPos(pos)
{
}

bool wxSearchFile::StartSearch(const wxString &input, const wxString &fullInput)
{
    wxMyTimeTrace trace("wxSearchFile::StartSearch");
    wxString niddle = input.Lower();
    bool bCaseSenstive = niddle != input;
    if (bCaseSenstive){
        niddle = input;
    }
    if (NULL != mpEdit){
        int i = 0;
        int pos = 0;
        int totalLine = mpEdit->GetLineCount();
        for (i = 0; i < totalLine; i++){
            wxString text = mpEdit->GetLineText(i);
            wxString value = text;
            if (!bCaseSenstive){
                value.MakeLower();
            }
            int offset = value.find(niddle);
            if (offset != wxString::npos){
                AddSearchResult(new wxSearchFileResult(wxString::Format("%d:\t%s", i+1, text), "", i, pos + offset));
            }
            pos += 1; // this is the newline
            pos += text.Length();
        }
    }
    
    return true;
}

bool wxSearchFile::StopSearch()
{
    return true;
}

wxString wxSearchFile::GetSummary(const wxString &input, int matchCount)
{
    wxString ret = wxString::Format("Find '%s' in '%s', %d%s match", input, mFileName, matchCount, 
                                    matchCount >= GetMaxCandidate() ? "+" : "");
    return ret;
}

wxString wxSearchFile::GetShortHelp() const
{
    if (mFileName.empty()){
        return wxSearch::GetShortHelp();
    }
    return wxString::Format("Search in '%s'", mFileName);
}

wxString wxSearchFile::GetHelp() const
{  
    wxString ret = wxSearch::GetHelp();
    return ret;
}
