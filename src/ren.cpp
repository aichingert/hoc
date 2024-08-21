#include <iostream>
#include <fstream>

const char* INDEX_HTML = "index.html";
const char* INDEX_JS   = "index.js";

const char* HTML_CONTENT = R"(
<!DOCTYPE html>
<html>
    <head>
        <title>ren</title>
        <script src="index.js"></script>
        <style>
            html {
                width: 100%;
                height: 100%;
                margin: 0px;
                padding: 0px;
            }
            body {
                margin: 0px;
                padding: 0px;
                width: 100%;
                height: 100%;
                overflow: hidden;
            }

            canvas {
                width: 100%;
                height: 100%;
            }
        </style>
    </head>
    <body>
        <canvas id="game"></canvas>
    </body>
</html>
)";

const char* JS_CONTENT = R"(
async function init() {

    const shaders_src = `
    struct WeirdTransform {
        position: vec2f,
    }

    struct VertexOut {
        @builtin(position) position : vec4f,
        @location(0) color : vec4f
    }

    @binding(0) @group(0) var<uniform> transform: WeirdTransform;

    @vertex
    fn vertex_main(@location(0) position: vec4f,
                   @location(1) color: vec4f) -> VertexOut
    {
        var output : VertexOut;
        output.position = position;
        output.color = color;
        return output;
    }

    @fragment
    fn fragment_main(fragData: VertexOut) -> @location(0) vec4f
    {
        return fragData.color;
    }
    `;

    let canvas = document.getElementById("game");
    canvas.width = canvas.clientWidth;
    canvas.height = canvas.clientHeight;

    const adapter = await navigator.gpu.requestAdapter();
    const device = await adapter.requestDevice();

    const shaderModule = device.createShaderModule({
        code: shaders_src,
    });

    const context = canvas.getContext("webgpu");
    context.configure({
        device: device,
        format: navigator.gpu.getPreferredCanvasFormat(),
        alphaMode: "premultiplied",
    });

    const vertices = new Float32Array([
        0.0, 0.6, 0, 1, // pos
        1, 0, 0, 1,     // col

        -0.5,-0.6, 0, 1,
        0, 1, 0, 1,

        -0.5,-0.6, 0, 1,
        0, 1, 0, 1,

        0.5,-0.6, 0, 1,
        0, 0, 1, 1,

        0.5,-0.6, 0, 1,
        0, 0, 1, 1,

        0.0, 0.6, 0, 1,
        1, 0, 0, 1,
    ]);

    const vertexBuffer = device.createBuffer({
        size: vertices.byteLength,
        usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST,
    });

    device.queue.writeBuffer(vertexBuffer, 0, vertices, 0, vertices.length);

    const vertexBuffers = [
        {
            attributes: [
                {
                    shaderLocation: 0,
                    offset: 0,
                    format: "float32x4",
                },
                {
                    shaderLocation: 1,
                    offset: 16,
                    format: "float32x4",
                },
            ],
            arrayStride: 32,
            stepMode: "vertex",
        },
    ];

    const uniformBuffer = device.createBuffer({
        size: 8,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
    })

    const bindGroupLayout = device.createBindGroupLayout({
        entries: [
            {
                binding: 0,
                visibility: GPUShaderStage.VERTEX,
                buffer: {},
            }
        ],
    });

    const bindGroup = device.createBindGroup({
        layout: bindGroupLayout,
        entries: [
            {
                binding: 0,
                resource: {
                    buffer: uniformBuffer,
                }
            },
        ],
    });

    const pipelineLayout = device.createPipelineLayout({
        bindGroupLayouts: [bindGroupLayout],
    });

    const pipelineDescriptor = {
        vertex: {
            module: shaderModule,
            entryPoint: "vertex_main",
            buffers: vertexBuffers,
        },
        fragment: {
            module: shaderModule,
            entryPoint: "fragment_main",
            targets: [
                {
                    format: navigator.gpu.getPreferredCanvasFormat(),
                }
            ],
        },
        primitive: {
            topology: "line-list",
        },
        layout: pipelineLayout,
    };

    const renderPipeline = device.createRenderPipeline(pipelineDescriptor);

    device.queue.writeBuffer(uniformBuffer, 0, new Float32Array([0.5, -0.5]));

    const commandEncoder = device.createCommandEncoder();
    const clearColor = { r: 0.0, g: 0.0, b: 0.0, a: 1.0 };

    const renderPassDescriptor = {
        colorAttachments: [
            {
                clearValue: clearColor,
                loadOp: "clear",
                storeOp: "store",
                view: context.getCurrentTexture().createView(),
            },
        ],
    };

    const passEncoder = commandEncoder.beginRenderPass(renderPassDescriptor);
    passEncoder.setPipeline(renderPipeline);
    passEncoder.setVertexBuffer(0, vertexBuffer);
    passEncoder.setBindGroup(0, bindGroup);
    passEncoder.draw(6);

    passEncoder.end();
    device.queue.submit([commandEncoder.finish()]);
}

window.onload = init;
)";

extern "C" void create_window(int _width, int _height) {
    std::ofstream js(INDEX_JS);
    std::ofstream html(INDEX_HTML);

    if (!(js.is_open() && html.is_open())) {
        std::cout << "Failed to create index.html and index.js" << std::endl;
        return;
    }

    js << JS_CONTENT << std::endl;
    html << HTML_CONTENT << std::endl;

    js.close();
    html.close();
}
