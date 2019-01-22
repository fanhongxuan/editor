#ifndef _CE_UTILS_HPP_
#define _CE_UTILS_HPP_
#include <wx/string.h>
#include <vector>
class wxEvtHandler;
int ceAsyncExec(const wxString &input, wxEvtHandler &handler, int msgId);
int ceSyncExec(const wxString &input, std::vector<wxString> &line);
int ceSplitString(const wxString &input, std::vector<wxString> &output, const wxString& sep = " ", bool allowempty = false);
void ceFindFiles(const wxString &dir, std::vector<wxString> &output);
wxString ceGetLine(const wxString &filename, long linenumber, int count = 1);
wxString ceGetExecPath();
#endif
