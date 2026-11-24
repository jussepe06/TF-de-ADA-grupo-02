#include "Graph.h"

#include <cmath>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthRadiusKm = 6371.0;
}

string Graph::trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

Graph::Graph(int n, const string& nombre)
    : numNodes(n), instanceName(nombre), geographicMode(false) {
    if (n < 0) {
        throw invalid_argument("El numero de nodos no puede ser negativo.");
    }
    adjMatrix.assign(static_cast<size_t>(n), vector<double>(static_cast<size_t>(n), 0.0));
}

void Graph::addEdge(int u, int v, double weight) {
    if (u < 0 || u >= numNodes || v < 0 || v >= numNodes) {
        throw out_of_range("Indice de nodo fuera de rango.");
    }
    if (weight < 0.0) {
        throw invalid_argument("La distancia entre nodos no puede ser negativa.");
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

const string& Graph::getInstanceName() const {
    return instanceName;
}

void Graph::setInstanceName(const string& nombre) {
    instanceName = nombre;
}

bool Graph::isGeographic() const {
    return geographicMode;
}

const vector<vector<double>>& Graph::getMatrix() const {
    return adjMatrix;
}

const vector<Punto>& Graph::getCoordinates() const {
    return coordinates;
}

const ParametrosCosto& Graph::getParametrosCosto() const {
    return costParams;
}

void Graph::setParametrosCosto(const ParametrosCosto& params) {
    if (params.rendimientoKmL <= 0.0 || params.velocidadPromKmH <= 0.0) {
        throw invalid_argument("Rendimiento y velocidad promedio deben ser mayores a cero.");
    }
    costParams = params;
}

double Graph::calcularCostoTramoUSD(int u, int v) const {
    double d = getCost(u, v);
    double cc = d * (costParams.precioCombustible / costParams.rendimientoKmL);
    double t = d / costParams.velocidadPromKmH;
    double cp = (v == 0) ? 0.0 : costParams.costoParadaCliente;
    return cc + (costParams.costoHoraChofer * t) + cp;
}

double Graph::calcularCostoRutaUSD(const vector<int>& ruta) const {
    if (ruta.size() < 2) return 0.0;
    double totalUSD = 0.0;
    for (size_t i = 0; i + 1 < ruta.size(); i++) {
        totalUSD += calcularCostoTramoUSD(ruta[i], ruta[i + 1]);
    }
    return totalUSD;
}

bool Graph::validateGraph() const {
    if (numNodes <= 1) return false;
    if (static_cast<int>(adjMatrix.size()) != numNodes) return false;

    for (int i = 0; i < numNodes; i++) {
        if (static_cast<int>(adjMatrix[i].size()) != numNodes) return false;
        if (abs(adjMatrix[i][i]) > 1e-6) return false;

        for (int j = i + 1; j < numNodes; j++) {
            if (adjMatrix[i][j] <= 0.0) return false;
            if (abs(adjMatrix[i][j] - adjMatrix[j][i]) > 1e-6) return false;

            for (int k = 0; k < numNodes; k++) {
                if (k != i && k != j) {
                    if (adjMatrix[i][j] > adjMatrix[i][k] + adjMatrix[k][j] + 1e-5) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

double Graph::haversineKm(double lat1, double lon1, double lat2, double lon2) {
    double dLat = (lat2 - lat1) * (kPi / 180.0);
    double dLon = (lon2 - lon1) * (kPi / 180.0);
    double radLat1 = lat1 * (kPi / 180.0);
    double radLat2 = lat2 * (kPi / 180.0);

    double sinLat = sin(dLat / 2.0);
    double sinLon = sin(dLon / 2.0);
    double a = sinLat * sinLat + cos(radLat1) * cos(radLat2) * sinLon * sinLon;
    double c = 2.0 * atan2(sqrt(a), sqrt(max(0.0, 1.0 - a)));
    return kEarthRadiusKm * c;
}

Graph Graph::fromCSVStream(istream& input,
                           const string& nombreInstancia,
                           TipoMetrica metrica,
                           int maxNodes) {
    vector<Punto> nodos;
    string linea;

    while (getline(input, linea)) {
        linea = trim(linea);
        if (linea.empty() || linea[0] == '#') {
            continue;
        }

        if (linea.find(',') != string::npos) {
            stringstream ss(linea);
            string sId, sNombre, sX, sY;
            if (!getline(ss, sId, ',')) continue;
            sId = trim(sId);

            // Ignorar fila de cabecera CSV (ej. id,nombre,lat_o_x,lon_o_y)
            if (sId.empty() || (!isdigit(static_cast<unsigned char>(sId[0])) && sId[0] != '-')) {
                continue;
            }

            if (!getline(ss, sNombre, ',') || !getline(ss, sX, ',') || !getline(ss, sY)) {
                throw runtime_error("Linea CSV mal formada en " + nombreInstancia + ": " + linea);
            }

            Punto p;
            p.id = stoi(sId);
            p.nombre = trim(sNombre);
            p.x = stod(trim(sX));
            p.y = stod(trim(sY));
            nodos.push_back(p);
        } else {
            stringstream ss(linea);
            Punto p;
            if (ss >> p.id >> p.x >> p.y) {
                p.nombre = (p.id == 0) ? "Almacen Central" : ("Cliente " + to_string(p.id));
                nodos.push_back(p);
            }
        }

        if (maxNodes > 0 && static_cast<int>(nodos.size()) >= maxNodes) {
            break;
        }
    }

    int n = static_cast<int>(nodos.size());
    if (n < 2) {
        throw runtime_error("Se requieren al menos 2 nodos validos en: " + nombreInstancia);
    }

    bool usarHaversine = (metrica == TipoMetrica::HAVERSINE);
    if (metrica == TipoMetrica::AUTO) {
        bool parecenLatLon = true;
        bool hayNegativos = false;
        for (const auto& p : nodos) {
            if (p.x < -90.0 || p.x > 90.0 || p.y < -180.0 || p.y > 180.0) {
                parecenLatLon = false;
                break;
            }
            if (p.x < 0.0 || p.y < 0.0) {
                hayNegativos = true;
            }
        }
        usarHaversine = parecenLatLon && hayNegativos;
    }

    // Normalizar IDs secuenciales 0..n-1 respetando el orden de carga
    for (int i = 0; i < n; i++) {
        nodos[i].id = i;
    }

    Graph g(n, nombreInstancia);
    g.coordinates = nodos;
    g.geographicMode = usarHaversine;

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double dist = 0.0;
            if (usarHaversine) {
                dist = haversineKm(nodos[i].x, nodos[i].y, nodos[j].x, nodos[j].y);
            } else {
                dist = hypot(nodos[i].x - nodos[j].x, nodos[i].y - nodos[j].y);
            }
            if (dist <= 0.0) {
                dist = 1e-4;
            }
            g.addEdge(i, j, dist);
        }
    }

    return g;
}

Graph Graph::loadFromCSV(const string& filepath, TipoMetrica metrica, int maxNodes) {
    ifstream file(filepath);
    if (!file.is_open()) {
        throw runtime_error("No se pudo abrir el archivo CSV: " + filepath);
    }
    return fromCSVStream(file, filepath, metrica, maxNodes);
}

Graph Graph::generateRandomInstance(int n, unsigned int seed, double maxCoordKm) {
    if (n < 2) {
        throw invalid_argument("La instancia generada requiere al menos N = 2 nodos.");
    }
    if (maxCoordKm <= 1.0) {
        throw invalid_argument("El rango maximo de coordenadas debe ser mayor a 1 km.");
    }

    mt19937 rng(seed);
    uniform_real_distribution<double> distCoord(5.0, maxCoordKm - 5.0);

    Graph g(n, "Simulada Determinista (N=" + to_string(n) + ", seed=" + to_string(seed) + ")");
    g.geographicMode = false;
    g.coordinates.reserve(static_cast<size_t>(n));

    // Nodo 0 siempre es el Almacen Central ubicado en el centro logistico del plano
    g.coordinates.push_back({0, "Almacen Central AndesExpress", maxCoordKm / 2.0, maxCoordKm / 2.0});

    for (int i = 1; i < n; i++) {
        Punto p;
        p.id = i;
        p.nombre = "Cliente_" + to_string(i);
        p.x = distCoord(rng);
        p.y = distCoord(rng);
        g.coordinates.push_back(p);
    }

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double d = hypot(g.coordinates[i].x - g.coordinates[j].x,
                             g.coordinates[i].y - g.coordinates[j].y);
            if (d <= 0.0) d = 1e-4;
            g.addEdge(i, j, d);
        }
    }

    return g;
}

Graph Graph::createPilotInstance(int n) {
    if (n < 2) {
        throw invalid_argument("Se requieren al menos N = 2 nodos.");
    }

    if (n <= 20) {
        string datosBase =
            "id,nombre,lat_o_x,lon_o_y\n"
            "0,Almacen Central,50.0,50.0\n"
            "1,Cliente 01,42.0,68.0\n"
            "2,Cliente 02,62.0,65.0\n"
            "3,Cliente 03,25.0,82.0\n"
            "4,Cliente 04,78.0,80.0\n"
            "5,Cliente 05,85.0,45.0\n"
            "6,Cliente 06,72.0,20.0\n"
            "7,Cliente 07,48.0,28.0\n"
            "8,Cliente 08,22.0,22.0\n"
            "9,Cliente 09,18.0,52.0\n"
            "10,Cliente 10,80.0,10.0\n"
            "11,Cliente 11,25.0,95.0\n"
            "12,Cliente 12,55.0,75.0\n"
            "13,Cliente 13,95.0,65.0\n"
            "14,Cliente 14,30.0,10.0\n"
            "15,Cliente 15,60.0,35.0\n"
            "16,Cliente 16,12.0,40.0\n"
            "17,Cliente 17,78.0,55.0\n"
            "18,Cliente 18,42.0,88.0\n"
            "19,Cliente 19,88.0,22.0\n";

        stringstream stream(datosBase);
        return fromCSVStream(stream, "Piloto Estandar (N=" + to_string(n) + ")",
                             TipoMetrica::EUCLIDIANA, n);
    }

    return generateRandomInstance(n, 42, 100.0);
}
