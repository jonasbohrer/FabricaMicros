#include <MKL25Z4.h>

#define pullup (1 << 0) + (1 << 1) + (1 << 4) + (1 << 8)

// Variaveis de sinais de controle
int status_valvula, status_esteira, status_pistao, produto_alvo, qtd_ao_leite, qtd_meio_amargo;
// Constantes PI
int Kp = 2, Ki = 0.1;
// Variaveis de medidas de temperatura
float temperatura, temperatura_alvo, erro, erros_anteriores[5] = {0, 0, 0, 0, 0};
//Variavel do teclado
char dataTeclado[4][4] = { '1','2','3','A', '4','5','6','B', '7','8','9','C', '*','0','#','D' };
// Variaveis de estado
int estado; // 0 = Deslogado, 1 = aguardando senha, 2 = loggado, 3 = emergencia
int estado_producao; //0 = inativa, 1 = ativa
int salvaEstado;



////////////////////////// FUNCOES LCD //////////////////////////

void atraso_40us_lcd()
{
	TPM0_SC = 8;	//Prescaler 1
	TPM0_MOD = 840;	//Para 40us
	while ((TPM0_SC & (1 << 7)) == 0);
	TPM0_SC |= (1 << 7);
}

void atraso_1650us()
{
	TPM0_SC = 8;	//Prescaler 1
	TPM0_MOD = 34650;	//Para 1650us
	while ((TPM0_SC & (1 << 7)) == 0);
	TPM0_SC |= (1 << 7);
}

void escrita_comando(char comando)
{
	int aux;
	GPIOE_PCOR = (1 << 29); //RS = 0;
	aux = comando & 0x0F;//porta = comando;
	GPIOB_PDOR = aux << 8;//porta = comando;
	aux = comando & 0xF0;//porta = comando;
	GPIOE_PDOR = aux >> 2;//porta = comando;
	GPIOE_PSOR = (1 << 23); //EN = 1;	
	GPIOE_PCOR = (1 << 23); //EN = 0;
	atraso_40us_lcd();
}

void nova_linha(){
	escrita_comando(0xC0);
}

void escrita_valor(char comando)
{
	int aux;
	aux = comando & 0x0F;//porta = comando;
	GPIOB_PDOR = aux << 8;//porta = comando;
	aux = comando & 0xF0;//porta = comando;
	GPIOE_PDOR = aux >> 2;//porta = comando;
	GPIOE_PSOR = (1 << 29); //RS = 1;
	GPIOE_PSOR = (1 << 23); //EN = 1;	
	GPIOE_PCOR = (1 << 23); //EN = 0;
	atraso_40us_lcd();
}

void escrita_texto(char texto[]){
	int i;
	for (i = 0; texto[i] != 0x00; i++){
		escrita_valor(texto[i]);
		atraso_40us_lcd();
	}
}

void escrita_ao_leite(){
	escrita_texto("Ao leite: ");
	int dez = qtd_ao_leite/10;
	int uni = qtd_ao_leite%10;
	escrita_valor(dez);
	escrita_valor(uni);
}

void escrita_meio_amargo(){
	escrita_texto("Ao leite: ");
	int dez = qtd_meio_amargo/10;
	int uni = qtd_meio_amargo%10;
	escrita_valor(dez);
	escrita_valor(uni);
}

void limpar_display()
{
	escrita_comando(0x01); //'00000001'
	atraso_1650us();
}

void lcd_config()
{
	escrita_comando(0x38); //'00111000'
	escrita_comando(0x38); //'00111000'
	escrita_comando(0x0C); //'00001100'
	atraso_1650us();
	escrita_comando(0x06); //'00000110'
	escrita_comando(0x01); //'00000001'
	atraso_1650us();
}

void init_lcd(){

	SIM_SCGC5 |= 1 << 10; //Libera porta B
	SIM_SCGC5 |= 1 << 13; //Libera porta E

    // LCD
    PORTB_PCR8 |= 1 << 8;
    PORTB_PCR9 |= 1 << 8;
    PORTB_PCR10 |= 1 << 8;
    PORTB_PCR11 |= 1 << 8;
    GPIOB_PDDR |= 1 << 8; //Coloca como saída
    GPIOB_PDDR |= 1 << 9; //Coloca como saída
    GPIOB_PDDR |= 1 << 10; //Coloca como saída
    GPIOB_PDDR |= 1 << 11; //Coloca como saída

    PORTE_PCR2 |= 1 << 8;
    PORTE_PCR3 |= 1 << 8;
    PORTE_PCR4 |= 1 << 8;
    PORTE_PCR5 |= 1 << 8;
    PORTE_PCR23 |= 1 << 8;
    PORTE_PCR29 |= 1 << 8;
    GPIOE_PDDR |= 1 << 2; //Coloca como saída
    GPIOE_PDDR |= 1 << 3; //Coloca como saída
    GPIOE_PDDR |= 1 << 4; //Coloca como saída
    GPIOE_PDDR |= 1 << 5; //Coloca como saída
    GPIOE_PDDR |= 1 << 23; //Coloca como saída
    GPIOE_PDDR |= 1 << 29; //Coloca como saída

    lcd_config();
}

void escrever_status(float temperatura, int status){
    //Escreve temperatura e status no LCD

    char temperatura_str[12];
    //sprintf(temperatura_str, "%f", temperatura);

    limpar_display();
    escrita_texto(temperatura_str); //escreve char[] temperatura
    escrita_comando(0xc0);  //line break
    
    if (status == 1) {
        escrita_texto("Ligado");  //escreve Ligado
    } else if (status == 0) {
        escrita_texto("Desligado");  //escreve Desligado
    } else {
        escrita_texto("?");  //escreve Desligado
    }
}

//Funções teclado

char testa_debounce(char l, char c){
	char valor = 0;
	int contador = 0;
	
	switch (l){
		case 0: GPIOA_PCOR = 1 << 1;
		break;
		case 1: GPIOA_PCOR = 1 << 2;
		break;
		case 2: GPIOA_PCOR = 1 << 12;
		break;
		case 3: GPIOA_PCOR = 1 << 5;
		break;
		default: return 'E';
	}
	
	while (contador < 15){
		switch (c){
			case 0: if ((GPIOC_PDIR & (1 << 3)) == valor){
				contador++;
			} else {
				valor = (GPIOC_PDIR & (1 << 3));
				contador = 0;
			}
			break;
			case 1: if ((GPIOC_PDIR & (1 << 4)) == valor){
				contador++;
			} else {
				valor = (GPIOC_PDIR & (1 << 4));
				contador = 0;
			}
			break;
			case 2: if ((GPIOC_PDIR & (1 << 5)) == valor){
				contador++;
			} else {
				valor = (GPIOC_PDIR & (1 << 5));
				contador = 0;
			}
			break;
			case 3: if ((GPIOC_PDIR & (1 << 6)) == valor){
				contador++;
			} else {
				valor = (GPIOC_PDIR & (1 << 6));
				contador = 0;
			}
			break;
			default: return 'E';
		};
	}
	
	return valor;
}

char testa_teclado() {
	int l, c;
		
	for (l=0; l<4; l++) {
		for (c=0; c<4; c++) {
			if (testa_debounce(l,c) == 0) {
				atraso_40us_lcd();
				return dataTeclado[l][c];
			}
		}
	}
	return 'E';
}

////////////////////////// FUNCOES INICIALIZACAO DO SISTEMA //////////////////////////

void init_portas(){
    //Habilita a maioria das portas utilizadas
	
	//TECLADO
	PORTA_PCR1 |= 1 << 8; //Coloca como digital
	GPIOA_PDDR |= 1 << 1; //Coloca como saída
	PORTA_PCR2 |= 1 << 8;  //Coloca como digital
	GPIOA_PDDR |= 1 << 2; //Coloca como saída
	PORTA_PCR12 |= 1 << 8;//Coloca como digital
	GPIOA_PDDR |= 1 << 12; //Coloca como saída
	PORTA_PCR5 |= 1 << 8; //Coloca como digital
	GPIOA_PDDR |= 1 << 5; //Coloca como saída
	PORTC_PCR3 |= pullup;
	PORTC_PCR4 |= pullup;
	PORTC_PCR5 |= pullup;
	PORTC_PCR6 |= pullup;
	GPIOC_PDDR |= 0 << 3; //Coloca como entrada
	GPIOC_PDDR |= 0 << 4; //Coloca como entrada
	GPIOC_PDDR |= 0 << 5; //Coloca como entrada
	GPIOC_PDDR |= 0 << 6; //Coloca como entrada

    //Habilita fontes de clock nas Portas
	SIM_SCGC5 |= 1 << 10; //Libera porta B
	SIM_SCGC5 |= 1 << 13; //Libera porta E
    SIM_SCGC5 |= 1 << 11; //Libera porta C

    //Habilita fontes de clock dos tmps
	SIM_SOPT2 |= (1 << 24); //Fonte de clock dos tmps

    //Habilita fontes de clock no TMP e ADC
    SIM_SCGC6 |= (1 << 24); //Habilita TPM0
    SIM_SCGC6 |= (1 << 27); //Habilita ADC0
	
	//LEDS
	PORTB_PCR18 |= 1 << 8; //Coloca como digital
	PORTB_PCR19 |= 1 << 8; //Coloca como digital	

	GPIOB_PDDR |= 1 << 18; //Coloca como saída
	GPIOB_PDDR |= 1 << 19; //Coloca como saída
	GPIOB_PTOR = (1 << 18); //Desliga led
	GPIOB_PTOR = (1 << 19); //Desliga led

    //OUTPUT DAC
    PORTE_PCR30 |= 3 << 8; //Coloca como digital
    GPIOE_PDDR |= 1 << 30; //Coloca como saída 

    //SINAL RESERVATORIO CHEIO
	PORTE_PCR20 |= 1 << 8; //Coloca como digital
    GPIOE_PDDR |= 0 << 20; //Coloca como entrada 

    //SINAL RESERVATORIO VAZIO
    PORTE_PCR21 |= 1 << 8; //Coloca como digital 
	GPIOE_PDDR |= 0 << 21; //Coloca como entrada

    //SINAL VALVULA
    PORTC_PCR6 |= 1 << 8; //Coloca como digital 
	GPIOC_PDDR |= 1 << 6; //Coloca como saida

    //SINAL ESTEIRA
    PORTC_PCR5 |= 1 << 8; //Coloca como digital 
	GPIOC_PDDR |= 1 << 5; //Coloca como saida

    //SINAL PISTAO
    PORTC_PCR4 |= 1 << 8; //Coloca como digital 
	GPIOC_PDDR |= 1 << 4; //Coloca como saida
}

void init_pit() {
    //Habilita PIT

	SIM_SCGC6 |= (1 << 23); 
	PIT_MCR = 0;
	PIT_LDVAL0 = 1066666; // timer para 100ms   
	PIT_TCTRL0 = 3;
	NVIC_EnableIRQ(PIT_IRQn);
}

////////////////////////// CONTROLE ADC E DAC //////////////////////////

float ler_valor_adc(){
    //Captura sinais de temperatura do conversor analogico-digital
    ADC0_CFG1 = 0x0D; //00001101: 11 - 16-bit conversion; 01 - bus_clock/2^;
	ADC0_SC1A = 0; //Reinicia o estado

	while (ADC0_SC2 & (1 << 7)); //Conversion Active: 0 - Conversion not in progress. 1 - Conversion in progress.
    float valor_dac = ADC0_RA*3.3/4095;

	return valor_dac; //Resultado da conversão em float
}

void seta_valor_dac(float valor){
    //Modifica o valor de tensao baseado no parametro "valor" (de 0 a 2.5V)
    //Valor analógico para sistema aquecedor

    //Garantindo que o valor está nos limites
    if (valor < 0) {
        valor = 0;
    } else if (valor > 2.5) {
        valor = 2.5;
    }

    PORTE_PCR30 = (0<<8) + (1<<1) + (1<<0); //DAC + Out + Pull Up
    GPIOE_PDDR |= (1<<30);	//Coloca como saída
    DAC0_C0 = (1<<7);	//Liga o DAC

    short int data;
    data = (4095/3.3)*valor;
    DAC0_DAT0H = (data>>8);
    DAC0_DAT0L = data;
}

////////////////////////// CONTROLE SINAIS DA FABRICA //////////////////////////

void PIT_IRQHandler() {
    //Gerencia interrupts dos timers0
    //Controla a temperatura através de Controle Integral (valor = Kp*erro + Ki*sum(erros_anteriores))

    //Le a temperatura no adc
    temperatura = ler_valor_adc();

    //Escreve a temperatura e status do sistema (esteira em operação ou desligada) no lcd
    escrever_status(temperatura, status_esteira);

    //Atualiza os valores de erro do Controle Integral
    erro = temperatura_alvo*(4095/3.3) - temperatura;
    float valor_dac = Kp*erro + Ki*(erros_anteriores[0]+erros_anteriores[1]+erros_anteriores[2]+erros_anteriores[3]+erros_anteriores[4]);

    erros_anteriores[4] = erros_anteriores[3];
    erros_anteriores[3] = erros_anteriores[2];
    erros_anteriores[2] = erros_anteriores[1];
    erros_anteriores[1] = erros_anteriores[0];
    erros_anteriores[0] = erro;

    //Transmite o valor dac desejado (0 a 2.5V) para o DAC
    seta_valor_dac(valor_dac);

    //Reinicia PIT
	PIT_TFLG0 = 1;
}	

int ler_sinal_reservatorio_cheio(){
    //Retorna o sinal de reservatorio cheio
    return (GPIOE_PDIR & (1 << 20));
}

int ler_sinal_reservatorio_vazio(){
    //Retorna o sinal de reservatorio vazio
    return (GPIOE_PDIR & (1 << 21));
}

int seta_valvula(int cmd){
    //Altera o sinal digital para ligar válvula de movimentação de chocolate
    if (cmd == 1) {
        GPIOC_PSOR = (1 << 6); //EN = 1
    }
    else if (cmd == 0) {
        GPIOC_PCOR = (1 << 6); //EN = 0
    }
    status_valvula = cmd;
    return 0;
}

int seta_esteira(int cmd){
    //Altera o sinal digital para ligar esteira
        if (cmd == 1) {
        GPIOC_PSOR = (1 << 5); //EN = 1
    }
    else if (cmd == 0) {
        GPIOC_PCOR = (1 << 5); //EN = 0
    }
    status_esteira = cmd;
    return 0;
}

int seta_pistao_saida(int cmd){
    //Altera o sinal digital para ligar pistão de saída
    if (cmd == 1) {
        GPIOC_PSOR = (1 << 4); //EN = 1
    }
    else if (cmd == 0) {
        GPIOC_PCOR = (1 << 4); //EN = 0
    } else return -1;
    status_pistao = cmd;
    return 0;
}

void init_sinais(){
    // Inicializa os sinais de controle da fabrica

    // Variaveis de sinais de controle
    seta_valvula(1);
    seta_esteira(1);
    seta_pistao_saida(1);

    // Variaveis de controle do produto
    produto_alvo = 0; //0 - barras 100g; 1 - bombons 25g; Fixado em 0 na opção B

    // Variaveis de medidas de temperatura
    temperatura = 0;
    temperatura_alvo = 27.5; //entre 27 e 28 (ao leite) ou entre 29 e 30 (meio amargo) na opção B; Fixado em 27 na opção A
    
    // Constantes e variaveis do controle PI
    int Kp = 2, Ki = 0.1;
    erro = 0;
    erros_anteriores[0] = 0;
    erros_anteriores[1] = 0;
    erros_anteriores[2] = 0;
    erros_anteriores[3] = 0;
    erros_anteriores[4] = 0;
}

////////////////////////// PROGRAMA PRINCIPAL //////////////////////////
void iniciar_producao(){
	escrita_texto("Iniciando Produção");
	estado_producao = 1;
}

void parar_producao(){
	escrita_texto("Parando Produção");
	estado_producao = 0;
}

void parada_de_emergencia(){
	if (estado != 3){
		salvaEstado = estado;
		escrita_texto("PARADA DE EMERGÊNCIA");
		nova_linha();
		escrita_texto("SE AFASTE DA ESTEIRA");
		estado = 3;
	} else {
		estado = salvaEstado;
	}
}

void fim_de_producao(){
	escrita_ao_leite();
	nova_linha();
	escrita_meio_amargo();
	estado = 0;
	qtd_ao_leite = 0;
	qtd_meio_amargo = 0;
	parar_producao();
}

void troca_tipo_chocolate(){
}

int testa_usuario(char tentUsuario[5]){
	char usuario[5] = "00000";
	int teste;
	for (teste = 0; teste < 5; teste++){
		if (tentUsuario[teste] != usuario[teste]){
			return -1;
		}
	}
	return 1;
}

int testa_senha(char tentSenha[5]){
	char senha[5] = "12345";
	int teste;
	for (teste = 0; teste < 5; teste++){
		if (tentSenha[teste] != senha[teste]){
			return -1;
		}
	}
	return 1;
}

void aguardando_usuario(char primeiraLetra){
	escrita_valor(primeiraLetra);
	char tentLogin[5];
	tentLogin[0] = primeiraLetra;
	int letras = 1;
	char tecla = testa_teclado();
	while (letras < 5){
		if (tecla != 'E'){
			escrita_valor(tecla);
			tentLogin[letras] = tecla;
			letras++;
		}
		tecla = testa_teclado();
	}
	if (testa_usuario(tentLogin) == 1){
		limpar_display();
		estado = 1;
		escrita_texto("Digite sua senha");
	} else {
		escrita_texto("Usuario Inválido");
	}
}

void aguardando_senha(char primeiraLetra){
	escrita_valor(primeiraLetra);
	char tentLogin[5];
	tentLogin[0] = primeiraLetra;
	int letras = 1;
	char tecla = testa_teclado();
	while (letras < 5){
		if (tecla != 'E'){
			escrita_valor('*');
			tentLogin[letras] = tecla;
			letras++;
		}
		tecla = testa_teclado();
	}
	if (testa_senha(tentLogin) == 1){
		limpar_display();
		estado = 2;
		escrita_texto("Usuario Logado");
	} else {
		escrita_texto("Senha Inválida");
	}
}

void processa_acao(char tecla){
    if (tecla != 'E'){
			limpar_display();
			if (tecla == '*'){ //PARADA DE EMERGÊNCIA
				parada_de_emergencia();
			} else if (estado == 0){ //TENTAR COLOCAR USUARIO
				aguardando_usuario(tecla);
			} else if (estado == 1) { //TENTAR COLOCAR SENHA
				aguardando_senha(tecla);
			} else if (estado == 2) {
				if (tecla == '#'){ //DESLOGAR
					escrita_texto("Usuario Deslogado");
					estado = 0;
				} else if (tecla == '9'){ //FIM DE PRODUÇÃO
					fim_de_producao();
				} else if (tecla == '2'){ //TROCA ENTRE CHOCOLATE AO LEITE E AMARGO
					troca_tipo_chocolate();
				} else if (tecla == '1'){ //PARA E INICIA PRODUÇÃO
					if (estado_producao == 0){
						iniciar_producao();
					} else {
						parar_producao();
					}
				}
			}
		}
}

int main(){
    //Programa principal
		estado = 0;
		estado_producao = 0;
	
    //Inicializa as portas a serem utilizadas para os sinais principais + timers
    init_portas();

    //Inicializa o PIT e interrupcoes
    init_pit();

    //Inicializa as portas e configura lcd
    init_lcd();

    //Inicializa os sinais de controle da fabrica
    init_sinais();

    while(1){
        //Ler teclado
        char tecla = testa_teclado();
        processa_acao(tecla);
    }
}
