#include "wxExplorer.hpp"
#include <wx/dir.h>
#include <wx/icon.h>
#include <wx/imaglist.h>
#include <wx/filename.h>
#include <wx/artprov.h>
#include <wx/wxcrtvararg.h> // for wxPrintf

#include <wx/config.h>
#include "ce.hpp"

enum {
    explorer_harddisk,
    explorer_file,
    explorer_file_select,
    explorer_folder,
    explorer_folder_select,
    explorer_icon_count,
};

class wxExplorerItemInfo: public wxTreeItemData
{
public:
    wxExplorerItemInfo(const wxString &path, bool bFile):mPath(path), mbFile(bFile){}
    wxString mPath;
    bool mbFile;
};

static void AddDir(const wxString &dir, wxExplorer &explorer, wxTreeItemId parent, const wxString &target)
{
    
    wxDir cwd(dir);
    if (!cwd.IsOpened()){
        return;
    }
    
    wxString filename;
    bool cont = false;

    // 1, add all the dirs
    cont = cwd.GetFirst(&filename, "*", wxDIR_DIRS|wxDIR_HIDDEN);
    while(cont){
        wxString child_dir = cwd.GetNameWithSep() + filename;
        wxExplorerItemInfo *pInfo =
            new wxExplorerItemInfo(child_dir, false);
        wxTreeItemId child = explorer.AppendItem(parent,
                                                 filename,
                                                 explorer_folder,
                                                 explorer_folder_select,
                                                 pInfo);
        // check if need to follow
        if (target.find(child_dir) == 0){
            AddDir(child_dir, explorer, child, target);
        }
        cont = cwd.GetNext(&filename);
    }
    
    // add all the files
    cont = cwd.GetFirst(&filename, "*", wxDIR_FILES|wxDIR_HIDDEN);
    while(cont){
        wxExplorerItemInfo *pInfo =
            new wxExplorerItemInfo(cwd.GetNameWithSep() + filename, true);
        explorer.AppendItem(parent,
                            filename,
                            explorer_file,
                            -1,
                            pInfo);
        cont = cwd.GetNext(&filename);
    }
    // note:fanhongxuan@gmail.com
    // directory first, then file.
    // sort all the children.
    explorer.SortChildren(parent);
    if (dir == target){
        explorer.SelectItem(parent);
        explorer.EnsureVisible(parent);
    }
}

// todo:fanhongxuan@gmail.com
// handle the active event to open related dir
// right key to select the parent and collapse it
// left key to expand the select one
wxBEGIN_EVENT_TABLE(wxExplorer, wxTreeCtrl)
EVT_TREE_ITEM_ACTIVATED(wxID_ANY, wxExplorer::OnItemActivated)
EVT_TREE_SEL_CHANGED(wxID_ANY, wxExplorer::OnSelectionChanged)
wxEND_EVENT_TABLE()

wxExplorer::wxExplorer(wxWindow *parent)
    :wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTR_HAS_BUTTONS |
                wxTR_EDIT_LABELS |
#ifndef WIN32                
                wxTR_HIDE_ROOT |  /*On linux all the file system is mounted at the root, so we hide it.*/
#endif                
                wxTR_LINES_AT_ROOT |
                wxTR_FULL_ROW_HIGHLIGHT)
{
    CreateImageList();
    // iterator the file system only add the path to the current dir.
    wxConfig config("CE");
    wxString cwd;
    config.Read("/Config/CurrentWorkingDirectory", &cwd);
    if (!cwd.empty()){
        // if the Stored Working Directory is exist, switch to, otherwize use current
        wxDir dir(cwd);
        if (!dir.IsOpened()){
            cwd = wxGetCwd();
        }
    }
    else{
        cwd = wxGetCwd();
    }
    wxString volume;
    wxFileName::SplitPath(cwd, &volume, NULL, NULL, NULL);
    if (volume.empty()){
        volume = "/";
    }
    else{
        volume += ":/";
    }
    wxTreeItemId root = AddRoot(volume, explorer_harddisk);
    AddDir(volume, *this, root, cwd);
}

wxExplorer::~wxExplorer()
{
    wxConfig config("CE");
    config.Write("/Config/CurrentWorkingDirectory", wxGetCwd());
}

void wxExplorer::OnItemActivated(wxTreeEvent &evt)
{
    wxTreeItemId id = evt.GetItem();
    wxExplorerItemInfo *pItem = dynamic_cast<wxExplorerItemInfo*>(GetItemData(id));
    if (NULL == pItem){
        return;
    }
    // wxPrintf("Active:%s\n", pItem->mPath);
    // this is a folder, but has no children, try to load it.
    if ((!pItem->mbFile) && (0 == GetChildrenCount(id))){
        AddDir(pItem->mPath, *this, id, "");
        Expand(id);
    }
    else{
        wxGetApp().frame().OpenFile(pItem->mPath, pItem->mPath, true);
    }
}

void wxExplorer::OnSelectionChanged(wxTreeEvent &evt)
{
    wxTreeItemId id = evt.GetItem();
    wxExplorerItemInfo *pItem = dynamic_cast<wxExplorerItemInfo*>(GetItemData(id));
    if (NULL == pItem){
        return;
    }
    wxString path = pItem->mPath;
    if (pItem->mbFile){
        wxFileName::SplitPath(pItem->mPath, NULL, &path, NULL, NULL);
    }
    // wxPrintf("SetWorkingDirectory:%s\n", path);
    wxSetWorkingDirectory(path);
}

void wxExplorer::CreateImageList()
{
    wxIcon icons[explorer_icon_count];
    int size = 16;
    wxSize iconSize(size, size);
    wxImageList *images = new wxImageList(size, size, true);
    icons[explorer_harddisk] = wxArtProvider::GetIcon(wxART_HARDDISK, wxART_LIST, iconSize);
    icons[explorer_file] =
        icons[explorer_file_select] = wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_LIST, iconSize);
    icons[explorer_folder] = wxArtProvider::GetIcon(wxART_FOLDER, wxART_LIST, iconSize);
    icons[explorer_folder_select] = wxArtProvider::GetIcon(wxART_FOLDER_OPEN, wxART_LIST, iconSize);
    for ( size_t i = 0; i < WXSIZEOF(icons); i++ )
    {
        int sizeOrig = icons[0].GetWidth();
        if ( size == sizeOrig )
        {
            images->Add(icons[i]);
        }
        else
        {
            images->Add(wxBitmap(wxBitmap(icons[i]).ConvertToImage().Rescale(size, size)));
        }
    }
    AssignImageList(images);
}

int wxExplorer::OnCompareItems(const wxTreeItemId &first, const wxTreeItemId &second)
{
    wxExplorerItemInfo *pFirst = dynamic_cast<wxExplorerItemInfo*>(GetItemData(first));
    wxExplorerItemInfo *pSecond = dynamic_cast<wxExplorerItemInfo*>(GetItemData(second));
    if (NULL == pFirst || NULL == pSecond){
        return wxTreeCtrl::OnCompareItems(first, second);
    }
    if (pFirst->mbFile && (!pSecond->mbFile)){
        return 1;
    }
    if ((!pFirst->mbFile) && pSecond->mbFile){
        return -1;
    }
    return wxTreeCtrl::OnCompareItems(first, second);
}

wxString wxExplorer::GetCwd()
{
    return wxGetCwd();
}

void wxExplorer::SetCwd(const wxString &cwd)
{
    // todo:fanhongxuan@gmail.com
}
