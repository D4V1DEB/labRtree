#include <ctime>

#include "tests/experiment1.hpp"
#include "tests/experiment2.hpp"
#include "tests/experiment3.hpp"

// Funcion para medir tiempo (milisegundos de CPU, portable en Linux/WSL).
// Usa clock() en vez de tiempo de pared: estable bajo carga de la maquina.
double elapsed(bool reset = false) {
    static clock_t start = clock();
    if (reset) start = clock();
    return (1000.0 * double(clock() - start)) / double(CLOCKS_PER_SEC);
}

double elapsed() {
    return elapsed(false);
}

int main() {
    runExperiment1();
    runExperiment2();
    runExperiment3();
    return 0;
}
