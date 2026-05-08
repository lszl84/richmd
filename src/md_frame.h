#pragma once
#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/filehistory.h>

class MDFrame : public wxFrame {
public:
    MDFrame();
    ~MDFrame() override;

private:
    wxTextCtrl* editor_ = nullptr;
    wxTimer     debounceTimer_;
    bool        styling_ = false;
    wxString    filePath_;
    wxFileHistory fileHistory_{10};

    // Fonts (proportional for body/headings, monospace for code)
    wxFont defFont_;
    wxFont h1Font_, h2Font_, h3Font_, h4Font_, h5Font_, h6Font_;
    wxFont boldFont_;
    wxFont italicFont_;
    wxFont boldItalicFont_;
    wxFont codeFont_;

    // Colours (selected from light/dark palette at construction)
    wxColour bgColour_;
    wxColour textColour_;
    wxColour headingColour_;
    wxColour codeBgColour_;
    wxColour codeTextColour_;
    wxColour blockBgColour_;
    wxColour fenceColour_;

    void BuildLayout();
    void BuildMenuBar();
    void InitFonts();
    void OnTextChanged(wxCommandEvent&);
    void OnDebounceTimer(wxTimerEvent&);
    void ApplyMarkdownStyles();
    void ApplyLineStyle(long lineStart, long lineEnd, const wxString& fullText);

    // Paragraph styling
    void StyleHeadingLine(long start, long end, int level);
    void StyleBlockquoteLine(long start, long end);
    void StyleListLine(long start, long end);
    void StyleCodeLine(long start, long end);
    void StyleNormalLine(long start, long end);

    // Inline styling
    void ApplyInlineStyles(long paraStart, long paraEnd, const wxString& fullText);

    // Code-block helpers
    bool IsInsideCodeBlock(const wxString& fullText, long lineStart) const;
    void GetCodeBlockRange(const wxString& fullText, long lineStart,
                           long& blockStart, long& blockEnd) const;

    wxFont GetHeadingFont(int level) const;

    // File menu
    void OnNew(wxCommandEvent&);
    void OnOpen(wxCommandEvent&);
    void OnSave(wxCommandEvent&);
    void OnSaveAs(wxCommandEvent&);
    void OnExit(wxCommandEvent&);
    void OnFileHistory(wxCommandEvent&);
    void DoSave(const wxString& path);
    void UpdateTitle();

    // Edit menu
    void OnUndo(wxCommandEvent&);
    void OnRedo(wxCommandEvent&);
    void OnCut(wxCommandEvent&);
    void OnCopy(wxCommandEvent&);
    void OnPaste(wxCommandEvent&);
    void OnSelectAll(wxCommandEvent&);
};
