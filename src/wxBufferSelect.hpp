// used to show a buffer list
// and search it.
#ifndef __WX_BUFFER_SELECT_HPP_
#define __WX_BUFFER_SELECT_HPP_
#include <map>
#include "wxSearch.hpp"

class wxBufferSelect: public wxSearch
{
public:
    wxBufferSelect(wxWindow *pParent);
    virtual ~wxBufferSelect();
    virtual bool StartSearch(const wxString &input);
    virtual bool StopSearch();

    bool AddBuffer(const wxString &name, const wxString &path);
    bool DelBuffer(const wxString &name, const wxString &path);
    bool SetMaxRecentFileCounts(int max); //
private:
    int mMaxRecentFileCounts;
    std::map<wxString, wxString> mBufferList;
    std::map<wxString, wxString> mRecentFile;
};
#endif
