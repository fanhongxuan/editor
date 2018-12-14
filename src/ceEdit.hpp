#ifndef _CE_EDIT_HPP_
#define _CE_EDIT_HPP_

#include <wx/stc/stc.h>
#include <set>
#include <map>

class ceSymbol;    
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
    void OnUpdateUI(wxStyledTextEvent &evt);    
        
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
    wxString GetPrevValue(int stopPos, int style, int *pStop = NULL);
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
    int HandleFunctionStart(int pos, int curStyle);
    int HandleFunctionBody(int pos, int curStyle);
    int HandleVariable(int pos, int curStyle);
    bool HandleFolder(long pos);
    void UpdateLineNumberMargin();
    wxFontInfo GetFontByStyle(int style, int type);
    wxColor GetColourByStyle(int style, int type);
    wxString GuessLanguage(const wxString &language);
    bool GetMatchRange(long pos, long &start, long &stop, char sc, char ec);
    void DoBraceMatch();
    bool LoadAutoComProvider(const wxString &filename);
    bool IsValidChar(char ch, const wxString &validCharList = wxEmptyString);
    bool GetCandidate(const wxString &input, std::set<wxString> &candidates);
    wxString WhichFunction(int pos, long *pStart = NULL, long *pStop = NULL);
    bool ClearLoalSymbol();
    bool BuildLocalSymbl();
    int PrepareFunctionParams(int pos);
    int HandleClass(int pos, int curStyle);
    int HandleParam(int startPos, int stopPos);
    bool IsValidParam(int startPos, int stopPos);
    bool IsValidVariable(int startPos, int stopPos, bool onlyHasName, int *pStart = NULL, int *pLen = NULL);    
    bool IsFunctionDeclare(int pos);
    bool IsFunctionDefination(int pos);
    wxString FindType(const wxString &value, int line = -1, int pos = -1);
    std::vector<wxAutoCompProvider *> mAllProviders;
        bool IsVariableValid(const wxString &variable, int pos);
        
private:
    std::map<wxString, std::set<ceSymbol *>* > mSymbolMap;
    std::set<ceSymbol *> mLocalSymbolMap;    
    std::set<wxString> mLocalTypes;
    std::map<wxString, int> mLocalVariable; // key is the name, second is the start pos
    std::map<wxString, int> mFunctionParames; // key is the name, seconds is the start pos
    // std::map<int, wxString> mVariables; // key is the pos, becase multi variable may have the same, name.
    std::map<wxString, std::vector<int> > mVariables; // 
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

