#include "wxAutoComp.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf

std::vector<wxAutoCompProvider*> wxAutoCompProvider::mAllProviders;

wxAutoCompProvider::wxAutoCompProvider()
{
    AddProvider(*this);
}

wxAutoCompProvider::~wxAutoCompProvider()
{
    DelProvider(*this);
}

bool wxAutoCompProvider::IsValidForFile(const wxString &filename) const
{
    return true;
}

bool wxAutoCompProvider::GetValidProvider(const wxString &filename, std::vector<wxAutoCompProvider*> &providers)
{
    int i = 0;
    for (i = 0; i < mAllProviders.size(); i++){
        if (mAllProviders[i] != NULL && mAllProviders[i]->IsValidForFile(filename)){
            providers.push_back(mAllProviders[i]);
        }
    }
    return true;
}

bool wxAutoCompProvider::AddProvider(wxAutoCompProvider &provider)
{
    mAllProviders.push_back(&provider);
    return true;
}

bool wxAutoCompProvider::DelProvider(wxAutoCompProvider &provider)
{
    int i = 0;
    for (i = 0; i < mAllProviders.size(); i++){
        if (mAllProviders[i] == &provider){
            mAllProviders[i] = NULL;
        }
    }
    return true;
}

static bool isAlphaNumber(char c)
{
    if (c >= '0' && c <= '9'){
        return true;
    }
    if (c >= 'A' && c <= 'Z'){
        return true;
    }
    if (c >= 'a' && c <= 'z'){
        return true;
    }
    return false;
}

bool wxAutoCompProvider::IsValidChar(char c) const
{
    return isAlphaNumber(c);
}

wxString gCppKeyWord = "asm auto bool break case catch char class const "
    "const_cast continue default delete do double dynamic_cast else enum "
    "explicit export extern false float for friend goto if inline int long "
    "mutable namespace new operator private protected public register "
    "reinterpret_cast return short signed sizeof static static_cast struct "
    "switch template this throw true try typedef typeid typename union "
    "unsigned unsing virtual void volatile wchar_t while";
wxString gCPreKeyWord = "#define #elif #else #error #if #ifdef #ifndef #include #pragma #undef";

wxString gJavaKeyWord = "";

extern void ParseString(const wxString &input, std::vector<wxString> &output, char sep, bool allowEmpty = false);

static wxAutoCompProviderKeyword &g_instance = wxAutoCompProviderKeyword::Instance();

wxAutoCompProviderKeyword *wxAutoCompProviderKeyword::mpInstance = NULL;
wxAutoCompProviderKeyword &wxAutoCompProviderKeyword::Instance()
{
    if (NULL == mpInstance){
        mpInstance = new wxAutoCompProviderKeyword;
    }
    return *mpInstance;
}

wxAutoCompProviderKeyword::wxAutoCompProviderKeyword()
{
    ParseString(gCppKeyWord, mCPPKeyWordList, ' ');
    ParseString(gCPreKeyWord, mCPPKeyWordList, ' ');
}

static bool isCpp(const wxString &opt){
    if (opt == "C" || opt == "CPP" || opt == "c" || opt == "cpp" || opt == "cxx" || opt == "c++" || opt == "C++"){
        return true;
    }
    return false;
}
static bool isJava(const wxString &opt){
    if (opt == "java" || opt == "JAVA" || opt == ".class"){
        return true;
    }
    return false;
}
bool wxAutoCompProviderKeyword::IsValidForFile(const wxString &opt) const
{
    if (isCpp(opt)){
        return true;
    }
    return false;
}

bool wxAutoCompProviderKeyword::IsValidChar(char c) const
{
    if (c == '#'){
        return true;
    }
    return wxAutoCompProvider::IsValidChar(c);
}

bool wxAutoCompProviderKeyword::GetCandidate(const wxString &input, std::set<wxString> &output, const wxString &opt)
{
    if (input.empty()){
        return false;
    }
    if (isCpp(opt)){
        int i = 0;
        for (i = 0; i < mCPPKeyWordList.size(); i++){
            const wxString &value = mCPPKeyWordList[i];
            if (value.find(input) != input.npos){
                output.insert(value);
            }
        }
    }
    else if (isJava(opt)){
    }
    else{
        
    }
    return true;
}

wxAutoCompWordInBufferProvider &g_wordinbuffer = wxAutoCompWordInBufferProvider::Instance();
wxAutoCompWordInBufferProvider *wxAutoCompWordInBufferProvider::mpInstance = NULL;
wxAutoCompWordInBufferProvider &wxAutoCompWordInBufferProvider::Instance()
{
    if (NULL == mpInstance){
        mpInstance = new wxAutoCompWordInBufferProvider;
    }
    return *mpInstance;
}

wxAutoCompWordInBufferProvider::wxAutoCompWordInBufferProvider()
{
}

bool wxAutoCompWordInBufferProvider::IsValidChar(char c) const
{
    if (c == '_' || c == '@' || c == '.' || c == ':' || c == '-' || c == '/' || c == '\\'){
        return true;
    }
    return wxAutoCompProvider::IsValidChar(c);
}

bool wxAutoCompWordInBufferProvider::GetCandidate(const wxString &input, std::set<wxString> &output, const wxString &opt)
{
    if (input.length() < 3){
        return false;
    }
    
    std::map<wxString, std::set<wxString> * >::iterator it = mCandidateMap.find(opt);
    std::set<wxString> *pSet = NULL;
    if (it == mCandidateMap.end()){
        pSet = new std::set<wxString>;
        mCandidateMap[opt] = pSet;
    }
    else{
        pSet = it->second;
    }
    // wxPrintf("Find Candidate for %s;opt:%s;\n", input, opt);
    std::set<wxString>::iterator sit = pSet->begin();
    while(sit != pSet->end()){
        if ((*sit).find(input) != input.npos){
            // wxPrintf("Candidate:%s;\n", (*sit));
            output.insert((*sit));
        }
        sit++;
    }
    return true;
}

bool wxAutoCompWordInBufferProvider::AddFileContent(const wxString &file, const wxString &opt)
{
    // note:fanhongxuan@gmail.com
    // when to update the content of one opt?
    // option 1:
    //    when user save one file, reparse it.
    //    when user close one file, destory it.
    // option 2:
    //    when use input one word, add it

    std::map<wxString, std::set<wxString> * >::iterator it = mCandidateMap.find(opt);
    std::set<wxString> *pSet = NULL;
    if (it == mCandidateMap.end()){
        pSet = new std::set<wxString>;
        mCandidateMap[opt] = pSet;
    }
    else{
        pSet = it->second;
    }
    if (NULL == pSet){
        return false;
    }
    // split the file content to word and add it to the buffer
    int i = 0, size = file.size();
    wxString word;
    for (i = 0; i < size; i++){
        if (IsValidChar(file[i])){
            word += file[i];
        }
        else{
            if (word.size() > 3){
                pSet->insert(word);
            }
            word = wxEmptyString;
        }
    }
    return true;
}

bool wxAutoCompWordInBufferProvider::DelFile(const wxString &file, const wxString &opt)
{
    // todo:fanhongxuan@gmail.com
    return true;
}

bool wxAutoCompWordInBufferProvider::AddCandidate(const wxString &candidate, const wxString &opt)
{
    if (candidate.length() < 3){
        // the input is two short.
        return false;
    }
    std::map<wxString, std::set<wxString> * >::iterator it = mCandidateMap.find(opt);
    std::set<wxString> *pSet = NULL;
    if (it == mCandidateMap.end()){
        pSet = new std::set<wxString>;
        mCandidateMap[opt] = pSet;
    }
    else{
        pSet = it->second;
    }
    if (NULL == pSet){
        return false;
    }
    // wxPrintf("Add new Candidate:%s;opt:%s;\n", candidate, opt);
    pSet->insert(candidate);
    return true;
}

bool wxAutoCompWordInBufferProvider::DelCandidate(const wxString &file, const wxString &opt)
{
    // todo:fanhongxuan@gmail.com
    return true;
}
