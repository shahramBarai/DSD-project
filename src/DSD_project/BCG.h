int bcgMeasuringTime = 2000;    // measuring time (ms) for BCG
float z, start, time1, time2;
int flaw;

int maxval=0;
int minval=100000;
boolean controller1=false;
boolean controller2=false;
boolean controller3=false;
boolean controller4=true;

//----------Kalman Filter----------

float _err_measure = 0.99;  // примерный шум измерений
float _q = 0.01;   // скорость изменения значений 0.001-1, варьировать самому
float _last_estimate;

float simpleKalman(float newVal) {
  float _kalman_gain, _current_estimate;
  static float _err_estimate = _err_measure;
  _kalman_gain = (float)_err_estimate / (_err_estimate + _err_measure);
  _current_estimate = _last_estimate + (float)_kalman_gain * (newVal - _last_estimate);
  _err_estimate =  (1.0 - _kalman_gain) * _err_estimate + fabs(_last_estimate - _current_estimate) * _q;
  _last_estimate = _current_estimate;
  return _current_estimate;
}
//----------------------------------

void start_BCG() {
  time1=0; time2=0;
  maxval=0; minval = 100000;
  controller1=false; controller2=false; controller3=false; controller4=true;
}

// We use this function to find the min and maximum value of the graph.
void calibrate_BCG() {
  start = millis();
  while(millis() < start + bcgMeasuringTime)
  {
    mpu_read();
    z = simpleKalman((float)AcZ);
    if (maxval < abs(z)){ maxval = abs(z); }
    if (minval > abs(z)){ minval = abs(z); }
  }
}

int get_BPM() {
  start = millis();
  while(millis() < start + bcgMeasuringTime)
  {
    mpu_read();
    z = simpleKalman((float)AcZ);
    if ((abs(z)<((minval+(maxval-minval)/4)))&& (controller4)){
      controller1=true;
      controller4=false;
    }
      
    if ((abs(z)>((minval+(maxval-minval)*3/4)))&& (controller1)){
      time1=millis();
      controller1=false;
      controller3=true;
      Serial.print("trigger 1");
    }
    if ((abs(z)<((minval+(maxval-minval)/4)))&& (controller3)){
      controller2=true;
      controller3=false;
      Serial.print("trigger 2");
    }
    if ((abs(z)>((minval+(maxval-minval)*3/4)))&& (controller2)){
      controller2=false;
      time2=millis();
      Serial.print("trigger 3");
    }
  }

  Serial.print("heart beat difference in time: ");
  Serial.print(time2-time1);
  Serial.print(" BPM: ");
  Serial.println((1.0/((time2-time1)/1000))*60.0);

  if ((time2 - time1) == 0) {
    return 0;
  } else {
    return (1.0/((time2-time1)/1000))*60.0; 
  }
}

int get_flaw() {
  return flaw;
}

int get_vBPM(){
  int reader, val, n;
  reader = 0; val = 0; n = 0;
  for(int i = 0; i < 10; i++){
    start_BCG();
    calibrate_BCG();
    val = get_BPM();
    if (40 < val && val < 200) {
      reader += val;
      n++;
    }
  }
  
  Serial.print("n = "); Serial.println(n);
  if (n == 0) {
    flaw = 100;
    return 0;
  }
  flaw = 100 - ((n / 10.0) * 100);
  return reader / n;
}
