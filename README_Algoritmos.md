# RouteOpt — Sistema de Optimización de Rutas Logísticas (AndesExpress Logistics)

Aplicación modular en **C++17** para resolver el Problema del Viajante Simétrico (**TSP**) sobre instancias geográficas reales (Haversine) y redes simuladas escalables, evaluando distancia física (km), costo operativo real (USD), tiempo de CPU, consumo de memoria y cantidad de estados/operaciones.

## Compilación y Ejecución (MSYS2 / MinGW-w64)

```bash
g++ -std=c++17 -O2 -Wall -Wextra Graph.cpp Algorithms.cpp main.cpp -lpsapi -o routeopt.exe
./routeopt.exe
```

Para ejecutar automáticamente toda la suite de benchmarking y generar las 4 gráficas del informe:

```bash
./routeopt.exe --benchmark
python generar_graficas.py
```

## Complejidad Algorítmica y Límites Operativos

| Estructura / Algoritmo | Complejidad Temporal | Complejidad Espacial | Garantía / Rango Recomendado |
| :--- | :--- | :--- | :--- |
| **Grafo (Matriz de Adyacencia)** | Carga: `O(N^2)` / Consulta: `O(1)` | `O(N^2)` | Soporta métrica Euclidiana y Haversine sin tope fijo de `N`. |
| **1. Backtracking (Branch & Bound)** | `O((N - 1)!)` | `O(N)` | **Óptimo exacto.** Usa cota superior voraz + cota inferior por aristas mínimas salientes (`N <= 12`). |
| **2. Held-Karp (DP + Bitmask)** | `O(N^2 * 2^N)` | `O(N * 2^N)` | **Óptimo exacto.** Tablas lineales contiguas para subconjuntos de clientes (`N <= 20`). |
| **3. MST + DFS (Prim)** | `O(N^2)` | `O(N)` | **2-Aproximación** garantizada en grafos métricos (`Costo <= 2 * OPT`). Escalable a `N >= 200`. |
| **4. Nearest Neighbor** | `O(N^2)` | `O(N)` | **Heurística voraz** ultrarrápida para despacho en tiempo real (`N >= 200`). |

## Modelo de Costo Operativo (USD)

Para cada tramo $(i \to j)$ de distancia $d_{ij}$ (km):

$$\text{Costo\_Ruta}(i \to j) = d_{ij} \cdot \frac{P_c}{R} + C_o \cdot \frac{d_{ij}}{V_{\text{prom}}} + C_p$$

donde $P_c = 1.25\text{ USD/L}$, $R = 10\text{ km/L}$, $V_{\text{prom}} = 30\text{ km/h}$, $C_o = 8.0\text{ USD/h}$, y $C_p = 2.5\text{ USD}$ por parada en cliente ($C_p = 0$ al retornar al almacén central `j = 0`).
