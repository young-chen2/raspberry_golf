#include "gpio.h"
#include "gpio_extra.h"
#include "uart.h"
#include "gl.h"
#include "fb.h"
#include "font.h"
#include "strings.h"
#include "rand.h"
#include "printf.h"
#include "bullet.h"
#include "malloc.h"
#include "timer.h"
#include "mcp3008.h"

/* 
 * Boxin Zhang, Yiyang (Young) Chen, March 6, 2022
 * This code would provide basis for bullet-bounce
 * game on Raspberry Pi.
 */
#define AIM_ROTOR 3
#define MOVE_ROTOR 4
const int WIDTH_BULLET = 640;
const int HEIGHT_BULLET = 512;

typedef struct{
    int x_pos;
    int y_pos;
    int x_vel;
    int y_vel;
} bullet_t;

typedef struct{
    int x_pos;
    int y_pos;
    int width;
    int height;
} target_t;

typedef struct{
    int x_start;
    int y_start;
    int width;
    int height;
    color_t color;
} obs_t;

static target_t target;
static bullet_t bullet;
static obs_t obstacle[3];
unsigned int previous_slope = 0;

/* Inspired by 101computing.net, bouncing algorithm */


void bullet_init(int slope, int start_position){
    bullet.x_pos = start_position;
    bullet.y_pos = HEIGHT_BULLET;
    bullet.x_vel = 5;
    bullet.y_vel = bullet.x_vel * slope;
}

void target_init(void){        
    target.width = 50;
    target.height = 50;
    target.x_pos = rand() % (WIDTH_BULLET - 2 * target.width);    // Make sure target not clipped on either side
    target.y_pos = rand() % (HEIGHT_BULLET - 2 * target.height);
}

void swap_velocities(void) {
    unsigned int temp = bullet.x_vel;
    bullet.x_vel = bullet.y_vel;
    bullet.y_vel = temp;
}

/*
   Permits users to use the rotor to move the aim up to m = 5
   away from the horizontal slope.
   */
void get_slope(void) {
    unsigned int level = mcp3008_read(AIM_ROTOR); // slope should range from 0-1023
    level = level / 68; //permits 15 different initial angles
    if(level == 0) {
        bullet.x_vel = 3;
        bullet.y_vel = 24;
    }
    if(level == 1) {
        bullet.x_vel = 3;
        bullet.y_vel = 21;
    }
    if(level == 2) {
        bullet.x_vel = 3;
        bullet.y_vel = 18;
    }
    if(level == 3) {
        bullet.x_vel = 3;
        bullet.y_vel = 15;
    }
    if(level == 4) {
        bullet.x_vel = 3;
        bullet.y_vel = 12;
    }
    if(level == 5) {
        bullet.x_vel = 3;
        bullet.y_vel = 9;
    }
    if(level == 6) {
        bullet.x_vel = 3;
        bullet.y_vel = 6;
    }
    if(level == 7) {
        bullet.x_vel = 3;
        bullet.y_vel = 3;
    }
    if(level == 8) {
        bullet.x_vel = 6;
        bullet.y_vel = 3;
    }
    if(level == 9) {
        bullet.x_vel = 9;
        bullet.y_vel = 3;
    }
    if(level == 10) {
        bullet.x_vel = 12;
        bullet.y_vel = 3;
    }
    if(level == 11) {
        bullet.x_vel = 15;
        bullet.y_vel = 3;
    }
    if(level == 12) {
        bullet.x_vel = 18;
        bullet.y_vel = 3;
    }
    if(level == 13) {
        bullet.x_vel = 21;
        bullet.y_vel = 3;
    }
    if(level == 14) {
        bullet.x_vel = 24;
        bullet.y_vel = 3;
    }
    if(level == 15) {
        bullet.x_vel = 27;
        bullet.y_vel = 3;
    }
    gl_draw_line(bullet.x_pos, HEIGHT_BULLET, bullet.x_vel * 20 + bullet.x_pos, HEIGHT_BULLET - bullet.y_vel * 20, GL_WHITE); //draws a line pointing in the direction of our bullet
    printf("Current y_vel: %d, x_vel: %d, slope: %d\n", bullet.y_vel, bullet.x_vel, level);
}

/*
   Permits users to use the rotor to move up to 200 pixels away from
   the starting position.
   */
void get_movement(void) {
    unsigned int pos = mcp3008_read(MOVE_ROTOR); // slope should range from 0-1023
    if(pos >= 1000) {
        bullet.x_pos = 200;
    }
    else {
        bullet.x_pos = pos / 5;
    }
    // printf("Current x_pos: %d, Read: %d\n", bullet.x_pos, pos);
}

void move_bullet(void){
    bullet.x_pos += bullet.x_vel;
    bullet.y_pos += bullet.y_vel;
    if (bullet.x_pos < 0 || bullet.x_pos > WIDTH_BULLET){
        bullet.x_vel = -(bullet.x_vel);
        bullet.x_pos += bullet.x_vel;
    }
    if (bullet.y_pos < 0 || bullet.y_pos > HEIGHT_BULLET){
        bullet.y_vel = -(bullet.y_vel);
        bullet.y_pos += bullet.y_vel;
    }
}

void hit_obstacle(void){
    /* If the bullet hits any of the array of 
     * obstacles, then bounce back */
    for (int i = 0; i < 3; i++){
        if (bullet_within_rect(obstacle[i].x_start, obstacle[i].y_start, obstacle[i].width, obstacle[i].height)){

            /* Check previous moment, four possibilities: 
             * 1. X out of bounds, Y inbounds
             * 2. X inbounds, Y out of bounds
             * 3. X out of bounds, Y out of bounds
             *    3.1 Decide which one the bullet would hit first
             *    3.2 Act accordingly
             */

            int prev_y_pos = bullet.y_pos - bullet.y_vel;
            int prev_x_pos = bullet.x_pos - bullet.x_vel;

            /* Condition one: x out of bounds, y inbounds */
            if (!x_inbounds(prev_x_pos, obstacle[i].x_start, obstacle[i].x_start + obstacle[i].width) && 
                    y_inbounds(prev_y_pos, obstacle[i].y_start, obstacle[i].y_start + obstacle[i].height)){

                bullet.x_vel = -(bullet.x_vel);
            }

            /* Condition two: x inbounds, y out of bounds */
            else if (x_inbounds(prev_x_pos, obstacle[i].x_start, obstacle[i].x_start + obstacle[i].width) &&
                    !y_inbounds(prev_y_pos, obstacle[i].y_start, obstacle[i].y_start + obstacle[i].height)){

                bullet.y_vel = -(bullet.y_vel);
            }

            /* Condition three: x out of bounds, y out of bounds */
            else if (!x_inbounds(prev_x_pos, obstacle[i].x_start, obstacle[i].x_start + obstacle[i].width) &&
                    !y_inbounds(prev_y_pos, obstacle[i].y_start, obstacle[i].y_start + obstacle[i].height)){

                /* Possibility 1: Lower left corner */
                if (prev_x_pos < obstacle[i].x_start && prev_y_pos < obstacle[i].y_start){
                    if (hit_side(prev_x_pos, prev_y_pos, obstacle[i].x_start, obstacle[i].y_start)){
                        bullet.x_vel = -(bullet.x_vel);
                    }
                    else{
                        bullet.y_vel = -(bullet.y_vel);
                    }
                }
                /* Possibility 2: Upper left corner */
                if (prev_x_pos < obstacle[i].x_start && prev_y_pos > obstacle[i].y_start + obstacle[i].height){
                    if (hit_side(prev_x_pos, prev_y_pos, obstacle[i].x_start, obstacle[i].y_start + obstacle[i].height)){
                        bullet.x_vel = -(bullet.x_vel);
                    }
                    else{
                        bullet.y_vel = -(bullet.y_vel);
                    }
                }

                /* Possibility 3: Lower right corner */
                if (prev_x_pos > obstacle[i].x_start + obstacle[i].width && prev_y_pos < obstacle[i].y_start){
                    if (hit_side(prev_x_pos, prev_y_pos, obstacle[i].x_start + obstacle[i].width, obstacle[i].y_start)){
                        bullet.x_vel = -(bullet.x_vel);
                    }
                    else{
                        bullet.y_vel = -(bullet.y_vel);
                    }
                }

                /* Possibility 4: Upper right corner */
                if (prev_x_pos > obstacle[i].x_start + obstacle[i].width && prev_y_pos > obstacle[i].y_start + obstacle[i].height){
                    if (hit_side(prev_x_pos, prev_y_pos, obstacle[i].x_start + obstacle[i].width, obstacle[i].y_start + obstacle[i].height)){
                        bullet.x_vel = -(bullet.x_vel);
                    }
                    else{
                        bullet.y_vel = -(bullet.y_vel);
                    }
                }
            }
        }
    }
}
void obstacle_init(void){
    for (int i = 0; i < 3; i++){
        obstacle[i].width = rand() % 10 + 25;   // Make sure obstacle no wider than 40, no shorter than 10
        obstacle[i].height = rand() % 200 + 200;  // Make sure obstacle height no taller than 400, no shorter than 100
        obstacle[i].x_start = rand() % 100 + 150 * (i+1);
        obstacle[i].y_start = rand() % ((HEIGHT_BULLET - obstacle[i].height) / 3);
        obstacle[i].color = GL_RED;
    }
}

void draw_background(void){
    gl_clear(GL_BLUE);
    /* Draw out the obstacles and target */
    for (int i = 0; i < 3; i++){
        gl_draw_rect(obstacle[i].x_start, obstacle[i].y_start, obstacle[i].width, obstacle[i].height, obstacle[i].color);
    }
    gl_draw_rect(target.x_pos, target.y_pos, target.width, target.height, GL_YELLOW);     // target
}


void draw_bullet(void){
    gl_draw_rect(bullet.x_pos - 2, bullet.y_pos - 2, 5, 5, GL_WHITE);
    // gl_draw_pixel(bullet.x_pos, bullet.y_pos, GL_WHITE);
    gl_swap_buffer();
    timer_delay_ms(3);
}

bool x_inbounds(int val, int x_min, int x_max){
    return (val >= x_min && val <= x_max);
}

bool y_inbounds(int val, int y_min, int y_max){
    return (val >= y_min && val <= y_max);
}

bool hit_target(void){
    return bullet_within_rect(target.x_pos, target.y_pos, target.width, target.height);
}

bool bullet_within_rect(int x, int y, int w, int h){
    if (bullet.x_pos >= x && bullet.x_pos <= x + w){
        if (bullet.y_pos >= y && bullet.y_pos <= y + h){
            return true;
        }
    }
    return false;
}

bool hit_side(int prev_x, int prev_y, int sqr_x, int sqr_y){
    if (bullet.y_vel * abs(prev_x - sqr_x) > bullet.x_vel * abs(prev_y - sqr_y)){
        return true;
    }
    return false;
}

int abs(int val){
    if (val > 0){
        return val;
    }
    else{
        return -val;
    }
}
