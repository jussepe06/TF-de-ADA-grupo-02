#include "Algorithms.h"
#include "Graph.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

string formatearRuta(const vector<int>& ruta) {
    ostringstream oss;
    for (size_t i = 0; i < ruta.size(); i++) {
        oss << ruta[i];
        if (i + 1 < ruta.size()) {
            oss << " -> ";
        }
    }
    return oss.str();
}

void mostrarResultadoIndividual(const TSPResult& res) {
    cout << "\n------------------------------------------------------------\n";
    cout << " Algoritmo ejecutado : " << res.algorithmName << "\n";
    cout << " Costo total (dist.) : " << fixed << setprecision(2) << res.minCost << "\n";
    cout << " Tiempo en microseg. : " << fixed << setprecision(2) << res.timeMicroseconds << " us\n";
    cout << " Tiempo en miliseg.  : " << fixed << setprecision(4) << res.timeMilliseconds << " ms\n";
    cout << " Ruta calculada      : " << formatearRuta(res.path) << "\n";
    cout << "------------------------------------------------------------\n";
}

void mostrarNodosYMatriz(const Graph& grafo) {
    int n = grafo.getNumNodes();
    const auto& coords = grafo.getCoordinates();
    const auto& mat = grafo.getMatrix();

    cout << "\n--- Coordenadas Cargadas (Nodo 0 = Almacen Central) ---\n";
    for (int i = 0; i < (int)coords.size(); i++) {
        cout << " Nodo " << setw(2) << coords[i].id
             << " : (" << fixed << setprecision(1) << coords[i].x
             << ", " << coords[i].y << ")\n";
    }

    cout << "\n--- Validacion del Grafo ---\n";
    if (grafo.validateGraph()) {
        cout << " [OK] Grafo valido: Conexo, Simetrico, Diagonal 0 y cumple Desigualdad Triangular.\n";
    } else {
        cout << " [ERROR] El grafo no supero la validacion.\n";
    }

    cout << "\n--- Matriz de Distancias (" << n << "x" << n << ") ---\n";
    cout << setw(6) << " ";
    for (int j = 0; j < n; j++) {
        cout << setw(7) << ("N" + to_string(j));
    }
    cout << "\n";

    for (int i = 0; i < n; i++) {
        cout << setw(5) << ("N" + to_string(i)) << " ";
        for (int j = 0; j < n; j++) {
            cout << setw(7) << fixed << setprecision(1) << mat[i][j];
        }
        cout << "\n";
    }
}

void ejecutarTablaComparativa(const Graph& grafo, const string& tituloInstancia) {
    int n = grafo.getNumNodes();
    Profiler profiler;
    vector<TSPResult> resultados;

    if (n <= 13) {
        profiler.start();
        TSPResult r1 = Algorithms::Backtracking(grafo);
        r1.timeMicroseconds = profiler.elapsedMicroseconds();
        r1.timeMilliseconds = profiler.elapsedMilliseconds();
        resultados.push_back(r1);
    }

    profiler.start();
    TSPResult r2 = Algorithms::HeldKarp(grafo);
    r2.timeMicroseconds = profiler.elapsedMicroseconds();
    r2.timeMilliseconds = profiler.elapsedMilliseconds();
    resultados.push_back(r2);

    profiler.start();
    TSPResult r3 = Algorithms::MST_DFS(grafo);
    r3.timeMicroseconds = profiler.elapsedMicroseconds();
    r3.timeMilliseconds = profiler.elapsedMilliseconds();
    resultados.push_back(r3);

    profiler.start();
    TSPResult r4 = Algorithms::NearestNeighbor(grafo, 0);
    r4.timeMicroseconds = profiler.elapsedMicroseconds();
    r4.timeMilliseconds = profiler.elapsedMilliseconds();
    resultados.push_back(r4);

    double costoOptimo = resultados[0].minCost;

    cout << "\n====================================================================================================\n";
    cout << "  " << tituloInstancia << " | N = " << n << " NODOS | Grafo Valido: "
         << (grafo.validateGraph() ? "SI" : "NO") << "\n";
    cout << "====================================================================================================\n";
    cout << left << setw(20) << "Algoritmo"
         << right << setw(12) << "Costo"
         << setw(12) << "Diferencia"
         << setw(14) << "Tiempo (us)"
         << setw(14) << "Tiempo (ms)"
         << "   " << left << "Ruta\n";
    cout << "----------------------------------------------------------------------------------------------------\n";

    cout << fixed;
    for (const auto& res : resultados) {
        double gap = ((res.minCost - costoOptimo) / costoOptimo) * 100.0;
        cout << left << setw(20) << res.algorithmName
             << right << setw(12) << setprecision(2) << res.minCost
             << setw(11) << setprecision(2) << gap << "%"
             << setw(14) << setprecision(2) << res.timeMicroseconds
             << setw(14) << setprecision(4) << res.timeMilliseconds
             << "   " << formatearRuta(res.path) << "\n";
    }
    cout << "====================================================================================================\n";
}

int main() {
    int numNodos = 10;
    Graph grafo = Graph::createPilotInstance(numNodos);
    Profiler profiler;
    int opcion = -1;

    while (opcion != 0) {
        cout << "\n============================================================\n";
        cout << "        SISTEMA LOGISTICO ROUTEOPT - MENU PRINCIPAL\n";
        cout << "============================================================\n";
        cout << " Instancia actual: N = " << numNodos << " nodos (Almacen 0 + " << numNodos - 1 << " clientes)\n";
        cout << "------------------------------------------------------------\n";
        cout << " [1] Ejecutar Backtracking / Branch & Bound (Exacto)\n";
        cout << " [2] Ejecutar Held-Karp / Prog. Dinamica (Exacto)\n";
        cout << " [3] Ejecutar MST + DFS con Prim (Aproximado)\n";
        cout << " [4] Ejecutar Nearest Neighbor / Vecino Mas Cercano (Voraz)\n";
        cout << " [5] Comparar los 4 algoritmos en la instancia actual\n";
        cout << " [6] Simulacion de Instancias Pequena (N=8) y Mediana (N=12)\n";
        cout << " [7] Ver coordenadas, matriz de distancias y validar grafo\n";
        cout << " [8] Cambiar cantidad de nodos N (de 4 a 20)\n";
        cout << " [0] Salir\n";
        cout << "============================================================\n";
        cout << "Seleccione una opcion: ";

        if (!(cin >> opcion)) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Entrada invalida. Ingrese un numero del 0 al 8.\n";
            opcion = -1;
            continue;
        }

        if (opcion == 1) {
            profiler.start();
            TSPResult r = Algorithms::Backtracking(grafo);
            r.timeMicroseconds = profiler.elapsedMicroseconds();
            r.timeMilliseconds = profiler.elapsedMilliseconds();
            mostrarResultadoIndividual(r);
        } else if (opcion == 2) {
            profiler.start();
            TSPResult r = Algorithms::HeldKarp(grafo);
            r.timeMicroseconds = profiler.elapsedMicroseconds();
            r.timeMilliseconds = profiler.elapsedMilliseconds();
            mostrarResultadoIndividual(r);
        } else if (opcion == 3) {
            profiler.start();
            TSPResult r = Algorithms::MST_DFS(grafo);
            r.timeMicroseconds = profiler.elapsedMicroseconds();
            r.timeMilliseconds = profiler.elapsedMilliseconds();
            mostrarResultadoIndividual(r);
        } else if (opcion == 4) {
            profiler.start();
            TSPResult r = Algorithms::NearestNeighbor(grafo, 0);
            r.timeMicroseconds = profiler.elapsedMicroseconds();
            r.timeMilliseconds = profiler.elapsedMilliseconds();
            mostrarResultadoIndividual(r);
        } else if (opcion == 5) {
            ejecutarTablaComparativa(grafo, "COMPARACION EN INSTANCIA ACTUAL");
        } else if (opcion == 6) {
            Graph gPequeno = Graph::createPilotInstance(8);
            Graph gMediano = Graph::createPilotInstance(12);
            ejecutarTablaComparativa(gPequeno, "SIMULACION INSTANCIA PEQUENA");
            ejecutarTablaComparativa(gMediano, "SIMULACION INSTANCIA MEDIANA");
        } else if (opcion == 7) {
            mostrarNodosYMatriz(grafo);
        } else if (opcion == 8) {
            int nuevoN;
            cout << "Ingrese la nueva cantidad de nodos (4 a 20): ";
            if (cin >> nuevoN && nuevoN >= 4 && nuevoN <= 20) {
                numNodos = nuevoN;
                grafo = Graph::createPilotInstance(numNodos);
                cout << "Grafo actualizado y validado con N = " << numNodos << " nodos.\n";
            } else {
                cin.clear();
                cin.ignore(10000, '\n');
                cout << "Valor fuera de rango (debe ser entre 4 y 20).\n";
            }
        } else if (opcion == 0) {
            cout << "Saliendo de RouteOpt...\n";
        } else {
            cout << "Opcion no valida.\n";
        }
    }

    return 0;
}
