#include "ceRefSearch.hpp"
#include "ceUtils.hpp"
#include <wx/filename.h>
#include <wx/wxcrtvararg.h> // for wxPrintf
ceRefSearch::ceRefSearch(wxWindow *parent)
:wxSearchFile(parent), mbHasRef(true), mbGrep(false)
{
    SetMaxCandidate(10000);
}

#ifdef WIN32
wxString buildGlobalCmd(const wxString &input, const wxString &dir, const wxString &opt)
{
    wxString ret, volume;
    wxFileName::SplitPath(dir, &volume, NULL, NULL, NULL);
    ret = "cmd /c \"";
    ret += " cd ";
    ret += dir;
    ret += " && ";
    ret += volume; 
    ret += ": && ";
    ret += ceGetExecPath();
    ret += "\\ext\\global.exe -x ";
    ret += " ";
    ret += opt;
    ret += " ";
    ret += input;
    return ret;
}
#else
static wxString buildGlobalCmd(const wxString &input, const wxString &dir, const wxString &opt)
{
    wxString ret = "cd ";
    ret += dir;
    ret += " && ";
    ret += ceGetExecPath();
    ret += "/ext/global";
    ret += " -x ";
    ret += " ";
    ret += opt;
    ret += " ";
    ret += input; 
    return ret;
}
#endif

wxString ceRefSearch::GetSummary(const wxString &input, int matchCount)
{
    wxString dir = "workspace";
    if (mTagDir.size() == 1){
        dir = "\'";
        dir += *(mTagDir.begin());
        dir += "\'";
    }
    if (mbGrep){
        return wxString::Format(wxT("Grep '%s' in %s, %d match"), input, dir, matchCount);
    }
    else if (mbHasRef){
        return wxString::Format(wxT("Find '%s' in %s, %d match"), input, dir, matchCount);
    }
    else{
        return wxString::Format(wxT("Find defination of '%s' in %s, %d match"), input, dir, matchCount);
    }
}

bool ceRefSearch::StartSearch(const wxString &input, const wxString &fullInput)
{
    if (mTagDir.empty()){
        wxPrintf("Empty tag file\n");
        return false;
    }

    // todo:fanhongxuan@gmail.com
    // if the current has no GTAGS, generate it first.
    std::set<wxString>::iterator it = mTagDir.begin();
    while(it != mTagDir.end()){
        int count = 0;
        std::vector<wxString> outputs;
        wxString cmd;
        if (mbGrep){
            cmd = buildGlobalCmd(fullInput, (*it), "-g");
        }
        else{
            cmd = buildGlobalCmd(fullInput, (*it), "-d");
        }
        ceSyncExec(cmd, outputs);
        for (int i = 0; i < outputs.size(); i++){
            count++;
            wxSearchFileResult *pRet = ParseLine(outputs[i], (*it));
            if (NULL != pRet){
                AddSearchResult(pRet);
            }
        }
        if ((!mbGrep) && mbHasRef){
            cmd = buildGlobalCmd(fullInput, (*it), "-r");
            outputs.clear();
            ceSyncExec(cmd, outputs);
            for (int i = 0; i < outputs.size(); i++){
                count++;
                wxSearchFileResult *pRet = ParseLine(outputs[i], (*it));
                if (NULL != pRet){
                    AddSearchResult(pRet);
                }
            }
            if (1){
                // in this case try -s (other symbol, which mean don't find the defination.
                cmd = buildGlobalCmd(fullInput, (*it), "-s");
                outputs.clear();
                ceSyncExec(cmd, outputs);
                for (int i = 0; i < outputs.size(); i++){
                    wxSearchFileResult *pRet = ParseLine(outputs[i], (*it));
                    if (NULL != pRet){
                        AddSearchResult(pRet);
                    }
                }
            }
        }
        it++;
    }
    return true;
}

bool ceRefSearch::StopSearch()
{
    return true;
}

int ceRefSearch::GetPreferedLine(const wxString &input)
{
    if (mbHasRef || mbGrep){
        return -1;
    }
    return wxSearchFile::GetPreferedLine(input);
}

bool ceRefSearch::FindDef(const wxString &symbol, std::vector<wxSearchFileResult *> &results)
{
    if (mTagDir.empty()){
        wxPrintf("Empty tag file\n");
        return false;
    }
    
    std::set<wxString>::iterator it = mTagDir.begin();
    while(it != mTagDir.end()){
        int count = 0;
        std::vector<wxString> outputs;
        wxString cmd;
        cmd = buildGlobalCmd(symbol, (*it), "-d");
        ceSyncExec(cmd, outputs);
        for (int i = 0; i < outputs.size(); i++){
            count++;
            wxSearchFileResult *pRet = ParseLine(outputs[i], (*it));
            if (NULL != pRet){
                results.push_back(pRet);
            }
        }
        it++;
    }
    
    // if no definitions find, maybe there is not impl in the workspace
    // try othersymbols in header files.
    if (!results.empty()){
        return true;
    }
    it = mTagDir.begin();
    while(it != mTagDir.end()){
        int count = 0;
        std::vector<wxString> outputs;
        wxString cmd;
        cmd = buildGlobalCmd(symbol, (*it), "-s");
        ceSyncExec(cmd, outputs);
        for (int i = 0; i < outputs.size(); i++){
            count++;
            wxSearchFileResult *pRet = ParseLine(outputs[i], (*it));
            if (NULL != pRet){
                wxString filename = pRet->Target();
                wxString ext;
                wxFileName::SplitPath(filename, NULL, NULL, NULL, &ext);
                if (ext.find("h") != ext.npos){
                    results.push_back(pRet);
                }
                else{
                    delete pRet;
                }
            }
        }
        it++;
    }
}

wxSearchFileResult *ceRefSearch::ParseLine(const wxString &line, const wxString &path)
{
    // wxPrintf("ParseLine:<%s>\n", line);
    std::vector<wxString> outputs;
    ceSplitString(line, outputs, " ");
    if (outputs.size() < 4){
        wxPrintf("Invalid input:<%s>\n", line);
        return NULL;
    }
    wxString lineNumber = outputs[1];
    wxString filename = outputs[2];
    wxString value;
    int pos = line.find(filename);
    if (pos == line.npos){
        return NULL;
    }
    pos += filename.size();
    value = line.substr(pos + 1);
    value = filename + "(" + lineNumber + "): " + value;
    long iLen = 0;
    lineNumber.ToLong(&iLen);
    return new wxSearchFileResult(value, path + "/"+ filename, --iLen, 0);
}

