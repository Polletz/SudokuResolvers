/**
 * Parallel and Distributed Systems: Paradigms and Models
 * Year 2019/2020
 * Final Project
 * Paoletti Riccardo
 * Student ID: 532143
*/

/* This file gathers together a bunch of utility function used in all project files */

#include <iostream>
#include <string>
#include <mutex>
#include <tgmath.h>
#include <vector>
#include <fstream>
#include <chrono>
#include <utility>
#include <thread>
#include <atomic>
#include <dirent.h>
#include <sys/types.h>

using namespace std;

#define UNASSIGNED 0

#define WORKING_ON -1

#define N 9

struct Cell
{
    int value = UNASSIGNED;
    std::vector<int> possible_values;
};

bool UsedInRow(Cell** grid, int row, int num)
{
    for (int col = 0; col < N; col++)
        if (grid[row][col].value == num)
            return true;
    return false;
}

bool UsedInCol(Cell** grid, int col, int num)
{
    for (int row = 0; row < N; row++)
        if (grid[row][col].value == num)
            return true;
    return false;
}

bool UsedInBox(Cell** grid, int boxStartRow, int boxStartCol, int num)
{
    for (int row = 0; row < sqrt(N); row++)
        for (int col = 0; col < sqrt(N); col++)
            if (grid[row + boxStartRow][col + boxStartCol].value == num)
                return true;
    return false;
}

bool isSafe(Cell** grid, int row, int col, int num)
{

    return  !UsedInRow(grid, row, num) && 
            !UsedInCol(grid, col, num) && 
            !UsedInBox(grid, row - row % ((int)sqrt(N)), col - col % ((int)sqrt(N)), num) &&
            grid[row][col].value == UNASSIGNED;
}

void printGrid(Cell **grid)
{
    for (int row = 0; row < N; row++)
    {
        for (int col = 0; col < N; col++)
            std::cout << grid[row][col].value << " ";
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << std::endl;
}

void allocateGrid(int**&grid){
    grid = new int *[N];
    for (int i = 0; i < N; i++)
        grid[i] = new int[N];
}

void readGrids(vector<int**>* &grids, string filename){
    
    ifstream file(filename);
    if (file.is_open()) {
        std::string line;
        while (getline(file, line)) {
            int** new_grid;
            allocateGrid(new_grid);
            size_t pos = 0;
            std::string token;
            int i=0, j=0;
            while ((pos = line.find(" ")) != string::npos) {
                token = line.substr(0, pos);
            
                new_grid[i][j] = atoi(token.c_str());
                j++;
                if(j==N){
                    i++;
                    j=0;
                }

                line.erase(0, pos + 1);
            }
            grids->push_back(new_grid);
        }
        file.close();
    }
}

bool FindUnassignedMinimumLocation(Cell **grid, int &row, int &col)
{
    int min = N+1;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if (grid[i][j].value == UNASSIGNED){
                int size = grid[i][j].possible_values.size();
                if(size < min){
                    min = size;
                    row = i;
                    col = j;
                }
            }
    
    return (min != N+1);
}

void calculatePossibleValues(Cell** grid){
    for(int row=0;row<N;row++)
        for(int col=0;col<N;col++)
        {
            grid[row][col].possible_values.clear();
            for(int num=1;num<=N;num++)
                if(isSafe(grid, row, col, num))
                    grid[row][col].possible_values.push_back(num);
        }
}

Cell** fillGrid(int** grid){
    Cell** filledGrid = new Cell*[N];
    for(int row=0;row<N;row++){
        filledGrid[row] = new Cell[N];
        for(int col=0;col<N;col++){
            Cell c;
            c.value = grid[row][col];
            filledGrid[row][col] = c;
        }
    }
    calculatePossibleValues(filledGrid);
    return filledGrid;
}

void copyCell(Cell** source, Cell** &dest){
    dest = new Cell *[N];
    for (int r = 0; r < N; r++)
    {
        dest[r] = new Cell[N];
        for (int c = 0; c < N; c++)
            if (source[r][c].value == WORKING_ON)
                dest[r][c].value = UNASSIGNED;
            else
                dest[r][c].value = source[r][c].value;
    }
}

void RemoveSingletons(Cell** &grid){
    int min = N+1;
    bool found = true;
    while(found){
        found = false;
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                if (grid[i][j].value == UNASSIGNED){
                    int size = grid[i][j].possible_values.size();
                    if(size == 1){
                        grid[i][j].value = grid[i][j].possible_values.back();
                        grid[i][j].possible_values.clear();
                        found = true;
                    }
                }
        calculatePossibleValues(grid);
    }
}