#include "ceUtils.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf

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
	ceSplitString(outputBuffer, output, '\n');
	return   TRUE;   
}   
#endif

int ceSyncExec(const wxString &cmd, std::vector<wxString> &output)
{
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
            //wxPrintf("Read:%s\n", line);
            //OnResult(cmd, line);
            wxString strLine = line;
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

int ceSplitString(const wxString &input, std::vector<wxString> &output, char sep, bool allowEmpty)
{
    size_t begin = 0;
    size_t end = 0;
    while(begin != input.npos && begin < input.length()){
        end = input.find_first_of(sep, begin);
        if (end == input.npos){
            output.push_back(input.substr(begin));
            break;
        }
        else{
            if (begin != end || allowEmpty){
                output.push_back(input.substr(begin, (end - begin)));
            }
            begin = end + 1;
        }
    }
	return output.size();
}
