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
    fig, ax = plt.subplots(figsize=(10.2, 5.8))

    max_n = int(df_escala["N"].max())
    ax.axvspan(0, 12, color="#E8F5E9", alpha=0.35, label="Zona Pequeña (N ≤ 12: 4 algoritmos viables)")
    ax.axvspan(12, 20, color="#E3F2FD", alpha=0.38, label="Zona Mediana (13–20: Held-Karp exacto viable)")
    ax.axvspan(20, max_n + 6, color="#FFF8E1", alpha=0.45, label="Zona Grande (N > 20: Solo Heurísticas O(N²))")

    for algo, grupo in df_escala.groupby("Algoritmo"):
        grupo = grupo.sort_values("N")
        tiempos = grupo["Tiempo_ms"].clip(lower=1e-4)
        ax.plot(
            grupo["N"],
            tiempos,
            label=algo,
            color=COLORES.get(algo, "#333333"),
            marker=MARCADORES.get(algo, "o"),
            linewidth=2.3,
            markersize=6.8,
        )

    hk = df_escala[df_escala["Algoritmo"] == "Held-Karp (DP)"].sort_values("N")
    if not hk.empty:
        top_hk = hk.iloc[-1]
        ax.annotate(
            f"N={int(top_hk['N'])}: {top_hk['Tiempo_ms']:.1f} ms",
            xy=(top_hk["N"], top_hk["Tiempo_ms"]),
            xytext=(top_hk["N"] + 6, top_hk["Tiempo_ms"] * 0.65),
            arrowprops=dict(arrowstyle="->", color="#1F77B4", lw=1.3),
            fontsize=9,
            fontweight="bold",
            color="#1F77B4",
        )

    ax.set_xlim(6, max_n + 6)
    ax.set_yscale("log")
    ax.set_title("RouteOpt — Tiempo de Ejecución vs Tamaño de Red (Escala Logarítmica)",
                 fontsize=13, fontweight="bold", pad=12)
    ax.set_xlabel("Número de Nodos (N = Almacén Central + Clientes)", fontsize=11)
    ax.set_ylabel("Tiempo Promedio de CPU en ms (Escala Log)", fontsize=11)
    ax.grid(True, which="both", linestyle="--", alpha=0.35)
    ax.legend(loc="center right", bbox_to_anchor=(0.99, 0.62), frameon=True,
              facecolor="white", framealpha=0.94, fontsize=8.8)
    fig.tight_layout()
    fig.savefig("tiempo_vs_n.png")
    plt.close(fig)


def graficar_memoria_vs_n(df_escala: pd.DataFrame) -> None:
    col_mem = "Memoria_Aux_KB" if "Memoria_Aux_KB" in df_escala.columns else "Memoria_KB"
    fig, (ax_lin, ax_log) = plt.subplots(1, 2, figsize=(12.6, 5.4))

    for algo, grupo in df_escala.groupby("Algoritmo"):
        grupo = grupo.sort_values("N")
        color = COLORES.get(algo, "#333333")
        marker = MARCADORES.get(algo, "o")
        mem_mb = grupo[col_mem] / 1024.0

        ax_lin.plot(grupo["N"], mem_mb, label=algo,
                    color=color, marker=marker, linewidth=2.2, markersize=6)
        ax_log.plot(grupo["N"], grupo[col_mem].clip(lower=0.03), label=algo,
                    color=color, marker=marker, linewidth=2.2, markersize=6)

    hk = df_escala[df_escala["Algoritmo"] == "Held-Karp (DP)"].sort_values("N")
    if not hk.empty:
        fila_max = hk.iloc[-1]
        mb_max = fila_max[col_mem] / 1024.0
        ax_lin.annotate(
            f"Explosión O(N·2^N)\nN={int(fila_max['N'])}: {mb_max:.1f} MB\n({fila_max[col_mem]:,.0f} KB)",
            xy=(fila_max["N"], mb_max),
            xytext=(fila_max["N"] + 15, mb_max * 0.72),
            arrowprops=dict(arrowstyle="->", color="#1F77B4", lw=1.4),
            fontsize=9.3,
            fontweight="bold",
            color="#1F77B4",
        )

    ax_lin.set_title("En Megabytes (MB) — Muro de RAM de Held-Karp", fontsize=11, fontweight="bold")
    ax_lin.set_xlabel("Número de Nodos (N)", fontsize=10.5)
    ax_lin.set_ylabel("Memoria Estructural Requerida (MB)", fontsize=10.5)
    ax_lin.grid(True, linestyle="--", alpha=0.35)
    ax_lin.legend(fontsize=8.8)

    ax_log.set_yscale("log")
    ax_log.set_title("En Kilobytes (Escala Log) — O(N) Lineal vs O(N·2^N)", fontsize=11, fontweight="bold")
    ax_log.set_xlabel("Número de Nodos (N)", fontsize=10.5)
    ax_log.set_ylabel("Memoria Auxiliar del Algoritmo en KB (Log)", fontsize=10.5)
    ax_log.grid(True, which="both", linestyle="--", alpha=0.35)
    ax_log.legend(fontsize=8.8)

    fig.suptitle("RouteOpt — Comparación Clara de Consumo de Memoria RAM por Algoritmo",
                 fontsize=13, fontweight="bold", y=1.01)
    fig.tight_layout()
    fig.savefig("memoria_vs_n.png", bbox_inches="tight")
    plt.close(fig)


def graficar_desviacion_vs_n(df: pd.DataFrame) -> None:
    df_escala = df[df["Escenario"] != "Lima_Real"].copy()
    df_lima = df[df["Escenario"] == "Lima_Real"].copy()

    df_peq_med = df_escala[df_escala["Desviacion_pct"].notna() & (df_escala["Desviacion_pct"] >= 0.0)].copy()
    df_grande = df_escala[df_escala["N"] > 20].copy()

    fig, (ax_opt, ax_grd) = plt.subplots(1, 2, figsize=(13.2, 5.6))

    # Panel 1: Error exacto comprobado respecto al Optimo Matematico (N <= 20)
    for algo, grupo in df_peq_med.groupby("Algoritmo"):
        grupo = grupo.sort_values("N")
        ax_opt.plot(
            grupo["N"],
            grupo["Desviacion_pct"],
            label=algo,
            color=COLORES.get(algo, "#333333"),
            marker=MARCADORES.get(algo, "o"),
            linewidth=2.2,
            markersize=6.5,
        )

    if not df_lima.empty:
        df_lima_ok = df_lima[df_lima["Desviacion_pct"].notna() & (df_lima["Desviacion_pct"] >= 0.0)]
        for _, row in df_lima_ok.iterrows():
            algo = row["Algoritmo"]
            ax_opt.scatter(
                [row["N"]],
                [row["Desviacion_pct"]],
                color=COLORES.get(algo, "#333333"),
                marker="*",
                s=200,
                edgecolors="black",
                linewidths=0.8,
                zorder=6,
            )
        ax_opt.scatter([], [], color="gray", marker="*", s=150, edgecolors="black",
                       label="Caso Real Lima (N=16)")

    ax_opt.axhline(0.0, color="#222222", linestyle="-", linewidth=0.9, alpha=0.5)
    ax_opt.set_title("Escala N ≤ 20: Error Exacto sobre el Óptimo Global (%)",
                     fontsize=11, fontweight="bold")
    ax_opt.set_xlabel("Número de Nodos (N)", fontsize=10.5)
    ax_opt.set_ylabel("Error Exacto respecto a OPT (%)", fontsize=10.5)
    ax_opt.grid(True, linestyle="--", alpha=0.35)
    ax_opt.legend(fontsize=8.5)

    # Panel 2: Gran escala (N > 20) usando Cota Inferior Matematica MST (W(MST) <= OPT)
    col_cota = "Brecha_Max_MST_pct" if "Brecha_Max_MST_pct" in df_grande.columns else "Dif_Mejor_Observado_pct"
    if not df_grande.empty and col_cota in df_grande.columns:
        for algo, grupo in df_grande.groupby("Algoritmo"):
            grupo = grupo.sort_values("N")
            ax_grd.plot(
                grupo["N"],
                grupo[col_cota],
                label=f"{algo} (vs Cota MST)",
                color=COLORES.get(algo, "#333333"),
                marker=MARCADORES.get(algo, "o"),
                linewidth=2.2,
                markersize=6.5,
            )
        ax_grd.axhline(100.0, color="#FF7F0E", linestyle="--", linewidth=1.2, alpha=0.7,
                       label="Tope teórico 2-Aproximación (+100% s/MST)")

    ax_grd.set_title("Gran Escala N > 20: Cota Máxima de Error vs MST (%)",
                     fontsize=11, fontweight="bold")
    ax_grd.set_xlabel("Número de Nodos (N = 30 a 200)", fontsize=10.5)
    ax_grd.set_ylabel("Exceso sobre Cota Inferior MST (%) [Error Real ≤ este valor]", fontsize=9.8)
    ax_grd.grid(True, linestyle="--", alpha=0.35)
    ax_grd.legend(fontsize=8.5)

    fig.suptitle("RouteOpt — Evaluación Transparente de Calidad de Ruta en Pequeña, Mediana y Gran Escala",
                 fontsize=12.8, fontweight="bold", y=1.02)
    fig.tight_layout()
    fig.savefig("desviacion_vs_n.png", bbox_inches="tight")
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

    n = len(nodos)
    for i in range(n):
        for j in range(i + 1, n):
            ax.plot([lons[i], lons[j]], [lats[i], lats[j]],
                    color="#E2E6EA", linewidth=0.5, zorder=1)

    for k in range(len(ruta) - 1):
        u, v = ruta[k], ruta[k + 1]
        ax.annotate(
            "",
            xy=(lons[v], lats[v]),
            xytext=(lons[u], lats[u]),
            arrowprops=dict(arrowstyle="-|>", color=color_linea, lw=1.9, mutation_scale=12),
            zorder=3,
        )

    ax.scatter(lons[1:], lats[1:], c="#1D3557", s=62, zorder=4, label="Clientes / Sedes (1..15)")
    ax.scatter([lons[0]], [lats[0]], c="#E63946", marker="*", s=260,
               edgecolors="black", linewidths=0.9, zorder=5, label="Almacén Central (Nodo 0)")

    for i in range(n):
        etiqueta_corta = (
            nombres[i].split("(")[0]
            .replace("Centro Distribucion ", "")
            .replace("Parque Industrial ", "")
            .replace("Terminal Portuario ", "")
            .replace("Centro Empresarial ", "")
            .replace("Centro Logistico ", "")
            .replace("Agencia ", "")
            .replace("Punto Entrega ", "")
            .replace("Tienda ", "")
            .replace("Planta ", "")
            .replace("Sede ", "")
            .replace("Hub ", "")
            .replace("Oficina ", "")
            .replace("Sucursal ", "")
            .replace("Almacen Central AndesExpress", "Almacén Central")
            .strip()
        )
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

        ahorro_txt = ""
        if opt_row is not None and nn_row is not None:
            ahorro_km = nn_row["Distancia_km"] - opt_row["Distancia_km"]
            ahorro_usd = nn_row["Costo_USD"] - opt_row["Costo_USD"]
            ahorro_txt = f" — Ahorro del Óptimo vs Voraz: {ahorro_km:.2f} km y ${ahorro_usd:.2f} USD por viaje"

        fig.suptitle(
            f"AndesExpress Logistics — Comparación de Rutas en Lima Metropolitana (N = 16){ahorro_txt}",
            fontsize=13.0,
            fontweight="bold",
            y=1.02,
        )
        fig.tight_layout()
        fig.savefig("grafo_rutas.png", bbox_inches="tight")
        plt.close(fig)
    else:
        fig, ax = plt.subplots(figsize=(8.5, 6.5))
        ruta, dist = ruta_vecino_mas_cercano_fallback(nodos)
        dibujar_ruta_en_mapa(ax, nodos, ruta, "#2CA02C", f"Ruta Nearest Neighbor — {dist:.2f} km")
        fig.tight_layout()
        fig.savefig("grafo_rutas.png", bbox_inches="tight")
        plt.close(fig)


def main() -> None:
    archivo_csv = "resultados_benchmark.csv"
    if not os.path.exists(archivo_csv):
        raise FileNotFoundError(
            f"No se encontró '{archivo_csv}'. Ejecute primero 'routeopt.exe --benchmark' o la opción [11] del menú."
        )

    df = pd.read_csv(archivo_csv, na_values=["N/A"])
    df_escala = df[df["Escenario"] != "Lima_Real"].copy()

    graficar_tiempo_vs_n(df_escala)
    graficar_memoria_vs_n(df_escala)
    graficar_desviacion_vs_n(df)
    graficar_grafo_rutas()

    print("[OK] 4 gráficas experimentales actualizadas correctamente:")
    print("  1) tiempo_vs_n.png")
    print("  2) memoria_vs_n.png")
    print("  3) desviacion_vs_n.png")
    print("  4) grafo_rutas.png")


if __name__ == "__main__":
    main()
