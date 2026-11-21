#include "bitboard/bitboard.h"
#include "uci/uci.h"

int main() {
    init_attacks();
    uci_loop();
    return 0;
}
