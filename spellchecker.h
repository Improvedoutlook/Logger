#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H

#include <windows.h>

typedef struct {
    DWORD startPos;
    DWORD endPos;
    char word[256];
} MisspelledWord;

typedef struct {
    MisspelledWord *words;
    int count;
    int capacity;
} MisspelledWordList;

typedef struct {
    char **words;
    int count;
    int capacity;
} Dictionary;

typedef struct {
    BOOL enabled;
    BOOL suggestionsEnabled;
    Dictionary mainDictionary;
    Dictionary userDictionary;
    Dictionary ignoredWords;
    MisspelledWordList misspelled;
    DWORD lastCheckTime;
} SpellChecker;

// Initialization and cleanup
SpellChecker* SpellChecker_Create(void);
void SpellChecker_Destroy(SpellChecker *sc);
BOOL SpellChecker_LoadDictionary(SpellChecker *sc, const char *filePath);
BOOL SpellChecker_LoadUserDictionary(SpellChecker *sc, const char *filePath);

// Spell checking
void SpellChecker_Check(SpellChecker *sc, const char *text);
BOOL SpellChecker_IsWordCorrect(SpellChecker *sc, const char *word);

// User dictionary management
void SpellChecker_AddToUserDictionary(SpellChecker *sc, const char *word);
void SpellChecker_SaveUserDictionary(SpellChecker *sc, const char *filePath);

// Ignore list management (session-only, not persisted)
void SpellChecker_AddToIgnoreList(SpellChecker *sc, const char *word);
void SpellChecker_ClearIgnoreList(SpellChecker *sc);

// Suggestions
char** SpellChecker_GetSuggestions(SpellChecker *sc, const char *word, int *count);
void SpellChecker_FreeSuggestions(char **suggestions, int count);

// Query results
MisspelledWordList* SpellChecker_GetMisspelledWords(SpellChecker *sc);
BOOL SpellChecker_IsMisspelledAtPosition(SpellChecker *sc, DWORD pos, char *outWord, int outWordLen);

#endif // SPELLCHECKER_H
