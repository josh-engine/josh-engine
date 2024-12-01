struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) normal: vec3f,
}

@fragment
fn main(in: VertexOut) -> @location(0) vec4f {
    return vec4f(in.uv, 0.0, 1.0);
}