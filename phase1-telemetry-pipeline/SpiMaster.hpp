#include "stm32f4xx.h"

#include <cstdint>
#include <span>

class SpiMaster {

    public:

        enum class Mode {
            MODE1,
            MODE2,
            MODE3,
            MODE4
        };

        enum class Prescaler : uint8_t {
            Div2 = 0b000,
            Div4 = 0b001,
            Div8 = 0b010,
            Div16 = 0b011,
            Div32 = 0b100,
            Div64 = 0b101,
            Div128 = 0b110,
            Div256 = 0b111

        };


        enum class Status {
            Ok,
            Overrun,
            ModeFault
        };

        SpiMaster(SPI_TypeDef* spi_instance) : spi(spi_instance) {}

        void init(Mode mode, Prescaler divider);

        Status read(uint8_t* buffer, size_t length);
        Status write(const uint8_t* data, size_t length);

        uint8_t transfer(uint8_t data);


        //PA4
        void select(){ GPIOA->BSRR = GPIO_BSRR_BR4;}
        void deselect(){ GPIOA->BSRR = GPIO_BSRR_BS4;}
        

    private:
        SPI_TypeDef* spi;
        void init_gpio();
        void enable_peripheral_clock();
};