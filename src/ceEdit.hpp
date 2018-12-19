#ifndef _CE_EDIT_HPP_
#define _CE_EDIT_HPP_

#include <wx/stc/stc.h>
#include <set>
#include <map>

class ceSymbol;
    class wxTimer;
    class wxTimerEvent;
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
    int FindStyleStart(int style, int curPos, bool bSkipNewline = false);
    wxString GetParamType(int pos);
    wxString GetVariableType(int pos);
        wxString QueryVariableType(const wxString &name);
        wxString QueryFunctionParamType(const wxString &name);
        void OnStyleNeeded(wxStyledTextEvent &evt);
    void OnModified(wxStyledTextEvent &evt);
    void OnMarginClick(wxStyledTextEvent &evt);
    void OnAutoCompSelection(wxStyledTextEvent &evt);
    void OnCallTipClick(wxStyledTextEvent &evt);
    void OnDwellStart(wxStyledTextEvent &evt);
    void OnDwellEnd(wxStyledTextEvent &evt);
    void OnSize(wxSizeEvent& event);
    void OnTimer(wxTimerEvent &event);
        
    void OnCharAdded(wxStyledTextEvent &event);
    
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
    int GetCurrentMode();
    bool EndCallTip();
    bool UpdateCallTip(int direction);
    wxString GetClassName(int pos);
    bool OnUpdateFunctionParam(wxKeyEvent &evt);
    bool HighLightNextParam(const wxString &desc);
    bool ShowFunctionParam(int pos, const wxString &desc);
    bool ShowCallTipsOfLocalVariableOrParam(int start, int stop);
    bool ShowCallTips(int pos);
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
    // bool IsKeyWord1(const wxString &value, const wxString &language);
    // bool IsKeyWord2(const wxString &value, const wxString &language);
    bool IsNumber(const wxString &value);
    bool LoadStyleByFileName(const wxString &filename);
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
    bool GetCandidate(const wxString &input, std::set<wxString> &candidates, int mode);
    wxString WhichFunction(int pos, long *pStart = NULL, long *pStop = NULL);
        wxString WhichClass(int pos);
        wxString WhichType(int pos);
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
        int SetClass(int pos);
private:
    std::map<wxString, std::set<ceSymbol *>* > mSymbolMap;
    std::set<ceSymbol *> mLocalSymbolMap;    
    std::set<wxString> mLocalTypes;
    std::map<wxString, std::pair<int, wxString> > mLocalVariable; // key is the name, second is the start pos
    std::map<wxString, std::pair<int, wxString> > mFunctionParames; // key is the name, seconds is the start pos
    // std::map<int, wxString> mVariables; // key is the pos, becase multi variable may have the same, name.
    std::map<wxString, std::vector<int> > mVariables; // 
        wxTimer *mpTimer;
    wxString mFilename;
    wxString mActiveFunctionParam;
    int mActiveFunctionParamStart;
    std::vector<ceSymbol *> mActiveCallTips;
    int mActiveCallTipsIndex;
    int mActiveCallTipsPosition;
    int mLocalCallTipLine;
    wxString mDefaultName;
    wxString mLanguage;
    int mLinenuMargin;
    int mDeviderMargin;
    int mFoldingMargin;
    bool mbLoadFinish;
    bool mbReplace;
        bool mbHasFocus;
    wxDECLARE_EVENT_TABLE();
};
#endif

