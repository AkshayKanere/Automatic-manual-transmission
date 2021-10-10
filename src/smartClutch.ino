/*
  fully released clutch pot value = 700
  fully pressed clutch pot value = 755

*/

#include<SPI.h>
#include <mcp2515.h>

#define BRAKE_SWITCH 9
#define GEAR_KNOB_SWITCH 10

MCP2515 mcp2515(49);


struct can_frame canMsg; // received data
struct can_frame canMsg1; // rpm
struct can_frame canMsg4; // speed


const int potPin = A0;
const int pot_val_fully_pressed = 765;
int pot_val_fully_released = 710;
const int pot_val_biting_point = 752;

const int pot_val_fully_released_no_stall = 710;
const int pot_val_fully_released_stall = 735;

const int press_clutch_relay_pin = 7;
const int release_clutch_relay_pin = 6;

int potValue = 0;
int pressed = 0;
// the setup function runs once when you press reset or power the board
void setup()
{
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(release_clutch_relay_pin, OUTPUT);
  pinMode(press_clutch_relay_pin, OUTPUT);
  digitalWrite(press_clutch_relay_pin, HIGH);
  digitalWrite(release_clutch_relay_pin, HIGH);

  pinMode(12, OUTPUT);
  //analogWrite(12,50);

  digitalWrite(12, LOW);

  pinMode(BRAKE_SWITCH, INPUT);
  pinMode(GEAR_KNOB_SWITCH, INPUT_PULLUP);

  // Serial.begin(115200);
  Serial1.begin(115200);
  SPI.begin();

  Serial.println("mcp init");
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  canMsg1.can_id  = 0x7DF;
  canMsg1.can_dlc = 8;
  canMsg1.data[0] = 0x02;
  canMsg1.data[1] = 0x01;
  canMsg1.data[2] = 0x0C;
  canMsg1.data[3] = 0;
  canMsg1.data[4] = 0;
  canMsg1.data[5] = 0;
  canMsg1.data[6] = 0;
  canMsg1.data[7] = 0;


  canMsg4.can_id  = 0x7DF;
  canMsg4.can_dlc = 8;
  canMsg4.data[0] = 0x02;
  canMsg4.data[1] = 0x01;
  canMsg4.data[2] = 0x0D;
  canMsg4.data[3] = 0x55;
  canMsg4.data[4] = 0x55;
  canMsg4.data[5] = 0x55;
  canMsg4.data[6] = 0x55;
  canMsg4.data[7] = 0x55;

}

int getSpeed()
{

  int flag = 0;
  unsigned long time1 = millis();
  mcp2515.sendMessage(&canMsg4);
  while (flag == 0 && millis() - time1 < 30)
  {
    // Serial.print("1");


    while (mcp2515.readMessage(&canMsg) != MCP2515::ERROR_OK && millis() - time1 < 30);

    if ((canMsg.can_id) == 2024 )
    {
      flag = 1;
      //Serial.print(canMsg.can_id, HEX); // print ID
      //Serial.print("got reply from car");
      return canMsg.data[3];
      canMsg.can_id  = 0;
    }
  }

  if (flag == 0)
  {
    return 9999;
  }

}

int getRPM()
{

  int flag = 0;
  unsigned long time1 = millis();
  mcp2515.sendMessage(&canMsg1);

  while (flag == 0 && millis() - time1 < 30)
  {
    while (mcp2515.readMessage(&canMsg) != MCP2515::ERROR_OK && millis() - time1 < 30);
    if ((canMsg.can_id) == 2024 )
    {
      flag = 1;
      //tft.print(  (( 256 * (canMsg.data[3]) ) + (canMsg.data[4]) ) / 4  );
      int rpm = (256 * canMsg.data[3]) / 4;
      canMsg.can_id  = 0;

      return rpm;
    }


  }
  if (flag == 0)
  {
    int invalid = 9999;
    return invalid;
  }

}

void loop()
{


  int rpm = getRPM();
  Serial1.println(rpm);

  /*
    Serial.print("r ");
    Serial.print(rpm);
    Serial.print("   b ");
    Serial.print(digitalRead(BRAKE_SWITCH));
    Serial.print("   g ");
    Serial.print(digitalRead(GEAR_KNOB_SWITCH));
    Serial.print("   p ");
    Serial.println(analogRead(potPin));
  */

  // if car key switch is in accessory, relase clutch
  if (rpm == 9999)
  {
    if (potValue > pot_val_fully_released)
    {

      digitalWrite(release_clutch_relay_pin, LOW);

      while (potValue > pot_val_fully_released)
      {
        potValue = analogRead(potPin);
        delay(1);
        //Serial.print("loop 2 ");
        //Serial.print("sensor = ");
        // Serial.println(potValue);

      }

      digitalWrite(release_clutch_relay_pin, HIGH);

    }
  }

  // if car key switch is ON , press clutch
  if (rpm == 0)
  {

    if (potValue < pot_val_fully_pressed)
    {

      digitalWrite(press_clutch_relay_pin, LOW);

      while (potValue < pot_val_fully_pressed)
      {
        potValue = analogRead(potPin);
        delay(1);
        //Serial.print("loop 1 ");
        // Serial.print("sensor = ");
        // Serial.println(potValue);

      }

      digitalWrite(press_clutch_relay_pin, HIGH);

    }

  }

  // if car engine is on
  if (rpm > 0 && rpm != 9999)
  {
    potValue = analogRead(potPin);
    delay(1);
    // press clutch if break applied or gear knob switch pressed or rpm low , car stalling
    // dont press clutch if speed is greater than 40
    if  ( (digitalRead(BRAKE_SWITCH) == HIGH && getSpeed() < 40 ) || digitalRead(GEAR_KNOB_SWITCH) == LOW || rpm < 400 )
    {
      pressed = 1;
      if (potValue < pot_val_fully_pressed)
      {

        digitalWrite(press_clutch_relay_pin, LOW);

        while (potValue < pot_val_fully_pressed)
        {
          potValue = analogRead(potPin);
          delay(1);
          //Serial.print("loop 1 ");
          // Serial.print("sensor = ");
          // Serial.println(potValue);

        }

        digitalWrite(press_clutch_relay_pin, HIGH);

      }
    }

    // release clutch till biting point when break is released and gear knob swith is not pressed
    if ( pressed == 1 && digitalRead(BRAKE_SWITCH) == LOW && digitalRead(GEAR_KNOB_SWITCH) == HIGH )
    {
      pressed = 0;
      if (potValue > pot_val_biting_point)
      {

        digitalWrite(release_clutch_relay_pin, LOW);

        while (potValue > pot_val_biting_point)
        {
          potValue = analogRead(potPin);
          delay(1);
          //Serial.print("loop 2 ");
          //Serial.print("sensor = ");
          // Serial.println(potValue);

        }

        digitalWrite(release_clutch_relay_pin, HIGH);

      }

    }

    //release clutch slightly if RPM > set value and gear knob and brake switch not pressed
    potValue = analogRead(potPin);
    delay(1);

    int Speed = getSpeed();

    // dont release clutch fully if at very low speed to prevent stalling.
    // however if user is pressing hard accelerator then release clutch
    // pot_val_fully_released_stall is used to limit clutch release so vehicle wont stall even if accelerator is not applied
    // this is designed specially for when car moves on inclined road from stopped contition
    if (Speed < 10 && rpm < 1500)
    {
      pot_val_fully_released = pot_val_fully_released_stall;
    }
    else
    {
      pot_val_fully_released = pot_val_fully_released_no_stall;
    }

    if ( digitalRead(BRAKE_SWITCH) == LOW && digitalRead(GEAR_KNOB_SWITCH) == HIGH    )
    {

      // if car is stalling then  press clucth slightly
      if (getRPM() < 500 && potValue < pot_val_biting_point - 7)
      {
        digitalWrite(press_clutch_relay_pin, LOW);
        delay(30);
        digitalWrite(press_clutch_relay_pin, HIGH);
        delay(10);
      }

      
      if (analogRead(potPin) > pot_val_fully_released && ( rpm > 1300 || getSpeed()> 40 ) )
      {
        // release clutch when rpm > set value
        int release_delay = 30;
        if (getSpeed() > 40)
        {
          // if speed is greater than 40 then release clutch fully immediately
          release_delay = 1000;
        }
        digitalWrite(release_clutch_relay_pin, LOW);


        unsigned long time2 = millis();
        // non blocking delay
        while ( millis() - time2 < release_delay && analogRead(potPin) > pot_val_fully_released )
        {
          if ( digitalRead(GEAR_KNOB_SWITCH) == LOW || digitalRead(BRAKE_SWITCH) == HIGH  )
          {
            //if gear knob switch or brake is preesed during clutch releasing then break so clutch can be pressed in nect iteration of loop
            break;
          }
          delay(1);
        }
        
        digitalWrite(release_clutch_relay_pin, HIGH);

         time2 = millis();
        // non blocking delay after setting pin
        while ( millis() - time2 < 300 )
        {
          if ( digitalRead(GEAR_KNOB_SWITCH) == LOW || digitalRead(BRAKE_SWITCH) == HIGH  )
          {
            break;
          }
          delay(1);
        }

        // continue if user presses gear knob switch
        // when car is motion and user presses knob switch
        // the clutch should be pressed immediately
        // without waiting for delay to complete
        // so dont wait if clutch is releasing and gear knob switch is pressed
        // before clutch fully rrleased

      }

    }
  }
}
