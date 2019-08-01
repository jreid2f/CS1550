/*
    Joseph Reidell
    CS 1550
    Project 1: Double Buffered Graphics Library
*/

#include "graphics.h"
#include <stdio.h>

int main(int argc, char **argv){
    int i, x, y;
    int new_x, new_y, new_direction; 
    printf("Use W, A, S, D Keys to move the snake!");
    printf("Press Q to quit the game");

    for(i = 10; i > 0; --i){
        sleep_ms(999);
    }

    char keys = getKey();
    init_graphics();

    color_t black = RGB(0,0,0);
    color_t blue = RGB(0,0,221);

    char *buffer = (char *) new_offscreen_buffer();

    x = 319;
    y = 239;

    draw_pixel(buffer, x, y, blue);
    blit(buffer);

    while(keys != 'q'){
        draw_pixel(buffer, x, y, black);

        if(keys != 0){
            if(keys == 'w'){
                new_x = 0;
                new_y = 1;
                new_direction = 0;
            }
            else if(keys == 'a'){
                new_x = 0;
                new_y = 1;
                new_direction = 1;
            }
            else if(keys == 's'){
                new_x = 1;
                new_y = 0;
                new_direction = 0;
            }
            else if(keys == 'd'){
                new_x = 1;
                new_y = 0;
                new_direction = 1;
            }
        }

        draw_pixel(buffer, x, y, blue);
        sleep_ms(1);
        blit(buffer);
        keys = getKey();

    }

    clear_screen(buffer);
    blit(buffer);
    exit_graphics();

    return 0;
}
