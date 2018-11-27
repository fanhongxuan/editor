#ifndef __CE_REF_SEARCH_HPP__
#define __CE_REF_SEARCH_HPP__

#include "wxSearch.hpp"

class ceRefSearch: public wxSearchFile
{
public:
    ceRefSearch(wxWindow *parent);
    ~ceRefSearch(){};
    
    virtual bool StartSearch(const wxString &input, const wxString &fullInput);
    virtual bool StopSearch();
    void SetTagDir(const wxString &tagDir){mTagDir = tagDir;}
private:
    bool ParseLine(const wxString &line);
private:
    wxString mTagDir;
};

#endif
