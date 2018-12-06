#include "ceInclude.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf
#include "ceUtils.hpp"

ceInclude *ceInclude::mpInstance = NULL;

ceInclude::ceInclude(){
    
}

ceInclude::~ceInclude(){
    
}

ceInclude &ceInclude::Instance(){
    if (NULL == mpInstance){
        mpInstance = new ceInclude();
    }
    return *mpInstance;
}

bool ceInclude::Reset(){
    return true;
}

bool ceInclude::AddIncDir(const wxString &dir)
{
    std::set<wxString>::iterator it = mDirs.find(dir);
    if (it == mDirs.end()){
        mDirs.insert(dir);
        UpdateFileList(dir);
        return true;
    }
    return false;
}

bool ceInclude::IsSupportedFile(const wxString &file){
    return true;
}

// #include <> & #include ""

bool ceInclude::UpdateFileList(const wxString &dir)
{
    if (dir.empty()){
        return false;
    }
    wxString target = dir;
    std::vector<wxString> output;
    ceFindFiles(target, output);
    if (target[target.size()-1] != '/'){
        target += "/";
    }
    std::vector<wxString>::iterator it = output.begin();
    while(it != output.end()){
        if ((*it).find(target) != 0){
            it++;
            continue;
        }
        wxString name = (*it).substr(target.length());
        if (name.empty()){
            it++;
            continue;
        }
        
        std::map<wxString, wxString>::iterator sit = mFiles.find(name);
        if (sit == mFiles.end()){
            mFiles[name] = *it;
        }
        else{
            mDupFiles.insert(name);
        }
        it++;
    }
    return true;
}

static wxString ParseIncludeFile(const wxString &line)
{
    // 1;0 8:#include "wxAgSearch.hpp"
    // 2;0 8:#include <wx/wxcrtvararg.h> // for wxPrintf
    wxString ret;
    // wxPrintf("line:%s\n", line);
    wxString include ="#include";
    int pos = line.find(":");
    if (pos == line.npos){
        return "";
    }
    ret = line.substr(pos+1);
    pos = ret.find_first_not_of("\r\n\t ");
    if (pos == ret.npos){
        return "";
    }
    ret = ret.substr(pos);
    pos = ret.find("#include");
    if (pos != 0){
        return "";
    }
    ret = ret.substr(pos+include.length());
    if (ret.empty()){
        return "";
    }
    pos = ret.find_first_not_of("\r\n\t ");
    if (pos == ret.npos){
        return "";
    }
    ret = ret.substr(pos);
    if (ret.empty()){
        return "";
    }
    int stop = ret.npos;
    if (ret[0] == '\"'){
        stop = ret.find("\"", 1);
    }
    else if (ret[0] == '<'){
        stop = ret.find(">", 1);
    }
    if (stop == ret.npos){
        return "";
    }
    ret = ret.substr(1, stop-1);
    // wxPrintf("ret:%s\n", ret);
    return ret;
}

static bool GetIncludeFileList(std::set<wxString> &files, const wxString &filename, 
    std::set<wxString> &dirs, std::map<wxString, wxString> &fileMap){
    std::set<wxString>::iterator it = files.find(filename);
    // if (it != files.end()){
    //     return false;
    // }
    std::set<wxString> fileList;
    static wxString ag_exec;
    if (ag_exec.empty()){
        ag_exec = ceGetExecPath();
        ag_exec += "/ext/ag -s --ackmate ";
    }
    std::vector<wxString> output;
    std::map<wxString, wxString>::iterator mIt = fileMap.find(filename);
    wxString fullName = filename;
    if (mIt != fileMap.end()){
        fullName = mIt->second;
    }
    
    wxString cmd = ag_exec + " \"#include\" " + fullName;
    ceSyncExec(cmd,output);
    std::vector<wxString>::iterator fit = output.begin();
    while(fit != output.end()){
        // output is like
        // #include "file"
        // #include <file>
        wxString file = ParseIncludeFile(*fit);
        if (!file.empty()){
            it = files.find(file);
            if (it == files.end()){
                fileList.insert(file);
            }
        }
        fit++;
    }
    
    
    it = fileList.begin();
    while(it != fileList.end()){
        files.insert(*it);
        it++;
    }
    
    // fileList is the new included files.
    it = fileList.begin();
    while(it != fileList.end()){
        GetIncludeFileList(files, *it, dirs, fileMap);
        it++;
    }
    return true;
}

bool ceInclude::GetIncludeFiles(std::set<wxString> &files, const wxString &filename){
    std::set<wxString> fileList;
    GetIncludeFileList(fileList, filename, mDirs, mFiles);
    std::set<wxString>::iterator it = fileList.begin();
    while(it != fileList.end()){
        std::map<wxString, wxString>::iterator mIt = mFiles.find(*it);
        if (mIt != mFiles.end()){
            files.insert(mIt->second);
        }
        it++;
    }
    files.insert(filename);
    return true;
}
