#include "stm32f4xx.h"


class I2CDriver {
    public:

        enum Status {
            Ok,
            Timeout,
            BusError,
            ArbitrationLost,
            Nack,
            Overrun
        };

        void init(){
            //we are using PB 8 and 9, these are the pins
            // used by I2C on stm32, GPIOB <--> I2C1

            //enable gpio and i2c clocks
            RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
            RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

            //Clear and set mode, alternate function
            GPIOB->MODER &= ~((3U << 16) | (3U <<18));
            GPIOB->MODER |= ((2U << 16) | (2U << 18));

            //Open drain
            GPIOB->OTYPER |= ((1U << 8) | (1U <<9));

            //High speed outputs
            GPIOB->OSPEEDR |= ((3U << GPIO_OSPEEDR_OSPEED8_Pos) | (3U <<GPIO_OSPEEDR_OSPEED9_Pos));

            //Alternate Function
            GPIOB->AFR[1] &=((0xFU << GPIO_AFRH_AFSEL9_Pos) | (0xFU << GPIO_AFRH_AFSEL9_Pos) );
            GPIOB->AFR[1] &=((4U << GPIO_AFRH_AFSEL9_Pos) | (4U << GPIO_AFRH_AFSEL9_Pos) );



            

            I2C1->CR1 &= ~I2C_CR1_PE;

            //Sets pclk1
            I2C1->CR2 = 16U;

            I2C1->CCR = 80U;
            I2C1->TRISE = 17U;
            
            I2C1->CR1 |= I2C_CR1_PE; 




        }

        void writeTo(){

        }

        Status readFrom(uint8_t device, 
            uint8_t rAddr, uint8_t& byteData){

                Status status =  Status::Ok;
                status = start();
                if(status != Status::Ok){
                    handleFailure(status);
                    return status;
                }
                                
                //send address
                status = sendAddress(device, false);
                if(status != Status::Ok)
                    return handleFailure(status);

                //Write register address
                status =  writeByte(&rAddr);
                if(status != Status::Ok)
                    return handleFailure(status);

                //Restart coms

                status = start();
                if(status != Status::Ok){
                    handleFailure(status);
                    return status;
                }

                //send address now for reading
                status = sendAddress(device, true);
                if(status != Status::Ok)
                    return handleFailure(status);

                //prepare to read     
                const uint32_t primask = __get_PRIMASK();
                __disable_irq();

                I2C1->CR1 &= ~I2C_CR1_ACK;
                clearAddressFlag();
                stop();

                __set_PRIMASK(primask); 

                //actually read the data
                status = waitFor(I2C_SR1_RXNE);
                if (status != Status::Ok)
                    return handleFailure(status);

                byteData = static_cast<uint8_t>(I2C1->DR);

                I2C1->CR1 |= I2C_CR1_ACK;
                
            return Status::Ok;

        }
    


    private:

    const uint32_t TIMEOUT = 1000000U;
    Status waitFor(uint32_t flag){
        uint32_t timeout = TIMEOUT;

        while (timeout > 0U)
        {
            uint32_t sr1 = I2C1->SR1;
            if( sr1 & flag)
                return Status::Ok;

            if(sr1 & I2C_SR1_AF)
                return Status::Nack;

            if(sr1 & I2C_SR1_BERR)
                return Status::BusError;

            if(sr1 & I2C_SR1_ARLO)
                return Status::ArbitrationLost;

            if(sr1 & I2C_SR1_OVR)
                return Status::Overrun;

            if(--timeout == 0U)
                return Status::Timeout;
        } 

    }

    bool waitBusIdle(){
        uint32_t timeout = TIMEOUT;

        while (I2C1->SR2 & I2C_SR2_BUSY)
        {
            if(--timeout == 0U)
                return false;
        }
        return true;
    }

    void clearAddressFlag(){

    }


    Status sendAddress(uint8_t addr, bool read){
        I2C1->DR = (addr << 1) | (read ? 1U :0U);

        auto status = waitFor(I2C_SR1_ADDR);
        if(status != Status::Ok)
            return status;

        clearAddressFlag();

    }

    Status start(){
        I2C1->CR1 |= I2C_CR1_START;
        return  waitFor(I2C_SR1_SB);
    }

    void stop(){
        I2C1->CR1 |= I2C_CR1_STOP;
    }

    Status writeByte(uint8_t* byte ){
        Status status = waitFor(I2C_SR1_TXE);
        if(Status::Ok != status)
            return status;

        I2C1->DR = *byte;
        return waitFor(I2C_SR1_BTF);
    }

    Status handleFailure(Status failure){
        if (I2C1->SR2 & I2C_SR2_MSL)
        {
            stop();
        }

        //Clear all error flags
        I2C1->SR1 = ~(
            I2C_SR1_AF |
            I2C_SR1_ARLO|
            I2C_SR1_BERR  | 
            I2C_SR1_OVR
        );

        I2C1->CR1 |= I2C_CR1_ACK; 
        

        return failure;
    }


};



/** start 
select device 
check for acknowledge
write internal register 
check for ack
select device for reading
check for ack
read **/

