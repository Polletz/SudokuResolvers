/**
 * Parallel and Distributed Systems: Paradigms and Models
 * Year 2019/2020
 * Final Project
 * Paoletti Riccardo
 * Student ID: 532143
*/
/*************************************************************************************/
/* This program implements a solution for the Sudoku problem using standard C++      */
/* threads and one blocking queue shared among all threads.                          */
/* The structure of the implementation is composed of one thread that pushes the     */
/* schemas to be resolved in the queue, and a pool of threads that pop the schemas   */
/* and  expand the solution tree of the schema, push in his own queue the branches   */
/* of the tree but keep one branch and recursively try to resolve it.                */
/*                                                                                   */
/* The emitted schemas are selected from the initial schema, choosing the cell with  */
/* less possible assignable values and enumerating them until a schema with that     */
/* value assigned has been emitted for each value.                                   */
/*                                                                                   */
/* WARNING : due to the nature of this implementation, for a schema with no solution */
/* the program does not end, the threads remain stuck popping something from the     */
/* queue that will never arrive. At the moment, I have no solution for this bug.     */
/*                                                                                   */
/* Usage : <program_name> <nw> <board_index>                                         */
/* Where nw=number of workers and board_index=which board you want to be resolved in */
/* a [0-9] range taken from the "input.txt" file.                                    */
/*************************************************************************************/
// #define PRINT_OVERHEAD = 1;
// #define PRINT_SOLUTION = 1;
// #define PRINT_TIMES = 1;

#include "utils.cpp"
#include "queue.cpp"
using namespace std;

syque<Cell **> task_queue;

vector<Cell **> *solutions;
vector<bool> *sols_found;

atomic_bool sol_found = false;

void setSolution(Cell **my_sol, int tid, int nw);
bool SolveSudoku(Cell **grid, int tid, int nw);
void threadBody(int tid, int nw);
static inline void usage(const char *argv0);
void solve(Cell **grid, int nw);
void sendEOF(int nw);

int main(int argc, char *argv[])
{
    if (argc != 3)
        usage(argv[0]);

    vector<int **> *grids = new vector<int **>();
    readGrids(grids, "input.txt");

    int nw = atoi(argv[1]);
    int board_index = atoi(argv[2]);

    solutions = new vector<Cell **>(nw);
    sols_found = new vector<bool>(nw);

    for (int i = 0; i < nw; i++)
    {
        (*solutions)[i] = new Cell *[N];
        for (int j = 0; j < N; j++)
            (*solutions)[i][j] = new Cell[N];
        sols_found->at(i) = false;
    }

    Cell **filledGrid = fillGrid((*grids)[board_index]);
    auto start = chrono::high_resolution_clock::now();
    solve(filledGrid, nw);
    auto elapsed = chrono::high_resolution_clock::now() - start;
    auto usec = chrono::duration_cast<chrono::microseconds>(elapsed).count();
#ifdef PRINT_SOLUTION    
    if (sol_found)
    {
        for (int i = 0; i < nw; i++)
            if ((*sols_found)[i])
            {
                printGrid(solutions->at(i));
                break;
            }
    }
    else
        cout << "No solution exists\n";
#endif

    cout << "Execution took : " << usec << " usecs." << endl;
    return 0;
}

void sendEOF(int nw){
    for(int i=0;i<nw;i++){
        Cell** c = NULL;
        task_queue.push(NULL);
    }
}

void setSolution(Cell **my_sol, int tid, int nw)
{
    sol_found = true;
#ifdef PRINT_SOLUTION
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            (*solutions)[tid][i][j].value = my_sol[i][j].value;

    (*sols_found)[tid] = true;
#endif
    sendEOF(nw);
}

bool SolveSudoku(Cell **grid, int tid, int nw)
{
    int row, col;
    if (FindUnassignedMinimumLocation(grid, row, col))
    {
        int size = grid[row][col].possible_values.size();
        if(size == 0) return false;
        for(int i=1; i<size; i++)
        {
            Cell** new_grid;
            copyCell(grid, new_grid);
            new_grid[row][col].value = grid[row][col].possible_values[i];
            calculatePossibleValues(new_grid);
            task_queue.push(new_grid);
        }
        grid[row][col].value = grid[row][col].possible_values[0];
        calculatePossibleValues(grid);
        return SolveSudoku(grid, tid, nw);
    }
    setSolution(grid, tid, nw);
    return true;
}

void threadBody(int tid, int nw)
{
#ifndef PRINT_OVERHEAD
    while(!sol_found){
        Cell** c = task_queue.pop();
        if(c==NULL) break;
        bool solved = SolveSudoku(c, tid, nw);
        if(solved){
            free(c);
            break;
        }
        free(c);
    }
#endif    
    return;
};

static inline void usage(const char *argv0)
{
    printf("--------------------\n");
    printf("Usage: %s <n_workers> <board_index>\n", argv0);
    printf("--------------------\n");
    exit(-1);
}

void solve(Cell **grid, int nw)
{
#ifdef PRINT_TIMES
    auto start_master = chrono::high_resolution_clock::now();
#endif
    RemoveSingletons(grid);
    vector<thread *> threadPool;
    int row, col;
    int k = 0;
    FindUnassignedMinimumLocation(grid, row, col);
    for (int j = 1; j <= N; j++)
    {
        if (isSafe(grid, row, col, j))
        {
            Cell** my_grid;
            copyCell(grid, my_grid);

            my_grid[row][col].value = j;
            calculatePossibleValues(my_grid);
            task_queue.push(my_grid);
            k++;
        }
    }

    for (int i = 0; i < nw; i++)
        threadPool.push_back(new std::thread(threadBody, i, nw));

#ifdef PRINT_TIMES
    auto elapsed_master = chrono::high_resolution_clock::now() - start_master;
    auto usec_master = chrono::duration_cast<chrono::microseconds>(elapsed_master).count();

    cout << "Master took : " << usec_master << " usecs." << endl;
#endif

#ifdef PRINT_TIMES
    auto start_worker = chrono::high_resolution_clock::now();
    for (thread *t : threadPool)
        t->join();
    auto elapsed_worker = chrono::high_resolution_clock::now() - start_worker;
    auto usec_worker = chrono::duration_cast<chrono::microseconds>(elapsed_worker).count();

    cout << "Workers took : " << usec_worker << " usecs." << endl;
#else
    for (thread *t : threadPool)
        t->join();
#endif

    return;
}