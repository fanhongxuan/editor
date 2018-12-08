#include "ceUtils.hpp"
#include <wx/filename.h>
#include <wx/wxcrtvararg.h> // for wxPrintf
#include "ceSymbolDb.hpp"
#include "ceInclude.hpp"

bool ceSymbol::IsSame(const ceSymbol &other){
    // todo:fanhongxuan@gmail.com
    return true;
}

bool ceSymbol::operator==(const ceSymbol &other){
    return IsSame(other);
}

ceSymbolDb::ceSymbolDb(){
    if (mRootDir.empty()){
        mRootDir = ceGetExecPath() + "/symboldb/";
        // todo:fanhongxuan@gmail.com
        // create the dir first if not exist.
    }
}

ceSymbolDb::~ceSymbolDb(){
    
}

wxString ceSymbolDb::GetSymbolDbName(const wxString &source_filename){
    static wxString rootdir;
    if (rootdir.empty()){
        rootdir = ceGetExecPath();
        rootdir += "/symboldb/";
    }
    // replace all the / \\ to - and add root dir
    wxString ret;
    wxString volume, path, filename, ext;
    wxFileName::SplitPath(source_filename, &volume, &path, &filename, &ext);
    ext = ext.Lower();
    if (ext != "cpp" && ext != "hpp" &&
        ext != "c" && ext != "h" &&
        ext != "cxx" && ext != "hxx"){
        // wxPrintf("Unknown ext:%s\n", ext);
        return "";
    }
    int i = 0;
    ret = volume;
    if (!ret.empty()){
        ret += ".";
    }
    ret += path; 
    ret += "."; ret += filename; 
    ret += "."; ret += ext;
    for (i = 0; i < ret.size(); i++){
        char c = ret[i];
        if (c == '/' || c == '\\'){
            ret[i] = '.';
        }
    }
    ret = rootdir + ret;
    ret += ".symbol";
    return ret;
}

bool ceSymbolDb::UpdateSymbol(const std::set<wxString> &dirs){
    std::set<wxString>::const_iterator it = dirs.begin();
    while(it != dirs.end()){
        UpdateSymbolByDir(*it);
        it++;
    }
    return true;
}

bool ceSymbolDb::UpdateSymbolByDir(const wxString &dir){
    std::vector<wxString> files;
    ceFindFiles(dir, files);
    std::vector<wxString>::const_iterator it = files.begin();
    while(it != files.end()){
        UpdateSymbolByFile(*it);
        it++;
    }
    return true;
}

bool ceSymbolDb::UpdateSymbolByFile(const wxString &filename){
    static wxString ctags_exec;
    if (ctags_exec.empty()){
        ctags_exec = ceGetExecPath();
        ctags_exec += "/ext/ctags -n --kinds-C++=+p -f  ";
    }
    wxString name = GetSymbolDbName(filename);
    if (name.empty()){
        return false;
    }
    wxString cmd = ctags_exec + name + " " + filename;
    std::vector<wxString> outputs;
    // fixme: current not used.
    ceSyncExec(cmd, outputs);
    return true;
}

bool ceSymbolDb::FindDef(std::set<ceSymbol*> &symbols, const wxString &name, const wxString &type, const wxString &filename){
    static wxString ag_exec;
    if (ag_exec.empty()){
        ag_exec = ceGetExecPath();
        ag_exec += "/ext/ag -s -w --ackmate --hidden --nonumbers ";
    }
    wxString cmd = ag_exec;
    // if (!filename.empty()){
    //     wxString symboldb = GetSymbolDbName(filename);
    //     if (symboldb.empty()){
    //         return false;
    //     }
    //     cmd += name;
    //     cmd += " ";
    //     cmd += symboldb;
    // }
    // else
    {
        cmd += name;
        cmd += " ";
        cmd += ceGetExecPath();
        cmd += "/symboldb/";
    }
    
    // todo:fanhongxuan@gmail.com
    // speed is two slow, current not used.
    std::set<wxString> files;
    if (!filename.empty()){
        // ceInclude::Instance().GetIncludeFiles(files, filename);
        // std::set<wxString>::iterator it = files.begin();
        // wxPrintf("FileName:%s\n", filename);
        // while(it != files.end()){
        //     wxPrintf("include:%s\n", *it);
        //     it++;
        // }
    }
    
    std::vector<wxString> outputs;
    ceSyncExec(cmd, outputs);
    for (int i = 0; i < outputs.size();i++){
        ceSymbol *pSymbol = ParseLine(outputs[i], name, type, files);
        if (NULL != pSymbol){
            symbols.insert(pSymbol);
        }
    }
    return true;
}


extern const wxString &getFullTypeName(const wxString &input, const wxString &language);
ceSymbol *ceSymbolDb::ParseLine(const wxString &line, const wxString &name, const wxString &type, std::set<wxString> &files){
    int pos = line.find_last_of('"');
    if (pos == line.npos){
        return NULL;
    }
    wxString ret, value = line.substr(0, pos);
    int ipos = value.find_first_of(':');
    if (ipos == value.npos){
        return NULL;
    }
    value = value.substr(ipos+1);
    std::vector<wxString> outputs;
    ceSplitString(value, outputs, '\t', true);
    if (outputs.size() < 3){
        wxPrintf("Invalid outputs size():%ld<%s>\n", outputs.size(), value);
        return NULL;
    }
    // wxPrintf("Line:%s\n", line);
    // wxPrintf("type:%s\n", type);
    wxString symbol = outputs[0]; // symbol name
    if (name != symbol){
        return NULL;
    }
    wxString file;
    for (int i = 1; i < (outputs.size() - 1); i++){
        if (!file.empty()){
            file += "\t";
        }
        file = outputs[i];
    }
    if (files.size() != 0 && files.find(file) == files.end()){
        wxPrintf("Not found:%s\n", file);
        return NULL;
    }
    wxString lineNumber = outputs[outputs.size() - 1]; // last is linenumber
    lineNumber = lineNumber.substr(0, lineNumber.size() - 1); // remove the last ';'
    long iLineNumber = 0;
    lineNumber.ToLong(&iLineNumber);
    // if (iLineNumber > 0){
    //     iLineNumber--;
    // }
    
    outputs.clear();
    value = line.substr(pos+1);
    ceSplitString(value, outputs, '\t', false);
    if (outputs.size() < 1){
        wxPrintf("Invalid outputs size()%ld<%s><%s>\n", outputs.size(), line, value);
        return NULL;
    }
    // wxPrintf("type:%s\n", outputs[0]);
    if (!type.empty() && type.find(outputs[0]) == wxNOT_FOUND){
        return NULL;
    }
    // if (outputs[0] != 'p'){
    //     return NULL;
    // }
    ceSymbol *pRet = new ceSymbol;
    pRet->type = getFullTypeName(outputs[0], "C++");
    pRet->name = symbol;
    pRet->file = file;
    pRet->line = iLineNumber;
    wxString desc = ceGetLine(file, iLineNumber);
    pos = desc.find_first_not_of("\r\n\t ");
    if (pos != desc.npos){
        desc = desc.substr(pos);
    }
    pRet->desc = desc;
    return pRet;
}
