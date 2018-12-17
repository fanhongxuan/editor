#ifndef __WX_AUTO_COMP__
#define __WX_AUTO_COMP__

#include <vector>
#include <map>
#include <set>
#include <wx/string.h>
class wxAutoCompProvider
{
public:
    wxAutoCompProvider();
    ~wxAutoCompProvider();
    
    // find the candidate list according the input
    virtual bool GetCandidate(const wxString &input, std::set<wxString> &output, const wxString &opt) = 0;
    virtual bool IsValidForFile(const wxString &filename) const;
    virtual bool IsValidChar(char c)const;

public:
    static bool GetValidProvider(const wxString &filename, std::vector<wxAutoCompProvider*> &providers);
    static bool AddProvider(wxAutoCompProvider &provider);
    static bool DelProvider(wxAutoCompProvider &provider);
private:
    static std::vector<wxAutoCompProvider*> mAllProviders;
};

class wxAutoCompProviderAllWord: public wxAutoCompProvider
{
public:
    virtual bool GetCandidate(const wxString &input, std::set<wxString> &output, const wxString &opt);
};

class wxAutoCompProviderKeyword: public wxAutoCompProvider
{
public:
    static wxAutoCompProviderKeyword &Instance();
    virtual bool GetCandidate(const wxString &input, std::set<wxString> &output, const wxString &opt);
    virtual bool IsValidChar(char c) const;
    virtual bool IsValidForFile(const wxString &filename) const;
    
private:
    wxAutoCompProviderKeyword();
    wxAutoCompProviderKeyword(const wxAutoCompProviderKeyword &other);
    wxAutoCompProviderKeyword &operator=(const wxAutoCompProviderKeyword &other);
private:
    std::vector<wxString> mCPPKeyWordList;
        std::vector<wxString> mJavaKeyWordList;
    static wxAutoCompProviderKeyword *mpInstance;
};

class wxAutoCompWordInBufferProvider: public wxAutoCompProvider
{
public:
    static wxAutoCompWordInBufferProvider &Instance();
    virtual bool GetCandidate(const wxString &input, std::set<wxString> &output, const wxString &opt);
        virtual bool IsValidForFile(const wxString &filename) const;
    bool AddFileContent(const wxString &file, const wxString &opt);
    bool DelFile(const wxString &file, const wxString &opt);
    bool AddCandidate(const wxString &word, const wxString &opt);
    bool DelCandidate(const wxString &word, const wxString &opt);
    bool IsValidChar(char c) const;
private:
    std::map<wxString, std::set<wxString> *> mCandidateMap;
    wxAutoCompWordInBufferProvider();
    
    wxAutoCompWordInBufferProvider(const wxAutoCompWordInBufferProvider &other);
    wxAutoCompWordInBufferProvider &operator=(const wxAutoCompWordInBufferProvider &other);
    
    static wxAutoCompWordInBufferProvider *mpInstance;
};

    class wxAutoCompMemberProvider: public wxAutoCompProvider
    {
    public:
        static wxAutoCompMemberProvider &Instance();
        virtual bool GetCandidate(const wxString &input, std::set<wxString> &output, const wxString &opt);
        virtual bool IsValidChar(char c) const;
        void SetClassName(const wxString &name, const wxString &language, const wxString &filename);
        void Reset();
    private:
        std::map<wxString, std::set<wxString> *> mCandidateMap;
        wxString mClassName;
    private:
        wxAutoCompMemberProvider();
        wxAutoCompMemberProvider (const wxAutoCompMemberProvider &other);
        wxAutoCompMemberProvider &operator=(const wxAutoCompMemberProvider &other);
        static wxAutoCompMemberProvider *mpInstance;
    };
// todo:fanhongxuan@gmail.com
// add wxAutoCompSystmIncludeProvider

#endif /*__WX_AUTO_COMP__*/

