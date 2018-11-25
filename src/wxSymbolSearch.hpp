#ifndef _WX_SYMBOL_SEARCH_HPP_
#define _WX_SYMBOL_SEARCH_HPP_

#include "wxSearch.hpp"

class wxSymbolSearch: public wxSearchFile
{
public:
    wxSymbolSearch(wxWindow *parent);
    ~wxSymbolSearch();
    
    virtual bool StartSearch(const wxString &input);
    virtual bool StopSearch();
private:
    bool OnResult(const wxString &cmd, const wxString &line);
};

#endif
