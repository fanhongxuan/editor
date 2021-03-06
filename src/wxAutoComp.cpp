#include "wxAutoComp.hpp"
#include <wx/wxcrtvararg.h> // for wxPrintf
#include "ceUtils.hpp"
#include "ce.hpp"
#include "ceSymbolDb.hpp"
#include <set>
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

wxString gCPreKeyWord = "#define #endif #elif #else #error #if #ifdef #ifndef #include #pragma #undef";
wxString gCKeyWord = "auto break case char const continue default do double else enum extern float for "
                     "goto if int long register return short signed sizeof static struct switch typedef "
                     "union unsigned void volatile while "
                     "inline restrict _Bool _Complex _Imaginary "
                     "_Alignas _Alignof _Atomic _Static_assert _Noreturn _Thread_local _Generic";
wxString gCppKeyWord = "asm auto bool break case catch char class const "
    "const_cast continue default delete do double dynamic_cast else enum "
    "explicit export extern false float for friend goto if inline int long "
    "mutable namespace new operator private protected public register "
    "reinterpret_cast return short signed sizeof static static_cast struct "
    "switch template this throw true try typedef typeid typename union "
    "unsigned using virtual void volatile wchar_t while";

wxString gJavaKeyWord = "abstract assert boolean break byte case catch char class const continue default do double "
    "else enum extends final finally float for goto if implements import instanceof int interface long native "
    "new package private protected public return short static strictfp super switch synchronized this throw throws "
    "transient try void volatile while";

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
    ceSplitString(gCppKeyWord, mCPPKeyWordList, ' ');
    ceSplitString(gCPreKeyWord, mCPPKeyWordList, ' ');
    ceSplitString(gJavaKeyWord, mJavaKeyWordList);
}

static bool isCpp(const wxString &name){
    wxString opt = name.Lower();
    if (opt == "c" || opt == "cpp" || opt == "cxx" || opt == "c++" || opt == "cxx"){
        return true;
    }
    return false;
}
static bool isJava(const wxString &name){
    wxString opt = name.Lower();
    if (opt == "java" || opt == ".class" || opt == ".aidl"){
        return true;
    }
    return false;
}
bool wxAutoCompProviderKeyword::IsValidForFile(const wxString &opt) const
{
    if (isCpp(opt)){
        return true;
    }
    if (isJava(opt)){
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

bool wxAutoCompProviderKeyword::GetCandidate(const wxString &input, std::set<wxString> &output, const wxString &opt, int mode)
{
    if (mode == 2){
        return false;
    }
    if (input.empty()){
        return false;
    }
    if (isCpp(opt)){
        int i = 0;
        for (i = 0; i < mCPPKeyWordList.size(); i++){
            wxString value = mCPPKeyWordList[i];
            if (value.find(input) == 0){
                value += "[Keyworkd]";
                output.insert(value);
            }
        }
    }
    else if (isJava(opt)){
        int i = 0;
        for (i = 0; i < mJavaKeyWordList.size(); i++){
            wxString value = mJavaKeyWordList[i];
            if (value.find(input) == 0){
                value += "[Keyword]";
                output.insert(value);
            }
        }
    }
    else{
        
    }
    return true;
}

static wxAutoCompWordInBufferProvider &g_wordinbuffer = wxAutoCompWordInBufferProvider::Instance();
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
    // fanhongxuan   
}

bool wxAutoCompWordInBufferProvider::IsValidForFile(const wxString &filename) const
{
    return true;
}

bool wxAutoCompWordInBufferProvider::IsValidChar(char c) const
{
    if (c == '_' || c == '@' /*|| c == '.' || c == ':' || c == '-' || c == '/' || c == '\\'*/){
        return true;
    }
    return wxAutoCompProvider::IsValidChar(c);
}

bool wxAutoCompWordInBufferProvider::GetCandidate(const wxString &input, std::set<wxString> &output, const wxString &opt, int mode)
{
    if (mode != 0){
        return false;
    }
    
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
            wxString candidate = (*sit) + "[WordInBuffer]";
            output.insert(candidate);
        }
        sit++;
    }
    return true;
}

bool wxAutoCompWordInBufferProvider::AddFileContent(const wxString &file, const wxString &opt)
{
    // todo:fanhongxuan@gmail.com
    // remove the duplicate value with wxAutoCompMemberProvider
    
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
        if (IsValidChar(file[i].GetValue())){
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


static wxAutoCompMemberProvider &gMemberProvier = wxAutoCompMemberProvider::Instance();
wxAutoCompMemberProvider *wxAutoCompMemberProvider::mpInstance = NULL;

wxAutoCompMemberProvider &wxAutoCompMemberProvider::Instance(){
    if (NULL == mpInstance){
        mpInstance = new wxAutoCompMemberProvider;
    }
    return *mpInstance;
}

wxAutoCompMemberProvider::wxAutoCompMemberProvider()
{
}

void wxAutoCompMemberProvider::SetClassName(const wxString &className, const wxString &language, const wxString &filename){
    if (className != "" && className != "__anon"){
        wxPrintf("SetClassName:<%s>(%s)\n", className, language);
    }
    mClassName = className;
    
    std::map<wxString, std::set<wxString> *>::iterator it = mCandidateMap.find(className);
    if (it != mCandidateMap.end()){
        return;
    }
    if (NULL == wxGetApp().frame()){
        return;
    }
    // wxPrintf("GetSymbols for <%s>(%s)\n", className, language);
    std::set<wxString> *pSet = new std::set<wxString>;
    std::set<ceSymbol*> symbols;
    if (!wxGetApp().frame()->GetSymbols(symbols, className, "", language, filename)){
        // note:fanhongxuan@gmail.com 
        // maybe this file is not belong to the workspace, 
        // directly return, DO NOT update the cache.
        return;
    }
    std::set<ceSymbol*>::iterator sit = symbols.begin();
    while(sit != symbols.end()){
        ceSymbol *pSymbol = (*sit);
        wxString candidate = pSymbol->ToAutoCompString();
        // wxPrintf("candidate:%s\n", candidate);
        pSet->insert(pSymbol->ToAutoCompString());
        delete pSymbol;
        pSymbol = NULL;
        sit++;
    }
    mCandidateMap[className] = pSet;
}

void wxAutoCompMemberProvider::Reset(){
    std::map<wxString, std::set<wxString> *>::iterator it = mCandidateMap.begin();
    while(it != mCandidateMap.end()){
        delete it->second;
        it++;
    }
    mCandidateMap.clear();
}

bool wxAutoCompMemberProvider::IsValidChar(char c) const
{
    // if (c == ':' || c == '-'  || c == '~' || c == '>'){
    //     return true;
    // }
    if (c == '_' || c == '~'){
        return true;
    }
    return wxAutoCompProvider::IsValidChar(c);
}

bool wxAutoCompMemberProvider::GetCandidate(const wxString &input, std::set<wxString> &output, const wxString &opt, int mode)
{
    if (mode != 1 && mode != 2){
        // note:fanhongxuan@gmail.com
        // mode 1 can include global value, mode 2 don't include global value
        return false;
    }
    
    std::map<wxString, std::set<wxString> *>::iterator it = mCandidateMap.find(mClassName);
    if (it == mCandidateMap.end()){
        return false;
    }
    wxPrintf("wxAutoCompMemberProvider::GetCandidate:class<%s>,input<%s>,mode<%d>\n", mClassName, input, mode);
    std::set<wxString> *pSet = it->second;
    std::set<wxString>::iterator sit = pSet->begin();
    bool bShowAll = false;
    // if (input.length() > 0){
    //     if (input[input.length()-1] == '.'){
    //         bShowAll = true;
    //     }
    //     if (input.length() > 1){
    //         if (input[input.length()-1] == ':' && input[input.length()-2] == ':'){
    //             bShowAll = true;
    //         }
    //         if (input[input.length()-1] == '>' && input[input.length()-2] == '-'){
    //             bShowAll = true;
    //         }
    //     }
    // }
    if (input.find_first_of(".>:") != input.npos){
        bShowAll = true;
    }
    
    if (bShowAll){
        while(sit != pSet->end()){
            // wxPrintf("All Candidate:%s;\n", (*sit));
            output.insert((*sit));
            sit++;
        }
    }
    else{
        while(sit != pSet->end()){
            if ((*sit).find(input) == 0){
                // wxPrintf("Candidate:%s;\n", (*sit));
                output.insert((*sit));
            }
            sit++;
        }        
    }
    
    if (mode == 1 && input.length() >= 3){
        // load the global value
        it = mCandidateMap.find("");
        if (it == mCandidateMap.end()){
            return true;
        }
        pSet = it->second;
        sit = pSet->begin();
        while(sit != pSet->end()){
            if ((*sit).find(input) == 0){
                output.insert(*sit);
            }
            sit++;
        }
        
        it = mCandidateMap.find("__anon");
        if (it == mCandidateMap.end()){
            return true;
        }
        pSet = it->second;
        sit = pSet->begin();
        while(sit != pSet->end()){
            // wxPrintf("__anon:%s\n", (*sit));
            if ((*sit).find(input) == 0){
                output.insert(*sit);
            }
            sit++;
        }
        
    }
    return true;
}
