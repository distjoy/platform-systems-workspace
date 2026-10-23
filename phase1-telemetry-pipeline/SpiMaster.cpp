#include "stm32f4xx.h"
#include "SpiMaster.hpp"


void SpiMaster::init_gpio(){

    //Enable    CLock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    //Set alternative function mode FOR PA5,6,7
    GPIOA->MODER &= ~(GPIO_MODER_MODE5,GPIO_MODER_MODE6,GPIO_MODER_MODE7);
    GPIOA->MODER &= ~(GPIO_MODER_MODE5,GPIO_MODER_MODE6,GPIO_MODER_MODE7);

    //select output speed = high 11
    GPIOA->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR5 | GPIO_OSPEEDER_OSPEEDR6 | GPIO_OSPEEDER_OSPEEDR7);

    //select SPI alternate function af 05 0x0101

    GPIOA->AFR[0] &= ~((0xFu << (5 * 4)) | (0xFu << (6 * 4)) | (0xFu << (7 * 4)));
    GPIOA->AFR[0] |=  ((5u   << (5 * 4)) | (5u   << (6 * 4)) | (5u   << (7 * 4)));


    // 5. Configure PA4 as Manual Software Chip Select (CS) - General Purpose Output (MODER = 01b)
    GPIOA->MODER &= ~GPIO_MODER_MODER4;
    GPIOA->MODER |=  GPIO_MODER_MODER4_0;


    deselect();
}


void SpiMaster::enable_peripheral_clock(){
    if(spi == SPI1) RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    #if defined(SPI2)
    else if(spi == SPI2) RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    #endif

    #if defined(SPI23)
    else if(spi == SPI3) RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
    #endif

    #if defined(SPI4)
    else if(spi == SPI4) RCC->APB2ENR |= RCC_APB2ENR_SPI4EN;
    #endif

}

void SpiMaster::init(Mode mode, Prescaler divider){
    init_gpio();


    enable_peripheral_clock();
    //Rest both configuration registers
    spi->CR1 = 0;
    spi->CR2 = 0;

    switch (mode)
    {
    case Mode::MODE1:
         //default cpol 0 cpha 0
        break;
    case Mode::MODE2:
        spi->CR1 |= 0 << SPI_CR1_CPOL_Pos | 1 << SPI_CR1_CPHA_Pos;
        break;
    case Mode::MODE3:
        spi->CR1 |= 1 << SPI_CR1_CPOL_Pos | 0 << SPI_CR1_CPHA_Pos;
        break;
    case Mode::MODE4:
        spi->CR1 |= 1 << SPI_CR1_CPOL_Pos | 1 << SPI_CR1_CPHA_Pos;
        break;
    default:
        break;
    }

    spi->CR1 |= static_cast<uint32_t>(divider) << SPI_CR1_BR_Pos;
    //set master
    spi->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI ;

    spi->CR1 = SPI_CR1_SPE;

}

uint8_t SpiMaster::transfer(uint8_t data){

    while(!(spi->SR & SPI_SR_TXE));
    
    *reinterpret_cast<volatile uint8_t*>(&spi->DR) = data;

    while(!(spi->SR & SPI_SR_RXNE));

    return *reinterpret_cast<volatile uint8_t*>(&spi->DR);
}

SpiMaster::Status SpiMaster::read(uint8_t* buffer, size_t length){

    for(size_t i =0; i<length; ++i){
        buffer[i] = transfer(0x00);
    }


    while (spi->SR & SPI_SR_BSY);





    return SpiMaster::Status::Ok;
   
}

SpiMaster::Status SpiMaster::write(const uint8_t* data, size_t length){

    for(size_t i =0; i<length; ++i){
        transfer(data[i]);
    }

    return SpiMaster::Status::Ok;

}