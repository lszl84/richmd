#include "md_frame.h"
#include <wx/sizer.h>
#include <wx/tokenzr.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/file.h>
#include <wx/config.h>

namespace {

constexpr int ID_EDITOR   = wxID_HIGHEST + 1;
constexpr int ID_DEBOUNCE = wxID_HIGHEST + 2;

constexpr int kDefSize  = 12, kH1Size = 30, kH2Size = 24, kH3Size = 19;
constexpr int kH4Size   = 16, kH5Size = 14, kH6Size = 13, kCodeSize = 11;
constexpr int kDebounceMs = 200;

const wxColour kBg          = wxColour(30,  30,  35);
const wxColour kText        = wxColour(210, 210, 215);
const wxColour kHeading     = wxColour(240, 240, 245);
const wxColour kCodeBg      = wxColour(45,  45,  52);
const wxColour kCodeText    = wxColour(200, 200, 210);
const wxColour kBlockBg     = wxColour(38,  40,  48);
constexpr int kLeftMargin = 32;  // px gutter for all text
constexpr int kListExtra  = 12;  // extra indent for lists

const wxColour kGutterColour = wxColour(80, 80, 90);  // dim # markers
const wxColour kFenceColour  = wxColour(120, 120, 130);

// ---- Markdown detectors ----

int DetectHeading(const wxString& line) {
    if (line.empty() || line[0] != '#') return 0;
    int level = 0;
    for (size_t i = 0; i < line.length() && i < 6; ++i) {
        if (line[i] == '#') ++level; else break;
    }
    if (level > 0 && level <= 6 && line.length() > (size_t)level && line[level] == ' ')
        return level;
    return 0;
}

bool IsBlockquote(const wxString& line) {
    return line.StartsWith("> ") || line == ">";
}

bool IsUnorderedList(const wxString& line) {
    wxString t = line; t.Trim(false);
    return (t.StartsWith("- ") || t.StartsWith("* ") || t.StartsWith("+ "));
}

bool IsOrderedList(const wxString& line) {
    wxString t = line; t.Trim(false);
    if (t.length() < 3) return false;
    if (t[0] < '0' || t[0] > '9') return false;
    size_t i = 0;
    while (i < t.length() && t[i] >= '0' && t[i] <= '9') ++i;
    return (i > 0 && i < t.length() && t[i] == '.' && (i+1 < t.length()) && t[i+1] == ' ');
}

bool IsFenceLine(const wxString& line) {
    wxString t = line; t.Trim(false);
    return t.StartsWith("```");
}

long FindClosing(const wxString& text, long start, const wxString& marker) {
    size_t mlen = marker.length();
    if ((size_t)start + mlen * 2 > text.length()) return -1;
    size_t pos = (size_t)start + mlen;
    while (pos + mlen <= text.length()) {
        size_t found = text.find(marker, pos);
        if (found == wxString::npos) return -1;
        if (found > 0 && text[found - 1] == '\\') { pos = found + mlen; continue; }
        if (marker == "*" && found + 1 < text.length() && text[found+1] == '*') { pos = found + 2; continue; }
        if (marker == "_" && found + 1 < text.length() && text[found+1] == '_') { pos = found + 2; continue; }
        return (long)found;
    }
    return -1;
}

void GetLineRange(const wxString& text, long pos, long& lineStart, long& lineEnd) {
    long len = (long)text.length();
    if (pos < 0) pos = 0;
    if (pos > len) pos = len;
    lineStart = pos;
    while (lineStart > 0 && text[(size_t)(lineStart - 1)] != '\n') --lineStart;
    lineEnd = pos;
    while (lineEnd < len && text[(size_t)lineEnd] != '\n') ++lineEnd;
}

}  // namespace

// ================================================================
// MDFrame
// ================================================================

MDFrame::MDFrame()
    : wxFrame(nullptr, wxID_ANY, "richmd - Markdown Editor",
              wxDefaultPosition, wxSize(800, 600)),
      debounceTimer_(this, ID_DEBOUNCE) {

    bgColour_       = kBg;
    textColour_     = kText;
    headingColour_  = kHeading;
    codeBgColour_   = kCodeBg;
    codeTextColour_ = kCodeText;

    InitFonts();
    BuildLayout();
    BuildMenuBar();

    Bind(wxEVT_TEXT,  &MDFrame::OnTextChanged,   this, ID_EDITOR);
    Bind(wxEVT_TIMER, &MDFrame::OnDebounceTimer, this, ID_DEBOUNCE);

    editor_->SetValue(
        "# richmd - Markdown Editor\n\n"
        "Type **markdown** and see it *style automatically*.\n\n"
        "## Features\n\n"
        "- Auto-headings with #\n"
        "- **Bold** and *italic* and ***both***\n"
        "- `inline code`\n"
        "- > Blockquotes\n\n"
        "## Code blocks\n\n"
        "```cpp\n"
        "int main() {\n"
        "    return 0;\n"
        "}\n"
        "```\n"
    );

    CallAfter([this] { editor_->SetFocus(); ApplyMarkdownStyles(); });
    SetMinSize(wxSize(400, 300));
}

MDFrame::~MDFrame() = default;

void MDFrame::InitFonts() {
    defFont_   = wxFont(wxFontInfo(kDefSize).Family(wxFONTFAMILY_DEFAULT));
    h1Font_    = wxFont(wxFontInfo(kH1Size).Family(wxFONTFAMILY_DEFAULT).Bold());
    h2Font_    = wxFont(wxFontInfo(kH2Size).Family(wxFONTFAMILY_DEFAULT).Bold());
    h3Font_    = wxFont(wxFontInfo(kH3Size).Family(wxFONTFAMILY_DEFAULT).Bold());
    h4Font_    = wxFont(wxFontInfo(kH4Size).Family(wxFONTFAMILY_DEFAULT).Bold());
    h5Font_    = wxFont(wxFontInfo(kH5Size).Family(wxFONTFAMILY_DEFAULT).Bold());
    h6Font_    = wxFont(wxFontInfo(kH6Size).Family(wxFONTFAMILY_DEFAULT).Bold());
    boldFont_  = wxFont(wxFontInfo(kDefSize).Family(wxFONTFAMILY_DEFAULT).Bold());
    italicFont_= wxFont(wxFontInfo(kDefSize).Family(wxFONTFAMILY_DEFAULT).Italic());
    boldItalicFont_ = wxFont(wxFontInfo(kDefSize).Family(wxFONTFAMILY_DEFAULT).Bold().Italic());
    codeFont_  = wxFont(wxFontInfo(kCodeSize).Family(wxFONTFAMILY_TELETYPE));
}

wxFont MDFrame::GetHeadingFont(int level) const {
    switch (level) {
        case 1:return h1Font_; case 2:return h2Font_;
        case 3:return h3Font_; case 4:return h4Font_;
        case 5:return h5Font_; case 6:return h6Font_;
        default:return defFont_;
    }
}

void MDFrame::BuildLayout() {
    auto* panel = new wxPanel(this);
    auto* outer = new wxBoxSizer(wxVERTICAL);
    editor_ = new wxTextCtrl(panel, ID_EDITOR, wxEmptyString,
                              wxDefaultPosition, wxDefaultSize,
                              wxTE_MULTILINE | wxTE_RICH2);
    editor_->SetFont(defFont_);
    editor_->SetBackgroundColour(bgColour_);
    editor_->SetForegroundColour(textColour_);
    outer->Add(editor_, 1, wxEXPAND);
    panel->SetSizer(outer);
}

void MDFrame::BuildMenuBar() {
    auto* menuBar = new wxMenuBar();

    // ---- File menu ----
    auto* fileMenu = new wxMenu();
    fileMenu->Append(wxID_NEW,    "&New\tCtrl+N");
    fileMenu->Append(wxID_OPEN,   "&Open\tCtrl+O");
    fileMenu->Append(wxID_SAVE,   "&Save\tCtrl+S");
    fileMenu->Append(wxID_SAVEAS, "Save &As...\tCtrl+Shift+S");
    fileMenu->AppendSeparator();
    fileHistory_.UseMenu(fileMenu);
    fileHistory_.Load(*new wxConfig("richmd"));
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tCtrl+Q");
    menuBar->Append(fileMenu, "&File");

    // Bind file-history range
    Bind(wxEVT_MENU, &MDFrame::OnFileHistory, this, wxID_FILE1, wxID_FILE9);

    // ---- Edit menu ----
    auto* editMenu = new wxMenu();
    editMenu->Append(wxID_UNDO, "&Undo\tCtrl+Z");
    editMenu->Append(wxID_REDO, "&Redo\tCtrl+Y");
    editMenu->AppendSeparator();
    editMenu->Append(wxID_CUT,   "Cu&t\tCtrl+X");
    editMenu->Append(wxID_COPY,  "&Copy\tCtrl+C");
    editMenu->Append(wxID_PASTE, "&Paste\tCtrl+V");
    editMenu->Append(wxID_SELECTALL, "Select &All\tCtrl+A");
    menuBar->Append(editMenu, "&Edit");

    SetMenuBar(menuBar);

    Bind(wxEVT_MENU, &MDFrame::OnNew,       this, wxID_NEW);
    Bind(wxEVT_MENU, &MDFrame::OnOpen,      this, wxID_OPEN);
    Bind(wxEVT_MENU, &MDFrame::OnSave,      this, wxID_SAVE);
    Bind(wxEVT_MENU, &MDFrame::OnSaveAs,    this, wxID_SAVEAS);
    Bind(wxEVT_MENU, &MDFrame::OnExit,      this, wxID_EXIT);
    Bind(wxEVT_MENU, &MDFrame::OnUndo,      this, wxID_UNDO);
    Bind(wxEVT_MENU, &MDFrame::OnRedo,      this, wxID_REDO);
    Bind(wxEVT_MENU, &MDFrame::OnCut,       this, wxID_CUT);
    Bind(wxEVT_MENU, &MDFrame::OnCopy,      this, wxID_COPY);
    Bind(wxEVT_MENU, &MDFrame::OnPaste,     this, wxID_PASTE);
    Bind(wxEVT_MENU, &MDFrame::OnSelectAll, this, wxID_SELECTALL);
}

// ---- File menu handlers ----

void MDFrame::OnNew(wxCommandEvent&) {
    if (!editor_->IsModified()) { editor_->Clear(); filePath_.clear(); UpdateTitle(); return; }
    int rc = wxMessageBox("Discard unsaved changes?", "New",
                           wxYES_NO | wxCENTRE, this);
    if (rc != wxYES) return;
    editor_->Clear();
    filePath_.clear();
    UpdateTitle();
    CallAfter([this] { ApplyMarkdownStyles(); });
}

void MDFrame::OnOpen(wxCommandEvent&) {
    wxFileDialog dlg(this, "Open Markdown", "", "",
        "Markdown (*.md;*.markdown)|*.md;*.markdown|All files (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_CANCEL) return;
    filePath_ = dlg.GetPath();
    wxFile f(filePath_);
    wxString content;
    if (f.IsOpened()) { f.ReadAll(&content); f.Close(); }
    editor_->SetValue(content);
    fileHistory_.AddFileToHistory(filePath_);
    fileHistory_.Save(*new wxConfig("richmd"));
    UpdateTitle();
    CallAfter([this] { ApplyMarkdownStyles(); });
}

void MDFrame::OnSave(wxCommandEvent&) {
    if (filePath_.empty()) { wxCommandEvent dummy; OnSaveAs(dummy); return; }
    DoSave(filePath_);
}

void MDFrame::OnSaveAs(wxCommandEvent&) {
    wxFileDialog dlg(this, "Save Markdown", "", "untitled.md",
        "Markdown (*.md)|*.md|All files (*.*)|*.*",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_CANCEL) return;
    filePath_ = dlg.GetPath();
    DoSave(filePath_);
}

void MDFrame::DoSave(const wxString& path) {
    wxFile f(path, wxFile::write);
    if (!f.IsOpened()) {
        wxMessageBox("Could not write file.", "Error", wxOK | wxCENTRE, this);
        return;
    }
    f.Write(editor_->GetValue());
    f.Close();
    fileHistory_.AddFileToHistory(path);
    fileHistory_.Save(*new wxConfig("richmd"));
    UpdateTitle();
}

void MDFrame::OnExit(wxCommandEvent&) { Close(true); }

void MDFrame::OnFileHistory(wxCommandEvent& evt) {
    wxString path = fileHistory_.GetHistoryFile(evt.GetId() - wxID_FILE1);
    if (path.empty()) return;
    filePath_ = path;
    wxFile f(filePath_);
    wxString content;
    if (f.IsOpened()) { f.ReadAll(&content); f.Close(); }
    editor_->SetValue(content);
    UpdateTitle();
    CallAfter([this] { ApplyMarkdownStyles(); });
}

void MDFrame::UpdateTitle() {
    wxString title = filePath_.empty()
        ? "richmd - Markdown Editor"
        : filePath_.AfterLast('/') + " - richmd";
    SetTitle(title);
}

// ---- Edit menu handlers ----

void MDFrame::OnUndo(wxCommandEvent&)      { editor_->Undo(); }
void MDFrame::OnRedo(wxCommandEvent&)      { editor_->Redo(); }
void MDFrame::OnCut(wxCommandEvent&)       { editor_->Cut(); }
void MDFrame::OnCopy(wxCommandEvent&)      { editor_->Copy(); }
void MDFrame::OnPaste(wxCommandEvent&)     { editor_->Paste(); }
void MDFrame::OnSelectAll(wxCommandEvent&) { editor_->SelectAll(); }

// ---- Code block helpers ----

bool MDFrame::IsInsideCodeBlock(const wxString& fullText, long lineStart) const {
    long bs, be;
    GetCodeBlockRange(fullText, lineStart, bs, be);
    return bs >= 0;
}

void MDFrame::GetCodeBlockRange(const wxString& fullText, long lineStart,
                                 long& blockStart, long& blockEnd) const {
    blockStart = blockEnd = -1;
    long totalLen = (long)fullText.length();

    // Count ``` fence lines from the start of the document up to the
    // beginning of the current line.  An odd count means we are inside
    // a code block.  Also track the position of the last fence seen
    // (the opening) and the first fence after our line (the closing).
    int fenceCount = 0;
    long lastFencePos = -1;
    long nextFencePos = -1;

    long pos = 0;
    wxStringTokenizer tkz(fullText, "\n", wxTOKEN_RET_EMPTY_ALL);

    while (tkz.HasMoreTokens()) {
        wxString line = tkz.GetNextToken();
        long lineLen = (long)line.length();
        long ls = pos;
        pos += lineLen + 1;

        if (IsFenceLine(line)) {
            if (ls < lineStart) {
                ++fenceCount;
                lastFencePos = ls;
            } else if (ls > lineStart && nextFencePos < 0) {
                nextFencePos = ls;
            }
        }
    }

    // If the current line itself is a fence, we're not inside a block.
    {
        long ls, le;
        GetLineRange(fullText, lineStart, ls, le);
        wxString curLine = fullText.Mid((size_t)ls, (size_t)(le - ls));
        if (IsFenceLine(curLine)) return;
    }

    // Odd fence count → we are inside a code block.
    if ((fenceCount % 2) == 1 && lastFencePos >= 0) {
        blockStart = lastFencePos;
        blockEnd   = (nextFencePos >= 0) ? nextFencePos : totalLen;
    }
}

// ---- Event handlers ----

void MDFrame::OnTextChanged(wxCommandEvent&) {
    if (styling_) return;
    styling_ = true;
    long caret = editor_->GetInsertionPoint();
    wxString fullText = editor_->GetValue();
    if (!fullText.empty()) {
        long lineStart, lineEnd;
        GetLineRange(fullText, caret, lineStart, lineEnd);
        ApplyLineStyle(lineStart, lineEnd, fullText);
    }
    styling_ = false;
    debounceTimer_.StartOnce(kDebounceMs);
}

void MDFrame::OnDebounceTimer(wxTimerEvent&) {
    ApplyMarkdownStyles();
}

// ---- Per-line immediate styling (called on every keystroke) ----

void MDFrame::ApplyLineStyle(long lineStart, long lineEnd,
                              const wxString& fullText) {
    if (lineStart >= lineEnd) return;
    wxString line = fullText.Mid((size_t)lineStart,
                                  (size_t)(lineEnd - lineStart));

    // Fence line (```) — dim marker
    if (IsFenceLine(line)) {
        wxTextAttr attr(kFenceColour, bgColour_, defFont_);
        editor_->SetStyle(lineStart, lineEnd, attr);
        return;
    }

    // Inside a code block?
    long cbStart, cbEnd;
    GetCodeBlockRange(fullText, lineStart, cbStart, cbEnd);
    if (cbStart >= 0 && lineStart > cbStart) {
        StyleCodeLine(lineStart, lineEnd);
        return;
    }

    int hLevel = DetectHeading(line);
    if (hLevel > 0)
        StyleHeadingLine(lineStart, lineEnd, hLevel);
    else if (IsBlockquote(line)) {
        StyleBlockquoteLine(lineStart, lineEnd);
        ApplyInlineStyles(lineStart, lineEnd, fullText);
    } else if (IsUnorderedList(line) || IsOrderedList(line)) {
        StyleListLine(lineStart, lineEnd);
        ApplyInlineStyles(lineStart, lineEnd, fullText);
    } else {
        StyleNormalLine(lineStart, lineEnd);
        ApplyInlineStyles(lineStart, lineEnd, fullText);
    }
}

// ---- Full-document styling (debounced) ----

void MDFrame::ApplyMarkdownStyles() {
    styling_ = true;
    long caretPos = editor_->GetInsertionPoint();
    wxString fullText = editor_->GetValue();
    if (fullText.empty()) { styling_ = false; return; }

    bool inCodeBlock = false;
    long pos = 0;
    wxStringTokenizer tkz(fullText, "\n", wxTOKEN_RET_EMPTY_ALL);

    while (tkz.HasMoreTokens()) {
        wxString line = tkz.GetNextToken();
        long lineLen  = (long)line.length();
        long lineStart = pos;
        long lineEnd   = pos + lineLen;

        if (IsFenceLine(line)) {
            wxTextAttr attr(kFenceColour, bgColour_, defFont_);
            editor_->SetStyle(lineStart, lineEnd, attr);
            inCodeBlock = !inCodeBlock;
        } else if (inCodeBlock) {
            StyleCodeLine(lineStart, lineEnd);
        } else if (lineLen > 0) {
            int hLevel = DetectHeading(line);
            if (hLevel > 0)
                StyleHeadingLine(lineStart, lineEnd, hLevel);
            else if (IsBlockquote(line)) {
                StyleBlockquoteLine(lineStart, lineEnd);
                ApplyInlineStyles(lineStart, lineEnd, fullText);
            } else if (IsUnorderedList(line) || IsOrderedList(line)) {
                StyleListLine(lineStart, lineEnd);
                ApplyInlineStyles(lineStart, lineEnd, fullText);
            } else {
                StyleNormalLine(lineStart, lineEnd);
                ApplyInlineStyles(lineStart, lineEnd, fullText);
            }
        } else {
            StyleNormalLine(lineStart, lineEnd);
        }
        pos += lineLen + 1;
    }

    editor_->SetInsertionPoint(caretPos);
    styling_ = false;
}

// ---- Paragraph styling ----

void MDFrame::StyleHeadingLine(long start, long end, int level) {
    wxTextAttr attr(headingColour_, bgColour_, GetHeadingFont(level));
    attr.SetLeftIndent(0, 0);
    editor_->SetStyle(start, end, attr);

    // Dim the # prefix so it visually recedes
    long prefixEnd = start + level + 1;
    if (prefixEnd > end) prefixEnd = end;
    wxTextAttr gutterAttr(kGutterColour);
    editor_->SetStyle(start, prefixEnd, gutterAttr);
}

void MDFrame::StyleBlockquoteLine(long start, long end) {
    wxTextAttr attr(textColour_, kBlockBg, defFont_);
    attr.SetLeftIndent(16, 0);
    editor_->SetStyle(start, end, attr);
}

void MDFrame::StyleListLine(long start, long end) {
    wxTextAttr attr(textColour_, bgColour_, defFont_);
    attr.SetLeftIndent(20, 4);
    editor_->SetStyle(start, end, attr);
}

void MDFrame::StyleCodeLine(long start, long end) {
    wxTextAttr attr(codeTextColour_, bgColour_, codeFont_);
    editor_->SetStyle(start, end, attr);
}

void MDFrame::StyleNormalLine(long start, long end) {
    wxTextAttr attr(textColour_, bgColour_, defFont_);
    attr.SetLeftIndent(0, 0);
    editor_->SetStyle(start, end, attr);
}

// ---- Inline styling ----

void MDFrame::ApplyInlineStyles(long paraStart, long paraEnd,
                                 const wxString& fullText) {
    if (paraStart >= paraEnd) return;
    wxString text = fullText.Mid((size_t)paraStart,
                                  (size_t)(paraEnd - paraStart));
    long i = 0, len = (long)text.length();

    while (i < len) {
        if (text[(size_t)i] == '\\' && i + 1 < len) { i += 2; continue; }

        // Inline code: `text`
        if (text[(size_t)i] == '`') {
            long close = FindClosing(text, i, "`");
            if (close > i) {
                wxTextAttr attr(codeTextColour_, codeBgColour_, codeFont_);
                editor_->SetStyle(paraStart + i, paraStart + close + 1, attr);
                i = close + 1;
                continue;
            }
        }

        // *** bold+italic ***
        if (i + 5 < len && text.Mid((size_t)i, 3) == "***") {
            long close = FindClosing(text, i, "***");
            if (close > i) {
                wxTextAttr attr(textColour_, bgColour_, boldItalicFont_);
                editor_->SetStyle(paraStart + i, paraStart + close + 3, attr);
                i = close + 3;
                continue;
            }
        }

        // **bold** or __bold__
        if (i + 3 < len && text.Mid((size_t)i, 2) == "**") {
            long close = FindClosing(text, i, "**");
            if (close > i) {
                wxTextAttr attr(textColour_, bgColour_, boldFont_);
                editor_->SetStyle(paraStart + i, paraStart + close + 2, attr);
                i = close + 2;
                continue;
            }
        }
        if (i + 3 < len && text.Mid((size_t)i, 2) == "__") {
            long close = FindClosing(text, i, "__");
            if (close > i) {
                wxTextAttr attr(textColour_, bgColour_, boldFont_);
                editor_->SetStyle(paraStart + i, paraStart + close + 2, attr);
                i = close + 2;
                continue;
            }
        }

        // *italic* or _italic_
        if (text[(size_t)i] == '*') {
            long close = FindClosing(text, i, "*");
            if (close > i) {
                wxTextAttr attr(textColour_, bgColour_, italicFont_);
                editor_->SetStyle(paraStart + i, paraStart + close + 1, attr);
                i = close + 1;
                continue;
            }
        }
        if (text[(size_t)i] == '_') {
            long close = FindClosing(text, i, "_");
            if (close > i) {
                wxTextAttr attr(textColour_, bgColour_, italicFont_);
                editor_->SetStyle(paraStart + i, paraStart + close + 1, attr);
                i = close + 1;
                continue;
            }
        }

        ++i;
    }
}
