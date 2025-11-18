#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "spellchecker.h"

// Helper macros for mouse position extraction
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

// Spell checker globals
static SpellChecker *g_spellChecker = NULL;
static HWND g_hwndInput = NULL;
static UINT_PTR g_spellCheckTimer = 0;
static DWORD g_lastSpellCheckTime = 0;
static BOOL g_spellCheckEnabled = TRUE;
static int g_contextMenuWordIndex = -1;
static HWND g_hwndTooltip = NULL;

// Global variables for view/edit mode
static BOOL isViewMode = FALSE;
static HWND hwndSaveBtn = NULL;
static HWND hwndCancelBtn = NULL;
static char originalContent[4096] = {0};
static char mainInputBackup[4096] = {0};

#define ID_INPUT 1
#define ID_ADD 2
#define ID_VIEW 3
#define ID_EXPORT 4
#define ID_SAVE 5
#define ID_CANCEL 6
#define ID_SPELLCHECK_TIMER 100
#define ID_CONTEXT_MENU_SUGGESTION_BASE 1000
#define ID_CONTEXT_MENU_ADD_DICT 1100
#define ID_CONTEXT_MENU_IGNORE 1101
#define SPELLCHECK_DEBOUNCE_MS 150

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AddLogEntry(HWND hwndInput);
void ExportLog();
void InitializeSpellChecker(void);
void CleanupSpellChecker(void);
void TriggerSpellCheck(void);
void CALLBACK SpellCheckTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
void DrawMisspelledUnderlines(HWND hwnd);
BOOL HandleSpellCheckContextMenu(HWND hwnd, int xPos, int yPos);
void ReplaceWord(const char *oldWord, const char *newWord);

// Keep original edit control procedure so we can forward messages we don't handle
static WNDPROC g_oldEditProc = NULL;

// Initialize spell checker at startup
void InitializeSpellChecker(void) {
    g_spellChecker = SpellChecker_Create();
    if (g_spellChecker) {
        if (!SpellChecker_LoadDictionary(g_spellChecker, "dictionary.txt")) {
            MessageBox(NULL, "Warning: Could not load spell-check dictionary. Spell checking disabled.", 
                      "Dictionary Load Error", MB_OK | MB_ICONWARNING);
            g_spellCheckEnabled = FALSE;
        } else {
            SpellChecker_LoadUserDictionary(g_spellChecker, "user_dictionary.txt");
            g_spellCheckEnabled = TRUE;
        }
    }
}

// Cleanup spell checker at shutdown
void CleanupSpellChecker(void) {
    if (g_spellCheckTimer) {
        KillTimer(NULL, g_spellCheckTimer);
        g_spellCheckTimer = 0;
    }
    if (g_spellChecker) {
        SpellChecker_SaveUserDictionary(g_spellChecker, "user_dictionary.txt");
        SpellChecker_Destroy(g_spellChecker);
        g_spellChecker = NULL;
    }
}

// Trigger spell check with debouncing
void TriggerSpellCheck(void) {
    if (!g_spellCheckEnabled || !g_spellChecker) return;
    
    // Kill existing timer if any
    if (g_spellCheckTimer) {
        KillTimer(NULL, g_spellCheckTimer);
    }
    
    // Set new timer for debounced spell check
    g_spellCheckTimer = SetTimer(NULL, ID_SPELLCHECK_TIMER, SPELLCHECK_DEBOUNCE_MS, SpellCheckTimerProc);
}

// Timer callback for spell checking
void CALLBACK SpellCheckTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    if (!g_spellCheckEnabled || !g_spellChecker || !g_hwndInput) return;
    
    // Get text from edit control
    int textLen = GetWindowTextLength(g_hwndInput);
    if (textLen == 0) {
        if (g_spellChecker->misspelled.count > 0) {
            g_spellChecker->misspelled.count = 0;
            InvalidateRect(g_hwndInput, NULL, FALSE);
        }
        goto cleanup;
    }
    
    char *text = (char *)malloc(textLen + 1);
    if (!text) goto cleanup;
    
    GetWindowText(g_hwndInput, text, textLen + 1);
    
    // Perform spell check
    SpellChecker_Check(g_spellChecker, text);
    
    // Create or update tooltip with misspelled words
    if (g_spellChecker->misspelled.count > 0) {
        char tooltipText[512] = "Misspelled words:\n";
        for (int i = 0; i < g_spellChecker->misspelled.count && i < 10; i++) {
            strcat(tooltipText, "- ");
            strcat(tooltipText, g_spellChecker->misspelled.words[i].word);
            strcat(tooltipText, "\n");
        }
        
        // Set title bar to indicate spell errors
        HWND hwndMain = GetParent(g_hwndInput);
        if (hwndMain) {
            char titleText[256];
            snprintf(titleText, sizeof(titleText), "Logger- Your Work Log Aggregator! - %d spelling error(s)", 
                    g_spellChecker->misspelled.count);
            SetWindowText(hwndMain, titleText);
        }
    } else {
        // Restore normal title
        HWND hwndMain = GetParent(g_hwndInput);
        if (hwndMain) {
            SetWindowText(hwndMain, "Logger- Your Work Log Aggregator!");
        }
    }
    
    // Trigger repaint
    InvalidateRect(g_hwndInput, NULL, FALSE);
    UpdateWindow(g_hwndInput);
    
    free(text);

cleanup:
    // Kill timer after spell check
    if (g_spellCheckTimer) {
        KillTimer(NULL, g_spellCheckTimer);
        g_spellCheckTimer = 0;
    }
}

// Replace a word in the text
void ReplaceWord(const char *oldWord, const char *newWord) {
    if (!g_hwndInput || !oldWord || !newWord) return;
    
    int textLen = GetWindowTextLength(g_hwndInput);
    if (textLen == 0) return;
    
    char *text = (char *)malloc(textLen + 1);
    if (!text) return;
    
    GetWindowText(g_hwndInput, text, textLen + 1);
    
    // Simple find and replace (replaces first occurrence)
    char *pos = strstr(text, oldWord);
    if (pos) {
        int oldLen = strlen(oldWord);
        int newLen = strlen(newWord);
        
        // Create new text with replacement
        char *newText = (char *)malloc(textLen + newLen - oldLen + 1);
        if (newText) {
            int offset = pos - text;
            strncpy(newText, text, offset);
            strcpy(newText + offset, newWord);
            strcpy(newText + offset + newLen, text + offset + oldLen);
            
            SetWindowText(g_hwndInput, newText);
            TriggerSpellCheck();
            
            free(newText);
        }
    }
    
    free(text);
}

// Handle right-click context menu for misspelled words
BOOL HandleSpellCheckContextMenu(HWND hwnd, int xPos, int yPos) {
    if (!g_spellChecker || g_spellChecker->misspelled.count == 0) return FALSE;
    
    // Get text to find word at position
    int textLen = GetWindowTextLength(hwnd);
    if (textLen == 0) return FALSE;
    
    char *text = (char *)malloc(textLen + 1);
    if (!text) return FALSE;
    
    GetWindowText(hwnd, text, textLen + 1);
    
    // Get cursor position (approximate from click point)
    POINT pt = {xPos, yPos};
    ScreenToClient(hwnd, &pt);
    
    // Find a misspelled word near the cursor
    char misspelledWord[256] = {0};
    int wordIndex = -1;
    
    for (int i = 0; i < g_spellChecker->misspelled.count; i++) {
        if (pt.x >= g_spellChecker->misspelled.words[i].startPos * 6 && 
            pt.x <= g_spellChecker->misspelled.words[i].endPos * 6) {
            strcpy(misspelledWord, g_spellChecker->misspelled.words[i].word);
            wordIndex = i;
            break;
        }
    }
    
    if (wordIndex < 0) {
        free(text);
        return FALSE;
    }
    
    // Create context menu
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) {
        free(text);
        return FALSE;
    }
    
    // Get suggestions
    int suggestCount = 0;
    char **suggestions = SpellChecker_GetSuggestions(g_spellChecker, misspelledWord, &suggestCount);
    
    // Add suggestion options
    if (suggestions && suggestCount > 0) {
        for (int i = 0; i < suggestCount && i < 5; i++) {
            AppendMenu(hMenu, MF_STRING, ID_CONTEXT_MENU_SUGGESTION_BASE + i, suggestions[i]);
        }
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        SpellChecker_FreeSuggestions(suggestions, suggestCount);
    } else {
        AppendMenu(hMenu, MF_STRING | MF_GRAYED, 0, "No suggestions");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    }
    
    // Add dictionary options
    AppendMenu(hMenu, MF_STRING, ID_CONTEXT_MENU_ADD_DICT, "Add to Dictionary");
    AppendMenu(hMenu, MF_STRING, ID_CONTEXT_MENU_IGNORE, "Ignore");
    
    // Show context menu
    int selection = TrackPopupMenu(hMenu, TPM_RETURNCMD, xPos, yPos, 0, hwnd, NULL);
    
    // Handle menu selection
    if (selection >= ID_CONTEXT_MENU_SUGGESTION_BASE && selection < ID_CONTEXT_MENU_SUGGESTION_BASE + 10) {
        // User selected a suggestion
        char **sugg = SpellChecker_GetSuggestions(g_spellChecker, misspelledWord, &suggestCount);
        if (sugg && selection - ID_CONTEXT_MENU_SUGGESTION_BASE < suggestCount) {
            ReplaceWord(misspelledWord, sugg[selection - ID_CONTEXT_MENU_SUGGESTION_BASE]);
            SpellChecker_FreeSuggestions(sugg, suggestCount);
        }
    } else if (selection == ID_CONTEXT_MENU_ADD_DICT) {
        SpellChecker_AddToUserDictionary(g_spellChecker, misspelledWord);
        TriggerSpellCheck();
    } else if (selection == ID_CONTEXT_MENU_IGNORE) {
        // Add word to ignore list for this session
        SpellChecker_AddToIgnoreList(g_spellChecker, misspelledWord);
        TriggerSpellCheck();
    }
    
    DestroyMenu(hMenu);
    free(text);
    return TRUE;
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "WorkLogAggregatorClass";

    // Initialize spell checker
    InitializeSpellChecker();

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);  // Set window background color

    // Load and set the icon from file
    wc.hIcon = (HICON)LoadImage(NULL, "Logger_icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Logger - Your Work Log Aggregator!",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        CleanupSpellChecker();
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CleanupSpellChecker();
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

        // Subclass the edit control so we can handle Ctrl+A (select all)
        if (hwndInput) {
            g_hwndInput = hwndInput;  // Store for spell checker
            g_oldEditProc = (WNDPROC)SetWindowLongPtr(hwndInput, GWLP_WNDPROC, (LONG_PTR)EditProc);
        }

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
                // Backup whatever the user has typed in the main input so we can restore it
                if (hwndInput) {
                    GetWindowText(hwndInput, mainInputBackup, sizeof(mainInputBackup));
                }
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

                // Show content in input box (this temporarily replaces what was in the main input)
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

                // Restore the user's previous main input (preserve what they were typing)
                SetWindowText(hwndInput, mainInputBackup);
                
                // Exit view mode
                goto exit_view_mode;
            }
            break;
        case ID_CANCEL:
            if (isViewMode) {
                // Restore the user's previous main input (preserve what they were typing)
                SetWindowText(hwndInput, mainInputBackup);
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

// Subclassed edit control procedure to support Ctrl+A for 'select all' and spell checking
LRESULT CALLBACK EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_KEYDOWN:
        // Check for Ctrl+A (VK_CONTROL held + 'A' or 'a')
        if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'A' || wParam == 'a')) {
            // Select all text: start = 0, end = -1 selects all
            SendMessage(hwnd, EM_SETSEL, 0, -1);
            return 0; // handled
        }
        // Trigger spell check on any key press
        TriggerSpellCheck();
        break;
    
    case WM_RBUTTONUP:
        // Handle right-click for spell check suggestions
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            POINT pt;
            pt.x = xPos;
            pt.y = yPos;
            ClientToScreen(hwnd, &pt);
            
            if (!HandleSpellCheckContextMenu(hwnd, pt.x, pt.y)) {
                // No misspelled word found, show default context menu
                if (g_oldEditProc) {
                    return CallWindowProc(g_oldEditProc, hwnd, uMsg, wParam, lParam);
                }
            }
            return 0;
        }
    }

    // Forward other messages to the original window procedure
    if (g_oldEditProc) {
        return CallWindowProc(g_oldEditProc, hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}