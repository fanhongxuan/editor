#ifndef __WX_SEARCH_HPP__
#define __WX_SEARCH_HPP__
#include <stdint.h>
#include <vector>
#include <wx/panel.h>
#include <wx/richtext/richtextctrl.h>
#include <map>
#include <set>
class wxStaticText;
class wxSearchResult;
class wxKeyEvent;

class Edit;
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
    // when user select onte item in the search result, this function will be called.
    virtual void OnSelect(wxSearchResult &result, bool bActive) = 0;
    // before StartSearch, will call this function to prepare some long term result.
    virtual void OnPrepareResult(const wxString &input, std::vector<wxSearchResult *> &results){};
    // before UpdateSearchList, will call this function to add some temp result, which will only valid for this match.
    virtual void OnUpdateSearchResultAfter(const wxString &input, std::vector<wxSearchResult *> &results, int match){}
};
    
class wxSearch: public wxPanel
{
public:
    wxSearch(wxWindow *parent);
    ~wxSearch();
    // the following function will be called when receive a front end message.
    // void OnChangeCWD(const int8_t* pCwd); // change Current Working Directory(CWD)
    
    // the following function will be called when receive a backend message.
    void AsyncAddSearchResult(wxSearchResult *pResult); // add a item and update the status immed
    void AddSearchResult(wxSearchResult *pResult);// add a item
    void Reset();
    void ClearTempResult();
    void OnDelItem(uint8_t index);   // delete a item
    void OnPercent(uint8_t percent); // update the search percent, when the percent is 100, mean the search is finished.
                                     // when the percent is 255, mean the percent is unknown
    bool SetMinStartLen(int minStartLen);// if the input len is less than the minStartLen, will not trigger the search
    int GetMinStartLen() const{return mMinStartLen;}
    bool SetMaxCandidate(int maxCandidate); // if the candidate count is more than maxCandidate, only display maxCandidate.
    int GetMaxCandidate() const{return mMaxCandidate;}
    bool AddHandler(wxSearchHandler *pHandler);
    bool DelHandler(wxSearchHandler *pHandler);
    bool SetInput(const wxString &input); // set the default input value.    

    static bool IsTempFile(const wxString &file);    
    static bool IsBinaryFile(const wxString &file);
    
    virtual bool OnKey(wxKeyEvent &evt); // when a key is input, call this function to handle it.
    virtual bool SelectLine(int line, bool bActive, bool bRequestFocus = true);
    virtual bool ProcessEvent(wxEvent &event);
    virtual bool UpdateSearchList(const wxString &input, bool bRequestFocus = true);
    virtual bool DoStartSearch(const wxString &input);

    // the following function need to be impl.
    virtual bool StartSearch(const wxString &input, const wxString &fullInput) = 0;
    virtual bool StopSearch();
    virtual wxString GetSummary(const wxString &input, int matchCount);
    virtual wxString GetShortHelp() const;
    virtual wxString GetHelp() const;
    virtual int GetPreferedLine(const wxString &input){return -1;};
    
protected:
    wxSearchInputCtrl *mpInput;
    wxSearchListCtrl *mpList;
    wxStaticText *mpStatus;
    std::vector<wxSearchResult*> mResults;
    std::vector<wxSearchResult*> mTempResults;
    std::vector<wxSearchHandler*> mHandlers;
    std::vector<wxString> mKeys;
    wxString mInput;
    int mCount;
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
    void SetDirs(const std::set<wxString> &dirs){mDirs = dirs;}
    virtual bool StartSearch(const wxString &input, const wxString &fullInput);
    virtual bool StopSearch();
    virtual wxString GetSummary(const wxString &input, int matchCount);
    virtual wxString GetShortHelp() const;
    virtual wxString GetHelp() const;    
private:
    std::set<wxString> mDirs;
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
    wxString GetFileName() const {return mFileName;}
    void SetEdit(Edit *pEdit);
    void SetBuffer(const wxString &buffer);
    virtual wxString GetShortHelp() const;
    virtual wxString GetHelp() const;
    virtual bool StartSearch(const wxString &input, const wxString &fullInput);
    virtual bool StopSearch();
    virtual wxString GetSummary(const wxString &input, int matchCount);
    virtual int GetPreferedLine(const wxString &input);
private:
    int mCurLine;
    wxString mFileName;
    wxString mBuffer;
    Edit *mpEdit;
};


// search in a project
class wxSearchProject: public wxSearch
{
public:
    virtual bool StartSearch(const wxString &input, const wxString &fullInput);
};

#endif

