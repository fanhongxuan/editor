#include "wxSearch.hpp"
#include <set>
// ALT+F to trigger this mini-frame
// If the workspace is active search in workspace, otherwise, search in current dir. 
class wxAgSearch: public wxSearchFile
{
public:
    wxAgSearch(wxWindow *pSearch);
    ~wxAgSearch();
    virtual bool BeforeResultMatch(const wxString &input, wxSearchResult *pRet);
    virtual bool AfterResultMatch(const wxString &input, wxSearchResult *pRet);
    
    virtual bool BeginMatch(const wxString &input);
    virtual bool FinishMatch(const wxString &input);
    
    virtual bool StartSearch(const wxString &input, const wxString &fullInput);
    virtual bool StopSearch();
    
    bool SetSearchDirs(const std::set<wxString> &input);
    bool SetSearchFiles(const std::set<wxString> &input);
    virtual int GetPreferedLine(const wxString &input){return -1;}
    virtual wxString GetShortHelp() const;
    virtual wxString GetSummary(const wxString &input, int matchCount);
    bool OnResult(const wxString &cmd, const wxString &result);
private:
    wxSearchFileResult *mpPrevResult;
    int mItemCount;
    int mFileCount;
    int mTotalCount;
    std::set<wxString> mCmds;
    std::set<wxString> mTargetDirs;
    std::set<wxString> mTargetFiles;
};
