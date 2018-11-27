#include "ceRefSearch.hpp"
#include "ceUtils.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf
ceRefSearch::ceRefSearch(wxWindow *parent)
:wxSearchFile(parent)
{
}

// note:fanhongxuan@gmail.com
// need to support more than one tag dir
bool ceRefSearch::StartSearch(const wxString &input, const wxString &fullInput)
{
    if (mTagDir.empty()){
        wxPrintf("Empty tag file\n");
        return false;
    }
    static wxString global_exec;
    if (global_exec.empty()){
        global_exec = ceGetExecPath();
#ifdef WIN32
        global_exec += "\\ext\\global.exe";
#else
        global_exec += "/ext/global";
#endif
        std::vector<wxString> outputs;
        wxString cmd = "GTAGSROOT=\"";
        cmd += mTagDir + "\" ";
        cmd += global_exec;
        cmd += " -x ";
        cmd += fullInput;
        wxPrintf("Exec:<%s>\n", cmd);
        ceSyncExec(cmd, outputs);
        for (int i = 0; i < outputs.size(); i++){
            //wxPrintf("outputs[%d]=<%s>\n", i, outputs[i]);
            ParseLine(outputs[i]);
        }
    }
    return true;
}

bool ceRefSearch::StopSearch()
{
    return true;
}

bool ceRefSearch::ParseLine(const wxString &line)
{
    std::vector<wxString> outputs;
    ceSplitString(line, outputs, " ");
    if (outputs.size() < 4){
        wxPrintf("Invalid input:<%s>\n", line);
        return false;
    }
    wxString lineNumber = outputs[1];
    wxString filename = outputs[2];
    wxString value;
    int pos = line.find(filename);
    if (pos == line.npos){
        return false;
    }
    pos += filename.size();
    value = line.substr(pos + 1);
    value = filename + "(" + lineNumber + "): " + value;
    long iLen = 0;
    lineNumber.ToLong(&iLen);
    AddSearchResult(new wxSearchFileResult(value, filename, iLen, 0));
    return true;
}

