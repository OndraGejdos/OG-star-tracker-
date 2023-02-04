const int EN=2;         //ENABLE PIN
const int Step=3;       // STEP PIN
const int dir=4;        // DIRECTION PIN
void setup()
{
pinMode(EN,OUTPUT);     // ENABLE AS OUTPUT
pinMode(dir,OUTPUT);    // DIRECTION AS OUTPUT
pinMode(Step,OUTPUT);   // STEP AS OUTPUT
digitalWrite(dir,LOW);
}
void loop()
{


digitalWrite(dir,LOW);
digitalWrite(Step,LOW);    // STEP HIGH
delay(66.455 );                   // WAIT
digitalWrite(Step,HIGH);     // STEP LOW
delay(66.455);                   // WAIT


}
