//-----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___   ___                       _    _ _
//   / _ \| _ ) / /| | || _ \ __| |_ _|_ __  __ _ __ _ ___  | |  (_) |__ _ _ __ _ _ _ _  _
//  | (_) | _ \/ _ \_  _|  _/ _|   | || '  \/ _` / _` / -_) | |__| | '_ \ '_/ _` | '_| || |
//   \__\_\___/\___/ |_||_| |___| |___|_|_|_\__,_\__, \___| |____|_|_.__/_| \__,_|_|  \_, |
//                                               |___/                                |__/
//
//  Powered by:
//      stb_image & stb_image_write (https://github.com/nothings/stb)
//      jo_gif (https://www.jonolick.com/code)
//      nanosvg (https://github.com/memononen/nanosvg)
//      qoi (https://qoiformat.org)
//      pixelscalers (https://github.com/janert/pixelscalers)
//      mmpx (https://github.com/ITotalJustice/mmpx)
//      sg_curico & sg_pcx (https://github.com/a740g)
//
//-----------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

#include "logging.h"

#define image_log_trace(...) libqb_log_with_scope_trace(logscope::Image, __VA_ARGS__)

#define image_log_info(...) libqb_log_with_scope_info(logscope::Image, __VA_ARGS__)

#define image_log_warn(...) libqb_log_with_scope_warn(logscope::Image, __VA_ARGS__)

#define image_log_error(...) libqb_log_with_scope_error(logscope::Image, __VA_ARGS__)

#define IMAGE_DEBUG_CHECK(_exp_)                                                                                                                               \
    do {                                                                                                                                                       \
        if (!(_exp_))                                                                                                                                          \
            image_log_warn("Condition (%s) failed", #_exp_);                                                                                                   \
    } while (0)

// This is returned to the caller if something goes wrong while loading the image
#define INVALID_IMAGE_HANDLE -1

struct qbs;

int32_t func__loadimage(qbs *qbsFileName, int32_t bpp, qbs *qbsRequirements, int32_t passed);
void sub__saveimage(qbs *qbsFileName, int32_t imageHandle, qbs *qbsRequirements, int32_t passed);
