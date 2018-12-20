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

wxString ceSymbol::ToAutoCompString()
{
    wxString ret = name;
    if (param != "-" && param != ""){
        // function
        ret += param;
        if (symbolType != "Function"){
            ret += "[Defination]";
        }
        if (scope != "-" && scope != ""){
            ret += "[<" + type + ">"+ scope + "]";
        }
        else{
            ret += "[<" + type + ">" + shortFilename +"]";
        }
    }
    else{
        // other type
        if (symbolType == "Member"){
            ret += "[" + type + "," + access + "," + scope +"]";
        }
        else{
            ret += "[" + symbolType + "," + shortFilename + "]";
        }
    }
    return ret;
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

bool ceSymbolDb::UpdateSymbolByFile(const wxString &filename, const wxString &dir){
    static wxString ctags_exec;
    if (ctags_exec.empty()){
        ctags_exec = ceGetExecPath();
        ctags_exec += "/ext/ctags -n --output-format=rocksdb --kinds-C++=+p -a --fields=+a+i+m+S+R+E -f ";
    }
    wxString name = GetSymbolDbName(dir + "/symbol");
    if (name.empty()){
        return false;
    }
    
    wxString cmd = ctags_exec + name + " " + filename;
    wxPrintf("exec<%s>\n", cmd);
    std::vector<wxString> outputs;
    ceSyncExec(cmd, outputs);
    return true;
}

enum{
    VALUE_INDEX_FILE = 0,
    VALUE_INDEX_LINE_NUMBER,
    VALUE_INDEX_LINE_END,
    VALUE_INDEX_ACCESS,
    VALUE_INDEX_INHERIT,
    VALUE_INDEX_TYPE_REF,
    VALUE_INDEX_PARAM,
    VALUE_INDEX_MAX
};

static bool GetInherit(std::vector<wxString> &outputs, const wxString &className, const wxString &language, DBOP *pDb){
    if (NULL == pDb){
        return false;
    }
    if (className.empty()){
        return false;
    }
    wxString key = "c/" + language + "//" + className;
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
            GetInherit(outputs, base[i], language, pDb);
        }
    }
    return true;
}

static ceSymbol* BuildSymbol(const wxString &name, 
    const wxString &type, const wxString &scope, 
    const wxString &language, const wxString &value){
    std::vector<wxString> sym;
    ceSplitString(value, sym, '\'');
    ceSymbol *pRet = new ceSymbol;
    // wxPrintf("BuildSymbol:<%s>\n", value);
    if (sym.size() < VALUE_INDEX_MAX){
        return NULL;
    }
    pRet->symbolType = getFullTypeName(type, language); 
    if (sym[VALUE_INDEX_TYPE_REF] != "-"){
        pRet->type = sym[VALUE_INDEX_TYPE_REF];
        if (pRet->type.find("typename:") != pRet->type.npos){
            pRet->type = pRet->type.substr(strlen("typename:"));
        }
    }
    pRet->name = name;
    pRet->scope = scope;
    pRet->lineNumber = sym[VALUE_INDEX_LINE_NUMBER];
    pRet->lineNumber.ToLong(&pRet->line);
    pRet->lineEnd = sym[VALUE_INDEX_LINE_END];
    pRet->access = sym[VALUE_INDEX_ACCESS];
    pRet->file = sym[VALUE_INDEX_FILE];
    pRet->param = sym[VALUE_INDEX_PARAM];
    if (pRet->symbolType == "Prototype"){
        pRet->desc += "(" + pRet->access+ ")";
    }
    // if (pRet->access == "public"){
    //     pRet->desc = "+";
    // }
    // else if (pRet->access == "private"){
    //     pRet->desc = "-";
    // }
    // else if (pRet->access == "protected"){
    //     pRet->desc = "*";
    // }
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
    else{
        pRet->desc += "[" + pRet->symbolType + "]";
    }
    return pRet;
}

static bool GetDbRange(std::set<ceSymbol*> &symbols, 
    const wxString &type, 
    const wxString &scope, 
    const wxString &language, 
    DBOP *pDb)
{
    wxString lan = language;
    if (lan == "C++"){
        lan = "C";
    }
    wxString key = type + "/" + lan +  "/" + scope + "/";
    // wxPrintf("Key:%s\n", key);
    const char *pValue = dbop_first(pDb, key, NULL, DBOP_PREFIX | DBOP_KEY);
    while(NULL != pValue){
        wxString name = pValue;
        name = name.substr(key.length());
        // wxPrintf("name:%s\n", name);
        // wxPrintf("Value:%s\n", pDb->lastdat);
        ceSymbol *pRet = BuildSymbol(name, type, scope, language, pDb->lastdat);
        if (NULL != pRet){
            symbols.insert(pRet);
        }
        pValue = dbop_next(pDb);
    }
    return true;
}

static bool GetDbValue(std::set<ceSymbol*> &symbols, 
    const wxString &type, 
    const wxString &className, 
    const wxString &name,
    const wxString &language,
    DBOP *pDb){
    wxString lan = language;
    if (lan == "C++"){
        lan = "C";
    }
    wxString key = type + "/" + lan + "/";
    key += className + "/";
    key += name;
    
    wxString value = dbop_get(pDb, static_cast<const char *>(key));
    // wxPrintf("Key:<%s>,value:<%s>\n", key, value);
    if (!value.empty()){
        ceSymbol *pRet = BuildSymbol(name, type, className, language, value);
        if (NULL != pRet){
            symbols.insert(pRet);
        }
        // wxPrintf("key:<%s>,value:<%s>\n", keys[i], value);
    }
    return true;
}

bool ceSymbolDb::GetSymbols(std::set<ceSymbol*> &symbols,
    const wxString &scope, 
    const wxString &types, 
    const wxString &language, 
    const wxString &dir)
{
    wxString lan = language;
    if (lan == "C++"){
        lan = "C";
    }
    wxString type = types;
    wxString db = GetSymbolDbName(dir + "/symbol");
    db += ".db";
    DBOP *pDb = dbop_open(static_cast<const char*>(db), 0, 0644, DBOP_RAW);
    if (NULL == pDb){
        wxPrintf("Failed to open %s\n", db);
        return false;
    }
    wxPrintf("GetSymbols:scope<%s>, type:<%s>,dir<%s>\n", scope, type, dir);
    std::vector<wxString> classNames;
    if (type.empty() && scope.empty()){
        // try all global value first.
        // type and class is all empty
        type = "dfec";
        classNames.push_back("");
        classNames.push_back("__anon");
    }
    else{
        classNames.push_back(scope);
        GetInherit(classNames, scope, lan, pDb);
        if (type.empty()){
            type = "mfpce";
        }
    }
    
    for (int i = 0; i < classNames.size(); i++){
        for (int j = 0; j < type.size(); j++){
            GetDbRange(symbols, type[j], classNames[i], lan, pDb);
        }
    }
    dbop_close(pDb);    
    return true;
}

bool ceSymbolDb::FindDef(std::set<ceSymbol*> &symbols,
    const wxString &name, 
    const wxString &className,
    const wxString &types,
    const wxString &language,
    const wxString &filename,
    const wxString &dir){
    wxString db = GetSymbolDbName(dir + "/symbol");
    db += ".db";
    DBOP *pDb = dbop_open(static_cast<const char*>(db), 0, 0644, DBOP_RAW);
    if (NULL == pDb){
        wxPrintf("Failed to open %s\n", db);
        return false;
    }
    // wxPrintf("FindDef:name<%s>,className<%s>, type<%s>, language:<%s>\n", name, className, types, language);
    wxString lan = language;
    if (lan == "C++"){
        lan = "C";
    }
    wxString type = types;
    std::vector<wxString> classNames;
    if (type.empty() && className.empty()){
        // try all global value first.
        // type and class is all empty
        type = "dfe";
    }
    else{
        classNames.push_back(className);
        GetInherit(classNames, className, lan, pDb);
        if (type.empty()){
            type = "mfpe";
        }
    }
    if (className != ""){
        classNames.push_back("");
    }
    if (className != "__anon"){
        classNames.push_back("__anon");
    }
    for (int i = 0; i < classNames.size(); i++){
        for (int j = 0; j < type.size(); j++){
            GetDbValue(symbols, type[j], classNames[i], name, lan, pDb);
        }
    }
    dbop_close(pDb);    
    return true;
}

wxString ceSymbolDb::GetDbRecordByKey(const wxString &key, const wxString &dir){
    wxString db = GetSymbolDbName(dir + "/symbol");
    db += ".db";
    DBOP *pDb = dbop_open(static_cast<const char*>(db), 0, 0644, DBOP_RAW);
    if (NULL == pDb){
        wxPrintf("Failed to open %s\n", db);
        return "";
    }
    wxString ret = dbop_get(pDb, static_cast<const char*>(key));
    dbop_close(pDb);
    return ret;
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
