#include "wxBufferSelect.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf

// todo:fanhongxuan@gmail.com
// 1, display the recent file list
// 2, display more info about the file, for example, if this file is modified?


wxBufferSelect::wxBufferSelect(wxWindow *pParent)
    :wxSearch(pParent),mMaxRecentFileCounts(100)
{
    SetMinStartLen(0);
}

wxBufferSelect::~wxBufferSelect()
{
    
}

bool wxBufferSelect::StartSearch(const wxString &input)
{
    std::map<wxString, wxString>::iterator it = mBufferList.begin();
    while(it != mBufferList.end()){
        AddSearchResult(new wxSearchResult(it->second, it->first));
        it++;
    }
    return true;
}

bool wxBufferSelect::StopSearch()
{
    return true;
}

bool wxBufferSelect::AddBuffer(const wxString &name, const wxString &path)
{
    // wxPrintf("AddBuffer:<%s><%s>\n", name, path);
    mBufferList[path] = name;
    return true;
}

bool wxBufferSelect::DelBuffer(const wxString &name, const wxString &path)
{
    // remove from the mBufferList, add to mRecentFile
    std::map<wxString, wxString>::iterator it = mBufferList.find(path);
    if (it != mBufferList.end()){
        mBufferList.erase(it);
    }
    // todo:fanhongxuan@gmail.com
    // if mRecentFile size is bigger than mMaxRecentFile, replease one
    mRecentFile[path] = name;
    return true;
}

bool wxBufferSelect::SetMaxRecentFileCounts(int max)
{
    mMaxRecentFileCounts = max;
    return true;
}
