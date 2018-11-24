#include "wxSymbolSearch.hpp"
#include "ceUtils.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf
#ifdef WIN32
#define popen _popen
#define pclose _pclose
#endif
#define CTAGS_EXEC "/home/fhx/code/editor/import/ctags/ctags/ctags -f - -n "

wxSymbolSearch::wxSymbolSearch(wxWindow *parent)
:wxSearchFile(parent)
{
    SetMinStartLen(0);
}

wxSymbolSearch::~wxSymbolSearch()
{
}


static const wxString &getFullTypeName(const wxString &input)
{
    static std::map<wxString, wxString> theTypeMaps;
    if (theTypeMaps.size() == 0){
        theTypeMaps["f"] = "Function";
        theTypeMaps["v"] = "Variable";
        theTypeMaps["d"] = "Define";
        theTypeMaps["m"] = "Member";
        theTypeMaps["c"] = "Class";
        theTypeMaps["e"] = "Enum";
    }
    std::map<wxString, wxString>::iterator it = theTypeMaps.find(input);
    if (it != theTypeMaps.end()){
        return it->second;
    }
    else{
        return input;
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
    if (outputs.size() < 2){
        wxPrintf("Invalid outputs size()%ld<%s>\n", outputs.size(), value);
        return false;
    }
    wxString type = getFullTypeName(outputs[0]);
    pos = outputs[1].find("class:");
    if (pos != wxString::npos){
        symbol = outputs[1].substr(pos + strlen("class:")) + "::" + symbol;
    }
    ret = symbol + "(" + type + ")";
    // todo:fanhongxuan@gmail.com
    // sort all the symbol according the value of ret.
    AddSearchResult(new wxSearchFileResult(ret, ret, iLineNumber, 0));
    return true;
}

bool wxSymbolSearch::StartSearch(const wxString &input)
{
    // call ctags to generate the tags info and store it.
    wxPrintf("wxSymbolSearch:StartSearch<%s>\n", input);
    if (mFileName.empty()){
        return false;
    }
    wxString cmd = CTAGS_EXEC;
    cmd += mFileName;
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
