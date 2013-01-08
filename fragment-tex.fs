uniform sampler2D texUnit;

varying vec4 color;
varying vec2 texCoord;

void main() {
vec4 texel = texture2D(texUnit,texCoord);
  gl_FragColor = texel * color;
//gl_FragColor = color;
//gl_FragColor = vec4(texCoord.t,texCoord.t,0.0,1.0);
}
