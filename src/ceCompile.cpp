#include "ceCompile.hpp"
#include <wx/textdlg.h>
#include <wx/wxcrtvararg.h> // for wxPrintf
#include <wx/filename.h>
#include "ceUtils.hpp"
#include "ce.hpp"
wxBEGIN_EVENT_TABLE(ceCompile, wxStyledTextCtrl)
EVT_STC_STYLENEEDED(wxID_ANY, ceCompile::OnStyleNeeded)
// EVT_STC_AUTOCOMP_SELECTION(wxID_ANY, ceCompile::OnAutoCompSelection)
// EVT_STC_CALLTIP_CLICK(wxID_ANY, ceCompile::OnCallTipClick)
// EVT_STC_DWELLSTART(wxID_ANY, ceCompile::OnDwellStart)
// EVT_STC_DWELLEND(wxID_ANY, ceCompile::OnDwellEnd)
// EVT_STC_MODIFIED(wxID_ANY,    ceCompile::OnModified)
// EVT_STC_MARGINCLICK(wxID_ANY, ceCompile::OnMarginClick)
// EVT_STC_UPDATEUI(wxID_ANY, ceCompile::OnUpdateUI)
// EVT_STC_CHARADDED(wxID_ANY,  ceCompile::OnCharAdded)
// EVT_STC_ZOOM(wxID_ANY, ceCompile::OnZoom)
// EVT_MENU(ceEdit_Update_Indent_By_Fold_Level, ceCompile::OnUpdateIndentByFolder)
EVT_SET_FOCUS(ceCompile::OnFocus)
// EVT_KILL_FOCUS(ceCompile::OnKillFocus)
EVT_KEY_DOWN(ceCompile::OnKeyDown )
// EVT_KEY_UP(ceCompile::OnKeyUp)
EVT_LEFT_DOWN(ceCompile::OnLeftDown)
// EVT_LEFT_UP(ceCompile::OnMouseLeftUp)
// EVT_LEFT_DCLICK(ceCompile::OnMouseLeftDclick)
// EVT_MOUSEWHEEL(ceCompile::OnMouseWheel)
// EVT_SIZE(ceCompile::OnSize)
// EVT_TIMER(ceEdit_CallTip_Timer, ceCompile::OnTimer)
// EVT_TIMER(ceEdit_Idle_Timer, ceCompile::OnIdleTimer)
EVT_THREAD(wxID_ANY, ceCompile::OnResult)
wxEND_EVENT_TABLE();

enum{
    COMPILE_STYLE_NORMAL,
    COMPILE_STYLE_WARN,
    COMPILE_STYLE_ERROR,
    COMPILE_STYLE_CMD,
};

ceCompile::ceCompile(wxWindow *parent)
:wxStyledTextCtrl(parent)
{
    SetWrapMode(wxSTC_WRAP_WORD);
    SetMarginType (0, wxSTC_MARGIN_NUMBER);
    SetLexer(wxSTC_LEX_CONTAINER);
    wxFontInfo info;
    info.Bold(false);
    for (int i = 0; i <= wxSTC_STYLE_LASTPREDEFINED; i++) {
        StyleSetFont(i, wxFont(info));
        StyleSetBackground(i, *wxBLACK);
        StyleSetForeground(i, *wxWHITE);
    }
    info.Bold();
    info.Underlined();
    StyleSetFont(COMPILE_STYLE_ERROR, wxFont(info));
    StyleSetFont(COMPILE_STYLE_WARN, wxFont(info));
    StyleSetForeground(COMPILE_STYLE_WARN, wxColor("yellow"));
    StyleSetForeground(COMPILE_STYLE_ERROR, wxColor("red"));
    StyleSetForeground(COMPILE_STYLE_CMD, wxColor("green"));
    StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColor("green"));
}

void ceCompile::OnKeyDown(wxKeyEvent &evt){
    // do nothing.
}

void ceCompile::OnLeftDown(wxMouseEvent &evt)
{
    wxStyledTextCtrl::OnMouseLeftDown(evt);
    int pos = GetCurrentPos();
    int style = GetStyleAt(pos);
    if (style != COMPILE_STYLE_ERROR && style != COMPILE_STYLE_WARN){
        return;
    }
    int line = LineFromPosition(pos);
    wxString text = GetLineText(line);
    wxPrintf("Target:<%s>\n", text);
    // format is :filename:line:columns:
    std::vector<wxString> outputs;
    ceSplitString(text, outputs, ":");
    if (outputs.size() >= 3){
        wxString filename = outputs[0];
        filename = mWorkDir + filename;
        long line;
        outputs[1].ToLong(&line);
        wxPrintf("Open file<%s>\n", filename);
        wxGetApp().frame()->OpenFile(filename, filename, true, line-1);
    }
}

bool ceCompile::IsWarn(int linenumber)
{
    return true;
}

bool ceCompile::IsError(int linenumber)
{
    return true;
}

void ceCompile::OnStyleNeeded(wxStyledTextEvent &evt)
{
    int startPos = GetEndStyled();
    int stopPos = evt.GetPosition();
    int startLine = LineFromPosition(startPos);
    int stopLine = LineFromPosition(stopPos);
    startPos = XYToPosition(0, startLine);
    stopPos = GetLineEndPosition(stopLine);
    wxPrintf("OnStyleNeeded:<%d-%d>\n", startLine, stopLine);
    // StartStyling(startPos);
    // SetStyling(stopPos - startPos, COMPILE_STYLE_NORMAL);
    for (int i = startLine; i <= stopLine; i++){
        wxString text = GetLineText(i);
        size_t offset = text.find(": error:");
        if (offset > 0 && offset != text.npos){
            // this is a error line
            startPos = XYToPosition(0, i);
            StartStyling(startPos);
            SetStyling(offset, COMPILE_STYLE_ERROR);
            startPos = startPos + offset;
            continue;
        }
        offset = text.find(": warning:");
        if (offset > 0 && offset != text.npos){
            startPos = XYToPosition(0, i);
            StartStyling(startPos);
            SetStyling(offset, COMPILE_STYLE_WARN);
            startPos = startPos + offset;
            // this is a warning line;
            continue;
        }
        offset = text.find(": note:");
        if (offset > 0 && offset != text.npos){
            startPos = XYToPosition(0, i);
            StartStyling(startPos);
            SetStyling(offset, COMPILE_STYLE_WARN);
            startPos = startPos + offset;
            continue;
        }
        offset = text.find_first_of(" \t\r\n:");
        if (offset > 0 && offset != text.npos && text[offset] == ':'){
            startPos = XYToPosition(0, i);
            StartStyling(startPos);
            SetStyling(offset, COMPILE_STYLE_CMD);
            startPos = startPos + offset;
            continue;
        }
        else if (offset != text.npos){
            wxPrintf("(line:%d)text[%d] = <%c>\n", i+1, offset, text[offset]);
        }
    }
    StartStyling(startPos);
    SetStyling(stopPos - startPos, COMPILE_STYLE_NORMAL);
}

static wxString UpdateDir(const wxString &cwd, const wxString &cd)
{
    wxString ret = cwd;
    int i = 0, len = cd.length();
    i = cd.find("cd ");
    for (; i < len; i++){
        if (cd[i] == '&'){
            break;
        }
        if (cd[i] == '.' && (i+1 < len ) && cd[i+1] == '.'){
            // meet ..
            size_t pos = ret.find_last_of("/\\");
            if (pos != ret.npos){
                ret = ret.substr(0, pos);
            }
        }
        else if (cd [i] == '.'){
            // meet .
            // skip this
        }
        else if (cd [i] == '/' || cd[i] == '\\'){
            if (ret[ret.length()-1] != '/'){
                ret += '/';
            }
        }
        else if (cd [i] == ' '){
            // skip this;
        }
        else{
            ret += cd[i];
        }
    }
    return ret;
}

void ceCompile::Compile(const wxString &filename)
{
    ClearAll();
    
    UpdateLineNumberMargin();
    wxString msg;
    wxString path;
    wxFileName::SplitPath(filename, &path, NULL, NULL);
    
    // todo:fanhongxuan@gmail.com
    // 1, prompt user to save all the unmodified file
    // 2, Get the compile cmd for this file.
    wxString cmd = GetCompileCmd(filename);
    
    // todo:fanhongxuan@gmail.com
    // if the cmd has cd, we need th change the path according the cd command.
    if (cmd.find("cd ") != cmd.npos){
        mWorkDir = UpdateDir(path, cmd);
    }
    else{
        mWorkDir = path;
    }
    if ((!mWorkDir.empty()) && mWorkDir[mWorkDir.size() - 1] != '/'){
        mWorkDir += "/";
    }
    mStartTime = time(NULL);
    wxDateTime now(mStartTime);
    wxString time_str = now.FormatDate();
    
    time_str += " ";
    time_str += now.FormatTime();
    msg = wxString::Format(wxT("Start: Compile <%s>\nDefaultDir:<%s>\nTime:<%s>\n"), filename, mWorkDir, time_str);
    AppendText(msg);
    
    
    AppendText(wxString::Format(wxT("Run:<%s>\n"), cmd));
    cmd = "cd " + path + " && " + cmd;
    // 3, If previous build is not complete, first stop it.
    // 4, Async run the command and get the output.
    ceAsyncExec(cmd, *this, wxID_ANY);
    // 5, highlight the error and warning.
}

void ceCompile::UpdateLineNumberMargin(){
    int lineCount = GetLineCount();
    wxString linenumber = wxT("_");
    while(lineCount != 0){
        linenumber += wxT("9");
        lineCount = lineCount / 10;
    }
    SetMarginWidth(0, TextWidth (wxSTC_STYLE_LINENUMBER, linenumber));
}


void ceCompile::OnResult(wxThreadEvent &evt)
{
    // wxPrintf("OnResult:<%s>\n", evt.GetString());
    wxString str = evt.GetString();
    if (str.empty()) // stop
    {
        if (evt.GetInt() == 0){
            AppendText(wxT("Compile: Completed\n"));
        }
        else{
            AppendText(wxT("Compile: Failed\n"));
        }
        wxString msg;
        wxDateTime now(time(NULL));
        wxString time_str = now.FormatDate();
        time_str += " ";
        time_str += now.FormatTime();
        msg = wxString::Format(wxT("Time:<%s>\n"), time_str);
        AppendText(msg);
        wxDateTime used(time(NULL) - mStartTime);
        time_str = used.Format("%M:%s");
        
        msg = wxString::Format(wxT("Use:<%s>\n"), time_str);
        AppendText(msg);
    }
    else if (!str.empty()){
        AppendText(str);
    }
    
    GotoLine(GetLineCount()-1);
    
    // if (str.empty()){
    //     AppendText(wxT("Compile completed"));
    // }
    // else{
    //     AppendText(str);
    // }
    
    UpdateLineNumberMargin();
}

wxString ceCompile::GetCompileCmd(const wxString &filename)
{
    wxString ret;
    std::map<wxString, wxString>::iterator it = mCompileCmds.find(filename);
    if (it != mCompileCmds.end()){
        ret = it->second;
    }
    if (ret.empty()){
        ret = "make";
    }
    
    wxTextEntryDialog dlg(this, wxT("Input the compile cmd\n"), wxT("Input the compile cmd"), ret);
    if (dlg.ShowModal() != wxID_OK){
        ret = "";
        return ret;
    }
    ret = dlg.GetValue();
    mCompileCmds[filename] = ret;
    return ret;
}

void ceCompile::OnFocus(wxFocusEvent &evt)
{
    // mbHasFocus = true;
    if (NULL != wxGetApp().frame()){
        wxGetApp().frame()->DoUpdate();
    }
    evt.Skip();
}
