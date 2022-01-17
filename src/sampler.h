// genere une direction sur l'hemisphere, 
// cf GI compendium, eq 34
Vector sample34( const float u1, const float u2 )
{
    // coordonnees theta, phi
    float cos_theta= u1;
    float phi= float(2 * M_PI) * u2;
    
    // passage vers x, y, z
    float sin_theta= std::sqrt(1 - cos_theta*cos_theta);
    return Vector( std::cos(phi) * sin_theta, std::sin(phi) * sin_theta, cos_theta );
}

// evalue la densite de proba, la pdf de la direction, cf GI compendium, eq 34
float pdf34( const Vector& w )
{
    if(w.z < 0) return 0;
    return 1 / float(2 * M_PI);
}

// genere une direction sur l'hemisphere, 
// cf GI compendium, eq 35
Vector sample35( const float u1, const float u2 )
{
    // coordonnees theta, phi
    float cos_theta= std::sqrt(u1);
    float phi= float(2 * M_PI) * u2;
    
    // passage vers x, y, z
    float sin_theta= std::sqrt(1 - cos_theta*cos_theta);
    return Vector( std::cos(phi) * sin_theta, std::sin(phi) * sin_theta, cos_theta );
}

// evalue la densite de proba, la pdf de la direction, cf GI compendium, eq 35
float pdf35( const Vector& w )
{
    if(w.z < 0) return 0;
    return w.z / float(M_PI);
}