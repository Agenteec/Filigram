#include <shaders/Shaders.h>

std::string Shaders::ping =
"#version 330 core\n\
\n\
uniform float time;\n\
uniform float stopingTime;\n\
uniform vec2 mouse;\n\
uniform vec2 resolution;\n\
\n\
const float PI = 3.14159265;\n\
\n\
float tex(vec2 pos) {\n\
    float upper = length(vec2(cos(PI / 3.), sin(PI / 11.)) * length(pos) - pos);\n\
    return min(pow(upper, .2), pow(1. - length(pos), .88));\n\
}\n\
\n\
out vec4 FragColor;\n\
void main(void) {\n\
    vec2 position = (gl_FragCoord.xy - resolution * 0.5) / ((resolution.x + resolution.y)*0.4);\n\
\n\
    position *= 2.;\n\
\n\
    float color = 0.0;\n\
    vec2 circleDistor = position;\n\
\n\
    float len = length(circleDistor);\n\
    if (len < sqrt(5.)) {\n\
        circleDistor /= (sqrt(5.) - len);\n\
    }\n\
\n\
    float modab = 1.;\n\
    float off = mod(time, 1.) * modab;\n\
    circleDistor = normalize(circleDistor) * mod(length(circleDistor) - off, modab);\n\
\n\
    float ang = asin(circleDistor.y / length(circleDistor));\n\
    if (circleDistor.x < 0.) {\n\
        ang = PI - ang;\n\
    }\n\
    ang = mod(ang, PI / 3.);\n\
    circleDistor = vec2(cos(ang), sin(ang)) * length(circleDistor);\n\
\n\
    vec4 col = mix(vec4(0.5), vec4(0.), tex(circleDistor));\n\
    vec4  fadeFactor = vec4(1.0, 1.0, 1.0, 1.0 -stopingTime);\n\
    col *= fadeFactor;\n\
    FragColor = col;\n\
}";
