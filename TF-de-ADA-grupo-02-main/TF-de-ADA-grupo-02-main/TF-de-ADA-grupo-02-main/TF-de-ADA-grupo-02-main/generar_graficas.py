import os
import math
import pandas as pd
import matplotlib.pyplot as plt

plt.rcParams["figure.dpi"] = 160
plt.rcParams["savefig.dpi"] = 220
plt.rcParams["font.sans-serif"] = ["Segoe UI", "DejaVu Sans", "Arial"]
plt.rcParams["axes.edgecolor"] = "#CCCCCC"
plt.rcParams["axes.linewidth"] = 0.9

COLORES = {
    "Backtracking (B&B)": "#D62728",
    "Held-Karp (DP)": "#1F77B4",
    "MST + DFS (Prim)": "#FF7F0E",
    "Nearest Neighbor": "#2CA02C",
}

MARCADORES = {
    "Backtracking (B&B)": "s",
    "Held-Karp (DP)": "o",
    "MST + DFS (Prim)": "^",
    "Nearest Neighbor": "D",
}


def graficar_tiempo_vs_n(df_escala: pd.DataFrame) -> None:
    fig, ax = plt.subplots(figsize=(9.5, 5.6))

    for algo, grupo in df_escala.groupby("Algoritmo"):
        grupo = grupo.sort_values("N")
        tiempos = grupo["Tiempo_ms"].clip(lower=1e-4)
        ax.plot(
            grupo["N"],
            tiempos,
            label=algo,
            color=COLORES.get(algo, "#333333"),
            marker=MARCADORES.get(algo, "o"),
            linewidth=2.2,
            markersize=6.5,
        )

    ax.axvline(12, color="#D62728", linestyle="--", alpha=0.45, linewidth=1.2,
               label="Límite seguro Backtracking (N=12)")
    ax.axvline(20, color="#1F77B4", linestyle=":", alpha=0.55, linewidth=1.3,
               label="Límite seguro Held-Karp RAM (N=20)")

    ax.set_yscale("log")
    ax.set_title("RouteOpt — Tiempo de Ejecución vs Tamaño de Instancia (N)",
                 fontsize=13, fontweight="bold", pad=12)
    ax.set_xlabel("Número de Nodos (N = Almacén + Clientes)", fontsize=11)
    ax.set_ylabel("Tiempo de Ejecución en ms (Escala Logarítmica)", fontsize=11)
    ax.grid(True, which="both", linestyle="--", alpha=0.35)
    ax.legend(frameon=True, facecolor="white", framealpha=0.92, fontsize=9.5)
    fig.tight_layout()
    fig.savefig("tiempo_vs_n.png")
    plt.close(fig)


def graficar_memoria_vs_n(df_escala: pd.DataFrame) -> None:
    fig, (ax_lin, ax_log) = plt.subplots(1, 2, figsize=(12.2, 5.3))

    for algo, grupo in df_escala.groupby("Algoritmo"):
        grupo = grupo.sort_values("N")
        color = COLORES.get(algo, "#333333")
        marker = MARCADORES.get(algo, "o")

        ax_lin.plot(grupo["N"], grupo["Memoria_KB"], label=algo,
                    color=color, marker=marker, linewidth=2.2, markersize=6)
        ax_log.plot(grupo["N"], grupo["Memoria_KB"].clip(lower=0.05), label=algo,
                    color=color, marker=marker, linewidth=2.2, markersize=6)

    hk = df_escala[df_escala["Algoritmo"] == "Held-Karp (DP)"].sort_values("N")
    if not hk.empty:
        fila_max = hk.iloc[-1]
        ax_lin.annotate(
            f"Pico O(N·2^N)\nN={int(fila_max['N'])}: {fila_max['Memoria_KB']:,.0f} KB",
            xy=(fila_max["N"], fila_max["Memoria_KB"]),
            xytext=(fila_max["N"] + 8, fila_max["Memoria_KB"] * 0.78),
            arrowprops=dict(arrowstyle="->", color="#1F77B4", lw=1.4),
            fontsize=9.5,
            fontweight="bold",
            color="#1F77B4",
        )

    ax_lin.set_title("Escala Lineal (Impacto Exponencial de Held-Karp)", fontsize=11, fontweight="bold")
    ax_lin.set_xlabel("Número de Nodos (N)", fontsize=10.5)
    ax_lin.set_ylabel("Memoria Asignada / Pico (KB)", fontsize=10.5)
    ax_lin.grid(True, linestyle="--", alpha=0.35)
    ax_lin.legend(fontsize=8.8)

    ax_log.set_yscale("log")
    ax_log.set_title("Escala Logarítmica (Comparativa Completa O(N) vs O(N·2^N))", fontsize=11, fontweight="bold")
    ax_log.set_xlabel("Número de Nodos (N)", fontsize=10.5)
    ax_log.set_ylabel("Memoria en KB (Log)", fontsize=10.5)
    ax_log.grid(True, which="both", linestyle="--", alpha=0.35)
    ax_log.legend(fontsize=8.8)

    fig.suptitle("RouteOpt — Consumo de Memoria vs Tamaño de Instancia (N)",
                 fontsize=13, fontweight="bold", y=1.01)
    fig.tight_layout()
    fig.savefig("memoria_vs_n.png", bbox_inches="tight")
    plt.close(fig)


def graficar_desviacion_vs_n(df: pd.DataFrame) -> None:
    df_escala = df[df["Escenario"] != "Lima_Real"].copy()
    df_lima = df[df["Escenario"] == "Lima_Real"].copy()

    # Filtrar solo registros con referencia de optimo exacto valida (evitar falsear 0% en heuristicas N > 20)
    if "Tipo_Referencia" in df_escala.columns:
        df_validos = df_escala[df_escala["Tipo_Referencia"] == "Optimo Exacto"].copy()
    else:
        df_validos = df_escala[df_escala["Desviacion_pct"] >= 0.0].copy()

    fig, ax = plt.subplots(figsize=(9.8, 5.8))

    for algo, grupo in df_validos.groupby("Algoritmo"):
        grupo = grupo.sort_values("N")
        ax.plot(
            grupo["N"],
            grupo["Desviacion_pct"],
            label=algo,
            color=COLORES.get(algo, "#333333"),
            marker=MARCADORES.get(algo, "o"),
            linewidth=2.2,
            markersize=6.5,
        )

    if not df_lima.empty:
        df_lima_validos = df_lima[df_lima["Desviacion_pct"] >= 0.0]
        for _, row in df_lima_validos.iterrows():
            algo = row["Algoritmo"]
            ax.scatter(
                [row["N"]],
                [row["Desviacion_pct"]],
                color=COLORES.get(algo, "#333333"),
                marker="*",
                s=190,
                edgecolors="black",
                linewidths=0.7,
                zorder=6,
            )
        ax.scatter([], [], color="gray", marker="*", s=150, edgecolors="black",
                   label="Caso Real Lima (N=16, Haversine)")

    max_n = df["N"].max()
    if max_n > 20:
        ax.axvline(20, color="#888888", linestyle=":", alpha=0.7, linewidth=1.2)
        ax.axvspan(20, max_n + 5, color="#F5F5F5", alpha=0.6,
                   label="Zona N > 20 (Óptimo Exacto Incomputable — Desv s/OPT N/A)")

    ax.axhline(0.0, color="#222222", linestyle="-", linewidth=0.9, alpha=0.5)
    ax.set_title("RouteOpt — Desviación Porcentual exclusivamente respecto al Óptimo Exacto (%)",
                 fontsize=12.0, fontweight="bold", pad=12)
    ax.set_xlabel("Número de Nodos (N = Almacén + Clientes)", fontsize=11)
    ax.set_ylabel("Desviación sobre Óptimo Global Exacto (%)", fontsize=11)
    ax.grid(True, linestyle="--", alpha=0.35)
    ax.legend(frameon=True, facecolor="white", framealpha=0.92, fontsize=8.8)
    fig.tight_layout()
    fig.savefig("desviacion_vs_n.png")
    plt.close(fig)


def haversine_km(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    r = 6371.0
    dlat = math.radians(lat2 - lat1)
    dlon = math.radians(lon2 - lon1)
    a = (math.sin(dlat / 2) ** 2 +
         math.cos(math.radians(lat1)) * math.cos(math.radians(lat2)) * math.sin(dlon / 2) ** 2)
    return 2.0 * r * math.atan2(math.sqrt(a), math.sqrt(max(0.0, 1.0 - a)))


def ruta_vecino_mas_cercano_fallback(nodos: pd.DataFrame):
    n = len(nodos)
    lats = nodos["lat_o_x"].tolist()
    lons = nodos["lon_o_y"].tolist()
    vis = [False] * n
    ruta = [0]
    vis[0] = True
    actual = 0
    dist_total = 0.0
    for _ in range(1, n):
        mejor = -1
        min_d = float("inf")
        for j in range(n):
            if not vis[j]:
                d = haversine_km(lats[actual], lons[actual], lats[j], lons[j])
                if d < min_d:
                    min_d = d
                    mejor = j
        vis[mejor] = True
        ruta.append(mejor)
        dist_total += min_d
        actual = mejor
    dist_total += haversine_km(lats[actual], lons[actual], lats[0], lons[0])
    ruta.append(0)
    return ruta, dist_total


def dibujar_ruta_en_mapa(ax, nodos: pd.DataFrame, ruta: list, color_linea: str, titulo: str) -> None:
    lats = nodos["lat_o_x"].to_numpy()
    lons = nodos["lon_o_y"].to_numpy()
    nombres = nodos["nombre"].tolist()

    # Aristas completas tenues de fondo para evidenciar la topologia del grafo
    n = len(nodos)
    for i in range(n):
        for j in range(i + 1, n):
            ax.plot([lons[i], lons[j]], [lats[i], lats[j]],
                    color="#E2E6EA", linewidth=0.5, zorder=1)

    # Tramos dirigidos del ciclo TSP
    for k in range(len(ruta) - 1):
        u, v = ruta[k], ruta[k + 1]
        ax.annotate(
            "",
            xy=(lons[v], lats[v]),
            xytext=(lons[u], lats[u]),
            arrowprops=dict(arrowstyle="-|>", color=color_linea, lw=1.9, mutation_scale=12),
            zorder=3,
        )

    # Clientes (Nodos 1..N-1)
    ax.scatter(lons[1:], lats[1:], c="#1D3557", s=62, zorder=4, label="Clientes / Sedes (1..N-1)")

    # Almacen Central (Nodo 0)
    ax.scatter([lons[0]], [lats[0]], c="#E63946", marker="*", s=260,
               edgecolors="black", linewidths=0.9, zorder=5, label="Almacén Central (Nodo 0)")

    for i in range(n):
        etiqueta_corta = nombres[i].split("(")[0].replace("Centro Distribucion ", "").replace("Parque Industrial ", "").replace("Terminal Portuario ", "").replace("Centro Empresarial ", "").replace("Centro Logistico ", "").replace("Agencia ", "").replace("Punto Entrega ", "").replace("Tienda ", "").replace("Planta ", "").replace("Sede ", "").replace("Hub ", "").replace("Oficina ", "").replace("Sucursal ", "").replace("Almacen Central AndesExpress", "Almacén Central").strip()
        ax.annotate(
            f"{i}:{etiqueta_corta}",
            (lons[i], lats[i]),
            textcoords="offset points",
            xytext=(4, 4),
            fontsize=7.6,
            fontweight="bold" if i == 0 else "normal",
            color="#9B0000" if i == 0 else "#1B263B",
            zorder=6,
        )

    ax.set_title(titulo, fontsize=10.8, fontweight="bold", pad=8)
    ax.set_xlabel("Longitud (°O)", fontsize=9.8)
    ax.set_ylabel("Latitud (°S)", fontsize=9.8)
    ax.grid(True, linestyle=":", alpha=0.45)
    ax.legend(loc="lower left", fontsize=8.3, frameon=True, facecolor="white", framealpha=0.9)


def graficar_grafo_rutas() -> None:
    ruta_nodos = "data/lima_real.csv" if os.path.exists("data/lima_real.csv") else "data/instancia_pequena.csv"
    nodos = pd.read_csv(ruta_nodos)

    ruta_benchmarks = "data/rutas_lima_benchmark.csv"
    if os.path.exists(ruta_benchmarks):
        df_rutas = pd.read_csv(ruta_benchmarks)
        filas = {row["Algoritmo"]: row for _, row in df_rutas.iterrows()}

        opt_row = filas.get("Held-Karp (DP)", filas.get("Backtracking (B&B)"))
        nn_row = filas.get("Nearest Neighbor")
        mst_row = filas.get("MST + DFS (Prim)")

        fig, axes = plt.subplots(1, 3, figsize=(17.5, 6.0))

        for ax, row, prefijo in [
            (axes[0], opt_row, "Óptimo Exacto"),
            (axes[1], nn_row, "Heurística Voraz"),
            (axes[2], mst_row, "2-Aproximación"),
        ]:
            if row is None:
                continue
            secuencia = [int(x) for x in str(row["Ruta"]).split("-")]
            algo = row["Algoritmo"]
            titulo = (
                f"{prefijo}: {algo}\n"
                f"Dist: {row['Distancia_km']:.2f} km | Costo: ${row['Costo_USD']:.2f} USD "
                f"(+{row['Desviacion_pct']:.2f}%)"
            )
            dibujar_ruta_en_mapa(ax, nodos, secuencia, COLORES.get(algo, "#1F77B4"), titulo)

        fig.suptitle(
            "AndesExpress Logistics — Comparación Geográfica de Rutas TSP en Lima Metropolitana (N = 16)",
            fontsize=13.5,
            fontweight="bold",
            y=1.02,
        )
        fig.tight_layout()
        fig.savefig("grafo_rutas.png", bbox_inches="tight")
        plt.close(fig)
    else:
        fig, ax = plt.subplots(figsize=(8.5, 6.5))
        ruta, dist = ruta_vecino_mas_cercano_fallback(nodos)
        dibujar_ruta_en_mapa(
            ax, nodos, ruta, "#2CA02C",
            f"Ruta Calculada Nearest Neighbor — {dist:.2f} km"
        )
        fig.tight_layout()
        fig.savefig("grafo_rutas.png", bbox_inches="tight")
        plt.close(fig)


def main() -> None:
    archivo_csv = "resultados_benchmark.csv"
    if not os.path.exists(archivo_csv):
        raise FileNotFoundError(
            f"No se encontró '{archivo_csv}'. Ejecute primero 'routeopt.exe --benchmark' o la opción [9] del menú."
        )

    df = pd.read_csv(archivo_csv)
    df_escala = df[df["Escenario"] != "Lima_Real"].copy()

    graficar_tiempo_vs_n(df_escala)
    graficar_memoria_vs_n(df_escala)
    graficar_desviacion_vs_n(df)
    graficar_grafo_rutas()

    print("[OK] Gráficas generadas correctamente:")
    print("  1) tiempo_vs_n.png")
    print("  2) memoria_vs_n.png")
    print("  3) desviacion_vs_n.png")
    print("  4) grafo_rutas.png")


if __name__ == "__main__":
    main()

