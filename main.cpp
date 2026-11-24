#include "Algorithms.h"
#include "Graph.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

string formatearRuta(const vector<int>& ruta, size_t maxNodosVisibles = 18) {
    if (ruta.empty()) return "(sin ruta)";
    ostringstream oss;
    if (ruta.size() <= maxNodosVisibles) {
        for (size_t i = 0; i < ruta.size(); i++) {
            oss << ruta[i];
            if (i + 1 < ruta.size()) oss << " -> ";
        }
    } else {
        for (size_t i = 0; i < 6; i++) {
            oss << ruta[i] << " -> ";
        }
        oss << "... -> ";
        for (size_t i = ruta.size() - 4; i < ruta.size(); i++) {
            oss << ruta[i];
            if (i + 1 < ruta.size()) oss << " -> ";
        }
        oss << " (" << ruta.size() - 1 << " tramos)";
    }
    return oss.str();
}

string formatearRutaGuiones(const vector<int>& ruta) {
    ostringstream oss;
    for (size_t i = 0; i < ruta.size(); i++) {
        oss << ruta[i];
        if (i + 1 < ruta.size()) oss << "-";
    }
    return oss.str();
}

void mostrarResultadoIndividual(const TSPResult& res) {
    cout << "\n----------------------------------------------------------------------\n";
    cout << " Algoritmo ejecutado   : " << res.algoritmo << "\n";
    if (!res.ejecutado) {
        cout << " Estado                : OMITIDO POR PROTECCION DE ESCALA\n";
        cout << " Motivo                : " << res.motivo_omision << "\n";
        cout << "----------------------------------------------------------------------\n";
        return;
    }
    cout << " Distancia total       : " << fixed << setprecision(2) << res.distancia_km << " km\n";
    cout << " Costo operativo total : $" << fixed << setprecision(2) << res.costo_usd << " USD\n";
    cout << " Tiempo en microseg.   : " << fixed << setprecision(2) << res.tiempo_us << " us\n";
    cout << " Tiempo en miliseg.    : " << fixed << setprecision(4) << res.tiempo_ms << " ms\n";
    cout << " Memoria estimada/pico : " << fixed << setprecision(2) << res.memoria_kb << " KB\n";
    cout << " Operaciones / Estados : " << res.operaciones_o_estados << "\n";
    cout << " Ruta calculada        : " << formatearRuta(res.ruta, 25) << "\n";
    cout << "----------------------------------------------------------------------\n";
}

vector<TSPResult> evaluarCuatroAlgoritmos(const Graph& grafo, bool imprimirAvisos = true) {
    vector<TSPResult> todos = {
        Algorithms::Backtracking(grafo, Algorithms::kMaxSafeBacktracking),
        Algorithms::HeldKarp(grafo, Algorithms::kMaxSafeHeldKarp),
        Algorithms::MST_DFS(grafo),
        Algorithms::NearestNeighbor(grafo, 0)
    };

    double mejorDistancia = numeric_limits<double>::infinity();
    for (const auto& r : todos) {
        if (r.ejecutado && r.distancia_km < mejorDistancia) {
            mejorDistancia = r.distancia_km;
        }
    }

    for (auto& r : todos) {
        if (r.ejecutado && mejorDistancia > 0.0 && mejorDistancia < numeric_limits<double>::infinity()) {
            r.desviacion_pct = max(0.0, ((r.distancia_km - mejorDistancia) / mejorDistancia) * 100.0);
        } else if (!r.ejecutado && imprimirAvisos) {
            cout << " [Aviso] " << r.algoritmo << " -> " << r.motivo_omision << "\n";
        }
    }

    return todos;
}

void mostrarTablaComparativa(const Graph& grafo, const string& titulo) {
    int n = grafo.getNumNodes();
    bool valido = grafo.validateGraph();

    cout << "\n========================================================================================================================\n";
    cout << "  " << titulo << " | Instancia: " << grafo.getInstanceName() << "\n";
    cout << "  N = " << n << " nodos | Metrica: " << (grafo.isGeographic() ? "Haversine (Geografica)" : "Euclidiana (Plana)")
         << " | TSP Metrico (Desig. Triangular): " << (valido ? "SI" : "NO") << "\n";
    cout << "========================================================================================================================\n";

    vector<TSPResult> resultados = evaluarCuatroAlgoritmos(grafo, true);

    cout << left << setw(20) << "Algoritmo"
         << right << setw(12) << "Dist (km)"
         << setw(13) << "Costo (USD)"
         << setw(11) << "Desv (%)"
         << setw(13) << "Tiempo (ms)"
         << setw(13) << "Memoria(KB)"
         << setw(15) << "Operaciones"
         << "   " << left << "Ruta\n";
    cout << "------------------------------------------------------------------------------------------------------------------------\n";

    cout << fixed;
    for (const auto& res : resultados) {
        if (!res.ejecutado) continue;
        cout << left << setw(20) << res.algoritmo
             << right << setw(12) << setprecision(2) << res.distancia_km
             << setw(13) << setprecision(2) << res.costo_usd
             << setw(10) << setprecision(2) << res.desviacion_pct << "%"
             << setw(13) << setprecision(4) << res.tiempo_ms
             << setw(13) << setprecision(2) << res.memoria_kb
             << setw(15) << res.operaciones_o_estados
             << "   " << formatearRuta(res.ruta, 12) << "\n";
    }
    cout << "========================================================================================================================\n";
}

void mostrarNodosYMatriz(const Graph& grafo) {
    int n = grafo.getNumNodes();
    const auto& coords = grafo.getCoordinates();
    const auto& mat = grafo.getMatrix();
    const auto& params = grafo.getParametrosCosto();

    cout << "\n--- Informacion General de la Instancia ---\n";
    cout << " Nombre             : " << grafo.getInstanceName() << "\n";
    cout << " Total de nodos (N) : " << n << " (Nodo 0 = Almacen Central + " << (n - 1) << " clientes)\n";
    cout << " Tipo de distancia  : " << (grafo.isGeographic() ? "Geografica Real (Haversine km)" : "Euclidiana Plana (km)") << "\n";
    cout << " Costos operativos  : Pc=$" << fixed << setprecision(2) << params.precioCombustible
         << "/L | R=" << params.rendimientoKmL << " km/L | V=" << params.velocidadPromKmH
         << " km/h | Co=$" << params.costoHoraChofer << "/h | Cp=$" << params.costoParadaCliente << "/cliente\n";

    cout << "\n--- Validacion Estructural del Grafo ---\n";
    if (grafo.validateGraph()) {
        cout << " [OK] Grafo valido: Conexo, Simetrico, Diagonal 0 y cumple Desigualdad Triangular.\n";
    } else {
        cout << " [ADVERTENCIA] El grafo no cumple alguna propiedad metrica estricta.\n";
    }

    int limiteCoords = min(n, 25);
    cout << "\n--- Nodos Cargados (mostrando " << limiteCoords << " de " << n << ") ---\n";
    for (int i = 0; i < limiteCoords; i++) {
        cout << " [" << setw(3) << coords[i].id << "] "
             << left << setw(40) << coords[i].nombre
             << right << " (" << fixed << setprecision(4) << coords[i].x
             << ", " << setprecision(4) << coords[i].y << ")\n";
    }
    if (n > limiteCoords) {
        cout << " ... (" << (n - limiteCoords) << " nodos adicionales omitidos en vista resumida)\n";
    }

    int limiteMatriz = min(n, 12);
    cout << "\n--- Matriz de Distancias en km (vista " << limiteMatriz << "x" << limiteMatriz << ") ---\n";
    cout << setw(6) << " ";
    for (int j = 0; j < limiteMatriz; j++) {
        cout << setw(8) << ("N" + to_string(j));
    }
    cout << "\n";

    for (int i = 0; i < limiteMatriz; i++) {
        cout << setw(5) << ("N" + to_string(i)) << " ";
        for (int j = 0; j < limiteMatriz; j++) {
            cout << setw(8) << fixed << setprecision(2) << mat[i][j];
        }
        cout << "\n";
    }
}

void ejecutarRecomendadorAdaptativo(const Graph& grafo) {
    int n = grafo.getNumNodes();
    bool exigirCota = false;

    if (n > 20) {
        cout << "\nInstancia grande detectada (N = " << n << ").\n";
        cout << "Seleccione criterio empresarial prioritario:\n";
        cout << " [1] Menor costo empirico y maxima velocidad en despacho diario (Nearest Neighbor)\n";
        cout << " [2] Exigir garantia matematica formal Costo <= 2*OPT (MST + DFS)\n";
        cout << "Opcion (1 o 2): ";
        int subOp = 1;
        if (cin >> subOp && subOp == 2) {
            exigirCota = true;
        } else if (!cin) {
            cin.clear();
            cin.ignore(10000, '\n');
        }
    }

    RecomendacionAdaptativa rec = Algorithms::SeleccionAdaptativa(grafo, exigirCota);

    cout << "\n================================================================================\n";
    cout << "          RECOMENDADOR ADAPTATIVO AUTOMATICO - ROUTEOPT (RF11)\n";
    cout << "================================================================================\n";
    cout << " Instancia analizada        : " << grafo.getInstanceName() << " (N = " << n << ")\n";
    cout << " Clasificacion de escala    : " << rec.categoriaEscala << "\n";
    cout << " Cumple Desig. Triangular   : " << (rec.cumpleDesigualdadTriangular ? "SI (TSP Metrico)" : "NO") << "\n";
    cout << " Estrategia seleccionada    : " << rec.algoritmoElegido << "\n";
    cout << " Sustento tecnico           : " << rec.justificacion << "\n";
    cout << "================================================================================\n";

    mostrarResultadoIndividual(rec.resultado);
}

void ejecutarSuiteBenchmarkCSV() {
    struct CasoBenchmark {
        string escenario;
        Graph grafo;
    };

    cout << "\nIniciando Suite de Benchmarking Automatizado (AndesExpress Logistics)...\n";

    vector<CasoBenchmark> casos;
    casos.push_back({"Pequeno", Graph::createPilotInstance(8)});
    try {
        casos.push_back({"Pequeno", Graph::loadFromCSV("data/instancia_pequena.csv", TipoMetrica::EUCLIDIANA)});
    } catch (...) {
        casos.push_back({"Pequeno", Graph::createPilotInstance(10)});
    }

    casos.push_back({"Mediano", Graph::createPilotInstance(12)});
    casos.push_back({"Mediano", Graph::createPilotInstance(15)});
    casos.push_back({"Mediano", Graph::createPilotInstance(18)});

    casos.push_back({"Grande", Graph::generateRandomInstance(30, 42, 100.0)});
    casos.push_back({"Grande", Graph::generateRandomInstance(50, 42, 100.0)});
    casos.push_back({"Grande", Graph::generateRandomInstance(100, 42, 100.0)});

    bool tieneLimaReal = false;
    try {
        Graph gLima = Graph::loadFromCSV("data/lima_real.csv", TipoMetrica::HAVERSINE);
        casos.push_back({"Lima_Real", gLima});
        tieneLimaReal = true;
    } catch (const exception& e) {
        cout << " [Advertencia] No se encontro data/lima_real.csv (" << e.what() << ").\n";
    }

    const string archivoSalida = "resultados_benchmark.csv";
    ofstream csv(archivoSalida);
    if (!csv.is_open()) {
        throw runtime_error("No se pudo crear el archivo " + archivoSalida);
    }

    csv << "Escenario,N,Algoritmo,Distancia_km,Costo_USD,Desviacion_pct,Tiempo_ms,Memoria_KB,Operaciones_Estados\n";
    csv << fixed;

    for (const auto& caso : casos) {
        int n = caso.grafo.getNumNodes();
        cout << " -> Evaluando escenario [" << left << setw(9) << caso.escenario << right
             << "] N = " << setw(3) << n << " (" << caso.grafo.getInstanceName() << ")...\n";

        vector<TSPResult> resultados = evaluarCuatroAlgoritmos(caso.grafo, false);

        for (const auto& res : resultados) {
            if (!res.ejecutado) continue;
            csv << caso.escenario << ","
                << n << ","
                << res.algoritmo << ","
                << setprecision(4) << res.distancia_km << ","
                << setprecision(4) << res.costo_usd << ","
                << setprecision(4) << res.desviacion_pct << ","
                << setprecision(6) << res.tiempo_ms << ","
                << setprecision(2) << res.memoria_kb << ","
                << res.operaciones_o_estados << "\n";
        }

        if (caso.escenario == "Lima_Real") {
            ofstream csvRutas("data/rutas_lima_benchmark.csv");
            if (csvRutas.is_open()) {
                csvRutas << "Algoritmo,Distancia_km,Costo_USD,Desviacion_pct,Tiempo_ms,Ruta\n";
                csvRutas << fixed;
                for (const auto& res : resultados) {
                    if (!res.ejecutado) continue;
                    csvRutas << res.algoritmo << ","
                             << setprecision(4) << res.distancia_km << ","
                             << setprecision(4) << res.costo_usd << ","
                             << setprecision(4) << res.desviacion_pct << ","
                             << setprecision(6) << res.tiempo_ms << ","
                             << formatearRutaGuiones(res.ruta) << "\n";
                }
            }
        }
    }

    csv.close();
    cout << "\n [OK] Benchmark finalizado con exito.\n";
    cout << " [OK] Resultados exportados a: " << archivoSalida << "\n";
    if (tieneLimaReal) {
        cout << " [OK] Rutas geograficas de Lima exportadas a: data/rutas_lima_benchmark.csv\n";
    }
    cout << " [TIP] Ejecute 'python generar_graficas.py' para generar los 4 graficos PNG del informe.\n";
}

void menuCargarInstancia(Graph& grafo) {
    cout << "\n--- Carga de Datos / Generador de Instancias ---\n";
    cout << " [1] Cargar data/lima_real.csv (16 sedes reales en Lima - Haversine km)\n";
    cout << " [2] Cargar data/instancia_pequena.csv (N = 10 - Euclidiana km)\n";
    cout << " [3] Cargar data/instancia_mediana.csv (N = 16 - Euclidiana km)\n";
    cout << " [4] Ingresar ruta de archivo CSV personalizado\n";
    cout << " [5] Generar instancia grande determinista con std::mt19937 (N = 30, 50, 100, 200+)\n";
    cout << "Seleccione una opcion: ";

    int sub = 0;
    if (!(cin >> sub)) {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Entrada invalida.\n";
        return;
    }

    try {
        if (sub == 1) {
            grafo = Graph::loadFromCSV("data/lima_real.csv", TipoMetrica::HAVERSINE);
        } else if (sub == 2) {
            grafo = Graph::loadFromCSV("data/instancia_pequena.csv", TipoMetrica::EUCLIDIANA);
        } else if (sub == 3) {
            grafo = Graph::loadFromCSV("data/instancia_mediana.csv", TipoMetrica::EUCLIDIANA);
        } else if (sub == 4) {
            cout << "Ruta del archivo CSV: ";
            string rutaArchivo;
            cin >> ws;
            getline(cin, rutaArchivo);
            grafo = Graph::loadFromCSV(rutaArchivo, TipoMetrica::AUTO);
        } else if (sub == 5) {
            int nuevoN = 50;
            unsigned int semilla = 42;
            cout << "Cantidad de nodos N (ej. 30, 50, 100, 200): ";
            if (!(cin >> nuevoN) || nuevoN < 2) {
                throw invalid_argument("N debe ser mayor o igual a 2.");
            }
            cout << "Semilla fija para std::mt19937 (ej. 42): ";
            if (!(cin >> semilla)) {
                throw invalid_argument("Semilla invalida.");
            }
            grafo = Graph::generateRandomInstance(nuevoN, semilla, 100.0);
        } else {
            cout << "Opcion cancelada.\n";
            return;
        }

        cout << "\n [OK] Instancia cargada correctamente: " << grafo.getInstanceName()
             << " | N = " << grafo.getNumNodes() << " nodos.\n";
    } catch (const exception& e) {
        cout << "\n [ERROR] " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        // Modo no interactivo por linea de comandos para automatizacion de pruebas
        if (argc > 1 && string(argv[1]) == "--benchmark") {
            ejecutarSuiteBenchmarkCSV();
            return 0;
        }

        Graph grafo;
        try {
            grafo = Graph::loadFromCSV("data/lima_real.csv", TipoMetrica::HAVERSINE);
        } catch (...) {
            grafo = Graph::createPilotInstance(10);
        }

        int opcion = -1;
        while (opcion != 0) {
            int n = grafo.getNumNodes();
            cout << "\n========================================================================\n";
            cout << "     SISTEMA LOGISTICO ROUTEOPT - ANDESEXPRESS LOGISTICS (C++17)\n";
            cout << "========================================================================\n";
            cout << " Instancia activa : " << grafo.getInstanceName() << "\n";
            cout << " Tamano actual    : N = " << n << " nodos (Almacen 0 + " << (n - 1) << " clientes)\n";
            cout << " Metrica activa   : " << (grafo.isGeographic() ? "Haversine (km reales)" : "Euclidiana (km)") << "\n";
            cout << "------------------------------------------------------------------------\n";
            cout << " [1] Ejecutar Backtracking con Poda / Branch & Bound (Exacto, N <= 12)\n";
            cout << " [2] Ejecutar Held-Karp / Prog. Dinamica + Bitmask (Exacto, N <= 20)\n";
            cout << " [3] Ejecutar MST + DFS con Prim (2-Aproximacion, O(N^2))\n";
            cout << " [4] Ejecutar Nearest Neighbor / Vecino Mas Cercano (Voraz, O(N^2))\n";
            cout << " [5] Comparar los 4 algoritmos en la instancia activa\n";
            cout << " [6] Seleccion Adaptativa Automatica (Recomendador Inteligente)\n";
            cout << " [7] Cargar dataset CSV (Lima Real / Pequena / Mediana) o Generar N grande\n";
            cout << " [8] Ver coordenadas, parametros USD, matriz y validacion del grafo\n";
            cout << " [9] Ejecutar Suite de Benchmarking Completa + Exportar CSV\n";
            cout << " [0] Salir\n";
            cout << "========================================================================\n";
            cout << "Seleccione una opcion: ";

            if (!(cin >> opcion)) {
                cin.clear();
                cin.ignore(10000, '\n');
                cout << "Entrada invalida. Ingrese un numero del 0 al 9.\n";
                opcion = -1;
                continue;
            }

            try {
                if (opcion == 1) {
                    mostrarResultadoIndividual(Algorithms::Backtracking(grafo, Algorithms::kMaxSafeBacktracking));
                } else if (opcion == 2) {
                    mostrarResultadoIndividual(Algorithms::HeldKarp(grafo, Algorithms::kMaxSafeHeldKarp));
                } else if (opcion == 3) {
                    mostrarResultadoIndividual(Algorithms::MST_DFS(grafo));
                } else if (opcion == 4) {
                    mostrarResultadoIndividual(Algorithms::NearestNeighbor(grafo, 0));
                } else if (opcion == 5) {
                    mostrarTablaComparativa(grafo, "COMPARATIVA MULTIALGORITMICA");
                } else if (opcion == 6) {
                    ejecutarRecomendadorAdaptativo(grafo);
                } else if (opcion == 7) {
                    menuCargarInstancia(grafo);
                } else if (opcion == 8) {
                    mostrarNodosYMatriz(grafo);
                } else if (opcion == 9) {
                    ejecutarSuiteBenchmarkCSV();
                } else if (opcion == 0) {
                    cout << "Cerrando RouteOpt. Hasta pronto.\n";
                } else {
                    cout << "Opcion fuera de rango (0 a 9).\n";
                }
            } catch (const exception& e) {
                cout << "\n [ERROR DE EJECUCION] " << e.what() << "\n";
            }
        }
    } catch (const exception& fatal) {
        cerr << "[ERROR CRITICO] " << fatal.what() << "\n";
        return 1;
    }

    return 0;
}
