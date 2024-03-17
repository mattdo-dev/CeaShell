#ifndef RAW_MODE_H
#define RAW_MODE_H

/**
 * Disables raw mode for the terminal.
 * Restores the terminal's original settings stored in `global_termios`.
 */
void disable_raw_mode(void);

/**
 * Enables raw mode for the terminal.
 * Modifies the terminal settings to disable echoing, canonical mode,
 * and certain control signals.
 */
void enable_raw_mode(void);

#endif // RAW_MODE_H
