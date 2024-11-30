@group(0) @binding(0) var<uniform> test: f32;

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) normal: vec3f,
}

@vertex
fn main(@location(0) position: vec3f, @location(1) uv: vec2f, @location(2) normal: vec3f) -> VertexOut {
    var out: VertexOut;
    out.position = vec4f(position - vec3f(0.0, test, 0.0), 1.0);
    out.uv = uv;
    out.normal = normal;
    return out;
}