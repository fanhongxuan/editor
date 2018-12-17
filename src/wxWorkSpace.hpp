#ifndef __WX_WORKSPACE_HPP__
#define __WX_WORKSPACE_HPP__
#include <wx/treectrl.h>

#include <set>

class ceSymbol;
class ceSymbolDb;
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
    void OnItemCollapsed(wxTreeEvent &evt);
    void OnItemExpanding(wxTreeEvent &evt);
    void OnKeyDown(wxKeyEvent &evt);
    void OnRightDown(wxMouseEvent &evt);
    void OnFocus(wxFocusEvent &evt);
    virtual int OnCompareItems(const wxTreeItemId &first,
                               const wxTreeItemId &second);
    bool UpdateTagForFile(const wxString &file);
        bool GetSymbols(std::set<ceSymbol*> &symbols, 
                        const wxString &scope, 
                        const wxString &type = "", 
                        const wxString &language = "",
                        const wxString &filename = "");
        bool FindDef(std::set<ceSymbol *> &symbols, 
            const wxString &name,
            const wxString &className,
            const wxString &type = "", 
            const wxString &language = "",
            const wxString &filename = "");
            
private:
    bool GenerateTagFile();
    void GenerateGTagFile(const wxString &dir);
    
private:
    void CreateImageList();
    ceSymbolDb *mpSymbolDb;
    std::set<wxString> mDirs;
    std::set<wxString> mFiles;
    wxDECLARE_EVENT_TABLE();
};

#endif
