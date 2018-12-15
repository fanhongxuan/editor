#ifndef _CE_SYMBOL_DB_HPP_
#define _CE_SYMBOL_DB_HPP_

#include <set>
#include <map>
#include <wx/string.h>

class ceSymbol
{
public:
    ceSymbol(){}
    virtual ~ceSymbol(){}
    
    bool operator==(const ceSymbol &other);
    bool IsSame(const ceSymbol &other);
public:
    wxString symbolType; // function, class etc, enum,
    wxString type; // for function, this is the return value type, for variable, this is the type.
    wxString name; // text name,
    wxString desc; // 
    wxString file; // abs filename
    wxString shortFilename; // short filename
        wxString scope;
        wxString access;
        wxString param;
        wxString lineNumber;
        wxString lineEnd;
    int line;
};

class ceSymbolDb
{
public:
    ceSymbolDb();
    virtual ~ceSymbolDb();
    
    bool GetSymbolByType(std::set<ceSymbol*> &symbols, const wxString &type, const wxString &filename = "");
    
    wxString GetType(const wxString &symbol, const wxString &type, const wxString &filename = "");
    bool IsType(const wxString &symbol, const wxString &type, const wxString &filename = "");
    
    bool GetLocalVariableByFile(std::vector<ceSymbol*> &symbols, const wxString &filename);
    bool GetParamtersByFile(std::set<ceSymbol*> &symbols, const wxString &filename);
    
        bool FindDef(std::set<ceSymbol *> &symbols, 
            const wxString &name, 
            const wxString &className = "", 
            const wxString &type = "", 
            const wxString &filename = "",
            const wxString &dir = "");
        
    bool UpdateSymbol(const std::set<wxString> &dirs);
    bool UpdateSymbolByDir(const wxString &dir);
    bool UpdateSymbolByFile(const wxString &filename);
    
    static bool GetFileSymbol(const wxString &file, std::set<ceSymbol*> &symbols);
        
    void SetRootDir(const wxString &rootdir){mRootDir = rootdir;}
    wxString GetRootDir() const{return mRootDir;}
private:
    wxString GetSymbolDbName(const wxString &source_filename);
    static ceSymbol *ParseLine(const wxString &line, const wxString &name, const wxString &type, std::set<wxString> &files);
private:
    wxString mRootDir;
    // note:fanhongxuan@gmail.com
    // we store all the symbol in single file.
    // all those map don't inclue the parameter and local variable, those info is build on fly.
    // std::map<std::string, std::set<ceSymbol*> > mSymbolFileMap; // key is the filename
    // std::map<std::string, std::set<ceSymbol*> > mSymbolTypeMap; // key is the type, function, class, struct, etc,
    // std::map<std::string, std::set<ceSymbol*> > mTypeMember; // store all the member variable/function of class/struct.
    // todo:fanhongxuan@gmail.com
    // for class, we need known the class tree.
};

#endif