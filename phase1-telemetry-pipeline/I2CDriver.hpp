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

        void init();

        void writeTo();

            
        Status prepareToRead(uint8_t device, uint8_t store);

        Status setupDMARead(uint8_t device, uint8_t store);

        void finishDMAReading();

        uint32_t getDataAddress();


        Status readFrom(uint8_t device, 
            uint8_t store, uint8_t& byteData);

    private:

    const uint32_t TIMEOUT = 1000000U;
    Status waitFor(uint32_t flag);

    bool waitBusIdle();
    void clearAddressFlag();


    Status sendAddress(uint8_t addr, bool read);

    Status start();

    void stop();

    Status writeByte(uint8_t& byte );

    Status handleFailure(Status failure);

};



/** start 
select device 
check for acknowledge
write internal register 
check for ack
select device for reading
check for ack
read **/

