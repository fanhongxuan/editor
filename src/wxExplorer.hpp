#ifndef __WX_EXPLORER_HPP__
#define __WX_EXPLORER_HPP__

/**
 * A explorer is a tree control which can used to explorer the file system
 * user use this to change the current working dir of CE.
 */
#include <wx/treectrl.h>

class wxExplorer: public wxTreeCtrl
{
    wxDECLARE_CLASS(wxExplorer);
public:
    wxExplorer(wxWindow *parent);
    ~wxExplorer();
    wxString GetCwd(); // Get the Cwd, should return the same result as getcwd
    void SetCwd(const wxString &cwd);
    void OnItemActivated(wxTreeEvent &evt);
    void OnSelectionChanged(wxTreeEvent &evt);
    void OnItemCollapsed(wxTreeEvent &evt);
    void OnItemExpanding(wxTreeEvent &evt);
    void OnRightDown(wxMouseEvent &evt);
    void OnAddDirToWorkSpace(wxCommandEvent &evt);
    void OnKeyDown(wxKeyEvent &evt);
    void OnFocus(wxFocusEvent &evt);
    virtual int OnCompareItems(const wxTreeItemId &first,
                               const wxTreeItemId &second);
private:
    bool LoadDir(const wxString &dir);
    void CreateImageList();
    wxString mCwd;
        wxString mAddPath;
    
    wxDECLARE_EVENT_TABLE();
};
#endif

