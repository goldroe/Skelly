#include "input.h"

global Input _input;

global bool _keys_down[256];
global bool _keys_pressed[256];

internal HMM_Vec2
get_cursor_delta() {
    return HMM_V2((float)_input.delta_x, (float)_input.delta_y);
}

internal bool
input_button_down(Keycode key) {
    assert(key > Key_Begin && key < Key_Last);
    return _keys_down[(int)key];
}

internal bool
input_button_pressed(Keycode key) {
    assert(key > Key_Begin && key < Key_Last);
    return _keys_pressed[(int)key];
}

internal void
input_reset_keys_pressed() {
    for (int i = 0; i < 256; i++) {
        _keys_pressed[i] = false;
    }
}
