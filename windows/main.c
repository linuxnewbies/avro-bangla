/*
 * Avro Bangla - Windows IME (Input Method Editor) Implementation
 * This creates a system-wide Bangla phonetic keyboard for Windows 10/11
 * 
 * Features:
 * - System-wide phonetic typing
 * - Works with Win+Space to switch keyboards
 * - No command prompt needed for installation (use installer.exe)
 */

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <imm.h>
#include <msctf.h>
#include <stdio.h>
#include "avro_engine.h"

/* GUID for our IME */
static const GUID CLSID_AvroBanglaIME = {
    0x12345678, 0x1234, 0x5678,
    { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 }
};

/* Global engine instance */
static AvroEngine g_engine;
static BOOL g_initialized = FALSE;

/* DLL Entry Point */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        if (!g_initialized) {
            avro_engine_init(&g_engine);
            g_initialized = TRUE;
        }
        break;
    case DLL_PROCESS_DETACH:
        if (g_initialized) {
            avro_engine_cleanup(&g_engine);
            g_initialized = FALSE;
        }
        break;
    }
    return TRUE;
}

/* Convert UTF-8 Bengali to wide string */
static int utf8_to_wide(const char* utf8, wchar_t* wide, int max_len) {
    int i = 0, j = 0;
    while (utf8[i] && j < max_len - 1) {
        unsigned char c = utf8[i];
        if (c < 0x80) {
            wide[j++] = c;
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= (int)strlen(utf8)) break;
            wide[j++] = ((c & 0x1F) << 6) | (utf8[i+1] & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= (int)strlen(utf8)) break;
            wide[j++] = ((c & 0x0F) << 12) | ((utf8[i+1] & 0x3F) << 6) | (utf8[i+2] & 0x3F);
            i += 3;
        } else {
            i++;
        }
    }
    wide[j] = L'\0';
    return j;
}

/* IME Processing Function */
static BOOL ProcessKey(HIMC himc, WPARAM wParam, LPARAM lParam) {
    if (!g_initialized) return FALSE;
    
    /* Only process key down events */
    if (lParam & 0x80000000) return FALSE;
    
    /* Get virtual key code */
    BYTE vkCode = (BYTE)(wParam & 0xFF);
    
    /* Convert virtual key to character */
    char ch = 0;
    HKL hkl = GetKeyboardLayout(0);
    BYTE keystate[256] = {0};
    
    /* Handle special keys */
    if (vkCode == VK_BACK) {
        ch = '\b';
    } else if (vkCode == VK_SPACE) {
        ch = ' ';
    } else if (vkCode >= 'A' && vkCode <= 'Z') {
        /* Check shift state */
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
            ch = (char)vkCode;  /* Uppercase */
        } else {
            ch = (char)(vkCode + 32);  /* Lowercase */
        }
    } else if (vkCode >= '0' && vkCode <= '9') {
        ch = (char)vkCode;
    } else {
        /* Try to get character from ToAscii */
        WORD ascii = 0;
        if (ToAsciiEx(vkCode, (lParam >> 16) & 0xFF, keystate, &ascii, hkl) == 1) {
            ch = (char)ascii;
        }
    }
    
    if (ch == 0) return FALSE;
    
    /* Process through Avro engine */
    wchar_t output[MAX_OUTPUT_BUFFER_LEN];
    int result = avro_engine_process_char(&g_engine, ch, output, MAX_OUTPUT_BUFFER_LEN);
    
    if (result > 0) {
        /* We have converted text - compose it */
        COMPOSITIONSTRING* comp = ImmLockCompStr(himc);
        if (comp) {
            /* Clear existing composition */
            comp->dwCompStrLen = 0;
            
            /* Set new composition string */
            LPWSTR compStr = (LPWSTR)((BYTE*)comp + comp->dwCompStrOffset);
            wcsncpy(compStr, output, result);
            comp->dwCompStrLen = result;
            
            /* Set cursor at end */
            comp->dwCursorPos = result;
            
            ImmUnlockCompStr(himc);
            ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
            return TRUE;
        }
    }
    
    /* Update candidate window if we have pre-edit text */
    wchar_t preedit[MAX_OUTPUT_BUFFER_LEN];
    int preedit_len = avro_engine_get_preedit(&g_engine, preedit, MAX_OUTPUT_BUFFER_LEN);
    
    if (preedit_len > 0) {
        COMPOSITIONSTRING* comp = ImmLockCompStr(himc);
        if (comp) {
            LPWSTR compStr = (LPWSTR)((BYTE*)comp + comp->dwCompStrOffset);
            wcsncpy(compStr, preedit, preedit_len);
            comp->dwCompStrLen = preedit_len;
            comp->dwCursorPos = preedit_len;
            ImmUnlockCompStr(himc);
            return TRUE;
        }
    }
    
    return FALSE;
}

/* Exported IME functions */

/*
 * ImeInquire - Return IME information
 */
EXPORT BOOL WINAPI ImeInquire(LPIMEINFO lpImeInfo) {
    if (!lpImeInfo) return FALSE;
    
    lpImeInfo->dwPrivateDataSize = 0;
    lpImeInfo->fdwProperty = IME_PROP_AT_CARET | IME_PROP_SPECIAL_UI;
    lpImeInfo->fdwConversionCaps = IME_CMODE_NATIVE;
    lpImeInfo->fdwSelectCaps = 0;
    
    return TRUE;
}

/*
 * ImeConfigure - Configure IME (not implemented)
 */
EXPORT BOOL WINAPI ImeConfigure(HWND hAppWnd, HIMC himc, DWORD dwMode, LPVOID lpData) {
    return TRUE;
}

/*
 * ImeConversionList - Get conversion candidates (not implemented)
 */
EXPORT DWORD WINAPI ImeConversionList(HIMC himc, LPCSTR lpSrc, LPMSGLLPTR lpMsg,
                                       DWORD dwBufLen, UINT uFlag) {
    return 0;
}

/*
 * ImeProcessKey - Process keyboard input
 */
EXPORT BOOL WINAPI ImeProcessKey(HIMC himc, WPARAM wParam, LPARAM lParam, 
                                  LPTRANSMSG lpTransMsg) {
    return ProcessKey(himc, wParam, lParam);
}

/*
 * ImeSelect - Select/deselect IME
 */
EXPORT BOOL WINAPI ImeSelect(HIMC himc, BOOL fSelect) {
    if (fSelect) {
        avro_engine_reset(&g_engine);
    }
    return TRUE;
}

/*
 * ImeSetActiveContext - Set active context
 */
EXPORT BOOL WINAPI ImeSetActiveContext(HIMC himc, BOOL fActive) {
    return TRUE;
}

/*
 * ImeSetCompositionString - Set composition string
 */
EXPORT BOOL WINAPI ImeSetCompositionString(HIMC himc, DWORD dwIndex, 
                                            LPVOID lpComp, DWORD dwCompLen,
                                            LPVOID lpRead, DWORD dwReadLen) {
    return TRUE;
}

/*
 * ImeNotifyIME - Notify IME of events
 */
EXPORT BOOL WINAPI ImeNotifyIME(HIMC himc, DWORD dwAction, DWORD dwIndex, DWORD dwValue) {
    switch (dwAction) {
    case NI_OPENCANDIDATE:
    case NI_CLOSECANDIDATE:
    case NI_SELECTCANDIDATE:
    case NI_CHANGECANDIDATELIST:
        return TRUE;
    default:
        return FALSE;
    }
}

/*
 * ImeGetImeMenuItems - Get IME menu items
 */
EXPORT BOOL WINAPI ImeGetImeMenuItems(HIMC himc, UINT uFlags, UINT uType,
                                       LPIMEMENUITEMINFOA lpImeParentMenu,
                                       LPIMEMENUITEMINFOA lpImeMenu, UINT uCount) {
    return FALSE;
}

/*
 * ImeEscape - Escape function
 */
EXPORT LRESULT WINAPI ImeEscape(HIMC himc, UINT uEscape, LPVOID lpData) {
    return 0;
}

/* Registration functions */

/*
 * Register the IME with Windows
 */
EXPORT BOOL WINAPI RegisterIME(void) {
    HKEY hKey;
    LONG result;
    
    /* Create registry key for IME */
    result = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\IMM\\ImeFile",
        0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    
    if (result != ERROR_SUCCESS) {
        return FALSE;
    }
    
    /* Set IME file path */
    wchar_t imePath[MAX_PATH];
    GetModuleFileNameW(NULL, imePath, MAX_PATH);
    
    result = RegSetValueExW(hKey, L"Avro Bangla", 0, REG_SZ,
                            (BYTE*)imePath, (wcslen(imePath) + 1) * sizeof(wchar_t));
    
    RegCloseKey(hKey);
    
    return (result == ERROR_SUCCESS);
}

/*
 * Unregister the IME
 */
EXPORT BOOL WINAPI UnregisterIME(void) {
    HKEY hKey;
    LONG result;
    
    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\IMM\\ImeFile",
        0, KEY_WRITE, &hKey);
    
    if (result != ERROR_SUCCESS) {
        return FALSE;
    }
    
    result = RegDeleteValueW(hKey, L"Avro Bangla");
    RegCloseKey(hKey);
    
    return (result == ERROR_SUCCESS);
}
