// Edward Close - 201833689
// Here is the implementation of my game MAZE FILL, a puzzle game where the user must
// fully explore a grid styke maze in order to move onto the next level.
#include "Game_2.h"
#include "InputHandler.h"
#include "Joystick.h"
#include "Menu.h"
#include "LCD.h"
#include "PWM.h"
#include "Buzzer.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>

extern ST7789V2_cfg_t cfg0;
extern PWM_cfg_t pwm_cfg;      // LED PWM control
extern Buzzer_cfg_t buzzer_cfg; // Buzzer control
extern Joystick_cfg_t joystick_cfg;
extern Joystick_t joystick_data;

#define LCD_WIDTH  240
#define LCD_HEIGHT  240
#define GRID_ROWS 10
#define GRID_COLS 10
#define CELL_SIZE 24
#define LVL1_START_ROW 8
#define LVL1_START_COL 1
#define LVL2_START_ROW 4
#define LVL2_START_COL 1
#define LVL3_START_ROW 4
#define LVL3_START_COL 7
#define LAST_LEVEL 3
#define MOVE_DELAY_MS 60

// Functions
static void load_level(uint8_t current_level);
static void startup(void);
static void player_movement(UserInput input);
static void render(void);

// Player movement
static uint8_t moving = 0;
static int8_t row_change = 0;
static int8_t col_change = 0;
static uint32_t last_move_time = 0;
static int8_t player_row = 0;
static int8_t player_col = 0;
static int8_t next_row = 0;
static int8_t next_col = 0;

// Tile info
static uint8_t tiles_filled = 0;
static uint8_t total_tiles = 0;


// Level info
static uint8_t current_level = 0;
static char level_str[32];
static char score_str[32];
static const uint8_t LEVEL_1[10][10] = {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,0,1},
    {1,0,1,0,0,0,0,1,0,1},
    {1,0,1,0,1,1,0,1,0,1},
    {1,0,1,0,1,1,0,1,0,1},
    {1,0,1,0,1,0,0,1,0,1},
    {1,0,1,0,1,1,1,1,0,1},
    {1,0,1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1}
};
static const uint8_t LEVEL_2[10][10] = {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,1,0,0,1,0,1},
    {1,0,1,0,1,0,0,1,0,1},
    {1,1,1,0,0,0,1,0,0,1},
    {1,0,1,1,0,0,0,0,1,1},
    {1,0,1,1,1,1,0,0,0,1},
    {1,0,0,0,0,1,0,0,0,1},
    {1,0,1,0,0,0,0,1,1,1},
    {1,0,0,0,1,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1}
};
static const uint8_t LEVEL_3[10][10] = {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,1,0,0,0,1},
    {1,0,1,0,0,0,0,0,0,1},
    {1,0,1,0,0,0,0,1,1,1},
    {1,0,0,0,0,0,1,0,0,1},
    {1,0,0,0,0,0,1,0,0,1},
    {1,1,0,0,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,1,0,1},
    {1,0,0,0,1,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1}
};
static uint8_t MAZE[10][10];

// Flourish 
static uint8_t flourish = 0;
static uint8_t flourish_frame = 0;
static uint8_t player_size = 24;
static int8_t offset = 0;
static uint8_t transform = 0;


// Frame rate for this game (in milliseconds)
#define GAME2_FRAME_TIME_MS 30  // ~33 FPS

MenuState Game2_Run(void) {
    // Initialize game state
    current_level = 1;
    // Call the load_level function to load level 1 at startup
    load_level(current_level);

    // Call the startup function, this shows the title screen, plays the intro music and gives the user instructions on the game
    startup();
    
    MenuState exit_state = MENU_STATE_HOME;  // Default: return to menu
    
    // Game's own loop - runs until exit condition
    while (1) {
        uint32_t frame_start = HAL_GetTick();
        
        // READ:
        Input_Read();
        
        // Check if button was pressed to return to menu
        if (current_input.btn2_pressed) {
            // Show a loading screen before returning to main
            for(int i = 0; i < 3; i++){
                LCD_Fill_Buffer(0);
                LCD_printString("returning to main.", 5, 120, 1, 2);
                LCD_Refresh(&cfg0);
                HAL_Delay(500);

                LCD_Fill_Buffer(0);
                LCD_printString("returning to main..", 5, 120, 1, 2);
                LCD_Refresh(&cfg0);
                HAL_Delay(500);

                LCD_Fill_Buffer(0);
                LCD_printString("returning to main...", 5, 120, 1, 2);
                LCD_Refresh(&cfg0);
                HAL_Delay(500);
                }
            exit_state = MENU_STATE_HOME;
            break;  // Exit game loop
        }
        // Button 3 is used for my flourish effect, this pulses my character and inverts the colour of my character and its trail
        if (current_input.btn3_pressed) {
            flourish = 1; // Set flourish flag to true
            flourish_frame = 0; // Initialise the frame counter for the flourish feature
            
        }
        // Store the joystick hardware values in joystick_data
        Joystick_Read(&joystick_cfg, &joystick_data);
        // Convert the raw values into directional input
        UserInput input = Joystick_GetInput(&joystick_data);
        

        // UPDATE: Game logic
        
        // Call the player_movement function to move the character
        player_movement(input);

        // RENDER:

        // Call the render function to render to LCD
        render();
        
        // Frame timing - wait for remainder of frame time
        uint32_t frame_time = HAL_GetTick() - frame_start;
        if (frame_time < GAME2_FRAME_TIME_MS) {
            HAL_Delay(GAME2_FRAME_TIME_MS - frame_time);
        }

        // WIN CONDITION:

        // If all tiles filled, load next level
        if(tiles_filled == total_tiles){
            HAL_Delay(400); // Delay so the game doesnt instantly jump to this screen
            LCD_Fill_Buffer(0); 
            sprintf(level_str, "Level %d ", current_level); // Convert the current level into a string for display
            LCD_printString(level_str, 10, 80, 1, 4);
            LCD_printString("complete", 10, 120, 1, 4);
            LCD_Refresh(&cfg0);
            // Play a short melody to indicate a level has been completed
            uint16_t melody[] = {NOTE_C5, NOTE_G5};
                uint8_t duration[] = {8, 8,};
                uint16_t base_duration = 1000;
                for(int i = 0; i < 2; i++){
                    uint16_t note_length = base_duration / duration[i];

                    buzzer_note(&buzzer_cfg, melody[i], 50);

                    HAL_Delay(note_length);

                    buzzer_off(&buzzer_cfg);
                    HAL_Delay(50);
                };
            HAL_Delay(1500);

            current_level++; // Increment the current level

            if(current_level > LAST_LEVEL){ // If the next level doesnt exist (>3) show victory screen
                LCD_Fill_Buffer(0);
                
                LCD_printString("YOU WIN!",  20, 80, 1, 4);

                // Print smiley
                uint16_t smiley_x[] = {72, 144, 60, 84, 108, 132 };
                uint16_t smiley_y[] = {120, 120, 168, 192, 192, 192 };

                for(int i = 0; i < 6; i++){
                    LCD_Draw_Rect(smiley_x[i], smiley_y[i], CELL_SIZE, CELL_SIZE, 4, 1);
                    LCD_Draw_Rect(smiley_x[i], smiley_y[i], CELL_SIZE, CELL_SIZE, 0, 0);
                }
                LCD_Draw_Rect(156, 168, CELL_SIZE, CELL_SIZE, 2, 1);
                LCD_Refresh(&cfg0);
                uint16_t melody[] = {NOTE_C5, NOTE_D5, NOTE_E5, NOTE_G5, NOTE_E5, NOTE_G5};
                uint8_t duration[] = {8, 8, 8, 4, 8, 1 };
                uint16_t base_duration = 1000;
                // Play a victory tune
                for(int i = 0; i < 6; i++){
                    uint16_t note_length = base_duration / duration[i]; // Set note duration of current note i in the array

                    buzzer_note(&buzzer_cfg, melody[i], 50); // Play that note from the buzzer

                    HAL_Delay(note_length); // Play the note for the set note duration

                    buzzer_off(&buzzer_cfg); // Stop the buzzer
                    HAL_Delay(50); // Short rest
                };
                HAL_Delay(1000);
                // Show a loading menu before exiting to the menu screen
                for(int i = 0; i < 3; i++){
                LCD_Fill_Buffer(0);
                LCD_printString("returning to main.", 5, 120, 1, 2);
                LCD_Refresh(&cfg0);
                HAL_Delay(500);

                LCD_Fill_Buffer(0);
                LCD_printString("returning to main..", 5, 120, 1, 2);
                LCD_Refresh(&cfg0);
                HAL_Delay(500);

                LCD_Fill_Buffer(0);
                LCD_printString("returning to main...", 5, 120, 1, 2);
                LCD_Refresh(&cfg0);
                HAL_Delay(500);
                }
                exit_state = MENU_STATE_HOME;
                break;
            }
            else{
            load_level(current_level); // Load the next level to be completed
            }
        }
    }
    
    return exit_state;  // Tell main where to go next
}


// Function for loading the current level of my game
static void load_level(uint8_t current_level){
    // Initialise variables at the start of each level.
    tiles_filled = 0;
    total_tiles = 0;
    moving = 0;
    row_change = 0;
    col_change = 0;
    last_move_time = HAL_GetTick();
    // This switch statement loads up the corresponding level depending on the value of current_level
    switch(current_level){
        case 1:
            player_row = LVL1_START_ROW; // Set start row
            player_col = LVL1_START_COL; // Set start col

            // Copy the level into the maze array
            for(int i = 0; i < GRID_ROWS; i++){
                for(int j = 0; j < GRID_COLS; j++){
                    MAZE[i][j] = LEVEL_1[i][j]; 
                }
            }
            // Count the number of empty tiles in this level
            for(int i = 0; i < GRID_ROWS; i++){
                for(int j = 0; j < GRID_COLS; j++){
                    if(MAZE[i][j] ==  0){
                        total_tiles++;
                    }
                }
            }
            // Set the tile the character starts on as filled and increment the tiles_filled counter
            MAZE[LVL1_START_ROW][LVL1_START_COL] = 2;
            tiles_filled++;
            break;
        
        case 2:
            player_row = LVL2_START_ROW;
            player_col = LVL2_START_COL;
            for(int i = 0; i < GRID_ROWS; i++){
                for(int j = 0; j < GRID_COLS; j++){
                    MAZE[i][j] = LEVEL_2[i][j];
                }
            }
    
            for(int i = 0; i < GRID_ROWS; i++){
                for(int j = 0; j < GRID_COLS; j++){
                    if(MAZE[i][j] ==  0){
                        total_tiles++;
                    }
                }
            }
            MAZE[LVL2_START_ROW][LVL2_START_COL] = 2;
            tiles_filled++;
            break;
        
        case 3:
            player_row = LVL3_START_ROW;
            player_col = LVL3_START_COL;
            for(int i = 0; i < GRID_ROWS; i++){
                for(int j = 0; j < GRID_COLS; j++){
                    MAZE[i][j] = LEVEL_3[i][j];
                }
            }
    
            for(int i = 0; i < GRID_ROWS; i++){
                for(int j = 0; j < GRID_COLS; j++){
                    if(MAZE[i][j] ==  0){
                        total_tiles++;
                    }
                }
            }
            MAZE[LVL3_START_ROW][LVL3_START_COL] = 2;
            tiles_filled++;
            break;
    }

}
// Function for the startup menu animations
static void startup(void){
    // CLear screen
    LCD_Fill_Buffer(0);

    // Show start up screen
    LCD_printString("MAZE FILL",  15, 110, 1, 4);
    LCD_Refresh(&cfg0);
    
    // Play a tune upon starting the game
    
        // Arrays for note values and durations
        uint16_t melody[] = {NOTE_D4, NOTE_E4, NOTE_FS4, NOTE_G4, NOTE_FS4, NOTE_A4};
        uint8_t duration[] = {4, 8, 8, 4, 4, 1 };
        // One whole note = 1 second (60bpm)
        uint16_t base_duration = 1000;

        // Simple method for playing the melody using HAL_Delay and looping through the melody array
        for(int i = 0; i < 6; i++){
            uint16_t note_length = base_duration / duration[i]; // Set the note length for the current note in the array

            buzzer_note(&buzzer_cfg, melody[i], 50); // Play the current note i in the array

            HAL_Delay(note_length); // Play this note for its set duration

            buzzer_off(&buzzer_cfg); // Turn the buzzer off after the note length
            HAL_Delay(50); // Have a short rest in between notes for clarity
        };
        // After the melody has finished, this for loop iterates through drawing the maze character and its trail to create a menu animation
        for(int i = 0; i< 8; i++){
            // Clear screen
            LCD_Fill_Buffer(0);
            // Replace the maze fill title
            LCD_printString("MAZE FILL",  15, 110, 1, 4);
            // This loop draws the trail behind the character, by drawing squares in each position up until the character square
            for( int j = 0; j <= i; j++){
                LCD_Draw_Rect(15 + j * CELL_SIZE, 150, CELL_SIZE, CELL_SIZE, 4, 1 );
                LCD_Draw_Rect(15 + j * CELL_SIZE, 150, CELL_SIZE, CELL_SIZE, 0, 0 );
            }
            // Redraws the characters position further along with each loop 
            LCD_Draw_Rect(15 + i * CELL_SIZE, 150, CELL_SIZE, CELL_SIZE, 2, 1 );

            // Refresh the screen
            LCD_Refresh(&cfg0);
            // Delay the loop
            HAL_Delay(200);
        }
    

    // Instructions printed to the screen
    LCD_Fill_Buffer(0);
    LCD_printString("Explore the maze.", 25, 60, 1, 2);
    LCD_printString("Fill each tile", 25, 90, 1, 2);
    LCD_printString("to progress to", 25, 120, 1, 2);
    LCD_printString("the next level!", 25, 150, 1, 2);
    LCD_Refresh(&cfg0);
    HAL_Delay(4000);

    // Play a brief startup sound
    buzzer_tone(&buzzer_cfg, 1000, 30);  
    HAL_Delay(50);  
    buzzer_off(&buzzer_cfg);  
}
// Function for player movement
static void player_movement(UserInput input){

// The player can change the direction of the character when its stopped still
// the moving flag handles user input when its stationary, and handles its movement until colliding with a wall/barrier

        if (!moving){ // If the character is not already moving, set it into motion dpending on direction
           // These conditionals set how the rows and columns would change with each direction and set the moving flag to true
            if (input.direction == N){
                row_change = -1;
                col_change = 0;
                moving = 1; 
            }
            else if (input.direction == E){
                row_change = 0;
                col_change = 1;
                moving = 1;
            }
            else if (input.direction == S){
                row_change = 1;
                col_change = 0;
                moving = 1;
            }
            else if (input.direction == W){
                row_change = 0;
                col_change = -1;
                moving = 1;
            }
        }
        // If the player is currently moving and enough time has passed since it last moved square, the players movement updates.
        // this makes my characters movement animated rather than it jumping to the final valid square in the direction of movement.
        if (moving && HAL_GetTick() - last_move_time >= MOVE_DELAY_MS){
            last_move_time = HAL_GetTick();

            // Update the values of next row (the intended movement of the player)
            next_row = player_row + row_change;
            next_col = player_col + col_change;

            // If the intended movement is out of bounds, stop the characters movement
            if (next_row < 0 || next_row >= GRID_ROWS || next_col < 0 || next_col >= GRID_COLS){
                moving = 0;
            }
            // If the intended movement is into a wall, stop the characters movement
            else if(MAZE[next_row][next_col] == 1) {
                moving = 0;
            }
            // If the move is valid, update the players row and collumn to be rendered 
            else {
                
                player_row = next_row;
                player_col = next_col;

                if(MAZE[player_row][player_col] == 0){
                    MAZE[player_row][player_col] = 2;
                    tiles_filled++; // If the tile was empty, fill it
                    }
                
            }
        }
}
// Function for rendering to my LCD
static void render(void){
        // Set the background of the maze screen to white
        LCD_Fill_Buffer(1);

        //Draw maze

        for(int i = 0; i < GRID_ROWS; i++){
            for(int j = 0; j < GRID_COLS; j++){
                if(MAZE[i][j] == 1){ // If the cell in the maze array is a wall, draw a black square
                    LCD_Draw_Rect(CELL_SIZE * j, CELL_SIZE * i, CELL_SIZE, CELL_SIZE, 0, 1);
                }
                else if(MAZE[i][j] == 0){ // If the cell in the maze array is an open tile, draw a black outline over the white background
                    LCD_Draw_Rect(CELL_SIZE * j, CELL_SIZE * i, CELL_SIZE, CELL_SIZE, 0, 0);
                }
                else if(MAZE[i][j] == 2){ // If the cell in the maze array is a filled square, draw a coloured square and black outline
                    if(!transform){ // If the transform flag is 0, filled squares are their original colour, blue
                        LCD_Draw_Rect(CELL_SIZE * j, CELL_SIZE * i, CELL_SIZE, CELL_SIZE, 4, 1);
                        LCD_Draw_Rect(CELL_SIZE * j, CELL_SIZE * i, CELL_SIZE, CELL_SIZE, 0, 0);
                    }
                    else{ // If the transform flag is 1, the filled square colour is inverted and becomes red
                        LCD_Draw_Rect(CELL_SIZE * j, CELL_SIZE * i, CELL_SIZE, CELL_SIZE, 2, 1);
                        LCD_Draw_Rect(CELL_SIZE * j, CELL_SIZE * i, CELL_SIZE, CELL_SIZE, 0, 0);
                    }
                }
            }
        }
        if(flourish){ //If button 3 is pressed, flourish is set to 1
            flourish_frame++; // For each frame passed the frame counter is increased
            
            if(flourish_frame > 5){ // After 6 frames, reset the flourish state 
                flourish = 0;
                flourish_frame = 0;
                transform = !transform; // Toggle transform
                player_size = 24; // Return size and offset to normal
                offset = 0;
            }
            else{
                if(flourish_frame < 2){ // First shrink and offset the square
                    player_size = 18; 
                    offset = 3;
                }
                else if(flourish_frame < 4){ // Then enlarge and offset in the opposite direction
                    player_size = 30;
                    offset = - 3;
                }
                else{
                    player_size = 18; // Then shrink it again
                    offset = 3;
                }
            }
        }
        else{
            player_size = 24; // If flourish isnt active set player size and offset to original values
            offset = 0;
        }
        // Draw the player square at its current position, player colour changes with the transform toggle
        if(!transform){ 
        LCD_Draw_Rect(player_col * CELL_SIZE + offset, player_row * CELL_SIZE + offset, player_size, player_size, 2, 1);
        }
        else{
        LCD_Draw_Rect(player_col * CELL_SIZE + offset, player_row * CELL_SIZE + offset, player_size, player_size, 4, 1);
       
        }

        // Print the current level and score at the top of the LCD
        sprintf(level_str, "level: %d", current_level); // Convert the current level to displayable text for use in LCD_printString
        LCD_printString(level_str, 20, 10, 1, 1);
        sprintf(score_str, "score: %d/%d", tiles_filled, total_tiles); // Convert the current score to displayable text for use in LCD_printString
        LCD_printString(score_str, 120, 10, 1, 1);
        
        
        LCD_Refresh(&cfg0); // Refresh LCD
}