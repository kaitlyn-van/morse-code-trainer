void setup() {
  // put your setup code here, to run once:

  cli(); //disables the arduino timers
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); //manipulate 4 least significant bits, first 3 are used to manipulate prescaler, 4th is used to compare timer counter and the OCR1A
  OCR1A = 250; //value for 1-ms timer
  TIMSK1 = (1 << OCIE1A);
  sei();

}

ISR(TIMER1_CONDPA_vect)
{
  counter++; // increase 1 each time interrupts fire (number of milliseconds since you started the program), new millis!
}

void loop() {
  // put your main code here, to run repeatedly:

}
