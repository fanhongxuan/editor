#include "ceUtils.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf
#include <wx/dir.h>
#include "wxSearch.hpp"
#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#ifdef WIN32
#define popen _popen
#define pclose _pclose
#include <Windows.h>
// todo:fanhongxuan@gmail.com
// the io speed is two slow in windows, need to fix it later.
BOOL system_hide(const wchar_t* CommandLine, std::vector<wxString> &output)   
{   
	SECURITY_ATTRIBUTES   sa;   
	HANDLE   hRead,hWrite;   
 
	sa.nLength   =   sizeof(SECURITY_ATTRIBUTES);   
	sa.lpSecurityDescriptor   =   NULL;   
	sa.bInheritHandle   =   TRUE;   
	if   (!CreatePipe(&hRead,&hWrite,&sa,0))     
	{   
		return   FALSE;   
	}
 
	STARTUPINFO   si;   
	PROCESS_INFORMATION   pi;     
	si.cb   =   sizeof(STARTUPINFO);   
	GetStartupInfo(&si);     
	si.hStdError   =   hWrite;   
	si.hStdOutput   =   hWrite;   
	si.wShowWindow   =   SW_HIDE;   
	si.dwFlags   =   STARTF_USESHOWWINDOW   |   STARTF_USESTDHANDLES;   
	//关键步骤，CreateProcess函数参数意义请查阅MSDN   
	if   (!CreateProcess(NULL, (LPWSTR)CommandLine, NULL,NULL,TRUE,NULL,NULL,NULL,&si,&pi))     
	{   
		return   FALSE;   
	}   
	CloseHandle(hWrite);   
 
	wxString outputBuffer;
	char   buffer[4096]   =   {0};   
	DWORD   bytesRead;     
	while(true)     
	{   
		memset(buffer,0,strlen(buffer));
		if(ReadFile(hRead,buffer,4095,&bytesRead,NULL)==NULL)   
			break;   
		//buffer中就是执行的结果，可以保存到文本，也可以直接输出   
		//printf(buffer);//这行注释掉就可以了   
		//output.push_back(
		outputBuffer += buffer;
		//Sleep(100);     
	} 
	ceSplitString(outputBuffer, output, "\r\n");
	return   TRUE;   
}   
#endif

int ceSyncExec(const wxString &cmd, std::vector<wxString> &output)
{
    wxPrintf("exec:<%s>\n", cmd);
    wxString rCmd = cmd;
    rCmd += " 2>&1";
#ifdef WIN32
    system_hide(static_cast<const wchar_t*>(cmd.c_str()), output);
#else
    FILE *fp = popen(cmd.c_str(), "r");
    if (NULL != fp){
        char buffer[1024];
        while(1){
            char *line = fgets(buffer, 1024, fp);
            if (line == NULL){
                break;
            }
            // fixme:fanhongxuan@gmail.com
            // if the line include invalid charsec, will convert to a null string.
            // wxPrintf("Read:%s\n", line);
            // OnResult(cmd, line);
            wxString strLine = wxString::FromUTF8(line);
            if (strLine[strLine.size() -1 ] == '\n'){
                strLine = strLine.substr(0, strLine.size() -1);
            }
            output.push_back(strLine);
        }
        pclose(fp);
    }
#endif
    return 0;
}

class MyThread: public wxThread
{
    wxEvtHandler *mpHandler;
    wxString mCmd;
    int mMsgId;
public:
    MyThread(const wxString &cmd, wxEvtHandler *pHandler, int msgId){
        mpHandler = pHandler;
        mCmd = cmd;
        mMsgId = msgId;
    }
    ~MyThread(){}
protected:
    virtual ExitCode Entry(){
        FILE *fp = popen(mCmd.c_str(), "r");
        if (NULL != fp){
            char buffer[1024];
            while(1){
                char *line = fgets(buffer, 1024, fp);
                if (line == NULL){
                    break;
                }
                
                wxString strLine = wxString::FromUTF8(line);
                std::cout << strLine << std::endl;
                //wxPrintf("strLine:<%s>\n", strLine);
                // output.push_back(strLine);
                if (NULL != mpHandler){
                    wxThreadEvent *pEvt = new wxThreadEvent();
                    pEvt->SetString(strLine);
                    pEvt->SetId(mMsgId);
                    pEvt->SetInt(0);
                    mpHandler->QueueEvent(pEvt);
                }
            }
            int ret = pclose(fp);
            if (NULL != mpHandler){
                wxThreadEvent *pEvt = new wxThreadEvent();
                pEvt->SetId(mMsgId);
                pEvt->SetInt(ret);
                mpHandler->QueueEvent(pEvt);
            }
        }
        // notify the result.
        return 0;
    }
};

int ceAsyncExec(const wxString &cmd, wxEvtHandler &handler, int msgId){
    // start a new thread to run the cmd
    wxString iCmd = cmd + " 2>&1";
    MyThread *pThread = new MyThread(iCmd, &handler, msgId);
    if (pThread->Run() != wxTHREAD_NO_ERROR){
        delete pThread;
        return -1;
    }
    return 0;
}

int ceSplitString(const wxString &input, std::vector<wxString> &output, const wxString &sep, bool allowEmpty)
{
    size_t begin = 0;
    size_t end = 0;
    while(begin != input.npos && begin < input.length()){
        end = input.find(sep, begin);
        if (end == input.npos){
            output.push_back(input.substr(begin));
            break;
        }
        else{
            if (begin != end || allowEmpty){
                output.push_back(input.substr(begin, (end - begin)));
            }
			begin = end + sep.length();
        }
    }
	return output.size();
}

wxString ceGetExecPath()
{
    char buffer[1024+1] = {0};
	wxString path;
	size_t pos = 0;
#ifdef WIN32
	// GetModuleFileName(NULL, (LPWSTR)buffer, 1024);
	path = _pgmptr;
	pos = path.find_last_of("\\");
#else
	readlink("/proc/self/exe", buffer, 1024);
	path = buffer;
    pos = path.find_last_of("/");
#endif
	if (pos != path.npos){
        path = path.substr(0, pos);
    }
    return path;
}

void ceFindFiles(const wxString &dir, std::vector<wxString> &output)
{
    wxDir cwd(dir);
    if (!cwd.IsOpened()){
        return;
    }
    wxString filename;
    bool cont = false;
    cont = cwd.GetFirst(&filename, "*", wxDIR_DIRS | wxDIR_HIDDEN);
    while(cont){
        wxString child_dir = cwd.GetNameWithSep() + filename;
        if (filename != ".git"){
            ceFindFiles(child_dir, output);
        }
        cont = cwd.GetNext(&filename);
    }

    cont = cwd.GetFirst(&filename, "*", wxDIR_FILES | wxDIR_HIDDEN);
    while(cont){
        // skip the edit temp file like:
        // test~ (emacs auto save)
        // #test# (emacs auto save)
        if ((!wxSearch::IsTempFile(filename)) && (!wxSearch::IsBinaryFile(filename))){
            output.push_back(cwd.GetNameWithSep() + filename);
        }
        cont = cwd.GetNext(&filename);
    }
}

wxString ceGetLine(const wxString &filename, long linenumber, int count)
{
    FILE *fp = fopen(filename, "r");
    if (NULL == fp){
        return wxEmptyString;
    }
    char buffer[1024+1];
    int i = 1;
    wxString ret;
    while(i++ < linenumber){
        fgets(buffer, 1024, fp);
    }
    i = 0;
    while (i++ < count){
        fgets(buffer, 1024, fp);
        ret += buffer;
    }
    if (count == 1){
        size_t pos = ret.find_first_of("\r\n");
        if (pos != ret.npos){
            ret = ret.substr(0, pos);
        }
    }
    fclose(fp);
    return ret;
}
