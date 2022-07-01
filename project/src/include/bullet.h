/*
 * 'move_bullet'
 *
 *  Move the bullet around the screen according to the elastic
 *  bouncing principle. Always starts from the bottom left corner
 *  (vertical position: HEIGHT, horizontal position: 0). Differs
 *  in trajectory according to angle and start position
 *
 */

void move_bullet(void);

/*
 * 'bullet_init'
 *
 *  Set up the initial params of the bullet, such as original pos,
 *  velocity, and the framebuffer on which we draw the bullet
 *
 *  @param angle: the angle of the bullet being shot
 *  @param start_position: the horizontal offset of shooter
 */

void bullet_init(int slope, int start_position);

/*
 * 'obstacle_init'
 *
 * Set up the initial params of the obstacles, a series of rectangles.
 *
 */
void obstacle_init();

/*
 * 'target_init'
 *
 * Set up the position and size of the rectangular target by putting
 * them into a struct
 *
 * @param x: x coordinate of bottom-left of rectangle
 * @param y: y coordinate of bottom-left of rectangle
 * @param w: width of rectangle
 * @param h: height of rectangle
 */
void target_init(void);

/* 'swap_velocities'
 *
 * Swaps y and x velocities by updating program-wide constants.
 */
void swap_velocities(void);

/* 'get_slope'
 *
 * Reads input from the MCP3008 / potentiometer to modify
 * the angle with which the bullet is shot. Draws a line / cursor in
 * the direction of the current shot trajectory.
 */
void get_slope(void);

/* 'get_movement'
 *
 * Reads input from the MCP3008 / potentiometer to modify
 * the position with which the bullet will be shot from (which is
 * in the left-most 200 pixels out of WIDTH).
 *
 */
void get_movement(void);

/* 'draw_bullet'
 *
 * Draw the bullet onto the framebuffer
 */

void draw_bullet(void);


/* 'hit_target'
 *
 * Check whether the bullet has hit the target or not
 */

bool hit_target(void);

/* 'draw_background'
 *
 * Redraw static background such as obstacles and target
 */

void draw_background(void);


/*  
 * 'hit_obstacle'
 *
 * Let the bullet reflect when hitting an obstacle
 */
void hit_obstacle(void);

/**
 * 'bullet_within_rect'
 * 
 * Evaluates whether a bullet is in the given rectangle
 * @param x: bottom-left corner of rect, x coord.
 * @param y: bottom-left corner of rect, y coord.
 * @param w: width of rect.
 * @param h: height of rect.
 */
bool bullet_within_rect(int x, int y, int w, int h);

/**
 * 'x_inbounds', 'y_inbounds'
 *
 * Check whether the x,y coordinate of the bullet is in bounds or not
 */
bool x_inbounds(int val, int x_min, int x_max);
bool y_inbounds(int val, int y_min, int y_max);


/**
 * 'hit_side'
 *
 * Return 1 if hit the vertical side; return 0 if hit the horizontal side
 * @param prev_x, prev_y: previous bullet position
 * @param sqr_x, sqr_y: nearest obstacle corner to the previous position
 */
bool hit_side(int prev_x, int prev_y, int sqr_x, int sqr_y);


/**
 * 'abs'
 * return absolute value
 */
int abs(int val);
