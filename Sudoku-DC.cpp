/**
 * Parallel and Distributed Systems: Paradigms and Models
 * Year 2019/2020
 * Final Project
 * Paoletti Riccardo
 * Student ID: 532143
*/
/*************************************************************************************/
/* This program implements a solution for the Sudoku problem using standard C++      */
/* threads.                                                                          */
/* The structure of the implementation relies on the Divide&Conquer programming      */
/* paradigm. The program starts with one thread that works on the initial schema.    */
/* If the program reached the requested depth level of the solution tree then the    */
/* schema is resolved sequentially by the single thread, otherwise, the solution     */
/* tree is explored but for every new discovered branch coming from the possible     */
/* values of a cell, a new thread is started. The new thread is given a copy of the  */
/* starting schema but with the specified cell value modified according to the       */
/* branch. The task of the thread will be to explore this new tree recursively.      */
/*                                                                                   */
/* The schemes emitted are selected from the initial schema, choosing the cell with  */
/* less possible assignable values and enumerating them all.                         */
/*                                                                                   */
/* Usage : <program_name> <par_degree> <board_index>                                 */
/* Where board_index=which board you want to be resolved in a [0-9] range taken      */
/* from the "input.txt" file, and par_degree=how many levels of the tree do you want */
/* to go down before reaching a base case.                                           */
/*************************************************************************************/
// #define PRINT_OVERHEAD = 1;
// #define PRINT_SOLUTION = 1;

#include "utils.cpp"
using namespace std;

std::mutex sol_mutex;
Cell** solution;

atomic_bool sol_found = false;

void setSolution(Cell** my_sol);
bool SolveSudoku(Cell** grid);
static inline void usage(const char *argv0);
void solve(Cell** grid, int par_degree, int tree_level);


int main(int argc, char *argv[])
{
    if (argc != 3)
        usage(argv[0]);

    vector<int**>* grids = new vector<int**>();
    readGrids(grids, "input.txt");

    int par_degree = atoi(argv[1]);
    int board_index = atoi(argv[2]);

    solution = new Cell*[N];
    for(int i=0;i<N;i++)
        solution[i] = new Cell[N];

    Cell** filledGrid = fillGrid((*grids)[board_index]);
	auto start = chrono::high_resolution_clock::now();
    solve(filledGrid, par_degree, 0);
	auto elapsed = chrono::high_resolution_clock::now() - start;
    auto usec = chrono::duration_cast<chrono::microseconds>(elapsed).count();
#ifdef PRINT_SOLUTION	
    if (sol_found)
        printGrid(solution); 
    else
	 	cout << "No solution exists\n"; 
#endif
	cout << "Execution took : " << usec << " usecs." << endl;
	return 0; 
}

void setSolution(Cell** my_sol)
{
    if(!sol_found){
        sol_found = true;
#ifdef PRINT_SOLUTION
        std::unique_lock<std::mutex> lock(sol_mutex);
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                solution[i][j].value = my_sol[i][j].value;
        lock.unlock();
#endif
    }
}

bool SolveSudoku(Cell** grid)
{
    int row, col;
    if(FindUnassignedMinimumLocation(grid, row, col))
    {
        for(int num=1;num<=N;num++)
        {
            if(isSafe(grid, row, col, num)){
                grid[row][col].value = num;
                calculatePossibleValues(grid);
                if(SolveSudoku(grid)) return true;
                grid[row][col].value = UNASSIGNED;
            }
        }
        return false;
    }
    setSolution(grid);
    return true;
}

static inline void usage(const char *argv0)
{
    printf("--------------------\n");
    printf("Usage: %s <par_degree> <board_index>\n", argv0);
    printf("--------------------\n");
    exit(-1);
}

void solve(Cell** grid, int par_degree, int tree_level)
{
    if(sol_found) return;
    if(par_degree==0 || tree_level == par_degree)
    {
#ifndef PRINT_OVERHEAD
        SolveSudoku(grid);
#else
        return;
#endif
    }else{
        std::vector<std::thread> tids;
        int row, col;
        RemoveSingletons(grid);
        bool find = FindUnassignedMinimumLocation(grid, row, col);
        for(int num : grid[row][col].possible_values){
            Cell** my_grid = new Cell *[N];
            for (int r = 0; r < N; r++)
            {
                my_grid[r] = new Cell[N];
                for (int c = 0; c < N; c++)
                    my_grid[r][c].value = grid[r][c].value;
            }
            my_grid[row][col].value = num;
            tids.push_back(thread(solve, my_grid, par_degree, tree_level+1));
        }
        for (std::thread &t : tids)
            t.join();
    }
    return;
}