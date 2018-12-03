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

wxIMPLEMENT_CLASS(wxExplorer, wxTreeCtrl);
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
        if (child_dir.find("System Volume Information") != child_dir.npos ||
            child_dir.find("Documents and Settings") != child_dir.npos ||
            child_dir.find("$Recycle.Bin") != child_dir.npos){
            cont = cwd.GetNext(&filename);
            continue;
        }
        wxExplorerItemInfo *pInfo =
            new wxExplorerItemInfo(child_dir, false);
        wxTreeItemId child_id = explorer.AppendItem(parent,
                                                    filename,
                                                    explorer_folder,
                                                    explorer_folder_select,
                                                    pInfo);
        // check if need to follow
        if (target.find(child_dir) == 0){
            AddDir(child_dir, explorer, child_id, target);
        }
        else{
            // note:fanhongxuan@gmail.com
            // check if this dir has children,
            // if has add a temp node to it.
            wxDir child(child_dir);
            if (child.IsOpened() &&(child.HasFiles() || child.HasSubDirs())){
                explorer.AppendItem(child_id, filename, explorer_folder, explorer_folder_select, NULL);
            }
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
EVT_TREE_ITEM_EXPANDING(wxID_ANY, wxExplorer::OnItemExpanding)
EVT_KEY_DOWN(wxExplorer::OnKeyDown)
EVT_SET_FOCUS(wxExplorer::OnFocus)
wxEND_EVENT_TABLE()

wxExplorer::wxExplorer(wxWindow *parent)
    :wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTR_HAS_BUTTONS |
                wxTR_EDIT_LABELS |
                wxTR_HIDE_ROOT   | 
                wxTR_LINES_AT_ROOT |
#ifdef WIN32
                wxTR_NO_LINES |
#endif
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
#ifdef WIN32
    wxTreeItemId root = AddRoot(wxT("FileSystem"), explorer_harddisk);
    
    // on win32, try other disk
    // from A-Z
    for (char i = 'A'; i < 'Z'; i++){
        wxString vol = i;
        vol += ":/";
        wxString target;
        if (vol == volume){
            target = cwd;
        }
        wxDir dir(vol);
        if (!dir.IsOpened()){
            continue;
        }
        wxTreeItemId data = AppendItem(root, vol, explorer_harddisk);
        AddDir(vol, *this, data, target);
    }
#else
    wxTreeItemId root = AddRoot(volume, explorer_harddisk);
    AddDir(volume, *this, root, cwd);
#endif    
}

wxExplorer::~wxExplorer()
{
    wxConfig config("CE");
    config.Write("/Config/CurrentWorkingDirectory", mCwd);
}

void wxExplorer::OnFocus(wxFocusEvent &evt)
{
    if (NULL != wxGetApp().frame()){
        wxGetApp().frame()->DoUpdate();
    }    
    evt.Skip();
}

void wxExplorer::OnKeyDown(wxKeyEvent &evt)
{
    int key = evt.GetKeyCode();
    if (WXK_LEFT == key){
        wxTreeItemId id = GetFocusedItem();
        if (id.IsOk() && IsExpanded(id)){
            Collapse(id);
            return;
        }
    }
    else{
        // if the key is a-z 0-9 try to match it the item on current level.
        if ((key >= 'A' && key <= 'Z') || (key <= '9' && key >= '0')){
            wxTreeItemId id = GetFocusedItem();
            if (id.IsOk()){
                wxTreeItemId next = GetNextSibling(id);
                bool bMatch = false;
                while(next.IsOk()){
                    wxString text = GetItemText(next).Upper();
                    if (text[0].GetValue() == key){
                        SelectItem(next);
                        return;
                    }
                    next = GetNextSibling(next);
                }

                // todo:fanhongxuan@gmail.com
                // find the first child
                next = GetItemParent(id);
                if (next.IsOk()){
                    wxTreeItemIdValue cookie = NULL;
                    next = GetFirstChild(next, cookie);
                }
                while(next.IsOk()){
                    wxString text = GetItemText(next).Upper();
                    if (text[0].GetValue() == key){
                        SelectItem(next);
                        return;
                    }                    
                    next = GetNextSibling(next);
                }
            }
            return;
        }
    }
    evt.Skip();
}

void wxExplorer::OnItemExpanding(wxTreeEvent &evt)
{
    evt.Skip();
    wxTreeItemId id = evt.GetItem();
    if (!id.IsOk()){
        return;
    }
    wxTreeItemData *pInfo = GetItemData(id);
    if (NULL == pInfo){
        return;
    }
    wxExplorerItemInfo *pItem = dynamic_cast<wxExplorerItemInfo*>(pInfo);
    if (NULL == pItem){
        return;
    }
    // wxPrintf("Active:%s\n", pItem->mPath);
    // this is a folder, but has no children, try to load it.
    if (pItem->mbFile){
        return;
    }
    if(1 == GetChildrenCount(id)){
        wxTreeItemIdValue cookie = id;
        wxTreeItemId child = GetFirstChild(id, cookie);
        if (child.IsOk() && NULL == GetItemData(child)){
            DeleteChildren(id);
            AddDir(pItem->mPath, *this, id, "");
            // Expand(id);
        }
    } 
}

void wxExplorer::OnItemActivated(wxTreeEvent &evt)
{
    wxTreeItemId id = evt.GetItem();
    if (!id.IsOk()){
        return;
    }
    wxTreeItemData *pInfo = GetItemData(id);
    if (NULL == pInfo){
        return;
    }
    wxExplorerItemInfo *pItem = dynamic_cast<wxExplorerItemInfo*>(pInfo);
    if (NULL == pItem){
        return;
    }
    // wxPrintf("Active:%s\n", pItem->mPath);
    // this is a folder, but has no children, try to load it.
    if ((!pItem->mbFile)){
        if(1 == GetChildrenCount(id)){
            wxTreeItemIdValue cookie = id;
            wxTreeItemId child = GetFirstChild(id, cookie);
            if (child.IsOk() && NULL == GetItemData(child)){
                DeleteChildren(id);
                AddDir(pItem->mPath, *this, id, "");
                Expand(id);
            }
        }
        else{
            Toggle(id);
        }
    }
    else{
        if (NULL != wxGetApp().frame()){
            wxGetApp().frame()->OpenFile(pItem->mPath, pItem->mPath, true);
        }
    }
}

void wxExplorer::OnSelectionChanged(wxTreeEvent &evt)
{
    wxTreeItemId id = evt.GetItem();
    if (!id.IsOk()){
        return;
    }
    wxTreeItemData *pInfo = GetItemData(id);
    if (NULL == pInfo){
        return;
    }
    wxExplorerItemInfo *pItem = dynamic_cast<wxExplorerItemInfo*>(pInfo);
    if (NULL == pItem){
        return;
    }
    wxString path = pItem->mPath;
    if (pItem->mbFile){
        wxFileName::SplitPath(pItem->mPath, NULL, &path, NULL, NULL);
    }
    
    mCwd = path;
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
    if ((!first.IsOk()) || (!second.IsOk())){
        return 1;
    }
    if (NULL == GetItemData(first)){
        return 1;
    }
    if (NULL == GetItemData(second)){
        return -1;
    }
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
    return mCwd;
}

void wxExplorer::SetCwd(const wxString &cwd)
{
    // todo:fanhongxuan@gmail.com
    // update the selection in the tree.
    mCwd = cwd;
}
