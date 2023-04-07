const int EN=2;         //ENABLE PIN
const int Step=3;       // STEP PIN
const int dir=4;        // DIRECTION PIN
void setup()
{
pinMode(EN,OUTPUT);     // ENABLE AS OUTPUT
pinMode(dir,OUTPUT);    // DIRECTION AS OUTPUT
pinMode(Step,OUTPUT);   // STEP AS OUTPUT
digitalWrite(dir,HIGH);
}
void loop()
{


digitalWrite(dir,HIGH);
digitalWrite(Step,HIGH);    // STEP HIGH
delay(66.48 );                   // WAIT
digitalWrite(Step,LOW);     // STEP LOW
delay(66.48);                   // WAIT


}
