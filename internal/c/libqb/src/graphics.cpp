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

// Module-level global variables
static int32_t depthbuffer_mode0 = DEPTHBUFFER_MODE__ON;
static int32_t depthbuffer_mode1 = DEPTHBUFFER_MODE__ON;

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
            error(258);
            return;
        }
        dst -= HARDWARE_IMG_HANDLE_OFFSET;
    } else {
        if (dst > 1) {
            error(5);
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
    static int32_t lhs, rhs, lhs1, lhs2, top, bottom, temp, flats, flatg, final, tile, no_edge_overlap;
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
    static uint32_t col, destcol, transparent_color;
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
                    error(258);
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
                error(258);
                return;
            }
            if (!img[si].valid) {
                error(258);
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
                error(258);
                return;
            }
            if (!img[di].valid) {
                error(258);
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
    if (v < 16 | v >= swidth2 - 16)
        tile = 1;
    if (v<nlimit2 | v> limit2) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    p[1].tx = v;
    v = ((int32_t)(sx2 * 65536.0)) + 32768;
    if (v < 16 | v >= swidth2 - 16)
        tile = 1;
    if (v<nlimit2 | v> limit2) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    p[2].tx = v;
    v = ((int32_t)(sx3 * 65536.0)) + 32768;
    if (v < 16 | v >= swidth2 - 16)
        tile = 1;
    if (v<nlimit2 | v> limit2) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    p[3].tx = v;
    v = ((int32_t)(sy1 * 65536.0)) + 32768;
    if (v < 16 | v >= sheight2 - 16)
        tile = 1;
    if (v<nlimit2 | v> limit2) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    p[1].ty = v;
    v = ((int32_t)(sy2 * 65536.0)) + 32768;
    if (v < 16 | v >= sheight2 - 16)
        tile = 1;
    if (v<nlimit2 | v> limit2) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    p[2].ty = v;
    v = ((int32_t)(sy3 * 65536.0)) + 32768;
    if (v < 0 | v >= sheight2 - 16)
        tile = 1;
    if (v<nlimit2 | v> limit2) {
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
                if (v < 16 | v >= swidth2 - 16) {
                    z = 1;
                    break;
                }
                v = tp->ty;
                if (v < 16 | v >= sheight2 - 16) {
                    z = 1;
                    break;
                }
            }
            if (z == 0)
                tile = 0; // remove tiling flag, this will greatly improve blit speed
        }
    }

    // validate dest points
    if (dx1<nlimit | dx1> limit) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    if (dx2<nlimit | dx2> limit) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    if (dx3<nlimit | dx3> limit) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    if (dy1<nlimit | dy1> limit) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    if (dy2<nlimit | dy2> limit) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }
    if (dy3<nlimit | dy3> limit) {
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

    if (bottom < 0 | top >= dheight | rhs < 0 | lhs >= dwidth)
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
#include "../../mtri1t.cpp"
            }
#include "../../mtri1.cpp"
        } else {
            if (tile) {
#include "../../mtri2t.cpp"
            }
#include "../../mtri2.cpp"
        }
    } // 4

    // assume 1 byte per pixel
    dst_offset = dst->offset;
    src_offset = src->offset;
    transparent_color = src->transparent_color;
    if (transparent_color == -1) {
        if (tile) {
#include "../../mtri3t.cpp"
        }
#include "../../mtri3.cpp"
    } else {
        if (tile) {
#include "../../mtri4t.cpp"
        }
#include "../../mtri4.cpp"
    } // 1

    error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
    return;
} // sub__maptriangle
