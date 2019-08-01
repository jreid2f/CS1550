/*
    Joseph Reidell
    CS 1550
    Project 1: Double Buffered Graphics Library
*/

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <fcntl.h>

#include "graphics.h"

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/select.h>

int devFile;
color_t *frameBuffer;
struct fb_var_screeninfo varScreen;
struct fb_fix_screeninfo fixScreen;
struct termios term;
int buffer_size;



/*
    Use System Call open, ioctl, mmap
*/

void init_graphics(){

    // Open's dev file
    devFile = open("/dev/fb0", O_RDWR);

    // Check if file can open
    if(devFile == -1){
        write(1, "Could not run open()\n", 23);
    }
    
    // Check the screen size and bits per pixel
    ioctl(devFile, FBIOGET_VSCREENINFO,&varScreen);
    ioctl(devFile, FBIOGET_FSCREENINFO,&fixScreen);
    
    buffer_size = varScreen.yres_virtual * fixScreen.line_length;
    // Memory mapping using mmap 
    frameBuffer = (color_t)mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, devFile, 0);


    write(STDIN_FILENO, "\033[2J", 8);

    ioctl(STDIN_FILENO, TCGETS, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    ioctl(STDIN_FILENO, TCSETS, &term);

}

/*
    Use System Call ioctl
*/

void exit_graphics(){

    write(STDIN_FILENO, "\033[2J", 8);
    ioctl(STDIN_FILENO, TCGETS, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    ioctl(STDIN_FILENO, TCSETS, &term);
    // Unmap the memory
    munmap(frameBuffer, buffer_size);

    // Close file
    close(devFile);
}

/*
    Use System Call select, read
    This will get which key is pressed and read it into the screen
*/

char getKey(){
    char key = 0;
    struct timeval value;
    int keyPress;
    fd_set kSet;

    FD_SET(0, &kSet);
    FD_ZERO(&kSet);

    value.tv_sec = 1;
    value.tv_usec = 0;
    keyPress = select(1, &kSet, NULL, NULL, &value);

    if(keyPress > 0){
        read(STDIN_FILENO, &key, sizeof(key));
    }
    // Return the value of the key 
    return key;

}

/*
    Use System Call nanosleep
*/

void sleep_ms(long ms){
    struct timespec nanoTime;
    
    nanoTime.tv_sec = 0;
    nanoTime.tv_nsec = ms * 1000000L;
    nanosleep(&nanoTime, NULL);

}

/*
    Loop through the buffer to clear the pixels off the screen
*/
void clear_screen(void *img){
    int i; // for loop variable
    color_t *mem = (color_t *)img;

    for(i = 0; i < buffer_size/2; i++){
        frameBuffer[i] = 0;
        mem[i] = 0;
    }

}

void draw_pixel(void *img, int x, int y, color_t color){
    int x_coord = fixScreen.line_length / 2;
    int pix_coord = (x + (y * x_coord)) * 2;
    color_t* pixel = img + (pix_coord);
    *pixel = color;

}

/*
    The code is based off of Coding Alpha's example. The code is modified for this method
    http://www.codingalpha.com/bresenham-line-drawing-algorithm-c-program/
*/
void draw_line(void *img, int x1, int y1, int x2, int y2, color_t c){
    int dx, dy, x, y, temp, t;

    if(x1 < x2){
        dx = x2 - x1;
        x = 1;
    }
    else{
        dx = x1 - x2;
        x = -1;
    }
    if(y1 < y2){
        dy = y2 - y1;
        y = 1;
    }
    else{
        dy = y1 - y2;
        y = -1;
    }
    if(dx > dy){
        temp = dx / 2;
    }
    else{
        temp = -dy / 2;
    }
    for(;;){
        draw_pixel(img, x1, y1, c);
        if(x1 == x2 && y1 == y2){
            break;
        }
        t = temp;
        if(t > -dx){
            temp -= dy;
            x1 += x;
        }
        if(t < dy){
            temp += dx;
            y1 += y;
        }

    }

}

/*
    Use System Call mmap
    This function returns mmap as if it was the function malloc()
*/
void *new_offscreen_buffer(){
    return mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void blit(void *src){
    int i; // for loop variable
    /*
        Copy's the bytes from the offscreen buffer
    */
    color_t *mem = (color_t *)src;

    /*
        Copy's the bytes from the offscreen buffer onto the frame buffer.
    */
    for(i = 0; i < buffer_size/2; i++){
        frameBuffer[i] = mem[i];
    } 
}




