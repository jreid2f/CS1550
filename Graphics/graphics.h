/*
    Joseph Reidell
    CS 1550
    Project 1: Double Buffered Graphics Library
*/

// typedef color_t variable
typedef unsigned short color_t;
// "public" functions
void init_graphics();
void exit_graphics();
char getKey();
void sleep_ms(long ms);
void clear_screen(void *img);
void draw_pixel(void *img, int x, int y, color_t color);
void draw_line(void *img, int x1, int y1, int x2, int y2, color_t c);
void *new_offscreen_buffer();
void blit(void *src);
// RGB Macro Conversion
#define RGB(R, G, B) (R << 11 | G << 5 | B)

