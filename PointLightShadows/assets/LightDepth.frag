#version 150

out vec4 oColor;

in vec3 vLightVector;

void main(){
  float depth = length( vLightVector );

  float moment1 = depth;
  float moment2 = depth * depth;

  float dx = dFdx(depth);
  float dy = dFdy(depth);
  moment2 += 0.25*(dx*dx+dy*dy);

  oColor = vec4( moment1, moment2, depth, 0.0f );
}