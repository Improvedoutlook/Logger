#include <windows.h>
#include <stdio.h>
#include <time.h>

#define ID_INPUT 1
#define ID_ADD 2
#define ID_EXPORT 3

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

        hwndExportBtn = CreateWindow(
            "BUTTON",
            "Export",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            140, 290, 100, 30,
            hwnd,
            (HMENU)ID_EXPORT,
            GetModuleHandle(NULL),
            NULL
        );
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_ADD:
            AddLogEntry(hwndInput);
            break;
        case ID_EXPORT:
            ExportLog();
            break;
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

    FILE *file = fopen("WorkLog.txt", "a");
    if (!file) {
        MessageBox(NULL, "Could not open log file!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(file, "[%02d:%02d] %s\n", t->tm_hour, t->tm_min, text);
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