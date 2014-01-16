#include "Maze.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <time.h>
#include <queue>
#include <vector>

#include "Constants.h"
#include "Parameters.h"
#include "Tile.h"

namespace sim{

Maze::Maze(int width, int height, std::string mazeFile){

    // Misc errand - seed the random number generator
    srand(time(NULL));
    
    // Initialize the tile positions
    for (int x = 0; x < width; x++){
        std::vector<Tile> col;
        for (int y = 0; y < height; y++){
            Tile tile;
            tile.setPos(x, y);
            col.push_back(tile);
        }
        m_maze.push_back(col);
    }
    
    // Initialize the tile wall values and tile neighbors
    if (mazeFile == ""){
        randomize();
        while (!solveShortestPath()){
            randomize();
        }
        // Optional - can be used to generate more maze files
        saveMaze("mazeFiles/auto_generated_maze.maz");
    }
    else{

        // Load the maze given by mazefile
        loadMaze(mazeFile);

        // Ensures that the maze is solvable
        if (!solveShortestPath()){
            std::cout << "Unsolvable Maze detected. Generating solvable maze..." << std::endl;
        }
        while (!solveShortestPath()){
            randomize();
        }

    }
    
    // Increment the passes for the starting position
    m_maze.at(0).at(0).incrementPasses();
}

Maze::~Maze()
{ }

int Maze::getWidth(){
    return m_maze.size();
}

int Maze::getHeight(){
    if (m_maze.size() > 0){
        return m_maze.at(0).size();
    }
    return 0;
}

Tile* Maze::getTile(int x, int y){
    return &m_maze.at(x).at(y);
}

void Maze::randomize(){
    
    // Declare width and height locally to reduce function calls
    int width = getWidth();
    int height = getHeight();

    // The probability of any one wall appearing is 1/probDenom
    int probDenom = 2;

    for (int x = 0; x < width; x++){
        for (int y = 0; y < height; y++){
            bool walls[4]; // Make a walls array for tile (x, y)
            for (int k = 0; k < 4; k++){
                walls[k] = rand() % probDenom == 0;
                switch (k){
                    case NORTH:
                        if (y + 1 < height){
                            getTile(x, y+1)->setWall(SOUTH, walls[k]);
                            getTile(x, y)->setWall(NORTH, walls[k]);
                        }
                        else {
                            getTile(x, y)->setWall(NORTH, true);
                        }
                        break;
                    case EAST:
                        if (x + 1 < width){
                            getTile(x+1, y)->setWall(WEST, walls[k]);
                            getTile(x, y)->setWall(EAST, walls[k]);
                        }
                        else {
                            getTile(x, y)->setWall(EAST, true);
                        }
                        break;
                    case SOUTH:
                        if (y > 0){
                            getTile(x, y-1)->setWall(NORTH, walls[k]);
                            getTile(x, y)->setWall(SOUTH, walls[k]);
                        }
                        else {
                            getTile(x, y)->setWall(SOUTH, true);
                        }
                        break;
                    case WEST:
                        if (x > 0){
                            getTile(x-1, y)->setWall(EAST, walls[k]);
                            getTile(x, y)->setWall(WEST, walls[k]);
                        }
                        else {
                            getTile(x, y)->setWall(WEST, true);
                        }
                        break;
                }
            }
        }
    }

    // Ensures that the middle is hallowed out
    if (width % 2 == 0){
        if (height % 2 == 0){
            getTile(width/2-1, height/2-1)->setWall(NORTH, false);
            getTile(width/2-1, height/2)->setWall(SOUTH, false);
            getTile(width/2, height/2-1)->setWall(NORTH, false);
            getTile(width/2, height/2)->setWall(SOUTH, false);
            getTile(width/2-1, height/2-1)->setWall(EAST, false);
            getTile(width/2, height/2-1)->setWall(WEST, false);
        }
        getTile(width/2-1, height/2)->setWall(EAST, false);
        getTile(width/2, height/2)->setWall(WEST, false);
            
    }
    if (height % 2 == 0){
        getTile(width/2, height/2-1)->setWall(NORTH, false);
        getTile(width/2, height/2)->setWall(SOUTH, false);
    }

    // Once the walls are assigned, we can assign neighbors
    assignNeighbors();
}

void Maze::saveMaze(std::string mazeFile){
    
    // Create the stream
    std::ofstream file(mazeFile.c_str());

    if (file.is_open()){

        // Very primitive, but will work
        for (int x = 0; x <  getWidth(); x++){
            for (int y = 0; y < getHeight(); y++){
                file << x << " " << y;
                for (int k = 0; k < 4; k++){
                    file << " " << (getTile(x, y)->isWall(k) ? 1 : 0);
                }
                file << std::endl;
            }
        }

        file.close();
    }
}

void Maze::loadMaze(std::string mazeFile){

    // Create the stream
    std::ifstream file(mazeFile.c_str());

    // Initialize a string variable
    std::string line("");

    if (file.is_open()){

        // Very primitive, but will work
        while (getline(file, line)){
            std::istringstream iss(line);
            std::vector<std::string> tokens;
            copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(),
                 std::back_inserter<std::vector<std::string> >(tokens));
            for (int i = 0; i < 4; i++){ // Set the values of all of the walls
                getTile(atoi(tokens.at(0).c_str()), atoi(tokens.at(1).c_str()))
                      ->setWall(i, atoi(tokens.at(2+i).c_str()));
            }
        }
    
        file.close();

        // Once the walls are assigned, we can assign neighbors
        assignNeighbors();
    }
}

void Maze::assignNeighbors(){
    for (int x = 0; x < getWidth(); x++){
        for (int y = 0; y < getHeight(); y++){

            Tile* tile = getTile(x, y);

            // First we ensure that we have a fresh (empty) list
            tile->resetNeighbors();

            // Then we assign new neighbors
            if (!tile->isWall(NORTH)){
                tile->addNeighbor(getTile(x, y+1));
            }
            if (!tile->isWall(EAST)){
                tile->addNeighbor(getTile(x+1, y));
            }
            if (!tile->isWall(SOUTH)){
                tile->addNeighbor(getTile(x, y-1));
            }
            if (!tile->isWall(WEST)){
                tile->addNeighbor(getTile(x-1, y));
            }
        }        
    }        
}

void Maze::printDistances(){
    std::cout << std::endl;
    for (int y = getHeight()-1; y >= 0; y--){
        for (int x = 0; x < getWidth(); x++){
            if (getTile(x, y)->getDistance() < 100){
                std::cout << " ";
                if (getTile(x, y)->getDistance() < 10){
                    std::cout << " ";
                }
            }
            std::cout << getTile(x, y)->getDistance() << " ";
        }
        std::cout << std::endl;
    }
}

bool Maze::solveShortestPath(){

    // Solves the maze, assigns tiles that are part of the shortest path
    std::vector<Tile*> sp = findPathToCenter();
    for (int i = 0; i < sp.size(); i++){
        getTile(sp.at(i)->getX(), sp.at(i)->getY())->setPosp(true);
    }

    // Returns whether or not the maze is solvable with the proper min steps
    return getClosestCenterTile()->getDistance() < MAX_DISTANCE  &&
           getClosestCenterTile()->getDistance() > MIN_MAZE_STEPS;
}

std::vector<Tile*> Maze::findPath(int x1, int y1, int x2, int y2){
    setDistancesFrom(x1, x2);
    return backtrace(getTile(x2, y2));
}

std::vector<Tile*> Maze::findPathToCenter(){
    setDistancesFrom(0, 0);
    return backtrace(getClosestCenterTile());
}

std::vector<Tile*> Maze::backtrace(Tile* end){

    // The vector to hold the solution nodes
    std::vector<Tile*> path;

    // Start at the ending node and simply trace back through the values
    std::queue<Tile*> queue;
    queue.push(end);
    path.push_back(end);

    while (!queue.empty()){
        Tile* node = queue.front();
        queue.pop(); // Removes the element
        std::vector<Tile*> neighbors = node->getNeighbors();
        for (int i = 0; i < neighbors.size(); i++){
            if (neighbors.at(i)->getDistance() == node->getDistance() - 1){
                path.push_back(neighbors.at(i));
                queue.push(neighbors.at(i));
            }
        }
    }

    return path;
}

void Maze::setDistancesFrom(int x, int y){
    
    // Ensure that the nodes are fresh every time this function is called
    for (int x = 0; x < getWidth(); x++){
        for (int y = 0; y < getHeight(); y++){
            Tile* tile = getTile(x, y);
            tile->setDistance(MAX_DISTANCE);
            tile->setExplored(false);
            tile->setPosp(false);
        }
    }

    // Queue for BFS
    std::queue<Tile*> queue;
    queue.push(getTile(x, y));
    getTile(x, y)->setDistance(0);
    getTile(x, y)->setExplored(true);

    while (!queue.empty()){
        Tile* node = queue.front();
        queue.pop(); // Removes the element
        std::vector<Tile*> neighbors = node->getNeighbors();
        for (int i = 0; i < neighbors.size(); i++){
            if (!neighbors.at(i)->getExplored()){
                neighbors.at(i)->setDistance(node->getDistance() + 1); 
                queue.push(neighbors.at(i));
                neighbors.at(i)->setExplored(true);
            }
        }
    }
}

Tile* Maze::getClosestCenterTile(){

    if (getWidth() > 0 && getHeight() > 0){
    
        int midWidth = getWidth()/2;
        int midHeight = getHeight()/2;
        Tile* closest = getTile(midWidth, midHeight);

        if (getWidth() % 2 == 0){
            if (getTile(midWidth-1, midHeight)->getDistance() < closest->getDistance()){
                closest = getTile(midWidth-1, midHeight);
            }
        }
        if (getHeight() % 2 == 0){
            if (getTile(midWidth, midHeight-1)->getDistance() < closest->getDistance()){
                closest = getTile(midWidth, midHeight-1);
            }
        }
        if (getWidth() % 2 == 0 && getHeight() % 2 == 0){
            if (getTile(midWidth-1, midHeight-1)->getDistance() < closest->getDistance()){
                closest = getTile(midWidth-1, midHeight-1);
            }
        }
        
        return closest;
    }
    
    // If either the width or the height is zero return NULL
    return NULL;
}

} // namespace sim