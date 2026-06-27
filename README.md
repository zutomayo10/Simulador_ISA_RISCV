# Simulador RISC-V 32-bit (RV32I)

Simulador de la arquitectura RISC-V de 32 bits con interfaz de línea de comandos interactiva. Implementa el conjunto base de instrucciones RV32I completo (37 instrucciones) según el estándar RISC-V.

---

## Estructura del proyecto

```
tarea6_arqui/
├── src/
│   ├── main.cpp       # CLI interactiva y punto de entrada
│   ├── memory.h/.cpp  # Memoria sparse por páginas, little-endian
│   ├── cpu.h/.cpp     # Estado de CPU y ejecución de instrucciones
│   └── disasm.h/.cpp  # Desensamblador de instrucciones
├── tests/
│   ├── gen_tests.py       # Genera riscvtest.bin y sum_loop.bin
│   ├── gen_ecall_test.py  # Genera ecall_test.bin
│   ├── riscvtest.bin      # Test: add/sub/slt/lw/sw/beq/jal
│   ├── sum_loop.bin       # Test: loop con bge (suma 1..10)
│   └── ecall_test.bin     # Test: syscalls de impresión
├── build/
│   └── riscv-sim          # Binario compilado (se genera al construir)
├── build.sh               # Script de compilación (usar en WSL)
├── run.ps1                # Lanzador desde PowerShell (auto-compila)
└── Makefile               # Alternativa con make
```

---

## Requisitos

- **WSL (Windows Subsystem for Linux)** con Ubuntu instalado
- **g++ 13+** con soporte C++17 (`sudo apt install build-essential`)
- Python 3 (solo para regenerar los binarios de prueba)

---

## Compilación

### Opción A — Desde PowerShell (recomendado)

```powershell
.\run.ps1
```

El script detecta si el binario existe, compila automáticamente si no, y lanza el simulador.

### Opción B — Desde WSL directamente

```bash
cd /mnt/c/Users/Isaac/Desktop/tarea6_arqui
bash build.sh
```

Esto produce `build/riscv-sim`.

---

## Cómo ejecutar el simulador

### Lanzar con un programa

```powershell
# Desde PowerShell
.\run.ps1 tests/riscvtest.bin

# Desde WSL
./build/riscv-sim tests/riscvtest.bin
```

### Lanzar sin programa (modo interactivo vacío)

```powershell
.\run.ps1
```

Al iniciar, el simulador muestra:

```
=== Simulador RISC-V 32-bit ===
Escribe 'help' para ver los comandos.
>
```

---

## Comandos disponibles

| Comando | Descripción |
|---|---|
| `load <archivo>` | Carga un binario crudo en memoria desde la dirección `0x00000000` |
| `step [n]` | Ejecuta `n` instrucciones paso a paso mostrando el desensamblado (defecto: 1) |
| `run` | Ejecuta hasta `ecall 10` (exit) o `ebreak` (límite: 200 millones de instrucciones) |
| `pc` | Muestra el valor actual del Program Counter |
| `regs [reg ...]` | Muestra todos los registros, o solo los indicados |
| `mem <inicio> <fin>` | Vuelca bytes de memoria en el rango dado (en hex) |
| `disasm [dir] [n]` | Desensambla `n` instrucciones desde `dir` (defecto: PC, 10 instrucciones) |
| `reset` | Reinicia la CPU (registros y PC) conservando la memoria cargada |
| `fullreset` | Reinicia CPU y borra toda la memoria |
| `help` | Muestra la ayuda |
| `exit` / `quit` | Sale del simulador |

### Nota sobre `regs`

Acepta nombres en formato ABI o numérico:

```
> regs a0 a1 t0          # por nombre ABI
> regs x10 x11 x5        # por número
> regs                   # todos los 32 registros + PC
```

### Nota sobre `mem` y `disasm`

Las direcciones pueden ser decimales o hexadecimales:

```
> mem 0x1000 0x101F
> mem 100 115
> disasm 0x00000000 20
```

---

## Instrucciones implementadas (37 en total)

### Loads (`opcode 0x03`)
| Instrucción | Descripción |
|---|---|
| `lb rd, imm(rs1)` | Carga byte con signo extendido |
| `lh rd, imm(rs1)` | Carga halfword con signo extendido |
| `lw rd, imm(rs1)` | Carga word (32 bits) |
| `lbu rd, imm(rs1)` | Carga byte sin signo (zero-extended) |
| `lhu rd, imm(rs1)` | Carga halfword sin signo (zero-extended) |

### Inmediatas (`opcode 0x13`)
| Instrucción | Descripción |
|---|---|
| `addi rd, rs1, imm` | Suma con inmediato con signo |
| `slli rd, rs1, shamt` | Desplazamiento lógico izquierda |
| `slti rd, rs1, imm` | Set if less than (signed) |
| `sltiu rd, rs1, imm` | Set if less than (unsigned) |
| `xori rd, rs1, imm` | XOR con inmediato |
| `srli rd, rs1, shamt` | Desplazamiento lógico derecha |
| `srai rd, rs1, shamt` | Desplazamiento aritmético derecha |
| `ori rd, rs1, imm` | OR con inmediato |
| `andi rd, rs1, imm` | AND con inmediato |

### Registro-Registro (`opcode 0x33`)
| Instrucción | Descripción |
|---|---|
| `add rd, rs1, rs2` | Suma |
| `sub rd, rs1, rs2` | Resta |
| `sll rd, rs1, rs2` | Desplazamiento lógico izquierda por registro |
| `slt rd, rs1, rs2` | Set if less than (signed) |
| `sltu rd, rs1, rs2` | Set if less than (unsigned) |
| `xor rd, rs1, rs2` | XOR |
| `srl rd, rs1, rs2` | Desplazamiento lógico derecha por registro |
| `sra rd, rs1, rs2` | Desplazamiento aritmético derecha por registro |
| `or rd, rs1, rs2` | OR |
| `and rd, rs1, rs2` | AND |

### Stores (`opcode 0x23`)
| Instrucción | Descripción |
|---|---|
| `sb rs2, imm(rs1)` | Escribe byte |
| `sh rs2, imm(rs1)` | Escribe halfword |
| `sw rs2, imm(rs1)` | Escribe word |

### Saltos condicionales (`opcode 0x63`)
| Instrucción | Descripción |
|---|---|
| `beq rs1, rs2, label` | Branch if equal |
| `bne rs1, rs2, label` | Branch if not equal |
| `blt rs1, rs2, label` | Branch if less than (signed) |
| `bge rs1, rs2, label` | Branch if greater or equal (signed) |
| `bltu rs1, rs2, label` | Branch if less than (unsigned) |
| `bgeu rs1, rs2, label` | Branch if greater or equal (unsigned) |

### Saltos incondicionales
| Instrucción | Descripción |
|---|---|
| `jal rd, label` | Jump and link — salta, guarda PC+4 en rd |
| `jalr rd, imm(rs1)` | Jump and link register |

### Inmediatas superiores
| Instrucción | Descripción |
|---|---|
| `lui rd, imm` | Load upper immediate (bits 31:12) |
| `auipc rd, imm` | Add upper immediate to PC |

### Sistema
| Instrucción | Descripción |
|---|---|
| `ecall` | Llamada al sistema (ver tabla de syscalls) |
| `ebreak` | Detiene la CPU (breakpoint) |
| `fence` | Barrera de memoria (NOP en este simulador) |

---

## Syscalls (ecall)

El simulador implementa las syscalls compatibles con SPIM/CPUlator:

| `a7` | Syscall | Acción |
|---|---|---|
| 1 | `print_int` | Imprime `a0` como entero con signo |
| 4 | `print_string` | Imprime cadena terminada en `\0` desde dirección `a0` |
| 5 | `read_int` | Lee un entero de stdin → guarda en `a0` |
| 10 | `exit` | Termina la ejecución (código de salida en `a0`) |
| 11 | `print_char` | Imprime el carácter en `a0` |
| 17 | `exit2` | Igual que exit |

---

## Pseudoinstrucciones reconocidas por el desensamblador

El desensamblador detecta y muestra las siguientes pseudoinstrucciones cuando correspondan:

| Patrón codificado | Pseudoinstrucción mostrada |
|---|---|
| `jalr zero, ra, 0` | `ret` |
| `jal x0, target` | `j 0xXXXXXXXX` |
| `jal ra, target` | `call 0xXXXXXXXX` |
| `addi zero, zero, 0` | `nop` |
| `jalr zero, rs1, 0` | `jr rs1` |

---

## Cómo funciona internamente

### Memoria

La memoria usa un modelo **sparse por páginas** (`std::unordered_map` de páginas de 4 KB). Esto permite simular el espacio de direcciones completo de 32 bits sin alocar 4 GB. Las lecturas a páginas no mapeadas devuelven `0`; las escrituras crean la página automáticamente. El orden de bytes es **little-endian**, conforme a la especificación RISC-V.

### CPU

El estado de la CPU es:
- `pc`: Program Counter (inicia en `0x00000000`)
- `regs[32]`: 32 registros de propósito general (x0 siempre es 0)
- `sp` (x2): inicializado en `0x7FFFFFFC` (compatible con CPUlator)
- `halted`: flag que se activa con `ecall 10` o `ebreak`

En cada `step()`, la CPU:
1. Lee la instrucción de 32 bits en `mem[pc]`
2. Extrae todos los campos: opcode, rd, rs1, rs2, funct3, funct7
3. Calcula los inmediatos con signo extendido según el tipo (I, S, B, U, J)
4. Despacha por opcode y ejecuta
5. Avanza el PC (o salta si es branch/jal)

### Extracción de inmediatos

La codificación RISC-V distribuye los bits del inmediato en posiciones no contiguas para simplificar el hardware. El simulador los reconstruye con las fórmulas exactas del estándar:

```
I-type: instr[31:20]  (sign-extended desde bit 31)
S-type: instr[31:25] | instr[11:7]
B-type: instr[31] | instr[7] | instr[30:25] | instr[11:8]  (múltiplo de 2)
U-type: instr[31:12] << 12
J-type: instr[31] | instr[19:12] | instr[20] | instr[30:21]  (múltiplo de 2)
```

---

## Paso a paso: probar el simulador

### Prueba 1 — `riscvtest` (add, sub, slt, lw, sw, beq, jal)

Este test verifica operaciones aritméticas, accesos a memoria y saltos.
**Resultado esperado:** `x7=25`, `x8=10`, `x11=1`, `x14=25`, `mem[100]=0x19`

```powershell
.\run.ps1 tests/riscvtest.bin
```

Dentro del simulador:

```
> run
> regs x7 x8 x11 x14
> mem 100 103
```

Salida esperada:

```
t2    (x7 ): 0x00000019  (25)
s0    (x8 ): 0x0000000a  (10)
a1    (x11): 0x00000001  (1)
a4    (x14): 0x00000019  (25)

Memoria (0x00000064 - 0x00000067):
  0x00000064: 19 00 00 00
```

---

### Prueba 2 — `sum_loop` (loop con bge, suma 1+2+...+10=55)

Este test verifica el correcto funcionamiento de bucles con branch condicional.
**Resultado esperado:** `x6=55`, `mem[200]=0x37`

```powershell
.\run.ps1 tests/sum_loop.bin
```

Dentro del simulador:

```
> run
> regs x6
> mem 200 203
```

Salida esperada:

```
t1    (x6 ): 0x00000037  (55)

Memoria (0x000000c8 - 0x000000cb):
  0x000000c8: 37 00 00 00
```

---

### Prueba 3 — `ecall_test` (syscalls de impresión)

Este test verifica las llamadas al sistema para imprimir enteros, caracteres y cadenas.
**Resultado esperado:** imprime `42`, `Hola RISC-V!` y `2025`

```powershell
.\run.ps1 tests/ecall_test.bin
```

Dentro del simulador:

```
> run
```

Salida esperada en consola:

```
42
Hola RISC-V!
2025

Programa finalizado (ecall 10). Codigo de salida: 0
```

---

### Prueba 4 — Ejecución paso a paso con desensamblado

Para ver en detalle cómo el simulador ejecuta instrucción por instrucción:

```powershell
.\run.ps1 tests/riscvtest.bin
```

```
> disasm 0x0 10       # ver las primeras 10 instrucciones
> step 5              # ejecutar 5 instrucciones
> regs x5 x6 x7      # inspeccionar registros intermedios
> step 3
> pc                  # ver dónde está el PC
> mem 0x64 0x67       # inspeccionar memoria en addr 100
```

---

### Prueba 5 — Cargar un programa de CPUlator

Para usar programas del simulador CPUlator (Apéndice B del enunciado):

1. En CPUlator, compilar el programa en RISC-V
2. Ir a la pestaña **Memory**
3. Seleccionar formato **Raw binary**
4. Guardar el archivo (p.ej. `quicksort.bin`)
5. Copiar el archivo a la carpeta `tarea6_arqui/`

```powershell
.\run.ps1 quicksort.bin
```

```
> run
> mem 0x1000 0x101B    # ver el arreglo resultante
```

---

### Regenerar los binarios de prueba

Si necesitas recrear los archivos `.bin` desde cero:

```bash
# Desde WSL o Python instalado
python3 tests/gen_tests.py
python3 tests/gen_ecall_test.py
```

---

## Registro de nombres ABI

| Registro | Nombre ABI | Uso convencional |
|---|---|---|
| x0 | zero | Siempre 0 |
| x1 | ra | Return address |
| x2 | sp | Stack pointer |
| x3 | gp | Global pointer |
| x4 | tp | Thread pointer |
| x5–x7 | t0–t2 | Temporales |
| x8 | s0/fp | Saved / frame pointer |
| x9 | s1 | Saved |
| x10–x11 | a0–a1 | Argumentos / valores de retorno |
| x12–x17 | a2–a7 | Argumentos / número de syscall |
| x18–x27 | s2–s11 | Saved registers |
| x28–x31 | t3–t6 | Temporales |
