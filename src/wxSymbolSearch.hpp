#ifndef _WX_SYMBOL_SEARCH_HPP_
#define _WX_SYMBOL_SEARCH_HPP_

#include "wxSearch.hpp"

class wxSymbolSearch: public wxSearchFile
{
public:
    wxSymbolSearch(wxWindow *parent);
    ~wxSymbolSearch();
    
    virtual bool StartSearch(const wxString &input, const wxString &fullInput);
    virtual bool StopSearch();
    virtual wxString GetSummary(const wxString &input, int matchCount);
private:
    bool OnResult(const wxString &cmd, const wxString &line);
};

#endif
