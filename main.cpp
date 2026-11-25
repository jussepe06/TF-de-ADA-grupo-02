#include "Algorithms.h"
#include "Graph.h"
#include "tests/test_routeopt.h"

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

string formatearMemoriaLegible(double kb) {
    if (isinf(kb) || kb >= 1024.0 * 1024.0 * 1024.0) {
        return "> 1 TB (Inviable)";
    }
    ostringstream oss;
    oss << fixed;
    if (kb >= 1024.0 * 1024.0) {
        oss << setprecision(2) << (kb / (1024.0 * 1024.0)) << " GB";
    } else if (kb >= 1024.0) {
        oss << setprecision(2) << (kb / 1024.0) << " MB (" << setprecision(0) << kb << " KB)";
    } else {
        oss << setprecision(2) << kb << " KB";
    }
    return oss.str();
}

string formatearTiempoViaje(double horas) {
    int totalMin = static_cast<int>(round(horas * 60.0));
    int h = totalMin / 60;
    int m = totalMin % 60;
    ostringstream oss;
    oss << h << " h " << setw(2) << setfill('0') << m << " min";
    return oss.str();
}

void mostrarResultadoIndividual(const Graph& grafo, const TSPResult& res) {
    cout << "\n================================================================================\n";
    cout << " REPORTE DETALLADO DE EJECUCION — " << res.algoritmo << "\n";
    cout << "================================================================================\n";
    cout << " Clasificacion         : " << res.tipo_solucion_str << "\n";

    if (!res.ejecutado) {
        cout << " Estado                : [OMITIDO POR PROTECCION OPERATIVA]\n";
        cout << " Explicacion           : " << res.motivo_omision << "\n";
        cout << "================================================================================\n";
        return;
    }

    DesgloseCostoRuta desg = grafo.desglosarCostoRuta(res.ruta);

    cout << "\n [1] METRICAS LOGISTICAS Y ECONOMICAS (ANDESEXPRESS)\n";
    cout << "  - Distancia total recorrida : " << fixed << setprecision(2) << res.distancia_km << " km\n";
    cout << "  - Tiempo estimado de manejo : " << formatearTiempoViaje(desg.tiempoConduccionHoras)
         << " (" << setprecision(2) << desg.tiempoConduccionHoras << " h a "
         << setprecision(0) << grafo.getParametrosCosto().velocidadPromKmH << " km/h)\n";
    cout << "  - Costo operativo total     : $" << setprecision(2) << res.costo_usd << " USD\n";
    cout << "      * Combustible (" << setprecision(2) << desg.litrosCombustible << " L) : $" << desg.costoCombustibleUSD << " USD\n";
    cout << "      * Horas de chofer       : $" << desg.costoChoferUSD << " USD\n";
    cout << "      * Paradas (" << desg.clientesVisitados << " clientes) : $" << desg.costoParadasUSD << " USD\n";

    cout << "\n [2] CALIDAD DE LA SOLUCION (QUE TAN BUENA ES LA RUTA)\n";
    if (res.desviacion_valida) {
        if (res.desviacion_pct < 1e-6) {
            cout << "  - Nivel de optimalidad      : 0.00% de error -> ES LA RUTA OPTIMA MATEMATICA GLOBAL.\n";
        } else {
            cout << "  - Error vs. Optimo Exacto   : +" << fixed << setprecision(2) << res.desviacion_pct
                 << "% mas larga que el optimo exacto";
            if (res.sobrecosto_usd_vs_mejor > 0.0) {
                cout << " (+$" << res.sobrecosto_usd_vs_mejor << " USD extra)";
            }
            cout << ".\n";
        }
    } else {
        double cotaMST = (res.cota_inferior_mst_km > 0.0)
                             ? res.cota_inferior_mst_km
                             : (grafo.checkTriangularInequality() ? Algorithms::calcularCotaInferiorMST(grafo) : 0.0);
        if (res.tipo_referencia == TipoReferencia::MEJOR_OBSERVADO) {
            cout << "  - Comparacion entre metodos : +" << fixed << setprecision(2)
                 << res.diferencia_mejor_observada_pct << "% respecto a la mejor heuristica observada.\n";
        }
        if (cotaMST > 0.0) {
            double brechaMax = max(0.0, ((res.distancia_km - cotaMST) / cotaMST) * 100.0);
            cout << "  - Cota Inferior MST (teor.) : " << fixed << setprecision(2) << cotaMST
                 << " km (ninguna ruta posible baja de este valor).\n";
            cout << "  - Garantia real calculada   : Esta ruta esta como MAXIMO a +" << setprecision(2)
                 << brechaMax << "% del optimo desconocido.\n";
        } else {
            cout << "  - Referencia de optimalidad : Sin optimo exacto ni cota metrica aplicable.\n";
        }
    }

    cout << "\n [3] RENDIMIENTO COMPUTACIONAL (TIEMPO, RAM Y ESFUERZO)\n";
    cout << "  - Tiempo de CPU             : " << fixed << setprecision(4) << res.tiempo_ms
         << " ms (" << setprecision(2) << res.tiempo_us << " microsegundos)\n";
    cout << "  - Memoria del algoritmo     : " << formatearMemoriaLegible(res.memoria_auxiliar_kb)
         << " (estructuras auxiliares propias)\n";
    cout << "  - Memoria total del proceso : " << formatearMemoriaLegible(res.memoria_proceso_kb) << " en Windows\n";
    cout << "  - Esfuerzo algoritmico      : " << res.detalle_operaciones << "\n";

    cout << "\n [4] VALIDACION Y SECUENCIA DE VISITAS\n";
    cout << "  - Auditoria del ciclo TSP   : "
         << (res.ruta_valida ? "[OK] Ciclo Hamiltoniano valido (inicia en 0, sin repetidos, retorna a 0)"
                             : ("[FALLO] " + res.error_validacion_ruta)) << "\n";
    cout << "  - Orden de paradas          : " << formatearRuta(res.ruta, 25) << "\n";
    cout << "================================================================================\n";
}

vector<TSPResult> evaluarCuatroAlgoritmos(const Graph& grafo, bool imprimirAvisos = true) {
    vector<TSPResult> todos = {
        Algorithms::Backtracking(grafo, Algorithms::kMaxSafeBacktracking),
        Algorithms::HeldKarp(grafo, Algorithms::kMaxSafeHeldKarp, 512.0),
        Algorithms::MST_DFS(grafo),
        Algorithms::NearestNeighbor(grafo, 0)
    };

    double cotaMST = todos[2].cota_inferior_mst_km;
    Algorithms::clasificarYCalcularDesviaciones(todos, cotaMST);

    if (imprimirAvisos) {
        for (const auto& r : todos) {
            if (!r.ejecutado) {
                cout << " [Proteccion de Escala] " << r.algoritmo << " -> " << r.motivo_omision << "\n";
            }
        }
    }

    return todos;
}

void mostrarTablaComparativa(const Graph& grafo, const string& titulo) {
    int n = grafo.getNumNodes();
    ValidacionGrafo v = grafo.validateGraphDetailed(true);

    cout << "\n===================================================================================================================\n";
    cout << "  " << titulo << " | Instancia: " << grafo.getInstanceName() << "\n";
    cout << "  Tamano: N = " << n << " (Almacen 0 + " << (n - 1) << " clientes) | Distancia: "
         << (grafo.isGeographic() ? "Haversine (km reales)" : "Euclidiana (km)")
         << " | TSP Metrico: " << (v.cumpleDesigualdadTriangular ? "SI" : "NO") << "\n";
    if (!v.esValido) {
        cout << "===================================================================================================================\n";
        cout << " [ERROR] La instancia actual no es estructuralmente valida. Use la opcion [9] para revisar los errores.\n";
        return;
    }
    cout << "===================================================================================================================\n";

    vector<TSPResult> resultados = evaluarCuatroAlgoritmos(grafo, true);

    bool hayOptimoExacto = false;
    const TSPResult* mejorRuta = nullptr;
    const TSPResult* peorRuta = nullptr;
    const TSPResult* masRapido = nullptr;
    const TSPResult* masLento = nullptr;
    const TSPResult* mayorMemoria = nullptr;

    for (const auto& r : resultados) {
        if (!r.ejecutado) continue;
        if (r.tipo_referencia == TipoReferencia::OPTIMO_EXACTO) hayOptimoExacto = true;
        if (!mejorRuta || r.distancia_km < mejorRuta->distancia_km) mejorRuta = &r;
        if (!peorRuta || r.distancia_km > peorRuta->distancia_km) peorRuta = &r;
        if (!masRapido || r.tiempo_ms < masRapido->tiempo_ms) masRapido = &r;
        if (!masLento || r.tiempo_ms > masLento->tiempo_ms) masLento = &r;
        if (!mayorMemoria || r.memoria_auxiliar_kb > mayorMemoria->memoria_auxiliar_kb) mayorMemoria = &r;
    }

    cout << left << setw(20) << "Algoritmo"
         << setw(15) << "Garantia"
         << right << setw(10) << "Dist(km)"
         << setw(11) << "Costo($)"
         << setw(13) << (hayOptimoExacto ? "Error s/OPT" : "Dif s/Mejor")
         << setw(12) << "Tiempo(ms)"
         << setw(12) << "Speedup"
         << setw(13) << "RAM Algo(KB)"
         << setw(13) << "Operaciones"
         << "  " << left << "Ciclo\n";
    cout << "-------------------------------------------------------------------------------------------------------------------\n";

    cout << fixed;
    for (const auto& res : resultados) {
        if (!res.ejecutado) continue;

        ostringstream ossBrecha;
        if (res.desviacion_valida) {
            ossBrecha << (res.desviacion_pct < 1e-6 ? "0.00% (OPT)" : ("+" + (ostringstream() << fixed << setprecision(2) << res.desviacion_pct).str() + "%"));
        } else {
            if (res.diferencia_mejor_observada_pct < 1e-6) {
                ossBrecha << "Mejor obs.";
            } else {
                ossBrecha << "+" << setprecision(2) << res.diferencia_mejor_observada_pct << "%";
            }
        }

        ostringstream ossSpeedup;
        ossSpeedup << fixed;
        if (res.speedup_vs_mas_lento >= 100.0) {
            ossSpeedup << setprecision(0) << res.speedup_vs_mas_lento << "x";
        } else {
            ossSpeedup << setprecision(1) << res.speedup_vs_mas_lento << "x";
        }

        string garantiaCorta = (res.tipo_solucion == TipoSolucion::EXACTA)
                                   ? "Exacto (0%)"
                                   : ((res.tipo_solucion == TipoSolucion::APROXIMADA) ? "Cota <=2*OPT" : "Voraz s/cota");

        cout << left << setw(20) << res.algoritmo
             << setw(15) << garantiaCorta
             << right << setw(10) << setprecision(2) << res.distancia_km
             << setw(11) << setprecision(2) << res.costo_usd
             << setw(13) << ossBrecha.str()
             << setw(12) << setprecision(4) << res.tiempo_ms
             << setw(12) << ossSpeedup.str()
             << setw(13) << setprecision(2) << res.memoria_auxiliar_kb
             << setw(13) << res.operaciones_o_estados
             << "  " << (res.ruta_valida ? "[OK]" : "[ERR]") << "\n";
    }
    cout << "===================================================================================================================\n";

    // Bloque de interpretacion en lenguaje claro para exposicion y analisis experimental
    if (mejorRuta && peorRuta && masRapido && masLento && mayorMemoria) {
        DesgloseCostoRuta desgMejor = grafo.desglosarCostoRuta(mejorRuta->ruta);
        double ahorroKm = max(0.0, peorRuta->distancia_km - mejorRuta->distancia_km);
        double ahorroUSD = max(0.0, peorRuta->costo_usd - mejorRuta->costo_usd);

        cout << " INTERPRETACION EXPERIMENTAL FACIL DE LEER:\n";
        cout << "  1. Mejor ruta hallada   : " << mejorRuta->algoritmo << " con "
             << setprecision(2) << mejorRuta->distancia_km << " km y $" << mejorRuta->costo_usd
             << " USD (" << formatearTiempoViaje(desgMejor.tiempoConduccionHoras) << " de conduccion;\n"
             << "                            $" << desgMejor.costoCombustibleUSD << " combustible + $"
             << desgMejor.costoChoferUSD << " chofer + $" << desgMejor.costoParadasUSD << " en paradas).\n";
        cout << "  2. Ahorro logistico     : Elegir " << mejorRuta->algoritmo << " frente a " << peorRuta->algoritmo
             << " ahorra " << ahorroKm << " km y $" << ahorroUSD << " USD por viaje\n"
             << "                            (~$" << (ahorroUSD * 30.0) << " USD/mes considerando 30 despachos).\n";
        cout << "  3. Velocidad (Speedup)  : " << masRapido->algoritmo << " (" << setprecision(4)
             << masRapido->tiempo_ms << " ms) fue " << setprecision(1) << masRapido->speedup_vs_mas_lento
             << " veces mas rapido que " << masLento->algoritmo << " (" << setprecision(4)
             << masLento->tiempo_ms << " ms).\n";
        cout << "  4. Consumo de memoria   : El mayor uso auxiliar fue de " << mayorMemoria->algoritmo
             << " con " << formatearMemoriaLegible(mayorMemoria->memoria_auxiliar_kb) << ".\n";

        if (hayOptimoExacto) {
            cout << "  5. Como leer 'Error'    : Como se ejecuto un metodo exacto, 0.00% (OPT) es el optimo matematico global comprobado.\n";
        } else {
            cout << "  5. Como leer 'Dif'      : Con N=" << n << " (> 20) el optimo exacto es incomputable; se compara contra la mejor ruta hallada.";
            if (mejorRuta->cota_inferior_mst_km > 0.0) {
                cout << "\n                            Ademas, usando el peso del MST (" << setprecision(2)
                     << mejorRuta->cota_inferior_mst_km << " km <= OPT), se garantiza matematicamente que\n"
                     << "                            " << mejorRuta->algoritmo << " esta como maximo a +"
                     << mejorRuta->diferencia_cota_inferior_pct << "% del optimo real desconocido.";
            }
            cout << "\n";
        }
        cout << "  6. Ruta ganadora        : " << formatearRuta(mejorRuta->ruta, 18) << "\n";
        cout << "===================================================================================================================\n";
    }
}

void mostrarDiagnosticoYMatriz(const Graph& grafo) {
    int n = grafo.getNumNodes();
    const auto& coords = grafo.getCoordinates();
    const auto& mat = grafo.getMatrix();
    const auto& params = grafo.getParametrosCosto();
    ValidacionGrafo v = grafo.validateGraphDetailed(true);

    cout << "\n================================================================================\n";
    cout << "     REPORTE DE VALIDACION DE INSTANCIA Y DIAGNOSTICO METRICO (RF04)\n";
    cout << "================================================================================\n";
    cout << " Instancia          : " << grafo.getInstanceName() << "\n";
    cout << " Nodos registrados  : N = " << n << " (Nodo 0 = Almacen Central + " << max(0, n - 1) << " clientes)\n";
    cout << " Modelo geometrico  : " << (grafo.isGeographic() ? "Geografico Real (Haversine en km)" : "Plano Cartesiano (Euclidiana en km)") << "\n";
    cout << " Tarifas operativas : Combustible $" << fixed << setprecision(2) << params.precioCombustible
         << "/L (" << params.rendimientoKmL << " km/L) | Chofer $" << params.costoHoraChofer
         << "/h (" << params.velocidadPromKmH << " km/h) | Parada $" << params.costoParadaCliente << "\n";

    cout << "\n [CHECKLIST DE INTEGRIDAD ESTRUCTURAL Y METRICA]\n";
    cout << "  1. Cantidad minima operativa (N >= 2)        : " << (n >= 2 ? "[CUMPLE]" : "[FALLO]") << "\n";
    cout << "  2. Almacen Central (ID=0) e IDs unicos       : " << ((v.tieneAlmacenCentral && v.idsValidos) ? "[CUMPLE]" : "[FALLO]") << "\n";
    cout << "  3. Valores finitos y rango de coordenadas    : " << ((v.sinNaNs && v.coordsValidas) ? "[CUMPLE]" : "[FALLO]") << "\n";
    cout << "  4. Matriz cuadrada NxN y diagonal en 0.0 km  : " << ((v.dimensionesValidas && v.diagonalCero) ? "[CUMPLE]" : "[FALLO]") << "\n";
    cout << "  5. Red conexa completa y costos >= 0         : " << ((v.conexo && v.noNegativo) ? "[CUMPLE]" : "[FALLO]") << "\n";
    cout << "  6. Simetria de tramos d(i,j) == d(j,i)       : " << (v.simetrico ? "[CUMPLE] (TSP Simetrico)" : "[FALLO] (Asimetrico)") << "\n";
    cout << "  7. Ubicaciones sin solapamiento (d > 0)      : "
         << (v.paresCoincidentes == 0 ? "[CUMPLE] (Sin puntos duplicados)" : ("[AVISO] " + to_string(v.paresCoincidentes) + " par(es) en misma coordenada")) << "\n";
    cout << "  8. Desigualdad Triangular d(i,j)<=d(i,k)+d(k,j): "
         << (v.cumpleDesigualdadTriangular
                 ? ("[CUMPLE] (" + to_string(v.ternasEvaluadas) + " ternas verificadas -> Habilita garantia 2*OPT)")
                 : ("[NO CUMPLE] (" + to_string(v.violacionesTriangulares) + " violaciones -> MST+DFS sin cota 2*OPT)")) << "\n";

    if (!v.errores.empty()) {
        cout << "\n [ERRORES DETECTADOS]\n";
        for (const auto& err : v.errores) cout << "  -> " << err << "\n";
    }
    if (!v.advertencias.empty()) {
        cout << "\n [ADVERTENCIAS TECNICAS]\n";
        for (const auto& adv : v.advertencias) cout << "  -> " << adv << "\n";
    }

    if (v.esValido) {
        cout << "\n [PERFIL ESTADISTICO DE DISTANCIAS DE LA RED]\n";
        cout << "  - Tramo mas corto (> 0)         : " << fixed << setprecision(2) << v.distanciaMinKm << " km\n";
        cout << "  - Distancia promedio entre pares: " << v.distanciaPromedioKm << " km\n";
        cout << "  - Tramo mas largo de la red     : " << v.distanciaMaxKm << " km\n";
        cout << "  - Cliente mas lejano al Almacen : " << v.radioMaxAlmacenKm << " km\n";
        cout << "  - Viabilidad de algoritmos hoy  : "
             << "Backtracking [" << (n <= Algorithms::kMaxSafeBacktracking ? "VIABLE" : "OMITIDO N>12") << "] | "
             << "Held-Karp [" << (n <= Algorithms::kMaxSafeHeldKarp ? ("VIABLE ~" + formatearMemoriaLegible(Algorithms::estimarMemoriaHeldKarpKB(n))) : "OMITIDO N>20") << "] | "
             << "MST+DFS y Voraz [VIABLES]\n";
    }

    int limiteCoords = min(n, 20);
    cout << "\n--- Ubicaciones Cargadas (mostrando " << limiteCoords << " de " << n << ") ---\n";
    for (int i = 0; i < limiteCoords; i++) {
        cout << " [" << setw(3) << coords[i].id << "] " << left << setw(40) << coords[i].nombre
             << right << " (" << fixed << setprecision(4) << coords[i].x
             << ", " << setprecision(4) << coords[i].y << ")\n";
    }
    if (n > limiteCoords) {
        cout << " ... (" << (n - limiteCoords) << " ubicaciones adicionales omitidas)\n";
    }

    int limiteMatriz = min(n, 10);
    cout << "\n--- Submatriz de Distancias en km (" << limiteMatriz << "x" << limiteMatriz << ") ---\n";
    cout << setw(6) << " ";
    for (int j = 0; j < limiteMatriz; j++) cout << setw(8) << ("N" + to_string(j));
    cout << "\n";
    for (int i = 0; i < limiteMatriz; i++) {
        cout << setw(5) << ("N" + to_string(i)) << " ";
        for (int j = 0; j < limiteMatriz; j++) {
            cout << setw(8) << fixed << setprecision(2) << mat[i][j];
        }
        cout << "\n";
    }
}

void menuGestionUbicaciones(Graph& grafo) {
    int op = -1;
    while (op != 0) {
        int n = grafo.getNumNodes();
        cout << "\n--------------------------------------------------------------\n";
        cout << "           GESTION DINAMICA DE UBICACIONES (RF02)\n";
        cout << "--------------------------------------------------------------\n";
        cout << " Nodos actuales: " << n << " | Instancia: " << grafo.getInstanceName() << "\n";
        cout << " [1] Listar todas las ubicaciones registradas\n";
        cout << " [2] Registrar / Actualizar Almacen Central (Nodo 0)\n";
        cout << " [3] Registrar nuevo Cliente (Nodo " << n << ")\n";
        cout << " [4] Modificar datos de una ubicacion por ID\n";
        cout << " [5] Eliminar un cliente por ID (N > 2)\n";
        cout << " [6] Alternar modo de distancia (Euclidiana / Geografica)\n";
        cout << " [0] Regresar al menu principal\n";
        cout << "--------------------------------------------------------------\n";
        cout << "Seleccione una opcion: ";

        if (!(cin >> op)) {
            if (cin.eof()) return;
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Entrada invalida.\n";
            op = -1;
            continue;
        }

        try {
            if (op == 1) {
                const auto& coords = grafo.getCoordinates();
                cout << "\n--- Ubicaciones Registradas (" << coords.size() << ") ---\n";
                for (const auto& p : coords) {
                    cout << " ID [" << setw(3) << p.id << "] " << left << setw(35) << p.nombre
                         << " (X/Lat: " << fixed << setprecision(4) << p.x
                         << ", Y/Lon: " << setprecision(4) << p.y << ")\n";
                }
            } else if (op == 2) {
                cout << "Nombre del Almacen Central: ";
                string nombre;
                cin >> ws;
                getline(cin, nombre);
                double x = 0, y = 0;
                cout << "Coordenada X (o Latitud): ";
                cin >> x;
                cout << "Coordenada Y (o Longitud): ";
                cin >> y;
                grafo.setWarehouse(nombre, x, y);
                cout << " [OK] Almacen Central actualizado y grafo revalidado: "
                     << (grafo.validateGraph() ? "VALIDO" : "REVISAR COORDENADAS") << ".\n";
            } else if (op == 3) {
                cout << "Nombre del nuevo cliente: ";
                string nombre;
                cin >> ws;
                getline(cin, nombre);
                double x = 0, y = 0;
                cout << "Coordenada X (o Latitud): ";
                cin >> x;
                cout << "Coordenada Y (o Longitud): ";
                cin >> y;
                int nuevoId = grafo.addNode(nombre, x, y);
                cout << " [OK] Cliente registrado con ID " << nuevoId
                     << ". Total nodos: " << grafo.getNumNodes() << ".\n";
            } else if (op == 4) {
                cout << "ID del nodo a modificar (0 a " << (n - 1) << "): ";
                int idMod = -1;
                cin >> idMod;
                Punto p;
                if (!grafo.getNode(idMod, p)) {
                    cout << " [ERROR] ID no encontrado.\n";
                    continue;
                }
                cout << "Modificando Nodo " << idMod << " (" << p.nombre << ")\n";
                cout << "Nuevo nombre (dejar vacio para conservar): ";
                string nuevoNom;
                cin.ignore(10000, '\n');
                getline(cin, nuevoNom);
                if (nuevoNom.empty()) nuevoNom = p.nombre;
                double nx = 0, ny = 0;
                cout << "Nueva coordenada X (o Latitud) [Actual: " << p.x << "]: ";
                cin >> nx;
                cout << "Nueva coordenada Y (o Longitud) [Actual: " << p.y << "]: ";
                cin >> ny;
                if (grafo.updateNode(idMod, nuevoNom, nx, ny)) {
                    cout << " [OK] Nodo actualizado y matriz reconstruida.\n";
                } else {
                    cout << " [ERROR] No se pudo modificar el nodo.\n";
                }
            } else if (op == 5) {
                cout << "ID del cliente a eliminar (1 a " << (n - 1) << "): ";
                int idElim = -1;
                cin >> idElim;
                if (idElim == 0) {
                    cout << " [ERROR] El Almacen Central (ID 0) no puede eliminarse.\n";
                    continue;
                }
                if (grafo.removeNode(idElim)) {
                    cout << " [OK] Cliente eliminado. Nodos restantes: " << grafo.getNumNodes() << "\n";
                } else {
                    cout << " [ERROR] No se pudo eliminar (verifique ID o N > 2).\n";
                }
            } else if (op == 6) {
                bool nuevoModo = !grafo.isGeographic();
                grafo.setGeographic(nuevoModo);
                grafo.rebuildDistanceMatrix();
                ValidacionGrafo v = grafo.validateGraphDetailed(false);
                cout << " [OK] Modo cambiado a "
                     << (nuevoModo ? "Geografico (Haversine km)" : "Euclidiano Plano (km)")
                     << " | Estado: " << (v.esValido ? "VALIDO" : "CON ERRORES DE RANGO") << ".\n";
            }
        } catch (const exception& e) {
            cout << " [ERROR] " << e.what() << "\n";
        }
    }
}

void menuConfiguracionParametros(Graph& grafo) {
    ParametrosCosto p = grafo.getParametrosCosto();
    cout << "\n--------------------------------------------------------------\n";
    cout << "       PARAMETROS DEL MODELO DE COSTO OPERATIVO (RF06)\n";
    cout << "--------------------------------------------------------------\n";
    cout << " [1] Precio Combustible (Pc)      : $" << fixed << setprecision(2) << p.precioCombustible << " USD/L\n";
    cout << " [2] Rendimiento Vehiculo (R)     : " << p.rendimientoKmL << " km/L\n";
    cout << " [3] Velocidad Promedio (V_prom)  : " << p.velocidadPromKmH << " km/h\n";
    cout << " [4] Costo Chofer por Hora (Co)   : $" << p.costoHoraChofer << " USD/h\n";
    cout << " [5] Costo Parada Cliente (Cp)    : $" << p.costoParadaCliente << " USD/cliente\n";
    cout << " [6] Modificar todos los parametros\n";
    cout << " [0] Regresar sin cambios\n";
    cout << "Seleccione opcion: ";

    int op = 0;
    if (!(cin >> op) || op == 0) {
        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
        }
        return;
    }

    try {
        if (op == 1) {
            cout << "Nuevo Pc (USD/L): "; cin >> p.precioCombustible;
        } else if (op == 2) {
            cout << "Nuevo R (km/L): "; cin >> p.rendimientoKmL;
        } else if (op == 3) {
            cout << "Nuevo V_prom (km/h): "; cin >> p.velocidadPromKmH;
        } else if (op == 4) {
            cout << "Nuevo Co (USD/h): "; cin >> p.costoHoraChofer;
        } else if (op == 5) {
            cout << "Nuevo Cp (USD/cliente): "; cin >> p.costoParadaCliente;
        } else if (op == 6) {
            cout << "Pc (USD/L): "; cin >> p.precioCombustible;
            cout << "R (km/L): "; cin >> p.rendimientoKmL;
            cout << "V_prom (km/h): "; cin >> p.velocidadPromKmH;
            cout << "Co (USD/h): "; cin >> p.costoHoraChofer;
            cout << "Cp (USD/cliente): "; cin >> p.costoParadaCliente;
        }
        grafo.setParametrosCosto(p);
        cout << " [OK] Parametros de costo actualizados.\n";
    } catch (const exception& e) {
        cout << " [ERROR] " << e.what() << "\n";
    }
}

void ejecutarRecomendadorAdaptativo(const Graph& grafo) {
    int n = grafo.getNumNodes();
    ConfiguracionAdaptativa cfg;

    cout << "\n----------------------------------------------------------------------\n";
    cout << "        MOTOR DE SELECCION ADAPTATIVA DE ESTRATEGIA (RF11)\n";
    cout << "----------------------------------------------------------------------\n";
    cout << " Instancia activa: " << grafo.getInstanceName() << " (N = " << n << " nodos)\n";
    cout << " Seleccione el objetivo de negocio:\n";
    cout << "  [1] Balanceado Automatico (Recomendado: mejor calidad posible segun N y RAM)\n";
    cout << "  [2] Exigir Optimo Exacto (0% error; degrada con seguridad si excede RAM/tiempo)\n";
    cout << "  [3] Exigir Garantia Matematica Formal (MST + DFS: Costo <= 2 * OPT)\n";
    cout << "  [4] Maxima Velocidad de Respuesta (Despacho inmediato en microsegundos)\n";
    cout << "  [5] Simular restriccion estricta de Memoria RAM (configurar MB maximos)\n";
    cout << "Opcion (1-5, defecto 1): ";

    int subOp = 1;
    if (cin >> subOp) {
        if (subOp == 2) cfg.criterio = CriterioPrioridad::OPTIMO_EXACTO;
        else if (subOp == 3) cfg.criterio = CriterioPrioridad::GARANTIA_TEORICA;
        else if (subOp == 4) cfg.criterio = CriterioPrioridad::MAXIMA_VELOCIDAD;
        else if (subOp == 5) {
            cfg.criterio = CriterioPrioridad::BALANCEADO;
            cout << "Presupuesto maximo de RAM permitido en MB (ej. 5, 50, 256): ";
            double mb = 512.0;
            if (cin >> mb && mb > 0.0) {
                cfg.memoriaMaximaMB = mb;
            }
        }
    } else {
        cin.clear();
        cin.ignore(10000, '\n');
    }

    RecomendacionAdaptativa rec = Algorithms::SeleccionAdaptativa(grafo, cfg);

    cout << "\n========================================================================================================\n";
    cout << "                    MATRIZ DE DECISION DEL SELECTOR ADAPTATIVO — ROUTEOPT\n";
    cout << "========================================================================================================\n";
    cout << " Validacion previa del grafo : " << (rec.instanciaValida ? "[APROBADA]" : "[RECHAZADA]")
         << " | Desigualdad Triangular: " << (rec.cumpleDesigualdadTriangular ? "SI (TSP Metrico)" : "NO")
         << " | Presupuesto RAM: " << fixed << setprecision(0) << cfg.memoriaMaximaMB << " MB\n";
    cout << " Clasificacion de instancia  : " << rec.categoriaEscala << "\n";
    if (rec.huboDegradacionPorRecursos) {
        cout << " Aviso de adaptabilidad      : [FALLBACK ACTIVO] Se aplico degradacion segura por limite de escala/recursos.\n";
    }
    cout << "--------------------------------------------------------------------------------------------------------\n";
    cout << left << setw(21) << "Candidato"
         << setw(14) << "Complejidad"
         << right << setw(15) << "RAM Estimada"
         << "   " << left << setw(22) << "Garantia"
         << "Veredicto y Razon\n";
    cout << "--------------------------------------------------------------------------------------------------------\n";

    for (const auto& c : rec.evaluacionCandidatos) {
        cout << left << setw(21) << c.algoritmo
             << setw(14) << c.complejidad
             << right << setw(15) << formatearMemoriaLegible(c.memoriaEstimadaKB)
             << "   " << left << setw(22) << c.garantiaCalidad
             << c.razonDecision << "\n";
    }
    cout << "--------------------------------------------------------------------------------------------------------\n";
    cout << " ALGORITMO ELEGIDO  : " << rec.algoritmoElegido << "\n";
    cout << " SUSTENTO TECNICO   : " << rec.justificacion << "\n";
    cout << "========================================================================================================\n";

    mostrarResultadoIndividual(grafo, rec.resultado);
}

void ejecutarSuiteBenchmarkCSV() {
    struct CasoBenchmark {
        string escenario;
        Graph grafo;
    };

    cout << "\n========================================================================\n";
    cout << "  SUITE DE BENCHMARKING EXPERIMENTAL — ANDESEXPRESS LOGISTICS\n";
    cout << "========================================================================\n";

    vector<CasoBenchmark> casos;
    casos.push_back({"Pequeno_8", Graph::createPilotInstance(8)});
    try {
        casos.push_back({"Pequeno_10", Graph::loadFromCSV("data/instancia_pequena.csv", TipoMetrica::EUCLIDIANA)});
    } catch (...) {
        casos.push_back({"Pequeno_10", Graph::createPilotInstance(10)});
    }

    casos.push_back({"Mediano_12", Graph::createPilotInstance(12)});
    casos.push_back({"Mediano_15", Graph::createPilotInstance(15)});
    casos.push_back({"Mediano_18", Graph::createPilotInstance(18)});
    casos.push_back({"Mediano_20", Graph::createPilotInstance(20)});

    casos.push_back({"Grande_30", Graph::generateRandomInstance(30, 42, 100.0)});
    casos.push_back({"Grande_50", Graph::generateRandomInstance(50, 42, 100.0)});
    casos.push_back({"Grande_100", Graph::generateRandomInstance(100, 42, 100.0)});
    casos.push_back({"Grande_200", Graph::generateRandomInstance(200, 42, 100.0)});

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

    csv << "Escenario,N,Algoritmo,Tipo_Solucion,Distancia_km,Costo_USD,Sobrecosto_USD,"
           "Tipo_Referencia,Desviacion_pct,Dif_Mejor_Observado_pct,Cota_MST_km,Brecha_Max_MST_pct,"
           "Tiempo_ms,Tiempo_Min_ms,Tiempo_Max_ms,Speedup_x,Memoria_KB,"
           "Memoria_Aux_KB,Memoria_Proceso_KB,Operaciones_Estados,Ruta_Valida,Repeticiones\n";
    csv << fixed;

    const int kRepeticiones = 3;

    for (const auto& caso : casos) {
        int n = caso.grafo.getNumNodes();
        cout << " -> Evaluando [" << left << setw(11) << caso.escenario << right
             << "] N = " << setw(3) << n << " (" << caso.grafo.getInstanceName()
             << ") | Metrico: " << (caso.grafo.checkTriangularInequality() ? "SI" : "NO") << "...\n";

        vector<TSPResult> resultadosBase = evaluarCuatroAlgoritmos(caso.grafo, false);

        for (const auto& res : resultadosBase) {
            if (!res.ejecutado) continue;

            double sumaTiempos = res.tiempo_ms;
            double minTiempo = res.tiempo_ms;
            double maxTiempo = res.tiempo_ms;

            for (int rep = 1; rep < kRepeticiones; rep++) {
                TSPResult rRep;
                if (res.algoritmo.find("Backtracking") != string::npos) {
                    rRep = Algorithms::Backtracking(caso.grafo, Algorithms::kMaxSafeBacktracking);
                } else if (res.algoritmo.find("Held-Karp") != string::npos) {
                    rRep = Algorithms::HeldKarp(caso.grafo, Algorithms::kMaxSafeHeldKarp, 512.0);
                } else if (res.algoritmo.find("MST") != string::npos) {
                    rRep = Algorithms::MST_DFS(caso.grafo);
                } else {
                    rRep = Algorithms::NearestNeighbor(caso.grafo, 0);
                }
                if (rRep.ejecutado) {
                    sumaTiempos += rRep.tiempo_ms;
                    minTiempo = min(minTiempo, rRep.tiempo_ms);
                    maxTiempo = max(maxTiempo, rRep.tiempo_ms);
                }
            }

            double promedioTiempo = sumaTiempos / kRepeticiones;

            csv << caso.escenario << ","
                << n << ","
                << res.algoritmo << ","
                << res.tipo_solucion_str << ","
                << setprecision(4) << res.distancia_km << ","
                << setprecision(4) << res.costo_usd << ","
                << setprecision(4) << res.sobrecosto_usd_vs_mejor << ","
                << res.tipo_referencia_str << ",";

            if (res.desviacion_valida) {
                csv << setprecision(4) << res.desviacion_pct << ",";
            } else {
                csv << "N/A,";
            }

            csv << setprecision(4) << res.diferencia_mejor_observada_pct << ","
                << setprecision(4) << res.cota_inferior_mst_km << ","
                << setprecision(4) << res.diferencia_cota_inferior_pct << ","
                << setprecision(6) << promedioTiempo << ","
                << setprecision(6) << minTiempo << ","
                << setprecision(6) << maxTiempo << ","
                << setprecision(2) << res.speedup_vs_mas_lento << ","
                << setprecision(2) << res.memoria_kb << ","
                << setprecision(2) << res.memoria_auxiliar_kb << ","
                << setprecision(2) << res.memoria_proceso_kb << ","
                << res.operaciones_o_estados << ","
                << (res.ruta_valida ? "SI" : "NO") << ","
                << kRepeticiones << "\n";
        }

        if (caso.escenario == "Lima_Real") {
            ofstream csvRutas("data/rutas_lima_benchmark.csv");
            if (csvRutas.is_open()) {
                csvRutas << "Algoritmo,Distancia_km,Costo_USD,Desviacion_pct,Tiempo_ms,Ruta\n";
                csvRutas << fixed;
                for (const auto& res : resultadosBase) {
                    if (!res.ejecutado) continue;
                    double dRep = res.desviacion_valida ? res.desviacion_pct : 0.0;
                    csvRutas << res.algoritmo << ","
                             << setprecision(4) << res.distancia_km << ","
                             << setprecision(4) << res.costo_usd << ","
                             << setprecision(4) << dRep << ","
                             << setprecision(6) << res.tiempo_ms << ","
                             << formatearRutaGuiones(res.ruta) << "\n";
                }
            }
        }
    }

    csv.close();
    cout << "\n [OK] Benchmark finalizado exitosamente.\n";
    cout << " [OK] Tabla experimental clara exportada en: " << archivoSalida << "\n";
    if (tieneLimaReal) {
        cout << " [OK] Rutas de Lima exportadas en: data/rutas_lima_benchmark.csv\n";
    }
    cout << " [TIP] Ejecute 'python generar_graficas.py' para actualizar las 4 imagenes PNG del informe.\n";
}

void menuCargarInstancia(Graph& grafo) {
    cout << "\n--- Carga y Validacion Automatica de Instancias ---\n";
    cout << " [1] Cargar data/lima_real.csv (16 sedes reales en Lima - Haversine km)\n";
    cout << " [2] Cargar data/instancia_pequena.csv (N = 10 - Euclidiana km)\n";
    cout << " [3] Cargar data/instancia_mediana.csv (N = 16 - Euclidiana km)\n";
    cout << " [4] Cargar archivo CSV de coordenadas personalizado\n";
    cout << " [5] Cargar matriz de distancias NxN directamente desde CSV\n";
    cout << " [6] Generar instancia determinista con std::mt19937 (N = 30, 50, 100, 200+)\n";
    cout << "Seleccione una opcion: ";

    int sub = 0;
    if (!(cin >> sub)) {
        if (cin.eof()) return;
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
            cout << "Ruta del archivo CSV de coordenadas: ";
            string rutaArchivo;
            cin >> ws;
            getline(cin, rutaArchivo);
            grafo = Graph::loadFromCSV(rutaArchivo, TipoMetrica::AUTO);
        } else if (sub == 5) {
            cout << "Ruta del archivo CSV de matriz de distancias: ";
            string rutaArchivo;
            cin >> ws;
            getline(cin, rutaArchivo);
            grafo = Graph::loadDistanceMatrixFromCSV(rutaArchivo);
        } else if (sub == 6) {
            int nuevoN = 50;
            unsigned int semilla = 42;
            cout << "Cantidad de nodos N (ej. 30, 50, 100, 200): ";
            if (!(cin >> nuevoN) || nuevoN < 2) {
                if (cin.eof()) return;
                cin.clear();
                cin.ignore(10000, '\n');
                throw invalid_argument("N debe ser mayor o igual a 2.");
            }
            cout << "Semilla fija para std::mt19937 (ej. 42): ";
            if (!(cin >> semilla)) {
                if (cin.eof()) return;
                cin.clear();
                cin.ignore(10000, '\n');
                throw invalid_argument("Semilla invalida.");
            }
            grafo = Graph::generateRandomInstance(nuevoN, semilla, 100.0);
        } else {
            cout << "Operacion cancelada.\n";
            return;
        }

        ValidacionGrafo val = grafo.validateGraphDetailed(true);
        cout << "\n [OK] Instancia cargada: " << grafo.getInstanceName() << " | N = " << grafo.getNumNodes() << " nodos.\n";
        cout << " [OK] Validacion estructural: " << (val.esValido ? "APROBADA" : "CON ERRORES")
             << " | Desigualdad Triangular: " << (val.cumpleDesigualdadTriangular ? "CUMPLE (TSP Metrico)" : "NO CUMPLE") << "\n";
    } catch (const exception& e) {
        cout << "\n [ERROR DE CARGA / VALIDACION] " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc > 1) {
            string arg = argv[1];
            if (arg == "--benchmark") {
                ejecutarSuiteBenchmarkCSV();
                return 0;
            }
            if (arg == "--test") {
                return ejecutarSuitePruebas();
            }
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
            cout << " [1]  Ejecutar Backtracking con Poda / Branch & Bound (Exacto, N <= 12)\n";
            cout << " [2]  Ejecutar Held-Karp / Prog. Dinamica + Bitmask (Exacto, N <= 20)\n";
            cout << " [3]  Ejecutar MST + DFS con Prim (2-Aproximacion / Heuristica)\n";
            cout << " [4]  Ejecutar Nearest Neighbor / Vecino Mas Cercano (Voraz, O(N^2))\n";
            cout << " [5]  Comparar los 4 algoritmos + Interpretacion Experimental Clara\n";
            cout << " [6]  Seleccion Adaptativa Automatica (Matriz de Decision Inteligente)\n";
            cout << " [7]  Gestionar ubicaciones dinamicamente (Almacen, Clientes)\n";
            cout << " [8]  Cargar dataset CSV o Generar instancia determinista\n";
            cout << " [9]  Validar instancia actual (Checklist de 8 puntos + Matriz)\n";
            cout << " [10] Configurar parametros de costo operativo AndesExpress (USD)\n";
            cout << " [11] Ejecutar Suite de Benchmarking Completa + Exportar CSV\n";
            cout << " [12] Ejecutar Suite de Pruebas Automatizadas (35 tests unitarios)\n";
            cout << " [0]  Salir\n";
            cout << "========================================================================\n";
            cout << "Seleccione una opcion: ";

            if (!(cin >> opcion)) {
                if (cin.eof()) {
                    cout << "\nEntrada finalizada (EOF). Cerrando RouteOpt.\n";
                    break;
                }
                cin.clear();
                cin.ignore(10000, '\n');
                cout << "Entrada invalida. Ingrese un numero del 0 al 12.\n";
                opcion = -1;
                continue;
            }

            try {
                if (opcion == 1) {
                    mostrarResultadoIndividual(grafo, Algorithms::Backtracking(grafo, Algorithms::kMaxSafeBacktracking));
                } else if (opcion == 2) {
                    mostrarResultadoIndividual(grafo, Algorithms::HeldKarp(grafo, Algorithms::kMaxSafeHeldKarp, 512.0));
                } else if (opcion == 3) {
                    mostrarResultadoIndividual(grafo, Algorithms::MST_DFS(grafo));
                } else if (opcion == 4) {
                    mostrarResultadoIndividual(grafo, Algorithms::NearestNeighbor(grafo, 0));
                } else if (opcion == 5) {
                    mostrarTablaComparativa(grafo, "COMPARATIVA EXPERIMENTAL MULTIALGORITMICA");
                } else if (opcion == 6) {
                    ejecutarRecomendadorAdaptativo(grafo);
                } else if (opcion == 7) {
                    menuGestionUbicaciones(grafo);
                } else if (opcion == 8) {
                    menuCargarInstancia(grafo);
                } else if (opcion == 9) {
                    mostrarDiagnosticoYMatriz(grafo);
                } else if (opcion == 10) {
                    menuConfiguracionParametros(grafo);
                } else if (opcion == 11) {
                    ejecutarSuiteBenchmarkCSV();
                } else if (opcion == 12) {
                    ejecutarSuitePruebas();
                } else if (opcion == 0) {
                    cout << "Cerrando RouteOpt. Hasta pronto.\n";
                } else {
                    cout << "Opcion fuera de rango (0 a 12).\n";
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
