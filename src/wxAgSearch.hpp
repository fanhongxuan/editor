#include "wxSearch.hpp"
#include <set>
// ALT+F to trigger this mini-frame
// If the workspace is active search in workspace, otherwise, search in current dir. 
class wxAgSearch: public wxSearchFile
{
public:
    wxAgSearch(wxWindow *pSearch);
    ~wxAgSearch();
    virtual bool StartSearch(const wxString &input);
    virtual bool StopSearch();
    
    bool SetSearchDirs(const std::set<wxString> &input);
    bool SetSearchFiles(const std::set<wxString> &input);
    virtual int GetPreferedLine(const wxString &input){return -1;}
    bool OnResult(const wxString &cmd, const wxString &result);
private:
    std::set<wxString> mCmds;
    std::set<wxString> mTargetDirs;
    std::set<wxString> mTargetFiles;
};