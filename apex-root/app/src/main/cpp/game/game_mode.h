#ifndef APEX_ROOT_GAME_MODE_H
#define APEX_ROOT_GAME_MODE_H

namespace apex {
namespace game {

// Activate game mode with full anti-detection
bool enter_game_mode();

// Exit game mode, restore normal operation
bool exit_game_mode();

// Check if currently in game mode
bool is_in_game_mode();

// Get list of currently hidden processes
int get_hidden_process_count();

// Add a game to monitored list
bool add_game_to_monitor(const char* package_name);

// Remove a game from monitored list
bool remove_game_from_monitor(const char* package_name);

} // namespace game
} // namespace apex

#endif
