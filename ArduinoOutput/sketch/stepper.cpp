/*
  stepper.cpp - Library for using Fuyu linear driver with st stepper driver
  Created by Albert Danielsen, November 18 2019
*/

#include "stepper.h"
#include "Arduino.h"

stepper::stepper(int step, int dir, int ena)
{
  STEP = step;
  DIR = dir;
  ENA = ena;
  // Sets the two pins as Outputs
  pinMode(STEP, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(ENA, OUTPUT);

  digitalWrite(STEP, HIGH);
  digitalWrite(DIR, HIGH);
  digitalWrite(ENA, HIGH); // HIGH is off
}

void stepper::enable()
{
  if (! disabled)
  {
    digitalWrite(ENA, LOW); // LOW is on
  }
}

void stepper::disable()
{
  digitalWrite(ENA, HIGH); // HIGH is off
}

void stepper::poweron()
{
   disabled = false;
}

void stepper::shutdown()
{
  this->disable();
   disabled = true;
}

unsigned long stepper::mm_to_pulses(unsigned long mm)
{
  return mm * MM_TO_PULSE;
}

unsigned long stepper::tenth_mm_to_pulses(unsigned long tenth_mm)
{
  return tenth_mm * TENTH_MM_TO_PULSE;
}

void stepper::drive_distance(unsigned long distance_mm, int direction)
{
  unsigned long pulses = this->mm_to_pulses(distance_mm);

  this->enable();

  if (direction == -1)
  {
    // Drive down
    digitalWrite(DIR, LOW);
    // Send the necessary amount of pulses
    for (unsigned long i = 0; i < pulses; i++)
    {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(500);
      digitalWrite(STEP, LOW);
      delayMicroseconds(500);
    }
  }
  else if (direction == 1)
  {
    // Drive up
    digitalWrite(DIR, HIGH);
    // Send the necessary amount of pulses
    for (unsigned long i = 0; i < pulses; i++)
    {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(500);
      digitalWrite(STEP, LOW);
      delayMicroseconds(500);
    }
  }

  this->disable();
}

void stepper::drive_distance_accurate(unsigned long distance_tenth_mm, int direction)
{
  unsigned long pulses = this->tenth_mm_to_pulses(distance_tenth_mm);

  this->enable();

  if (direction == -1)
  {
    // Drive down
    digitalWrite(DIR, LOW);
    // Send the necessary amount of pulses
    for (unsigned long i = 0; i < pulses; i++)
    {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(500);
      digitalWrite(STEP, LOW);
      delayMicroseconds(500);
    }
  }
  else if (direction == 1)
  {
    // Drive up
    digitalWrite(DIR, HIGH);
    // Send the necessary amount of pulses
    for (unsigned long i = 0; i < pulses; i++)
    {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(500);
      digitalWrite(STEP, LOW);
      delayMicroseconds(500);
    }
  }

  this->disable();
}

void stepper::drive_down_pulse()
{
  this->enable();

  digitalWrite(DIR, LOW);

  digitalWrite(STEP, HIGH);
  delayMicroseconds(500);
  digitalWrite(STEP, LOW);
  delayMicroseconds(500);

  this->disable();
}

void stepper::drive_up_pulse()
{
  this->enable();

  digitalWrite(DIR, HIGH);

  digitalWrite(STEP, HIGH);
  delayMicroseconds(500);
  digitalWrite(STEP, LOW);
  delayMicroseconds(500);

  this->disable();
}

void stepper::drive_down_10mm()
{
  this->enable();

  unsigned long pulses = this->mm_to_pulses(10);

  digitalWrite(DIR, LOW);
  for (unsigned long i = 0; i < pulses; i++)
  {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(500);
    digitalWrite(STEP, LOW);
    delayMicroseconds(500);
  }

  this->disable();
}

void stepper::drive_up_10mm()
{
  this->enable();

  unsigned long pulses = this->mm_to_pulses(10);

  digitalWrite(DIR, HIGH);
  for (unsigned long i = 0; i < pulses; i++)
  {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(500);
    digitalWrite(STEP, LOW);
    delayMicroseconds(500);
  }

  this->disable();
}
