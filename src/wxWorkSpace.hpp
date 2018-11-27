#ifndef __WX_WORKSPACE_HPP__
#define __WX_WORKSPACE_HPP__
#include <wx/treectrl.h>

#include <set>

class wxWorkSpace: public wxTreeCtrl
{
    wxDECLARE_CLASS(wxExplorer);
public:
    wxWorkSpace(wxWindow *parent);
    ~wxWorkSpace();
    
    bool GetDirs(std::set<wxString> &dirs) const {dirs = mDirs; return true;}
    bool GetFiles(std::set<wxString> &files) const {files = mFiles; return true;}
    
    bool AddDirToWorkSpace(const wxString &dir);
    
    void OnAddDirToWorkSpace(wxCommandEvent &evt);
    void OnDelDirFromWorkSpace(wxCommandEvent &evt);
    void OnGenerateTag(wxCommandEvent &evt);
    
    void OnItemActivated(wxTreeEvent &evt);
    void OnSelectionChanged(wxTreeEvent &evt);
    void OnItemExpanding(wxTreeEvent &evt);
    void OnKeyDown(wxKeyEvent &evt);
    void OnRightDown(wxMouseEvent &evt);
    void OnFocus(wxFocusEvent &evt);
    virtual int OnCompareItems(const wxTreeItemId &first,
                               const wxTreeItemId &second);
    wxString GetTagDir(){return mTagDir;}
    bool UpdateTagForFile(const wxString &file);
    bool GenerateTagFile();
private:
    wxString mTagDir;
    void CreateImageList();
    std::set<wxString> mDirs;
    std::set<wxString> mFiles;
    wxDECLARE_EVENT_TABLE();
};

#endif
