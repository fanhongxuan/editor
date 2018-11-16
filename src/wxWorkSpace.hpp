#ifndef __WX_WORKSPACE_HPP__
#define __WX_WORKSPACE_HPP__
#include <wx/treectrl.h>

#include <vector>

class wxWorkSpace: public wxTreeCtrl
{
    wxDECLARE_CLASS(wxExplorer);
public:
    wxWorkSpace(wxWindow *parent);
    ~wxWorkSpace();
    
    bool GetDirs(std::vector<wxString> &dirs);

    bool AddDirToWorkSpace(const wxString &dir);
    
    void OnAddDirToWorkSpace(wxCommandEvent &evt);
    void OnDelDirFromWorkSpace(wxCommandEvent &evt);
    
    void OnItemActivated(wxTreeEvent &evt);
    void OnSelectionChanged(wxTreeEvent &evt);
    void OnItemExpanding(wxTreeEvent &evt);
    void OnKeyDown(wxKeyEvent &evt);
    void OnRightDown(wxMouseEvent &evt);
    virtual int OnCompareItems(const wxTreeItemId &first,
                               const wxTreeItemId &second);
                               
private:
    void CreateImageList();
    std::vector<wxString> mDirs;
    wxDECLARE_EVENT_TABLE();
};

#endif
