#include <MKL25Z4.h>

int amarelo = 0;
int verde = 0;
int vermelho = 0;
int status_valvula, status_esteira, status_pistao, produto_alvo;
float temperatura, temperatura_alvo;
char status[12];

//Funções LCD

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

//Funções sistema

void pwm(){
	TPM0_SC = 11;	//Prescaler 8
	TPM0_MOD = 48000;	//Para 18ms (com F*8/3)
	//while ((TPM0_SC & (1 << 7)) == 0);
	//TPM0_SC |= (1 << 7);
	
	TPM0_C3SC = (2 << 4) + (2 << 2); //Modo high true
	TPM0_C3V = 36000;
}

void pit() {
	SIM_SCGC6 |= (1 << 23); //Habilita PIT
	PIT_MCR = 0;
	PIT_LDVAL0 = 224000000; // timer para 21s   
	//PIT_TCTRL0 = (1 << 30); //TIE enable Timer 1 interrupts      
	//PIT_TCTRL0 |= (1 << 31); //TEN start Timer 1
	PIT_TCTRL0 = 3;
	NVIC_EnableIRQ(PIT_IRQn);
	GPIOB_PCOR = (1 << 19); //Liga Verde
	verde = 1;
	PIT_LDVAL0 = 32000000; // timer para 3s  
}

void PIT_IRQHandler() { // Gerencia interrupts dos 3 timers
	if (verde == 1) //Verde acabou, amarelo come�ou e carrega vermelho
	{
		GPIOB_PCOR = (1 << 18); //Liga o Vermelho
		GPIOB_PCOR = (1 << 19); //Liga o Verde
		PIT_LDVAL0 = 256000000; // timer para 24s
		verde = 0;
		amarelo = 1;
	}
	else if (amarelo == 1) //Amarelo acabou, vermelho come�ou e carrega verde
	{
		GPIOB_PSOR = (1 << 19); //Desliiga o Verde
		GPIOB_PCOR = (1 << 18); //Liga o Vermelho
		PIT_LDVAL0 = 224000000; // timer para 21s
		amarelo = 0;
		vermelho = 1;
	}
	else if (vermelho == 1) //Vermelho acabou, Verde come�ou e carrega amarelo
	{
		GPIOB_PSOR = (1 << 18); //Desliga o Vermelho
		GPIOB_PCOR = (1 << 19); //Liga o Verde
		PIT_LDVAL0 = 32000000; // timer para 3s
		vermelho = 0;
		verde = 1;
	}
	PIT_TFLG0 = 1;
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

int init_portas(){
    // Habilita fontes de clock nas Portas
	SIM_SCGC5 |= 1 << 10; //Libera porta B
	SIM_SCGC5 |= 1 << 13; //Libera porta E
    SIM_SCGC5 |= 1 << 11; //Libera porta C

    // Habilita fontes de clock dos tmps
	SIM_SOPT2 |= (1 << 24); //Fonte de clock dos tmps

    // Habilita fontes de clock no TMP e ADC
    SIM_SCGC6 |= (1 << 24); //Habilita TPM0
    SIM_SCGC6 |= (1 << 27) //Habilita ADC
	
	// LEDS
	PORTB_PCR18 |= 1 << 8; //Coloca como digital
	PORTB_PCR19 |= 1 << 8; //Coloca como digital	

	GPIOB_PDDR |= 1 << 18; //Coloca como saída
	GPIOB_PDDR |= 1 << 19; //Coloca como saída
	GPIOB_PTOR = (1 << 18); //Desliga led
	GPIOB_PTOR = (1 << 19); //Desliga led

    // OUTPUT DAC
    PORTE_PCR30 |= 3 << 8; //Coloca como digital
    GPIOE_PDDR |= 1 << 30; //Coloca como saída 

    // SINAL RESERVATORIO CHEIO
	PORTE_PCR20 |= 1 << 8; //Coloca como digital
    GPIOE_PDDR |= 0 << 20; //Coloca como entrada 

    // SINAL RESERVATORIO VAZIO
    PORTE_PCR21 |= 1 << 8; //Coloca como digital 
	GPIOE_PDDR |= 0 << 21; //Coloca como entrada

    // SINAL VALVULA
    PORTC_PCR6 |= 1 << 8; //Coloca como digital 
	GPIOC_PDDR |= 1 << 6; //Coloca como saida

    // SINAL ESTEIRA
    PORTC_PCR5 |= 1 << 8; //Coloca como digital 
	GPIOC_PDDR |= 1 << 5; //Coloca como saida

    // SINAL PISTAO
    PORTC_PCR4 |= 1 << 8; //Coloca como digital 
	GPIOC_PDDR |= 1 << 4; //Coloca como saida
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

float ler_temperatura(){
    //Captura sinais de temperatura do conversor analogico-digital
    ADC0_CFG1 = 0x0D //00001101: 11 - 16-bit conversion; 01 - bus_clock/2^;
	ADC0_SC1A = 0; //Reinicia o estado

	while (ADC0_SC2 & (1 << 7)); //Conversion Active: 0 - Conversion not in progress. 1 - Conversion in progress.
	return (ADC0_RA + 0.0); //Resultado da conversão em float
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

int gera_tensao_dac(float valor){
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

int main(){
    //Programa principal

    //Inicializa as portas a serem utilizadas para os sinais principais + timers
    init_portas();
    //Inicializa as portas e configura lcd
    init_lcd();

    while(1){
        //Ler teclado
        temperatura_alvo = 27.5; //entre 27 e 28 (ao leite) ou entre 29 e 30 (meio amargo) na opção B; Fixado em 27 na opção A
        produto_alvo = 0; //0 - barras 100g; 1 - bombons 25g; Fixado em 0 na opção B
        
        //Captura a temperatura e status do sistema (esteira em operação ou desligada)
        temperatura = ler_temperatura();

        //Escreve a temperatura e status do sistema (esteira em operação ou desligada) no lcd
        escrever_status(temperatura, status_esteira);
                
        //Calcula o valor PID
        pid();

        //Envia o valor para a saída DAC
        gera_tensao_dac(1.0);

    }



}