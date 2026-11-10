#include "Graph.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

using namespace std;

Graph::Graph(int n) {
    if (n < 0) {
        throw invalid_argument("El numero de nodos no puede ser negativo.");
    }
    numNodes = n;
    adjMatrix.assign(n, vector<double>(n, 0.0));
}

void Graph::addEdge(int u, int v, double weight) {
    if (u < 0 || u >= numNodes || v < 0 || v >= numNodes) {
        throw out_of_range("Indice de nodo fuera de rango.");
    }
    adjMatrix[u][v] = weight;
    adjMatrix[v][u] = weight;
}

double Graph::getCost(int u, int v) const {
    if (u < 0 || u >= numNodes || v < 0 || v >= numNodes) {
        throw out_of_range("Indice de nodo fuera de rango.");
    }
    return adjMatrix[u][v];
}

int Graph::getNumNodes() const {
    return numNodes;
}

const vector<vector<double>>& Graph::getMatrix() const {
    return adjMatrix;
}

Graph Graph::fromStructuredStream(istream& input, int maxNodes) {
    vector<Punto> nodos;
    string linea;

    while (getline(input, linea)) {
        if (linea.empty() || linea[0] == '#') {
            continue;
        }
        stringstream ss(linea);
        Punto p;
        if (ss >> p.id >> p.x >> p.y) {
            nodos.push_back(p);
            if (maxNodes > 0 && (int)nodos.size() >= maxNodes) {
                break;
            }
        }
    }

    int n = (int)nodos.size();
    Graph g(n);

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double dx = nodos[i].x - nodos[j].x;
            double dy = nodos[i].y - nodos[j].y;
            double dist = round(hypot(dx, dy) * 100.0) / 100.0;
            g.addEdge(i, j, dist);
        }
    }

    return g;
}

Graph Graph::createPilotInstance(int n) {
    if (n <= 0 || n > 20) {
        throw invalid_argument("La instancia piloto soporta entre 1 y 20 nodos.");
    }

    string datosSimulados =
        "# ID X Y\n"
        "0 50.0 50.0\n"
        "1 20.0 80.0\n"
        "2 85.0 75.0\n"
        "3 15.0 25.0\n"
        "4 65.0 15.0\n"
        "5 90.0 40.0\n"
        "6 35.0 60.0\n"
        "7 70.0 85.0\n"
        "8 10.0 55.0\n"
        "9 45.0 20.0\n"
        "10 80.0 10.0\n"
        "11 25.0 95.0\n"
        "12 55.0 75.0\n"
        "13 95.0 65.0\n"
        "14 30.0 10.0\n"
        "15 60.0 35.0\n"
        "16 12.0 40.0\n"
        "17 78.0 55.0\n"
        "18 42.0 88.0\n"
        "19 88.0 22.0\n";

    stringstream stream(datosSimulados);
    return fromStructuredStream(stream, n);
}
