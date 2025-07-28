// Trabalho Final: Jogos Digitais na DE1-SoC
// Andressa Sena - 16/0112303
// Brayan Barbosa - 17/0030229

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h> // Para strlen

// Macros de Endereços
#define VGA_BASE 0xc8000000
#define LW_BRIDGE_BASE    0xFF200000
#define LW_BRIDGE_SPAN    0x00005000
#define DEVICES_LEDS      (0x00000000)
#define DEVICES_HEX_DISPLAY_01 (0x00000020)
#define DEVICES_HEX_DISPLAY_02 (0x00000030)
#define DEVICES_SWITCHES (0x00000040)
#define DEVICES_BUTTONS   (0x00000050)
#define KEYBOARD_DATA 0xFF200100
#define KEYBOARD_CONTROL 0xFF200104
#define HEX_DISPLAY1_BASE ((volatile uint32_t *)(LW_BRIDGE_BASE + DEVICES_HEX_DISPLAY_01))
#define HEX_DISPLAY2_BASE ((volatile uint32_t *)(LW_BRIDGE_BASE + DEVICES_HEX_DISPLAY_02))
#define BUTTONS_BASE ((volatile uint32_t *)(LW_BRIDGE_BASE + DEVICES_BUTTONS))
#define SWITCHES_BASE ((volatile uint32_t *)(LW_BRIDGE_BASE + DEVICES_SWITCHES))

// Cores
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

// Configurações do Jogo e Tela
#define SIZE_Y 240
#define SIZE_X 320
#define SIZE_SNAKE 15
#define PONTOS_INICIAIS_MACA 50
#define MACAS_POR_NIVEL 3
#define NIVEL_MAX 5
#define MAX_OBSTACULOS 100

// Configurações da Fonte
#define FONT_WIDTH 8    // Largura de cada caractere em pixels
#define FONT_HEIGHT 7   // Altura de cada caractere em pixels
#define CHAR_SPACING 2  // Espaço entre caracteres (ajustei para 2 para um bom visual)
                        // A largura total de um caractere + espaço será FONT_WIDTH + CHAR_SPACING (8 + 2 = 10)
#define FONT_ASCII_START 32 // Caractere inicial na tabela ASCII que a fonte suporta (' ' Espaço)
#define FONT_ASCII_END   90 // Caractere final na tabela ASCII que a fonte suporta ('Z')
#define FONT_CHAR_COUNT (FONT_ASCII_END - FONT_ASCII_START + 1) // Total de caracteres na fonte

#define VGA_MEM_PITCH 512

// Variáveis Globais de Hardware
volatile uint32_t *leds;
volatile uint32_t *switches, *buttons;
volatile uint32_t *hex_display1, *hex_display2;
uint16_t *ppix = (uint16_t *)(VGA_BASE);

// Variáveis Globais do Jogo
int apple_x = 0, apple_y = 0;
int valor_maca = PONTOS_INICIAIS_MACA;
int nivel = 1;
int dificuldade = 0; // Usado para ajustar a velocidade do jogo
uint32_t previous_button_state = 0; // Guardar estado do botão
int pontos = 0;
int flag_fim_de_jogo = 0;
int maca_get = 0;

// Estrutura da lista encadeada para a cobrinha
typedef struct node {
    int x, y;
    struct node* next;
} node;

// Obstáculos
typedef struct {
    int x, y;
} Obstaculo;

Obstaculo obstaculos[MAX_OBSTACULOS];
int qtd_obstaculos = 0;

// Função de delay (espera ocupada)
void delay(int time_ms) { // Tempo em milissegundos aproximados
    volatile int count = 0;
    // O valor 10000 aqui é uma calibração para seu hardware/simulador.
    // Pode ser que 1ms real precise de um valor maior ou menor.
    // 10000 foi o valor original. Usaremos 'time_ms' para escalá-lo.
    int i = 0;
    while (i < time_ms * 10000) { // Multiplica por 1000 para converter para unidades da sua calibração
        count++;
        i++;
    }
}

// Seta um pixel na tela VGA com a cor especificada
void set_pixel(int y, int x, uint16_t cor) {
    // Garante que o pixel esteja dentro dos limites da tela
    if (x >= 0 && x < SIZE_X && y >= 0 && y < SIZE_Y) {
        ppix[y * VGA_MEM_PITCH  + x] = cor; // Corrigido para usar SIZE_X como largura da tela
    }
}

// Preenche a tela inteira com uma cor (preto por padrão)
void muda_cor_fundo(uint16_t cor) {
    int y = 0;
    while (y < SIZE_Y) {
        int x = 0;
        while (x < SIZE_X) {
            set_pixel(y, x, cor);
            x++;
        }
        y++;
    }
}

// Libera toda a memória alocada para a lista encadeada da cobrinha
void liberarLista(node* lista) {
    while (lista) {
        node* temp = lista;
        lista = lista->next;
        free(temp);
    }
}

// Função para imprimir a lista
void imprimirLista(node* lista) {
    node* atual = lista;
    while (atual != NULL) {
        printf("Ponto: (%d, %d)\n", atual->x, atual->y);
        atual = atual->next;
    }
}


// Define os códigos para cada dígito no display de 7 segmentos
const char HEX_DIGITS[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F  // 9
};

// Mostra os pontos do jogador nos displays de 7 segmentos
void mostra_pontos(int pontos) {
    // Limita os pontos para 6 dígitos
    if (pontos < 0) {
        pontos = 0;
    }
    if (pontos > 999999) {
        pontos = 999999;
    }

    uint32_t display1_val = 0;
    uint32_t display2_val = 0;

    // Calcula e seta os dígitos para o HEX_DISPLAY1 (4 dígitos da direita)
    display1_val |= (HEX_DIGITS[pontos % 10] << 0);       // Unidade
    pontos /= 10;
    display1_val |= (HEX_DIGITS[pontos % 10] << 8);       // Dezena
    pontos /= 10;
    display1_val |= (HEX_DIGITS[pontos % 10] << 16);      // Centena
    pontos /= 10;
    display1_val |= (HEX_DIGITS[pontos % 10] << 24);      // Milhar
    pontos /= 10;

    // Seta os dígitos para o HEX_DISPLAY2 (2 dígitos da esquerda)
    display2_val |= (HEX_DIGITS[pontos % 10] << 0);       // Dezena de milhar
    pontos /= 10;
    display2_val |= (HEX_DIGITS[pontos % 10] << 8);       // Centena de milhar

    *hex_display1 = display1_val;
    *hex_display2 = display2_val;
}

// --- FONT_BITMAP ATUALIZADA ---
// Ordem dos caracteres: ' ' (espaço) até 'Z'
// Índices: (caractere ASCII) - FONT_ASCII_START
static const uint8_t FONT_BITMAP[FONT_CHAR_COUNT][FONT_HEIGHT] = {
    // Caractere: ' ' (Espaço - ASCII 32)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

    // Caractere: '!' (ASCII 33)
    {0x0C, 0x0C, 0x0C, 0x0C, 0x00, 0x0C, 0x00},

    // Caractere: '"' (ASCII 34)
    {0x00, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00},

    // Caractere: '#' (ASCII 35)
    {0x00, 24, 60, 24, 60, 24, 0x00}, // 0x18, 0x3C, 0x18, 0x3C, 0x18

    // Caractere: '$' (ASCII 36)
    {0x00, 0x18, 0x3C, 0x0C, 0x18, 0x30, 0x3C},

    // Caractere: '%' (ASCII 37) - NOVO
    {0x63, 0x63, 0x0C, 0x18, 0x30, 0x66, 0x66},

    // Caractere: '&' (ASCII 38)
    {0x00, 0x1C, 0x22, 0x1C, 0x24, 0x1C, 0x00},

    // Caractere: ''' (ASCII 39)
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00},

    // Caractere: '(' (ASCII 40)
    {0x00, 0x08, 0x10, 0x10, 0x10, 0x08, 0x00},

    // Caractere: ')' (ASCII 41)
    {0x00, 0x10, 0x08, 0x08, 0x08, 0x10, 0x00},

    // Caractere: '*' (ASCII 42)
    {0x00, 0x00, 0x2A, 0x3E, 0x2A, 0x00, 0x00},

    // Caractere: '+' (ASCII 43) - NOVO
    {0x00, 0x00, 0x10, 0x38, 0x10, 0x00, 0x00},

    // Caractere: ',' (ASCII 44)
    {0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},

    // Caractere: '-' (ASCII 45)
    {0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00},

    // Caractere: '.' (ASCII 46)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00},

    // Caractere: '/' (ASCII 47)
    {0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00},

    // Caractere: '0' (ASCII 48)
    {0x7C, 0x82, 0x82, 0x82, 0x82, 0x82, 0x7C},

    // Caractere: '1' (ASCII 49)
    {0x00, 0x20, 0x60, 0x20, 0x20, 0x20, 0x70},

    // Caractere: '2' (ASCII 50)
    {0x7C, 0x82, 0x02, 0x78, 0x80, 0x82, 0xFE},

    // Caractere: '3' (ASCII 51)
    {0x7E, 0x02, 0x02, 0x3C, 0x02, 0x02, 0x7E},

    // Caractere: '4' (ASCII 52)
    {0x10, 0x30, 0x50, 0x90, 0xFE, 0x10, 0x10},

    // Caractere: '5' (ASCII 53)
    {0xFE, 0x80, 0x80, 0xFC, 0x02, 0x02, 0xFC},

    // Caractere: '6' (ASCII 54)
    {0x7C, 0x82, 0x80, 0xFC, 0x82, 0x82, 0x7C},

    // Caractere: '7' (ASCII 55)
    {0xFE, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02},

    // Caractere: '8' (ASCII 56)
    {0x7C, 0x82, 0x82, 0x7C, 0x82, 0x82, 0x7C},

    // Caractere: '9' (ASCII 57)
    {0x7C, 0x82, 0x82, 0x7E, 0x02, 0x02, 0x7C},

    // Caractere: ':' (ASCII 58)
    {0x00, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x00},

    // Caractere: ';' (ASCII 59)
    {0x00, 0x00, 0x0C, 0x00, 0x0C, 0x18, 0x00},

    // Caractere: '<' (ASCII 60)
    {0x00, 0x08, 0x10, 0x20, 0x10, 0x08, 0x00},

    // Caractere: '=' (ASCII 61) - NOVO
    {0x00, 0x00, 0x38, 0x00, 0x38, 0x00, 0x00},

    // Caractere: '>' (ASCII 62)
    {0x00, 0x10, 0x08, 0x04, 0x08, 0x10, 0x00},

    // Caractere: '?' (ASCII 63)
    {0x3C, 0x42, 0x40, 0x1C, 0x08, 0x00, 0x08},

    // Caractere: '@' (ASCII 64)
    {0x3E, 0x41, 0x49, 0x45, 0x41, 0x00, 0x00}, // Simplificado, pode ser mais complexo

    // Caracteres A-Z (ASCII 65-90) - Suas definições originais
    {0x7E, 0x11, 0x11, 0x7E, 0x11, 0x11, 0x11}, // A (ASCII 65)
    {0x7C, 0x12, 0x12, 0x7C, 0x12, 0x12, 0x7C}, // B
    {0x3C, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3C}, // C
    {0x78, 0x14, 0x12, 0x12, 0x12, 0x14, 0x78}, // D
    {0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x7E}, // E
    {0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x40}, // F
    {0x3C, 0x42, 0x40, 0x4E, 0x42, 0x42, 0x3C}, // G
    {0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42}, // H
    {0x7E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x7E}, // I
    {0x1E, 0x04, 0x04, 0x04, 0x44, 0x44, 0x38}, // J
    {0x42, 0x44, 0x48, 0x70, 0x48, 0x44, 0x42}, // K
    {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7E}, // L
    {0x42, 0x66, 0x5A, 0x42, 0x42, 0x42, 0x42}, // M
    {0x42, 0x62, 0x52, 0x4A, 0x46, 0x42, 0x42}, // N
    {0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C}, // O
    {0x7C, 0x42, 0x42, 0x7C, 0x40, 0x40, 0x40}, // P
    {0x3C, 0x42, 0x42, 0x42, 0x4A, 0x44, 0x3A}, // Q
    {0x7C, 0x42, 0x42, 0x7C, 0x48, 0x44, 0x42}, // R
    {0x3C, 0x42, 0x40, 0x3C, 0x02, 0x42, 0x3C}, // S
    {0x7F, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}, // T
    {0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C}, // U
    {0x42, 0x42, 0x42, 0x42, 0x42, 0x24, 0x18}, // V
    {0x42, 0x42, 0x42, 0x5A, 0x5A, 0x66, 0x42}, // W
    {0x42, 0x24, 0x18, 0x18, 0x18, 0x24, 0x42}, // X
    {0x42, 0x24, 0x18, 0x18, 0x08, 0x08, 0x08}, // Y
    {0x7E, 0x02, 0x04, 0x18, 0x20, 0x40, 0x7E}  // Z (ASCII 90)
};

// Desenha um único caractere na tela VGA
void desenhar_letra(char letra, int x, int y, uint16_t cor) {
    // Converte para maiúsculo se for minúsculo (apenas para letras)
    // Se você adicionar letras minúsculas na sua fonte, remova esta linha.
    if (letra >= 'a' && letra <= 'z') {
        letra = letra - 'a' + 'A';
    }

    // Acessa o bitmap da fonte usando o índice correto
    if (letra >= FONT_ASCII_START && letra <= FONT_ASCII_END) {
        const uint8_t* bitmap = FONT_BITMAP[letra - FONT_ASCII_START];        
        int row = 0;
        while (row < FONT_HEIGHT) {
            int col = 0;
            while (col < FONT_WIDTH) {
                if (bitmap[row] & (0x80 >> col)) {
                    set_pixel(y + row, x + col, cor);
                }
                col++;
            }
            row++;
}

    }
    // Se o caractere não estiver dentro do intervalo FONT_ASCII_START a FONT_ASCII_END,
    // ele simplesmente não será desenhado, o que é o comportamento esperado para caracteres não suportados.
}

// Calcula a largura visual de uma string
int calcular_largura_texto(const char* texto) {
    int largura_total = 0;
    int i = 0;
    while(texto[i] != '\0') {
        // Se não for um espaço, adicione a largura da fonte
        if (texto[i] != ' ') {
            largura_total += FONT_WIDTH;
        }
        // Adiciona espaçamento entre caracteres (mas não após o último)
        if (texto[i+1] != '\0') {
            largura_total += CHAR_SPACING;
        }
        i++;
    }
    return largura_total;
}

// Desenha uma string na tela VGA
void desenhar_texto(const char* texto, int x, int y, uint16_t cor) {
    int pos_x = x;
    int i = 0;
    while(texto[i] != '\0') {
        if (texto[i] != ' ') {
            
            desenhar_letra(texto[i], pos_x, y, cor);

        }
        pos_x += FONT_WIDTH + CHAR_SPACING; // Move para a próxima posição do caractere
        i++;
    }
}

// Adiciona um novo nó (segmento) à cabeça da cobrinha
void addnode(node** lista, int x, int y) {
    node* novo = malloc(sizeof(node));
    if (novo == NULL) {
        flag_fim_de_jogo = -1;
        return; 
    }
    novo->x = x;
    novo->y = y;
    novo->next = *lista;
    *lista = novo;
}

// Limpa o rastro da cauda da cobrinha na tela VGA
void limpar_cauda(node* lista) {
    if (!lista || !lista->next) return; // Se a cobra tem 1 ou nenhum segmento, não faz nada

    node* atual = lista;

    // Percorre até o penúltimo nó
    while (atual->next && atual->next->next) {
        atual = atual->next;
    }

    node* cauda = atual->next;
    if (!cauda) return;
    int dy = 0, dx = 0;
    // Limpa visualmente a cauda da tela
    while(dy < SIZE_SNAKE) {
    dx = 0;
    while(dx < SIZE_SNAKE) {
        set_pixel(cauda->y + dy, cauda->x + dx, BLACK);
        dy++;}
    dx++;}
}


// Desenha a cobrinha na tela VGA
void desenha_snake(node* lista) {
    int i = 0;
    while (lista) {
        uint16_t color = (i%2 == 0) ? WHITE : CORAL; // Cabeça branca, corpo coral
        // Nota: Sua versão anterior alternava entre BLACK e CORAL.
        // A cabeça da cobra deve ser sempre visível e diferente.
        // Mudei para WHITE para a cabeça e CORAL para o corpo.

        int dy = 0;
        while (dy < SIZE_SNAKE) {
            int dx = 0;
            while (dx < SIZE_SNAKE) {
                set_pixel(lista->y + dy, lista->x + dx, color);
                dx++;
            }
            dy++;
        }
        lista = lista->next;
        i++;
    }
}

// Desenha a maçã na tela VGA
void desenha_apple() {
    int dy = 0;
    while (dy < SIZE_SNAKE) {
        int dx = 0;
        while (dx < SIZE_SNAKE) {
            set_pixel(apple_y + dy, apple_x + dx, RED);
            dx++;
        }
        dy++;
    }
}

void apaga_apple() {
    int dy = 0;
    while (dy < SIZE_SNAKE) {
        int dx = 0;
        while (dx < SIZE_SNAKE) {
            set_pixel(apple_y + dy, apple_x + dx, BLACK);
            dx++;
        }
        dy++;
    }
}

// Verifica se a cabeça da cobra colidiu com seu próprio corpo
bool colidiu_com_corpo(node* lista) {
    if (lista == NULL) return false; // Lista vazia, sem colisão
    int x = lista->x;
    int y = lista->y;
    node* temp = lista->next; // Começa a verificar do segundo segmento
    while (temp) {
        // Colisão é verificada pelo canto superior esquerdo do bloco
        if (temp->x == x && temp->y == y) return true;
        temp = temp->next;
    }
    return false;
}

// Gera uma nova posição válida para a maçã (não sobrepondo obstáculos)
void gerar_maca_valida() {
    apple_x = 90, apple_y = 90;
    int i;
    while (1) {
        // Gera coordenadas alinhadas à grade da cobra (multiplas de SIZE_SNAKE)
        apple_x = (rand() % (SIZE_X / SIZE_SNAKE)) * SIZE_SNAKE;
        apple_y = (rand() % (SIZE_Y / SIZE_SNAKE)) * SIZE_SNAKE;

        bool conflito = false;
        // Verifica conflito com obstáculos
        i = 0;
        while(i < qtd_obstaculos){
            if (obstaculos[i].x == apple_x && obstaculos[i].y == apple_y) {
                conflito = true;
                break;
            }
            i++;
        }
        // Se a maçã não estiver em conflito, a posição é válida
        if (!conflito) break;
    }
}

// Move a cobra, verifica colisões e come maçãs
void mover_snake(node** lista, int dx, int dy, int* pontos) {
    node* cabeca = *lista;
    int novo_x = cabeca->x + dx;
    int novo_y = cabeca->y + dy;

    // Verifica colisão com as bordas
    if (novo_x < 0 || novo_x >= SIZE_X || novo_y < 0 || novo_y >= SIZE_Y) {
        flag_fim_de_jogo = -1; // Indica Game Over
        return;
    }

    // Verifica colisão com obstáculo
    int i=0;
    while(i < qtd_obstaculos){
        if (obstaculos[i].x == novo_x && obstaculos[i].y == novo_y) {
            flag_fim_de_jogo = -1; // Indica Game Over
            return;
        }
        i++;
    }
    // Cria a nova cabeça
    node* novo = malloc(sizeof(node));
    if (novo == NULL) {
        flag_fim_de_jogo = -1; // Erro fatal de memória, considerar Game Over
        return;
    }
    novo->x = novo_x;
    novo->y = novo_y;
    novo->next = *lista;
    *lista = novo; // Atualiza a cabeça da lista

    int my = 0, mx, maca_eat = 0;
    while (my < SIZE_SNAKE) {
        mx = 0;
        while (mx < SIZE_SNAKE) {
            if (novo_x == (apple_x+mx) && novo_y == (apple_y+my))
            {
                maca_eat = 1;
                maca_get++;
                break;
            }
            if (novo_x == (apple_x+mx) && (novo_y+SIZE_SNAKE-1) == (apple_y+my))
            {
                maca_eat = 1;
                maca_get++;
                break;
            }
            if ((novo_x+(SIZE_SNAKE-1)) == (apple_x+mx) && novo_y == (apple_y+my))
            {
                maca_eat = 1;
                maca_get++;
                break;
            }
            if ((novo_x+(SIZE_SNAKE-1)) == (apple_x+mx) && (novo_y+SIZE_SNAKE-1) == (apple_y+my))
            {
                maca_eat = 1;
                maca_get++;
                break;
            }
            mx++;
        }
        my++;
    }
    
    // Se a cobra não comeu a maçã, remove o último segmento da cauda
    if (maca_eat) {
        *pontos += valor_maca;
        apaga_apple();
        gerar_maca_valida(); // Gera uma nova maçã
    } else {
        // Encontra o penúltimo nó para remover o último
        node* current = *lista;
        while (current->next != NULL && current->next->next != NULL) {
            current = current->next;
        }
        
        // Se houver um último nó para remover (a cobra tem pelo menos 2 segmentos)
        if (current->next != NULL) {
            node* tail_to_remove = current->next;
            // A limpeza visual da cauda já é feita por `limpar_cauda` antes desta função.
            free(tail_to_remove);
            current->next = NULL;
        }
    }
}

// Gera obstáculos aleatórios com base no nível atual
void gerar_obstaculos() {
    qtd_obstaculos = 0; // Reseta a contagem de obstáculos para o novo nível
    int area_total_blocos = (SIZE_X / SIZE_SNAKE) * (SIZE_Y / SIZE_SNAKE);
    int porcentagem = (nivel - 1) * 1; // Aumenta a porcentagem de obstáculos com o nível
    int total_obstaculos_desejado = (area_total_blocos * porcentagem) / 100;

    // Limita o número de obstáculos ao máximo permitido
    if (total_obstaculos_desejado > MAX_OBSTACULOS) {
        total_obstaculos_desejado = MAX_OBSTACULOS;
    }

    while (qtd_obstaculos < total_obstaculos_desejado) {
        int x = (rand() % (SIZE_X / SIZE_SNAKE)) * SIZE_SNAKE;
        int y = (rand() % (SIZE_Y / SIZE_SNAKE)) * SIZE_SNAKE;

        // Evita gerar obstáculos na área central (onde a cobra começa)
        if ((x >= 140 && x <= 180 && y >= 100 && y <= 140)) continue;

        bool conflito = false;
        // Verifica conflito com a maçã
        if (x == apple_x && y == apple_y) {
            conflito = true;
        }

        // Verifica conflito com outros obstáculos já gerados
        int i = 0;
        while(i < qtd_obstaculos) {
            if (obstaculos[i].x == x && obstaculos[i].y == y) {
                conflito = true;
                break;
            }
        i++;
        }
        
        // Adiciona o obstáculo se não houver conflito
        if (!conflito) {
            obstaculos[qtd_obstaculos].x = x;
            obstaculos[qtd_obstaculos].y = y;
            qtd_obstaculos++;
        }
    }
}

// Desenha todos os obstáculos na tela VGA
void desenha_obstaculos() {
    int i = 0, dy = 0, dx = 0;
    while(i < qtd_obstaculos){
        dy = 0;
        while(dy < SIZE_SNAKE){
            dx = 0;
            while(dx < SIZE_SNAKE){
                set_pixel(obstaculos[i].y + dy, obstaculos[i].x + dx, GRAY);
                dx++;
            }
            dy++;
        }
        i++;
    }
}

// Exibe a tela de subida de nível
void subir_nivel() {
    muda_cor_fundo(BLACK);
    desenhar_texto("NIVEL", (SIZE_X - calcular_largura_texto("NIVEL")) / 2, 100, GREEN);
    char texto_nivel[10];
    sprintf(texto_nivel, "%d", nivel);
    desenhar_texto(texto_nivel, (SIZE_X - calcular_largura_texto(texto_nivel)) / 2, 130, GREEN);
    delay(1000); // Exibe por um tempo para o jogador ver
}

// Exibe a tela de "Fim de Jogo"
void fim_de_jogo(int pontos) {
    muda_cor_fundo(BLACK);
    desenhar_texto("FIM DE JOGO", (SIZE_X - calcular_largura_texto("FIM DE JOGO")) / 2, 80, RED);
    desenhar_texto("PONTOS:", (SIZE_X - calcular_largura_texto("PONTOS:")) / 2, 110, WHITE);
    char texto_pontos[10];
    sprintf(texto_pontos, "%d", pontos);
    desenhar_texto(texto_pontos, (SIZE_X - calcular_largura_texto(texto_pontos)) / 2, 130, WHITE);
    desenhar_texto("KEY1 MENU", (SIZE_X - calcular_largura_texto("KEY1 MENU")) / 2, 160, YELLOW);
    desenhar_texto("KEY0 SAIR", (SIZE_X - calcular_largura_texto("KEY0 SAIR")) / 2, 180, CYAN);

    while (1) {
        if (buttons == NULL) { 
            buttons = BUTTONS_BASE; 
            delay(10);
            continue;
        }
        uint32_t current_button_state = *buttons; // Lê o estado ATUAL dos botões
        if ((current_button_state & 0x01) && !(previous_button_state & 0x01)) {
            delay(50); // Debounce
            previous_button_state = current_button_state; // Atualiza o estado ANTERIOR
            muda_cor_fundo(BLACK);
            exit(0);
        }
        
        // KEY1 para Iniciar
        if ((current_button_state & 0x02) && !(previous_button_state & 0x02)) {
            delay(50); // Debounce
            previous_button_state = current_button_state; // Atualiza o estado ANTERIOR
            break; // Sai do menu para iniciar o jogo
        }
        previous_button_state = current_button_state;
        delay(10); // Pequeno delay para debounce
    }
}

// Exibe o menu inicial do jogo
void menu_inicial() {
    muda_cor_fundo(BLACK); // Limpa a tela UMA VEZ
    
    // Desenha as mensagens estáticas UMA VEZ
    desenhar_texto("SNAKE GAME", (SIZE_X - calcular_largura_texto("SNAKE GAME")) / 2, 50, YELLOW);    
    desenhar_texto("KEY1   START", (SIZE_X - calcular_largura_texto("KEY1  START")) / 2, 100, GREEN);
    desenhar_texto("KEY0    END", (SIZE_X - calcular_largura_texto("KEY0   END")) / 2, 120, RED);
    
    while (1) {
        if (buttons == NULL) { 
            buttons = BUTTONS_BASE; 
            delay(10);
            continue;
        }

        uint32_t current_button_state = *buttons; // Lê o estado ATUAL dos botões

        // --- Detecção de borda de subida para KEY0 e KEY1 ---

        // KEY0 para Sair
        if ((current_button_state & 0x01) && !(previous_button_state & 0x01)) {
            delay(50); // Debounce
            previous_button_state = current_button_state; // Atualiza o estado ANTERIOR
            exit(0);
        }
        
        // KEY1 para Iniciar
        if ((current_button_state & 0x02) && !(previous_button_state & 0x02)) {
            delay(50); // Debounce
            previous_button_state = current_button_state; // Atualiza o estado ANTERIOR
            break; // Sai do menu para iniciar o jogo
        }
        
        // --- Atualiza o estado anterior dos botões para a próxima iteração ---
        previous_button_state = current_button_state; 
        
        delay(10); // Pequeno delay no loop
    }
}

// Permite ao jogador escolher a dificuldade usando os switches
// Funções auxiliares de desenho de texto já definidas no seu código
// desenhar_texto, calcular_largura_texto, muda_cor_fundo

int ler_dificuldade() {
    int aux = 0;
    int dificuldade_escolhida = 0;
    
    while (1) {
        // Apenas limpa a tela UMA VEZ ao entrar nesta função
        if (aux == 0) {
            muda_cor_fundo(BLACK); 

            // Desenha as mensagens estáticas UMA VEZ
            desenhar_texto("ESCOLHA DIFICULDADE", (SIZE_X - calcular_largura_texto("ESCOLHA DIFICULDADE")) / 2, 80, WHITE);
            desenhar_texto("SWITCH = +10% VEL", (SIZE_X - calcular_largura_texto("SWITCH = +10% VEL")) / 2, 120, WHITE);
            desenhar_texto("KEY1 CONTINUAR", (SIZE_X - calcular_largura_texto("KEY1 CONTINUAR")) / 2, 160, CYAN);
            aux = 1;
        }
        // Verifica a leitura dos botões (KEY1 para continuar)
        if (buttons == NULL) {
            buttons = BUTTONS_BASE;
            delay(10);
            continue;
        }
        if (switches == NULL) {
            switches = SWITCHES_BASE;
            delay(10);
            continue;
        }

        uint32_t current_button_state = *buttons; // Lê o estado ATUAL dos botões
        int sw_estado_atual = *switches; // Lê o estado ATUAL dos switches

        // --- Lógica para ler dificuldade dos switches e exibir ---
        int soma_switches = 0, i = 0;
        while(i < 4){
            if (sw_estado_atual & (1 << i)) {
                soma_switches++;
            }
            i++;
        }
        
        if (soma_switches != dificuldade_escolhida) aux = 0;        
        dificuldade_escolhida = soma_switches;

        // Limpa a área do texto de dificuldade anterior (opcional, se piscar muito)
        // Você pode desenhar um retângulo preto sobre a área antiga, ou
        // apenas confiar que o novo texto vai sobrescrever o antigo.
        // Para uma transição suave, se o número de dígitos mudar muito,
        // pode ser melhor redesenhar um fundo preto na área específica do texto.
        // Por agora, vamos apenas redesenhar o texto.
        
        char texto_dificuldade[20];
        sprintf(texto_dificuldade, "DIFICULDADE: %d", dificuldade_escolhida);
        // printf("Dif: %d\n", dificuldade_escolhida);
        // printf("%s\n", texto_dificuldade);
        desenhar_texto(texto_dificuldade, (SIZE_X - calcular_largura_texto(texto_dificuldade)) / 2, 140, YELLOW);
        // --- Fim da lógica de exibição de dificuldade ---

        // --- Detecção de borda de subida para KEY1 ---
        // Verifica se KEY1 (0x02) foi pressionado AGORA e NÃO ESTAVA pressionado ANTES
        if ((current_button_state & 0x02) && !(previous_button_state & 0x02)) {
            // KEY1 foi recém-pressionado
            delay(50); // Pequeno debounce para garantir um único registro
            previous_button_state = current_button_state; // Atualiza o estado ANTERIOR
            break; // Sai do loop de escolha de dificuldade
        }
        
        // --- Atualiza o estado anterior dos botões para a próxima iteração ---
        previous_button_state = current_button_state; 
        
        delay(10); // Pequeno delay para evitar loop muito rápido
    }
    
    return dificuldade_escolhida; 
}

int main() {
    // Inicializa os ponteiros para os periféricos de hardware
    hex_display1 = HEX_DISPLAY1_BASE;
    hex_display2 = HEX_DISPLAY2_BASE;
    buttons = BUTTONS_BASE;
    switches = SWITCHES_BASE;
    int dx = 0, dy = SIZE_SNAKE;

    // Loop principal do jogo, que permite jogar múltiplas vezes
    while (1) {
        // Variáveis de estado do jogo, reinicializadas a cada nova rodada
        pontos = 0;
        // int direcao = 2;    // 1:Cima, 2:Baixo, 3:Direita, 4:Esquerda (inicialmente para a direita)
        node* lista = NULL; // A lista da cobrinha
        // Inicia com o menu principal
        mostra_pontos(pontos);
        menu_inicial();
        
        // Permite ao jogador definir a dificuldade
        dificuldade = ler_dificuldade();

        // Limpa a tela para o início do jogo
        muda_cor_fundo(BLACK);
        nivel = 1;
        valor_maca = PONTOS_INICIAIS_MACA;

        // Inicializa a cobra, maçã e obstáculos
        addnode(&lista, 160, 120); // Posição inicial da cabeça da cobra
        addnode(&lista, 160, 105);
        gerar_maca_valida();
        gerar_obstaculos();

        // Loop principal do jogo (enquanto o jogador não perde)
        while (1) {
            mostra_pontos(pontos);
            limpar_cauda(lista); // Limpa o rastro da cauda antiga

            // Leitura e atualização da direção da cobra
            uint32_t btns = *buttons;
            
            // Para joystick/D-pad:
            // if (btns & 0x01) direcao = 4; // BTN0 = Esquerda
            // else if (btns & 0x02) direcao = 3; // BTN1 = Direita
            // else if (btns & 0x04) direcao = 1; // BTN2 = Cima
            // else if (btns & 0x08) direcao = 2; // BTN3 = Baixo
            

            // // Move a cobra na direção atual
            
            // // CORREÇÃO: Reajustando a chamada de mover_snake para usar as variáveis dx, dy
            // int current_dx = 0;
            // int current_dy = 0;
            // switch (direcao) {
            //     case 1: current_dy = -SIZE_SNAKE; break; // Cima
            //     case 2: current_dy = SIZE_SNAKE; break;  // Baixo
            //     case 3: current_dx = SIZE_SNAKE; break;  // Direita
            //     case 4: current_dx = -SIZE_SNAKE; break; // Esquerda
            // }

            if ((btns & 0x1) && dx == 0) {           // ESQUERDA (KEY0)
                dx = -SIZE_SNAKE;
                dy = 0;
            } else if ((btns & 0x2) && dx == 0) {    // DIREITA (KEY1)
                dx = SIZE_SNAKE;
                dy = 0;
            } else if ((btns & 0x4) && dy == 0) {    // BAIXO (KEY2)
                dx = 0;
                dy = SIZE_SNAKE; 
            } else if ((btns & 0x8) && dy == 0) {    // CIMA (KEY3)
                dx = 0;
                dy = -SIZE_SNAKE; 
            }

            mover_snake(&lista, dx, dy, &pontos);

            // Verifica se o jogo acabou (colisão com borda/obstáculo ou erro de memória)
            if (flag_fim_de_jogo == -1 || colidiu_com_corpo(lista)) {
                liberarLista(lista); // Libera memória da cobra
                fim_de_jogo(pontos); // Exibe tela de fim de jogo
                flag_fim_de_jogo = 0;
                break;               // Sai do loop do jogo atual para voltar ao menu
            }

            // Atualiza pontos e verifica subida de nível
            pontos += nivel; // Pontos aumentam com o tempo e nível
            
            // Lógica de subida de nível
            // Condição: (pontos acumulados / valor da maçã) >= (nível * maçãs_necessárias_por_nível)
            if (maca_get >= MACAS_POR_NIVEL && nivel < NIVEL_MAX) {
                nivel++;
                valor_maca = (int)(valor_maca * 1.2 + 0.5); // Aumenta o valor da maçã para o próximo nível
                liberarLista(lista); // Libera a cobra antiga
                lista = NULL; // Garante que o ponteiro é nulo após liberar
                addnode(&lista, 160, 120); // Recria a cobra no centro
                addnode(&lista, 160, 105);
                maca_get = 0;
                muda_cor_fundo(BLACK); // Limpa a tela para o novo nível
                gerar_maca_valida(); // Gera nova maçã
                gerar_obstaculos(); // Gera novos obstáculos para o novo nível
                subir_nivel(); // Exibe a tela de subida de nível
                muda_cor_fundo(BLACK);
            }

            // Desenha os elementos na tela
            desenha_snake(lista);
            desenha_apple();
            desenha_obstaculos();
            mostra_pontos(pontos); // Atualiza os displays HEX

            // Controla a velocidade do jogo com base na dificuldade
            int game_speed_delay = 100; // Delay base (maior valor = mais lento)
            game_speed_delay -= dificuldade * 10; // Aumenta velocidade com a dificuldade
            if (game_speed_delay < 10) game_speed_delay = 10; // Garante um delay mínimo
            delay(game_speed_delay);
        }
    }
    return 0;
}
