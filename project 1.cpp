#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <fstream>
#include <string>
#include <iomanip>
#include <windows.h>
#include <ctime>
#include <direct.h>

using namespace std;

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void setColor(int color) {
    SetConsoleTextAttribute(hConsole, color);
}

void resetColor() {
    SetConsoleTextAttribute(hConsole, 7);
}

string getCurrentDirectory() {
    char buf[1024];
    if (_getcwd(buf, sizeof(buf)) != NULL) {
        return string(buf);
    }
    return "";
}

class Timer {
    clock_t startTime;
public:
    Timer() {
        startTime = clock();
    }

    double elapsedMilliseconds() {
        clock_t now = clock();
        return (double)(now - startTime) * 1000.0 / CLOCKS_PER_SEC;
    }
};

class Maze {
    vector<vector<int> > grid;
    int startR, startC;
    int endR, endC;
    int rows, cols;

public:
    Maze() {
        rows = cols = 0;
        startR = startC = 0;
        endR = endC = 0;
    }

    bool loadFromFile(const string &fileName) {
        string fullPath = fileName;

        ifstream fin(fullPath.c_str());
        if (!fin.is_open()) {
            fullPath = getCurrentDirectory() + "\\" + fileName;
            fin.open(fullPath.c_str());
            if (!fin.is_open()) {
                cout << "Error: could not open file: " << fileName << endl;
                cout << "Current directory: " << getCurrentDirectory() << endl;
                return false;
            }
        }

        fin >> rows >> cols;
        fin >> startR >> startC;
        fin >> endR >> endC;

        if (!fin.good()) {
            cout << "Error: invalid file format!" << endl;
            fin.close();
            return false;
        }

        grid.assign(rows, vector<int>(cols, 1));
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                fin >> grid[r][c];
            }
        }
        fin.close();

        if (startR < 0 || startR >= rows || startC < 0 || startC >= cols) {
            cout << "Error: start position out of range!" << endl;
            return false;
        }
        if (endR < 0 || endR >= rows || endC < 0 || endC >= cols) {
            cout << "Error: end position out of range!" << endl;
            return false;
        }
        if (grid[startR][startC] == 1 || grid[endR][endC] == 1) {
            cout << "Error: start or end cell lies on a wall!" << endl;
            return false;
        }

        cout << "Maze loaded from: " << fullPath << endl;
        return true;
    }

    bool isValidCell(int r, int c) const {
        return (r >= 0 && r < rows && c >= 0 && c < cols);
    }

    bool isPath(int r, int c) const {
        return isValidCell(r, c) && grid[r][c] == 0;
    }

    bool isWall(int r, int c) const {
        return isValidCell(r, c) && grid[r][c] == 1;
    }

    void printMaze() const {
        cout << "\n=== MAZE (" << rows << " x " << cols << ") ===" << endl;
        cout << "Start: (" << startR << "," << startC << ")" << endl;
        cout << "Goal : (" << endR << "," << endC << ")" << endl;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if (r == startR && c == startC) {
                    setColor(10);
                    cout << "S ";
                } else if (r == endR && c == endC) {
                    setColor(12);
                    cout << "G ";
                } else if (isWall(r, c)) {
                    setColor(8);
                    cout << "# ";
                } else {
                    cout << ". ";
                }
                resetColor();
            }
            cout << endl;
        }
        resetColor();
    }

    int getRows() const { return rows; }
    int getCols() const { return cols; }
    int getStartR() const { return startR; }
    int getStartC() const { return startC; }
    int getEndR() const { return endR; }
    int getEndC() const { return endC; }
    int getCell(int r, int c) const { return isValidCell(r, c) ? grid[r][c] : -1; }
};

class Graph {
    vector<vector<int> > adjList;
    int rows, cols;

    int coordToIndex(int r, int c) const {
        return r * cols + c;
    }

public:
    Graph(const Maze &m) {
        rows = m.getRows();
        cols = m.getCols();
        int total = rows * cols;
        adjList.assign(total, vector<int>());

        int dr[4] = {-1, 1, 0, 0};
        int dc[4] = {0, 0, -1, 1};

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if (m.isWall(r, c)) continue;

                int idx = coordToIndex(r, c);
                for (int k = 0; k < 4; k++) {
                    int nr = r + dr[k];
                    int nc = c + dc[k];
                    if (m.isValidCell(nr, nc) && m.isPath(nr, nc)) {
                        int nid = coordToIndex(nr, nc);
                        adjList[idx].push_back(nid);
                    }
                }
            }
        }
    }

    vector<int> neighbors(int node) const {
        if (node < 0 || node >= (int)adjList.size()) return vector<int>();
        return adjList[node];
    }

    pair<int,int> indexToCoord(int idx) const {
        if (idx < 0 || idx >= rows * cols) return make_pair(-1, -1);
        return make_pair(idx / cols, idx % cols);
    }

    int getIndex(int r, int c) const {
        if (r < 0 || r >= rows || c < 0 || c >= cols) return -1;
        return coordToIndex(r, c);
    }

    int totalNodes() const {
        return rows * cols;
    }

    void showMemoryStats() const {
        int total = rows * cols;
        int matrixBytes = total * sizeof(int);

        int adjBytes = 0;
        adjBytes += adjList.size() * sizeof(vector<int>);
        for (size_t i = 0; i < adjList.size(); i++) {
            adjBytes += adjList[i].size() * sizeof(int);
        }

        cout << "\n=== MEMORY STATISTICS ===" << endl;
        cout << "Matrix (rows*cols*int): " << matrixBytes << " bytes" << endl;
        cout << "Adjacency list:         " << adjBytes << " bytes" << endl;
        cout << "Memory saved:           " << (matrixBytes - adjBytes) << " bytes" << endl;
        if (matrixBytes > 0) {
            double eff = (double)(matrixBytes - adjBytes) * 100.0 / matrixBytes;
            cout << "Efficiency:             " << fixed << setprecision(1) << eff << " %" << endl;
        }
    }
};

class BFSSolver {
    const Graph &g;
    int visitedCount;
    double execTime;

public:
    BFSSolver(const Graph &graphRef) : g(graphRef) {
        visitedCount = 0;
        execTime = 0.0;
    }

    vector<int> solve(int startNode, int endNode) {
        Timer t;
        visitedCount = 0;

        int n = g.totalNodes();
        if (startNode < 0 || startNode >= n || endNode < 0 || endNode >= n) {
            execTime = t.elapsedMilliseconds();
            return vector<int>();
        }

        vector<bool> vis(n, false);
        vector<int> parent(n, -1);
        queue<int> q;

        q.push(startNode);
        vis[startNode] = true;

        bool found = false;

        while (!q.empty() && !found) {
            int cur = q.front();
            q.pop();
            visitedCount++;

            if (cur == endNode) {
                found = true;
                break;
            }

            vector<int> neigh = g.neighbors(cur);
            for (size_t i = 0; i < neigh.size(); i++) {
                int nxt = neigh[i];
                if (!vis[nxt]) {
                    vis[nxt] = true;
                    parent[nxt] = cur;
                    q.push(nxt);
                }
            }
        }

        execTime = t.elapsedMilliseconds();

        vector<int> path;
        if (found) {
            int cur = endNode;
            while (cur != -1) {
                path.push_back(cur);
                cur = parent[cur];
            }
            for (size_t i = 0; i < path.size() / 2; i++) {
                int tmp = path[i];
                path[i] = path[path.size() - 1 - i];
                path[path.size() - 1 - i] = tmp;
            }
        }
        return path;
    }

    int getVisited() const { return visitedCount; }
    double getTime() const { return execTime; }
};

class DFSSolver {
    const Graph &g;
    int visitedCount;
    double execTime;

    void dfs(int cur, int target, vector<bool> &vis, vector<int> &parent, bool &found) {
        vis[cur] = true;
        visitedCount++;

        if (cur == target) {
            found = true;
            return;
        }

        vector<int> neigh = g.neighbors(cur);
        for (size_t i = 0; i < neigh.size(); i++) {
            int nxt = neigh[i];
            if (!vis[nxt] && !found) {
                parent[nxt] = cur;
                dfs(nxt, target, vis, parent, found);
            }
        }
    }

public:
    DFSSolver(const Graph &graphRef) : g(graphRef) {
        visitedCount = 0;
        execTime = 0.0;
    }

    vector<int> solve(int startNode, int endNode) {
        Timer t;
        visitedCount = 0;

        int n = g.totalNodes();
        if (startNode < 0 || startNode >= n || endNode < 0 || endNode >= n) {
            execTime = t.elapsedMilliseconds();
            return vector<int>();
        }

        vector<bool> vis(n, false);
        vector<int> parent(n, -1);
        bool found = false;

        dfs(startNode, endNode, vis, parent, found);

        execTime = t.elapsedMilliseconds();

        vector<int> path;
        if (found) {
            int cur = endNode;
            while (cur != -1) {
                path.push_back(cur);
                cur = parent[cur];
            }
            for (size_t i = 0; i < path.size() / 2; i++) {
                int tmp = path[i];
                path[i] = path[path.size() - 1 - i];
                path[path.size() - 1 - i] = tmp;
            }
        }
        return path;
    }

    int getVisited() const { return visitedCount; }
    double getTime() const { return execTime; }
};

void drawPath(const Maze &m, const Graph &g, const vector<int> &path, bool bfsMode) {
    if (path.empty()) {
        cout << (bfsMode ? "\nNo path found using BFS." : "\nNo path found using DFS.") << endl;
        return;
    }

    cout << (bfsMode ? "\n=== BFS PATH ===" : "\n=== DFS PATH ===") << endl;

    vector<vector<char> > out(m.getRows(), vector<char>(m.getCols(), ' '));

    for (int r = 0; r < m.getRows(); r++) {
        for (int c = 0; c < m.getCols(); c++) {
            if (m.isWall(r, c)) out[r][c] = '#';
            else out[r][c] = '.';
        }
    }

    for (size_t i = 0; i < path.size(); i++) {
        pair<int,int> p = g.indexToCoord(path[i]);
        int r = p.first;
        int c = p.second;

        if (r == m.getStartR() && c == m.getStartC()) out[r][c] = 'S';
        else if (r == m.getEndR() && c == m.getEndC()) out[r][c] = 'G';
        else out[r][c] = '*';
    }

    for (int r = 0; r < m.getRows(); r++) {
        for (int c = 0; c < m.getCols(); c++) {
            char ch = out[r][c];
            if (ch == 'S') {
                setColor(10);
                cout << "S ";
            } else if (ch == 'G') {
                setColor(12);
                cout << "G ";
            } else if (ch == '*') {
                setColor(bfsMode ? 14 : 11);
                cout << "* ";
            } else if (ch == '#') {
                setColor(8);
                cout << "# ";
            } else {
                cout << ". ";
            }
            resetColor();
        }
        cout << endl;
    }

    cout << "\nPath length: " << (int)path.size() - 1 << " steps" << endl;
}

void compareAlgorithms(const BFSSolver &bfs, const DFSSolver &dfs,
                       const vector<int> &bfsPath, const vector<int> &dfsPath) {
    cout << "\n=== ALGORITHM COMPARISON ===" << endl;
    cout << "+---------------------+------------+------------+" << endl;
    cout << "| Metric              | BFS        | DFS        |" << endl;
    cout << "+---------------------+------------+------------+" << endl;

    cout << "| Path Found          | ";
    setColor(bfsPath.empty() ? 12 : 10);
    cout << (bfsPath.empty() ? "No " : "Yes");
    resetColor();
    cout << "         | ";
    setColor(dfsPath.empty() ? 12 : 10);
    cout << (dfsPath.empty() ? "No " : "Yes");
    resetColor();
    cout << "         |" << endl;

    int bfsLen = bfsPath.empty() ? 0 : (int)bfsPath.size() - 1;
    int dfsLen = dfsPath.empty() ? 0 : (int)dfsPath.size() - 1;

    cout << "| Path Length         | " << setw(10) << bfsLen
         << " | " << setw(10) << dfsLen << " |" << endl;

    cout << "| Nodes Visited       | " << setw(10) << bfs.getVisited()
         << " | " << setw(10) << dfs.getVisited() << " |" << endl;

    cout << "| Execution Time (ms) | " << setw(10) << fixed << setprecision(3) << bfs.getTime()
         << " | " << setw(10) << fixed << setprecision(3) << dfs.getTime() << " |" << endl;

    cout << "+---------------------+------------+------------+" << endl;

    if (!bfsPath.empty() && !dfsPath.empty()) {
        if (bfsLen < dfsLen) {
            cout << "\nBFS found a shorter (optimal) path in this unweighted maze." << endl;
        } else if (bfsLen == dfsLen) {
            cout << "\nBoth algorithms found paths of equal length." << endl;
        } else {
            cout << "\nDFS ended up with a longer path (DFS is not guaranteed shortest)." << endl;
        }
    }
}

void createSampleMazeFile() {
    string candidates[2] = {
        "sample_maze.txt",
        getCurrentDirectory() + "\\sample_maze.txt"
    };

    bool ok = false;
    string used;

    for (int i = 0; i < 2; i++) {
        ofstream fout(candidates[i].c_str());
        if (fout.is_open()) {
            fout << "10 10\n";
            fout << "0 0\n";
            fout << "9 9\n";

            fout << "0 1 0 0 0 0 0 1 0 0\n";
            fout << "0 1 0 1 1 1 0 1 0 0\n";
            fout << "0 0 0 0 0 0 0 1 0 0\n";
            fout << "1 1 1 1 0 1 1 1 0 1\n";
            fout << "0 0 0 0 0 0 0 0 0 0\n";
            fout << "0 1 1 1 1 1 1 1 1 0\n";
            fout << "0 0 0 0 0 0 0 0 0 0\n";
            fout << "0 1 0 1 0 1 0 1 0 1\n";
            fout << "0 1 0 1 0 1 0 1 0 0\n";
            fout << "0 0 0 1 0 0 0 1 0 0\n";

            fout.close();
            ok = true;
            used = candidates[i];
            break;
        }
    }

    if (ok) {
        cout << "sample_maze.txt created successfully at: " << used << endl;
    } else {
        cout << "Error: could not create sample_maze.txt" << endl;
        cout << "Current directory: " << getCurrentDirectory() << endl;
    }
}

int main() {
    Maze maze;
    string fileName;

    cout << "Enter maze file name (or press Enter to auto-create sample_maze.txt): ";
    getline(cin, fileName);

    if (fileName.empty()) {
        createSampleMazeFile();
        fileName = "sample_maze.txt";
    }

    if (!maze.loadFromFile(fileName)) {
        return 0;
    }

    maze.printMaze();

    Graph graph(maze);
    graph.showMemoryStats();

    int startIndex = graph.getIndex(maze.getStartR(), maze.getStartC());
    int endIndex = graph.getIndex(maze.getEndR(), maze.getEndC());

    BFSSolver bfs(graph);
    DFSSolver dfs(graph);

    vector<int> bfsPath = bfs.solve(startIndex, endIndex);
    vector<int> dfsPath = dfs.solve(startIndex, endIndex);

    drawPath(maze, graph, bfsPath, true);
    drawPath(maze, graph, dfsPath, false);

    compareAlgorithms(bfs, dfs, bfsPath, dfsPath);

    cout << "\nPress Enter to exit...";
    cin.ignore();
    return 0;
}
