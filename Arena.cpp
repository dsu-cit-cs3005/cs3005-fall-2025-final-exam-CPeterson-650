#include <iostream>
#include <dlfcn.h>
#include <vector>
#include <cmath>
#include "RobotBase.h"
#include "RadarObj.h"

static const int BOARD_ROWS = 20;
static const int BOARD_COLS = 20;

// --------------------------------------------------------
// Weapon damage helper
// --------------------------------------------------------
int weapon_damage(RobotBase* R) {
    switch (R->get_weapon()) {
        case flamethrower: return 4;
        case railgun:      return 3;
        case grenade:      return 5;
        case hammer:       return 2;
    }
    return 1;
}

// --------------------------------------------------------
// Simple hit check
// --------------------------------------------------------
bool shot_hits_robot(int sr, int sc, int rr, int rc) {
    return (sr == rr && sc == rc);
}

// --------------------------------------------------------
// Radar scanning
// --------------------------------------------------------
std::vector<RadarObj> perform_radar_scan(
        int start_row, int start_col, int direction,
        const std::vector<RadarObj>& obstacles,
        int enemy_row, int enemy_col)
{
    std::vector<RadarObj> results;

    int dr[8] = {-1,-1, 0, 1, 1, 1, 0,-1};
    int dc[8] = { 0, 1, 1, 1, 0,-1,-1,-1};

    int r = start_row;
    int c = start_col;

    while (true) {
        r += dr[direction];
        c += dc[direction];

        if (r < 0 || c < 0 || r >= BOARD_ROWS || c >= BOARD_COLS)
            break;

        if (r == enemy_row && c == enemy_col) {
            results.emplace_back('R', r, c);
            break;
        }

        for (auto &ob : obstacles) {
            if (ob.m_row == r && ob.m_col == c) {
                results.push_back(ob);
                return results;   // radar stops at obstacle
            }
        }
    }

    return results;
}

// --------------------------------------------------------
// Dynamic loading of robot SO
// --------------------------------------------------------
RobotBase* load_robot(const char* so_path, void*& handle_out) {
    handle_out = dlopen(so_path, RTLD_LAZY);
    if (!handle_out) {
        std::cerr << "Error loading " << so_path << ": " << dlerror() << "\n";
        return nullptr;
    }

    typedef RobotBase* (*create_fn)();
    create_fn create_robot = (create_fn)dlsym(handle_out, "create_robot");
    if (!create_robot) {
        std::cerr << "Error: create_robot not found in " << so_path << "\n";
        return nullptr;
    }
    return create_robot();
}

// --------------------------------------------------------
// Board Print
// --------------------------------------------------------

void print_board(int A_row, int A_col, char A_char,
                 int B_row, int B_col, char B_char,
                 const std::vector<RadarObj>& obstacles)
{
    // Create empty board
    char board[BOARD_ROWS][BOARD_COLS];

    for (int r = 0; r < BOARD_ROWS; r++)
        for (int c = 0; c < BOARD_COLS; c++)
            board[r][c] = '.';  // empty space

    // Place obstacles
    for (const auto& obj : obstacles)
    {
        board[obj.m_row][obj.m_col] = obj.m_type;
    }

    // Place robots
    board[A_row][A_col] = A_char;
    board[B_row][B_col] = B_char;

    // Print board
    std::cout << "\n=== ARENA ===\n";
    for (int r = 0; r < BOARD_ROWS; r++)
    {
        for (int c = 0; c < BOARD_COLS; c++)
            std::cout << board[r][c] << ' ';
        std::cout << '\n';
    }
    std::cout << '\n';
}


// --------------------------------------------------------
// MAIN ARENA SIMULATION
// --------------------------------------------------------
int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cout << "Usage: ./RobotWarz robot1.so robot2.so\n";
        return 0;
    }

    void* handle1 = nullptr;
    void* handle2 = nullptr;

    RobotBase* A = load_robot(argv[1], handle1);
    RobotBase* B = load_robot(argv[2], handle2);
    if (!A || !B) return -1;

    // board limits
    A->set_boundaries(BOARD_ROWS, BOARD_COLS);
    B->set_boundaries(BOARD_ROWS, BOARD_COLS);

    // starting positions
    int A_row = 2, A_col = 2;
    int B_row = 10, B_col = 10;

    A->move_to(A_row, A_col);
    B->move_to(B_row, B_col);

    // obstacles on board
    std::vector<RadarObj> obstacles = {
        {'M', 5, 5},
        {'P', 12, 8},
        {'F', 3, 15}
    };

    int turn = 0;

    while (true) {
        turn++;
        std::cout << "\n=== TURN " << turn << " ===\n";

        // -------- Radar: A scans --------
        int scan_dir;
        A->get_radar_direction(scan_dir);
        auto A_scan = perform_radar_scan(A_row, A_col, scan_dir, obstacles, B_row, B_col);
        A->process_radar_results(A_scan);

        // -------- Radar: B scans --------
        B->get_radar_direction(scan_dir);
        auto B_scan = perform_radar_scan(B_row, B_col, scan_dir, obstacles, A_row, A_col);
        B->process_radar_results(B_scan);

        // -------- A Shoots --------
        int shot_r, shot_c;
        if (A->get_shot_location(shot_r, shot_c)) {
            std::cout << "A fires at (" << shot_r << "," << shot_c << ")\n";
            if (shot_hits_robot(shot_r, shot_c, B_row, B_col)) {
                std::cout << "B is hit!\n";
                B->take_damage(weapon_damage(A));
            }
        }

        // -------- B Shoots --------
        if (B->get_shot_location(shot_r, shot_c)) {
            std::cout << "B fires at (" << shot_r << "," << shot_c << ")\n";
            if (shot_hits_robot(shot_r, shot_c, A_row, A_col)) {
                std::cout << "A is hit!\n";
                A->take_damage(weapon_damage(B));
            }
        }

        // Check death before movement
        if (A->get_armor() <= 0 || A->get_health() <= 0) {
            std::cout << "B WINS!\n"; break;
        }
        if (B->get_armor() <= 0 || B->get_health() <= 0) {
            std::cout << "A WINS!\n"; break;
        }

        // -------- Move A --------
        int move_dir, move_dist;
        A->get_move_direction(move_dir, move_dist);

        int dr[8] = {-1,-1, 0, 1, 1, 1, 0,-1};
        int dc[8] = { 0, 1, 1, 1, 0,-1,-1,-1};

        int newA_row = std::clamp(A_row + dr[move_dir] * move_dist, 0, BOARD_ROWS - 1);
        int newA_col = std::clamp(A_col + dc[move_dir] * move_dist, 0, BOARD_COLS - 1);

        A_row = newA_row;
        A_col = newA_col;
        A->move_to(A_row, A_col);

        // -------- Move B --------
        B->get_move_direction(move_dir, move_dist);

        int newB_row = std::clamp(B_row + dr[move_dir] * move_dist, 0, BOARD_ROWS - 1);
        int newB_col = std::clamp(B_col + dc[move_dir] * move_dist, 0, BOARD_COLS - 1);

        B_row = newB_row;
        B_col = newB_col;
        B->move_to(B_row, B_col);

        // Collision rule
        if (A_row == B_row && A_col == B_col) {
            std::cout << "Robots collide! Each loses 1 armor.\n";
            A->take_damage(1);
            B->take_damage(1);
        }

        // Final check
        if (A->get_health() <= 0) { std::cout << "B WINS!\n"; break; }
        if (B->get_health() <= 0) { std::cout << "A WINS!\n"; break; }
    }

    dlclose(handle1);
    dlclose(handle2);
    return 0;
}
