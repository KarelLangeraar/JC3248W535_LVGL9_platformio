#ifndef STAR_ANIMATIONS_H
#define STAR_ANIMATIONS_H

#include <lvgl.h>

/**
 * @brief StarAnimations module - Manages twinkling and shooting star animations
 * 
 * This module encapsulates all logic for animating star objects on the main screen.
 * It handles:
 * - Twinkling stars with random fade in/out
 * - Shooting stars with linear motion trajectories
 * - Screen presence detection (pause animations when off main screen)
 * - Dynamic animation parameters (duration, scale, position, rotation)
 */

/**
 * @brief Initialize star animation system
 * 
 * Creates animation timers for twinkling and shooting stars.
 * Must be called after ui_init() when star objects are available.
 * 
 * @param starcontainer Container object holding star animations
 * @param star Template twinkling star animimg object
 * @param shooting_star Template shooting star animimg object
 * @param main_screen The main screen object (used to detect when we switch away)
 */
void StarAnimations_init(lv_obj_t *starcontainer, lv_obj_t *star, 
                         lv_obj_t *shooting_star, lv_obj_t *main_screen);

/**
 * @brief Cleanup star animation system
 * 
 * Stops all timers and cleans up animation resources.
 */
void StarAnimations_shutdown(void);

#endif // STAR_ANIMATIONS_H
