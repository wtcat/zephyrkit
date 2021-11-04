#define GX_SOURCE_CODE
#include "gx_api.h"
#include "gx_display.h"
#include "gx_utility.h"


#define __write_pixel(_addr, _color) \
    *(USHORT *)(_addr) = __builtin_bswap16((USHORT)_color)
#define __read_pixel(_addr) \
    __builtin_bswap16(*(USHORT *)_addr)

#define REDVAL(_c)     (GX_UBYTE)(((_c) >> 11) & 0x1f)
#define GREENVAL(_c)   (GX_UBYTE)(((_c) >> 5) & 0x3f)
#define BLUEVAL(_c)    (GX_UBYTE)(((_c)) & 0x1f)
#define ASSEMBLECOLOR(_r, _g, _b) \
    ((((_r) & 0x1f) << 11) |      \
     (((_g) & 0x3f) << 5) |       \
     (((_b) & 0x1f)))
     

static VOID _gx_drv_16bpp_simple_line_draw(GX_DRAW_CONTEXT *context, INT xstart, 
    INT ystart, INT xend, INT yend)
{
INT           curx;
INT           cury;
INT           x_sign;
INT           y_sign;
INT           decision;
INT           nextx;
INT           nexty;
INT           y_increment;
GX_POINT      end_point;
GX_POINT      mid_point;
GX_RECTANGLE  half_rectangle;
GX_RECTANGLE  half_over;
INT           sign;
INT           steps;

USHORT       *put;
USHORT       *next_put;

GX_BOOL       clipped = GX_TRUE;
INT           dx = GX_ABS(xend - xstart);
INT           dy = GX_ABS(yend - ystart);

GX_RECTANGLE *clip = context -> gx_draw_context_clip;
GX_COLOR      linecolor = context -> gx_draw_context_brush.gx_brush_line_color;
#if defined GX_BRUSH_ALPHA_SUPPORT  
GX_UBYTE      alpha;

    alpha = context -> gx_draw_context_brush.gx_brush_alpha;
    if (alpha == 0)
    {
        /* Nothing to drawn. Just return. */
        return;
    }
    if (alpha != 0xff)
    {
        _gx_display_driver_simple_line_alpha_draw(context, xstart, ystart, 
            xend, yend, alpha);
        return;
    }
#endif

    if (((dx >= dy && (xstart > xend)) || ((dy > dx) && ystart > yend)))
    {
        GX_SWAP_VALS(xend, xstart);
        GX_SWAP_VALS(yend, ystart);
    }
    x_sign = (xend - xstart) / dx;
    y_sign = (yend - ystart) / dy;

    if (y_sign > 0)
    {
        y_increment = context -> gx_draw_context_pitch;
    }
    else
    {
        y_increment = 0 - context -> gx_draw_context_pitch;
    }

    put = (USHORT *)(context -> gx_draw_context_memory) + 
            ystart * context -> gx_draw_context_pitch + xstart;
    next_put = (USHORT *)(context -> gx_draw_context_memory) + 
            yend * context -> gx_draw_context_pitch + xend;


    end_point.gx_point_x = (GX_VALUE)xstart;
    end_point.gx_point_y = (GX_VALUE)ystart;

    if (_gx_utility_rectangle_point_detect(clip, end_point))
    {
        end_point.gx_point_x = (GX_VALUE)xend;
        end_point.gx_point_y = (GX_VALUE)yend;

        if (_gx_utility_rectangle_point_detect(clip, end_point))
        {
            clipped = GX_FALSE;
        }
    }

    if (clipped)
    {
        /* here if we must do clipping in the inner loop, because one
           or both of the end points are outside clipping rectangle */

        /* Calculate the middle point of the line.  */
        mid_point.gx_point_x = (GX_VALUE)((xend + xstart) >> 1);
        mid_point.gx_point_y = (GX_VALUE)((yend + ystart) >> 1);

        /* Judge the clip in which side.  */
        if (_gx_utility_rectangle_point_detect(clip, mid_point))
        {

            /* the clip in two sides.  */
            if (dx >= dy)
            {
                /* walk out the clipping point.  */
                for (curx = xstart, cury = ystart, decision = (dx >> 1); 
                    curx < mid_point.gx_point_x;
                    curx++, decision += dy)
                {
                    if (decision >= dx)
                    {
                        decision -= dx;
                        cury += y_sign;
                        put += y_increment;
                    }

                    if (curx >= clip -> gx_rectangle_left &&
                        cury >= clip -> gx_rectangle_top &&
                        cury <= clip -> gx_rectangle_bottom)
                    {
                        break;
                    }
                    put++;
                }
                for (; curx <= mid_point.gx_point_x;
                     curx++, decision += dy)
                {
                    if (decision >= dx)
                    {
                        decision -= dx;
                        cury += y_sign;
                        put += y_increment;
                    }
                    __write_pixel(put, linecolor);
                    put++;
                }
                for (nextx = xend, nexty = yend, decision = (dx >> 1); 
                    nextx > mid_point.gx_point_x;
                    nextx--, decision += dy)
                {
                    if (decision >= dx)
                    {
                        decision -= dx;
                        nexty -= y_sign;
                        next_put -= y_increment;
                    }
                    if (nextx <= clip -> gx_rectangle_right &&
                        nexty >= clip -> gx_rectangle_top &&
                        nexty <= clip -> gx_rectangle_bottom)
                    {
                        break;
                    }
                    next_put--;
                }

                for (; nextx > mid_point.gx_point_x;
                     nextx--, decision += dy)
                {
                    if (decision >= dx)
                    {
                        decision -= dx;
                        nexty -= y_sign;
                        next_put -= y_increment;
                    }
                    __write_pixel(next_put, linecolor);
                    next_put--;
                }
            }
            else
            {
                for (nextx = xend, nexty = yend, decision = (dy >> 1); 
                    nexty > mid_point.gx_point_y;
                    nexty--, decision += dx)
                {
                    if (decision >= dy)
                    {
                        decision -= dy;
                        nextx -= x_sign;
                        next_put -= x_sign;
                    }
                    if (nextx >= clip -> gx_rectangle_left &&
                        nextx <= clip -> gx_rectangle_right &&
                        nexty <= clip -> gx_rectangle_bottom)
                    {
                        break;
                    }
                    next_put -= context -> gx_draw_context_pitch;
                }

                for (; nexty > mid_point.gx_point_y;
                     nexty--, decision += dx)
                {
                    if (decision >= dy)
                    {
                        decision -= dy;
                        nextx -= x_sign;
                        next_put -= x_sign;
                    }
                    __write_pixel(next_put, linecolor);
                    next_put -= context -> gx_draw_context_pitch;
                }

                /* walk out the clipping point.  */
                for (curx = xstart, cury = ystart, decision = (dy >> 1); 
                    cury < mid_point.gx_point_y;
                    cury++, decision += dx)
                {
                    if (decision >= dy)
                    {
                        decision -= dy;
                        curx += x_sign;
                        put += x_sign;
                    }

                    if (curx >= clip -> gx_rectangle_left &&
                        curx <= clip -> gx_rectangle_right &&
                        cury >= clip -> gx_rectangle_top)
                    {
                        break;
                    }
                    put += context -> gx_draw_context_pitch;
                }
                for (; cury <= mid_point.gx_point_y;
                     cury++, decision += dx)
                {
                    if (decision >= dy)
                    {
                        decision -= dy;
                        curx += x_sign;
                        put += x_sign;
                    }
                    __write_pixel(put, linecolor);
                    put += context -> gx_draw_context_pitch;
                }
            }   
        }
        else
        {
            /* The clip stay at one side.  */
            if (dx >= dy)
            {
                half_rectangle.gx_rectangle_left = (GX_VALUE)xstart;
                half_rectangle.gx_rectangle_right = mid_point.gx_point_x;
                if (y_sign == 1)
                {
                    half_rectangle.gx_rectangle_top = (GX_VALUE)ystart;
                    half_rectangle.gx_rectangle_bottom = mid_point.gx_point_y;
                }
                else
                {
                    half_rectangle.gx_rectangle_top = mid_point.gx_point_y;
                    half_rectangle.gx_rectangle_bottom = (GX_VALUE)ystart;
                }

                if (_gx_utility_rectangle_overlap_detect(clip, &half_rectangle, &half_over))
                {
                    curx = xstart;
                    cury = ystart;
                    steps = mid_point.gx_point_x - curx + 1;
                    sign = 1;
                }
                else
                {
                    curx = xend;
                    cury = yend;
                    steps = xend - mid_point.gx_point_x;
                    sign = -1;
                    y_increment = 0 - y_increment;
                    y_sign = 0 - y_sign;
                    put = next_put;
                }
                for (decision = (dx >> 1); steps > 0; curx += sign, decision += dy, steps--)
                {
                    if (decision >= dx)
                    {
                        decision -= dx;
                        cury += y_sign;
                        put += y_increment;
                    }

                    if (curx >= clip -> gx_rectangle_left &&
                        curx <= clip -> gx_rectangle_right &&
                        cury >= clip -> gx_rectangle_top &&
                        cury <= clip -> gx_rectangle_bottom)
                    {
                        __write_pixel(put, linecolor);
                    }
                    put += sign;
                }
            }
            else
            {
                half_rectangle.gx_rectangle_top = (GX_VALUE)ystart;
                half_rectangle.gx_rectangle_bottom = mid_point.gx_point_y;
                if (x_sign == 1)
                {
                    half_rectangle.gx_rectangle_right = mid_point.gx_point_x;
                    half_rectangle.gx_rectangle_left = (GX_VALUE)xstart;
                }
                else
                {
                    half_rectangle.gx_rectangle_right = (GX_VALUE)xstart;
                    half_rectangle.gx_rectangle_left = mid_point.gx_point_x;
                }

                if (_gx_utility_rectangle_overlap_detect(clip, &half_rectangle, &half_over))
                {
                    curx = xstart;
                    cury = ystart;
                    steps = mid_point.gx_point_y - cury + 1;
                    y_increment = context -> gx_draw_context_pitch;
                    sign = 1;
                }
                else
                {
                    curx = xend;
                    cury = yend;
                    steps = yend - mid_point.gx_point_y;
                    sign = -1;
                    y_increment = 0 - context -> gx_draw_context_pitch;
                    x_sign = 0 - x_sign;
                    put = next_put;
                }

                for (decision = (dy >> 1); steps > 0; cury += sign, decision += dx, steps--)
                {
                    if (decision >= dy)
                    {
                        decision -= dy;
                        curx += x_sign;
                        put += x_sign;
                    }
                    if (curx >= clip -> gx_rectangle_left &&
                        curx <= clip -> gx_rectangle_right &&
                        cury >= clip -> gx_rectangle_top &&
                        cury <= clip -> gx_rectangle_bottom)
                    {
                        __write_pixel(put, linecolor);
                    }
                    put += y_increment;
                }
            }
        }
    }
    else
    {
        /* here if both line ends lie within clipping rectangle, we can
           run a faster inner loop */
        if (dx >= dy)
        {
            put = (USHORT *)(context -> gx_draw_context_memory) + ystart * context -> gx_draw_context_pitch + xstart;
            next_put = (USHORT *)(context -> gx_draw_context_memory) + yend * context -> gx_draw_context_pitch + xend;

            for (curx = xstart, cury = ystart, nextx = xend, nexty = yend,
                 decision = (dx >> 1); curx <= nextx; curx++, nextx--,
                 decision += dy)
            {

                if (decision >= dx)
                {
                    decision -= dx;
                    cury += y_sign;
                    nexty -= y_sign;

                    put += y_increment;
                    next_put -= y_increment;
                }
                __write_pixel(put, linecolor);
                __write_pixel(next_put, linecolor);

                put++;
                next_put--;
            }
        }
        else
        {

            for (curx = xstart, cury = ystart, nextx = xend, nexty = yend,
                 decision = (dy >> 1); cury <= nexty; cury++, nexty--,
                 decision += dx)
            {
                if (decision >= dy)
                {
                    decision -= dy;
                    curx += x_sign;
                    nextx -= x_sign;

                    put += x_sign;
                    next_put -= x_sign;
                }
                __write_pixel(put, linecolor);
                __write_pixel(next_put, linecolor);

                put += context -> gx_draw_context_pitch;
                next_put -= context -> gx_draw_context_pitch;
            }
        }
    }
}

static VOID _gx_drv_16bpp_horizontal_line_draw(GX_DRAW_CONTEXT *context, INT xstart, 
    INT xend, INT ypos, INT width, GX_COLOR color)
{
INT     row;
INT     column;
USHORT *put;
USHORT *rowstart;
INT     len = xend - xstart + 1;

#if defined GX_BRUSH_ALPHA_SUPPORT  
GX_UBYTE alpha;

    alpha = context -> gx_draw_context_brush.gx_brush_alpha;
    if (alpha == 0)
    {
        /* Nothing to drawn. Just return. */
        return;
    }

    if (alpha != 0xff)
    {
        _gx_display_driver_horizontal_line_alpha_draw(context, xstart, xend, ypos, width, color, alpha);
        return;
    }
#endif

    /* pick up start address of canvas memory */
    rowstart = (USHORT *)context -> gx_draw_context_memory;

    /* calculate start of row address */
    rowstart += context -> gx_draw_context_pitch * ypos;

    /* calculate pixel address */
    rowstart += xstart;
    /* draw 1-pixel hi lines to fill width */
    for (row = 0; row < width; row++)
    {
        put = rowstart;

        /* draw one line, left to right */
        for (column = 0; column < len; column++)
        {
            __write_pixel(put, color);
            put++;
        }
        rowstart += context -> gx_draw_context_pitch;
    }

}

static VOID _gx_drv_16bpp_vertical_line_draw(GX_DRAW_CONTEXT *context, INT ystart, 
    INT yend, INT xpos, INT width, GX_COLOR color)
{
INT     row;
INT     column;
USHORT *put;
USHORT *rowstart;
INT     len = yend - ystart + 1;
#if defined GX_BRUSH_ALPHA_SUPPORT  
GX_UBYTE alpha;

    alpha = context -> gx_draw_context_brush.gx_brush_alpha;
    if (alpha == 0)
    {
        /* Nothing to drawn. Just return. */
        return;
    }
    if (alpha != 0xff)
    {
        _gx_display_driver_vertical_line_alpha_draw(context, ystart, yend, xpos, width, color, alpha);
        return;
    }
#endif

    /* pick up starting address of canvas memory */
    rowstart = (USHORT *)context -> gx_draw_context_memory;

    /* calculate start of scanline */
    rowstart += context -> gx_draw_context_pitch * ystart;

    /* offset into starting pixel */
    rowstart += xpos;

    /* draw line from top to bottom */
    for (row = 0; row < len; row++)
    {
        put = rowstart;

        /* draw line width from left to right */
        for (column = 0; column < width; column++)
        {
            __write_pixel(put, color);
             put++;
        }

        /* advance to the next scaneline */
        rowstart += context -> gx_draw_context_pitch;
    }
}

static VOID _gx_drv_16bpp_horizontal_pattern_line_draw(GX_DRAW_CONTEXT *context, 
    INT xstart, INT xend, INT ypos)
{
INT     column;
USHORT *put;
USHORT *rowstart;
ULONG   pattern;
ULONG   mask;
USHORT  on_color;
USHORT  off_color;

INT     len = xend - xstart + 1;

    /* pick up start address of canvas memory */
    rowstart = (USHORT *)context -> gx_draw_context_memory;

    /* calculate start of row address */
    rowstart +=  context -> gx_draw_context_pitch * ypos;

    /* calculate pixel address */
    rowstart +=  xstart;
    /* draw 1-pixel hi lines to fill width */

    /* pick up the requested pattern and mask */
    pattern = context -> gx_draw_context_brush.gx_brush_line_pattern;
    mask = context -> gx_draw_context_brush.gx_brush_pattern_mask;
    on_color = (USHORT)context -> gx_draw_context_brush.gx_brush_line_color;
    off_color = (USHORT)context -> gx_draw_context_brush.gx_brush_fill_color;

    put = rowstart;

    /* draw one line, left to right */
    for (column = 0; column < len; column++)
    {
        if (pattern & mask)
        {
            __write_pixel(put, on_color);
             put++;
        }
        else
        {
            __write_pixel(put, off_color);
            put++;
        }
        mask >>= 1;
        if (!mask)
        {
            mask = 0x80000000;
        }
    }

    /* save current masks value back to brush */
    context -> gx_draw_context_brush.gx_brush_pattern_mask = mask;
}

static VOID _gx_drv_16bpp_vertical_pattern_line_draw(GX_DRAW_CONTEXT *context, 
    INT ystart, INT yend, INT xpos)
{
INT     row;
USHORT *put;
USHORT *rowstart;
ULONG   pattern;
ULONG   mask;
USHORT  on_color;
USHORT  off_color;

INT     len = yend - ystart + 1;

    /* pick up starting address of canvas memory */
    rowstart =  (USHORT *)context -> gx_draw_context_memory;

    /* calculate start of scanline */
    rowstart += context -> gx_draw_context_pitch * ystart;

    /* offset into starting pixel */
    rowstart += xpos;

    /* pick up the requested pattern and mask */
    pattern = context -> gx_draw_context_brush.gx_brush_line_pattern;
    mask = context -> gx_draw_context_brush.gx_brush_pattern_mask;
    on_color = (USHORT)context -> gx_draw_context_brush.gx_brush_line_color;
    off_color = (USHORT)context -> gx_draw_context_brush.gx_brush_fill_color;

    /* draw line from top to bottom */
    for (row = 0; row < len; row++)
    {
        put = rowstart;

        if (pattern & mask)
        {
            __write_pixel(put, on_color);
        }
        else
        {
            __write_pixel(put, off_color);
        }

        mask >>= 1;
        if (!mask)
        {
            mask = 0x80000000;
        }

        /* advance to the next scaneline */
        rowstart +=  context -> gx_draw_context_pitch;
    }
    /* save current masks value back to brush */
    context -> gx_draw_context_brush.gx_brush_pattern_mask = mask;
}


static VOID _gx_drv_16bpp_pixel_write(GX_DRAW_CONTEXT *context, INT x, INT y,
    GX_COLOR color)
{
USHORT *put = (USHORT *)context -> gx_draw_context_memory;

    /* calculate address of scan line */
    put += context -> gx_draw_context_pitch * y;

    /* step in by x coordinate */
    put += x;

    /* write the pixel value */
    __write_pixel(put, color);
}

static VOID _gx_drv_565rgb_pixel_blend(GX_DRAW_CONTEXT *context, INT x, INT y, 
    GX_COLOR fcolor, GX_UBYTE alpha)
{
GX_UBYTE fred, fgreen, fblue;
GX_UBYTE bred, bgreen, bblue;
GX_UBYTE balpha;

USHORT   bcolor;
USHORT  *put;


    /* Is the pixel non-transparent? */
    if (alpha > 0)
    {
        /* calculate address of pixel */
        put = (USHORT *)context -> gx_draw_context_memory;
        put += context -> gx_draw_context_pitch * y;
        put += x;

        /* No need to blend if alpha value is 255. */
        if (alpha == 255)
        {
            __write_pixel(put, fcolor);
            return;
        }

        /* split foreground into red, green, and blue components */
        fred = REDVAL(fcolor);
        fgreen = GREENVAL(fcolor);
        fblue = BLUEVAL(fcolor);

        /* read background color */
        //bcolor = *put;
        bcolor = __read_pixel(put);

        /* split background color into red, green, and blue components */
        bred = REDVAL(bcolor);
        bgreen = GREENVAL(bcolor);
        bblue = BLUEVAL(bcolor);

        /* background alpha is inverse of foreground alpha */
        balpha = (GX_UBYTE)(256 - alpha);

        /* blend foreground and background, each color channel */
        fred = (GX_UBYTE)(((bred * balpha) + (fred * alpha)) >> 8);
        fgreen = (GX_UBYTE)(((bgreen * balpha) + (fgreen * alpha)) >> 8);
        fblue = (GX_UBYTE)(((bblue * balpha) + (fblue * alpha)) >> 8);

        /* re-assemble into 16-bit color and write it out */
        __write_pixel(put, ASSEMBLECOLOR(fred, fgreen, fblue));
    }
}

static VOID _gx_drv_565rgb_canvas_blend(GX_CANVAS *canvas, GX_CANVAS *composite)
{
GX_RECTANGLE dirty;
GX_RECTANGLE overlap;
USHORT      *read;
USHORT      *read_start;
USHORT      *write;
USHORT      *write_start;
USHORT       fcolor;
GX_UBYTE     fred, fgreen, fblue;
GX_UBYTE     bred, bgreen, bblue;
GX_UBYTE     alpha, balpha;

USHORT       bcolor;
INT          row;
INT          col;

    dirty.gx_rectangle_left = dirty.gx_rectangle_top = 0;
    dirty.gx_rectangle_right = (GX_VALUE)(canvas -> gx_canvas_x_resolution - 1);
    dirty.gx_rectangle_bottom = (GX_VALUE)(canvas -> gx_canvas_y_resolution - 1);

    _gx_utility_rectangle_shift(&dirty, canvas -> gx_canvas_display_offset_x, canvas -> gx_canvas_display_offset_y);

    if (_gx_utility_rectangle_overlap_detect(&dirty, &composite -> gx_canvas_dirty_area, &overlap))
    {
        alpha = canvas -> gx_canvas_alpha;
        balpha = (GX_UBYTE)(256 - alpha);

        read_start = (USHORT *)canvas -> gx_canvas_memory;

        /* index into starting row */
        read_start += (overlap.gx_rectangle_top - dirty.gx_rectangle_top) * canvas -> gx_canvas_x_resolution;

        /* index into pixel */

        read_start += overlap.gx_rectangle_left - dirty.gx_rectangle_left;

        /* calculate the write pointer */
        write_start = (USHORT *)composite -> gx_canvas_memory;
        write_start += overlap.gx_rectangle_top * composite -> gx_canvas_x_resolution;
        write_start += overlap.gx_rectangle_left;

        for (row = overlap.gx_rectangle_top; row <= overlap.gx_rectangle_bottom; row++)
        {
            read = read_start;
            write = write_start;

            for (col = overlap.gx_rectangle_left; col <= overlap.gx_rectangle_right; col++)
            {
                /* read the foreground color */
                fcolor = *read++;

                /* split foreground into red, green, and blue components */
                fred = REDVAL(fcolor);
                fgreen = GREENVAL(fcolor);
                fblue = BLUEVAL(fcolor);

                /* read background color */
                bcolor = *write;

                /* split background color into red, green, and blue components */
                bred = REDVAL(bcolor);
                bgreen = GREENVAL(bcolor);
                bblue = BLUEVAL(bcolor);

                /* blend foreground and background, each color channel */
                fred = (GX_UBYTE)(((bred * balpha) + (fred * alpha)) >> 8);
                fgreen = (GX_UBYTE)(((bgreen * balpha) + (fgreen * alpha)) >> 8);
                fblue = (GX_UBYTE)(((bblue * balpha) + (fblue * alpha)) >> 8);

                /* re-assemble into 16-bit color and write it out */
                __write_pixel(write, ASSEMBLECOLOR(fred, fgreen, fblue));
                write++;
            }
            write_start += composite -> gx_canvas_x_resolution;
            read_start += canvas -> gx_canvas_x_resolution;
        }
    }
}

VOID guix_rgb565_display_driver_swap_byteorder(GX_DISPLAY *display)
{
    display->gx_display_driver_simple_line_draw = 
        _gx_drv_16bpp_simple_line_draw;
    display->gx_display_driver_horizontal_line_draw = 
        _gx_drv_16bpp_horizontal_line_draw;
    display->gx_display_driver_vertical_line_draw = 
        _gx_drv_16bpp_vertical_line_draw;
    display->gx_display_driver_horizontal_pattern_line_draw = 
        _gx_drv_16bpp_horizontal_pattern_line_draw;
    display->gx_display_driver_vertical_pattern_line_draw = 
        _gx_drv_16bpp_vertical_pattern_line_draw;
    display->gx_display_driver_pixel_write = _gx_drv_16bpp_pixel_write;
    display->gx_display_driver_canvas_blend = _gx_drv_565rgb_canvas_blend;
    display->gx_display_driver_pixel_blend = _gx_drv_565rgb_pixel_blend;
}

