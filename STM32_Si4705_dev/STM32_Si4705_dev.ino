#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include <VMA11.h>
#include <Wire.h>
#include <RotaryEncoder.h>


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
// Color definitions

#define ITERATIONS 500000L    // number of iterations
#define REFRESH_TFT 7500      // refresh bar every 7500 iterations
#define ACTIVATED LOW 

#define FUN_TUNE 2
#define FUN_VOL 1
#define FUN_SEEK 0

struct data
{
  int channel;
  uint8_t var_channel;  
  int volume;  
  uint8_t var_volume; 
  int function;
  char RDSname[9];
  char pRDSname[9];
  char RadioText[65];
  char pRadioText[65];  
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

struct data mdata = {945,0,1,0,0};
struct datacls clsdata = {&_radio,&_encoder,&tft};

int buttonCount = 0;
long lastUpdateMillis = 0;
int oldPosition = 0;



void drawButtSeek(void* pvParameters){
  struct datacls *clsd=(struct datacls *)pvParameters;    
  if(mdata.function==FUN_SEEK){    
    clsd->tft->fillRoundRect(0, 50, 50,25, 5, ST7735_WHITE);  
    clsd->tft->setTextColor(ST7735_GREEN);       
  }else{
    clsd->tft->fillRoundRect(0, 50, 50,25, 5, ST7735_BLACK);      
    clsd->tft->setTextColor(ST7735_RED);       
  }  
  clsd->tft->setTextSize(1);
  clsd->tft->setCursor(5,60);  
  clsd->tft->println("-SEEK+");    
  clsd->tft->drawRoundRect(0,50, 50, 25, 5, ST7735_BLUE);  
}


void drawVol(void* pvParameters){
  struct datacls *clsd=(struct datacls *)pvParameters;   
  clsd->tft->fillRoundRect(80, 0, 160,10, 0, ST7735_BLACK);     
  if(mdata.function==FUN_VOL){ 
    clsd->tft->setTextColor(ST7735_RED);   
  }else{
    clsd->tft->setTextColor(ST7735_WHITE);       
  }  
  clsd->tft->setTextSize(1);
  clsd->tft->setCursor(90, 0);  
  clsd->tft->println("Vol");  
  int vol = (55/15)*mdata.volume;             
  if(mdata.function==FUN_VOL){ 
    clsd->tft->fillRoundRect(110, 0, vol, 10, 0, ST7735_WHITE);  
  }else{
    clsd->tft->fillRoundRect(110, 0, vol, 10, 0, ST7735_BLUE);    
    mdata.var_volume = 0;
  }  
  clsd->tft->setCursor(130, 0);  
  clsd->tft->print(mdata.volume);    
}

                          
void GUIrender(void* pvParameters){
  //Serial.println("Ciao");
  struct datacls *clsd=(struct datacls *)pvParameters;      
  if(mdata.var_channel==1){                
    drawTuner(pvParameters);  
  }  
  else if(mdata.var_volume==1){            
    drawVol(pvParameters);     
  }
  clsd->tft->drawLine(0, 85, 160,85, ST7735_YELLOW);  
  //if(strcmp(mdata.RDSname,mdata.pRDSname)<0){
    clsd->tft->fillRoundRect(0, 90, 160, 20, 0, ST7735_BLACK);
    clsd->tft->setTextColor(ST7735_GREEN);
    clsd->tft->setTextSize(2);  
    clsd->tft->setCursor(0, 90);  
    clsd->tft->println(mdata.RDSname);
  //}   
  if(strcmp(mdata.RadioText,mdata.pRadioText)!=0&&strlen(mdata.RadioText)>0){     
    clsd->tft->fillRoundRect(0, 110, 160, 20, 0, ST7735_BLACK);
    clsd->tft->setTextColor(ST7735_WHITE);
    clsd->tft->setTextSize(1);  
    clsd->tft->setCursor(0, 110);  
    clsd->tft->setTextWrap(true);
    clsd->tft->println(mdata.RadioText);       
  }
  if((strlen(mdata.RadioText)==0)&&strlen(mdata.pRadioText)==0){     
     clsd->tft->fillRoundRect(0, 110, 160, 20, 0, ST7735_BLACK);
  }
  drawButtSeek(pvParameters); 
}

void drawTuner(void* pvParameters){
  struct datacls *clsd=(struct datacls *)pvParameters;  
  clsd->tft->fillRoundRect(0, 0, 80,10, 0, ST7735_BLACK);
  clsd->tft->setCursor(0, 0);  
  clsd->tft->setTextSize(1);
  clsd->tft->setTextColor(ST7735_WHITE);  
  clsd->tft->print(mdata.channel);
  clsd->tft->println("Mhz");        
  clsd->tft->fillRoundRect(0, 10, 160, 30, 6, ST7735_WHITE);     
  clsd->tft->drawLine(0, 20, 160,20, ST7735_BLACK);
  clsd->tft->drawLine(0, 21, 160,21, ST7735_GREEN);
  clsd->tft->drawLine(0, 22, 160,22, ST7735_BLACK);  
  for(int k=0;k<18;k++){
    if(k==4 || k==8 ||  k==12 ){
      clsd->tft->drawLine(3+k*10,35,3+k*10,15, ST7735_GREEN);
    }else{
      clsd->tft->drawLine(3+k*10,27,3+k*10,15, ST7735_GREEN);  
    }
  }
  clsd->tft->setCursor(0, 30);
  clsd->tft->setTextColor(ST7735_RED);
  clsd->tft->setTextSize(0); 
  clsd->tft->print("875"); 
  clsd->tft->setCursor(75, 30);
  clsd->tft->setTextColor(ST7735_RED);
  clsd->tft->setTextSize(0); 
  clsd->tft->print("981"); 
  clsd->tft->setCursor(140, 30);
  clsd->tft->setTextColor(ST7735_RED);
  clsd->tft->setTextSize(0); 
  clsd->tft->print("108"); 
  int fDelta = (F_MAX-F_MIN);
  int  xFreq = (((float)158/(float)fDelta)*(fDelta-(F_MAX-mdata.channel)));  
  if(mdata.function==FUN_TUNE){ 
     clsd->tft->fillRect((int)xFreq,12,3, 26, ST7735_RED);  
  }else{
    clsd->tft->fillRect((int)xFreq,12,3, 26, ST7735_BLUE);  
    mdata.var_channel= 0;  
  }
}
void splashScreen(void* pvParameters) {  
  struct datacls *clsd=(struct datacls *)pvParameters;  
  clsd->tft->setCursor(0, 0);
  clsd->tft->fillScreen(ST7735_BLACK);
  clsd->tft->setTextColor(ST7735_WHITE);
  clsd->tft->setTextSize(1);  
  clsd->tft->print(mdata.channel);
  clsd->tft->println("Mhz"); 
  drawTuner(pvParameters);  
  drawVol(pvParameters);   
  drawButtSeek(pvParameters);
  clsd->tft->drawLine(0, 85, 160,85, ST7735_YELLOW);
}

void InitGUI(void* pvParameters){  
  struct datacls *clsd=(struct datacls *)pvParameters; 
  clsd->tft->initR(INITR_BLACKTAB);     
  clsd->tft->fillScreen(ST7735_BLACK);
  clsd->tft->setRotation(1);
  splashScreen(pvParameters);  
}



void EncoderGestRotation(void* pvParameters){
  struct datacls *clsd=(struct datacls *)pvParameters;
  clsd->encoder->tick();
  int newPosition = clsd->encoder->getPosition();    
  if (oldPosition != newPosition) { 
     
    //struct data _mdata;
    //xQueueReceive(queue_info, &_mdata, portMAX_DELAY);   
    if (newPosition < oldPosition) {      
      if(mdata.function==FUN_TUNE){
        memset(mdata.RDSname,0,9);
        memset(mdata.pRDSname,0,9);
        memset(mdata.RadioText,0,65);
        memset(mdata.pRadioText,0,65);                                
        mdata.channel ++;
        if (mdata.channel >=1081) mdata.channel = 875;
        mdata.var_channel = 1;
      }else if(mdata.function==FUN_VOL){
        mdata.volume ++;
        if (mdata.volume >=16) mdata.volume = 15;                        
        mdata.var_volume = 1;
      }
      else if(mdata.function==FUN_SEEK){
        memset(mdata.RDSname,0,9);
        memset(mdata.pRDSname,0,9);
        memset(mdata.RadioText,0,65);
        memset(mdata.pRadioText,0,65);   
        mdata.channel = clsd->radio->seekUp();
        mdata.var_channel = 1;            
      }
    }else {
      if(mdata.function==FUN_TUNE){
        memset(mdata.RDSname,0,9);
        memset(mdata.pRDSname,0,9);
        memset(mdata.RadioText,0,65);
        memset(mdata.pRadioText,0,65);                        
        mdata.channel --; 
        if (mdata.channel <=874) mdata.channel = 1080;      
        mdata.var_channel = 1;
      }else if(mdata.function==FUN_VOL){
        mdata.volume --; 
        if (mdata.volume <=0) mdata.volume = 0;      
        mdata.var_volume = 1;
      }else if(mdata.function==FUN_SEEK){
        memset(mdata.RDSname,0,9);
        memset(mdata.pRDSname,0,9);
        memset(mdata.RadioText,0,65);
        memset(mdata.pRadioText,0,65);   
        mdata.channel = clsd->radio->seekDown();
        mdata.var_channel = 1;            
      }
    }    
    //xQueueSend(queue_info, &_mdata, portMAX_DELAY); 
    oldPosition = newPosition;    
    if(mdata.function==FUN_TUNE){      
      clsd->radio->setChannel(mdata.channel);              
    }else if(mdata.function==FUN_VOL){
      clsd->radio->setVolume(mdata.volume);              
    }
    GUIrender(pvParameters);    
  }      
}

void buttonFunction(){  
  mdata.function ++;
  if (mdata.function >=3) mdata.function = 0;    
}
void longButtonFunction(){  
  mdata.function = FUN_SEEK;    
}
static void EncoderGestButton(void *pvParameters,void(*ButtonFunction)(),void (*LongButtonFunction)()){
   if (digitalRead(PIN_FUNC) == 0 && millis() - lastUpdateMillis > 500) {
      if(buttonCount == 2){
        (*LongButtonFunction)();        
        buttonCount = 0;
        GUIrender(pvParameters);
      }                
      else if(buttonCount==0){
        (*ButtonFunction)();
        GUIrender(pvParameters);
      }
      lastUpdateMillis = millis();
      buttonCount ++;
       
   }
   else if (digitalRead(PIN_FUNC) == 1) {
      buttonCount = 0;             
   }
}


static void vEncoder(void *pvParameters) {    
    struct datacls *clsd=(struct datacls *)pvParameters;
    long lastMillis = 0;
    
    for (;;) {            
      EncoderGestRotation(pvParameters);     
      EncoderGestButton(pvParameters,buttonFunction,longButtonFunction);           
      if(clsd->radio->readRDSRadioText(mdata.RadioText))
      {   
          if(strlen(mdata.RadioText)==0){            
              strcpy(mdata.RadioText,mdata.pRadioText);                         
          }else{
            if(strcmp(mdata.RadioText,mdata.pRadioText))
            {            
              strcpy(mdata.pRadioText,mdata.RadioText);
              
            }     
          }    
          GUIrender(pvParameters);    
      }                   
      if (millis() - lastMillis > 5000) {   
            
        clsd->radio->readRDSRadioStation(mdata.RDSname);
        
        if(strlen(mdata.RDSname)==0){
          strcpy(mdata.RDSname,mdata.pRDSname);          
        }else{          
          strcpy(mdata.pRDSname,mdata.RDSname);          
        }                
        GUIrender(pvParameters);        
        lastMillis = millis();       
      }              
    }    
}



void setup(void)
{
  pinMode(PIN_FUNC, INPUT_PULLUP);  
  _radio.powerOn();  
  _radio.setVolume(mdata.volume);  
  _radio.setChannel(mdata.channel);
  delay(100);  
  memset(mdata.RDSname,0,9);
  memset(mdata.pRDSname,0,9);
  memset(mdata.RadioText,0,65);
  memset(mdata.pRadioText,0,65);
  
  InitGUI(&clsdata);
}

void loop(void){
 vEncoder(&clsdata);
}
