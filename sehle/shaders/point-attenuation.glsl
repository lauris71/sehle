/** IN float distance - The distance from pixel to light source */
/** IN float radius - The outer radius of light influence */
/** IN float falloff DEFAULT 2.0 - The decay factor (higher decays faster) */
/** RETURN float - The relative intensity at given pixel */
/** FUNCTION point_attenuation */

float
point_attenuation(float distance, float radius, float falloff)
{
	if (radius <= distance) return 0.0;
    float s = distance / radius;
    return (1.0 - s * s) * (1.0 - s * s) / (1.0 + falloff * s);
}
