#include "stm32f4xx.h"


/*
    Bare Metal Code

*/

int main(){

}


void init_dma(){


}


void init_acdc(){

    //Enable ADC Clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    //Choose adc channel
    ADC1->SQR1 &= ~ADC_SQR1_L; //0000 tell ac to do just one channel conversion

    
    ADC1->SQR3 &= ~ADC_SQR3_SQ1; //clear what first coversion was, sq1
    ADC1->SQR3 |= (0U << ADC_SQR3_SQ1_Pos); //Set first conversion (sq1) to channel 0

    // Configure resolution and sampling time
    ADC1->CR1 &= ~ADC_CR1_RES;

    ADC1->SMPR2 &= ~ADC_SMPR2_SMP0;
    ADC1->SMPR2 |= (3U << ADC_SMPR2_SMP0_Pos); //011: 56 cycles

    //Enable adc
    ADC1->CR2 |= ADC_CR2_ADON;

}


void init_gppio_a(){
    //We are using Port A
    //Enable GP_IO port Clock, 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    //COnfigure pin as analog mode
    GPIOA->MODER |= (3U << (0 * 2));

    GPIOA->PUPDR &= ~(3U << (0*2));

}

void adc_read(){

    //Start COnversion
    //Wait for conversion  *
    //Read 
}


