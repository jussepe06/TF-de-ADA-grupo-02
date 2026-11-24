#ifndef GRAPH_H
#define GRAPH_H

#include <istream>
#include <string>
#include <vector>

using namespace std;

// Representa un punto logistico (Nodo 0 = Almacen Central, 1..N-1 = Clientes)
struct Punto {
    int id = 0;
    string nombre;
    double x = 0.0; // Latitud (grados) o coordenada X (km)
    double y = 0.0; // Longitud (grados) o coordenada Y (km)
};

// Parametros del modelo de costo operativo en USD (RF06 - AndesExpress Logistics)
struct ParametrosCosto {
    double precioCombustible = 1.25; // Pc: USD por litro
    double rendimientoKmL = 10.0;    // R: km por litro
    double velocidadPromKmH = 30.0;  // V_prom: velocidad urbana promedio (km/h)
    double costoHoraChofer = 8.0;    // Co: costo operativo/chofer por hora (USD/h)
    double costoParadaCliente = 2.5; // Cp: costo fijo de entrega por cliente (USD; 0 si j == 0)
};

enum class TipoMetrica {
    EUCLIDIANA,
    HAVERSINE,
    AUTO
};

class Graph {
private:
    int numNodes;
    string instanceName;
    bool geographicMode;
    ParametrosCosto costParams;
    vector<Punto> coordinates;
    vector<vector<double>> adjMatrix;

    static string trim(const string& str);

public:
    explicit Graph(int n = 0, const string& nombre = "Instancia Sin Nombre");

    void addEdge(int u, int v, double weight);
    double getCost(int u, int v) const;
    int getNumNodes() const;
    const string& getInstanceName() const;
    void setInstanceName(const string& nombre);
    bool isGeographic() const;

    const vector<vector<double>>& getMatrix() const;
    const vector<Punto>& getCoordinates() const;

    const ParametrosCosto& getParametrosCosto() const;
    void setParametrosCosto(const ParametrosCosto& params);

    // Funcion de costo operativo real en USD (RF06): Costo_Ruta(i -> j) = Cc + (Co * t) + Cp
    double calcularCostoTramoUSD(int u, int v) const;
    double calcularCostoRutaUSD(const vector<int>& ruta) const;

    // Valida conexidad, simetria, diagonal cero y desigualdad triangular (TSP metrico)
    bool validateGraph() const;

    // Formula de Haversine para coordenadas geograficas reales (latitud, longitud) en km
    static double haversineKm(double lat1, double lon1, double lat2, double lon2);

    // Carga desacoplada desde flujos y archivos CSV externos (RF01, RF02, RF03)
    static Graph fromCSVStream(istream& input,
                               const string& nombreInstancia = "Stream CSV",
                               TipoMetrica metrica = TipoMetrica::AUTO,
                               int maxNodes = -1);

    static Graph loadFromCSV(const string& filepath,
                             TipoMetrica metrica = TipoMetrica::AUTO,
                             int maxNodes = -1);

    // Generador determinista con semilla fija std::mt19937 para instancias grandes (RNF04, RNF06)
    static Graph generateRandomInstance(int n,
                                        unsigned int seed = 42,
                                        double maxCoordKm = 100.0);

    // Instancia piloto escalable sin limite artificial de N <= 20
    static Graph createPilotInstance(int n = 10);
};

#endif
