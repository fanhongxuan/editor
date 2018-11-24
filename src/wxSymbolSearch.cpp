#include "wxSymbolSearch.hpp"
#include "ceUtils.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf
#ifdef WIN32
#define popen _popen
#define pclose _pclose
#endif
#define CTAGS_EXEC "/home/hq/fanhongxuan/editor/import/ctags/ctags/ctags"

wxSymbolSearch::wxSymbolSearch(wxWindow *parent)
:wxSearchFile(parent)
{
    SetMinStartLen(0);
}

wxSymbolSearch::~wxSymbolSearch()
{
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
    ret = outputs[0];
    outputs.clear();
    value = line.substr(pos+1);
    ceSplitString(value, outputs, '\t', false);
    if (outputs.size() < 2){
        wxPrintf("Invalid outputs size()%ld<%s>\n", outputs.size(), value);
        return false;
    }
    ret += "(" + outputs[0] + ")";
    AddSearchResult(new wxSearchFileResult(ret, ret, 0, 0));
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
    cmd += " -f - " + mFileName;
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
