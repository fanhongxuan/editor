#include "ceUtils.hpp"
#include <wx/filename.h>
#include <wx/wxcrtvararg.h> // for wxPrintf
#include "ceSymbolDb.hpp"
#include "ceInclude.hpp"
#include "ceRocksDb.hpp"
// note:fanhongxuan@gmail.com
// store all the content to a rocks db
// key is the filename:
// value is the ctags outputs.

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
        rootdir += "/symboldb/tags";
    }
    // // replace all the / \\ to - and add root dir
    // wxString ret;
    // wxString volume, path, filename, ext;
    // wxFileName::SplitPath(source_filename, &volume, &path, &filename, &ext);
    // ext = ext.Lower();
    // if (ext != "cpp" && ext != "hpp" &&
    //     ext != "c" && ext != "h" &&
    //     ext != "cxx" && ext != "hxx"){
    //     // wxPrintf("Unknown ext:%s\n", ext);
    //     return "";
    // }
    // int i = 0;
    // ret = volume;
    // if (!ret.empty()){
    //     ret += ".";
    // }
    // ret += path; 
    // ret += "."; ret += filename; 
    // ret += "."; ret += ext;
    // for (i = 0; i < ret.size(); i++){
    //     char c = ret[i];
    //     if (c == '/' || c == '\\'){
    //         ret[i] = '.';
    //     }
    // }
    // ret = rootdir + ret;
    // ret += ".symbol";
    // return ret;
    return rootdir;
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
    // static wxString ctags_exec;
    // if (ctags_exec.empty()){
    //     ctags_exec = ceGetExecPath();
    //     ctags_exec += "/ext/ctags -n --kinds-C++=+p -f  ";
    // }
    // wxString name = GetSymbolDbName(dir);
    // if (name.empty()){
    //     return false;
    // }
    // wxString cmd = ctags_exec + name + " -R " + dir;
    // std::vector<wxString> outputs;
    // // fixme: current not used.
    // ceSyncExec(cmd, outputs);
    std::vector<wxString> files;
    ceFindFiles(dir, files);
    std::vector<wxString>::const_iterator it = files.begin();
    while(it != files.end()){
        UpdateSymbolByFile(*it);
        it++;
    }
    return true;
}

bool ceSymbolDb::UpdateRecord(const wxString &line){
    // a symbol record <id, name, filename, line, returntype, signature, access, > 
    // key is:
    // type,(d,e,f,m,N,etc) <-- use a table to get the full name
    // classname::symbolname, (if have namespace, need include the namespace here)
    
    // the ctag output is like:
    // later we need to change the ctags direclty output the data to a database.
    // FindDef	src/ceRefSearch.cpp	136;"	f	class:ceRefSearch	typeref:typename:bool	signature:(const wxString & symbol,std::vector<wxSearchFileResult * > & results)
    // 
    
    // value is :
    // filename,
    // line,
    // signature
    // access,
    
    // use case:
    // known class name, find all the member function,
    // known class name, find all the member function,
    // know function name, find the signature, and return type.
    // find all the global function name,
    // find all the global variable name,
}

bool ceSymbolDb::UpdateSymbolByFile(const wxString &filename){
    static wxString ctags_exec;
    if (ctags_exec.empty()){
        ctags_exec = ceGetExecPath();
        ctags_exec += "/ext/ctags -n --kinds-C++=+p --fields=+a+i+m+S+R+E -f - ";
    }
    
    // todo:fanhongxuan@gmail.com
    // delete all the record of this file.
    // wxString name = GetSymbolDbName(filename);
    // if (name.empty()){
    //     return false;
    // }
    wxString cmd = ctags_exec + " " + filename;
    std::vector<wxString> outputs;
    // fixme: current not used.
    ceSyncExec(cmd, outputs);
    wxString value;
    for (int i = 0; i < outputs.size();i++){
        UpdateRecord(outputs[i]);
        // ceRocksDb::Instance().WriteValue();
    }
    
    // store the output to db.
    ceRocksDb::Instance().WriteValue(filename, value);
    return true;
}

bool ceSymbolDb::FindDef(std::set<ceSymbol*> &symbols, const wxString &name, const wxString &type, const wxString &filename){
    
    // fixme:fanhongxuan@gmail.com
    // test code begin
    wxPrintf("Call ceRocksDb::Dump\n");
    ceRocksDb::Instance().Dump("", "");
    // test code end
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
        // fixme:fanhongxuan@gmail.com
        // if user change a anonymous enum define, it maybe generte two define.
        // so we check if it's duplicate here.
        // if name, filename, line is same, skip it.
        if (NULL != pSymbol){
            std::set<ceSymbol*>::iterator it = symbols.begin();
            while(it != symbols.end()){
                if ((*it)->name == pSymbol->name &&
                    (*it)->file == pSymbol->file &&
                    (*it)->line == pSymbol->line &&
                    (*it)->symbolType == pSymbol->symbolType &&
                    (*it)->type == pSymbol->type){
                    break;
                }
                it++;
            }
            if (it != symbols.end()){
                delete pSymbol;
            }
            else{
                symbols.insert(pSymbol);
            }
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
    if ((!name.empty()) && name != symbol){
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
    pRet->symbolType = outputs[0];
    wxString strTypeName = "typename:";
    pos = value.find(strTypeName);
    if (pos != value.npos){
        pRet->type = value.substr(pos + strTypeName.length());
        // int stopType = value.find("file:", pos + strTypeName.length());
        // if (stopType != value.npos){
        //     pRet->type = value.substr(pos + strTypeName.length(), stopType);
        // }
    }
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

static ceSymbol *ParseSymbolLine(const wxString &line, const wxString &file){
    int pos = line.find_last_of('"');
    if (pos == line.npos){
        wxPrintf("No \" found\n");
        return NULL;
    }
    wxString ret, value = line.substr(0, pos);
    std::vector<wxString> outputs;
    ceSplitString(value, outputs, '\t', false);
    if (outputs.size() < 3){
        wxPrintf("Invalid outputs size():%ld<%s>\n", outputs.size(), value);
        return NULL;
    }
    wxString symbol = outputs[0]; // symbol name
    wxString lineNumber = outputs[outputs.size() - 1]; // last is linenumber
    lineNumber = lineNumber.substr(0, lineNumber.size() - 1); // remove the last ';'
    long iLineNumber = 0;
    lineNumber.ToLong(&iLineNumber);
    if (iLineNumber > 0){
        iLineNumber--;
    }
    
    outputs.clear();
    value = line.substr(pos+1);
    ceSplitString(value, outputs, '\t', false);
    if (outputs.size() < 1){
        wxPrintf("Invalid outputs size()%ld<%s><%s>\n", outputs.size(), line, value);
        return NULL;
    }
    ceSymbol *pRet = new ceSymbol;
    pRet->symbolType = outputs[0];
    wxString strTypeName = "typename:";
    pos = value.find(strTypeName);
    if (pos != value.npos){
        pRet->type = value.substr(pos + strTypeName.length());
        // int stopType = value.find("file:", pos + strTypeName.length());
        // if (stopType != value.npos){
        //     pRet->type = value.substr(pos + strTypeName.length(), stopType);
        // }
    }
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

bool ceSymbolDb::GetFileSymbol(const wxString &filename, std::set<ceSymbol*> &symbols){
    static wxString ctags_exec;
    if (ctags_exec.empty()){
        ctags_exec = ceGetExecPath();
        ctags_exec += "/ext/ctags -n --kinds-C++=+p+z+l -f - ";
    }
    if (filename.empty()){
        return false;
    }
    wxString cmd = ctags_exec + filename;
    std::vector<wxString> outputs;
    std::set<wxString> files; // empty files
    // fixme: current not used.
    ceSyncExec(cmd, outputs);
    int i = 0;
    wxPrintf("%s has %ld line output\n", filename, outputs.size());
    for (i = 0; i < outputs.size(); i++ ){
        ceSymbol *pRet = ParseSymbolLine(outputs[i], filename);
        if (NULL != pRet){
            symbols.insert(pRet);
        }
    }
    return true;
}
