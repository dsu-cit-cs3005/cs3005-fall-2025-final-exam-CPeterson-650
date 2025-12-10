#include "RobotBase.h"
#include <vector>
#include <iostream>
#include <algorithm>

//
//  Robot_Toland — Advanced Sniper
//  • Uses railgun for long-range precision
//  • Slowly patrols when no target
//  • Moves only when necessary to dodge obstacles or reposition
//  • Locks onto the first robot detected
//

class Robot_Toland : public RobotBase
{
private:
    // Target information
    int m_target_row = -1;
    int m_target_col = -1;
    bool m_has_target = false;

    // Obstacle memory
    std::vector<RadarObj> m_obstacles;

    // Patrol direction: 1 = right, -1 = left
    int m_patrol_dir = 1;

    // Sniper rarely moves; move only if needed
    bool is_obstacle(int r, int c) const
    {
        return std::any_of(
            m_obstacles.begin(),
            m_obstacles.end(),
            [&](const RadarObj& obj) {
                return obj.m_row == r && obj.m_col == c;
            });
    }

public:
    // Toland: low movement, medium armor, railgun
    Robot_Toland() : RobotBase(2, 3, railgun) 
    {
        m_name = "Toland";
    }

    // ----------------------------------------
    // Radar direction (scan east)
    // ----------------------------------------
    virtual void get_radar_direction(int& radar_direction) override
    {
        radar_direction = 3;   // Look right (east)
    }

    // ----------------------------------------
    // Process radar findings
    // ----------------------------------------
    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        m_has_target = false;

        for (const auto& obj : radar_results)
        {
            // Save static obstacles
            if ((obj.m_type == 'M' || obj.m_type == 'P' || obj.m_type == 'F') &&
                !is_obstacle(obj.m_row, obj.m_col))
            {
                m_obstacles.push_back(obj);
            }

            // Acquire target
            if (obj.m_type == 'R' && !m_has_target)
            {
                m_target_row = obj.m_row;
                m_target_col = obj.m_col;
                m_has_target = true;
            }
        }
    }

    // ----------------------------------------
    // Shoot the locked target
    // ----------------------------------------
    virtual bool get_shot_location(int& shot_row, int& shot_col) override
    {
        if (m_has_target)
        {
            shot_row = m_target_row;
            shot_col = m_target_col;

            // Do NOT clear target. Keep tracking until radar loses it.
            return true;
        }
        return false;
    }

    // ----------------------------------------
    // Intelligent sniper movement
    // ----------------------------------------
    virtual void get_move_direction(int& move_direction, int& move_distance) override
    {
        int r, c;
        get_current_location(r, c);

        move_direction = 0;  // default: no movement
        move_distance = 0;

        // ------------------------------------------------
        // CASE 1 — Obstacle directly ahead (east)
        // ------------------------------------------------
        int ahead_r = r;
        int ahead_c = c + 1;

        if (is_obstacle(ahead_r, ahead_c))
        {
            move_direction = 1; // move up
            move_distance = 1;
            return;
        }

        // ------------------------------------------------
        // CASE 2 — Enemy targeted; reposition subtly
        // ------------------------------------------------
        if (m_has_target)
        {
            // If enemy is above, move up slightly
            if (m_target_row < r)
            {
                move_direction = 1;   // up
                move_distance = 1;
            }
            // If below, move down slightly
            else if (m_target_row > r)
            {
                move_direction = 5;   // down
                move_distance = 1;
            }
            else
            {
                // same row, no need to move
                move_distance = 0;
            }
            return;
        }

        // ------------------------------------------------
        // CASE 3 — No target: Patrol left ↔ right slowly
        // ------------------------------------------------
        if (c == 0) m_patrol_dir = 1;         // go right
        if (c == m_board_col_max - 1) m_patrol_dir = -1; // go left

        if (m_patrol_dir == 1)
        {
            move_direction = 3;  // right
            move_distance = 1;
        }
        else
        {
            move_direction = 7;  // left
            move_distance = 1;
        }
    }
};

// Factory function
extern "C" RobotBase* create_robot()
{
    return new Robot_Toland();
}
