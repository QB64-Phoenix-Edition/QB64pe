//----------------------------------------------------------------------------------------------------------------------
// QB64-PE graphics support
//----------------------------------------------------------------------------------------------------------------------

#include "graphics.h"
#include "error_handle.h"
#include "libqb-common.h"
#include "qblist.h"
#include "rounding.h"
#include <cstring>

// External functions. These should be moved here in the future.
void flush_old_hardware_commands();
void validatepage(int32_t pageNumber);

// Global variables. These should be cleaned up and moved here in the future.
extern list *hardware_img_handles;
extern int32_t HARDWARE_IMG_HANDLE_OFFSET;
extern list *hardware_graphics_command_handles;
extern int64_t display_frame_order_next;
extern int32_t last_hardware_command_added;
extern int32_t first_hardware_command;
extern int32_t nextimg;
extern int32_t *page;
extern img_struct *img;
extern img_struct *write_page;
extern img_struct *read_page;
extern img_struct *display_page;
extern uint8_t *cblend;
extern uint8_t *ablend;
extern uint8_t *ablend127;
extern uint8_t *ablend128;

// Module-level global variables
static int32_t depthbuffer_mode0 = DEPTHBUFFER_MODE__ON;
static int32_t depthbuffer_mode1 = DEPTHBUFFER_MODE__ON;

void hsb2rgb(hsb_color *hsb, rgb_color *rgb) {
    double hu, hi, hf, pv, qv, tv;

    if (hsb->s == 0.0) {
        rgb->r = hsb->b; rgb->g = hsb->b; rgb->b = hsb->b; // no saturation = grayscale
    } else {
        hu = hsb->h / 60.0;  // to sixtant [0,5]
        if (hu >= 6.0) hu = hu - 6.0;
        hf = modf(hu, &hi);  // int/frac parts of hue
        pv = hsb->b * (1.0 - hsb->s);
        qv = hsb->b * (1.0 - (hsb->s * hf));
        tv = hsb->b * (1.0 - (hsb->s * (1.0 - hf)));
        switch (lround(hi)) {
            case 0: {rgb->r = hsb->b; rgb->g = tv; rgb->b = pv; break;} //   0- 60 = Red->Yellow
            case 1: {rgb->r = qv; rgb->g = hsb->b; rgb->b = pv; break;} //  60-120 = Yellow->Green
            case 2: {rgb->r = pv; rgb->g = hsb->b; rgb->b = tv; break;} // 120-180 = Green->Cyan
            case 3: {rgb->r = pv; rgb->g = qv; rgb->b = hsb->b; break;} // 180-240 = Cyan->Blue
            case 4: {rgb->r = tv; rgb->g = pv; rgb->b = hsb->b; break;} // 240-300 = Blue->Magenta
            case 5: {rgb->r = hsb->b; rgb->g = pv; rgb->b = qv; break;} // 300-360 = Magenta->Red
        }
    }
}

void rgb2hsb(rgb_color *rgb, hsb_color *hsb) {
    double mini, maxi, diff, hu;
    // --- find min/max and difference ---
    mini = fmin(fmin(rgb->r, rgb->g), rgb->b);
    maxi = fmax(fmax(rgb->r, rgb->g), rgb->b);
    diff = maxi - mini;
    // --- brightness ---
    hsb->b = maxi;
    // --- saturation (avoid division by zero) ---
    maxi != 0.0 ? hsb->s = diff / maxi : hsb->s = 0.0;
    // --- hue in degrees ---
    if (hsb->s != 0.0) {
        if (rgb->r == maxi) {
            hu = ((rgb->g - rgb->b) / diff);       // between Yellow & Magenta
            if (hu < 0.0) hu = hu + 6.0;
        } else if (rgb->g == maxi) {
            hu = 2.0 + ((rgb->b - rgb->r) / diff); // between Cyan & Yellow
        } else {
            hu = 4.0 + ((rgb->r - rgb->g) / diff); // between Magenta & Cyan
        }
        hsb->h = hu * 60.0; // to degrees
    } else {
        hsb->h = 0.0; // technically there's no hue w/o saturation, commonly used is 0 (red)
    }
}

uint32_t func__hsb32(double hue, double sat, double bri) {
    hsb_color hsb; rgb_color rgb;
    // --- prepare values for conversion ---
    (hue < 0.0) ? hsb.h = 0.0 : ((hue > 360.0) ? hsb.h = 360.0 : hsb.h = hue);
    (sat < 0.0) ? hsb.s = 0.0 : ((sat > 100.0) ? hsb.s = 100.0 : hsb.s = sat);
    (bri < 0.0) ? hsb.b = 0.0 : ((bri > 100.0) ? hsb.b = 100.0 : hsb.b = bri);
    hsb.s /= 100.0; hsb.b /= 100.0; // range [0,1]
    // --- convert colorspace ---
    hsb2rgb(&hsb, &rgb);
    // --- build result ---
    return ((lround(rgb.r * 255.0) << 16) + (lround(rgb.g * 255.0) << 8) + lround(rgb.b * 255.0)) | 0xFF000000;
}

uint32_t func__hsba32(double hue, double sat, double bri, double alf) {
    hsb_color hsb; rgb_color rgb; double alpha;
    // --- prepare values for conversion ---
    (hue < 0.0) ? hsb.h = 0.0 : ((hue > 360.0) ? hsb.h = 360.0 : hsb.h = hue);
    (sat < 0.0) ? hsb.s = 0.0 : ((sat > 100.0) ? hsb.s = 100.0 : hsb.s = sat);
    (bri < 0.0) ? hsb.b = 0.0 : ((bri > 100.0) ? hsb.b = 100.0 : hsb.b = bri);
    (alf < 0.0) ? alpha = 0.0 : ((alf > 100.0) ? alpha = 100.0 : alpha = alf);
    hsb.s /= 100.0; hsb.b /= 100.0; alpha /= 100.0; // range [0,1]
    // --- convert colorspace ---
    hsb2rgb(&hsb, &rgb);
    // --- build result ---
    return (lround(alpha * 255.0) << 24) + (lround(rgb.r * 255.0) << 16) + (lround(rgb.g * 255.0) << 8) + lround(rgb.b * 255.0);
}

double func__hue32(uint32_t argb) {
    rgb_color rgb; hsb_color hsb;
    // --- prepare values for conversion ---
    rgb.r = ((argb >> 16) & 0xFF) / 255.0;
    rgb.g = ((argb >> 8) & 0xFF) / 255.0;
    rgb.b = (argb & 0xFF) / 255.0;
    // --- convert colorspace ---
    rgb2hsb(&rgb, &hsb);
    // --- build result ---
    return hsb.h;
}

double func__sat32(uint32_t argb) {
    rgb_color rgb; hsb_color hsb;
    // --- prepare values for conversion ---
    rgb.r = ((argb >> 16) & 0xFF) / 255.0;
    rgb.g = ((argb >> 8) & 0xFF) / 255.0;
    rgb.b = (argb & 0xFF) / 255.0;
    // --- convert colorspace ---
    rgb2hsb(&rgb, &hsb);
    // --- build result ---
    return hsb.s * 100.0;
}

double func__bri32(uint32_t argb) {
    rgb_color rgb; hsb_color hsb;
    // --- prepare values for conversion ---
    rgb.r = ((argb >> 16) & 0xFF) / 255.0;
    rgb.g = ((argb >> 8) & 0xFF) / 255.0;
    rgb.b = (argb & 0xFF) / 255.0;
    // --- convert colorspace ---
    rgb2hsb(&rgb, &hsb);
    // --- build result ---
    return hsb.b * 100.0;
}

void sub__depthbuffer(int32_t options, int32_t dst, int32_t passed) {
    //                    {ON|OFF|LOCK|_CLEAR}

    if (is_error_pending())
        return;

    if ((passed & 1) == 0)
        dst = 0; // the primary hardware surface is implied
    hardware_img_struct *dst_himg = NULL;
    if (dst < 0) {
        dst_himg = (hardware_img_struct *)list_get(hardware_img_handles, dst - HARDWARE_IMG_HANDLE_OFFSET);
        if (dst_himg == NULL) {
            error(QB_ERROR_INVALID_HANDLE);
            return;
        }
        dst -= HARDWARE_IMG_HANDLE_OFFSET;
    } else {
        if (dst > 1) {
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return;
        }
        dst = -dst;
    }

    if (options == 4) {
        flush_old_hardware_commands();
        int32_t hgch = list_add(hardware_graphics_command_handles);
        hardware_graphics_command_struct *hgc = (hardware_graphics_command_struct *)list_get(hardware_graphics_command_handles, hgch);
        hgc->remove = 0;
        // set command values
        hgc->command = HARDWARE_GRAPHICS_COMMAND__CLEAR_DEPTHBUFFER;
        hgc->dst_img = dst;
        // queue the command
        hgc->next_command = 0;
        hgc->order = display_frame_order_next;
        if (last_hardware_command_added) {
            hardware_graphics_command_struct *hgc2 =
                (hardware_graphics_command_struct *)list_get(hardware_graphics_command_handles, last_hardware_command_added);
            hgc2->next_command = hgch;
        }
        last_hardware_command_added = hgch;
        if (first_hardware_command == 0)
            first_hardware_command = hgch;
        return;
    }

    int32_t new_mode;
    if (options == 1) {
        new_mode = DEPTHBUFFER_MODE__ON;
    }
    if (options == 2) {
        new_mode = DEPTHBUFFER_MODE__OFF;
    }
    if (options == 3) {
        new_mode = DEPTHBUFFER_MODE__LOCKED;
    }

    if (dst == 0) {
        depthbuffer_mode0 = new_mode;
        return;
    }
    if (dst == -1) {
        depthbuffer_mode1 = new_mode;
        return;
    }
    dst_himg->depthbuffer_mode = new_mode;
}

void sub__maptriangle(int32_t cull_options, float sx1, float sy1, float sx2, float sy2, float sx3, float sy3, int32_t si, float fdx1, float fdy1, float fdz1,
                      float fdx2, float fdy2, float fdz2, float fdx3, float fdy3, float fdz3, int32_t di, int32_t smooth_options, int32_t passed) {
    //[{_CLOCKWISE|_ANTICLOCKWISE}][{_SEAMLESS}](?,?)-(?,?)-(?,?)[,?]{TO}(?,?[,?])-(?,?[,?])-(?,?[,?])[,[?][,{_SMOOTH|_SMOOTHSHRUNK|_SMOOTHSTRETCHED}]]"
    //  (1)       (2)              1                             2           4         8         16    32   (1)     (2)           (3)

    if (is_error_pending())
        return;

    static int32_t dwidth, dheight, swidth, sheight, swidth2, sheight2;
    static int32_t lhs, rhs, lhs1, lhs2, top, bottom, flats, flatg, final, tile, no_edge_overlap;
    flats = 0;
    final = 0;
    tile = 0;
    no_edge_overlap = 0;
    static int32_t v, i, x, x1, x2, y, y1, y2, z, h, ti, lhsi, rhsi, d;
    static int32_t g1x, g2x, g1tx, g2tx, g1ty, g2ty, g1xi, g2xi, g1txi, g2txi, g1tyi, g2tyi, tx, ty, txi, tyi, roff, loff;
    static int64_t i64;
    static img_struct *src, *dst;
    static uint8_t *pixel_offset;
    static uint32_t *pixel_offset32;
    static uint8_t *dst_offset;
    static uint32_t *dst_offset32;
    static uint8_t *src_offset;
    static uint32_t *src_offset32;
    static uint32_t col, destcol;
    static uint8_t *cp;

    // hardware support
    // is source a hardware handle?
    if (si) {

        static int32_t src, dst; // scope is a wonderful thing
        src = si;
        dst = di;
        hardware_img_struct *src_himg = (hardware_img_struct *)list_get(hardware_img_handles, src - HARDWARE_IMG_HANDLE_OFFSET);
        if (src_himg != NULL) { // source is hardware image
            src -= HARDWARE_IMG_HANDLE_OFFSET;

            flush_old_hardware_commands();

            // check dst
            hardware_img_struct *dst_himg = NULL;
            if (dst < 0) {
                dst_himg = (hardware_img_struct *)list_get(hardware_img_handles, dst - HARDWARE_IMG_HANDLE_OFFSET);
                if (dst_himg == NULL) {
                    error(QB_ERROR_INVALID_HANDLE);
                    return;
                }
                dst -= HARDWARE_IMG_HANDLE_OFFSET;
            } else {
                if (dst > 1) {
                    error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                    return;
                }
                dst = -dst;
            }

            static int32_t use3d;
            use3d = 0;
            if (passed & (4 + 8 + 16))
                use3d = 1;

            if ((passed & 1) == 1 && use3d == 0) {
                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                return;
            } // seamless not supported for 2D hardware version yet

            // create new command handle & structure
            int32_t hgch = list_add(hardware_graphics_command_handles);
            hardware_graphics_command_struct *hgc = (hardware_graphics_command_struct *)list_get(hardware_graphics_command_handles, hgch);

            hgc->remove = 0;

            // set command values
            if (use3d) {
                hgc->command = HARDWARE_GRAPHICS_COMMAND__MAPTRIANGLE3D;
                hgc->cull_mode = CULL_MODE__NONE;
                if (cull_options == 1)
                    hgc->cull_mode = CULL_MODE__CLOCKWISE_ONLY;
                if (cull_options == 2)
                    hgc->cull_mode = CULL_MODE__ANTICLOCKWISE_ONLY;
            } else {
                hgc->command = HARDWARE_GRAPHICS_COMMAND__MAPTRIANGLE;
            }

            hgc->src_img = src;
            hgc->src_x1 = sx1;
            hgc->src_y1 = sy1;
            hgc->src_x2 = sx2;
            hgc->src_y2 = sy2;
            hgc->src_x3 = sx3;
            hgc->src_y3 = sy3;

            hgc->dst_img = dst;
            hgc->dst_x1 = fdx1;
            hgc->dst_y1 = fdy1;
            hgc->dst_x2 = fdx2;
            hgc->dst_y2 = fdy2;
            hgc->dst_x3 = fdx3;
            hgc->dst_y3 = fdy3;
            if (use3d) {
                hgc->dst_z1 = fdz1;
                hgc->dst_z2 = fdz2;
                hgc->dst_z3 = fdz3;
                if (dst == 0)
                    hgc->depthbuffer_mode = depthbuffer_mode0;
                if (dst == -1)
                    hgc->depthbuffer_mode = depthbuffer_mode1;
                if (dst_himg != NULL) {
                    hgc->depthbuffer_mode = dst_himg->depthbuffer_mode;
                }
            }

            hgc->smooth = smooth_options;

            hgc->use_alpha = 1;
            if (src_himg->alpha_disabled)
                hgc->use_alpha = 0;
            // only consider dest alpha setting if it is a hardware image
            if (dst_himg != NULL) {
                if (dst_himg->alpha_disabled)
                    hgc->use_alpha = 0;
            }

            // queue the command
            hgc->next_command = 0;
            hgc->order = display_frame_order_next;

            if (last_hardware_command_added) {
                hardware_graphics_command_struct *hgc2 =
                    (hardware_graphics_command_struct *)list_get(hardware_graphics_command_handles, last_hardware_command_added);
                hgc2->next_command = hgch;
            }
            last_hardware_command_added = hgch;
            if (first_hardware_command == 0)
                first_hardware_command = hgch;

            return;
        }
    }

    if (passed & (4 + 8 + 16)) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    } // 3D not supported using software surfaces

    // recreate old calling convention
    static int32_t passed_original;
    passed_original = passed;
    passed = 0;
    if (passed_original & 1)
        passed += 1;
    if (passed_original & 2)
        passed += 2;
    if (passed_original & 32)
        passed += 4;
    if (passed_original & 64)
        passed += 8;

    static int32_t dx1, dy1, dx2, dy2, dx3, dy3;
    dx1 = qbr_float_to_long(fdx1);
    dy1 = qbr_float_to_long(fdy1);
    dx2 = qbr_float_to_long(fdx2);
    dy2 = qbr_float_to_long(fdy2);
    dx3 = qbr_float_to_long(fdx3);
    dy3 = qbr_float_to_long(fdy3);

    // get/validate src/dst images
    if (passed & 2) {
        if (si >= 0) { // validate si
            validatepage(si);
            si = page[si];
        } else {
            si = -si;
            if (si >= nextimg) {
                error(QB_ERROR_INVALID_HANDLE);
                return;
            }
            if (!img[si].valid) {
                error(QB_ERROR_INVALID_HANDLE);
                return;
            }
        }
        src = &img[si];
    } else {
        src = read_page;
    }
    if (passed & 4) {
        if (di >= 0) { // validate di
            validatepage(di);
            di = page[di];
        } else {
            di = -di;
            if (di >= nextimg) {
                error(QB_ERROR_INVALID_HANDLE);
                return;
            }
            if (!img[di].valid) {
                error(QB_ERROR_INVALID_HANDLE);
                return;
            }
        }
        dst = &img[di];
    } else {
        dst = write_page;
    }
    if (src->text || dst->text) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    if (src->bytes_per_pixel != dst->bytes_per_pixel) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }

    if (passed & 1)
        no_edge_overlap = 1;

    dwidth = dst->width;
    dheight = dst->height;
    swidth = src->width;
    sheight = src->height;
    swidth2 = swidth << 16;
    sheight2 = sheight << 16;

    struct PointType {
        int32_t x;
        int32_t y;
        int32_t tx;
        int32_t ty;
    };

    static PointType p[4], *p1, *p2, *tp, *tempp;

    struct GradientType {
        int32_t x;
        int32_t xi;
        int32_t tx;
        int32_t ty;
        int32_t txi;
        int32_t tyi;
        int32_t y1;
        int32_t y2;
        //----
        PointType *p1;
        PointType *p2; // needed for clipping above screen
    };

    static GradientType g[4], *tg, *g1, *g2, *g3, *tempg;
    memset(&g, 0, sizeof(GradientType) * 4);

    /*
        'Reference:
        'Fixed point division: a/b -> a*65536/b (using intermediate _INTEGER64)
    */

    /* debugging method
        std::ofstream f;
        char fn[] = "c:\\qb64\\20c.txt";
        f.open(fn, std::ios::app);
        f<<"\n";
        f<<variablename;
        f<<"\n";
        f.close();
    */

    static int32_t limit, limit2, nlimit, nlimit2;

    //----------------------------------------------------------------------------------------------------------------------------------------------------

    limit = 16383;
    limit2 = (limit << 16) + 32678;
    nlimit = -limit;
    nlimit2 = -limit2;

    // convert texture coords to fixed-point & adjust so 0,0 effectively becomes 0.5,0.5 (ie. 32768,32768)
    v = ((int32_t)(sx1 * 65536.0)) + 32768;
    if (v < 16 || v >= swidth2 - 16)
        tile = 1;
    if (v < nlimit2 || v > limit2) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    p[1].tx = v;
    v = ((int32_t)(sx2 * 65536.0)) + 32768;
    if (v < 16 || v >= swidth2 - 16)
        tile = 1;
    if (v < nlimit2 || v > limit2) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    p[2].tx = v;
    v = ((int32_t)(sx3 * 65536.0)) + 32768;
    if (v < 16 || v >= swidth2 - 16)
        tile = 1;
    if (v < nlimit2 || v > limit2) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    p[3].tx = v;
    v = ((int32_t)(sy1 * 65536.0)) + 32768;
    if (v < 16 || v >= sheight2 - 16)
        tile = 1;
    if (v < nlimit2 || v > limit2) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    p[1].ty = v;
    v = ((int32_t)(sy2 * 65536.0)) + 32768;
    if (v < 16 || v >= sheight2 - 16)
        tile = 1;
    if (v < nlimit2 || v > limit2) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    p[2].ty = v;
    v = ((int32_t)(sy3 * 65536.0)) + 32768;
    if (v < 0 || v >= sheight2 - 16)
        tile = 1;
    if (v < nlimit2 || v > limit2) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    p[3].ty = v;

    if (tile) {
        // shifting to positive range is required for tiling | mod on negative coords will fail
        // shifting may also alleviate the need for tiling if(shifted coords fall within textures normal range
        // does texture extend beyond surface dimensions?
        lhs = 2147483647;
        rhs = -2147483648;
        top = 2147483647;
        bottom = -2147483648;
        for (i = 1; i <= 3; i++) {
            tp = &p[i];
            y = tp->ty;
            if (y > bottom)
                bottom = y;
            if (y < top)
                top = y;
            x = tp->tx;
            if (x > rhs)
                rhs = x;
            if (x < lhs)
                lhs = x;
        }
        z = 0;
        if (lhs < 0) {
            // shift texture coords right
            v = ((lhs + 1) / -swidth2 + 1) * swidth2; // offset to move by
            for (i = 1; i <= 3; i++) {
                tp = &p[i];
                tp->tx = tp->tx + v;
                z = 1;
            }
        } else {
            if (lhs >= swidth2) {
                // shift texture coords left
                z = 1;
                v = (lhs / swidth2) * swidth2; // offset to move by
                for (i = 1; i <= 3; i++) {
                    tp = &p[i];
                    tp->tx = tp->tx - v;
                    z = 1;
                }
            }
        }
        if (top < 0) {
            // shift texture coords down
            v = ((top + 1) / -sheight2 + 1) * sheight2; // offset to move by
            for (i = 1; i <= 3; i++) {
                tp = &p[i];
                tp->ty = tp->ty + v;
                z = 1;
            }
        } else {
            if (top >= swidth2) {
                // shift texture coords up
                v = (top / sheight2) * sheight2; // offset to move by
                for (i = 1; i <= 3; i++) {
                    tp = &p[i];
                    tp->ty = tp->ty - v;
                    z = 1;
                }
                z = 1;
            }
        }
        if (z) {
            // reassess need for tiling
            z = 0;
            for (i = 1; i <= 3; i++) {
                tp = &p[i];
                v = tp->tx;
                if (v < 16 || v >= swidth2 - 16) {
                    z = 1;
                    break;
                }
                v = tp->ty;
                if (v < 16 || v >= sheight2 - 16) {
                    z = 1;
                    break;
                }
            }
            if (z == 0)
                tile = 0; // remove tiling flag, this will greatly improve blit speed
        }
    }

    // validate dest points
    if (dx1 < nlimit || dx1 > limit) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    if (dx2 < nlimit || dx2 > limit) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    if (dx3 < nlimit || dx3 > limit) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    if (dy1 < nlimit || dy1 > limit) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    if (dy2 < nlimit || dy2 > limit) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    if (dy3 < nlimit || dy3 > limit) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }

    // setup dest points
    p[1].x = (dx1 << 16) + 32768;
    p[2].x = (dx2 << 16) + 32768;
    p[3].x = (dx3 << 16) + 32768;
    p[1].y = dy1;
    p[2].y = dy2;
    p[3].y = dy3;

    // get dest extents
    lhs = 2147483647;
    rhs = -2147483648;
    top = 2147483647;
    bottom = -2147483648;
    for (i = 1; i <= 3; i++) {
        tp = &p[i];
        y = tp->y;
        if (y > bottom)
            bottom = y;
        if (y < top)
            top = y;
        if (tp->x < 0)
            x = (tp->x - 65535) / 65536;
        else
            x = tp->x / 65536;
        if (x > rhs)
            rhs = x;
        if (x < lhs)
            lhs = x;
    }

    if (bottom < 0 || top >= dheight || rhs < 0 || lhs >= dwidth)
        return; // clip entire triangle

    for (i = 1; i <= 3; i++) {
        tg = &g[i];
        p1 = &p[i];
        if (i == 3)
            p2 = &p[1];
        else
            p2 = &p[i + 1];

        if (p1->y > p2->y) {
            tempp = p1;
            p1 = p2;
            p2 = tempp;
        }
        tg->tx = p1->tx;
        tg->ty = p1->ty; // starting co-ordinates
        tg->x = p1->x;
        tg->y1 = p1->y;
        tg->y2 = p2->y; // top & bottom
        h = tg->y2 - tg->y1;
        if (h == 0) {
            flats = flats + 1; // number of flat(horizontal) gradients
            flatg = i;         // last detected flat gradient
        } else {
            tg->xi = (p2->x - p1->x) / h;
            tg->txi = (p2->tx - p1->tx) / h;
            tg->tyi = (p2->ty - p1->ty) / h;
        }
        tg->p2 = p2;
        tg->p1 = p1;
    }

    g1 = &g[1];
    g2 = &g[2];
    g3 = &g[3];
    if (flats == 0) {
        // select 2 top-most gradients
        if (g3->y1 < g1->y1) {
            tempg = g1;
            g1 = g3;
            g3 = tempg;
        }
        if (g3->y1 < g2->y1) {
            tempg = g2;
            g2 = g3;
            g3 = tempg;
        }
    } else {
        if (flats == 1) {
            // select the only 2 non-flat gradients available
            if (flatg == 1) {
                tempg = g1;
                g1 = g3;
                g3 = tempg;
            }
            if (flatg == 2) {
                tempg = g2;
                g2 = g3;
                g3 = tempg;
            }
            final = 1; // don't check for continuation
        } else {
            // 3 flats
            // create a row from the leftmost to rightmost point
            // find leftmost-rightmost points
            lhs = 2147483647;
            rhs = -2147483648;
            for (ti = 1; ti <= 3; ti++) {
                x = p[ti].x;
                if (x <= lhs) {
                    lhs = x;
                    lhsi = ti;
                }
                if (x >= rhs) {
                    rhs = x;
                    rhsi = ti;
                }
            }
            // build (dummy) gradients
            g[1].x = lhs;
            g[2].x = rhs;
            g[1].tx = p[lhsi].tx;
            g[1].ty = p[lhsi].ty;
            g[2].tx = p[rhsi].tx;
            g[2].ty = p[rhsi].ty;
            final = 1; // don't check for continuation
        }
    }

    y1 = g1->y1;
    if (g1->y2 > g2->y2)
        y2 = g2->y2;
    else
        y2 = g1->y2;

    // compare the mid-point x-coords of both runs to determine which is on the left & right
    y = y2 - y1;
    lhs1 = g1->x + (g1->xi * y) / 2;
    lhs2 = g2->x + (g2->xi * y) / 2;
    if (lhs1 > lhs2) {
        tempg = g1;
        g1 = g2;
        g2 = tempg;
    }

    //----------------------------------------------------------------------------------------------------------------------------------------------------

    if (src->bytes_per_pixel == 4) {
        dst_offset32 = dst->offset32;
        src_offset32 = src->offset32;
        if (src->alpha_disabled || dst->alpha_disabled) {
            if (tile) {

            mtri1t_usegrad3:;

                if (final == 1) {
                    if (no_edge_overlap)
                        y2 = y2 - 1;
                }

                // not on screen?
                if (y1 >= dheight) {
                    return;
                }
                if (y2 < 0) {
                    if (final)
                        return;
                    // jump to y2's position
                    // note; original point locations are referenced because they are unmodified
                    // & represent the true distance of the run
                    y = y2 - y1;
                    p1 = g1->p1;
                    p2 = g1->p2;
                    d = g1->y2 - g1->y1;
                    if (d) {
                        i64 = p2->tx - p1->tx;
                        g1->tx += i64 * y / d;
                        i64 = p2->ty - p1->ty;
                        g1->ty += i64 * y / d;
                        i64 = p2->x - p1->x;
                        g1->x += i64 * y / d;
                        p1 = g2->p1;
                        p2 = g2->p2;
                    }
                    d = g2->y2 - g2->y1;
                    if (d) {
                        i64 = p2->tx - p1->tx;
                        g2->tx += i64 * y / d;
                        i64 = p2->ty - p1->ty;
                        g2->ty += i64 * y / d;
                        i64 = p2->x - p1->x;
                        g2->x += i64 * y / d;
                    }
                    goto mtri1t_final;
                }

                // clip top
                if (y1 < 0) {
                    // note; original point locations are referenced because they are unmodified
                    // & represent the true distance of the run
                    y = -y1;
                    p1 = g1->p1;
                    p2 = g1->p2;
                    d = g1->y2 - g1->y1;
                    if (d) {
                        i64 = p2->tx - p1->tx;
                        g1->tx += i64 * y / d;
                        i64 = p2->ty - p1->ty;
                        g1->ty += i64 * y / d;
                        i64 = p2->x - p1->x;
                        g1->x += i64 * y / d;
                        p1 = g2->p1;
                        p2 = g2->p2;
                    }
                    d = g2->y2 - g2->y1;
                    if (d) {
                        i64 = p2->tx - p1->tx;
                        g2->tx += i64 * y / d;
                        i64 = p2->ty - p1->ty;
                        g2->ty += i64 * y / d;
                        i64 = p2->x - p1->x;
                        g2->x += i64 * y / d;
                    }
                    y1 = 0;
                }

                if (y2 >= dheight) { // clip bottom
                    y2 = dheight - 1;
                }

                // move indexed variable values into direct variables for faster referencing
                // within 2nd bottleneck
                g1x = g1->x;
                g2x = g2->x;
                g1tx = g1->tx;
                g2tx = g2->tx;
                g1ty = g1->ty;
                g2ty = g2->ty;
                g1xi = g1->xi;
                g2xi = g2->xi;
                g1txi = g1->txi;
                g2txi = g2->txi;
                g1tyi = g1->tyi;
                g2tyi = g2->tyi;

                // 2nd bottleneck
                for (y = y1; y <= y2; y++) {

                    if (g1x < 0)
                        x1 = (g1x - 65535) / 65536;
                    else
                        x1 = g1x / 65536; // int-style rounding of fixed-point value
                    if (g2x < 0)
                        x2 = (g2x - 65535) / 65536;
                    else
                        x2 = g2x / 65536;

                    if (x1 >= dwidth || x2 < 0)
                        goto mtri1t_donerow; // crop if(entirely offscreen

                    tx = g1tx;
                    ty = g1ty;

                    // calculate gradients if they might be required
                    if (x1 != x2) {
                        d = g2x - g1x;
                        i64 = g2tx - g1tx;
                        txi = (i64 << 16) / d;
                        i64 = g2ty - g1ty;
                        tyi = (i64 << 16) / d;
                    } else {
                        txi = 0;
                        tyi = 0;
                    }

                    // calculate pixel offsets from ideals
                    loff = ((g1x & 65535) - 32768); // note; works for positive & negative
                                                    // values
                    roff = ((g2x & 65535) - 32768);

                    if (roff < 0) {                                // not enough of rhs pixel exists to use
                        if (x2 < dwidth && no_edge_overlap == 0) { // onscreen check
                            // draw rhs pixel as is
                            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                            *(dst_offset32 + (y * dwidth + x2)) = src_offset32[((g2ty >> 16) % sheight) * swidth + ((g2tx >> 16) % swidth)];
                            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        }
                        // move left one position
                        x2--;
                        if (x1 > x2 || x2 < 0)
                            goto mtri1t_donerow; // no more to do
                    } else {
                        if (no_edge_overlap) {
                            x2 = x2 - 1;
                            if (x1 > x2 || x2 < 0)
                                goto mtri1t_donerow; // no more to do
                        }
                    }

                    if (loff > 0) {
                        // draw lhs pixel as is
                        if (x1 >= 0) {
                            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                            *(dst_offset32 + (y * dwidth + x1)) = src_offset32[((ty >> 16) % sheight) * swidth + ((tx >> 16) % swidth)];
                            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        }
                        // skip to next x location, effectively reducing steps by 1
                        x1++;
                        if (x1 > x2)
                            goto mtri1t_donerow;
                        loff = -(65536 - loff); // adjust alignment to jump to next ideal offset
                    }

                    // align to loff
                    i64 = -loff;
                    tx += (i64 * txi) / 65536;
                    ty += (i64 * tyi) / 65536;

                    if (x1 < 0) { // clip left
                        d = g2x - g1x;
                        i64 = g2tx - g1tx;
                        tx += ((i64 << 16) * -x1) / d;
                        i64 = g2ty - g1ty;
                        ty += ((i64 << 16) * -x1) / d;
                        if (x1 < 0)
                            x1 = 0;
                    }

                    if (x2 >= dwidth) {
                        x2 = dwidth - 1; // clip right
                    }

                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    pixel_offset32 = dst_offset32 + (y * dwidth + x1);
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                    // bottleneck
                    for (x = x1; x <= x2; x++) {

                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        *(pixel_offset32++) = src_offset32[((ty >> 16) % sheight) * swidth + ((tx >> 16) % swidth)];
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                        tx += txi;
                        ty += tyi;
                    }

                mtri1t_donerow:;

                    if (y != y2) {
                        g1x += g1xi;
                        g1tx += g1txi;
                        g1ty += g1tyi;
                        g2x += g2xi;
                        g2tx += g2txi;
                        g2ty += g2tyi;
                    }
                }

                if (final == 0) {

                    // update indexed variable values with direct variable values which have
                    // changed & may be required
                    g1->x = g1x;
                    g2->x = g2x;
                    g1->tx = g1tx;
                    g2->tx = g2tx;
                    g1->ty = g1ty;
                    g2->ty = g2ty;

                mtri1t_final:;
                    if (y2 < dheight - 1) { // no point continuing if(offscreen!
                        if (g1->y2 < g2->y2)
                            g1 = g3;
                        else
                            g2 = g3;

                        // avoid doing the same row twice
                        y1 = g3->y1 + 1;
                        y2 = g3->y2;
                        g1->x += g1->xi;
                        g1->tx += g1->txi;
                        g1->ty += g1->tyi;
                        g2->x += g2->xi;
                        g2->tx += g2->txi;
                        g2->ty += g2->tyi;

                        final = 1;
                        goto mtri1t_usegrad3;
                    }
                }

                return;
            }

        mtri1_usegrad3:;

            if (final == 1) {
                if (no_edge_overlap)
                    y2 = y2 - 1;
            }

            // not on screen?
            if (y1 >= dheight) {
                return;
            }
            if (y2 < 0) {
                if (final)
                    return;
                // jump to y2's position
                // note; original point locations are referenced because they are unmodified
                // & represent the true distance of the run
                y = y2 - y1;
                p1 = g1->p1;
                p2 = g1->p2;
                d = g1->y2 - g1->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g1->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g1->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g1->x += i64 * y / d;
                    p1 = g2->p1;
                    p2 = g2->p2;
                }
                d = g2->y2 - g2->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g2->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g2->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g2->x += i64 * y / d;
                }
                goto mtri1_final;
            }

            // clip top
            if (y1 < 0) {
                // note; original point locations are referenced because they are unmodified
                // & represent the true distance of the run
                y = -y1;
                p1 = g1->p1;
                p2 = g1->p2;
                d = g1->y2 - g1->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g1->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g1->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g1->x += i64 * y / d;
                    p1 = g2->p1;
                    p2 = g2->p2;
                }
                d = g2->y2 - g2->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g2->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g2->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g2->x += i64 * y / d;
                }
                y1 = 0;
            }

            if (y2 >= dheight) { // clip bottom
                y2 = dheight - 1;
            }

            // move indexed variable values into direct variables for faster referencing
            // within 2nd bottleneck
            g1x = g1->x;
            g2x = g2->x;
            g1tx = g1->tx;
            g2tx = g2->tx;
            g1ty = g1->ty;
            g2ty = g2->ty;
            g1xi = g1->xi;
            g2xi = g2->xi;
            g1txi = g1->txi;
            g2txi = g2->txi;
            g1tyi = g1->tyi;
            g2tyi = g2->tyi;

            // 2nd bottleneck
            for (y = y1; y <= y2; y++) {

                if (g1x < 0)
                    x1 = (g1x - 65535) / 65536;
                else
                    x1 = g1x / 65536; // int-style rounding of fixed-point value
                if (g2x < 0)
                    x2 = (g2x - 65535) / 65536;
                else
                    x2 = g2x / 65536;

                if (x1 >= dwidth || x2 < 0)
                    goto mtri1_donerow; // crop if(entirely offscreen

                tx = g1tx;
                ty = g1ty;

                // calculate gradients if they might be required
                if (x1 != x2) {
                    d = g2x - g1x;
                    i64 = g2tx - g1tx;
                    txi = (i64 << 16) / d;
                    i64 = g2ty - g1ty;
                    tyi = (i64 << 16) / d;
                } else {
                    txi = 0;
                    tyi = 0;
                }

                // calculate pixel offsets from ideals
                loff = ((g1x & 65535) - 32768); // note; works for positive & negative
                                                // values
                roff = ((g2x & 65535) - 32768);

                if (roff < 0) {                                // not enough of rhs pixel exists to use
                    if (x2 < dwidth && no_edge_overlap == 0) { // onscreen check
                        // draw rhs pixel as is
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        *(dst_offset32 + (y * dwidth + x2)) = src_offset32[(g2ty >> 16) * swidth + (g2tx >> 16)];
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    }
                    // move left one position
                    x2--;
                    if (x1 > x2 || x2 < 0)
                        goto mtri1_donerow; // no more to do
                } else {
                    if (no_edge_overlap) {
                        x2 = x2 - 1;
                        if (x1 > x2 || x2 < 0)
                            goto mtri1_donerow; // no more to do
                    }
                }

                if (loff > 0) {
                    // draw lhs pixel as is
                    if (x1 >= 0) {
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        *(dst_offset32 + (y * dwidth + x1)) = src_offset32[(ty >> 16) * swidth + (tx >> 16)];
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    }
                    // skip to next x location, effectively reducing steps by 1
                    x1++;
                    if (x1 > x2)
                        goto mtri1_donerow;
                    loff = -(65536 - loff); // adjust alignment to jump to next ideal offset
                }

                // align to loff
                i64 = -loff;
                tx += (i64 * txi) / 65536;
                ty += (i64 * tyi) / 65536;

                if (x1 < 0) { // clip left
                    d = g2x - g1x;
                    i64 = g2tx - g1tx;
                    tx += ((i64 << 16) * -x1) / d;
                    i64 = g2ty - g1ty;
                    ty += ((i64 << 16) * -x1) / d;
                    if (x1 < 0)
                        x1 = 0;
                }

                if (x2 >= dwidth) {
                    x2 = dwidth - 1; // clip right
                }

                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                pixel_offset32 = dst_offset32 + (y * dwidth + x1);
                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                // bottleneck
                for (x = x1; x <= x2; x++) {

                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    *(pixel_offset32++) = src_offset32[(ty >> 16) * swidth + (tx >> 16)];
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                    tx += txi;
                    ty += tyi;
                }

            mtri1_donerow:;

                if (y != y2) {
                    g1x += g1xi;
                    g1tx += g1txi;
                    g1ty += g1tyi;
                    g2x += g2xi;
                    g2tx += g2txi;
                    g2ty += g2tyi;
                }
            }

            if (final == 0) {

                // update indexed variable values with direct variable values which have
                // changed & may be required
                g1->x = g1x;
                g2->x = g2x;
                g1->tx = g1tx;
                g2->tx = g2tx;
                g1->ty = g1ty;
                g2->ty = g2ty;

            mtri1_final:;
                if (y2 < dheight - 1) { // no point continuing if(offscreen!
                    if (g1->y2 < g2->y2)
                        g1 = g3;
                    else
                        g2 = g3;

                    // avoid doing the same row twice
                    y1 = g3->y1 + 1;
                    y2 = g3->y2;
                    g1->x += g1->xi;
                    g1->tx += g1->txi;
                    g1->ty += g1->tyi;
                    g2->x += g2->xi;
                    g2->tx += g2->txi;
                    g2->ty += g2->tyi;

                    final = 1;
                    goto mtri1_usegrad3;
                }
            }

            return;
        } else {
            if (tile) {

            mtri2t_usegrad3:;

                if (final == 1) {
                    if (no_edge_overlap)
                        y2 = y2 - 1;
                }

                // not on screen?
                if (y1 >= dheight) {
                    return;
                }
                if (y2 < 0) {
                    if (final)
                        return;
                    // jump to y2's position
                    // note; original point locations are referenced because they are unmodified
                    // & represent the true distance of the run
                    y = y2 - y1;
                    p1 = g1->p1;
                    p2 = g1->p2;
                    d = g1->y2 - g1->y1;
                    if (d) {
                        i64 = p2->tx - p1->tx;
                        g1->tx += i64 * y / d;
                        i64 = p2->ty - p1->ty;
                        g1->ty += i64 * y / d;
                        i64 = p2->x - p1->x;
                        g1->x += i64 * y / d;
                        p1 = g2->p1;
                        p2 = g2->p2;
                    }
                    d = g2->y2 - g2->y1;
                    if (d) {
                        i64 = p2->tx - p1->tx;
                        g2->tx += i64 * y / d;
                        i64 = p2->ty - p1->ty;
                        g2->ty += i64 * y / d;
                        i64 = p2->x - p1->x;
                        g2->x += i64 * y / d;
                    }
                    goto mtri2t_final;
                }

                // clip top
                if (y1 < 0) {
                    // note; original point locations are referenced because they are unmodified
                    // & represent the true distance of the run
                    y = -y1;
                    p1 = g1->p1;
                    p2 = g1->p2;
                    d = g1->y2 - g1->y1;
                    if (d) {
                        i64 = p2->tx - p1->tx;
                        g1->tx += i64 * y / d;
                        i64 = p2->ty - p1->ty;
                        g1->ty += i64 * y / d;
                        i64 = p2->x - p1->x;
                        g1->x += i64 * y / d;
                        p1 = g2->p1;
                        p2 = g2->p2;
                    }
                    d = g2->y2 - g2->y1;
                    if (d) {
                        i64 = p2->tx - p1->tx;
                        g2->tx += i64 * y / d;
                        i64 = p2->ty - p1->ty;
                        g2->ty += i64 * y / d;
                        i64 = p2->x - p1->x;
                        g2->x += i64 * y / d;
                    }
                    y1 = 0;
                }

                if (y2 >= dheight) { // clip bottom
                    y2 = dheight - 1;
                }

                // move indexed variable values into direct variables for faster referencing
                // within 2nd bottleneck
                g1x = g1->x;
                g2x = g2->x;
                g1tx = g1->tx;
                g2tx = g2->tx;
                g1ty = g1->ty;
                g2ty = g2->ty;
                g1xi = g1->xi;
                g2xi = g2->xi;
                g1txi = g1->txi;
                g2txi = g2->txi;
                g1tyi = g1->tyi;
                g2tyi = g2->tyi;

                // 2nd bottleneck
                for (y = y1; y <= y2; y++) {

                    if (g1x < 0)
                        x1 = (g1x - 65535) / 65536;
                    else
                        x1 = g1x / 65536; // int-style rounding of fixed-point value
                    if (g2x < 0)
                        x2 = (g2x - 65535) / 65536;
                    else
                        x2 = g2x / 65536;

                    if (x1 >= dwidth || x2 < 0)
                        goto mtri2t_donerow; // crop if(entirely offscreen

                    tx = g1tx;
                    ty = g1ty;

                    // calculate gradients if they might be required
                    if (x1 != x2) {
                        d = g2x - g1x;
                        i64 = g2tx - g1tx;
                        txi = (i64 << 16) / d;
                        i64 = g2ty - g1ty;
                        tyi = (i64 << 16) / d;
                    } else {
                        txi = 0;
                        tyi = 0;
                    }

                    // calculate pixel offsets from ideals
                    loff = ((g1x & 65535) - 32768); // note; works for positive & negative
                                                    // values
                    roff = ((g2x & 65535) - 32768);

                    if (roff < 0) {                                // not enough of rhs pixel exists to use
                        if (x2 < dwidth && no_edge_overlap == 0) { // onscreen check
                            // draw rhs pixel as is
                            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                            pixel_offset32 = dst_offset32 + (y * dwidth + x2);
                            //--------plot pixel--------
                            switch ((col = src_offset32[((g2ty >> 16) % sheight) * swidth + ((g2tx >> 16) % swidth)]) & 0xFF000000) {
                            case 0xFF000000:
                                *pixel_offset32 = col;
                                break;
                            case 0x0:
                                break;
                            case 0x80000000:
                                *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend128[*pixel_offset32 >> 24] << 24);
                                break;
                            case 0x7F000000:
                                *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend127[*pixel_offset32 >> 24] << 24);
                                break;
                            default:
                                destcol = *pixel_offset32;
                                cp = cblend + (col >> 24 << 16);
                                *pixel_offset32 = cp[(col << 8 & 0xFF00) + (destcol & 255)] + (cp[(col & 0xFF00) + (destcol >> 8 & 255)] << 8) +
                                                  (cp[(col >> 8 & 0xFF00) + (destcol >> 16 & 255)] << 16) +
                                                  (ablend[(col >> 24) + (destcol >> 16 & 0xFF00)] << 24);
                            }; // switch
                            //--------done plot pixel--------
                            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        }
                        // move left one position
                        x2--;
                        if (x1 > x2 || x2 < 0)
                            goto mtri2t_donerow; // no more to do
                    } else {
                        if (no_edge_overlap) {
                            x2 = x2 - 1;
                            if (x1 > x2 || x2 < 0)
                                goto mtri2t_donerow; // no more to do
                        }
                    }

                    if (loff > 0) {
                        // draw lhs pixel as is
                        if (x1 >= 0) {
                            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                            pixel_offset32 = dst_offset32 + (y * dwidth + x1);
                            //--------plot pixel--------
                            switch ((col = src_offset32[((ty >> 16) % sheight) * swidth + ((tx >> 16) % swidth)]) & 0xFF000000) {
                            case 0xFF000000:
                                *pixel_offset32 = col;
                                break;
                            case 0x0:
                                break;
                            case 0x80000000:
                                *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend128[*pixel_offset32 >> 24] << 24);
                                break;
                            case 0x7F000000:
                                *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend127[*pixel_offset32 >> 24] << 24);
                                break;
                            default:
                                destcol = *pixel_offset32;
                                cp = cblend + (col >> 24 << 16);
                                *pixel_offset32 = cp[(col << 8 & 0xFF00) + (destcol & 255)] + (cp[(col & 0xFF00) + (destcol >> 8 & 255)] << 8) +
                                                  (cp[(col >> 8 & 0xFF00) + (destcol >> 16 & 255)] << 16) +
                                                  (ablend[(col >> 24) + (destcol >> 16 & 0xFF00)] << 24);
                            }; // switch
                            //--------done plot pixel--------
                            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        }
                        // skip to next x location, effectively reducing steps by 1
                        x1++;
                        if (x1 > x2)
                            goto mtri2t_donerow;
                        loff = -(65536 - loff); // adjust alignment to jump to next ideal offset
                    }

                    // align to loff
                    i64 = -loff;
                    tx += (i64 * txi) / 65536;
                    ty += (i64 * tyi) / 65536;

                    if (x1 < 0) { // clip left
                        d = g2x - g1x;
                        i64 = g2tx - g1tx;
                        tx += ((i64 << 16) * -x1) / d;
                        i64 = g2ty - g1ty;
                        ty += ((i64 << 16) * -x1) / d;
                        if (x1 < 0)
                            x1 = 0;
                    }

                    if (x2 >= dwidth) {
                        x2 = dwidth - 1; // clip right
                    }

                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    pixel_offset32 = dst_offset32 + (y * dwidth + x1);
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                    // bottleneck
                    for (x = x1; x <= x2; x++) {

                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        //--------plot pixel--------
                        switch ((col = src_offset32[((ty >> 16) % sheight) * swidth + ((tx >> 16) % swidth)]) & 0xFF000000) {
                        case 0xFF000000:
                            *pixel_offset32 = col;
                            break;
                        case 0x0:
                            break;
                        case 0x80000000:
                            *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend128[*pixel_offset32 >> 24] << 24);
                            break;
                        case 0x7F000000:
                            *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend127[*pixel_offset32 >> 24] << 24);
                            break;
                        default:
                            destcol = *pixel_offset32;
                            cp = cblend + (col >> 24 << 16);
                            *pixel_offset32 = cp[(col << 8 & 0xFF00) + (destcol & 255)] + (cp[(col & 0xFF00) + (destcol >> 8 & 255)] << 8) +
                                              (cp[(col >> 8 & 0xFF00) + (destcol >> 16 & 255)] << 16) + (ablend[(col >> 24) + (destcol >> 16 & 0xFF00)] << 24);
                        }; // switch
                        //--------done plot pixel--------
                        pixel_offset32++;
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                        tx += txi;
                        ty += tyi;
                    }

                mtri2t_donerow:;

                    if (y != y2) {
                        g1x += g1xi;
                        g1tx += g1txi;
                        g1ty += g1tyi;
                        g2x += g2xi;
                        g2tx += g2txi;
                        g2ty += g2tyi;
                    }
                }

                if (final == 0) {

                    // update indexed variable values with direct variable values which have
                    // changed & may be required
                    g1->x = g1x;
                    g2->x = g2x;
                    g1->tx = g1tx;
                    g2->tx = g2tx;
                    g1->ty = g1ty;
                    g2->ty = g2ty;

                mtri2t_final:;
                    if (y2 < dheight - 1) { // no point continuing if(offscreen!
                        if (g1->y2 < g2->y2)
                            g1 = g3;
                        else
                            g2 = g3;

                        // avoid doing the same row twice
                        y1 = g3->y1 + 1;
                        y2 = g3->y2;
                        g1->x += g1->xi;
                        g1->tx += g1->txi;
                        g1->ty += g1->tyi;
                        g2->x += g2->xi;
                        g2->tx += g2->txi;
                        g2->ty += g2->tyi;

                        final = 1;
                        goto mtri2t_usegrad3;
                    }
                }

                return;
            }

        mtri2_usegrad3:;

            if (final == 1) {
                if (no_edge_overlap)
                    y2 = y2 - 1;
            }

            // not on screen?
            if (y1 >= dheight) {
                return;
            }
            if (y2 < 0) {
                if (final)
                    return;
                // jump to y2's position
                // note; original point locations are referenced because they are unmodified
                // & represent the true distance of the run
                y = y2 - y1;
                p1 = g1->p1;
                p2 = g1->p2;
                d = g1->y2 - g1->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g1->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g1->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g1->x += i64 * y / d;
                    p1 = g2->p1;
                    p2 = g2->p2;
                }
                d = g2->y2 - g2->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g2->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g2->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g2->x += i64 * y / d;
                }
                goto mtri2_final;
            }

            // clip top
            if (y1 < 0) {
                // note; original point locations are referenced because they are unmodified
                // & represent the true distance of the run
                y = -y1;
                p1 = g1->p1;
                p2 = g1->p2;
                d = g1->y2 - g1->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g1->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g1->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g1->x += i64 * y / d;
                    p1 = g2->p1;
                    p2 = g2->p2;
                }
                d = g2->y2 - g2->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g2->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g2->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g2->x += i64 * y / d;
                }
                y1 = 0;
            }

            if (y2 >= dheight) { // clip bottom
                y2 = dheight - 1;
            }

            // move indexed variable values into direct variables for faster referencing
            // within 2nd bottleneck
            g1x = g1->x;
            g2x = g2->x;
            g1tx = g1->tx;
            g2tx = g2->tx;
            g1ty = g1->ty;
            g2ty = g2->ty;
            g1xi = g1->xi;
            g2xi = g2->xi;
            g1txi = g1->txi;
            g2txi = g2->txi;
            g1tyi = g1->tyi;
            g2tyi = g2->tyi;

            // 2nd bottleneck
            for (y = y1; y <= y2; y++) {

                if (g1x < 0)
                    x1 = (g1x - 65535) / 65536;
                else
                    x1 = g1x / 65536; // int-style rounding of fixed-point value
                if (g2x < 0)
                    x2 = (g2x - 65535) / 65536;
                else
                    x2 = g2x / 65536;

                if (x1 >= dwidth || x2 < 0)
                    goto mtri2_donerow; // crop if(entirely offscreen

                tx = g1tx;
                ty = g1ty;

                // calculate gradients if they might be required
                if (x1 != x2) {
                    d = g2x - g1x;
                    i64 = g2tx - g1tx;
                    txi = (i64 << 16) / d;
                    i64 = g2ty - g1ty;
                    tyi = (i64 << 16) / d;
                } else {
                    txi = 0;
                    tyi = 0;
                }

                // calculate pixel offsets from ideals
                loff = ((g1x & 65535) - 32768); // note; works for positive & negative
                                                // values
                roff = ((g2x & 65535) - 32768);

                if (roff < 0) {                                // not enough of rhs pixel exists to use
                    if (x2 < dwidth && no_edge_overlap == 0) { // onscreen check
                        // draw rhs pixel as is
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        pixel_offset32 = dst_offset32 + (y * dwidth + x2);
                        //--------plot pixel--------
                        switch ((col = src_offset32[(g2ty >> 16) * swidth + (g2tx >> 16)]) & 0xFF000000) {
                        case 0xFF000000:
                            *pixel_offset32 = col;
                            break;
                        case 0x0:
                            break;
                        case 0x80000000:
                            *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend128[*pixel_offset32 >> 24] << 24);
                            break;
                        case 0x7F000000:
                            *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend127[*pixel_offset32 >> 24] << 24);
                            break;
                        default:
                            destcol = *pixel_offset32;
                            cp = cblend + (col >> 24 << 16);
                            *pixel_offset32 = cp[(col << 8 & 0xFF00) + (destcol & 255)] + (cp[(col & 0xFF00) + (destcol >> 8 & 255)] << 8) +
                                              (cp[(col >> 8 & 0xFF00) + (destcol >> 16 & 255)] << 16) + (ablend[(col >> 24) + (destcol >> 16 & 0xFF00)] << 24);
                        }; // switch
                        //--------done plot pixel--------
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    }
                    // move left one position
                    x2--;
                    if (x1 > x2 || x2 < 0)
                        goto mtri2_donerow; // no more to do
                } else {
                    if (no_edge_overlap) {
                        x2 = x2 - 1;
                        if (x1 > x2 || x2 < 0)
                            goto mtri2_donerow; // no more to do
                    }
                }

                if (loff > 0) {
                    // draw lhs pixel as is
                    if (x1 >= 0) {
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        pixel_offset32 = dst_offset32 + (y * dwidth + x1);
                        //--------plot pixel--------
                        switch ((col = src_offset32[(ty >> 16) * swidth + (tx >> 16)]) & 0xFF000000) {
                        case 0xFF000000:
                            *pixel_offset32 = col;
                            break;
                        case 0x0:
                            break;
                        case 0x80000000:
                            *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend128[*pixel_offset32 >> 24] << 24);
                            break;
                        case 0x7F000000:
                            *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend127[*pixel_offset32 >> 24] << 24);
                            break;
                        default:
                            destcol = *pixel_offset32;
                            cp = cblend + (col >> 24 << 16);
                            *pixel_offset32 = cp[(col << 8 & 0xFF00) + (destcol & 255)] + (cp[(col & 0xFF00) + (destcol >> 8 & 255)] << 8) +
                                              (cp[(col >> 8 & 0xFF00) + (destcol >> 16 & 255)] << 16) + (ablend[(col >> 24) + (destcol >> 16 & 0xFF00)] << 24);
                        }; // switch
                        //--------done plot pixel--------
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    }
                    // skip to next x location, effectively reducing steps by 1
                    x1++;
                    if (x1 > x2)
                        goto mtri2_donerow;
                    loff = -(65536 - loff); // adjust alignment to jump to next ideal offset
                }

                // align to loff
                i64 = -loff;
                tx += (i64 * txi) / 65536;
                ty += (i64 * tyi) / 65536;

                if (x1 < 0) { // clip left
                    d = g2x - g1x;
                    i64 = g2tx - g1tx;
                    tx += ((i64 << 16) * -x1) / d;
                    i64 = g2ty - g1ty;
                    ty += ((i64 << 16) * -x1) / d;
                    if (x1 < 0)
                        x1 = 0;
                }

                if (x2 >= dwidth) {
                    x2 = dwidth - 1; // clip right
                }

                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                pixel_offset32 = dst_offset32 + (y * dwidth + x1);
                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                // bottleneck
                for (x = x1; x <= x2; x++) {

                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    //--------plot pixel--------
                    switch ((col = src_offset32[(ty >> 16) * swidth + (tx >> 16)]) & 0xFF000000) {
                    case 0xFF000000:
                        *pixel_offset32 = col;
                        break;
                    case 0x0:
                        break;
                    case 0x80000000:
                        *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend128[*pixel_offset32 >> 24] << 24);
                        break;
                    case 0x7F000000:
                        *pixel_offset32 = (((*pixel_offset32 & 0xFEFEFE) + (col & 0xFEFEFE)) >> 1) + (ablend127[*pixel_offset32 >> 24] << 24);
                        break;
                    default:
                        destcol = *pixel_offset32;
                        cp = cblend + (col >> 24 << 16);
                        *pixel_offset32 = cp[(col << 8 & 0xFF00) + (destcol & 255)] + (cp[(col & 0xFF00) + (destcol >> 8 & 255)] << 8) +
                                          (cp[(col >> 8 & 0xFF00) + (destcol >> 16 & 255)] << 16) + (ablend[(col >> 24) + (destcol >> 16 & 0xFF00)] << 24);
                    }; // switch
                    //--------done plot pixel--------
                    pixel_offset32++;
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                    tx += txi;
                    ty += tyi;
                }

            mtri2_donerow:;

                if (y != y2) {
                    g1x += g1xi;
                    g1tx += g1txi;
                    g1ty += g1tyi;
                    g2x += g2xi;
                    g2tx += g2txi;
                    g2ty += g2tyi;
                }
            }

            if (final == 0) {

                // update indexed variable values with direct variable values which have
                // changed & may be required
                g1->x = g1x;
                g2->x = g2x;
                g1->tx = g1tx;
                g2->tx = g2tx;
                g1->ty = g1ty;
                g2->ty = g2ty;

            mtri2_final:;
                if (y2 < dheight - 1) { // no point continuing if(offscreen!
                    if (g1->y2 < g2->y2)
                        g1 = g3;
                    else
                        g2 = g3;

                    // avoid doing the same row twice
                    y1 = g3->y1 + 1;
                    y2 = g3->y2;
                    g1->x += g1->xi;
                    g1->tx += g1->txi;
                    g1->ty += g1->tyi;
                    g2->x += g2->xi;
                    g2->tx += g2->txi;
                    g2->ty += g2->tyi;

                    final = 1;
                    goto mtri2_usegrad3;
                }
            }

            return;
        }
    } // 4

    // assume 1 byte per pixel
    dst_offset = dst->offset;
    src_offset = src->offset;
    auto transparent_color = src->transparent_color;
    if (transparent_color == -1) {
        if (tile) {

        mtri3t_usegrad3:;

            if (final == 1) {
                if (no_edge_overlap)
                    y2 = y2 - 1;
            }

            // not on screen?
            if (y1 >= dheight) {
                return;
            }
            if (y2 < 0) {
                if (final)
                    return;
                // jump to y2's position
                // note; original point locations are referenced because they are unmodified
                // & represent the true distance of the run
                y = y2 - y1;
                p1 = g1->p1;
                p2 = g1->p2;
                d = g1->y2 - g1->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g1->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g1->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g1->x += i64 * y / d;
                    p1 = g2->p1;
                    p2 = g2->p2;
                }
                d = g2->y2 - g2->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g2->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g2->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g2->x += i64 * y / d;
                }
                goto mtri3t_final;
            }

            // clip top
            if (y1 < 0) {
                // note; original point locations are referenced because they are unmodified
                // & represent the true distance of the run
                y = -y1;
                p1 = g1->p1;
                p2 = g1->p2;
                d = g1->y2 - g1->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g1->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g1->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g1->x += i64 * y / d;
                    p1 = g2->p1;
                    p2 = g2->p2;
                }
                d = g2->y2 - g2->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g2->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g2->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g2->x += i64 * y / d;
                }
                y1 = 0;
            }

            if (y2 >= dheight) { // clip bottom
                y2 = dheight - 1;
            }

            // move indexed variable values into direct variables for faster referencing
            // within 2nd bottleneck
            g1x = g1->x;
            g2x = g2->x;
            g1tx = g1->tx;
            g2tx = g2->tx;
            g1ty = g1->ty;
            g2ty = g2->ty;
            g1xi = g1->xi;
            g2xi = g2->xi;
            g1txi = g1->txi;
            g2txi = g2->txi;
            g1tyi = g1->tyi;
            g2tyi = g2->tyi;

            // 2nd bottleneck
            for (y = y1; y <= y2; y++) {

                if (g1x < 0)
                    x1 = (g1x - 65535) / 65536;
                else
                    x1 = g1x / 65536; // int-style rounding of fixed-point value
                if (g2x < 0)
                    x2 = (g2x - 65535) / 65536;
                else
                    x2 = g2x / 65536;

                if (x1 >= dwidth || x2 < 0)
                    goto mtri3t_donerow; // crop if(entirely offscreen

                tx = g1tx;
                ty = g1ty;

                // calculate gradients if they might be required
                if (x1 != x2) {
                    d = g2x - g1x;
                    i64 = g2tx - g1tx;
                    txi = (i64 << 16) / d;
                    i64 = g2ty - g1ty;
                    tyi = (i64 << 16) / d;
                } else {
                    txi = 0;
                    tyi = 0;
                }

                // calculate pixel offsets from ideals
                loff = ((g1x & 65535) - 32768); // note; works for positive & negative
                                                // values
                roff = ((g2x & 65535) - 32768);

                if (roff < 0) {                                // not enough of rhs pixel exists to use
                    if (x2 < dwidth && no_edge_overlap == 0) { // onscreen check
                        // draw rhs pixel as is
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        *(dst_offset + (y * dwidth + x2)) = src_offset[((g2ty >> 16) % sheight) * swidth + ((g2tx >> 16) % swidth)];
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    }
                    // move left one position
                    x2--;
                    if (x1 > x2 || x2 < 0)
                        goto mtri3t_donerow; // no more to do
                } else {
                    if (no_edge_overlap) {
                        x2 = x2 - 1;
                        if (x1 > x2 || x2 < 0)
                            goto mtri3t_donerow; // no more to do
                    }
                }

                if (loff > 0) {
                    // draw lhs pixel as is
                    if (x1 >= 0) {
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        *(dst_offset + (y * dwidth + x1)) = src_offset[((ty >> 16) % sheight) * swidth + ((tx >> 16) % swidth)];
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    }
                    // skip to next x location, effectively reducing steps by 1
                    x1++;
                    if (x1 > x2)
                        goto mtri3t_donerow;
                    loff = -(65536 - loff); // adjust alignment to jump to next ideal offset
                }

                // align to loff
                i64 = -loff;
                tx += (i64 * txi) / 65536;
                ty += (i64 * tyi) / 65536;

                if (x1 < 0) { // clip left
                    d = g2x - g1x;
                    i64 = g2tx - g1tx;
                    tx += ((i64 << 16) * -x1) / d;
                    i64 = g2ty - g1ty;
                    ty += ((i64 << 16) * -x1) / d;
                    if (x1 < 0)
                        x1 = 0;
                }

                if (x2 >= dwidth) {
                    x2 = dwidth - 1; // clip right
                }

                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                pixel_offset = dst_offset + (y * dwidth + x1);
                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                // bottleneck
                for (x = x1; x <= x2; x++) {

                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    *(pixel_offset++) = src_offset[((ty >> 16) % sheight) * swidth + ((tx >> 16) % swidth)];
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                    tx += txi;
                    ty += tyi;
                }

            mtri3t_donerow:;

                if (y != y2) {
                    g1x += g1xi;
                    g1tx += g1txi;
                    g1ty += g1tyi;
                    g2x += g2xi;
                    g2tx += g2txi;
                    g2ty += g2tyi;
                }
            }

            if (final == 0) {

                // update indexed variable values with direct variable values which have
                // changed & may be required
                g1->x = g1x;
                g2->x = g2x;
                g1->tx = g1tx;
                g2->tx = g2tx;
                g1->ty = g1ty;
                g2->ty = g2ty;

            mtri3t_final:;
                if (y2 < dheight - 1) { // no point continuing if(offscreen!
                    if (g1->y2 < g2->y2)
                        g1 = g3;
                    else
                        g2 = g3;

                    // avoid doing the same row twice
                    y1 = g3->y1 + 1;
                    y2 = g3->y2;
                    g1->x += g1->xi;
                    g1->tx += g1->txi;
                    g1->ty += g1->tyi;
                    g2->x += g2->xi;
                    g2->tx += g2->txi;
                    g2->ty += g2->tyi;

                    final = 1;
                    goto mtri3t_usegrad3;
                }
            }

            return;
        }

    mtri3_usegrad3:;

        if (final == 1) {
            if (no_edge_overlap)
                y2 = y2 - 1;
        }

        // not on screen?
        if (y1 >= dheight) {
            return;
        }
        if (y2 < 0) {
            if (final)
                return;
            // jump to y2's position
            // note; original point locations are referenced because they are unmodified
            // & represent the true distance of the run
            y = y2 - y1;
            p1 = g1->p1;
            p2 = g1->p2;
            d = g1->y2 - g1->y1;
            if (d) {
                i64 = p2->tx - p1->tx;
                g1->tx += i64 * y / d;
                i64 = p2->ty - p1->ty;
                g1->ty += i64 * y / d;
                i64 = p2->x - p1->x;
                g1->x += i64 * y / d;
                p1 = g2->p1;
                p2 = g2->p2;
            }
            d = g2->y2 - g2->y1;
            if (d) {
                i64 = p2->tx - p1->tx;
                g2->tx += i64 * y / d;
                i64 = p2->ty - p1->ty;
                g2->ty += i64 * y / d;
                i64 = p2->x - p1->x;
                g2->x += i64 * y / d;
            }
            goto mtri3_final;
        }

        // clip top
        if (y1 < 0) {
            // note; original point locations are referenced because they are unmodified
            // & represent the true distance of the run
            y = -y1;
            p1 = g1->p1;
            p2 = g1->p2;
            d = g1->y2 - g1->y1;
            if (d) {
                i64 = p2->tx - p1->tx;
                g1->tx += i64 * y / d;
                i64 = p2->ty - p1->ty;
                g1->ty += i64 * y / d;
                i64 = p2->x - p1->x;
                g1->x += i64 * y / d;
                p1 = g2->p1;
                p2 = g2->p2;
            }
            d = g2->y2 - g2->y1;
            if (d) {
                i64 = p2->tx - p1->tx;
                g2->tx += i64 * y / d;
                i64 = p2->ty - p1->ty;
                g2->ty += i64 * y / d;
                i64 = p2->x - p1->x;
                g2->x += i64 * y / d;
            }
            y1 = 0;
        }

        if (y2 >= dheight) { // clip bottom
            y2 = dheight - 1;
        }

        // move indexed variable values into direct variables for faster referencing
        // within 2nd bottleneck
        g1x = g1->x;
        g2x = g2->x;
        g1tx = g1->tx;
        g2tx = g2->tx;
        g1ty = g1->ty;
        g2ty = g2->ty;
        g1xi = g1->xi;
        g2xi = g2->xi;
        g1txi = g1->txi;
        g2txi = g2->txi;
        g1tyi = g1->tyi;
        g2tyi = g2->tyi;

        // 2nd bottleneck
        for (y = y1; y <= y2; y++) {

            if (g1x < 0)
                x1 = (g1x - 65535) / 65536;
            else
                x1 = g1x / 65536; // int-style rounding of fixed-point value
            if (g2x < 0)
                x2 = (g2x - 65535) / 65536;
            else
                x2 = g2x / 65536;

            if (x1 >= dwidth || x2 < 0)
                goto mtri3_donerow; // crop if(entirely offscreen

            tx = g1tx;
            ty = g1ty;

            // calculate gradients if they might be required
            if (x1 != x2) {
                d = g2x - g1x;
                i64 = g2tx - g1tx;
                txi = (i64 << 16) / d;
                i64 = g2ty - g1ty;
                tyi = (i64 << 16) / d;
            } else {
                txi = 0;
                tyi = 0;
            }

            // calculate pixel offsets from ideals
            loff = ((g1x & 65535) - 32768); // note; works for positive & negative
                                            // values
            roff = ((g2x & 65535) - 32768);

            if (roff < 0) {                                // not enough of rhs pixel exists to use
                if (x2 < dwidth && no_edge_overlap == 0) { // onscreen check
                    // draw rhs pixel as is
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    *(dst_offset + (y * dwidth + x2)) = src_offset[(g2ty >> 16) * swidth + (g2tx >> 16)];
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                }
                // move left one position
                x2--;
                if (x1 > x2 || x2 < 0)
                    goto mtri3_donerow; // no more to do
            } else {
                if (no_edge_overlap) {
                    x2 = x2 - 1;
                    if (x1 > x2 || x2 < 0)
                        goto mtri3_donerow; // no more to do
                }
            }

            if (loff > 0) {
                // draw lhs pixel as is
                if (x1 >= 0) {
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    *(dst_offset + (y * dwidth + x1)) = src_offset[(ty >> 16) * swidth + (tx >> 16)];
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                }
                // skip to next x location, effectively reducing steps by 1
                x1++;
                if (x1 > x2)
                    goto mtri3_donerow;
                loff = -(65536 - loff); // adjust alignment to jump to next ideal offset
            }

            // align to loff
            i64 = -loff;
            tx += (i64 * txi) / 65536;
            ty += (i64 * tyi) / 65536;

            if (x1 < 0) { // clip left
                d = g2x - g1x;
                i64 = g2tx - g1tx;
                tx += ((i64 << 16) * -x1) / d;
                i64 = g2ty - g1ty;
                ty += ((i64 << 16) * -x1) / d;
                if (x1 < 0)
                    x1 = 0;
            }

            if (x2 >= dwidth) {
                x2 = dwidth - 1; // clip right
            }

            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
            pixel_offset = dst_offset + (y * dwidth + x1);
            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

            // bottleneck
            for (x = x1; x <= x2; x++) {

                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                *(pixel_offset++) = src_offset[(ty >> 16) * swidth + (tx >> 16)];
                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                tx += txi;
                ty += tyi;
            }

        mtri3_donerow:;

            if (y != y2) {
                g1x += g1xi;
                g1tx += g1txi;
                g1ty += g1tyi;
                g2x += g2xi;
                g2tx += g2txi;
                g2ty += g2tyi;
            }
        }

        if (final == 0) {

            // update indexed variable values with direct variable values which have
            // changed & may be required
            g1->x = g1x;
            g2->x = g2x;
            g1->tx = g1tx;
            g2->tx = g2tx;
            g1->ty = g1ty;
            g2->ty = g2ty;

        mtri3_final:;
            if (y2 < dheight - 1) { // no point continuing if(offscreen!
                if (g1->y2 < g2->y2)
                    g1 = g3;
                else
                    g2 = g3;

                // avoid doing the same row twice
                y1 = g3->y1 + 1;
                y2 = g3->y2;
                g1->x += g1->xi;
                g1->tx += g1->txi;
                g1->ty += g1->tyi;
                g2->x += g2->xi;
                g2->tx += g2->txi;
                g2->ty += g2->tyi;

                final = 1;
                goto mtri3_usegrad3;
            }
        }

        return;
    } else {
        if (tile) {

        mtri4t_usegrad3:;

            if (final == 1) {
                if (no_edge_overlap)
                    y2 = y2 - 1;
            }

            // not on screen?
            if (y1 >= dheight) {
                return;
            }
            if (y2 < 0) {
                if (final)
                    return;
                // jump to y2's position
                // note; original point locations are referenced because they are unmodified
                // & represent the true distance of the run
                y = y2 - y1;
                p1 = g1->p1;
                p2 = g1->p2;
                d = g1->y2 - g1->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g1->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g1->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g1->x += i64 * y / d;
                    p1 = g2->p1;
                    p2 = g2->p2;
                }
                d = g2->y2 - g2->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g2->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g2->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g2->x += i64 * y / d;
                }
                goto mtri4t_final;
            }

            // clip top
            if (y1 < 0) {
                // note; original point locations are referenced because they are unmodified
                // & represent the true distance of the run
                y = -y1;
                p1 = g1->p1;
                p2 = g1->p2;
                d = g1->y2 - g1->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g1->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g1->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g1->x += i64 * y / d;
                    p1 = g2->p1;
                    p2 = g2->p2;
                }
                d = g2->y2 - g2->y1;
                if (d) {
                    i64 = p2->tx - p1->tx;
                    g2->tx += i64 * y / d;
                    i64 = p2->ty - p1->ty;
                    g2->ty += i64 * y / d;
                    i64 = p2->x - p1->x;
                    g2->x += i64 * y / d;
                }
                y1 = 0;
            }

            if (y2 >= dheight) { // clip bottom
                y2 = dheight - 1;
            }

            // move indexed variable values into direct variables for faster referencing
            // within 2nd bottleneck
            g1x = g1->x;
            g2x = g2->x;
            g1tx = g1->tx;
            g2tx = g2->tx;
            g1ty = g1->ty;
            g2ty = g2->ty;
            g1xi = g1->xi;
            g2xi = g2->xi;
            g1txi = g1->txi;
            g2txi = g2->txi;
            g1tyi = g1->tyi;
            g2tyi = g2->tyi;

            // 2nd bottleneck
            for (y = y1; y <= y2; y++) {

                if (g1x < 0)
                    x1 = (g1x - 65535) / 65536;
                else
                    x1 = g1x / 65536; // int-style rounding of fixed-point value
                if (g2x < 0)
                    x2 = (g2x - 65535) / 65536;
                else
                    x2 = g2x / 65536;

                if (x1 >= dwidth || x2 < 0)
                    goto mtri4t_donerow; // crop if(entirely offscreen

                tx = g1tx;
                ty = g1ty;

                // calculate gradients if they might be required
                if (x1 != x2) {
                    d = g2x - g1x;
                    i64 = g2tx - g1tx;
                    txi = (i64 << 16) / d;
                    i64 = g2ty - g1ty;
                    tyi = (i64 << 16) / d;
                } else {
                    txi = 0;
                    tyi = 0;
                }

                // calculate pixel offsets from ideals
                loff = ((g1x & 65535) - 32768); // note; works for positive & negative
                                                // values
                roff = ((g2x & 65535) - 32768);

                if (roff < 0) {                                // not enough of rhs pixel exists to use
                    if (x2 < dwidth && no_edge_overlap == 0) { // onscreen check
                        // draw rhs pixel as is
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        col = src_offset[((g2ty >> 16) % sheight) * swidth + ((g2tx >> 16) % swidth)];
                        if (col != uint32_t(transparent_color))
                            *(dst_offset + (y * dwidth + x2)) = col;
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    }
                    // move left one position
                    x2--;
                    if (x1 > x2 || x2 < 0)
                        goto mtri4t_donerow; // no more to do
                } else {
                    if (no_edge_overlap) {
                        x2 = x2 - 1;
                        if (x1 > x2 || x2 < 0)
                            goto mtri4t_donerow; // no more to do
                    }
                }

                if (loff > 0) {
                    // draw lhs pixel as is
                    if (x1 >= 0) {
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        col = src_offset[((ty >> 16) % sheight) * swidth + ((tx >> 16) % swidth)];
                        if (col != uint32_t(transparent_color))
                            *(dst_offset + (y * dwidth + x1)) = col;
                        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    }
                    // skip to next x location, effectively reducing steps by 1
                    x1++;
                    if (x1 > x2)
                        goto mtri4t_donerow;
                    loff = -(65536 - loff); // adjust alignment to jump to next ideal offset
                }

                // align to loff
                i64 = -loff;
                tx += (i64 * txi) / 65536;
                ty += (i64 * tyi) / 65536;

                if (x1 < 0) { // clip left
                    d = g2x - g1x;
                    i64 = g2tx - g1tx;
                    tx += ((i64 << 16) * -x1) / d;
                    i64 = g2ty - g1ty;
                    ty += ((i64 << 16) * -x1) / d;
                    if (x1 < 0)
                        x1 = 0;
                }

                if (x2 >= dwidth) {
                    x2 = dwidth - 1; // clip right
                }

                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                pixel_offset = dst_offset + (y * dwidth + x1);
                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                // bottleneck
                for (x = x1; x <= x2; x++) {

                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    col = src_offset[((ty >> 16) % sheight) * swidth + ((tx >> 16) % swidth)];
                    if (col != uint32_t(transparent_color))
                        *pixel_offset = col;
                    pixel_offset++;
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                    tx += txi;
                    ty += tyi;
                }

            mtri4t_donerow:;

                if (y != y2) {
                    g1x += g1xi;
                    g1tx += g1txi;
                    g1ty += g1tyi;
                    g2x += g2xi;
                    g2tx += g2txi;
                    g2ty += g2tyi;
                }
            }

            if (final == 0) {

                // update indexed variable values with direct variable values which have
                // changed & may be required
                g1->x = g1x;
                g2->x = g2x;
                g1->tx = g1tx;
                g2->tx = g2tx;
                g1->ty = g1ty;
                g2->ty = g2ty;

            mtri4t_final:;
                if (y2 < dheight - 1) { // no point continuing if(offscreen!
                    if (g1->y2 < g2->y2)
                        g1 = g3;
                    else
                        g2 = g3;

                    // avoid doing the same row twice
                    y1 = g3->y1 + 1;
                    y2 = g3->y2;
                    g1->x += g1->xi;
                    g1->tx += g1->txi;
                    g1->ty += g1->tyi;
                    g2->x += g2->xi;
                    g2->tx += g2->txi;
                    g2->ty += g2->tyi;

                    final = 1;
                    goto mtri4t_usegrad3;
                }
            }

            return;
        }

    mtri4_usegrad3:;

        if (final == 1) {
            if (no_edge_overlap)
                y2 = y2 - 1;
        }

        // not on screen?
        if (y1 >= dheight) {
            return;
        }
        if (y2 < 0) {
            if (final)
                return;
            // jump to y2's position
            // note; original point locations are referenced because they are unmodified
            // & represent the true distance of the run
            y = y2 - y1;
            p1 = g1->p1;
            p2 = g1->p2;
            d = g1->y2 - g1->y1;
            if (d) {
                i64 = p2->tx - p1->tx;
                g1->tx += i64 * y / d;
                i64 = p2->ty - p1->ty;
                g1->ty += i64 * y / d;
                i64 = p2->x - p1->x;
                g1->x += i64 * y / d;
                p1 = g2->p1;
                p2 = g2->p2;
            }
            d = g2->y2 - g2->y1;
            if (d) {
                i64 = p2->tx - p1->tx;
                g2->tx += i64 * y / d;
                i64 = p2->ty - p1->ty;
                g2->ty += i64 * y / d;
                i64 = p2->x - p1->x;
                g2->x += i64 * y / d;
            }
            goto mtri4_final;
        }

        // clip top
        if (y1 < 0) {
            // note; original point locations are referenced because they are unmodified
            // & represent the true distance of the run
            y = -y1;
            p1 = g1->p1;
            p2 = g1->p2;
            d = g1->y2 - g1->y1;
            if (d) {
                i64 = p2->tx - p1->tx;
                g1->tx += i64 * y / d;
                i64 = p2->ty - p1->ty;
                g1->ty += i64 * y / d;
                i64 = p2->x - p1->x;
                g1->x += i64 * y / d;
                p1 = g2->p1;
                p2 = g2->p2;
            }
            d = g2->y2 - g2->y1;
            if (d) {
                i64 = p2->tx - p1->tx;
                g2->tx += i64 * y / d;
                i64 = p2->ty - p1->ty;
                g2->ty += i64 * y / d;
                i64 = p2->x - p1->x;
                g2->x += i64 * y / d;
            }
            y1 = 0;
        }

        if (y2 >= dheight) { // clip bottom
            y2 = dheight - 1;
        }

        // move indexed variable values into direct variables for faster referencing
        // within 2nd bottleneck
        g1x = g1->x;
        g2x = g2->x;
        g1tx = g1->tx;
        g2tx = g2->tx;
        g1ty = g1->ty;
        g2ty = g2->ty;
        g1xi = g1->xi;
        g2xi = g2->xi;
        g1txi = g1->txi;
        g2txi = g2->txi;
        g1tyi = g1->tyi;
        g2tyi = g2->tyi;

        // 2nd bottleneck
        for (y = y1; y <= y2; y++) {

            if (g1x < 0)
                x1 = (g1x - 65535) / 65536;
            else
                x1 = g1x / 65536; // int-style rounding of fixed-point value
            if (g2x < 0)
                x2 = (g2x - 65535) / 65536;
            else
                x2 = g2x / 65536;

            if (x1 >= dwidth || x2 < 0)
                goto mtri4_donerow; // crop if(entirely offscreen

            tx = g1tx;
            ty = g1ty;

            // calculate gradients if they might be required
            if (x1 != x2) {
                d = g2x - g1x;
                i64 = g2tx - g1tx;
                txi = (i64 << 16) / d;
                i64 = g2ty - g1ty;
                tyi = (i64 << 16) / d;
            } else {
                txi = 0;
                tyi = 0;
            }

            // calculate pixel offsets from ideals
            loff = ((g1x & 65535) - 32768); // note; works for positive & negative
                                            // values
            roff = ((g2x & 65535) - 32768);

            if (roff < 0) {                                // not enough of rhs pixel exists to use
                if (x2 < dwidth && no_edge_overlap == 0) { // onscreen check
                    // draw rhs pixel as is
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    col = src_offset[(g2ty >> 16) * swidth + (g2tx >> 16)];
                    if (col != uint32_t(transparent_color))
                        *(dst_offset + (y * dwidth + x2)) = col;
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                }
                // move left one position
                x2--;
                if (x1 > x2 || x2 < 0)
                    goto mtri4_donerow; // no more to do
            } else {
                if (no_edge_overlap) {
                    x2 = x2 - 1;
                    if (x1 > x2 || x2 < 0)
                        goto mtri4_donerow; // no more to do
                }
            }

            if (loff > 0) {
                // draw lhs pixel as is
                if (x1 >= 0) {
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    col = src_offset[(ty >> 16) * swidth + (tx >> 16)];
                    if (col != uint32_t(transparent_color))
                        *(dst_offset + (y * dwidth + x1)) = col;
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                }
                // skip to next x location, effectively reducing steps by 1
                x1++;
                if (x1 > x2)
                    goto mtri4_donerow;
                loff = -(65536 - loff); // adjust alignment to jump to next ideal offset
            }

            // align to loff
            i64 = -loff;
            tx += (i64 * txi) / 65536;
            ty += (i64 * tyi) / 65536;

            if (x1 < 0) { // clip left
                d = g2x - g1x;
                i64 = g2tx - g1tx;
                tx += ((i64 << 16) * -x1) / d;
                i64 = g2ty - g1ty;
                ty += ((i64 << 16) * -x1) / d;
                if (x1 < 0)
                    x1 = 0;
            }

            if (x2 >= dwidth) {
                x2 = dwidth - 1; // clip right
            }

            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
            pixel_offset = dst_offset + (y * dwidth + x1);
            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

            // bottleneck
            for (x = x1; x <= x2; x++) {

                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                col = src_offset[(ty >> 16) * swidth + (tx >> 16)];
                if (col != uint32_t(transparent_color))
                    *pixel_offset = col;
                pixel_offset++;
                //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                tx += txi;
                ty += tyi;
            }

        mtri4_donerow:;

            if (y != y2) {
                g1x += g1xi;
                g1tx += g1txi;
                g1ty += g1tyi;
                g2x += g2xi;
                g2tx += g2txi;
                g2ty += g2tyi;
            }
        }

        if (final == 0) {

            // update indexed variable values with direct variable values which have
            // changed & may be required
            g1->x = g1x;
            g2->x = g2x;
            g1->tx = g1tx;
            g2->tx = g2tx;
            g1->ty = g1ty;
            g2->ty = g2ty;

        mtri4_final:;
            if (y2 < dheight - 1) { // no point continuing if(offscreen!
                if (g1->y2 < g2->y2)
                    g1 = g3;
                else
                    g2 = g3;

                // avoid doing the same row twice
                y1 = g3->y1 + 1;
                y2 = g3->y2;
                g1->x += g1->xi;
                g1->tx += g1->txi;
                g1->ty += g1->tyi;
                g2->x += g2->xi;
                g2->tx += g2->txi;
                g2->ty += g2->tyi;

                final = 1;
                goto mtri4_usegrad3;
            }
        }

        return;
    } // 1

    error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
    return;
} // sub__maptriangle
