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
  
  // Draw sphere
  vec3 o, x, y, z;
  spline->findLocalSystem( t, o, x, y, z );

  mat4 T = translate(0, 0, 2);

  mat4 M   = translate( o ) * T *  scale( 5, 5, 5 );
  mat4 MV  = WCStoVCS * M;
  mat4 MVP = WCStoCCS * M;

  sphere->draw( MV, MVP, lightDir, vec3( SPHERE_COLOUR ) );

  //draw 4 cylinders behind the first sphere
for (int i = 1; i < 5; i++) {
  float offset = float(i*-10);
  float currentPos = fmod(pos + spline->totalArcLength() + offset, spline->totalArcLength());
  float t = spline->paramAtArcLength( currentPos );
  vec3 o, x, y, z;
  spline->findLocalSystem( t, o, x, y, z );

  //for small bouncing
  // if(((sin(2.0f * M_PI * currentPos / spline->totalArcLength())+i)*10) % 2 += 0) {
  //   o.y += 2;
  // }else {
  //   o.y -= 2;
  // }
  // cout << "sin" << sin(2.0f * M_PI * currentPos / spline->totalArcLength()) << endl;

  vec3 axis = vec3(1,0,0) ^ z;
  float angle = atan2( axis.length(), vec3(1,0,0)*z );

  //translation matrix to move them slighllty in the world y direction
  mat4 T = translate(0, 0, 2);


  mat4 M   = translate( o ) * T * rotate(angle,axis)  * scale( 7, 4, 4) * rotate(1.5, vec3(1, 0, 0)) * rotate(1.5, vec3(0, 1, 0));
  mat4 MV  = WCStoVCS * M;
  mat4 MVP = WCStoCCS * M;

  cylinder->draw( MV, MVP, lightDir, vec3( SPHERE_COLOUR ) );


  float currentPos2 = fmod(pos + spline->totalArcLength()+offset+5, spline->totalArcLength());
  float t2 = spline->paramAtArcLength( currentPos2 );
  vec3 o2, x2, y2, z2;
  spline->findLocalSystem( t2, o2, x2, y2, z2 );

  //draw connecitng pieces
  mat4 T2 = translate(0, -1, 2);
  mat4 rectangleM = translate(o2) * T2 * rotate(angle, axis) * scale(3, 1.3f, 1.3f)*  rotate(1.5, vec3(1, 0, 0)) * rotate(1.5, vec3(0, 1, 0)); 
  mat4 rectangleMV = WCStoVCS * rectangleM;
  mat4 rectangleMVP = WCStoCCS * rectangleM;

  cylinder->draw(rectangleMV, rectangleMVP, lightDir, vec3(1.0f, 1.0f, 1.0f));
}

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
