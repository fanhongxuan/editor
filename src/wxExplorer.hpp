#ifndef __WX_EXPLORER_HPP__
#define __WX_EXPLORER_HPP__

/**
 * A explorer is a tree control which can used to explorer the file system
 * user use this to change the current working dir of CE.
 */
#include <wx/treectrl.h>

class wxExplorer: public wxTreeCtrl
{
public:
    wxExplorer(wxWindow *parent);
    wxString GetCwd(); // Get the Cwd, should return the same result as getcwd

    void OnItemActivated(wxTreeEvent &evt);
    void OnSelectionChanged(wxTreeEvent &evt);
    virtual int OnCompareItems(const wxTreeItemId &first,
                               const wxTreeItemId &second);
private:
    void CreateImageList();

    wxDECLARE_EVENT_TABLE();
};
#endif

