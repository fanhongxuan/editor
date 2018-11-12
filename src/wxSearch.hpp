#ifndef __WX_SEARCH_HPP__
#define __WX_SEARCH_HPP__
#include <stdint.h>
#include <vector>
#include <wx/panel.h>
#include <wx/richtext/richtextctrl.h>
#include <map>
class wxStaticText;
class wxSearchResult;
class wxKeyEvent;

class wxSearchInputCtrl;
class wxSearchListCtrl;

class wxSearchResult
{
public:
    wxSearchResult(const wxString &content, const wxString &target, void *pCustomData = NULL);
    virtual ~wxSearchResult(){} // note:fanhongxuan@gmail.com, dynamic_cast need at least one virtual function
    bool IsMatch() const{return mbMatch;}
    bool IsInRange(long position) const { return mRange.GetStart() <= position && mRange.GetEnd() >= position;}
    bool ConvertToRichText(wxSearchListCtrl &rich, const std::vector<wxString> &rets, bool bEnableHighlight);
    const wxString &Content() const{return mContent;}
    const wxString &Target() const{return mTarget;}
    void *CustomData() const{return mpCustomData;}
private:
    // void ParseString(const wxString &input, std::vector<wxString> &output, char sep) const;
    int IsMatch(int pos, const std::map<int, int> &match) const;
private:
    void *mpCustomData;
    bool mbMatch;
    wxString mContent; // content to display on the search result
    wxString mLowContent; // content in lowcase
    wxString mTarget;  // a uri to the target, maybe a file and a line number etc.
    wxRichTextRange mRange; // used to to select a range
};

class wxSearchHandler
{
public:
    virtual void OnSelect(wxSearchResult &result, bool bActive) = 0;
    virtual void OnPrepareResult(const wxString &input, std::vector<wxSearchResult *> &results){};
};
    
class wxSearch: public wxPanel
{
public:
    wxSearch(wxWindow *parent);
    ~wxSearch();
    // the following function will be called when receive a front end message.
    void OnChangeCWD(const int8_t* pCwd); // change Current Working Directory(CWD)
    
    // the following function will be called when receive a backend message.
    void AddSearchResult(wxSearchResult *pResult);// add a item
    void Reset();
    void OnDelItem(uint8_t index);   // delete a item
    void OnPercent(uint8_t percent); // update the search percent, when the percent is 100, mean the search is finished.
                                     // when the percent is 255, mean the percent is unknown
    bool SetMinStartLen(int minStartLen);// if the input len is less than the minStartLen, will not trigger the search
    bool SetMaxCandidate(int maxCandidate); // if the candidate count is more than maxCandidate, only display maxCandidate.
    bool AddHandler(wxSearchHandler *pHandler);
    bool DelHandler(wxSearchHandler *pHandler);
    bool SetInput(const wxString &input); // set the default input value.
    
    virtual bool OnKey(wxKeyEvent &evt); // when a key is input, call this function to handle it.
    virtual bool SelectLine(int line, bool bActive, bool bRequestFocus = true);
    virtual bool ProcessEvent(wxEvent &event);
    virtual bool UpdateSearchList(const wxString &input);
    virtual bool DoStartSearch(const wxString &input);

    // the following function need to be impl.
    virtual bool StartSearch(const wxString &input) = 0;
    virtual bool StopSearch();
    virtual wxString GetSummary(const wxString &input, int matchCount);
    virtual wxString GetShortHelp() const;
    virtual wxString GetHelp() const;
    
private:
    wxSearchInputCtrl *mpInput;
    wxSearchListCtrl *mpList;
    wxStaticText *mpStatus;
    std::vector<wxSearchResult*> mResults;
    std::vector<wxSearchHandler*> mHandlers;
    std::vector<wxString> mKeys;
    int mCurrentLine;
    int mMinStartLen;
    int mMaxCandidate;
    bool mbStartSearch;
};

// search in a local dir
class wxSearchDir: public wxSearch
{
public:
    wxSearchDir(wxWindow *parent);
    void SetDir(const wxString &dir){mDir = dir;}
    virtual bool StartSearch(const wxString &input);
    virtual bool StopSearch();
    virtual wxString GetSummary(const wxString &input, int matchCount);
    virtual wxString GetShortHelp() const;
    virtual wxString GetHelp() const;
private:
    wxString mDir;
};

// search in a local file
class wxSearchFileResult: public wxSearchResult
{
public:
    wxSearchFileResult(const wxString &content, const wxString &target, int line, int pos);
    int GetLine() const{return mLine;}
    int GetPos() const{return mPos;}
private:
    int mLine;
    int mPos;
};

class wxSearchFile: public wxSearch
{
public:
    wxSearchFile(wxWindow *parent);
    void SetFileName(const wxString &fileName);
    void SetBuffer(const wxString &buffer);
    
    virtual wxString GetShortHelp() const;
    virtual wxString GetHelp() const;
    virtual bool StartSearch(const wxString &input);
    virtual bool StopSearch();
    virtual wxString GetSummary(const wxString &input, int matchCount);
private:
    wxString mFileName;
    wxString mBuffer;
};


// search in a project
class wxSearchProject: public wxSearch
{
public:
    virtual bool StartSearch(const wxString &input);
};

#endif

