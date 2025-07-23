#version 330 core

out vec4 FragColor;

in float height;
in vec3 rgb_colormap0;
in vec3 rgb_colormap1;

vec3 colormap(float height);
vec3 srgb_to_oklab(vec3 rgb);
vec3 oklab_to_srgb(vec3 lab);

void main()
{
    // provide some depth based lighting
    float depth_contribution = 0.8; // [0.0, 1.0]
    vec3 inverted_depth = 1.0 - vec3(gl_FragCoord.z) * depth_contribution;

    // colormap
    vec3 color = colormap(height); // Blue to Red based on height

    // combining colormap with depth
    color *= inverted_depth;

    FragColor = vec4(color, 1.0);
}

vec3 colormap(float height)
{
    // convert to OKLAB before interpolate
    vec3 oklab_colormap0 = srgb_to_oklab(rgb_colormap0);
    vec3 oklab_colormap1 = srgb_to_oklab(rgb_colormap1);

    // creat colormap spectrum with interpolation in OKLAB
    // since height is clamp between [0, 1]
    vec3 oklab_colormap =   height * oklab_colormap0
                          + (1.0 - height) * oklab_colormap1;

    // convert back to RGB and return
    return oklab_to_srgb(oklab_colormap);
}

vec3 srgb_to_oklab(vec3 rgb)
{
    mat3 m1 = mat3 (
         0.4122214708,  0.2119034982,  0.0883024619,
         0.5363325363,  0.6806995451,  0.2817188376,
         0.0514459929,  0.1073969566,  0.6299787005
    );

    mat3 m2 = mat3 (
         0.2104542553,  1.9779984951,  0.0259040371,
         0.7936177850, -2.4285922050,  0.7827717662,
        -0.0040720468,  0.4505937099, -0.8086757660
    );

    vec3 lms = m1 * rgb;
    vec3 lms_ = pow(lms, vec3(0.333333333));

    return m2 * lms_;
}

vec3 oklab_to_srgb(vec3 lab)
{
    mat3 m2_ = mat3 (
         1.0000000000,  1.0000000000,  1.0000000000,
         0.3963377774, -0.1055613458, -0.0894841775,
         0.2158037573, -0.0638541728, -1.2914855480
    );

    mat3 m1_ = mat3 (
         4.0767416621, -1.2684380046, -0.0041960863,
        -3.3077115913,  2.6097574011, -0.7034186147,
         0.2309699292, -0.3413193965,  1.7076147010
    );

    vec3 lms_ = m2_ * lab;
    vec3 lms = lms_ * lms_ * lms_;

    return m1_ * lms;
}
