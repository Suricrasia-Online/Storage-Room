#version 420
out vec4 fragCol;

float box(vec3 p, vec3 d) {
	vec3 q = abs(p)-d;
	return length(max(q,0.))+min(0.,max(q.x,max(q.y,q.z)));
}

float tex(vec3 p) {
	//marbly texture, also used for RNG
	return sin(dot(sin(p*32.),vec3(2,3,1)))*cos(dot(cos(p*43.),vec3(3,1,2)))
		+sin(dot(sin(p*52.),vec3(2,3,1)))*cos(dot(cos(p*73.),vec3(3,1,2)));
}

float ref;
float scene(vec3 p) {
	float bx = -box(p, vec3(7.9));
	float tx = tex(p*100.)/5000.*(abs(tex(p)+tex(p/5.))*.5+.5); //add very tiny bumps to the SDF itself, which will cause glossy reflections!
	p = asin(sin(p));
	float sbx = box(p,vec3(.6))-.02;
	ref = sbx>bx?sin(length(p)*20.):1.;
	return min(bx, sbx)+tx;
}

void main() {
	vec2 uv = (gl_FragCoord.xy-vec2(960,540))/1080;
	fragCol = vec4(0);
	for (int j = 0; j < 500; j++) {
		uv += vec2(tex(vec3(j+1)),tex(vec3(j)))/2160;

		vec3 cam = normalize(vec3(.5-length(uv)*.5,uv)) + vec3(0,tex(vec3(j)),tex(vec3(j+1)))*.01;
		vec3 p = vec3(-6.5,2,-5) - vec3(0,tex(vec3(j)),tex(vec3(j+1)))*.03;
		bool hit = false;
		float atten = 1.;
		float dist;
		for (int i = 0; i < 100; i++) {
			dist = scene(p);
			if (dist*dist < 1e-6 && ref > -0.5) { //reflect within the raymarching loop!
				hit = false;
				mat3 k = mat3(p,p,p)-mat3(0.01);
				vec3 n = normalize(scene(p)-vec3(scene(k[0]),scene(k[1]),scene(k[2])));
				atten *= 1.-abs(dot(cam,n))*.98;
				cam = reflect(cam, n);
				dist = 0.1;
			}
			p += cam*dist;
		}
		fragCol += vec4(dist*dist<1e-6?(sin(vec3(0,5,4)-p*.2)*.2+1.2)*atten:vec3(0.01), 1);
	}
	fragCol=sqrt(fragCol/fragCol.w);
}
