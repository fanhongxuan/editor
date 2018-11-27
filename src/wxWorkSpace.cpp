#include "wxWorkSpace.hpp"
#include <wx/dir.h>
#include <wx/icon.h>
#include <wx/imaglist.h>
#include <wx/filename.h>
#include <wx/artprov.h>
#include <wx/wxcrtvararg.h> // for wxPrintf
#include <wx/menu.h>
#include <wx/config.h>
#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <wx/busyinfo.h>
#include "ceUtils.hpp"
#include "ce.hpp"

enum workspace_icon{
    workspace_harddisk,
    workspace_file,
    workspace_file_select,
    workspace_folder,
    workspace_folder_select,
    workspace_icon_count,
};

enum workspace_id{
    workspace_id_start = 1000,
    workspace_id_add_dir,
    workspace_id_del_dir,
    workspace_id_generate_tag,
};

// note:fanhongxuan@gmail.com
// user can add/del a directory to/from the workspace
// when quit, will store all the content of the current workspace

wxIMPLEMENT_CLASS(wxWorkSpace, wxTreeCtrl);
class wxWorkSpaceItemInfo: public wxTreeItemData
{
public:
    wxWorkSpaceItemInfo(const wxString &path, bool bFile):mPath(path), mbFile(bFile){}
    wxString mPath;
    bool mbFile;
};

static void AddDir(const wxString &dir, wxWorkSpace &workspace, wxTreeItemId parent, const wxString &target)
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
        wxWorkSpaceItemInfo *pInfo =
            new wxWorkSpaceItemInfo(child_dir, false);
        wxTreeItemId child_id = workspace.AppendItem(parent,
                                                    filename,
                                                    workspace_folder,
                                                    workspace_folder_select,
                                                    pInfo);
        // check if need to follow
        if (target.find(child_dir) == 0){
            AddDir(child_dir, workspace, child_id, target);
        }
        else{
            // note:fanhongxuan@gmail.com
            // check if this dir has children,
            // if has add a temp node to it.
            wxDir child(child_dir);
            if (child.IsOpened() &&(child.HasFiles() || child.HasSubDirs())){
                workspace.AppendItem(child_id, filename, workspace_folder, workspace_folder_select, NULL);
            }
        }
                
        cont = cwd.GetNext(&filename);
    }
    
    // add all the files
    cont = cwd.GetFirst(&filename, "*", wxDIR_FILES|wxDIR_HIDDEN);
    while(cont){
        wxWorkSpaceItemInfo *pInfo =
            new wxWorkSpaceItemInfo(cwd.GetNameWithSep() + filename, true);
        workspace.AppendItem(parent,
                            filename,
                            workspace_file,
                            -1,
                            pInfo);
        cont = cwd.GetNext(&filename);
    }
    // note:fanhongxuan@gmail.com
    // directory first, then file.
    // sort all the children.
    workspace.SortChildren(parent);
    if (dir == target){
        workspace.SelectItem(parent);
        workspace.EnsureVisible(parent);
    }
}

wxBEGIN_EVENT_TABLE(wxWorkSpace, wxTreeCtrl)
EVT_TREE_ITEM_ACTIVATED(wxID_ANY, wxWorkSpace::OnItemActivated)
EVT_TREE_SEL_CHANGED(wxID_ANY, wxWorkSpace::OnSelectionChanged)
EVT_TREE_ITEM_EXPANDING(wxID_ANY, wxWorkSpace::OnItemExpanding)
EVT_MENU(workspace_id_add_dir, wxWorkSpace::OnAddDirToWorkSpace)
EVT_MENU(workspace_id_del_dir, wxWorkSpace::OnDelDirFromWorkSpace)
EVT_MENU(workspace_id_generate_tag, wxWorkSpace::OnGenerateTag)
EVT_RIGHT_DOWN(wxWorkSpace::OnRightDown)
EVT_KEY_DOWN(wxWorkSpace::OnKeyDown)
EVT_SET_FOCUS(wxWorkSpace::OnFocus)
wxEND_EVENT_TABLE()

wxWorkSpace::wxWorkSpace(wxWindow *parent)
    :wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTR_HAS_BUTTONS |
                wxTR_EDIT_LABELS |
                wxTR_HIDE_ROOT |
                wxTR_LINES_AT_ROOT |
#ifdef WIN32
                wxTR_NO_LINES |
#endif
                wxTR_FULL_ROW_HIGHLIGHT)
{
    CreateImageList();
    wxConfig config("CE");
    wxTreeItemId root = AddRoot(wxT("WorkSpace"));
    
    int i = 0;
    while(1){
        wxString dir;
        config.Read(wxString::Format("/Config/WorkSpace/Dir%d", i++), &dir);
        if (dir.empty()){
            break;
        }
        AddDirToWorkSpace(dir);
    }
}

wxWorkSpace::~wxWorkSpace()
{
    wxConfig config("CE");
    wxTreeItemId root = GetRootItem();
    wxTreeItemIdValue cookie = root;
    wxTreeItemId id = GetFirstChild(root, cookie);
    int i = 0;
    // first clear all the exist value
    config.SetPath("/Config/");
    if (config.HasGroup("WorkSpace")){
        config.DeleteGroup("WorkSpace");
    }
    
    while(id.IsOk()){
        wxString value = wxString::Format("WorkSpace/Dir%d", i++);
        wxTreeItemData *pItem = GetItemData(id);
        if (NULL != pItem){
            wxWorkSpaceItemInfo *pInfo = dynamic_cast<wxWorkSpaceItemInfo *>(pItem);
            if (NULL != pInfo){
                config.Write(value, pInfo->mPath);
            }
        }
        id = GetNextChild(root, cookie);
    }
}

bool wxWorkSpace::UpdateTagForFile(const wxString &file)
{
    // first check if this file is belong to current workspace.
    // use gtags --single-update file to update the current file.
    std::set<wxString>::iterator it = mDirs.begin();
    while(it != mDirs.end()){
        if (file.find(*it) == 0){
            // belong to this dir.
            wxString cmd;
#ifdef WIN32
            cmd = "cmd.exe /c \" cd ";
            cmd += (*it);
            cmd += " && ";
            wxString volume;
            wxFileName::SplitPath((*it), &volume, NULL, NULL, NULL);
            cmd += volume;
            cmd += ": && ";
            cmd += ceGetExecPath();
            cmd += "\\ext\\gtags.exe --single-update ";
            cmd += file;
            cmd += "\"";
#else
            cmd += "cd ";
            cmd += (*it);
            cmd += " && ";
            cmd += ceGetExecPath();
            cmd += "/ext/gtags --single-update ";
            cmd += file;
#endif
            std::vector<wxString> outputs;
            ceSyncExec(cmd, outputs);
        }
        it++;
    }
    return true;
}

void wxWorkSpace::GenerateGTagFile(const wxString &dir)
{
    wxString msg = wxString::Format(wxT("Generate tag file for %s, please waiting..."), dir);
    wxBusyInfo busy(msg);
    wxString cmd;
#ifdef WIN32
    cmd = "cmd.exe /c \" cd ";
    cmd += dir;
    cmd += " && ";
    wxString volume;
    wxFileName::SplitPath(dir, &volume, NULL, NULL, NULL);
    cmd += volume;
    cmd += ": && ";
    cmd += ceGetExecPath();
    cmd += "\\ext\\gtags.exe\"";
#else
    cmd = "cd ";
    cmd += dir;
    cmd += " && ";
    cmd += ceGetExecPath();
    cmd += "/ext/gtags";
#endif
    wxPrintf("exec:<%s>\n", cmd);
    std::vector<wxString> outputs;
    ceSyncExec(cmd, outputs);
    
}

bool wxWorkSpace::GenerateTagFile()
{
    std::set<wxString>::iterator it = mDirs.begin();
    while (it != mDirs.end()){
        GenerateGTagFile(*it);
        it++;
    }
    return true;
}
    

void wxWorkSpace::OnFocus(wxFocusEvent &evt)
{
    if (NULL != wxGetApp().frame()){
        wxGetApp().frame()->DoUpdate();
    }
    evt.Skip();
}

// todo:fanhongxuan@gmail.com
// show popup
// on workspace, we can only add/remove dir on the root level
void wxWorkSpace::OnAddDirToWorkSpace(wxCommandEvent &evt)
{
    // show dialog to select a dir;
    // call Add Dir
    wxDirDialog dlg(NULL, wxT("Choose the Dir to Addr"), "",
                    wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK)return;
    wxString dir = dlg.GetPath();
    if (dir.empty()){
        return;
    }
    AddDirToWorkSpace(dir);
    GenerateGTagFile(dir);
}

void wxWorkSpace::OnGenerateTag(wxCommandEvent &evt)
{
    GenerateTagFile();
}

bool wxWorkSpace::AddDirToWorkSpace(const wxString &dir)
{
    wxString name = dir;
    if (name.empty()){
        return false;
    }
    if (name[name.size() -1] == '/' || name[name.size() -1] == '\\'){
        name = name.substr(0, name.size() - 1);
    }
    int pos = name.find_last_of("/\\");
    if (pos != name.npos){
        name = name.substr(pos+1);
    }
    if (name.empty()){
        return false;
    }
    
    std::set<wxString>::iterator it = mDirs.find(dir);
    if (it != mDirs.end()){
        wxMessageBox(wxString::Format(wxT("%s already add to the WorkSpace"), dir), "Error", wxOK, this);
        return false;
    }
    
    mDirs.insert(dir);
    
    wxTreeItemId root = GetRootItem();
    wxTreeItemIdValue cookie = root;
    wxTreeItemId id = GetFirstChild(root, cookie);
    
    /*
    while(id.IsOk()){
        wxTreeItemData *pItem = GetItemData(id);
        if (NULL != pItem){
            wxWorkSpaceItemInfo *pInfo = dynamic_cast<wxWorkSpaceItemInfo *>(pItem);
            if (NULL != pInfo && (!pInfo->mbFile) && dir == pInfo->mPath){
                wxMessageBox(wxString::Format(wxT("%s already add to the WorkSpace"), dir), "Error", wxOK, this);
                return false;    
            }
        }
        id = GetNextChild(id, cookie);
    }
    */
    
    cookie = root;
    id = GetFirstChild(root, cookie);
    bool bHasPath = false;
    while(id.IsOk()){
        wxString text = GetItemText(id);
        wxTreeItemData *pItem = GetItemData(id);
        if (text == name && NULL != pItem){
            wxWorkSpaceItemInfo *pInfo = dynamic_cast<wxWorkSpaceItemInfo *>(pItem);
            if (NULL != pInfo && (!pInfo->mbFile)){
                text += "("; text += pInfo->mPath; text += ")";
                SetItemText(id, text);
                bHasPath = true;
            }
        }
        id = GetNextChild(id, cookie);
    }
    if (bHasPath){
        name += "("; name += dir; name += ")";
    }
    wxWorkSpaceItemInfo *pInfo = new wxWorkSpaceItemInfo(dir, false);
    id = AppendItem(GetRootItem(), name, workspace_folder, workspace_folder_select, pInfo);
    AddDir(dir, *this, id, "");
    return true;
}

void wxWorkSpace::OnDelDirFromWorkSpace(wxCommandEvent &evt)
{
    wxTreeItemId id = GetFocusedItem();
    if (!id.IsOk()){
        return;
    }
    wxTreeItemData *pItem = GetItemData(id);
    if (NULL != pItem){
        wxWorkSpaceItemInfo *pInfo = dynamic_cast<wxWorkSpaceItemInfo *>(pItem);
        if (NULL != pInfo){
            mDirs.erase(pInfo->mPath);
        }
    }
    Delete(id);
}

void wxWorkSpace::OnRightDown(wxMouseEvent &evt)
{
    wxMenu menu;
    wxTreeItemId id = GetFocusedItem();
    menu.Append(workspace_id_add_dir, wxT("Add Dir"));
    if (id.IsOk() && GetItemParent(id) == GetRootItem()){
        // note:fanhongxuan@gmail.com
        // only the children of root can be delete
        menu.Append(workspace_id_del_dir, wxT("Del Dir"));
    }
    menu.Append(workspace_id_generate_tag, wxT("Update Database"));
    PopupMenu(&menu);
}

void wxWorkSpace::OnKeyDown(wxKeyEvent &evt)
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

void wxWorkSpace::OnItemExpanding(wxTreeEvent &evt)
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
    wxWorkSpaceItemInfo *pItem = dynamic_cast<wxWorkSpaceItemInfo*>(pInfo);
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

void wxWorkSpace::OnItemActivated(wxTreeEvent &evt)
{
    wxTreeItemId id = evt.GetItem();
    if (!id.IsOk()){
        return;
    }
    wxTreeItemData *pInfo = GetItemData(id);
    if (NULL == pInfo){
        return;
    }
    wxWorkSpaceItemInfo *pItem = dynamic_cast<wxWorkSpaceItemInfo*>(pInfo);
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

void wxWorkSpace::OnSelectionChanged(wxTreeEvent &evt)
{
}

void wxWorkSpace::CreateImageList()
{
    wxIcon icons[workspace_icon_count];
    int size = 16;
    wxSize iconSize(size, size);
    wxImageList *images = new wxImageList(size, size, true);
    icons[workspace_harddisk] = wxArtProvider::GetIcon(wxART_HARDDISK, wxART_LIST, iconSize);
    icons[workspace_file] =
        icons[workspace_file_select] = wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_LIST, iconSize);
    icons[workspace_folder] = wxArtProvider::GetIcon(wxART_FOLDER, wxART_LIST, iconSize);
    icons[workspace_folder_select] = wxArtProvider::GetIcon(wxART_FOLDER_OPEN, wxART_LIST, iconSize);
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

int wxWorkSpace::OnCompareItems(const wxTreeItemId &first, const wxTreeItemId &second)
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
    wxWorkSpaceItemInfo *pFirst = dynamic_cast<wxWorkSpaceItemInfo*>(GetItemData(first));
    wxWorkSpaceItemInfo *pSecond = dynamic_cast<wxWorkSpaceItemInfo*>(GetItemData(second));
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
