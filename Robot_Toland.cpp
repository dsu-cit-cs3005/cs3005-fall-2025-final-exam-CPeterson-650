#include "RobotBase.h"
#include <vector>
#include <iostream>
#include <algorithm>

//
//  Robot_Toland — The Sniper
//  • Railgun
//  • Minimal movement
//  • Scans one direction
//  • Fires instantly on detection
//

class Robot_Toland : public RobotBase
{
private:
    int m_target_row = -1;
    int m_target_col = -1;
    bool m_has_target = false;

    std::vector<RadarObj> m_obstacles; // mines, pits, fire

    bool is_obstacle(int r, int c) const
    {
        return std::any_of(
            m_obstacles.begin(),
            m_obstacles.end(),
            [&](const RadarObj &obj)
            {
                return obj.m_row == r && obj.m_col == c;
            });
    }

public:
    Robot_Toland() : RobotBase(2, 3, railgun)
    {
        m_name = "Toland";
        m_character = 'T';
    }

    // -----------------------------
    // Sniper Radar Direction
    // -----------------------------
    virtual void get_radar_direction(int &radar_direction) override
    {
        radar_direction = 3; // Look east (right)
    }

    // -----------------------------
    // Process Radar Results
    // -----------------------------
    virtual void process_radar_results(const std::vector<RadarObj> &results) override
    {
        m_has_target = false;
        m_target_row = -1;
        m_target_col = -1;

        for (const auto &obj : results)
        {
            // record obstacles
            if (obj.m_type == 'M' || obj.m_type == 'P' || obj.m_type == 'F')
            {
                if (!is_obstacle(obj.m_row, obj.m_col))
                    m_obstacles.push_back(obj);
            }

            // enemy robot
            if (obj.m_type == 'R' && !m_has_target)
            {
                m_target_row = obj.m_row;
                m_target_col = obj.m_col;
                m_has_target = true;
            }
        }
    }

    // -----------------------------
    // Shooting logic
    // -----------------------------
    virtual bool get_shot_location(int &shot_row, int &shot_col) override
    {
        if (m_has_target)
        {
            shot_row = m_target_row;
            shot_col = m_target_col;
            m_has_target = false;
            return true;
        }
        return false;
    }

    // -----------------------------
    // Sniper movement
    // -----------------------------
    virtual void get_move_direction(int &move_direction, int &move_distance) override
    {
        int r, c;
        get_current_location(r, c);

        move_direction = 0;
        move_distance = 0;

        // Dodge ONLY if there is an obstacle in front
        int ahead_r = r;
        int ahead_c = c + 1; // east

        if (is_obstacle(ahead_r, ahead_c))
        {
            move_direction = 1; // up
            move_distance = 1;
        }
    }
};

extern "C" RobotBase *create_robot()
{
    return new Robot_Toland();
}
