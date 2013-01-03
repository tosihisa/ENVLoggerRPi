/** Tiny library of drawing in framebuffer
 *
 * @section LICENSE
 * This source code is "GPLv2" license. 
 * The original of the font image is Linux kernel source. 
 * Because the Linux kernel license is "GPLv2 with the exception articles",
 * this program also applies GPLv2. 
 *
 * Copyright (C) 2010 tosihisa
 *
 * @section DESCRIPTION
 * Tiny library of drawing in framebuffer
 *
 */

#ifndef __TINYDRAW_H    /* { */
#define    __TINYDRAW_H

#include "linuxfont/linuxfont.h"

/** FrameBuffer(VRAM) X(Width) size */
#define TD_X_MAX  (128)
/** FrameBuffer(VRAM) Y(Width) size */
#define TD_Y_MAX  (64)

/** X/Y coordinate range type */
typedef short TD_XY_t;

/* X/Y coordinate type */
typedef struct td_point_s {
    TD_XY_t x;
    TD_XY_t y;
} TD_POINT_t;

/** color type */
typedef unsigned char TD_COLOR_t;

/** color type (foreground and background pair) */
typedef struct td_color_pair_s {
    TD_COLOR_t fore;    /**< foreground */
    TD_COLOR_t back;    /**< background */
} TD_COLOR_PAIR_t;

/**
 * Tiny library of drawing in framebuffer
 */
class TinyDraw {
public:
    /**
     * The frame buffer is cleared by 0. 
     *
     * @param none.
     * @return none.
     */
    void clear(void);

    /**
     * The dot is drawn in specified coordinates by the specified color code. 
     *
     * @param x [in] X coordinates
     * @param y [in] Y coordinates
     * @param fcol [in] Color-code (0 or 1 or COL_TRANS).
     * @return none.
     */
    void drawPoint(TD_XY_t x,TD_XY_t y,TD_COLOR_t fcol);

    /**
     * It draws in the line. 
     *
     * @param p1 [in] point of start.
     * @param p2 [in] point of end.
     * @param col [in] Color-code (now only use 'fore').
     * @return none.
     */
    void drawLine(TD_POINT_t *p1,TD_POINT_t *p2,TD_COLOR_PAIR_t *col);

    /**
     * It draws in the character.
     *
     * @param pnt [in] point of draw (upper left corner of font)
     * @param c [in] character code(only use ASCII-CODE now)
     * @param col [in] Color-code.
     * @param fontdesc [in] font-descriptor.
     * @return none.
     */
    void drawChar(TD_POINT_t *pnt,int c,TD_COLOR_PAIR_t *col,const struct font_desc *fontdesc);

    /**
     * It draws in the string.
     *
     * @param pnt [in] point of draw (upper left corner of font)
     * @param str [in] string (only use ASCII-CODE now)
     * @param col [in] Color-code.
     * @param fontdesc [in] font-descriptor.
     * @return none.
     */
    void drawStr(TD_POINT_t *pnt,char *str,TD_COLOR_PAIR_t *col,const struct font_desc *fontdesc);

    /**
     * Get the bit map image of the font. 
     *
     * @param c [in] character code(only use ASCII-CODE now)
     * @param fontdesc [in] font-descriptor.
     * @return pointer for bit map image.
     */
    char *getCharImage(int c,const struct font_desc *fontdesc);

    /**
     * Get the framebuffer pointer. 
     *
     * @param pnt [in] Coordinates.
     * @return pointer for framebuffer.
     */
    TD_COLOR_t *getVRAMPtr(TD_POINT_t *pnt);

    /**
     * Get the top of framebuffer pointer. 
     *
     * @param none.
     * @return pointer for top of framebuffer.
     */
    TD_COLOR_t *getVRAMPtr(void);

    static const int X_MAX = TD_X_MAX;
    static const int Y_MAX = TD_Y_MAX;
    static const TD_COLOR_t COL_TRANS = 0x80;

private:
    TD_XY_t calcAbs(TD_XY_t v);
    TD_XY_t calcMin(TD_XY_t v1,TD_XY_t v2);
    int checkSign(TD_XY_t v);

    TD_COLOR_t VRAM[(TD_X_MAX * TD_Y_MAX * sizeof(TD_COLOR_t))/8];
};

#endif /* __TINYDRAW_H    } */

