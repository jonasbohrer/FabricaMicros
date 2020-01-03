#include <MKL25Z4.h>

int amarelo = 0;
int verde = 0;
int vermelho = 0;

// Variaveis de sinais de controle
int status_valvula, status_esteira, status_pistao, produto_alvo;
// Constantes PI
int Kp = 2, Ki = 0.1;
// Variaveis de medidas de temperatura
float temperatura, temperatura_alvo, erro, erros_anteriores[5] = {0, 0, 0, 0, 0};

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

int init_lcd(){

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

int escrever_status(float temperatura, int status){
    //Escreve temperatura e status no LCD

    char temperatura_str[12];
    sprintf(temperatura_str, "%f", temperatura);

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

////////////////////////// FUNCOES INICIALIZACAO DO SISTEMA //////////////////////////

int init_portas(){
    //Habilita a maioria das portas utilizadas

    //Habilita fontes de clock nas Portas
	SIM_SCGC5 |= 1 << 10; //Libera porta B
	SIM_SCGC5 |= 1 << 13; //Libera porta E
    SIM_SCGC5 |= 1 << 11; //Libera porta C

    //Habilita fontes de clock dos tmps
	SIM_SOPT2 |= (1 << 24); //Fonte de clock dos tmps

    //Habilita fontes de clock no TMP e ADC
    SIM_SCGC6 |= (1 << 24); //Habilita TPM0
    SIM_SCGC6 |= (1 << 27) //Habilita ADC0
	
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

int init_pit() {
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
    ADC0_CFG1 = 0x0D //00001101: 11 - 16-bit conversion; 01 - bus_clock/2^;
	ADC0_SC1A = 0; //Reinicia o estado

	while (ADC0_SC2 & (1 << 7)); //Conversion Active: 0 - Conversion not in progress. 1 - Conversion in progress.
    float valor_dac = ADC0_RA*3.3/4095;

	return valor_dac; //Resultado da conversão em float
}

int seta_valor_dac(float valor){
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

    Short int data;
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

int init_sinais(){
    // Inicializa os sinais de controle da fabrica

    // Variaveis de sinais de controle
    status_valvula = 1;
    status_esteira = 1;
    status_pistao = 1;
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

int ler_teclado(){
    return 0;
}

int processa_acao(char tecla[]){
    return 0;
}

int main(){
    //Programa principal

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
        char tecla = ler_teclado();
        processa_acao(tecla);
    }
}