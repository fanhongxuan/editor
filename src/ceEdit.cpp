#include "ceEdit.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf
#include <map>
#include <set>
#include "ceUtils.hpp"

wxBEGIN_EVENT_TABLE(ceEdit, wxStyledTextCtrl)
EVT_STC_STYLENEEDED(wxID_ANY, ceEdit::OnStyleNeeded)
EVT_STC_MODIFIED(wxID_ANY,    ceEdit::OnModified)
EVT_SIZE(ceEdit::OnSize)
wxEND_EVENT_TABLE()

enum{
    STYLE_DEFAULT = 0,
    STYLE_STRING,  // "strng"
    STYLE_CHAR,    // 'c'
    STYLE_COMMENTS, // //
    STYLE_CSTYLE_COMMENTS, // /**/
    STYLE_PREPROCESS,   // #define/#ifdef
    STYLE_PREPROCESS_LOCAL, // #include "file.h"
    STYLE_PREPROCESS_SYSTEM, // #include <file.h>
    STYLE_KEYWORD1,
    STYLE_KEYWORD2,
    STYLE_KEYWORD3,
    STYLE_MACRO,        // macro
    STYLE_OPERATOR,     // operator,
    STYLE_FUNCTION,     // function,
    STYLE_TYPE,         // type, use defined data type.
    STYLE_FOLDER,       // (){}
    STYLE_NUMBER,       // 123, 0x123, 0.123 etc.
    STYLE_IDENTY,       // unknown id,
    STYLE_NORMAL,       // other words.
    STYLE_MAX,
};

class ceParseState{
public:
    int currentStyle;
    
    ceParseState(){
        currentStyle = STYLE_DEFAULT;
    }
};

ceEdit::ceEdit(wxWindow *parent)
:wxStyledTextCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize){
    mpParseState = NULL;
    mbLoadFinish = true;
    mLinenuMargin = 0;
    mDeviderMargin = 1;
    mFoldingMargin = 2;
    SetLexer(wxSTC_LEX_CONTAINER);
    mpParseState = new ceParseState;
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
    return true;
}

bool ceEdit::NewFile(const wxString &filename){
    LoadStyleByFileName(filename);
    return true;
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
        
        mForegrounds[STYLE_OPERATOR] = wxColor("YELLOW");
        mBackgrounds[STYLE_OPERATOR] = *wxBLACK;
        
        mForegrounds[STYLE_FOLDER] = wxColor("YELLOW");
        mBackgrounds[STYLE_FOLDER] = *wxBLACK;
        
        
        mForegrounds[STYLE_NUMBER] = wxColor("YELLOW");
        mBackgrounds[STYLE_NUMBER] = *wxBLACK;
        
        // mForegrounds[STYLE_IDENTY] = wxColor("BLACK");
        // mBackgrounds[STYLE_IDENTY] = *wxWHITE;
        
        mForegrounds[STYLE_KEYWORD1] = wxColor("RED");
        mBackgrounds[STYLE_KEYWORD1] = *wxBLACK;        
        
        mForegrounds[STYLE_PREPROCESS_SYSTEM] = wxColor("RED");
        mBackgrounds[STYLE_PREPROCESS_SYSTEM] = *wxBLACK;        
    }
    std::map<int, wxColor>::iterator it;
    if (type == 1){
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

bool ceEdit::LoadStyleByFileName(const wxString &filename)
{
    StyleClearAll();
#ifdef WIN32
    SetEOLMode(wxSTC_EOL_CRLF);
#else
    SetEOLMode(wxSTC_EOL_LF);
#endif
    
    int charset = 65001;
    for (int i = 0; i < wxSTC_STYLE_LASTPREDEFINED; i++) {
        StyleSetCharacterSet (i, charset);
    }
    SetCodePage (charset);
    
    wxFont font(wxFontInfo(11).Family(wxFONTFAMILY_MODERN));
    StyleSetFont (wxSTC_STYLE_DEFAULT, font);
    StyleSetForeground (wxSTC_STYLE_DEFAULT, *wxWHITE);
    StyleSetBackground (wxSTC_STYLE_DEFAULT, *wxBLACK);
    
    // set margin type and style for line number
    SetMarginType (mLinenuMargin, wxSTC_MARGIN_NUMBER);
    // note:fanhongxuan@gmail.com
    // do not set the margin width here
    // update the linenumber margin width when file is modified.
    // if we set a fix value here, the later update is not work.
    // SetMarginWidth(mLinenuMargin, 0);
    StyleSetForeground (wxSTC_STYLE_LINENUMBER, wxColour (wxT("GREEN")));
    StyleSetBackground (wxSTC_STYLE_LINENUMBER, *wxBLACK);
    
    // set style for indentguide
    StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, wxColour (wxT("GREEN")));
    StyleSetBackground(wxSTC_STYLE_INDENTGUIDE, wxColour (wxT("GREEN")));
    
    // SetCaretWidth(20);
    SetCaretStyle(wxSTC_CARETSTYLE_BLOCK);
    SetCaretForeground(wxColor(wxT("RED")));
    SetCaretLineBackground(wxColour(193, 213, 255));
    SetCaretLineBackAlpha(60);
    SetCaretLineVisible(true);
    SetCaretLineVisibleAlways(true);
    
    CallTipSetBackground(*wxYELLOW);
    CallTipSetForeground(*wxBLUE);
    
    // set style for brace light and bad
    StyleSetForeground(wxSTC_STYLE_BRACELIGHT, wxColour (wxT("BLACK")));
    StyleSetBackground(wxSTC_STYLE_BRACELIGHT, wxColour (wxT("GREEN")));
    
    StyleSetForeground(wxSTC_STYLE_BRACEBAD, wxColour(wxT("BLACK")));
    StyleSetBackground(wxSTC_STYLE_BRACEBAD, wxColour(wxT("RED")));
    
    // for multselection and replace
    StyleSetFont (wxSTC_STYLE_LASTPREDEFINED + 1, font);
    StyleSetForeground (wxSTC_STYLE_LASTPREDEFINED + 1, *wxGREEN);
    StyleSetBackground (wxSTC_STYLE_LASTPREDEFINED + 1, *wxBLACK);
    
    // default fonts for all styles!
    int Nr;
    for (Nr = 0; Nr < wxSTC_STYLE_LASTPREDEFINED; Nr++) {
        wxFont font(wxFontInfo(11).Family(wxFONTFAMILY_MODERN));
        StyleSetFont (Nr, font);
    }
    
    // set margin as unused
    SetMarginType (mDeviderMargin, wxSTC_MARGIN_SYMBOL);
    SetMarginWidth (mDeviderMargin, 0);
    SetMarginSensitive (mDeviderMargin, false);
    
    // folding
    SetMarginType (mFoldingMargin, wxSTC_MARGIN_SYMBOL);
    SetMarginMask (mFoldingMargin, wxSTC_MASK_FOLDERS);
    SetMarginWidth (mFoldingMargin, 16);
    SetMarginSensitive (mFoldingMargin, true); // don't handle the mouse click
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
    SetProperty(wxT("fold.comment"), wxT("1"));
    SetProperty(wxT("fold.compact"), wxT("1"));
    SetProperty(wxT("fold.preprocessor"), wxT("0"));
    
    SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | // wxSTC_FOLDFLAG_LEVELNUMBERS |
                 wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);
    
    // set spaces and indentation
    SetTabWidth(4);
    SetUseTabs(false);
    SetTabIndents(false);
    SetBackSpaceUnIndents(false);
    CmdKeyClear(wxSTC_KEY_TAB, 0); // disable the default tab handle;
    SetIndent(4);
    
    // others
    SetViewEOL(false);
    SetIndentationGuides(true);
    SetEdgeColumn(120);
    SetEdgeMode(wxSTC_EDGE_LINE);
    SetViewWhiteSpace(wxSTC_WS_INVISIBLE);
    SetOvertype(false);
    SetReadOnly(false);
    SetWrapMode(wxSTC_WRAP_WORD);

    for (int i = STYLE_DEFAULT; i < STYLE_MAX; i++){
        StyleSetFont(i, font);
        StyleSetForeground(i, GetColourByStyle(i, 0)); 
        StyleSetBackground(i, GetColourByStyle(i, 1));
    }
    
    // about autocomp
    // LoadAutoComProvider(m_language->name);
    // AutoCompSetMaxWidth(50);
}

bool ceEdit::IsInPreproces(int stopPos)
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
            return true;
        }
        startPos++;
    }
    return false;
}

extern wxString gCppKeyWord;
bool ceEdit::IsKeyWord1(const wxString &value){
    static std::set<wxString> sCppKeyWords;
    if (sCppKeyWords.size() == 0){
        std::vector<wxString> outputs;
        ceSplitString(gCppKeyWord, outputs, ' ');
        for (int i = 0; i < outputs.size(); i++ ){
            sCppKeyWords.insert(outputs[i]);
        }
    }
    return(sCppKeyWords.find(value) != sCppKeyWords.end());
}

bool ceEdit::IsNumber(const wxString &value){
    // todo:fanhongxuan@gmail.com
    // use regext to match if is a number.
    static wxString sNumber = "0123456789";
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
    return false;
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
    if (IsKeyWord1(text)){
        StartStyling(startPos);
        SetStyling(pos+1 - startPos, STYLE_KEYWORD1);
    }
    if (IsNumber(text)){
        StartStyling(startPos);
        SetStyling(pos+1 - startPos, STYLE_NUMBER);
    }
    return true;
}

int ceEdit::ParseChar( int curStyle,long pos)
{
    static wxString sAlphaNumber = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    static wxString sPreprocess = "#";
    static wxString sOperator = "~!@%^&*+-<>/?:|.=";
    static wxString sFolder = "(){}";
    static wxString sWhitespace = "\r\n\t ";
    
    char c = GetCharAt(pos);
    // wxPrintf("pos:%ld, char:<%c>, style:%d\n", pos, c, curStyle);
    StartStyling(pos);
    SetStyling(1, curStyle);
    
    // todo:fanhongxuan@gmail.com
    // support \ to expand the preprocessor to multiple line
    switch (curStyle){
    case STYLE_PREPROCESS:
        if (c == '\r' || c== '\n' || c == '\t' || c == ' '){
            curStyle = STYLE_DEFAULT;
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
    case STYLE_CSTYLE_COMMENTS:
        if (c == '/' && pos > 0 && GetCharAt(pos - 1) == '*'){
            curStyle = STYLE_DEFAULT;
        }
        break;
    case STYLE_CHAR:
        if ((c == '\'' && pos == 0) ||
            (c == '\'' && pos > 0 && GetCharAt(pos-1) != '\\')){
            curStyle = STYLE_DEFAULT;
        }
        break;
    case STYLE_PREPROCESS_LOCAL:
    case STYLE_STRING:
        if ((c == '\"' && pos == 0) ||
            (c == '\"' && pos > 0 && GetCharAt(pos-1) != '\\')){
            //
            curStyle = STYLE_DEFAULT;
        }
        break;
    case STYLE_IDENTY:
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
        // note:fanhongxuan@gmail.com
        // this is the word end, parse this char again.
    default:
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
            StartStyling(pos);
            SetStyling(1, STYLE_PREPROCESS);
            curStyle = STYLE_PREPROCESS;
        }
        else if ((c == '\'' && pos > 0 && GetCharAt(pos-1) != '\\') ||
            (c == '\'' && pos == 0)){
            StartStyling(pos);
            SetStyling(1, STYLE_CHAR);
            curStyle = STYLE_CHAR;
        }
        else if ((c == '\"' && pos > 0 && GetCharAt(pos-1) != '\\') ||
            (c == '\"' && pos == 0)){
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
        }
        else if (sFolder.find(c) != sFolder.npos){
            StartStyling(pos);
            SetStyling(1, STYLE_FOLDER);
        }
        else if (sAlphaNumber.find(c) != sAlphaNumber.npos){
            StartStyling(pos);
            SetStyling(1, STYLE_IDENTY);
            curStyle = STYLE_IDENTY;
        }
        else if (sWhitespace.find(c) != sWhitespace.npos){
            StartStyling(pos);
            SetStyling(1, STYLE_DEFAULT);
            curStyle = STYLE_NORMAL;
        }
        // todo:fanhongxuan@gmail.com
        // support #define to get macro define name.
        else {
            StartStyling(pos);
            SetStyling(1, STYLE_NORMAL);
            curStyle = STYLE_NORMAL;
        }
        break;
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
    
    // wxPrintf("OnStyleNeeded:%d<->%d, style:%d\n", startPos, stopPos, mpParseState->currentStyle);
    for (int i = startPos; i < stopPos; i++ ){
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
    // linenumber = wxT("99999999999");
    wxPrintf("updateLineNumberMargin:%d\n", GetLineCount());
    SetMarginWidth(mLinenuMargin, TextWidth (wxSTC_STYLE_LINENUMBER, linenumber));
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
}

void ceEdit::OnSize( wxSizeEvent& event ) {
    int x = GetClientSize().x + GetMarginWidth(mLinenuMargin) + GetMarginWidth(mFoldingMargin);
    if (x > 0) SetScrollWidth (x);
    event.Skip();
}