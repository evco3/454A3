// train.cpp

#include "train.h"
#include "main.h"
#include "linalg.h"
#include "spline.h"


#define SPHERE_RADIUS 5.0
#define SPHERE_COLOUR 238/255.0, 106/255.0, 20/255.0

float velocity = 70;
float acceleration;

// Draw the train.
//
// 'flag' is toggled by pressing 'F' and can be used for debugging

 
void Train::draw( mat4 &WCStoVCS, mat4 &WCStoCCS, vec3 lightDir, bool flag )

{
#if 1

  // YOUR CODE HERE
  float t = spline->paramAtArcLength( pos );
  

  // Draw cube
  vec3 o, x, y, z;
  spline->findLocalSystem( t, o, x, y, z );

  vec3 axis = vec3(1,0,0) ^ z;
  float theta = atan2( axis.length(), vec3(1,0,0)*z );

  mat4 R;
  R.rows[0] = vec4(x, 0);
  R.rows[1] = vec4(y, 0);
  R.rows[2] = vec4(z, 0);
  R.rows[3] = vec4(0, 0, 0, 1);


  mat4 M   = translate( o ) *  scale( 10, 10, 10 );
  mat4 MV  = WCStoVCS * M;
  mat4 MVP = WCStoCCS * M;

  cube->draw( MV, MVP, lightDir, vec3( SPHERE_COLOUR ) );

#else
  
 

#endif
}

//function to do dot product vetween to vec3s
float dot(vec3 a, vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

  //function that finds the magnitude of downard z direction at local position
  float magnitudeAtPos(float pos, Spline* spline) {
  float t = spline->paramAtArcLength( pos );
  
  vec3 z = spline->eval(t, TANGENT).normalize();

  vec3 localZ = z;
  vec3 worldZ = vec3(0, 0, -1);

  float projection = dot(localZ, worldZ);

  return projection;
}

void Train::advance( float elapsedSeconds )
{
#if 1

  // YOUR CODE HERE
  float arcLength = spline->totalArcLength();

  //check projection of trains' local x onto world z to calculate the angle its facing and adjust speed based on magnitude of that projection
  float magSum = 0;
  for (int i = 0; i < 5; i++) {
    float offset = float(i - 2);
    float currentPos = fmod(pos + arcLength + offset, arcLength);
    magSum += magnitudeAtPos(currentPos, spline);
  }

  //get magnitude at local position
  float magnitude = magSum / 5;


  // float angle = anglesSum / 5;

  //update speed based on the magnitude of the angle
  acceleration = magnitude * 0.8;

  if (velocity + acceleration < 35) {
    velocity = 35;
  } else if (velocity + acceleration > 175) {
    velocity = 175;
  } else {
    velocity += acceleration;
  }
  

  //update the position of the train

  pos += velocity * elapsedSeconds;

  // cout << "Magnitude:" << magnitude << endl;
  // cout << "Velocity:" << velocity << endl;
  // cout << "Acceleration:" << acceleration << endl;


  if(pos > arcLength) {
    pos = 0;
  }


#else

 
  

#endif
}
