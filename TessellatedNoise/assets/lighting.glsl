vec3 getDiffuse( vec3 normal, vec3 lightVector ){
    float dotProduct = dot( normal, normalize(lightVector) );
    return vec3( max( dotProduct, 0.0 ) );
}


float getSpecular( vec3 position, vec3 normal, vec3 lightVector, float gloss ){
    vec3 halfVector             = normalize( lightVector - normalize( position ) );
    float dotNormalHalf         = max( dot( normal, halfVector ), 0.0 );
    float modifiedSpecularPower = exp2(10 * gloss + 1.528766);
    float specularLighting      = (0.08664 * modifiedSpecularPower + 0.25) * exp2( modifiedSpecularPower * dotNormalHalf - modifiedSpecularPower);
    return specularLighting;
}

float getAttenuation( float dist, float lightRadius, float cutoff ){
    float denom = dist / lightRadius + 1.0;
    float attenuation = 1.0 / ( denom * denom );
    attenuation = ( attenuation - cutoff ) / ( 1.0 - cutoff );
    attenuation = max( attenuation, 0.0 );
    return attenuation;
}