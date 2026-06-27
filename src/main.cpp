#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <stdexcept>
#include "memory.h"
#include "cpu.h"
#include "disasm.h"


static std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

static uint32_t parseUInt(const std::string& s) {
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        return (uint32_t)std::stoul(s, nullptr, 16);
    return (uint32_t)std::stoul(s, nullptr, 0);
}

static void printHelp() {
    std::cout <<
        "\nComandos disponibles:\n"
        "  load <archivo>         Cargar un binario crudo (base 0x00000000)\n"
        "  step [n]               Ejecutar n instrucciones paso a paso (defecto: 1)\n"
        "  run                    Ejecutar hasta ecall exit o ebreak\n"
        "  pc                     Mostrar el contador de programa\n"
        "  regs [reg ...]         Mostrar registros (todos si no se especifican)\n"
        "                           Acepta: x0..x31, zero, ra, sp, a0, t0, ...\n"
        "  mem <inicio> <fin>     Mostrar bytes de memoria en el rango dado\n"
        "  disasm [dir] [n]       Desensamblar n instrucciones desde dir\n"
        "                           (defecto: desde PC, 10 instrucciones)\n"
        "  reset                  Reiniciar CPU (registros y PC, conserva memoria)\n"
        "  fullreset              Reiniciar CPU y limpiar memoria\n"
        "  help                   Mostrar esta ayuda\n"
        "  exit | quit            Salir del simulador\n\n";
}


static std::string fmtHex(uint32_t v) {
    std::ostringstream o;
    o << "0x" << std::hex << std::setw(8) << std::setfill('0') << v;
    return o.str();
}

static void printReg(const CPU& cpu, int n) {
    char name_buf[8];
    std::snprintf(name_buf, sizeof(name_buf), "%-5s", CPU::regName(n));
    std::cout << name_buf
              << " (x" << std::dec << std::setw(2) << std::setfill(' ') << n << "): "
              << fmtHex(cpu.getReg(n))
              << "  (" << (int32_t)cpu.getReg(n) << ")\n";
}

static void printAllRegs(const CPU& cpu) {
    std::cout << "PC       : " << fmtHex(cpu.pc) << "\n";
    for (int i = 0; i < 32; ++i) printReg(cpu, i);
}


int main(int argc, char** argv) {
    Memory mem;
    CPU    cpu(mem);

    std::cout << "=== Simulador RISC-V 32-bit ===\n";
    std::cout << "Escribe 'help' para ver los comandos.\n\n";

    if (argc > 1) {
        if (mem.loadFromFile(argv[1]))
            cpu.reset();
    }

    std::string line;
    while (true) {
        std::cout << "> ";
        std::cout.flush();

        if (!std::getline(std::cin, line)) {
            std::cout << "\nEOF. Saliendo.\n";
            break;
        }

        auto tokens = tokenize(line);
        if (tokens.empty()) continue;

        const std::string& cmd = tokens[0];

        try {
            if (cmd == "exit" || cmd == "quit") {
                std::cout << "Hasta luego...\n";
                break;
            }

            
            else if (cmd == "help") {
                printHelp();
            }

            
            else if (cmd == "load") {
                if (tokens.size() < 2) { std::cout << "Uso: load <archivo>\n"; continue; }
                mem.reset();
                cpu.reset();
                mem.loadFromFile(tokens[1]);
            }

            
            else if (cmd == "step" || cmd == "s") {
                int n = 1;
                if (tokens.size() > 1) n = std::stoi(tokens[1]);
                for (int i = 0; i < n && !cpu.halted; ++i) {
                    uint32_t curPC = cpu.pc;
                    uint32_t instr = mem.read32(curPC);
                    std::cout << "  " << fmtHex(curPC) << ":  "
                              << disassemble(curPC, instr) << "\n";
                    cpu.step();
                }
                std::cout << "PC = " << fmtHex(cpu.pc) << "\n";
            }

            
            else if (cmd == "run" || cmd == "r") {
                uint64_t steps = 0;
                const uint64_t LIMIT = 200'000'000ULL;
                while (!cpu.halted) {
                    cpu.step();
                    if (++steps >= LIMIT) {
                        std::cout << "\n[Limite de " << LIMIT << " instrucciones alcanzado. "
                                     "Use 'step' para continuar.]\n";
                        break;
                    }
                }
                std::cout << steps << " instrucciones ejecutadas.\n";
            }

            
            else if (cmd == "pc") {
                std::cout << "pc = " << fmtHex(cpu.pc) << "\n";
            }

            
            else if (cmd == "regs") {
                if (tokens.size() == 1) {
                    printAllRegs(cpu);
                } else {
                    for (size_t i = 1; i < tokens.size(); ++i) {
                        int n = CPU::regIndex(tokens[i]);
                        if (n < 0)
                            std::cout << "Registro desconocido: " << tokens[i] << "\n";
                        else
                            printReg(cpu, n);
                    }
                }
            }

            
            else if (cmd == "mem") {
                if (tokens.size() < 3) { std::cout << "Uso: mem <inicio> <fin>\n"; continue; }
                uint32_t start = parseUInt(tokens[1]);
                uint32_t end   = parseUInt(tokens[2]);
                mem.dump(start, end);
            }

            
            else if (cmd == "disasm" || cmd == "d") {
                uint32_t addr  = cpu.pc;
                int      count = 10;
                if (tokens.size() > 1) addr  = parseUInt(tokens[1]);
                if (tokens.size() > 2) count = std::stoi(tokens[2]);
                for (int i = 0; i < count; ++i, addr += 4) {
                    uint32_t instr = mem.read32(addr);
                    std::cout << (addr == cpu.pc ? "->" : "  ")
                              << " " << fmtHex(addr) << ":  "
                              << disassemble(addr, instr) << "\n";
                }
            }

            
            else if (cmd == "reset") {
                cpu.reset();
                std::cout << "CPU reiniciada. PC = 0x00000000\n";
            }

            
            else if (cmd == "fullreset") {
                mem.reset();
                cpu.reset();
                std::cout << "CPU y memoria reiniciadas.\n";
            }

            
            else {
                std::cout << "Comando desconocido: '" << cmd << "'. Escribe 'help'.\n";
            }

        } catch (const std::exception& e) {
            std::cerr << "Error de ejecucion: " << e.what() << "\n";
        }
    }

    return 0;
}
