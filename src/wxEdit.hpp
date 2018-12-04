//////////////////////////////////////////////////////////////////////////////
// File:        edit.h
// Purpose:     STC test module
// Maintainer:  Wyo
// Created:     2003-09-01
// Copyright:   (c) wxGuide
// Licence:     wxWindows licence
//////////////////////////////////////////////////////////////////////////////

#ifndef _EDIT_H_
#define _EDIT_H_

//----------------------------------------------------------------------------
// information
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// headers
//----------------------------------------------------------------------------

//! wxWidgets headers

//! wxWidgets/contrib headers
#include "wx/stc/stc.h"  // styled text control

//! application headers
#include "wxDefsext.hpp"
#include "wxPrefs.hpp"       // preferences
#include <set>

//============================================================================
// declarations
//============================================================================
class wxAutoCompProvider;

class EditPrint;
class EditProperties;


//----------------------------------------------------------------------------
//! Edit
class Edit: public wxStyledTextCtrl {
    friend class EditProperties;
    friend class EditPrint;
        
public:
    //! constructor
    Edit (wxWindow *parent, wxWindowID id = wxID_ANY,
          const wxPoint &pos = wxDefaultPosition,
          const wxSize &size = wxDefaultSize,
          long style =
#ifndef __WXMAC__
          // wxSUNKEN_BORDER|
#endif
          wxHSCROLL
         );

    //! destructor
    ~Edit ();

    wxString GetCurrentWord(const wxString &validCharList = wxEmptyString);
    void UpdateLineNumberMargin();

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
    bool GetSymbolList(std::vector<wxString> &symbol, int iBase = 0);
    
    // event handlers
    // common
    void OnSize( wxSizeEvent &event );
    void OnModified(wxStyledTextEvent &evt);
    // edit
    void OnEditRedo (wxCommandEvent &event);
    void OnEditUndo (wxCommandEvent &event);
    void OnEditClear (wxCommandEvent &event);
    void OnEditCut (wxCommandEvent &event);
    void OnEditCopy (wxCommandEvent &event);
    void OnEditPaste (wxCommandEvent &event);
    // find
    void OnFind (wxCommandEvent &event);
    void OnFindNext (wxCommandEvent &event);
    void OnReplace (wxCommandEvent &event);
    void OnReplaceNext (wxCommandEvent &event);
    void OnBraceMatch (wxCommandEvent &event);
    void OnGoto (wxCommandEvent &event);
    void OnEditIndentInc (wxCommandEvent &event);
    void OnEditIndentRed (wxCommandEvent &event);
    void OnEditSelectAll (wxCommandEvent &event);
    void OnEditSelectLine (wxCommandEvent &event);
    //! view
    void OnHighlightLang (wxCommandEvent &event);
    void OnDisplayEOL (wxCommandEvent &event);
    void OnIndentGuide (wxCommandEvent &event);
    void OnLineNumber (wxCommandEvent &event);
    void OnLongLineOn (wxCommandEvent &event);
    void OnWhiteSpace (wxCommandEvent &event);
    void OnFoldToggle (wxCommandEvent &event);
    void OnSetOverType (wxCommandEvent &event);
    void OnSetReadOnly (wxCommandEvent &event);
    void OnWrapmodeOn (wxCommandEvent &event);
    void OnUseCharset (wxCommandEvent &event);
    // annotations
    void OnAnnotationAdd(wxCommandEvent& event);
    void OnAnnotationRemove(wxCommandEvent& event);
    void OnAnnotationClear(wxCommandEvent& event);
    void OnAnnotationStyle(wxCommandEvent& event);
    //! extra
    void OnChangeCase (wxCommandEvent &event);
    void OnConvertEOL (wxCommandEvent &event);
    void OnMultipleSelections(wxCommandEvent& event);
    void OnMultiPaste(wxCommandEvent& event);
    void OnMultipleSelectionsTyping(wxCommandEvent& event);
    void OnCustomPopup(wxCommandEvent& evt);
    // stc
    void OnMarginClick (wxStyledTextEvent &event);
    void OnCharAdded  (wxStyledTextEvent &event);

    void OnMouseLeftDown(wxMouseEvent &evt);
    void OnMouseLeftUp(wxMouseEvent &evt);
    void OnMouseLeftDclick(wxMouseEvent &evt);
    void OnMouseWheel(wxMouseEvent &evt);
    void OnKillFocus(wxFocusEvent &evt);
    void OnFocus(wxFocusEvent &evt);
    void OnKeyDown(wxKeyEvent &event);
    void OnKeyUp(wxKeyEvent &event);
    
    //! language/lexer
    wxString DeterminePrefs (const wxString &filename);
    bool InitializePrefs (const wxString &filename);
    bool UserSettings (const wxString &filename);
    LanguageInfo const* GetLanguageInfo () {return m_language;};

    //! load/save file
    bool NewFile(const wxString &defaultName);
    bool LoadFile ();
    bool LoadFile (const wxString &filename);
    bool SaveFile (bool bClose = false);
    bool SaveFile (const wxString &filename, bool bClose = false);
    bool Modified ();
    wxString GetFilename () {return m_filename;};
    void SetFilename (const wxString &filename) {m_filename = filename;};

private:
    bool GetMatchRange(long &start, long &stop);
    void DoBraceMatch();
    bool LoadAutoComProvider(const wxString &filename);
    bool IsValidChar(char ch, const wxString &validCharList = wxEmptyString);
    bool GetCandidate(const wxString &input, std::set<wxString> &candidates);
    std::vector<wxAutoCompProvider *> mAllProviders;
private:
    wxString mInputWord;
    wxString mDefaultName;
    
    // file
    wxString m_filename;

    // language properties
    LanguageInfo const* m_language;

    bool mbLoadFinish;
    bool mbReplace;
    // margin variables
    int m_LineNrID;
    int m_LineNrMargin;
    int m_FoldingID;
    int m_FoldingMargin;
    int m_DividerID;

    wxDECLARE_EVENT_TABLE();
};

//----------------------------------------------------------------------------
//! EditProperties
class EditProperties: public wxDialog {

public:

    //! constructor
    EditProperties (Edit *edit, long style = 0);

private:

};

#if wxUSE_PRINTING_ARCHITECTURE

//----------------------------------------------------------------------------
//! EditPrint
class EditPrint: public wxPrintout {

public:

    //! constructor
    EditPrint (Edit *edit, const wxChar *title = wxT(""));

    //! event handlers
    bool OnPrintPage (int page) wxOVERRIDE;
    bool OnBeginDocument (int startPage, int endPage) wxOVERRIDE;

    //! print functions
    bool HasPage (int page) wxOVERRIDE;
    void GetPageInfo (int *minPage, int *maxPage, int *selPageFrom, int *selPageTo) wxOVERRIDE;

private:
    Edit *m_edit;
    wxArrayInt m_pageEnds;
    wxRect m_pageRect;
    wxRect m_printRect;

    bool PrintScaling (wxDC *dc);
};

#endif // wxUSE_PRINTING_ARCHITECTURE

#endif // _EDIT_H_
