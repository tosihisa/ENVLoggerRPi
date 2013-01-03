
/**
 * @section LICENSE
 *
 * This source code is "GPLv2" license. 
 * The original of the font image is Linux kernel source. 
 * Because the Linux kernel license is "GPLv2 with the exception articles",
 * this program also applies GPLv2. 
 *
 * Copyright (C) 2010 tosihisa
 *
 * @section DESCRIPTION
 *
 * Tiny library of drawing in framebuffer
 *
 */

#include "tinydraw.h"

TD_COLOR_t *TinyDraw::getVRAMPtr(void) {
    return &VRAM[0];
}

TD_COLOR_t *TinyDraw::getVRAMPtr(TD_POINT_t *pnt) {
    int pageNo = pnt->y / 8;
    return &(VRAM[(X_MAX * pageNo) + pnt->x]);
}

void TinyDraw::drawPoint(TD_XY_t x,TD_XY_t y,TD_COLOR_t fcol) {
    TD_COLOR_t *byte_ptr;
    TD_COLOR_t bitmask = 0x80;
    TD_POINT_t pnt;

    pnt.x = x;
    pnt.y = y;
    byte_ptr = getVRAMPtr(&pnt);
    bitmask = 0x01 << (y % 8);

    /* When it is not a transparent shade, it writes it in VRAM. */
    if((fcol & COL_TRANS) == 0){
        if(fcol != 0x00){
            *byte_ptr |= bitmask;        /* write 1 */
        } else {
            *byte_ptr &= (~bitmask);    /* write 0 */
        }
    }
}

void TinyDraw::clear(void) {
    int i;
    for (i = 0;i < sizeof(VRAM);i++) {
        VRAM[i] = 0;
    }
}

char *TinyDraw::getCharImage(int c,const struct font_desc *fontdesc) {
    int w;
    w = (fontdesc->width > 8) ? 2 : 1;
    return ((char *)fontdesc->data) + ((w*fontdesc->height)*(c & 0xff));
}

/* It corresponds only when the width is below 16dots. */
void TinyDraw::drawChar(TD_POINT_t *pnt,int c,TD_COLOR_PAIR_t *col,const struct font_desc *fontdesc) {
    int w,h;
    char *imagePtr = getCharImage(c,fontdesc);
    unsigned long mask = 0;
    unsigned long maskTbl[] = { 0x80,0x8000 };
    unsigned short yline;
    TD_COLOR_t wcol;

    for (h = 0;h < fontdesc->height;h++) {
        mask = maskTbl[ (fontdesc->width - 1) / 8 ];
        yline = (unsigned short)(*(imagePtr + 0));
        if(fontdesc->width > 8){
            yline = yline << 8;
            yline |= (unsigned short)(*(imagePtr + 1));
        }
        for (w = 0;w < fontdesc->width;w++) {
            wcol = (yline & mask) ? col->fore : col->back;
            drawPoint(pnt->x+w,pnt->y+h,wcol);
            mask = mask >> 1;
        }
        imagePtr += (fontdesc->width > 8) ? 2 : 1;
    }
}

void TinyDraw::drawStr(TD_POINT_t *pnt,char *str,TD_COLOR_PAIR_t *col,const struct font_desc *fontdesc) {
    char c;
    TD_POINT_t tmp = *pnt;
    int i = 0;
    for (i = 0;(c = *(str+i)) != (char)('\0');i++) {
        drawChar(&tmp,c,col,fontdesc);
        tmp.x += fontdesc->width;
    }
}

TD_XY_t TinyDraw::calcAbs(TD_XY_t v) {
    return (v < 0) ? (0-v) : v;
}

TD_XY_t TinyDraw::calcMin(TD_XY_t v1,TD_XY_t v2) {
    return (v1 <= v2) ? v1 : v2;
}

int TinyDraw::checkSign(TD_XY_t v) {
    if (v < 0)
        return -1;
    else if (v == 0)
        return 0;
    return 1;
}

void TinyDraw::drawLine(TD_POINT_t *p1,TD_POINT_t *p2,TD_COLOR_PAIR_t *col) {
    TD_XY_t d_x;
    TD_XY_t d_y;
    int s_1;
    int s_2;
    int i_c = 0;
    TD_XY_t tmp;
    long e;
    TD_XY_t i;
    TD_XY_t n_x;
    TD_XY_t n_y;

    n_x = p1->x;
    n_y = p1->y;
    d_x = calcAbs(p2->x - p1->x);
    d_y = calcAbs(p2->y - p1->y);
    s_1 = checkSign(p2->x - p1->x);
    s_2 = checkSign(p2->y - p1->y);

    if (d_x == 0) {
        /* vertical */
        n_y = calcMin(p1->y,p2->y);
        for (i=0;i <= d_y;i++) {
            drawPoint(n_x,n_y,col->fore);
            n_y++;
        }
    } else if (d_y == 0) {
        /* horizontal */
        n_x = calcMin(p1->x,p2->x);
        for (i=0;i <= d_x;i++) {
            drawPoint(n_x,n_y,col->fore);
            n_x++;
        }
    } else {
        i_c = 0;
        if (d_y > d_x) {
            tmp = d_x;
            d_x = d_y;
            d_y = tmp;
            i_c = 1;
        }

        e = 2 * d_y - d_x;
        for (i=0;i <= d_x;i++) {
            drawPoint(n_x,n_y,col->fore);
            while (e>=0) {
                if (i_c) {
                    n_x = n_x + s_1;
                } else {
                    n_y = n_y + s_2;
                }
                e = e - 2 * d_x;
            }
            if (i_c) {
                n_y = n_y + s_2;
            } else {
                n_x = n_x + s_1;
            }
            e = e + 2 * d_y;
        }
    }
    /* last point (or p1 == p2 (1dot)) */
    drawPoint(p2->x,p2->y,col->fore);
}

#if 0
void TinyDraw::drawEllipse( int x0, int y0, int r, int a, int b, int col )
{
  int x = (int)( (double)r / sqrt( (double)a ) );
  int y = 0;
  double d = sqrt( (double)a ) * (double)r;
  int F = (int)( -2.0 * d ) +     a + 2 * b;
  int H = (int)( -4.0 * d ) + 2 * a     + b;

  while ( x >= 0 ) {
    pset( x0 + x, y0 + y, col );
    pset( x0 - x, y0 + y, col );
    pset( x0 + x, y0 - y, col );
    pset( x0 - x, y0 - y, col );
    if ( F >= 0 ) {
      --x;
      F -= 4 * a * x;
      H -= 4 * a * x - 2 * a;
    }
    if ( H < 0 ) {
      ++y;
      F += 4 * b * y + 2 * b;
      H += 4 * b * y;
    }
  }
}

void TinyDraw::drawCircle( int x0, int y0, int r, unsigned int col )
{
  int x = r;
  int y = 0;
  int F = -2 * r + 3;

  while ( x >= y ) {
    pset( x0 + x, y0 + y, col );
    pset( x0 - x, y0 + y, col );
    pset( x0 + x, y0 - y, col );
    pset( x0 - x, y0 - y, col );
    pset( x0 + y, y0 + x, col );
    pset( x0 - y, y0 + x, col );
    pset( x0 + y, y0 - x, col );
    pset( x0 - y, y0 - x, col );
    if ( F >= 0 ) {
      x--;
      F -= 4 * x;
    }
    y++;
    F += 4 * y + 2;
  }
}

void TinyDraw::rotatePoint(int p1x,int p1y,int cx,int cy,double deg,int *p2x,int *p2y)
{
	double xl1;
	double yl1;
	double xl2;
	double yl2;
	double _rad = deg * PI / 180;
	double _cos_rad = cos(_rad);
	double _sin_rad = sin(_rad);

	xl1 = p1x - cx;
	yl1 = p1y - cy;

	xl2 = (_cos_rad*xl1) - (_sin_rad*yl1);
	yl2 = (_sin_rad*xl1) - (_cos_rad*yl1);
	*p2x = cx + (int)xl2;
	*p2y = cy + (int)yl2;
}
#endif
