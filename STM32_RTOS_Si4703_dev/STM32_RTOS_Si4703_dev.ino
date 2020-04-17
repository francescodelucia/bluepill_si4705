#include <MapleFreeRTOS900.h>
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include <VMA11.h>
#include <Wire.h>
#include <RotaryEncoder.h>

#define TFT_GREY   0xC638 

#define TFT_CS     PB12
#define TFT_RST    PA4  
#define TFT_DC     PB1
#define TFT_DIN    PA7
#define TFT_CLK    PA5
#define PIN_FUNC   PA15


#define SI4703_RST  PB3
#define SI4703_SDIO PB7
#define SI4703_SCLK PB6

#define F_MIN      875
#define F_MAX      1080

#define BEGIN_FREQ 945
#define BEGIN_VOL  6

#define FUN_TUNE 0x02
#define FUN_VOL 0x01
#define FUN_SEEK 0x00

#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1ULL<<(b))))        // '!!' to make sure this returns 0 or 1

#define BITMASK_SET(x,y) ((x) |= (y))
#define BITMASK_CLEAR(x,y) ((x) &= (~(y)))
#define BITMASK_FLIP(x,y) ((x) ^= (y))
#define BITMASK_CHECK_ALL(x,y) (((x) & (y)) == (y))   // warning: evaluates y twice
#define BITMASK_CHECK_ANY(x,y) ((x) & (y))

#define SET_BITS(x, bm) ((x) |= (bm) ) // same but for multiple bits
#define UNSET_BITS(x, bm) ((x) &= (~(bm))) // same but for multiple bits
//#define CHECK_BITS(y, bm) ((((y) & (bm)) == (bm) ) ? 1u : 0u )
#define CHECK_BITS(y, bm) ((((y) & (bm))) ? 1u : 0u )

#define GUI_VOL   0x0001 //0b00000001 
#define GUI_FRQ   0x0002 //0b00000010
#define GUI_BUT   0x0004 //0b00000100
#define GUI_RDS   0x0008 //0b00001000
#define GUI_TXT   0x0016 //0b00010000
#define GUI_SEEK  0x0032 //0b00100000

/*********************************************************
 this macro to help to write code to ligth
**********************************************************/
#define _TFT ((struct datacls *)pvParameters)->tft
#define _RADIO ((struct datacls *)pvParameters)->radio
#define _ENCODER  ((struct datacls *)pvParameters)->encoder
#define _DATA  ((struct data*)pmdata)

#define GUI_CLR(x) UNSET_BITS(_DATA->render,x)
#define GUI_UPD(x) SET_BITS(_DATA->render,x)  
#define GUI_CHK(x) CHECK_BITS(mdata.render,x)
/******************************************************** 
*********************************************************/

struct data
{
  int channel;  
  int volume;    
  int function; 
  char RadioText[65];
  char pRadioText[65];
  char RDSname[9];
  char pRDSname[9];
  uint8_t render;  
}; 

struct datacls
{
  VMA11* radio;
  RotaryEncoder* encoder; 
  Adafruit_ST7735* tft;  
}; 


VMA11 _radio(SI4703_RST, SI4703_SDIO, SI4703_SCLK);
RotaryEncoder _encoder(PB8, PB9);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

struct data _mdata; 
struct datacls clsdata = {&_radio,&_encoder,&tft};


QueueHandle_t queue_info;
int queueSize_info = 1;

int buttonCount = 0;
long lastUpdateMillis = 0;
int oldPosition = 0;

void drawButtSeek(void* pvParameters,void* pmdata){  
  if(_DATA->function==FUN_SEEK){    
    _TFT->fillRoundRect(0, 50, 50,25, 5, ST7735_WHITE);  
    _TFT->setTextColor(ST7735_GREEN);       
  }else{
    _TFT->fillRoundRect(0, 50, 50,25, 5, TFT_GREY);      
    _TFT->setTextColor(ST7735_WHITE);       
  }  
  _TFT->setTextSize(1);
  _TFT->setCursor(5,60);  
  _TFT->println("-SEEK+");  
  if(_DATA->function==FUN_SEEK){    
    _TFT->drawRoundRect(0,50, 50, 25, 5, ST7735_BLUE);  
  }
  else{
    _TFT->drawRoundRect(0,50, 50, 25, 5, ST7735_WHITE); 
  }
  GUI_CLR(GUI_SEEK);  
}

void drawVol(void* pvParameters,void* pmdata){  
  _TFT->fillRoundRect(80, 0, 160,10, 0, ST7735_BLACK);     
  if(_DATA->function==FUN_VOL){ 
    _TFT->setTextColor(ST7735_RED);   
  }else{
    _TFT->setTextColor(ST7735_WHITE);       
  }  
  _TFT->setTextSize(1);
  _TFT->setCursor(90, 0);  
  _TFT->println("Vol");  
  int x= _DATA->volume;
  int vol = (55/15)*x;             
  if(_DATA->function==FUN_VOL){ 
    _TFT->fillRoundRect(110, 0, vol, 10, 0, ST7735_WHITE);  
  }else{
    _TFT->fillRoundRect(110, 0, vol, 10, 0, ST7735_BLUE);            
  }  
  _TFT->setCursor(130, 0);  
  _TFT->print(_DATA->volume);
  GUI_CLR(GUI_VOL);        
}

void drawRDSname(void* pvParameters,void* pmdata){    
  _TFT->fillRoundRect(0, 90, 160, 20, 0, ST7735_BLACK);
  _TFT->setTextColor(ST7735_GREEN);
  _TFT->setTextSize(2);  
  _TFT->setCursor(0, 90);  
  _TFT->println((_DATA->RDSname));  
  GUI_CLR(GUI_RDS);
}

void drawRDStxt(void* pvParameters,void* pmdata){ 
  _TFT->fillRoundRect(0, 110, 160, 20, 0, ST7735_BLACK);      
  _TFT->setTextColor(ST7735_WHITE);
  _TFT->setTextSize(1);  
  _TFT->setCursor(0, 110);  
  _TFT->setTextWrap(true);
  _TFT->println(_DATA->pRadioText);
  GUI_CLR(GUI_TXT);
}
                          
static void vGUIrender(void* pvParameters){        
  struct data mdata;  
  for(;;){    
    xQueueReceive(queue_info, &mdata, ( portTickType ) 10);                           
    if(GUI_CHK(GUI_FRQ)) drawTuner(pvParameters,&mdata);        
    if(GUI_CHK(GUI_VOL)) drawVol(pvParameters,&mdata);           
    if(GUI_CHK(GUI_SEEK)) drawButtSeek(pvParameters,&mdata);              
    if(GUI_CHK(GUI_TXT)) drawRDStxt(pvParameters,&mdata);                
    if(GUI_CHK(GUI_RDS)) drawRDSname(pvParameters,&mdata);    
    xQueueOverwrite( queue_info, &mdata );  
    vTaskDelay(1);
  }      
}

void drawTuner(void* pvParameters,void* pmdata){   
  GUI_CLR(GUI_FRQ);
  _TFT->fillRoundRect(0, 0, 80,10, 0, ST7735_BLACK);
  _TFT->setCursor(0, 0);  
  _TFT->setTextSize(1);
  _TFT->setTextColor(ST7735_WHITE);  
  _TFT->print(_DATA->channel);
  _TFT->println("Mhz");        
  _TFT->fillRoundRect(0, 10, 160, 30, 6, ST7735_WHITE);     
  _TFT->drawLine(0, 20, 160,20, ST7735_BLACK);
  _TFT->drawLine(0, 21, 160,21, ST7735_GREEN);
  _TFT->drawLine(0, 22, 160,22, ST7735_BLACK);  
  for(int k=0;k<18;k++){
    if(k==4 || k==8 ||  k==12 ){
      _TFT->drawLine(3+k*10,35,3+k*10,15, ST7735_BLACK);
    }else{
      _TFT->drawLine(3+k*10,27,3+k*10,15, ST7735_BLACK);  
    }
  }
  _TFT->setCursor(0, 30);
  _TFT->setTextColor(ST7735_RED);
  _TFT->setTextSize(0); 
  _TFT->print(F_MIN); 
  _TFT->setCursor(75, 30);
  _TFT->setTextColor(ST7735_RED);
  _TFT->setTextSize(0); 
  _TFT->print(F_MIN + ((F_MAX-F_MIN)/2)); 
  _TFT->setCursor(135, 30);
  _TFT->setTextColor(ST7735_RED);
  _TFT->setTextSize(0); 
  _TFT->print(F_MAX); 
  int fDelta = (F_MAX-F_MIN);
  int  xFreq = (((float)158/(float)fDelta)*(fDelta-(F_MAX-_DATA->channel)));  
  if(_DATA->function==FUN_TUNE){ 
     _TFT->fillRect((int)xFreq,12,3, 26, ST7735_RED);  
  }else{
    _TFT->fillRect((int)xFreq,12,3, 26, ST7735_BLUE);      
  } 
}

void splashScreen(void* pvParameters,void* pmdata) {    
  _TFT->setCursor(0, 0);
  _TFT->fillScreen(ST7735_BLACK);
  _TFT->setTextColor(ST7735_WHITE);
  _TFT->setTextSize(1);  
  _TFT->print("--");
  _TFT->println("Mhz"); 
  drawTuner(pvParameters,pmdata);  
  drawVol(pvParameters,pmdata);   
  drawButtSeek(pvParameters,pmdata);
  _TFT->drawLine(0, 85, 160,85, ST7735_YELLOW);
}

void InitGUI(void* pvParameters,void* pmdata){      
  _TFT->initR(INITR_BLACKTAB);     
  _TFT->fillScreen(ST7735_BLACK);
  _TFT->setRotation(1);
  splashScreen(pvParameters,pmdata);  
}
void CleanMem(void* pmdata){
  memset(_DATA->RDSname,0,9);
  memset(_DATA->pRDSname,0,9);
  memset(_DATA->RadioText,0,65);
  memset(_DATA->pRadioText,0,65);
}
void EncoderGestRotation(void* pvParameters,void* pmdata){  
  _ENCODER->tick();
  int newPosition = _ENCODER->getPosition();   
  if (oldPosition != newPosition) {        
    if (newPosition < oldPosition) { 
      switch(_DATA->function){     
        case FUN_TUNE:
          CleanMem(pmdata);                                      
          _DATA->channel ++;
          if (_DATA->channel >=1081) _DATA->channel = 875;    
          _RADIO->setChannel(_DATA->channel);                
          GUI_UPD(GUI_VOL);  
          GUI_UPD(GUI_FRQ);  
          GUI_UPD(GUI_RDS);
          GUI_UPD(GUI_TXT);
          break;
        case FUN_VOL:
          _DATA->volume ++;
          if (_DATA->volume >=16) _DATA->volume = 15;   
          _RADIO->setVolume(_DATA->volume);                               
          SET_BITS(_DATA->render,GUI_VOL);       
          break;
        case FUN_SEEK:
          CleanMem(pmdata);         
          _DATA->channel = _RADIO->seekUp();                               
          GUI_UPD(GUI_RDS);
          GUI_UPD(GUI_FRQ);           
          GUI_UPD(GUI_VOL);  
          GUI_UPD(GUI_TXT);
          break;
      }
    }else {
      switch(_DATA->function){
        case FUN_TUNE:  
          CleanMem(pmdata);                               
          _DATA->channel --; 
          if (_DATA->channel <=874) _DATA->channel = 1080;      
          //mdata->var_channel = 1;          
          _RADIO->setChannel(_DATA->channel);                     
          GUI_UPD(GUI_VOL);  
          GUI_UPD(GUI_FRQ);  
          GUI_UPD(GUI_RDS);
          GUI_UPD(GUI_TXT);  
          break;
       case FUN_VOL:
          _DATA->volume --; 
          if (_DATA->volume <=0) _DATA->volume = 0;   
          _RADIO->setVolume(_DATA->volume);    
          GUI_UPD(GUI_VOL);      
          break;
       case FUN_SEEK:
          CleanMem(pmdata);            
          _DATA->channel = _RADIO->seekDown();                    
          GUI_UPD(GUI_VOL);  
          GUI_UPD(GUI_RDS);
          GUI_UPD(GUI_FRQ);            
          GUI_UPD(GUI_TXT);
          break;
      }
    }                      
    oldPosition = newPosition;  
  }        
}

void buttonFunction(void* pmdata){    
  _DATA->function ++;  
  if (_DATA->function >=3) _DATA->function = 0;   
  GUI_UPD(GUI_VOL);   
  GUI_UPD(GUI_FRQ);  
  GUI_UPD(GUI_SEEK);
}

void longButtonFunction(void* pmdata){    
  _DATA->function = FUN_SEEK;  
  GUI_UPD(GUI_VOL);   
  GUI_UPD(GUI_FRQ);  
  GUI_UPD(GUI_SEEK);    
}

void EncoderGestButton(void *pvParameters,void* pmdata,void(*ButtonFunction)(void*),void (*LongButtonFunction)(void*)){
   if (digitalRead(PIN_FUNC) == 0 && millis() - lastUpdateMillis > 500) {
      if(buttonCount == 2){
        (*LongButtonFunction)(pmdata);        
        buttonCount = 0;        
      }                
      else if(buttonCount==0){
        (*ButtonFunction)(pmdata);       
      }
      lastUpdateMillis = millis();
      buttonCount ++;      
   }
   else if (digitalRead(PIN_FUNC) == 1) {
      buttonCount = 0;             
   }
}

void procRDS(void *pvParameters,void* pmdata,void *ticks){      
  long *startTicks =(long*)ticks ; 
  if(_RADIO->readRDSRadioText(&_DATA->RadioText[0]))    
  {               
    if(strlen(_DATA->RadioText)>0){ 
      strcpy(_DATA->pRadioText,_DATA->RadioText);                                                       
    }    
    GUI_UPD(GUI_TXT);       
  }         
  long nowTicks = xTaskGetTickCount();        
  if ((nowTicks-*(startTicks)) > 3000) {                                         
    _RADIO->readRDSRadioStation(_DATA->RDSname);                                      
    if(strlen(_DATA->RDSname)==0){
      memcpy(_DATA->RDSname,_DATA->pRDSname,9);          
    }else{          
      memcpy(_DATA->pRDSname,_DATA->RDSname,9);          
    }    
    GUI_UPD(GUI_RDS);                   
    *(startTicks) = nowTicks;                  
  }    
}

static void vEncoder(void *pvParameters) {              
    struct data mdata;
    long startTicks = xTaskGetTickCount();      
    for (;;) {
      xQueueReceive(queue_info, &mdata, ( portTickType ) 0);                   
      EncoderGestRotation(pvParameters,&mdata);     
      EncoderGestButton(pvParameters,&mdata,buttonFunction,longButtonFunction); 
      procRDS(pvParameters,&mdata,&startTicks);     
      xQueueOverwrite( queue_info, &mdata );           
    }      
}

void setup(void)
{
  
  //Serial.begin(9600);
  memset(&_mdata,0,sizeof(struct data));
  _mdata.volume = BEGIN_VOL;
  _mdata.channel= BEGIN_FREQ;
  delay(200);                                  
  pinMode(PIN_FUNC, INPUT_PULLUP);  
  _radio.powerOn();  
  _radio.setVolume(_mdata.volume);  
  _radio.setChannel(_mdata.channel);
  delay(100);  
  
  
  queue_info = xQueueCreate( queueSize_info, sizeof(struct data) );
  if(queue_info == NULL){ }  
  xQueueSend(queue_info,&_mdata, ( portTickType ) 0  );

  InitGUI(&clsdata,&_mdata);
  
   xTaskCreate(vEncoder,
                "EncRotary",
                configMINIMAL_STACK_SIZE,
                &clsdata,
                tskIDLE_PRIORITY + 2,
                NULL); 
  xTaskCreate(vGUIrender,
                "GUI",
                800,//configMINIMAL_STACK_SIZE,
                &clsdata,
                tskIDLE_PRIORITY + 4,
                NULL);       
  vTaskStartScheduler();
}

void loop(void){}
