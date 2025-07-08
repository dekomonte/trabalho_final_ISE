//Trabalho Final: Jogos Digitais na DE1-SoC
//Andressa Sena - 16/0112303
//Brayan Barbosa - 17/0030229

//------------------
//Bibliotecas - Template (padrao)
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

//Macros de Enderecos
#define VGA_BASE 0xc8000000

//Cores
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GRAY 0x8410
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define CYAN 0x07FF

//Array de pixels
uint16_t *ppix = (uint16_t *)(VGA_BASE);

//Variavel global que guarda a cor atual
uint16_t cor_atual = BLACK;
