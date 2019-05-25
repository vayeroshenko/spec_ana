// ********************
// Comments
// ********************

// See http://www.ti.com/lit/ds/symlink/tm4c1294ncpdt.pdf for general datasheet.

// See https://www.cse.iitb.ac.in/~erts/html_pages/Resources/Tiva/PeripheralDriverLibrary.pdf
// for peripheral API's.

// See http://e2e.ti.com/support/microcontrollers/tiva_arm/f/908/t/690227 this thread
// for the 2Msps important bug.

// ********************
// Comments End
// ********************

// ********************
// Prototypes Begin
// ********************

// System
void setup();
void setupData();
void setupSerial();
void setupADC();
void setupSysTick();
void loop();

// Networking
void setupNetwork();
void sendUDPData();
void sendKeepAlive();
void sendPeaksData();
void sendHandshake();
void sendBuffer(const char* buffer, unsigned int size);

// ********************
// Prototypes End
// ********************

// ********************
// Global variables begin
// ********************

// Main globals
int i, j;

// ADC globals
unsigned long int values[8];
unsigned long int loops = 0;
unsigned long int timeTotal, timeBegin, timeEnd;

// Detector globals
unsigned int reports = 0;

struct {
  unsigned long int peaks[4096];
  unsigned long int beforeMax = 0;
  unsigned long int currentMax = 0;
  unsigned long int afterMax = 0;
  unsigned long int overTheNoise = 0;
} DetectorRuntime;

// Detector config

struct {
  unsigned long int noiseLevel      = 360;

//  unsigned long int noiseLevel      = 0;

  unsigned long int loopsPerReport  = 8192;
//  unsigned long int loopsPerReport  = 1024*512;
  unsigned long int reportsPerReset = 32;
} DetectorConfig;

// Networking

#include <Ethernet.h>
#include <EthernetUdp.h>
EthernetUDP Udp;

const unsigned int HANDSHAKE = 1;
const unsigned int KEEPALIVE = 2;
const unsigned int CONFIG    = 3;
const unsigned int DATA      = 4;

struct {
  byte mac[6] = {0x00, 0x1A, 0xB6, 0x03, 0x2F, 0x23};
  IPAddress ip = IPAddress(192,168,1,177);
  unsigned int port = 5525;
} NetworkConfig;

struct {
  bool available = true;
  IPAddress ip = IPAddress(192,168,1,100);
  unsigned int port = 5005;
} NetworkClient;


// ********************
// Global variables end
// ********************

// ********************
// Networking begin
// ********************

void setupNetwork() {
  Serial.println("Setting up the ethernet interface.");
  Ethernet.begin(NetworkConfig.mac,NetworkConfig.ip);
  Udp.begin(NetworkConfig.port);
  Serial.print("Waiting for connection at ");
  Serial.print(NetworkConfig.ip);
  Serial.print(":");
  Serial.println(NetworkConfig.port);
}

void sendBuffer(const char* buffer, unsigned int size) {
  if (!NetworkClient.available) {
    return;
  }
  Udp.beginPacket(NetworkClient.ip, NetworkClient.port);
  Udp.write(buffer, size);
  Udp.endPacket();
}

void sendUDPData() {
  sendKeepAlive();
  sendPeaksData(); 
}

void sendKeepAlive() {
  char reply[4];
  memcpy(reply, (char*)(&KEEPALIVE), sizeof(int));
  sendBuffer(reply, sizeof(reply));
}

void sendHandshake() {
  char reply[4];
  memcpy(reply, (char*)(&HANDSHAKE), sizeof(int));
  sendBuffer(reply, sizeof(reply));
}

void sendPeaksData() {
  const int step = 128;
  char reply[3*sizeof(int) + step*sizeof(int)];
  memcpy(reply, (char*)(&DATA), sizeof(int));
  for(int i = 0; i < 32; ++i) {
    int from = i*step;
    int to   = from+step;
    memcpy(reply+sizeof(int), (char*)(&from), sizeof(int));
    memcpy(reply+2*sizeof(int), (char*)(&to), sizeof(int));
    memcpy(reply+3*sizeof(int), (char*)(&(DetectorRuntime.peaks))+sizeof(int)*from, sizeof(int)*step);
    sendBuffer(reply, sizeof(reply));
  }
}

// ********************
// Networking end
// ********************

// ********************
// System Begin
// ********************

#include "driverlib/rom_map.h"
#include "inc/hw_memmap.h" 
#include "inc/hw_types.h" 
#include "driverlib/debug.h" 
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/adc.h"
#include "inc/tm4c1294ncpdt.h"

void setup() {
  int C_FREQ = SysCtlClockFreqSet((SYSCTL_OSC_MAIN|SYSCTL_USE_PLL|SYSCTL_XTAL_25MHZ|SYSCTL_CFG_VCO_320),120000000);
  setupSerial();
  Serial.println("Setting the network up");
  setupNetwork();
  Serial.println("Setting the data up");
  setupData();
  Serial.println("Setting the ADC up");
  setupADC();
  Serial.println("Setting the SysTick up");
  setupSysTick();
  pinMode(RED_LED, OUTPUT);
  Serial.println("Ready to loop");
//  noInterrupts();
  sendHandshake();
}

void setupData() {
}

void setupSerial() {
  Serial.begin(172800); // 115200 * 3/2
  Serial.println("Serial communication established.");
}

void setupADC() {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
  delay(10);
  ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL, 5); // 320MHz / 2 / 5 = 32 MHz
  delay(10);
  ADCSequenceDisable(ADC0_BASE, 0);
  ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_ALWAYS, 0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_CH0 | ADC_CTL_IE);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_CH0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 7, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
  delay(10);
  ADCSequenceEnable(ADC0_BASE, 0);
}

void setupSysTick() {
  SysTickIntDisable();
  SysTickPeriodSet(16000000);
  SysTickIntEnable();
}

int t1, t2, t3;
int tOld = 0;
int tCur = 0;

#include "inc/hw_adc.h"
#define ADC_SEQ                 (ADC_O_SSMUX0)
#define ADC_SEQ_STEP            (ADC_O_SSMUX1 - ADC_O_SSMUX0)
#define ADC_SSFIFO              (ADC_O_SSFIFO0 - ADC_O_SSMUX0)

bool c = false;
const int ui32Base = ADC0_BASE + ADC_SEQ + (ADC_SEQ_STEP * 0);

#define READ_DATA_FROM_ADC0SEQ0 HWREG(ui32Base + ADC_SSFIFO)
#define INTERRUPT_ON_ADC0SEQ0 ((HWREG(ADC0_BASE + ADC_O_RIS) & (0x10000 | (1 << 0))))
#define CLEAR_INTERRUPT_ON_ADC0SEQ0 HWREG(ADC0_BASE + ADC_O_ISC) = 1
#define PROCESS_VALUE(X)  if (X <= DetectorConfig.noiseLevel && DetectorRuntime.currentMax != 0) { \
  DetectorRuntime.peak[DetectorRuntime.beforeMax  -  \
  (3*DetectorRuntime.beforeMax - 4*DetectorRuntime.currentMax + DetectorRuntime.afterMax)* \
  (3*DetectorRuntime.beforeMax - 4*DetectorRuntime.currentMax + DetectorRuntime.afterMax)/ \
  (DetectorRuntime.beforeMax - 2*DetectorRuntime.currentMax + DetectorRuntime.afterMax) / 8] += 1; \
  DetectorRuntime.overTheNoise =  0;                 \
  DetectorRuntime.currentMax = 0;                   \
}                                   \
if (X > DetectorConfig.noiseLevel) {                  \
    if (X > DetectorRuntime.currentMax) {               \
      DetectorRuntime.beforeMax = DetectorRuntime.currentMax;                 \
      DetectorRuntime.currentMax = X;                \
    } \
    
    if (X < DetectorRuntime.currentMax && DetectorRuntime.afterMax == 0) {               \
      DetectorRuntime.afterMax = X     \
    } \
}

int collectedData[9000];
int last = 0;

void loop() {
loopGoTo:
  loops++;
  // Wait for interrupt from Seq 0. See ADCIntStatus for reference
  while(!INTERRUPT_ON_ADC0SEQ0);
  // See ADCIntClear for the source code.
  CLEAR_INTERRUPT_ON_ADC0SEQ0; // Clear interrupt on Seq 0
   collectedData[last] = READ_DATA_FROM_ADC0SEQ0;
   collectedData[last+1] = READ_DATA_FROM_ADC0SEQ0;
   collectedData[last+2] = READ_DATA_FROM_ADC0SEQ0;
   collectedData[last+3] = READ_DATA_FROM_ADC0SEQ0;
   last += 4;
// values[0] = READ_DATA_FROM_ADC0SEQ0;
// values[1] = READ_DATA_FROM_ADC0SEQ0;
// values[2] = READ_DATA_FROM_ADC0SEQ0;
// values[3] = READ_DATA_FROM_ADC0SEQ0;
 PROCESS_VALUE(collectedData[last]);
 PROCESS_VALUE(collectedData[last+1]);
 PROCESS_VALUE(collectedData[last+2]);
 PROCESS_VALUE(collectedData[last+3]);
    if (loops == 2048) {
       
//       for (int z = 0; z < 8192; ++z) {
//         Serial.println(collectedData[z]);
//         
//       }
       sendUDPData();
       loops = 0;
       last = 0;
     }
     
// if (loops == DetectorConfig.loopsPerReport) {
//   loops = 0;
//   reports++;
//   int packetSize = Udp.parsePacket();
//   if (packetSize) {
//     Serial.println("PACKET!");
//   }
//   sendUDPData();
// }

//  if (reports == DetectorConfig.reportsPerReset) {
//    reports = 0;
//    memset((char*)DetectorRuntime.peaks, 0, 4096*sizeof(int));
//    DetectorRuntime.currentMax = 0;
//    DetectorRuntime.overTheNoise = 0;
//  }
  goto loopGoTo;
}
// ********************
// System End
// ********************

// ********************
// REFERENCE
// ********************
/*
uint32_t
SysTickValueGet(void)
{
    //
    // Return the current value of the SysTick counter.
    //
    return(HWREG(NVIC_ST_CURRENT));
}

void
ADCIntClear(uint32_t ui32Base, uint32_t ui32SequenceNum)
{
    //
    // Check the arguments.
    //
    ASSERT((ui32Base == ADC0_BASE) || (ui32Base == ADC1_BASE));
    ASSERT(ui32SequenceNum < 4);

    //
    // Clear the interrupt.
    //
    HWREG(ui32Base + ADC_O_ISC) = 1 << ui32SequenceNum;
}

void
ADCSequenceEnable(uint32_t ui32Base, uint32_t ui32SequenceNum)
{
    //
    // Check the arguments.
    //
    ASSERT((ui32Base == ADC0_BASE) || (ui32Base == ADC1_BASE));
    ASSERT(ui32SequenceNum < 4);

    //
    // Enable the specified sequence.
    //
    HWREG(ui32Base + ADC_O_ACTSS) |= 1 << ui32SequenceNum;
}

void
ADCSequenceDisable(uint32_t ui32Base, uint32_t ui32SequenceNum)
{
    //
    // Check the arguments.
    //
    ASSERT((ui32Base == ADC0_BASE) || (ui32Base == ADC1_BASE));
    ASSERT(ui32SequenceNum < 4);

    //
    // Disable the specified sequences.
    //
    HWREG(ui32Base + ADC_O_ACTSS) &= ~(1 << ui32SequenceNum);
}

uint32_t
ADCIntStatus(uint32_t ui32Base, uint32_t ui32SequenceNum, bool bMasked)
{
    uint32_t ui32Temp;

    //
    // Check the arguments.
    //
    ASSERT((ui32Base == ADC0_BASE) || (ui32Base == ADC1_BASE));
    ASSERT(ui32SequenceNum < 4);

    //
    // Return either the interrupt status or the raw interrupt status as
    // requested.
    //
    if(bMasked)
    {
        ui32Temp = HWREG(ui32Base + ADC_O_ISC) & (0x10001 << ui32SequenceNum);
    }
    else
    {
        ui32Temp = (HWREG(ui32Base + ADC_O_RIS) &
                    (0x10000 | (1 << ui32SequenceNum)));

        //
        // If the digital comparator status bit is set, reflect it to the
        // appropriate sequence bit.
        //
        if(ui32Temp & 0x10000)
        {
            ui32Temp |= 0xF0000;
            ui32Temp &= ~(0x10000 << ui32SequenceNum);
        }
    }

    //
    // Return the interrupt status
    //
    return(ui32Temp);
}
 */
