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
#define BEGIN_VOL  1

/*#define ITERATIONS 500000L    // number of iterations
#define REFRESH_TFT 7500      // refresh bar every 7500 iterations
#define ACTIVATED LOW 
*/

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
  if(((struct data*)pmdata)->function==FUN_SEEK){    
    ((struct datacls *)pvParameters)->tft->fillRoundRect(0, 50, 50,25, 5, ST7735_WHITE);  
    ((struct datacls *)pvParameters)->tft->setTextColor(ST7735_GREEN);       
  }else{
    ((struct datacls *)pvParameters)->tft->fillRoundRect(0, 50, 50,25, 5, TFT_GREY);      
    ((struct datacls *)pvParameters)->tft->setTextColor(ST7735_WHITE);       
  }  
  ((struct datacls *)pvParameters)->tft->setTextSize(1);
  ((struct datacls *)pvParameters)->tft->setCursor(5,60);  
  ((struct datacls *)pvParameters)->tft->println("-SEEK+");  
  if(((struct data*)pmdata)->function==FUN_SEEK){    
    ((struct datacls *)pvParameters)->tft->drawRoundRect(0,50, 50, 25, 5, ST7735_BLUE);  
  }
  else{
    ((struct datacls *)pvParameters)->tft->drawRoundRect(0,50, 50, 25, 5, ST7735_WHITE); 
  }
  UNSET_BITS(((struct data*)pmdata)->render,GUI_SEEK);  
}

void drawVol(void* pvParameters,void* pmdata){  
  ((struct datacls *)pvParameters)->tft->fillRoundRect(80, 0, 160,10, 0, ST7735_BLACK);     
  if(((struct data*)pmdata)->function==FUN_VOL){ 
    ((struct datacls *)pvParameters)->tft->setTextColor(ST7735_RED);   
  }else{
    ((struct datacls *)pvParameters)->tft->setTextColor(ST7735_WHITE);       
  }  
  ((struct datacls *)pvParameters)->tft->setTextSize(1);
  ((struct datacls *)pvParameters)->tft->setCursor(90, 0);  
  ((struct datacls *)pvParameters)->tft->println("Vol");  
  int x=((struct data*)pmdata)->volume;
  int vol = (55/15)*x;             
  if(((struct data*)pmdata)->function==FUN_VOL){ 
    ((struct datacls *)pvParameters)->tft->fillRoundRect(110, 0, vol, 10, 0, ST7735_WHITE);  
  }else{
    ((struct datacls *)pvParameters)->tft->fillRoundRect(110, 0, vol, 10, 0, ST7735_BLUE);            
  }  
  ((struct datacls *)pvParameters)->tft->setCursor(130, 0);  
  ((struct datacls *)pvParameters)->tft->print(((struct data*)pmdata)->volume);
  UNSET_BITS(((struct data*)pmdata)->render,GUI_VOL);        
}

void drawRDSname(void* pvParameters,void* pmdata){    
  ((struct datacls *)pvParameters)->tft->fillRoundRect(0, 90, 160, 20, 0, ST7735_BLACK);
  ((struct datacls *)pvParameters)->tft->setTextColor(ST7735_GREEN);
  ((struct datacls *)pvParameters)->tft->setTextSize(2);  
  ((struct datacls *)pvParameters)->tft->setCursor(0, 90);  
  ((struct datacls *)pvParameters)->tft->println((((struct data*)pmdata)->RDSname));  
  UNSET_BITS(((struct data*)pmdata)->render,GUI_RDS);
}

void drawRDStxt(void* pvParameters,void* pmdata){ 
  ((struct datacls *)pvParameters)->tft->fillRoundRect(0, 110, 160, 20, 0, ST7735_BLACK);      
  ((struct datacls *)pvParameters)->tft->setTextColor(ST7735_WHITE);
  ((struct datacls *)pvParameters)->tft->setTextSize(1);  
  ((struct datacls *)pvParameters)->tft->setCursor(0, 110);  
  ((struct datacls *)pvParameters)->tft->setTextWrap(true);
  ((struct datacls *)pvParameters)->tft->println(((struct data*)pmdata)->pRadioText);
  UNSET_BITS(((struct data*)pmdata)->render,GUI_TXT);
}
                          
static void vGUIrender(void* pvParameters){        
  struct data mdata;  
  for(;;){    
    xQueueReceive(queue_info, &mdata, ( portTickType ) 10);                           
    if(CHECK_BITS(mdata.render,GUI_FRQ)) drawTuner(pvParameters,&mdata);        
    if(CHECK_BITS(mdata.render,GUI_VOL)) drawVol(pvParameters,&mdata);           
    if(CHECK_BITS(mdata.render,GUI_SEEK)) drawButtSeek(pvParameters,&mdata);              
    if(CHECK_BITS(mdata.render,GUI_TXT)) drawRDStxt(pvParameters,&mdata);                
    if(CHECK_BITS(mdata.render,GUI_RDS)) drawRDSname(pvParameters,&mdata);    
    xQueueOverwrite( queue_info, &mdata );  
    vTaskDelay(1);
  }      
}

void drawTuner(void* pvParameters,void* pmdata){   
  UNSET_BITS(((struct data*)pmdata)->render,GUI_FRQ);
  ((struct datacls *)pvParameters)->tft->fillRoundRect(0, 0, 80,10, 0, ST7735_BLACK);
  ((struct datacls *)pvParameters)->tft->setCursor(0, 0);  
  ((struct datacls *)pvParameters)->tft->setTextSize(1);
  ((struct datacls *)pvParameters)->tft->setTextColor(ST7735_WHITE);  
  ((struct datacls *)pvParameters)->tft->print(((struct data*)pmdata)->channel);
  ((struct datacls *)pvParameters)->tft->println("Mhz");        
  ((struct datacls *)pvParameters)->tft->fillRoundRect(0, 10, 160, 30, 6, ST7735_WHITE);     
  ((struct datacls *)pvParameters)->tft->drawLine(0, 20, 160,20, ST7735_BLACK);
  ((struct datacls *)pvParameters)->tft->drawLine(0, 21, 160,21, ST7735_GREEN);
  ((struct datacls *)pvParameters)->tft->drawLine(0, 22, 160,22, ST7735_BLACK);  
  for(int k=0;k<18;k++){
    if(k==4 || k==8 ||  k==12 ){
      ((struct datacls *)pvParameters)->tft->drawLine(3+k*10,35,3+k*10,15, ST7735_GREEN);
    }else{
      ((struct datacls *)pvParameters)->tft->drawLine(3+k*10,27,3+k*10,15, ST7735_GREEN);  
    }
  }
  ((struct datacls *)pvParameters)->tft->setCursor(0, 30);
  ((struct datacls *)pvParameters)->tft->setTextColor(ST7735_RED);
  ((struct datacls *)pvParameters)->tft->setTextSize(0); 
  ((struct datacls *)pvParameters)->tft->print("875"); 
  ((struct datacls *)pvParameters)->tft->setCursor(75, 30);
  ((struct datacls *)pvParameters)->tft->setTextColor(ST7735_RED);
  ((struct datacls *)pvParameters)->tft->setTextSize(0); 
  ((struct datacls *)pvParameters)->tft->print("981"); 
  ((struct datacls *)pvParameters)->tft->setCursor(140, 30);
  ((struct datacls *)pvParameters)->tft->setTextColor(ST7735_RED);
  ((struct datacls *)pvParameters)->tft->setTextSize(0); 
  ((struct datacls *)pvParameters)->tft->print("108"); 
  int fDelta = (F_MAX-F_MIN);
  int  xFreq = (((float)158/(float)fDelta)*(fDelta-(F_MAX-((struct data*)pmdata)->channel)));  
  if(((struct data*)pmdata)->function==FUN_TUNE){ 
     ((struct datacls *)pvParameters)->tft->fillRect((int)xFreq,12,3, 26, ST7735_RED);  
  }else{
    ((struct datacls *)pvParameters)->tft->fillRect((int)xFreq,12,3, 26, ST7735_BLUE);      
  } 
}

void splashScreen(void* pvParameters,void* pmdata) {    
  ((struct datacls *)pvParameters)->tft->setCursor(0, 0);
  ((struct datacls *)pvParameters)->tft->fillScreen(ST7735_BLACK);
  ((struct datacls *)pvParameters)->tft->setTextColor(ST7735_WHITE);
  ((struct datacls *)pvParameters)->tft->setTextSize(1);  
  ((struct datacls *)pvParameters)->tft->print("--");
  ((struct datacls *)pvParameters)->tft->println("Mhz"); 
  drawTuner(pvParameters,pmdata);  
  drawVol(pvParameters,pmdata);   
  drawButtSeek(pvParameters,pmdata);
  ((struct datacls *)pvParameters)->tft->drawLine(0, 85, 160,85, ST7735_YELLOW);
}

void InitGUI(void* pvParameters,void* pmdata){    
  struct datacls *clsd=(struct datacls *)pvParameters; 
  clsd->tft->initR(INITR_BLACKTAB);     
  clsd->tft->fillScreen(ST7735_BLACK);
  clsd->tft->setRotation(1);
  splashScreen(pvParameters,pmdata);  
}

void EncoderGestRotation(void* pvParameters,void* pmdata){  
  ((struct datacls *)pvParameters)->encoder->tick();
  int newPosition = ((struct datacls *)pvParameters)->encoder->getPosition();   
  if (oldPosition != newPosition) {        
    if (newPosition < oldPosition) { 
      switch(((struct data*)pmdata)->function){     
        case FUN_TUNE:
          memset(((struct data*)pmdata)->RDSname,0,9);
          memset(((struct data*)pmdata)->pRDSname,0,9);
          memset(((struct data*)pmdata)->RadioText,0,65);
          memset(((struct data*)pmdata)->pRadioText,0,65);                                
          ((struct data*)pmdata)->channel ++;
          if (((struct data*)pmdata)->channel >=1081) ((struct data*)pmdata)->channel = 875;    
          ((struct datacls *)pvParameters)->radio->setChannel(((struct data*)pmdata)->channel);                
          SET_BITS(((struct data*)pmdata)->render,GUI_VOL);  
          SET_BITS(((struct data*)pmdata)->render,GUI_FRQ);  
          SET_BITS(((struct data*)pmdata)->render,GUI_RDS);
          SET_BITS(((struct data*)pmdata)->render,GUI_TXT);
          break;
        case FUN_VOL:
          ((struct data*)pmdata)->volume ++;
          if (((struct data*)pmdata)->volume >=16) ((struct data*)pmdata)->volume = 15;   
          ((struct datacls *)pvParameters)->radio->setVolume(((struct data*)pmdata)->volume);                               
          SET_BITS(((struct data*)pmdata)->render,GUI_VOL);       
          break;
        case FUN_SEEK:
          memset(((struct data*)pmdata)->RDSname,0,9);
          memset(((struct data*)pmdata)->pRDSname,0,9);
          memset(((struct data*)pmdata)->RadioText,0,65);
          memset(((struct data*)pmdata)->pRadioText,0,65);   
          ((struct data*)pmdata)->channel = ((struct datacls *)pvParameters)->radio->seekUp();                               
          SET_BITS(((struct data*)pmdata)->render,GUI_RDS);
          SET_BITS(((struct data*)pmdata)->render,GUI_FRQ);           
          SET_BITS(((struct data*)pmdata)->render,GUI_VOL);  
          SET_BITS(((struct data*)pmdata)->render,GUI_TXT);
          break;
      }
    }else {
      switch(((struct data*)pmdata)->function){
        case FUN_TUNE:      
          memset(((struct data*)pmdata)->RDSname,0,9);
          memset(((struct data*)pmdata)->pRDSname,0,9);
          memset(((struct data*)pmdata)->RadioText,0,65);
          memset(((struct data*)pmdata)->pRadioText,0,65);                        
          ((struct data*)pmdata)->channel --; 
          if (((struct data*)pmdata)->channel <=874) ((struct data*)pmdata)->channel = 1080;      
          //mdata->var_channel = 1;          
          ((struct datacls *)pvParameters)->radio->setChannel(((struct data*)pmdata)->channel);                     
          SET_BITS(((struct data*)pmdata)->render,GUI_VOL);  
          SET_BITS(((struct data*)pmdata)->render,GUI_FRQ);  
          SET_BITS(((struct data*)pmdata)->render,GUI_RDS);
          SET_BITS(((struct data*)pmdata)->render,GUI_TXT);  
          break;
       case FUN_VOL:
          ((struct data*)pmdata)->volume --; 
          if (((struct data*)pmdata)->volume <=0) ((struct data*)pmdata)->volume = 0;   
          ((struct datacls *)pvParameters)->radio->setVolume(((struct data*)pmdata)->volume);    
          SET_BITS(((struct data*)pmdata)->render,GUI_VOL);      
          break;
       case FUN_SEEK:
          memset(((struct data*)pmdata)->RDSname,0,9);
          memset(((struct data*)pmdata)->pRDSname,0,9);
          memset(((struct data*)pmdata)->RadioText,0,65);
          memset(((struct data*)pmdata)->pRadioText,0,65);   
          ((struct data*)pmdata)->channel = ((struct datacls *)pvParameters)->radio->seekDown();                    
          SET_BITS(((struct data*)pmdata)->render,GUI_VOL);  
          SET_BITS(((struct data*)pmdata)->render,GUI_RDS);
          SET_BITS(((struct data*)pmdata)->render,GUI_FRQ);            
          SET_BITS(((struct data*)pmdata)->render,GUI_TXT);
          break;
      }
    }                      
    oldPosition = newPosition;  
  }        
}

void buttonFunction(void* pmdata){    
  ((struct data *)pmdata)->function ++;  
  if (((struct data *)pmdata)->function >=3) ((struct data *)pmdata)->function = 0;   
  SET_BITS(((struct data *)pmdata)->render,GUI_VOL);   
  SET_BITS(((struct data *)pmdata)->render,GUI_FRQ);  
  SET_BITS(((struct data *)pmdata)->render,GUI_SEEK);
}

void longButtonFunction(void* pmdata){    
  ((struct data *)pmdata)->function = FUN_SEEK;  
  SET_BITS(((struct data *)pmdata)->render,GUI_VOL);   
  SET_BITS(((struct data *)pmdata)->render,GUI_FRQ);  
  SET_BITS(((struct data *)pmdata)->render,GUI_SEEK);    
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
  if(((struct datacls *)pvParameters)->radio->readRDSRadioText(&((struct data*)pmdata)->RadioText[0]))    
  {               
    if(strlen(((struct data*)pmdata)->RadioText)>0){ 
      strcpy(((struct data*)pmdata)->pRadioText,((struct data*)pmdata)->RadioText);                                                       
    }    
    SET_BITS(((struct data*)pmdata)->render,GUI_TXT);       
  }         
  long nowTicks = xTaskGetTickCount();        
  if ((nowTicks-*(startTicks)) > 3000) {                                         
    ((struct datacls *)pvParameters)->radio->readRDSRadioStation(((struct data*)pmdata)->RDSname);                                      
    if(strlen(((struct data*)pmdata)->RDSname)==0){
      memcpy(((struct data*)pmdata)->RDSname,((struct data*)pmdata)->pRDSname,9);          
    }else{          
      memcpy(((struct data*)pmdata)->pRDSname,((struct data*)pmdata)->RDSname,9);          
    }    
    SET_BITS(((struct data*)pmdata)->render,GUI_RDS);                   
    *(startTicks) = nowTicks;                  
  }    
}

static void vEncoder(void *pvParameters) {    
    //struct datacls *clsd=(struct datacls *)pvParameters;       
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
  if(queue_info == NULL){    
  }  
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
