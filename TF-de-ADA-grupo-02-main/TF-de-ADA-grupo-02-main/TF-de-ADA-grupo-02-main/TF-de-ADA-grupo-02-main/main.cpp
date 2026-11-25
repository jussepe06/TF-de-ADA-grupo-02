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

string formatearRuta(const vector<int> &ruta, size_t maxNodosVisibles = 18) {
  if (ruta.empty())
    return "(sin ruta)";
  ostringstream oss;
  if (ruta.size() <= maxNodosVisibles) {
    for (size_t i = 0; i < ruta.size(); i++) {
      oss << ruta[i];
      if (i + 1 < ruta.size())
        oss << " -> ";
    }
  } else {
    for (size_t i = 0; i < 6; i++) {
      oss << ruta[i] << " -> ";
    }
    oss << "... -> ";
    for (size_t i = ruta.size() - 4; i < ruta.size(); i++) {
      oss << ruta[i];
      if (i + 1 < ruta.size())
        oss << " -> ";
    }
    oss << " (" << ruta.size() - 1 << " tramos)";
  }
  return oss.str();
}

string formatearRutaGuiones(const vector<int> &ruta) {
  ostringstream oss;
  for (size_t i = 0; i < ruta.size(); i++) {
    oss << ruta[i];
    if (i + 1 < ruta.size())
      oss << "-";
  }
  return oss.str();
}

void mostrarResultadoIndividual(const TSPResult &res) {
  cout << "\n----------------------------------------------------------------------\n";
  cout << " Algoritmo ejecutado   : " << res.algoritmo << "\n";
  cout << " Tipo de solucion      : " << res.tipo_solucion_str << "\n";

  if (!res.ejecutado) {
    cout << " Estado                : OMITIDO POR PROTECCION DE SEGURIDAD / ESCALA\n";
    cout << " Motivo                : " << res.motivo_omision << "\n";
    cout << "----------------------------------------------------------------------\n";
    return;
  }

  cout << " Distancia total       : " << fixed << setprecision(2)
       << res.distancia_km << " km\n";
  cout << " Costo operativo total : $" << fixed << setprecision(2)
       << res.costo_usd << " USD\n";

  // Mostrar clasificacion de referencia y desviacion
  if (res.desviacion_valida) {
    cout << " Desviacion s/Optimo   : " << fixed << setprecision(2)
         << res.desviacion_pct << "% (" << res.tipo_referencia_str << ")\n";
  } else if (res.tipo_referencia == TipoReferencia::MEJOR_OBSERVADO) {
    cout << " Referencia comparada  : " << res.tipo_referencia_str
         << " (Dif vs mejor obs: +" << fixed << setprecision(2)
         << res.diferencia_mejor_observada_pct << "%)\n";
    if (res.cota_inferior_mst_km > 0.0) {
      cout << " Cota inf. teorica MST : " << fixed << setprecision(2)
           << res.cota_inferior_mst_km << " km (Dif vs cota: +"
           << res.diferencia_cota_inferior_pct << "%)\n";
    }
  } else {
    cout << " Desviacion porcentual : N/A (Sin referencia valida)\n";
  }

  cout << " Tiempo en microseg.   : " << fixed << setprecision(2)
       << res.tiempo_us << " us\n";
  cout << " Tiempo en miliseg.    : " << fixed << setprecision(4)
       << res.tiempo_ms << " ms\n";
  cout << " Memoria auxiliar est. : " << fixed << setprecision(2)
       << res.memoria_auxiliar_kb << " KB\n";
  cout << " Memoria proceso pico  : " << fixed << setprecision(2)
       << res.memoria_proceso_kb << " KB\n";
  cout << " Operaciones relevantes: " << res.operaciones_o_estados;
  if (!res.detalle_operaciones.empty()) {
    cout << " (" << res.detalle_operaciones << ")";
  }
  cout << "\n";

  cout << " Validacion de ruta    : "
       << (res.ruta_valida ? "[OK] Ciclo Hamiltoniano Valido (Retorno a 0 comprobado)"
                           : ("[FALLO] " + res.error_validacion_ruta))
       << "\n";
  cout << " Ruta calculada        : " << formatearRuta(res.ruta, 25) << "\n";
  cout << "----------------------------------------------------------------------\n";
}

vector<TSPResult> evaluarCuatroAlgoritmos(const Graph &grafo,
                                          bool imprimirAvisos = true) {
  vector<TSPResult> todos = {
      Algorithms::Backtracking(grafo, Algorithms::kMaxSafeBacktracking),
      Algorithms::HeldKarp(grafo, Algorithms::kMaxSafeHeldKarp, 512.0),
      Algorithms::MST_DFS(grafo),
      Algorithms::NearestNeighbor(grafo, 0)};

  // Cota inferior teorica (peso del MST) si esta disponible
  double cotaMST = todos[2].cota_inferior_mst_km;

  // Clasificar y calcular desviaciones de forma cientifica y rigurosa
  Algorithms::clasificarYCalcularDesviaciones(todos, cotaMST);

  if (imprimirAvisos) {
    for (const auto &r : todos) {
      if (!r.ejecutado) {
        cout << " [Aviso] " << r.algoritmo << " -> " << r.motivo_omision << "\n";
      }
    }
  }

  return todos;
}

void mostrarTablaComparativa(const Graph &grafo, const string &titulo) {
  int n = grafo.getNumNodes();
  ValidacionGrafo v = grafo.validateGraphDetailed(true);

  cout << "\n=========================================================================================================\n";
  cout << "  " << titulo << " | Instancia: " << grafo.getInstanceName() << "\n";
  cout << "  N = " << n << " nodos | Metrica: "
       << (grafo.isGeographic() ? "Haversine (Geografica)" : "Euclidiana (Plana)")
       << " | TSP Metrico (Desig. Triangular): " << (v.cumpleDesigualdadTriangular ? "SI" : "NO") << "\n";
  cout << "=========================================================================================================\n";

  vector<TSPResult> resultados = evaluarCuatroAlgoritmos(grafo, true);

  bool hayOptimoExacto = false;
  for (const auto &r : resultados) {
    if (r.ejecutado && r.tipo_referencia == TipoReferencia::OPTIMO_EXACTO) {
      hayOptimoExacto = true;
      break;
    }
  }

  cout << left << setw(20) << "Algoritmo"
       << setw(16) << "Tipo Solucion"
       << right << setw(11) << "Dist (km)"
       << setw(13) << "Costo (USD)"
       << setw(13) << (hayOptimoExacto ? "Desv s/OPT(%)" : "Dif s/Mejor(%)")
       << setw(13) << "Tiempo (ms)"
       << setw(13) << "Mem.Aux(KB)"
       << setw(14) << "Operaciones"
       << "   " << left << "Ruta Valida\n";
  cout << "---------------------------------------------------------------------------------------------------------\n";

  cout << fixed;
  for (const auto &res : resultados) {
    if (!res.ejecutado) continue;

    string desvStr;
    if (res.desviacion_valida) {
      ostringstream oss;
      oss << fixed << setprecision(2) << res.desviacion_pct << "%";
      desvStr = oss.str();
    } else {
      ostringstream oss;
      oss << "+" << fixed << setprecision(2) << res.diferencia_mejor_observada_pct << "%*";
      desvStr = oss.str();
    }

    string tipoCorto = res.tipo_solucion == TipoSolucion::EXACTA ? "Optima Exacta"
                      : (res.tipo_solucion == TipoSolucion::APROXIMADA ? "2-Aprox (Metr)" : "Heuristica");

    cout << left << setw(20) << res.algoritmo
         << setw(16) << tipoCorto
         << right << setw(11) << setprecision(2) << res.distancia_km
         << setw(13) << setprecision(2) << res.costo_usd
         << setw(13) << desvStr
         << setw(13) << setprecision(4) << res.tiempo_ms
         << setw(13) << setprecision(2) << res.memoria_auxiliar_kb
         << setw(14) << res.operaciones_o_estados
         << "   " << (res.ruta_valida ? "[OK]" : "[FALLO]") << " "
         << formatearRuta(res.ruta, 12) << "\n";
  }
  cout << "=========================================================================================================\n";
  if (!hayOptimoExacto) {
    cout << " * Nota: Ningun algoritmo exacto fue ejecutado (instancia N > "
         << Algorithms::kMaxSafeHeldKarp
         << "). La columna muestra diferencia respecto a la Mejor Heuristica Observada, NO error sobre optimo.\n";
  }
}

void mostrarNodosYMatriz(const Graph &grafo) {
  int n = grafo.getNumNodes();
  const auto &coords = grafo.getCoordinates();
  const auto &mat = grafo.getMatrix();
  const auto &params = grafo.getParametrosCosto();

  cout << "\n--- Informacion General de la Instancia ---\n";
  cout << " Nombre             : " << grafo.getInstanceName() << "\n";
  cout << " Total de nodos (N) : " << n << " (Nodo 0 = Almacen Central + "
       << (n - 1) << " clientes)\n";
  cout << " Tipo de distancia  : "
       << (grafo.isGeographic() ? "Geografica Real (Haversine km)"
                                : "Euclidiana Plana (km)")
       << "\n";
  cout << " Costos operativos  : Pc=$" << fixed << setprecision(2)
       << params.precioCombustible << "/L | R=" << params.rendimientoKmL
       << " km/L | V=" << params.velocidadPromKmH << " km/h | Co=$"
       << params.costoHoraChofer << "/h | Cp=$" << params.costoParadaCliente
       << "/cliente\n";

  cout << "\n--- Validacion Estructural y Diagnostico del Grafo ---\n";
  ValidacionGrafo v = grafo.validateGraphDetailed(true);
  if (v.esValido) {
    cout << " [OK] Grafo estructuralmente valido: Conexo, Simetrico, Diagonal 0, No negativo.\n";
  } else {
    cout << " [ERROR ESTRUCTURAL] El grafo contiene errores invalidantes:\n";
    for (const auto &err : v.errores) {
      cout << "   -> " << err << "\n";
    }
  }

  if (v.cumpleDesigualdadTriangular) {
    cout << " [OK] Cumple Desigualdad Triangular estricta (Grafo Metrico). Permite garantia formal 2*OPT.\n";
  } else {
    cout << " [AVISO METRICO] El grafo NO cumple la Desigualdad Triangular:\n";
    for (const auto &adv : v.advertencias) {
      cout << "   -> " << adv << "\n";
    }
  }

  int limiteCoords = min(n, 25);
  cout << "\n--- Nodos Cargados (mostrando " << limiteCoords << " de " << n
       << ") ---\n";
  for (int i = 0; i < limiteCoords; i++) {
    cout << " [" << setw(3) << coords[i].id << "] " << left << setw(40)
         << coords[i].nombre << right << " (" << fixed << setprecision(4)
         << coords[i].x << ", " << setprecision(4) << coords[i].y << ")\n";
  }
  if (n > limiteCoords) {
    cout << " ... (" << (n - limiteCoords)
         << " nodos adicionales omitidos en vista resumida)\n";
  }

  int limiteMatriz = min(n, 12);
  cout << "\n--- Matriz de Distancias en km (vista " << limiteMatriz << "x"
       << limiteMatriz << ") ---\n";
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

void menuGestionUbicaciones(Graph &grafo) {
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
        const auto &coords = grafo.getCoordinates();
        cout << "\n--- Ubicaciones Registradas (" << coords.size() << ") ---\n";
        for (const auto &p : coords) {
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
        cout << " [OK] Almacen Central registrado/actualizado correctamente.\n";
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
        cout << " [OK] Cliente registrado con exito con ID: " << nuevoId
             << ". Total nodos ahora: " << grafo.getNumNodes() << "\n";
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
        cout << "Nuevo nombre (dejar vacio para no cambiar): ";
        string nuevoNom;
        cin >> ws;
        getline(cin, nuevoNom);
        if (nuevoNom.empty()) nuevoNom = p.nombre;
        double nx = 0, ny = 0;
        cout << "Nueva coordenada X (o Latitud) [Actual: " << p.x << "]: ";
        cin >> nx;
        cout << "Nueva coordenada Y (o Longitud) [Actual: " << p.y << "]: ";
        cin >> ny;
        if (grafo.updateNode(idMod, nuevoNom, nx, ny)) {
          cout << " [OK] Nodo " << idMod << " modificado y matriz recalculada.\n";
        } else {
          cout << " [ERROR] No se pudo modificar el nodo.\n";
        }
      } else if (op == 5) {
        cout << "ID del cliente a eliminar (1 a " << (n - 1) << "): ";
        int idElim = -1;
        cin >> idElim;
        if (idElim == 0) {
          cout << " [ERROR] No se puede eliminar el Almacen Central (Nodo 0). Use la opcion modificar.\n";
          continue;
        }
        if (grafo.removeNode(idElim)) {
          cout << " [OK] Cliente eliminado y nodos re-indexados. Nodos restantes: "
               << grafo.getNumNodes() << "\n";
        } else {
          cout << " [ERROR] No se pudo eliminar el cliente (verifique ID o N > 2).\n";
        }
      } else if (op == 6) {
        bool nuevoModo = !grafo.isGeographic();
        grafo.setGeographic(nuevoModo);
        grafo.rebuildDistanceMatrix();
        cout << " [OK] Modo cambiado a: "
             << (nuevoModo ? "Geografico Real (Haversine km)" : "Euclidiano Plano (km)")
             << " y matriz recalculada.\n";
      } else if (op == 0) {
        break;
      }
    } catch (const exception &e) {
      cout << " [ERROR] " << e.what() << "\n";
    }
  }
}

void menuConfiguracionParametros(Graph &grafo) {
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
      cout << "Nuevo Pc (USD/L): ";
      cin >> p.precioCombustible;
    } else if (op == 2) {
      cout << "Nuevo R (km/L): ";
      cin >> p.rendimientoKmL;
    } else if (op == 3) {
      cout << "Nuevo V_prom (km/h): ";
      cin >> p.velocidadPromKmH;
    } else if (op == 4) {
      cout << "Nuevo Co (USD/h): ";
      cin >> p.costoHoraChofer;
    } else if (op == 5) {
      cout << "Nuevo Cp (USD/cliente): ";
      cin >> p.costoParadaCliente;
    } else if (op == 6) {
      cout << "Pc (USD/L): "; cin >> p.precioCombustible;
      cout << "R (km/L): "; cin >> p.rendimientoKmL;
      cout << "V_prom (km/h): "; cin >> p.velocidadPromKmH;
      cout << "Co (USD/h): "; cin >> p.costoHoraChofer;
      cout << "Cp (USD/cliente): "; cin >> p.costoParadaCliente;
    }
    grafo.setParametrosCosto(p);
    cout << " [OK] Parametros de costo operativo actualizados exitosamente.\n";
  } catch (const exception &e) {
    cout << " [ERROR] " << e.what() << "\n";
  }
}

void ejecutarRecomendadorAdaptativo(const Graph &grafo) {
  int n = grafo.getNumNodes();
  ConfiguracionAdaptativa cfg;

  cout << "\n--------------------------------------------------------------\n";
  cout << "     CONFIGURACION DEL RECOMENDADOR ADAPTATIVO (RF11)\n";
  cout << "--------------------------------------------------------------\n";
  cout << " Instancia activa: " << grafo.getInstanceName() << " (N = " << n << ")\n";
  cout << " Seleccione Criterio Prioritario:\n";
  cout << " [1] Balanceado Automatico (Optimo segun escala y recursos)\n";
  cout << " [2] Exigir Optimo Global Exacto (Backtracking / Held-Karp)\n";
  cout << " [3] Exigir Garantia Formal Teorica (MST + DFS: Costo <= 2*OPT)\n";
  cout << " [4] Maxima Velocidad en Despacho (Nearest Neighbor Voraz)\n";
  cout << "Opcion (1-4, defecto 1): ";

  int subOp = 1;
  if (cin >> subOp) {
    if (subOp == 2) cfg.criterio = CriterioPrioridad::OPTIMO_EXACTO;
    else if (subOp == 3) cfg.criterio = CriterioPrioridad::GARANTIA_TEORICA;
    else if (subOp == 4) cfg.criterio = CriterioPrioridad::MAXIMA_VELOCIDAD;
    else cfg.criterio = CriterioPrioridad::BALANCEADO;
  } else {
    cin.clear();
    cin.ignore(10000, '\n');
  }

  RecomendacionAdaptativa rec = Algorithms::SeleccionAdaptativa(grafo, cfg);

  cout << "\n================================================================================\n";
  cout << "          RECOMENDADOR ADAPTATIVO AUTOMATICO - ROUTEOPT (RF11)\n";
  cout << "================================================================================\n";
  cout << " Instancia analizada        : " << grafo.getInstanceName() << " (N = " << n << ")\n";
  cout << " Clasificacion de escala    : " << rec.categoriaEscala << "\n";
  cout << " Cumple Desig. Triangular   : "
       << (rec.cumpleDesigualdadTriangular ? "SI (TSP Metrico)" : "NO") << "\n";
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

  cout << "\n========================================================================\n";
  cout << "  INICIANDO SUITE DE BENCHMARKING AUTOMATIZADO - ANDESEXPRESS ROUTEOPT\n";
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
  } catch (const exception &e) {
    cout << " [Advertencia] No se encontro data/lima_real.csv (" << e.what() << ").\n";
  }

  const string archivoSalida = "resultados_benchmark.csv";
  ofstream csv(archivoSalida);
  if (!csv.is_open()) {
    throw runtime_error("No se pudo crear el archivo " + archivoSalida);
  }

  // Encabezado completo y compatible con generar_graficas.py y auditoria cientifica
  csv << "Escenario,N,Algoritmo,Tipo_Solucion,Distancia_km,Costo_USD,Tipo_Referencia,"
         "Desviacion_pct,Tiempo_ms,Tiempo_Min_ms,Tiempo_Max_ms,Memoria_KB,"
         "Memoria_Aux_KB,Memoria_Proceso_KB,Operaciones_Estados,Ruta_Valida,Repeticiones\n";
  csv << fixed;

  const int kRepeticiones = 3;

  for (const auto &caso : casos) {
    int n = caso.grafo.getNumNodes();
    cout << " -> Evaluando escenario [" << left << setw(11) << caso.escenario
         << right << "] N = " << setw(3) << n << " ("
         << caso.grafo.getInstanceName() << ") con " << kRepeticiones << " repeticiones...\n";

    // Medir y calcular estadisticas con kRepeticiones
    vector<TSPResult> resultadosBase = evaluarCuatroAlgoritmos(caso.grafo, false);

    for (const auto &res : resultadosBase) {
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
      double desvReportar = res.desviacion_valida ? res.desviacion_pct : -1.0;

      csv << caso.escenario << ","
          << n << ","
          << res.algoritmo << ","
          << res.tipo_solucion_str << ","
          << setprecision(4) << res.distancia_km << ","
          << setprecision(4) << res.costo_usd << ","
          << res.tipo_referencia_str << ","
          << setprecision(4) << desvReportar << ","
          << setprecision(6) << promedioTiempo << ","
          << setprecision(6) << minTiempo << ","
          << setprecision(6) << maxTiempo << ","
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
        for (const auto &res : resultadosBase) {
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
  cout << "\n [OK] Suite de Benchmark finalizada exitosamente.\n";
  cout << " [OK] Resultados guardados en: " << archivoSalida << "\n";
  if (tieneLimaReal) {
    cout << " [OK] Rutas optimizadas de Lima exportadas a: data/rutas_lima_benchmark.csv\n";
  }
  cout << " [TIP] Ejecute 'python generar_graficas.py' para regenerar los 4 graficos analiticos del informe.\n";
}

void menuCargarInstancia(Graph &grafo) {
  cout << "\n--- Carga de Datos / Generador de Instancias ---\n";
  cout << " [1] Cargar data/lima_real.csv (16 sedes reales en Lima - Haversine km)\n";
  cout << " [2] Cargar data/instancia_pequena.csv (N = 10 - Euclidiana km)\n";
  cout << " [3] Cargar data/instancia_mediana.csv (N = 16 - Euclidiana km)\n";
  cout << " [4] Cargar archivo CSV de coordenadas personalizado\n";
  cout << " [5] Cargar matriz de distancias directamente desde CSV\n";
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
      cout << "Opcion cancelada.\n";
      return;
    }

    cout << "\n [OK] Instancia cargada correctamente: "
         << grafo.getInstanceName() << " | N = " << grafo.getNumNodes()
         << " nodos.\n";
  } catch (const exception &e) {
    cout << "\n [ERROR] " << e.what() << "\n";
  }
}

int main(int argc, char *argv[]) {
  try {
    // Modo no interactivo para automatizacion por consola
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
      cout << " Tamano actual    : N = " << n << " nodos (Almacen 0 + "
           << (n - 1) << " clientes)\n";
      cout << " Metrica activa   : "
           << (grafo.isGeographic() ? "Haversine (km reales)" : "Euclidiana (km)")
           << "\n";
      cout << "------------------------------------------------------------------------\n";
      cout << " [1]  Ejecutar Backtracking con Poda / Branch & Bound (Exacto, N <= 12)\n";
      cout << " [2]  Ejecutar Held-Karp / Prog. Dinamica + Bitmask (Exacto, N <= 20)\n";
      cout << " [3]  Ejecutar MST + DFS con Prim (2-Aproximacion / Heuristica)\n";
      cout << " [4]  Ejecutar Nearest Neighbor / Vecino Mas Cercano (Voraz, O(N^2))\n";
      cout << " [5]  Comparar los 4 algoritmos en la instancia activa\n";
      cout << " [6]  Seleccion Adaptativa Automatica (Recomendador Inteligente)\n";
      cout << " [7]  Gestionar ubicaciones dinamicamente (Almacen, Clientes)\n";
      cout << " [8]  Cargar dataset CSV o Generar instancia determinista\n";
      cout << " [9]  Ver coordenadas, costos, matriz y diagnostico estructural\n";
      cout << " [10] Configurar parametros de costo operativo AndesExpress\n";
      cout << " [11] Ejecutar Suite de Benchmarking Completa + Exportar CSV\n";
      cout << " [12] Ejecutar Suite de Pruebas Automatizadas (29 tests unitarios)\n";
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
          mostrarResultadoIndividual(Algorithms::Backtracking(
              grafo, Algorithms::kMaxSafeBacktracking));
        } else if (opcion == 2) {
          mostrarResultadoIndividual(
              Algorithms::HeldKarp(grafo, Algorithms::kMaxSafeHeldKarp, 512.0));
        } else if (opcion == 3) {
          mostrarResultadoIndividual(Algorithms::MST_DFS(grafo));
        } else if (opcion == 4) {
          mostrarResultadoIndividual(Algorithms::NearestNeighbor(grafo, 0));
        } else if (opcion == 5) {
          mostrarTablaComparativa(grafo, "COMPARATIVA MULTIALGORITMICA");
        } else if (opcion == 6) {
          ejecutarRecomendadorAdaptativo(grafo);
        } else if (opcion == 7) {
          menuGestionUbicaciones(grafo);
        } else if (opcion == 8) {
          menuCargarInstancia(grafo);
        } else if (opcion == 9) {
          mostrarNodosYMatriz(grafo);
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
      } catch (const exception &e) {
        cout << "\n [ERROR DE EJECUCION] " << e.what() << "\n";
      }
    }
  } catch (const exception &fatal) {
    cerr << "[ERROR CRITICO] " << fatal.what() << "\n";
    return 1;
  }

  return 0;
}
