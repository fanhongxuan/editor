#include "wxSymbolSearch.hpp"
#include "ceUtils.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf

wxSymbolSearch::wxSymbolSearch(wxWindow *parent)
:wxSearchFile(parent)
{
    SetMinStartLen(0);
    SetMaxCandidate(1000);
}

wxSymbolSearch::~wxSymbolSearch()
{
}


const wxString &getFullTypeName(const wxString &input, const wxString &language)
{
    static std::map<wxString, wxString> theCMaps;
    static std::map<wxString, wxString> theJavaMaps;
    if (theCMaps.size() == 0){
        theCMaps['A'] = "Alias";
        theCMaps['L'] = "Label";
        theCMaps['N'] = "Name";
        theCMaps['U'] = "Using";
        theCMaps['c'] = "Class";
        theCMaps['d'] = "Macro";
        theCMaps['e'] = "Enumerator";
        theCMaps['f'] = "Function";
        theCMaps['g'] = "Enum";
        theCMaps['h'] = "Header";
        theCMaps['l'] = "Local";
        theCMaps['m'] = "Member";
        theCMaps['n'] = "Namespace";
        theCMaps['p'] = "Prototype";
        theCMaps['s'] = "Struct";
        theCMaps['t'] = "Typedef";
        theCMaps['u'] = "Union";
        theCMaps['v'] = "Variable";
        theCMaps['x'] = "Externvar";
        theCMaps['z'] = "Parameter";
    }
    if (theJavaMaps.size() == 0){
    }
    if (language == "C++"){
        std::map<wxString, wxString>::iterator it = theCMaps.find(input);
        if (it != theCMaps.end()){
            return it->second;
        }
        else{
            return input;
        }
    }
}

bool wxSymbolSearch::OnResult(const wxString &cmd, const wxString &line)
{
    int pos = line.find_last_of('"');
    if (pos == line.npos){
        wxPrintf("No \" found\n");
        return false;
    }
    wxString ret, value = line.substr(0, pos);
    std::vector<wxString> outputs;
    ceSplitString(value, outputs, '\t', false);
    if (outputs.size() < 3){
        wxPrintf("Invalid outputs size():%ld<%s>\n", outputs.size(), value);
        return false;
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
        return false;
    }
    wxString type = getFullTypeName(outputs[0], "C++");
    if (outputs.size() >= 2){
        pos = outputs[1].find("class:");
        if (pos != wxString::npos){
            symbol = outputs[1].substr(pos + strlen("class:")) + "::" + symbol;
        }
    }
    ret = symbol + "(" + type + ")";
    // todo:fanhongxuan@gmail.com
    // sort all the symbol according the value of ret.
    AddSearchResult(new wxSearchFileResult(ret, ret, iLineNumber, 0));
    return true;
}

static wxString findCtags()
{
    wxString ret = ceGetExecPath();
#ifdef WIN32
	ret += "\\ext\\ctags -f - -n --kinds-C++=+z+l ";
#else
    // ret += "/ext/ctags -f - -n --kinds-C++=+z+l ";
    ret += "/ext/ctags -f - -n ";
#endif
    return ret;
}

wxString wxSymbolSearch::GetSummary(const wxString &input, int matchCount)
{
    if (input.find_first_not_of("\r\n\t ") == input.npos){
        return wxString::Format(wxT("Total %d symbols"), matchCount);
    }
    return wxSearchFile::GetSummary(input, matchCount);
}

bool wxSymbolSearch::StartSearch(const wxString &input, const wxString &fullInput)
{
    // call ctags to generate the tags info and store it.
    wxPrintf("wxSymbolSearch:StartSearch<%s>\n", input);
    if (GetFileName().empty()){
        return false;
    }
    static wxString ctags_exec;
    if (ctags_exec.empty()){
        ctags_exec = findCtags();
    }
    wxString cmd = ctags_exec + GetFileName();
    
    std::vector<wxString> outputs;
    ceSyncExec(cmd, outputs);
    int i = 0;
    for (i = 0; i < outputs.size(); i++){
        OnResult(cmd, outputs[i]);
    }
    return true;
}

bool wxSymbolSearch::StopSearch()
{
    return true;
}
