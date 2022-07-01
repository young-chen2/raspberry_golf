#include "gpio.h"
#include "gpio_extra.h"
#include "uart.h"
#include "gl.h"
#include "fb.h"
#include "font.h"
#include "strings.h"
#include "rand.h"
#include "printf.h"
#include "malloc.h"
#include "timer.h"
#include "mcp3008.h"
#include "bullet.h"
#include "golf.h"

/* 
 * Boxin Zhang, Yiyang (Young) Chen, March 10, 2022
 * This code would implement the mini-golf game on Raspberry Pi.
 * Main things to consider: lakes / golf / target & flag / obstacles
 */

#define AIM_ROTOR 3
#define MOVE_ROTOR 4

/* Basic parameters: screen size, radius of golf */
const int WIDTH_SCREEN = 640;
const int HEIGHT_SCREEN = 512;
const int RADIUS = 5;
const color_t LAKE_BLUE = 0x4BB6EF;
const color_t LIGHT_BLUE = 0xBFE1F4;
const color_t GRASS = 0x567d46;
const color_t LIGHT_GREEN = 0x7ec850;
const color_t LIGHT_GRASS = 0xB3D48E;
const color_t FLOWER = 0xE36B89;

/* Setup six lakes and three obstacles in golf game */
static lake_t lakes[3];
static obs_t obstacle[4];
static ball_t ball;
static goal_t goal;

/* Constants for dividing quadrants */
const static int Q1 = 255;
const static int Q2 = 511;
const static int Q3 = 767;
const static int Q4 = 1023;
int num_cycles = 0;

/* Initialize the ball at fixed position, velocity required */
void ball_init(int angle, int start_position){
    ball.x_pos = start_position;
    ball.y_pos = HEIGHT_SCREEN;
    ball.x_vel = 5;
    ball.y_vel = ball.x_vel * angle;
}

/* Initialize the lakes */
void lake_init(void) {     
    lakes[0].width = rand() % 10 + 25;   // Make sure lake no thinner than 20, no larger than 30
    lakes[0].height = rand() % 10 + 25;  
    lakes[0].x_pos = rand() % (WIDTH_SCREEN - lakes[0].width);    // Make sure goal not clipped on either side
    lakes[0].y_pos = rand() % (HEIGHT_SCREEN - lakes[0].height);
    
    while (1){
        lakes[1].width = rand() % 10 + 25;
        lakes[1].height = rand() % 10 + 25; 
        lakes[1].x_pos = rand() % (WIDTH_SCREEN - lakes[1].width);
        lakes[1].y_pos = rand() % (HEIGHT_SCREEN - lakes[1].height);
        if (dist_squared(lakes[0].x_pos, lakes[0].y_pos, lakes[1].x_pos, lakes[1].y_pos) >= 45){
            break;
        }
    }

    while (1){
        lakes[2].width = rand() % 10 + 25;
        lakes[2].height = rand() % 10 + 25; 
        lakes[2].x_pos = rand() % (WIDTH_SCREEN - lakes[2].width);
        lakes[2].y_pos = rand() % (HEIGHT_SCREEN - lakes[2].height);
        if (dist_squared(lakes[0].x_pos, lakes[0].y_pos, lakes[2].x_pos, lakes[2].y_pos) >= 45 &&
            dist_squared(lakes[1].x_pos, lakes[1].y_pos, lakes[2].x_pos, lakes[2].y_pos) >= 45){
            break;
        }
    }
}

void wall_init(void){
    /* Two vertical obstacles, one horizontal obstacle */

    obstacle[0].width = rand() % 5 + 35;   // Make sure obstacle no wider than 40, no shorter than 10
    obstacle[0].height = rand() % 50 + 200;  // Make sure obstacle height no taller than 400, no shorter than 100
    obstacle[0].x_start = rand() % 100 + 25;  // x between 25-115
    obstacle[0].y_start = rand() % 50;        // y between 0-50


    obstacle[1].width = rand() % 5 + 35;   // Make sure obstacle no wider than 40, no shorter than 10
    obstacle[1].height = rand() % 50 + 200;  // Make sure obstacle height no taller than 400, no shorter than 100
    obstacle[1].x_start = rand() % 100 + 115;  // x between 115-215
    obstacle[1].y_start = rand() % 50 + 350;   // y between 350-400


    obstacle[2].width = rand() % 50 + 200;   // Make sure obstacle no wider than 40, no shorter than 10
    obstacle[2].height = rand() % 5 + 35;  // Make sure obstacle height no taller than 400, no shorter than 100
    obstacle[2].x_start = rand() % 100 + 300;  // x between 300-400
    obstacle[2].y_start = rand() % 60 + 300;   // y between 300-360

    obstacle[3].width = rand() % 5 + 35;   // Make sure obstacle no wider than 40, no shorter than 10
    obstacle[3].height = rand() % 50 + 50;  // Make sure obstacle height no taller than 400, no shorter than 100
    obstacle[3].x_start = rand() % 100 + 430;  
    obstacle[3].y_start = rand() % 50 + 200;
}

/* Initialize the goal as square at rand pos */
void goal_init(void){        
    goal.width = 30;
    goal.height = 30;
    goal.x_pos = rand() % 440 + 140;
    goal.y_pos = rand() % 400 + 50;

    // goal.x_pos = rand() % (WIDTH_SCREEN - 2 * goal.width);    // Make sure goal not clipped on either side
    // goal.y_pos = rand() % (HEIGHT_SCREEN - 2 * goal.height);
}

void draw_line_radius(int x1, int y1, int x2, int y2, int radius) {
    return;
}

int get_ball_xvel(void) {
    return ball.x_vel;
}

int get_ball_yvel(void) {
    return ball.y_vel;
}

/*
   Permits users to use the rotor to modify the strength/velocity with which
   the billard ball is hit from 1 - 5; 
   */
int get_strength(void) {
    unsigned int level = mcp3008_read(AIM_ROTOR); // slope should range from 0-1023
    return level / 255 + 1; 
}

/*
   Permits users to use the rotor to move 360 degrees around their starting
   position to permit full-motion shooting
   */
void get_angle(void) {
    unsigned int pos = mcp3008_read(MOVE_ROTOR); // slope should range from 0-1023
    if (pos <= Q1) {
        //enable up to 6 angles in each quadrant
        ball.y_vel = pos / 42; 
        ball.x_vel = (Q1 - pos) / 42;
    }
    else if (pos > Q1 && pos <= Q2) {
        ball.x_vel = -1 * ((pos - Q1) / 42);
        ball.y_vel = (Q2 - pos) / 42; 
    }
    else if (pos > Q2 && pos <= Q3) {
        ball.y_vel = -1 * ((pos - Q2) / 42);
        ball.x_vel = -1 * ((Q3 - pos) / 42);
    }
    else if (pos > Q3 && pos <= Q4) {
        ball.x_vel = (pos - Q3) / 42; 
        ball.y_vel = -1 * ((Q4 - pos) / 42); 
    }
    ball.x_vel *= get_strength();
    ball.y_vel *= get_strength(); 
    gl_draw_line(ball.x_pos, ball.y_pos, 2 * ball.x_vel + ball.x_pos, ball.y_pos + 2 * ball.y_vel, GL_WHITE); //draws a line pointing in the direction of our ball ball
}

void draw_ball(void){
    gl_draw_circle(ball.x_pos, ball.y_pos, RADIUS, GL_WHITE);
    gl_swap_buffer();
    timer_delay_ms(3);
}

bool hit_lake(void){
    for (int i = 0; i < 4; i++){
        if (ball_within_rect(lakes[i].x_pos, lakes[i].y_pos, lakes[i].width, lakes[i].height)){
            return true;
        }
    }
    return false;
}

bool hit_goal(void){
    if (ball_within_rect(goal.x_pos, goal.y_pos, goal.width, goal.height)){
        return true;
    }
    return false;
}

bool ball_within_rect(int x, int y, int w, int h){
    if (ball.x_pos >= x && ball.x_pos <= x + w){
        if (ball.y_pos >= y && ball.y_pos <= y + h){
            return true;
        }
    }
    return false;
}

void gl_draw_circle(int x, int y, int r, color_t color){
    for (int i = x - r; i <= x + r; i++){
        for (int j = y - r; j <= y + r; j++){
            if ((i - x) * (i - x) + (j - y) * (j - y) <= (r * r)){
                gl_draw_pixel(i, j, color);
            }
        }
    }
}

void gl_draw_banner(int x, int y, int parity) {
    gl_draw_line(x, y, x, y - 50, GL_SILVER);
    if(parity == 0) {
        gl_draw_triangle(x, y - 25, x, y - 50, x + 30, y - 25, GL_YELLOW);
    }
    else {
        gl_draw_triangle(x, y - 25, x, y - 45, x + 25, y - 45, GL_YELLOW);
    }
}

void gl_draw_water(int x, int y, int w, int h, int parity) {
    for(int y_len = y; y_len < y + h; y_len++) {
        for(int x_len = x; x_len < x + w; x_len++) {
            if((x_len + y_len) % (5 + 3 * parity) == 0) {
                gl_draw_pixel(x_len, y_len, LIGHT_BLUE); 
            }
            else {
                gl_draw_pixel(x_len, y_len, LAKE_BLUE); 
            }
        }
    }
}

void gl_draw_lakes(int parity) {
    for (int i = 0; i < 3; i++) {
        gl_draw_water(lakes[i].x_pos, lakes[i].y_pos, lakes[i].width, lakes[i].height, parity);
    }
}

void gl_draw_grass(int parity)
{
    color_t (*im)[fb_get_pitch() / 4] = fb_get_draw_buffer();
    int per_row = fb_get_pitch() / 4;
    for(int y = 0; y < HEIGHT_SCREEN; y++) {
        for(int x = 0; x < per_row; x++) {
            if((x + y) % (3 + 3 * parity) == 0) {
                im[y][x] = LIGHT_GRASS;
            }
            else {
                im[y][x] = GRASS;
            }
        }
    }
}

void gl_draw_hedge(int x, int y, int w, int h, int parity) {
    for(int y_len = y; y_len < y + h; y_len++) {
        for(int x_len = x; x_len < x + w; x_len++) {
            if((x_len + y_len) % (7 + 2 * parity) == 0) {
                gl_draw_pixel(x_len, y_len, FLOWER); 
            }
            else {
                gl_draw_pixel(x_len, y_len, GRASS); 
            }
        }
    }
}

void hit_wall(void){
    /* If the ball hits any of the array of 
     * obstacles, then bounce back */
    for (int i = 0; i < 4; i++){
        if (ball_within_rect(obstacle[i].x_start, obstacle[i].y_start, obstacle[i].width, obstacle[i].height)){

            /* Check previous moment, four possibilities: 
             * 1. X out of bounds, Y inbounds
             * 2. X inbounds, Y out of bounds
             * 3. X out of bounds, Y out of bounds
             *    3.1 Decide which one the ball would hit first
             *    3.2 Act accordingly
             */

            int prev_y_pos = ball.y_pos - ball.y_vel;
            int prev_x_pos = ball.x_pos - ball.x_vel;

            /* Condition one: x out of bounds, y inbounds */
            if (!x_inbounds(prev_x_pos, obstacle[i].x_start, obstacle[i].x_start + obstacle[i].width) && 
                    y_inbounds(prev_y_pos, obstacle[i].y_start, obstacle[i].y_start + obstacle[i].height)){

                ball.x_vel = -(ball.x_vel);
            }

            /* Condition two: x inbounds, y out of bounds */
            else if (x_inbounds(prev_x_pos, obstacle[i].x_start, obstacle[i].x_start + obstacle[i].width) &&
                    !y_inbounds(prev_y_pos, obstacle[i].y_start, obstacle[i].y_start + obstacle[i].height)){

                ball.y_vel = -(ball.y_vel);
            }

            /* Condition three: x out of bounds, y out of bounds */
            else if (!x_inbounds(prev_x_pos, obstacle[i].x_start, obstacle[i].x_start + obstacle[i].width) &&
                    !y_inbounds(prev_y_pos, obstacle[i].y_start, obstacle[i].y_start + obstacle[i].height)){

                /* Possibility 1: Lower left corner */
                if (prev_x_pos < obstacle[i].x_start && prev_y_pos < obstacle[i].y_start){
                    if (hit_boundary(prev_x_pos, prev_y_pos, obstacle[i].x_start, obstacle[i].y_start)){
                        ball.x_vel = -(ball.x_vel);
                    }
                    else{
                        ball.y_vel = -(ball.y_vel);
                    }
                }
                /* Possibility 2: Upper left corner */
                if (prev_x_pos < obstacle[i].x_start && prev_y_pos > obstacle[i].y_start + obstacle[i].height){
                    if (hit_boundary(prev_x_pos, prev_y_pos, obstacle[i].x_start, obstacle[i].y_start + obstacle[i].height)){
                        ball.x_vel = -(ball.x_vel);
                    }
                    else{
                        ball.y_vel = -(ball.y_vel);
                    }
                }

                /* Possibility 3: Lower right corner */
                if (prev_x_pos > obstacle[i].x_start + obstacle[i].width && prev_y_pos < obstacle[i].y_start){
                    if (hit_boundary(prev_x_pos, prev_y_pos, obstacle[i].x_start + obstacle[i].width, obstacle[i].y_start)){
                        ball.x_vel = -(ball.x_vel);
                    }
                    else{
                        ball.y_vel = -(ball.y_vel);
                    }
                }

                /* Possibility 4: Upper right corner */
                if (prev_x_pos > obstacle[i].x_start + obstacle[i].width && prev_y_pos > obstacle[i].y_start + obstacle[i].height){
                    if (hit_boundary(prev_x_pos, prev_y_pos, obstacle[i].x_start + obstacle[i].width, obstacle[i].y_start + obstacle[i].height)){
                        ball.x_vel = -(ball.x_vel);
                    }
                    else{
                        ball.y_vel = -(ball.y_vel);
                    }
                }
            }
        }
    }
}

void move_ball(void){
    ball.x_pos += ball.x_vel;
    ball.y_pos += ball.y_vel;
    if (ball.x_pos < 0 || ball.x_pos > WIDTH_SCREEN){
        ball.x_vel = -(ball.x_vel);
        ball.x_pos += ball.x_vel;
    }
    else if (ball.y_pos < 0 || ball.y_pos > HEIGHT_SCREEN){
        ball.y_vel = -(ball.y_vel);
        ball.y_pos += ball.y_vel;
    }
    //friction
    if (num_cycles == 5) {
        if(ball.x_vel > 0) {
            ball.x_vel -= 1;
        }
        else if(ball.x_vel < 0) {
            ball.x_vel += 1;
        }
        if (ball.y_vel > 0) {
            ball.y_vel -= 1;
        }
        else if (ball.y_vel < 0) {
            ball.y_vel += 1;
        }
        num_cycles = 0;
    }
    num_cycles++;
}

void draw_field(int parity){
    gl_clear(LIGHT_GREEN);
    /* Draw out the obstacles and goal */
    for (int i = 0; i < 4; i++) {
        gl_draw_hedge(obstacle[i].x_start, obstacle[i].y_start, obstacle[i].width, obstacle[i].height, parity);
    }
    gl_draw_lakes(parity);
    gl_draw_rect(goal.x_pos, goal.y_pos, goal.width, goal.height, GL_CAYENNE);     // goal
    gl_draw_banner(goal.x_pos + (goal.width / 2), goal.y_pos + (goal.height / 2), parity);
}

bool hit_boundary(int prev_x, int prev_y, int sqr_x, int sqr_y){
    if (ball.y_vel * abs_val(prev_x - sqr_x) > ball.x_vel * abs_val(prev_y - sqr_y)){
        return true;
    }
    return false;
}

int abs_val(int val){
    if (val > 0){
        return val;
    }
    else{
        return -val;
    }
}

double dist_squared(int x1, int y1, int x2, int y2){
    int x_diff_squared = (x2 - x1) * (x2 - x1);
    int y_diff_squared = (y2 - y1) * (y2 - y1);
    return x_diff_squared + y_diff_squared;
}
