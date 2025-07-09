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
#include <stdbool.h>
#include <fcntl.h> 
//#include <sys/mman.h>

//Macros de Enderecos
#define VGA_BASE 0xc8000000
#define LW_BRIDGE_BASE  0xFF200000
#define LW_BRIDGE_SPAN  0x00005000
#define DEVICES_LEDS     (0x00000000)
#define DEVICES_HEX_DISPLAY_01 (0x00000020)
#define DEVICES_HEX_DISPLAY_02 (0x00000030)
#define DEVICES_SWITCHES (0x00000040)
#define DEVICES_BUTTONS  (0x00000050)

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
#define FOREST_GREEN 0x0280
#define DARK_GREEN 0x03E0


//Array de pixels
uint16_t *ppix = (uint16_t *)(VGA_BASE);

void set_pixel(int linha, int coluna){
	if((linha >= 0 && linha < 240) && (coluna >= 0 && coluna <320)){
		ppix[linha * 512 + coluna] = DARK_GREEN;
	}
}

//Seta a cor de fundo do VGA 
void muda_cor_fundo(){
	for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            set_pixel(y, x);
        }
    }
}


int main(){

	muda_cor_fundo();	
	//while(1){
		
	//}
	return 0;
}

