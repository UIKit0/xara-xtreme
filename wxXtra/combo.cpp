// $Id: odcombo.cpp 751 2006-03-31 15:43:49Z alex $
/* @@tag:xara-cn-tp@@ THIRD PARTY COPYRIGHT */
// The following line makes normalize.pl skip type fixing
/* SKIPFIXTYPES: START */

/////////////////////////////////////////////////////////////////////////////
// Name:        combocmn.cpp
// Purpose:     wxComboControlBase
// Author:      Jaakko Salli
// Modified by:
// Created:     Apr-30-2006
// RCS-ID:
// Copyright:   (c) 2005 Jaakko Salli
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include <wx/wx.h>

#include "combo.h"
#if wxWXXTRA_COMBOCONTROL

#include <wx/renderer.h>

// constants
// ----------------------------------------------------------------------------

// Milliseconds to wait for two mouse-ups after focus inorder
// to trigger a double-click.
#define DOUBLE_CLICK_CONVERSION_TRESHOLD        500

#define DEFAULT_DROPBUTTON_WIDTH                19

#define BMP_BUTTON_MARGIN                       4

#define DEFAULT_POPUP_HEIGHT                    200

#define DEFAULT_TEXT_INDENT                     3

#define COMBO_MARGIN                            2 // spacing right of wxTextCtrl


#if defined(__WXMSW__)

#define USE_TRANSIENT_POPUP           1 // Use wxPopupWindowTransient (preferred, if it works properly on platform)

//#undef wxUSE_POPUPWIN
//#define wxUSE_POPUPWIN 0

#elif defined(__WXGTK__)

#define USE_TRANSIENT_POPUP           1 // Use wxPopupWindowTransient (preferred, if it works properly on platform)

#elif defined(__WXMAC__)

#define USE_TRANSIENT_POPUP           0 // Use wxPopupWindowTransient (preferred, if it works properly on platform)

#else

#define USE_TRANSIENT_POPUP           0 // Use wxPopupWindowTransient (preferred, if it works properly on platform)

#endif


// Popupwin is really only supported on wxMSW (not WINCE) and wxGTK, regardless
// what the wxUSE_POPUPWIN says.
// FIXME: Why isn't wxUSE_POPUPWIN reliable any longer? (it was in wxW2.6.2)
#if (!defined(__WXMSW__) && !defined(__WXGTK__)) || defined(__WXWINCE__)
#undef wxUSE_POPUPWIN
#define wxUSE_POPUPWIN 0
#endif


#if wxUSE_POPUPWIN
    #include "wx/popupwin.h"
#else
    #undef USE_TRANSIENT_POPUP
    #define USE_TRANSIENT_POPUP 0
#endif


#if USE_TRANSIENT_POPUP

    #define wxComboPopupWindowBase wxPopupTransientWindow
    #define INSTALL_TOPLEV_HANDLER       0

#elif wxUSE_POPUPWIN

    #define wxComboPopupWindowBase wxPopupWindow
    #define INSTALL_TOPLEV_HANDLER       1

#else

    #define wxComboPopupWindowBase wxDialog
    #define INSTALL_TOPLEV_HANDLER      0 // Doesn't need since can monitor active event

#endif



//
// ** TODO **
// * wxComboPopupWindow for external use (ie. replace old wxUniv wxPopupComboWindow)
//


// ----------------------------------------------------------------------------
// wxComboFrameEventHandler takes care of hiding the popup when events happen
// in its top level parent.
// ----------------------------------------------------------------------------

#if INSTALL_TOPLEV_HANDLER

//
// This will no longer be necessary after wxTransientPopupWindow
// works well on all platforms.
//

class wxComboFrameEventHandler : public wxEvtHandler
{
public:
    wxComboFrameEventHandler( wxComboControlBase* pCb );
    ~wxComboFrameEventHandler();

    void OnPopup();

    void OnIdle( wxIdleEvent& event );
    void OnMouseEvent( wxMouseEvent& event );
    void OnActivate( wxActivateEvent& event );
    void OnResize( wxSizeEvent& event );
    void OnMove( wxMoveEvent& event );
    void OnMenuEvent( wxMenuEvent& event );
    void OnClose( wxCloseEvent& event );

protected:
    wxWindow*                       m_focusStart;
    wxComboControlBase*     m_combo;

private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxComboFrameEventHandler, wxEvtHandler)
    EVT_IDLE(wxComboFrameEventHandler::OnIdle)
    EVT_LEFT_DOWN(wxComboFrameEventHandler::OnMouseEvent)
    EVT_RIGHT_DOWN(wxComboFrameEventHandler::OnMouseEvent)
    EVT_SIZE(wxComboFrameEventHandler::OnResize)
    EVT_MOVE(wxComboFrameEventHandler::OnMove)
    EVT_MENU_HIGHLIGHT(wxID_ANY,wxComboFrameEventHandler::OnMenuEvent)
    EVT_MENU_OPEN(wxComboFrameEventHandler::OnMenuEvent)
    EVT_ACTIVATE(wxComboFrameEventHandler::OnActivate)
    EVT_CLOSE(wxComboFrameEventHandler::OnClose)
END_EVENT_TABLE()

wxComboFrameEventHandler::wxComboFrameEventHandler( wxComboControlBase* combo )
    : wxEvtHandler()
{
    m_combo = combo;
}

wxComboFrameEventHandler::~wxComboFrameEventHandler()
{
}

void wxComboFrameEventHandler::OnPopup()
{
    m_focusStart = ::wxWindow::FindFocus();
}

void wxComboFrameEventHandler::OnIdle( wxIdleEvent& event )
{
    wxWindow* winFocused = ::wxWindow::FindFocus();

    wxWindow* popup = m_combo->GetPopupControl();
    wxWindow* winpopup = m_combo->GetPopupWindow();

    if (
         winFocused != m_focusStart &&
         winFocused != popup &&
         winFocused->GetParent() != popup &&
         winFocused != winpopup &&
         winFocused->GetParent() != winpopup &&
         winFocused != m_combo &&
         winFocused != m_combo->GetButton() // GTK (atleast) requires this
        )
    {
        m_combo->HidePopup();
    }

    event.Skip();
}

void wxComboFrameEventHandler::OnMenuEvent( wxMenuEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

void wxComboFrameEventHandler::OnMouseEvent( wxMouseEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

void wxComboFrameEventHandler::OnClose( wxCloseEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

void wxComboFrameEventHandler::OnActivate( wxActivateEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

void wxComboFrameEventHandler::OnResize( wxSizeEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

void wxComboFrameEventHandler::OnMove( wxMoveEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

#endif // INSTALL_TOPLEV_HANDLER

// ----------------------------------------------------------------------------
// wxComboPopupWindow is wxPopupWindow customized for
// wxComboControl.
// ----------------------------------------------------------------------------

class wxComboPopupWindow : public wxComboPopupWindowBase
{
public:

    wxComboPopupWindow( wxComboControlBase *parent, int style = wxBORDER_NONE );

#if USE_TRANSIENT_POPUP
    virtual bool ProcessLeftDown(wxMouseEvent& event);
#endif

    void OnKeyEvent(wxKeyEvent& event);

    void OnMouseEvent( wxMouseEvent& event );
#if !wxUSE_POPUPWIN
    void OnActivate( wxActivateEvent& event );
#endif

protected:

#if USE_TRANSIENT_POPUP
    virtual void OnDismiss();
#endif

private:
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(wxComboPopupWindow, wxComboPopupWindowBase)
    EVT_MOUSE_EVENTS(wxComboPopupWindow::OnMouseEvent)
#if !wxUSE_POPUPWIN
    EVT_ACTIVATE(wxComboPopupWindow::OnActivate)
#endif
    EVT_KEY_DOWN(wxComboPopupWindow::OnKeyEvent)
    EVT_KEY_UP(wxComboPopupWindow::OnKeyEvent)
END_EVENT_TABLE()


wxComboPopupWindow::wxComboPopupWindow( wxComboControlBase *parent,
                                        int style )
#if wxUSE_POPUPWIN
                                       : wxComboPopupWindowBase(parent,style)
#else
                                       : wxComboPopupWindowBase(parent,
                                                                wxID_ANY,
                                                                wxEmptyString,
                                                                wxPoint(-21,-21),
                                                                wxSize(20,20),
                                                                style)
#endif
{
}

void wxComboPopupWindow::OnKeyEvent( wxKeyEvent& event )
{
    // Relay keyboard event to the main child controls
    // (just skipping may just cause the popup to close)
    wxWindowList children = GetChildren();
    wxWindowList::iterator node = children.begin();
    wxWindow* child = (wxWindow*)*node;
    child->AddPendingEvent(event);
}

void wxComboPopupWindow::OnMouseEvent( wxMouseEvent& event )
{
    event.Skip();
}

#if !wxUSE_POPUPWIN
void wxComboPopupWindow::OnActivate( wxActivateEvent& event )
{
    if ( !event.GetActive() )
    {
        // Tell combo control that we are dismissed.
        wxComboControl* combo = (wxComboControl*) GetParent();
        wxASSERT( combo );
        wxASSERT( combo->IsKindOf(CLASSINFO(wxComboControl)) );

        combo->HidePopup();

        event.Skip();
    }
}
#endif

#if USE_TRANSIENT_POPUP
bool wxComboPopupWindow::ProcessLeftDown(wxMouseEvent& event )
{
    return wxComboPopupWindowBase::ProcessLeftDown(event);
}
#endif

#if USE_TRANSIENT_POPUP
// First thing that happens when a transient popup closes is that this method gets called.
void wxComboPopupWindow::OnDismiss()
{
    wxComboControlBase* combo = (wxComboControlBase*) GetParent();
    wxASSERT_MSG( combo->IsKindOf(CLASSINFO(wxComboControlBase)),
                  wxT("parent might not be wxComboControl, but check IMPLEMENT_DYNAMIC_CLASS(2) macro for correctness") );

    combo->OnPopupDismiss();
}
#endif

// ----------------------------------------------------------------------------
// wxComboPopup
//
// ----------------------------------------------------------------------------

wxComboPopup::~wxComboPopup()
{
}

void wxComboPopup::OnPopup()
{
}

void wxComboPopup::OnDismiss()
{
}

wxSize wxComboPopup::GetAdjustedSize( int minWidth,
                                      int prefHeight,
                                      int WXUNUSED(maxHeight) )
{
    return wxSize(minWidth,prefHeight);
}

void wxComboPopup::PaintComboControl( wxDC& dc, const wxRect& rect )
{
    if ( m_combo->GetWindowStyle() & wxCB_READONLY ) // ie. no textctrl
    {
        m_combo->DrawFocusBackground(dc,rect,0);

        dc.DrawText( GetStringValue(),
                     rect.x + m_combo->GetTextIndent(),
                     (rect.height-dc.GetCharHeight())/2 + m_combo->m_widthCustomBorder );
    }
}

void wxComboPopup::OnComboKeyEvent( wxKeyEvent& event )
{
    event.Skip();
}

void wxComboPopup::OnComboDoubleClick()
{
}

void wxComboPopup::SetStringValue( const wxString& WXUNUSED(value) )
{
}

bool wxComboPopup::LazyCreate()
{
    return false;
}

void wxComboPopup::Dismiss()
{
    m_combo->HidePopup();
}

// ----------------------------------------------------------------------------
// input handling
// ----------------------------------------------------------------------------

//
// This is pushed to the event handler queue of either combo box
// or its textctrl (latter if not readonly combo).
//
class wxComboBoxExtraInputHandler : public wxEvtHandler
{
public:

    wxComboBoxExtraInputHandler( wxComboControlBase* combo )
        : wxEvtHandler()
    {
        m_combo = combo;
    }
    ~wxComboBoxExtraInputHandler() { }
    void OnKey(wxKeyEvent& event);
    void OnFocus(wxFocusEvent& event);

protected:
    wxComboControlBase*   m_combo;

private:
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(wxComboBoxExtraInputHandler, wxEvtHandler)
    EVT_KEY_DOWN(wxComboBoxExtraInputHandler::OnKey)
    EVT_SET_FOCUS(wxComboBoxExtraInputHandler::OnFocus)
END_EVENT_TABLE()


void wxComboBoxExtraInputHandler::OnKey(wxKeyEvent& event)
{
    int keycode = event.GetKeyCode();

    if ( keycode == WXK_TAB )
    {
        wxNavigationKeyEvent evt;
        evt.SetFlags(wxNavigationKeyEvent::FromTab|
                     (!event.ShiftDown()?wxNavigationKeyEvent::IsForward:
                                         wxNavigationKeyEvent::IsBackward));
        evt.SetEventObject(m_combo);
        m_combo->GetParent()->GetEventHandler()->AddPendingEvent(evt);
        return;
    }

    if ( m_combo->IsPopupShown() )
    {
        // pass it to the popped up control
        m_combo->GetPopupControl()->AddPendingEvent(event);
    }
    else // no popup
    {
        int comboStyle = m_combo->GetWindowStyle();
        wxComboPopup* popupInterface = m_combo->GetPopup();

        if ( !popupInterface )
        {
            event.Skip();
            return;
        }

        if ( (comboStyle & wxCB_READONLY) ||
             ( keycode != WXK_RIGHT && keycode != WXK_LEFT )
            )
        {
            // Alternate keys: UP and DOWN show the popup instead of cycling
            if ( (comboStyle & wxCC_ALT_KEYS) )
            {
                if ( keycode == WXK_UP || keycode == WXK_DOWN )
                {
                    m_combo->OnButtonClick();
                    return;
                }
            }
            else
                popupInterface->OnComboKeyEvent(event);
        }
        else
            event.Skip();
    }
}


void wxComboBoxExtraInputHandler::OnFocus(wxFocusEvent& event)
{
    // FIXME: This code does run when control is clicked,
    //        yet on Windows it doesn't select all the text.
    if ( !(m_combo->GetInternalFlags() & wxCC_NO_TEXT_AUTO_SELECT) )
    {
        if ( m_combo->GetTextCtrl() )
            m_combo->GetTextCtrl()->SelectAll();
        else
            m_combo->SetSelection(-1,-1);
    }

    event.Skip();
}


//
// This is pushed to the event handler queue of the control in popup.
//

class wxComboPopupExtraEventHandler : public wxEvtHandler
{
public:

    wxComboPopupExtraEventHandler( wxComboControlBase* combo )
        : wxEvtHandler()
    {
        m_combo = combo;
        m_beenInside = false;
    }
    ~wxComboPopupExtraEventHandler() { }

    void OnMouseEvent( wxMouseEvent& event );

    // Called from wxPGComboControlBase::OnPopupDismiss
    void OnPopupDismiss()
    {
        m_beenInside = false;
    }

protected:
    wxComboControlBase*     m_combo;

    bool                            m_beenInside;

private:
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(wxComboPopupExtraEventHandler, wxEvtHandler)
    EVT_MOUSE_EVENTS(wxComboPopupExtraEventHandler::OnMouseEvent)
END_EVENT_TABLE()


void wxComboPopupExtraEventHandler::OnMouseEvent( wxMouseEvent& event )
{
    wxPoint pt = event.GetPosition();
    wxSize sz = m_combo->GetPopupControl()->GetClientSize();
    int evtType = event.GetEventType();
    bool isInside = pt.x >= 0 && pt.y >= 0 && pt.x < sz.x && pt.y < sz.y;

    if ( evtType == wxEVT_MOTION ||
         evtType == wxEVT_LEFT_DOWN ||
         evtType == wxEVT_RIGHT_DOWN )
    {
        // Block motion and click events outside the popup
        if ( !isInside )
        {
            event.Skip(false);
            return;
        }
    }
    else if ( evtType == wxEVT_LEFT_UP )
    {
        // Don't let left-down events in if outside
        if ( evtType == wxEVT_LEFT_DOWN )
        {
            if ( !isInside )
                return;
        }

        if ( !m_beenInside )
        {
            if ( isInside )
            {
                m_beenInside = true;
            }
            else
            {
                //
                // Some mouse events to popup that happen outside it, before cursor
                // has been inside the popu, need to be ignored by it but relayed to
                // the dropbutton.
                //
                wxWindow* btn = m_combo->GetButton();
                if ( btn )
                    btn->GetEventHandler()->AddPendingEvent(event);
                else
                    m_combo->GetEventHandler()->AddPendingEvent(event);

                return;
            }

            event.Skip();
        }
    }

    event.Skip();
}

// ----------------------------------------------------------------------------
// wxComboControlBase
// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE(wxComboControlBase, wxControl)
    EVT_TEXT(wxID_ANY,wxComboControlBase::OnTextCtrlEvent)
    EVT_SIZE(wxComboControlBase::OnSizeEvent)
    EVT_SET_FOCUS(wxComboControlBase::OnFocusEvent)
    EVT_KILL_FOCUS(wxComboControlBase::OnFocusEvent)
    //EVT_BUTTON(wxID_ANY,wxComboControlBase::OnButtonClickEvent)
    EVT_TEXT_ENTER(wxID_ANY,wxComboControlBase::OnTextCtrlEvent)
    EVT_SYS_COLOUR_CHANGED(wxComboControlBase::OnSysColourChanged)
END_EVENT_TABLE()


IMPLEMENT_ABSTRACT_CLASS(wxComboControlBase, wxControl)

// Have global double buffer - should be enough for multiple combos
static wxBitmap* gs_doubleBuffer = (wxBitmap*) NULL;

void wxComboControlBase::Init()
{
    m_winPopup = (wxWindow *)NULL;
    m_popup = (wxWindow *)NULL;
    m_isPopupShown = false;
    m_btn = (wxWindow*) NULL;
    m_text = (wxTextCtrl*) NULL;
    m_popupInterface = (wxComboPopup*) NULL;

    m_extraEvtHandler = (wxEvtHandler*) NULL;
    m_popupExtraHandler = (wxEvtHandler*) NULL;
    m_textEvtHandler = (wxEvtHandler*) NULL;

#if INSTALL_TOPLEV_HANDLER
    m_toplevEvtHandler = (wxEvtHandler*) NULL;
#endif

    m_heightPopup = -1;
    m_widthMinPopup = -1;
    m_anchorSide = 0;
    m_widthCustomPaint = 0;
    m_widthCustomBorder = 0;

    m_btnState = 0;
    m_btnWidDefault = 0;
    m_blankButtonBg = false;
    m_btnWid = m_btnHei = 0;
    m_btnSide = wxRIGHT;
    m_btnSpacingX = 0;

    m_extLeft = 0;
    m_extRight = 0;
    m_absIndent = -1;
    m_iFlags = 0;
    m_downReceived = false;
    m_timeCanAcceptClick = 0;
}

bool wxComboControlBase::Create(wxWindow *parent,
                                wxWindowID id,
                                const wxString& value,
                                const wxPoint& pos,
                                const wxSize& size,
                                long style,
                                const wxValidator& validator,
                                const wxString& name)
{
    if ( !wxControl::Create(parent,
                            id,
                            pos,
                            size,
                            style | wxWANTS_CHARS,
                            validator,
                            name) )
        return false;

    m_valueString = value;

    // Get colours
    OnThemeChange();
    m_absIndent = GetNativeTextIndent();

    return true;
}

void wxComboControlBase::InstallInputHandlers( bool alsoTextCtrl )
{
    if ( m_text && alsoTextCtrl )
    {
        m_textEvtHandler = new wxComboBoxExtraInputHandler(this);
        m_text->PushEventHandler(m_textEvtHandler);
    }

    wxComboBoxExtraInputHandler* inputHandler = new wxComboBoxExtraInputHandler(this);
    PushEventHandler(inputHandler);
    m_extraEvtHandler = inputHandler;
}

void wxComboControlBase::CreateTextCtrl( int extraStyle, const wxValidator& validator )
{
    if ( !(m_windowStyle & wxCB_READONLY) )
    {
        m_text = new wxTextCtrl(this,
                                12345,
                                m_valueString,
                                wxDefaultPosition,
                                wxDefaultSize,
                                // wxTE_PROCESS_TAB is needed because on Windows, wxTAB_TRAVERSAL is
                                // not used by the wxPropertyGrid and therefore the tab is
                                // processed by looking at ancestors to see if they have
                                // wxTAB_TRAVERSAL. The navigation event is then sent to
                                // the wrong window.
                                wxTE_PROCESS_TAB |
                                extraStyle,
                                validator);

        // This is required for some platforms (GTK+ atleast)
        m_text->SetSizeHints(2,4);
    }
}

void wxComboControlBase::OnThemeChange()
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
}

wxComboControlBase::~wxComboControlBase()
{
    if ( HasCapture() )
        ReleaseMouse();

    delete gs_doubleBuffer;
    gs_doubleBuffer = (wxBitmap*) NULL;

#if INSTALL_TOPLEV_HANDLER
    delete ((wxComboFrameEventHandler*)m_toplevEvtHandler);
    m_toplevEvtHandler = (wxEvtHandler*) NULL;
#endif

    if ( m_popup )
        m_popup->RemoveEventHandler(m_popupExtraHandler);

    delete m_popupExtraHandler;

    HidePopup();

    delete m_popupInterface;
    delete m_winPopup;

    RemoveEventHandler(m_extraEvtHandler);

    if ( m_text )
        m_text->RemoveEventHandler(m_textEvtHandler);

    delete m_textEvtHandler;
    delete m_extraEvtHandler;
}


// ----------------------------------------------------------------------------
// geometry stuff
// ----------------------------------------------------------------------------

// Recalculates button and textctrl areas
void wxComboControlBase::CalculateAreas( int btnWidth )
{
    wxSize sz = GetClientSize();
    int customBorder = m_widthCustomBorder;
    bool buttonOutside;
    int btnBorder; // border for button only

    if ( ( (m_iFlags & wxCC_BUTTON_OUTSIDE_BORDER) || m_blankButtonBg ) &&
         m_btnSpacingX == 0 && m_btnWid == 0 && m_btnHei == 0 &&
         (!m_bmpNormal.Ok() || m_blankButtonBg) )
    {
        buttonOutside = true;
        m_iFlags |= wxCC_IFLAG_BUTTON_OUTSIDE;
        btnBorder = 0;
    }
    else
    {
        buttonOutside = false;
        m_iFlags &= ~(wxCC_IFLAG_BUTTON_OUTSIDE);
        btnBorder = customBorder;
    }

    // Defaul indentation
    if ( m_absIndent < 0 )
        m_absIndent = GetNativeTextIndent();

    int butWidth = btnWidth;

    if ( butWidth <= 0 )
        butWidth = m_btnWidDefault;
    else
        m_btnWidDefault = butWidth;

    if ( butWidth <= 0 )
        return;

    // Adjust button width
    if ( m_btnWid < 0 )
        butWidth += m_btnWid;
    else if ( m_btnWid > 0 )
        butWidth = m_btnWid;

    int butHeight = sz.y;

    butHeight -= btnBorder*2;

    // Adjust button height
    if ( m_btnHei < 0 )
        butHeight += m_btnHei;
    else if ( m_btnHei > 0 )
        butHeight = m_btnHei;

    // Use size of normal bitmap if...
    //   It is larger
    //   OR
    //   button width is set to default and blank button bg is not drawn
    if ( m_bmpNormal.Ok() )
    {
        int bmpReqWidth = m_bmpNormal.GetWidth();
        int bmpReqHeight = m_bmpNormal.GetHeight();

        // If drawing blank button background, we need to add some margin.
        if ( m_blankButtonBg )
        {
            bmpReqWidth += BMP_BUTTON_MARGIN*2;
            bmpReqHeight += BMP_BUTTON_MARGIN*2;
        }

        if ( butWidth < bmpReqWidth || ( m_btnWid == 0 && !m_blankButtonBg ) )
            butWidth = bmpReqWidth;
        if ( butHeight < bmpReqHeight || ( m_btnHei == 0 && !m_blankButtonBg ) )
            butHeight = bmpReqHeight;

        // Need to fix height?
        if ( (sz.y-(customBorder*2)) < butHeight && btnWidth == 0 )
        {
            int newY = butHeight+(customBorder*2);
            SetClientSize(-1,newY);
            sz.y = newY;
        }
    }

    int butAreaWid = butWidth + (m_btnSpacingX*2);

    m_btnSize.x = butWidth;
    m_btnSize.y = butHeight;

    m_btnArea.x = ( m_btnSide==wxRIGHT ? sz.x - butAreaWid - btnBorder : btnBorder );
    m_btnArea.y = btnBorder;
    m_btnArea.width = butAreaWid;
    m_btnArea.height = sz.y - (btnBorder*2);

    m_tcArea.x = ( m_btnSide==wxRIGHT ? 0 : butAreaWid ) + customBorder;
    m_tcArea.y = customBorder;
    m_tcArea.width = sz.x - butAreaWid - (customBorder*2);
    m_tcArea.height = sz.y - (customBorder*2);

/*
    if ( m_text )
    {
        ::wxMessageBox(wxString::Format(wxT("ButtonArea (%i,%i,%i,%i)\n"),m_btnArea.x,m_btnArea.y,m_btnArea.width,m_btnArea.height) +
                       wxString::Format(wxT("TextCtrlArea (%i,%i,%i,%i)"),m_tcArea.x,m_tcArea.y,m_tcArea.width,m_tcArea.height));
    }
*/
}

void wxComboControlBase::PositionTextCtrl( int textCtrlXAdjust, int textCtrlYAdjust )
{
    if ( !m_text )
        return;

    wxSize sz = GetClientSize();
    int customBorder = m_widthCustomBorder;

    if ( (m_text->GetWindowStyleFlag() & wxBORDER_MASK) == wxNO_BORDER )
    {
        // Centre textctrl
        int tcSizeY = m_text->GetBestSize().y;
        int diff = sz.y - tcSizeY;
        int y = textCtrlYAdjust + (diff/2);

        if ( y < customBorder )
            y = customBorder;

        m_text->SetSize( m_tcArea.x + m_widthCustomPaint + m_absIndent + textCtrlXAdjust,
                         y,
                         m_tcArea.width - COMBO_MARGIN -
                         (textCtrlXAdjust + m_widthCustomPaint + m_absIndent),
                         -1 );

        // Make sure textctrl doesn't exceed the bottom custom border
        wxSize tsz = m_text->GetSize();
        diff = (y + tsz.y) - (sz.y - customBorder);
        if ( diff >= 0 )
        {
            tsz.y = tsz.y - diff - 1;
            m_text->SetSize(tsz);
        }
    }
    else
    {
        m_text->SetSize( m_tcArea.x,
                         0,
                         sz.x - m_btnArea.x - m_widthCustomPaint - customBorder,
                         sz.y );
    }
}

wxSize wxComboControlBase::DoGetBestSize() const
{
    wxSize sizeText(150,0);

    if ( m_text )
        sizeText = m_text->GetBestSize();

    // TODO: Better method to calculate close-to-native control height.

    int fhei;
    if ( m_font.Ok() )
        fhei = (m_font.GetPointSize()*2) + 5;
    else if ( wxNORMAL_FONT->Ok() )
        fhei = (wxNORMAL_FONT->GetPointSize()*2) + 5;
    else
        fhei = sizeText.y + 4;

    // Need to force height to accomodate bitmap?
    int btnSizeY = m_btnSize.y;
    if ( m_bmpNormal.Ok() && fhei < btnSizeY )
        fhei = btnSizeY;

    // Control height doesn't depend on border
/*
    // Add border
    int border = m_windowStyle & wxBORDER_MASK;
    if ( border == wxSIMPLE_BORDER )
        fhei += 2;
    else if ( border == wxNO_BORDER )
        fhei += (m_widthCustomBorder*2);
    else
        // Sunken etc.
        fhei += 4;
*/

    // Final adjustments
#ifdef __WXGTK__
    fhei += 1;
#endif

    wxSize ret(sizeText.x + COMBO_MARGIN + DEFAULT_DROPBUTTON_WIDTH,
               fhei);

    CacheBestSize(ret);
    return ret;
}

void wxComboControlBase::DoMoveWindow(int x, int y, int width, int height)
{
    // SetSize is called last in create, so it marks the end of creation
    m_iFlags |= wxCC_IFLAG_CREATED;

    wxControl::DoMoveWindow(x, y, width, height);
}

void wxComboControlBase::OnSizeEvent( wxSizeEvent& event )
{
    if ( !IsCreated() )
        return;

    // defined by actual wxComboControls
    OnResize();

    event.Skip();
}

// ----------------------------------------------------------------------------
// standard operations
// ----------------------------------------------------------------------------

bool wxComboControlBase::Enable(bool enable)
{
    if ( !wxControl::Enable(enable) )
        return false;

    if ( m_btn )
        m_btn->Enable(enable);
    if ( m_text )
        m_text->Enable(enable);

    return true;
}

bool wxComboControlBase::Show(bool show)
{
    if ( !wxControl::Show(show) )
        return false;

    if (m_btn)
        m_btn->Show(show);

    if (m_text)
        m_text->Show(show);

    return true;
}

bool wxComboControlBase::SetFont ( const wxFont& font )
{
    if ( !wxControl::SetFont(font) )
        return false;

    if (m_text)
        m_text->SetFont(font);

    return true;
}

#if wxUSE_TOOLTIPS
void wxComboControlBase::DoSetToolTip(wxToolTip *tooltip)
{
    wxControl::DoSetToolTip(tooltip);

    // Set tool tip for button and text box
    if ( tooltip )
    {
        const wxString &tip = tooltip->GetTip();
        if ( m_text ) m_text->SetToolTip(tip);
        if ( m_btn ) m_btn->SetToolTip(tip);
    }
    else
    {
        if ( m_text ) m_text->SetToolTip( (wxToolTip*) NULL );
        if ( m_btn ) m_btn->SetToolTip( (wxToolTip*) NULL );
    }
}
#endif // wxUSE_TOOLTIPS

// ----------------------------------------------------------------------------
// painting
// ----------------------------------------------------------------------------

// draw focus background on area in a way typical on platform
void wxComboControlBase::DrawFocusBackground( wxDC& dc, const wxRect& rect, int flags )
{
    wxSize sz = GetClientSize();
    bool isEnabled;
    bool isFocused; // also selected

    // For smaller size control (and for disabled background) use less spacing
    int focusSpacingX;
    int focusSpacingY;

    if ( !(flags & wxCONTROL_ISSUBMENU) )
    {
        // Drawing control
        isEnabled = IsEnabled();
        isFocused = ShouldDrawFocus();

        // Windows-style: for smaller size control (and for disabled background) use less spacing
        focusSpacingX = isEnabled ? 2 : 1;
        focusSpacingY = sz.y > (GetCharHeight()+2) && isEnabled ? 2 : 1;
    }
    else
    {
        // Drawing a list item
        isEnabled = true; // they are never disabled
        isFocused = flags & wxCONTROL_SELECTED ? true : false;

        focusSpacingX = 0;
        focusSpacingY = 0;
    }

    // Set the background sub-rectangle for selection, disabled etc
    wxRect selRect(rect);
    selRect.y += focusSpacingY;
    selRect.height -= (focusSpacingY*2);
    selRect.x += m_widthCustomPaint + focusSpacingX;
    selRect.width -= m_widthCustomPaint + (focusSpacingX*2);

    wxColour bgCol;

    if ( isEnabled )
    {
        // If popup is hidden and this control is focused,
        // then draw the focus-indicator (selbgcolor background etc.).
        if ( isFocused )
        {
            dc.SetTextForeground( wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT) );
            bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
        }
        else
        {
            dc.SetTextForeground( wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT) );
            bgCol = GetBackgroundColour();
        }
    }
    else
    {
        dc.SetTextForeground( wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT) );
        bgCol = GetBackgroundColour();
    }

    dc.SetBrush( bgCol );
    dc.SetPen( bgCol );
    dc.DrawRectangle( selRect );
}

void wxComboControlBase::DrawButton( wxDC& dc, const wxRect& rect, bool paintBg )
{
    int drawState = m_btnState;

#ifdef __WXGTK__
    if ( m_isPopupShown )
        drawState |= wxCONTROL_PRESSED;
#endif

    wxRect drawRect(rect.x+m_btnSpacingX,
                    rect.y+((rect.height-m_btnSize.y)/2),
                    m_btnSize.x,
                    m_btnSize.y);

    // Make sure area is not larger than the control
    if ( drawRect.y < rect.y )
        drawRect.y = rect.y;
    if ( drawRect.height > rect.height )
        drawRect.height = rect.height;

    bool enabled = IsEnabled();

    if ( !enabled )
        drawState |= wxCONTROL_DISABLED;

    if ( !m_bmpNormal.Ok() )
    {
        // Need to clear button background even if m_btn is present
        // (assume non-button background was cleared just before this call so brushes are good)
        if ( paintBg )
            dc.DrawRectangle(rect);

        // Draw standard button
        wxRendererNative::Get().DrawComboBoxDropButton(this,
                                                       dc,
                                                       drawRect,
                                                       drawState);
    }
    else
    {
        // Draw bitmap

        wxBitmap* pBmp;

        if ( !enabled )
            pBmp = &m_bmpDisabled;
        else if ( m_btnState & wxCONTROL_PRESSED )
            pBmp = &m_bmpPressed;
        else if ( m_btnState & wxCONTROL_CURRENT )
            pBmp = &m_bmpHover;
        else
            pBmp = &m_bmpNormal;

        if ( m_blankButtonBg )
        {
            // If using blank button background, we need to clear its background
            // with button face colour instead of colour for rest of the control.
            if ( paintBg )
            {
                wxColour bgCol = GetParent()->GetBackgroundColour(); //wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
                //wxColour bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
                dc.SetPen(bgCol);
                dc.SetBrush(bgCol);
                dc.DrawRectangle(rect);
            }

            wxRendererNative::Get().DrawPushButton(this,
                                                   dc,
                                                   drawRect,
                                                   drawState);

        }
        else

        {
            // Need to clear button background even if m_btn is present
            // (assume non-button background was cleared just before this call so brushes are good)
            if ( paintBg )
                dc.DrawRectangle(rect);
        }

        // Draw bitmap centered in drawRect
        dc.DrawBitmap(*pBmp,
                      drawRect.x + (drawRect.width-pBmp->GetWidth())/2,
                      drawRect.y + (drawRect.height-pBmp->GetHeight())/2,
                      true);
    }
}

void wxComboControlBase::RecalcAndRefresh()
{
    if ( IsCreated() )
    {
        wxSizeEvent evt(GetSize(),GetId());
        GetEventHandler()->ProcessEvent(evt);
        Refresh();
    }
}

wxBitmap& wxComboControlBase::GetBufferBitmap( const wxSize& sz ) const
{
    // If size is larger, recalculate double buffer bitmap
    if ( !gs_doubleBuffer ||
         sz.x > gs_doubleBuffer->GetWidth() ||
         sz.y > gs_doubleBuffer->GetHeight() )
    {
        delete gs_doubleBuffer;
        gs_doubleBuffer = new wxBitmap(sz.x+25,sz.y);
    }
    return *gs_doubleBuffer;
}


bool wxComboControlBase::OnDrawListItem( wxDC& WXUNUSED(dc),
                                         const wxRect& WXUNUSED(rect),
                                         int WXUNUSED(item),
                                         int WXUNUSED(flags) )
{
    return false; // signals caller to make default drawing
}

wxCoord wxComboControlBase::OnMeasureListItem( int WXUNUSED(item) )
{
    return -1; // signals caller to use default
}

wxCoord wxComboControlBase::OnMeasureListItemWidth( int WXUNUSED(item) )
{
    return -1; // signals caller to use default
}

// ----------------------------------------------------------------------------
// miscellaneous event handlers
// ----------------------------------------------------------------------------

void wxComboControlBase::OnTextCtrlEvent(wxCommandEvent& event)
{
    // Change event id and relay it forward
    event.SetId(GetId());
    event.Skip();
}

// call if cursor is on button area or mouse is captured for the button
bool wxComboControlBase::HandleButtonMouseEvent( wxMouseEvent& event,
                                                 int flags )
{
    int type = event.GetEventType();

    if ( type == wxEVT_MOTION )
    {
        if ( flags & wxCC_MF_ON_BUTTON )
        {
            if ( !(m_btnState & wxCONTROL_CURRENT) )
            {
                // Mouse hover begins
                m_btnState |= wxCONTROL_CURRENT;
                if ( HasCapture() ) // Retain pressed state.
                    m_btnState |= wxCONTROL_PRESSED;
                Refresh();
            }
        }
        else if ( (m_btnState & wxCONTROL_CURRENT) )
        {
            // Mouse hover ends
            m_btnState &= ~(wxCONTROL_CURRENT|wxCONTROL_PRESSED);
            Refresh();
        }
    }
    else if ( type == wxEVT_LEFT_DOWN )
    {
        // Only accept event if it wasn't right after popup dismiss
        //if ( ::wxGetLocalTimeMillis() > m_timeCanClick )
        {
            // Need to test this, because it might be outside.
            if ( flags & wxCC_MF_ON_BUTTON )
            {
                m_btnState |= wxCONTROL_PRESSED;
                Refresh();

                if ( !(m_iFlags & wxCC_POPUP_ON_MOUSE_UP) )
                    OnButtonClick();
                else
                    // If showing popup now, do not capture mouse or there will be interference
                    CaptureMouse();
            }
        }
        /*else
        {
            m_btnState = 0;
        }*/
    }
    else if ( type == wxEVT_LEFT_UP )
    {

        // Only accept event if mouse was left-press was previously accepted
        if ( HasCapture() )
            ReleaseMouse();

        if ( m_btnState & wxCONTROL_PRESSED )
        {
            // If mouse was inside, fire the click event.
            if ( m_iFlags & wxCC_POPUP_ON_MOUSE_UP )
            {
                if ( flags & wxCC_MF_ON_BUTTON )
                    OnButtonClick();
            }

            m_btnState &= ~(wxCONTROL_PRESSED);
            Refresh();
        }
    }
    else if ( type == wxEVT_LEAVE_WINDOW )
    {
        if ( m_btnState & (wxCONTROL_CURRENT|wxCONTROL_PRESSED) )
        {
            m_btnState &= ~(wxCONTROL_CURRENT);

            // Mouse hover ends
            if ( !m_isPopupShown )
            {
                m_btnState &= ~(wxCONTROL_PRESSED);
                Refresh();
            }
        }
    }
    else
        return false;

    return true;
}

// Conversion to double-clicks and some basic filtering
// returns true if event was consumed or filtered
//bool wxComboControlBase::PreprocessMouseEvent( wxMouseEvent& event, bool isOnButtonArea )
bool wxComboControlBase::PreprocessMouseEvent( wxMouseEvent& event,
                                               int flags )
{
    wxLongLong t = ::wxGetLocalTimeMillis();
    int evtType = event.GetEventType();

    //
    // Generate our own double-clicks
    // (to allow on-focus dc-event on double-clicks instead of triple-clicks)
    if ( (m_windowStyle & wxCC_SPECIAL_DCLICK) &&
         !m_isPopupShown &&
         //!(handlerFlags & wxCC_MF_ON_BUTTON) )
         !(flags & wxCC_MF_ON_BUTTON) )
    {
        if ( evtType == wxEVT_LEFT_DOWN )
        {
            // Set value to avoid up-events without corresponding downs
            m_downReceived = true;
        }
        else if ( evtType == wxEVT_LEFT_DCLICK )
        {
            // We'll make our own double-clicks
            //evtType = 0;
            event.SetEventType(0);
            return true;
        }
        else if ( evtType == wxEVT_LEFT_UP )
        {
            if ( m_downReceived || m_timeLastMouseUp == 1 )
            {
                wxLongLong timeFromLastUp = (t-m_timeLastMouseUp);

                if ( timeFromLastUp < DOUBLE_CLICK_CONVERSION_TRESHOLD )
                {
                    //type = wxEVT_LEFT_DCLICK;
                    event.SetEventType(wxEVT_LEFT_DCLICK);
                    m_timeLastMouseUp = 1;
                }
                else
                {
                    m_timeLastMouseUp = t;
                }

                //m_downReceived = false;
            }
        }
    }

    // Filter out clicks on button immediately after popup dismiss (Windows like behaviour)
    if ( evtType == wxEVT_LEFT_DOWN && t < m_timeCanAcceptClick )
    {
        event.SetEventType(0);
        return true;
    }

    return false;
}

void wxComboControlBase::HandleNormalMouseEvent( wxMouseEvent& event )
{
    int evtType = event.GetEventType();

    if ( (evtType == wxEVT_LEFT_DOWN || evtType == wxEVT_LEFT_DCLICK) &&
         (m_windowStyle & wxCB_READONLY) )
    {
        if ( m_isPopupShown )
        {
    #if !wxUSE_POPUPWIN
        // Normally do nothing - evt handler should close it for us
    #elif !USE_TRANSIENT_POPUP
            // Click here always hides the popup.
            HidePopup();
    #endif
        }
        else
        {
            if ( !(m_windowStyle & wxCC_SPECIAL_DCLICK) )
            {
                // In read-only mode, clicking the text is the
                // same as clicking the button.
                OnButtonClick();
            }
            else if ( /*evtType == wxEVT_LEFT_UP || */evtType == wxEVT_LEFT_DCLICK )
            {
                //if ( m_popupInterface->CycleValue() )
                //    Refresh();
                if ( m_popupInterface )
                    m_popupInterface->OnComboDoubleClick();
            }
        }
    }
    else
    if ( m_isPopupShown )
    {
        // relay (some) mouse events to the popup
        if ( evtType == wxEVT_MOUSEWHEEL )
            m_popup->AddPendingEvent(event);
    }
    else if ( evtType )
        event.Skip();
}

void wxComboControlBase::OnFocusEvent( wxFocusEvent& )
{
    // First click is the first part of double-click
    // Some platforms don't generate down-less mouse up-event
    // (Windows does, GTK+2 doesn't), so that's why we have
    // to do this.
    m_timeLastMouseUp = ::wxGetLocalTimeMillis();

    if ( m_text )
    {
        m_text->SetFocus();
    }
    else
        // no need to check for m_widthCustomPaint - that
        // area never gets special handling when selected
        // (in writable mode, that is)
        Refresh();
}

void wxComboControlBase::OnSysColourChanged(wxSysColourChangedEvent& WXUNUSED(event))
{
    OnThemeChange();
    // indentation may also have changed
    if ( !(m_iFlags & wxCC_IFLAG_INDENT_SET) )
        m_absIndent = GetNativeTextIndent();
    RecalcAndRefresh();
}

// ----------------------------------------------------------------------------
// popup handling
// ----------------------------------------------------------------------------

// Create popup window and the child control
void wxComboControlBase::CreatePopup()
{
    wxComboPopup* popupInterface = m_popupInterface;
    wxWindow* popup;

    if ( !m_winPopup )
        m_winPopup = new wxComboPopupWindow( this, wxNO_BORDER );

    popupInterface->Create(m_winPopup);
    m_popup = popup = popupInterface->GetControl();

    m_popupExtraHandler = new wxComboPopupExtraEventHandler(this);
    popup->PushEventHandler( m_popupExtraHandler );

    // This may be helpful on some platforms
    //   (eg. it bypasses a wxGTK popupwindow bug where
    //    window is not initially hidden when it should be)
    m_winPopup->Hide();

    popupInterface->m_iFlags |= wxCP_IFLAG_CREATED;
}

void wxComboControlBase::SetPopupControl( wxComboPopup* iface )
{
    delete m_popupInterface;
    delete m_winPopup;

    m_popupInterface = iface;

    if ( !iface->LazyCreate() || m_winPopup )
    {
        CreatePopup();
    }
    else
    {
        m_popup = (wxWindow*) NULL;
    }

    // This must be after creation
    if ( m_valueString )
        iface->SetStringValue(m_valueString);

}

void wxComboControlBase::OnButtonClick()
{
    // Derived classes can override this method for totally custom
    // popup action
    ShowPopup();
}

void wxComboControlBase::ShowPopup()
{
    wxCHECK_RET( m_popupInterface, wxT("no popup interface set for wxComboControl") );
    wxCHECK_RET( !IsPopupShown(), wxT("popup window already shown") );

    SetFocus();

    // Space above and below
    int screenHeight;
    wxPoint scrPos;
    int spaceAbove;
    int spaceBelow;
    int maxHeightPopup;
    wxSize ctrlSz = GetSize();

    screenHeight = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );
    scrPos = GetParent()->ClientToScreen(GetPosition());

    spaceAbove = scrPos.y;
    spaceBelow = screenHeight - spaceAbove - ctrlSz.y;

    maxHeightPopup = spaceBelow;
    if ( spaceAbove > spaceBelow )
        maxHeightPopup = spaceAbove;

    // Width
    int widthPopup = ctrlSz.x + m_extLeft + m_extRight;

    if ( widthPopup < m_widthMinPopup )
        widthPopup = m_widthMinPopup;

    wxWindow* winPopup = m_winPopup;
    wxWindow* popup;

    // Need to disable tab traversal of parent
    //
    // NB: This is to fix a bug in wxMSW. In theory it could also be fixed
    //     by, for instance, adding check to window.cpp:wxWindowMSW::MSWProcessMessage
    //     that if transient popup is open, then tab traversal is to be ignored.
    //     However, I think this code would still be needed for cases where
    //     transient popup doesn't work yet (wxWinCE?).
    wxWindow* parent = GetParent();
    int parentFlags = parent->GetWindowStyle();
    if ( parentFlags & wxTAB_TRAVERSAL )
    {
        parent->SetWindowStyle( parentFlags & ~(wxTAB_TRAVERSAL) );
        m_iFlags |= wxCC_IFLAG_PARENT_TAB_TRAVERSAL;
    }

    if ( !winPopup )
    {
        CreatePopup();
        winPopup = m_winPopup;
        popup = m_popup;
    }
    else
    {
        popup = m_popup;
    }

    wxASSERT( !m_popup || m_popup == popup ); // Consistency check.

    wxSize adjustedSize = m_popupInterface->GetAdjustedSize(widthPopup,
                                                            m_heightPopup<=0?DEFAULT_POPUP_HEIGHT:m_heightPopup,
                                                            maxHeightPopup);

    popup->SetSize(adjustedSize);
    popup->Move(0,0);
    m_popupInterface->OnPopup();

    //
    // Reposition and resize popup window
    //

    wxSize szp = popup->GetSize();

    int popupX;
    int popupY = scrPos.y + ctrlSz.y;

    int anchorSide = m_anchorSide;
    if ( !anchorSide )
        anchorSide = m_btnSide;

    // Anchor popup to the side the dropbutton is on
    if ( anchorSide == wxRIGHT )
        popupX = scrPos.x + ctrlSz.x + m_extRight- szp.x;
    else
        popupX = scrPos.x - m_extLeft;

    if ( spaceBelow < szp.y )
    {
        popupY = scrPos.y - szp.y;
    }

    // Move to position
    //wxLogDebug(wxT("popup scheduled position1: %i,%i"),ptp.x,ptp.y);
    //wxLogDebug(wxT("popup position1: %i,%i"),winPopup->GetPosition().x,winPopup->GetPosition().y);

    // Some platforms (GTK) may need these two to be separate
    winPopup->SetSize( szp.x, szp.y );
    winPopup->Move( popupX, popupY );

    //wxLogDebug(wxT("popup position2: %i,%i"),winPopup->GetPosition().x,winPopup->GetPosition().y);

    m_popup = popup;

    // Set string selection (must be this way instead of SetStringSelection)
    if ( m_text )
    {
        if ( !(m_iFlags & wxCC_NO_TEXT_AUTO_SELECT) )
            m_text->SelectAll();

        m_popupInterface->SetStringValue( m_text->GetValue() );
    }
    else
    {
        // This is neede since focus/selection indication may change when popup is shown
        // FIXME: But in that case, would m_isPopupShown need to go before this?
        Refresh();
    }

    // This must be after SetStringValue
    m_isPopupShown = true;

    // Show it
#if USE_TRANSIENT_POPUP
    ((wxPopupTransientWindow*)winPopup)->Popup(popup);
#else
    winPopup->Show();
#endif

#if INSTALL_TOPLEV_HANDLER
    // Put top level window event handler into place
    if ( !m_toplevEvtHandler )
        m_toplevEvtHandler = new wxComboFrameEventHandler(this);

    wxWindow* toplev = ::wxGetTopLevelParent( this );
    wxASSERT( toplev );
    ((wxComboFrameEventHandler*)m_toplevEvtHandler)->OnPopup();
    toplev->PushEventHandler( m_toplevEvtHandler );
#endif

}

void wxComboControlBase::OnPopupDismiss()
{
    // Just in case, avoid double dismiss
    if ( !m_isPopupShown )
        return;

    // *Must* set this before focus etc.
    m_isPopupShown = false;

    // Inform popup control itself
    m_popupInterface->OnDismiss();

    if ( m_popupExtraHandler )
        ((wxComboPopupExtraEventHandler*)m_popupExtraHandler)->OnPopupDismiss();

#if INSTALL_TOPLEV_HANDLER
    // Remove top level window event handler
    if ( m_toplevEvtHandler )
    {
        wxWindow* toplev = ::wxGetTopLevelParent( this );
        if ( toplev )
            toplev->RemoveEventHandler( m_toplevEvtHandler );
    }
#endif

    m_timeCanAcceptClick = ::wxGetLocalTimeMillis() + 150;

    // If cursor not on dropdown button, then clear its state
    // (technically not required by all ports, but do it for all just in case)
    if ( !m_btnArea.Inside(ScreenToClient(::wxGetMousePosition())) )
        m_btnState = 0;

    // Return parent's tab traversal flag.
    // See ShowPopup for notes.
    if ( m_iFlags & wxCC_IFLAG_PARENT_TAB_TRAVERSAL )
    {
        wxWindow* parent = GetParent();
        parent->SetWindowStyle( parent->GetWindowStyle() | wxTAB_TRAVERSAL );
        m_iFlags &= ~(wxCC_IFLAG_PARENT_TAB_TRAVERSAL);
    }

    // refresh control (necessary even if m_text)
    Refresh();

#if !wxUSE_POPUPWIN
    SetFocus();
#endif

}

void wxComboControlBase::HidePopup()
{
    // Should be able to call this without popup interface
    //wxCHECK_RET( m_popupInterface, _T("no popup interface") );
    if ( !m_isPopupShown )
        return;

    // transfer value and show it in textctrl, if any
    SetValue( m_popupInterface->GetStringValue() );

#if USE_TRANSIENT_POPUP
    ((wxPopupTransientWindow*)m_winPopup)->Dismiss();
#else
    m_winPopup->Hide();
#endif

    OnPopupDismiss();
}

// ----------------------------------------------------------------------------
// customization methods
// ----------------------------------------------------------------------------

void wxComboControlBase::SetButtonPosition( int width, int height,
                                            int side, int spacingX )
{
    m_btnWid = width;
    m_btnHei = height;
    m_btnSide = side;
    m_btnSpacingX = spacingX;

    RecalcAndRefresh();
}

void wxComboControlBase::SetButtonBitmaps( const wxBitmap& bmpNormal,
                                           bool blankButtonBg,
                                           const wxBitmap& bmpPressed,
                                           const wxBitmap& bmpHover,
                                           const wxBitmap& bmpDisabled )
{
    m_bmpNormal = bmpNormal;
    m_blankButtonBg = blankButtonBg;

    if ( bmpPressed.Ok() )
        m_bmpPressed = bmpPressed;
    else
        m_bmpPressed = bmpNormal;

    if ( bmpHover.Ok() )
        m_bmpHover = bmpHover;
    else
        m_bmpHover = bmpNormal;

    if ( bmpDisabled.Ok() )
        m_bmpDisabled = bmpDisabled;
    else
        m_bmpDisabled = bmpNormal;

    RecalcAndRefresh();
}

void wxComboControlBase::SetCustomPaintWidth( int width )
{
    if ( m_text )
    {
        // move textctrl accordingly
        wxRect r = m_text->GetRect();
        int inc = width - m_widthCustomPaint;
        r.x += inc;
        r.width -= inc;
        m_text->SetSize( r );
    }

    m_widthCustomPaint = width;

    RecalcAndRefresh();
}

void wxComboControlBase::SetTextIndent( int indent )
{
    if ( indent < 0 )
    {
        m_absIndent = GetNativeTextIndent();
        m_iFlags &= ~(wxCC_IFLAG_INDENT_SET);
    }
    else
    {
        m_absIndent = indent;
        m_iFlags |= wxCC_IFLAG_INDENT_SET;
    }

    RecalcAndRefresh();
}

wxCoord wxComboControlBase::GetNativeTextIndent() const
{
    return DEFAULT_TEXT_INDENT;
}

// ----------------------------------------------------------------------------
// methods forwarded to wxTextCtrl
// ----------------------------------------------------------------------------

wxString wxComboControlBase::GetValue() const
{
    if ( m_text )
        return m_text->GetValue();
    return m_valueString;
}

void wxComboControlBase::SetValue(const wxString& value)
{
    if ( m_text )
    {
        m_text->SetValue(value);
        if ( !(m_iFlags & wxCC_NO_TEXT_AUTO_SELECT) )
            m_text->SelectAll();
    }

    // Since wxComboPopup may want to paint the combo as well, we need
    // to set the string value here (as well as sometimes in ShowPopup).
    if ( m_valueString != value && m_popupInterface )
    {
        m_popupInterface->SetStringValue(value);
    }

    m_valueString = value;

    Refresh();
}

void wxComboControlBase::Copy()
{
    if ( m_text )
        m_text->Copy();
}

void wxComboControlBase::Cut()
{
    if ( m_text )
        m_text->Cut();
}

void wxComboControlBase::Paste()
{
    if ( m_text )
        m_text->Paste();
}

void wxComboControlBase::SetInsertionPoint(long pos)
{
    if ( m_text )
        m_text->SetInsertionPoint(pos);
}

void wxComboControlBase::SetInsertionPointEnd()
{
    if ( m_text )
        m_text->SetInsertionPointEnd();
}

long wxComboControlBase::GetInsertionPoint() const
{
    if ( m_text )
        return m_text->GetInsertionPoint();

    return 0;
}

long wxComboControlBase::GetLastPosition() const
{
    if ( m_text )
        return m_text->GetLastPosition();

    return 0;
}

void wxComboControlBase::Replace(long from, long to, const wxString& value)
{
    if ( m_text )
        m_text->Replace(from, to, value);
}

void wxComboControlBase::Remove(long from, long to)
{
    if ( m_text )
        m_text->Remove(from, to);
}

void wxComboControlBase::SetSelection(long from, long to)
{
    if ( m_text )
        m_text->SetSelection(from, to);
}

void wxComboControlBase::Undo()
{
    if ( m_text )
        m_text->Undo();
}

#endif // wxUSE_COMBOCONTROL