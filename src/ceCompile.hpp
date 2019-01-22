#ifndef CE_COMPILE_HPP_
#define CE_COMPILE_HPP_
#include <wx/stc/stc.h>
#include <map>
#include <time.h>
class ceCompile: public wxStyledTextCtrl
{
public:
    ceCompile(wxWindow *parent);
    void Compile(const wxString &filename);
    void OnFocus(wxFocusEvent &evt);
    void OnResult(wxThreadEvent &evt);
    void OnStyleNeeded(wxStyledTextEvent &evt);
    void OnKeyDown(wxKeyEvent &evt);
    void OnLeftDown(wxMouseEvent &evt);
    wxString GetCompileCmd(const wxString &filename);
    
    void UpdateLineNumberMargin();
    bool IsError(int linenumber);
    bool IsWarn(int linenumer);
private:
    time_t mStartTime;
    wxString mWorkDir;
    std::map<wxString, wxString> mCompileCmds;
    wxDECLARE_EVENT_TABLE();
};
    
#endif /**/