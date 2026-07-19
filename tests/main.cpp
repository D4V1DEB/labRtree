#include <ctime>

#include "experiment1.hpp"

// Funcion para medir tiempo (milisegundos de CPU, portable en Linux/WSL).
// Usa clock() en vez de tiempo de pared: estable bajo carga de la maquina.
inline double elapsed(bool reset = false) {
    static clock_t start = clock();
    if (reset) start = clock();
    return (1000.0 * double(clock() - start)) / double(CLOCKS_PER_SEC);
}

double elapsed() {
    return elapsed(false);
}

int main() {
    runExperiment1();
    return 0;
}
