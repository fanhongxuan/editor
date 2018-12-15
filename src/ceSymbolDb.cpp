#include "ceUtils.hpp"
#include <wx/filename.h>
#include <wx/wxcrtvararg.h> // for wxPrintf
#include "ceSymbolDb.hpp"
#include "ceInclude.hpp"
extern "C"{
#include "dbop.h"
}
// #include "ceRocksDb.hpp"
// note:fanhongxuan@gmail.com
// store all the content to a rocks db
// key is the filename:
// value is the ctags outputs.

extern const wxString &getFullTypeName(const wxString &input, const wxString &language);

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
    // ext = ext.Lower();
    // if (ext != "cpp" && ext != "hpp" &&
    //     ext != "c" && ext != "h" &&
    //     ext != "cxx" && ext != "hxx"){
    //     // wxPrintf("Unknown ext:%s\n", ext);
    //     return "";
    // }
    int i = 0;
    ret = volume;
    if (!ret.empty()){
        ret += ".";
    }
    ret += path; 
    // ret += "."; ret += filename; 
    // ret += "."; ret += ext;
    for (i = 0; i < ret.size(); i++){
        char c = ret[i];
        if (c == '/' || c == '\\'){
            ret[i] = '.';
        }
    }
    ret = rootdir + ret;
    // ret += ".symbol";
    return ret;
//     return rootdir;
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
    static wxString ctags_exec;
    if (ctags_exec.empty()){
        ctags_exec = ceGetExecPath();
        ctags_exec += "/ext/ctags -n --output-format=rocksdb --kinds-C++=+p --fields=+a+i+m+S+R+E -f  ";
    }
    wxString name = GetSymbolDbName(dir+"/symbol");
    if (name.empty()){
        return false;
    }
    wxString cmd = ctags_exec + name + " -R " + dir;
    std::vector<wxString> outputs;
    // fixme: current not used.
    ceSyncExec(cmd, outputs);
    // std::vector<wxString> files;
    // ceFindFiles(dir, files);
    // std::vector<wxString>::const_iterator it = files.begin();
    // while(it != files.end()){
    //     UpdateSymbolByFile(*it);
    //     it++;
    // }
    return true;
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
    // ceSyncExec(cmd, outputs);
    // wxString value;
    // for (int i = 0; i < outputs.size();i++){
    //     UpdateRecord(outputs[i]);
    //     // ceRocksDb::Instance().WriteValue();
    // }
    
    // store the output to db.
    // ceRocksDb::Instance().WriteValue(filename, value);
    return true;
}

enum{
    VALUE_INDEX_FILE = 0,
    VALUE_INDEX_LINE_NUMBER,
    VALUE_INDEX_LINE_END,
    VALUE_INDEX_LANGUAGE,
    VALUE_INDEX_ACCESS,
    VALUE_INDEX_INHERIT,
    VALUE_INDEX_TYPE_REF,
    VALUE_INDEX_PARAM,
    VALUE_INDEX_MAX
};

static bool GetInherit(std::vector<wxString> &outputs, const wxString &className, DBOP *pDb){
    if (NULL == pDb){
        return false;
    }
    if (className.empty()){
        return false;
    }
    wxString key = "c//" + className;
    wxString value = dbop_get(pDb, key);
    // wxPrintf("GetInherit:<%s>-<%s>\n", key, value);
    std::vector<wxString> values;
    ceSplitString(value, values, '\'');
    if (values.size() < VALUE_INDEX_MAX){
        return false;
    }
    value = values[VALUE_INDEX_INHERIT];
    if ((!value.empty()) && value != "-" && value != "(null)"){
        std::vector<wxString> base;
        ceSplitString(value, base, ',');
        for (int i = 0; i < base.size(); i++){
            // wxPrintf("Add base <%s>\n", base[i]);
            outputs.push_back(base[i]);
            GetInherit(outputs, base[i], pDb);
        }
    }
    return true;
}

static bool GetDbValue(std::set<ceSymbol*> &symbols, 
    const wxString &type, 
    const wxString &className, 
    const wxString &name,
    DBOP *pDb){
    wxString key = type + "/";
    key += className + "/";
    key += name;
    wxString value = dbop_get(pDb, static_cast<const char *>(key));
    wxPrintf("Key:<%s>,value:<%s>\n", key, value);
    if (!value.empty()){
        std::vector<wxString> sym;
        ceSplitString(value, sym, '\'');
        ceSymbol *pRet = new ceSymbol;
        pRet->symbolType = getFullTypeName(type, sym[VALUE_INDEX_LANGUAGE]); 
        if (sym[VALUE_INDEX_TYPE_REF] != "-"){
            pRet->type = sym[VALUE_INDEX_TYPE_REF];
            if (pRet->type.find("typename:") != pRet->type.npos){
                pRet->type = pRet->type.substr(strlen("typename:"));
            }
        }
        pRet->name = name;
        pRet->scope = className;
        pRet->lineNumber = sym[VALUE_INDEX_LINE_NUMBER];
        pRet->lineEnd = sym[VALUE_INDEX_LINE_END];
        pRet->access = sym[VALUE_INDEX_ACCESS];
        pRet->file = sym[VALUE_INDEX_FILE];
        pRet->param = sym[VALUE_INDEX_PARAM];
        if (pRet->access == "public"){
            pRet->desc = "+";
        }
        else if (pRet->access == "private"){
            pRet->desc = "-";
        }
        else if (pRet->access == "protected"){
            pRet->desc = "*";
        }
        pRet->desc += pRet->type;
        pRet->desc += " ";
        if (!pRet->scope.empty()){
            pRet->desc += pRet->scope;
            pRet->desc += "::";
        }
        pRet->desc += name;
        if (pRet->param != "-"){
            pRet->desc += pRet->param;
        }
        symbols.insert(pRet);
        // wxPrintf("key:<%s>,value:<%s>\n", keys[i], value);
    }
    return true;
}

bool ceSymbolDb::FindDef(std::set<ceSymbol*> &symbols,
    const wxString &name, 
    const wxString &className,
    const wxString &type,
    const wxString &filename,
    const wxString &dir){
    wxString db = GetSymbolDbName(dir + "/symbol");
    db += ".db";
    DBOP *pDb = dbop_open(static_cast<const char*>(db), 0, 0644, 0);
    if (NULL == pDb){
        wxPrintf("Failed to open %s\n", db);
        return false;
    }
    if (type.empty() && className.empty()){
        // try all global value first.
        // type and class is all empty
        GetDbValue(symbols, "d", "" , name, pDb);
        GetDbValue(symbols, "f", "" , name, pDb);
        GetDbValue(symbols, "e", "" , name, pDb);
    }
    else if (className.empty()){
        for (int i = 0; i < type.size(); i++){
            GetDbValue(symbols, type[i], "", name, pDb);
        }
    }
    else if (type.empty()){
        std::vector<wxString> classNames;
        classNames.push_back(className);
        classNames.push_back("");
        GetInherit(classNames, className, pDb);
        for (int i = 0; i < classNames.size(); i++){
            GetDbValue(symbols, "m", classNames[i], name, pDb);
            GetDbValue(symbols, "f", classNames[i], name, pDb);
            GetDbValue(symbols, "p", classNames[i], name, pDb);
        }
    }
    else{
        std::vector<wxString> classNames;
        classNames.push_back(className);
        classNames.push_back("");
        GetInherit(classNames, className, pDb);
        for (int i = 0; i < classNames.size(); i++){
            for (int j = 0; j < type.size(); j++){
                GetDbValue(symbols, type[j], classNames[i], name, pDb);
            }
        }
    }
    
    dbop_close(pDb);    
    return true;
}

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
