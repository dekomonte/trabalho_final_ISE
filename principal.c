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
#define KEYBOARD_DATA 0xFF200100  
#define KEYBOARD_CONTROL 0xFF200104
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
#define CORAL 0xfb00 

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

// ================TELA INICIAL===================
void desenha_tela_inicial(uint16_t *ppix) {
    // Preenche o fundo com verde escuro
    muda_cor_fundo();

    // Desenha a cobrinha
    int snake_x[] = {80, 100, 100, 120, 120, 140, 140, 160, 160};
    int snake_y[] = {100, 100, 80, 80, 100, 100, 80, 80, 100};
    
    for (int i = 0; i < 9; i++) {
        for (int dy = 0; dy < 15; dy++) {
            for (int dx = 0; dx < 15; dx++) {
                uint16_t color = (i == 0) ? BLACK : CORAL;
                ppix[(snake_y[i]+dy) * 512 + (snake_x[i]+dx)] = color;
            }
        }
		// Olhos brancos na cabeça da cobrinha
    	if (i == 0) {
        	ppix[(snake_y[i]+5) * 512 + (snake_x[i]+4)] = WHITE;
        	ppix[(snake_y[i]+10) * 512 + (snake_x[i]+4)] = WHITE;
    	}
    }

    // Desenha uma maçã 
    static int apple_x = 0, apple_y = 0;
    static bool first_run = true;
    
    if (first_run) {
        apple_x = 60 + (rand() % 200); 
        apple_y = 60 + (rand() % 120);
        first_run = false;
    }
    
    for (int dy = 0; dy < 15; dy++) {
        for (int dx = 0; dx < 15; dx++) {
            ppix[(apple_y + dy) * 512 + (apple_x + dx)] = RED;
        }
    }
    
    for (int dy = -3; dy < 0; dy++) {
        for (int dx = 6; dx < 9; dx++) {
            ppix[(apple_y + dy) * 512 + (apple_x + dx)] = FOREST_GREEN;
        }
    }

    // Desenha "SNAKE" de amarelo
    const uint8_t letters[5][7] = {
        // S
        {0x1F,0x10,0x10,0x0F,0x01,0x01,0x1F},
        // N
        {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
        // A
        {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
        // K
        {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
        // E
        {0x1F,0x10,0x10,0x1F,0x10,0x10,0x1F}
    };

    for (int l = 0; l < 5; l++) {
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (letters[l][y] & (1 << (4-x))) {
                    for (int dy = 0; dy < 3; dy++) {
                        for (int dx = 0; dx < 3; dx++) {
                            ppix[(50+y*4+dy)*512 + (100+l*25+x*4+dx)] = YELLOW;
                        }
                    }
                }
            }
        }
    }
	
	const uint8_t basic_font[][7] = {
		// A
		{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
		// E
		{0x1F, 0x10, 0x10, 0x1F, 0x10, 0x10, 0x1F},
		// N
		{0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
		// P
		{0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
		// R
		{0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
		// T
		{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
		// Espaço
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};

	const uint8_t *get_basic_font_char(char c) {
		switch(c) {
			case 'A': return basic_font[0];
			case 'E': return basic_font[1];
			case 'N': return basic_font[2];
			case 'P': return basic_font[3];
			case 'R': return basic_font[4];
			case 'T': return basic_font[5];
			default:  return basic_font[6]; 
		}
	}

    // Desenha "APERTE ENTER" (branco)
    const char *msg = "APERTE ENTER";
    int start_x = 110;
    int start_y = 180;
    
    for (int i = 0; msg[i]; i++) {
        const uint8_t *font = get_basic_font_char(msg[i]);
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (font[y] & (1 << (4-x))) {
                    ppix[(start_y+y)*512 + (start_x+i*6+x)] = WHITE;
                }
            }
        }
    }
}
// ================TELA INICIAL===================

int main(){

	hex_display1 = HEX_DISPLAY1_BASE;
	hex_display2 = HEX_DISPLAY2_BASE;

	int pontos = 0;
	
	mostra_pontos(pontos);
	desenha_tela_inicial(ppix);
	
	
	//muda_cor_fundo();	
	//while(1){
		
	//}
	return 0;
}
