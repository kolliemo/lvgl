/**
 * @file lv_draw_sw_blend_rgb888.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../lv_draw_sw.h"
#if LV_USE_DRAW_SW

#include "lv_draw_sw_blend.h"
#include "../../../misc/lv_math.h"
#include "../../../core/lv_disp.h"
#include "../../../core/lv_refr.h"
#include LV_COLOR_EXTERN_INCLUDE

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

LV_ATTRIBUTE_FAST_MEM static void rgb565_image_blend(_lv_draw_sw_blend_image_dsc_t * dsc, uint32_t dest_px_size);

LV_ATTRIBUTE_FAST_MEM static void rgb888_image_blend(_lv_draw_sw_blend_image_dsc_t * dsc, const uint8_t dest_px_size,
                                                     uint32_t src_px_size);

LV_ATTRIBUTE_FAST_MEM static void argb8888_image_blend(_lv_draw_sw_blend_image_dsc_t * dsc, uint32_t dest_px_size);

LV_ATTRIBUTE_FAST_MEM static inline void lv_color_24_24_mix(const uint8_t * src, uint8_t * dest, uint8_t mix);

LV_ATTRIBUTE_FAST_MEM static inline void blend_non_normal_pixel(uint8_t * dest, lv_color32_t src, lv_blend_mode_t mode);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

LV_ATTRIBUTE_FAST_MEM void lv_draw_sw_blend_color_to_rgb888(_lv_draw_sw_blend_fill_dsc_t * dsc, uint32_t dest_px_size)
{
    int32_t w = dsc->dest_w;
    int32_t h = dsc->dest_h;
    lv_opa_t opa = dsc->opa;
    const lv_opa_t * mask = dsc->mask_buf;
    lv_coord_t mask_stride = dsc->mask_stride;
    lv_coord_t dest_stride = dsc->dest_stride;

    int32_t x;
    int32_t y;

    /*Simple fill*/
    if(mask == NULL && opa >= LV_OPA_MAX) {
        if(dest_px_size == 3) {
            uint8_t * dest_buf_u8 = dsc->dest_buf;
            uint8_t * dest_buf_ori = dsc->dest_buf;
            dest_stride *= dest_px_size;
            w *= dest_px_size;

            for(x = 0; x < w; x += 3) {
                dest_buf_u8[x + 0] = dsc->color.blue;
                dest_buf_u8[x + 1] = dsc->color.green;
                dest_buf_u8[x + 2] = dsc->color.red;
            }

            dest_buf_u8 += dest_stride;

            for(y = 1; y < h; y++) {
                lv_memcpy(dest_buf_u8, dest_buf_ori, w);
                dest_buf_u8 += dest_stride;
            }
        }
        if(dest_px_size == 4) {
            uint32_t color32 = lv_color_to_u32(dsc->color);
            uint32_t * dest_buf_u32 = dsc->dest_buf;
            for(y = 0; y < h; y++) {
                for(x = 0; x <= w - 16; x += 16) {
                    dest_buf_u32[x + 0] = color32;
                    dest_buf_u32[x + 1] = color32;
                    dest_buf_u32[x + 2] = color32;
                    dest_buf_u32[x + 3] = color32;

                    dest_buf_u32[x + 4] = color32;
                    dest_buf_u32[x + 5] = color32;
                    dest_buf_u32[x + 6] = color32;
                    dest_buf_u32[x + 7] = color32;

                    dest_buf_u32[x + 8] = color32;
                    dest_buf_u32[x + 9] = color32;
                    dest_buf_u32[x + 10] = color32;
                    dest_buf_u32[x + 11] = color32;

                    dest_buf_u32[x + 12] = color32;
                    dest_buf_u32[x + 13] = color32;
                    dest_buf_u32[x + 14] = color32;
                    dest_buf_u32[x + 15] = color32;
                }
                for(; x < w; x ++) {
                    dest_buf_u32[x] = color32;
                }

                dest_buf_u32 += dest_stride;
            }
        }
    }
    /*Opacity only*/
    else if(mask == NULL && opa < LV_OPA_MAX) {
        uint32_t color32 = lv_color_to_u32(dsc->color);
        uint8_t * dest_buf = dsc->dest_buf;
        dest_stride *= dest_px_size;
        w *= dest_px_size;
        for(y = 0; y < h; y++) {
            for(x = 0; x < w; x += dest_px_size) {
                lv_color_24_24_mix((const uint8_t *)&color32, &dest_buf[x], opa);
            }
            dest_buf += dest_stride;
        }
    }
    /*Masked with full opacity*/
    else if(mask && opa >= LV_OPA_MAX) {
        uint32_t color32 = lv_color_to_u32(dsc->color);
        uint8_t * dest_buf = dsc->dest_buf;
        dest_stride *= dest_px_size;
        w *= dest_px_size;

        for(y = 0; y < h; y++) {
            uint32_t mask_x;
            for(x = 0, mask_x = 0; x < w; x += dest_px_size, mask_x++) {
                lv_color_24_24_mix((const uint8_t *)&color32, &dest_buf[x], mask[mask_x]);
            }
            dest_buf += dest_stride;
            mask += mask_stride;
        }
    }
    /*Masked with opacity*/
    else {
        uint32_t color32 = lv_color_to_u32(dsc->color);
        uint8_t * dest_buf = dsc->dest_buf;
        dest_stride *= dest_px_size;
        w *= dest_px_size;

        for(y = 0; y < h; y++) {
            uint32_t mask_x;
            for(x = 0, mask_x = 0; x < w; x += dest_px_size, mask_x++) {
                lv_color_24_24_mix((const uint8_t *) &color32, &dest_buf[x], (opa * mask[mask_x]) >> 8);
            }
            dest_buf += dest_stride;
            mask += mask_stride;
        }
    }
}

LV_ATTRIBUTE_FAST_MEM void lv_draw_sw_blend_image_to_rgb888(_lv_draw_sw_blend_image_dsc_t * dsc, uint32_t dest_px_size)
{

    switch(dsc->src_color_format) {
        case LV_COLOR_FORMAT_RGB565:
            rgb565_image_blend(dsc, dest_px_size);
            break;
        case LV_COLOR_FORMAT_RGB888:
            rgb888_image_blend(dsc, dest_px_size, 3);
            break;
        case LV_COLOR_FORMAT_XRGB8888:
            rgb888_image_blend(dsc, dest_px_size, 4);
            break;
        case LV_COLOR_FORMAT_ARGB8888:
            argb8888_image_blend(dsc, dest_px_size);
            break;
        default:
            LV_LOG_WARN("Not supported source color format");
            break;
    }
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

LV_ATTRIBUTE_FAST_MEM static void rgb565_image_blend(_lv_draw_sw_blend_image_dsc_t * dsc, uint32_t dest_px_size)
{
    int32_t w = dsc->dest_w;
    int32_t h = dsc->dest_h;
    lv_opa_t opa = dsc->opa;
    uint8_t * dest_buf = dsc->dest_buf;
    lv_coord_t dest_stride = dsc->dest_stride * dest_px_size;
    const lv_color16_t * src_buf_c16 = (const lv_color16_t *) dsc->src_buf;
    lv_coord_t src_stride = dsc->src_stride;
    const lv_opa_t * mask_buf = dsc->mask_buf;
    lv_coord_t mask_stride = dsc->mask_stride;

    int32_t src_x;
    int32_t dest_x;
    int32_t y;

    if(dsc->blend_mode == LV_BLEND_MODE_NORMAL) {
        if(mask_buf == NULL && opa >= LV_OPA_MAX) {
            for(y = 0; y < h; y++) {
                for(src_x = 0, dest_x = 0; src_x < w; dest_x += dest_px_size, src_x++) {
                    dest_buf[dest_x + 0] = (src_buf_c16[src_x].red * 2106) >> 8;  /*To make it rounded*/
                    dest_buf[dest_x + 1] = (src_buf_c16[src_x].green * 1037) >> 8;
                    dest_buf[dest_x + 2] = (src_buf_c16[src_x].blue * 2106) >> 8;
                }
                dest_buf += dest_stride;
                src_buf_c16 += src_stride;
            }
        }
        if(mask_buf == NULL && opa < LV_OPA_MAX) {
            uint8_t res[3];
            for(y = 0; y < h; y++) {
                for(src_x = 0, dest_x = 0; src_x < w; dest_x += dest_px_size, src_x++) {
                    res[0] = (src_buf_c16[src_x].red * 2106) >> 8; /*To make it rounded*/
                    res[1] = (src_buf_c16[src_x].green * 1037) >> 8;
                    res[2] = (src_buf_c16[src_x].blue * 2106) >> 8;
                    lv_color_24_24_mix(res, &dest_buf[dest_x], opa);
                }
                dest_buf += dest_stride;
                src_buf_c16 += src_stride;
            }
        }
        else if(mask_buf && opa >= LV_OPA_MAX) {
            uint8_t res[3];
            for(y = 0; y < h; y++) {
                for(src_x = 0, dest_x = 0; src_x < w; dest_x += dest_px_size, src_x++) {
                    res[0] = (src_buf_c16[src_x].red * 2106) >> 8;  /*To make it rounded*/
                    res[1] = (src_buf_c16[src_x].green * 1037) >> 8;
                    res[2] = (src_buf_c16[src_x].blue * 2106) >> 8;
                    lv_color_24_24_mix(res, &dest_buf[dest_x], mask_buf[src_x]);
                }
                dest_buf += dest_stride;
                src_buf_c16 += src_stride;
            }
        }
        else {
            uint8_t res[3];
            for(y = 0; y < h; y++) {
                for(src_x = 0, dest_x = 0; src_x < w; dest_x += dest_px_size, src_x++) {
                    res[0] = (src_buf_c16[src_x].red * 2106) >> 8;  /*To make it rounded*/
                    res[1] = (src_buf_c16[src_x].green * 1037) >> 8;
                    res[2] = (src_buf_c16[src_x].blue * 2106) >> 8;
                    lv_color_24_24_mix(res, &dest_buf[dest_x], (opa * mask_buf[src_x]) >> 8);
                }
                dest_buf += dest_stride;
                src_buf_c16 += src_stride;
            }
        }
    }
    else {
        lv_color32_t src_argb;
        for(y = 0; y < h; y++) {
            for(src_x = 0, dest_x = 0; src_x < w; src_x++, dest_x += dest_px_size) {
                src_argb.red = (src_buf_c16[src_x].red * 2106) >> 8;
                src_argb.green = (src_buf_c16[src_x].green * 1037) >> 8;
                src_argb.blue = (src_buf_c16[src_x].blue * 2106) >> 8;
                if(mask_buf == NULL) src_argb.alpha = opa;
                else src_argb.alpha = (mask_buf[src_x] * opa) >> 8;
                blend_non_normal_pixel(&dest_buf[dest_x], src_argb, dsc->blend_mode);
            }
            if(mask_buf) mask_buf += mask_stride;
            dest_buf += dest_stride;
            src_buf_c16 += src_stride;
        }
    }
}

LV_ATTRIBUTE_FAST_MEM static void rgb888_image_blend(_lv_draw_sw_blend_image_dsc_t * dsc, const uint8_t dest_px_size,
                                                     uint32_t src_px_size)
{

    int32_t w = dsc->dest_w * dest_px_size;
    int32_t h = dsc->dest_h;
    lv_opa_t opa = dsc->opa;
    uint8_t * dest_buf = dsc->dest_buf;
    lv_coord_t dest_stride = dsc->dest_stride * dest_px_size;
    const uint8_t * src_buf = dsc->src_buf;
    lv_coord_t src_stride = dsc->src_stride * src_px_size;
    const lv_opa_t * mask_buf = dsc->mask_buf;
    lv_coord_t mask_stride = dsc->mask_stride;

    int32_t dest_x;
    int32_t src_x;
    int32_t y;

    if(dsc->blend_mode == LV_BLEND_MODE_NORMAL) {
        /*Special case*/
        if(mask_buf == NULL && opa >= LV_OPA_MAX) {
            if(src_px_size == dest_px_size) {
                uint32_t line_in_bytes = w * src_px_size;
                for(y = 0; y < h; y++) {
                    lv_memcpy(dest_buf, src_buf, line_in_bytes);
                    dest_buf += dest_stride;
                    src_buf += src_stride;
                }
            }
            else {
                for(y = 0; y < h; y++) {
                    for(dest_x = 0, src_x = 0; dest_x < w; dest_x += dest_px_size, src_x += src_px_size) {
                        dest_buf[dest_x + 0] = src_buf[src_x + 0];
                        dest_buf[dest_x + 1] = src_buf[src_x + 1];
                        dest_buf[dest_x + 2] = src_buf[src_x + 2];
                    }
                    dest_buf += dest_stride;
                    src_buf += src_stride;
                }
            }
        }
        if(mask_buf == NULL && opa < LV_OPA_MAX) {
            for(y = 0; y < h; y++) {
                for(dest_x = 0, src_x = 0; dest_x < w; dest_x += dest_px_size, src_x += src_px_size) {
                    lv_color_24_24_mix(&src_buf[src_x], &dest_buf[dest_x], opa);
                }
                dest_buf += dest_stride;
                src_buf += src_stride;
            }
        }
        if(mask_buf && opa >= LV_OPA_MAX) {
            uint32_t mask_x;
            for(y = 0; y < h; y++) {
                for(mask_x = 0, dest_x = 0, src_x = 0; dest_x < w; mask_x++, dest_x += dest_px_size, src_x += src_px_size) {
                    lv_color_24_24_mix(&src_buf[src_x], &dest_buf[dest_x], mask_buf[mask_x]);
                }
                dest_buf += dest_stride;
                src_buf += src_stride;
                mask_buf += mask_stride;
            }
        }
        if(mask_buf && opa < LV_OPA_MAX) {
            uint32_t mask_x;
            for(y = 0; y < h; y++) {
                for(mask_x = 0, dest_x = 0, src_x = 0; dest_x < w; mask_x++, dest_x += dest_px_size, src_x += src_px_size) {
                    lv_color_24_24_mix(&src_buf[src_x], &dest_buf[dest_x], (opa * mask_buf[mask_x]) >> 8);
                }
                dest_buf += dest_stride;
                src_buf += src_stride;
                mask_buf += mask_stride;
            }
        }
    }
    else {
        lv_color32_t src_argb;
        for(y = 0; y < h; y++) {
            for(dest_x = 0, src_x = 0; dest_x < w; dest_x += dest_px_size, src_x += src_px_size) {
                src_argb.red = src_buf[src_x + 2];
                src_argb.green = src_buf[src_x + 1];
                src_argb.blue = src_buf[src_x + 0];
                if(mask_buf == NULL) src_argb.alpha = opa;
                else src_argb.alpha = (mask_buf[dest_x] * opa) >> 8;

                blend_non_normal_pixel(&dest_buf[dest_x], src_argb, dsc->blend_mode);
            }
            if(mask_buf) mask_buf += mask_stride;
            dest_buf += dest_stride;
            src_buf += src_stride;
        }
    }
}

LV_ATTRIBUTE_FAST_MEM static void argb8888_image_blend(_lv_draw_sw_blend_image_dsc_t * dsc, uint32_t dest_px_size)
{
    int32_t w = dsc->dest_w;
    int32_t h = dsc->dest_h;
    lv_opa_t opa = dsc->opa;
    uint8_t * dest_buf = dsc->dest_buf;
    lv_coord_t dest_stride = dsc->dest_stride * dest_px_size;
    const lv_color32_t * src_buf_c32 = dsc->src_buf;
    lv_coord_t src_stride = dsc->src_stride;
    const lv_opa_t * mask_buf = dsc->mask_buf;
    lv_coord_t mask_stride = dsc->mask_stride;

    int32_t dest_x;
    int32_t src_x;
    int32_t y;


    if(dsc->blend_mode == LV_BLEND_MODE_NORMAL) {
        if(mask_buf == NULL && opa >= LV_OPA_MAX) {
            for(y = 0; y < h; y++) {
                for(dest_x = 0, src_x = 0; src_x < w; dest_x += dest_px_size, src_x++) {
                    lv_color_24_24_mix((const uint8_t *)&src_buf_c32[src_x], &dest_buf[dest_x], src_buf_c32[src_x].alpha);
                }
                dest_buf += dest_stride;
                src_buf_c32 += src_stride;
            }
        }
        else if(mask_buf == NULL && opa < LV_OPA_MAX) {
            for(y = 0; y < h; y++) {
                for(dest_x = 0, src_x = 0; src_x < w; dest_x += dest_px_size, src_x++) {
                    lv_color_24_24_mix((const uint8_t *)&src_buf_c32[src_x], &dest_buf[dest_x], (src_buf_c32[src_x].alpha * opa) >> 8);
                }
                dest_buf += dest_stride;
                src_buf_c32 += src_stride;
            }
        }
        else if(mask_buf && opa >= LV_OPA_MAX) {
            for(y = 0; y < h; y++) {
                for(dest_x = 0, src_x = 0; src_x < w; dest_x += dest_px_size, src_x++) {
                    lv_color_24_24_mix((const uint8_t *)&src_buf_c32[src_x], &dest_buf[dest_x],
                                       (src_buf_c32[src_x].alpha * mask_buf[src_x]) >> 8);
                }
                dest_buf += dest_stride;
                src_buf_c32 += src_stride;
                mask_buf += mask_stride;
            }
        }
        else if(mask_buf && opa < LV_OPA_MAX) {
            for(y = 0; y < h; y++) {
                for(dest_x = 0, src_x = 0; src_x < w; dest_x += dest_px_size, src_x++) {
                    lv_color_24_24_mix((const uint8_t *)&src_buf_c32[src_x], &dest_buf[dest_x],
                                       (src_buf_c32[src_x].alpha * mask_buf[src_x] * opa) >> 16);
                }
                dest_buf += dest_stride;
                src_buf_c32 += src_stride;
                mask_buf += mask_stride;
            }
        }
    }
    else {
        lv_color32_t src_argb;
        for(y = 0; y < h; y++) {
            for(dest_x = 0, src_x = 0; src_x < w; dest_x += dest_px_size, src_x ++) {
                src_argb = src_buf_c32[src_x];
                if(mask_buf == NULL) src_argb.alpha = (src_argb.alpha * opa) >> 8;
                else src_argb.alpha = (src_argb.alpha * mask_buf[dest_x] * opa) >> 16;

                blend_non_normal_pixel(&dest_buf[dest_x], src_argb, dsc->blend_mode);
            }
            if(mask_buf) mask_buf += mask_stride;
            dest_buf += dest_stride;
            src_buf_c32 += src_stride;
        }
    }
}

LV_ATTRIBUTE_FAST_MEM static inline void blend_non_normal_pixel(uint8_t * dest, lv_color32_t src, lv_blend_mode_t mode)
{
    uint8_t res[3];
    switch(mode) {
        case LV_BLEND_MODE_ADDITIVE:
            res[0] = LV_MIN(dest[0] + src.red, 255);
            res[1] = LV_MIN(dest[1] + src.green, 255);
            res[2] = LV_MIN(dest[2] + src.blue, 255);
            break;
        case LV_BLEND_MODE_SUBTRACTIVE:
            res[0] = LV_MAX(dest[0] - src.red, 0);
            res[1] = LV_MAX(dest[1] - src.green, 0);
            res[2] = LV_MAX(dest[2] - src.blue, 0);
            break;
        case LV_BLEND_MODE_MULTIPLY:
            res[0] = (dest[0] * src.red) >> 8;
            res[1] = (dest[1] * src.green) >> 8;
            res[2] = (dest[2] * src.blue) >> 8;
            break;
    }
    lv_color_24_24_mix(res, dest, src.alpha);
}


LV_ATTRIBUTE_FAST_MEM static inline void lv_color_24_24_mix(const uint8_t * src, uint8_t * dest, uint8_t mix)
{

    if(mix >= LV_OPA_MAX) {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
    }
    else {
        lv_opa_t mix_inv = 255 - mix;
        dest[0] = (uint32_t)((uint32_t)src[0] * mix + dest[0] * mix_inv) >> 8;
        dest[1] = (uint32_t)((uint32_t)src[1] * mix + dest[1] * mix_inv) >> 8;
        dest[2] = (uint32_t)((uint32_t)src[2] * mix + dest[2] * mix_inv) >> 8;
    }
}




#endif