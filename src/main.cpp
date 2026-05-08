#include <wx/wx.h>
#include "md_frame.h"

class RichMDApp : public wxApp {
public:
    bool OnInit() override {
        SetAppName("richmd");
        SetVendorName("richmd");
        auto* frame = new MDFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(RichMDApp);
