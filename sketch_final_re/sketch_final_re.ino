#include <Adafruit_NeoPixel.h>
//BETA
  //Hardware
    byte LED              = 4; //LED-Streifen auf Pin
    byte whtbut           = 3; //roter Taster auf Pin
    byte redbut           = 7; //weißer Taster auf Pin
    byte pumpe            = 6; //Pumpenpin
    byte sensorInterrupt  = 0; //= digital pin 2
    byte hallsens         = 2; //Durchflusssensorpin   ->U=3,4V
    byte heat             = 5; //Heizung
    
    //Reverse
    /*byte LED              = 5;//= 6; //LED-Streifen auf Pin
    byte redbut           = 6;//= 4; //roter Taster auf Pin
    byte whtbut           = 7;//= 3; //weißer Taster auf Pin
    byte pumpe            = 3;//= 7; //Pumpenpin
    byte sensorInterrupt  = 0;// 0 = digital pin 2
    byte hallsens         = 2;//= 2; //Durchflusssensorpin
    byte heat             = 4;//= 13;//Heizung*/
    Adafruit_NeoPixel pixels = Adafruit_NeoPixel(12, LED);

  //Variablen LED-Streifen             
    uint32_t red = 0xff0000; //LED rot
    uint32_t blu = 0x0000ff; //LED blau
    uint32_t grn = 0x00ff00; //LED grün
    uint32_t off = 0x000000; //LED aus

  //Globale Variablen
    short mugset = 0; //Anzahl der Tassen
    short mugdone = 0; //Anzahl der fertigen Tassen
    bool skip = false; //zurücksetzen durch gedrückt halten des roten Tasters
    int interrupt = 0; //zurücksetzen bei leerem Tank

  //Variablen Durchflusssensor
    volatile double waterFlow = 0; //Durchfluss insgesamt; volatile für jederzeit änderbaren Wert
    double waterFlowDiff =0; //Alter Durchfluss für automatisches Abschalten
   

void setup() {
    Serial.begin(9600);

    
    //LED-Streifen
    pinMode(redbut, INPUT_PULLUP);
    pinMode(whtbut, INPUT_PULLUP);
    pixels.begin();
    pixels.setBrightness(20); // Helligkeit (255=100%; 0=0%; 120~1/3->Flackern)


  //Durchflusssensor
    waterFlow = 0;
    attachInterrupt(0, pulse, RISING); //DIGITAL Pin 2: Interrupt 0; bei steigender Flanke auf Pin 2 wird die Methode 'pulse' aufgerufen

  //Relais
    pinMode(heat, OUTPUT);
    digitalWrite(heat, LOW); //Heizung standard an
    pinMode(pumpe, OUTPUT);
    digitalWrite(pumpe, HIGH); //Pumpe standard aus

 
  //BOOTANIMATION-1/4: Vier gegenüberliegende LEDs im Ventilatorstil; alle Viertel im Uhrzeigersinn an und anschließend wieder aus
    bootanimation(grn);
    bootanimation(off);
    delay(500);
}



void loop() {
  //LED-Streifen
    LEDeinfaerben(0, red);
      
  //Buttoncases
    if (not digitalRead(whtbut) && not digitalRead(redbut))//kochvorgang starten
      kochen();
      
    if (digitalRead(redbut) && mugset < 12) //Tassen hinzufügen
       mugset++;
      
    if (digitalRead(whtbut) && mugset > 0)//Tassen entfernen
       mugset--;
    delay(150);
}



  //Methode zum Einfärben der LEDs; mode 3 - animation fertig/mode 2 - Farbzusatz zu mode 1/mode 1 - blinken/mode 0 - einfärben 
void LEDeinfaerben(byte mode, uint32_t farbe){
   if (mode == 0){                                          //einfaerben
     for (byte cdel = 0;cdel < 12;cdel ++){
        pixels.setPixelColor(cdel, off);
        pixels.setPixelColor(mugset, farbe);
     }
        
     for (byte cset = 0; cset < mugset;cset++){
        pixels.setPixelColor(cset, farbe);
     }
     pixels.show();
   }
   else if (mode == 1){                                     //blinken
      for (byte cdel = 0;cdel <= mugset;cdel ++)
        pixels.setPixelColor(cdel, off);
      pixels.show();
      delay(250);
      LEDeinfaerben(2, red);
   }
   else if (mode == 2){                                     //fertige Tassen LEDs einfaerben (standard gruen)
      for (byte csetms = 0;csetms < mugset-mugdone; csetms ++)
        pixels.setPixelColor(csetms, farbe);
      for (byte csetmd = mugset-mugdone; csetmd < mugset; csetmd++)
          pixels.setPixelColor(csetmd, grn);
      /*if (mugdone > 0)
        pixels.setPixelColor(mugset-mugdone, 0xffff00);*///eventuell erweitern um aktuelle TassenLED gelb zu färben
      pixels.show();
   }
   else if (mode == 3){                                     //Animation fertig, pulsing circle
      for (byte cini = 0; cini < 12; cini++){
          pixels.setPixelColor(cini, farbe);  //Kreis nacheinander füllen
          pixels.show();
          delay(100);
          skip = not digitalRead(redbut);
      }
      for (byte cini = 11; cini >= 0 && cini < 12; cini--){
          pixels.setPixelColor(cini, off); //Kreis umgekehrt ausschalten
          pixels.show();
          skip = not digitalRead(redbut);
          delay(100);
      }
   }
}
 
 
void kochen(){
    digitalWrite(pumpe, LOW);                         //Relaisport Pumpe einschalten
    mugset++; //Tasse um einen erhöhen, da 1 LED => Wert '0'
    while (mugset > mugdone && interrupt < 3){
      LEDeinfaerben(1, red);
      mugdone = int(waterFlow / 0.075); //Fertige Tassen ergeben sich aus Durchfluss / 75ml(Tasse) -> Messungenauigkeit Sensor 
      //mugdone++; //Funktionssimulation ohne Durchfluss
   
      if (waterFlowDiff == waterFlow)
        interrupt++;
      else if (waterFlowDiff < waterFlow)
        waterFlowDiff = waterFlow;
        
      Serial.println(waterFlow);
      Serial.println(waterFlowDiff);
      Serial.println(mugdone);
      LEDeinfaerben(2, red);
      delay(1000);
    }

    //Reset für erneutes Kochen
    digitalWrite(pumpe, HIGH);
    mugset    = 0;
    mugdone   = 0;
    waterFlow = 0;
    waterFlowDiff = 0;

    while (not skip && interrupt < 3)
        LEDeinfaerben(3, grn);
    while (not skip && interrupt  >= 3)
        LEDeinfaerben(3, red);
    interrupt = 0;
    skip = false;
}


void pulse(){
    waterFlow += 1.0 / 5880.0;
}

void bootanimation(uint32_t farbe){
   for (byte cini = 0; cini < 4; cini++){
      pixels.setPixelColor(cini, farbe);
      pixels.setPixelColor(cini+4, farbe);
      pixels.setPixelColor(cini+8 , farbe);
      pixels.show();
      delay(250);
          }
}

