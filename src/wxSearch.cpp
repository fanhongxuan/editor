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
#include "wxEdit.hpp"
#ifndef WIN32
#include <sys/time.h>
#endif
#include "wxSearch.hpp"

#include <stdio.h>

#include <map>
#include <vector>

// todo:fanhongxuan@gmail.com
// in some case the need to display the search result in some different group.
// maybe later we can use stc instread of wxTextEntry to show the search result.

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

class wxSearchListCtrl: public wxTextCtrl
{
private:
    wxSearch *mpParent;
    long mStart;
    long mEnd;
public:
    wxSearchListCtrl(wxSearch *parent, wxSize size)
        :wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, size,
                    wxVSCROLL | wxHSCROLL /*| wxBORDER_NONE*/ |
                    wxTE_RICH | wxWANTS_CHARS | wxTE_MULTILINE | wxTE_READONLY)
    {
        mpParent = parent;
        mStart = -1;
        mEnd = -1;
    }
    virtual void Clear()
    {
        mStart = -1;
        mEnd = -1;
        wxTextCtrl::Clear();
    }
    
    bool SetSelection(int start, int end)
    {
        wxTextAttr normal(wxNullColour, wxColour(255, 255, 255));
        wxTextAttr select(wxNullColour, wxColour(193, 213, 255));
        if (mStart >= 0 && mEnd >= 0){
            SetStyle(mStart, mEnd, normal);
        }
        if (start >= 0 && end >= 0 && start <= end){
            mStart = start;
            mEnd = end;
            SetStyle(mStart, mEnd, select);
        }
        else{
            wxPrintf("Invalid range:%d, %d\n", start, end);
        }
        wxTextCtrl::SetSelection(start, start);
        return true;
    }
    
    virtual bool AcceptsFocus() const {return false;}
    virtual bool AcceptsFocusFromKeyboard() const {return false;}
    virtual bool AcceptsFocusRecursively() const {return false;}
    virtual bool ProcessEvent(wxEvent &evt)
    {
        // note:fanhongxuan@gmail.com
        // left click to select and active
        // left double click to select and switch the focus.
        if (wxEVT_LEFT_DOWN == evt.GetEventType() || wxEVT_LEFT_DCLICK == evt.GetEventType()){
            wxMouseEvent *pEvt = dynamic_cast<wxMouseEvent*>(&evt);
            if (NULL != mpParent && NULL != pEvt){
                long pos = -1;
                HitTest(pEvt->GetPosition(), &pos);
                if (pos >= 0){
                    long x = 0, y = 0;
                    PositionToXY(pos, &x, &y);
                    mpParent->SelectLine(y, true, wxEVT_LEFT_DOWN == evt.GetEventType());
                    // note:fanhongxuan@gmail.com
                    // skip the default handler of mouse event in wxTextCtrl::ProcessEvent;
                    evt.StopPropagation();
                    return true;
                }
            }
        }
        return wxTextCtrl::ProcessEvent(evt);
    }
};

class wxSearchInputCtrl: public wxSearchCtrl
{
private:
    wxSearch *mpParent;
    wxSearchListCtrl *mpList;
public:
    wxSearchInputCtrl(wxSearch *parent)
        :wxSearchCtrl(parent, wxID_ANY), mpParent(parent), mpList(NULL){}
    void SetList(wxSearchListCtrl *pList){mpList = pList;}
    virtual bool ProcessEvent(wxEvent &evt)
    {
        // note:fanhongxuan@gmail.com
        // If we use wxSearchCtrl, we can't receive the wxEVT_KEY_DOWN, wxEVT_KEY_UP evt.
        // So use wxEVT_CHAR_HOOK.
        // If use wxTextCtrl instead of wxSearchCtrl, we can use wxEVT_KEY_DOWN here.
        if (wxEVT_CHAR_HOOK == evt.GetEventType()){
            wxKeyEvent *pEvt = dynamic_cast<wxKeyEvent*>(&evt);
            if (NULL != pEvt){
                if (pEvt->GetKeyCode() == WXK_UP || pEvt->GetKeyCode() == WXK_DOWN){
                    // note:fanhongxuan@gmail.com
                    // the default process of wxk_up and wxk_down will switch the keyboard focus,
                    // we don't want it, so directly process it here, and stop propagation to our parent.
                    evt.StopPropagation();
                    mpParent->OnKey(*pEvt);
                    return true;
                }
            }
        }
        else if (wxEVT_TEXT == evt.GetEventType()){
            mpParent->UpdateSearchList(GetValue());
        }
        else if (wxEVT_MOUSEWHEEL == evt.GetEventType()){
            wxMouseEvent *pEvt = dynamic_cast<wxMouseEvent*>(&evt);
            if (NULL != mpList && NULL != pEvt){
                mpList->ScrollLines(-pEvt->GetWheelDelta()/pEvt->GetWheelRotation());
            }
        }
        return wxSearchCtrl::ProcessEvent(evt);
    }
};


wxSearchResult::wxSearchResult(const wxString &content, const wxString &target, void *pCustomData)
    :mContent(content), mTarget(target), mpCustomData(pCustomData)
{
    mbMatch = false;
    mLowContent = mContent.Lower();
}

void ParseString(const wxString &input, std::vector<wxString> &output, char sep, bool allowEmpty = false)
{
    size_t begin = 0;
    size_t end = 0;
    while(begin != input.npos && begin < input.length()){
        end = input.find_first_of(sep, begin);
        if (end == input.npos){
            output.push_back(input.substr(begin));
            break;
        }
        else{
            if (begin != end || allowEmpty){
                output.push_back(input.substr(begin, (end - begin)));
            }
            begin = end + 1;
        }
    }
}

int wxSearchResult::IsMatch(int pos, const std::map<int, int> &match) const
{
    std::map<int, int>::const_iterator it = match.begin();
    while(it != match.end()){
        if (pos >= it->first && pos < it->first + it->second){
            return it->second;
        }
        it++;
    }
    return 0;
}

bool wxSearchResult::ConvertToRichText(wxSearchListCtrl &rich, const std::vector<wxString> &rets, bool bEnableHighlight)
{
    std::map<int, int> match;
    bool isNormal = true;
    wxString result;
    int i = 0;
    int minMatch = mContent.length(), maxMatch = 0;
    if (rets.empty()){
        // note:fanhongxuan@gmail.com
        // if the rets is empty, all is match, but don't highlight
        mbMatch = true;
        if (rich.GetLastPosition() != 0){
            rich.WriteText("\n");
        }
        mRange.SetStart(rich.GetLastPosition());
        rich.WriteText(mContent);
        // wxPrintf("Add <%s>\n", mContent);
        mRange.SetEnd(rich.GetLastPosition());
        return true;
    }
    mbMatch = false;
    for (i = 0; i < rets.size(); i++){
        wxString low = rets[i].Lower();
        int start = 0;
        bool isMatched = false;
        bool bCaseSenstive = (low != rets[i]);
        while(start < mContent.length()){
            int pos = mContent.npos;
            if (bCaseSenstive){
                pos = mContent.find(rets[i], start);
            }
            else{
                pos = mLowContent.find(low, start); 
            }
            if (pos == wxString::npos){
                if (isMatched){
                    break;
                }
                else{
                    return false;
                }
            }
            else{
                isMatched = true;
                if (minMatch > pos){
                    minMatch = pos;
                }
                if (maxMatch < (pos + rets[i].length())){
                    maxMatch = pos + rets[i].length();
                }
                // note:fanhongxuan@gmail.com
                // incase one pos has two match, we use the max one.
                if (match[pos] < rets[i].size()){
                    match[pos] = rets[i].size();
                }
            }
            start += (pos+1);
        }
    }

    mbMatch = true;
    // wxPrintf("match(%d<-->%d)\n", minMatch, maxMatch);
    if (rich.GetLastPosition() != 0){
        rich.WriteText("\n");
    }
    mRange.SetStart(rich.GetLastPosition());
    wxTextAttr black(wxColor(0, 0, 0));
    wxTextAttr red(wxColor(255, 0, 0));
    rich.SetDefaultStyle(black);
    if (bEnableHighlight){
        wxString value = mContent;
        // note:fanhongxuan@gmail.com
        // WriteText use mostly time, need to call it less.
        rich.WriteText(value.substr(0, minMatch));
        // wxPrintf("<%s><%s><%s>\n", value.substr(0, minMatch),
        //          value.substr(minMatch, (maxMatch-minMatch)),
        //          value.substr(maxMatch));
        
        value = value.substr(minMatch, (maxMatch-minMatch));
        wxString miss;
        for (i = 0; i < value.length();){
            int matchLen = IsMatch(i + minMatch, match);
            if (matchLen > 0){
                if (!miss.empty()){
                    rich.WriteText(miss);
                }
                if (isNormal){
                    rich.SetDefaultStyle(red);
                    isNormal = false;
                }
                rich.WriteText(value.substr(i, matchLen));
                i += matchLen;
                miss = wxEmptyString;
            }
            else{
                if (!isNormal){
                    rich.SetDefaultStyle(black);
                    isNormal = true;
                }
                miss.Append(value[i]);
                // rich.WriteText(value[i]);
                i++;
            }
        }
        if (!miss.empty()){
            rich.WriteText(miss);
        }
        rich.SetDefaultStyle(black);
        value = mContent.substr(maxMatch);
        rich.WriteText(value);
    }
    else{
        rich.WriteText(mContent);
    }
    rich.SetDefaultStyle(black);
    mRange.SetEnd(rich.GetLastPosition());
    return true;
}

wxSearch::wxSearch(wxWindow *pParent)
    :wxPanel(pParent), mCurrentLine(-1), mMinStartLen(1), mMaxCandidate(100), mbStartSearch(false)
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
    int length = mpList->GetLineLength(mCurrentLine);
    int pos = mpList->XYToPosition(0, mCurrentLine);

    mpList->SetSelection(pos, pos + length);
    mpList->ShowPosition(pos);
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
    }
    if ((!mpInput->HasFocus()) && bRequestFocus){
        mpInput->SetFocus();
        int len = mpInput->GetValue().Length();
        // note:fanhongxuan@gmail.com
        // this will cause the mpInput change it's size a little.
        // mpInput->SetPosition(wxPoint(0, len));
        mpInput->SetSelection(len, len);
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
        SelectLine(mCurrentLine, key != WXK_UP);
        return true;
    }
    else if (key == WXK_DOWN || (key == WXK_RETURN && evt.AltDown())){
        mCurrentLine++;
        if (mCurrentLine >= mpList->GetNumberOfLines()){
            mCurrentLine = 0;
        }
        SelectLine(mCurrentLine, key != WXK_DOWN);
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
    return wxString::Format("Search %s, total %d match", input, matchCount);
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

void wxSearch::Reset()
{
    int i = 0;
    for (i = 0; i < mResults.size(); i++){
        wxSearchResult *pRet = mResults[i];
        if (NULL != pRet){
            delete pRet;
        }
    }
    mpStatus->SetLabel(GetShortHelp());
    mKeys.clear();
    mResults.clear();
    mpList->Clear();
    mbStartSearch = false;
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
    // wxPrintf("DoStartSearch %s\n", input);
    // note:fanhongxuan@gmail.com
    // when call StartSearch we only use the first minStartLen char.
    StartSearch(input.substr(0, mMinStartLen));
    return true;
}

bool wxSearch::UpdateSearchList(const wxString &input)
{
    // fixme:fanhongxuan@gmail.com
    // on windows, then the edit is empty, will not call UpdateSearch list
    // so in buffer list, can not show all the candidate.
    int i = 0;
    if (NULL == mpList){
        return false;
    }

    if (input.length() < mMinStartLen){
        Reset();
        return false;
    }

    // note:fanhongxuan@gmail.com
    // if the mMinStartLen is 0
    // when the input is empty, show all the result
    if (input.find_first_not_of("\r\n\t ") == wxString::npos){
        Reset();
        if (mMinStartLen != 0){
            return false;
        }
    }
    
    if (!mbStartSearch){
        Reset(); // make sure the status has been reset.
        mbStartSearch = true;
        DoStartSearch(input);
    }
    
    std::vector<wxString> rets;
    ParseString(input, rets, ' ');

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
            return false;
        }
    }
    mKeys = rets;

    // here, we start update the list
    // wxMyTimeTrace trace("UpdateSearchList");
    mpList->Clear();
    mCurrentLine = -1;
    
    int count = 0;
    for (i = 0; i < mResults.size(); i++){
        // todo:fanhongxuan@gmail.com
        // currently, if the match count is bigger than mMaxCandidate
        // we directly return, later we need to show more result.
        if (count >= mMaxCandidate){
            break;
        }
        if (mResults[i]->ConvertToRichText(*mpList, rets, count <= mMaxCandidate)){
            count++;
        }
    }
    mpList->SetInsertionPoint(0);
    mpStatus->SetLabel(GetSummary(input, count));

    if (mCurrentLine < 0){
        int prefer = GetPreferedLine(input);
        if (prefer >= 0){
            SelectLine(prefer, true, true);
        }
        // no selection, getPreferedSelection
        
    }
    return true;
}

bool wxSearch::ProcessEvent(wxEvent &evt)
{
    wxEventType type = evt.GetEventType();
    if (wxEVT_CHAR_HOOK == type){
        wxKeyEvent *pEvt = dynamic_cast<wxKeyEvent*>(&evt);
        if (NULL != pEvt){
            OnKey(*pEvt);
        }
    }
    if (wxEVT_SEARCH_CANCEL == type){
        if (mbStartSearch){
            mbStartSearch = false;
            StopSearch();
        }
    }
    return wxPanel::ProcessEvent(evt);
}


wxSearchDir::wxSearchDir(wxWindow *parent)
    :wxSearch(parent)
{
}

wxString wxSearchDir::GetSummary(const wxString &input, int matchCount)
{
    wxString cwd = wxGetCwd();
    wxString ret = wxString::Format("Search '%s' in '%s', total %d match", input, cwd, matchCount);
    return ret;
}

wxString wxSearchDir::GetShortHelp() const
{
    return "UP/DOWN to select, CTRL/ALT+ENTER to select and open, ENTER to open and switch";
}

wxString wxSearchDir::GetHelp() const
{
    wxString ret = wxSearch::GetHelp();
    return ret;
}

static bool IsTempFile(const wxString &file)
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
    if (file[file.size() -1] == '#'){
        return true;
    }
    return false;
}

static bool IsBinaryFile(const wxString &file)
{
    wxString ext;
    wxFileName::SplitPath(file, NULL, NULL, NULL, &ext);
    if (ext == "obj" || ext == "o" || ext == "pdb" || ext == "exe"){
        return true;
    }
    return false;
}

static void FindFiles(const wxString &dir, std::vector<wxString> &output)
{
    wxDir cwd(dir);
    if (!cwd.IsOpened()){
        return;
    }
    wxString filename;
    bool cont = false;
    cont = cwd.GetFirst(&filename, "*", wxDIR_DIRS | wxDIR_HIDDEN);
    while(cont){
        wxString child_dir = cwd.GetNameWithSep() + filename;
        if (filename != ".git"){
            FindFiles(child_dir, output);
        }
        cont = cwd.GetNext(&filename);
    }

    cont = cwd.GetFirst(&filename, "*", wxDIR_FILES | wxDIR_HIDDEN);
    while(cont){
        // skip the edit temp file like:
        // test~ (emacs auto save)
        // #test# (emacs auto save)
        if ((!IsTempFile(filename)) && (!IsBinaryFile(filename))){
            output.push_back(cwd.GetNameWithSep() + filename);
        }
        cont = cwd.GetNext(&filename);
    }
}

bool wxSearchDir::StartSearch(const wxString &input)
{
    // todo:fanhongxuan@gmail.com
    // skip some dir, like the .git
    // skip all the file and dir list in a file named .ignore

    // wxMyTimeTrace trace("StartSearch");
    // wxArrayString files;
    // note:fanhongxuan@gmail.com
    // wxDir::GetAllFiles default is casesensitive
    wxString filter = "*.*";
    wxString path, name, cwd = wxGetCwd();
    int i = 0;
    if (cwd.length()!= 0 && cwd[cwd.length()-1] != '/'){
        cwd += "/";
    }
    // todo:fanhongxuan@gmail.com
    // skip the .git dir
    // wxDir::GetAllFiles(cwd, &files, filter);
    std::vector<wxString> files;
    FindFiles(cwd, files);
    
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
    return true;
}

bool wxSearchDir::StopSearch()
{
    // todo:fanhongxuan@gmail.com
    // stop the background working thread.
    return true;
}

wxSearchFile::wxSearchFile(wxWindow *parent)
    :wxSearch(parent), mTotalLine(0), mCurLine(0), mpEdit(NULL)
{
}

int wxSearchFile::GetPreferedLine(const wxString &input)
{
    int i = 0;
    int dis = 0;
    int line = 0;
    int bestLine = 0;
    int distance = mTotalLine;
    for (i = 0; i < mResults.size(); i++){
        if (mResults[i]->IsMatch()){
            wxSearchFileResult *pRet = dynamic_cast<wxSearchFileResult*>(mResults[i]);
            if (NULL != pRet){
                // if this is near to the mCurLine, store it
                int curDist = pRet->GetLine() - mCurLine;
                if (curDist < 0){ curDist = -curDist;}
                if (curDist < distance){
                    distance = curDist;
                    bestLine = line;
                }
            }
            line++;
        }
    }
    return bestLine;
}

void wxSearchFile::SetCurrentLine(int line)
{
    mCurLine = line;
    // int i = 0;
    // for (i = 0; i < mResults.size(); i++){
    //     // todo:fanhongxuan@gmail.com
    //     // update the mCurentLine of wxSearch
    //     // find the respond mResults according the line.
    //     // but the mResults maybe filted again
    // }
}

void wxSearchFile::SetFileName(const wxString &fileName)
{
    mFileName = fileName;
}

void wxSearchFile::SetBuffer(const wxString &buffer)
{
    mBuffer = buffer;
}

void wxSearchFile::SetEdit(Edit *pEdit)
{
    mpEdit = pEdit;
}
wxSearchFileResult::wxSearchFileResult(const wxString &content, const wxString &target, int line, int pos)
    :wxSearchResult(content, target), mLine(line), mPos(pos)
{
}

bool wxSearchFile::StartSearch(const wxString &input)
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
        mTotalLine = mpEdit->GetLineCount();
        for (i = 0; i < mTotalLine; i++){
            wxString text = mpEdit->GetLineText(i);
            wxString value = text;
            if (!bCaseSenstive){
                value.MakeLower();
            }
            int offset = value.find(niddle);
            if (offset != wxString::npos){
                AddSearchResult(new wxSearchFileResult(wxString::Format("%d\t%s", i+1, text), text, i, pos + offset));
            }
            pos += 1; // this is the newline
            pos += text.Length();
        }
    }
    
    // int pos = 0;
    // int line = 0;
    // int i = 0;
    // wxString value;
    // int size = mBuffer.size();
    // for (i = 0; i < size; i++){
    //     int chr= mBuffer[i].GetValue();
    //     if (chr != '\n'){
    //         value += chr;
    //         continue;
    //     }
    //     if (!bCaseSenstive){
    //         value.MakeLower();
    //     }
    //     int offset = value.find(niddle);
    //     if (offset != wxString::npos){
    //         AddSearchResult(new wxSearchFileResult(value, value, line, pos + offset));
    //     }
    //     pos += 1; // this is the newline
    //     pos += value.size();
    //     line++;
    //     value.Clear();
    // }

    // std::vector<wxString> rets;
    // ParseString(mBuffer, rets, '\n', true);
    // int pos = 0;
    // int line = 0;
    // int i = 0;
    // for (i = 0; i < rets.size(); i++){
    //     wxString value = rets[i];
    //     if (!bCaseSenstive){
    //         value.MakeLower();
    //     }
    //     int offset = value.find(niddle);
    //     if (offset != wxString::npos){
    //         AddSearchResult(new wxSearchFileResult(rets[i], rets[i], line, pos + offset));
    //     }
    //     pos += 1; // this is the newline
    //     pos += strlen(rets[i]);
    //     line ++;
    // }
    
    // mTotalLine = line;
    // update mCurrentLine according the position
    return true;
}

bool wxSearchFile::StopSearch()
{
    return true;
}

wxString wxSearchFile::GetSummary(const wxString &input, int matchCount)
{
    wxString ret = wxString::Format("Search %s in %s, total %d match", input, mFileName, matchCount);
    return ret;
}

wxString wxSearchFile::GetShortHelp() const
{
    return wxSearch::GetShortHelp();
}

wxString wxSearchFile::GetHelp() const
{  
    wxString ret = wxSearch::GetHelp();
    return ret;
}
