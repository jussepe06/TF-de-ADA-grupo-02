#include "Graph.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

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
    if (isnan(weight) || isinf(weight) || weight < 0.0) {
        throw invalid_argument("El peso de la arista debe ser un numero real no negativo y finito.");
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

void Graph::setGeographic(bool geo) {
    geographicMode = geo;
}

const vector<vector<double>>& Graph::getMatrix() const {
    return adjMatrix;
}

const vector<Punto>& Graph::getCoordinates() const {
    return coordinates;
}

bool Graph::getNode(int id, Punto& outPunto) const {
    if (id >= 0 && id < static_cast<int>(coordinates.size())) {
        outPunto = coordinates[id];
        return true;
    }
    return false;
}

const ParametrosCosto& Graph::getParametrosCosto() const {
    return costParams;
}

void Graph::setParametrosCosto(const ParametrosCosto& params) {
    if (params.rendimientoKmL <= 0.0 || params.velocidadPromKmH <= 0.0 ||
        params.precioCombustible < 0.0 || params.costoHoraChofer < 0.0 ||
        params.costoParadaCliente < 0.0) {
        throw invalid_argument("Rendimiento y velocidad deben ser > 0, y los costos unitarios >= 0.");
    }
    costParams = params;
}

void Graph::rebuildDistanceMatrix() {
    int n = static_cast<int>(coordinates.size());
    numNodes = n;
    adjMatrix.assign(static_cast<size_t>(n), vector<double>(static_cast<size_t>(n), 0.0));

    for (int i = 0; i < n; i++) {
        coordinates[i].id = i;
        adjMatrix[i][i] = 0.0;
        for (int j = i + 1; j < n; j++) {
            double dist = 0.0;
            if (geographicMode) {
                dist = haversineKm(coordinates[i].x, coordinates[i].y, coordinates[j].x, coordinates[j].y);
            } else {
                dist = hypot(coordinates[i].x - coordinates[j].x, coordinates[i].y - coordinates[j].y);
            }
            if (isnan(dist) || dist < 0.0) {
                dist = 0.0;
            }
            adjMatrix[i][j] = dist;
            adjMatrix[j][i] = dist;
        }
    }
}

int Graph::addNode(const string& nombre, double x, double y) {
    if (isnan(x) || isinf(x) || isnan(y) || isinf(y)) {
        throw invalid_argument("Las coordenadas del nodo deben ser valores numericos finitos.");
    }
    int nuevoId = static_cast<int>(coordinates.size());
    Punto p;
    p.id = nuevoId;
    p.nombre = nombre.empty() ? ((nuevoId == 0) ? "Almacen Central" : ("Cliente " + to_string(nuevoId))) : nombre;
    p.x = x;
    p.y = y;
    coordinates.push_back(p);
    rebuildDistanceMatrix();
    return nuevoId;
}

bool Graph::setWarehouse(const string& nombre, double x, double y) {
    if (isnan(x) || isinf(x) || isnan(y) || isinf(y)) {
        throw invalid_argument("Las coordenadas del almacen deben ser finitas.");
    }
    if (coordinates.empty()) {
        addNode(nombre, x, y);
        return true;
    }
    coordinates[0].nombre = nombre.empty() ? "Almacen Central" : nombre;
    coordinates[0].x = x;
    coordinates[0].y = y;
    rebuildDistanceMatrix();
    return true;
}

bool Graph::updateNode(int id, const string& nuevoNombre, double nuevoX, double nuevoY) {
    if (id < 0 || id >= static_cast<int>(coordinates.size())) {
        return false;
    }
    if (isnan(nuevoX) || isinf(nuevoX) || isnan(nuevoY) || isinf(nuevoY)) {
        return false;
    }
    if (!nuevoNombre.empty()) {
        coordinates[id].nombre = nuevoNombre;
    }
    coordinates[id].x = nuevoX;
    coordinates[id].y = nuevoY;
    rebuildDistanceMatrix();
    return true;
}

bool Graph::removeNode(int id) {
    if (id <= 0 || id >= static_cast<int>(coordinates.size()) || coordinates.size() <= 2) {
        return false;
    }
    coordinates.erase(coordinates.begin() + id);
    rebuildDistanceMatrix();
    return true;
}

double Graph::calcularDistanciaRutaKm(const vector<int>& ruta) const {
    if (ruta.size() < 2) return 0.0;
    double dTotal = 0.0;
    for (size_t i = 0; i + 1 < ruta.size(); i++) {
        dTotal += getCost(ruta[i], ruta[i + 1]);
    }
    return dTotal;
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

DesgloseCostoRuta Graph::desglosarCostoRuta(const vector<int>& ruta) const {
    DesgloseCostoRuta info;
    if (ruta.size() < 2) return info;

    for (size_t i = 0; i + 1 < ruta.size(); i++) {
        int u = ruta[i];
        int v = ruta[i + 1];
        double d = getCost(u, v);
        double litros = d / costParams.rendimientoKmL;
        double horas = d / costParams.velocidadPromKmH;
        double cc = litros * costParams.precioCombustible;
        double co = horas * costParams.costoHoraChofer;
        double cp = (v == 0) ? 0.0 : costParams.costoParadaCliente;

        info.distanciaTotalKm += d;
        info.litrosCombustible += litros;
        info.tiempoConduccionHoras += horas;
        info.costoCombustibleUSD += cc;
        info.costoChoferUSD += co;
        info.costoParadasUSD += cp;
        if (v != 0) {
            info.clientesVisitados++;
        }
    }
    info.costoTotalUSD = info.costoCombustibleUSD + info.costoChoferUSD + info.costoParadasUSD;
    return info;
}

ValidacionGrafo Graph::validateGraphDetailed(bool verificarDesigualdadTriangular) const {
    ValidacionGrafo v;

    if (numNodes < 2) {
        v.esValido = false;
        v.dimensionesValidas = false;
        v.tieneAlmacenCentral = (numNodes >= 1);
        v.errores.push_back("El grafo requiere al menos N >= 2 nodos (Almacen Central 0 + al menos 1 cliente).");
        return v;
    }

    if (static_cast<int>(adjMatrix.size()) != numNodes) {
        v.esValido = false;
        v.dimensionesValidas = false;
        v.errores.push_back("Dimension de matriz invalida: se esperaban " + to_string(numNodes) +
                            " filas, pero existen " + to_string(adjMatrix.size()) + ".");
        return v;
    }

    // 1. Validacion de coordenadas e identificadores
    if (!coordinates.empty()) {
        if (static_cast<int>(coordinates.size()) != numNodes) {
            v.esValido = false;
            v.dimensionesValidas = false;
            v.errores.push_back("Desfase entre coordenadas registradas (" + to_string(coordinates.size()) +
                                ") y numero de nodos de la matriz (" + to_string(numNodes) + ").");
        }

        unordered_set<int> idsUnicos;
        bool vioIdCero = false;
        for (const auto& p : coordinates) {
            if (p.id == 0) vioIdCero = true;

            if (idsUnicos.count(p.id)) {
                v.esValido = false;
                v.idsValidos = false;
                v.errores.push_back("Identificador duplicado detectado en nodo: ID " + to_string(p.id));
            }
            idsUnicos.insert(p.id);

            if (isnan(p.x) || isinf(p.x) || isnan(p.y) || isinf(p.y)) {
                v.esValido = false;
                v.sinNaNs = false;
                v.coordsValidas = false;
                v.errores.push_back("Coordenadas no finitas (NaN/Inf) en nodo ID " + to_string(p.id));
            } else if (geographicMode) {
                if (p.x < -90.0 || p.x > 90.0 || p.y < -180.0 || p.y > 180.0) {
                    v.esValido = false;
                    v.coordsValidas = false;
                    v.errores.push_back("Coordenadas geograficas fuera de rango [-90..90, -180..180] en nodo " +
                                        to_string(p.id) + " (" + p.nombre + ").");
                }
            }
        }

        if (!vioIdCero) {
            v.esValido = false;
            v.tieneAlmacenCentral = false;
            v.idsValidos = false;
            v.errores.push_back("No se encontro el Almacen Central obligatorio con ID = 0.");
        }
    }

    // 2. Validacion de la matriz de adyacencia y recoleccion de estadisticas descriptivas
    double minPositivo = numeric_limits<double>::infinity();
    double maxDist = 0.0;
    double sumaDist = 0.0;
    int conteoPares = 0;

    for (int i = 0; i < numNodes; i++) {
        if (static_cast<int>(adjMatrix[i].size()) != numNodes) {
            v.esValido = false;
            v.dimensionesValidas = false;
            v.errores.push_back("La fila " + to_string(i) + " de la matriz tiene " +
                                to_string(adjMatrix[i].size()) + " columnas en lugar de " + to_string(numNodes) + ".");
            return v;
        }

        if (isnan(adjMatrix[i][i]) || abs(adjMatrix[i][i]) > 1e-6) {
            v.esValido = false;
            v.diagonalCero = false;
            v.errores.push_back("La diagonal principal debe ser 0.0 km (nodo " + to_string(i) +
                                " tiene " + to_string(adjMatrix[i][i]) + ").");
        }

        for (int j = i + 1; j < numNodes; j++) {
            double w_ij = adjMatrix[i][j];
            double w_ji = adjMatrix[j][i];

            if (isnan(w_ij) || isnan(w_ji)) {
                v.esValido = false;
                v.sinNaNs = false;
                v.errores.push_back("Distancia indefinida (NaN) entre nodos (" + to_string(i) + ", " + to_string(j) + ").");
                continue;
            }
            if (isinf(w_ij) || isinf(w_ji)) {
                v.esValido = false;
                v.conexo = false;
                v.errores.push_back("Arista infinita / nodos desconectados entre (" + to_string(i) + ", " + to_string(j) + ").");
                continue;
            }
            if (w_ij < 0.0 || w_ji < 0.0) {
                v.esValido = false;
                v.noNegativo = false;
                v.errores.push_back("Distancia negativa no permitida entre (" + to_string(i) + ", " + to_string(j) + ").");
                continue;
            }
            if (abs(w_ij - w_ji) > 1e-4) {
                v.esValido = false;
                v.simetrico = false;
                v.errores.push_back("Matriz asimetrica en par (" + to_string(i) + ", " + to_string(j) +
                                    "): d(i,j)=" + to_string(w_ij) + " vs d(j,i)=" + to_string(w_ji) + ".");
            }

            if (w_ij == 0.0) {
                v.paresCoincidentes++;
            } else if (w_ij < minPositivo) {
                minPositivo = w_ij;
            }

            if (w_ij > maxDist) maxDist = w_ij;
            if (i == 0 && w_ij > v.radioMaxAlmacenKm) v.radioMaxAlmacenKm = w_ij;

            sumaDist += w_ij;
            conteoPares++;
        }
    }

    if (conteoPares > 0) {
        v.distanciaMinKm = (minPositivo == numeric_limits<double>::infinity()) ? 0.0 : minPositivo;
        v.distanciaMaxKm = maxDist;
        v.distanciaPromedioKm = sumaDist / conteoPares;
    }

    if (v.paresCoincidentes > 0) {
        v.advertencias.push_back("Se detectaron " + to_string(v.paresCoincidentes) +
                                 " par(es) de ubicaciones con coordenadas identicas (distancia 0.00 km).");
    }

    // 3. Verificacion exhaustiva de la Desigualdad Triangular (propiedad metrica para cota <= 2*OPT)
    if (verificarDesigualdadTriangular && v.esValido) {
        string peorCasoEjemplo;
        for (int i = 0; i < numNodes; i++) {
            for (int j = i + 1; j < numNodes; j++) {
                double dDirecta = adjMatrix[i][j];
                for (int k = 0; k < numNodes; k++) {
                    if (k == i || k == j) continue;
                    v.ternasEvaluadas++;
                    double dEscala = adjMatrix[i][k] + adjMatrix[k][j];
                    double exceso = dDirecta - dEscala;
                    if (exceso > 1e-4) {
                        v.cumpleDesigualdadTriangular = false;
                        v.esMetrico = false;
                        v.violacionesTriangulares++;
                        if (exceso > v.peorExcesoTriangularKm) {
                            v.peorExcesoTriangularKm = exceso;
                            ostringstream oss;
                            oss << fixed << setprecision(2)
                                << "Ir directo de N" << i << " a N" << j << " (" << dDirecta
                                << " km) es mas caro que pasar por N" << k << " (" << dEscala
                                << " km; exceso +" << exceso << " km).";
                            peorCasoEjemplo = oss.str();
                        }
                    }
                }
            }
        }

        if (!v.cumpleDesigualdadTriangular) {
            v.advertencias.push_back("No cumple Desigualdad Triangular en " +
                                     to_string(v.violacionesTriangulares) + " de " +
                                     to_string(v.ternasEvaluadas) + " ternas evaluadas. Peor caso: " +
                                     peorCasoEjemplo);
        }
    }

    return v;
}

bool Graph::validateGraph() const {
    ValidacionGrafo v = validateGraphDetailed(false);
    return v.esValido;
}

bool Graph::checkTriangularInequality() const {
    ValidacionGrafo v = validateGraphDetailed(true);
    return v.esValido && v.cumpleDesigualdadTriangular;
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
    unordered_set<int> idsArchivo;
    string linea;
    int numLinea = 0;

    while (getline(input, linea)) {
        numLinea++;
        linea = trim(linea);
        if (linea.empty() || linea[0] == '#') {
            continue;
        }

        if (linea.find(',') != string::npos) {
            stringstream ss(linea);
            string sId, sNombre, sX, sY;
            if (!getline(ss, sId, ',')) continue;
            sId = trim(sId);

            // Ignorar cabecera no numerica (ej. id,nombre,lat_o_x,lon_o_y) solo en primeras lineas
            if (nodos.empty() && (sId.empty() || (!isdigit(static_cast<unsigned char>(sId[0])) && sId[0] != '-'))) {
                continue;
            }

            if (!getline(ss, sNombre, ',') || !getline(ss, sX, ',') || !getline(ss, sY)) {
                throw runtime_error("Linea CSV incompleta o corrupta (linea " + to_string(numLinea) +
                                    ") en " + nombreInstancia + ": " + linea);
            }

            Punto p;
            size_t posId = 0, posX = 0, posY = 0;
            string tX = trim(sX);
            string tY = trim(sY);
            p.id = stoi(sId, &posId);
            p.x = stod(tX, &posX);
            p.y = stod(tY, &posY);

            if (posId != sId.size() || posX != tX.size() || posY != tY.size() ||
                isnan(p.x) || isinf(p.x) || isnan(p.y) || isinf(p.y)) {
                throw runtime_error("Valores numericos invalidos en linea " + to_string(numLinea) +
                                    " de " + nombreInstancia);
            }
            if (idsArchivo.count(p.id)) {
                throw runtime_error("ID duplicado (" + to_string(p.id) + ") en archivo CSV: " + nombreInstancia);
            }
            idsArchivo.insert(p.id);
            p.nombre = trim(sNombre);
            if (p.nombre.empty()) {
                p.nombre = (p.id == 0) ? "Almacen Central" : ("Cliente " + to_string(p.id));
            }
            nodos.push_back(p);
        } else {
            stringstream ss(linea);
            Punto p;
            if (ss >> p.id >> p.x >> p.y) {
                if (isnan(p.x) || isinf(p.x) || isnan(p.y) || isinf(p.y)) {
                    throw runtime_error("Coordenadas invalidas en linea " + to_string(numLinea));
                }
                if (idsArchivo.count(p.id)) {
                    throw runtime_error("ID duplicado (" + to_string(p.id) + ") en flujo de entrada.");
                }
                idsArchivo.insert(p.id);
                p.nombre = (p.id == 0) ? "Almacen Central" : ("Cliente " + to_string(p.id));
                nodos.push_back(p);
            } else {
                throw runtime_error("Formato de fila no reconocido en linea " + to_string(numLinea) + ": " + linea);
            }
        }

        if (maxNodes > 0 && static_cast<int>(nodos.size()) >= maxNodes) {
            break;
        }
    }

    int n = static_cast<int>(nodos.size());
    if (n < 2) {
        throw runtime_error("Instancia invalida (" + nombreInstancia + "): se requieren al menos 2 nodos validos.");
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

    if (usarHaversine) {
        for (const auto& p : nodos) {
            if (p.x < -90.0 || p.x > 90.0 || p.y < -180.0 || p.y > 180.0) {
                throw runtime_error("Coordenadas fuera de rango geografico [-90..90, -180..180] en nodo " + p.nombre);
            }
        }
    }

    // Asegurar indices internos correlativos 0..n-1 manteniendo al primer registro como Almacen Central
    for (int i = 0; i < n; i++) {
        nodos[i].id = i;
    }

    Graph g(n, nombreInstancia);
    g.coordinates = nodos;
    g.geographicMode = usarHaversine;

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double dist = usarHaversine
                              ? haversineKm(nodos[i].x, nodos[i].y, nodos[j].x, nodos[j].y)
                              : hypot(nodos[i].x - nodos[j].x, nodos[i].y - nodos[j].y);
            if (isnan(dist) || dist < 0.0) {
                dist = 0.0;
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

Graph Graph::fromDistanceMatrixCSV(istream& input, const string& nombreInstancia) {
    vector<vector<double>> filas;
    string linea;

    while (getline(input, linea)) {
        linea = trim(linea);
        if (linea.empty() || linea[0] == '#') continue;

        if (linea.find(',') != string::npos) {
            stringstream ss(linea);
            string token;
            vector<double> fila;
            bool esCabecera = false;

            while (getline(ss, token, ',')) {
                token = trim(token);
                if (token.empty()) continue;
                try {
                    size_t pos = 0;
                    double v = stod(token, &pos);
                    if (pos != token.size() || isnan(v) || isinf(v)) {
                        esCabecera = filas.empty();
                        if (!esCabecera) {
                            throw runtime_error("Valor no numerico o infinito en matriz CSV: " + token);
                        }
                        break;
                    }
                    fila.push_back(v);
                } catch (const invalid_argument&) {
                    if (filas.empty()) {
                        esCabecera = true;
                        break;
                    }
                    throw runtime_error("Celda no numerica en matriz CSV: " + token);
                }
            }
            if (!esCabecera && !fila.empty()) {
                filas.push_back(fila);
            }
        } else {
            stringstream ss(linea);
            double val;
            vector<double> fila;
            while (ss >> val) {
                if (isnan(val) || isinf(val)) {
                    throw runtime_error("Valor NaN o Inf detectado en matriz de distancias.");
                }
                fila.push_back(val);
            }
            if (!fila.empty()) {
                filas.push_back(fila);
            }
        }
    }

    int n = static_cast<int>(filas.size());
    if (n < 2) {
        throw runtime_error("La matriz de distancias debe tener al menos dimension 2x2.");
    }
    for (int i = 0; i < n; i++) {
        if (static_cast<int>(filas[i].size()) != n) {
            throw runtime_error("Fila " + to_string(i) + " de la matriz no es cuadrada (tiene " +
                                to_string(filas[i].size()) + " columnas, esperadas " + to_string(n) + ").");
        }
    }

    Graph g(n, nombreInstancia);
    g.geographicMode = false;
    for (int i = 0; i < n; i++) {
        Punto p;
        p.id = i;
        p.nombre = (i == 0) ? "Almacen Central" : ("Cliente " + to_string(i));
        p.x = 0.0;
        p.y = 0.0;
        g.coordinates.push_back(p);
        for (int j = 0; j < n; j++) {
            g.adjMatrix[i][j] = filas[i][j];
        }
    }

    ValidacionGrafo val = g.validateGraphDetailed(false);
    if (!val.esValido) {
        string detalle = val.errores.empty() ? "Estructura invalida." : val.errores.front();
        throw runtime_error("La matriz cargada no supero la validacion estructural: " + detalle);
    }

    return g;
}

Graph Graph::loadDistanceMatrixFromCSV(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        throw runtime_error("No se pudo abrir el archivo de matriz CSV: " + filepath);
    }
    return fromDistanceMatrixCSV(file, filepath);
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
            if (d < 0.0) d = 0.0;
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
