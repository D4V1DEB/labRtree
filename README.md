# labRtree — Comparación R-Tree vs R*-Tree

Implementación de un R-Tree (Guttman, linear split) y un R*-Tree
(re-inserción forzada + split optimizado por overlap), con tres
experimentos de comparación y gráficas para el informe.

## Estructura

```
include/        Clases: rect.hpp, node.hpp, rtree.hpp, rstar.hpp, dataset.hpp
tests/         experiment1/2/3 .cpp/.hpp (lógica de cada experimento)
main.cpp       Orquesta los 3 experimentos y define elapsed()
plot_results.py  Lee los CSV y genera las gráficas (matplotlib)
results/        experimento1/2/3.csv y figures/*.png
```

## Requisitos

- Compilador C++17 (g++ / clang++).
- Python 3 + matplotlib para las gráficas: `pip install matplotlib`.

## Ejecutar (Windows / MSYS2)

```bash
g++ -std=c++17 -O2 -Iinclude main.cpp tests/experiment1.cpp tests/experiment2.cpp tests/experiment3.cpp -o experimentos
.\experimentos
python plot_results.py
```

## Ejecutar (Linux / WSL)

```bash
g++ -std=c++17 -O2 -Iinclude main.cpp tests/experiment1.cpp tests/experiment2.cpp tests/experiment3.cpp -o experimentos
./experimentos
python3 plot_results.py
```

## Métricas medidas (en los 3 experimentos)

- Tiempo de inserción (construcción del árbol).
- Tiempo de búsqueda (promedio de 100 consultas).
- Nodos visitados por búsqueda (refleja la poda del árbol).
