#ifndef _CE_EDIT_HPP_
#define _CE_EDIT_HPP_

#include <wx/stc/stc.h>
#include <set>

class wxAutoCompProvider;
class ceEdit: public wxStyledTextCtrl
{
public:
    ceEdit(wxWindow *parent);
    
    bool NewFile(const wxString &defaultName);
    bool LoadFile ();
    bool LoadFile (const wxString &filename);
    bool SaveFile (bool bClose = false);
    bool SaveFile (const wxString &filename, bool bClose = false);
    bool Modified ();
    wxString GetFilename ();
    void SetFilename (const wxString &filename);
    wxString GetCurrentWord(const wxString &validCharList = wxEmptyString);
    
    void OnStyleNeeded(wxStyledTextEvent &evt);
    void OnModified(wxStyledTextEvent &evt);
    void OnMarginClick(wxStyledTextEvent &evt);
    void OnSize(wxSizeEvent& event);
    
    void OnCharAdded  (wxStyledTextEvent &event);
    
    void OnMouseLeftDown(wxMouseEvent &evt);
    void OnMouseLeftUp(wxMouseEvent &evt);
    void OnMouseLeftDclick(wxMouseEvent &evt);
    void OnMouseWheel(wxMouseEvent &evt);
    void OnKillFocus(wxFocusEvent &evt);
    void OnFocus(wxFocusEvent &evt);
    void OnKeyDown(wxKeyEvent &event);
    void OnKeyUp(wxKeyEvent &event);
    
protected:
    bool ShowCallTips();
    bool InsertNewLine(long pos);
    long GetLineStartPosition(long line);
    bool StartReplaceInRegion();
    bool TriggerCommentRange(long start, long stop);
    bool HungerBack();
    void AutoIndentWithNewline(int currentLine);
    bool AutoIndentWithTab(int currentLine);
    void MoveCharBeforeRightParentheses(int currentLine);
    void OnColon(int currentLine);
    void OnEndBrace(int currentLine);
    void InsertPair(int currentLine, char c = '{');
    int CalcLineIndentByFoldLevel(int line, int level);
    
    bool IsInPreproces(int stopPos);
    bool IsKeyWord1(const wxString &value, const wxString &language);
    bool IsKeyWord2(const wxString &value, const wxString &language);
    bool IsNumber(const wxString &value);
    bool LoadStyleByFileName(const wxString &filename);
    int FindStyleStart(int style, int curPos, bool bSkipNewline = false);
    int ParseChar(int curStyle, long pos);
    int ParseCharInDefault(char c, int curStyle, long pos);
    bool ParseWord(int pos);
    int GetFoldLevelDelta(int line);
    bool HandleFolder(long pos);
    void UpdateLineNumberMargin();
    wxColor GetColourByStyle(int style, int type);
    wxString GuessLanguage(const wxString &language);
    bool GetMatchRange(long &start, long &stop, char sc, char ec);
    void DoBraceMatch();
    bool LoadAutoComProvider(const wxString &filename);
    bool IsValidChar(char ch, const wxString &validCharList = wxEmptyString);
    bool GetCandidate(const wxString &input, std::set<wxString> &candidates);
    std::vector<wxAutoCompProvider *> mAllProviders;
    
private:
    wxString mFilename;
    wxString mDefaultName;
    wxString mLanguage;
    int mLinenuMargin;
    int mDeviderMargin;
    int mFoldingMargin;
    bool mbLoadFinish;
    bool mbReplace;
    wxDECLARE_EVENT_TABLE();
};
#endif

