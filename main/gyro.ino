void test_gyro_turn() {
    byte turn_speed = 16; //slow to minimize error
    //spin right
    ST.drive(0);
    ST.turn(turn_speed);
    //make 5 full right turns
    angle = 0;
    gyro_angle(360*5);
    ST.stop();
}

//PID demo: go back and forth, report angle
void test_gyro_PID() {
    Serial.print("Enter power: ");
    while(Serial.available() < 2)
        led_blink(1000);
    byte speed = Serial.parseInt();
    
    gyro_PID_output = 64;
    gyro_PID_setpoint = 0;
    angle = 0;
    
    //start PID
    gyro_PID.SetMode(AUTOMATIC);
    
    while(!Serial.available()){ //press key to stop
        //change forwards/backwards every 5 seconds
        if((millis() / 5000) % 2){
            ST.drive(speed);
        } else {
            ST.drive(-speed);
        }
        follow_gyro();
    }
    
    //stop
    ST.stop();
    
    //stop PID
    gyro_PID.SetMode(MANUAL);
}

/* Usage for this function (stale example):
 *
 *  //initialize PID
 *  angle = 0;             //start with angle 0
 *  gyro_PID_setpoint = 0; //keep angle at 0
 *  gyro_PID_output = 64; //start without turning
 *
 *  //go forward (or backwards)
 *  mcWrite(MC_FORWARD, your_speed);
 *  mcWrite(MC_TURN_7BIT, 64);
 *
 *  //enable PID
 *  gyroPID.SetMode(AUTOMATIC);
 *
 *  //wait for some other condition
 *  while(your_condition())
 *    followGyro();
 *
 *  //stop
 *  mcWrite(MC_FORWARD, 0);
 *  mcWrite(MC_TURN_7BIT, 64);
 *
 *  //disable PID
 *  gyroPID.SetMode(MANUAL);
 *
 * (would consider using function pointer for your_condition(), but types of any parameters would need to be known)  */

bool follow_gyro() {
    bool was_updated;
    static unsigned long last_ST_update_ms = 0; //cap rate at which commands are sent
    //update angle, PID, and turning with new gyro reading
    if(gyro_data_ready()){
        was_updated = update_angle();
        if((millis() - last_ST_update_ms) > 50){ //limit to 20Hz*4bytes*10bits/byte=800bps
            last_ST_update_ms = millis();
            int new_turn = gyro_PID_output;
            Serial.print("New turn speed: ");
            Serial.println(new_turn);
            ST.turn(new_turn);
        }
    }
    return was_updated;
}

//A more readable way to test gyro status
//poll status register ZYXDA bit for new data
//instead of DRDY line, check corresponding status register
bool gyro_data_ready(){
    return (gyro.readReg(L3G::STATUS) & ZYXDA) == ZYXDA;
}

/* Gyro calibration
 * based on GyroTest.ino example by G. C. Hill (2013, EE444 at CSULB) (must inquire terms of reuse)
 * from lab 5, p. 6: "8  Calibrate the Gyro"
 *
 * Note: the time required for this initialization
 * might be cutting into time for our robot to start moving
 * Check if rules allow calibration before pressing "go" button
 *
 * gyro -y axis corresponds to robot's +z axis
 */
void gyro_calibrate() {
    //"8.1  Measure Gyro Offset at Rest (Zero-rate Level)"
    Serial.print("Gyro DC Offset: ");
    int32_t dc_offset_sum = 0; //original type "int" overflows!
    for(int n = 0; n < sample_num; n++){
        while(!gyro_data_ready()); //wait for new reading
        digitalWrite(COLOR_LED_PIN, HIGH);//debug LED
        gyro.read();
        digitalWrite(COLOR_LED_PIN,LOW);//debug LED
        dc_offset_sum += gyro_robot_z;
    }
    dc_offset = dc_offset_sum / sample_num;
    Serial.println(dc_offset);
    
    // unused, will need to fix if used
#ifdef GYRO_NOISE_THRESHOLD
    Serial.print("Gyro Noise Level: ");
    for(int n = 0; n < sample_num; n++)
    {
        digitalWrite(COLOR_LED_PIN,HIGH);//debug LED
        gyro.read();
        digitalWrite(COLOR_LED_PIN,LOW);//debug LED
        if((gyro_robot_z - dc_offset) > noise)
            noise = gyro_robot_z - dc_offset;
        else if((gyro_robot_z - dc_offset) < -noise)
            noise = -gyro_robot_z - dc_offset;
    }
    noise /= 100; //"gyro returns hundredths of degrees/sec"
    Serial.println(noise,4); //prints 4 decimal places
#endif
    
}

//wait until past target angle (initialize angle before calling!)
//if turning clockwise, use false for is_counter_clockwise
//Based on "9  Measure Rotational Velocity" and "10  Measure Angle" (Hill 2013, p. 8)
void gyro_angle(float target) {
    bool is_counter_clockwise = (target > angle);
    //debug output:
    Serial.print("gyro_angle(");
    Serial.print(target);
    Serial.println(")");
    //Wait for angle to cross target
    while((is_counter_clockwise && (angle < target)) ||     //increasing angle
          (!is_counter_clockwise && (angle > target)))     //decreasing angle
        if(gyro_data_ready())
            update_angle();
} //end gyroAngle

bool update_angle(){
    
    gyro.read();
    
    rate = (float)(gyro_robot_z - dc_offset) * ADJUSTED_SENSITIVITY;
#ifdef  GYRO_NOISE_THRESHOLD
    //"11  Design Considerations" (p. 10)
    //"Ignore the gyro if our angular velocity does not meet our threshold"
    if(rate >= noise || rate <= -noise)
        //will make angle += ... conditional
#endif
        //angle += ((prev_rate + rate) * ((float)sampleTime / 1000)) / 2; //as-is from p. 9: numerical integration using trapezoidal average of rates
        angle += ((prev_rate + rate) / SAMPLE_RATE) / 2;                   //using measured output data rate
    
    //"remember the current speed for the next loop rate integration."
    prev_rate = rate;
    
    return gyro_PID.Compute();
}


