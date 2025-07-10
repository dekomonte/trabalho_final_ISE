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
#define HEX_DISPLAY1_BASE ((volatile uint32_t *)(LW_BRIDGE_BASE + DEVICES_HEX_DISPLAY_01))
#define HEX_DISPLAY2_BASE ((volatile uint32_t *)(LW_BRIDGE_BASE + DEVICES_HEX_DISPLAY_02))


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

volatile uint32_t *leds;
volatile unsigned int *switches, *buttons;
volatile uint32_t *hex_display1, *hex_display2;
	
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

//Funcao de delay
void delay(unsigned int tempo_delay){
    volatile unsigned int i, j;
    for (i = 0; i < tempo_delay; i++) {
        for (j = 0; j < 10000; j++) {
            // Só para gastar tempo
        }
    }
}

//Funcoes que escrevem no display de 7 segmentos
void write_hex_menu(const char display[]) {
    *(hex_display1) = display[3] << 24 |display[2]<< 16 | display[1] << 8 | display[0];
	*(hex_display2) = display[5] << 8 | display[4];
}

void write_hex(int value[]) {
    char display[] = {0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F};
    int i = 5;
    int pos = 0;

    while (i >= 0) {
        switch (value[i]) {
            case 0: display[pos] = 0x3F; break;
            case 1: display[pos] = 0x06; break;
            case 2: display[pos] = 0x5B; break;
            case 3: display[pos] = 0x4F; break;
            case 4: display[pos] = 0x66; break;
            case 5: display[pos] = 0x6D; break;
            case 6: display[pos] = 0x7D; break;
            case 7: display[pos] = 0x07; break;
            case 8: display[pos] = 0x7F; break;
            case 9: display[pos] = 0x6F; break;
            default: display[pos] = 0x3F; break;
        }
        i--; 
        pos++;
    }

    *(hex_display1) = display[3] << 24 | display[2] << 16 | display[1] << 8 | display[0];
    *(hex_display2) = display[5] << 8 | display[4];
}

//Mostra os pontos do jogador no display de 7 segmentos
void mostra_pontos(int pontos){
	
	int digitos[6] = {0}; //Limita o total de pontos para 6 digitos
	
	if (pontos < 0){
		pontos = 0;
	}
	
	if (pontos > 999999){
		pontos = 999999;
	}
	
	for(int i = 5; i >= 0; i--){
		digitos[i] = pontos % 10;
		pontos /= 10;
	}
	
	write_hex(digitos);
	
}

int main(){

	hex_display1 = HEX_DISPLAY1_BASE;
	hex_display2 = HEX_DISPLAY2_BASE;

	int pontos = 543647;
	
	mostra_pontos(pontos);
	
	muda_cor_fundo();	
	//while(1){
		
	//}
	return 0;
}
