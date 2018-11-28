//////////////////////////////////////////////////////////////////////////////
// File:        contrib/samples/stc/edit.cpp
// Purpose:     STC test module
// Maintainer:  Wyo
// Created:     2003-09-01
// Copyright:   (c) wxGuide
// Licence:     wxWindows licence
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// informations
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// headers
//----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all 'standard' wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
    #include "wx/textdlg.h"
#endif

//! wxWidgets headers
#include "wx/file.h"     // raw file io support
#include "wx/filename.h" // filename support

//! application headers
#include "wxDefsext.hpp"     // additional definitions

#include "wxEdit.hpp"        // edit module

#include "wxAutoComp.hpp"
#include "ce.hpp"

//----------------------------------------------------------------------------
// resources
//----------------------------------------------------------------------------


//============================================================================
// declarations
//============================================================================

// The (uniform) style used for the annotations.
const int ANNOTATION_STYLE = wxSTC_STYLE_LASTPREDEFINED + 1;

//============================================================================
// implementation
//============================================================================

//----------------------------------------------------------------------------
// Edit
//----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE (Edit, wxStyledTextCtrl)
    // common
    // add by fanhongxuan@gmail.com
    EVT_STC_MODIFIED(wxID_ANY,         Edit::OnModified)
    EVT_SIZE (                         Edit::OnSize)
    // edit
    EVT_MENU (wxID_CLEAR,              Edit::OnEditClear)
    EVT_MENU (wxID_CUT,                Edit::OnEditCut)
    EVT_MENU (wxID_COPY,               Edit::OnEditCopy)
    EVT_MENU (wxID_PASTE,              Edit::OnEditPaste)
    EVT_MENU (myID_INDENTINC,          Edit::OnEditIndentInc)
    EVT_MENU (myID_INDENTRED,          Edit::OnEditIndentRed)
    EVT_MENU (wxID_SELECTALL,          Edit::OnEditSelectAll)
    EVT_MENU (myID_SELECTLINE,         Edit::OnEditSelectLine)
    EVT_MENU (wxID_REDO,               Edit::OnEditRedo)
    EVT_MENU (wxID_UNDO,               Edit::OnEditUndo)
    // find
    EVT_MENU (wxID_FIND,               Edit::OnFind)
    EVT_MENU (myID_FINDNEXT,           Edit::OnFindNext)
    EVT_MENU (myID_REPLACE,            Edit::OnReplace)
    EVT_MENU (myID_REPLACENEXT,        Edit::OnReplaceNext)
    EVT_MENU (myID_BRACEMATCH,         Edit::OnBraceMatch)
    EVT_MENU (myID_GOTO,               Edit::OnGoto)
    // view
    EVT_MENU_RANGE (myID_HIGHLIGHTFIRST, myID_HIGHLIGHTLAST,
                                       Edit::OnHighlightLang)
    EVT_MENU (myID_DISPLAYEOL,         Edit::OnDisplayEOL)
    EVT_MENU (myID_INDENTGUIDE,        Edit::OnIndentGuide)
    EVT_MENU (myID_LINENUMBER,         Edit::OnLineNumber)
    EVT_MENU (myID_LONGLINEON,         Edit::OnLongLineOn)
    EVT_MENU (myID_WHITESPACE,         Edit::OnWhiteSpace)
    EVT_MENU (myID_FOLDTOGGLE,         Edit::OnFoldToggle)
    EVT_MENU (myID_OVERTYPE,           Edit::OnSetOverType)
    EVT_MENU (myID_READONLY,           Edit::OnSetReadOnly)
    EVT_MENU (myID_WRAPMODEON,         Edit::OnWrapmodeOn)
    EVT_MENU (myID_CHARSETANSI,        Edit::OnUseCharset)
    EVT_MENU (myID_CHARSETMAC,         Edit::OnUseCharset)
    // annotations
    EVT_MENU (myID_ANNOTATION_ADD,     Edit::OnAnnotationAdd)
    EVT_MENU (myID_ANNOTATION_REMOVE,  Edit::OnAnnotationRemove)
    EVT_MENU (myID_ANNOTATION_CLEAR,   Edit::OnAnnotationClear)
    EVT_MENU (myID_ANNOTATION_STYLE_HIDDEN,   Edit::OnAnnotationStyle)
    EVT_MENU (myID_ANNOTATION_STYLE_STANDARD, Edit::OnAnnotationStyle)
    EVT_MENU (myID_ANNOTATION_STYLE_BOXED,    Edit::OnAnnotationStyle)
    // extra
    EVT_MENU (myID_CHANGELOWER,                 Edit::OnChangeCase)
    EVT_MENU (myID_CHANGEUPPER,                 Edit::OnChangeCase)
    EVT_MENU (myID_CONVERTCR,                   Edit::OnConvertEOL)
    EVT_MENU (myID_CONVERTCRLF,                 Edit::OnConvertEOL)
    EVT_MENU (myID_CONVERTLF,                   Edit::OnConvertEOL)
    EVT_MENU(myID_MULTIPLE_SELECTIONS,          Edit::OnMultipleSelections)
    EVT_MENU(myID_MULTI_PASTE,                  Edit::OnMultiPaste)
    EVT_MENU(myID_MULTIPLE_SELECTIONS_TYPING,   Edit::OnMultipleSelectionsTyping)
    EVT_MENU(myID_CUSTOM_POPUP,                 Edit::OnCustomPopup)
    EVT_SET_FOCUS(Edit::OnFocus)
    // stc
    EVT_STC_MARGINCLICK (wxID_ANY,     Edit::OnMarginClick)
    EVT_STC_CHARADDED (wxID_ANY,       Edit::OnCharAdded)

    EVT_KEY_DOWN( Edit::OnKeyDown )
EVT_KEY_UP(Edit::OnKeyUp)
wxEND_EVENT_TABLE()

Edit::Edit (wxWindow *parent,
            wxWindowID id,
            const wxPoint &pos,
            const wxSize &size,
            long style)
    : wxStyledTextCtrl (parent, id, pos, size, style) {

    m_filename = wxEmptyString;

    m_LineNrID = 0;
    m_DividerID = 1;
    m_FoldingID = 2;

    // initialize language
    m_language = NULL;

    // default font for all styles
    SetViewEOL (g_CommonPrefs.displayEOLEnable);
    SetIndentationGuides (g_CommonPrefs.indentGuideEnable);
    SetEdgeMode (g_CommonPrefs.longLineOnEnable?
                 wxSTC_EDGE_LINE: wxSTC_EDGE_NONE);
    SetViewWhiteSpace (g_CommonPrefs.whiteSpaceEnable?
                       wxSTC_WS_VISIBLEALWAYS: wxSTC_WS_INVISIBLE);
    SetOvertype (g_CommonPrefs.overTypeInitial);
    SetReadOnly (g_CommonPrefs.readOnlyInitial);
    SetWrapMode (g_CommonPrefs.wrapModeInitial?
                 wxSTC_WRAP_WORD: wxSTC_WRAP_NONE);
    // wxFont font(wxFontInfo(10).Family(wxFONTFAMILY_MODERN));
    // StyleSetFont (wxSTC_STYLE_DEFAULT, font);
    StyleSetForeground (wxSTC_STYLE_DEFAULT, *wxBLACK);
    StyleSetBackground (wxSTC_STYLE_DEFAULT, *wxWHITE);
    StyleSetForeground (wxSTC_STYLE_LINENUMBER, wxColour (wxT("DARK GREY")));
    StyleSetBackground (wxSTC_STYLE_LINENUMBER, *wxWHITE);
    StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, wxColour (wxT("DARK GREY")));
    InitializePrefs (DEFAULT_LANGUAGE);

    // set visibility
    SetVisiblePolicy (wxSTC_VISIBLE_STRICT|wxSTC_VISIBLE_SLOP, 1);
    SetXCaretPolicy (wxSTC_CARET_EVEN|wxSTC_VISIBLE_STRICT|wxSTC_CARET_SLOP, 1);
    SetYCaretPolicy (wxSTC_CARET_EVEN|wxSTC_VISIBLE_STRICT|wxSTC_CARET_SLOP, 1);

    // markers
    MarkerDefine(wxSTC_MARKNUM_FOLDER,        wxSTC_MARK_BOXPLUS, wxT("WHITE"), wxT("BLACK"));
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN,    wxSTC_MARK_BOXMINUS,  wxT("WHITE"), wxT("BLACK"));
    MarkerDefine(wxSTC_MARKNUM_FOLDERSUB,     wxSTC_MARK_VLINE,     wxT("WHITE"), wxT("BLACK"));
    MarkerDefine(wxSTC_MARKNUM_FOLDEREND,     wxSTC_MARK_BOXPLUSCONNECTED, wxT("WHITE"), wxT("BLACK"));
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED, wxT("WHITE"), wxT("BLACK"));
    MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER,     wxT("WHITE"), wxT("BLACK"));
    MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL,    wxSTC_MARK_LCORNER,     wxT("WHITE"), wxT("BLACK"));

    // miscellaneous
    m_LineNrMargin = TextWidth (wxSTC_STYLE_LINENUMBER, wxT("_09999"));
    m_FoldingMargin = 16;

    
    SetCaretLineBackground(wxColour(193, 213, 255));
    SetCaretLineVisible(true);
    SetCaretLineVisibleAlways(true);
    
    SetLayoutCache (wxSTC_CACHE_PAGE);
    UsePopUp(wxSTC_POPUP_ALL);
}

Edit::~Edit () {}

wxString Edit::GetCurrentWord(const wxString &validCharList)
{
    // if has selection, return selection
    wxString ret = GetSelectedText();
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

//----------------------------------------------------------------------------
// common event handlers

void Edit::OnFocus(wxFocusEvent &evt)
{
    if (NULL != wxGetApp().frame()){
        wxGetApp().frame()->DoUpdate();
        wxGetApp().frame()->SetActiveEdit(this);
    }
    evt.Skip();
}

void Edit::OnModified(wxStyledTextEvent &evt)
{
    int type = evt.GetModificationType();
    // wxPrintf("OnModified:0x%08X\n", type);
    if (type & (wxSTC_MOD_INSERTTEXT | wxSTC_MOD_DELETETEXT)){        
        UpdateLineNumberMargin();
    }
    if (type & (wxSTC_MOD_CHANGEMARKER | wxSTC_MOD_CHANGEFOLD)){
        // fold status changed.
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
        
        int level = evt.GetFoldLevelNow() & wxSTC_FOLDLEVELNUMBERMASK - wxSTC_FOLDLEVELBASE;
    
        wxString value = GetLineText(line);
        int start = value.find_first_not_of("\r\n\t ");
        int end = value.find_last_not_of("\r\n\t ", start);
        if (start != value.npos && end != value.npos){
            value = value.substr(start, end + 1-start);
            if (value == "}" || value == "public:" || value == "protected:" || value == "private:" || value == "case:"){
                level--;
            }
        }
        SetLineIndentation(evt.GetLine(), GetIndent() * level);
        if (line == curLine /*&& evt.GetFoldLevelNow() & wxSTC_FOLDLEVELWHITEFLAG*/){
            start = GetCurrentPos();
            end = GetLineEndPosition(curLine);
            start += level * GetIndent();
            if (start >= end){
                start = end;
            }
            GotoPos(start);
        }
    }
}

void Edit::OnSize( wxSizeEvent& event ) {
    int x = GetClientSize().x +
            (g_CommonPrefs.lineNumberEnable? m_LineNrMargin: 0) +
            (g_CommonPrefs.foldEnable? m_FoldingMargin: 0);
    if (x > 0) SetScrollWidth (x);
    event.Skip();
}

// edit event handlers
void Edit::OnEditRedo (wxCommandEvent &WXUNUSED(event)) {
    if (!CanRedo()) return;
    Redo ();
}

void Edit::OnEditUndo (wxCommandEvent &WXUNUSED(event)) {
    if (!CanUndo()) return;
    Undo ();
}

void Edit::OnEditClear (wxCommandEvent &WXUNUSED(event)) {
    if (GetReadOnly()) return;
    Clear ();
}

void Edit::OnKeyDown (wxKeyEvent &event)
{
    // if (CallTipActive())
    //     CallTipCancel();
    // if (event.GetKeyCode() == WXK_SPACE && event.ControlDown() && event.ShiftDown())
    // {
    //     int pos = GetCurrentPos();
    //     CallTipSetBackground(*wxYELLOW);
    //     CallTipShow(pos,
    //                 "This is a CallTip with multiple lines.\n"
    //                 "It is meant to be a context sensitive popup helper for the user.");
    //     return;
    // }
    // if (WXK_TAB == event.GetKeyCode()){
    // }
    event.Skip();
}

void Edit::OnKeyUp(wxKeyEvent &event)
{
    
}

void Edit::OnEditCut (wxCommandEvent &WXUNUSED(event)) {
    if (GetReadOnly() || (GetSelectionEnd()-GetSelectionStart() <= 0)) return;
    Cut ();
}

void Edit::OnEditCopy (wxCommandEvent &WXUNUSED(event)) {
    if (GetSelectionEnd()-GetSelectionStart() <= 0) return;
    Copy ();
}

void Edit::OnEditPaste (wxCommandEvent &WXUNUSED(event)) {
    if (!CanPaste()) return;
    Paste ();
    // update the linenumber margin
    // todo:fanhongxuan@gmail.com
    // get the pasted content and update the wxAutoCompWordInBufferProvider.
}

void Edit::OnFind (wxCommandEvent &WXUNUSED(event)) {
}

void Edit::OnFindNext (wxCommandEvent &WXUNUSED(event)) {
}

void Edit::OnReplace (wxCommandEvent &WXUNUSED(event)) {
    // todo:fanhongxuan@gmail.com
    // update the wxAutoCompWordInBufferProvider
}

void Edit::OnReplaceNext (wxCommandEvent &WXUNUSED(event)) {
}

void Edit::OnBraceMatch (wxCommandEvent &WXUNUSED(event)) {
    int min = GetCurrentPos ();
    int max = BraceMatch (min);
    if (max > (min+1)) {
        BraceHighlight (min+1, max);
        SetSelection (min+1, max);
    }else{
        BraceBadLight (min);
    }
}

void Edit::OnGoto (wxCommandEvent &WXUNUSED(event)) {
}

void Edit::OnEditIndentInc (wxCommandEvent &WXUNUSED(event)) {
    CmdKeyExecute (wxSTC_CMD_TAB);
}

void Edit::OnEditIndentRed (wxCommandEvent &WXUNUSED(event)) {
    CmdKeyExecute (wxSTC_CMD_DELETEBACK);
}

void Edit::OnEditSelectAll (wxCommandEvent &WXUNUSED(event)) {
    SetSelection (0, GetTextLength ());
}

void Edit::OnEditSelectLine (wxCommandEvent &WXUNUSED(event)) {
    int lineStart = PositionFromLine (GetCurrentLine());
    int lineEnd = PositionFromLine (GetCurrentLine() + 1);
    SetSelection (lineStart, lineEnd);
}

void Edit::OnHighlightLang (wxCommandEvent &event) {
    InitializePrefs (g_LanguagePrefs [event.GetId() - myID_HIGHLIGHTFIRST].name);
}

void Edit::OnDisplayEOL (wxCommandEvent &WXUNUSED(event)) {
    SetViewEOL (!GetViewEOL());
}

void Edit::OnIndentGuide (wxCommandEvent &WXUNUSED(event)) {
    SetIndentationGuides (!GetIndentationGuides());
}

void Edit::OnLineNumber (wxCommandEvent &WXUNUSED(event)) {
    SetMarginWidth (m_LineNrID,
                    GetMarginWidth (m_LineNrID) == 0? m_LineNrMargin: 0);
}

void Edit::OnLongLineOn (wxCommandEvent &WXUNUSED(event)) {
    SetEdgeMode (GetEdgeMode() == 0? wxSTC_EDGE_LINE: wxSTC_EDGE_NONE);
}

void Edit::OnWhiteSpace (wxCommandEvent &WXUNUSED(event)) {
    SetViewWhiteSpace (GetViewWhiteSpace() == 0?
                       wxSTC_WS_VISIBLEALWAYS: wxSTC_WS_INVISIBLE);
}

void Edit::OnFoldToggle (wxCommandEvent &WXUNUSED(event)) {
    ToggleFold (GetFoldParent(GetCurrentLine()));
}

void Edit::OnSetOverType (wxCommandEvent &WXUNUSED(event)) {
    SetOvertype (!GetOvertype());
}

void Edit::OnSetReadOnly (wxCommandEvent &WXUNUSED(event)) {
    SetReadOnly (!GetReadOnly());
}

void Edit::OnWrapmodeOn (wxCommandEvent &WXUNUSED(event)) {
    SetWrapMode (GetWrapMode() == 0? wxSTC_WRAP_WORD: wxSTC_WRAP_NONE);
}

void Edit::OnUseCharset (wxCommandEvent &event) {
    int Nr;
    int charset = GetCodePage();
    switch (event.GetId()) {
        case myID_CHARSETANSI: {charset = wxSTC_CHARSET_ANSI; break;}
        case myID_CHARSETMAC: {charset = wxSTC_CHARSET_ANSI; break;}
    }
    for (Nr = 0; Nr < wxSTC_STYLE_LASTPREDEFINED; Nr++) {
        StyleSetCharacterSet (Nr, charset);
    }
    SetCodePage (charset);
}

void Edit::OnAnnotationAdd(wxCommandEvent& WXUNUSED(event))
{
    const int line = GetCurrentLine();

    wxString ann = AnnotationGetText(line);
    ann = wxGetTextFromUser
          (
            wxString::Format("Enter annotation for the line %d", line),
            "Edit annotation",
            ann,
            this
          );
    if ( ann.empty() )
        return;

    AnnotationSetText(line, ann);
    AnnotationSetStyle(line, ANNOTATION_STYLE);

    // Scintilla doesn't update the scroll width for annotations, even with
    // scroll width tracking on, so do it manually.
    const int width = GetScrollWidth();

    // NB: The following adjustments are only needed when using
    //     wxSTC_ANNOTATION_BOXED annotations style, but we apply them always
    //     in order to make things simpler and not have to redo the width
    //     calculations when the annotations visibility changes. In a real
    //     program you'd either just stick to a fixed annotations visibility or
    //     update the width when it changes.

    // Take into account the fact that the annotation is shown indented, with
    // the same indent as the line it's attached to.
    int indent = GetLineIndentation(line);

    // This is just a hack to account for the width of the box, there doesn't
    // seem to be any way to get it directly from Scintilla.
    indent += 3;

    const int widthAnn = TextWidth(ANNOTATION_STYLE, ann + wxString(indent, ' '));

    if (widthAnn > width)
        SetScrollWidth(widthAnn);
}

void Edit::OnAnnotationRemove(wxCommandEvent& WXUNUSED(event))
{
    AnnotationSetText(GetCurrentLine(), wxString());
}

void Edit::OnAnnotationClear(wxCommandEvent& WXUNUSED(event))
{
    AnnotationClearAll();
}

void Edit::OnAnnotationStyle(wxCommandEvent& event)
{
    int style = 0;
    switch (event.GetId()) {
        case myID_ANNOTATION_STYLE_HIDDEN:
            style = wxSTC_ANNOTATION_HIDDEN;
            break;

        case myID_ANNOTATION_STYLE_STANDARD:
            style = wxSTC_ANNOTATION_STANDARD;
            break;

        case myID_ANNOTATION_STYLE_BOXED:
            style = wxSTC_ANNOTATION_BOXED;
            break;
    }

    AnnotationSetVisible(style);
}

void Edit::OnChangeCase (wxCommandEvent &event) {
    switch (event.GetId()) {
        case myID_CHANGELOWER: {
            CmdKeyExecute (wxSTC_CMD_LOWERCASE);
            break;
        }
        case myID_CHANGEUPPER: {
            CmdKeyExecute (wxSTC_CMD_UPPERCASE);
            break;
        }
    }
}

void Edit::OnConvertEOL (wxCommandEvent &event) {
    int eolMode = GetEOLMode();
    switch (event.GetId()) {
        case myID_CONVERTCR: { eolMode = wxSTC_EOL_CR; break;}
        case myID_CONVERTCRLF: { eolMode = wxSTC_EOL_CRLF; break;}
        case myID_CONVERTLF: { eolMode = wxSTC_EOL_LF; break;}
    }
    ConvertEOLs (eolMode);
    SetEOLMode (eolMode);
}

void Edit::OnMultipleSelections(wxCommandEvent& WXUNUSED(event)) {
    bool isSet = GetMultipleSelection();
    SetMultipleSelection(!isSet);
}

void Edit::OnMultiPaste(wxCommandEvent& WXUNUSED(event)) {
    int pasteMode = GetMultiPaste();
    if (wxSTC_MULTIPASTE_EACH == pasteMode) {
        SetMultiPaste(wxSTC_MULTIPASTE_ONCE);
    }
    else {
        SetMultiPaste(wxSTC_MULTIPASTE_EACH);
    }
}

void Edit::OnMultipleSelectionsTyping(wxCommandEvent& WXUNUSED(event)) {
    bool isSet = GetAdditionalSelectionTyping();
    SetAdditionalSelectionTyping(!isSet);
}

void Edit::OnCustomPopup(wxCommandEvent& evt)
{
    UsePopUp(evt.IsChecked() ? wxSTC_POPUP_NEVER : wxSTC_POPUP_ALL);
}

//! misc
void Edit::OnMarginClick (wxStyledTextEvent &event) {
    if (event.GetMargin() == 2) {
        int lineClick = LineFromPosition (event.GetPosition());
        int levelClick = GetFoldLevel (lineClick);
        if ((levelClick & wxSTC_FOLDLEVELHEADERFLAG) > 0) {
            ToggleFold (lineClick);
        }
    }
}

bool Edit::LoadAutoComProvider(const wxString &filename)
{
    wxString opt = filename;
    if (NULL != m_language){
        opt = m_language->name;
    }
    mAllProviders.clear();
    return wxAutoCompProvider::GetValidProvider(opt, mAllProviders);
}

bool Edit::IsValidChar(char ch, const wxString &validCharList)
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

bool Edit::GetCandidate(const wxString &input, std::set<wxString> &candidate)
{
    int i = 0;
    wxString opt;
    
    if (NULL != m_language){
        opt = m_language->name;
    }
    
    for (i = 0; i < mAllProviders.size(); i++){
        mAllProviders[i]->GetCandidate(input, candidate, opt);
    }
    
    return true;
}

static bool IsWhiteSpace(char ch)
{
    if (ch == '\r' || ch == '\n' || ch == '\t' || ch == ' '){
        return true;
    }
    return false;
}

static bool IsNeedIncreaseIndentation(char ch){
    // todo:fanhongxuan@gmail.com
    // skip the value in the comment
    if (ch == '{' || ch == ':' || ch == '.'){
        return true;
    }
    return false;
}

static bool IsNeedDecreaseIndentation(char ch){
    if (ch == '}'){
        return true;
    }
    return false;
}

bool Edit::GetSymbolList(std::vector<wxString> &symbols, 
                        int iBase)
{
    int i = 0;
    /*for (i = 0; i < GetNumberOfLines(); i++){
        int foldstatus = GetFoldLevel(i);
        int foldlevel = foldstatus & wxSTC_FOLDLEVELNUMBERMASK - wxSTC_FOLDLEVELBASE;
        wxPrintf("--<%d><%d><%s>\n", i+1, foldlevel, GetLineText(i));
    }*/
    // use ctags to generate the symbol list of one file
    // ctags -x -Q --declarations wxAgSearch.cpp
    long cur = GetCurrentLine();
    GotoLine(GetNumberOfLines());
    ScrollToEnd();
    GotoLine(cur);
    for (i = 0; i < GetNumberOfLines(); i++){
        int foldstatus = GetFoldLevel(i);
        if (foldstatus & wxSTC_FOLDLEVELHEADERFLAG){
            int foldlevel = foldstatus & wxSTC_FOLDLEVELNUMBERMASK - wxSTC_FOLDLEVELBASE;
            if (foldlevel <= iBase){
                wxString text = GetLineText(i);
                if (text.find('{') != text.npos){
                    // must have a (), if no try the previous one
                    int start = text.find('('), stop = text.find(')');
                    if (start != text.npos && stop != text.npos && start < stop){
                        wxPrintf("<%d><%d><%s>\n", i+1, foldlevel, GetLineText(i));
                    }
                    else{
                        // should check the previous line
                    }
                }
                else{
                    // class construct which use : to define the param list, will be lost in this case.
                }
            }
        }
        else{
        }
    }
    return true;
}

void Edit::OnColon(int currentLine)
{
    // when user input a : in cpp mode
    // if the line only has the following cpp keyword, need to decrease the indent.
    // public, private, protected:
    if (NULL == m_language || wxString(m_language->name) != "C++"){
        return;
    }
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
    if (line == "public" || line == "private" || line == "protected" || line == "case"){
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

void Edit::OnBeginBrace(int currentLine, char c)
{
    if (NULL == m_language || wxString(m_language->name) != "C++"){
        // only handle this in C/C++
        return;
    }
    char end = '0';
    if (c == '{'){ end = '}';}
    if (c == '\''){end = '\'';}
    if (c == '\"'){end = '\"';}
    if (c == '('){end = ')';}
    if (c == '['){end = ']';}
    if (c == '<'){end = '>';}
    int pos = GetCurrentPos();
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

void Edit::OnEndBrace(int currentLine)
{
    if (NULL == m_language || wxString(m_language->name) != "C++"){
        // only handle this in C/C++
        return;
    }
    wxPrintf("OnEndBrace\n");
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

static char GetPrevNoneWhiteSpaceChar(Edit *pEdit, int pos)
{
    // todo:fanhongxuan@gmail.com
    // skip the comments
    char ret = 0;
    while(pos > 0){
        ret = pEdit->GetCharAt(pos);
        if (ret != ' ' && ret != '\r' && ret != '\t' && ret != '\n'){
            return ret;
        }
        pos--;
    }
    return 0;
}

static char GetNextNoneWhiteSpaceChar(Edit *pEdit, int pos)
{
    // todo:fanhongxuan@gmail.com
    // skip the comments
    char ret = 0;
    int size = pEdit->GetLastPosition();
    while(pos < size){
        ret = pEdit->GetCharAt(pos);
        if (ret != ' ' && ret != '\r' && ret != '\t' && ret != '\n'){
            return ret;
        }
        pos++;
    }
    return 0;    
}

// todo:fanhongxuan@gmail.com
// auto pair "", '', (), [], {},
// when delete auto delete.
// auto indent with endline
void Edit::OnReturn(int currentLine)
{
    // note:fanhongxuan@gmail.com
    if (NULL == m_language || wxString(m_language->name) != "C++"){
        // only handle this in C/C++
        return;
    }
    int pos = GetInsertionPoint();
    if (pos >= 2 && GetCharAt(pos - 2) == '{' && GetCharAt(pos) == '}'){
        // todo:fanhongxuan@gmail.com
        InsertText(pos, "\n    ");
    }
}

void Edit::OnCharAdded (wxStyledTextEvent &event) {
    // note:fanhongxuan@gmail.com
    // if the next is a validchar for autocomp, we don't show the autocomp
    // don't use mInputWord, we find the word backword start from current pos
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

    if (bNewCandidate){
        wxString opt;
        if (NULL != m_language){
            opt = m_language->name;
        }
        mInputWord = GetTextRange(start, pos);
        wxAutoCompWordInBufferProvider::Instance().AddCandidate(mInputWord, opt);
        char chr = (char)event.GetKey();
        int currentLine = GetCurrentLine();
        // Change this if support for mac files with \r is needed
        if (chr == '\n'){
            OnReturn(currentLine);
        }
        else if (chr == '{' || chr == '\'' || chr == '\"' || chr == '(' || chr == '['){
            OnBeginBrace(currentLine, chr);
        }
        else if (chr == '}'){
            OnEndBrace(currentLine);
        }
        else if (chr == ':'){
            OnColon(currentLine);
        }
        return;
    }
    
    mInputWord = GetTextRange(start, pos+1);
    std::set<wxString> candidate;
    GetCandidate(mInputWord, candidate);
    if (!candidate.empty()){
        wxString candidateStr;
        std::set<wxString>::iterator it = candidate.begin();
        while(it != candidate.end()){
            candidateStr += (*it);
            candidateStr += " ";
            it++;
        }
        AutoCompShow(mInputWord.length(), candidateStr);
    }
}


//----------------------------------------------------------------------------
// private functions
wxString Edit::DeterminePrefs (const wxString &filename) {

    LanguageInfo const* curInfo;

    // determine language from filepatterns
    int languageNr;
    for (languageNr = 0; languageNr < g_LanguagePrefsSize; languageNr++) {
        curInfo = &g_LanguagePrefs [languageNr];
        wxString filepattern = curInfo->filepattern;
        filepattern.Lower();
        while (!filepattern.empty()) {
            wxString cur = filepattern.BeforeFirst (';');
            if ((cur == filename) ||
                (cur == (filename.BeforeLast ('.') + wxT(".*"))) ||
                (cur == (wxT("*.") + filename.AfterLast ('.')))) {
                return curInfo->name;
            }
            filepattern = filepattern.AfterFirst (';');
        }
    }
    return wxEmptyString;

}

bool Edit::InitializePrefs (const wxString &name) {

    // initialize styles
    StyleClearAll();
    LanguageInfo const* curInfo = NULL;

    // determine language
    bool found = false;
    int languageNr;
    for (languageNr = 0; languageNr < g_LanguagePrefsSize; languageNr++) {
        curInfo = &g_LanguagePrefs [languageNr];
        if (curInfo->name == name) {
            found = true;
            break;
        }
    }
    if (!found) return false;

    // set lexer and language
    SetLexer (curInfo->lexer);
    m_language = curInfo;

    // set margin for line numbers
    SetMarginType (m_LineNrID, wxSTC_MARGIN_NUMBER);
    StyleSetForeground (wxSTC_STYLE_LINENUMBER, wxColour (wxT("DARK GREY")));
    StyleSetBackground (wxSTC_STYLE_LINENUMBER, *wxWHITE);
    // SetMarginWidth (m_LineNrID, 0); // start out not visible

#if 0    
    // annotations style
    StyleSetBackground(ANNOTATION_STYLE, wxColour(244, 220, 220));
    StyleSetForeground(ANNOTATION_STYLE, *wxBLACK);
    StyleSetSizeFractional(ANNOTATION_STYLE,
            (StyleGetSizeFractional(wxSTC_STYLE_DEFAULT)*4)/5);
#endif

    // default fonts for all styles!
    int Nr;
    for (Nr = 0; Nr < wxSTC_STYLE_LASTPREDEFINED; Nr++) {
        wxFont font(wxFontInfo(10).Family(wxFONTFAMILY_MODERN));
        StyleSetFont (Nr, font);
    }

    // set common styles
    StyleSetForeground (wxSTC_STYLE_DEFAULT, wxColour (wxT("BLACK")));
    StyleSetForeground (wxSTC_STYLE_INDENTGUIDE, wxColour (wxT("DARK GREY")));

    // initialize settings
    if (g_CommonPrefs.syntaxEnable) {
        int keywordnr = 0;
        for (Nr = 0; Nr < STYLE_TYPES_COUNT; Nr++) {
            if (curInfo->styles[Nr].type == -1) continue;
            const StyleInfo &curType = g_StylePrefs [curInfo->styles[Nr].type];
            wxFont font(wxFontInfo(curType.fontsize)
                            .Family(wxFONTFAMILY_MODERN)
                            .FaceName(curType.fontname));
            StyleSetFont (Nr, font);
            if (curType.foreground) {
                StyleSetForeground (Nr, wxColour (curType.foreground));
            }
            if (curType.background) {
                StyleSetBackground (Nr, wxColour (curType.background));
            }
            StyleSetBold (Nr, (curType.fontstyle & mySTC_STYLE_BOLD) > 0);
            StyleSetItalic (Nr, (curType.fontstyle & mySTC_STYLE_ITALIC) > 0);
            StyleSetUnderline (Nr, (curType.fontstyle & mySTC_STYLE_UNDERL) > 0);
            StyleSetVisible (Nr, (curType.fontstyle & mySTC_STYLE_HIDDEN) == 0);
            StyleSetCase (Nr, curType.lettercase);
            const char *pwords = curInfo->styles[Nr].words;
            if (pwords) {
                SetKeyWords (keywordnr, pwords);
                keywordnr += 1;
            }
        }
    }

    // set margin as unused
    SetMarginType (m_DividerID, wxSTC_MARGIN_SYMBOL);
    SetMarginWidth (m_DividerID, 0);
    SetMarginSensitive (m_DividerID, false);

    // folding
    SetMarginType (m_FoldingID, wxSTC_MARGIN_SYMBOL);
    SetMarginMask (m_FoldingID, wxSTC_MASK_FOLDERS);
    StyleSetBackground (m_FoldingID, *wxWHITE);
    SetMarginWidth (m_FoldingID, 0);
    SetMarginSensitive (m_FoldingID, false);
    if (g_CommonPrefs.foldEnable) {
        SetMarginWidth (m_FoldingID, curInfo->folds != 0? m_FoldingMargin: 0);
        SetMarginSensitive (m_FoldingID, curInfo->folds != 0);
        SetProperty (wxT("fold"), curInfo->folds != 0? wxT("1"): wxT("0"));
        SetProperty (wxT("fold.comment"),
                     (curInfo->folds & mySTC_FOLD_COMMENT) > 0? wxT("1"): wxT("0"));
        SetProperty (wxT("fold.compact"),
                     (curInfo->folds & mySTC_FOLD_COMPACT) > 0? wxT("1"): wxT("0"));
        SetProperty (wxT("fold.preprocessor"),
                     (curInfo->folds & mySTC_FOLD_PREPROC) > 0? wxT("1"): wxT("0"));
        SetProperty (wxT("fold.html"),
                     (curInfo->folds & mySTC_FOLD_HTML) > 0? wxT("1"): wxT("0"));
        SetProperty (wxT("fold.html.preprocessor"),
                     (curInfo->folds & mySTC_FOLD_HTMLPREP) > 0? wxT("1"): wxT("0"));
        SetProperty (wxT("fold.comment.python"),
                     (curInfo->folds & mySTC_FOLD_COMMENTPY) > 0? wxT("1"): wxT("0"));
        SetProperty (wxT("fold.quotes.python"),
                     (curInfo->folds & mySTC_FOLD_QUOTESPY) > 0? wxT("1"): wxT("0"));
    }
    SetFoldFlags (wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED |
                  wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);

    // set spaces and indentation
    SetTabWidth (4);
    SetUseTabs (false);
    SetTabIndents (true);
    SetBackSpaceUnIndents (true);
    SetIndent(g_CommonPrefs.indentEnable? 4: 0);

    // others
    SetViewEOL (g_CommonPrefs.displayEOLEnable);
    SetIndentationGuides (g_CommonPrefs.indentGuideEnable);
    SetEdgeColumn (120);
    SetEdgeMode (g_CommonPrefs.longLineOnEnable? wxSTC_EDGE_LINE: wxSTC_EDGE_NONE);
    SetViewWhiteSpace (g_CommonPrefs.whiteSpaceEnable?
                       wxSTC_WS_VISIBLEALWAYS: wxSTC_WS_INVISIBLE);
    SetOvertype (g_CommonPrefs.overTypeInitial);
    SetReadOnly (g_CommonPrefs.readOnlyInitial);
    SetWrapMode (g_CommonPrefs.wrapModeInitial?
                 wxSTC_WRAP_WORD: wxSTC_WRAP_NONE);
    
    LoadAutoComProvider(m_language->name);
    AutoCompSetMaxWidth(50);
    return true;
}

bool Edit::LoadFile ()
{
#if wxUSE_FILEDLG
    // get filename
    if (!m_filename) {
        wxFileDialog dlg (this, wxT("Open file"), wxEmptyString, wxEmptyString,
                          wxT("Any file (*)|*"), wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR);
        if (dlg.ShowModal() != wxID_OK) return false;
        m_filename = dlg.GetPath();
    }

    // load file
    return LoadFile (m_filename);
#else
    return false;
#endif // wxUSE_FILEDLG
}

void Edit::UpdateLineNumberMargin()
{
    int lineCount = GetLineCount();
    wxString linenumber = wxT("_");
    while(lineCount != 0){
        linenumber += wxT("9");
        lineCount = lineCount / 10;
    }
    SetMarginWidth(m_LineNrID, TextWidth (wxSTC_STYLE_LINENUMBER, linenumber));
}

bool Edit::LoadFile (const wxString &filename) {

    // load file in edit and clear undo
    if (!filename.empty()) m_filename = filename;
    
    wxStyledTextCtrl::LoadFile(m_filename);

    EmptyUndoBuffer();

    // determine lexer language
    wxFileName fname (m_filename);
    InitializePrefs (DeterminePrefs (fname.GetFullName()));

    wxString opt;
    if (NULL != m_language){
        opt = m_language->name;
    }
    wxAutoCompWordInBufferProvider::Instance().AddFileContent(GetText(), opt);
    return true;
}

bool Edit::NewFile(const wxString &defaultName)
{
    mDefaultName = defaultName;
    wxFileName fname (mDefaultName);
    InitializePrefs (DeterminePrefs (fname.GetFullName()));
    return true;
}

bool Edit::SaveFile (bool bClose)
{
#if wxUSE_FILEDLG
    // return if no change
    if (!Modified()) return true;

    // get filename
    if (!m_filename) {
        wxFileDialog dlg (this, wxT("Save file"), wxEmptyString, mDefaultName, wxT("Any file (*)|*"),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return false;
        m_filename = dlg.GetPath();
        mDefaultName = wxEmptyString;
        wxFileName fname (m_filename);
        InitializePrefs (DeterminePrefs (fname.GetFullName()));
    }

    // save file
    return SaveFile (m_filename, bClose);
#else
    return false;
#endif // wxUSE_FILEDLG
}

bool Edit::SaveFile (const wxString &filename, bool bClose) {
    // return if no change
    if (!Modified()) return true;

    wxString opt;
    if (NULL != m_language){
        opt = m_language->name;
    }
    bool ret = wxStyledTextCtrl::SaveFile(filename);
    if (!bClose){
        // todo:fanhongxuan@gmail.com
        // handle the pasted event, when save file, do not need to update this again.
        wxAutoCompWordInBufferProvider::Instance().AddFileContent(GetText(), opt);
    }
    return ret;
}

bool Edit::Modified () {
    // return modified state
    return (GetModify() && !GetReadOnly());
}

//----------------------------------------------------------------------------
// EditProperties
//----------------------------------------------------------------------------

EditProperties::EditProperties (Edit *edit,
                                long style)
        : wxDialog (edit, wxID_ANY, wxEmptyString,
                    wxDefaultPosition, wxDefaultSize,
                    style | wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

    // sets the application title
    SetTitle (_("Properties"));
    wxString text;

    // full name
    wxBoxSizer *fullname = new wxBoxSizer (wxHORIZONTAL);
    fullname->Add (10, 0);
    fullname->Add (new wxStaticText (this, wxID_ANY, _("Full filename"),
                                     wxDefaultPosition, wxSize(80, wxDefaultCoord)),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    fullname->Add (new wxStaticText (this, wxID_ANY, edit->GetFilename()),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);

    // text info
    wxGridSizer *textinfo = new wxGridSizer (4, 0, 2);
    textinfo->Add (new wxStaticText (this, wxID_ANY, _("Language"),
                                     wxDefaultPosition, wxSize(80, wxDefaultCoord)),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    textinfo->Add (new wxStaticText (this, wxID_ANY, edit->m_language->name),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
    textinfo->Add (new wxStaticText (this, wxID_ANY, _("Lexer-ID: "),
                                     wxDefaultPosition, wxSize(80, wxDefaultCoord)),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    text = wxString::Format (wxT("%d"), edit->GetLexer());
    textinfo->Add (new wxStaticText (this, wxID_ANY, text),
                   0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
    wxString EOLtype = wxEmptyString;
    switch (edit->GetEOLMode()) {
        case wxSTC_EOL_CR: {EOLtype = wxT("CR (Unix)"); break; }
        case wxSTC_EOL_CRLF: {EOLtype = wxT("CRLF (Windows)"); break; }
        case wxSTC_EOL_LF: {EOLtype = wxT("CR (Macintosh)"); break; }
    }
    textinfo->Add (new wxStaticText (this, wxID_ANY, _("Line endings"),
                                     wxDefaultPosition, wxSize(80, wxDefaultCoord)),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    textinfo->Add (new wxStaticText (this, wxID_ANY, EOLtype),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);

    // text info box
    wxStaticBoxSizer *textinfos = new wxStaticBoxSizer (
                     new wxStaticBox (this, wxID_ANY, _("Informations")),
                     wxVERTICAL);
    textinfos->Add (textinfo, 0, wxEXPAND);
    textinfos->Add (0, 6);

    // statistic
    wxGridSizer *statistic = new wxGridSizer (4, 0, 2);
    statistic->Add (new wxStaticText (this, wxID_ANY, _("Total lines"),
                                     wxDefaultPosition, wxSize(80, wxDefaultCoord)),
                    0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    text = wxString::Format (wxT("%d"), edit->GetLineCount());
    statistic->Add (new wxStaticText (this, wxID_ANY, text),
                    0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
    statistic->Add (new wxStaticText (this, wxID_ANY, _("Total chars"),
                                     wxDefaultPosition, wxSize(80, wxDefaultCoord)),
                    0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    text = wxString::Format (wxT("%d"), edit->GetTextLength());
    statistic->Add (new wxStaticText (this, wxID_ANY, text),
                    0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
    statistic->Add (new wxStaticText (this, wxID_ANY, _("Current line"),
                                     wxDefaultPosition, wxSize(80, wxDefaultCoord)),
                    0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    text = wxString::Format (wxT("%d"), edit->GetCurrentLine());
    statistic->Add (new wxStaticText (this, wxID_ANY, text),
                    0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
    statistic->Add (new wxStaticText (this, wxID_ANY, _("Current pos"),
                                     wxDefaultPosition, wxSize(80, wxDefaultCoord)),
                    0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    text = wxString::Format (wxT("%d"), edit->GetCurrentPos());
    statistic->Add (new wxStaticText (this, wxID_ANY, text),
                    0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);

    // char/line statistics
    wxStaticBoxSizer *statistics = new wxStaticBoxSizer (
                     new wxStaticBox (this, wxID_ANY, _("Statistics")),
                     wxVERTICAL);
    statistics->Add (statistic, 0, wxEXPAND);
    statistics->Add (0, 6);

    // total pane
    wxBoxSizer *totalpane = new wxBoxSizer (wxVERTICAL);
    totalpane->Add (fullname, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    totalpane->Add (0, 6);
    totalpane->Add (textinfos, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    totalpane->Add (0, 10);
    totalpane->Add (statistics, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    totalpane->Add (0, 6);
    wxButton *okButton = new wxButton (this, wxID_OK, _("OK"));
    okButton->SetDefault();
    totalpane->Add (okButton, 0, wxALIGN_CENTER | wxALL, 10);

    SetSizerAndFit (totalpane);

    ShowModal();
}

#if wxUSE_PRINTING_ARCHITECTURE

//----------------------------------------------------------------------------
// EditPrint
//----------------------------------------------------------------------------

EditPrint::EditPrint (Edit *edit, const wxChar *title)
              : wxPrintout(title)
              , m_edit(edit)
{
}

bool EditPrint::OnPrintPage (int page) {

    wxDC *dc = GetDC();
    if (!dc) return false;

    // scale DC
    PrintScaling (dc);

    // print page
    m_edit->FormatRange(true, page == 1 ? 0 : m_pageEnds[page-2], m_pageEnds[page-1],
                                     dc, dc, m_printRect, m_pageRect);
    return true;
}

bool EditPrint::OnBeginDocument (int startPage, int endPage) {

    if (!wxPrintout::OnBeginDocument (startPage, endPage)) {
        return false;
    }

    return true;
}

void EditPrint::GetPageInfo (int *minPage, int *maxPage, int *selPageFrom, int *selPageTo) {

    // // initialize values
    // *minPage = 0;
    // *maxPage = 0;
    // *selPageFrom = 0;
    // *selPageTo = 0;

    // // scale DC if possible
    // wxDC *dc = GetDC();
    // if (!dc) return;
    // PrintScaling (dc);

    // // get print page informations and convert to printer pixels
    // wxSize ppiScr;
    // GetPPIScreen (&ppiScr.x, &ppiScr.y);
    // wxSize page = g_pageSetupData->GetPaperSize();
    // page.x = static_cast<int> (page.x * ppiScr.x / 25.4);
    // page.y = static_cast<int> (page.y * ppiScr.y / 25.4);
    // // In landscape mode we need to swap the width and height
    // if ( g_pageSetupData->GetPrintData().GetOrientation() == wxLANDSCAPE )
    // {
    //     wxSwap(page.x, page.y);
    // }

    // m_pageRect = wxRect (0,
    //                      0,
    //                      page.x,
    //                      page.y);

    // // get margins informations and convert to printer pixels
    // wxPoint pt = g_pageSetupData->GetMarginTopLeft();
    // int left = pt.x;
    // int top = pt.y;
    // pt = g_pageSetupData->GetMarginBottomRight();
    // int right = pt.x;
    // int bottom = pt.y;

    // top = static_cast<int> (top * ppiScr.y / 25.4);
    // bottom = static_cast<int> (bottom * ppiScr.y / 25.4);
    // left = static_cast<int> (left * ppiScr.x / 25.4);
    // right = static_cast<int> (right * ppiScr.x / 25.4);

    // m_printRect = wxRect (left,
    //                       top,
    //                       page.x - (left + right),
    //                       page.y - (top + bottom));

    // // count pages
    // m_pageEnds.Clear();
    // int printed = 0;
    // while ( printed < m_edit->GetLength() ) {
    //     printed = m_edit->FormatRange(false, printed, m_edit->GetLength(),
    //                                   dc, dc, m_printRect, m_pageRect);
    //     m_pageEnds.Add(printed);
    //     *maxPage += 1;
    // }
    // if (*maxPage > 0) *minPage = 1;
    // *selPageFrom = *minPage;
    // *selPageTo = *maxPage;
}

bool EditPrint::HasPage (int page)
{
    return page <= (int)m_pageEnds.Count();
}

bool EditPrint::PrintScaling (wxDC *dc){

    // check for dc, return if none
    if (!dc) return false;

    // get printer and screen sizing values
    wxSize ppiScr;
    GetPPIScreen (&ppiScr.x, &ppiScr.y);
    if (ppiScr.x == 0) { // most possible guess 96 dpi
        ppiScr.x = 96;
        ppiScr.y = 96;
    }
    wxSize ppiPrt;
    GetPPIPrinter (&ppiPrt.x, &ppiPrt.y);
    if (ppiPrt.x == 0) { // scaling factor to 1
        ppiPrt.x = ppiScr.x;
        ppiPrt.y = ppiScr.y;
    }
    wxSize dcSize = dc->GetSize();
    wxSize pageSize;
    GetPageSizePixels (&pageSize.x, &pageSize.y);

    // set user scale
    float scale_x = (float)(ppiPrt.x * dcSize.x) /
                    (float)(ppiScr.x * pageSize.x);
    float scale_y = (float)(ppiPrt.y * dcSize.y) /
                    (float)(ppiScr.y * pageSize.y);
    dc->SetUserScale (scale_x, scale_y);

    return true;
}

#endif // wxUSE_PRINTING_ARCHITECTURE
