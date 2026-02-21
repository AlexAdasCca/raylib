/*******************************************************************************************
*
*   raylib [core] example - keyboard testbed
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   NOTE: raylib defined keys refer to ENG-US Keyboard layout, 
*   mapping to other layouts is up to the user
*
*   Example originally created with raylib 5.6, last time updated with raylib 5.6
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2026 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define  KEY_REC_SPACING      4       // Space in pixels between key rectangles

//------------------------------------------------------------------------------------
// Module Functions Declaration
//------------------------------------------------------------------------------------
static const char *GetKeyText(int key);
static void GuiKeyboardKey(RLRectangle bounds, int key);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - keyboard testbed");
    RLSetExitKey(RL_E_KEY_NULL); // Avoid exit on KEY_ESCAPE

    // Keyboard line 01
    int line01KeyWidths[15] = { 0 };
    for (int i = 0; i < 15; i++) line01KeyWidths[i] = 45;
    line01KeyWidths[13] = 62;   // PRINTSCREEN
    int line01Keys[15] = { 
        RL_E_KEY_ESCAPE, RL_E_KEY_F1, RL_E_KEY_F2, RL_E_KEY_F3, RL_E_KEY_F4, RL_E_KEY_F5, 
        RL_E_KEY_F6, RL_E_KEY_F7, RL_E_KEY_F8, RL_E_KEY_F9, RL_E_KEY_F10, RL_E_KEY_F11, 
        RL_E_KEY_F12, RL_E_KEY_PRINT_SCREEN, RL_E_KEY_PAUSE 
    };
    
    // Keyboard line 02
    int line02KeyWidths[15] = { 0 };
    for (int i = 0; i < 15; i++) line02KeyWidths[i] = 45;
    line02KeyWidths[0] = 25;    // GRAVE
    line02KeyWidths[13] = 82;   // BACKSPACE
    int line02Keys[15] = { 
        RL_E_KEY_GRAVE, RL_E_KEY_ONE, RL_E_KEY_TWO, RL_E_KEY_THREE, RL_E_KEY_FOUR, 
        RL_E_KEY_FIVE, RL_E_KEY_SIX, RL_E_KEY_SEVEN, RL_E_KEY_EIGHT, RL_E_KEY_NINE, 
        RL_E_KEY_ZERO, RL_E_KEY_MINUS, RL_E_KEY_EQUAL, RL_E_KEY_BACKSPACE, RL_E_KEY_DELETE };

    // Keyboard line 03
    int line03KeyWidths[15] = { 0 };
    for (int i = 0; i < 15; i++) line03KeyWidths[i] = 45;
    line03KeyWidths[0] = 50;    // TAB
    line03KeyWidths[13] = 57;   // BACKSLASH
    int line03Keys[15] = {
        RL_E_KEY_TAB, RL_E_KEY_Q, RL_E_KEY_W, RL_E_KEY_E, RL_E_KEY_R, RL_E_KEY_T, RL_E_KEY_Y,
        RL_E_KEY_U, RL_E_KEY_I, RL_E_KEY_O, RL_E_KEY_P, RL_E_KEY_LEFT_BRACKET,
        RL_E_KEY_RIGHT_BRACKET, RL_E_KEY_BACKSLASH, RL_E_KEY_INSERT
    };

    // Keyboard line 04
    int line04KeyWidths[14] = { 0 };
    for (int i = 0; i < 14; i++) line04KeyWidths[i] = 45;
    line04KeyWidths[0] = 68;    // CAPS
    line04KeyWidths[12] = 88;   // ENTER
    int line04Keys[14] = {
        RL_E_KEY_CAPS_LOCK, RL_E_KEY_A, RL_E_KEY_S, RL_E_KEY_D, RL_E_KEY_F, RL_E_KEY_G,
        RL_E_KEY_H, RL_E_KEY_J, RL_E_KEY_K, RL_E_KEY_L, RL_E_KEY_SEMICOLON,
        RL_E_KEY_APOSTROPHE, RL_E_KEY_ENTER, RL_E_KEY_PAGE_UP
    };

    // Keyboard line 05
    int line05KeyWidths[14] = { 0 };
    for (int i = 0; i < 14; i++) line05KeyWidths[i] = 45;
    line05KeyWidths[0] = 80;    // LSHIFT
    line05KeyWidths[11] = 76;   // RSHIFT
    int line05Keys[14] = {
        RL_E_KEY_LEFT_SHIFT, RL_E_KEY_Z, RL_E_KEY_X, RL_E_KEY_C, RL_E_KEY_V, RL_E_KEY_B,
        RL_E_KEY_N, RL_E_KEY_M, RL_E_KEY_COMMA, RL_E_KEY_PERIOD, /*KEY_MINUS*/
        RL_E_KEY_SLASH, RL_E_KEY_RIGHT_SHIFT, RL_E_KEY_UP, RL_E_KEY_PAGE_DOWN
    };

    // Keyboard line 06
    int line06KeyWidths[11] = { 0 };
    for (int i = 0; i < 11; i++) line06KeyWidths[i] = 45;
    line06KeyWidths[0] = 80;    // LCTRL
    line06KeyWidths[3] = 208;   // SPACE
    line06KeyWidths[7] = 60;    // RCTRL
    int line06Keys[11] = {
        RL_E_KEY_LEFT_CONTROL, RL_E_KEY_LEFT_SUPER, RL_E_KEY_LEFT_ALT,
        RL_E_KEY_SPACE, RL_E_KEY_RIGHT_ALT, 162, RL_E_KEY_NULL,
        RL_E_KEY_RIGHT_CONTROL, RL_E_KEY_LEFT, RL_E_KEY_DOWN, RL_E_KEY_RIGHT
    };
    
    RLVector2 keyboardOffset = { 26, 80 };

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        int key = RLGetKeyPressed(); // Get pressed keycode
        if (key > 0) RLTraceLog(RL_E_LOG_INFO, "KEYBOARD TESTBED: KEY PRESSED:    %d", key);

        int ch = RLGetCharPressed(); // Get pressed char for text input, using OS mapping
        if (ch > 0) RLTraceLog(RL_E_LOG_INFO,  "KEYBOARD TESTBED: CHAR PRESSED:   %c (%d)", ch, ch);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawText("KEYBOARD LAYOUT: ENG-US", 26, 38, 20, LIGHTGRAY);
            
            // Keyboard line 01 - 15 keys
            // ESC, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, IMP, CLOSE
            for (int i = 0, recOffsetX = 0; i < 15; i++) 
            {
                GuiKeyboardKey((RLRectangle){ keyboardOffset.x + recOffsetX, keyboardOffset.y, line01KeyWidths[i], 30 }, line01Keys[i]);
                recOffsetX += line01KeyWidths[i] + KEY_REC_SPACING;
            }
 
            // Keyboard line 02 - 15 keys
            // `, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, -, =, BACKSPACE, DEL
            for (int i = 0, recOffsetX = 0; i < 15; i++) 
            {
                GuiKeyboardKey((RLRectangle){ keyboardOffset.x + recOffsetX, keyboardOffset.y + 30 + KEY_REC_SPACING, line02KeyWidths[i], 38 }, line02Keys[i]);
                recOffsetX += line02KeyWidths[i] + KEY_REC_SPACING;
            }
            
            // Keyboard line 03 - 15 keys
            // TAB, Q, W, E, R, T, Y, U, I, O, P, [, ], \, INS
            for (int i = 0, recOffsetX = 0; i < 15; i++)
            {
                GuiKeyboardKey((RLRectangle){ keyboardOffset.x + recOffsetX, keyboardOffset.y + 30 + 38 + KEY_REC_SPACING*2, line03KeyWidths[i], 38 }, line03Keys[i]);
                recOffsetX += line03KeyWidths[i] + KEY_REC_SPACING;
            }

            // Keyboard line 04 - 14 keys
            // MAYUS, A, S, D, F, G, H, J, K, L, ;, ', ENTER, REPAG
            for (int i = 0, recOffsetX = 0; i < 14; i++)
            {
                GuiKeyboardKey((RLRectangle){ keyboardOffset.x + recOffsetX, keyboardOffset.y + 30 + 38*2 + KEY_REC_SPACING*3, line04KeyWidths[i], 38 }, line04Keys[i]);
                recOffsetX += line04KeyWidths[i] + KEY_REC_SPACING;
            }

            // Keyboard line 05 - 14 keys
            // LSHIFT, Z, X, C, V, B, N, M, ,, ., /, RSHIFT, UP, AVPAG
            for (int i = 0, recOffsetX = 0; i < 14; i++)
            {
                GuiKeyboardKey((RLRectangle){ keyboardOffset.x + recOffsetX, keyboardOffset.y + 30 + 38*3 + KEY_REC_SPACING*4, line05KeyWidths[i], 38 }, line05Keys[i]);
                recOffsetX += line05KeyWidths[i] + KEY_REC_SPACING;
            }

            // Keyboard line 06 - 11 keys
            // LCTRL, WIN, LALT, SPACE, ALTGR, \, FN, RCTRL, LEFT, DOWN, RIGHT
            for (int i = 0, recOffsetX = 0; i < 11; i++)
            {
                GuiKeyboardKey((RLRectangle){ keyboardOffset.x + recOffsetX, keyboardOffset.y + 30 + 38*4 + KEY_REC_SPACING*5, line06KeyWidths[i], 38 }, line06Keys[i]);
                recOffsetX += line06KeyWidths[i] + KEY_REC_SPACING;
            }

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definition
//------------------------------------------------------------------------------------
// Get keyboard keycode as text (US keyboard)
// NOTE: Mapping for other keyboard layouts can be done here
static const char *GetKeyText(int key)
{
    switch (key)
    {
        case RL_E_KEY_APOSTROPHE      : return "'";          // Key: '
        case RL_E_KEY_COMMA           : return ",";          // Key: ,
        case RL_E_KEY_MINUS           : return "-";          // Key: -
        case RL_E_KEY_PERIOD          : return ".";          // Key: .
        case RL_E_KEY_SLASH           : return "/";          // Key: /
        case RL_E_KEY_ZERO            : return "0";          // Key: 0
        case RL_E_KEY_ONE             : return "1";          // Key: 1
        case RL_E_KEY_TWO             : return "2";          // Key: 2
        case RL_E_KEY_THREE           : return "3";          // Key: 3
        case RL_E_KEY_FOUR            : return "4";          // Key: 4
        case RL_E_KEY_FIVE            : return "5";          // Key: 5
        case RL_E_KEY_SIX             : return "6";          // Key: 6
        case RL_E_KEY_SEVEN           : return "7";          // Key: 7
        case RL_E_KEY_EIGHT           : return "8";          // Key: 8
        case RL_E_KEY_NINE            : return "9";          // Key: 9
        case RL_E_KEY_SEMICOLON       : return ";";          // Key: ;
        case RL_E_KEY_EQUAL           : return "=";          // Key: =
        case RL_E_KEY_A               : return "A";          // Key: A | a
        case RL_E_KEY_B               : return "B";          // Key: B | b
        case RL_E_KEY_C               : return "C";          // Key: C | c
        case RL_E_KEY_D               : return "D";          // Key: D | d
        case RL_E_KEY_E               : return "E";          // Key: E | e
        case RL_E_KEY_F               : return "F";          // Key: F | f
        case RL_E_KEY_G               : return "G";          // Key: G | g
        case RL_E_KEY_H               : return "H";          // Key: H | h
        case RL_E_KEY_I               : return "I";          // Key: I | i
        case RL_E_KEY_J               : return "J";          // Key: J | j
        case RL_E_KEY_K               : return "K";          // Key: K | k
        case RL_E_KEY_L               : return "L";          // Key: L | l
        case RL_E_KEY_M               : return "M";          // Key: M | m
        case RL_E_KEY_N               : return "N";          // Key: N | n
        case RL_E_KEY_O               : return "O";          // Key: O | o
        case RL_E_KEY_P               : return "P";          // Key: P | p
        case RL_E_KEY_Q               : return "Q";          // Key: Q | q
        case RL_E_KEY_R               : return "R";          // Key: R | r
        case RL_E_KEY_S               : return "S";          // Key: S | s
        case RL_E_KEY_T               : return "T";          // Key: T | t
        case RL_E_KEY_U               : return "U";          // Key: U | u
        case RL_E_KEY_V               : return "V";          // Key: V | v
        case RL_E_KEY_W               : return "W";          // Key: W | w
        case RL_E_KEY_X               : return "X";          // Key: X | x
        case RL_E_KEY_Y               : return "Y";          // Key: Y | y
        case RL_E_KEY_Z               : return "Z";          // Key: Z | z
        case RL_E_KEY_LEFT_BRACKET    : return "[";          // Key: [
        case RL_E_KEY_BACKSLASH       : return "\\";         // Key: '\'
        case RL_E_KEY_RIGHT_BRACKET   : return "]";          // Key: ]
        case RL_E_KEY_GRAVE           : return "`";          // Key: `
        case RL_E_KEY_SPACE           : return "SPACE";      // Key: Space
        case RL_E_KEY_ESCAPE          : return "ESC";        // Key: Esc
        case RL_E_KEY_ENTER           : return "ENTER";      // Key: Enter
        case RL_E_KEY_TAB             : return "TAB";        // Key: Tab
        case RL_E_KEY_BACKSPACE       : return "BACK";       // Key: Backspace
        case RL_E_KEY_INSERT          : return "INS";        // Key: Ins
        case RL_E_KEY_DELETE          : return "DEL";        // Key: Del
        case RL_E_KEY_RIGHT           : return "RIGHT";      // Key: Cursor right
        case RL_E_KEY_LEFT            : return "LEFT";       // Key: Cursor left
        case RL_E_KEY_DOWN            : return "DOWN";       // Key: Cursor down
        case RL_E_KEY_UP              : return "UP";         // Key: Cursor up
        case RL_E_KEY_PAGE_UP         : return "PGUP";       // Key: Page up
        case RL_E_KEY_PAGE_DOWN       : return "PGDOWN";     // Key: Page down
        case RL_E_KEY_HOME            : return "HOME";       // Key: Home
        case RL_E_KEY_END             : return "END";        // Key: End
        case RL_E_KEY_CAPS_LOCK       : return "CAPS";       // Key: Caps lock
        case RL_E_KEY_SCROLL_LOCK     : return "LOCK";       // Key: Scroll down
        case RL_E_KEY_NUM_LOCK        : return "NUMLOCK";    // Key: Num lock
        case RL_E_KEY_PRINT_SCREEN    : return "PRINTSCR";   // Key: Print screen
        case RL_E_KEY_PAUSE           : return "PAUSE";      // Key: Pause
        case RL_E_KEY_F1              : return "F1";         // Key: F1
        case RL_E_KEY_F2              : return "F2";         // Key: F2
        case RL_E_KEY_F3              : return "F3";         // Key: F3
        case RL_E_KEY_F4              : return "F4";         // Key: F4
        case RL_E_KEY_F5              : return "F5";         // Key: F5
        case RL_E_KEY_F6              : return "F6";         // Key: F6
        case RL_E_KEY_F7              : return "F7";         // Key: F7
        case RL_E_KEY_F8              : return "F8";         // Key: F8
        case RL_E_KEY_F9              : return "F9";         // Key: F9
        case RL_E_KEY_F10             : return "F10";        // Key: F10
        case RL_E_KEY_F11             : return "F11";        // Key: F11
        case RL_E_KEY_F12             : return "F12";        // Key: F12
        case RL_E_KEY_LEFT_SHIFT      : return "LSHIFT";     // Key: Shift left
        case RL_E_KEY_LEFT_CONTROL    : return "LCTRL";      // Key: Control left
        case RL_E_KEY_LEFT_ALT        : return "LALT";       // Key: Alt left
        case RL_E_KEY_LEFT_SUPER      : return "WIN";        // Key: Super left
        case RL_E_KEY_RIGHT_SHIFT     : return "RSHIFT";     // Key: Shift right
        case RL_E_KEY_RIGHT_CONTROL   : return "RCTRL";      // Key: Control right
        case RL_E_KEY_RIGHT_ALT       : return "ALTGR";      // Key: Alt right
        case RL_E_KEY_RIGHT_SUPER     : return "RSUPER";     // Key: Super right
        case RL_E_KEY_KB_MENU         : return "KBMENU";     // Key: KB menu
        case RL_E_KEY_KP_0            : return "KP0";        // Key: Keypad 0
        case RL_E_KEY_KP_1            : return "KP1";        // Key: Keypad 1
        case RL_E_KEY_KP_2            : return "KP2";        // Key: Keypad 2
        case RL_E_KEY_KP_3            : return "KP3";        // Key: Keypad 3
        case RL_E_KEY_KP_4            : return "KP4";        // Key: Keypad 4
        case RL_E_KEY_KP_5            : return "KP5";        // Key: Keypad 5
        case RL_E_KEY_KP_6            : return "KP6";        // Key: Keypad 6
        case RL_E_KEY_KP_7            : return "KP7";        // Key: Keypad 7
        case RL_E_KEY_KP_8            : return "KP8";        // Key: Keypad 8
        case RL_E_KEY_KP_9            : return "KP9";        // Key: Keypad 9
        case RL_E_KEY_KP_DECIMAL      : return "KPDEC";      // Key: Keypad .
        case RL_E_KEY_KP_DIVIDE       : return "KPDIV";      // Key: Keypad /
        case RL_E_KEY_KP_MULTIPLY     : return "KPMUL";      // Key: Keypad *
        case RL_E_KEY_KP_SUBTRACT     : return "KPSUB";      // Key: Keypad -
        case RL_E_KEY_KP_ADD          : return "KPADD";      // Key: Keypad +
        case RL_E_KEY_KP_ENTER        : return "KPENTER";    // Key: Keypad Enter
        case RL_E_KEY_KP_EQUAL        : return "KPEQU";      // Key: Keypad =
        default: return "";
    }
}

// Draw keyboard key
static void GuiKeyboardKey(RLRectangle bounds, int key)
{
    if (key == RL_E_KEY_NULL) RLDrawRectangleLinesEx(bounds, 2.0f, LIGHTGRAY);
    else
    {
        if (RLIsKeyDown(key))
        {
            RLDrawRectangleLinesEx(bounds, 2.0f, MAROON);
            RLDrawText(GetKeyText(key), bounds.x + 4, bounds.y + 4, 10, MAROON);
        }
        else
        {
            RLDrawRectangleLinesEx(bounds, 2.0f, DARKGRAY);
            RLDrawText(GetKeyText(key), bounds.x + 4, bounds.y + 4, 10, DARKGRAY);
        }
    }
    
    if (RLCheckCollisionPointRec(RLGetMousePosition(), bounds)) 
    {
        RLDrawRectangleRec(bounds, RLFade(RED, 0.2f));
        RLDrawRectangleLinesEx(bounds, 3.0f, RED);
    }
}