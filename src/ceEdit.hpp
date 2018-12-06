#ifndef _CE_EDIT_HPP_
#define _CE_EDIT_HPP_

#include <wx/stc/stc.h>

class ceParseState;
class ceEdit: public wxStyledTextCtrl
{
public:
    ceEdit(wxWindow *parent);
    bool LoadFile(const wxString &filename);
    bool NewFile(const wxString &filename);
    wxString GetFilename();

    void OnStyleNeeded(wxStyledTextEvent &evt);
    void OnModified(wxStyledTextEvent &evt);
    void OnSize(wxSizeEvent& event);
protected:
    bool IsInPreproces(int stopPos);
    bool IsKeyWord1(const wxString &value);
    bool IsNumber(const wxString &value);
    bool LoadStyleByFileName(const wxString &filename);
    int ParseChar(int curStyle, long pos);
    bool ParseWord(int pos);
    void UpdateLineNumberMargin();
    wxColor GetColourByStyle(int style, int type);
private:
    wxString mFilename;
    int mLinenuMargin;
    int mDeviderMargin;
    int mFoldingMargin;
    bool mbLoadFinish;

    ceParseState *mpParseState;
    wxDECLARE_EVENT_TABLE();
};
#endif

