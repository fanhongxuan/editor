#include "wxAgSearch.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf
#include <wx/process.h>
#include <wx/stattext.h>

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
    :wxSearchFile(parent), mpPrevResult(NULL), mItemCount(0)
{
}

wxAgSearch::~wxAgSearch()
{
}

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
    ceSplitString(result, rets, ':');
#ifdef WIN32
#define RESULT_COUNT 4
#define FILE_NAME_INDEX 1
#define LINE_INDEX 2
#else
#define RESULT_COUNT 3
#define FILE_NAME_INDEX 0
#define LINE_INDEX 1
#endif
    if (rets.size() < RESULT_COUNT){
        return false;
    }
    
    wxString filename = rets[FILE_NAME_INDEX];
    if (FILE_NAME_INDEX > 0){
        filename = rets[0] + ":" + filename;
    }
    if (IsTempFile(filename) || IsBinaryFile(filename)){
        return false;
    }
    wxString line = rets[LINE_INDEX];
	int len = 0;
	for (int i = 0; i <= LINE_INDEX; i++){
		len += rets[i].size();
		len++;
	}
    wxString value = result.substr(len);
    if (!value.empty() && value[value.size() -1] == '\n'){
        value = value.substr(0, value.size()-1);
    }
    if (!value.empty() && value[value.size() -1] == '\r'){
        value = value.substr(0, value.size()-1);
    }
    if (!value.empty() && value[0] == ':'){
        value = value.substr(1);
    }
    rets.clear();
    ceSplitString(line, rets, ';');
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
    wxString content = wxString::Format("%s:\t%s", line, value);
    unsigned long iLen = 0;
    line.ToULong(&iLen);
#ifdef WIN32
    // note:fanhongxuan@gmail.com
    for (int i = 0; i < filename.size(); i++){
        if (filename[i] == '/'){
            filename[i] = '\\';
        }
    }
#endif
    AddSearchResult(new wxSearchFileResult(content, filename, --iLen, 0));
    return true;
}

wxString wxAgSearch::GetShortHelp() const
{
    wxString dir = "workspace";
    if (mTargetDirs.size() == 1){
        dir = "'" + *mTargetDirs.begin() + "'";
    }
    else if (mTargetDirs.empty()){
        dir = "'" + wxGetCwd() + "'";
    }
    return wxString::Format("Search in %s", dir);
}

wxString wxAgSearch::GetSummary(const wxString &input, int matchCount)
{
    wxString dir = "workspace";
    if (mTargetDirs.size() == 1){
        dir = "'" + *mTargetDirs.begin() + "'";
    }
    else if (mTargetDirs.empty()){
        dir = "'" + wxGetCwd() + "'";
    }
    return wxString::Format(wxT("Find '%s' in %s, %d%s match"), input, dir, matchCount,
        matchCount >= GetMaxCandidate() ? "+" : "");
}

bool wxAgSearch::BeforeResultMatch(const wxString &input, wxSearchResult *pRet)
{
    wxSearchFileResult *pFRet = dynamic_cast<wxSearchFileResult*>(pRet);
    if (NULL == pFRet){
        return false;
    }
    // if tne file name is different with the previous value.
    if (NULL != mpPrevResult && mpPrevResult->Target() != pRet->Target()){
        // if the file name is different with the previous value:
        EndGroup(wxString::Format(wxT("In '%s', %d item(s)"), pRet->Target(), mItemCount), true);
        mpPrevResult = NULL;
        mItemCount = 0;
    }
    if (NULL == mpPrevResult){
        BeginGroup(wxString::Format(wxT("Search '%s' in '%s'"), input, pRet->Target()), true);
        mpPrevResult = pFRet;
        mFileCount++;
    }
    mTotalCount++;
    mItemCount++;
    return true;
}

bool wxAgSearch::AfterResultMatch(const wxString &input, wxSearchResult *pRet)
{
    return true;
}

bool wxAgSearch::BeginMatch(const wxString &input)
{
    wxString dir = "workspace";
    if (mTargetDirs.size() == 1){
        dir = "'" + *mTargetDirs.begin() + "'";
    }
    else if (mTargetDirs.empty()){
        dir = "'" + wxGetCwd() + "'";
    }
    BeginGroup(wxString::Format(wxT("Search '%s' in %s"), input, dir), true);
    return true;
}

bool wxAgSearch::FinishMatch(const wxString &input)
{
    if (NULL != mpPrevResult){
        EndGroup(wxString::Format(wxT("In '%s', %d items(s)"), mpPrevResult->Target(), mItemCount), true);
        mpPrevResult = NULL;
        mItemCount = 0;
    }
    wxString dir = "workspace";
    if (mTargetDirs.size() == 1){
        dir = "'" + *mTargetDirs.begin() + "'";
    }
    else if (mTargetDirs.empty()){
        dir = "'" + wxGetCwd() + "'";
    }
    EndGroup(wxString::Format(wxT("In '%s' match %d files, %d item(s)"), dir, mFileCount, mTotalCount), true);
    mFileCount = 0;
    mTotalCount = 0;
    return true;
}

bool wxAgSearch::StartSearch(const wxString &input, const wxString &fullInput)
{
    // here call ag to search the targe dirs and prepare the result.
    if (mTargetDirs.empty()){
        mTargetDirs.insert(wxGetCwd());
    }
    mCmds.clear();
    std::set<wxString>::iterator it;
    
    mpPrevResult = NULL;
    mItemCount = 0;
    mFileCount = 0;
    mTotalCount = 0;
    
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
    static wxString ag_exec;
    if (ag_exec.empty()){
        ag_exec = ceGetExecPath();
        // note:fanhongxuan@gmail.com
        // only with --ackmate the ag output can be all captured by the pipe
#ifdef WIN32
        ag_exec += "\\ext\\ag.exe --ackmate --noheading --depth=-1 ";
#else
        ag_exec += "/ext/ag --ackmate --noheading --depth=-1 ";
#endif
    }
    while(it != mTargetDirs.end()){
        // wxPrintf("Search <%s> in dir:<%s>\n", input, (*it));
        wxString cmd = ag_exec + input;
        cmd += " \""; cmd += (*it); cmd += "\"";
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
    if (dirs.size() == mTargetDirs.size()){
        std::set<wxString>::const_iterator it = dirs.begin();
        std::set<wxString>::const_iterator sit = mTargetDirs.begin();
        bool bSame = true;
        while(it != dirs.end() && sit != mTargetDirs.end()){
            if (*it != *sit){
                bSame = false;
                break;
            }
            it++;
            sit++;
        }
        if (bSame){
            return true;
        }
    }
    mTargetDirs = dirs;
    mpStatus->SetLabel(GetShortHelp());
    ResetSearch();
    return true;
}

bool wxAgSearch::SetSearchFiles(const std::set<wxString> &files)
{
    if (files.size() == mTargetFiles.size()){
        std::set<wxString>::const_iterator it = files.begin();
        std::set<wxString>::const_iterator sit = mTargetFiles.begin();
        bool bSame = true;
        while(it != files.end() && sit != mTargetFiles.end()){
            if (*it != *sit){
                bSame = false;
                break;
            }
            it++;
            sit++;
        }
        if (bSame){
            return true;
        }
    }
    mTargetFiles = files;
    mpStatus->SetLabel(GetShortHelp());
    ResetSearch();
    return true;
}
