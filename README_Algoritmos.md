# RouteOpt — Optimización de Rutas Logísticas (AndesExpress Logistics)

Sistema modular en **C++17** para resolver el Problema del Viajante Simétrico (**TSP**) sobre coordenadas reales en Lima Metropolitana (Haversine) y redes simuladas deterministas (`std::mt19937`), integrando **validación estructural y métrica de instancias**, **selección adaptativa con matriz de decisión** e **interpretación clara de métricas experimentales**.

## Compilación y Ejecución (MSYS2 / MinGW-w64)

```bash
g++ -std=c++17 -O2 -Wall -Wextra Graph.cpp Algorithms.cpp main.cpp tests/test_routeopt.cpp -lpsapi -o routeopt.exe
./routeopt.exe
```

Ejecución directa de pruebas automatizadas (35 tests) y suite de benchmarking:

```bash
./routeopt.exe --test
./routeopt.exe --benchmark
python generar_graficas.py
```

## Resumen de Complejidad y Selección Adaptativa

| Algoritmo | Complejidad Temporal | Memoria Auxiliar | Garantía de Calidad | Criterio de Selección Adaptativa |
| :--- | :--- | :--- | :--- | :--- |
| **Backtracking (B&B)** | `O((N - 1)!)` | `O(N)` | **Óptimo Exacto (0% error)** | Instancias pequeñas (`N <= 10`, o `N <= 12` con restricción de RAM). |
| **Held-Karp (DP + Bitmask)** | `O(N^2 * 2^N)` | `O(N * 2^N)` | **Óptimo Exacto (0% error)** | Instancias medianas (`11 <= N <= 20`) dentro del presupuesto de RAM. |
| **MST + DFS (Prim)** | `O(N^2)` | `O(N)` | **2-Aproximación (`<= 2 * OPT`)** | Instancias grandes (`N > 20`) cuando el grafo es métrico y se exige cota formal. |
| **Nearest Neighbor** | `O(N^2)` | `O(N)` | **Heurística Voraz** | Despacho en tiempo real o instancias grandes (`N > 20`) priorizando menor costo empírico. |

## Métricas Experimentales Reportadas

* **Logísticas y Económicas:** Distancia física (`km`), tiempo estimado de conducción (`h/min`), litros de combustible y desglose exacto en `USD` (Combustible + Chofer + Paradas).
* **Calidad de Solución:** Error exacto frente al óptimo global cuando `N <= 20`, y comparación frente a la **Mejor Ruta Observada** + **Cota Inferior Matemática del MST ($W(\text{MST}) \le \text{OPT}$)** cuando `N > 20`.
* **Rendimiento Computacional:** Tiempo en `ms`/`us` (`steady_clock`), factor de aceleración (`Speedup`), memoria propia del algoritmo (`KB`/`MB`) frente a la huella del proceso, y esfuerzo interno (ramas podadas `%`, subconjuntos DP y comparaciones).
