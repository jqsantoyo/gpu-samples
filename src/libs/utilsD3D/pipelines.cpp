#include "pipelines.h"
#include <utils/utils.h>
using namespace DirectX;
using namespace Microsoft::WRL;


namespace gpu {








bool RootSig::initVoid(Device& device) {
    ComPtr<ID3DBlob>            sig;
    ComPtr<ID3DBlob>            error;
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    GUARDHR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
    GUARDHR(device.obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&obj)));
    return true;
}

bool RootSig::init1Cbv(Device& device) {
    ComPtr<ID3DBlob>            sig;
    ComPtr<ID3DBlob>            error;

    CD3DX12_ROOT_PARAMETER rootParam[1];
    rootParam[0].InitAsConstantBufferView(0);
    
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(1, rootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    GUARDHR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
    GUARDHR(device.obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&obj)));
    return true;
}

bool RootSig::init1Cbv1TableNSamplers(Device& device) {
    ComPtr<ID3DBlob>            sig;
    ComPtr<ID3DBlob>            error;

    CD3DX12_DESCRIPTOR_RANGE descTable;
    descTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_ROOT_PARAMETER rootParam[2];
    rootParam[0].InitAsConstantBufferView(0);
    rootParam[1].InitAsDescriptorTable(1, &descTable);
    
    CD3DX12_STATIC_SAMPLER_DESC samplerDesc(0, D3D12_FILTER_ANISOTROPIC);
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(2, rootParam, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    GUARDHR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
    GUARDHR(device.obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&obj)));
    return true;
}

bool RootSig::init3Cbv1TableNSamplers(Device& device) {
    ComPtr<ID3DBlob>            sig;
    ComPtr<ID3DBlob>            error;

    CD3DX12_DESCRIPTOR_RANGE descTable;
    descTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0);

    CD3DX12_ROOT_PARAMETER rootParam[4];
    rootParam[0].InitAsConstantBufferView(0);
    rootParam[1].InitAsConstantBufferView(1);
    rootParam[2].InitAsConstantBufferView(2);
    rootParam[3].InitAsDescriptorTable(1, &descTable);
    
    CD3DX12_STATIC_SAMPLER_DESC samplerAniso(0, D3D12_FILTER_ANISOTROPIC);
    D3D12_STATIC_SAMPLER_DESC samplerShadow = {
        .Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .MipLODBias = 0.0f,
        .MaxAnisotropy = 16,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
        .MinLOD = 0.f,
        .MaxLOD = D3D12_FLOAT32_MAX,
        .ShaderRegister = 1,
        .RegisterSpace = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
    };
    D3D12_STATIC_SAMPLER_DESC samplers[] = { samplerAniso, samplerShadow };

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(4, rootParam, 2, samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    GUARDHR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
    GUARDHR(device.obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&obj)));
    return true;
}




bool PipelineBasic::init(Device& device, RootSig& sig) {
    Shader vShader;
    Shader pShader;
    GUARD(vShader.load("shaders_v"));
    GUARD(pShader.load("shaders_p"));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature         = sig.obj.Get(),
        .VS                     = vShader.bytecode,
        .PS                     = pShader.bytecode,
        .BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask             = UINT_MAX,
        .RasterizerState        = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState      = { .DepthEnable = FALSE, .StencilEnable = FALSE, },
        .InputLayout            = { inputElementDescs, _countof(inputElementDescs) },
        .PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets       = 1,
        .RTVFormats             = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .SampleDesc             = { .Count = 1},
    };
    GUARDHR(device.obj->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&obj)));
    return true;
}



bool PipelineFill::init(Device& device, RootSig& sig) {
    Shader vShader;
    Shader pShader;
    GUARD(vShader.load("shaders_v"));
    GUARD(pShader.load("shaders_p")); 

    // D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels = {
    //     .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
    //     .SampleCount = 4,
    //     .Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
    //     .NumQualityLevels = 0,
    // };
    // GUARDHR(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels)));
    // GUARD((qualityLevels.NumQualityLevels > 0));
    // DXGI_SAMPLE_DESC sampleDesc = { .Count = 4, .Quality = qualityLevels.NumQualityLevels - 1 }; 
    DXGI_SAMPLE_DESC sampleDesc = { .Count = 1, .Quality = 0 };

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 1,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    D3D12_RASTERIZER_DESC fillRasterDesc = {
        .FillMode               = D3D12_FILL_MODE_SOLID,
        .CullMode               = D3D12_CULL_MODE_NONE,
        // .CullMode               = D3D12_CULL_MODE_BACK,
        .FrontCounterClockwise  = FALSE,
        .DepthBias              = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp         = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias   = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable        = TRUE,
        .MultisampleEnable      = FALSE,
        .AntialiasedLineEnable  = FALSE,
        .ForcedSampleCount      = 0,
        .ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };
    D3D12_DEPTH_STENCIL_DESC depthStateDesc = {
        .DepthEnable        = TRUE,
        .DepthWriteMask     = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc          = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .StencilEnable      = FALSE,
        // .StencilReadMask    = 0,
        // .StencilWriteMask   = 0,
        // .FrontFace = {
        //     .StencilFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilPassOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilFunc = D3D12_COMPARISON_FUNC_GREATER,
        // },
        // .BackFace = {
        //     .StencilFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilPassOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilFunc = D3D12_COMPARISON_FUNC_GREATER,
        // },

    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoFillDesc = {
        .pRootSignature         = sig.obj.Get(),
        .VS                     = vShader.bytecode,
        .PS                     = pShader.bytecode,
        .BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask             = UINT_MAX,
        .RasterizerState        = fillRasterDesc,
        .DepthStencilState      = depthStateDesc,
        .InputLayout            = { inputElementDescs, _countof(inputElementDescs) },
        .PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets       = 1,
        .RTVFormats             = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .DSVFormat              = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc             = sampleDesc,
    };
    GUARDHR(device.obj->CreateGraphicsPipelineState(&psoFillDesc, IID_PPV_ARGS(&obj)));
    return true;
}



bool PipelineWire::init(Device& device, RootSig& sig) {
    Shader vShaderWire;
    Shader pShaderWire;
    GUARD(vShaderWire.load("shadersWire_v"));
    GUARD(pShaderWire.load("shadersWire_p"));

    DXGI_SAMPLE_DESC sampleDesc = { .Count = 1, .Quality = 0 };

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_RASTERIZER_DESC wireRasterDesc = {
        .FillMode               = D3D12_FILL_MODE_WIREFRAME,
        .CullMode               = D3D12_CULL_MODE_NONE,
        .FrontCounterClockwise  = FALSE,
        .DepthBias              = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp         = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias   = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable        = TRUE,
        .MultisampleEnable      = FALSE,
        .AntialiasedLineEnable  = FALSE,
        .ForcedSampleCount      = 0,
        .ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };
    D3D12_DEPTH_STENCIL_DESC depthStateDesc = {
        .DepthEnable        = TRUE,
        .DepthWriteMask     = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc          = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .StencilEnable      = FALSE,
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoWireDesc = {
        .pRootSignature         = sig.obj.Get(),
        .VS                     = vShaderWire.bytecode,
        .PS                     = pShaderWire.bytecode,
        .BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask             = UINT_MAX,
        .RasterizerState        = wireRasterDesc,
        .DepthStencilState      = depthStateDesc,
        .InputLayout            = { inputElementDescs, _countof(inputElementDescs) },
        .PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets       = 1,
        .RTVFormats             = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .DSVFormat              = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc             = sampleDesc,
    };
    GUARDHR(device.obj->CreateGraphicsPipelineState(&psoWireDesc, IID_PPV_ARGS(&obj)));
    return true;
};



bool PipelineTex::init(Device& device, RootSig& sig) {
    Shader vShader;
    Shader pShader;
    GUARD(vShader.load("shaders_v"));
    GUARD(pShader.load("shaders_p"));

    DXGI_SAMPLE_DESC sampleDesc = { .Count = 1, .Quality = 0 };

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "UV",    0, DXGI_FORMAT_R32G32_FLOAT, 1,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    D3D12_RASTERIZER_DESC wireRasterDesc = {
        .FillMode               = D3D12_FILL_MODE_SOLID,
        .CullMode               = D3D12_CULL_MODE_NONE,
        .FrontCounterClockwise  = FALSE,
        .DepthBias              = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp         = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias   = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable        = TRUE,
        .MultisampleEnable      = FALSE,
        .AntialiasedLineEnable  = FALSE,
        .ForcedSampleCount      = 0,
        .ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };
    D3D12_DEPTH_STENCIL_DESC depthStateDesc = {
        .DepthEnable        = TRUE,
        .DepthWriteMask     = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc          = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .StencilEnable      = FALSE,
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoWireDesc = {
        .pRootSignature         = sig.obj.Get(),
        .VS                     = vShader.bytecode,
        .PS                     = pShader.bytecode,
        .BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask             = UINT_MAX,
        .RasterizerState        = wireRasterDesc,
        .DepthStencilState      = depthStateDesc,
        .InputLayout            = { inputElementDescs, _countof(inputElementDescs) },
        .PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets       = 1,
        .RTVFormats             = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .DSVFormat              = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc             = sampleDesc,
    };
    GUARDHR(device.obj->CreateGraphicsPipelineState(&psoWireDesc, IID_PPV_ARGS(&obj)));
    return true;
};

bool PipelineLights::init(Device& device, RootSig& sig) {
    Shader vShader;
    Shader pShader;
    GUARD(vShader.load("shaders_v"));
    GUARD(pShader.load("shaders_p"));

    DXGI_SAMPLE_DESC sampleDesc = { .Count = 1, .Quality = 0 };

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    1,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "UV",       0, DXGI_FORMAT_R32G32_FLOAT,       2,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_RASTERIZER_DESC wireRasterDesc = {
        .FillMode               = D3D12_FILL_MODE_SOLID,
        .CullMode               = D3D12_CULL_MODE_NONE,
        .FrontCounterClockwise  = FALSE,
        .DepthBias              = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp         = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias   = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable        = TRUE,
        .MultisampleEnable      = FALSE,
        .AntialiasedLineEnable  = FALSE,
        .ForcedSampleCount      = 0,
        .ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };
    D3D12_DEPTH_STENCIL_DESC depthStateDesc = {
        .DepthEnable            = TRUE,
        .DepthWriteMask         = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc              = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .StencilEnable          = FALSE,
    };
    D3D12_BLEND_DESC blendDesc = {
        .AlphaToCoverageEnable  = FALSE,
        .IndependentBlendEnable = FALSE,
        .RenderTarget = {
            {
                .BlendEnable            = TRUE,
                .LogicOpEnable          = FALSE,
                .SrcBlend               = D3D12_BLEND_SRC_ALPHA,
                .DestBlend              = D3D12_BLEND_INV_SRC_ALPHA,
                .BlendOp                = D3D12_BLEND_OP_ADD,
                .SrcBlendAlpha          = D3D12_BLEND_ONE,
                .DestBlendAlpha         = D3D12_BLEND_INV_SRC_ALPHA,
                .BlendOpAlpha           = D3D12_BLEND_OP_ADD,
                .RenderTargetWriteMask  = D3D12_COLOR_WRITE_ENABLE_ALL,
            },
        }
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoWireDesc = {
        .pRootSignature         = sig.obj.Get(),
        .VS                     = vShader.bytecode,
        .PS                     = pShader.bytecode,
        .BlendState             = blendDesc,
        .SampleMask             = UINT_MAX,
        .RasterizerState        = wireRasterDesc,
        .DepthStencilState      = depthStateDesc,
        .InputLayout            = { inputElementDescs, _countof(inputElementDescs) },
        .PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets       = 1,
        .RTVFormats             = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .DSVFormat              = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc             = sampleDesc,
    };
    GUARDHR(device.obj->CreateGraphicsPipelineState(&psoWireDesc, IID_PPV_ARGS(&obj)));
    return true;
};






bool DepthBuffer::init(Device* device, uint64_t width, uint32_t height) {
    this->device = device;
    DXGI_SAMPLE_DESC sampleDesc = { .Count = 1, .Quality = 0 };
    D3D12_RESOURCE_DESC depthDesc = {
        .Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment          = 0,
        .Width              = width,
        .Height             = height,
        .DepthOrArraySize   = 1,
        .MipLevels          = 1,
        .Format             = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc         = sampleDesc,
        .Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
    };
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_CLEAR_VALUE optClear = {
        .Format = DXGI_FORMAT_D32_FLOAT,
        .DepthStencil = { .Depth = 1, .Stencil = 0 },
    };
    GUARDHR(device->obj->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optClear,
        IID_PPV_ARGS(obj.GetAddressOf())
    ));

    GUARD(device->dsvHeap.next(dsvIdx));
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = device->dsvHeap.getCpu(dsvIdx);

    D3D12_DEPTH_STENCIL_VIEW_DESC depthViewDesc = {
        .Format         = DXGI_FORMAT_D32_FLOAT,
        .ViewDimension  = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags          = D3D12_DSV_FLAG_NONE,
        .Texture2D      = { .MipSlice = 0 },
    };
    device->obj->CreateDepthStencilView(obj.Get(), &depthViewDesc, dsvDescriptor);
    return true;
}


bool Shadow::init(Device* device, ID3D12RootSignature* root, uint32_t width, uint32_t height) {
    this->device = device;
    this->width = width;
    this->height = height;

    DXGI_SAMPLE_DESC sampleDesc = { .Count = 1, .Quality = 0 };
    Shader vShader;
    Shader pShader;
    GUARD(vShader.load("shadow_v"));
    GUARD(pShader.load("shadow_p"));


    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_RASTERIZER_DESC wireRasterDesc = {
        .FillMode               = D3D12_FILL_MODE_SOLID,
        .CullMode               = D3D12_CULL_MODE_NONE,
        .FrontCounterClockwise  = FALSE,
        .DepthBias              = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp         = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias   = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable        = TRUE,
        .MultisampleEnable      = FALSE,
        .AntialiasedLineEnable  = FALSE,
        .ForcedSampleCount      = 0,
        .ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };
    D3D12_DEPTH_STENCIL_DESC depthStateDesc = {
        .DepthEnable            = TRUE,
        .DepthWriteMask         = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc              = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .StencilEnable          = FALSE,
    };
    D3D12_BLEND_DESC blendDesc = {
        .AlphaToCoverageEnable  = FALSE,
        .IndependentBlendEnable = FALSE,
        .RenderTarget = {
            {
                .BlendEnable            = TRUE,
                .LogicOpEnable          = FALSE,
                .SrcBlend               = D3D12_BLEND_SRC_ALPHA,
                .DestBlend              = D3D12_BLEND_INV_SRC_ALPHA,
                .BlendOp                = D3D12_BLEND_OP_ADD,
                .SrcBlendAlpha          = D3D12_BLEND_ONE,
                .DestBlendAlpha         = D3D12_BLEND_INV_SRC_ALPHA,
                .BlendOpAlpha           = D3D12_BLEND_OP_ADD,
                .RenderTargetWriteMask  = D3D12_COLOR_WRITE_ENABLE_ALL,
            },
        }
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature         = root,
        .VS                     = vShader.bytecode,
        .PS                     = pShader.bytecode,
        .BlendState             = blendDesc,
        .SampleMask             = UINT_MAX,
        .RasterizerState        = wireRasterDesc,
        .DepthStencilState      = depthStateDesc,
        .InputLayout            = { inputElementDescs, _countof(inputElementDescs) },
        .PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets       = 1,
        .RTVFormats             = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .DSVFormat              = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .SampleDesc             = sampleDesc,
    };
    GUARDHR(device->obj->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

    D3D12_RESOURCE_DESC desc = {
        .Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment          = 0,
        .Width              = width,
        .Height             = height,
        .DepthOrArraySize   = 1,
        .MipLevels          = 1,
        .Format             = DXGI_FORMAT_R24G8_TYPELESS,
        .SampleDesc         = sampleDesc,
        .Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
    };
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_CLEAR_VALUE optClear = {
        .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .DepthStencil = { .Depth = 1, .Stencil = 0 },
    };
    GUARDHR(device->obj->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(target.GetAddressOf())
    ));

    GUARD(device->cbvHeap.next(srvIdx));
    GUARD(device->dsvHeap.next(dsvIdx));
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor = device->cbvHeap.getCpu(srvIdx);
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = device->dsvHeap.getCpu(dsvIdx);
    
    
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
        .Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = 0,
            .MipLevels = 1,
            .PlaneSlice = 0,
            .ResourceMinLODClamp = 0.0f,
        }
    };
    device->obj->CreateShaderResourceView(target.Get(), &srvDesc, srvDescriptor);

    D3D12_DEPTH_STENCIL_VIEW_DESC depthViewDesc = {
        .Format         = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .ViewDimension  = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags          = D3D12_DSV_FLAG_NONE,
        .Texture2D      = { .MipSlice = 0 },
    };
    device->obj->CreateDepthStencilView(target.Get(), &depthViewDesc, dsvDescriptor);
    return true;
}

}