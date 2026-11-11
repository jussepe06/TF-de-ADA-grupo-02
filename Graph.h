#ifndef GRAPH_H
#define GRAPH_H

#include <istream>
#include <string>
#include <vector>

using namespace std;

struct Punto {
    int id;
    double x;
    double y;
};

class Graph {
private:
    int numNodes;
    vector<Punto> coordinates;
    vector<vector<double>> adjMatrix;

public:
    Graph(int n = 0);

    void addEdge(int u, int v, double weight);
    double getCost(int u, int v) const;
    int getNumNodes() const;
    const vector<vector<double>>& getMatrix() const;
    const vector<Punto>& getCoordinates() const;

    bool validateGraph() const;

    static Graph fromStructuredStream(istream& input, int maxNodes = -1);
    static Graph createPilotInstance(int n = 10);
};

#endif
