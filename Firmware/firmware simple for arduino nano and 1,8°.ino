const int EN=2;         //ENABLE PIN
const int Step=3;       // STEP PIN
const int dir=4;        // DIRECTION PIN
#define button 11 ;
boolean buttonState;
void setup()
{
pinMode(EN,OUTPUT);     // ENABLE AS OUTPUT
pinMode(dir,OUTPUT);    // DIRECTION AS OUTPUT
pinMode(Step,OUTPUT);   // STEP AS OUTPUT
digitalWrite(dir,LOW);
pinMode(button,INPUT_PULLUP);
}
void loop()
{

buttonState = digitalRead(button)
digitalWrite(dir,LOW);
digitalWrite(Step,LOW);    // STEP HIGH
delay(132.91 );                   // WAIT
digitalWrite(Step,HIGH);     // STEP LOW
delay(132.91 );                   // WAIT


}
for(int )