#include <MKL25Z4.h>

int amarelo = 0;
int verde = 0;
int vermelho = 0;
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

int init_portas(){
    // LED
	SIM_SCGC5 |= 1 << 10; //Libera porta B
	// Portas LCD
	SIM_SCGC5 |= 1 << 13; //Libera porta E
	
	//LED
	PORTB_PCR18 |= 1 << 8; //Coloca como digital
	PORTB_PCR19 |= 1 << 8; //Coloca como digital	
	PORTE_PCR20 |= 1 << 8; //Coloca como digital 
	PORTE_PCR30 |= 3 << 8; //Coloca como digital 
	GPIOE_PDDR |= 1 << 20; //Coloca como saída 
	GPIOE_PDDR |= 1 << 30; //Coloca como saída 
	GPIOB_PDDR |= 1 << 18; //Coloca como saída
	GPIOB_PDDR |= 1 << 19; //Coloca como saída
	GPIOB_PTOR = (1 << 18); //Desliga led
	GPIOB_PTOR = (1 << 19); //Desliga led

    //LCD
    PORTE_PCR2 |= 1 << 8;
    PORTE_PCR3 |= 1 << 8;
    PORTE_PCR4 |= 1 << 8;
    PORTE_PCR5 |= 1 << 8;
    PORTE_PCR23 |= 1 << 8;
    PORTE_PCR29 |= 1 << 8;
    PORTB_PCR8 |= 1 << 8;
    PORTB_PCR9 |= 1 << 8;
    PORTB_PCR10 |= 1 << 8;
    PORTB_PCR11 |= 1 << 8;
    
    GPIOE_PDDR |= 1 << 2; //Coloca como saída
    GPIOE_PDDR |= 1 << 3; //Coloca como saída
    GPIOE_PDDR |= 1 << 4; //Coloca como saída
    GPIOE_PDDR |= 1 << 5; //Coloca como saída
    GPIOE_PDDR |= 1 << 23; //Coloca como saída
    GPIOE_PDDR |= 1 << 29; //Coloca como saída
    GPIOB_PDDR |= 1 << 8; //Coloca como saída
    GPIOB_PDDR |= 1 << 9; //Coloca como saída
    GPIOB_PDDR |= 1 << 10; //Coloca como saída
    GPIOB_PDDR |= 1 << 11; //Coloca como saída
	
	// Portas TIMER
	SIM_SOPT2 |= (1 << 24); //Fonte de clock dos tmps
	SIM_SCGC6 |= (1 << 24); //Habilita TPM0

    lcd_config();
}

int captura_status(){
    //Captura sinais de temperatura e status
}

int escreve_status(float temperatura, char[] status){
    //Escreve temperatura e status no LCD

    char temperatura_str[12];
    sprintf(temperatura_str, "%f", temperatura);

    limpar_display();
    escrita_texto(temperatura_str); //escreve char[] temperatura
    escrita_comando(0xc0);  //break line
    escrita_texto(status);  //escreve char[] status
}

int gera_tensao(float valor){
    //Modifica o valor de tensao baseado no parametro "valor" (de 0 a 2.5V)

    PORTE_PCR30 = (0<<8) + (1<<1) + (1<<0); //DAC + Out + Pull Up
    GPIOE_PDDR |= (1<<30);	//Coloca como saída
    DAC0_C0 = (1<<7);	//Liga o DAC

    Short int data;
    data = (4095/3.3)*valor;
    DAC0_DAT0H = (data>>8);
    DAC0_DAT0L = data;
}

int main()
{
    //Inicializa as portas a serem utilizadas e os demais componentes como lcd
    init_sistema();

    while(1){
        //Ler teclado
        //temperatura_alvo = le_teclado();
        temperatura_alvo = 27.0;
        
        //Captura a temperatura e status do sistema (esteira em operação ou desligada)
        captura_status();

        //Escreve a temperatura e status do sistema (esteira em operação ou desligada) no lcd
        escreve_status(temperatura, status);
                
        //
        gera_tensao(1.4);

    }



}