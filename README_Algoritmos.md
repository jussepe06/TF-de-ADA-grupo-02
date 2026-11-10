# RouteOpt - Avance 2 (Grupo 02)

Prototipo algorítmico en C++ para resolver el Problema del Viajante (TSP simétrico) aplicado al ruteo desde un almacén central (nodo 0) hacia $n$ clientes.

## Cómo compilar y ejecutar

```bash
g++ -std=c++17 -Wall -Wextra Graph.cpp Algorithms.cpp main.cpp -o routeopt
./routeopt
```

## Resumen de Complejidades

Para un grafo completo de $N$ nodos representado con matriz de adyacencia:

| Estructura / Algoritmo | Complejidad Temporal | Complejidad Espacial | Observaciones |
| :--- | :--- | :--- | :--- |
| **Grafo (Matriz de Adyacencia)** | Inicialización: `O(N^2)` / Consulta: `O(1)` | `O(N^2)` | Guarda las distancias simétricas entre todos los nodos. |
| **1. Backtracking (con poda)** | `O((N - 1)!)` $\approx$ `O(N!)` | `O(N)` | Exacto. Fija el nodo 0 como inicio y poda ramas cuando el costo acumulado ya supera al mejor costo encontrado. |
| **2. Held-Karp (PD + Bitmask)** | `O(N^2 * 2^N)` | `O(N * 2^N)` | Exacto. Usa una máscara de bits para representar los clientes visitados y evita recalcular subproblemas. |
| **3. MST + DFS (Prim)** | `O(N^2)` | `O(N)` | Aproximado (2-aproximación). Arma el árbol de expansión mínima con Prim y hace un recorrido DFS en preorden. |
| **4. Nearest Neighbor** | `O(N^2)` | `O(N)` | Heurístico voraz. Desde el nodo actual siempre elige ir al nodo no visitado más cercano. |

## Estructura del proyecto

* `Graph.h` / `Graph.cpp`: Clase `Graph` con matriz de adyacencia y carga de datos de prueba simulando lectura de archivo con `stringstream`.
* `Algorithms.h` / `Algorithms.cpp`: Implementación de los 4 algoritmos solicitados y la clase `Profiler` con `<chrono>`.
* `main.cpp`: Caso de prueba piloto con $N = 10$ nodos que compara costos, diferencia porcentual y tiempos de ejecución.
