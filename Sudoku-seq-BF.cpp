/**
 * Parallel and Distributed Systems: Paradigms and Models
 * Year 2019/2020
 * Final Project
 * Paoletti Riccardo
 * Student ID: 532143
*/
/*************************************************************************************/
/* This program implements a sequential brute force resolution of a Sudoku schema.   */
/*                                                                                   */
/* Usage : <program_name> <board_index>                                              */
/* Where board_index=which board you want to be resolved in a [0-9] range taken      */
/* from the "input.txt" file.                                                        */
/*************************************************************************************/
// #define PRINT_SOLUTION = 1;

#include "utils.cpp"
#include "queue.cpp"
using namespace std; 

bool solve(Cell** &grid);
static inline void usage(const char *argv0);

int main(int argc, char* argv[]) 
{
    if (argc != 2)
        usage(argv[0]);

	vector<int**>* grids = new vector<int**>();
    readGrids(grids, "input.txt");
	
    Cell** filledGrid = fillGrid((*grids)[atoi(argv[1])]);
	auto start = chrono::high_resolution_clock::now();
    RemoveSingletons(filledGrid);
    auto result = solve(filledGrid);
	auto elapsed = chrono::high_resolution_clock::now() - start;
    auto usec = chrono::duration_cast<chrono::microseconds>(elapsed).count();
#ifdef PRINT_SOLUTION
	if (result) 
	 	printGrid(filledGrid); 
	else
	 	cout << "No solution exists\n"; 
#endif
	cout << "Execution took : " << usec << " usecs." << endl;
	return 0; 
}

static inline void usage(const char *argv0)
{
    printf("--------------------\n");
    printf("Usage: %s <board_index>\n", argv0);
    printf("--------------------\n");
    exit(-1);
}

bool solve(Cell** &grid){
    int row, col;
    if(FindUnassignedMinimumLocation(grid, row, col))
    {
        for(int num=1;num<=N;num++)
        {
            if(isSafe(grid, row, col, num)){
                grid[row][col].value = num;
                calculatePossibleValues(grid);
                if(solve(grid)) return true;
                grid[row][col].value = UNASSIGNED;
            }
        }
        return false;
    }
    return true;
}