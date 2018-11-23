#ifndef _CE_UTILS_HPP_
#define _CE_UTILS_HPP_
#include <wx/string.h>
#include <vector>
int ceSyncExec(const wxString &input, std::vector<wxString> &line);
int ceSplitString(const wxString &input, std::vector<wxString> &output, char sep = ' ', bool allowempty = false);
#endif
