#include "wxAgSearch.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf
#include <wx/process.h>

#include "ceUtils.hpp"
class MyProcess: public wxProcess
{
    wxAgSearch *mpParent;
    wxString mCmd;
public:
    MyProcess(wxAgSearch *parent, const wxString &cmd)
    :wxProcess(parent), mCmd(cmd)
    {
        Redirect();
        mpParent = parent;
    }
    
    bool HasInput();
    virtual void OnTerminate(int pid, int state);
};

bool 
MyProcess::HasInput()
{
    bool hasInput = false;
    
    if (IsInputAvailable()){
        wxInputStream *in = GetInputStream();
        wxTextInputStream tis(*in);
        if (NULL != mpParent){
            mpParent->OnResult(mCmd, tis.ReadLine());
        }
        
        //wxPrintf("%s\n", tis.ReadLine());
        hasInput = true;
    }
    return hasInput;
}

void MyProcess::OnTerminate(int pid, int state)
{
    while(HasInput());
    wxPrintf("Capture all the output\n");
    wxProcess::OnTerminate(pid, state);
}

wxAgSearch::wxAgSearch(wxWindow *parent)
    :wxSearchFile(parent)
{
}

wxAgSearch::~wxAgSearch()
{
}

extern void ParseString(const wxString &input, std::vector<wxString> &output, char sep, bool allowEmpty = false);
bool wxAgSearch::OnResult(const wxString &cmd, const wxString &result)
{
    //wxPrintf("OnResult:%s\n", result);
    if (mCmds.find(cmd) == mCmds.end()){
        return false;
    }
    if (result.size() <= 1){
        return false;
    }
    std::vector<wxString> rets;
    ParseString(result, rets, ':');
    if (rets.size() < 3){
        return false;
    }
    
    wxString filename = rets[0];
    if (IsTempFile(filename) || IsBinaryFile(filename)){
        return false;
    }
    wxString line = rets[1];
    wxString value = result.substr(rets[0].size() + rets[1].size() + 2 /*two : */);
    if (!value.empty() && value[value.size() -1] == '\n'){
        value = value.substr(0, value.size()-1);
    }
    if (!value.empty() && value[value.size() -1] == '\r'){
        value = value.substr(0, value.size()-1);
    }
    rets.clear();
    ParseString(line, rets, ';');
    if (rets.empty()){
        return false;
    }
    line = rets[0];
    // todo:fanhongxuan@gmail.com
    // if we search in workspace, remove the workspace dir from the filename
    // if there are more than one directyr in the targetDirs, we need to make the user known which dir it is.
    wxString name = filename;
    std::set<wxString>::iterator it = mTargetDirs.begin();
    while(it != mTargetDirs.end()){
        if (name.find((*it)) == 0){
            name = name.substr((*it).length());
            if (name.length() > 0 && (name[0] == '/' || name[0] == '\\')){
                name = name.substr(1);
            }
            break;
        }
        it++;
    }
    wxString content = name + "(" + line + ")" + value;
    unsigned long iLen = 0;
    line.ToULong(&iLen);
    AddSearchResult(new wxSearchFileResult(content, filename, --iLen, 0));
    return true;
}

bool wxAgSearch::StartSearch(const wxString &input)
{
    // here call ag to search the targe dirs and prepare the result.
    if (mTargetDirs.empty()){
        mTargetDirs.insert(wxGetCwd());
    }
    mCmds.clear();
    std::set<wxString>::iterator it;
    
    it = mTargetFiles.begin();
    while(it != mTargetFiles.end()){
        // wxPrintf("Search <%s> in file:<%s>\n", input, (*it));
        wxString cmd = "ag --ackmate --noheading --depth=-1 ";
        cmd += input; cmd += " \""; cmd += (*it); cmd += "\"";
        wxPrintf("Execute:%s\n", cmd);
        mCmds.insert(cmd);
        MyProcess *process = new MyProcess(this, cmd);
        wxExecute(cmd, wxEXEC_ASYNC, process);
        it++;
    }
    
    it = mTargetDirs.begin();
    while(it != mTargetDirs.end()){
        // wxPrintf("Search <%s> in dir:<%s>\n", input, (*it));
        // note:fanhongxuan@gmail.com
        // only with --ackmate the ag output can be all captured by the pipe.
#ifdef WIN32            
        wxString cmd = "f:\\editor\\import\\ag\\ag.exe --ackmate --noheading --depth=-1 ";
#else
        wxString cmd = "ag --ackmate --noheading --depth=-1 ";
#endif    
        cmd += input; cmd += " \""; cmd += (*it); cmd += "\"";
        mCmds.insert(cmd);
        //MyProcess *process = new MyProcess(this, cmd);
        //long ret = wxExecute(cmd, wxEXEC_ASYNC, process);
        std::vector<wxString> output;
        ceSyncExec(cmd, output);
        int i = 0; 
        for (i = 0; i < output.size(); i++){
            OnResult(cmd, output[i]);
        }
        it++;
    }
    return true;
}

bool wxAgSearch::StopSearch()
{
    // todo:fanhongxuan@gmail.com
    // stop all the running task
    mCmds.clear();
    return true;
}

bool wxAgSearch::SetSearchDirs(const std::set<wxString> &dirs)
{
    mTargetDirs = dirs;
    return true;
}

bool wxAgSearch::SetSearchFiles(const std::set<wxString> &files)
{
    mTargetFiles = files;
    return true;
}
