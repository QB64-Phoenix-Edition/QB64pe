//----------------------------------------------------------------------------------------------------------------------
// QB64-PE graphics support
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

struct img_struct {
    void *lock_offset;
    int64_t lock_id;
    uint8_t valid;   // 0,1 0=invalid
    uint8_t text;    // if set, surface is a text surface
    uint8_t console; // dummy surface to absorb unimplemented console functionality
    uint16_t width, height;
    uint8_t bytes_per_pixel;  // 1,2,4
    uint8_t bits_per_pixel;   // 1,2,4,8,16(text),32
    uint32_t mask;            // 1,3,0xF,0xFF,0xFFFF,0xFFFFFFFF
    uint16_t compatible_mode; // 0,1,2,7,8,9,10,11,12,13,32,256
    uint32_t color, background_color, draw_color;
    uint32_t font;               // 8,14,16,?
    int16_t top_row, bottom_row; // VIEW PRINT settings, unique (as in QB) to each "page"
    int16_t cursor_x, cursor_y;  // unique (as in QB) to each "page"
    uint8_t cursor_show, cursor_firstvalue, cursor_lastvalue;

    union {
        uint8_t *offset;
        uint32_t *offset32;
    };

    uint32_t flags;
    uint32_t *pal;
    int32_t transparent_color; //-1 means no color is transparent
    uint8_t alpha_disabled;
    uint8_t holding_cursor;
    uint8_t print_mode;
    // BEGIN apm ('active page migration')
    // everything between apm points is migrated during active page changes
    // note: apm data is only relevant to graphics modes
    uint8_t apm_p1;
    int32_t view_x1, view_y1, view_x2, view_y2;
    int32_t view_offset_x, view_offset_y;
    float x, y;
    uint8_t clipping_or_scaling;
    float scaling_x, scaling_y, scaling_offset_x, scaling_offset_y;
    float window_x1, window_y1, window_x2, window_y2;
    double draw_ta;
    double draw_scale;
    uint8_t apm_p2;
    // END apm
};

// img_struct flags
#define IMG_FREEPAL 1 // free palette data before freeing image
#define IMG_SCREEN 2  // img is linked to other screen pages
#define IMG_FREEMEM 4 // if set, it means memory must be freed

// used by HSB/RGB color conversion routines
struct hsb_color
{
    double h; // [0,360] hue
    double s; // [0,1]   saturation
    double b; // [0,1]   brightness
};
struct rgb_color
{
    double r; // [0,1] red
    double g; // [0,1] green
    double b; // [0,1] blue
};

/********** Render State **********/
/*
    Apart from 'glTexParameter' based settings (with are texture specific)
    all other OpenGL states are global.
    This means when switching between dest FBOs a complete state change is inevitable.
*/
struct RENDER_STATE_DEST { // could be the primary render target or a FBO
    int32_t ignore;        // at present no relevant states appear to be FBO specific
};

struct RENDER_STATE_SOURCE { // texture states
    int32_t smooth_stretched;
    int32_t smooth_shrunk;
    int32_t texture_wrap;
    int32_t PO2_fix;
};

struct RENDER_STATE_GLOBAL { // settings not bound to specific source/target
    RENDER_STATE_DEST *dest;
    RENDER_STATE_SOURCE *source;
    int32_t dest_handle;
    int32_t source_handle;
    int32_t view_mode;
    int32_t use_alpha;
    int32_t depthbuffer_mode;
    int32_t cull_mode;
};

#define VIEW_MODE__UNKNOWN 0
#define VIEW_MODE__2D 1
#define VIEW_MODE__3D 2
#define VIEW_MODE__RESET 3
#define ALPHA_MODE__UNKNOWN -1
#define ALPHA_MODE__DONT_BLEND 0
#define ALPHA_MODE__BLEND 1
#define TEXTURE_WRAP_MODE__UNKNOWN -1
#define TEXTURE_WRAP_MODE__DONT_WRAP 0
#define TEXTURE_WRAP_MODE__WRAP 1
#define SMOOTH_MODE__UNKNOWN -1
#define SMOOTH_MODE__DONT_SMOOTH 0
#define SMOOTH_MODE__SMOOTH 1
#define PO2_FIX__OFF 0
#define PO2_FIX__EXPANDED 1
#define PO2_FIX__MIPMAPPED 2

#define DEPTHBUFFER_MODE__UNKNOWN -1
#define DEPTHBUFFER_MODE__OFF 0
#define DEPTHBUFFER_MODE__ON 1
#define DEPTHBUFFER_MODE__LOCKED 2
#define DEPTHBUFFER_MODE__CLEAR 3
#define CULL_MODE__UNKNOWN -1
#define CULL_MODE__NONE 0
#define CULL_MODE__CLOCKWISE_ONLY 1
#define CULL_MODE__ANTICLOCKWISE_ONLY 2
/********** Render State **********/

#define INVALID_HARDWARE_HANDLE -1

struct hardware_img_struct {
    int32_t w;
    int32_t h;
    int32_t texture_handle;          // if 0, imports from software_pixel_buffer automatically
    int32_t dest_context_handle;     // used when rendering other images onto this image
    int32_t depthbuffer_handle;      // generated when 3D commands are called
    int32_t pending_commands;        // incremented with each command, decremented after command is processed
    int32_t remove;                  // if =1, free immediately after all pending commands are processed
    uint32_t *software_pixel_buffer; // if NULL, generates a blank texture
    int32_t alpha_disabled;          // changed by _BLEND/_DONTBLEND commands
    int32_t depthbuffer_mode;        // changed by _DEPTHBUFFER
    int32_t valid;
    RENDER_STATE_SOURCE source_state;
    RENDER_STATE_DEST dest_state;
    int32_t PO2_w; // if PO2_FIX__EXPANDED/MIPMAPPED, these are the texture size
    int32_t PO2_h;
};

struct hardware_graphics_command_struct {
    int64_t order;        // which _DISPLAY event to bind the operation to
    int32_t next_command; // the handle of the next hardware_graphics_command of the same display-order, of 0 if last
    int64_t command;      // the command type, actually a set of bit flags

    // Bit 00: Decimal value 000001: _PUTIMAGE
    union {
        int32_t option;
        int32_t src_img; // MUST be a hardware handle
    };

    union {
        int32_t dst_img; // MUST be a hardware handle or 0 for the default 2D rendering context
        int32_t target;
    };

    float src_x1;
    float src_y1;
    float src_x2;
    float src_y2;
    float src_x3;
    float src_y3;
    float dst_x1;
    float dst_y1;
    float dst_z1;
    float dst_x2;
    float dst_y2;
    float dst_z2;
    float dst_x3;
    float dst_y3;
    float dst_z3;
    int32_t smooth; // 0 or 1 (whether to apply texture filtering)
    int32_t cull_mode;
    int32_t depthbuffer_mode;
    int32_t use_alpha; // 0 or 1 (whether to refer to the alpha component of pixel values)
    int32_t remove;
};

#define HARDWARE_GRAPHICS_COMMAND__PUTIMAGE 1
#define HARDWARE_GRAPHICS_COMMAND__FREEIMAGE_REQUEST 2
#define HARDWARE_GRAPHICS_COMMAND__FREEIMAGE 3
#define HARDWARE_GRAPHICS_COMMAND__MAPTRIANGLE 4
#define HARDWARE_GRAPHICS_COMMAND__MAPTRIANGLE3D 5
#define HARDWARE_GRAPHICS_COMMAND__CLEAR_DEPTHBUFFER 6

uint32_t func__hsb32(double hue, double sat, double bri);
uint32_t func__hsba32(double hue, double sat, double bri, double alf);
double func__hue32(uint32_t argb);
double func__sat32(uint32_t argb);
double func__bri32(uint32_t argb);

void sub__depthbuffer(int32_t options, int32_t dst, int32_t passed);
void sub__maptriangle(int32_t cull_options, float sx1, float sy1, float sx2, float sy2, float sx3, float sy3, int32_t si, float dx1, float dy1, float dz1,
                      float dx2, float dy2, float dz2, float dx3, float dy3, float dz3, int32_t di, int32_t smooth_options, int32_t passed);
