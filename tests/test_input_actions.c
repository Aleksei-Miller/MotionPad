#include <stdio.h>
#include <string.h>
#include <windows.h>

static int g_failures = 0;

static void expectInt(const char *name, int expected, int actual)
{
    if (expected != actual) {
        fprintf(stderr, "FAIL: %s expected=%d actual=%d\n", name, expected, actual);
        ++g_failures;
    }
}

static void expectBool(const char *name, int expected, int actual)
{
    if (expected != actual) {
        fprintf(stderr, "FAIL: %s expected=%s actual=%s\n", 
            name, expected ? "true" : "false", actual ? "true" : "false");
        ++g_failures;
    }
}

typedef enum ActionKind {
    ActionKind_None = 0,
    ActionKind_Keyboard,
    ActionKind_MouseButton,
    ActionKind_MouseWheel,
    ActionKind_MouseWheel_Pos,
    ActionKind_MouseWheel_Neg,
    ActionKind_XboxButton,
    ActionKind_AltMode
} ActionKind;

static int resolveKeyboardVk(const wchar_t *name)
{
    if (_wcsicmp(name, L"a") == 0) return 0x41;
    if (_wcsicmp(name, L"w") == 0) return 0x57;
    if (_wcsicmp(name, L"s") == 0) return 0x53;
    if (_wcsicmp(name, L"d") == 0) return 0x44;
    if (_wcsicmp(name, L"enter") == 0) return 0x0D;
    if (_wcsicmp(name, L"space") == 0) return 0x20;
    if (_wcsicmp(name, L"shift") == 0) return 0x10;
    if (_wcsicmp(name, L"ctrl") == 0) return 0x11;
    if (_wcsicmp(name, L"alt") == 0) return 0x12;
    return 0;
}

static int parseBindingToken(const wchar_t *token, ActionKind *out_kind, UINT *out_value)
{
    const wchar_t *colon = wcschr(token, L':');
    if (!colon) return 0;
    
    wchar_t prefix[16];
    int prefix_len = colon - token;
    if (prefix_len >= sizeof(prefix) / sizeof(wchar_t)) return 0;
    wcsncpy(prefix, token, prefix_len);
    prefix[prefix_len] = L'\0';
    
    const wchar_t *name = colon + 1;
    
    if (_wcsicmp(prefix, L"k") == 0) {
        *out_kind = ActionKind_Keyboard;
        *out_value = resolveKeyboardVk(name);
        return (*out_value != 0);
    }
    
    if (_wcsicmp(prefix, L"m") == 0) {
        if (_wcsicmp(name, L"lb") == 0) { *out_kind = ActionKind_MouseButton; *out_value = 0x01; return 1; }
        if (_wcsicmp(name, L"rb") == 0) { *out_kind = ActionKind_MouseButton; *out_value = 0x02; return 1; }
        if (_wcsicmp(name, L"mb") == 0) { *out_kind = ActionKind_MouseButton; *out_value = 0x04; return 1; }
        if (_wcsicmp(name, L"mw+") == 0) { *out_kind = ActionKind_MouseWheel_Pos; *out_value = 1; return 1; }
        if (_wcsicmp(name, L"mw-") == 0) { *out_kind = ActionKind_MouseWheel_Neg; *out_value = 1; return 1; }
        if (_wcsicmp(name, L"mw") == 0) { *out_kind = ActionKind_MouseWheel; *out_value = 1; return 1; }
        return 0;
    }
    
    if (_wcsicmp(prefix, L"x") == 0) {
        *out_kind = ActionKind_XboxButton;
        if (_wcsicmp(name, L"a") == 0) { *out_value = 0x1000; return 1; }
        if (_wcsicmp(name, L"b") == 0) { *out_value = 0x2000; return 1; }
        if (_wcsicmp(name, L"x") == 0) { *out_value = 0x4000; return 1; }
        if (_wcsicmp(name, L"y") == 0) { *out_value = 0x8000; return 1; }
        if (_wcsicmp(name, L"start") == 0) { *out_value = 0x0010; return 1; }
        return 0;
    }
    
    if (_wcsicmp(prefix, L"o") == 0) {
        if (_wcsicmp(name, L"alt_mode") == 0) { *out_kind = ActionKind_AltMode; return 1; }
        return 0;
    }
    
    return 0;
}

static void test_parseKeyboardBinding(void)
{
    printf("=== test_parseKeyboardBinding ===\n");
    
    ActionKind kind;
    UINT value;
    
    expectBool("k:a", 1, parseBindingToken(L"k:a", &kind, &value));
    expectInt("k:a kind", ActionKind_Keyboard, kind);
    expectInt("k:a value", 0x41, value);
    
    expectBool("k:w", 1, parseBindingToken(L"k:w", &kind, &value));
    expectInt("k:w kind", ActionKind_Keyboard, kind);
    expectInt("k:w value", 0x57, value);
    
    expectBool("k:enter", 1, parseBindingToken(L"k:enter", &kind, &value));
    expectInt("k:enter kind", ActionKind_Keyboard, kind);
    expectInt("k:enter value", 0x0D, value);
    
    expectBool("k:space", 1, parseBindingToken(L"k:space", &kind, &value));
    expectInt("k:space kind", ActionKind_Keyboard, kind);
    expectInt("k:space value", 0x20, value);
}

static void test_parseMouseBinding(void)
{
    printf("=== test_parseMouseBinding ===\n");
    
    ActionKind kind;
    UINT value;
    
    expectBool("m:lb", 1, parseBindingToken(L"m:lb", &kind, &value));
    expectInt("m:lb kind", ActionKind_MouseButton, kind);
    expectInt("m:lb value", 0x01, value);
    
    expectBool("m:rb", 1, parseBindingToken(L"m:rb", &kind, &value));
    expectInt("m:rb kind", ActionKind_MouseButton, kind);
    expectInt("m:rb value", 0x02, value);
    
    expectBool("m:mb", 1, parseBindingToken(L"m:mb", &kind, &value));
    expectInt("m:mb kind", ActionKind_MouseButton, kind);
    expectInt("m:mb value", 0x04, value);
}

static void test_parseMouseWheelBinding(void)
{
    printf("=== test_parseMouseWheelBinding ===\n");
    
    ActionKind kind;
    UINT value;
    
    expectBool("m:mw+", 1, parseBindingToken(L"m:mw+", &kind, &value));
    expectInt("m:mw+ kind", ActionKind_MouseWheel_Pos, kind);
    
    expectBool("m:mw-", 1, parseBindingToken(L"m:mw-", &kind, &value));
    expectInt("m:mw- kind", ActionKind_MouseWheel_Neg, kind);
    
    expectBool("m:mw", 1, parseBindingToken(L"m:mw", &kind, &value));
    expectInt("m:mw kind", ActionKind_MouseWheel, kind);
}

static void test_parseXboxBinding(void)
{
    printf("=== test_parseXboxBinding ===\n");
    
    ActionKind kind;
    UINT value;
    
    expectBool("x:a", 1, parseBindingToken(L"x:a", &kind, &value));
    expectInt("x:a kind", ActionKind_XboxButton, kind);
    expectInt("x:a value", 0x1000, value);
    
    expectBool("x:b", 1, parseBindingToken(L"x:b", &kind, &value));
    expectInt("x:b kind", ActionKind_XboxButton, kind);
    expectInt("x:b value", 0x2000, value);
    
    expectBool("x:start", 1, parseBindingToken(L"x:start", &kind, &value));
    expectInt("x:start kind", ActionKind_XboxButton, kind);
    expectInt("x:start value", 0x0010, value);
}

static void test_parseInternalBinding(void)
{
    printf("=== test_parseInternalBinding ===\n");
    
    ActionKind kind;
    UINT value;
    
    expectBool("o:alt_mode", 1, parseBindingToken(L"o:alt_mode", &kind, &value));
    expectInt("o:alt_mode kind", ActionKind_AltMode, kind);
}

static void test_parseInvalidBinding(void)
{
    printf("=== test_parseInvalidBinding ===\n");
    
    ActionKind kind;
    UINT value;
    
    expectBool("invalid", 0, parseBindingToken(L"invalid", &kind, &value));
    expectBool("x:unknown", 0, parseBindingToken(L"x:unknown", &kind, &value));
    expectBool("k:", 0, parseBindingToken(L"k:", &kind, &value));
    expectBool(":a", 0, parseBindingToken(L":a", &kind, &value));
}

int main(void)
{
    printf("MotionPad Unit Tests\n");
    printf("===================\n\n");
    
    test_parseKeyboardBinding();
    test_parseMouseBinding();
    test_parseMouseWheelBinding();
    test_parseXboxBinding();
    test_parseInternalBinding();
    test_parseInvalidBinding();
    
    printf("\n===================\n");
    if (g_failures == 0) {
        printf("All tests PASSED (%d failures)\n", g_failures);
        return 0;
    } else {
        printf("Tests FAILED (%d failures)\n", g_failures);
        return 1;
    }
}
