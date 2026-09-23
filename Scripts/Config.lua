-- config.lua

-- WINDOW
window_title = "Ympd ECS prototype"
screen_width = 2000
screen_height = 2000
map_size =  Vector2D.new(5000.0, 5000.0)
map_movement_boundaries = 1.0
is_fullscreen = false

-- CAMERA
camera_start_pos =  Vector2D.new(0.0, 0.0)
camera_acceleration = 2000
camera_deceleration = 2000
camera_max_speed = 1000

-- FOLDERS
sprites_folder = "Sprites/";

-- DEBUG
display_lua_debug_messages = true;
is_text_debug = false;
is_input_text_debug = false;
display_stats = true;
stats_display_interval = 0.5;

-- COLLISIONS
cell_size = 32;
alpha_threshold = 20;