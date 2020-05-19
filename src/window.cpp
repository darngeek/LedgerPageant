#include "window.h"

#include "resource.h"
#include <map> // window map
#include <string> // error string
#include <strsafe.h> // StringCchCopy

#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <Dbt.h>

#include "memoryMap.h"
#include "agent.h"
#include "logger.h"

#define WMAPP_NOTIFYCALLBACK (WM_APP + 1)

wchar_t const szWindowClass[] = L"Pageant";
class __declspec(uuid("9D0B8B92-4E1C-488e-A1E1-2331AFCE2CB5")) LedgerPeageantGUID;

HINSTANCE g_hInst = nullptr;

Window::Window() {
    
}

Window::~Window() {

}

BOOL AddNotificationIcon(HWND hwnd)
{
    NOTIFYICONDATA data = { sizeof(data) };
    data.uFlags = NIF_ICON | NIF_MESSAGE;
    data.guidItem = __uuidof(LedgerPeageantGUID);
    data.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
    data.hWnd = hwnd;
    data.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_NOTIFICATIONICON));
    Shell_NotifyIcon(NIM_ADD, &data);

    data.uVersion = NOTIFYICON_VERSION_4;
    BOOL success = Shell_NotifyIcon(NIM_SETVERSION, &data);
    
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID != 0) {
        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);
    }

    return success;
}

BOOL DeleteNotificationIcon() {
    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.uFlags = NIF_GUID;
    nid.guidItem = __uuidof(LedgerPeageantGUID);
    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

// open the rightclick menu on notification icon
void ShowContextMenu(HWND hwnd, POINT pt) {
    HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_NOTIFY_MENU));
    if (hMenu) {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu) {
            // our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
            SetForegroundWindow(hwnd);

            // respect menu drop alignment
            UINT uFlags = TPM_RIGHTBUTTON;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0) {
                uFlags |= TPM_RIGHTALIGN;
            }
            else {
                uFlags |= TPM_LEFTALIGN;
            }

            TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
        }
        DestroyMenu(hMenu);
    }
}

std::wstring readDialogItemWStr(HWND hwnd, DWORD item) {
    LPWSTR aasd[255];
    GetDlgItemText(hwnd, item, (LPWSTR)&aasd, 254);
    std::wstring itemWstr((LPWSTR)&aasd);
    return itemWstr;
}

int readDialogItemNumber(HWND hwnd, DWORD item) {
    int retVar = 0;
    BOOL success = FALSE;
    BOOL checkSign = FALSE;

    int read = GetDlgItemInt(hwnd, item, &success, checkSign);
    if (success == TRUE) {
        retVar = read;
    }

    return retVar;
}

// store the position/width of elements as they come from resource.
RECT windowStartScreenSize;
RECT windowStartClientSize;
std::map<int, RECT> controlRects;
std::map<int, HWND> controlHandles;
void storeControl(HWND hwnd, int control) {
    HWND windowHandle = hwnd;

    // store handle
    HWND controlHandle = GetDlgItem(hwnd, control);
    controlHandles[control] = controlHandle;

    // get control rect
    RECT screenRect;

    // given in screen coordinates
    if (GetWindowRect(controlHandle, &screenRect) != 0) {
        // translate rect
        RECT localRect = screenRect;
        POINT controlPosition = { screenRect.left, screenRect.top };
        POINT controlSize = { screenRect.right - screenRect.left, screenRect.bottom - screenRect.top };
        ScreenToClient(windowHandle, (LPPOINT)&controlPosition);

        controlRects[control] = { controlPosition.x, controlPosition.y, controlSize.x, controlSize.y};
    }
}

void storeLayout(HWND hwnd) {
    // Store Window size for minimum size:
    RECT rect;
    if (GetWindowRect(hwnd, &rect) != 0) {
        windowStartScreenSize = rect;
    }

    // Store Window size for relative distance compare on resize
    //RECT rect;
    if (GetClientRect(hwnd, &rect) != 0) {
        windowStartClientSize = rect;
    }

    storeControl(hwnd, IDC_LBL_IDENTITIES);
    storeControl(hwnd, IDC_LST_IDENTITIES);
    storeControl(hwnd, IDC_BTN_ADD);
    storeControl(hwnd, IDC_BTN_GETKEY);
    storeControl(hwnd, IDC_BTN_REMOVE);
    storeControl(hwnd, IDC_GRP_IDENTITY);
    storeControl(hwnd, IDC_LBL_DISPLAYNAME);
    storeControl(hwnd, IDC_TXT_DISPLAYNAME);
    storeControl(hwnd, IDC_LBL_PROTOCOL);
    storeControl(hwnd, IDC_TXT_PROTOCOL);
    storeControl(hwnd, IDC_LBL_HOSTNAME);
    storeControl(hwnd, IDC_TXT_HOSTNAME);
    storeControl(hwnd, IDC_LBL_PORT);
    storeControl(hwnd, IDC_TXT_PORT);
    storeControl(hwnd, IDC_LBL_USERNAME);
    storeControl(hwnd, IDC_TXT_USERNAME);
    storeControl(hwnd, IDC_BTN_SAVE);
    storeControl(hwnd, IDC_BTN_CLOSE);
}

void updateControl(int control, int x, int y, int width, int height) {
    // Get control position and size
    RECT controlRect = controlRects[control];

    // Update position and size
    POINT controlPosition = { controlRect.left + x, controlRect.top + y };
    POINT controlSize = { controlRect.right + width, controlRect.bottom + height };

    // SetWindowPos in client coordinates
    constexpr int flags = 0;
    SetWindowPos(controlHandles[control], HWND_TOP, controlPosition.x, controlPosition.y, controlSize.x, controlSize.y, flags);
}

void scaleControl(int control, int width, int height) {
    updateControl(control, 0, 0, width, height);
}

void moveControl(int control, int x, int y) {
    updateControl(control, x, y, 0, 0);
}

void resizeControls(int clientWidth, int clientHeight) {
    int deltaX = clientWidth - windowStartClientSize.right;
    int deltaY = clientHeight - windowStartClientSize.bottom;

    // move left controls below list, further down:
    // moveControl(IDC_LBL_IDENTITIES, 0, 0);
    moveControl(IDC_BTN_ADD, 0, deltaY);
    moveControl(IDC_BTN_GETKEY, 0, deltaY);
    moveControl(IDC_BTN_REMOVE, 0, deltaY);

    // list view stretches with delta:
    scaleControl(IDC_LST_IDENTITIES, deltaX, deltaY);
    
    // Fill with publickey column width
    ListView_SetColumnWidth(controlHandles[IDC_LST_IDENTITIES], 1, LVSCW_AUTOSIZE_USEHEADER);

    // controls right move right:
    moveControl(IDC_GRP_IDENTITY, deltaX, 0);
    moveControl(IDC_LBL_DISPLAYNAME, deltaX, 0);
    moveControl(IDC_TXT_DISPLAYNAME, deltaX, 0);
    moveControl(IDC_LBL_PROTOCOL, deltaX, 0);
    moveControl(IDC_TXT_PROTOCOL, deltaX, 0);
    moveControl(IDC_LBL_HOSTNAME, deltaX, 0);
    moveControl(IDC_TXT_HOSTNAME, deltaX, 0);
    moveControl(IDC_LBL_PORT, deltaX, 0);
    moveControl(IDC_TXT_PORT, deltaX, 0);
    moveControl(IDC_LBL_USERNAME, deltaX, 0);
    moveControl(IDC_TXT_USERNAME, deltaX, 0);
    moveControl(IDC_BTN_SAVE, deltaX, 0);
    moveControl(IDC_BTN_CLOSE, deltaX, deltaY);
}

int AddNewIdentity(HWND listHandle) {
    Identity ident;
    ident.name = L"New Identity";
    ident.protocol = L"ssh";
    ident.port = 22;
    int index = Window::GetPtr()->getApp()->addIdentity(ident);

    // Raw Add to list:
    LVITEM lvI;
    lvI.pszText = LPSTR_TEXTCALLBACK;
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.stateMask = 0;
    lvI.iSubItem = 0;
    lvI.state = 0;
    lvI.iItem = index;
    lvI.iImage = index;
    ListView_InsertItem(listHandle, &lvI);

    // Select item in list
    ListView_SetItemState(listHandle, index, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);

    return index;
}

void RefreshIdentityList(HWND listBox, int startIndex) {
    Application* app = Window::GetPtr()->getApp();
    for (uint32_t i = startIndex; i < app->getNumIdentities(); ++i) {
        ListView_Update(listBox, i);
    }
}

bool RemoveIdentityByIndex(HWND listBox, int index) {
    if (index != -1) {
        LPCWSTR title = L"Remove Identity";
        LPCWSTR description = L"Removing this Identity will also remove it from other applications like Putty!\n\nAre you sure?";

        int msgboxID = MessageBox(NULL, description, title, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2);
        if (msgboxID == IDYES) {
            Application* app = Window::GetPtr()->getApp();
            if (app->removeIdentityByIndex(index)) {
                ListView_DeleteItem(listBox, index);
                RefreshIdentityList(listBox, index);
                return true;
            }
        }
    }

    return false;
}

bool SaveIdentity(HWND windowHandle, int index, bool changed) {
    if (changed) {
        LPCWSTR title = L"Save Identity";
        LPCWSTR description = L"Changing an Identity will also change its Public Key\n\nAre you sure?";

        int msgboxID = MessageBox(NULL, description, title, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2);
        if (msgboxID == IDNO) {
            return false;
        }
    }

    Application* app = Window::GetPtr()->getApp();
    Identity ident = Window::GetPtr()->getApp()->getIdentityByIndex(index);

    ident.protocol = readDialogItemWStr(windowHandle, IDC_TXT_PROTOCOL);
    ident.name = readDialogItemWStr(windowHandle, IDC_TXT_DISPLAYNAME);
    ident.host = readDialogItemWStr(windowHandle, IDC_TXT_HOSTNAME);
    ident.user = readDialogItemWStr(windowHandle, IDC_TXT_USERNAME);
    ident.port = readDialogItemNumber(windowHandle, IDC_TXT_PORT);

    return app->writeIdentity(ident);
}

Identity& GetSelectedIdentity(HWND listHandle) {
    int iPos = ListView_GetNextItem(listHandle, -1, LVNI_SELECTED);
    Application* app = Window::GetPtr()->getApp();
    return app->getIdentityByIndex(iPos);
}

void GetKeyForSelectedItem(HWND listHandle) {
    Identity& ident = GetSelectedIdentity(listHandle);
    Application* app = Window::GetPtr()->getApp();
    ident.pubkey_cached = app->getPubKeyFor(ident);
    RefreshIdentityList(listHandle, 0);
}

void CopyPubkeyToClipboard(HWND listHandle) {
    // get public key as string:
    Identity& ident = GetSelectedIdentity(listHandle);
    Application* app = Window::GetPtr()->getApp();
    std::string keyStr = app->getPubKeyStrFor(ident.pubkey_cached, ident);

    // copy key to clipboard
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, keyStr.size());
    memcpy(GlobalLock(hMem), keyStr.c_str(), keyStr.size());
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

BOOL CALLBACK ManageIdentProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    HWND m_hListBox = GetDlgItem(hwnd, IDC_LST_IDENT);

    switch (Message)
    {
    case WM_INITDIALOG:
    {
        // Load Caption
        WCHAR szTitle[100];
        LoadString(g_hInst, IDS_MNGIDENT_TITLE, szTitle, ARRAYSIZE(szTitle));
        SetWindowText(hwnd, szTitle);

        // Load Icon
        HICON hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL),
            MAKEINTRESOURCEW(IDI_NOTIFICATIONICON),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            0);
        if (hIcon)
        {
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
        
        // Set Listbox to have Fullrow Select
        SendMessage(m_hListBox, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

        // Meak columns
        LVCOLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; // format, width, text, subitem are valid:
        lvc.fmt = LVCFMT_LEFT;

        lvc.iSubItem = 0;
        lvc.cx = 170;
        WCHAR columnName[100];
        LoadString(g_hInst, IDS_LST_COL_NAME, columnName, ARRAYSIZE(columnName));
        lvc.pszText = columnName;
        ListView_InsertColumn(m_hListBox, lvc.iSubItem, &lvc);

        lvc.iSubItem = 1;
        lvc.cx = LVSCW_AUTOSIZE_USEHEADER;
        WCHAR columnPubKey[100];
        LoadString(g_hInst, IDS_LST_COL_PUBKEY, columnPubKey, ARRAYSIZE(columnPubKey));
        lvc.pszText = columnPubKey;
        ListView_InsertColumn(m_hListBox, lvc.iSubItem, &lvc);

        // Fill with publickey column width
        ListView_SetColumnWidth(m_hListBox, 1, LVSCW_AUTOSIZE_USEHEADER);

        // This is where we set up the dialog box, and initialise any default values
        Application* app = Window::GetPtr()->getApp();
        for (uint32_t i = 0; i < app->getNumIdentities(); ++i) {
            LVITEM lvI;
            lvI.pszText = LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
            lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
            lvI.stateMask = 0;
            lvI.iSubItem = 0;
            lvI.state = 0;
            lvI.iItem = i;
            lvI.iImage = i;

            ListView_InsertItem(m_hListBox, &lvI);
        }

        ListView_SetItemState(m_hListBox, app->getNumIdentities() - 1, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);

        storeLayout(hwnd);

        break;
    }
    case WM_NOTIFY:
    {
        // Fill Item display info when requested:
        NMLVDISPINFO* plvdi;
        Application* app = Window::GetPtr()->getApp();
        switch (((LPNMHDR)lParam)->code) {
        case LVN_GETDISPINFO:
            {
                plvdi = (NMLVDISPINFO*)lParam;

                Identity ident = app->getIdentityByIndex(plvdi->item.iItem);
                if (plvdi->item.iSubItem == 0) {
                    HRESULT hr = StringCchCopy(plvdi->item.pszText, ident.name.size() * sizeof(WCHAR), (LPWSTR)ident.name.c_str());
                }
                else if (plvdi->item.iSubItem == 1) {
                    if (!ident.pubkey_cached.empty()) {
                        std::string pubStr = app->getPubKeyStrFor(ident.pubkey_cached, ident);
                        std::wstring pubWStr = std::wstring(pubStr.begin(), pubStr.end());
                        HRESULT hr = StringCchCopy(plvdi->item.pszText, pubWStr.size() * sizeof(WCHAR), (LPWSTR)pubWStr.c_str());
                    }
                }

            }
            break;
        default:
            break;
        };

        // Get selection changed event:
        NMLISTVIEW* VAL_notify = (NMLISTVIEW*)lParam;
        if (VAL_notify->hdr.code == LVN_ITEMCHANGED && VAL_notify->hdr.idFrom == IDC_LST_IDENT && (VAL_notify->uNewState & LVIS_SELECTED)) {
            // get identity for selected item
            int iPos = ListView_GetNextItem(m_hListBox, -1, LVNI_SELECTED);
            Identity ident = Window::GetPtr()->getApp()->getIdentityByIndex(iPos);

            // fill selected fields with identity
            SetDlgItemText(hwnd, IDC_TXT_DISPLAYNAME, ident.name.c_str());
            SetDlgItemText(hwnd, IDC_TXT_PROTOCOL, ident.protocol.c_str());
            SetDlgItemText(hwnd, IDC_TXT_HOSTNAME, ident.host.c_str());
            if (ident.port != -1) {
                SetDlgItemInt(hwnd, IDC_TXT_PORT, ident.port, FALSE);
            }
            else {
                SetDlgItemInt(hwnd, IDC_TXT_PORT, 22, FALSE);
            }
            SetDlgItemText(hwnd, IDC_TXT_USERNAME, ident.user.c_str());
        }
        break;
    }
    case WM_CONTEXTMENU:
        {
            // listbox right click:
            if ((HWND)wParam == m_hListBox) {
                HMENU m_hListMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_LSTIDENT_MENU));
                if (m_hListMenu) {
                    HMENU hSubMenu = GetSubMenu(m_hListMenu, 0);
                    if (hSubMenu) {
                        TrackPopupMenu(hSubMenu, TPM_TOPALIGN | TPM_LEFTALIGN, LOWORD(lParam), HIWORD(lParam), 0, hwnd, NULL);
                    }
                }
            }
        }
        break;
    case WM_COMMAND:
        {
            const int controlHandle = LOWORD(wParam);
            switch (controlHandle)
            {
            case IDC_BTN_ADD:
                AddNewIdentity(m_hListBox);
                RefreshIdentityList(m_hListBox, 0);
                break;
            case IDC_BTN_GETKEY:
                GetKeyForSelectedItem(m_hListBox);
                break;
            case IDC_BTN_REMOVE:
                RemoveIdentityByIndex(m_hListBox, ListView_GetNextItem(m_hListBox, -1, LVNI_ALL | LVNI_SELECTED));
                break;
            case IDC_BTN_SAVE:
                SaveIdentity(hwnd, ListView_GetNextItem(m_hListBox, -1, LVNI_ALL | LVNI_SELECTED), true);
                break;
            case IDC_BTN_CLOSE:
                EndDialog(hwnd, 0);
                break;

            // button in popup menu over listbox with identities:
            case ID__REMOVE: 
                RemoveIdentityByIndex(m_hListBox, ListView_GetNextItem(m_hListBox, -1, LVNI_ALL | LVNI_SELECTED));
                break;
            case ID__GETPUBLICKEY:
                GetKeyForSelectedItem(m_hListBox);
                break;
            case ID__COPYKEYTOCLIPBOARD:
                {
                    Identity& ident = GetSelectedIdentity(m_hListBox);
                    if (ident.pubkey_cached.empty()) {
                        GetKeyForSelectedItem(m_hListBox);
                    }
                    CopyPubkeyToClipboard(m_hListBox);
                }
                break;
            case ID__CLEARPUBLICKEY:
                {
                    Identity& ident = GetSelectedIdentity(m_hListBox);
                    ident.pubkey_cached = byte_array();
                }
                break;
            }
        }
        break;
    case WM_SIZING:
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_NOERASE | RDW_INTERNALPAINT);
        return DefWindowProc(hwnd, Message, wParam, lParam);
    case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            resizeControls(width, height);
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
        }
        break;
    case WM_GETMINMAXINFO:
        ((LPMINMAXINFO)lParam)->ptMinTrackSize.x = windowStartScreenSize.right - windowStartScreenSize.left;
        ((LPMINMAXINFO)lParam)->ptMinTrackSize.y = windowStartScreenSize.bottom - windowStartScreenSize.top;
        break;
    case WM_CLOSE:
        //DestroyWindow(hwnd);
        EndDialog(hwnd, 0);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

// Callback for notification menu
LRESULT CALLBACK NotificationIconCallback(HWND hwnd, unsigned int message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
    case WM_CREATE:
        // add the notification icon
        if (!AddNotificationIcon(hwnd))
        {
            return -1;
        }
        break;
    case WM_COMMAND:
    {
        int const wmId = LOWORD(wParam);
        
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_MNGIDENT:
            {
                // find window by caption and focus
                WCHAR szTitle[100];
                LoadString(g_hInst, IDS_MNGIDENT_TITLE, szTitle, ARRAYSIZE(szTitle));
                HWND windowHandle = FindWindow(NULL, szTitle);
                if (windowHandle == NULL) {
                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_MNGIDENT), hwnd, (DLGPROC)ManageIdentProc);
                }
                else {
                    SetFocus(windowHandle);
                }
            }   
            break;
        case IDM_EXIT:
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
    }
    break;

    /*
        TODO: handle ledger detection/removal
    case WM_DEVICECHANGE:
    {
        switch (wParam)
        {
        case DBT_DEVICEARRIVAL:
            // added device, could've been ledger
            LOG_DBG("device arrival");
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            // removed device, could've been ledger
            LOG_DBG("device removal");
            break;
        case DBT_DEVNODES_CHANGED:
            // device updated, could've been ledger?
            LOG_DBG("device changed");
            break;
        default:
            break;
        };
    }
    break;*/

    case WMAPP_NOTIFYCALLBACK:
        switch (LOWORD(lParam))
        {
            case WM_CONTEXTMENU:
            {
                POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
                ShowContextMenu(hwnd, pt);
                break;
            }
            default:
                break;
        }
        break;
    case WM_DESTROY:
        DeleteNotificationIcon();
        PostQuitMessage(0);
        break;
    case WM_COPYDATA:
        {
            PCOPYDATASTRUCT cds = (PCOPYDATASTRUCT)lParam;
            const char* str = reinterpret_cast<const char*>(cds->lpData);
                                  
            Application* app = Window::GetPtr()->getApp();
            MemoryMap& mapTransfer = app->getOrCreateMap(str);
            bool success = app->handleMemoryMap(mapTransfer);

            return success ? 1 : 0;
        }
        return 0;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void RegisterWindowClass(HINSTANCE instance, LPCWSTR pszClassName, LPCWSTR pszMenuName, WNDPROC lpfnWndProc) {
    WNDCLASSEX wcex = {sizeof(wcex)};
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = lpfnWndProc;
    wcex.hInstance      = instance;
    wcex.hIcon          = LoadIcon(instance, TEXT("IDI_NOTIFICATIONICON"));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = pszMenuName;
    wcex.lpszClassName  = pszClassName;
    RegisterClassEx(&wcex);
}

Window* Window::GetPtr() {
    if (!gWindow) {
        gWindow = new Window();
    }

    return gWindow;
}

void Window::DeletePtr() {
    if (gWindow) {
        delete gWindow;
        gWindow = nullptr;
    }
}

HWND Window::make(HINSTANCE instance) {
    g_hInst = instance;
	RegisterWindowClass(instance, szWindowClass, MAKEINTRESOURCE(IDC_NOTIFICATIONICON), NotificationIconCallback);
	
    WCHAR szTitle[100];
    LoadString(instance, IDS_APP_TITLE, szTitle, ARRAYSIZE(szTitle));

    HWND retHandle = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 250, 200, NULL, NULL, g_hInst, NULL);

    return retHandle;
}
