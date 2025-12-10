#include <iostream>
#include <dlfcn.h>
#include <vector>
#include <iomanip>
#include <algorithm>
#include "RobotBase.h"
#include "RadarObj.h"

//
// =========================================================
//  ARENA CONSTANTS
// =========================================================
//

static const int BOARD_ROWS = 20;
static const int BOARD_COLS = 20;

//
// =========================================================
//  WEAPON DAMAGE TABLE  (Professor style)
// =========================================================
//

int get_weapon_damage(WeaponType w)
{
    switch (w)
    {
        case flamethrower: return 5;
        case railgun:      return 12;
        case grenade:      return 20;
        case hammer:       return 8;
        default:           return 0;
    }
}

//
// =========================================================
//  SIMPLE SHOT CHECK
// =========================================================
//

bool shot_hits_robot(int shot_r, int shot_c, int robot_r, int robot_c)
{
    return (shot_r == robot_r && shot_c == robot_c);
}

//
// =========================================================
//  RADAR SCAN FUNCTION (8-direction line scan)
// =========================================================
//

std::vector<RadarObj> perform_radar_scan(int start_r, int start_c, int direction,
                                         const std::vector<RadarObj> &obstacles,
                                         int enemy_r, int enemy_c)
{
    std::vector<RadarObj> results;

    // movement vectors for 8 directions
    int dr[8] = {-1,-1,0,1,1,1,0,-1};
    int dc[8] = { 0, 1,1,1,0,-1,-1,-1};

    int r = start_r;
    int c = start_c;

    while (true)
    {
        r += dr[direction];
        c += dc[direction];

        if (r < 0 || c < 0 || r >= BOARD_ROWS || c >= BOARD_COLS)
            break;

        // enemy seen
        if (r == enemy_r && c == enemy_c)
        {
            results.push_back(RadarObj('R', r, c));
            break;
        }

        // obstacle check
        for (auto &ob : obstacles)
        {
            if (ob.m_row == r && ob.m_col == c)
            {
                results.push_back(ob);
                return results;
            }
        }
    }

    return results;
}

//
// =========================================================
//  ARENA DISPLAY (Professor style)
// =========================================================
//

void print_arena(int round,
                 int A_r, int A_c,
                 int B_r, int B_c,
                 const std::vector<RadarObj> &obstacles)
{
    std::cout << "=========== starting round " << round << " ===========\n   ";

    // column labels
    for (int c = 0; c < BOARD_COLS; c++)
        std::cout << std::setw(2) << c;
    std::cout << "\n";

    for (int r = 0; r < BOARD_ROWS; r++)
    {
        std::cout << std::setw(2) << r << " ";

        for (int c = 0; c < BOARD_COLS; c++)
        {
            char ch = '.';

            // obstacles
            for (auto &o : obstacles)
                if (o.m_row == r && o.m_col == c)
                    ch = o.m_type;

            // robots overwrite obstacles visually
            if (r == A_r && c == A_c) ch = 'A';
            if (r == B_r && c == B_c) ch = 'B';

            std::cout << " " << ch;
        }
        std::cout << "\n";
    }
}

//
// =========================================================
//  ROBOT LOADING
// =========================================================
//

RobotBase *load_robot(const char *path, void *&handle)
{
    handle = dlopen(path, RTLD_LAZY);
    if (!handle)
    {
        std::cerr << "dlopen error: " << dlerror() << "\n";
        return nullptr;
    }

    typedef RobotBase *(*CreateFn)();
    CreateFn create_robot = (CreateFn)dlsym(handle, "create_robot");

    if (!create_robot)
    {
        std::cerr << "ERROR: create_robot() not found in " << path << "\n";
        return nullptr;
    }

    return create_robot();
}

//
// =========================================================
//  MAIN ARENA LOOP
// =========================================================
//

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: ./RobotWarz robot1.so robot2.so\n";
        return 0;
    }

    // load robots
    void *h1 = nullptr, *h2 = nullptr;
    RobotBase *A = load_robot(argv[1], h1);
    RobotBase *B = load_robot(argv[2], h2);

    if (!A || !B) return -1;

    // boundaries
    A->set_boundaries(BOARD_ROWS, BOARD_COLS);
    B->set_boundaries(BOARD_ROWS, BOARD_COLS);

    // starting locations
    int A_r = 2,  A_c = 2;
    int B_r = 17, B_c = 14;

    A->move_to(A_r, A_c);
    B->move_to(B_r, B_c);

    // fixed arena obstacles
    std::vector<RadarObj> obstacles =
    {
        {'M',12,11},
        {'M',13,11},
        {'M',14,11},
        {'F', 6,14},
        {'P',10, 1}
    };

    //
    //  MAIN TURN LOOP
    //
    int round = 0;
    while (true)
    {
        print_arena(round, A_r, A_c, B_r, B_c, obstacles);

        //
        // ================= ROBOT A TURN =================
        //
        int scan_dir;
        A->get_radar_direction(scan_dir);
        auto radar_A = perform_radar_scan(A_r, A_c, scan_dir, obstacles, B_r, B_c);
        A->process_radar_results(radar_A);

        int shot_r, shot_c;
        if (A->get_shot_location(shot_r, shot_c))
        {
            std::cout << "A SHOOTS at (" << shot_r << "," << shot_c << ")\n";
            if (shot_hits_robot(shot_r, shot_c, B_r, B_c))
            {
                int dmg = get_weapon_damage(A->get_weapon());
                std::cout << "B IS HIT! Damage = " << dmg << "\n";
                B->take_damage(dmg);
            }
        }

        //
        // ================= ROBOT B TURN =================
        //
        B->get_radar_direction(scan_dir);
        auto radar_B = perform_radar_scan(B_r, B_c, scan_dir, obstacles, A_r, A_c);
        B->process_radar_results(radar_B);

        if (B->get_shot_location(shot_r, shot_c))
        {
            std::cout << "B SHOOTS at (" << shot_r << "," << shot_c << ")\n";
            if (shot_hits_robot(shot_r, shot_c, A_r, A_c))
            {
                int dmg = get_weapon_damage(B->get_weapon());
                std::cout << "A IS HIT! Damage = " << dmg << "\n";
                A->take_damage(dmg);
            }
        }

        //
        // ================= MOVEMENT =================
        //
        int move_dir, move_dist;
        int dr[8] = {-1,-1,0,1,1,1,0,-1};
        int dc[8] = { 0, 1,1,1,0,-1,-1,-1};

        // A moves
        A->get_move_direction(move_dir, move_dist);
        A_r = std::clamp(A_r + dr[move_dir] * move_dist, 0, BOARD_ROWS - 1);
        A_c = std::clamp(A_c + dc[move_dir] * move_dist, 0, BOARD_COLS - 1);
        A->move_to(A_r, A_c);

        // B moves
        B->get_move_direction(move_dir, move_dist);
        B_r = std::clamp(B_r + dr[move_dir] * move_dist, 0, BOARD_ROWS - 1);
        B_c = std::clamp(B_c + dc[move_dir] * move_dist, 0, BOARD_COLS - 1);
        B->move_to(B_r, B_c);

        //
        // ================= COLLISION =================
        //
        if (A_r == B_r && A_c == B_c)
        {
            std::cout << "COLLISION! Both robots take 1 damage.\n";
            A->take_damage(1);
            B->take_damage(1);
        }

        //
        // ================= WIN CHECK =================
        //
        if (A->get_health() <= 0)
        {
            std::cout << "\n===== B WINS! =====\n";
            break;
        }
        if (B->get_health() <= 0)
        {
            std::cout << "\n===== A WINS! =====\n";
            break;
        }

        round++;
    }

    dlclose(h1);
    dlclose(h2);
    return 0;
}
