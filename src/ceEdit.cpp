#include "ceEdit.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf
#include <wx/filename.h>
#include <wx/file.h>     // raw file io support
#include <wx/filename.h> // filename support
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <map>
#include <set>
#include <wx/timer.h>
#include "ceUtils.hpp"

#include "wxAutoComp.hpp"
#include "ce.hpp"
#include "wxSearch.hpp"
#include "ceSymbolDb.hpp"

enum{
    ceEdit_Idle_Timer = 62345,
    ceEdit_CallTip_Timer,
    ceEdit_Update_Indent_By_Fold_Level,
};

wxBEGIN_EVENT_TABLE(ceEdit, wxStyledTextCtrl)
EVT_STC_STYLENEEDED(wxID_ANY, ceEdit::OnStyleNeeded)
EVT_STC_AUTOCOMP_SELECTION(wxID_ANY, ceEdit::OnAutoCompSelection)
EVT_STC_CALLTIP_CLICK(wxID_ANY, ceEdit::OnCallTipClick)
EVT_STC_DWELLSTART(wxID_ANY, ceEdit::OnDwellStart)
EVT_STC_DWELLEND(wxID_ANY, ceEdit::OnDwellEnd)
EVT_STC_MODIFIED(wxID_ANY,    ceEdit::OnModified)
EVT_STC_MARGINCLICK(wxID_ANY, ceEdit::OnMarginClick)
EVT_STC_UPDATEUI(wxID_ANY, ceEdit::OnUpdateUI)
EVT_STC_CHARADDED(wxID_ANY,  ceEdit::OnCharAdded)
EVT_STC_ZOOM(wxID_ANY, ceEdit::OnZoom)
EVT_MENU(ceEdit_Update_Indent_By_Fold_Level, ceEdit::OnUpdateIndentByFolder)
EVT_SET_FOCUS(ceEdit::OnFocus)
EVT_KILL_FOCUS(ceEdit::OnKillFocus)
EVT_KEY_DOWN(ceEdit::OnKeyDown )
EVT_KEY_UP(ceEdit::OnKeyUp)
EVT_LEFT_DOWN(ceEdit::OnMouseLeftDown)
EVT_LEFT_UP(ceEdit::OnMouseLeftUp)
EVT_LEFT_DCLICK(ceEdit::OnMouseLeftDclick)
EVT_MOUSEWHEEL(ceEdit::OnMouseWheel)
EVT_SIZE(ceEdit::OnSize)
EVT_TIMER(ceEdit_CallTip_Timer, ceEdit::OnTimer)
EVT_TIMER(ceEdit_Idle_Timer, ceEdit::OnIdleTimer)
wxEND_EVENT_TABLE();

enum{
    STYLE_DEFAULT = 0,
    STYLE_STRING,  // "string"
    STYLE_CHAR,    // 'c'
    STYLE_COMMENTS, // //
    STYLE_CSTYLE_COMMENTS, // /**/
    STYLE_PREPROCESS,   // #define/#ifdef
    STYLE_PREPROCESS_LOCAL, // #include "file.h"
    STYLE_PREPROCESS_SYSTEM, // #include <file.h>
    STYLE_KEYWORD1,
    STYLE_KEYWORD2,
    STYLE_KEYWORD3,
    STYLE_COMMENT_KEY,  // @xxx in comments
    STYLE_MACRO,        // macro
    STYLE_OPERATOR,     // operator,
    STYLE_FUNCTION,     // function,
    STYLE_PARAMETER,    // function parameter,
    STYLE_PARAMETER_REF,
    STYLE_VARIABLE,
    STYLE_VARIABLE_REF,
    STYLE_TYPE,         // type, use defined data type.
    STYLE_FOLDER,       // (){}
    STYLE_NUMBER,       // 123, 0x123, 0.123 etc.
    STYLE_IDENTY,       // unknown id,
    STYLE_ESC_CHAR,     // char start with a '\'
    STYLE_NORMAL,       // other words.
    STYLE_ERROR,        // error 
    STYLE_MAX,
};


static inline bool IsWhiteSpace(char ch)
{
    if (ch == '\r' || ch == '\n' || ch == '\t' || ch == ' '){
        return true;
    }
    return false;
}

static inline bool IsCommentOrWhiteSpace(char c, int style){
    if (IsWhiteSpace(c)){
        return true;
    }
    if (style == STYLE_COMMENTS || style == STYLE_CSTYLE_COMMENTS || style == STYLE_COMMENT_KEY){
        return true;
    }
    return false;
}

static time_t GetLastModifyTime(const wxString &path){
    int fd = open(static_cast<const char*>(path), O_RDONLY
#ifndef WIN32
                  |O_NOATIME|O_SYNC
#endif
                  );
    if (fd < 0){
        return 0;
    }
    struct stat st;
    fstat(fd, &st);
    close(fd);
    return st.st_mtime;
}

ceEdit::ceEdit(wxWindow *parent)
:wxStyledTextCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize){
    mpTimer = NULL;
    mpIdleTimer = NULL;
    mbLoadFinish = true;
    mbReplace = false;
    mbHasFocus = false;
    mLinenuMargin = 0;
    mFoldingMargin = 2;
    mDeviderMargin = 1;
    mLastModifyTime = 0;
    SetMouseDwellTime(800);
}

ceEdit::~ceEdit(){
    if (NULL != mpTimer){
        mpTimer->Stop();
        delete mpTimer;
        mpTimer = NULL;
    }
    if (NULL != mpIdleTimer){
        mpIdleTimer->Stop();
        delete mpIdleTimer;
        mpIdleTimer = NULL;
    }
}

wxString ceEdit::GetFilename(){
    return mFilename;
}

bool ceEdit::LoadFile(const wxString &filename){
    mbLoadFinish = false;
    mFilename = filename;
    wxStyledTextCtrl::LoadFile(mFilename);
    mbLoadFinish = true;
    EmptyUndoBuffer();
    LoadStyleByFileName(filename);
    mLastModifyTime = GetLastModifyTime(filename);
    UpdateLineNumberMargin();
    wxAutoCompWordInBufferProvider::Instance().AddFileContent(GetText(), mLanguage);
    return true;
}

bool ceEdit::NewFile(const wxString &filename){
    mDefaultName = filename;
    LoadStyleByFileName(filename);
    mLastModifyTime = time(NULL);
    return true;
}

bool ceEdit::Modified () {
    // return modified state
    return (GetModify() && !GetReadOnly());
}

bool ceEdit::SaveFile (bool bClose)
{
    if (mFilename.empty()) {
        wxFileDialog dlg (this, wxT("Save file"), wxEmptyString, mDefaultName, wxT("Any file (*)|*"),
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return false;
        mFilename = dlg.GetPath();
        mDefaultName = wxEmptyString;
        wxFileName fname (mFilename);
    }
    else if (mLastModifyTime < GetLastModifyTime(GetFilename())){
        // file is modified outside, confirm to overwride that change.
        int ret = wxMessageBox(wxString::Format(wxT("File(%s) is modified outside, do you want to overwride the change?"), GetFilename()), 
                               wxT("Confirm"), wxYES_NO| wxCANCEL, this);
        if (ret != wxYES){
            return true;
        }
    }
    else if (GetLastModifyTime(GetFilename()) == 0){
        // file is deleted outsize, confirm to create it again?
        int ret = wxMessageBox(wxString::Format(wxT("File(%s) is delete outside, do you want to created it again?"), GetFilename()), 
                               wxT("Confirm"),wxYES_NO| wxCANCEL, this);
        if (ret != wxYES){
            return true;
        }
    }
    else if (!Modified()){
        // return if no change
        return true;
    }
    
    // save file
    return SaveFile (mFilename, bClose);
}

bool ceEdit::SaveFile (const wxString &filename, bool bClose) {
    // return if no change
    // if (!Modified()) return true;
    bool ret = wxStyledTextCtrl::SaveFile(filename);
    if (!bClose){
        mLastModifyTime = GetLastModifyTime(filename);
        // todo:fanhongxuan@gmail.com
        // handle the pasted event, when save file, do not need to update this again.
        wxAutoCompWordInBufferProvider::Instance().AddFileContent(GetText(), mLanguage);
        ClearLoalSymbol();
        if (NULL != wxGetApp().frame()){
            wxStyledTextEvent evt;
            evt.SetEventObject(this);
            wxGetApp().frame()->OnFileSaved(evt);
        }
    }
    return ret;
}

wxFontInfo ceEdit::GetFontByStyle(int style, int type){
    static std::map<int, wxFontInfo> mFontMap;
    if (mFontMap.empty()){
        mFontMap[STYLE_MACRO] = wxFontInfo(13).Bold();
        mFontMap[STYLE_FUNCTION] = wxFontInfo(13).Bold();
        mFontMap[STYLE_PARAMETER] = wxFontInfo(11).Bold();
        mFontMap[STYLE_VARIABLE] = wxFontInfo(11).Bold();
    }
    std::map<int, wxFontInfo>::iterator it = mFontMap.find(style);
    wxFontInfo info(11);
    if (it != mFontMap.end()){
        info = it->second;
    }
#ifdef WIN32
    info.FaceName("Consolas");
#else
    info.Family(wxFONTFAMILY_MODERN);
#endif
    return info;
}

wxColor ceEdit::GetColourByStyle(int style, int type)
{
    static std::map<int, wxColor> mBackgrounds;
    static std::map<int, wxColor> mForegrounds;
    if (mBackgrounds.empty()){
        mForegrounds[STYLE_DEFAULT] = *wxWHITE;
        mBackgrounds[STYLE_DEFAULT] = *wxBLACK;
        
        mForegrounds[STYLE_NORMAL] = *wxWHITE;
        mBackgrounds[STYLE_NORMAL] = *wxBLACK;
        
        mForegrounds[STYLE_STRING] = wxColor("BROWN");
        mBackgrounds[STYLE_STRING] = *wxBLACK;
        
        mForegrounds[STYLE_CHAR] = wxColor("BROWN");
        mBackgrounds[STYLE_CHAR] = *wxBLACK;
        
        mForegrounds[STYLE_COMMENTS] = wxColor("GREEN");
        mBackgrounds[STYLE_COMMENTS] = *wxBLACK;
        
        mForegrounds[STYLE_CSTYLE_COMMENTS] = wxColor("GREEN");
        mBackgrounds[STYLE_CSTYLE_COMMENTS] = *wxBLACK;
        
        mForegrounds[STYLE_PREPROCESS] = wxColor("BLUE");
        mBackgrounds[STYLE_PREPROCESS] = *wxBLACK;
        
        mForegrounds[STYLE_PREPROCESS_LOCAL] = wxColor("YELLOW");
        mBackgrounds[STYLE_PREPROCESS_LOCAL] = *wxBLACK;
        
        mForegrounds[STYLE_PREPROCESS_SYSTEM] = wxColor("RED");
        mBackgrounds[STYLE_PREPROCESS_SYSTEM] = *wxBLACK;
        
        mForegrounds[STYLE_MACRO] = wxColor("RED");
        mBackgrounds[STYLE_MACRO] = *wxBLACK;
        
        
        // mForegrounds[STYLE_OPERATOR] = wxColor("BLUE");
        // mBackgrounds[STYLE_OPERATOR] = *wxBLACK;
        
        // mForegrounds[STYLE_FOLDER] = wxColor("RED");
        // mBackgrounds[STYLE_FOLDER] = *wxBLACK;
        
        
        mForegrounds[STYLE_NUMBER] = wxColor("YELLOW");
        mBackgrounds[STYLE_NUMBER] = *wxBLACK;
        
        
        mForegrounds[STYLE_TYPE] = wxColor("YELLOW");
        mBackgrounds[STYLE_TYPE] = *wxBLACK;
        
        
        // mForegrounds[STYLE_IDENTY] = wxColor("BLACK");
        // mBackgrounds[STYLE_IDENTY] = *wxWHITE;
        wxColour violet = wxColour(238, 130, 238);
        mForegrounds[STYLE_KEYWORD1] = wxColor("RED");
        mBackgrounds[STYLE_KEYWORD1] = *wxBLACK;        
        mForegrounds[STYLE_KEYWORD2] = wxColor("purple");
        mBackgrounds[STYLE_KEYWORD2] = *wxBLACK;
        
        mForegrounds[STYLE_COMMENT_KEY] = violet;// wxColor("violet");
        mBackgrounds[STYLE_COMMENT_KEY] = *wxBLACK;        
        
        mForegrounds[STYLE_PREPROCESS_SYSTEM] = wxColor("RED");
        mBackgrounds[STYLE_PREPROCESS_SYSTEM] = *wxBLACK;  
        
        
        mForegrounds[STYLE_FUNCTION] = wxColor("GREEN");
        mBackgrounds[STYLE_FUNCTION] = wxColor("BLACK");
        
        mForegrounds[STYLE_ERROR] = wxColor("BLACK");
        mBackgrounds[STYLE_ERROR] = wxColor("RED");
        
        mForegrounds[STYLE_PARAMETER] = violet;// wxColor("violet");
        mBackgrounds[STYLE_PARAMETER] = *wxBLACK;
        
        mForegrounds[STYLE_PARAMETER_REF] = violet; // wxColor("violet");
        mBackgrounds[STYLE_PARAMETER_REF] = *wxBLACK;
        
        
        mForegrounds[STYLE_VARIABLE] = wxColor("purple");
        mBackgrounds[STYLE_VARIABLE] = *wxBLACK;
        
        mForegrounds[STYLE_VARIABLE_REF] = wxColor("purple");
        mBackgrounds[STYLE_VARIABLE_REF] = *wxBLACK;
        
        mForegrounds[STYLE_ESC_CHAR] = wxColor("BLUE");
        mBackgrounds[STYLE_ESC_CHAR] = wxColor("RED");
        
        // predefined style
        
        mForegrounds[wxSTC_STYLE_FOLDDISPLAYTEXT] = wxColor("YELLOW");
        mBackgrounds[wxSTC_STYLE_FOLDDISPLAYTEXT] = wxColor("BLACK");
        
        // for line number
        mForegrounds[wxSTC_STYLE_LINENUMBER] = wxColor("GREEN");
        
        // for indent guide
        mForegrounds[wxSTC_STYLE_INDENTGUIDE] = wxColor("GREEN");
        mBackgrounds[wxSTC_STYLE_INDENTGUIDE] = wxColor("GREEN");
        
        // for brace light and bad
        mForegrounds[wxSTC_STYLE_BRACELIGHT] = wxColor("BLACK");
        mBackgrounds[wxSTC_STYLE_BRACELIGHT] = wxColor("GREEN");
        mForegrounds[wxSTC_STYLE_BRACEBAD] = wxColor("BLACK");
        mBackgrounds[wxSTC_STYLE_BRACEBAD] = wxColor("RED");
    }
    std::map<int, wxColor>::iterator it;
    if (type == 1){ // for background
        it = mBackgrounds.find(style);
        if (it != mBackgrounds.end()){
            return it->second;
        }
        return *wxBLACK;
    }
    
    it = mForegrounds.find(style);
    if (it != mForegrounds.end()){
        return it->second;
    }
    return *wxWHITE;
}

wxString ceEdit::GuessLanguage(const wxString &filename){
    wxString volume, path, name, ext;
    wxFileName::SplitPath(filename, &volume, &path, &name, &ext);
    ext = ext.Lower();
    if (ext == "c"){
        return "C";
    }
    else if (ext == "h" || ext == "cpp" || ext == "hpp" || ext == "cc" ||
        ext == "cxx" || ext == "hxx"){
        return "C++";
    }
    else if (ext == "java" || ext == "aidl"){
        return "Java";
    }
    else if (ext == "bp"){
        return "BP"; // For Android.bp
    }
    return "";
}

bool ceEdit::LoadStyleByFileName(const wxString &filename)
{
    if (NULL == mpIdleTimer){
        mpIdleTimer = new wxTimer(this, ceEdit_Idle_Timer);
    }
    RestartIdleTimer();
    StyleClearAll();
    SetEndAtLastLine(false); // allows scrolling one page below the last line
#ifdef WIN32
    SetEOLMode(wxSTC_EOL_CRLF);
#else
    SetEOLMode(wxSTC_EOL_LF);
#endif
    int charset = 65001;
    int i = 0;
    for (i = 0; i <= wxSTC_STYLE_LASTPREDEFINED; i++) {
        StyleSetCharacterSet (i, charset);
    }
    SetCodePage (charset);
    
    // default fonts for all styles!
#ifdef WIN32
    wxFont font(wxFontInfo(11).FaceName("Consolas"));
#else        
    wxFont font(wxFontInfo(11).Family(wxFONTFAMILY_MODERN));
#endif        
    for (i = 0; i <= wxSTC_STYLE_LASTPREDEFINED; i++) {
        StyleSetFont (i, wxFont(GetFontByStyle(i, 0)));
        StyleSetForeground(i, GetColourByStyle(i, 0)); 
        StyleSetBackground(i, GetColourByStyle(i, 1));
    }
    
    // linenumber
    SetMarginType (mLinenuMargin, wxSTC_MARGIN_NUMBER);
    // note:fanhongxuan@gmail.com
    // do not set the margin width here
    // update the linenumber margin width when file is modified.
    // if we set a fix value here, the later update is not work.
    // SetMarginWidth(mLinenuMargin, 0);
    
    // Caret
    SetCaretStyle(wxSTC_CARETSTYLE_BLOCK);
    SetCaretForeground(wxColor(wxT("RED")));
    SetCaretLineBackground(wxColour(193, 213, 255));
    SetCaretLineBackAlpha(60);
    SetCaretLineVisible(true);
    SetCaretLineVisibleAlways(true);
    
    // about call tips
    CallTipSetBackground(*wxYELLOW);
    CallTipSetForegroundHighlight(*wxRED);
    CallTipSetForeground(*wxBLUE);
    
    // set margin 1 is hide, currently not used.
    // SetMarginType (mDeviderMargin, wxSTC_MARGIN_SYMBOL);
    SetMarginWidth (mDeviderMargin, 0);
    SetMarginSensitive (mDeviderMargin, false);
    
    // folding
    FoldDisplayTextSetStyle(wxSTC_FOLDDISPLAYTEXT_STANDARD); // display text after folder header line.
    SetMarginType (mFoldingMargin, wxSTC_MARGIN_SYMBOL);
    SetMarginMask (mFoldingMargin, wxSTC_MASK_FOLDERS);
    SetMarginWidth (mFoldingMargin, 16);
    SetMarginSensitive (mFoldingMargin, true);
    SetFoldMarginColour(true, *wxBLACK);
    SetFoldMarginHiColour(true, *wxBLACK);
    MarkerDefine(wxSTC_MARKNUM_FOLDER,        wxSTC_MARK_BOXPLUS, wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN,    wxSTC_MARK_BOXMINUS,  wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDERSUB,     wxSTC_MARK_VLINE,     wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDEREND,     wxSTC_MARK_BOXPLUSCONNECTED, wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED, wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER,     wxT("BLACK"), wxT("WHITE"));
    MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL,    wxSTC_MARK_LCORNER,     wxT("BLACK"), wxT("WHITE"));
    SetProperty(wxT("fold"), wxT("1"));
    SetFoldFlags(wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED 
                 // #define DEBUG_FOLD
#ifdef DEBUG_FOLD    
                 // | wxSTC_FOLDFLAG_LEVELNUMBERS
                 | wxSTC_FOLDFLAG_LINESTATE
#endif        
                 // wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED
                 );
    
    // set spaces and indentation
    SetTabWidth(4);
    SetUseTabs(false);
    SetTabIndents(false);
    SetBackSpaceUnIndents(false);
    CmdKeyClear(wxSTC_KEY_TAB, 0); // disable the default tab handle;
    SetIndent(4);
    
    // for multi input
    SetMultipleSelection(true);
    SetMultiPaste(wxSTC_MULTIPASTE_EACH);
    SetAdditionalSelectionTyping(true);
    
    // others
    SetViewEOL(false);
    SetIndentationGuides(true);
    SetEdgeColumn(120);
    SetEdgeMode(wxSTC_EDGE_LINE);
    SetViewWhiteSpace(wxSTC_WS_INVISIBLE);
    SetOvertype(false);
    SetReadOnly(false);
    // SetWrapMode(wxSTC_WRAP_WORD);
    SetWrapMode(wxSTC_WRAP_NONE);
    
    mLanguage = GuessLanguage(filename);
    if (mLanguage != "Java"  && mLanguage != "C++" && mLanguage != "C" && mLanguage != "BP"){
        SetLexer(wxSTC_LEX_NULL);
    }
    else{
        SetLexer(wxSTC_LEX_CONTAINER);
    }
    
    // about autocomp
    LoadAutoComProvider(mLanguage);
    AutoCompSetSeparator('\1');
    return true;
}

int ceEdit::FindStyleStart(int style, int curPos, bool bSkipNewline){
    int pos = curPos;
    while(pos > 0){
        char c = GetCharAt(pos);
        if (bSkipNewline && (c == '\r' || c == '\n')){
            pos--;
            continue;
        }
        if (c != '\t' && c != ' '){
            if (GetStyleAt(pos-1) != style){
                break;
            }
        }
        pos--;
    }
    // wxPrintf("FindStyleStart(style:%d, curPos:%d, ret:%d)\n", style, curPos, pos);
    return pos;
}

// note:fanhongxuan@gmail.com
// we check from the stopPos;
// skip comments and whitespace.
// if first meet other style, return empty;
wxString ceEdit::GetPrevValue(int stopPos, int style, int *pStop)
{
    if (IsCommentOrWhiteSpace(0, style)){
        // the target style is comments, return,
        // this should be a error;
        wxPrintf("Bug:GetPrevValue set a comments style as input, please check the code!!!\n");
        return "";
    }
    int start = stopPos;
    while(start > 0){
        char c = GetCharAt(start);
        int sty = GetStyleAt(start);
        if (!IsCommentOrWhiteSpace(c, sty)){
            if (sty != style){
                return "";
            }
            int vStart = FindStyleStart(style, start);
            if (NULL != pStop){
                *pStop = vStart;
            }
            if (vStart >= 0){
                return GetTextRange(vStart, start+1);
            }
            return "";
        }
        start--;
    }
    if (NULL != pStop){
        *pStop = start;
    }
    return "";
}

bool ceEdit::IsInPreproces(int stopPos, const wxString &keyword)
{
    int pos = stopPos;
    while(pos > 0){
        if (GetStyleAt(pos) != STYLE_CSTYLE_COMMENTS){
            break;
        }
        pos--;
    }
    
    int startLine = LineFromPosition(pos);
    int startPos = XYToPosition(0, startLine);
    while(startPos < pos){
        if (GetStyleAt(startPos) == STYLE_PREPROCESS){
            int stop = startPos;
            wxString text;
            while(stop < pos){
                if (GetStyleAt(stop) != STYLE_PREPROCESS){
                    break;
                }
                text += GetCharAt(stop);
                stop++;
            }
            if (keyword.empty()){
                return true;
            }
            // int start = FindStyleStart(STYLE_PREPROCESS, startPos);
            // wxString text = GetTextRange(start, startPos);
            // wxPrintf("Preprocess:%s\n", text);
            if (text.find(keyword) != text.npos){
                return true;
            }
            break;
            // return true;
        }
        startPos++;
    }
    return false;
}

extern wxString gCppKeyWord;
extern wxString gCKeyWord;
extern wxString gJavaKeyWord;
static bool IsKeyWord1(const wxString &value, const wxString &language){
    static std::set<wxString> sCKeyWords;
    static std::set<wxString> sCppKeyWords;
    static std::set<wxString> sJavaKeyWords;
    
    if (language == "C"){
        if (sCKeyWords.empty()){
            std::vector<wxString> outputs;
            ceSplitString(gCKeyWord, outputs, ' ');
            for (int i = 0; i < outputs.size(); i++){
                sCKeyWords.insert(outputs[i]);
            }
        }
        return (sCKeyWords.find(value) != sCKeyWords.end());
    }
    else if (language == "C++"){
        if (sCppKeyWords.empty()){
            std::vector<wxString> outputs;
            ceSplitString(gCppKeyWord, outputs, ' ');
            for (int i = 0; i < outputs.size(); i++ ){
                sCppKeyWords.insert(outputs[i]);
            }
        }
        return(sCppKeyWords.find(value) != sCppKeyWords.end());
    }
    else if (language == "Java"){
        if (sJavaKeyWords.empty()){
            std::vector<wxString> outputs;
            ceSplitString(gJavaKeyWord, outputs, ' ');
            for (int i = 0; i < outputs.size(); i++ ){
                sJavaKeyWords.insert(outputs[i]);
            }
        }
        return(sJavaKeyWords.find(value) != sJavaKeyWords.end());
    }
    // todo:fanhongxuan@gmail.com
    // add keywords for bp
    return false;
}

static bool IsKeyWord2(const wxString &value, const wxString &language){
    if (value.size() >= 3 && value[0] == '_' && value[1] == '_' && (language == "C" || language == "C++")){
        // c/c++: id start with __ is mark as keyword 2
        return true;
    }
    // if (value == "size_t" && ((language == "C") || language == "C++")){
    //     return true;
    // }
    if (value.size() >=2 && value[0] == '@' && language == "Java"){
        // java: id start with @ is mark as keyword 2
        return true;
    }
    return false;
}

bool ceEdit::IsNumber(const wxString &value){
    // todo:fanhongxuan@gmail.com
    // use regext to match if is a number.
    static wxString sNumber = "0123456789";
    if (value.empty()){
        wxPrintf("empty string, mark as error\n");
        return false;
    }
    if (value.find_first_not_of(sNumber) == value.npos){
        return true; // only has number
    }
    double dValue = .0;
    long lValue = 0;
    long long llValue = 0;
    if (value.ToDouble(&dValue)){
        return true;
    }
    if (value.ToLong(&lValue)){
        return true;
    }
    if (value.ToLongLong(&llValue)){
        return true;
    }
    // wxPrintf("IsNotNumber:<%s>, mark as error\n", value);
    return false;
}

int ceEdit::HandleClass(int pos, int curStyle){
    if (pos <= 2){
        return curStyle;
    }
    
    if (GetCharAt(pos) == ':' && GetCharAt(pos - 1) == ':' &&
        GetStyleAt(pos) == STYLE_OPERATOR && GetStyleAt(pos-1) == STYLE_OPERATOR){
        int startPos = pos - 2;
        while(startPos > 0){
            char c = GetCharAt(startPos);
            int style = GetStyleAt(startPos);
            if (style == STYLE_IDENTY){
                int stop = FindStyleStart(STYLE_IDENTY, startPos);
                StartStyling(stop);
                SetStyling(startPos+1 - stop, STYLE_TYPE);
                wxString text = GetTextRange(stop, startPos+1);
                mLocalTypes.insert(text);
                break;
            }
            else if (!IsCommentOrWhiteSpace(c, style)){
                break;
            }
            startPos--;
        }
    }
    return curStyle;
}

// Fixme:fanhongxuan@gmail.com
// When paste something, the param is not right cloured.

// if is a variable define, return true
// if pStart && pStop != NULL, then set the variable define start and stop.
// otherwise return false;
bool ceEdit::IsValidVariable(int startPos, int stopPos, bool onlyHasName, int *pStart, int *pLen){
    if (startPos >= stopPos){
        return false;
    }
    // if has ], goto [,
    int start = startPos;
    int stop = stopPos;
    int variableCount = 0;
    int identyCount = 0;
    int typeCount = 0;
    int keywordCount = 0;
    int iColonCount = 0;
    int variableStart = -1;
    int variableStop = -1;
    std::set< std::pair<int, int> > localTypes;
    int curLine = -1;
	wxString variable = GetTextRange(startPos, stopPos+1);
    while(stop > start){
        char c = GetCharAt(stop);
        int style = GetStyleAt(stop);
        long line = 0;
        PositionToXY(stop, NULL, &line);
        if (curLine != line && IsInPreproces(stop, "")){
            // skip the content of preprocesor
            curLine = line;
            long lineStart = XYToPosition(0, line);
            stop = lineStart-1;
            continue;
        }
        if (style == STYLE_NORMAL && c == ']'){
            stop = BraceMatch(stop);
        }
        else if (style == STYLE_VARIABLE || style == STYLE_VARIABLE_REF){
            variableCount++;
            variableStop = stop;
            stop = FindStyleStart(style, stop);
            variableStart = stop;
        }
        else if (style == STYLE_FUNCTION){
            // for example:     bool OnInit() wxOVERRIDE;
            // in this case wxOVERRIDE is not a variable.
            return false;
        }
        else if (style == STYLE_IDENTY){
            if (variableStop < 0){
                variableStop = stop;
            }
            identyCount++;
            int iTypeStop = stop;
            stop = FindStyleStart(style, stop);
            if (variableStart < 0){
                variableStart = stop;
            }
            else{   
                localTypes.insert(std::pair<int, int>(stop, iTypeStop+1));
            }
        }
        else if (c == ')' && style == STYLE_FOLDER){
            // skip the content in between
            // todo:fanhongxuan@gmail.com
            // if the value between () is a function param list, this is a function defile.
            stop = BraceMatch(stop);
        }
        else if (style == STYLE_TYPE){
            typeCount++;
            stop = FindStyleStart(style, stop);
        }
        else if (style == STYLE_KEYWORD1){
            keywordCount++;
            int keywordStop = stop+1;
            stop = FindStyleStart(style, stop);
            wxString text = GetTextRange(stop, keywordStop);
            if (text == "return" || text == "delete" || text == "typedef"){
                return false;
            }
        }
        else if (style == STYLE_OPERATOR){
            // only can has *, &, <>, :, ","
            // "," is for:
            // std::map<int, int> map;
            if (c == ':' && stop > 1 && GetCharAt(stop-1) == ':' && GetStyleAt(stop-1) == STYLE_OPERATOR){
                // :: in variable name;
                identyCount--;
                stop--;
            }
            else if (c != '*' && c != '&' && c != '<' && c != '>' && c != ','){
                return false;
            }
        }
        stop--;
    }
    
    if (iColonCount == 1){
        return false;
    }
    
    if (variableStart < 0 || variableStop < 0){
        return false;
    }
    int totalCount = variableCount + identyCount + typeCount + keywordCount;
    if (onlyHasName){
        totalCount++;
    }
    if (totalCount < 2){
        return false;
    }
    std::set< std::pair<int, int> >::iterator it = localTypes.begin();
    while(it != localTypes.end()){
        wxString text = GetTextRange((*it).first, (*it).second);
        // wxPrintf("Mark %s as type\n", text);
        StartStyling((*it).first);
        SetStyling((*it).second - (*it).first, STYLE_TYPE);
        it++;
    }
    // wxPrintf("var:%d, id:%d, type:%d, key:%d\n", variableCount, identyCount, typeCount, keywordCount);
    // wxPrintf("IsValidVariable %s:%d)<%s>\n", GetFilename(),
    //     LineFromPosition(startPos)+1, GetTextRange(startPos, stopPos+1));
    if (NULL != pStart && NULL != pLen){
        *pStart = variableStart;
        *pLen = variableStop - variableStart+1;
    }
    return true;
}

int ceEdit::HandleVariable(int pos, int curStyle){
    // build the variable map.
    // variable is like:
    // int i;
    // int i, j, k;
    // wxString value = "";
    // int *a = 0, *p = 0;
    // int a[100] = 0;
    // int a = (b ? a : c);
    // int b, d;
    // int a = (b, d);
    // int b,
    // class test;
    // test t(100);
    // struct value = {0, 1, {can be a big expresss}, };
    // handle variable when input ,
    // input =
    // intput ;
    
    // todo:fanhongxuan@gmail.com 
    // add the variable type to mLocalTypes;
    
    if (pos == 0){
        return curStyle;
    }
    int style = GetStyleAt(pos);
	char c = GetCharAt(pos);
    if (c != ';' || style != STYLE_NORMAL){
        return curStyle;
    }
    
    std::vector<std::pair<int, int> > variables;
    std::vector<std::pair<int, int> > validVariables;
    int startPos = pos - 1;
    int stopPos = pos - 1;
    bool hasEqual = false;
    bool beforeEqual = false;
    bool hasIde = false;
    int prevStop = stopPos;
    int iParentheses = false;
    
    // check if the express has a equal.
    while(startPos > 0){
        c = GetCharAt(startPos);
        style = GetStyleAt(startPos);
        if (style != STYLE_COMMENTS && style != STYLE_CSTYLE_COMMENTS && style != STYLE_COMMENT_KEY &&
            style != STYLE_DEFAULT && style != STYLE_NORMAL && style != STYLE_FOLDER && style != STYLE_OPERATOR){
            hasIde = true;
        }
        
        if (c == ';' && style == STYLE_NORMAL){
            break;
        }
        else if (c == '=' && style == STYLE_OPERATOR){
            hasEqual = true;
            break;
        }
        else if (c == '}' && style == STYLE_FOLDER){
            if (hasIde){
                break;
            }
            // this is the init value of a array variable.
            startPos = BraceMatch(startPos);
            // the previously value must be a =, othersize, is not a variable define.
            while(startPos > 0){
                c = GetCharAt(startPos-1);
                style = GetStyleAt(startPos-1);
                if (c == '=' && style == STYLE_OPERATOR){
                    break;
                }
                else if (IsCommentOrWhiteSpace(c, style)){       
                }
                else{
                    // not a variable
                    return curStyle;
                }
                startPos--;
            }
            stopPos = startPos - 1;
        }
        else if (c == ')' && style == STYLE_FOLDER){
            startPos = BraceMatch(startPos);
            if (startPos <= 0){
                return curStyle;
            }
            stopPos = startPos;
        }
        else if (c == '{' && STYLE_FOLDER == style){
            break;
        }
        else if (c == '(' && STYLE_FOLDER == style){
            return curStyle;
        }
        startPos--;
    }
    
    hasIde = false;
    startPos = pos - 1;
    while (startPos > 0){
        c = GetCharAt(startPos);
        style = GetStyleAt(startPos);
        if (style != STYLE_COMMENTS && style != STYLE_CSTYLE_COMMENTS && style != STYLE_COMMENT_KEY &&
            style != STYLE_DEFAULT && style != STYLE_NORMAL && style != STYLE_FOLDER && style != STYLE_OPERATOR){
            hasIde = true;
        }
        if (c == ';' && style == STYLE_NORMAL){
            break;
        }
        else if (style == STYLE_KEYWORD1){
            int start = FindStyleStart(style, startPos);
            startPos = start;
        }
        else if (style == STYLE_OPERATOR && c == ':' && startPos > 1 && 
            GetStyleAt(startPos - 1) == STYLE_OPERATOR && GetCharAt(startPos -1) == ':'){
            startPos--;
        }
        else if (style == STYLE_OPERATOR && c == ':'){
            break;
        }
        else if (c == '=' && style == STYLE_OPERATOR){
            beforeEqual = true;
            // todo:fanhongxuan@gmail.com
            // need to support variable define like:
            // int a = 0, b = 0, c = 0;
            // we don't check the status after '='
            prevStop = startPos - 1;
        }
        else if (c == '>' && style == STYLE_OPERATOR){
            // if this is after the =, skip this,
            // if this is before the =, it must be pair,
            // and we skip the content between <>
            // maybe a template define like:
            // std::map<int, int> map;
            if (beforeEqual || (!hasEqual)){
                int start = FindTemplateParamStart(startPos);
                if (start < 0){
                    return curStyle;
                }
                startPos = start;
            }
            else{
            }
        }
        else if (c == ',' && style == STYLE_OPERATOR){
            beforeEqual = false;
            variables.push_back(std::pair<int, int>(startPos, prevStop));
            prevStop = startPos - 1;
        }
        else if (c == '}' && style == STYLE_FOLDER){
            if (hasIde){
                break;
            }
            
            // this is the init value of a array variable.
            startPos = BraceMatch(startPos);
            // the previously value must be a =, othersize, is not a variable define.
            while(startPos > 0){
                c = GetCharAt(startPos -1 );
                style = GetStyleAt(startPos - 1);
                if (c == '=' && style == STYLE_OPERATOR){
                    break;
                }
                else if (IsCommentOrWhiteSpace(c, style)){
                    
                }
                else{
                    // not a variable
                    return curStyle;
                }
                startPos--;
            }
            stopPos = startPos - 1;
        }
        else if (c == '{' && STYLE_FOLDER == style){
            break;
        }
        else if (c == '(' && STYLE_FOLDER == style){
            return curStyle;
        }
        else if (c == ')' && style == STYLE_FOLDER){
            startPos = BraceMatch(startPos);
            if (startPos <= 0){
                break;
            }
            iParentheses++;
            // if this is the param list of a constructor function call
            // if this is the init value,
            // needEqual = true;
            stopPos = startPos;
        }
        startPos--;
    }
	if (startPos < 0){
		startPos = 0;
	}
    
    if (iParentheses > 1){
        return curStyle;
    }
    
    if (startPos >= stopPos){
        return curStyle;
    }
    variables.push_back(std::pair<int,int>(startPos, prevStop));
    for (int i = 0; i < variables.size(); i++){
        int start = 0, len = 0;
        if (!IsValidVariable(variables[i].first, 
                variables[i].second,
                i != variables.size()-1, &start, &len)){
            return false;
        }
        validVariables.push_back(std::pair<int, int>(start, len));
    }
    for (int i = 0; i < validVariables.size(); i++){
        wxString text = GetTextRange(validVariables[i].first, validVariables[i].first + validVariables[i].second);
        // wxPrintf("Mark <%s> as variable\n", text);
        StartStyling(validVariables[i].first);
        SetStyling(validVariables[i].second, STYLE_VARIABLE);
        // std::map<wxString, int>::iterator it = mLocalVariable.find(text);
        // mLocalVariable[text] = std::make_pair<int, wxString>(pos, GetVariableType(pos));
        mLocalVariable[text] = std::pair<int, wxString>(pos, GetVariableType(pos));
    }
    return true;
}

bool ceEdit::IsVariableValid(const wxString &variable, int pos){
    std::map<wxString, std::pair<int, wxString> >::iterator it = mLocalVariable.find(variable);
    if (it == mLocalVariable.end()){
        return false;
    }
    return true;
}

bool ceEdit::IsValidParam(int startPos, int stopPos){
    // if a valid param?
    // should be the following type:
    // keyword type *& name = "defaultValue"
    // only can have, & * =, in a param list,
    // if (param.empty()){
    //     return true;
    // }
    // std::vector<wxString> outputs;
    // ceSplitString(param, outputs, ' ');
    // one keyword + one id,
    // one type + one id.
    // two id.
    // only can have the follwoing operator
    // * & =, **,
    // :<> ==> std::map<std::string, std::string> map
    // // wxPrintf
    if (startPos > stopPos){
        // wxPrintf("StopPos > stopPos, invalid\n");
        return false;
    }
    if (startPos == stopPos){
        // wxPrintf("startPos == stopPos\n");
        return true;
    }
    
    int pos = stopPos;
    int iEqual = 0;
    while(pos >= startPos){
        char c = GetCharAt(pos);
        int style = GetStyleAt(pos);
        if (style == STYLE_OPERATOR && c == '='){
            iEqual++;
            stopPos = pos - 1;
            if (iEqual > 1){
                // not a valid param.
                return false;
            }
        }
        pos--;
    }
    // todo:fanhongxuan@gmail.com
    // normally we need to skip the comments
    // later change GetTextRange to skip the comments and whitespace
    wxString param = GetTextRange(startPos, stopPos);
    // wxPrintf("param:%s\n", param);
    pos = stopPos;
    int keywordCount = 0;
    int typeCount = 0;
    int idCount = 0;
    int iCount = 0;
    int operatorCount = 0;
    while(pos >= startPos){
        int style = GetStyleAt(pos);
        char c = GetCharAt(pos);
        if (STYLE_OPERATOR == style){
            if (c == '>'){
                int start = FindTemplateParamStart(pos);
                if (start < 0){
                    return false;
                }
                pos = start-1;
                continue;
            }
            else if (c == '&' || c == '*'){
                // supported.
            }
            else if (c == ':' && pos > 1 && GetCharAt(pos-1) == ':' && GetStyleAt(pos-1) == STYLE_OPERATOR){
                // supported
                pos--; // skip the previously ':'
            }
            else{
                return false;
            }
            operatorCount++;
        }
        else if (style == STYLE_NUMBER || style == STYLE_CHAR || style == STYLE_STRING){
            // the init value is not handled by IsValidVariable
            // if meet a number, char, string, mean this is not a valid function define variable.
            return false;
        }
        else if (STYLE_IDENTY == style || STYLE_VARIABLE == style || STYLE_VARIABLE_REF == style){
            idCount++;
        }
        else if (STYLE_PARAMETER == style|| STYLE_PARAMETER_REF == style){
            idCount++;
        }
        else if (STYLE_TYPE == style){
            typeCount++;
        }
        else if (STYLE_KEYWORD1 == style){
            keywordCount++;
        }
        if (style == STYLE_IDENTY || style == STYLE_TYPE || style == STYLE_KEYWORD1 || 
            style == STYLE_PARAMETER || style == STYLE_PARAMETER_REF || 
            style == STYLE_VARIABLE || style == STYLE_VARIABLE_REF){
            int start = FindStyleStart(style, pos);
            if (start < startPos){
                // // wxPrintf("<%s> is not a valid param2\n", param);
                return false;
            }
            else{
                param = GetTextRange(start, pos+1);
                pos = start;
            }
        }
        pos--;
    }
    if ((typeCount + keywordCount + idCount + operatorCount) == 0){
        // empty param
        return true;
    }
    if (keywordCount == 1 && param == "void"){
        // single void is a valid param.
        return true;
    }
    if (idCount < 1){
        // wxPrintf("<%s> is not a valid param1\n", param);
        return false;
    }
    if ((typeCount+keywordCount + idCount) < 2){
        // wxPrintf("<%s> is not a valid param\n", param);
        return false;
    }
    
    // wxPrintf("<%s> is a valid param\n", param);
    return true;
}

wxString ceEdit::GetParamType(int pos){
    wxString str;
    int start = pos;
    while(start > 0){
        int style = GetStyleAt(start-1);
        char c = GetCharAt(start-1);
        if (c == ';' && style == STYLE_NORMAL){
            break;
        }
        else if ((c == '}' || c == '{' || c == '(' || c == ')') && style == STYLE_FOLDER){
            break;
        }
        else if (c == '>' && style == STYLE_OPERATOR){
            int startPos = start-2;
            int count = 1;
            while(startPos > 0){
                if (GetCharAt(startPos) == '>'){
                    count++;
                }
                if (GetCharAt(startPos) == '<'){
                    count--;
                    if (count == 0){
                        break;
                    }
                }
                startPos --;
            }
            start = startPos;
        }
        else if (c == ',' && style == STYLE_OPERATOR){
            break;
        }
        else if (c == ':' && style == STYLE_OPERATOR){
            // meat a :
            if (start > 2 && GetCharAt(start-2) == ':' && GetStyleAt(start-2) == STYLE_OPERATOR){
                // this is a ::
                start--;
            }
            else{
                break;
            }
        }
        start--;
    }
    // skip the following type:
    while(start < pos){
        int style = GetStyleAt(start);
        int c = GetCharAt(start);
        if (!IsCommentOrWhiteSpace(c, style)){
            if (style == STYLE_PREPROCESS){
                // skip the entileline.
                while(start < pos){
                    if (GetCharAt(start+1) == '\r' || GetCharAt(start+1) == '\n'){
                        break;
                    }
                    start++;
                }
            }
            else{
                break;
            }
        }
        start++;
    }
    int stop = start;
    while (stop < pos){
        int style = GetStyleAt(stop+1);
        int c = GetCharAt(stop+1);
        if (c == '=' && style == STYLE_OPERATOR){
            break;
        }
        else if (style == STYLE_PARAMETER){
            break;
        }
        stop++;
    }
    
    str = GetTextRange(start, stop);
    // wxPrintf("GetParamType:%s\n", str);
    return str;
}

wxString ceEdit::GetVariableType(int pos)
{
    wxString str;
    int start = pos;
    while(start > 0){
        int style = GetStyleAt(start-1);
        char c = GetCharAt(start-1);
        if (c == ';' && style == STYLE_NORMAL){
            break;
        }
        else if ((c == '}' || c == '{') && style == STYLE_FOLDER){
            break;
        }
        else if (c == ':' && style == STYLE_OPERATOR){
            // meat a :
            if (start > 2 && GetCharAt(start-2) == ':' && GetStyleAt(start-2) == STYLE_OPERATOR){
                // this is a ::
                start--;
            }
            else{
                break;
            }
        }
        start--;
    }
    // skip the following type:
    while(start < pos){
        int style = GetStyleAt(start);
        int c = GetCharAt(start);
        if (!IsCommentOrWhiteSpace(c, style)){
            if (style == STYLE_PREPROCESS){
                // skip the entileline.
                while(start < pos){
                    if (GetCharAt(start+1) == '\r' || GetCharAt(start+1) == '\n'){
                        break;
                    }
                    start++;
                }
            }
            else{
                break;
            }
        }
        start++;
    }
    int stop = start;
    while (stop < pos){
        int style = GetStyleAt(stop+1);
        int c = GetCharAt(stop+1);
        if (c == '=' && style == STYLE_OPERATOR){
            break;
        }
        else if (style == STYLE_VARIABLE){
            break;
        }
        stop++;
    }
    
    str = GetTextRange(start, stop);
    // wxPrintf("GetVariableType:<%s>\n", str);
    return str;
}

int ceEdit::PrepareFunctionParams(int pos){
    long start, stop;
    mFunctionParames.clear();
    mLocalVariable.clear();
    wxString func = WhichFunction(pos, &start, &stop);
    if (func.empty()){
        return 0;
    }
    
    if (start != 0 && start != stop){
        while (pos > start){
            char c = GetCharAt(pos);
            int style = GetStyleAt(pos);
            if (c == '}' && style == STYLE_FOLDER){
                pos = BraceMatch(pos);
            }
            if (style == STYLE_VARIABLE){
                int variableStart = FindStyleStart(style, pos);
                if (variableStart >= 0  && variableStart <= pos){
                    wxString variable = GetTextRange(variableStart, pos+1);
                    wxString type = GetVariableType(variableStart-1);
                    mLocalVariable[variable] = std::pair<int,wxString>(pos, type);
                    pos = variableStart;
                }
            }
            pos--;
        }
        stop = start;
        while(start > 0){
			char c = GetCharAt(start);
            int style = GetStyleAt(start);
            if (style == STYLE_PARAMETER){
                int paramStart = FindStyleStart(STYLE_PARAMETER, start);
                if (paramStart >= 0 && paramStart <= start){
                    wxString param = GetTextRange(paramStart, start+1);
                    // wxPrintf("Add Param:%s\n", param);
                    wxString type = GetParamType(paramStart-1);
                    mFunctionParames[param] = std::pair<int, wxString>(stop, type);
                }
                start = paramStart - 1;
            }
            
            else if (style == STYLE_FUNCTION){
                break;
            }
            start--;
        }
    }
    // wxPrintf("We are in function<%s>\n", func);
    return 0;
}

bool ceEdit::HandleParam(int startPos, int stopPos){
    // param should like:
    wxString text = GetTextRange(startPos, stopPos);
    // wxPrintf("handleParam:<%s>(%d)\n", text, LineFromPosition(startPos)+1);
    int pos = stopPos;
    int paramStop = stopPos;
    while(pos >= startPos){
        char c = GetCharAt(pos);
        int style = GetStyleAt(pos);
        if (c == ',' && style == STYLE_OPERATOR){
            if(!IsValidParam(pos+1, paramStop)){
                return false;
            }
            paramStop = pos - 1;
        }
        else if (c == '>' && style == STYLE_OPERATOR){
            int start = FindTemplateParamStart(pos);
            if (start < 0){
                return false;
            }
            pos = start;
        }
        else if (c == '(' && style == STYLE_FOLDER){
            if (!IsValidParam(pos+1, paramStop)){
                return false;
            }
        }
        pos --;
    }
    
    // clear the function param map, first.
    mFunctionParames.clear();
    mLocalVariable.clear();
    // wxPrintf("Clear function params\n");
    
    // mark the last value before the =, as the param.
    bool bParam = true;
    pos = stopPos;
    while(pos >= startPos){
        char c = GetCharAt(pos);
        int style = GetStyleAt(pos);
        if (style == STYLE_IDENTY || style == STYLE_PARAMETER || style == STYLE_PARAMETER_REF || style == STYLE_TYPE){
            int start = FindStyleStart(style, pos);
            if (start <= startPos){
                break;
            }
            {
                // if this id is started with a =, skip this one,
                // should be init value.
                int prev = start;
                bool hasEqual = false;
                while(prev >= startPos){
                    char c = GetCharAt(prev);
                    int style = GetStyleAt(prev);
                    if (style == STYLE_OPERATOR && c == '='){
                        hasEqual = true;
                        break;
                    }
                    else if (style == STYLE_OPERATOR && c == ','){
                        break;
                    }
                    prev--;
                }
                if (hasEqual){
                    pos = prev;
                    continue;
                }
            }
            StartStyling(start);
            if (bParam){
                SetStyling(pos+1 - start, STYLE_PARAMETER);
                bParam = false;
                wxString text = GetTextRange(start, pos+1);
                // wxPrintf("AddFunction param:%s(%d)\n", text, LineFromPosition(pos+1) + 1);
                // mFunctionParames[text] = std::make_pair(-1, GetParamType(start));
                mFunctionParames[text] = std::pair<int, wxString>(-1, GetParamType(start));
            }
            else{
                wxString text = GetTextRange(start, pos+1);
                // wxPrintf("Add Local Type:<%s>(%d)\n", text, LineFromPosition(pos+1) + 1);
                mLocalTypes.insert(text);
                SetStyling(pos+1 - start, STYLE_TYPE);
                // bParam = true;
            }
            pos = start;
            //continue;
            // find the next ','
        }
        else if (c == ',' && style == STYLE_OPERATOR){
            bParam = true;
        }
        else if (c == '>' && style == STYLE_OPERATOR){
            int start = FindTemplateParamStart(pos);
            if (start < 0){
                return false;
            }
            pos = start;
        }
        pos--;
        //
    }
    return true;
}

int ceEdit::HandleFunctionBody(int pos, int curStyle){
    if (pos == 0){
        return curStyle;
    }
    int style = GetStyleAt(pos);
	char c = GetCharAt(pos);
    if ( c == '{' && style == STYLE_FOLDER){
        if (mFunctionParames.size() != 0 && mFunctionParames.begin()->second.first == -1){
            mFunctionParames.begin()->second.first = pos;
        }
    }
    else if (c == '}' && style == STYLE_FOLDER){
        int start = BraceMatch(pos);
        if (mFunctionParames.size() != 0){
            if(start == mFunctionParames.begin()->second.first){
                // wxPrintf("clear function param on function end\n");
                mFunctionParames.clear();
            }
        }
        // delete all the local variable which is not valid anymore.
        std::map<wxString, std::pair<int, wxString> >::iterator it = mLocalVariable.begin();
        while(it != mLocalVariable.end()){
            std::map<wxString, std::pair<int, wxString> >::iterator cur = it;
            it++;
            if (cur->second.first > start){
                mLocalVariable.erase(cur);
            }
        }
    }
    else if (c == ';' && style == STYLE_NORMAL){
        // end of a function declare, clear the function param table;
        if (mFunctionParames.size() != 0 && mFunctionParames.begin()->second.first == -1){
            // wxPrintf("clear function param on function declear end\n");
            mFunctionParames.clear();
        }
    }
    return curStyle;
}

// stop is point to the '>'
// return the pos of <, if no valid start find, return -1;
int ceEdit::FindTemplateParamStart(int stop)
{
    int start = stop;
    int count = 0;
    while(start > 0){
        char c = GetCharAt(start);
        int style = GetStyleAt(start);
        if (c == '>' && (style == STYLE_OPERATOR || style == STYLE_DEFAULT)){
            count++;
        }
        else if (c == '<' && (style == STYLE_OPERATOR || style == STYLE_DEFAULT)){
            count--;
        }
        if (style == STYLE_FOLDER){
            // in template param, can not have {,}, (,)'
            return -1;
        }
        if (count == 0){
            break;
        }
        start--;
    }
    return start;
}


int ceEdit::HandleFunctionStart(int pos, int curStyle){
    if (pos == 0){
        return curStyle;
    }
    
    int style = GetStyleAt(pos);
	char c = GetCharAt(pos);
    if (c != ')' || style != STYLE_FOLDER){
        return curStyle;
    }
    
    long paramStop = pos;
    long paramStart = BraceMatch(paramStop);
    if (paramStart <= 0){
        return curStyle;
    }
    int stopPos = 0;
    int startPos = paramStart - 1;
    bool bNeedReturn = true;
    if (mLanguage == "Java"){
        // bNeedReturn = false; // Java function can have no return value.
    }
    
    // 1, find the function name start.
    while(startPos > 0){
        style = GetStyleAt(startPos);
        c = GetCharAt(startPos);
        if (style == STYLE_IDENTY){
            stopPos = startPos;
            startPos = FindStyleStart(STYLE_IDENTY, startPos);
            break;
        }
        else if (style == STYLE_FUNCTION){
            stopPos = startPos;
            startPos = FindStyleStart(style, startPos);
            break;
        }
        else if (style == STYLE_TYPE){
            // this is the constructor of a type, don't need a return type.
            bNeedReturn = false;
            stopPos = startPos;
            startPos = FindStyleStart(STYLE_TYPE, startPos);
            break;
        }
        else if (style == STYLE_OPERATOR){
            break;
        }
        else if (style == STYLE_KEYWORD1){
            int keyStart = FindStyleStart(STYLE_KEYWORD1, startPos);
            if (keyStart < startPos){
                wxString value = GetTextRange(keyStart, startPos+1);
                // only const/mutable can be used to describe a function before the ().
                if (value != "const" && value != "mutable"){
                    return curStyle;
                }
            }
            startPos = keyStart;
        }
        else if (style == STYLE_NORMAL && c == ';'){
            break;
        }
        else if (style == STYLE_FOLDER){
            if (c == '(' || c == ')'){
                // function can not be started after ( or ).
                return curStyle;
            }
            break;
        }
        startPos--;
    }
    
    // 
    int returnStart = -1;
    int returnStop = -1;
    if (startPos > 0){
        bool hasReturnValue = false;
        bool bUseTypeAsId = false;
        int prev = startPos - 1;
        while(prev > 0){
            style = GetStyleAt(prev);
            c = GetCharAt(prev);
            if (style == STYLE_OPERATOR){
                if (c == ':' && prev > 1 && GetStyleAt(prev-1) == STYLE_OPERATOR && GetCharAt(prev-1) == ':'){
                    // ::function, 
                    bUseTypeAsId = true;
                    prev--;
                    // break;
                }
                else if (c == ':' && style == STYLE_OPERATOR){
                    break;
                }
                else if (c == '~'){
                    // destructor a class, don't need a return type.
                    bNeedReturn = false;
                    // hasReturnValue = true;
                    break;
                }
                else if (c == '&'){
                    // return type is a reference
                }
                else if (c == '*'){
                    // return type is a pointer
                }
                else if (c == '>'){
                    // maybe return type is a template<>
                    int start = FindTemplateParamStart(prev);
                    if (start < 0){
                        // not a valid template.
                        return curStyle;
                    }
                    prev = start;
                }
                else{
                    // other operator, not a function.
                    // wxPrintf("Other operator:%c\n", c);
                    return curStyle;
                }
            }
            else if (style == STYLE_IDENTY){
                returnStop = prev;
                returnStart = FindStyleStart(STYLE_IDENTY, returnStop);
                prev = returnStart;
                hasReturnValue = true;
            }
            else if (style == STYLE_TYPE){
                if (bUseTypeAsId){
                    prev = FindStyleStart(style, prev);
                    bUseTypeAsId = false;
                }
                else{
                    hasReturnValue = true;
                }
            }
            else if (style == STYLE_KEYWORD1){
                int keyStart = FindStyleStart(style, prev);
                wxString text = GetTextRange(keyStart, prev+1);
                // wxPrintf("keyword in return is:<%s>\n", text);
                if (text == "return" || text == "new" || text == "delete"){
                    return curStyle;
                }
                hasReturnValue = true;
                break;
            }
            else if (c == '}' && style == STYLE_FOLDER){
                break;
            }
            else if (c == ';' && style == STYLE_NORMAL){
                break;
            }
            else if ( c == '{' && style == STYLE_FOLDER){
                if (bNeedReturn){
                    if (!hasReturnValue){
                        return curStyle;
                    }
                }
                else{
                    break;
                }
            }
            else if ((c == ')'|| c == '(') && style == STYLE_FOLDER){
                return curStyle; // no ; find, not a function
            }
            prev--;
        }

        // todo:fanhongxuan@gmail.com
        // mark the return type as a type.
        // class constructor has no return value, need to handle it here.
        if (bNeedReturn && (!hasReturnValue)){
            // wxPrintf("No Return value, not a function:%s\n", GetTextRange(prev, pos));
            return curStyle;
        }
        
        // verify the param
        wxString param = GetTextRange(paramStart, paramStop);
        if (!HandleParam(paramStart, paramStop)){
            return curStyle;
        }
        
        if (returnStart >= 0 && returnStop >= returnStart){
            mLocalTypes.insert(GetTextRange(returnStart, returnStop+1));
            StartStyling(returnStart);
            SetStyling(returnStop+1 - returnStart, STYLE_TYPE);
        }
        
        StartStyling(startPos);
        SetStyling(stopPos+1 - startPos, STYLE_FUNCTION);
    }
    return curStyle;
}

wxString ceEdit::GetClassName(int pos){
    wxString func;
    int nameStart = pos;
    int nameStop = pos;
    bool hasFindClassName = false;
    while(nameStart > 0){
        char c = GetCharAt(nameStart-1);
        int style = GetStyleAt(nameStart-1);
        if (!IsCommentOrWhiteSpace(c, style)){
            if (hasFindClassName){
                if (style == STYLE_IDENTY || style == STYLE_TYPE){
                    nameStop = FindStyleStart(style, nameStart);
                    func = GetTextRange(nameStop, nameStart) + "::";
                }
                break;
            }
            else if ((!hasFindClassName) && c == ':' && (style == STYLE_OPERATOR||style == STYLE_DEFAULT) && nameStart > 2 && 
                GetCharAt(nameStart-2) == ':' && (GetStyleAt(nameStart-2) == STYLE_OPERATOR || GetStyleAt(nameStart-2)== 0)){
                hasFindClassName = true;
                nameStart -= 2;
                continue;
            }
            break;
        }
        nameStart--;
    }
    return func;
}

wxString ceEdit::WhichType(int pos){
    return "";
}

wxString ceEdit::WhichClass(int pos){
    // find current class name
    // if we are in the body of a class, return the class name,
    // other wise, return empty string.
    if (pos == 0){
        return "";
    }
    long startPos = 0, stopPos = 0;
    GetMatchRange(pos, startPos, stopPos, '{', '}');
    if (startPos == 0 && stopPos == GetLastPosition()){
        return "";
    }
    bool isClass = false;
    int nameStart = startPos;
    while(nameStart > 0){
        char c = GetCharAt(nameStart-1);
        int style = GetStyleAt(nameStart - 1);
        if (c == ';' && style == STYLE_OPERATOR){
            break;
        }
        else if (c == '}' && style == STYLE_FOLDER){
            break;
        }
        else if (c == '{' && style == STYLE_FOLDER){
            startPos = nameStart - 1;
        }
        else if (c == 's' && style == STYLE_KEYWORD1){
            int stop = nameStart-1;
            wxString key = GetPrevValue(stop, style, &nameStart);
            if (key == "class"){
                nameStart = stop+1;
                isClass = true;
                break;
            }
        }
        nameStart--;
    }
    if (!isClass){
        return WhichClass(nameStart);
    }
    wxString value;
    bool bNameStart = false;
    while(nameStart < startPos){
        char c = GetCharAt(nameStart);
        int style = GetStyleAt(nameStart);
        if (style == STYLE_TYPE || style == STYLE_IDENTY){
            if (!bNameStart){
                bNameStart = true;
            }
            value += c;
        }
        else if (bNameStart){
            break;
        }
        nameStart++;
    }
    return value;
}

// return the function name of the respond pos;
// if not in any function, return an empty string.
wxString ceEdit::WhichFunction(int pos, long *pStart, long *pStop){
    // find the {}, if the before the {}, has a
    if (pos == 0){
        return "";
    }
    long startPos = 0, stopPos = 0;
    GetMatchRange(pos, startPos, stopPos, '{', '}');
    if (startPos == 0 && stopPos == GetLastPosition()){
        if (NULL != pStart && NULL != pStop){
            *pStart = 0;
            *pStop = 0;
        }
        return "";
    }
    bool isFunction = false;
    int nameStart = startPos;
    while(nameStart > 0){
        int c = GetCharAt(nameStart-1);
        int style = GetStyleAt(nameStart -1);
        if (c == ';' && style == STYLE_OPERATOR){
            break;
        }
        else if (c == '}' && style == STYLE_FOLDER){
            break;
        }
        else if (c == '{' && style == STYLE_FOLDER){
            startPos = nameStart-1;
        }
        if (style == STYLE_FUNCTION){
            isFunction = true;
            // here we get the function.
			break;
        }
        nameStart--;
    }
    if (!isFunction){
        return WhichFunction(nameStart, pStart, pStop);
    }
    // this is a function, return the real name
    int nameStop = nameStart;
    nameStart = FindStyleStart(STYLE_FUNCTION, nameStop);
    if (nameStart > 1 && GetCharAt(nameStart-1) == '~' && GetStyleAt(nameStart-1) == STYLE_OPERATOR){
        // this is the deconstructor.
        nameStart--;
    }
    if (NULL != pStop && NULL != pStart){
        *pStart = startPos;
        *pStop = BraceMatch(startPos);
    }
    
    return GetClassName(nameStart) +  GetTextRange(nameStart, nameStop);
}

extern const wxString &getFullTypeName(const wxString &input, const wxString &language);

bool ceEdit::ClearLoalSymbol(){
    std::map<wxString, std::set<ceSymbol *>* >::iterator mIt = mSymbolMap.begin();
    while(mIt != mSymbolMap.end()){
        std::set<ceSymbol *>::iterator sIt = mIt->second->begin();
        while(sIt != mIt->second->end()){
            delete (*sIt);
            sIt++;
        }
        delete mIt->second;
        mIt++;
    }
    mSymbolMap.clear();
    
    std::set<ceSymbol*>::iterator sIt = mLocalSymbolMap.begin();
    while(sIt != mLocalSymbolMap.end()){
        delete (*sIt);
        sIt++;
    }
    mLocalSymbolMap.clear();
    return true;
}

bool ceEdit::BuildLocalSymbl(){
    std::set<wxString>::iterator tIt;
    std::set<ceSymbol *>::iterator it;
    
    if (mLocalSymbolMap.empty()){
        ceSymbolDb::GetFileSymbol(GetFilename(), mLocalSymbolMap);
    }
    if (mLocalTypes.empty()){
        // todo:fanhongxuan@gmail.com
        // add class to the local types
        it = mLocalSymbolMap.begin();
        while(it != mLocalSymbolMap.end()){
            std::vector<wxString> outputs;
            wxPrintf("symbolType:%s, type:<%s>\n", (*it)->symbolType, (*it)->type);
            ceSplitString((*it)->type, outputs, ' ');
            int i = 0;
            for (i = 0; i < outputs.size(); i++){
                mLocalTypes.insert(outputs[i]);
            }
            it++;
        }
    }
    return true;
}

wxString ceEdit::FindType(const wxString &value, int line, int pos){
    wxString type;
    std::set<wxString>::iterator tIt;
    std::set<ceSymbol *>::iterator it;
    
    BuildLocalSymbl();
    
    // if mLocalSymbolMap && mLocalTypes is empty, try to load it first.
    tIt = mLocalTypes.find(value);
    if (tIt != mLocalTypes.end()){
        return "Typedef";
    }
    
    it = mLocalSymbolMap.begin();
    while(it != mLocalSymbolMap.end()){
        ceSymbol *pSymbol = (*it);
        if (NULL != pSymbol && pSymbol->name == value){
            return getFullTypeName(pSymbol->symbolType, mLanguage);
        }
        it++;
    }
    
    // todo:fanhongxuan@gmail.com
    // check if this is a global variable, enum, macro define?
    return "";
    // std::map<wxString, std::set<ceSymbol *>* >::iterator mIt = mSymbolMap.find(value);
    // if (mIt != mSymbolMap.end() && mIt->second->size() != 0){
    //     std::set<ceSymbol *>::iterator sit = mIt->second->begin();
    //     return (*sit)->type;
    // }
    // if (NULL == wxGetApp().frame()){
    //     return type;
    // }
    // std::set<ceSymbol *> *symbol = new std::set<ceSymbol *>;
    // wxGetApp().frame()->FindDef(*symbol, value, type, GetFilename());
    // if (symbol->empty()){
    //     return type;
    // }
    // mSymbolMap[value] = symbol;
    // return (*symbol->begin())->type;
}

bool ceEdit::IsFunctionDeclare(int pos){
    return true;
}

bool ceEdit::IsFunctionDefination(int pos){
    // return_value class:function(args,){}
    //
	return true;
}

bool ceEdit::ParseWord(int pos){
    int startPos = pos;
    while(startPos > 0){
        if (GetStyleAt(startPos-1) != STYLE_IDENTY){
            break;
        }
        startPos--;
    }
    wxString text = GetTextRange(startPos, pos+1);
    if (IsKeyWord1(text, mLanguage)){
        StartStyling(startPos);
        SetStyling(pos+1 - startPos, STYLE_KEYWORD1);
        return true;
    }
    else if (IsKeyWord2(text, mLanguage)){
        StartStyling(startPos);
        SetStyling(pos+1 - startPos, STYLE_KEYWORD2);
        return true;
    }
    
    // if this identy is started with a class,
    // this is a class define.
    {
        wxString prevValue = GetPrevValue(startPos-1, STYLE_KEYWORD1);
        if (prevValue == "class" || prevValue == "struct" || 
            prevValue == "protected" || prevValue == "private" || prevValue == "public"){
            StartStyling(startPos);
            SetStyling(pos+1 - startPos, STYLE_TYPE);
            mLocalTypes.insert(text);
            return true;
        }
    }
    
    // fixme:fanhongxuan@gmail.com
    // if a word is started with ., ->, ::, don't mark it as variable and function param and type.
    std::set<wxString>::iterator tIt = mLocalTypes.find(text);
    if (tIt != mLocalTypes.end()){
        StartStyling(startPos);
        SetStyling(pos+1 - startPos, STYLE_TYPE);
        return true;
    }
    
    if (IsVariableValid(text, pos)){
        StartStyling(startPos);
        SetStyling(pos+1 - startPos, STYLE_VARIABLE_REF);
        return true;
    }
    
    std::map<wxString, std::pair<int, wxString> >::iterator pIt = mFunctionParames.find(text);
    if (pIt != mFunctionParames.end()){
        // the mFunctionParames table will be cleared when function is end by ; or }
        // wxPrintf("Set <%s><%d> as a param\n", text, LineFromPosition(pos));
        StartStyling(startPos);
        SetStyling(pos+1 - startPos, STYLE_PARAMETER_REF);
        return true;
    }
    
    // wxString type = FindType(text);
    // if (!type.empty()){
    //     wxPrintf("%s is %s\n", text, type);
    //     if (type == "Typedef"){
    //         StartStyling(startPos);
    //         SetStyling(pos+1 - startPos, STYLE_TYPE);
    //     }
    //     else if (type == "Macro" || type == "Enumerator"){
    //         StartStyling(startPos);
    //         SetStyling(pos+1 - startPos, STYLE_MACRO);
    //     }
    //     else if (type == "Function" || type == "Prototype"){
    //         StartStyling(startPos);
    //         SetStyling(pos+1 - startPos, STYLE_FUNCTION);
    //     }
    //     else if (type == "Parameter"){
    //         StartStyling(startPos);
    //         SetStyling(pos+1 - startPos, STYLE_PARAMETER);
    //     }
    //     else if (type == "Local"){
    //         StartStyling(startPos);
    //         SetStyling(pos+ 1 - startPos, STYLE_LOCAL_VARIABLE);
    //     }
    // }
    
    // todo:fanhongxuan@gmail.com
    // function call is like:
    // ::function();
    // ->function();
    // ->function();
    return true;
}

int ceEdit::GetFoldLevelDelta(int line, int &indentChange){
    int ret = 0;
    int start = XYToPosition(0, line);
    int stop = GetLineEndPosition(line);
    // wxPrintf("GetFoldLevelDelta:%d(%d<->%d)\n", line, start, stop);
    int delta = 0;
    for (int i = start; i < stop; i++){
        char c = GetCharAt(i);
        int style = GetStyleAt(i);
        if ((c == '{') && style == STYLE_FOLDER){
            indentChange += GetIndent();
            ret++;
        }
        else if (c == '}' && style == STYLE_FOLDER){
            indentChange -= GetIndent();
            ret--;
        }
        else if ((c == '(' || c == ')') && style == STYLE_FOLDER){
            int match = BraceMatch(i);
            if (match > stop || match < start){
                // not in the same line, 
                // need to change it.
                if (c == '('){
                    delta = i - start - GetLineIndentation(line)+1;
                    // wxPrintf("Delta:<%d><%d>\n", line+1, delta);
                    ret++;
                }
                else{ // c == ')'
                    long startLine = 0;
                    PositionToXY(match, NULL, &startLine);
                    delta = -GetLineState(startLine);
                    ret--;
                }
            }
        }
        else if (c == '/' && (i+1) < stop && GetCharAt(i+1) == '*' && style == STYLE_CSTYLE_COMMENTS){
            indentChange += 1;
            ret++;
            i++;
        }
        else if (c == '*' && (i+1) < stop && GetCharAt(i+1) == '/' && style == STYLE_CSTYLE_COMMENTS){
            indentChange -= 1;
            i++;
            ret--;
        }
    }
    
    indentChange += delta;
    
    // support fold by preprocessor;
    wxString text = GetLineText(line);
    int startChar = text.find_first_not_of("\r\n\t ");
    int endChar = text.find_last_not_of("\r\n\t ");
    if (startChar != text.npos && endChar != text.npos && endChar >= startChar){
        text = text.substr(startChar, endChar + 1-startChar);
    }
    
    // #ifdef ret++
    int pos = text.find("#ifdef");
    if (pos != text.npos && GetStyleAt(start+ pos) == STYLE_PREPROCESS){
        ret++;
        return ret;
    }
    
    pos = text.find("#ifndef");
    if (pos != text.npos && GetStyleAt(start+pos) == STYLE_PREPROCESS){
        ret++;
        return ret;
    }
    
    pos = text.find("#if");
    if (pos != text.npos && GetStyleAt(start+pos) == STYLE_PREPROCESS){
        ret++;
        return ret;
    }    
#if 0    
        pos = text.find("#else");
        if (pos != text.npos && GetStyleAt(start+pos) == STYLE_PREPROCESS){
            ret++;
        }
        
        pos = text.find("#eldef");
        if (pos != text.npos && GetStyleAt(start+pos) == STYLE_PREPROCESS){
            ret++;
        }
        
        pos = text.find("#elif");
        if (pos != text.npos && GetStyleAt(start+pos) == STYLE_PREPROCESS){
            ret++;
        }
#endif    
    pos = text.find("#endif");
    if (pos != text.npos && GetStyleAt(start+pos) == STYLE_PREPROCESS){
        ret--;
        return ret;
    }

    return ret;
}

bool ceEdit::HandleFolder(long pos)
{
    // get the folder level of previously line.
    // child's fold level is parent+1
    // fold header has wxSTC_FOLDLEVELHEADERFLAG 
    // empty line set wxSTC_FOLDLEVELWHITEFLAG
    int line = LineFromPosition(pos);
    int indentChange = 0;
    int foldLevel = wxSTC_FOLDLEVELBASE;
    if (line != 0){
        foldLevel = GetFoldLevel(line - 1) & wxSTC_FOLDLEVELNUMBERMASK;
        int delta = GetFoldLevelDelta(line - 1, indentChange);
        foldLevel = foldLevel + delta;
        // wxPrintf("setLineIndentationChange:%d\n", indentChange);
        SetLineState(line-1, indentChange);
        // if (indentChange < 0 && line > 1){
        //     SetLineState(line-2, indentChange);
        //     SetLineState(line-1, 0);
        // }
        if (foldLevel < wxSTC_FOLDLEVELBASE){
            foldLevel = wxSTC_FOLDLEVELBASE;
        }
    }
    
    int delta = GetFoldLevelDelta(line, indentChange);
    if (delta > 0){
        foldLevel |= wxSTC_FOLDLEVELHEADERFLAG;
    }
    wxString text = GetLineText(line);
    if (text.find_first_not_of("\r\n\t ") == text.npos){
        foldLevel |= wxSTC_FOLDLEVELWHITEFLAG;
    }
    // store the indent level in SetLineState;
    SetFoldLevel(line, foldLevel);
    return true;
}

int ceEdit::ParseCharInDefault(char c, int curStyle, long pos)
{
    static wxString sIdStart = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static wxString sNumberStart = ".0123456789";
    static wxString sPreprocess = "#";
    static wxString sOperator = "~!%^&*+-<>/?:|.=,";
    static wxString sFolder = "(){}";
    static wxString sNormal = ";[]";
    static wxString sWhitespace = "\r\n\t ";
    // wxPrintf("c:<%d>\n", c);
    if (c < 0){
        // c is not a valid ascii.
        // mark as error,
        StartStyling(pos);
        SetStyling(1, STYLE_ERROR);
        return curStyle;
    }
    if (c == '*' && pos > 0 && GetCharAt(pos - 1) == '/'){
        StartStyling(pos - 1);
        SetStyling(2, STYLE_CSTYLE_COMMENTS);
        curStyle = STYLE_CSTYLE_COMMENTS;
    }
    else if (c == '/' && pos > 0 && GetCharAt(pos - 1) == '/'){
        StartStyling(pos - 1);
        SetStyling(2, STYLE_COMMENTS);
        curStyle = STYLE_COMMENTS;
    }
    else if (c == '#'){
        // this must be the first not white space.
        bool hasOtherChar = false;
        int start = pos-1;
        while(start > 0){
            char ch = GetCharAt(start);
            if (ch == '\r' || ch == '\n'){
                break;
            }
            else if (ch != ' ' || ch != '\t'){
                hasOtherChar = true;
                break;
            }
            start--;
        }
        if (!hasOtherChar){
            StartStyling(pos);
            SetStyling(1, STYLE_PREPROCESS);
            curStyle = STYLE_PREPROCESS;
        }
    }
    else if ((c == '\'' && pos > 0 && GetCharAt(pos-1) != '\\') ||
        (c == '\'' && pos == 0)){
        StartStyling(pos);
        SetStyling(1, STYLE_CHAR);
        curStyle = STYLE_CHAR;
    }
    else if ((c == '\"' && pos > 0 && GetCharAt(pos-1) != '\\') || (c == '\"' && pos == 0)){
        if (IsInPreproces(pos)){
            StartStyling(pos);
            SetStyling(1, STYLE_PREPROCESS_LOCAL);
            curStyle = STYLE_PREPROCESS_LOCAL;
        }
        else{
            StartStyling(pos);
            SetStyling(1, STYLE_STRING);
            curStyle = STYLE_STRING;
        }
    }
    else if (c == '<' && IsInPreproces(pos)){
        StartStyling(pos);
        SetStyling(1, STYLE_PREPROCESS_SYSTEM);
        curStyle = STYLE_PREPROCESS_SYSTEM;
    }
    else if (sOperator.find(c) != sOperator.npos){
        StartStyling(pos);
        SetStyling(1, STYLE_OPERATOR);
        // todo:fanhongxuan@gmail.com
        // word before :: mark as class
        HandleClass(pos, curStyle);
        curStyle = STYLE_NORMAL;
    }
    else if (sFolder.find(c) != sFolder.npos){
        StartStyling(pos);
        SetStyling(1, STYLE_FOLDER);
        curStyle = STYLE_NORMAL;
        HandleFunctionStart(pos, curStyle);
        HandleFunctionBody(pos, curStyle);
    }
    else if (sIdStart.find(c) != sIdStart.npos){
        StartStyling(pos);
        SetStyling(1, STYLE_IDENTY);
        curStyle = STYLE_IDENTY;
    }
    else if (c == '@' && mLanguage == "Java"){
        // note:fanhongxuan@gmail.com
        // in java, use @can start a id
        StartStyling(pos);
        SetStyling(1, STYLE_IDENTY);
        curStyle = STYLE_IDENTY;
    }
    else if (sWhitespace.find(c) != sWhitespace.npos){
        StartStyling(pos);
        SetStyling(1, STYLE_DEFAULT);
        curStyle = STYLE_NORMAL;
    }
    else if (sNumberStart.find(c) != sNumberStart.npos){
        StartStyling(pos);
        SetStyling(1, STYLE_NUMBER);
        curStyle = STYLE_NUMBER;
    }
    // todo:fanhongxuan@gmail.com
    // support #define to get macro define name.
    else {
        StartStyling(pos);
        SetStyling(1, STYLE_NORMAL);
        curStyle = STYLE_NORMAL;
        
        HandleVariable(pos, curStyle);
    }
    return curStyle;
}

// int ceEdit::HandlePreprocess(char c, int curStyle, long pos){
//     switch(curStyle){
//     case STYLE_PREPROCESS:
//         // if \ is the last char before \r\n, we can goto the next line.
//         // else we end at the \r or \n
//         // when meet include ", enter preprocess_local
//         // when meet include <, enter preprocess_system
//         // when meet define, undef, defined, ifndef, ifdef, the following is macro
//         // in preprocess can support one line comments.
//         if ((c == '\r' || c == '\n') && pos > 1 && GetCharAt(pos-1) != '\\'){
//             curStyle = STYLE_DEFAULT;
//             break;
//         }
//     case STYLE_PREPROCESS_LOCAL:
//     case STYLE_PREPROCESS_SYSTEM:
//         break;
//     }
//     return curStyle;
// }

static bool IsCPreprocessDefineKeyword(const wxString &input){
    wxString text = input;
    static wxString sCPreProcessDefineKeyword = "ifdef ifndef define defined if elif undef";
    static std::vector<wxString> outputs;
    if (outputs.empty()){
        ceSplitString(sCPreProcessDefineKeyword, outputs, ' ');
    }
    int start = text.find_first_not_of("\r\n\t ");
    int stop = text.find_last_not_of("\r\n\t ");
    if (start != text.npos && stop != text.npos && start <= stop){
        text = text.substr(start, stop - start+1);
    }
    for (int i = 0; i < outputs.size(); i++){
        if (outputs[i] == text){
            return true;
        }
        else if (text == "#" + outputs[i]){
            return true;
        }
        else if (text == "#"){
            return true;
        }
    }
    return false;
}

int ceEdit::ParseChar( int curStyle,long pos)
{   
    static wxString sNumber = ".0123456789abcdefABCDEFhxHX";
    static wxString sAlphaNumber = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    
    char c = GetCharAt(pos);
    // wxPrintf("pos:%ld, char:<%c>, style:%d\n", pos, c, curStyle);
    StartStyling(pos);
    SetStyling(1, curStyle);
    
    // todo:fanhongxuan@gmail.com
    // support \ to expand the preprocessor to multiple line
    // preprocess -> meet < switch to preprocess_system
    // preprocess -> meet " swtich to preprocess_local
    // other string following by preprocess is marked as preprocess,
    // and will be skipped when process
    switch (curStyle){
    case STYLE_PREPROCESS:
        if (c == '\r' || c == '\n'){
            curStyle = STYLE_DEFAULT;
            StartStyling(pos);
            SetStyling(1, curStyle);
        }
        else if (c == '\t' || c == ' '){
            int start = FindStyleStart(curStyle, pos);
            wxString text = GetTextRange(start, pos);
            if (IsCPreprocessDefineKeyword(text)){
                // wxPrintf("cppkey:<%s>\n", text);
            }
            else{
                curStyle = STYLE_DEFAULT;
            }
            StartStyling(pos);
            SetStyling(1, STYLE_DEFAULT);
            // if (c == '\r' || c== '\n' || c == '\t' || c == ' '){
            // curStyle = STYLE_DEFAULT;
        }
        else if ( c == '('){
            curStyle = STYLE_DEFAULT;
            StartStyling(pos);
            SetStyling(1, STYLE_DEFAULT);
        }
        break;
    case STYLE_PREPROCESS_SYSTEM:
        if (c == '>'){
            curStyle = STYLE_DEFAULT;
        }
        break;
    case STYLE_COMMENTS:
        if (c == '\r' || c == '\n'){
            curStyle = STYLE_DEFAULT;
        }
        break;
    case STYLE_COMMENT_KEY:
        if ((c == '>' || c == '}') && mLanguage == "Java"){
            curStyle = STYLE_CSTYLE_COMMENTS;
        }
        else if (c == '\r' || c == '\n'){
            // this is not a comments key, fix it.
            int start = FindStyleStart(STYLE_COMMENT_KEY, pos);
            if (start > 0){
                StartStyling(start);
                SetStyling(pos - start, STYLE_CSTYLE_COMMENTS);
            }
            curStyle = STYLE_CSTYLE_COMMENTS;
        }
        else if (sAlphaNumber.find(c) == sAlphaNumber.npos && 
            (mLanguage == "C++" || mLanguage == "C")){
            curStyle = STYLE_CSTYLE_COMMENTS;
        }
        break;
    case STYLE_CSTYLE_COMMENTS:
        // note:fanhongxuan@gmail.com
        // currently we only support the comments key in c-style comments,
        // not in cpp style comments.
        if ((c == '{' || c == '<') && mLanguage == "Java"){
            StartStyling(pos);
            SetStyling(1, STYLE_COMMENT_KEY);
            curStyle = STYLE_COMMENT_KEY;
        }
        if (c == '@' && (mLanguage =="C++" || mLanguage == "C")){
            StartStyling(pos);
            SetStyling(1, STYLE_COMMENT_KEY);
            curStyle = STYLE_COMMENT_KEY;
        }
        else if (c == '/' && pos > 0 && GetCharAt(pos - 1) == '*'){
            curStyle = STYLE_DEFAULT;
        }
        break;
    case STYLE_ESC_CHAR:
        StartStyling(pos);
        SetStyling(1, STYLE_ESC_CHAR);
        {
            int startPos = pos;
            while(startPos > 0 && curStyle == STYLE_ESC_CHAR){
                curStyle = GetStyleAt(startPos--);
            }
        }
        break;
    case STYLE_CHAR:
        if (c == '\\'){
            StartStyling(pos);
            SetStyling(1, STYLE_ESC_CHAR);
            curStyle = STYLE_ESC_CHAR;
        }
        else if (c == '\'' && pos > 0){
            if (GetCharAt(pos-1) == '\\' && GetStyleAt(pos-1) == STYLE_CHAR){
                // we meet \', not the real end.
            }
            else{
                curStyle = STYLE_DEFAULT;
            }
        }
        break;
    case STYLE_PREPROCESS_LOCAL:
        // note:fanhongxuan@gmail.com
        // preprocessor local don't support esc char.
    case STYLE_STRING:
        if (c == '\\'){
            StartStyling(pos);
            SetStyling(1, STYLE_ESC_CHAR);
            curStyle = STYLE_ESC_CHAR;
        }
        else if (c == '\"' && pos > 0){
            if(GetCharAt(pos-1) == '\\' && GetStyleAt(pos-1) == STYLE_STRING){
                // we meet \", not the real end.
            }
            else{
                curStyle = STYLE_DEFAULT;
            }
        }
        break;
    case STYLE_NUMBER:
        //
        if (sAlphaNumber.find(c) == sAlphaNumber.npos && pos > 1){
            // the number is end, check if this is a valid number?
            int start = FindStyleStart(STYLE_NUMBER, pos);
            if (start > 0){
                wxString text = GetTextRange(start, pos);
                if (!IsNumber(text)){
                    StartStyling(start);
                    SetStyling(pos - start, STYLE_ERROR);
                }
            }
            curStyle = ParseCharInDefault(c, curStyle, pos);
        }
        break;
    case STYLE_IDENTY:
    case STYLE_VARIABLE:
    case STYLE_VARIABLE_REF:    
        if (sAlphaNumber.find(c) != sAlphaNumber.npos){
            break;
        }
        else{
            // this wods is end.
            // check if this wod is a keyword or something else?
            if (pos > 0){
                ParseWord(pos-1);
            }
            curStyle = STYLE_DEFAULT;
        }
        curStyle = ParseCharInDefault(c, curStyle, pos);
        break;
    default:
        curStyle = ParseCharInDefault(c, curStyle, pos);
        break;
    }
    if (c == '\r' || c == '\n' || pos == GetLastPosition()){
        HandleFolder(pos);
    }
    return curStyle;
}

void ceEdit::OnStyleNeeded(wxStyledTextEvent &evt)
{
    int startPos = GetEndStyled();
    int stopPos = evt.GetPosition();
    int startLine = LineFromPosition(startPos);
    int stopLine = LineFromPosition(stopPos);
    startPos = XYToPosition(0, startLine);
    stopPos = GetLineEndPosition(stopLine);
    // set the mpParseState->currentStyle right
    int pos = startPos;
    int curStyle = STYLE_DEFAULT;
    PrepareFunctionParams(pos);
    StartStyling(startPos);
    SetStyling(stopPos - startPos, STYLE_NORMAL);
    while(pos > 0){
        char c = GetCharAt(pos);
        if (c != '\r' && c != '\t' && c != '\n' && c != ' '){
            int style = GetStyleAt(pos);
            if (c == '/' && pos > 0 && GetCharAt(pos-1) == '*'){
                // found a */
                if (STYLE_CSTYLE_COMMENTS == style || STYLE_COMMENTS == style){
                    // */ is csytle comments or cpp comments, we are normal
                    break;
                }
                else if (STYLE_CHAR == style || STYLE_STRING == style){
                    // we are in char,
                    curStyle = style;
                    break;
                }
                else{
                    wxPrintf("Bug: invalid style:%d, pos:%d\n", style, pos);
                }
            }
            else if (c == '\''){
                // found a '
                if (STYLE_CSTYLE_COMMENTS == style|| STYLE_STRING == style){
                    // ' is in cstyle comments,
                    curStyle = style;
                    break;
                }
                else if (STYLE_CHAR == style || STYLE_COMMENTS == style){
                    break;
                }
            }
            else if (c == '\"'){
                // found a "
                if (STYLE_CSTYLE_COMMENTS == style|| STYLE_CHAR == style){
                    // ' is in cstyle comments,
                    curStyle = style;
                    break;
                }
                else if (STYLE_STRING == style || STYLE_COMMENTS == style){
                    break;
                }
            }
            else{
                // other case    
                if (style == STYLE_CSTYLE_COMMENTS || style == STYLE_STRING || style == STYLE_CHAR){
                    curStyle = style;
                }
            }
        }
        pos--;
    }
    
    ClearLoalSymbol();
    for (int i = startPos; i <= stopPos; i++ ){
        curStyle = ParseChar(curStyle, i);
    }
}

void ceEdit::UpdateLineNumberMargin()
{
    int lineCount = GetLineCount();
    wxString linenumber = wxT("_");
    while(lineCount != 0){
        linenumber += wxT("9");
        lineCount = lineCount / 10;
    }
    // fixme: for debug the auto indent
    // show the fold level before the number.
#ifdef DEBUG_FOLD    
    linenumber = wxT("99999999999");
#endif    
    SetMarginWidth(mLinenuMargin, TextWidth (wxSTC_STYLE_LINENUMBER, linenumber));
}

bool ceEdit::EndCallTip(){
    if (CallTipActive()){
        CallTipCancel();
    }
    mActiveFunctionParam = "";
    mActiveFunctionParamStart = 0;
    
    mActiveCallTipsIndex = 0;
    mActiveCallTipsPosition = 0;
    for (int i = 0; i < mActiveCallTips.size(); i++){
        delete mActiveCallTips[i];
    }
    mActiveCallTips.clear();
    return true;
}

bool ceEdit::UpdateCallTip(int direction){
    if (!CallTipActive()){
        return false;
    }
    if (direction == 0){
        if (!mActiveFunctionParam.empty()){
            return false;
        }
        if (wxGetApp().frame() != NULL && mActiveCallTipsIndex < mActiveCallTips.size() && 
            mActiveCallTips[mActiveCallTipsIndex] != NULL){
            wxString file = mActiveCallTips[mActiveCallTipsIndex]->file;
            int line = mActiveCallTips[mActiveCallTipsIndex]->line - 1;
            EndCallTip();
            wxGetApp().frame()->OpenFile(file, file, true, line);
            // wxGetApp().frame()->OpenFile(mActiveCallTips[mActiveCallTipsIndex]->file,
            //     mActiveCallTips[mActiveCallTipsIndex]->file, true, mActiveCallTips[mActiveCallTipsIndex]->line-1);
            return true;
        }
        if (wxGetApp().frame() != NULL && mLocalCallTipLine > 0){
            wxGetApp().frame()->OpenFile(GetFilename(), GetFilename(), true, mLocalCallTipLine);
            mLocalCallTipLine = -1;
        }
        return false;
    }
    
    if (mActiveCallTips.size() <= 1){
        return false;
    }
    
    if (direction == 1){
        mActiveCallTipsIndex--;
    }
    else if (direction == 2){
        mActiveCallTipsIndex++;
    }
    else{
        return false;
    }
    // note:fanhongxuan@gmail.com
    // when compare a unsigned value with a signed value,
    // default will both convert to unsigned value to compare.
    if (mActiveCallTipsIndex >= (int)mActiveCallTips.size()){
        mActiveCallTipsIndex = 0;
    }
    if (mActiveCallTipsIndex < 0){
        mActiveCallTipsIndex = mActiveCallTips.size()-1;
    }
    wxString value;
    if (mActiveCallTips.size() > 1){
        value += "\001\002";
    }
    // wxPrintf("mActiveCallTips.size():%ld\n", mActiveCallTips.size());
    ceSymbol *pSymbol = mActiveCallTips[mActiveCallTipsIndex];
    // wxPrintf("UpdateCallTips:<%s>\n", pSymbol->ToAutoCompString());
    if (!pSymbol->shortFilename.empty()){
        value += pSymbol->shortFilename;
    }
    else{
        value += pSymbol->file;
    }
    value += "(" + pSymbol->lineNumber + "):" + pSymbol->desc;
    if (value.empty()){
        return false;
    }
    if (CallTipActive()){
        CallTipCancel();
    }
    // wxPrintf("UpdateCallTip:<%s>\n", value);
    CallTipShow(mActiveCallTipsPosition, value);
    return true;
}

void ceEdit::OnCallTipClick(wxStyledTextEvent &evt){
    // wxPrintf("OnCallTipClick<%d>\n", evt.GetPosition());
    UpdateCallTip(evt.GetPosition());
}

void ceEdit::OnDwellStart(wxStyledTextEvent &evt){
    if ((!AutoCompActive()) && !CallTipActive() && mbHasFocus){
        ShowCallTips(evt.GetPosition());
    }
    evt.Skip();
}

void ceEdit::OnDwellEnd(wxStyledTextEvent &evt){
    evt.Skip();
}

void ceEdit::OnAutoCompSelection(wxStyledTextEvent &evt)
{
    wxString value = evt.GetString();
    int pos = value.find('(');
    if (pos != value.npos){
        int offset = -1;
        if (value.find("()") != value.npos){
            offset = 0;
        }
        value = value.substr(0, pos);
        value += "()";
        // InsertText(evt.GetPosition(), value);
        Replace(evt.GetPosition(), GetCurrentPos(), value);
        GotoPos(evt.GetPosition() + value.length() + offset);
        AutoCompCancel();
        if (offset != 0){
            ShowFunctionParam(evt.GetPosition(), evt.GetString());
        }
    }
    pos = value.find('[');
    if (pos != value.npos){
        value = value.substr(0, pos);
        Replace(evt.GetPosition(), GetCurrentPos(), value);
        GotoPos(evt.GetPosition() + value.length());
        AutoCompCancel();
    }
}

void ceEdit::OnModified(wxStyledTextEvent &evt)
{
    int type = evt.GetModificationType();
    // wxPrintf("OnModified:0x%08X\n", type);
    if (type & (wxSTC_MOD_INSERTTEXT | wxSTC_MOD_DELETETEXT)){        
        UpdateLineNumberMargin();
    }
    if (type & wxSTC_MOD_CHANGESTYLE){
        // current not used.
        // wxPrintf("Style:%d:%d\n", evt.GetPosition(), GetStyleAt(evt.GetPosition()));
    }
    if (type & (wxSTC_MOD_CHANGEMARKER | wxSTC_MOD_CHANGEFOLD /* | wxSTC_MOD_CHANGELINESTATE */)){
        // fold status changed.
        if (!mbLoadFinish){
            // document is still loading, skip this.
            return;
        }
        
        if (!GetModify()){
            return;
        }
        
        if (mbReplace){
            return;
        }
        // indent the line by foldlevel
        int line = evt.GetLine();
        int curLine = GetCurrentLine();
        if (line > (curLine+1)){
            //wxPrintf("skip otherline:%d\n", curLine+1);
            return;
        }
        if (line >= GetLineCount() || line < 0){
            // wxPrintf(" Skip the invalid line\n");
            return;
        }
        
        // note:fanhongxuan@gmail.com
        // DO NOT update the indent here
        // send a message to the edit, and update the indent in the may thread.
        // Othersize, the style of this line maybe wrong.
        wxCommandEvent *pEvt = new wxCommandEvent;
        pEvt->SetEventType(wxEVT_MENU);
        pEvt->SetId(ceEdit_Update_Indent_By_Fold_Level);
        pEvt->SetInt(evt.GetLine());
        wxQueueEvent(this, pEvt);
    }
}

void ceEdit::OnTimer(wxTimerEvent &evt){
    wxStyledTextEvent event;
    event.SetPosition(GetCurrentPos());
    OnDwellStart(event);
}

void ceEdit::OnIdleTimer(wxTimerEvent &evt){
    // wxPrintf("OnIdleTimer\n");
    // todo:fanhongxuan@gmail.com
    // 1, backup the modified file automatic.
    // 2, check if the file is modified outside the edit.
    // 1, if the last modified is different with the mLastModifiedTime
    //    if currenlty no un-saved modification
    //    pop up a dialog to notify user if we need to reload?
    //    if currenlty has un-saved modification,
    time_t lmt = GetLastModifyTime(GetFilename());
    if ((!mFilename.empty()) && lmt == 0){
        SaveFile();
    }
    if (lmt > mLastModifyTime){
        // wxPrintf("File %s is modified outside\n", GetFilename());
        int ret = wxMessageBox(wxString::Format(wxT("File(%s) is Modified outside, Do you want to reload the file?"), GetFilename()),
                               wxT("Confirm"), wxYES_NO|wxCANCEL, this);
        if (ret == wxYES){
            int curPos = GetCurrentPos();
            int curLine = GetFirstVisibleLine();
            LoadFile(GetFilename());
            // SetCurrentPos(curPos);
            SetFirstVisibleLine(curLine);
            GotoPos(curPos);
        }
    }
    wxAutoCompMemberProvider::Instance().SetClassName("", mLanguage, GetFilename());
    wxAutoCompMemberProvider::Instance().SetClassName("__anon", mLanguage, GetFilename());
    RestartIdleTimer();
}

void ceEdit::OnSize( wxSizeEvent& event ) {
    int x = GetClientSize().x + GetMarginWidth(mLinenuMargin) + GetMarginWidth(mFoldingMargin);
    if (x > 0) SetScrollWidth (x);
    event.Skip();
}

void ceEdit::OnUpdateUI(wxStyledTextEvent &evt){
    RestartIdleTimer();
    wxString func = WhichFunction(GetCurrentPos());
    if (NULL != wxGetApp().frame()){
        long x, y;
        PositionToXY(GetCurrentPos(), &x, &y);
        wxString value = wxString::Format("Line:%ld(%ld)", y+1,x+1);
        wxGetApp().frame()->ShowStatus(value, 0);
        if (!func.empty()){
            if (func.find("::") == func.npos){
                wxString cls = WhichClass(GetCurrentPos());
                if (!cls.empty()){
                    func = cls + "::" + func;
                }
            }
            wxGetApp().frame()->ShowStatus(wxString::Format(wxT("CurrentFunction:[%s]"), func), 5);
        }
        else{
            wxString cls = WhichClass(GetCurrentPos());
            if (!cls.empty()){
                wxGetApp().frame()->ShowStatus(wxString::Format(wxT("DefineClass:[%s]"), cls), 5);
            }
            else{
                wxGetApp().frame()->ShowStatus("", 5);
            }
        }
        
        if (NULL == mpTimer){
            mpTimer = new wxTimer(this, ceEdit_CallTip_Timer);
        }
        else{
            mpTimer->Stop();
        }
        mpTimer->StartOnce(800);
    }
}

void ceEdit::OnMarginClick(wxStyledTextEvent &evt)
{
    if (evt.GetMargin() == mFoldingMargin) {
        int lineClick = LineFromPosition (evt.GetPosition());
        int levelClick = GetFoldLevel (lineClick);
        if ((levelClick & wxSTC_FOLDLEVELHEADERFLAG) > 0) {
            // ToggleFold (lineClick);
            ToggleFoldShowText(lineClick, "...");
        }
    }
}

void ceEdit::OnFocus(wxFocusEvent &evt)
{
    mbHasFocus = true;
    if (NULL != wxGetApp().frame()){
        wxGetApp().frame()->DoUpdate();
        wxGetApp().frame()->SetActiveEdit(this);
    }
    evt.Skip();
}

void ceEdit::OnKillFocus(wxFocusEvent &evt)
{
    mbHasFocus = false;
    if (AutoCompActive()){
        AutoCompCancel();
    }
    
    // note:fanhongxuan@gmail.com
    // when need to take screen shut, need to comments this line.
    EndCallTip();
    evt.Skip();
}

void ceEdit::OnKeyDown (wxKeyEvent &event)
{
    RestartIdleTimer();
    if (GetLexer() != wxSTC_LEX_CONTAINER){
        event.Skip();
        return;
    }
    
    if (WXK_RETURN == event.GetKeyCode()){
        if (AutoCompActive()){
            // fixme:fanhongxuan@gmail.com
            // this will end the multiple typing.
            AutoCompComplete();
            return;
        }
    }
    
    // handle the call tips.
    if (WXK_UP == event.GetKeyCode() && event.ControlDown()){
        if (UpdateCallTip(1)){
            return;
        }
    }
    else if (WXK_DOWN == event.GetKeyCode() && event.ControlDown()){
        if (UpdateCallTip(2)){
            return;
        }
    }
    else if (WXK_RIGHT == event.GetKeyCode() && event.ControlDown()){
        if (UpdateCallTip(0)){
            return;
        }
    }
    
    if (mActiveFunctionParam.empty()){
        if ((!event.ControlDown()) && (!event.AltDown())){
            if (CallTipActive()){
                CallTipCancel();
            }
        }
    }
    
    if (event.GetKeyCode() == 'G' && event.ControlDown()){
        wxCommandEvent evt;
        OnGotoLine(evt);
    }
    
    if ('C' == event.GetKeyCode() && event.ControlDown()){
        long start, stop;
        GetSelection(&start, &stop);
        ClearSelections();
        SetSelection(start, stop);
    }
    // todo:fanhongxuan@gmail.com
    // store all the goto target in a stack, use ctrl-right/left to go throw the stack
    // in this case, can easyly return to the previously viewed 
    
    if (WXK_PAGEUP == event.GetKeyCode() && event.AltDown()){
        GotoPos(0);
    }
    else if (WXK_PAGEDOWN == event.GetKeyCode() && event.AltDown()){
        GotoPos(GetLastPosition());
    }
    
    if (';' == event.GetKeyCode() && event.AltDown()){
        long start, end;
        GetSelection(&start, &end);
        TriggerCommentRange(start, end);
        return;
    }
    
    if ('R' == event.GetKeyCode() && event.ControlDown()){
        SetMultiPaste(wxSTC_MULTIPASTE_EACH);
        SetAdditionalSelectionTyping(true);
        StartReplaceInRegion();
        return;
    }
    
    if (WXK_ESCAPE == event.GetKeyCode()){
        if (AutoCompActive()){
            AutoCompCancel();
            return;
        }
        if (mbReplace){
            mbReplace = false;
            SetMultiPaste(wxSTC_MULTIPASTE_EACH);
            SetAdditionalSelectionTyping(true);
            SetSelForeground(true, *wxBLACK);
            SetSelBackground(true, *wxWHITE);
            if (NULL != wxGetApp().frame()){
                wxGetApp().frame()->ShowStatus("", 1);
            }
        }
    }
    
    if (WXK_BACK ==  event.GetKeyCode()){
        // 
        long start = 0, stop = 0;
        GetSelection(&start, &stop);
        if (start == stop){
            HungerBack();
        }
    }
    
    if (WXK_DELETE == event.GetKeyCode() || WXK_BACK == event.GetKeyCode()){
        // note:fanhongxuan@gmail.com
        // when backspace, will not call OnCharAdded
        // we need to prepare the function params here.
        PrepareFunctionParams(GetCurrentPos());    
    }
    event.Skip();
}


static inline bool IsBraceChar(char cur){
    if (cur == '[' || cur == ']' || cur == '(' || cur == ')' || cur == '{' || cur == '}' || cur == '\'' || cur == '\"'){
        return true;
    }
    return false;
}

void ceEdit::DoBraceMatch()
{
    bool bBrace = false;
    int min = GetCurrentPos();
    char cur = GetCharAt(min);
    bBrace = IsBraceChar(cur);
    if (!bBrace /*&& IsWhiteSpace(cur) || cur == ';'*/){
        min--;
        cur = GetCharAt(min);
        bBrace = IsBraceChar(cur);
    }
    
    if (bBrace){
        int max = BraceMatch (min);
        if (max >= 0) {
            BraceHighlight (min, max);
        }
        else{
            BraceBadLight (min);
        } 
    }
    else{
        BraceHighlight(-1, -1);
    }
}

void ceEdit::RestartIdleTimer()
{
    if (NULL != mpIdleTimer){
        mpIdleTimer->Stop();
        mpIdleTimer->StartOnce(3000);
        // mpIdleTimer->StartOnce(5000);
    }
}

void ceEdit::OnMouseLeftDown(wxMouseEvent &evt)
{
    RestartIdleTimer();
    int pos = GetCurrentPos();
    if (mbReplace){
        mbReplace = false;
        SetSelection(pos, pos);
        SetMultiPaste(wxSTC_MULTIPASTE_EACH);
        SetAdditionalSelectionTyping(true);
        SetSelBackground(true, *wxWHITE);
        SetSelForeground(true, *wxBLACK);
        if (NULL != wxGetApp().frame()){
            wxGetApp().frame()->ShowStatus("", 1);
        }
    }
    evt.Skip();
}

void ceEdit::OnMouseLeftUp(wxMouseEvent &evt)
{
    DoBraceMatch();
    evt.Skip();
}

void ceEdit::OnMouseLeftDclick(wxMouseEvent &evt){
    int pos = GetCurrentPos();
    char c = GetCharAt(pos);
    bool bSelect = false;
    if (GetStyleAt(pos) == STYLE_FOLDER ){
        bSelect = true;
    }
    if (pos >=1 && GetStyleAt(pos-1) == STYLE_FOLDER){
        pos--;
        bSelect = true;
    }
    if (bSelect){
        int stop = BraceMatch(pos);
        if (stop > 0){
            if (pos <= stop){
                SetSelection(pos, stop);
            }
            else{
                SetSelection(stop, pos);
            }
        }
        return;
    }
    
    // first select the key;
    SetAdditionalSelectionTyping(false);
    SetMultiPaste(wxSTC_MULTIPASTE_ONCE);
    StartReplaceInRegion();
    //evt.Skip();
}

void ceEdit::OnMouseWheel(wxMouseEvent &evt)
{
    EndCallTip();
    if (AutoCompActive()){
        AutoCompCancel();
    }
    wxStyledTextCtrl::OnMouseWheel(evt);
    Refresh();
    // evt.Skip();
}

void ceEdit::OnKeyUp(wxKeyEvent &event)
{
    DoBraceMatch();
    if (WXK_TAB == event.GetKeyCode()){
        PrepareFunctionParams(GetCurrentPos());
        if (AutoCompActive()){
            AutoCompComplete();
        }
        else{
            long start = 0, stop = 0, startLine = 0, stopLine = 0;
            GetSelection(&start, &stop);
            PositionToXY(start, NULL, &startLine);
            PositionToXY(stop, NULL, &stopLine);
            // wxPrintf("startLine:%ld, stopLine:%ld\n", startLine, stopLine);
            if (startLine != stopLine){
                if (startLine > stopLine){
                    long temp = stopLine; stopLine = startLine; startLine = temp;
                }
                for (int i = startLine; i <= stopLine; i++){
                    AutoIndentWithTab(i);
                }
                int pos = GetLineIndentPosition(stopLine);
                SetSelection(pos, pos);
                GotoPos(pos);
                ChooseCaretX();
            }
            else{
                AutoIndentWithTab(GetCurrentLine());
            }
        }
    }
    
    if (OnUpdateFunctionParam(event)){
    }
    event.Skip();
}

bool ceEdit::LoadAutoComProvider(const wxString &mLanguage)
{
    mAllProviders.clear();
    return wxAutoCompProvider::GetValidProvider(mLanguage, mAllProviders);
}

bool ceEdit::IsValidChar(char ch, const wxString &validCharList)
{
    if (!validCharList.empty()){
        if (validCharList.find(ch) != validCharList.npos){
            return true;
        }
        return false;
    }
    int i = 0;
    for (i = 0; i < mAllProviders.size(); i++){
        if (mAllProviders[i]->IsValidChar(ch)){
            return true;
        }
    }
    return false;
}

int ceEdit::GetCurrentMode(){
    int pos = GetCurrentPos();
    while(pos > 0){
        char c = GetCharAt(pos);
        int style = GetStyleAt(pos);
        // wxPrintf("GetCurrentMode<%d>(%c,%d)\n", pos, c, style);
        if (!IsWhiteSpace(c) && style != 0){
            if (style == STYLE_STRING || style == STYLE_CHAR || style == STYLE_COMMENTS || 
                style == STYLE_COMMENT_KEY || style == STYLE_ESC_CHAR){
                // wxPrintf("GetCurrentMode<%c><%d>\n", c, style);
                // wxPrintf("Mode is 0\n");
                return 0;
            }
            else if (style == STYLE_CSTYLE_COMMENTS && c == '/' && pos > 1 && GetCharAt(pos-1) == '*' && GetStyleAt(pos-1) == style){
                break;
            }
            else if (style == STYLE_CSTYLE_COMMENTS){
                return 0;
            }
            else if ((c == '.' && style == STYLE_OPERATOR) ||
                (c == '>' && style == STYLE_OPERATOR && pos > 1 && GetCharAt(pos-1) == '-' && GetStyleAt(pos-1) == style) ||
                (c == ':' && style == STYLE_OPERATOR && pos > 1 && GetCharAt(pos-1) == ':' && GetStyleAt(pos-1) == style)){
                return 2;
            }
            else{
                // wxPrintf("GetCurrentMode<%c><%d>\n", c, style);
                break;
            }
        }
        if (style == 0){
            if (c == '.' && pos > 1 && (!IsCommentOrWhiteSpace(GetCharAt(pos-1), GetStyleAt(pos-1)))){
                return 2;
            }
            if (c == '>' && pos > 1 && GetCharAt(pos-1) == '-' && GetStyleAt(pos-1) == STYLE_OPERATOR){
                return 2;
            }
            if (c == ':' && pos > 1 && GetCharAt(pos-1) == ':' && GetStyleAt(pos-1) == STYLE_OPERATOR){
                return 2;
            }
        }
        pos--;
    }
    // wxPrintf("mode is 1\n"); 
    /** get */
    return 1;
}

static wxString GetTypeFromString(const wxString &input, const wxString &language){
    int pos = 0;
    wxString ret = input;
    // if retType is like std::vector<int>, return std::vector instead.
    pos = ret.find("<");
    if (pos != ret.npos){
        ret = ret.substr(0, pos);
    }
    // const wxString &, 
    // const wxString *
    std::vector<wxString> outputs;
    ceSplitString(ret, outputs, ' ');
    for (int i = 0; i < outputs.size(); i++){
        if (!IsKeyWord1(outputs[i], language)){
            ret = outputs[i];
            break;
        }
    }
    
    // remove *, & from the ret.
    pos = ret.find("*");
    if (pos != ret.npos){
        ret = ret.substr(0, pos);
    }
    pos = ret.find("&");
    if (pos != ret.npos){
        ret = ret.substr(0, pos);
    }
    // wxPrintf("currentType:%s\n", ret);
    return ret;
}

static wxString GetType(int curPos, ceEdit &edit, const wxString &curClass, 
                        const wxString &language, const wxString &filename)
{
    int pos = curPos;
    // a.b.c.d;
    // a->b->c->d;
    // a
    // wxPrintf
    wxString ret;
    wxString base;
    bool bFunction = false;
    bool bVariable = false;
    bool bParam = false;
    while( pos > 0){
        char c = edit.GetCharAt(pos);
        int style = edit.GetStyleAt(pos);
        // wxPrintf("GetType:c<%c>,style<%d>\n", c, style);
        if (c == ';' && (style == STYLE_DEFAULT || style == STYLE_NORMAL)){
            break;
        }
        else if (( c == '{' || c == '('||c == '}') && (style == STYLE_DEFAULT || style == STYLE_FOLDER)){
            break;
        }
        else if (c == ')' && (style == STYLE_DEFAULT || style == STYLE_FOLDER)){
            bFunction = true;
            pos = edit.BraceMatch(pos);
            // break;
        }
        else if (c == '.' && (style == STYLE_DEFAULT || style == STYLE_OPERATOR)){
            base = GetType(pos-1, edit, curClass, language, filename);
            break;
        }
        else if (c == '>' && (style == STYLE_DEFAULT || style == STYLE_OPERATOR) && pos > 1 &&
            edit.GetCharAt(pos-1) == '-' && (edit.GetStyleAt(pos-1) == STYLE_DEFAULT || edit.GetStyleAt(pos-1) == STYLE_OPERATOR)){
            base = GetType(pos-2, edit, curClass, language, filename);
            break;
        }
        else if (c == ':' && (style == STYLE_DEFAULT || style == STYLE_OPERATOR) && pos > 1 &&
            edit.GetCharAt(pos-1) == ':' && ((edit.GetStyleAt(pos-1) == STYLE_DEFAULT || edit.GetStyleAt(pos-1) == STYLE_OPERATOR))){
            // ::
            base = GetType(pos-2, edit, curClass, language, filename);
            break;
        }
        else if (style == STYLE_TYPE || style == STYLE_IDENTY || style == STYLE_FUNCTION){
            int start = edit.FindStyleStart(style, pos);
            if (!ret.empty()){
                //return ret;
                break;
            }
            ret = edit.GetTextRange(start, pos+1);
            pos = start;
        }
        else if (style == STYLE_VARIABLE_REF || style == STYLE_VARIABLE){
            bVariable = true;
            int start = edit.FindStyleStart(style, pos);
            // if (!ret.empty()){
            //     break;
            // }
            ret = edit.GetTextRange(start, pos+1);
            pos = start;
            break;
        }
        else if ((c == '[' || c == ']') && style == STYLE_NORMAL){
            break;
        }
        else if (style == STYLE_OPERATOR){
            // other operator will break a call.
            break;
        }
        else if (style == STYLE_PARAMETER_REF){
            bParam = true;
            int start = edit.FindStyleStart(style, pos);
            // if (!ret.empty()){
            //     break;
            // }
            ret = edit.GetTextRange(start, pos+1);
            pos = start;
            break;
            // pos = start;
        }
        pos--;
    }
    
    wxString retType;
    if (bVariable){
        retType = edit.QueryVariableType(ret);
        // wxPrintf("GetType variable: retType:<%s>,ret<%s>\n", retType, ret);
    }
    else if (bParam){
        retType = edit.QueryFunctionParamType(ret);
        // wxPrintf("GetType param: retType:<%s>,ret<%s>\n", retType, ret);
    }
    else{
        // query from the database.
        wxString type = "cmf"; // class
        if (ret.empty()){
            return base;
        }
        // wxPrintf("Ret:<%s>, bFunction:%d, bParam:%d, bVariable:%d\n", ret, (int)bFunction, (int)bParam, (int)bVariable);
        wxString className;
        std::set<ceSymbol*> outputs;
        std::vector<wxString> classNames;
        if (base.empty()){
            classNames.push_back("");
            if (!curClass.empty()){
                classNames.push_back(curClass);
            }
        }
        else{
            classNames.push_back(base);
        }
        for (int i = 0; i < classNames.size(); i++){
            // wxPrintf("FindDef value:<%s>, scope:<%s>, type:<%s>\n", ret, classNames[i], type);
            wxGetApp().frame()->FindDef(outputs, ret, classNames[i], type, language, filename);
        }
        std::set<ceSymbol*>::iterator it = outputs.begin();
        while(it != outputs.end()){
            // wxPrintf("symbol:<%s>\n", (*it)->ToAutoCompString());
            ceSymbol *pSymbol = (*it);
            if (pSymbol->symbolType == "Class"){
                if (!pSymbol->name.empty()){
                    // if (!retType.empty()){
                    //     retType += "\n";
                    // }
                    retType = pSymbol->name;
                }
            }
            else if (pSymbol->symbolType == "Function" || pSymbol->symbolType == "Member"){
                if (!pSymbol->type.empty()){
                    // if (!retType.empty()){
                    //     retType += "\n";
                    // }
                    retType = pSymbol->type;
                }
            }
            delete (*it);
            it++;
        }
        if (retType.empty()){
            retType = base;
        }
    }
    return GetTypeFromString(retType, language);
}

wxString ceEdit::QueryVariableType(const wxString &name){
    std::map<wxString, std::pair<int, wxString> >::iterator it = mLocalVariable.find(name);
    if (it != mLocalVariable.end()){
        return it->second.second;
    }
    return "";
}

wxString ceEdit::QueryFunctionParamType(const wxString &name){
    std::map<wxString, std::pair<int, wxString> >::iterator it = mFunctionParames.find(name);
    if (it != mFunctionParames.end()){
        return it->second.second;
    }
    return "";
}

int ceEdit::SetClass(int curPos){
    // how to set the scope:
    // 1, if we define a class, use the class and his parent,
    // 2, if we are in a class function, use the class, his parent, and global.
    // 3, if we are after ::, use the class, his parent
    // 4, if we are after . find the type before
    // 5, if we are after -> find the type before
    // search backword, meet ; }, will stop search.
    
    // note:fanhongxuan@gmail.com
    // add a idle timer, to do some work when system is idle.
    // note:fanhongxuan@gmail.com
    // always load the global value here for later use.
    wxAutoCompMemberProvider::Instance().SetClassName("", mLanguage, GetFilename());
    wxAutoCompMemberProvider::Instance().SetClassName("__anon", mLanguage, GetFilename());
    wxString name;
    int mode = 0;
    name = WhichFunction(curPos);
    int pos = name.find("::");
    if (pos != name.npos){
        name = name.substr(0, pos);
    }
    else{
        name = WhichClass(GetCurrentPos());
    }
    wxString type = GetType(curPos-1, *this, name, mLanguage, GetFilename());
        if (!type.empty()){
        name = type;
        mode = 2;
        wxPrintf("SetClass(%s)\n", name);
    }
    wxAutoCompMemberProvider::Instance().SetClassName(name, mLanguage, GetFilename());
    
    if (mode != 2){
        return GetCurrentMode();
    }
    return mode;
}

bool ceEdit::GetCandidate(const wxString &input, std::set<wxString> &candidate, int mode)
{
    wxPrintf("GetCandidate:%s<%d>\n", input, mode);
    // add local variable
    if (mode != 2){
        std::map<wxString, std::pair<int, wxString> >::iterator it = mLocalVariable.begin();
        while(it != mLocalVariable.end()){
            if (it->first.find(input) == 0){
                wxString value = it->first + "[<" + it->second.second+  ">Local variable]";
                candidate.insert(value);
            }
            it++;
        }
        
        // add function param
        it = mFunctionParames.begin();
        while (it != mFunctionParames.end()){
            if (it->first.find(input) == 0){
                wxString value = it->first + "[<" + it->second.second + ">Function Param]";
                candidate.insert(value);
            }
            it++;
        }
        
        // add local type
        std::set<wxString>::iterator sit = mLocalTypes.begin();
        while(sit != mLocalTypes.end()){
            if ((*sit).find(input) == 0){
                wxString value = (*sit) + "[Local Type]";
                candidate.insert(value);
            }
            sit++;
        }
    }
    
    int i = 0;
    for (i = 0; i < mAllProviders.size(); i++){
        mAllProviders[i]->GetCandidate(input, candidate, mLanguage, mode);
    }
    
    // todo:fanhongxuan@gmail.com
    // move the duplicate value.
    // only compare the value before [].
    return true;
}

void ceEdit::MoveCharBeforeRightParentheses(int currentLine){
    int pos = GetCurrentPos();
    if (pos <= 0){
        return;
    }
    if (GetLexer() != wxSTC_LEX_CONTAINER){
        return;
    }
    // wxString line = GetLineText(currentLine);
    
    int offset = 0;
    int stop = GetLineEndPosition(currentLine);
    while((pos + offset) < stop){
        char c = GetCharAt(pos + offset);
        if ( c != ')' && c != ']'){
            break;
        }
        offset++;
    }
    
    if (offset != 0){
        if (GetCharAt(pos-1) == ';' && GetCharAt(pos+offset-1) == ')' 
            && GetStyleAt(pos + offset-1) == STYLE_FOLDER){
            // if we are after after a for, don't move the ;
            stop = XYToPosition(0, currentLine);
            int start = BraceMatch(pos+offset - 1);
            while(start > stop){
                if (GetStyleAt(start) == STYLE_KEYWORD1){
                    int prev = FindStyleStart(STYLE_KEYWORD1, start);
                    if (prev > 0){
                        wxString value = GetTextRange(prev, start+1);
                        if (value == "for"){
                            return;
                        }
                    }
                }
                start--;
            }
        }
        char c = GetCharAt(pos - 1);
        InsertText(pos + offset, c);
        DeleteRange(pos-1, 1);
        GotoPos(pos + offset);
        ChooseCaretX();
    }
}

void ceEdit::OnColon(int currentLine)
{
    // when user input a : in cpp mode
    // if the line only has the following cpp keyword, need to decrease the indent.
    // public, private, protected:
    int lineInd = GetLineIndentation(currentLine);
    wxString line = GetLineText(currentLine);
    int pos = line.find_first_not_of("\r\n\t ");
    if (pos == line.npos){
        return;
    }
    line = line.substr(pos);
    pos = line.find_last_not_of("\r\n\t :");
    if (pos == line.npos){
        return;
    }
    line = line.substr(0, pos+1);
    if (line == "public" || line == "private" || line == "protected" || line == "default" || line.find("case ") == 0){
        int parent = GetFoldParent(currentLine);
        if (parent >= 0){
            SetLineIndentation(currentLine, GetLineIndentation(parent));
        }
        else{
            lineInd -= GetIndent();
            SetLineIndentation(currentLine, lineInd);
        }
    }
}

void ceEdit::InsertPair(int currentLine, char c)
{
    int pos = GetCurrentPos();
    int style = GetStyleAt(pos);
    if (GetLexer() != wxSTC_LEX_CONTAINER){
        return;
    }
    if (style == STYLE_COMMENTS || style == STYLE_CSTYLE_COMMENTS && c == '\''){
        // skip ' in comments
        return;
    }
    char end = '0';
    if (c == '{'){ end = '}';}
    if (c == '\''){end = '\'';}
    if (c == '\"'){end = '\"';}
    if (c == '('){end = ')';}
    if (c == '['){end = ']';}
    if (c == '<'){end = '>';}
    // note:fanhongxuan@gmail.com
    // in which case we don't need to insert a } here?
    int parent = GetFoldParent(currentLine);
    if (parent >= 0){
        InsertText(pos, end);
    }
    else{
        // todo:fanhongxuan@gmail.com
        // check if the {} is pair.
        // if the length is less than 10000, check if all the {} is pair
        InsertText(pos, end);
    }
}

void ceEdit::OnEndBrace(int currentLine)
{
    // update the indent according the fold level;
    wxString line = GetLineText(currentLine);
    if (line.find_first_not_of("\r\n\t }") != line.npos){
        return;
    }
    // this line only has white space or }, 
    int parent = GetFoldParent(currentLine);
    int ind = GetLineIndentation(parent);
    SetLineIndentation(currentLine, ind);
}

bool ceEdit::HungerBack(){
    int start = GetCurrentPos();
    int end = start;
    // if we are in middle of (), {}, "", ''; we will delete the last char.
    if (start > 1){
        char prevCh = GetCharAt(start - 1);
        char nextCh = GetCharAt(start);
        if ((prevCh == '(' && nextCh == ')') ||
            (prevCh == '{' && nextCh == '}') ||
            (prevCh == '(' && nextCh == ')') || 
            (prevCh == '[' && nextCh == ']') ||
            (prevCh == '\'' && nextCh =='\'') ||
            (prevCh == '\"' && nextCh =='\"')){
            DeleteRange(start, 1);
            return true;
        }
    }
    while(start > 0){
        if(!IsWhiteSpace(GetCharAt(start-1))){
            break;
        }
        start--;
    }
    
    DeleteRange(start+1, end - start - 1);
    return true;
}

static wxString GetTypeByStyle(int style){
    if (style == STYLE_IDENTY){
        return "fpdgem";
    }
    if (style == STYLE_FUNCTION){
        return "fp";
    }
    return "";
}

bool ceEdit::HighLightNextParam(const wxString &desc)
{
    // auto highlith from ( to the first ',' 
    // update the mAcitveFunctionParamIndex by the position.
    int paramIndex = 0;
    int pos = GetCurrentPos();
    int stopPos = BraceMatch(mActiveFunctionParamStart);
    if (pos < mActiveFunctionParamStart || pos > stopPos){
        // wxPrintf("HighLightNextParam, cancel\n");
        EndCallTip();
        return false;
    }
    while(pos-1 >  mActiveFunctionParamStart){
        // wxPrintf("HighLightNextParam:<%c>\n", (char)GetCharAt(pos-1));
        if (GetCharAt(pos-1) == ',' && GetStyleAt(pos-1) == STYLE_OPERATOR){
            paramIndex++;
        }
        pos--;
    }
    // wxPrintf("paramIndex:%d\n", paramIndex);
    int start = desc.find("(");
    int stop = desc.find(")");
    if (start == desc.npos || stop == desc.npos || (start+1 == stop)){
        return true;
    }
    int highlightStart = start+1;
    int highlightStop = stop;
    int iCommaCount = 0;
    bool bSkipComma = false;
    bool bBeforeEqual = true;
    while(start <= stop){
        if (desc[start] == '<'){
            if (bBeforeEqual){
                bSkipComma = true;
            }
        }
        else if (bSkipComma){
            if (desc[start] == '>'){
                bSkipComma = false;
            }
        }
        else if (desc[start] == ','){
            bBeforeEqual = true;
            iCommaCount++;
            if (iCommaCount == paramIndex){
                highlightStart = start+1;
            }
            else if (iCommaCount == (paramIndex+1)){
                highlightStop = start;
            }
        }
        else if (desc[start] == '='){
            bBeforeEqual = false;
        }
        start++;
    }
    
    // int value1, int value2, int value3,
    // std::map<int, test> &input, std::map<int,test> &output
    mActiveFunctionParam = desc;
    CallTipSetHighlight(highlightStart, highlightStop);
    return true;
}

bool ceEdit::OnUpdateFunctionParam(wxKeyEvent &event)
{
    if (mActiveFunctionParam.empty()){
        return false;
    }
    if (WXK_DOWN == event.GetKeyCode() || WXK_UP == event.GetKeyCode() ||
        WXK_ESCAPE == event.GetKeyCode() || WXK_HOME == event.GetKeyCode() || WXK_END == event.GetKeyCode() ||
        WXK_PAGEUP == event.GetKeyCode() || WXK_PAGEDOWN == event.GetKeyCode() ||
        '{' == event.GetKeyCode() || ';' == event.GetKeyCode()){
        if (CallTipActive()){
            EndCallTip();
            return true;
        }
    }
    if (WXK_BACK == event.GetKeyCode() || WXK_DELETE == event.GetKeyCode() || 
        WXK_LEFT == event.GetKeyCode() || WXK_RIGHT == event.GetKeyCode() || ')' == event.GetKeyCode() ||
        ',' == event.GetKeyCode()){
        HighLightNextParam(mActiveFunctionParam);
    }
    return true;
}

bool ceEdit::ShowFunctionParam(int pos, const wxString &desc)
{
    if (CallTipActive()){
        CallTipCancel();
    }
    mActiveFunctionParam = "";
    mActiveFunctionParamStart = 0;
    CallTipShow(pos, desc);
    int start = desc.find("(");
    if (start == desc.npos){
        return false;
    }
    mActiveFunctionParamStart = pos + start;
    HighLightNextParam(desc);
    return true;
}

bool ceEdit::ShowCallTipsOfLocalVariableOrParam(int start, int stop)
{
    wxString value;
    int pos = start;
    int lastPos = start;
    int targetStyle = GetStyleAt(start);
    if (targetStyle == STYLE_PARAMETER_REF){
        targetStyle = STYLE_PARAMETER;
    }
    else if (targetStyle == STYLE_VARIABLE_REF){
        targetStyle = STYLE_VARIABLE;
    }
    else{
        return false;
    }
    wxString targetWord = GetTextRange(start, stop);
    // wxPrintf("TargetWord:<%s>\n", targetWord);
    while(pos > 0){
        int style = GetStyleAt(pos);
        char c = GetCharAt(pos);
        if (style == targetStyle){
            int startPos = FindStyleStart(style, pos);
            wxString word = GetTextRange(startPos, pos+1);
            // wxPrintf("Word:<%s>\n", word);
            if (word == targetWord){
                lastPos = startPos;
                break;
            }
            pos = startPos;
        }
        if (style == STYLE_FUNCTION){
            break;
        }
        pos--;
    }
    int line = LineFromPosition(lastPos);
    if (line == LineFromPosition(start)){
        // wxPrintf("Line is same:%d\n", line+1);
        return false;
    }
    value = GetLineText(line);
    pos = value.find_first_not_of("\r\n\t ");
    if (pos == value.npos){
        return false;
    }
    value = value.substr(pos);
    mLocalCallTipLine = line;
    value = wxString::Format(wxT("Line:%d %s"), line+1, value);
    CallTipShow(start, value);
    return true;
}

bool ceEdit::ShowCallTips(int curPos)
{
    int start = WordStartPosition(curPos, true);
    int stop = WordEndPosition(curPos, true);
    wxString value = GetTextRange(start, stop);
    if (NULL == wxGetApp().frame()){
        return false;
    }
    if (value.find_first_not_of("\r\n\t ") == value.npos){
        return false;
    }
    // skip comments, string &keywords, operator
    int style = GetStyleAt(start);
    wxString type = GetTypeByStyle(style);
    if (type.empty()){
        if (style == STYLE_VARIABLE_REF || style == STYLE_PARAMETER_REF){
            ShowCallTipsOfLocalVariableOrParam(start, stop);
        }
        return false;
    }
    EndCallTip();
    // wxPrintf("ShowCallTips for:<%d><%s>\n", curPos, value);
    wxString line = GetLineText(GetCurrentLine());
    long x = 0;
    PositionToXY(start, &x, NULL);
    
    wxString className;
    if (GetStyleAt(start) == STYLE_FUNCTION){
        if (start > 1 && GetCharAt(start-1) == '~' && GetStyleAt(start-1) == STYLE_OPERATOR){
            value = "~" + value;
            start--;
        }
        className = GetClassName(start);
    }
    else
    {
        className = WhichFunction(curPos);
    }
    int pos = className.find("::");
    if (pos != className.npos){
        className = className.substr(0, pos);
    }
    else{
        className = WhichClass(curPos);
    }
    // wxGetApp().frame()->FindDef(outputs, value, className, type, , GetFilename());
    PrepareFunctionParams(curPos);
    wxString typeClass = GetType(curPos, *this, className, mLanguage, GetFilename());
    if (!typeClass.empty()){
        // wxPrintf("typeClass:%s\n", typeClass);
        className = typeClass;
    }
    std::set<ceSymbol*> outputs;
    wxPrintf("ShowCallTips:<%s><%s>\n", value, className);
    // wxString lan = mLanguage; if (lan == "C++"){ lan = "C";}
    wxGetApp().frame()->FindDef(outputs, value, className, type, mLanguage, GetFilename());
    long curLine;
    PositionToXY(curPos, NULL, &curLine); curLine++;
    std::set<ceSymbol*>::iterator it = outputs.begin();
    while(it != outputs.end()){
        if ((*it)->file == GetFilename() && curLine == (*it)->line){
            delete (*it);
        }
        else{
            // wxPrintf("ShowCallTips:%s\n", (*it)->ToAutoCompString());
            mActiveCallTips.push_back(*it);
        }
        it++;
    }
    if (mActiveCallTips.size() == 0){
        return false;
    }
    
    value = wxEmptyString;
    if (mActiveCallTips.size() > 1){
        value = "\001\002";
    }
    mActiveCallTipsIndex = 0;
    mActiveCallTipsPosition = start;
    ceSymbol *pSymbol = mActiveCallTips[mActiveCallTipsIndex];
    if (!pSymbol->shortFilename.empty()){
        value += pSymbol->shortFilename;
    }
    else{
        value += pSymbol->file;
    }
    value += "(" + pSymbol->lineNumber + "):" + pSymbol->desc;
    if (value.empty()){
        return false;
    }
    CallTipShow(mActiveCallTipsPosition, value);
    return true;
}

static inline bool IsStyleNeedToSkip(int style, char c){
    if (style == STYLE_FOLDER || style == STYLE_OPERATOR){
        return false;
    }
    return true;
}

bool ceEdit::GetMatchRange(long curPos, long &startPos, long &stopPos, char sc, char ec)
{
    // todo:fanhongxuan@gmail.com
    // if user select in a function param, we need to auto select all the function body
    // set the start and end
    startPos = curPos;
    // skip the {} in comments and string
    while(startPos >0){
        char c = GetCharAt(startPos);
        if (!IsStyleNeedToSkip(GetStyleAt(startPos), c)){
            if (c == ec){
                startPos = BraceMatch(startPos);
            }
            else if (c == sc){
                break;
            }
        }
        startPos--;
    }
    
    stopPos = curPos;
    int end = GetLastPosition();
    while(stopPos < end && stopPos > 0){
        char c = GetCharAt(stopPos);
        if (!IsStyleNeedToSkip(GetStyleAt(stopPos), c)){
            if (c == sc){
                stopPos = BraceMatch(stopPos);
            }
            else if (c == ec){
                break;
            }
        }
        stopPos++;
    }
    if (startPos < 0){
        startPos = 0;
    }
    if (stopPos < 0){
        stopPos = end;
    }    
    return true;
}

bool ceEdit::StartReplaceInRegion(){
    // todo:fanhongxuan@gmail.com
    // when repace in region is run, don't update the indent according the fold level.
    
    long startPos = 0, stopPos = 0;
    long start = 0, stop = 0;
    int count = 0;
    mbReplace = true;
    GetSelection(&start, &stop);
    
    if (start == stop){
        start = WordStartPosition(GetCurrentPos(), true);
        stop = WordEndPosition(GetCurrentPos(), true);
        SetSelection(start, stop);
    }
    
    // wxString text = GetSelectedText();
    wxString text= GetTextRange(start, stop);
    if (text.empty()){
        return false;
    }
    
    wxString lowText = text.Lower();
    int flag = wxSTC_FIND_WHOLEWORD;
    if (lowText != text){
        flag |= wxSTC_FIND_MATCHCASE;
    }
    int style = GetStyleAt(start);
    if (style == STYLE_PARAMETER){
        startPos = 0, stopPos = 0;
        // if (startPos == 0 && stopPos == 0){
        // we are in the param list of the function.
        // goto next to include the function body.
        startPos = start;
        long last = GetLastPosition();
        while(startPos < last){
            if (GetCharAt(startPos) == '{' && GetStyleAt(startPos) == STYLE_FOLDER){
                stopPos = BraceMatch(startPos);
                break;
            }
            startPos++;
        }
        startPos = start;
    }
    else if (style == STYLE_PARAMETER_REF){
        WhichFunction(start, &startPos, &stopPos);
        // we are in the body of the function. go ahead to include the function param list.
        while(startPos > 0){
            if (GetStyleAt(startPos) == STYLE_FUNCTION){
                break;
            }
            startPos--;
        }
    }
    else if (style == STYLE_VARIABLE || style == STYLE_VARIABLE_REF){
        WhichFunction(start, &startPos, &stopPos);
        if (startPos == 0 && stopPos == 0){
            GetMatchRange(GetCurrentPos(), startPos, stopPos, '{', '}');
        }
    }
    else{
        GetMatchRange(GetCurrentPos(), startPos, stopPos, '{', '}');
    }
    long sx, sy, ex, ey;
    PositionToXY(startPos, &sx, &sy);
    PositionToXY(stopPos, &ex, &ey);
    // wxPrintf("Replace in:%ld:%ld<-->%ld:%ld(%ld<->%ld)\n", sy+1, sx+1, ey+1, ex+1, startPos, stopPos);
    SetSelForeground(false, wxColour("BLUE"));
    SetSelBackground(true, wxColour("BLUE"));
    
    // wxPrintf("StartReplaceInRegion:%ld<->%ld\n", startPos, stopPos);
    while(startPos < stopPos){
        int ret = FindText(stopPos, startPos, text, flag);
        if (ret < 0){
            break;
        }
        if (ret != start && stop != (ret+text.length())){
            AddSelection(ret, ret + text.length());
        }
        count++;
        stopPos = ret;
    }
    AddSelection(start, stop);
    if (NULL != wxGetApp().frame()){
        wxGetApp().frame()->ShowStatus(wxString::Format(wxT(" %d match(s) for '%s'"), count, text), 1);
    }
    return true;
}

bool ceEdit::TriggerCommentRange(long start, long stop)
{
    // find all the line in the range.
    long startLine = 0, endLine = 0, endColum = 0, stopLine = 0;
    PositionToXY(start, NULL, &startLine);
    PositionToXY(stop, &endLine, &endLine);
    stopLine = endLine;
    
    // wxPrintf("select line:<%ld-%ld>\n", startLine+1, endLine+1);
    wxString comment = "// ";
    if (start == stop){
        // no selection, add comments at the line end
        int pos = 0;// 
        wxString value = GetLineText(startLine);
        pos = value.find(comment);
        // fixme:fanhongxuan@gmail.com
        // later,we need to skip the in the string.
        if (pos == value.npos){
            pos = GetLineEndPosition(startLine);
            InsertText(pos, comment);
        }
        pos = GetLineEndPosition(startLine);
        GotoPos(pos);
        ChooseCaretX();
        return true;
    }
    else{
        int minIndent = -1;
        int i = 0;
        int count = 0;
        
        // make sure stop is bigger than start
        if (startLine > endLine){
            long temp = endLine;endLine = startLine; startLine = temp;
        }
        
        // if all the line is start with "// ", we will try to delete all the "// " at the beginning.
        for (i = startLine; i <= endLine; i++){
            wxString text;
            if (i == startLine){
                text = GetTextRange(start, GetLineEndPosition(i));
            }
            else if (i == endLine){
                text = GetTextRange(XYToPosition(0, i), stop);
            }
            else{
                text = GetLineText(i);
            }
            // wxPrintf("Text:%d<%s>\n", i+1,text);
            int pos = text.find_first_not_of("\r\n\t ");
            if (pos != text.npos){
                text = text.substr(pos);
                if (text.find(comment) != 0){
                    break;
                }
            }
        }
        
        if (i == (endLine+1)){
            // wxPrintf("all the line is start with // , try to remove the comments\n");
            size_t len = comment.length();
            for (i = startLine; i <= endLine; i++){
                int pos = 0;
                wxString text;
                if (i == startLine){
                    text = GetTextRange(start, GetLineEndPosition(i));
                    pos = text.find(comment);
                    if (pos != text.npos){
                        DeleteRange(start + pos, len);
                    }
                }
                else{
                    text = GetLineText(i);
                    pos = text.find(comment);
                    if (pos != text.npos){
                        DeleteRange(XYToPosition(0, i) + pos, len);
                    }
                }
            }
                
            int pos = XYToPosition(endColum, stopLine);
            SetSelection(pos, pos);
            GotoPos(pos);
            ChooseCaretX();            
            return true;
        }
        
        // otherwsize, add "// " at the min indent
        for (i = startLine; i <= endLine; i++){
            int indent = GetLineIndentation(i);
            if (minIndent == -1 || indent < minIndent){
                minIndent = indent;
            }
        }
        
        for (i = startLine; i <= endLine; i++){
            int pos = XYToPosition(0, i);
            if (i == startLine && start > GetLineIndentPosition(i)){
                // if the first line is not select all the content, keep the left content
                pos = start;
            }
            else{
                pos += minIndent;
            }
            wxString text = GetTextRange(pos, GetLineEndPosition(i));
            if (text.find_first_not_of("\r\n\t ") == text.npos){
                // skip the empty line
                continue;
            }
            if (i == endLine){
                text = GetTextRange(XYToPosition(0, i), stop + count);
                if (text.find_first_not_of("\r\n\t ") == text.npos){
                    continue;
                }
            }
            InsertText(pos, comment);
            count += comment.length();
            
            if (i == endLine){
                text = GetTextRange(stop + count, GetLineEndPosition(i));
                if (text.find_first_not_of("\r\n\t ") != text.npos){
                    // has some thing left, add a newline.
                    InsertNewLine(stop + count);
                    if (stopLine >= endLine){
                        stopLine++;
                    }
                }
            }
        }
    }
    int pos = XYToPosition(endColum, stopLine);
    
    SetSelection(pos, pos);
    GotoPos(pos);
    ChooseCaretX();
    return true;
}

void ceEdit::OnUpdateIndentByFolder(wxCommandEvent &evt){
    int line = evt.GetInt();
    int curLine = GetCurrentLine();
    int indent = CalcLineIndentByFoldLevel(line, GetFoldLevel(line));
    SetLineIndentation(line, indent);
    if (line == curLine && line > 0){
        int start = GetCurrentPos();
        int end = GetLineEndPosition(curLine-1);
        if (GetEOLMode() == wxSTC_EOL_CRLF){
            end++;
        }
        if (start == (end + 1)){
            GotoPos(GetLineIndentPosition(curLine));
            // note:fanhongxuan@gmail.com
            // See SCI_CHOOSECARETX
            ChooseCaretX();
        }
    }
}

bool ceEdit::AutoIndentWithTab(int line)
{
    // wxPrintf("AutoIndentWithTab:%d\n", line);
    // auto indent current line
    int pos = GetCurrentPos();
    // calc the line indent according the level
    int curIndent = GetLineIndentation(line);
    int indent = CalcLineIndentByFoldLevel(line, GetFoldLevel(line));
    if (indent != curIndent){
        SetLineIndentation(line, indent);
        pos = pos + indent - curIndent;
        if (line > 1 && pos <= GetLineEndPosition(line - 1)){
            pos = GetLineIndentPosition(line);
        }
        GotoPos(pos);
        ChooseCaretX();
    }
    else{
        if (pos < GetLineIndentPosition(line)){
            GotoPos(GetLineIndentPosition(line));
            ChooseCaretX();
        }
    }
    return true;
}

bool ceEdit::InsertNewLine(long pos)
{
    int eol = GetEOLMode();
    if (eol == wxSTC_EOL_LF){
        // wxPrintf("Insert LF\n");
        InsertText(pos, "\n");
    }
    else if (eol == wxSTC_EOL_CRLF){
        // wxPrintf("Insert CRLF\n");
        InsertText(pos, "\r\n");
    }
    else{
        // wxPrintf("Insert CR\n");
        InsertText(pos, "\r");    
    }
    return true;
}

// todo:fanhongxuan@gmail.com
// auto pair "", '', (), [], {},
// when delete auto delete.
// auto indent with endline
void ceEdit::AutoIndentWithNewline(int currentLine)
{
    if (mbReplace){
        // when use is replace, skip this
        return;
    }
    
    int pos = GetCurrentPos();
    if (pos >= 2){
        char prev = GetCharAt(pos - 2);
        char next =GetCharAt(pos);
        if ( pos >= 3 && prev == '\r'){
            prev = GetCharAt(pos-3);
        }
        // wxPrintf("Prev:%c, next:%c\n", prev, next);        
        if (prev == '{' && next == '}'){
            InsertNewLine(pos);
        }
        else{
            // this is a normal newline. try to indent according the fold level.
            int foldstatus = GetFoldLevel(currentLine);
            int foldlevel = foldstatus & wxSTC_FOLDLEVELNUMBERMASK - wxSTC_FOLDLEVELBASE;
            if (foldlevel >= 0){
                SetLineIndentation(currentLine, GetIndent() * foldlevel);
                GotoPos(GetLineIndentPosition(currentLine));
                ChooseCaretX();
            }
        }
    }
    
}

void ceEdit::OnCharAdded (wxStyledTextEvent &event) {
    // note:fanhongxuan@gmail.com
    // if the next is a validchar for autocomp, we don't show the autocomp
    // don't use inputWord, we find the word backword start from current pos
    // todo:fanhongxuan@gmail.com
    // 1, fill the mFunctionParames table.
    PrepareFunctionParams(GetCurrentPos());
    bool bNewCandidate = false;
    long pos = GetInsertionPoint();
    if (pos > 0){
        pos--;
    }
    
    long start = pos;
    int ch = GetCharAt(pos);
    if (!IsValidChar(ch)){
        // in this case we add the latest string as a new candidate.
        bNewCandidate = true;
    }
    
    int i = 0;
    while(start > 0){
        ch = GetCharAt(start-1);
        if (IsValidChar(ch)){
            start--;
        }
        else{
            break;
        }
    }
    
    // wxPrintf("OnCharAdded:<%d>, ch:%d, %d\n", event.GetKey(), ch, bNewCandidate);
    int mode = SetClass(GetCurrentPos());
    // fixme:fanhongxuan@gmail.com
    // if getType return a none empty value, will use mode 2
    // int mode = GetCurrentMode();
    wxString inputWord = GetTextRange(start, pos+1);
    int candidateLen = inputWord.length();
    if (bNewCandidate){
        inputWord = GetTextRange(start, pos);
        wxAutoCompWordInBufferProvider::Instance().AddCandidate(inputWord, mLanguage);
        char chr = (char)event.GetKey();
        int currentLine = GetCurrentLine();
        // Change this if support for mac files with \r is needed
        if (GetLexer() != wxSTC_LEX_CONTAINER){
            return;
        }
        if (chr == '\n' || chr == '\r'){
            AutoIndentWithNewline(currentLine);
        }
        if (chr == ';' || chr == '{'){
            MoveCharBeforeRightParentheses(currentLine);
        }
        
        if (chr == '{' || chr == '\'' || chr == '\"' || chr == '(' || chr == '['){
            InsertPair(currentLine, chr);
        }
        
        if (chr == '}'){
            OnEndBrace(currentLine);
        }
        
        if (chr == ':'){
            OnColon(currentLine);
        }
        
        if (mode != 2){
            return;
        }
        else{
            inputWord += (char)event.GetKey();
            candidateLen = 0;
        }
    }
    
    if (CallTipActive()){
        // when calltip is active, don't show autocomp;
        return;
    }
    
    std::set<wxString> candidate;
    GetCandidate(inputWord, candidate, mode);
    if (!candidate.empty()){
        wxString candidateStr;
        std::set<wxString>::iterator it = candidate.begin();
        int max = 20;
        while(it != candidate.end()){
            wxString item = (*it);
            if (item.length() >= max){
                max = item.length();
            }
            candidateStr += item;
            candidateStr += '\1';
            it++;
        }
        max = max * 1.1;
        if (max > 200){
            max = 200;
        }
        AutoCompSetMaxWidth(max+1);
        AutoCompShow(candidateLen, candidateStr);
    }
}

int ceEdit::CalcLineIndentByFoldLevel(int line, int level)
{
    int fixValue = 0;
    if (line == 0){
        return 0;
    }
    level = level & wxSTC_FOLDLEVELNUMBERMASK;
    if (level < wxSTC_FOLDLEVELBASE){
        level = 0;
    }
    else{
        level -= wxSTC_FOLDLEVELBASE;
    }
    
    wxString value = GetLineText(line);
    int startChar = value.find_first_not_of("\r\n\t ");
    int endChar = value.find_last_not_of("\r\n\t ");
    if (startChar != value.npos && endChar != value.npos && endChar >= startChar){
        value = value.substr(startChar, endChar + 1-startChar);
        if (value == "}" || 
            value == "public:" || 
            value == "protected:" || 
            value == "private:" || 
            value == "default:" ||
            value.find("case ") == 0){
            fixValue = GetIndent();
        }
        // else if (value.size() > 0 && value[value.size()-1] == ':'){
        //     fixValue = GetIndent();
        // }
        // if this line is start with # set the indent to 0
        if (value.size() >= 1 && value[0] == '#'){
            level = 0;
        }
        // if (value.find("// ") == 0){
        //     // skip the auto comments line
        //     return GetLineIndentation(line);
        // }
    }
    
    int indent = 0;
    int parent = line;
    while(level > 0){
        parent = GetFoldParent(parent);
        if (parent >= 0){
            indent += GetLineState(parent);
            // indent += GetIndent();
        }
        level--;
    }
    // return level * GetIndent();
    return indent - fixValue;
}

wxString ceEdit::GetCurrentWord(const wxString &validCharList)
{
    // note:fanhongxuan@gmail.com
    // when multselection is enabled, GetSelectText will return all the selected text.
    // so we use GetSelection to get the main selection, and then GetTextRange.
    // if has selection, return selection
    int startPos = 0, stopPos = 0;
    GetSelection(&startPos, &stopPos);
    wxString ret = GetTextRange(startPos, stopPos);
    if (!ret.empty()){
        return ret;
    }
    long pos = GetInsertionPoint();
    long max = GetText().Length();
    long start = pos, end = pos;
    int i = 0;
    while(start > 0){
        int ch = GetCharAt(start-1);
        if (IsValidChar(ch)){
            start--;
        }
        else{
            break;
        }
    }
    
    while(end < max){
        int ch = GetCharAt(end+1);
        if (IsValidChar(ch)){
            end++;
        }
        else{
            break;
        }
    }
    if (start == end){
        return wxEmptyString;
    }
    if (!IsValidChar(GetCharAt(pos))){
        if (start != pos){
            end = pos - 1;
        }
        else{
            start = pos+1;
        }
    }
    return GetTextRange(start, end+1);
}

void ceEdit::OnGotoLine(wxCommandEvent &evt){
    wxTextEntryDialog  dlg(this, 
                           wxString::Format(wxT("Please input the target line  number(1<->%d):"), GetLineCount()),
                           wxT("Please Input the target line number"),
                           wxString::Format("%d", GetCurrentLine()+1));
    if (dlg.ShowModal() == wxID_OK){
        long line = 0;
        dlg.GetValue().ToLong(&line);
        if (line < 1){
            line = 1;
        }
        if (line > GetLineCount()){
            line = GetLineCount();
        }
        line--;
        int firstVisiableLine = GetFirstVisibleLine();
        if (firstVisiableLine > line ||  line != GetCurrentLine()){
            SetFirstVisibleLine(line-20);
            GotoLine(line);
        }
    }
}

void ceEdit::OnZoom(wxStyledTextEvent &evt)
{
    UpdateLineNumberMargin();
}
