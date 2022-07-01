#include "assert.h"
#include "printf.h"
#include "strings.h"
#include "gl.h"
#include "bullet.h"
#include "golf.h"
#include "font.h"
#include "uart.h"
#include "timer.h"
#include "malloc.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "uart.h"
#include "mcp3008.h"
#include "button.h"
#include "shell.h"
#include "shell_commands.h"
#include "ps2.h"
#include "keyboard.h"

#define AIM_ROTOR 3
#define MOVE_ROTOR 4
static const int BUTTON = GPIO_PIN20;
static int HEIGHT = 512;
static int WIDTH = 640;
static int num_index = 0;

static int points = 0;
static int total_shots = 5;
static int MAX_OUTPUT_LEN = 100;
int parity = 0;
int parity_delay = 0;
char *leaderboard_names[5];
int leaderboard_scores[5];

void init_leaderboard(void) {
    for(int i = 0; i < 5; i++) {
        leaderboard_scores[i] = 0;
    }
}

void flip_parity(void) {
    if(parity == 0) {
        parity = 1;
    }
    else {
        parity = 0;
    }
}

void get_user_input_stage(void) {
    while (gpio_read(BUTTON) == 1) {
        draw_background(); // Draw background
        get_slope();
        get_movement();
        draw_bullet();     // Draw the bullet
    }
}

void get_golf_input_stage(void) {
    char str_buffer[MAX_OUTPUT_LEN];
    memset(str_buffer, '\0', MAX_OUTPUT_LEN);

    gl_clear(0xE36B89);
    snprintf(str_buffer, MAX_OUTPUT_LEN, "You have %d shots left this round", total_shots);
    gl_draw_string(100, HEIGHT / 2 - 20, str_buffer, GL_GREEN);
    gl_swap_buffer();
    timer_delay(2);  

    while (gpio_read(BUTTON) == 1) {
        if(parity_delay == 2) {
            parity_delay = 0;
            flip_parity();
        }
        else {
            parity_delay++;
        }
        draw_field(parity); // Draw field
        get_angle();
        draw_ball();     // Draw the ball
    }

    total_shots--;
}

void frame(void) {
    if(parity_delay == 2) {
        parity_delay = 0;
        flip_parity();
    }
    else {
        parity_delay++;
    }

    draw_field(parity);
    draw_ball();
    hit_wall();
    move_ball();
}

void test_bullet(void){
    bullet_init(5, 10);
    // gl_clear(GL_BLUE);
    while (1){
        draw_bullet();
        move_bullet();
    }
}

void test_hole_init(void){
    bullet_init(5, 0);
    target_init();
    while (1){
        draw_bullet();
        move_bullet();
    }
}

void test_obstacles(void){
    gl_init(640, 512, GL_DOUBLEBUFFER);
    bullet_init(2, 0);     // Using slope and start position, given by the rotor
    target_init();         // Using randomized algorithm
    obstacle_init();
    while (1){
        draw_background(); // Draw background
        draw_bullet();     // Draw the bullet
        hit_obstacle();
        move_bullet();     // Move the bullet
        if (hit_target()){
            points++;
            printf("Success! Now you have %d points\n", points); // print out success when hit target
            bullet_init(1, 20);
            target_init();
            obstacle_init();
        }
    }
}

void test_mcp3008(void) {
    gpio_init();
    uart_init();
    mcp3008_init();
    while (1) {
        // read channel 0
        printf("%d\n", mcp3008_read(AIM_ROTOR)); 
    }
}

void test_button(void) {
    while(gpio_read(BUTTON) == 1) { /* Spin */}
    printf("You've pressed the button!\n");
}

void test_potentiometer(void) {
    gpio_init();
    uart_init();
    mcp3008_init();

    gl_init(640, 512, GL_DOUBLEBUFFER);
    //SLOPE MUST BE INIT TO 1 FOR READ_SLOPE TO WORK
    bullet_init(2, 0);     // Using slope and start position, given by the rotor
    target_init();         // Using randomized algorithm
    obstacle_init();

    while (1) { //restarts new rounds
        get_user_input_stage();
        while (1) { //post-shooting stage
            draw_background(); // Draw background
            draw_bullet();     // Draw the bullet
            hit_obstacle();
            move_bullet();     // Move the bullet

            if (hit_target()){
                points++;
                printf("Success! Now you have %d points\n", points); // print out success when hit target
                bullet_init(2, 20);
                target_init();
                obstacle_init();
                break;
            }
        }
    }
}

void test_field_init(void){
    gl_init(640, 512, GL_DOUBLEBUFFER);
    lake_init();
    ball_init(5, 100);
    while (1){
        draw_field(parity);
        ball_init(5, 100);
    }
}

void test_golf_readings(void) {
    gpio_init();
    uart_init();
    mcp3008_init();

    gl_init(640, 512, GL_DOUBLEBUFFER);
    lake_init();
    goal_init();
    ball_init(5, 0);

    while(1) {
        draw_field(parity);
        draw_ball();
        get_angle();
    }
}

void test_golf(void) {
    gpio_init();
    uart_init();
    mcp3008_init();
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
    shell_init(keyboard_read_next, printf);

    bool stop_game_bit = 1;

    gl_init(640, 512, GL_DOUBLEBUFFER);
    init_leaderboard();
    ball_init(5, 0);
    lake_init();
    wall_init();
    goal_init();         // Using randomized algorithm

    //drawing tracker screen
    gl_clear(0xE36B89);
    gl_draw_string(180, HEIGHT / 2 - 20, "Ready, Set, Go!", GL_GREEN);
    gl_draw_string(100, HEIGHT / 2 + 20, "Type your name on the keyboard :)", GL_GREEN);
    gl_swap_buffer();
    timer_delay(5);

    char str_buffer[MAX_OUTPUT_LEN];
    memset(str_buffer, '\0', MAX_OUTPUT_LEN);

    while (stop_game_bit) {

        printf("\nTYPE YOUR NAME HERE: \n");
        // reading user input from the keyboard & storing it to history
        char line[30];
        char* line_ptr = malloc(30);
        shell_readline(line, sizeof(line));
        memset(line_ptr, '\0', 30);
        memcpy(line_ptr, line, strlen(line));
    
        while (stop_game_bit) { //restarts new golf hits
            get_golf_input_stage();

            while (stop_game_bit) { //post-shooting stage
                frame();

                if(total_shots < 0) {
                    stop_game_bit = 0;
                    break;
                }
                if (hit_lake()){
                    ball_init(5, 0);
                    break;
                }
                if(get_ball_xvel() == 0 && get_ball_yvel() ==0) {
                    break;
                }
                if (hit_goal()){
                    points++;
                    // if(points > high_score) {
                    //     high_score = points;
                    // }
                    total_shots = 5;

                    //drawing tracker screen
                    gl_clear(0xE36B89);
                    snprintf(str_buffer, MAX_OUTPUT_LEN, "Yay! You have %d point(s) :D", points);
                    gl_draw_string(150, HEIGHT / 2, str_buffer, GL_GREEN);
                    gl_swap_buffer();
                    timer_delay(5);

                    // printf("Success! Now you have %d points\n", points); // print out success when hit target
                    ball_init(5, 0);
                    lake_init();
                    wall_init();
                    goal_init();
                    break;
                }
            }
        }

        //storing to leaderboards if applicable
        leaderboard_names[num_index] = line_ptr;
        leaderboard_scores[num_index] = points;
        num_index++;

        //resets points
        points = 0;
        //drawing tracker screen
        gl_clear(GL_RED);
        gl_draw_string(220, HEIGHT / 2 - 20, "GAME OVER :/", GL_WHITE);
        gl_swap_buffer();
        timer_delay(5);

        //prints current scoreboard onto the terminal
        printf("\n++++++++++CURRENT LEADERBOARD!++++++++++ \n");
        for(int i = 0; i < 5; i++) {
            printf("%s has %d points\n", leaderboard_names[i], leaderboard_scores[i]);
        }

        stop_game_bit = 1;
        points = 0;
        total_shots = 5;

        //drawing tracker screen
        ball_init(5, 0);
        lake_init();
        wall_init();
        goal_init();         // Using randomized algorithm

        gl_clear(0xE36B89);
        gl_draw_string(180, HEIGHT / 2 - 20, "Ready, Set, Go!", GL_GREEN);
        gl_draw_string(100, HEIGHT / 2 + 20, "Type your name on the keyboard :)", GL_GREEN);
        gl_swap_buffer();
        timer_delay(5);
    }
}

void main(void){
    gpio_init();
    uart_init();    
    timer_init();
    printf("Executing main in project_test.c\n");

    gpio_set_input(BUTTON); // configure button
    gpio_set_pullup(BUTTON);
    
    // test_table_init();
    // test_golf_readings();
    test_golf();

    // test_bullet();
    // test_hole_init();
    // test_obstacles();
    // test_mcp3008();
    // test_button();
    // test_potentiometer();
    // test_reasonable_spacing();
    // test_table_init();

    printf("Completed main in project_test.c\n");
    uart_putchar(EOT);
}
