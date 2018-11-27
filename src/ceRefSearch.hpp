#ifndef __CE_REF_SEARCH_HPP__
#define __CE_REF_SEARCH_HPP__

#include "wxSearch.hpp"
#include <set>
class ceRefSearch: public wxSearchFile
{
public:
    ceRefSearch(wxWindow *parent);
    ~ceRefSearch(){};
    
    virtual bool StartSearch(const wxString &input, const wxString &fullInput);
    virtual bool StopSearch();
    virtual wxString GetSummary(const wxString &input, int matchCount);
    virtual int GetPreferedLine(const wxString &input);
    void SetTagDir(const std::set<wxString> &tagDir){mTagDir = tagDir;}
    void SetHasRef(bool bHasRef){mbHasRef = bHasRef;}
    void SetGrep(bool bGrep){mbGrep = bGrep;}
private:
    bool ParseLine(const wxString &line, const wxString &path);
private:
    bool mbHasRef;
    bool mbGrep;
    std::set<wxString> mTagDir;
};

#endif
