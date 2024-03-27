#ifndef INPUT_H
#define INPUT_H

struct Input {
    int cursor_px, cursor_py;
    int last_cursor_px, last_cursor_py;
    int delta_x, delta_y;
};

enum Keycode {
    Key_Begin = 0,
    
    Key_Enter  = 0x0D,
    Key_Shift  = 0x10,
    Key_Ctrl   = 0x11,
    Key_Alt    = 0x12,

    Key_Escape = 0x1B,

    Key_Left = 0x25,
    Key_Up,
    Key_Right,
    Key_Down,

    Key_0 = 0x30,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,

    Key_A = 0x41,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,

    Key_Last,
};

#endif // INPUT_H
