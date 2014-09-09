float hash( float n ) { return fract(sin(n)*43758.5453123); }
float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0-2.0*f);
    
    float n = p.x + p.y*157.0 + 113.0*p.z;
    return mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                   mix( hash(n+157.0), hash(n+158.0),f.x),f.y),
               mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                   mix( hash(n+270.0), hash(n+271.0),f.x),f.y),f.z);
}


float fbm( vec3 p, int octaves, float lacunarity, float gain )
{
    float result = 0.0f;
    float amp = 0.5f;
    float dx = 0.0f;
    for( int i = 0; i < octaves; i++ ) {
        float d = noise( p );
        dx += d;
        
        result += amp * d / ( 1.0f + dx*dx );
        p *= lacunarity;
        amp *= gain;
    }
    return result;
}

float fbm( vec3 p )
{
    return fbm( p, 5, 2.0f, 0.5f );
}