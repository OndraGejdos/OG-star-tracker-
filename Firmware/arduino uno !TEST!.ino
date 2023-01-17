#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#define Dir  0 //define output for direction pin to D0
#define Step  1 // define output for step pin to pin D1
#define Eneb 2 // define enable pin to D2
#define Cam 3 //define output pin for release shutter
#define powr 4 //define output for power pin 
int timer1_compare_match; // Define timer compare match register value
bool StpSt = true ; // boolean state that holds current state of step pin . true = high, flase = low
ISR(TIMER1_COMPA_vect){ // Interrupt Service Routine for compare mode
  TCNT1 = timer1_compare_match; // It preload timer and compares it
  if(StpSt == true){ // check if stps is true or false
    digitalWrite(Step, HIGH); // write high to stepper motor
    StpSt = false; // change value of StpSt
  }
  else{
    digitalWrite(Step, LOW);
    StpSt = true;
  }
}
#define btnRIGHT  0 // Define variable to hold current button constant value
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5
int adc_key_in = 0; // Define variable to hold button analog value
int i = 0;
int read_LCD_buttons()
{
   adc_key_in = analogRead(0);// read the value from the sensor
   if (adc_key_in > 1000) return btnNONE; // No button is pressed
   if (adc_key_in < 50)   return btnRIGHT; // Right pressed
   if (adc_key_in < 195)  return btnUP;  // Up presed
   if (adc_key_in < 380)  return btnDOWN;  // Down pressed
   if (adc_key_in < 555)  return btnLEFT;  // Left pressed
   if (adc_key_in < 790)  return btnSELECT; // Select pressed 
   return btnNONE;  // If no valid response return No button pressed
} 
const unsigned long eventInterval = 500;
unsigned long previousTime = 0;
byte blank[8] = {
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
};
void setup(){
Serial.begin(9600);
pinMode(Dir, OUTPUT);
pinMode(Step, OUTPUT);
pinMode(Eneb, OUTPUT);
pinMode(Cam, OUTPUT);
pinMode(powr, OUTPUT);
digitalWrite(Dir,LOW);
noInterrupts();
TCCR1A = 0; // Initialize Timer1
TCCR1B = 0;
timer1_compare_match = 33226; 
TCNT1 = timer1_compare_match;
TCCR1B |= (1 << CS11) | (1 << CS10); // set prescaler to 64
TIMSK1 |= (1 << OCIE1A);
interrupts();
lcd.begin(16, 2);
lcd.createChar(1, blank);
lcd.setCursor(0,0);
lcd.print("OG star tracker ");
lcd.setCursor(0,1);
lcd.print("V1 Clear sky!");
delay(3000);
lcd.write(byte(0));
lcd.clear();
}
int get_number(int x, int y) //funciton for displaying number on screen and returnig entered number int x and y is for position on display
{
  int s = 0;
  int p = 0;
  int d = 0;
  const int gg = x; 
  const int hh = x +1 ;
  const int xx = x + 2 ;
  bool tax = false;
  int pds = 0;
  do{
    lcd.setCursor(gg,y); // print positions where would be numbers displayed
    lcd.print(s);
    lcd.print(p);
    lcd.print(d);
    pds = read_LCD_buttons();
    switch(pds)
    {
      case btnRIGHT :{
      x = x + 1 ;
      break;
      }
      case btnLEFT :{
      x = x - 1 ;
      break;
      }
      case btnUP :{  // check if button down is presed
        if(x == gg ){ // check cursor position
          s = s+1; // add + 1
          if(s>9){s = 0;} // if number is bigger thean 9 it would go to 0
        }
        else if(x == hh ){ 
          p = p+1; 
          if(p>9){p = 0;}
        }
        else if(x == xx ){ 
          d = d+1;
          if(d>9){d = 0;}
        }
      break;
      }
      case btnDOWN :{// check if button down is presed
        if(x == gg ){ // check cursor position 
          s = s-1; // deduct number by -1
          if(s<0){s = 0;} // if number is smaller thean 0 it would go to 0
        }
        else if(x == hh ){ 
          p = p-1;
          if(p<0){p = 0;}
        }
        else if(x == xx ){ 
          d = d-1;
          if(d<0){d = 0;}
        }
        break;
      }
      case btnSELECT :{ // check if button select is presed 
        int exposure = s*100 + p*10 + d ; // make final number
        s = 0 ; p = 0 ; d = 0 ; // return variables to 0 
        tax = true;
        return exposure; // return number inputed by user
        break; // end the cycle
      }
    }
    if(x > xx || x  < gg){x = gg;}
  }while( tax != true );
}
void exp(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Enter expousure");
  lcd.setCursor(0,1);
  lcd.print("lenght :     S"); 
  delay(1000);
  int leng = get_number(9,1);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Enter number of");
  lcd.setCursor(0,1);
  lcd.print("exposures : "); 
  delay(1000);
  int exp = get_number(12,1);
  lcd.setCursor(0,0);
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("TRACKING");
  lcd.setCursor(0,1);
  lcd.print("EXPOSURE : ");
  int exp_amount = 0;
  exp_amount = leng / 0.26582 ;
  int l = 0;
  while(true){
    i = i +1;
    if(i == exp_amount){
      digitalWrite(Cam,HIGH);
      delay(leng);
      digitalWrite(Cam,LOW);
      delay(500);
      lcd.setCursor(11,1);
      l = l + 1;
      lcd.print(l);
      i = 0;
    }
    if(l == exp){
      lcd.setCursor(0,1);
      lcd.print("     DONE     ");
    }
    
  }
}

void loop(){
  int la = 0;
  String stringArray[2] = {"Set Exposure", "Dithering"};
  int menuItem = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Menu item");
  lcd.setCursor(0,1);
  lcd.print(stringArray[0]);
  int button = 5;
  int buttong = 5;
  while(true){
    button =  read_LCD_buttons();
    Serial.println(button);
    Serial.println("AHOJ");
    switch(button){
      case btnUP : {
        lcd.setCursor(0,1);
        la = la + 1;
        if (la > 1){
          la = 0;        
        lcd.print(stringArray[la]);
        }
        while(true){
          buttong =  read_LCD_buttons();
          switch(buttong){
            case btnSELECT :{
              exp();
              break;
            }
            case btnRIGHT :{
              break;
            }
          }
          break;
        }
      }
      case btnDOWN : {
        lcd.setCursor(0,1);
        la = la + 1;
        if (la > 1){
          la = 0;        
        lcd.print(stringArray[la]);
        }
      }
      case btnNONE : {
        break;
      }
      case btnSELECT :{
        if (la == 0);{
          exp();
        }
        if (la == 1){
          break;
        }
      }
    }
  }
}
