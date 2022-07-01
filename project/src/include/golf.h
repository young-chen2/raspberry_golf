/* Struct ball: Info on pos and vel */
typedef struct{
    int x_pos;
    int y_pos;
    int x_vel;
    int y_vel;
} ball_t;

/* Struct lake: Info on pos and size */
typedef struct{
    int x_pos;
    int y_pos;
    int width;
    int height;
} lake_t;

/* Struct obs: Info on pos and size */
typedef struct{
    int x_start;
    int y_start;
    int width;
    int height;
} obs_t;

/* Struct goal: Info on pos and size */
typedef struct{
    int x_pos;
    int y_pos;
    int width;
    int height;
} goal_t;

/*
 * 'gl_draw_circle'
 *  draw a circle of given origin and radius
 */
void gl_draw_circle(int x, int y, int r, color_t color);

/*
 * 'ball_init'
 *
 *  Set up the initial params of the billiard, such as original pos,
 *  velocity, and the framebuffer on which we draw the billiard.
 *
 */
void ball_init(int angle, int start_position);


/*
 * 'goal_init'
 *
 * Set up the position and size of the goal hole in minigulf round.
 */
void goal_init(void);

/*
 * 'lake_init'
 *
 * Set up the position and size of all lake-type obstacles.
 */
void lake_init(void);

/*
 * 'wall_init'
 *
 * Set up the position and size of all obstacles/walls within
 * the minigulf game.
 */
void wall_init(void);

/*
 * 'draw_line_radius'
 *  Keep the length of the drawn line constant, following the slope
 *  of (y2 - y1) / (x2 - x1)
 */
void draw_line_radius(int x1, int y1, int x2, int y2, int radius);


/* 'get_strength'
 *
 * Reads input from the MCP3008 / potentiometer to modify
 * the strength/momentum with which the ball will be shot from.
 */
int get_strength(void);


/* 'get_angle'
 *
 * Reads input from the MCP3008 / potentiometer to modify
 * the angle with which the ball is shot. Draws a line / cursor in
 * the direction of the current shot trajectory.
 */
void get_angle(void);

void move_ball(void);

/* 'draw_ball'
 *
 * Draw the ball onto the framebuffer
 */
void draw_ball(void);

/*
 * 'ball_within_rect'
 * 
 * Evaluates whether a ball is in the given rectangle
 * @param x: bottom-left corner of rect, x coord.
 * @param y: bottom-left corner of rect, y coord.
 * @param w: width of rect.
 * @param h: height of rect.
 * @param ball: the specified ball.
 */
bool ball_within_rect(int x, int y, int w, int h);

int get_ball_xvel(void);

int get_ball_yvel(void);

/*
 * 'hit_wall'
 *
 */
void hit_wall(void);

/*
 * 'hit_lake'
 */
bool hit_lake(void);

/* 'draw_field'
 *
 * Redraw static background such as obstacles and goal

 @ param: parity indicates the simple back-and-forth movement graphics of
 objects in the game. 1 indicates one state while 0 indicates the other.
 */
void draw_field(int parity);

/*
 * 'x_inbounds'
 *
 */
bool x_inbounds(int val, int x_min, int x_max);

/*
 * 'y_inbounds'
 *
 */
bool y_inbounds(int val, int y_min, int y_max);

/* 'hit_goal'
 *
 * Check whether the ball has hit the goal or not
 */
bool hit_goal(void);

/*
 * 'hit_boundary'
 *
 */
bool hit_boundary(int prev_x, int prev_y, int sqr_x, int sqr_y);

/**
 * 'abs'
 * return absolute value
 */
int abs_val(int val);

/**
 * @brief distance calculation
 * 
 * @param x1 x coord of first param
 * @param x2 x coord of second param
 * @param y1 y coord of first param
 * @param y2 y coord of second param 
 */
double dist_squared(int x1, int y1, int x2, int y2);
