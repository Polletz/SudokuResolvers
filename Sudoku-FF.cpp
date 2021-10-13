/**
 * Parallel and Distributed Systems: Paradigms and Models
 * Year 2019/2020
 * Final Project
 * Paoletti Riccardo
 * Student ID: 532143
*/
/*************************************************************************************/
/* This program implements a solution for the Sudoku problem using FastFlow library. */
/* The structure of the implementation is composed of one Master that sends the      */
/* schemes to be resolved, and a farm of Workers that receives the schemes, open     */
/* the solution tree of that scheme, keep a branch for itself to be explored and     */
/* stores the other branches in a vector. Than either the explored branch leads to   */
/* a solution, or the other branches are sent to the Master to be distributed to the */
/* other Workers to be recursively explored themselves. The Worker than restart to   */
/* wait for schemes sent by the Master.                                              */
/*                                                                                   */
/* The schemes emitted are selected from the initial schema, choosing the cell with  */
/* less possible assignable values and enumerating them until a schema with that     */
/* value assigned has been emitted for each value.                                   */
/*                                                                                   */
/* Usage : <program_name> <nw> <board_index>                                         */
/* Where nw=number of workers and board_index=which board you want to be resolved in */
/* a [0-9] range taken from the "input.txt" file.                                    */
/*************************************************************************************/
// #define PRINT_OVERHEAD = 1;
// #define PRINT_SOLUTION = 1;
// #define PRINT_TIMES = 1;

#include "utils.cpp"
#include <ff/ff.hpp>

using namespace ff;

std::vector<Cell **> *solutions;
std::vector<bool> *sols_found;

std::atomic_bool sol_found = false;

static inline void usage(const char *argv0);

struct W : ff_node
{
    void *svc(void *task)
    {
#ifndef PRINT_OVERHEAD
        Cell **grid = (Cell **)task;
        std::vector<Cell **> *tasks = SolveSudoku(grid, get_my_id());
        return tasks;
#else
        return (new std::vector<Cell**>());
#endif
    }

    std::vector<Cell **> *SolveSudoku(Cell **grid, int tid)
    {
        std::vector<Cell **> *tasks = new std::vector<Cell **>();
        int row, col;
        while (FindUnassignedMinimumLocation(grid, row, col))
        {
            int size = grid[row][col].possible_values.size();
            if (size == 0)
                return tasks;
            for (int i = 1; i < size; i++)
            {
                Cell **new_grid;
                copyCell(grid, new_grid);
                new_grid[row][col].value = grid[row][col].possible_values[i];
                calculatePossibleValues(new_grid);
                tasks->push_back(new_grid);
            }
            grid[row][col].value = grid[row][col].possible_values[0];
            calculatePossibleValues(grid);
        }
        setSolution(grid, tid);
        free(grid);
        return tasks;
    }

    void setSolution(Cell **my_sol, int tid)
    {
        sol_found = true;
#ifdef PRINT_SOLUTION
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                (*solutions)[tid][i][j].value = my_sol[i][j].value;

        (*sols_found)[tid] = true;
#endif
    }

};

class E : public ff_node_t<std::vector<Cell **>, long>
{
public:
    E(Cell **grid) : grid(grid) {}
    long *svc(std::vector<Cell **> *task)
    {
#ifdef PRINT_TIMES
        auto start = chrono::high_resolution_clock::now();
#endif
        if (task == nullptr)
        {

            EmitTasks();
#ifdef PRINT_TIMES
        elapsed += chrono::high_resolution_clock::now() - start;
#endif
            return GO_ON;
        }
#ifndef PRINT_OVERHEAD
        for (Cell **c : *task)
        {
            ff_send_out(c);
            numtasks++;
        }
        free(task);

        if (--numtasks == 0 || sol_found){
#ifdef PRINT_TIMES
            elapsed += chrono::high_resolution_clock::now() - start;
            auto usec = chrono::duration_cast<chrono::microseconds>(elapsed).count();
            cout << "Master took : " << usec << " usecs." << endl;
#endif
            return EOS;
        }
#ifdef PRINT_TIMES
        elapsed += chrono::high_resolution_clock::now() - start;
#endif
        return GO_ON;
#else
        return EOS;
#endif
    }

    void EmitTasks()
    {
        RemoveSingletons(grid);
        int row, col;
        FindUnassignedMinimumLocation(grid, row, col);
        for (int j = 1; j <= N; j++)
        {
            if (isSafe(grid, row, col, j))
            {
                Cell **my_grid = new Cell *[N];
                for (int r = 0; r < N; r++)
                {
                    my_grid[r] = new Cell[N];
                    for (int c = 0; c < N; c++)
                        if (grid[r][c].value == WORKING_ON)
                            my_grid[r][c].value = 0;
                        else
                            my_grid[r][c].value = grid[r][c].value;
                }
                my_grid[row][col].value = j;
                calculatePossibleValues(my_grid);
                ff_send_out(my_grid);
                numtasks++;
            }
        }
    }

private:
    Cell **grid;
    long numtasks = 0;
    long num_times = 0;
    std::chrono::nanoseconds elapsed = std::chrono::nanoseconds(0);
};

int main(int argc, char *argv[])
{
    if (argc != 3)
        usage(argv[0]);

    std::vector<int **> *grids = new std::vector<int **>();
    readGrids(grids, "input.txt");

    int nw = atoi(argv[1]);
    int board_index = atoi(argv[2]);

    solutions = new std::vector<Cell **>(nw);
    sols_found = new std::vector<bool>(nw);

    for (int i = 0; i < nw; i++)
    {
        (*solutions)[i] = new Cell *[N];
        for (int j = 0; j < N; j++)
            (*solutions)[i][j] = new Cell[N];
        sols_found->at(i) = false;
    }
    Cell **filledGrid = fillGrid((*grids)[board_index]);

    E emitter(filledGrid);

    std::vector<std::unique_ptr<ff_node>> workers;
    for (size_t i = 0; i < nw; i++)
        workers.push_back(make_unique<W>());

    ff_Farm<void> farm(std::move(workers), emitter);

    farm.remove_collector();
    farm.wrap_around();
    auto start_farm = chrono::high_resolution_clock::now();
    if (farm.run_and_wait_end() < 0)
    {
        error("running farm");
        return -1;
    }
    auto elapsed_farm = chrono::high_resolution_clock::now() - start_farm;
    auto usec_farm = chrono::duration_cast<chrono::microseconds>(elapsed_farm).count();

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
        std::cout << "No solution exists\n";
#endif

    cout << "Execution took : " << usec_farm << " usecs." << endl;
    return 0;
}

static inline void usage(const char *argv0)
{
    printf("--------------------\n");
    printf("Usage: %s <n_workers> <board_index>\n", argv0);
    printf("--------------------\n");
    exit(-1);
}