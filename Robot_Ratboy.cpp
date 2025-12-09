#include "RobotBase.h"
#include <vector>
#include <iostream>
#include <algorithm>

class Robot_Ratboy : public RobotBase
{
private:
    bool m_moving_down = true;

    int to_shoot_row = -1;
    int to_shoot_col = -1;

    std::vector<RadarObj> known_obstacles;

    bool is_obstacle(int row, int col) const
    {
        return std::any_of(known_obstacles.begin(),
                           known_obstacles.end(),
                           [&](const RadarObj &obj)
                           { return obj.m_row == row && obj.m_col == col; });
    }

    void clear_target()
    {
        to_shoot_row = -1;
        to_shoot_col = -1;
    }

    void add_obstacle(const RadarObj &obj)
    {
        // FIXED WARNING: parentheses around OR chain
        if ((obj.m_type == 'M' || obj.m_type == 'P' || obj.m_type == 'F') &&
            !is_obstacle(obj.m_row, obj.m_col))
        {
            known_obstacles.push_back(obj);
        }
    }

public:
    Robot_Ratboy() : RobotBase(3, 4, railgun)
    {
        m_name = "Ratboy";
        m_character = 'R';
    }

    virtual void get_radar_direction(int &radar_direction) override
    {
        int r, c;
        get_current_location(r, c);

        radar_direction = (c > 0) ? 7 : 3; // left if not at wall, else right
    }

    virtual void process_radar_results(const std::vector<RadarObj> &results) override
    {
        clear_target();

        for (const auto &obj : results)
        {
            add_obstacle(obj);

            if (obj.m_type == 'R' && to_shoot_row == -1)
            {
                to_shoot_row = obj.m_row;
                to_shoot_col = obj.m_col;
            }
        }
    }

    virtual bool get_shot_location(int &row, int &col) override
    {
        if (to_shoot_row != -1)
        {
            row = to_shoot_row;
            col = to_shoot_col;
            clear_target();
            return true;
        }
        return false;
    }

    virtual void get_move_direction(int &move_direction, int &move_distance) override
    {
        int r, c;
        get_current_location(r, c);
        int mv = get_move_speed();

        // Sweep left
        if (c > 0)
        {
            move_direction = 7; // left
            move_distance = std::min(mv, c);
            return;
        }

        // Vertical sweep
        if (m_moving_down)
        {
            if (r + mv < m_board_row_max - 1)
            {
                move_direction = 5; // down
                move_distance = mv;
            }
            else
            {
                m_moving_down = false;
                move_direction = 1;
                move_distance = 1;
            }
        }
        else
        {
            if (r - mv >= 0)
            {
                move_direction = 1; // up
                move_distance = mv;
            }
            else
            {
                m_moving_down = true;
                move_direction = 5;
                move_distance = 1;
            }
        }
    }
};

extern "C" RobotBase *create_robot()
{
    return new Robot_Ratboy();
}
