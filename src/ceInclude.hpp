#ifndef _CE_INLCUDE_HPP_
#define _CE_INLCUDE_HPP_

#include <wx/string.h>
#include <set>
#include <map>
class ceInclude
{
public:
    static ceInclude &Instance();
    
    bool AddIncDir(const wxString &dir);
    bool GetIncludeFiles(std::set<wxString> &files, const wxString &filename);
    
    bool GetDuplicateFiles(std::set<wxString> &files);
    bool Reset();
private:
    bool UpdateFileList(const wxString &dir);
    bool IsSupportedFile(const wxString &file);
private:
    // todo:fanhongxuan@gmail.com
    // we need to monitor all the dirs in mDirs, if file has been modified, we need tobe
    // updated.
    std::set<wxString> mDirs;
    std::map<wxString, wxString> mFiles;
    std::set<wxString> mDupFiles;
private:
    ceInclude();
    virtual ~ceInclude();
    
private:
    ceInclude(const ceInclude &other);
    ceInclude &operator=(const ceInclude &other);
    static ceInclude *mpInstance;
};

#endif

