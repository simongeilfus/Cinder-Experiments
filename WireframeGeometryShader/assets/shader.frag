#version 150

in vec3     gColor;
in vec3     gTriDistance;
out vec4    oColor;

// From Florian Boesch post on barycentric coordinates
// http://codeflow.org/entries/2012/aug/02/easy-wireframe-display-with-barycentric-coordinates/
float edgeFactor( float lineWidth ) {
    vec3 d = fwidth( gTriDistance );
    vec3 a3 = smoothstep( vec3( 0.0 ), d * lineWidth, gTriDistance );
    return min(min(a3.x, a3.y), a3.z);
}

// returns a wireframed color from a fill, stroke and linewidth
vec3 wireframe( vec3 fill, vec3 stroke, float lineWidth ) {
    return mix( stroke, fill, edgeFactor( lineWidth ) );
}
// returns a black wireframed color from a fill and linewidth
vec3 wireframe( vec3 color, float lineWidth ) {
    return wireframe( color, vec3( 0.0 ), lineWidth );
}
// returns a black wireframed color
vec3 wireframe( vec3 color ) {
    return wireframe( color, 1.5 );
}


void main( void ) {
    // output the colored wireframe
    oColor = vec4( wireframe( vec3( 0.5 ) + gColor ), 1.0 );
}