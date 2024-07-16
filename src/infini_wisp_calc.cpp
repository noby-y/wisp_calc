#include <stdint.h>
#include <array>
#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <numeric>
#include <queue>
#include <vector>
#include "D:/Programming projects/wisp_calc/lib/fort.c"
#include "D:/Programming projects/wisp_calc/lib/fort.hpp"

//* ==================================== Globals

constexpr int mod_count = 7;
int sol_count = 0;

std::array<int, mod_count> coef{-42, -30, 25, 50, 75, 80, 280};

std::map<std::string, int> config = {
    {"advanced_mode", 1},
    {"table_style", 0},
    {"reduce_lifetime", 1},
    {"chain_spell", 1},
    {"orbit_ping_pong", 1},
    {"spiral_arc", 1},
    {"incrase_lifetime", 1},
    {"phasing_true_orbit", 1},
    {"null_shot", 1}
};

// String to Int parsing 

bool tryParse(std::string& input, int& output){
    try{
        output = std::stoi(input);
    } catch (std::invalid_argument) {
        return false;
    }
    return true;
}

//* ==================================== CONFIG 


int parse_config(){
    std::ifstream cFile ("config.cfg");
    if (cFile.is_open())
    {
        std::string line;
        while(getline(cFile, line)){
            line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
            if(line[0] == '#' || line.empty()) 
                continue;
            
            auto delimiter = line.find("=");
            auto key = line.substr(0, delimiter);
            auto value = line.substr(delimiter + 1);
            
            int value_int;
            if (!tryParse(value, value_int)){
                std::cerr << "Invalid value type for " << key << " variable in config file - must be a NUMBER. Using default value.\n";
                continue;
            }
            if (value_int == 0 || value_int == 1){
                config[key] = value_int;
            }
            else {
                std::cerr << "Invalid value for " << key << " variable in config file. - must be 0 or 1. Using default value.\n";
            }
        }
    }
    else {
        std::cerr << "Couldn't open config file. Generating default config.\n";
        std::ofstream stream("config.cfg");
        if (!stream)
            return 1; // Error while opening file!

        // Create default config.cfg
        stream  <<  "#* This is a config file. You can adjust values to change how the solver behaves" <<  std::endl
                <<  std::endl
                <<  "# Accepted values are either 0 (No) or 1 (Yes). Invalid values will default to 1" <<  std::endl
                <<  "# Removed variables will default to 1. If you want to change that but don't remember the names, delete config.cfg file and the program will remake it " <<  std::endl
                <<  "# You can add comments in this file so long as they start with a '#'. Spaces are ignored" <<  std::endl
                <<  std::endl
                <<  "# Enable advanced mode with extra information" <<  std::endl
                <<  "advanced_mode=1" <<  std::endl
                <<  std::endl
                <<  "# Set table_style=1 to make pretty table" <<  std::endl
                <<  "# In case your terminal doesn't support utf-8 encoding (table_style=1 looks weird) refer to this guide: https://stackoverflow.com/questions/57131654/using-utf-8-encoding-chcp-65001-in-command-prompt-windows-powershell-window" <<  std::endl
                <<  "table_style=0" <<  std::endl
                <<  std::endl
                <<  "# Select which modifiers to allow when searching for a solution " <<  std::endl
                <<  "# WARNING! Removing certain modifier combinations makes it impossible to find certain solutions" <<  std::endl
                <<  "reduce_lifetime=1" <<  std::endl
                <<  "chain_spell=1" <<  std::endl
                <<  "orbit_ping_pong=1" <<  std::endl
                <<  "spiral_arc=1" <<  std::endl
                <<  "incrase_lifetime=1" <<  std::endl
                <<  "phasing_true_orbit=1" <<  std::endl
                <<  "null_shot=1";
    }
    return 0;
}

int apply_config(){
    // Apply config to modifiers
    coef[0] *= config["reduce_lifetime"];
    coef[1] *= config["chain_spell"];
    coef[2] *= config["orbit_ping_pong"];
    coef[3] *= config["spiral_arc"];
    coef[4] *= config["incrase_lifetime"];
    coef[5] *= config["phasing_true_orbit"];
    coef[6] *= config["null_shot"];

    return 0;
}

//* ==================================== BFS SOLUTION SEARCH 


struct Solution {
    int lhs;
    int mod_used;
    std::array<int, mod_count> mods;
};

// Make a list of solution elements
std::vector<Solution> solution_list;

bool compare_mod_used(const Solution& a, const Solution& b) {
    return a.mod_used < b.mod_used;
}

Solution bfs_solve(int mask, int l, int r){
    Solution init{};
    int low = std::numeric_limits<int>::max(), high = std::numeric_limits<int>::min();

    for (int i = 0; i < mod_count; ++i)
        if (mask & 1 << i)
        {
            init.lhs += coef[i];
            init.mods[i] = 1;
            low = std::min(low, r + 1 + coef[i]);
            high = std::max(high, l - 1 + coef[i]);
        }
    if (l <= init.lhs && init.lhs <= r)
    {
        return init;
    }

    low = std::min(low, init.lhs);
    high = std::max(high, init.lhs);
    
    std::queue<int> q;
    q.push(init.lhs - low);
    
    std::vector<int8_t> last_move(high - low + 1);
    last_move[init.lhs - low] = -1;
    
    int restore_from = 0;
    l -= low, r -= low;
    while (!q.empty())
    {
        int cur_val = q.front();
        q.pop();
        // #pragma GCC unroll 7
        for (int i = 0; i < mod_count; ++i)
            if (mask & 1 << i)
            {
                if (coef[i] < 0 ? cur_val < l : cur_val > r)
                {
                    continue;
                }
                int next_val = cur_val + coef[i];
                if (last_move[next_val])
                {
                    continue;
                }
                last_move[next_val] = i + 1;
                if (l <= next_val && next_val <= r)
                {
                    restore_from = next_val;
                    q = {};
                    break;
                }
                q.push(next_val);
            }
    }
    if (l > restore_from || restore_from > r)
    {
        return {l + low - 1, {}};
    }
    init.lhs = restore_from + low;
    while (last_move[restore_from] != -1)
    {
        ++init.mods[last_move[restore_from] - 1];
        restore_from -= coef[last_move[restore_from] - 1];
    }
    return init;
}

int loop_mask(int range_min, int range_max){
    for (int R = range_min; R <= range_max; ++R){
        for (int mask = 0; mask < 1 << mod_count; ++mask){
            if (auto sol = bfs_solve(mask, R, R); sol.lhs == R){
                bool skip = false;
                for(int i = 0; i < mod_count; ++i){
                    if (sol.mods[i] != 0 && coef[i] == 0){
                        skip = true;
                        break;
                    }
                }
                if (!skip){
                    sol_count += 1;
                    sol.mod_used = std::accumulate(sol.mods.begin(), sol.mods.end(), 0);    
                    solution_list.push_back(sol);
                }
            }
        }
    }
    return 0;
}

//* ==================================== TABLE FORMATTING

int set_table_style(fort::utf8_table& table){
    if (config["table_style"] == 0){
        table.set_border_style(FT_BASIC_STYLE);
    }
    else if(config["table_style"] == 1){
        table.set_border_style(FT_SOLID_STYLE);
    }
    return 0;
}

int set_table_header(fort::utf8_table& table){
    table << fort::header << "Mod count";
    if (coef[0] != 0) table << "RL";
    if (coef[1] != 0) table << "Chain";
    if (coef[2] != 0) table << "Orb/PP";
    if (coef[3] != 0) table << "Spir";
    if (coef[4] != 0) table << "IL";
    if (coef[5] != 0) table << "Phas/TO";
    if (coef[6] != 0) table << "Null";
    table << fort::endr;
    return 0;
}

int fill_table(fort::utf8_table& table){
    for (auto sol : solution_list){
        table << sol.mod_used;
        for (int i = 0; i < mod_count; ++i){
            table << sol.mods[i];
        }
        table << fort::endr;
    }
    for(int i = 0; i <= mod_count; ++i) 
        table.column(i).set_cell_text_align(fort::text_align::right);
    return 0;
}
//* ==================================== MAIN PROGRAM 


int main()
{
    parse_config();
    apply_config();
    int range_max = 0;
    int range_min = 0;

    // make a whule true loop
    while (true){
        std::cout << "Enter min lifetime: ";
        std::string input;
        getline(std::cin, input);
        while (!tryParse(input, range_max))
        {
            std::cout << "Bad input. Enter a NUMBER: ";
            getline(std::cin, input);
        }

        std::cout << "Enter max lifetime: ";
        std::string input2;
        getline(std::cin, input2);
        while (!tryParse(input2, range_min))
        {
            std::cout << "Bad input. Enter a NUMBER: ";
            getline(std::cin, input);
        }

        range_max = (range_max * -1) - 1;
        range_min = (range_min * -1) - 1;
        
        loop_mask(range_min, range_max);

        std::sort(solution_list.begin(), solution_list.end(), compare_mod_used);
        {
            fort::utf8_table table;

            set_table_style(table);
            set_table_header(table);
            fill_table(table);

            std::cout << table.to_string() << std::endl;
            std::cout << "Total solutions found: " << sol_count << '\n';
        }
        sol_count = 0;
        solution_list.clear();
        std::cout << "Close the program or make another input.\n";
        // getline(std::cin, input);
    }
}