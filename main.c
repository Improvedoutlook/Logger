#include <windows.h>
#include <stdio.h>
#include <time.h>

// Global variables for view/edit mode
static BOOL isViewMode = FALSE;
static HWND hwndSaveBtn = NULL;
static HWND hwndCancelBtn = NULL;
static char originalContent[4096] = {0};

#define ID_INPUT 1
#define ID_ADD 2
#define ID_VIEW 3
#define ID_EXPORT 4
#define ID_SAVE 5
#define ID_CANCEL 6

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AddLogEntry(HWND hwndInput);
void ExportLog();

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "WorkLogAggregatorClass";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);  // Set window background color

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Work Log Aggregator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// Handle messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hwndInput, hwndAddBtn, hwndExportBtn;

    switch (uMsg) {
    case WM_CREATE:
        hwndInput = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            "EDIT",
            "",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
            20, 20, 440, 250,
            hwnd,
            (HMENU)ID_INPUT,
            GetModuleHandle(NULL),
            NULL
        );

        hwndAddBtn = CreateWindow(
            "BUTTON",
            "Add Entry",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 290, 100, 30,
            hwnd,
            (HMENU)ID_ADD,
            GetModuleHandle(NULL),
            NULL
        );

        CreateWindow(
            "BUTTON",
            "View Entry",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            140, 290, 100, 30,
            hwnd,
            (HMENU)ID_VIEW,
            GetModuleHandle(NULL),
            NULL
        );

        hwndExportBtn = CreateWindow(
            "BUTTON",
            "Export",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            260, 290, 100, 30,
            hwnd,
            (HMENU)ID_EXPORT,
            GetModuleHandle(NULL),
            NULL
        );
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_ADD:
            if (!isViewMode) {
                AddLogEntry(hwndInput);
            }
            break;
        case ID_VIEW:
            if (!isViewMode) {
                // Load content from file
                FILE *file = fopen("WorkLog.txt", "r");
                if (!file) {
                    MessageBox(NULL, "No entries to view!", "Error", MB_OK | MB_ICONERROR);
                    break;
                }

                // Read file content and normalize line endings to CRLF for the edit control
                char raw[4096] = {0};
                size_t bytesRead = fread(raw, 1, sizeof(raw) - 1, file);
                fclose(file);

                if (bytesRead == 0) {
                    MessageBox(NULL, "No entries to view!", "Error", MB_OK | MB_ICONERROR);
                    break;
                }

                // Convert lone LF to CRLF so the Windows edit control shows new lines correctly.
                // Use a larger buffer for converted content.
                char converted[8192] = {0};
                size_t ri = 0, wi = 0;
                for (ri = 0; ri < bytesRead && wi + 2 < sizeof(converted); ++ri) {
                    unsigned char c = raw[ri];
                    if (c == '\r') {
                        // keep CR as-is
                        converted[wi++] = '\r';
                        // If next is LF, it'll be handled in next iteration and appended
                    } else if (c == '\n') {
                        // If previous char wasn't CR, insert CR before LF
                        if (ri == 0 || raw[ri - 1] != '\r') {
                            converted[wi++] = '\r';
                        }
                        converted[wi++] = '\n';
                    } else {
                        converted[wi++] = c;
                    }
                }

                // Ensure null-terminated
                converted[wi] = '\0';

                // Store original content (raw) for possible later comparison
                strncpy(originalContent, converted, sizeof(originalContent) - 1);

                // Show content in input box
                SetWindowText(hwndInput, converted);

                // Hide regular buttons and show Save/Cancel buttons
                ShowWindow(hwndAddBtn, SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, ID_VIEW), SW_HIDE);
                ShowWindow(hwndExportBtn, SW_HIDE);

                // Create Save and Cancel buttons
                hwndSaveBtn = CreateWindow(
                    "BUTTON", "Save Changes",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                    20, 290, 100, 30,
                    hwnd, (HMENU)ID_SAVE,
                    GetModuleHandle(NULL), NULL
                );

                hwndCancelBtn = CreateWindow(
                    "BUTTON", "Cancel",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                    140, 290, 100, 30,
                    hwnd, (HMENU)ID_CANCEL,
                    GetModuleHandle(NULL), NULL
                );

                isViewMode = TRUE;
            }
            break;
        case ID_SAVE:
            if (isViewMode) {
                // Get current content
                char newContent[4096];
                GetWindowText(hwndInput, newContent, sizeof(newContent));

                // Save to file
                FILE *file = fopen("WorkLog.txt", "w");
                if (file) {
                    fputs(newContent, file);
                    fclose(file);
                    MessageBox(NULL, "Changes saved successfully!", "Success", MB_OK | MB_ICONINFORMATION);
                }

                // Clear the input field before exiting view mode
                SetWindowText(hwndInput, "");
                
                // Exit view mode
                goto exit_view_mode;
            }
            break;
        case ID_CANCEL:
            if (isViewMode) {
                // Clear the input field
                SetWindowText(hwndInput, "");
exit_view_mode:
                // Clean up view mode
                DestroyWindow(hwndSaveBtn);
                DestroyWindow(hwndCancelBtn);
                hwndSaveBtn = hwndCancelBtn = NULL;

                // Show regular buttons
                ShowWindow(hwndAddBtn, SW_SHOW);
                ShowWindow(GetDlgItem(hwnd, ID_VIEW), SW_SHOW);
                ShowWindow(hwndExportBtn, SW_SHOW);

                isViewMode = FALSE;
            }
            break;
        case ID_EXPORT:
            if (!isViewMode) {
                ExportLog();
            }
            break;
        }
        break;

    case WM_ERASEBKGND:
        {
            RECT rect;
            HDC hdc = (HDC)wParam;
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
            return TRUE;
        }
        break;

    case WM_SIZE:
        {
            // Get the dimensions of the client area
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            
            // Calculate margins and spacing
            int margin = 20;
            int buttonHeight = 30;
            int buttonWidth = 100;
            int buttonSpacing = 20;
            
            // Calculate the height for the input box (leave space for buttons at bottom)
            int inputHeight = rcClient.bottom - (margin * 2 + buttonHeight);
            
            // Resize the input box
            MoveWindow(hwndInput,
                margin,                                    // x position
                margin,                                    // y position
                rcClient.right - (margin * 2),            // width
                inputHeight - margin,                      // height
                TRUE);
            
            // Position the buttons at the bottom
            MoveWindow(hwndAddBtn,
                margin,                                    // x position
                rcClient.bottom - (margin + buttonHeight), // y position
                buttonWidth,                              // width
                buttonHeight,                             // height
                TRUE);
                
            // Position View Entry button
            MoveWindow(GetDlgItem(hwnd, ID_VIEW),
                margin + buttonWidth + buttonSpacing,      // x position
                rcClient.bottom - (margin + buttonHeight), // y position
                buttonWidth,                              // width
                buttonHeight,                             // height
                TRUE);

            // Position Export button
            MoveWindow(hwndExportBtn,
                margin + (buttonWidth + buttonSpacing) * 2, // x position
                rcClient.bottom - (margin + buttonHeight),  // y position
                buttonWidth,                               // width
                buttonHeight,                              // height
                TRUE);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Add an entry to today's log
void AddLogEntry(HWND hwndInput) {
    char text[1024];
    GetWindowText(hwndInput, text, sizeof(text));

    if (strlen(text) == 0) {
        MessageBox(NULL, "Please enter a note before adding.", "No Entry", MB_OK | MB_ICONWARNING);
        return;
    }

    // Open file for update so we can check last character
    FILE *file = fopen("WorkLog.txt", "a+");
    if (!file) {
        MessageBox(NULL, "Could not open log file!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Move to end and check if file is non-empty and doesn't end with a newline
    int needSep = 0;
    if (fseek(file, 0, SEEK_END) == 0) {
        long pos = ftell(file);
        if (pos > 0) {
            // Go back one char to read last byte
            if (fseek(file, -1, SEEK_END) == 0) {
                int last = fgetc(file);
                if (last != '\n' && last != '\r') {
                    needSep = 1;
                }
            }
        }
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    if (needSep) {
        // write CRLF separator
        fputc('\r', file);
        fputc('\n', file);
    }

    // Convert to 12-hour format with am/pm
    int hour12 = t->tm_hour % 12;
    if (hour12 == 0) hour12 = 12; // midnight or noon -> 12
    const char *ampm = (t->tm_hour >= 12) ? "pm" : "am";
    // Write entry with CRLF line ending and am/pm
    fprintf(file, "[%d:%02d%s] %s\r\n", hour12, t->tm_min, ampm, text);
    fclose(file);

    SetWindowText(hwndInput, ""); // clear input box
    MessageBox(NULL, "Entry added to WorkLog.txt!", "Success", MB_OK | MB_ICONINFORMATION);
}

// Export the log (just copies to a daily file)
void ExportLog() {
    char buffer[1024];
    FILE *src = fopen("WorkLog.txt", "r");
    if (!src) {
        MessageBox(NULL, "No log file found!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char filename[64];
    sprintf(filename, "WorkLog_%04d-%02d-%02d.txt",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);

    FILE *dest = fopen(filename, "w");
    if (!dest) {
        MessageBox(NULL, "Could not create export file!", "Error", MB_OK | MB_ICONERROR);
        fclose(src);
        return;
    }

    while (fgets(buffer, sizeof(buffer), src)) {
        fputs(buffer, dest);
    }

    fclose(src);
    fclose(dest);

    MessageBox(NULL, "Daily log exported!", "Export Complete", MB_OK | MB_ICONINFORMATION);
}