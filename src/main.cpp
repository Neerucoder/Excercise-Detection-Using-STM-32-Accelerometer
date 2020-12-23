 #include <mbed.h>
 #include "LIS3DSH.h"
 #include <USBSerial.h>


 USBSerial serial;

 LIS3DSH acc(PA_7, PA_6, PA_5, PE_3);
 //          mosi, miso, clk , cs
 DigitalOut MyLed(LED1);
 DigitalOut MyLed1(LED2);
 DigitalOut MyLed2(LED5);
 DigitalOut MyLed3(LED6); //LED Pins

 int main() {
    int16_t X, Y, Z;    //signed integer variables for raw X,Y,Z values
    int state = 1;      //initial state of statemachine
    int jacks =0,situps = 0,pushups = 0,squats = 0; // counter for excercises
    int jackreps = 0,situpreps=0,pushupreps=0,squatreps=0; //counter for reps
	  float g_z = 0;  						// acceleration in z (g force)
    float g_y = 0;              //acceleration in y
    float g_x = 0;              //acceleration i x
    float angle = 0;						// angle relative to x axis
    float anglez = 0;           // angle related to z axis
    float angley = 0;           //angle related to y axis
	  const float PI = 3.1415926;				// pi
    const uint8_t N = 20; 					// filter length
    float ringbuf[N];						// sample buffer for x
    uint8_t ringbuf_index = 0;				// index to insert sample for x
    float ringbufY[N];						// sample buffer for y
    uint8_t ringbuf_indexY = 0;				// index to insert sample for y
    float ringbufZ[N];						// sample buffer for z
    uint8_t ringbuf_indexZ = 0;				// index to insert sample for z


    if(acc.Detect() != 1) {
        serial.printf("LIS3DSH Acceleromoter not detected!\n");
        while(1){ };
    }
    
    while(1) {
      acc.ReadData(&X, &Y, &Z);           //read X, Y, Z values    
      g_z = (float)Z/17694.0;
      g_y = (float)Y/17694.0;
      g_x = (float)X/17694.0;

      
		/* insert in to circular buffer */
		ringbuf[ringbuf_index++] = g_x;
    ringbufY[ringbuf_indexY++] = g_y;
    ringbufZ[ringbuf_indexZ++] = g_z;

		/* at the end of the buffer, wrap around to the beginning */
		if (ringbuf_index >= N) {
			ringbuf_index = 0;
		}
    if (ringbuf_indexY >= N) {
			ringbuf_indexY = 0;
		}
    if (ringbuf_indexZ >= N) {
			ringbuf_indexZ = 0;
		}

		//filtering of raw data
		
		float g_x_filt = 0;
    float g_y_filt = 0;
    float g_z_filt = 0;

		//adding samples
		for (uint8_t i = 0; i < N; i++) {
			g_x_filt += ringbuf[i];
      g_y_filt += ringbufY[i];
      g_z_filt += ringbufZ[i];
		}

		//getting average
		g_x_filt /= (float)N;
    g_y_filt /= (float)N;
    g_z_filt /= (float)N;

		//restrivting to 1g
		if (g_x_filt > 1) {
			g_x_filt = 1;
		}
    if (g_y_filt > 1) {
			g_y_filt = 1;
		}
    if (g_z_filt > 1) {
			g_z_filt = 1;
		}

		


		//angl computations for x,y and z
		angle = 180*acos(g_x_filt)/PI;
    angley = 180*acos(g_y_filt)/PI;
    anglez = 180*acos(g_z_filt)/PI;

    serial.printf("Angle X: %.2f \t Angle Y: %.2f \t Angle Z: %.2f \n", angle,angley,anglez);


      
      //State Machine
      switch (state)
      {
      case 1: //initial state
        MyLed1.write(0); //all LED's off 
        MyLed2.write(0);
        MyLed3.write(0);
        MyLed.write(0);
        if(angle<100){//you are lying face up
          state = 3; //situps
        } 
        else if(angle>145&&anglez<90){ //you are squatting
          state = 4; //squats
        }
        else if(anglez>85){ //you are lying face down
          state = 2; //pushups
        } 
        else if(angle>90&&angle<139&&anglez>55){ //you are on an elevated surface
          state = 5; //jumping jacks
        }  
        break;
      case 2: //pushups
        MyLed1.write(1);
        if(anglez<85){ //you are in a plank position
          if((pushups+1)%15==0){
            pushups=0;
            pushupreps = pushupreps+1; //counter resets after 15 pushups and reps increases 1 rep = 15 excercises
          }
          else{
            pushups=pushups+1; //counter for pushups
          }
        
          state=1;
        }
        break;
      case 3: //Situps
        MyLed2.write(1);
        
        if(angle>100){ // you are sitting
          if((situps+1)%15==0){
            situps=0;
            situpreps = situpreps+1; //counter resets after 15 situpsre increases
          }
          else{
            situps = situps+1; // counter for situps
          }
          
          state = 1;
        }
        break;
      case 4: //Squats
        MyLed3.write(1);
        if(angle<145||anglez>90){ //you are standing up
          //Light Up LED
          if((squats+1)%15==0){ //After completion of 15 Squats Counter is reset and rep increases
            squats=0;
            squatreps = squatreps+1;
          }
          else{
            squats=squats+1; //Counter increases after completion of a squat
          }
          
          state = 1; //Return to initial state
        }
        break;
      case 5: //Jumping Jacks
        MyLed.write(1); //Light Up LED
        
        if(angle>139||angle<135||anglez<55){
             if((jacks+1)%15==0){ //After 15 Jumping jacks the counter resets and the rep increases by 1
            jacks=0;
            jackreps = jackreps+1;
          }
          else{
            jacks = jacks+1; //Counter increases after each jumping jack
          }
          
          state=1; //Return to initial state
          }
        break;
      default:
        serial.printf("Excercise not Detected\n");
        break;
      }
      //print number of Times and Reps completed for each excercise 1 Rep = 15 Times
      serial.printf(" Pushups : %d Reps : %d \n Jacks : %d Reps : %d \n Squats : %d Reps : %d \n Situps : %d Reps : %d \n",pushups,pushupreps,jacks,jackreps,squats,squatreps,situps,situpreps);
      
      wait_ms(100);  //delay before reading next values
      
    }
  }