// persp.cpp


#include "headers.h"
#include "scene.h"
#include "font.h"
#include "main.h"
#include "linalg.h"

#include <strstream>
#include <fstream>
#include <iomanip>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIGHT_DIR 1,1,3

Scene::Scene( char *sceneFilename, GLFWwindow *w )

{
  window = w;

  glfwSetWindowUserPointer( window, this );
  
  // Key handler

  auto static_keyCallback = [](GLFWwindow* ww, int key, int scancode, int action, int mods ) {
    static_cast<Scene*>(glfwGetWindowUserPointer(ww))->keyCallback( ww, key, scancode, action, mods );
  };
 
  glfwSetKeyCallback( window, static_keyCallback );

  // Mouse callbacks

  auto static_mouseButtonCallback = [](GLFWwindow* ww, int button, int action, int keyModifiers ) {
    static_cast<Scene*>(glfwGetWindowUserPointer(ww))->mouseButtonCallback( ww, button, action, keyModifiers );
  };
 
  glfwSetMouseButtonCallback( window, static_mouseButtonCallback );

  auto static_mouseScrollCallback = [](GLFWwindow* ww, double xoffset, double yoffset ) {
    static_cast<Scene*>(glfwGetWindowUserPointer(ww))->mouseScrollCallback( ww, xoffset, yoffset );
  };
 
  glfwSetScrollCallback( window, static_mouseScrollCallback );

  // Set up arcball interface

  arcball = new Arcball( window );

  readView();  // change eye position from default if 'view.txt' exists

  // Set up GPU program
  
  gpu = new GPUProgram();
  gpu->init( vertexShader, fragmentShader, "in Scene constructor" );

  // Set up scene
   
  spline     = new Spline();
  ctrlPoints = new CtrlPoints( spline, window );
  train      = new Train( spline );

  read( sceneFilename );

  // Miscellaneous stuff

  pause = false;
  dragging = false;
  arcballActive = false;
  drawTrack = true;
  drawCoaster = true;
  drawUndersideOnly = false;
  useArcLength = false;
  showAxes = false;
  debug = false;
  flag = false;
}


void Scene::draw( bool useItemTags )

{
  glClearColor( 0,0,0, 0 );

  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glEnable( GL_DEPTH_TEST );

  // model-view transform (i.e. OCS-to-VCS)

  float diag = sqrt( terrain->texture->width*terrain->texture->width + terrain->texture->height*terrain->texture->height );
  
  VCStoCCS = perspective( fovy, 
                          windowWidth/(float)windowHeight, 
                          MAX(0.1,arcball->distToCentre - 1.2*diag),
                          arcball->distToCentre + 1.2*diag );

  if (carView == true){
    float t = spline->paramAtArcLength(train->getPos());
    vec3 o, x, y, z;
    spline->findLocalSystem( t, o, x, y, z );
    
    vec3 carPosition = vec3(-260,-260,15) + spline->value(t);
    vec3 lookAtPoint = carPosition + z - vec3(0,0,0.1); // A point in front of the car

    arcball->setV(carPosition, lookAtPoint, y);
  } else {
    readView();
  }


  mat4 M = translate( -1*(int)(terrain->texture->width)/2, -1*(int)(terrain->texture->height)/2, 0 );
  mat4 V = arcball->V;

  mat4 MV = V * M;
  mat4 MVP = VCStoCCS * MV;
  
  vec3 lightDir = vec3( LIGHT_DIR ).normalize();

  // Draw control points

  ctrlPoints->draw( drawTrack, MV, MVP, lightDir, POST_COLOUR );

  if (ctrlPoints->count() > 1) {
    if (drawTrack)
      drawAllTrack( MV, MVP, lightDir );
    else { // Draw spline
      if (useArcLength)
        spline->drawWithArcLength( MV, MVP, lightDir, debug );
      else
        spline->draw( MV, MVP, lightDir, debug );
    }
  }

  // Draw heightfield

  gpu->activate();

  gpu->setMat4( "MV",  MV );
  gpu->setMat4( "MVP", MVP );

  gpu->setVec3( "lightDir", lightDir );

  terrain->draw( MV, MVP, lightDir, drawUndersideOnly );

  gpu->deactivate();

  // Draw train

  if (ctrlPoints->count() > 1 && drawCoaster)
    train->draw( MV, MVP, lightDir, flag );

  // Now the axes
    
  if (showAxes) {
    MVP = VCStoCCS * V * scale(10,10,10); // no object transformation, since axes are already at origin and aligned.
    axes->draw( MVP );
  }

  // Draw status message

  ostrstream message;
  message << "using " << spline->name() << "        speed " << std::setprecision(2) << train->getSpeed() << '\0';
  render_text( message.str(), 10, 10, window );

  // Done
  
  glfwSwapBuffers( window );
}



// Key callback


void Scene::keyCallback( GLFWwindow* window, int key, int scancode, int action, int mods )

{
  if (action == GLFW_PRESS)
    switch (key) {

    case GLFW_KEY_ESCAPE:
      glfwSetWindowShouldClose( window, GL_TRUE );
      break;

    case 'D':
      debug = !debug;           // enable/disable debugging
      break;

    case 'T':
      drawTrack = !drawTrack;   // enable/disable track drawing
      break;

    case 'C':
      drawCoaster = !drawCoaster; // enable/disable coaster drawing
      break;

    case 'A':
      useArcLength = !useArcLength; // enable/disable arc-length drawing
      break;

    case 'U':
      drawUndersideOnly = !drawUndersideOnly; // enable/disable drawing of terrain
      break;

    case 'P':
      pause = !pause;
      break;

    case 'F':
      flag = !flag;  // general purpose
      break;

    case 'S':                   // store the scene
      if (write())
        cout << "Scene stored in '" << sceneFile << "'." << endl;
      else
        cerr << "FAILED to store scene in '" << sceneFile << "'." << endl;
      break;

    case 'M':                   // change the change-of-basis matrix
      spline->nextCOB();
      break;

    case '+':
    case '=':
      train->accelerate();
      break;

    case '-':
    case '_':
      train->brake();
      break;

    case 'R':
      readView();
      break;

    case 'W':
      writeView();
      break;

    case 'X':
      showAxes = !showAxes;
      break;
    
    case 'V':
      carView = !carView;
      break;

    case '/':  // = ?
      cout << "Click to add a control point." << endl
           << "Ctrl-click to delete a control point." << endl
           << "Move a control point by dragging its base." << endl
           << "Change a control point's height by dragging its top." << endl
           << endl
	   << "-/+ change train speed" << endl
           << "a - toggle arc length parameterization" << endl
           << "c - toggle coaster drawing" << endl
           << "d - toggle debug mode (shows local coordinate frame on track)" << endl
           << "f - toggle flag (useful for debugging)" << endl
           << "m - cycle through CoB matrices" << endl
           << "p - toggle pause" << endl
           << "r - read initial view" << endl
           << "s - store scene in '" << sceneFile << "'" << endl
           << "t - toggle track drawing" << endl
           << "u - toggle underside of terrain" << endl
           << "v - toggle car view" << endl
           << "w - write initial view (for use on next startup)" << endl
           << "x - toggle world axes" << endl
        ;

        break;
    }
}




void Scene::readView()

{
  ifstream in( "../data/view.txt" );

  if (!in)
    return;

  in >> arcball->V;
  in >> arcball->distToCentre;

  float angle;
  in >> angle;
  fovy = angle/180.0*M_PI;
}


void Scene::writeView()

{
  ofstream out( "../data/view.txt" );

  out << arcball->V << endl;
  out << arcball->distToCentre << endl;
  out << fovy*180/M_PI << endl;
}




// Return rayStart and rayDir for the ray in the WCS from the
// viewpoint through the current mouse position (mouseX,mouseY).

void Scene::getMouseRay( int mouseX, int mouseY, vec3 &rayStart, vec3 &rayDir )

{
  vec4 ccsMouse( mouseX/(float)windowWidth*2-1, -1 * (mouseY/(float)windowHeight*2-1), 0, 1 );  // [-1,1]x[-1,1]x0x1
  vec3 wcsMouse = ((VCStoCCS * arcball->V).inverse() * ccsMouse).toVec3();
  
  rayStart = arcball->eyePosition();
  rayDir   = (wcsMouse - rayStart).normalize();
}




void Scene::mouseButtonCallback( GLFWwindow* window, int button, int action, int keyModifiers )

{
  // Get mouse position
  
  double xpos, ypos;
  glfwGetCursorPos( window, &xpos, &ypos );

  if (action == GLFW_PRESS) {

    // Handle mouse press

    // Start tracking the mouse

    auto static_mouseMovementCallback = [](GLFWwindow* window, double xpos, double ypos ) {
      static_cast<Scene*>(glfwGetWindowUserPointer(window))->mouseMovementCallback( window, xpos, ypos );
    };
 
    glfwSetCursorPosCallback( window, static_mouseMovementCallback );

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {

      // 'arcball' handles right mouse event

      arcballActive = true;

      arcball->mousePress( button, xpos, ypos );

    } else if (button == GLFW_MOUSE_BUTTON_LEFT) {

      // 'scene' handles left mouse events

      static vec3 lastMousePos;

      lastMousePos.x = xpos;
      lastMousePos.y = ypos;

      draggingWasDone = false;

      vec3 start,dir;
      getMouseRay( xpos, ypos, start, dir ); // sets 'start' and 'dir'

      mat4 M = translate( -1*(int)(terrain->texture->width)/2, -1*(int)(terrain->texture->height)/2, 0 );

      int hitID = ctrlPoints->findSelectedPoint( start, dir, M );

      if (hitID >= 0) {

        // mouse has picked a control point handle

        startMousePos = vec3( xpos, ypos, 0 );

        dragging = true;

        selectedCtrlPoint = hitID / 2;
        movingSelectedBase = ((hitID % 2) == 0); // base names are even; top names are odd
        startCtrlPointPos = ctrlPoints->points[selectedCtrlPoint];
      }
    }

  } else { 

    // handle mouse release

    if (!draggingWasDone && button == GLFW_MOUSE_BUTTON_LEFT) 
      mouseClick( vec3( xpos, ypos, 0 ), keyModifiers );

    glfwSetCursorPosCallback( window, NULL );
    dragging = false;
    arcballActive = false;
  }
}




// Mouse callbacks

void Scene::mouseMovementCallback( GLFWwindow* window, double xpos, double ypos )

{
  if (arcballActive) {
    arcball->mouseDrag( xpos, ypos );
    return;
  }

  if (!dragging)
    return;

  draggingWasDone = true;

  // Find mouse in world

  vec3 start,dir;
  getMouseRay( xpos, ypos, start, dir ); // sets 'start' and 'dir'

  vec3 updir = arcball->upDirection();
  vec3 n     = (dir ^ updir).normalize();

  mat4 M = translate( -1*(int)(terrain->texture->width)/2, -1*(int)(terrain->texture->height)/2, 0 );

  // Perform the action

  if (movingSelectedBase) {

    // Moving the base along the terrain

    vec3 intPoint;
    bool found = terrain->findIntPoint( start, dir, n, intPoint, M );

    if (found)
      ctrlPoints->moveBase( selectedCtrlPoint, intPoint );
    
  } else {

    // Moving the top up or down

    // Find the plane through eye and mouseInWorld

    vec3 n = ((dir ^ updir) ^ dir).normalize();
    float d = n * start;

    // Find where the vertical line from the base upward intersects this plane
    //
    // Solve for t in n*(base + t*vertical) = d

    vec3 base = (M * vec4( ctrlPoints->bases[ selectedCtrlPoint ], 1 )).toVec3();
    vec3 vertical(0,0,1);

    if (fabs(n*vertical) < 0.001)
      return;

    float t = (d - n*base) / (n*vertical);

    if (t < 1)
      return;

    ctrlPoints->setHeight( selectedCtrlPoint, t );
  }
}



void Scene::mouseScrollCallback( GLFWwindow* window, double xoffset, double yoffset )

{
  arcball->mouseScroll( xoffset, yoffset ); // tell arcball about this
}



void Scene::mouseClick( vec3 mousePosition, int keyModifiers )

{
  // Find ray from eye through mouse

  mat4 M = translate( -1*(int)(terrain->texture->width)/2, -1*(int)(terrain->texture->height)/2, 0 );

  vec3 start,dir;
  getMouseRay( mousePosition.x, mousePosition.y, start, dir ); // sets 'start' and 'dir'

  vec3 updir = arcball->upDirection();
  vec3 n     = (dir ^ updir).normalize();

  if (keyModifiers & GLFW_MOD_CONTROL) {

    // CTRL is held down.  Delete the control point under the mouse

    int hitID = ctrlPoints->findSelectedPoint( start, dir, M );

    if (hitID >= 0)
      ctrlPoints->deletePoint( hitID/2 ); // IDs i and i+1 are for the bottom/top of a post.  Post ID is i/2.

  } else {

    // CTRL is not held down.  Insert a new control point.

    vec3 intPoint;

    if (terrain->findIntPoint( start, dir, n, intPoint, M ))
      ctrlPoints->addPoint( intPoint );
  }
}



// read a scene file


void Scene::read( const char *filename )

{
  sceneFile = strdup( filename );

  // Find directory of this scene file

  char *basePath = strdup( sceneFile );
  char *p = strrchr( basePath, '/' );
  if (p != NULL)
    *p = '\0';
  else {
    char* p = strrchr(basePath, '\\');
    if (p != NULL)
      *p = '\0';
    else
      basePath[0] = '\0';
  }

  // Open file
  
  ifstream in( filename );

  if (!in) {
    cerr << "Could not open file '" << filename << "'." << endl;
    exit(1);
  }

  string cmd;
  in >> cmd;
  while (in) {

    if (cmd == "terrain") {

      string heightFile, textureFile;
      in >> heightFile >> textureFile;

      terrain = new Terrain( string(basePath), heightFile, textureFile );
      in >> cmd;

    } else if (cmd == "points") {

      ctrlPoints->clear();

      in >> cmd;

      while (in && (isdigit(cmd.c_str()[0]) || cmd.c_str()[0] == '-' || cmd.c_str()[0] == '.')) {
        float y, z, h;
        in >> y >> z >> h;
        ctrlPoints->addPointWithHeight( vec3( atof(cmd.c_str()), y, z ), h );
        in >> cmd;

      }
    }
  }

  free( basePath ); 
}


// Write to the scenefile


bool Scene::write()

{
  ofstream out( sceneFile );

  if (!out)
    return false;

  out << "terrain" << endl;
  out << "  " << terrain->heightfield->name << endl;
  out << "  " << terrain->texture->name << endl;
  out << endl;
  out << "points" << endl;
  for (int i=0; i<ctrlPoints->bases.size(); i++)
    out << "  " << ctrlPoints->bases[i] << " " << ctrlPoints->points[i].z - ctrlPoints->bases[i].z << endl;

  return true;
}


// Draw the track


#define DIST_BETWEEN_TIES 15.0
#define NUM_SEGMENTS_BETWEEN_TIES 4


void Scene::drawAllTrack( mat4 &MV, mat4 &MVP, vec3 lightDir )
{
  int divs_per_seg = 20;

  vec3 color = vec3(0.2,1.0,0.4);

  vec3 triangleLineColors[3] = {color, color, color};


  float distanceBetweenRails = 5;
  float triangleHeight = 5;
  

  float trackLength = spline->totalArcLength();
  int numPoints = trackLength;
  float posIncrement = 1;

  vec3 maxPointY = vec3(0,0,0);
  vec3 minPointY = vec3(100000,100000,1000000);

  vec3 points[numPoints];
  vec3 leftPoints[numPoints];
  vec3 rightPoints[numPoints];
  vec3 colours[numPoints];
  for (int i = 0; i < numPoints; i++) {
    float pos = i * posIncrement;
    float t = spline->paramAtArcLength(pos);
    vec3 point = spline->value(t);
    points[i] = point;
    colours[i] = color;

    if (point.y > maxPointY.y) {
      maxPointY = point;
    }
    if (point.y < minPointY.y) {
      minPointY = point;
    }

    vec3 o, x, y, z;
    spline->findLocalSystem(t, o, x, y, z);

    vec3 leftPoint = point + distanceBetweenRails / 2 * x + triangleHeight * y;
    vec3 rightPoint = point - distanceBetweenRails / 2 * x + triangleHeight * y;
    leftPoints[i] = leftPoint;
    rightPoints[i] = rightPoint;
  }
  
  // draw main track
  segs->drawSegs(GL_LINE_LOOP, points, colours, numPoints, MV, MVP, lightDir);

  // draw left and right rails
  segs->drawSegs(GL_LINE_LOOP, leftPoints, colours, numPoints, MV, MVP, lightDir);
  segs->drawSegs(GL_LINE_LOOP, rightPoints, colours, numPoints, MV, MVP, lightDir);

  // Draw Triangles
  for (float pos = 0; pos < trackLength; pos += DIST_BETWEEN_TIES) {
    float t = spline->paramAtArcLength(pos);
    vec3 o, x, y, z;
    spline->findLocalSystem(t, o, x, y, z);
    vec3 point = spline->value(t);
    vec3 leftPoint = point + distanceBetweenRails / 2 * x + triangleHeight * y;
    vec3 rightPoint = point - distanceBetweenRails / 2 * x + triangleHeight * y;
    vec3 triPoints[3] = {point, leftPoint, rightPoint};
    segs->drawSegs(GL_LINE_LOOP, triPoints, triangleLineColors, 3, MV, MVP, lightDir);
  }

  for (float pos = 0; pos < trackLength; pos += 25) {
    float t = spline->paramAtArcLength(pos);
    vec3 o, x, y, z;
    spline->findLocalSystem(t, o, x, y, z);
    vec3 point = spline->value(t);
    
    // Makes sure posts are not to close to control points posts
    if (abs(t - round(t)) < 0.1)  {
      continue;
    }

    // Draw cylinder under point
    mat4 T = translate(0, 0, -point.z/2);
    mat4 M   = translate( o ) * T * scale( 2,2, point.z);
    mat4 MVnew = MV * M;
    mat4 MVPnew = MVP * M;
    cylinder->draw( MVnew, MVPnew, lightDir, vec3( .5, 0.5, 0.5 ) );
  }

  // Draw points evenly spaced in the parameter
  if (debug)
    for (float t=0; t<spline->data.size(); t+=1/(float)divs_per_seg)
      spline->drawLocalSystem( t, MVP );

}



const char *Scene::vertexShader = R"(

  // vertex shader

  #version 300 es

  uniform mat4 MVP;		// OCS-to-CCS
  uniform mat4 MV;		// OCS-to-VCS

  layout (location = 0) in vec3 vertPosition;
  layout (location = 1) in vec3 vertNormal;

  flat out mediump vec3 colour;
  smooth out mediump vec3 normal;


  void main()

  {
    // calc vertex position in CCS

    gl_Position = MVP * vec4( vertPosition, 1.0f );

    // assign (r,g,b) colour equal to (x,y,z) position.  But the (x,y,z)
    // components have to be transformed from [-1,1] to [0,1].

    colour = 0.5 * (vertPosition + vec3(1.0,1.0,1.0));

    // calc normal in VCS
    //
    // To do this, apply the non-translational parts of MV to the model
    // normal.  The 0.0 as the fourth component of the normal ensures
    // that no translational component is added.

    normal = vec3( MV * vec4( vertNormal, 0.0 ) );
 }

)";


const char *Scene::fragmentShader = R"(

  // fragment shader

  #version 300 es

  uniform mediump vec3 lightDir; // direction *toward* light in VCS

  flat in mediump vec3 colour;
  smooth in mediump vec3 normal;

  out mediump vec4 outputColour;


  void main()

  {
    // attenuate colour by cosine of angle between fragment normal and
    // light direction.  But ... if dot product is negative, light is
    // behind fragment and fragment is black (or can appear grey if some
    // "ambient" light is added).

    mediump float NdotL = dot( normalize(normal), lightDir );

    if (NdotL < 0.0)
      NdotL = 0.0;

    mediump vec3 diffuseColour = NdotL * colour;

    //outputColour = vec4( colour, 1.0 );

    //outputColour = vec4( diffuseColour, 1.0 );

    mediump float ambient = 0.15;
    outputColour = vec4( (1.0-ambient) * diffuseColour + ambient * colour, 1.0 );
  }

)";
